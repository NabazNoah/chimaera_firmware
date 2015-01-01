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

#ifndef _CMC_PRIVATE_H_
#define _CMC_PRIVATE_H_

#include <cmc.h>

#define POLE_NORTH 1
#define POLE_SOUTH 0

#define ENGINE_MAX 6 // tuio1, tuio2, scsynth, oscmidi, dummy, custom

typedef enum {
	CMC_BLOB_INVALID,
	CMC_BLOB_EXISTED_STILL,
	CMC_BLOB_EXISTED_DIRTY,
	CMC_BLOB_IGNORED,
	CMC_BLOB_APPEARED,
	CMC_BLOB_DISAPPEARED
} CMC_Blob_State;

typedef struct _CMC_Filt CMC_Filt;
typedef struct _CMC_Blob CMC_Blob;

struct _CMC_Filt {
	float f1;
	float f11;
};

struct _CMC_Blob {
	uint32_t sid;
	CMC_Group *group;
	float x, y;
	CMC_Filt vx, vy;
	float v, m;
	uint16_t pid;
	uint8_t above_thresh;
	CMC_Blob_State state;
};

#endif // _CMC_PRIVATE_H_ 
