/*
 * PROJECT:     ReactOS C++ Runtime Library
 * LICENSE:     CC0-1.0 (https://spdx.org/licenses/CC0-1.0)
 * PURPOSE:     __std_terminate implementation
 * COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <exception>

extern "C"
void __std_terminate()
{
    terminate();
}
