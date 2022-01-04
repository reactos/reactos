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
        ID_NAME(CLSID_CERTMGR_CertMgrAboutObject_1),
        {
            {    0x0,   &IID_ISnapInAbout },
            {    0x0,       &IID_IUnknown },
        },
        L"both"
    },
    {
        ID_NAME(CLSID_CERTMGR_CertMgrObject_1),
        {
            {  -0x1c,   &IID_IComponentData },
            {    0x0,   &IID_IExtendPropertySheet },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistStream },
        },
        L"both"
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(certmgr)
{
    TestClasses(L"certmgr", ExpectedInterfaces, ExpectedInterfaceCount);
}
