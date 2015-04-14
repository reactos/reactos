/*
 * COPYRIGHT:       BSD, see COPYING.ARM in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/math/arm/__rt_udiv.c
 * PURPOSE:         Implementation of __rt_udiv
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

#define __rt_div_worker __rt_udiv_worker

#include "__rt_div_worker.h"

ARM_DIVRESULT
__rt_udiv(
    unsigned int divisor,
    unsigned int dividend)
{
    ARM_DIVRESULT result;

    __rt_udiv_worker(&result, divisor, dividend);
    return result;
}

