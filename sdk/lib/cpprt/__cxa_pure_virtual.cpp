/*
 * PROJECT:     ReactOS C++ runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     __cxa_pure_virtual implementation
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <intrin.h>

extern "C" void __cxa_pure_virtual(void)
{
    __debugbreak();
}
