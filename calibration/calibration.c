/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <math.h>

#include <chimaera.h>
#include <debug.h>
#include <eeprom.h>
#include <linalg.h>
#include "../cmc/cmc_private.h"

#include "calibration_private.h"

// globals
Calibration range;
uint_fast8_t zeroing = 0;
uint_fast8_t calibrating = 0;
float curve [0x800];

// when calibrating, we use the curve buffer as temporary memory, it's big enough and not used during calibration
static Calibration_Array *arr =(Calibration_Array *)curve;
static Calibration_Point point;

static float
_as(uint16_t qui, uint16_t out_s, uint16_t out_n, uint16_t b)
{
	float _qui =(float)qui;
	float _out_s =(float)out_s;
	float _out_n =(float)out_n;
	float _b =(float)b;

	return _qui / _b *(_out_s - _out_n) /(_out_s + _out_n);
}

uint_fast8_t
range_load(uint_fast8_t pos)
{
	/* TODO
	if(version_match()) // EEPROM and FLASH config versions match
		eeprom_bulk_read(eeprom_24LC64, EEPROM_RANGE_OFFSET + pos*EEPROM_RANGE_SIZE,(uint8_t *)&range, sizeof(range));
	else // EEPROM and FLASH config version do not match, overwrite old with new default one
	{
		range_reset();
		range_save(pos);
	}
	*/
	eeprom_bulk_read(eeprom_24LC64, EEPROM_RANGE_OFFSET + pos*EEPROM_RANGE_SIZE,(uint8_t *)&range, sizeof(range));
	
	range_curve_update();

	return 1;
}

uint_fast8_t
range_reset(void)
{
	uint_fast8_t i;
	for(i=0; i<SENSOR_N; i++)
	{
		range.thresh[i] = 0;
		range.qui[i] = ADC_HALF_BITDEPTH;
		range.U[i] = 1.f;
	}
	
	range.W = 0.f;
	range.C[0] = 0.f; // ~ cbrtf(x)
	range.C[1] = 1.f; // ~ sqrtf(x)
	range.C[2] = 0.f; // ~ x

	return 1;
}

uint_fast8_t
range_save(uint_fast8_t pos)
{
	eeprom_bulk_write(eeprom_24LC64, EEPROM_RANGE_OFFSET + pos*EEPROM_RANGE_SIZE,(uint8_t *)&range, sizeof(range));

	return 1;
}

void
range_curve_update(void)
{
	uint32_t i;

	for(i=0; i<0x800; i++)
	{
		float x =(float)i /(float)0x7ff;
		float y = range.C[0]*cbrtf(x) + range.C[1]*sqrtf(x) + range.C[2]*x;
		y = y < 0.f ? 0.f :(y > 1.f ? 1.f : y);
		curve[i] = y;
	}
}

void
range_calibrate(int16_t *raw12, int16_t *raw3, uint8_t *order12, uint8_t *order3, int16_t *sum, int16_t *rela)
{
	(void)sum;
	uint_fast8_t i;
	uint_fast8_t pos;
	
	// fill rela vector from raw vector
#if(ADC_DUAL_LENGTH > 0)
	for(i=0; i<MUX_MAX*ADC_DUAL_LENGTH*2; i++)
	{
		pos = order12[i];
		rela[pos] = raw12[i];
	}
#else
	(void)raw12;
	(void)order12;
#endif

#if(ADC_SING_LENGTH > 0)
	for(i=0; i<MUX_MAX*ADC_SING_LENGTH; i++)
	{
		pos = order3[i];
		rela[pos] = raw3[i];
	}
#else
	(void)raw3;
	(void)order3;
#endif

	// do the calibration
	for(i=0; i<SENSOR_N; i++)
	{
		uint16_t avg;

		if(zeroing)
		{
			// moving average over 16 samples
			range.qui[i] -= range.qui[i] >> 4;
			range.qui[i] += rela[i];
		}

		// XXX is this the best way to get a mean of min and max?
		if(rela[i] >(avg = arr->arr[POLE_SOUTH][i] >> 4) )
		{
			arr->arr[POLE_SOUTH][i] -= avg;
			arr->arr[POLE_SOUTH][i] += rela[i];
		}

		if(rela[i] <(avg =arr->arr[POLE_NORTH][i] >> 4) )
		{
			arr->arr[POLE_NORTH][i] -= avg;
			arr->arr[POLE_NORTH][i] += rela[i];
		}
	}
}

