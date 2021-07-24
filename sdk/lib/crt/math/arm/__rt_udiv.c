/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __rt_udiv
 * COPYRIGHT:   Copyright 2015 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Raman Masanin <36927roma@gmail.com>
 */

#define __rt_div_worker __rt_udiv_worker

#include "__rt_div_worker.h"

 /*
  * Returns quotient in R0, remainder in R1
  */
unsigned long long
__rt_udiv(
    unsigned int divisor,
    unsigned int dividend)
{
    ARM_DIVRESULT result;

    __rt_udiv_worker(divisor, dividend, &result);

    return result.raw_data;
}
