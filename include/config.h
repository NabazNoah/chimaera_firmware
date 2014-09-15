/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#include <cmc.h>
#include <chimaera.h>

#include <scsynth.h>
#include <custom.h>

#define SRC_PORT 0
#define DST_PORT 1
#define NAME_LENGTH 32

typedef void (*Socket_Enable_Cb)(uint8_t flag);
typedef struct _Firmware_Version Firmware_Version;
typedef struct _Socket_Config Socket_Config;
typedef struct _OSC_Config OSC_Config;
typedef struct _Config Config;
typedef enum _OSC_Mode OSC_Mode;

struct _Firmware_Version {
	uint8_t revision;	// board layout revision
	uint8_t major;		// major version
	uint8_t minor;		// minor version
	uint8_t patch;		// patch level
};

struct _Socket_Config {
	uint8_t sock;
	uint8_t enabled;
	uint16_t port[2]; // SRC_PORT, DST_PORT
	uint8_t ip[4];
};

enum _OSC_Mode {
	OSC_MODE_UDP	= 0,
	OSC_MODE_TCP	= 1,
	OSC_MODE_SLIP	= 2
};

extern const OSC_Query_Value config_mode_args_values [3];

struct _OSC_Config {
	Socket_Config socket;
	uint8_t mode;
	uint8_t server;
};

enum {
	SOCK_ARP		= 0,
	SOCK_DHCPC	= 0,
	SOCK_SNTP		= 1,
	SOCK_PTP_EV = 2,
	SOCK_PTP_GE = 3,
	SOCK_OUTPUT	= 4,
	SOCK_CONFIG = 5,
	SOCK_DEBUG	= 6,
	SOCK_MDNS		= 7,
};

struct _Config {
	/*
	 * read-only
	 */
 	Firmware_Version version;

	/*
	 * read-write
	 */
	char name[NAME_LENGTH];

	struct _comm {
		uint8_t custom_mac; // locally administered MAC flag
		uint8_t mac [6];
		uint8_t ip [4];
		uint8_t gateway [4];
		uint8_t subnet [4];
	} comm;

	struct _dump {
		uint8_t enabled;
	} dump;

	struct _tuio2 {
		uint8_t enabled;
		uint8_t long_header;
	} tuio2;

	struct _tuio1 {
		uint8_t enabled;
		uint8_t custom_profile;
	} tuio1;

	struct _scsynth {
		uint8_t enabled;
	} scsynth;

	struct _oscmidi {
		uint8_t enabled;
		uint8_t effect;
		float offset;
		float range;
		float mul;
	} oscmidi;

	struct _dummy {
		uint8_t enabled;
	} dummy;

	struct _custom {
		uint8_t enabled;
		Custom_Item items [CUSTOM_MAX_EXPR];
	} custom;

	struct _output {
		OSC_Config osc;
		OSC_Timetag offset;
		struct {
			uint8_t x;
			uint8_t z;
		} invert;
		uint8_t parallel;
	} output;

	struct _config {
		OSC_Config osc;
	} config;

	struct _ptp {
		uint8_t multiplier;
		uint8_t offset_stiffness;
		uint8_t delay_stiffness;
		Socket_Config event;
		Socket_Config general;
	} ptp;

	struct _sntp {
		uint8_t tau;
		Socket_Config socket;
	} sntp;

	struct _debug {
		OSC_Config osc;
	} debug;

	struct _ipv4ll {
		uint8_t enabled;
	} ipv4ll;

	struct _mdns {
		Socket_Config socket;
	} mdns;

	struct _dhcpc {
		Socket_Config socket;
	} dhcpc;

	struct _sensors {
		uint8_t movingaverage_bitshift;
		uint8_t interpolation_mode;
		uint16_t rate; // the maximal update rate the chimaera should run at
	} sensors;

	CMC_Group groups [GROUP_MAX];
	SCSynth_Group scsynth_groups [GROUP_MAX];
};

extern Config config;
extern const OSC_Method config_serv [];

uint_fast8_t version_match(void);

uint_fast8_t config_load(void);
uint_fast8_t config_save(void);

uint_fast8_t groups_load(void);
uint_fast8_t groups_save(void);

uint16_t CONFIG_SUCCESS(const char *fmt, ...);
uint16_t CONFIG_FAIL(const char *fmt, ...);
void CONFIG_SEND(uint16_t size);

uint_fast8_t config_socket_enabled(Socket_Config *socket, const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf);
uint_fast8_t config_address(Socket_Config *socket, const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf);
uint_fast8_t config_check_uint8(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf, uint8_t *val);
uint_fast8_t config_check_bool(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf, uint8_t *boolean);
uint_fast8_t config_check_float(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf, float *val);

const OSC_Query_Argument config_boolean_args [1];
const OSC_Query_Argument config_mode_args [1];
const OSC_Query_Argument config_address_args [1];

#endif // _CONFIG_H_
