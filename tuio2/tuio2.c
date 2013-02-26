/*
 * Copyright (c) 2012 Hanspeter Portner (agenthp@users.sf.net)
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

#include <chimaera.h>
#include <chimutil.h>
#include <config.h>
#include <cmc.h>

#include "tuio2_private.h"

Tuio2 tuio;
char *frm_str = "/tuio2/frm";
char *tok_str = "/tuio2/_STxz";
char *alv_str = "/tuio2/alv";

char *frm_fmt_short = "it";
char *frm_fmt_long = "itsiii";
char *tok_fmt = "iiff";
char alv_fmt [BLOB_MAX+1]; // this has a variable string len

nOSC_Bundle tuio2_bndl = tuio.bndl;

void
tuio2_init ()
{
	uint8_t i;

	// initialize bundle
	nosc_item_message_set (tuio.bndl, 0, tuio.frm, frm_str, frm_fmt_short);

	for (i=0; i<BLOB_MAX; i++)
		nosc_item_message_set (tuio.bndl, i+1, tuio.tok[i], tok_str, tok_fmt);

	nosc_item_message_set (tuio.bndl, BLOB_MAX + 1, tuio.alv, alv_str, alv_fmt);
	for (i=0; i<BLOB_MAX; i++)

	// initialize frame
	nosc_message_set_int32 (tuio.frm, 0, 0);
	nosc_message_set_timestamp (tuio.frm, 1, nOSC_IMMEDIATE);

	// long header
	tuio2_long_header_enable (config.tuio.long_header);
	tuio2_compact_token_enable (config.tuio.compact_token);

	// initialize tok
	for (i=0; i<BLOB_MAX; i++)
	{
		nOSC_Message msg = tuio.tok[i];

		nosc_message_set_int32 (msg, 0, 0);
		nosc_message_set_int32 (msg, 1, 0);
		nosc_message_set_float (msg, 2, 0.0);
		nosc_message_set_float (msg, 3, 0.0);
	}

	// initialize alv
	for (i=0; i<BLOB_MAX; i++)
	{
		nosc_message_set_int32 (tuio.alv, i, 0);
		alv_fmt[i] = nOSC_INT32;
	}
	alv_fmt[BLOB_MAX] = nOSC_END;
}

void
tuio2_long_header_enable (uint8_t on)
{
	config.tuio.long_header = on;

	if (on)
	{
		// use EUI_64
		//int32_t addr = (EUI_64[0] << 24) | (EUI_64[1] << 16) | (EUI_64[2] << 8) | EUI_64[3];
		//int32_t inst = (EUI_64[4] << 24) | (EUI_64[5] << 16) | (EUI_64[6] << 8) | EUI_64[7];

		// or use EUI_48 + port
		uint16_t port = config.output.socket.port[SRC_PORT];
		int32_t addr = (EUI_48[0] << 24) | (EUI_48[1] << 16) | (EUI_48[2] << 8) | EUI_48[3];
		int32_t inst = (EUI_48[4] << 24) | (EUI_48[5] << 16) | (port & 0xff00) | (port & 0xff);

		nosc_message_set_string (tuio.frm, 2, config.name);
		nosc_message_set_int32 (tuio.frm, 3, addr);
		nosc_message_set_int32 (tuio.frm, 4, inst);
		nosc_message_set_int32 (tuio.frm, 5, (SENSOR_N << 16) | 1);

		nosc_item_message_set (tuio.bndl, 0, tuio.frm, frm_str, frm_fmt_long);
	}
	else // off
		nosc_item_message_set (tuio.bndl, 0, tuio.frm, frm_str, frm_fmt_short);
}

void
tuio2_compact_token_enable (uint8_t on)
{
	//TODO
	if (on)
	{
		// /tuio2/_STxz sid tid x z
	}
	else // off
	{
		// /tuio2/tok sid uid tid x y z p
	}
}

static uint8_t old_end = BLOB_MAX;
static uint8_t tok = 0;

void
tuio2_engine_frame_cb (uint32_t fid, uint64_t timestamp, uint8_t nblob_old, uint8_t end)
{
	tuio.frm[0].i = fid;
	tuio.frm[1].t = timestamp;

	// first undo previous unlinking at position old_end
	if (old_end < BLOB_MAX)
	{
		// relink alv message
		alv_fmt[old_end] = nOSC_INT32;

		nosc_item_message_set (tuio.bndl, old_end + 1, tuio.tok[old_end], tok_str, tok_fmt);

		if (old_end < BLOB_MAX-1)
			nosc_item_message_set (tuio.bndl, old_end + 2, tuio.tok[old_end+1], tok_str, tok_fmt);
	}

	// then unlink at position end
	if (end < BLOB_MAX)
	{
		// unlink alv message
		alv_fmt[end] = nOSC_END;

		// prepend alv message
		nosc_item_message_set (tuio.bndl, end + 1, tuio.bndl[BLOB_MAX+1].content.message.msg, tuio.bndl[BLOB_MAX+1].content.message.path, tuio.bndl[BLOB_MAX+1].content.message.fmt);

		// unlink bundle
		nosc_item_term_set (tuio.bndl, end + 2);
	}

	old_end = end;

	tok = 0; // reset token pointer
}

void
tuio2_engine_token_cb (uint32_t sid, uint16_t uid, uint16_t tid, float x, float y)
{
	nOSC_Message msg = tuio.tok[tok];

	msg[0].i = sid;
	msg[1].i = ((uint32_t)uid << 16) | tid;
	msg[2].f = x;
	msg[3].f = y;

	nosc_message_set_int32 (tuio.alv, tok, sid);

	tok++; // increase token pointer
}

CMC_Engine tuio2_engine = {
	&config.tuio.enabled,
	tuio2_engine_frame_cb,
	tuio2_engine_token_cb,
	NULL,
	tuio2_engine_token_cb
};
