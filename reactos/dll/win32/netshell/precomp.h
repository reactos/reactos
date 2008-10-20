#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <malloc.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>

#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shldisp.h>
#include <cpl.h>
#include <objbase.h>
#include <ole2.h>
#include <ocidl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <olectl.h>
#include <iphlpapi.h>
#include <shtypes.h>
#include <setupapi.h>
#include <devguid.h>
#include <netcon.h>
#include <docobj.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <prsht.h>


#include "wine/debug.h"
#include "wine/unicode.h"
#include "resource.h"

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

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
typedef struct {
    REFIID riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;

typedef struct tagVALUEStruct
{
    BYTE dummy;
    INetConnection * pItem;
}VALUEStruct;

/* globals */
extern HINSTANCE netshell_hInstance;
extern const GUID CLSID_NetworkConnections;
extern const GUID CLSID_LANConnectUI;
extern const GUID GUID_DEVCLASS_NET;


/* shfldr_netconnect.c */
HRESULT WINAPI ISF_NetConnect_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* enumlist.c */
IEnumIDList * IEnumIDList_Constructor(void);
LPITEMIDLIST _ILCreateNetConnect();
LPITEMIDLIST ILCreateNetConnectItem(INetConnection * pItem);
BOOL _ILIsNetConnect (LPCITEMIDLIST pidl);
BOOL AddToEnumList(IEnumIDList * iface, LPITEMIDLIST pidl);
VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl);

/* classfactory.c */
IClassFactory * IClassFactory_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* connectmanager.c */
HRESULT WINAPI INetConnectionManager_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* lanconnectui.c */
HRESULT WINAPI LanConnectUI_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* lanstatusui.c */
HRESULT WINAPI LanConnectStatusUI_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

#define NCCF_NOTIFY_DISCONNECTED 0x100000

#endif
