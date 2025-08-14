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
        ID_NAME(CLSID_StdComponentCategoriesMgr),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            { FARAWY, FARAWY,   &IID_ICatRegister },
            { FARAWY, FARAWY,   &IID_ICatInformation },
        },
        L"Both"
    },
};

START_TEST(combase)
{
    if (GetNTVersion() <= _WIN32_WINNT_WIN7)
        skip("No expected interfaces for combase on Windows 7 and older!\n");
    else
        TestClasses(L"combase", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
