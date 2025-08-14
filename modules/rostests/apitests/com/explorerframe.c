/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for explorerframe server
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win7[] =
{
    {
        ID_NAME(CLSID_TaskbarList),
        {
            {    0x0,    0x0,   &IID_ITaskbarList3 },
            {    0x0,    0x0,       &IID_ITaskbarList4 },
            {    0x0,    0x0,           &IID_ITaskbarList2 },
            {    0x0,    0x0,               &IID_ITaskbarList },
            {    0x0,    0x0,                   &IID_IUnknown },
        }
    },
};

START_TEST(explorerframe)
{
    if (GetNTVersion() <= _WIN32_WINNT_VISTA)
        skip("No explorerframe tests for Vista and older!\n");
    else
        TestClasses(L"explorerframe", ExpectedInterfaces_Win7, RTL_NUMBER_OF(ExpectedInterfaces_Win7));
}