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
        ID_NAME(CLSID_CNetCfg, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetCfg },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetCfgLock },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetCfgPnpReconfigCallback },
        },
        L"Both"
    },
};

START_TEST(netcfgx)
{
    if (GetNTVersion() < _WIN32_WINNT_WIN10)
        TestClasses(L"netcfgx", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
    else
        TestClasses(L"NetSetupShim", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
