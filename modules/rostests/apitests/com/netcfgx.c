/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         COM interface test for netcfgx classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_CNetCfg),
        {
            {    0x0,    0x0,   &IID_INetCfg },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_INetCfgLock },
            {   0x10,   0x20,   &IID_INetCfgPnpReconfigCallback },
        },
        L"Both"
    },
};

START_TEST(netcfgx)
{
    TestClasses(L"netcfgx", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
