/*
 * psx/safeobj.h
 *
 * types and definitions for safe checking of user-provided objects
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __PSX_SAFEOBJ_H_INCLUDED__
#define __PSX_SAFEOBJ_H_INCLUDED__

/* INCLUDES */
#include <inttypes.h>

/* OBJECTS */

/* TYPES */
typedef uint32_t __magic_t;

/* CONSTANTS */

/* PROTOTYPES */
int __safeobj_validate(void *, __magic_t);

/* MACROS */
/* builds a magic number from 4 characters */
#define MAGIC(a,b,c,d) ( \
 (((uint32_t)(uint8_t)(a)) << 24) | \
 (((uint32_t)(uint8_t)(b)) << 16) | \
 (((uint32_t)(uint8_t)(c)) <<  8) | \
 (((uint32_t)(uint8_t)(d)) <<  0) \
)

/* retrieves a comma-separated list of the 4 characters in a magic number */
#define MAGIC_DECOMPOSE(m) \
 ((uint8_t)(m >> 24)), \
 ((uint8_t)(m >> 16)), \
 ((uint8_t)(m >>  8)), \
 ((uint8_t)(m >>  0))

#endif /* __PSX_SAFEOBJ_H_INCLUDED__ */

/* EOF */

