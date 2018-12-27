#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winreg.h>
#include <winnls.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <devguid.h>
#include <netcon.h>
#include <shlguid_undoc.h>
#include <prsht.h>
#include <undocshell.h>
#include <shellutils.h>

#include <netcfgx.h>
#include <netcfgn.h>
#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shell);

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

#define USE_CUSTOM_CONMGR 1

/* globals */
extern HINSTANCE netshell_hInstance;

/* enumlist.c */
typedef struct tagNETCONIDSTRUCT
{
    BYTE             type;
    GUID             guidId;
    NETCON_STATUS    Status;
    NETCON_MEDIATYPE MediaType;
    DWORD            dwCharacter;
    ULONG_PTR        uNameOffset;
    ULONG_PTR        uDeviceNameOffset;
} NETCONIDSTRUCT, *PNETCONIDSTRUCT;

PNETCONIDSTRUCT ILGetConnData(PCITEMID_CHILD pidl);
PWCHAR ILGetConnName(PCITEMID_CHILD pidl);
PWCHAR ILGetDeviceName(PCITEMID_CHILD pidl);
PITEMID_CHILD ILCreateNetConnectItem(INetConnection * pItem);
HRESULT ILGetConnection(PCITEMID_CHILD pidl, INetConnection ** pItem);
HRESULT CEnumIDList_CreateInstance(HWND hwndOwner, DWORD dwFlags, REFIID riid, LPVOID * ppv);

#define NCCF_NOTIFY_DISCONNECTED 0x100000

HPROPSHEETPAGE InitializePropertySheetPage(LPWSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);

#include "connectmanager.h"
#include "lanconnectui.h"
#include "lanstatusui.h"
#include "shfldr_netconnect.h"


#endif /* _PRECOMP_H__ */
