/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for prnfldr server
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win7[] =
{
    {
        ID_NAME(CLSID_Printers),
        {
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x8,   0x10,   &IID_IRemoteComputer },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x18,   0x30,   &IID_IContextMenuCB },
            {   0x20,   0x40,   &IID_IResolveShellLink },
            {   0x24,   0x48,   &IID_IFolderType },
            {   0x2c,   0x58,   &IID_IObjectWithBackReferences },
        }
    },
};

START_TEST(prnfldr)
{
    if (GetNTVersion() <= _WIN32_WINNT_VISTA)
        skip("No prnfldr class tests for Vista and older!\n");
    else
        TestClasses(L"prnfldr", ExpectedInterfaces_Win7, RTL_NUMBER_OF(ExpectedInterfaces_Win7));
}