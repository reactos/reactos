/*
 * PROJECT:     ReactOS delayimport Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     __pfnDliNotifyHook2 symbol for delayimport library
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <windef.h>
#include <delayimp.h>

__attribute__((selectany)) PfnDliHook __pfnDliNotifyHook2 = NULL;
