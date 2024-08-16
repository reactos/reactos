/*
 * PROJECT:     GCC c++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     __mingw_vsprintf implementation
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>

int __cdecl __mingw_vsprintf(char* dest, const char* format, va_list arglist)
{
	return vsprintf(dest, format, arglist);
}
