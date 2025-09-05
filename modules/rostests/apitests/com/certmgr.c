/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     COM interface test for certmgr classes
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_CERTMGR_CertMgrAboutObject_1, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ISnapInAbout },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"both"
    },
    {
        ID_NAME(CLSID_CERTMGR_CertMgrObject_1, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IComponentData },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IExtendPropertySheet },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
        },
        L"both"
    },
};

START_TEST(certmgr)
{
    TestClasses(L"certmgr", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
