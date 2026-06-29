/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         COM interface test for netshell classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    /* CLSID_ConnectionCommonUi has two entries here because the
     * threading model changed between Windows versions. */
    {
        ID_NAME(CLSID_ConnectionCommonUi, NTDDI_MIN, NTDDI_WS03),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ConnectionCommonUi, NTDDI_VISTA, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_INetLanConnectionUiInfo },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IMarshal2 },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IMarshal },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IClientSecurity },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IRpcOptions },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_ICallFactory },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IForegroundTransfer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IMultiQI },
        },
        L"Free"
    },
    {
        ID_NAME(CLSID_NetworkConnections, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolderViewCB },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ConnectionFolderEnum, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IEnumIDList },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ConnectionTray, NTDDI_MIN, NTDDI_WIN7SP1),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_DialupConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_DirectConnectionUi, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_INetConnectionPropertyUi2 },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InboundConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_LanConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetLanConnectionUiInfo },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_NetConnectionUiUtilities, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_SharedAccessConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_PPPoEUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_VpnConnectionUi, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionConnectUi },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INetConnectionPropertyUi2 },
        },
        L"Both"
    },
};

START_TEST(netshell)
{
    TestClasses(L"netshell", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
