/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions giving information about DEVMODE structures
 * COPYRIGHT:   Copyright 2016 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
IsValidDevmodeA(PDEVMODEA pDevmode, size_t DevmodeSize)
{
    // Check if a Devmode was given, its dmSize member is at least as big as the DEVMODEA structure
    // and DevmodeSize is large enough for the public and private members of the structure.
    if (!pDevmode ||
        pDevmode->dmSize < sizeof(DEVMODEA) ||
        DevmodeSize < pDevmode->dmSize + pDevmode->dmDriverExtra)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    // Return success without setting the error code.
    return TRUE;
}

BOOL WINAPI
IsValidDevmodeW(PDEVMODEW pDevmode, size_t DevmodeSize)
{
    // Check if a Devmode was given, its dmSize member is at least as big as the DEVMODEW structure
    // and DevmodeSize is large enough for the public and private members of the structure.
    if (!pDevmode ||
        pDevmode->dmSize < sizeof(DEVMODEW) ||
        DevmodeSize < pDevmode->dmSize + pDevmode->dmDriverExtra)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    // Return success without setting the error code.
    return TRUE;
}
