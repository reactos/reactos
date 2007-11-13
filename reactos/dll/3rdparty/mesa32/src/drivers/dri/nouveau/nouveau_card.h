/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/


#ifndef __NOUVEAU_CARD_H__
#define __NOUVEAU_CARD_H__

#include "dri_util.h"
#include "drm.h"
#include "nouveau_drm.h"

typedef struct nouveau_card_t {
	uint16_t id; /* last 4 digits of pci id, last digit is always 0 */
	char* name; /* the user-friendly card name */
	uint32_t class_3d; /* the object class this card uses for 3D */
	uint32_t type; /* the major card family */
	uint32_t flags;
}
nouveau_card;

#define NV_HAS_LMA 0x00000001

extern nouveau_card* nouveau_card_lookup(uint32_t device_id);

#endif