// initialize sensor range
void
range_init(void)
{
	uint_fast8_t i;
	for(i=0; i<SENSOR_N; i++)
	{
		// moving average over 16 samples
		range.qui[i] = ADC_HALF_BITDEPTH << 4;
		
		arr->arr[POLE_SOUTH][i] = range.qui[i];
		arr->arr[POLE_NORTH][i] = range.qui[i];
	}

	point.state = 1;
}

// calibrate quiescent current
static void
range_update_quiescent(void)
{
	uint_fast8_t i;
	for(i=0; i<SENSOR_N; i++)
	{
		// final average over last 16 samples
		range.qui[i] >>= 4;

		// reset array to quiescent value
		arr->arr[POLE_SOUTH][i] = range.qui[i] << 4;
		arr->arr[POLE_NORTH][i] = range.qui[i] << 4;
	}
}

// calibrate threshold
static uint_fast8_t
range_update_b0(void)
{
	uint_fast8_t i;
	uint16_t thresh_s, thresh_n;

	for(i=0; i<SENSOR_N; i++)
	{
		arr->arr[POLE_SOUTH][i] >>= 4; // average over 16 samples
		arr->arr[POLE_NORTH][i] >>= 4;

		thresh_s = arr->arr[POLE_SOUTH][i] - range.qui[i];
		thresh_n = range.qui[i] - arr->arr[POLE_NORTH][i];

		range.thresh[i] =(thresh_s + thresh_n) / 2;

		// reset thresh to quiescent value
		arr->arr[POLE_SOUTH][i] = range.qui[i] << 4;
		arr->arr[POLE_NORTH][i] = range.qui[i] << 4;
	}

	// search for smallest value
	point.B0 = 0xfff;
	for(i=0; i<SENSOR_N; i++)
		if(range.thresh[i] < point.B0)
		{
			point.i = i; // that'll be the sensor we will fit the distance-magnetic flux relationship curve to
			point.B0 = range.thresh[i];
		}

	return point.i;
}

// calibrate distance-magnetic flux relationship curve
static uint_fast8_t
range_update_b1(float y)
{
	uint_fast8_t i;
	uint16_t b_s, b_n;
	uint_fast8_t ret = 0;

	i = point.i;
	{
		arr->arr[POLE_SOUTH][i] >>= 4; // average over 16 samples
		arr->arr[POLE_NORTH][i] >>= 4;

		b_s = arr->arr[POLE_SOUTH][i] - range.qui[i];
		b_n = range.qui[i] - arr->arr[POLE_NORTH][i];

		switch(point.state)
		{
			case 1:
				if(y <= 0.f) // check for increasing y
					goto exit;
				point.y1 = y;
				point.B1 =(b_s + b_n) / 2.f;
				point.state++;
				break;
			case 2:
				if(y <= point.y1) // check for increasing y
					goto exit;
				point.y2 = y;
				point.B2 =(b_s + b_n) / 2.f;
				point.state++;
				break;
			case 3:
				if(y <= point.y2) // check for increasing y
					goto exit;
				point.y3 = y;
				point.B3 =(b_s + b_n) / 2.f;
				point.state++;
				break;
		}
	}

	ret = 1;

exit:

	for(i=0; i<SENSOR_N; i++)
	{
		// reset arr to quiescent values
		arr->arr[POLE_SOUTH][i] = range.qui[i] << 4;
		arr->arr[POLE_NORTH][i] = range.qui[i] << 4;
	}

	return ret;
}

