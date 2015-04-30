/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * FILE:            lib/sdk/crt/math/arm/__rt_sdiv_worker.c
 * PURPOSE:         Implementation of __rt_sdiv_worker
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

#define __rt_div_worker __rt_sdiv64_worker
#define _SIGNED_DIV_
#define _USE_64_BITS_

#include "__rt_div_worker.h"

/* __rt_sdiv64 is implemented in __rt_sdiv64.s */
