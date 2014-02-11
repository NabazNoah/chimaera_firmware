/*
 * Copyright (c) 2013 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdint.h>

#include <nosc.h>
#include <wiz.h>
#include <config.h>

#define DEBUG(...) \
({ \
	if (config.debug.socket.enabled) { \
		uint16_t size; \
		size = nosc_message_vararg_serialize (BUF_O_OFFSET(buf_o_ptr), "/debug", __VA_ARGS__); \
		udp_send (config.debug.socket.sock, BUF_O_BASE(buf_o_ptr), size); \
	} \
})

#define debug_str(s) DEBUG("s", s)
#define debug_int32(i) DEBUG("i", i)
#define debug_float(f) DEBUG("f", f)
#define debug_double(d) DEBUG("d", d)
#define debug_timestamp(t) DEBUG("t", t)

extern const nOSC_Query_Item debug_tree [2];

#endif
