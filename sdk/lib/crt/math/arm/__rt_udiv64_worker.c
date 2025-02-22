/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * PURPOSE:         Implementation of __rt_udiv64_worker
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

#define __rt_div_worker __rt_udiv64_worker
#define _USE_64_BITS_

#include "__rt_div_worker.h"

/* __rt_udiv64 is implemented in __rt_udiv64.s */
