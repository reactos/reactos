//
// __report_rangecheckfailure.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __report_rangecheckfailure.
//
// SPDX-License-Identifier: MIT
//

#include <intrin.h>
#include <ntrtl.h>

__declspec(noreturn) void __cdecl __report_rangecheckfailure(void)
{
    __fastfail(FAST_FAIL_RANGE_CHECK_FAILURE);
}
