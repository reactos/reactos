/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for IsValidDevmodeA/IsValidDevmodeW
 * COPYRIGHT:   Copyright 2016 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#include <wchar.h>

typedef struct _MINIMUM_SIZE_TABLE
{
    DWORD dwField;
    WORD wSize;
}
MINIMUM_SIZE_TABLE, *PMINIMUM_SIZE_TABLE;

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
    { 0, FIELD_OFFSET(DEVMODEA, dmFields) + RTL_FIELD_SIZE(DEVMODEA, dmFields) }
};

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
    { 0, FIELD_OFFSET(DEVMODEW, dmFields) + RTL_FIELD_SIZE(DEVMODEW, dmFields) }
};



START_TEST(IsValidDevmodeA)
{
    DEVMODEA DevMode;
    PMINIMUM_SIZE_TABLE pTable = MinimumSizeA;

    // Give no Devmode at all, this has to fail without crashing.
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(NULL, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // Give a zeroed Devmode, this has to fail, because dmSize isn't set.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // Now set only the dmSize member, IsValidDevmodeA should return TRUE now. The Last Error isn't touched again.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // IsValidDevmodeA should also succeed if the DevMode appears to be larger.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA) + 1;
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // The DevmodeSize parameter may be larger than dmSize, but not the other way round!
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA) + 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // dmDriverExtra is also taken into account.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    DevMode.dmDriverExtra = 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // dmSize must be a multiple of 4 if any dmDriverExtra is given.
    for (DevMode.dmDriverExtra = 1; DevMode.dmDriverExtra <= 4; DevMode.dmDriverExtra++)
    {
        for (DevMode.dmSize = sizeof(DEVMODEA); DevMode.dmSize < sizeof(DEVMODEA) + 4; DevMode.dmSize++)
        {
            BOOL bExpected = (DevMode.dmSize % 4 == 0);
            DWORD dwExpectedError = (bExpected ? 0xDEADBEEF : ERROR_INVALID_DATA);

            SetLastError(0xDEADBEEF);
            ok(IsValidDevmodeA(&DevMode, DevMode.dmSize + DevMode.dmDriverExtra) == bExpected, "IsValidDevmodeA returns %d!\n", !bExpected);
            ok(GetLastError() == dwExpectedError, "IsValidDevmodeA returns error %lu!\n", GetLastError());
        }
    }

    // IsValidDevmodeA also fixes missing null-terminations if the corresponding fields are used.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    memset(DevMode.dmDeviceName, 'A', 32);
    memset(DevMode.dmFormName, 'A', 32);
    ok(IsValidDevmodeA(&DevMode, DevMode.dmSize), "IsValidDevmodeA returns FALSE!\n");
    ok(DevMode.dmDeviceName[31] == '\0', "Expected dmDeviceName to be '\\0', but is '%c'\n", DevMode.dmDeviceName[31]);
    ok(DevMode.dmFormName[31] == 'A', "Expected dmFormName to be 'A', but is '%c'\n", DevMode.dmDeviceName[31]);

    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    DevMode.dmFields = DM_FORMNAME;
    memset(DevMode.dmDeviceName, 'A', 32);
    memset(DevMode.dmFormName, 'A', 32);
    ok(IsValidDevmodeA(&DevMode, DevMode.dmSize), "IsValidDevmodeA returns FALSE!\n");
    ok(DevMode.dmDeviceName[31] == '\0', "Expected dmDeviceName to be '\\0', but is '%c'\n", DevMode.dmDeviceName[31]);
    ok(DevMode.dmFormName[31] == '\0', "Expected dmFormName to be '\\0', but is '%c'\n", DevMode.dmDeviceName[31]);

    // Depending on which fields are given in dmFields, the minimum value for dmSize is different.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));

    do
    {
        DevMode.dmFields = pTable->dwField;
        DevMode.dmSize = pTable->wSize;

        // This size should succeed!
        ok(IsValidDevmodeA(&DevMode, DevMode.dmSize), "IsValidDevmodeA returns FALSE!\n");

        // One byte less should not succeed!
        DevMode.dmSize--;
        ok(!IsValidDevmodeA(&DevMode, DevMode.dmSize), "IsValidDevmodeA returns TRUE!\n");

        pTable++;
    }
    while (pTable->dwField);
}

