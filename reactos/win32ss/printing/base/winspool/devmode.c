/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions giving information about DEVMODE structures
 * COPYRIGHT:   Copyright 2016 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

typedef struct _MINIMUM_SIZE_TABLE
{
    DWORD dwField;
    WORD wSize;
}
MINIMUM_SIZE_TABLE, *PMINIMUM_SIZE_TABLE;

/**
 * Minimum required DEVMODEA structure size based on the used fields. Must be in descending order!
 */
static MINIMUM_SIZE_TABLE MinimumSizeA[] = {
    { DM_PANNINGHEIGHT, FIELD_OFFSET(DEVMODEA, dmPanningHeight) + RTL_FIELD_SIZE(DEVMODEA, dmPanningHeight) },
    { DM_PANNINGWIDTH, FIELD_OFFSET(DEVMODEA, dmPanningWidth) + RTL_FIELD_SIZE(DEVMODEA, dmPanningWidth) },
    { DM_DITHERTYPE, FIELD_OFFSET(DEVMODEA, dmDitherType) + RTL_FIELD_SIZE(DEVMODEA, dmDitherType) },
    { DM_MEDIATYPE, FIELD_OFFSET(DEVMODEA, dmMediaType) + RTL_FIELD_SIZE(DEVMODEA, dmMediaType) },
    { DM_ICMINTENT, FIELD_OFFSET(DEVMODEA, dmICMIntent) + RTL_FIELD_SIZE(DEVMODEA, dmICMIntent) },
    { DM_ICMMETHOD, FIELD_OFFSET(DEVMODEA, dmICMMethod) + RTL_FIELD_SIZE(DEVMODEA, dmICMMethod) },
    { DM_DISPLAYFREQUENCY, FIELD_OFFSET(DEVMODEA, dmDisplayFrequency) + RTL_FIELD_SIZE(DEVMODEA, dmDisplayFrequency) },
    { DM_NUP, FIELD_OFFSET(DEVMODEA, dmNup) + RTL_FIELD_SIZE(DEVMODEA, dmNup) },
    { DM_DISPLAYFLAGS, FIELD_OFFSET(DEVMODEA, dmDisplayFlags) + RTL_FIELD_SIZE(DEVMODEA, dmDisplayFlags) },
    { DM_PELSHEIGHT, FIELD_OFFSET(DEVMODEA, dmPelsHeight) + RTL_FIELD_SIZE(DEVMODEA, dmPelsHeight) },
    { DM_PELSWIDTH, FIELD_OFFSET(DEVMODEA, dmPelsWidth) + RTL_FIELD_SIZE(DEVMODEA, dmPelsWidth) },
    { DM_BITSPERPEL, FIELD_OFFSET(DEVMODEA, dmBitsPerPel) + RTL_FIELD_SIZE(DEVMODEA, dmBitsPerPel) },
    { DM_LOGPIXELS, FIELD_OFFSET(DEVMODEA, dmLogPixels) + RTL_FIELD_SIZE(DEVMODEA, dmLogPixels) },
    { DM_FORMNAME, FIELD_OFFSET(DEVMODEA, dmFormName) + RTL_FIELD_SIZE(DEVMODEA, dmFormName) },
    { DM_COLLATE, FIELD_OFFSET(DEVMODEA, dmCollate) + RTL_FIELD_SIZE(DEVMODEA, dmCollate) },
    { DM_TTOPTION, FIELD_OFFSET(DEVMODEA, dmTTOption) + RTL_FIELD_SIZE(DEVMODEA, dmTTOption) },
    { DM_YRESOLUTION, FIELD_OFFSET(DEVMODEA, dmYResolution) + RTL_FIELD_SIZE(DEVMODEA, dmYResolution) },
    { DM_DUPLEX, FIELD_OFFSET(DEVMODEA, dmDuplex) + RTL_FIELD_SIZE(DEVMODEA, dmDuplex) },
    { DM_COLOR, FIELD_OFFSET(DEVMODEA, dmColor) + RTL_FIELD_SIZE(DEVMODEA, dmColor) },
    { DM_DISPLAYFIXEDOUTPUT, FIELD_OFFSET(DEVMODEA, dmDisplayFixedOutput) + RTL_FIELD_SIZE(DEVMODEA, dmDisplayFixedOutput) },
    { DM_DISPLAYORIENTATION, FIELD_OFFSET(DEVMODEA, dmDisplayOrientation) + RTL_FIELD_SIZE(DEVMODEA, dmDisplayOrientation) },
    { DM_POSITION, FIELD_OFFSET(DEVMODEA, dmPosition) + RTL_FIELD_SIZE(DEVMODEA, dmPosition) },
    { DM_PRINTQUALITY, FIELD_OFFSET(DEVMODEA, dmPrintQuality) + RTL_FIELD_SIZE(DEVMODEA, dmPrintQuality) },
    { DM_DEFAULTSOURCE, FIELD_OFFSET(DEVMODEA, dmDefaultSource) + RTL_FIELD_SIZE(DEVMODEA, dmDefaultSource) },
    { DM_COPIES, FIELD_OFFSET(DEVMODEA, dmCopies) + RTL_FIELD_SIZE(DEVMODEA, dmCopies) },
    { DM_SCALE, FIELD_OFFSET(DEVMODEA, dmScale) + RTL_FIELD_SIZE(DEVMODEA, dmScale) },
    { DM_PAPERWIDTH, FIELD_OFFSET(DEVMODEA, dmPaperWidth) + RTL_FIELD_SIZE(DEVMODEA, dmPaperWidth) },
    { DM_PAPERLENGTH, FIELD_OFFSET(DEVMODEA, dmPaperLength) + RTL_FIELD_SIZE(DEVMODEA, dmPaperLength) },
    { DM_PAPERSIZE, FIELD_OFFSET(DEVMODEA, dmPaperSize) + RTL_FIELD_SIZE(DEVMODEA, dmPaperSize) },
    { DM_ORIENTATION, FIELD_OFFSET(DEVMODEA, dmOrientation) + RTL_FIELD_SIZE(DEVMODEA, dmOrientation) },
    { 0, 0 }
};

