//
// __std_terminate.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __std_terminate.
//
// SPDX-License-Identifier: MIT
//

#include <process.h>

__declspec(noreturn) void __cdecl terminate();

__declspec(noreturn)
void __std_terminate(void)
{
    terminate();
}
