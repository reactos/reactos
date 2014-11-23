/* $Id: assert.h,v 1.5 2002/10/29 04:45:08 rex Exp $
 */
/*
 * assert.h
 *
 * verify program assertion. Conforming to the Single UNIX(r) Specification
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
#ifndef __ASSERT_H_INCLUDED__
#define __ASSERT_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */
#ifdef NDEBUG
#define assert(IGNORE) ((void) 0)
#else /* !NDEBUG */

#include <stdio.h>
#include <stdlib.h>

#define assert(EXPRESSION) \
 if(!(EXPRESSION)) \
 { \
  fputs("__FILE__, line __LINE__: assertion \"EXPRESSION\" failed\n", stderr); \
  abort(); \
 }

#endif /* NDEBUG */

#endif /* __ASSERT_H_INCLUDED__ */

/* EOF */

