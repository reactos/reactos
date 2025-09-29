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
        ID_NAME(CLSID_Printers, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IRemoteComputer },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IResolveShellLink },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithBackReferences },
        }
    },
};

START_TEST(prnfldr)
{
    TestClassesEx(L"prnfldr",
                  ExpectedInterfaces_Win7, RTL_NUMBER_OF(ExpectedInterfaces_Win7),
                  NTDDI_WIN7, NTDDI_MAX,
                  FALSE);
}
