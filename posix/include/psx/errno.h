/* $Id: errno.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * psx/errno.h
 *
 * internal errno.h
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
#ifndef __PSX_ERRNO_H_INCLUDED__
#define __PSX_ERRNO_H_INCLUDED__

/* INCLUDES */
#include <errno.h>

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */
#define __status_to_errno(s) (s)
#define __set_errno_from_status(s) (errno = __status_to_errno((s)))

#endif /* __PSX_ERRNO_H_INCLUDED__ */

/* EOF */

