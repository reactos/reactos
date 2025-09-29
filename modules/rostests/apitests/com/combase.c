/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for prnfldr server
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_StdComponentCategoriesMgr, NTDDI_WIN8, NTDDI_MAX),
        {
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_ICatRegister },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_ICatInformation },
        },
        L"Both"
    },
};

START_TEST(combase)
{
    TestClassesEx(L"combase",
                  ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces),
                  NTDDI_WIN8, NTDDI_MAX,
                  FALSE);
}
