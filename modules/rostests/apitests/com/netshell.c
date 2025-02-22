/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         COM interface test for netshell classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ConnectionCommonUi),
        {
            {    0x0,   &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_NetworkConnections),
        {
            {    0x0,   &IID_IPersistFolder2 },
            {    0x0,       &IID_IPersistFolder },
            {    0x0,           &IID_IPersist },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
            {    0x8,   &IID_IShellFolder2 },
            {    0x8,       &IID_IShellFolder },
            {    0xc,   &IID_IOleCommandTarget },
            {   0x10,   &IID_IShellFolderViewCB },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ConnectionFolderEnum),
        {
            {    0x0,   &IID_IEnumIDList },
            {    0x0,       &IID_IUnknown },
        },
        L"Both"
    },
#if 0
    {
        ID_NAME(CLSID_ConnectionManager),
        {
            {    0x0,   &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_ConnectionTray),
        {
            {    0x0,   &IID_IOleCommandTarget },
            {    0x0,       &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_DialupConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            //{    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_DirectConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            //{    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InboundConnectionUi),
        {
            {    0x0,   &IID_INetConnectionPropertyUi2 },
            {    0x0,       &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            //{    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_LanConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            {    0x4,       &IID_INetConnectionPropertyUi },
            {   0x10,   &IID_INetLanConnectionUiInfo },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_NetConnectionUiUtilities),
        {
            {    0x0,   &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_SharedAccessConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            {    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_PPPoEUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            //{    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_VpnConnectionUi),
        {
            {    0x0,   &IID_INetConnectionConnectUi },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_INetConnectionPropertyUi2 },
            //{    0x4,       &IID_INetConnectionPropertyUi },
        },
        L"Both"
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(netshell)
{
    TestClasses(L"netshell", ExpectedInterfaces, ExpectedInterfaceCount);
}
