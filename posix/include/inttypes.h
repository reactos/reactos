/* $Id: inttypes.h,v 1.2 2002/02/20 09:17:54 hyperion Exp $
 */
/*
 * inttypes.h
 *
 * fixed size integral types. Conforming to the Single UNIX(r) Specification
 * Version 2, System Interface & Headers Issue 5
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
#ifndef __INTTYPES_H_INCLUDED__
#define __INTTYPES_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
/* signed */
typedef signed char        int8_t;  /*  8-bit signed integral type. */
typedef signed short int   int16_t; /* 16-bit signed integral type. */
typedef signed long int    int32_t; /* 32-bit signed integral type. */
typedef signed long long   int64_t; /* 64-bit signed integral type. */

/* unsigned */
typedef unsigned char      uint8_t;  /*  8-bit unsigned integral type. */
typedef unsigned short int uint16_t; /* 16-bit unsigned integral type. */
typedef unsigned long int  uint32_t; /* 32-bit unsigned integral type. */
typedef unsigned long long uint64_t; /* 64-bit unsigned integral type. */

/* pointer-sized */
typedef signed long int    intptr_t; /* Signed integral type large enough
                                          to hold any pointer. */
typedef unsigned long int  uintptr_t; /* Unsigned integral type large
                                           enough to hold any pointer. */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */

#endif /* __INTTYPES_H_INCLUDED__ */

/* EOF */

