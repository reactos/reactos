/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of IsValidLocaleName
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "k32_vista.h"
#include <winnls.h>

#define NDEBUG
#include <debug.h>

BOOL
WINAPI
IsValidLocaleName(
    LPCWSTR lpLocaleName)
{
    LCID lcid = LocaleNameToLCID(lpLocaleName, LOCALE_ALLOW_NEUTRAL_NAMES);
    return lcid != 0;
}
