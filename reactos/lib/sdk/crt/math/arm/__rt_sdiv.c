/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * FILE:            lib/sdk/crt/math/arm/__rt_sdiv.c
 * PURPOSE:         Implementation of __rt_sdiv
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

#define __rt_div_worker __rt_sdiv_worker
#define _SIGNED_DIV_

#include "__rt_div_worker.h"

ARM_DIVRESULT
__rt_sdiv(
    int divisor,
    int dividend)
{
    ARM_DIVRESULT result;

    __rt_sdiv_worker(&result, divisor, dividend);
    return result;
}