// calibrate amplification and sensitivity
static uint_fast8_t
range_update_b2(void)
{
	uint_fast8_t i = point.i;
	uint16_t b_s, b_n;
	uint_fast8_t ret = 0;

	if(point.state == 4)
	{
		arr->arr[POLE_SOUTH][i] >>= 4; // average over 16 samples
		arr->arr[POLE_NORTH][i] >>= 4;

		b_s = arr->arr[POLE_SOUTH][i] - range.qui[i];
		b_n = range.qui[i] - arr->arr[POLE_NORTH][i];
		
		float Br =(b_s + b_n)/2.f - point.B0;

		point.B1 =(point.B1 - point.B0) / Br;
		point.B2 =(point.B2 - point.B0) / Br;
		point.B3 =(point.B3 - point.B0) / Br;

		double C [3];
		//DEBUG("ffffff", point.B1, point.y1, point.B2, point.y2, point.B3, point.y3);
		//linalg_least_squares_quadratic(point.B1, point.y1, point.B2, point.y2, point.B3, point.y3, &C[0], &C[1]);
		//DEBUG("dd", C[0], C[1]);
		linalg_least_squares_cubic(point.B1, point.y1, point.B2, point.y2, point.B3, point.y3, &C[0], &C[1], &C[2]);
		//DEBUG("ddd", C[0], C[1], C[2]);
		range.C[0] =(float)C[0];
		range.C[1] =(float)C[1];
		range.C[2] =(float)C[2];

		ret = 1;
	}

	for(i=0; i<SENSOR_N; i++)
	{
		// reset thresh to quiescent value
		arr->arr[POLE_SOUTH][i] = range.qui[i] << 4;
		arr->arr[POLE_NORTH][i] = range.qui[i] << 4;
	}

	return ret;
}

// calibrate amplification and sensitivity
static void
range_update_b3(float y)
{
	uint_fast8_t i;
	float as_1;
	float bmin, bmax_s, bmax_n;
	float sc_1;

	// find magnetic flux b corresponding to distance y
	uint16_t b;
	float bf;
	for(b=0; b<0x800; b++)
	{
		bf =(float)b /(float)0x7ff;
		float _y = range.C[0]*cbrtf(bf) + range.C[1]*sqrtf(bf) + range.C[2]*bf;
		if(_y > y)
			break;
	}

	float m_bmin = 0.f;
	float m_bmax = 0.f;

	for(i=0; i<SENSOR_N; i++)
	{
		arr->arr[POLE_SOUTH][i] >>= 4; // average over 16 samples
		arr->arr[POLE_NORTH][i] >>= 4;

		as_1 = 1.f / _as(range.qui[i], arr->arr[POLE_SOUTH][i], arr->arr[POLE_NORTH][i], b);

		bmin =(float)range.thresh[i] * as_1;
		bmax_s =((float)arr->arr[POLE_SOUTH][i] -(float)range.qui[i]) * as_1 / bf;
		bmax_n =((float)range.qui[i] -(float)arr->arr[POLE_NORTH][i]) * as_1 / bf;

		m_bmin += bmin;
		m_bmax +=(bmax_s + bmax_n) / 2.f;
	}

	m_bmin /=(float)SENSOR_N;
	m_bmax /=(float)SENSOR_N;

	sc_1 = 1.f /(m_bmax - m_bmin);
	range.W = m_bmin * sc_1;

	for(i=0; i<SENSOR_N; i++)
	{
		as_1 = 1.f / _as(range.qui[i], arr->arr[POLE_SOUTH][i], arr->arr[POLE_NORTH][i], b);
		range.U[i] = as_1 * sc_1;

		// reset thresh to quiescent value
		arr->arr[POLE_SOUTH][i] = range.qui[i] << 4;
		arr->arr[POLE_NORTH][i] = range.qui[i] << 4;
	}

	// update curve lookup table
	range_curve_update();
}

/*
 * Config
 */

