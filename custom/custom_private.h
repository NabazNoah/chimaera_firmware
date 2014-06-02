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

#ifndef _CUSTOM_PRIVATE_H_
#define _CUSTOM_PRIVATE_H_

#include <custom.h>
#include <nosc.h>

#define RPN_STACK_HEIGHT	8

typedef nOSC_Arg Custom_Msg [5];
typedef struct _RPN_Stack RPN_Stack;

struct _RPN_Stack {
	uint32_t fid;
	//nOSC_Timestamp now;
	//nOSC_Timestamp offset;
	//uint_fast8_t old_n;
	//uint_fast8_t new_n;
	uint32_t sid;
	uint16_t gid;
	uint16_t pid;
	float x;
	float z;

	float arr [RPN_STACK_HEIGHT];
};

void rpn_run(nOSC_Message msg, Custom_Item *itm, RPN_Stack *stack);
uint_fast8_t rpn_compile(char *args, RPN_VM *vm);

#endif // _CUSTOM_PRIVATE_H_