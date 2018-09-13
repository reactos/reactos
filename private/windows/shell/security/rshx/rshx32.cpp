/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rshx32.cpp

Abstract:

    Remote administration shell extension.

Author:

    Don Ryan (donryan) 23-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define INC_OLE2
#include <windows.h>
#include <initguid.h>
#include <commctrl.h>
#include <comctrlp.h>
#define NO_SHLWAPI_PATH
#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wfext.h>
#include <winnetwk.h>
#include <help.h>   // For help IDs of everything in windows.hlp
extern "C"
{
    #include "dfsext.h"
}
#include "rshx32.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HINSTANCE        g_hInstance = NULL;
#ifndef WINNT
// No prtq32.dll on NT
HINSTANCE        g_hPrtQ32Dll = NULL;
#endif
HINSTANCE        g_hNet32Dll = NULL;
HINSTANCE        g_hAclEditDll = NULL;
HINSTANCE        g_hShell32Dll = NULL;
UINT             g_cRefThisDll = 0;
CLIPFORMAT       g_cfNetResource = 0;
ATOM             g_atomFMXWndClass = 0;
#ifndef WINNT
// No prtq32.dll on NT
EDITQACL         g_pfnEditQACL = NULL;
EDITQACL2        g_pfnEditQACL2 = NULL;
#endif
SHGETNETRESOURCE g_pfnSHGetNetResource = NULL;
SHDRAGQUERYFILEA g_pfnSHDragQueryFileA = NULL;
#ifdef UNICODE
SHDRAGQUERYFILEW g_pfnSHDragQueryFileW = NULL;
#endif
NETUSEGETINFO    g_pfnNetUseGetInfo = NULL;
NETSERVERGETINFO g_pfnNetServerGetInfo = NULL;
#ifdef WINNT
NETAPIBUFFERFREE g_pfnNetApiBufferFree = NULL;
#endif
ACLEDITPROC      g_apfnAclEditExps[] = { NULL, NULL, NULL };

#pragma data_seg(".text")
const static DWORD aSecurityPageHelpIDs[] = {  // Context Help IDs
    IDC_PERMS,   IDH_FPROP_SECURITY_PERMISSIONS,
    IDC_AUDIT,   IDH_FPROP_SECURITY_AUDITING,
    IDC_OWNER,   IDH_FPROP_SECURITY_OWNERSHIP,
    0, 0
};
#pragma data_seg()

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
AclEditDlgProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

UINT CALLBACK
AclEditPrshtCallback(
    HWND    hwnd,
    UINT    uMsg,
    LPPROPSHEETPAGE ppsp
    );

#ifndef WINNT
// No prtq32.dll on NT
VOID
DoEditQACL(
    HWND            hDlg,
    UINT            fnId,
    PRPROPSHEETPAGE rpsp
    );
#endif

VOID
DoEditFSACL(
    HWND            hDlg,
    UINT            fnId,
    PRPROPSHEETPAGE rpsp
    );

ATOM
RegisterFMXWndClass(
    );

LRESULT CALLBACK
FMXWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
GetServerFlags(
    LPTSTR  lpPath,
    LPDWORD lpFlags
    );

#if DBG
VOID
DumpNetResource(
    LPNETRESOURCE lpNetResource
    );
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// General routines                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern "C"
int APIENTRY
InitializeDLL(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpReserved
    )

/*++

Routine Description:

    Dll's entry point.

    In order to service requests for file selection information from
    any of the file manager extensions to be included in this library,
    we must first register a window class to accept these requests.

    The Microsoft_Network provider transfers information via a private
    clipboard format called "Net Resource" which we must register.

Arguments:

    Same as DllEntryPoint.

Return Values:

    Same as DllEntryPoint.

--*/

{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance; // instance handle...

        if (g_hShell32Dll = LoadLibrary(DLL_SHELL32))
        {
            g_pfnSHGetNetResource = (SHGETNETRESOURCE)GetProcAddress(
                                                        g_hShell32Dll,
                                                        EXP_SHGETNETRESOURCE
                                                        );

            g_pfnSHDragQueryFileA = (SHDRAGQUERYFILEA)GetProcAddress(
                                                        g_hShell32Dll,
                                                        EXP_SHDRAGQUERYFILEA
                                                        );
#ifdef UNICODE
            g_pfnSHDragQueryFileW = (SHDRAGQUERYFILEW)GetProcAddress(
                                                        g_hShell32Dll,
                                                        EXP_SHDRAGQUERYFILEW
                                                        );
#endif

        }

        if (!(g_pfnSHGetNetResource && g_pfnSHDragQueryFileA
#ifdef UNICODE
                && g_pfnSHDragQueryFileW
#endif
        )) {
            if (g_hShell32Dll)
                FreeLibrary(g_hShell32Dll);

            return FALSE; // bail...
        }

        g_cfNetResource = RegisterClipboardFormat(CF_NETRESOURCE);
        g_atomFMXWndClass = RegisterFMXWndClass();

        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifndef WINNT
        // No prtq32.dll on NT
        if (g_hPrtQ32Dll)
            FreeLibrary(g_hPrtQ32Dll);
#endif
        if (g_hNet32Dll)
            FreeLibrary(g_hNet32Dll);

        if (g_hShell32Dll)
            FreeLibrary(g_hShell32Dll);

        if (g_hAclEditDll)
            FreeLibrary(g_hAclEditDll);

        UnregisterClass((LPTSTR)MAKELONG(g_atomFMXWndClass, 0), g_hInstance);
    }

    return TRUE;
}


STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID   riid,
    LPVOID*  ppv
    )

/*++

Routine Description:

    Called by shell to create a class factory object.

Arguments:

    rclsid - reference to class id specifier.
    riid   - reference to interface id specifier.
    ppv    - pointer to location to receive interface pointer.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    *ppv = NULL;

    if (!IsEqualCLSID(rclsid, CLSID_RShellExt))
        return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);

    CRShellExtCF *pShellExtCF = new CRShellExtCF();

    if (!pShellExtCF)
        return ResultFromScode(E_OUTOFMEMORY);

    HRESULT hr = pShellExtCF->QueryInterface(riid, ppv);
    pShellExtCF->Release();

    return hr;
}


STDAPI
DllCanUnloadNow(
    )

/*++

Routine Description:

    Called by shell to find out if dll can be unloaded.

Arguments:

    None.

Return Values:

    Returns S_OK if dll can be unloaded, S_FALSE if not.

--*/

{
    return ResultFromScode((g_cRefThisDll == 0) ? S_OK : S_FALSE);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRShellExtCF::CRShellExtCF()

/*++

Routine Description:

    Constructor.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_cRef = 1;
    g_cRefThisDll++;
}


CRShellExtCF::~CRShellExtCF()

/*++

Routine Description:

    Destructor.

Arguments:

    None.

Return Values:

    None.

--*/

{
    g_cRefThisDll--;
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRShellExtCF::AddRef()

/*++

Routine Description:

    Support for IUnknown::AddRef.

Arguments:

    None.

Return Values:

    Returns number of references on object.

--*/

{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CRShellExtCF::Release()

/*++

Routine Description:

    Support for IUnknown::Release.

Arguments:

    None.

Return Values:

    Returns number of references on object.

--*/

{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CRShellExtCF::QueryInterface(
    REFIID      riid,
    LPVOID FAR* ppv
    )

/*++

Routine Description:

    Support for IUnknown::QueryInterface.

Arguments:

    riid - reference to interface id specifier.
    ppv  - pointer to location to receive interface pointer.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN)(LPCLASSFACTORY)this;
        m_cRef++;
        return NOERROR;
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRShellExtCF::CreateInstance(
    LPUNKNOWN   pUnkOuter,
    REFIID      riid,
    LPVOID FAR* ppvObj
    )

/*++

Routine Description:

    Support for IClassFactory::CreateInstance.

Arguments:

    pUnkOuter - pointer to controlling unknown.
    riid      - reference to interface id specifier.
    ppvObj    - pointer to location to receive interface pointer.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    *ppvObj = NULL;

    if (pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    CRShellExt *pShellExt = new CRShellExt();

    if (!pShellExt)
        return ResultFromScode(E_OUTOFMEMORY);

    HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    pShellExt->Release();

    return hr;
}


STDMETHODIMP
CRShellExtCF::LockServer(
    BOOL fLock
    )

/*++

Routine Description:

    Support for IClassFactory::LockServer (not implemented).

Arguments:

    fLock - true if lock count to be incremented.

Return Values:

    Returns E_NOTIMPL.

--*/

{
    return ResultFromScode(E_NOTIMPL);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRShellExt::CRShellExt ()

/*++

Routine Description:

    Constructor.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_cRef = 1;
    g_cRefThisDll++;

    m_lpdobj = NULL;
}


CRShellExt::~CRShellExt ()

/*++

Routine Description:

    Destructor (release IDataObject pointer saved in Initialize).

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_lpdobj)
        m_lpdobj->Release();

    g_cRefThisDll--;
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IUnknown)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRShellExt::AddRef()

/*++

Routine Description:

    Support for IUnknown::AddRef.

Arguments:

    None.

Return Values:

    Returns number of references on object.

--*/

{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CRShellExt::Release()

/*++

Routine Description:

    Support for IUnknown::Release.

Arguments:

    None.

Return Values:

    Returns number of references on object.

--*/

{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CRShellExt::QueryInterface(
    REFIID      riid,
    LPVOID FAR* ppv
    )

/*++

Routine Description:

    Support for IUnknown::QueryInterface.

Arguments:

    riid - reference to interface id specifier.
    ppv  - pointer to location to receive interface pointer.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN)(LPSHELLPROPSHEETEXT)this;
        m_cRef++;
        return NOERROR;
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppv = (LPSHELLEXTINIT)this;
        m_cRef++;
        return NOERROR;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
        m_cRef++;
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IShellExtInit)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRShellExt::Initialize(
    LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT  lpdobj,
    HKEY          hKeyProgID
    )

/*++

Routine Description:

    Support for IShellExtInit::Initialize.

Arguments:

    pidlFolder - pointer to id list identifying parent folder.
    lpdobj     - pointer to IDataObject interface for selected object(s).
    hKeyProgId - registry key handle.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    if (m_lpdobj)
        m_lpdobj->Release();

    m_lpdobj = lpdobj; // processed in AddPages

    if (m_lpdobj)
        m_lpdobj->AddRef();

    return NOERROR;
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IShellPropSheetExt)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRShellExt::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )

/*++

Routine Description:

    Support for IShellPropSheetExt::AddPages.

Arguments:

    lpfnAddPage - pointer to function called to add a page.
    lParam      - lParam parameter to be passed to lpfnAddPage.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

{
    HRESULT hr;
    STGMEDIUM medium;
    RPROPSHEETPAGE rpsp;
    FORMATETC fe = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    if (!m_lpdobj)
        return ResultFromScode(E_FAIL);

    BOOL fAddPage = FALSE; // assume failure...

    fe.cfFormat = CF_HDROP;
    hr = m_lpdobj->GetData(&fe, &medium);

    if (SUCCEEDED(hr))
    {
        HDROP hdrop = (HDROP)medium.hGlobal;
#ifdef UNICODE
        rpsp.cItems = (*g_pfnSHDragQueryFileW)(hdrop, (UINT)-1, NULL, 0);
#else
        rpsp.cItems = (*g_pfnSHDragQueryFileA)(hdrop, (UINT)-1, NULL, 0);
#endif

        for (DWORD dw = 0; dw < rpsp.cItems; dw++)
        {
#ifdef UNICODE
            (*g_pfnSHDragQueryFileW)(
                hdrop,
                dw,
                rpsp.szBuf,
                ARRAYSIZE(rpsp.szBuf)
                );
#else
            (*g_pfnSHDragQueryFileA)(
                hdrop,
                dw,
                rpsp.szBuf,
                ARRAYSIZE(rpsp.szBuf)
                );
#endif

#ifdef WINNT
#ifdef UNICODE
            // First check to see if this is a DFS path.
            // If so, don't show the security page (since
            // acledit.dll can't handle DFS yet)

            if (IsThisADfsPath(rpsp.szBuf, NULL))
            {
                fAddPage = FALSE;
                break;
            }
#endif
#endif

            rpsp.flags = DOBJ_RES_DISK|DOBJ_CF_HDROP;
            ::GetServerFlags(rpsp.szBuf, &rpsp.flags);

            if (IsServerNT(rpsp.flags))
            {
                // volume must be NTFS (or OFS) because no
                // persistent ACLs on FAT or HPFS...
                if (IsVolumeNTACLS(rpsp.flags))
                {
                    fAddPage = TRUE;
                }
                else
                {
                    fAddPage = FALSE;
                    break;
                }
            }
            else if (IsServerOS2(rpsp.flags))
            {
                fAddPage = TRUE;
            }
            else
            {
                fAddPage = FALSE;
                break;
            }
        }

        ReleaseStgMedium(&medium);
    }
    else
    {
        fe.cfFormat = g_cfNetResource;
        hr = m_lpdobj->GetData(&fe, &medium);

        if (FAILED(hr))
            return ResultFromScode(E_FAIL); // don't support format...

        HNRES hnres = (HNRES)medium.hGlobal;
        rpsp.cItems = (*g_pfnSHGetNetResource)(hnres, (UINT)-1, NULL, 0);

        if (rpsp.cItems == 1) // only support single network resource...
        {
            LPNETRESOURCE lpNetRes = (LPNETRESOURCE)&rpsp.szBuf[0];

            (*g_pfnSHGetNetResource)(
                hnres,
                0,
                lpNetRes ,
                ARRAYSIZE(rpsp.szBuf)
                );
#if DBG
            DumpNetResource(lpNetRes);
#endif
            if ((lpNetRes->dwType == RESOURCETYPE_PRINT) &&
                (lpNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE))
            {
                rpsp.flags = DOBJ_RES_PRINT|DOBJ_CF_HNRES;
                ::GetServerFlags(lpNetRes->lpRemoteName, &rpsp.flags);

                if (IsServerNT(rpsp.flags) ||
                    IsServerOS2(rpsp.flags))
                {
                    fAddPage = TRUE;
                    lstrcpy(rpsp.szBuf, lpNetRes->lpRemoteName);
                }
            }
        }

        ReleaseStgMedium(&medium);
    }

    if (fAddPage)
    {
        HPROPSHEETPAGE hPage;

        rpsp.psp.dwSize      = SIZEOF(rpsp);
        rpsp.psp.hInstance   = g_hInstance;
        rpsp.psp.dwFlags     = PSP_USEREFPARENT|PSP_USETITLE|PSP_USECALLBACK;
        rpsp.psp.pszTemplate = MAKEINTRESOURCE(IDD_SECURITY);
        rpsp.psp.pszTitle    = MAKEINTRESOURCE(IDS_SECURITY);
        rpsp.psp.pfnDlgProc  = (DLGPROC)AclEditDlgProc;
        rpsp.psp.pfnCallback = AclEditPrshtCallback;
        rpsp.psp.pcRefParent = &g_cRefThisDll;
        rpsp.psp.lParam      = (LPARAM)m_lpdobj;

        if (hPage = ::CreatePropertySheetPage(&rpsp.psp))
        {
            if (lpfnAddPage(hPage, lParam))
                m_lpdobj->AddRef(); // just to be sure...
            else
                DestroyPropertySheetPage(hPage);
        }
    }

    return NOERROR;
}


STDMETHODIMP
CRShellExt::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM               lParam
    )

/*++

Routine Description:

    Support for IShellPropSheetExt::ReplacePages (not supported).

Arguments:

    uPageID         - page to replace.
    lpfnReplaceWith - pointer to function called to replace a page.
    lParam          - lParam parameter to be passed to lpfnReplaceWith.

Return Values:

    Returns E_FAIL.

--*/

{
    return ResultFromScode(E_FAIL);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Security property page routines                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
AclEditDlgProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for security property page.

Arguments:

    Same as DlgProc.

Return Values:

    Same as DlgProc.

--*/

{
    PRPROPSHEETPAGE rpsp = (PRPROPSHEETPAGE)GetWindowLong(hWnd, DWL_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLong(hWnd, DWL_USER, lParam);
        if (IsServerOS2(((PRPROPSHEETPAGE)lParam)->flags))
            EnableWindow(GetDlgItem(hWnd, IDC_OWNER), FALSE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_PERMS: // must be first
        case IDC_AUDIT: // must be second
        case IDC_OWNER: // must be third
#ifndef WINNT
            if (IsResPrintQ(rpsp->flags))
                DoEditQACL(hWnd, LOWORD(wParam)-IDC_PERMS, rpsp);
            else
#else
            if (!IsResPrintQ(rpsp->flags))
#endif
                DoEditFSACL(hWnd, LOWORD(wParam)-IDC_PERMS, rpsp);
        }
        break;

    case WM_NOTIFY:
        SetWindowLong(hWnd, DWL_MSGRESULT, FALSE);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, TEXT("ntsecui.hlp"),
            HELP_WM_HELP, (DWORD)(LPTSTR) aSecurityPageHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, TEXT("ntsecui.hlp"), HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aSecurityPageHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//
// Release the data object AddRef'd during the AddPage
//
UINT CALLBACK AclEditPrshtCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    PRPROPSHEETPAGE rpsp = (PRPROPSHEETPAGE)ppsp;
    LPDATAOBJECT lpdobj;

    switch (uMsg) {
        case PSPCB_RELEASE:
            lpdobj = (LPDATAOBJECT)rpsp->psp.lParam;
            lpdobj->Release(); // finished with object...
            break;
    }

    return 1;
}



#ifndef WINNT
// No prtq32.dll on NT
VOID
DoEditQACL(
    HWND            hDlg,
    UINT            fnId,
    PRPROPSHEETPAGE rpsp
    )

/*++

Routine Description:

    Wrapper around queue acl editor.

Arguments:

    hDlg - property page handle.
    fnId - procedure identifier.
    rpsp - property sheet page.

Return Values:

    None.

--*/

{
    if (IsServerNT(rpsp->flags))
    {
        if (!g_hPrtQ32Dll)
            g_hPrtQ32Dll = LoadLibrary(DLL_PRTQ32);

        if (g_hPrtQ32Dll && !g_pfnEditQACL2)
            g_pfnEditQACL2 = (EDITQACL2)GetProcAddress(
                                        g_hPrtQ32Dll,
                                        EXP_EDITQACL2
                                        );
        if (g_pfnEditQACL2)
            (*g_pfnEditQACL2)(hDlg, rpsp->szBuf, fnId);

    }
#ifndef WINNT
    // The NT ACLEDIT.DLL doesn't have this entrypoint.  After talking
    // this over, we might want to move this entry point into the
    // prtq32.dll along with the EditQACL2 function... Just my guess
    // -BobDay

    else if (IsServerOS2(rpsp->flags))
    {
        if (!g_hAclEditDll)
            g_hAclEditDll = LoadLibrary(DLL_ACLEDIT);

        if (g_hAclEditDll && !g_pfnEditQACL)
            g_pfnEditQACL = (EDITQACL)GetProcAddress(
                                        g_hAclEditDll,
                                        EXP_EDITQACL
                                        );
        if (g_pfnEditQACL)
            (*g_pfnEditQACL)(hDlg, rpsp->szBuf, (fnId == 0));
    }
#endif
}

#endif // !WINNT  No prtq32.dll on NT

VOID
DoEditFSACL(
    HWND            hDlg,
    UINT            fnId,
    PRPROPSHEETPAGE rpsp
    )

/*++

Routine Description:

    Wrapper around file system acl editor.

Arguments:

    hDlg - property page handle.
    fnId - procedure identifier.
    rpsp - property sheet page.

Return Values:

    None.

--*/

{
    if (!g_hAclEditDll)
        g_hAclEditDll = LoadLibrary(DLL_ACLEDIT);

    if (g_hAclEditDll && !g_apfnAclEditExps[fnId])
    {
        g_apfnAclEditExps[0] = (ACLEDITPROC)GetProcAddress(
                                                g_hAclEditDll,
                                                EXP_EDITPERMSINFO
                                                );

        g_apfnAclEditExps[1] = (ACLEDITPROC)GetProcAddress(
                                                g_hAclEditDll,
                                                EXP_EDITAUDITINFO
                                                );

        g_apfnAclEditExps[2] = (ACLEDITPROC)GetProcAddress(
                                                g_hAclEditDll,
                                                EXP_EDITOWNERINFO
                                                );
    }

    if (g_apfnAclEditExps[0] &&
        g_apfnAclEditExps[1] &&
        g_apfnAclEditExps[2])
    {
        HWND hFMXWnd; // Process requests in FMXWndProc...

        if (hFMXWnd = CreateWindow(
                        (LPTSTR)MAKELONG(g_atomFMXWndClass, 0),
                        TEXT(""),
                        WS_CHILD,
                        100,
                        100,
                        1,
                        1,
                        hDlg,
                        NULL,
                        g_hInstance,
                        rpsp
                        ))
        {
            (*g_apfnAclEditExps[fnId])(hFMXWnd);
            DestroyWindow(hFMXWnd);
        }
    }
}


ATOM
RegisterFMXWndClass()

/*++

Routine Description:

    Registers file manager proxy window class.

Arguments:

    None.

Return Values:

    Returns window class identifier.

--*/

{
    WNDCLASS wc;

    wc.style          = 0;
    wc.lpfnWndProc    = FMXWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = SIZEOF(RPROPSHEETPAGE*);
    wc.hInstance      = g_hInstance;
    wc.hIcon          = NULL;
    wc.hCursor        = NULL;
    wc.hbrBackground  = NULL;
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = FMXCLASSNAME;

    return RegisterClass(&wc);
}


LRESULT CALLBACK
FMXWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for file manager proxy.

Arguments:

    Same as WndProc.

Return Values:

    Same as WndProc.

--*/

{
    HRESULT hr;
    STGMEDIUM medium;
    LPDATAOBJECT lpdobj;
    FMS_GETFILESELA* pSelA;
    FMS_GETFILESELW* pSelW;
    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    PRPROPSHEETPAGE rpsp = (PRPROPSHEETPAGE)GetWindowLong(hWnd, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
            rpsp = (PRPROPSHEETPAGE)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLong(hWnd, GWL_USERDATA, (LONG)rpsp);
            break;

        case FM_GETFOCUS:
            return FMFOCUS_DIR;

        case FM_GETSELCOUNT:
        case FM_GETSELCOUNTLFN:
            return rpsp->cItems;

        case FM_GETFILESELA:
        case FM_GETFILESELLFNA:
            pSelA  = (FMS_GETFILESELA*)lParam;
            lpdobj = (LPDATAOBJECT)rpsp->psp.lParam;

            hr = lpdobj->GetData(&fe, &medium);

            if (FAILED(hr))
                return FALSE;

            pSelA->szName[0] = '\0';    // initialize...

            (*g_pfnSHDragQueryFileA)(
                (HDROP)medium.hGlobal,
                wParam,
                pSelA->szName,
                ARRAYSIZE(pSelA->szName)
                );

            ReleaseStgMedium(&medium);

            pSelA->bAttr = (BYTE)GetFileAttributesA(pSelA->szName);
            return TRUE;

#ifdef UNICODE
        case FM_GETFILESELW:
        case FM_GETFILESELLFNW:
            pSelW  = (FMS_GETFILESELW*)lParam;
            lpdobj = (LPDATAOBJECT)rpsp->psp.lParam;

            hr = lpdobj->GetData(&fe, &medium);

            if (FAILED(hr))
                return FALSE;

            pSelW->szName[0] = TEXT('\0'); // initialize...

            (*g_pfnSHDragQueryFileW)(
                (HDROP)medium.hGlobal,
                wParam,
                pSelW->szName,
                ARRAYSIZE(pSelW->szName)
                );

            ReleaseStgMedium(&medium);

            pSelW->bAttr = (BYTE)GetFileAttributes(pSelW->szName);
            return TRUE;
#else
        case FM_GETFILESELW:
        case FM_GETFILESELLFNW:
            break;
#endif

#ifdef DEBUG
        case FM_GETDRIVEINFOA:
        case FM_GETDRIVEINFOW:
            OutputDebugString(TEXT("FMX message not implemented\r\n"));
            break;
#endif
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Server inspection routine                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
GetServerFlags(
    LPTSTR  lpPath,
    LPDWORD lpFlags
    )

/*++

Routine Description:

    Determines server type and resource information.

Arguments:

    lpPath  - path to resource.
    lpFlags - pointer to flags to set.

Return Values:

    None.

--*/

{
    if (!g_hNet32Dll)
        g_hNet32Dll = LoadLibrary(DLL_NET32);

    if (g_hNet32Dll && !g_pfnNetUseGetInfo)
    {
        g_pfnNetUseGetInfo = (NETUSEGETINFO)GetProcAddress(
                                                g_hNet32Dll,
                                                EXP_NETUSEGETINFO
                                                );

        g_pfnNetServerGetInfo = (NETSERVERGETINFO)GetProcAddress(
                                                    g_hNet32Dll,
                                                    EXP_NETSERVERGETINFO
                                                    );
#ifdef WINNT
        g_pfnNetApiBufferFree = (NETAPIBUFFERFREE)GetProcAddress(
                                                    g_hNet32Dll,
                                                    EXP_NETAPIBUFFERFREE
                                                    );
#endif
    }

    if (g_pfnNetUseGetInfo && g_pfnNetServerGetInfo
#ifdef WINNT
        && g_pfnNetApiBufferFree
#endif
    ) {
        UINT   cch;
        UINT   err;
        TCHAR  szShareName[MAX_PATH];
        TCHAR  szServerName[MAX_PATH];
#ifndef WINNT
        USHORT cbAvail;
        char   ReturnBuffer[MAX_ONE_RESOURCE];
#endif
        LPBYTE lpBuff;
        LPTSTR pSlash;

        if ((lpPath[0] == TEXT('\\')) && (lpPath[1] == TEXT('\\')))
        {

            lstrcpy(szShareName, lpPath);
            lstrcpy(szServerName, lpPath);

            pSlash = StrChr( &szServerName[2], TEXT('\\'));
            if (pSlash)
                *pSlash = TEXT('\0');       // Terminate server at \\foo

            cch = lstrlen(szServerName)+1;

            pSlash = StrChr( &szShareName[cch], TEXT('\\'));
            if (pSlash)
                *pSlash = TEXT('\0');       // Terminate share at \\foo\bar

            TCHAR szLocalName[MAX_COMPUTERNAME_LENGTH+1];
            DWORD cchLocalName = ARRAYSIZE(szLocalName);

            if (GetComputerName(szLocalName, &cchLocalName))
            {
                if (lstrcmpi(szLocalName, &szServerName[2]))
                    *lpFlags |= DOBJ_FOC_REMOTE;
                else
                    *lpFlags |= DOBJ_FOC_LOCAL;
            }
        }
        else if (lpPath[1] == TEXT(':'))
        {
            TCHAR szLocalDevice[3];

            szLocalDevice[0] = lpPath[0];
            szLocalDevice[1] = lpPath[1];
            szLocalDevice[2] = TEXT('\0');

#ifdef WINNT
            err = (*g_pfnNetUseGetInfo)(
                        NULL,
                        szLocalDevice,
                        1,
                        &lpBuff);
#else
            lpBuff = ReturnBuffer;

            err = (*g_pfnNetUseGetInfo)(
                        NULL,
                        szLocalDevice,
                        1,
                        lpBuff,
                        SIZEOF(ReturnBuffer),
                        &cbAvail
                        );
#endif

            if (err == NERR_Success)
            {
                *lpFlags |= DOBJ_FOC_REMOTE;

                USE_INFO_1* pUseInfo1;
                pUseInfo1 = (USE_INFO_1*)lpBuff;

                lstrcpy(szShareName, pUseInfo1->ui1_remote);
                lstrcpy(szServerName, pUseInfo1->ui1_remote);

                pSlash = StrChr( &szServerName[2], TEXT('\\'));
                if (pSlash)
                    *pSlash = TEXT('\0');       // Terminate server at \\foo
#ifdef WINNT
                (*g_pfnNetApiBufferFree)(lpBuff);
#endif

            }
            else
            {
                // Assume local.  If it's remote, but some
                // net or RPC error caused NetUseGetInfo to fail,
                // then the GetVolumeInformation below should fail, too,
                // so we should get the right result in the end.
                lstrcpy(szShareName, szLocalDevice);
                *lpFlags |= DOBJ_FOC_LOCAL;
            }
        }

#ifdef WINNT
        if (IsFocusRemote(*lpFlags))
        {
            if ((*g_pfnNetServerGetInfo)(
                    szServerName,
                    101,
                    &lpBuff) == NERR_Success)
            {
                SERVER_INFO_101* pSrvInfo101 = (SERVER_INFO_101 *)lpBuff;

                if (pSrvInfo101->sv101_platform_id == SV_PLATFORM_ID_NT)
                    *lpFlags |= DOBJ_SRV_NT;

                if (pSrvInfo101->sv101_platform_id == SV_PLATFORM_ID_OS2)
                {
                    if (pSrvInfo101->sv101_type & SV_TYPE_WINDOWS)
                        *lpFlags |= DOBJ_SRV_W95;
                    else
                        *lpFlags |= DOBJ_SRV_OS2;
                }

                (*g_pfnNetApiBufferFree)(lpBuff);
            }
        }
        else
            *lpFlags |= DOBJ_SRV_NT;        // For GetVolumeInfo on NT
#else
        if (IsFocusRemote(*lpFlags))
        {
            if ((*g_pfnNetServerGetInfo)(
                    szServerName,
                    50,
                    ReturnBuffer,
                    SIZEOF(ReturnBuffer),
                    &cbAvail
                    ) == NERR_Success)
            {
                *lpFlags |= DOBJ_SRV_W95;
            }
            else if ((*g_pfnNetServerGetInfo)(
                        szServerName,
                        1,
                        ReturnBuffer,
                        SIZEOF(ReturnBuffer),
                        &cbAvail
                        ) == NERR_Success)
            {
                SERVER_INFO_1* pSrvInfo1;
                pSrvInfo1 = (SERVER_INFO_1*)ReturnBuffer;

                *lpFlags |= ((pSrvInfo1->sv1_version_major & MAJOR_VER_MASK)
                                >= NT_NOS_MAJOR_VER)
                                    ? DOBJ_SRV_NT
                                    : DOBJ_SRV_OS2;
            }
        }
        else
            *lpFlags |= DOBJ_SRV_W95;   // Don't bother with GetVolumeInfo
#endif

        if (IsServerNT(*lpFlags) && IsResDisk(*lpFlags))
        {
            cch = lstrlen(szShareName);
            szShareName[cch]   = TEXT('\\');
            szShareName[cch+1] = TEXT('\0');

            DWORD flags; // file system flags...

            if (GetVolumeInformation(
                    szShareName,
                    NULL,           // lpVolumeNameBuffer
                    0,              // dwVolumeNameSize
                    NULL,           // lpVolumeSerialNumber
                    NULL,           // lpMaximumComponentLength
                    &flags,         // lpFileSystemFlags
                    NULL,           // lpFileSystemNameBuffer
                    0               // dwFileSystemNameSize
                    ))
            {
                *lpFlags |= (flags & FS_PERSISTENT_ACLS)
                                ? DOBJ_VOL_NTACLS
                                : DOBJ_VOL_FAT;
            }
            else
            {
                //
                // If we can't get the volume information, then if it was
                // because we didn't have access, then there must be security!
                //
                if (GetLastError() == ERROR_ACCESS_DENIED)
                {
                    *lpFlags |= DOBJ_VOL_NTACLS;
                }
            }
        }
    }
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Debug routines                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if DBG

VOID
DumpNetResource(
    LPNETRESOURCE lpNetRes
    )

/*++

Routine Description:

    Dumps content of network resource buffer.

Arguments:

    lpNetRes - buffer with CF_NETRESOURCE format.

Return Values:

    None.

--*/

{
    LPCTSTR psz;

    OutputDebugString(TEXT("\r\nDumpNetResource..."));

    switch (lpNetRes->dwScope)
    {
    case RESOURCE_CONNECTED:
        psz = TEXT("CONNECTED");
        break;
    case RESOURCE_GLOBALNET:
        psz = TEXT("GLOBALNET");
        break;
    case RESOURCE_REMEMBERED:
        psz = TEXT("REMEMBERED");
        break;
    case RESOURCE_RECENT:
        psz = TEXT("RECENT");
        break;
    case RESOURCE_CONTEXT:
        psz = TEXT("CONTEXT");
        break;
    default:
        psz = TEXT("???");
        break;
    }

    OutputDebugString(TEXT("\r\nlpNetRes->dwScope: "));
    OutputDebugString(psz);

    switch (lpNetRes->dwType)
    {
    case RESOURCETYPE_ANY:
        psz = TEXT("ANY");
        break;
    case RESOURCETYPE_DISK:
        psz = TEXT("DISK");
        break;
    case RESOURCETYPE_PRINT:
        psz = TEXT("PRINT");
        break;
    case RESOURCETYPE_RESERVED:
        psz = TEXT("RESERVED");
        break;
    case RESOURCETYPE_UNKNOWN:
        psz = TEXT("UNKNOWN");
        break;
    default:
        psz = TEXT("???");
        break;
    }

    OutputDebugString(TEXT("\r\nlpNetRes->dwType: "));
    OutputDebugString(psz);

    switch (lpNetRes->dwDisplayType)
    {
    case RESOURCEDISPLAYTYPE_GENERIC:
        psz = TEXT("GENERIC");
        break;
    case RESOURCEDISPLAYTYPE_DOMAIN:
        psz = TEXT("DOMAIN");
        break;
    case RESOURCEDISPLAYTYPE_SERVER:
        psz = TEXT("SERVER");
        break;
    case RESOURCEDISPLAYTYPE_SHARE:
        psz = TEXT("SHARE");
        break;
    case RESOURCEDISPLAYTYPE_FILE:
        psz = TEXT("FILE");
        break;
    case RESOURCEDISPLAYTYPE_GROUP:
        psz = TEXT("GROUP");
        break;
    case RESOURCEDISPLAYTYPE_NETWORK:
        psz = TEXT("NETWORK");
        break;
    case RESOURCEDISPLAYTYPE_ROOT:
        psz = TEXT("ROOT");
        break;
    case RESOURCEDISPLAYTYPE_SHAREADMIN:
        psz = TEXT("SHAREADMIN");
        break;
    case RESOURCEDISPLAYTYPE_DIRECTORY:
        psz = TEXT("DIRECTORY");
        break;
    case RESOURCEDISPLAYTYPE_TREE:
        psz = TEXT("TREE");
        break;
    default:
        psz = TEXT("???");
        break;
    }

    OutputDebugString(TEXT("\r\nlpNetRes->dwDisplayType: "));
    OutputDebugString(psz);

    switch (lpNetRes->dwUsage)
    {
    case RESOURCEUSAGE_CONNECTABLE:
        psz = TEXT("CONNECTABLE");
        break;
    case RESOURCEUSAGE_CONTAINER:
        psz = TEXT("CONTAINER");
        break;
    case RESOURCEUSAGE_NOLOCALDEVICE:
        psz = TEXT("NOLOCALDEVICE");
        break;
    case RESOURCEUSAGE_SIBLING:
        psz = TEXT("SIBLING");
        break;
    case RESOURCEUSAGE_ALL:
        psz = TEXT("ALL");
        break;
    case RESOURCEUSAGE_RESERVED:
        psz = TEXT("RESERVED");
        break;
    default:
        psz = TEXT("???");
        break;
    }

    OutputDebugString(TEXT("\r\nlpNetRes->dwUsage: "));
    OutputDebugString(psz);

    OutputDebugString(TEXT("\r\nlpNetRes->lpLocalName: "));
    OutputDebugString(lpNetRes->lpLocalName
                        ? lpNetRes->lpLocalName
                        : TEXT("nul"));
    OutputDebugString(TEXT("\r\nlpNetRes->lpRemoteName: "));
    OutputDebugString(lpNetRes->lpRemoteName
                        ? lpNetRes->lpRemoteName
                        : TEXT("nul"));
    OutputDebugString(TEXT("\r\nlpNetRes->lpComment: "));
    OutputDebugString(lpNetRes->lpComment
                        ? lpNetRes->lpComment
                        : TEXT("nul"));
    OutputDebugString(TEXT("\r\nlpNetRes->lpProvider: "));
    OutputDebugString(lpNetRes->lpProvider
                        ? lpNetRes->lpProvider
                        : TEXT("nul"));

    OutputDebugStringA("\r\nDumpNetResource...done!\r\n");
}

#endif
