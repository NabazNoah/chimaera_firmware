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

#ifndef _MDNS_H_
#define _MDNS_H_

#include <stdint.h>

#include <oscquery.h>

extern const OSC_Query_Item mdns_tree [1];

typedef void(*mDNS_Resolve_Cb)(uint8_t *ip, void *data);

void mdns_dispatch(uint8_t *buf, uint16_t len);

void mdns_announce(void);

//TODO allow multiple concurrent resolvings
void mdns_resolve_timeout(void);
uint_fast8_t mdns_resolve(const char *name, mDNS_Resolve_Cb cb, void *data);

#endif // _MDNS_H_
