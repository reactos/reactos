/* $Id: interlock.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * psx/interlock.h
 *
 * inter-locked increment/decrement
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
#ifndef __PSX_INTERLOCK_H_INCLUDED__
#define __PSX_INTERLOCK_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */
int __interlock_inc(int *);
int __interlock_dec(int *);
int __interlock_add(int *, int);

/* MACROS */

#endif /* __PSX_INTERLOCK_H_INCLUDED__ */

/* EOF */

