/* $Id: math.h,v 1.2 2002/02/20 09:17:54 hyperion Exp $
 */
/*
 * math.h
 *
 * mathematical declarations. Conforming to the Single UNIX(r)
 * Specification Version 2, System Interface & Headers Issue 5
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
#ifndef __MATH_H_INCLUDED__
#define __MATH_H_INCLUDED__

/* INCLUDES */
#ifdef __PSXDLL__

/* headers for internal usage by psxdll.dll and ReactOS */

#else /* ! __PSXDLL__ */

/* standard POSIX headers */

#endif

/* OBJECTS */

/* TYPES */

/* CONSTANTS */
#define M_E        ((double) 2.7182818285) /* Value of e */
#define M_LOG2E    ((double) 1.4426950419) /* Value of log2(e) */
#define M_LOG10E   ((double) 0.4342944819) /* Value of log10(e) */
#define M_LN2      ((double)-0.6931471806) /* Value of loge2 */
#define M_LN10     ((double) 2.3025850929) /* Value of loge10 */
#define M_PI       ((double) 3.1415926536) /* Value of Pi */
#define M_PI_2     ((double) 1.5707963268) /* Value of Pi/2 */
#define M_PI_4     ((double) 0.7853981634) /* Value of Pi/4 */
#define M_1_PI     ((double) 0.3183098862) /* Value of 1/Pi */
#define M_2_PI     ((double) 0.6366197724) /* Value of 2/Pi */
#define M_2_SQRTPI ((double) 1.1283791671) /* Value of 2/Sqrt(Pi) */
#define M_SQRT2    ((double) 1.4142135624) /* Value of Sqrt(2) */
#define M_SQRT1_2  ((double) 0.7071067812) /* Value of Sqrt(1/2) */

/* PROTOTYPES */

/* MACROS */

#endif /* __MATH_H_INCLUDED__ */

/* EOF */