static uint_fast8_t
_calibration_start(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	range_init();

	// enable calibration
	zeroing = 1;
	calibrating = 1;

	size = CONFIG_SUCCESS("is", uuid, path);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_zero(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	if(calibrating)
	{
		// update new range
		zeroing = 0;
		range_update_quiescent();
		size = CONFIG_SUCCESS("is", uuid, path);
	}
	else
		size = CONFIG_FAIL("iss", uuid, path, "not in calibration mode");

	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_min(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	if(calibrating)
	{
		// update new range
		int32_t si = range_update_b0();
		size = CONFIG_SUCCESS("isi", uuid, path, si);
	}
	else
		size = CONFIG_FAIL("iss", uuid, path, "not in calibration mode");

	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_mid(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	if(calibrating)
	{
		float y;
		buf_ptr = osc_get_float(buf_ptr, &y);

		// update mid range
		if(range_update_b1(y))
			size = CONFIG_SUCCESS("is", uuid, path);
		else
			size = CONFIG_FAIL("iss", uuid, path, "vicinity must be increasing for five-point curve-fit");
	}
	else
		size = CONFIG_FAIL("iss", uuid, path, "not in calibration mode");
		
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_max(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	if(calibrating)
	{
		// update max range
		if(range_update_b2())
			size = CONFIG_SUCCESS("is", uuid, path);
		else
			size = CONFIG_FAIL("iss", uuid, path, "not all points given for five-point curve-fit");
	}
	else
		size = CONFIG_FAIL("iss", uuid, path, "not in calibration mode");

	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_end(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);
	
	if(calibrating)
	{
		float y;
		buf_ptr = osc_get_float(buf_ptr, &y);

		// update max range
		range_update_b3(y);

		// end calibration procedure
		calibrating = 0;

		size = CONFIG_SUCCESS("is", uuid, path);
	}
	else
		size = CONFIG_FAIL("iss", uuid, path, "not in calibration mode");
		
	CONFIG_SEND(size);

	return 1;
}

// get calibration data per sensor
static uint_fast8_t
_calibration_quiescent(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	uint16_t *qui = NULL;
	size = CONFIG_SUCCESS("isB", uuid, path, sizeof(range.qui), &qui);

	// hton
	uint_fast8_t i;
	for(i=0; i<SENSOR_N; i++)
		qui[i] = hton(range.qui[i]);

	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_threshold(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	uint16_t *thresh = NULL;
	size = CONFIG_SUCCESS("isB", uuid, path, sizeof(range.thresh), &thresh);

	// hton
	uint_fast8_t i;
	for(i=0; i<SENSOR_N; i++)
		thresh[i] = hton(range.thresh[i]);

	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_amplification(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	uint32_t *U = NULL;
	size = CONFIG_SUCCESS("isB", uuid, path, sizeof(range.U), &U);

	// htonl
	uint_fast8_t i;
	uint32_t *_U = (uint32_t *)range.U;
	for(i=0; i<SENSOR_N; i++)
	{
		U[i] = htonl(_U[i]);
	}

	CONFIG_SEND(size);

	return 1;
}

// get calibration data per array
static uint_fast8_t
_calibration_offset(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	size = CONFIG_SUCCESS("isf", uuid, path, range.W);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_curve_c0(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	size = CONFIG_SUCCESS("isf", uuid, path, range.C[0]);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_curve_c1(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	size = CONFIG_SUCCESS("isf", uuid, path, range.C[1]);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_curve_c2(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	size = CONFIG_SUCCESS("isf", uuid, path, range.C[2]);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_save(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;
	int32_t pos;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);
	buf_ptr = osc_get_int32(buf_ptr, &pos);

	range_save(pos);
	size = CONFIG_SUCCESS("is", uuid, path);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_load(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;
	int32_t pos;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);
	buf_ptr = osc_get_int32(buf_ptr, &pos);

	range_load(pos);
	size = CONFIG_SUCCESS("is", uuid, path);
	CONFIG_SEND(size);

	return 1;
}

static uint_fast8_t
_calibration_reset(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	(void)fmt;
	(void)argc;
	osc_data_t *buf_ptr = buf;
	uint16_t size;
	int32_t uuid;

	buf_ptr = osc_get_int32(buf_ptr, &uuid);

	// reset calibration range
	range_reset();

	size = CONFIG_SUCCESS("is", uuid, path);
	CONFIG_SEND(size);

	return 1;
}

/*
 * Query
 */

static const OSC_Query_Argument calibration_load_args [] = {
	OSC_QUERY_ARGUMENT_INT32("Slot", OSC_QUERY_MODE_W, 0, EEPROM_RANGE_MAX, 1)
};

static const OSC_Query_Argument calibration_save_args [] = {
	OSC_QUERY_ARGUMENT_INT32("Slot", OSC_QUERY_MODE_W, 0, EEPROM_RANGE_MAX, 1)
};

static const OSC_Query_Argument calibration_min_args [] = {
	OSC_QUERY_ARGUMENT_INT32("Sensor", OSC_QUERY_MODE_R, 0, SENSOR_N - 1, 1)
};

static const OSC_Query_Argument calibration_mid_args [] = {
	OSC_QUERY_ARGUMENT_FLOAT("Relative vicinity", OSC_QUERY_MODE_W, 0.f, 1.f, 0.01)
};

static const OSC_Query_Argument calibration_end_args [] = {
	OSC_QUERY_ARGUMENT_FLOAT("Relative vicinity", OSC_QUERY_MODE_W, 0.f, 1.f, 0.01)
};
	
static const OSC_Query_Argument calibration_blob_args [] = {
	OSC_QUERY_ARGUMENT_BLOB("Blob", OSC_QUERY_MODE_R)
};

static const OSC_Query_Argument calibration_offset_args [] = {
	OSC_QUERY_ARGUMENT_FLOAT("W", OSC_QUERY_MODE_R, -100.f, 100.f, 0.f)
};

static const OSC_Query_Argument calibration_curve_args [] = {
	OSC_QUERY_ARGUMENT_FLOAT("ci", OSC_QUERY_MODE_R, -100.f, 100.f, 0.f)
};

const OSC_Query_Item calibration_tree [] = {
	OSC_QUERY_ITEM_METHOD("load", "Load calibration from EEPROM", _calibration_load, calibration_load_args),
	OSC_QUERY_ITEM_METHOD("save", "Save calibration to EEPROM", _calibration_save, calibration_save_args),
	OSC_QUERY_ITEM_METHOD("reset", "Reset calibration to factory settings", _calibration_reset, NULL),

	OSC_QUERY_ITEM_METHOD("start", "Start calibration procedure", _calibration_start, NULL),
	OSC_QUERY_ITEM_METHOD("zero", "Calibrate quiescent values", _calibration_zero, NULL),
	OSC_QUERY_ITEM_METHOD("min", "Calibrate threshold values / curve fit point 1", _calibration_min, calibration_min_args),
	OSC_QUERY_ITEM_METHOD("mid", "Curve fit points 2-4", _calibration_mid, calibration_mid_args),
	OSC_QUERY_ITEM_METHOD("max", "Curve fit point 5", _calibration_max, NULL),
	OSC_QUERY_ITEM_METHOD("end", "End calibration procedure", _calibration_end, calibration_end_args),

	OSC_QUERY_ITEM_METHOD("quiescent", "Query calibration quiescent data", _calibration_quiescent, calibration_blob_args),
	OSC_QUERY_ITEM_METHOD("threshold", "Query calibration threshold data", _calibration_threshold, calibration_blob_args),
	OSC_QUERY_ITEM_METHOD("U", "Query calibration amplification data", _calibration_amplification, calibration_blob_args),

	OSC_QUERY_ITEM_METHOD("W", "Query array calibration offset data", _calibration_offset, calibration_offset_args),
	OSC_QUERY_ITEM_METHOD("c0", "Query curve-fit parameter 0", _calibration_curve_c0, calibration_curve_args),
	OSC_QUERY_ITEM_METHOD("c1", "Query curve-fit parameter 1", _calibration_curve_c1, calibration_curve_args),
	OSC_QUERY_ITEM_METHOD("c2", "Query curve-fit parameter 2", _calibration_curve_c2, calibration_curve_args)
};
