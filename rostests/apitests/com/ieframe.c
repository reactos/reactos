/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for ieframe classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ShellWindows),
        {
            {  -0xa0,   &IID_IMarshal },
            {  -0x20,   &IID_IClientSecurity },
            {    0x0,   &IID_IMultiQI },
            {    0x0,       &IID_IUnknown },
            { FARAWY,   &IID_IShellWindows },
            { FARAWY,   &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_CURLSearchHook),
        {
            {    0x0,   &IID_IURLSearchHook2 },
            {    0x0,       &IID_IURLSearchHook },
            {    0x0,           &IID_IUnknown },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(ieframe)
{
    TestClasses(L"ieframe", ExpectedInterfaces, ExpectedInterfaceCount);
}