START_TEST(IsValidDevmodeW)
{
    DEVMODEW DevMode;
    PMINIMUM_SIZE_TABLE pTable = MinimumSizeW;

    // Give no Devmode at all, this has to fail without crashing.
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(NULL, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // Give a zeroed Devmode, this has to fail, because dmSize isn't set.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // Now set only the dmSize member, IsValidDevmodeW should return TRUE now. The Last Error isn't touched again.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // IsValidDevmodeW should also succeed if the DevMode appears to be larger.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW) + 1;
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // The DevmodeSize parameter may be larger than dmSize, but not the other way round!
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW) + 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // dmDriverExtra is also taken into account.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    DevMode.dmDriverExtra = 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // dmSize must be a multiple of 4 if any dmDriverExtra is given.
    for (DevMode.dmDriverExtra = 1; DevMode.dmDriverExtra <= 4; DevMode.dmDriverExtra++)
    {
        for (DevMode.dmSize = sizeof(DEVMODEW); DevMode.dmSize < sizeof(DEVMODEW) + 4; DevMode.dmSize++)
        {
            BOOL bExpected = (DevMode.dmSize % 4 == 0);
            DWORD dwExpectedError = (bExpected ? 0xDEADBEEF : ERROR_INVALID_DATA);

            SetLastError(0xDEADBEEF);
            ok(IsValidDevmodeW(&DevMode, DevMode.dmSize + DevMode.dmDriverExtra) == bExpected, "IsValidDevmodeW returns %d!\n", !bExpected);
            ok(GetLastError() == dwExpectedError, "IsValidDevmodeW returns error %lu!\n", GetLastError());
        }
    }

    // IsValidDevmodeW also fixes missing null-terminations if the corresponding fields are used.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    wmemset(DevMode.dmDeviceName, L'A', 32);
    wmemset(DevMode.dmFormName, L'A', 32);
    ok(IsValidDevmodeW(&DevMode, DevMode.dmSize), "IsValidDevmodeW returns FALSE!\n");
    ok(DevMode.dmDeviceName[31] == L'\0', "Expected dmDeviceName to be '\\0', but is '%C'\n", DevMode.dmDeviceName[31]);
    ok(DevMode.dmFormName[31] == L'A', "Expected dmFormName to be 'A', but is '%C'\n", DevMode.dmDeviceName[31]);

    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    DevMode.dmFields = DM_FORMNAME;
    wmemset(DevMode.dmDeviceName, L'A', 32);
    wmemset(DevMode.dmFormName, L'A', 32);
    ok(IsValidDevmodeW(&DevMode, DevMode.dmSize), "IsValidDevmodeW returns FALSE!\n");
    ok(DevMode.dmDeviceName[31] == L'\0', "Expected dmDeviceName to be '\\0', but is '%C'\n", DevMode.dmDeviceName[31]);
    ok(DevMode.dmFormName[31] == L'\0', "Expected dmFormName to be '\\0', but is '%C'\n", DevMode.dmDeviceName[31]);

    // Depending on which fields are given in dmFields, the minimum value for dmSize is different.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));

    do
    {
        DevMode.dmFields = pTable->dwField;
        DevMode.dmSize = pTable->wSize;

        // This size should succeed!
        ok(IsValidDevmodeW(&DevMode, DevMode.dmSize), "IsValidDevmodeW returns FALSE!\n");

        // One byte less should not succeed!
        DevMode.dmSize--;
        ok(!IsValidDevmodeW(&DevMode, DevMode.dmSize), "IsValidDevmodeW returns TRUE!\n");

        pTable++;
    }
    while (pTable->dwField);
}
