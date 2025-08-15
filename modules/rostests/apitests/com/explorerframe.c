/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for explorerframe server
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_TaskbarList, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7, NTDDI_MAX, &IID_ITaskbarList3 },
            { NTDDI_WIN7, NTDDI_MAX, &IID_ITaskbarList4 },
            { NTDDI_WIN7, NTDDI_MAX, &IID_ITaskbarList2 },
            { NTDDI_WIN7, NTDDI_MAX, &IID_ITaskbarList },
            { NTDDI_WIN7, NTDDI_MAX, &IID_IUnknown },
        }
    },
};

START_TEST(explorerframe)
{
    TestClassesEx(L"explorerframe", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces), NTDDI_WIN7, NTDDI_MAX);
}