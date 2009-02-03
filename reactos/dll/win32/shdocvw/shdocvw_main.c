/*
 * SHDOCVW - Internet Explorer Web Control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define COM_NO_WINDOWS_H

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "shlwapi.h"

#include "shdocvw.h"
#include "uuids.h"
#include "urlmon.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

LONG SHDOCVW_refCount = 0;

static const WCHAR szMozDlPath[] = {
    'S','o','f','t','w','a','r','e','\\','R','e','a','c','t','O','S','\\',
    's','h','d','o','c','v','w',0
};

DEFINE_GUID( CLSID_MozillaBrowser, 0x1339B54C,0x3453,0x11D2,0x93,0xB9,0x00,0x00,0x00,0x00,0x00,0x00);

typedef HRESULT (WINAPI *fnGetClassObject)(REFCLSID rclsid, REFIID iid, LPVOID *ppv);
typedef HRESULT (WINAPI *fnCanUnloadNow)(void);

HINSTANCE shdocvw_hinstance = 0;
static HMODULE SHDOCVW_hshell32 = 0;
static HMODULE hMozCtl = (HMODULE)~0UL;


/* convert a guid to a wide character string */
static void SHDOCVW_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

static BOOL SHDOCVW_GetMozctlPath( LPWSTR szPath, DWORD sz )
{
    DWORD r, type;
    BOOL ret = FALSE;
    HKEY hkey;
    static const WCHAR szPre[] = {
        'S','o','f','t','w','a','r','e','\\',
        'C','l','a','s','s','e','s','\\',
        'C','L','S','I','D','\\',0 };
    static const WCHAR szPost[] = {
        '\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2',0 };
    WCHAR szRegPath[(sizeof(szPre)+sizeof(szPost))/sizeof(WCHAR)+40];

    strcpyW( szRegPath, szPre );
    SHDOCVW_guid2wstr( &CLSID_MozillaBrowser, &szRegPath[strlenW(szRegPath)] );
    strcatW( szRegPath, szPost );

    TRACE("key = %s\n", debugstr_w( szRegPath ) );

    r = RegOpenKeyW( HKEY_LOCAL_MACHINE, szRegPath, &hkey );
    if( r != ERROR_SUCCESS )
        return FALSE;

    r = RegQueryValueExW( hkey, NULL, NULL, &type, (LPBYTE)szPath, &sz );
    ret = ( r == ERROR_SUCCESS ) && ( type == REG_SZ );
    RegCloseKey( hkey );

    return ret;
}

/*************************************************************************
 * SHDOCVW DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("%p 0x%lx %p\n", hinst, fdwReason, fImpLoad);
	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
	    shdocvw_hinstance = hinst;
	    break;
	  case DLL_PROCESS_DETACH:
	    if (SHDOCVW_hshell32) FreeLibrary(SHDOCVW_hshell32);
	    if (hMozCtl && hMozCtl != (HMODULE)~0UL) FreeLibrary(hMozCtl);
	    break;
	}
	return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (SHDOCVW.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    HRESULT moz_can_unload = S_OK;
    fnCanUnloadNow pCanUnloadNow;

    if (hMozCtl)
    {
        pCanUnloadNow = (fnCanUnloadNow)
            GetProcAddress(hMozCtl, "DllCanUnloadNow");
        if (pCanUnloadNow)
            moz_can_unload = pCanUnloadNow();
    }

    if (moz_can_unload == S_OK && SHDOCVW_refCount == 0)
        return S_OK;

    return S_FALSE;
}

/*************************************************************************
 *              SHDOCVW_TryDownloadMozillaControl
 */
typedef struct _IBindStatusCallbackImpl {
    const IBindStatusCallbackVtbl *vtbl;
    LONG ref;
    HWND hDialog;
    BOOL *pbCancelled;
} IBindStatusCallbackImpl;

