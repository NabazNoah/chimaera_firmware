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

#ifndef SCSYNTH_H_
#define SCSYNTH_H_

#include <stdint.h>

#include <netdef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SCSynth_Add_Action {
	SCSYNTH_ADD_TO_HEAD = 0,
	SCSYNTH_ADD_TO_TAIL = 1,
	SCSYNTH_ADD_BEFORE = 2,
	SCSYNTH_ADD_AFTER = 3,
	SCSYNTH_ADD_REPLACE = 4
} SCSynth_Add_Action;

extern nOSC_Item scsynth_bndl [];
extern char scsynth_fmt [];
extern CMC_Engine scsynth_engine;

void scsynth_init ();

#ifdef __cplusplus
}
#endif

#endif