/**
 * Minimum required DEVMODEW structure size based on the used fields. Must be in descending order!
 */
static MINIMUM_SIZE_TABLE MinimumSizeW[] = {
    { DM_PANNINGHEIGHT, FIELD_OFFSET(DEVMODEW, dmPanningHeight) + RTL_FIELD_SIZE(DEVMODEW, dmPanningHeight) },
    { DM_PANNINGWIDTH, FIELD_OFFSET(DEVMODEW, dmPanningWidth) + RTL_FIELD_SIZE(DEVMODEW, dmPanningWidth) },
    { DM_DITHERTYPE, FIELD_OFFSET(DEVMODEW, dmDitherType) + RTL_FIELD_SIZE(DEVMODEW, dmDitherType) },
    { DM_MEDIATYPE, FIELD_OFFSET(DEVMODEW, dmMediaType) + RTL_FIELD_SIZE(DEVMODEW, dmMediaType) },
    { DM_ICMINTENT, FIELD_OFFSET(DEVMODEW, dmICMIntent) + RTL_FIELD_SIZE(DEVMODEW, dmICMIntent) },
    { DM_ICMMETHOD, FIELD_OFFSET(DEVMODEW, dmICMMethod) + RTL_FIELD_SIZE(DEVMODEW, dmICMMethod) },
    { DM_DISPLAYFREQUENCY, FIELD_OFFSET(DEVMODEW, dmDisplayFrequency) + RTL_FIELD_SIZE(DEVMODEW, dmDisplayFrequency) },
    { DM_NUP, FIELD_OFFSET(DEVMODEW, dmNup) + RTL_FIELD_SIZE(DEVMODEW, dmNup) },
    { DM_DISPLAYFLAGS, FIELD_OFFSET(DEVMODEW, dmDisplayFlags) + RTL_FIELD_SIZE(DEVMODEW, dmDisplayFlags) },
    { DM_PELSHEIGHT, FIELD_OFFSET(DEVMODEW, dmPelsHeight) + RTL_FIELD_SIZE(DEVMODEW, dmPelsHeight) },
    { DM_PELSWIDTH, FIELD_OFFSET(DEVMODEW, dmPelsWidth) + RTL_FIELD_SIZE(DEVMODEW, dmPelsWidth) },
    { DM_BITSPERPEL, FIELD_OFFSET(DEVMODEW, dmBitsPerPel) + RTL_FIELD_SIZE(DEVMODEW, dmBitsPerPel) },
    { DM_LOGPIXELS, FIELD_OFFSET(DEVMODEW, dmLogPixels) + RTL_FIELD_SIZE(DEVMODEW, dmLogPixels) },
    { DM_FORMNAME, FIELD_OFFSET(DEVMODEW, dmFormName) + RTL_FIELD_SIZE(DEVMODEW, dmFormName) },
    { DM_COLLATE, FIELD_OFFSET(DEVMODEW, dmCollate) + RTL_FIELD_SIZE(DEVMODEW, dmCollate) },
    { DM_TTOPTION, FIELD_OFFSET(DEVMODEW, dmTTOption) + RTL_FIELD_SIZE(DEVMODEW, dmTTOption) },
    { DM_YRESOLUTION, FIELD_OFFSET(DEVMODEW, dmYResolution) + RTL_FIELD_SIZE(DEVMODEW, dmYResolution) },
    { DM_DUPLEX, FIELD_OFFSET(DEVMODEW, dmDuplex) + RTL_FIELD_SIZE(DEVMODEW, dmDuplex) },
    { DM_COLOR, FIELD_OFFSET(DEVMODEW, dmColor) + RTL_FIELD_SIZE(DEVMODEW, dmColor) },
    { DM_DISPLAYFIXEDOUTPUT, FIELD_OFFSET(DEVMODEW, dmDisplayFixedOutput) + RTL_FIELD_SIZE(DEVMODEW, dmDisplayFixedOutput) },
    { DM_DISPLAYORIENTATION, FIELD_OFFSET(DEVMODEW, dmDisplayOrientation) + RTL_FIELD_SIZE(DEVMODEW, dmDisplayOrientation) },
    { DM_POSITION, FIELD_OFFSET(DEVMODEW, dmPosition) + RTL_FIELD_SIZE(DEVMODEW, dmPosition) },
    { DM_PRINTQUALITY, FIELD_OFFSET(DEVMODEW, dmPrintQuality) + RTL_FIELD_SIZE(DEVMODEW, dmPrintQuality) },
    { DM_DEFAULTSOURCE, FIELD_OFFSET(DEVMODEW, dmDefaultSource) + RTL_FIELD_SIZE(DEVMODEW, dmDefaultSource) },
    { DM_COPIES, FIELD_OFFSET(DEVMODEW, dmCopies) + RTL_FIELD_SIZE(DEVMODEW, dmCopies) },
    { DM_SCALE, FIELD_OFFSET(DEVMODEW, dmScale) + RTL_FIELD_SIZE(DEVMODEW, dmScale) },
    { DM_PAPERWIDTH, FIELD_OFFSET(DEVMODEW, dmPaperWidth) + RTL_FIELD_SIZE(DEVMODEW, dmPaperWidth) },
    { DM_PAPERLENGTH, FIELD_OFFSET(DEVMODEW, dmPaperLength) + RTL_FIELD_SIZE(DEVMODEW, dmPaperLength) },
    { DM_PAPERSIZE, FIELD_OFFSET(DEVMODEW, dmPaperSize) + RTL_FIELD_SIZE(DEVMODEW, dmPaperSize) },
    { DM_ORIENTATION, FIELD_OFFSET(DEVMODEW, dmOrientation) + RTL_FIELD_SIZE(DEVMODEW, dmOrientation) },
    { 0, 0 }
};

