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
        ID_NAME(CLSID_StdComponentCategoriesMgr),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            { FARAWY, FARAWY,   &IID_ICatRegister },
            { FARAWY, FARAWY,   &IID_ICatInformation },
        },
        L"Both"
    },
};

START_TEST(ole32)
{
    TestClasses(L"ole32", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
