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

#include "wine/debug.h"
#include "wine/unicode.h"
#include "resource.h"

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
    DWORD dwType;
    DWORD dwOperStatus;
    DWORD dwNameLength;
    WCHAR szName[1];
}VALUEStruct;

/* globals */
extern HINSTANCE netshell_hInstance;
extern const GUID CLSID_NetworkConnections;
extern const GUID GUID_DEVCLASS_NET;


/* shfldr_netconnect.c */
HRESULT WINAPI ISF_NetConnect_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* enumlist.c */
IEnumIDList * IEnumIDList_Constructor(void);
LPITEMIDLIST _ILCreateNetConnect();
LPITEMIDLIST ILCreateNetConnectItem(MIB_IFROW * pRow, LPWSTR szName, LPWSTR szAdapterName);
BOOL _ILIsNetConnect (LPCITEMIDLIST pidl);
BOOL AddToEnumList(IEnumIDList * iface, LPITEMIDLIST pidl);
VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl);

/* classfactory.c */
IClassFactory * IClassFactory_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

#endif
