/* $Id: pthread.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * psx/pthread.h
 *
 * internal pthread.h
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
#ifndef __PSX_PTHREAD_H_INCLUDED__
#define __PSX_PTHREAD_H_INCLUDED__

/* INCLUDES */
#include <psx/safeobj.h>

/* OBJECTS */

/* TYPES */

struct __mutexattr
{
 __magic_t signature;
 int       pshared;
 int       protocol;
 int       type;
};

struct __mutex
{
 __magic_t signature;
 void *    handle;
 int       protocol;
 int       type;       
};

/* CONSTANTS */
#define __PTHREAD_MUTEX_MAGIC      (MAGIC('P', 'T', 'M', 'X'))
#define __PTHREAD_MUTEX_ATTR_MAGIC (MAGIC('P', 'T', 'M', 'A'))

/* PROTOTYPES */

/* MACROS */

#endif /* __PSX_PTHREAD_H_INCLUDED__ */

/* EOF */