static HRESULT WINAPI
dlQueryInterface( IBindStatusCallback* This, REFIID riid, void** ppvObject )
{
    if (ppvObject == NULL) return E_POINTER;

    if( IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef( This );
        *ppvObject = This;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI dlAddRef( IBindStatusCallback* iface )
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;

    SHDOCVW_LockModule();

    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dlRelease( IBindStatusCallback* iface )
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
    DWORD ref = InterlockedDecrement( &This->ref );

    if( !ref )
    {
        DestroyWindow( This->hDialog );
        HeapFree( GetProcessHeap(), 0, This );
    }

    SHDOCVW_UnlockModule();

    return ref;
}

static HRESULT WINAPI
dlOnStartBinding( IBindStatusCallback* iface, DWORD dwReserved, IBinding* pib)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlGetPriority( IBindStatusCallback* iface, LONG* pnPriority)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlOnLowResource( IBindStatusCallback* iface, DWORD reserved)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlOnProgress( IBindStatusCallback* iface, ULONG ulProgress,
              ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
    HWND hItem;
    LONG r;

    hItem = GetDlgItem( This->hDialog, 1000 );
    if( hItem && ulProgressMax )
        SendMessageW(hItem,PBM_SETPOS,(ulProgress*100)/ulProgressMax,0);

    hItem = GetDlgItem(This->hDialog, 104);
    if( hItem )
        SendMessageW(hItem,WM_SETTEXT, 0, (LPARAM) szStatusText);

    SetLastError(0);
    r = GetWindowLongPtrW( This->hDialog, GWLP_USERDATA );
    if( r || GetLastError() )
    {
        *This->pbCancelled = TRUE;
        ERR("Cancelled\n");
        return E_ABORT;
    }

    return S_OK;
}

static HRESULT WINAPI
dlOnStopBinding( IBindStatusCallback* iface, HRESULT hresult, LPCWSTR szError)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlGetBindInfo( IBindStatusCallback* iface, DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlOnDataAvailable( IBindStatusCallback* iface, DWORD grfBSCF,
                   DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ERR("\n");
    return S_OK;
}

static HRESULT WINAPI
dlOnObjectAvailable( IBindStatusCallback* iface, REFIID riid, IUnknown* punk)
{
    ERR("\n");
    return S_OK;
}

static const IBindStatusCallbackVtbl dlVtbl =
{
    dlQueryInterface,
    dlAddRef,
    dlRelease,
    dlOnStartBinding,
    dlGetPriority,
    dlOnLowResource,
    dlOnProgress,
    dlOnStopBinding,
    dlGetBindInfo,
    dlOnDataAvailable,
    dlOnObjectAvailable
};

static IBindStatusCallback* create_dl(HWND dlg, BOOL *pbCancelled)
{
    IBindStatusCallbackImpl *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    This->vtbl = &dlVtbl;
    This->ref = 1;
    This->hDialog = dlg;
    This->pbCancelled = pbCancelled;

    return (IBindStatusCallback*) This;
}

static DWORD WINAPI ThreadFunc( LPVOID info )
{
    IBindStatusCallback *dl;
    static const WCHAR szUrlVal[] = {'M','o','z','i','l','l','a','U','r','l',0};
    static const WCHAR szFileProtocol[] = {'f','i','l','e',':','/','/','/',0};
    WCHAR path[MAX_PATH], szUrl[MAX_PATH];
    LPWSTR p;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HWND hDlg = info;
    DWORD r, sz, type;
    HKEY hkey;
    BOOL bCancelled = FALSE;
    BOOL bTempfile = FALSE;

    /* find the name of the thing to download */
    szUrl[0] = 0;
    /* @@ Wine registry key: HKCU\Software\Wine\shdocvw */
    r = RegOpenKeyW( HKEY_LOCAL_MACHINE, szMozDlPath, &hkey );
    if( r == ERROR_SUCCESS )
    {
        sz = MAX_PATH;
        r = RegQueryValueExW( hkey, szUrlVal, NULL, &type, (LPBYTE)szUrl, &sz );
        RegCloseKey( hkey );
    }
    if( r != ERROR_SUCCESS )
        goto end;

    if( !strncmpW(szUrl, szFileProtocol, strlenW(szFileProtocol)) )
        lstrcpynW( path, szUrl+strlenW(szFileProtocol), MAX_PATH );
    else
    {
        /* built the path for the download */
        p = strrchrW( szUrl, '/' );
        if (!p)
            goto end;
        if (!GetTempPathW( MAX_PATH, path ))
            goto end;
        strcatW( path, p+1 );

        /* download it */
        bTempfile = TRUE;
        dl = create_dl(info, &bCancelled);
        r = URLDownloadToFileW( NULL, szUrl, path, 0, dl );
        if( dl )
            IBindStatusCallback_Release( dl );
        if( (r != S_OK) || bCancelled )
            goto end;
    }

    /* run it */
    memset( &si, 0, sizeof si );
    si.cb = sizeof si;
    r = CreateProcessW( path, NULL, NULL, NULL, 0, 0, NULL, NULL, &si, &pi );
    if( !r )
        goto end;
    WaitForSingleObject( pi.hProcess, INFINITE );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

end:
    if( bTempfile )
        DeleteFileW( path );
    EndDialog( hDlg, 0 );
    return 0;
}

static INT_PTR CALLBACK
dlProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hThread;
    DWORD ThreadId;
    HWND hItem;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW( hwndDlg, GWLP_USERDATA, 0 );
        hItem = GetDlgItem(hwndDlg, 1000);
        if( hItem )
        {
            SendMessageW(hItem,PBM_SETRANGE,0,MAKELPARAM(0,100));
            SendMessageW(hItem,PBM_SETPOS,0,0);
        }
        hThread = CreateThread(NULL,0,ThreadFunc,hwndDlg,0,&ThreadId);
        if (!hThread)
            return FALSE;
        return TRUE;
    case WM_COMMAND:
        if( wParam == IDCANCEL )
            SetWindowLongPtrW( hwndDlg, GWLP_USERDATA, 1 );
        return FALSE;
    default:
        return FALSE;
    }
}

