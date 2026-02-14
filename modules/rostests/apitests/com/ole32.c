/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         COM interface test for ole32 classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_StdComponentCategoriesMgr, NTDDI_MIN, NTDDI_WIN7SP1),
        {
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_ICatRegister },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_ICatInformation },
        },
        L"Both"
    },
};

START_TEST(ole32)
{
    TestClasses(L"ole32", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
