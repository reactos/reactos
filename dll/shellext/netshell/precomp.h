#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <devguid.h>
#include <netcon.h>
#include <shlguid_undoc.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shell);

#include "resource.h"

#if defined(_MSC_VER) && _MSC_VER < 1700
#define final sealed
#endif

#define NCF_VIRTUAL                     0x1
#define NCF_SOFTWARE_ENUMERATED         0x2
#define NCF_PHYSICAL                    0x4
#define NCF_HIDDEN                      0x8
#define NCF_NO_SERVICE                  0x10
#define NCF_NOT_USER_REMOVABLE          0x20
#define NCF_MULTIPORT_INSTANCED_ADAPTER 0x40
#define NCF_HAS_UI                      0x80
#define NCF_FILTER                      0x400
#define NCF_NDIS_PROTOCOL               0x4000

typedef struct {
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

typedef struct tagVALUEStruct
{
    BYTE dummy;
    INetConnection * pItem;
} VALUEStruct;

/* globals */
extern HINSTANCE netshell_hInstance;

/* shfldr_netconnect.c */
HRESULT ShowNetConnectionProperties(INetConnection * pNetConnect, HWND hwnd);
HRESULT WINAPI ISF_NetConnect_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* enumlist.c */
PITEMID_CHILD _ILCreateNetConnect(void);
PITEMID_CHILD ILCreateNetConnectItem(INetConnection *pItem);
BOOL _ILIsNetConnect(LPCITEMIDLIST pidl);
const VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl);

/* classfactory.c */
HRESULT IClassFactory_fnConstructor(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut);

/* connectmanager.c */
HRESULT WINAPI INetConnectionManager_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);
BOOL GetAdapterIndexFromNetCfgInstanceId(PIP_ADAPTER_INFO pAdapterInfo, LPWSTR szNetCfg, PDWORD pIndex);

/* lanconnectui.c */
HPROPSHEETPAGE InitializePropertySheetPage(LPWSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);
HRESULT WINAPI LanConnectUI_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);

/* lanstatusui.c */
HRESULT WINAPI LanConnectStatusUI_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);

#define NCCF_NOTIFY_DISCONNECTED 0x100000

#include "enumlist.h"

#endif /* _PRECOMP_H__ */