static BOOL SHDOCVW_TryDownloadMozillaControl(void)
{
    DWORD r;
    WCHAR buf[0x100];
    static const WCHAR szTitle[] = { 'R','e','a','c','t','O','S',0 };
    HANDLE hsem;
	BOOL ret = TRUE;

    SetLastError( ERROR_SUCCESS );
    hsem = CreateSemaphoreA( NULL, 0, 1, "mozctl_install_semaphore");
    if( GetLastError() != ERROR_ALREADY_EXISTS )
    {
        LoadStringW( shdocvw_hinstance, 1001, buf, sizeof buf/sizeof(WCHAR) );
        r = MessageBoxW(NULL, buf, szTitle, MB_YESNO | MB_ICONQUESTION);
        if( r == IDYES )
			DialogBoxW(shdocvw_hinstance, MAKEINTRESOURCEW(100), 0, dlProc);
		else
			ret = FALSE;
    }
    else
        WaitForSingleObject( hsem, INFINITE );

    ReleaseSemaphore( hsem, 1, NULL );
    CloseHandle( hsem );

    return ret;
}

static BOOL SHDOCVW_TryLoadMozillaControl(void)
{
    WCHAR szPath[MAX_PATH];
    BOOL bTried = FALSE;

    if( hMozCtl != (HMODULE)~0UL )
        return hMozCtl ? TRUE : FALSE;

    while( 1 )
    {
        if( SHDOCVW_GetMozctlPath( szPath, sizeof szPath ) )
        {
            hMozCtl = LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if( hMozCtl )
                return TRUE;
        }
        if( bTried )
        {
            MESSAGE("You need to install the Mozilla ActiveX control to\n");
            MESSAGE("use ReactOS's builtin CLSID_WebBrowser from SHDOCVW.DLL\n");
            return FALSE;
        }
        SHDOCVW_TryDownloadMozillaControl();
        bTried = TRUE;
    }
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("\n");

    if( IsEqualGUID( &CLSID_WebBrowser, rclsid ) &&
        SHDOCVW_TryLoadMozillaControl() )
    {
        HRESULT r;
        fnGetClassObject pGetClassObject;

        TRACE("WebBrowser class %s\n", debugstr_guid(rclsid) );

        pGetClassObject = (fnGetClassObject)
            GetProcAddress( hMozCtl, "DllGetClassObject" );

        if( !pGetClassObject )
            return CLASS_E_CLASSNOTAVAILABLE;
        r = pGetClassObject( &CLSID_MozillaBrowser, riid, ppv );

        TRACE("r = %08lx  *ppv = %p\n", r, *ppv );

        return r;
    }

    if (IsEqualCLSID(&CLSID_WebBrowser, rclsid) &&
        IsEqualIID(&IID_IClassFactory, riid))
    {
        /* Pass back our shdocvw class factory */
        *ppv = (LPVOID)&SHDOCVW_ClassFactory;
        IClassFactory_AddRef((IClassFactory*)&SHDOCVW_ClassFactory);

        return S_OK;
    }

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

/***********************************************************************
 *              DllGetVersion (SHDOCVW.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}

/*************************************************************************
 *              DllInstall (SHDOCVW.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;
}

/*************************************************************************
 * SHDOCVW_LoadShell32
 *
 * makes sure the handle to shell32 is valid
 */
 BOOL SHDOCVW_LoadShell32(void)
{
     if (SHDOCVW_hshell32)
       return TRUE;
     return ((SHDOCVW_hshell32 = LoadLibraryA("shell32.dll")) != NULL);
}

/***********************************************************************
 *		@ (SHDOCVW.110)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI WinList_Init(void)
{
    FIXME("(), stub!\n");
    return 0x0deadfeed;
}

/***********************************************************************
 *		@ (SHDOCVW.118)
 *
 * Called by Win98 explorer.exe main binary, definitely has only one
 * parameter.
 */
static BOOL (WINAPI *pShellDDEInit)(BOOL start) = NULL;

BOOL WINAPI ShellDDEInit(BOOL start)
{
    TRACE("(%d)\n", start);

    if (!pShellDDEInit)
    {
      if (!SHDOCVW_LoadShell32())
        return FALSE;
      pShellDDEInit = GetProcAddress(SHDOCVW_hshell32, (LPCSTR)188);
    }

    if (pShellDDEInit)
      return pShellDDEInit(start);
    else
      return FALSE;
}

/***********************************************************************
 *		@ (SHDOCVW.125)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI RunInstallUninstallStubs(void)
{
    FIXME("(), stub!\n");
    return 0x0deadbee;
}

/***********************************************************************
 *              SetQueryNetSessionCount (SHDOCVW.@)
 */
DWORD WINAPI SetQueryNetSessionCount(DWORD arg)
{
    FIXME("(%lu), stub!\n", arg);
    return 0;
}
