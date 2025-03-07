//
// __report_gsfailure.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __report_gsfailure.
//
// SPDX-License-Identifier: MIT
//

#include <intrin.h>
#include <ntrtl.h>

#if defined(_M_IX86)
__declspec(noreturn) void __cdecl __report_gsfailure(void)
#else
__declspec(noreturn) void __cdecl __report_gsfailure(_In_ uintptr_t _StackCookie)
#endif
{
    __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
}