/**
 * Replace the last character by a null terminator if the given ANSI string is not null-terminated.
 */
static __inline void
_FixStringA(PBYTE String, DWORD cbString)
{
    const PBYTE pLastCharacter = &String[cbString / sizeof(BYTE) - 1];
    PBYTE p = String;

    while (*p)
    {
        if (p == pLastCharacter)
        {
            *p = 0;
            break;
        }

        p++;
    }
}

/**
 * Replace the last character by a null terminator if the given Unicode string is not null-terminated.
 */
static __inline void
_FixStringW(PWSTR String, DWORD cbString)
{
    const PWSTR pLastCharacter = &String[cbString / sizeof(WCHAR) - 1];
    PWSTR p = String;

    while (*p)
    {
        if (p == pLastCharacter)
        {
            *p = 0;
            break;
        }

        p++;
    }
}

BOOL WINAPI
IsValidDevmodeA(PDEVMODEA pDevmode, size_t DevmodeSize)
{
    PMINIMUM_SIZE_TABLE pTable = MinimumSizeA;
    WORD wRequiredSize;

    // Check if a Devmode was given at all.
    if (!pDevmode)
        goto Failure;

    // Verify that DevmodeSize is large enough to hold the public and private members of the structure.
    if (DevmodeSize < pDevmode->dmSize + pDevmode->dmDriverExtra)
        goto Failure;

    // If the structure has private members, the public structure must be 32-bit packed.
    if (pDevmode->dmDriverExtra && pDevmode->dmSize % 4)
        goto Failure;

    // Now determine the minimum possible dmSize based on the given fields in dmFields.
    wRequiredSize = FIELD_OFFSET(DEVMODEA, dmFields) + RTL_FIELD_SIZE(DEVMODEA, dmFields);

    while (pTable->dwField)
    {
        if (pDevmode->dmFields & pTable->dwField)
        {
            wRequiredSize = pTable->wSize;
            break;
        }

        pTable++;
    }

    // Verify that the value in dmSize is big enough for the used fields.
    if (pDevmode->dmSize < wRequiredSize)
        goto Failure;

    // Check if dmDeviceName and (if used) dmFormName are null-terminated.
    // Fix this if they aren't.
    _FixStringA(pDevmode->dmDeviceName, sizeof(pDevmode->dmDeviceName));
    if (pDevmode->dmFields & DM_FORMNAME)
        _FixStringA(pDevmode->dmFormName, sizeof(pDevmode->dmFormName));

    // Return success without setting the error code.
    return TRUE;

Failure:
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

BOOL WINAPI
IsValidDevmodeW(PDEVMODEW pDevmode, size_t DevmodeSize)
{
    PMINIMUM_SIZE_TABLE pTable = MinimumSizeW;
    WORD wRequiredSize;

    // Check if a Devmode was given at all.
    if (!pDevmode)
        goto Failure;

    // Verify that DevmodeSize is large enough to hold the public and private members of the structure.
    if (DevmodeSize < pDevmode->dmSize + pDevmode->dmDriverExtra)
        goto Failure;

    // If the structure has private members, the public structure must be 32-bit packed.
    if (pDevmode->dmDriverExtra && pDevmode->dmSize % 4)
        goto Failure;

    // Now determine the minimum possible dmSize based on the given fields in dmFields.
    wRequiredSize = FIELD_OFFSET(DEVMODEW, dmFields) + RTL_FIELD_SIZE(DEVMODEW, dmFields);

    while (pTable->dwField)
    {
        if (pDevmode->dmFields & pTable->dwField)
        {
            wRequiredSize = pTable->wSize;
            break;
        }

        pTable++;
    }

    // Verify that the value in dmSize is big enough for the used fields.
    if (pDevmode->dmSize < wRequiredSize)
        goto Failure;

    // Check if dmDeviceName and (if used) dmFormName are null-terminated.
    // Fix this if they aren't.
    _FixStringW(pDevmode->dmDeviceName, sizeof(pDevmode->dmDeviceName));
    if (pDevmode->dmFields & DM_FORMNAME)
        _FixStringW(pDevmode->dmFormName, sizeof(pDevmode->dmFormName));

    // Return success without setting the error code.
    return TRUE;

Failure:
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}
