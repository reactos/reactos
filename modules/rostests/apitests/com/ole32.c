/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         COM interface test for ole32 classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedOle32Interfaces[] =
{
    {
        ID_NAME(CLSID_StdComponentCategoriesMgr),
        {
            {    0x0,   &IID_IUnknown },
            { FARAWY,   &IID_ICatRegister },
            { FARAWY,   &IID_ICatInformation },
        },
        L"Both"
    },
};
static const INT ExpectedOle32InterfaceCount = RTL_NUMBER_OF(ExpectedOle32Interfaces);

START_TEST(ole32)
{
    TestClasses(L"ole32", ExpectedOle32Interfaces, ExpectedOle32InterfaceCount);
}
