//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       rshx32.cpp
//
//  Remote administration shell extension.
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "rshx32.h"
#include <winnetwk.h>   // WNetGetConnection
#include <lm.h>
#include <lmdfs.h>      // NetDfsGetClientInfo
#include <atlconv.h>

#include <initguid.h>
DEFINE_GUID(CLSID_NTFSSecurityExt, 0x1f2e5c40, 0x9550,
            0x11ce, 0x99, 0xd2, 0x00, 0xaa, 0x00, 0x6e, 0x08, 0x6c);
DEFINE_GUID(CLSID_PrintSecurityExt, 0xf37c5810, 0x4d3f,
            0x11d0, 0xb4, 0xbf, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x23);


#define RSX_SECURITY_CHECKED    0x00000001L
#define RSX_HAVE_SECURITY       0x00000002L

#define DOBJ_RES_CONT           0x00000001L
#define DOBJ_RES_ROOT           0x00000002L
#define DOBJ_VOL_NTACLS         0x00000004L     // NTFS or OFS


class CRShellExtCF : public IClassFactory
{
protected:
    ULONG m_cRef;
    SE_OBJECT_TYPE m_seType;

public:
    CRShellExtCF(SE_OBJECT_TYPE seType);
    ~CRShellExtCF();

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};

class CRShellExt : public IShellExtInit, IShellPropSheetExt, IContextMenu
{
protected:
    ULONG           m_cRef;
    SE_OBJECT_TYPE  m_seType;
    LPDATAOBJECT    m_lpdobj; // interface passed in by shell
    HRESULT         m_hrSecurityCheck;
    DWORD           m_dwSIFlags;
    LPTSTR          m_pszServer;
    LPTSTR          m_pszObject;
    HDPA            m_hItemList;

public:
    CRShellExt(SE_OBJECT_TYPE seType);
    ~CRShellExt();

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellExtInit method
    STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

    // IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM);

    //IContextMenu methods
    STDMETHODIMP QueryContextMenu(HMENU hMenu,
                                  UINT indexMenu,
                                  UINT idCmdFirst,
                                  UINT idCmdLast,
                                  UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd,
                                  UINT uFlags,
                                  UINT FAR *reserved,
                                  LPSTR pszName,
                                  UINT cchMax);
private:
    STDMETHODIMP DoSecurityCheck(LPIDA pIDA);
    STDMETHODIMP CheckForSecurity(LPIDA pIDA);
    STDMETHODIMP CreateSI(LPSECURITYINFO *ppsi);
    STDMETHODIMP AddSecurityPage(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    BOOL IsAddPrinterWizard() const;
#if(_WIN32_WINNT >= 0x0500)
    STDMETHODIMP AddMountedVolumePage(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                      LPARAM               lParam);
#endif
};
typedef CRShellExt* PRSHELLEXT;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HINSTANCE        g_hInstance = NULL;
LONG             g_cRefThisDll = 0;
CLIPFORMAT       g_cfShellIDList = 0;
CLIPFORMAT       g_cfPrinterGroup = 0;
CLIPFORMAT       g_cfMountedVolume = 0;
HMODULE          g_hAclui = NULL;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void GetFileInfo(LPCTSTR pszPath,
                 LPDWORD pdwFileType,
                 LPTSTR  pszServer,
                 ULONG   cchServer,
                 LPTSTR *ppszAlternatePath);

void _ReleaseStgMedium(LPSTGMEDIUM p)
{
    // This is the only function we call in ole32.dll, and it's delay-loaded
    // so wrap it with SEH.
    __try
    {
        ReleaseStgMedium(p);
    }
    __finally
    {
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// General routines                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

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

STDAPI_(BOOL)
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        g_cfShellIDList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        g_cfPrinterGroup = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PRINTERGROUP);
        g_cfMountedVolume = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
        DebugProcessAttach();
        TraceSetMaskFromCLSID(CLSID_NTFSSecurityExt);
#ifndef DEBUG
        DisableThreadLibraryCalls(hInstance);
#endif
        break;

    case DLL_PROCESS_DETACH:
        if (g_hAclui)
            FreeLibrary(g_hAclui);
        DebugProcessDetach();
        break;

    case DLL_THREAD_DETACH:
        DebugThreadDetach();
        break;
    }

    return TRUE;
}


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

STDAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID   riid,
                  LPVOID*  ppv)
{
    HRESULT hr;
    SE_OBJECT_TYPE seType;

    *ppv = NULL;

    if (IsEqualCLSID(rclsid, CLSID_NTFSSecurityExt))
        seType = SE_FILE_OBJECT;
    else if (IsEqualCLSID(rclsid, CLSID_PrintSecurityExt))
        seType = SE_PRINTER;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    CRShellExtCF *pShellExtCF = new CRShellExtCF(seType);   // ref == 1

    if (!pShellExtCF)
        return E_OUTOFMEMORY;

    hr = pShellExtCF->QueryInterface(riid, ppv);

    pShellExtCF->Release();     // release initial ref

    return hr;
}


/*++

Routine Description:

    Called by shell to find out if dll can be unloaded.

Arguments:

    None.

Return Values:

    Returns S_OK if dll can be unloaded, S_FALSE if not.

--*/

STDAPI
DllCanUnloadNow()
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}


/*-----------------------------------------------------------------------------
/ DllRegisterServer
/ -----------------
/   Called to allow us to setup the registry entries that we use.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI
DllRegisterServer(void)
{
    return CallRegInstall(g_hInstance, "DefaultInstall");
}


/*-----------------------------------------------------------------------------
/ DllUnregisterServer
/ -------------------
/   Called to allow us to remove the registry entries that we use,
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI
DllUnregisterServer(void)
{
    return CallRegInstall(g_hInstance, "DefaultUninstall");
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRShellExtCF::CRShellExtCF(SE_OBJECT_TYPE seType) : m_cRef(1), m_seType(seType)
{
    InterlockedIncrement(&g_cRefThisDll);
}

CRShellExtCF::~CRShellExtCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CRShellExtCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CRShellExtCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRShellExtCF::QueryInterface(REFIID      riid,
                             LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


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

STDMETHODIMP
CRShellExtCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CRShellExt *pShellExt = new CRShellExt(m_seType);// ref count == 1

    if (!pShellExt)
        return E_OUTOFMEMORY;

    HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    pShellExt->Release();                       // release initial ref

    return hr;
}



/*++

Routine Description:

    Support for IClassFactory::LockServer (not implemented).

Arguments:

    fLock - true if lock count to be incremented.

Return Values:

    Returns E_NOTIMPL.

--*/

STDMETHODIMP
CRShellExtCF::LockServer(BOOL /*fLock*/)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRShellExt::CRShellExt(SE_OBJECT_TYPE seType) : m_cRef(1), m_seType(seType),
    m_dwSIFlags(SI_EDIT_ALL | SI_ADVANCED), m_hrSecurityCheck((HRESULT)-1),
    m_hItemList(NULL)
{
    InterlockedIncrement(&g_cRefThisDll);
}

CRShellExt::~CRShellExt()
{
    DoRelease(m_lpdobj);

    LocalFreeString(&m_pszServer);
    LocalFreeString(&m_pszObject);

    LocalFreeDPA(m_hItemList);

    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IUnknown)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CRShellExt::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRShellExt::QueryInterface(REFIID      riid,
                           LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPSHELLEXTINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IShellExtInit)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


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

STDMETHODIMP CRShellExt::Initialize(LPCITEMIDLIST /*pidlFolder*/,
                                    LPDATAOBJECT  lpdobj,
                                    HKEY          /*hKeyProgID*/)
{
    DoRelease(m_lpdobj);

    m_lpdobj = lpdobj; // processed in AddPages

    if (m_lpdobj)
        m_lpdobj->AddRef();

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IShellPropSheetExt)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


/*++

Routine Description:

    Support for IShellPropSheetExt::AddPages.

Arguments:

    lpfnAddPage - pointer to function called to add a page.
    lParam      - lParam parameter to be passed to lpfnAddPage.

Return Values:

    Returns HRESULT signifying success or failure.

--*/

STDMETHODIMP
CRShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage,
                     LPARAM               lParam)
{
    HRESULT hr;
    STGMEDIUM medium = {0};
    FORMATETC fe = { g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    LPIDA pIDA = NULL;

    TraceEnter(TRACE_RSHX32, "CRShellExt::AddPages");

    // Get the ID List data
    hr = m_lpdobj->GetData(&fe, &medium);
#if(_WIN32_WINNT >= 0x0500)
    if (FAILED(hr) && m_seType == SE_FILE_OBJECT)
        TraceLeaveResult(AddMountedVolumePage(lpfnAddPage, lParam));
#endif
    FailGracefully(hr, "Can't get ID List format from data object");

    pIDA = (LPIDA)GlobalLock(medium.hGlobal);
    TraceAssert(pIDA != NULL);

    // Only support single selection for printers
    if (m_seType == SE_PRINTER && pIDA->cidl != 1)
        ExitGracefully(hr, S_FALSE, "Printer multiple selection not supported");

    hr = DoSecurityCheck(pIDA);

    if (S_OK == hr)
        hr = AddSecurityPage(lpfnAddPage, lParam);

exit_gracefully:

    if (pIDA)
        GlobalUnlock(medium.hGlobal);
    _ReleaseStgMedium(&medium);
    TraceLeaveResult(hr);
}



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

STDMETHODIMP
CRShellExt::ReplacePage(UINT                 /* uPageID */,
                        LPFNADDPROPSHEETPAGE /* lpfnReplaceWith */,
                        LPARAM               /* lParam */)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shell extension object implementation (IContextMenu)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
//  FUNCTION: IContextMenu::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//           This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//    HRESULT signifying success or failure.
//
//  COMMENTS:
//

STDMETHODIMP
CRShellExt::QueryContextMenu(HMENU hMenu,
                             UINT indexMenu,
                             UINT idCmdFirst,
                             UINT /*idCmdLast*/,
                             UINT uFlags)
{
    HRESULT hr = ResultFromShort(0);
    STGMEDIUM medium = {0};
    FORMATETC fe = { g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    LPIDA pIDA = NULL;

    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return hr;

    TraceEnter(TRACE_RSHX32, "CRShellExt::QueryContextMenu");

    // Get the ID List data
    hr = m_lpdobj->GetData(&fe, &medium);
    FailGracefully(hr, "Can't get ID List format from data object");

    pIDA = (LPIDA)GlobalLock(medium.hGlobal);
    TraceAssert(pIDA != NULL);

    // Only support single selection
    if (pIDA->cidl != 1)
        ExitGracefully(hr, S_OK, "Context verb on multiple selection not supported");

    if (S_OK == DoSecurityCheck(pIDA))
    {
        TCHAR szSecurity[32];
        if (!LoadString(g_hInstance, IDS_SECURITY_MENU, szSecurity, ARRAYSIZE(szSecurity)))
        {
            DWORD dwErr = GetLastError();
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "LoadString failed");
        }

        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst;
        mii.dwTypeData = szSecurity;
        mii.cch = lstrlen(szSecurity);

        InsertMenuItem(hMenu,
                   indexMenu,
                   TRUE /*fByPosition*/,
                   &mii);

        hr = ResultFromShort(1);    // Return number of items we added
    }

exit_gracefully:

    if (pIDA)
        GlobalUnlock(medium.hGlobal);
    _ReleaseStgMedium(&medium);
    TraceLeaveResult(hr);
}

//
//  FUNCTION: IContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//    HRESULT signifying success or failure.
//
//  COMMENTS:
//

STDMETHODIMP
CRShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = S_OK;
    STGMEDIUM medium;
    FORMATETC fe = { g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    // Don't support named verbs
    if (HIWORD(lpcmi->lpVerb))
        return E_NOTIMPL;

    TraceEnter(TRACE_RSHX32, "CRShellExt::InvokeCommand");

    // We only have one command, so we should get zero here
    TraceAssert(LOWORD(lpcmi->lpVerb) == 0);

    // This must be true for us to have added the command to the menu
    TraceAssert(S_OK == m_hrSecurityCheck);

    //
    // Call ShellExecuteEx to execute the "Properties" verb on this object, and
    // tell it to select the security property page.
    //

    // Get the ID List data
    hr = m_lpdobj->GetData(&fe, &medium);

    if (SUCCEEDED(hr))
    {
        LPIDA pIDA = (LPIDA)GlobalLock(medium.hGlobal);
        LPITEMIDLIST pidl;

        // We only support single selection for context menus
        TraceAssert(pIDA && pIDA->cidl == 1);

        // Build a fully qualified ID List for this object
        pidl = ILCombine((LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[0]),
                         (LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[1]));

        if (pidl != NULL)
        {
            TCHAR szTitle[64];
            SHELLEXECUTEINFO sei =
            {
                SIZEOF(SHELLEXECUTEINFO),
                (lpcmi->fMask  & (SEE_MASK_HOTKEY | SEE_MASK_ICON)) | SEE_MASK_INVOKEIDLIST,
                lpcmi->hwnd,
                c_szProperties,     // lpVerb ("Properties")
                NULL,               // lpFile
                szTitle,            // lpParameters ("Security")
                NULL,               // lpDirectory,
                lpcmi->nShow,       // nShow
                NULL,               // hInstApp
                (LPVOID)pidl,       // lpIDList
                NULL,               // lpClass
                NULL,               // hkeyClass
                lpcmi->dwHotKey,    // dwHotKey
                lpcmi->hIcon,       // hIcon
                NULL                // hProcess
            };

            LoadString(g_hInstance, IDS_PROPPAGE_TITLE, szTitle, ARRAYSIZE(szTitle));

            // Put up the properties dialog
            if (!ShellExecuteEx(&sei))
            {
                DWORD dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);
            }

            ILFree(pidl);
        }

        GlobalUnlock(medium.hGlobal);
        _ReleaseStgMedium(&medium);
    }

#if 0
    //
    // SHObjectProperties builds a pidl to the object and then calls
    // ShellExecuteEx.  Similar to above, but it does more work to obtain the
    // ID lists (which we already have).
    //
    SHObjectProperties(lpcmi->hwnd,
                       m_seType == SE_PRINTER ? SHOP_PRINTERNAME : SHOP_FILEPATH,
                       m_pszObject,
                       TEXT("Security"));
#endif

    TraceLeaveResult(hr);
}

//
//  FUNCTION: IContextMenu::GetCommandString(UINT, UINT, UINT, LPSTR, UINT)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//    HRESULT signifying success or failure.
//
//  COMMENTS:
//
STDMETHODIMP
CRShellExt::GetCommandString(UINT_PTR /*idCmd*/,
                             UINT uFlags,
                             LPUINT /*reserved*/,
                             LPSTR pszName,
                             UINT cchMax)
{
    if (uFlags == GCS_HELPTEXT)
    {
        LoadString(g_hInstance, IDS_SECURITY_HELPSTRING, (LPTSTR)pszName, cchMax);
        return S_OK;
    }

    // Must be some other flag that we don't handle
    return E_NOTIMPL;
}


//
//  FUNCTION: CRShellExt::DoSecurityCheck(LPIDA)
//
//  PURPOSE: Helper function called by the Property Sheet and Context Menu
//           extension code.  Used to determine whether to add the menu item
//           or property sheet.
//
//  PARAMETERS:
//      pIDA - pointer to ID List Array specifying selected objects
//
//  RETURN VALUE: none
//
//  COMMENTS:
//      The results are stored in m_hrSecurityCheck, m_dwSIFlags, m_pszServer, and m_pszObject
//
STDMETHODIMP
CRShellExt::DoSecurityCheck(LPIDA pIDA)
{
    if (((HRESULT)-1) == m_hrSecurityCheck)
    {
        if (m_seType == SE_PRINTER && IsAddPrinterWizard())
            m_hrSecurityCheck = HRESULT_FROM_WIN32(ERROR_NO_SECURITY_ON_OBJECT);
        else
            m_hrSecurityCheck = CheckForSecurity(pIDA);
    }
    return m_hrSecurityCheck;
}


//
//  FUNCTION: CRShellExt::CheckForSecurity(LPIDA)
//
//  PURPOSE: Helper function called by CRShellExt::DoSecurityCheck
//
//  PARAMETERS: pIDA - pointer to ID List array
//
//  RETURN VALUE: HRESULT - S_OK if ACL editing can proceed
//
//  COMMENTS:
//      The results are stored in m_dwSIFlags, m_pszServer, and m_pszObject
//
STDMETHODIMP
CRShellExt::CheckForSecurity(LPIDA pIDA)
{
    HRESULT hr;
    TCHAR szServer[MAX_PATH];
    LPTSTR pszItem = NULL;
    LPTSTR pszAlternate = NULL;
    DWORD dwFlags = 0;
    UINT cItems;
    LPSHELLFOLDER psf = NULL;
    LPCITEMIDLIST pidl;
    DWORD dwAttr;
    DWORD dwPrivs[] = { SE_SECURITY_PRIVILEGE, SE_TAKE_OWNERSHIP_PRIVILEGE };
    HANDLE hToken = INVALID_HANDLE_VALUE;
    ACCESS_MASK dwAccess = 0;
    UINT i;

    TraceEnter(TRACE_RSHX32, "CRShellExt::CheckForSecurity");
    TraceAssert(m_pszServer == NULL);   // Shouldn't get called twice
    TraceAssert(pIDA != NULL);

    szServer[0] = TEXT('\0');

    cItems = pIDA->cidl;
    TraceAssert(cItems >= 1);

    hr = IDA_BindToFolder(pIDA, &psf);
    FailGracefully(hr, "Unable to bind to folder");
    TraceAssert(psf);

    // Create list for item paths
    TraceAssert(NULL == m_hItemList);
    m_hItemList = DPA_Create(4);
    if (NULL == m_hItemList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA");

    //
    // Get the first item and see if it supports security
    //
    pidl = (LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[1]);
    hr = IDA_GetItemName(psf, pidl, &pszItem);
    FailGracefully(hr, "Unable to get item name");

    dwAttr = SFGAO_FOLDER;
    hr = psf->GetAttributesOf(1, &pidl, &dwAttr);
    FailGracefully(hr, "Unable to get item attributes");

    if (dwAttr & SFGAO_FOLDER)
        dwFlags |= DOBJ_RES_CONT;

    //
    // Check access on the first item only. If we can write the DACL
    // on the first one, we will try (later) to write to all items
    // in the selection and report any errors at that time.
    //
    hToken = EnablePrivileges(dwPrivs, ARRAYSIZE(dwPrivs));

    switch (m_seType)
    {
    case SE_FILE_OBJECT:
        GetFileInfo(pszItem, &dwFlags, szServer, ARRAYSIZE(szServer), &pszAlternate);
        if (pszAlternate)
        {
            LocalFreeString(&pszItem);
            pszItem = pszAlternate;
            pszAlternate = NULL;
        }
        if (dwFlags & DOBJ_VOL_NTACLS)
            hr = CheckFileAccess(pszItem, &dwAccess);
        else
            hr = HRESULT_FROM_WIN32(ERROR_NO_SECURITY_ON_OBJECT);
        break;

    case SE_PRINTER:
        // Printers are containers (they contain documents)
        // and they don't have a parent (for acl editing purposes)
        dwFlags = DOBJ_RES_CONT | DOBJ_RES_ROOT;
        hr = CheckPrinterAccess(pszItem, &dwAccess, szServer, ARRAYSIZE(szServer));
        break;

    default:
        hr = E_UNEXPECTED;
    }
    FailGracefully(hr, "No access");

    // If we can't do anything security related, don't continue.
    if (!(dwAccess & ALL_SECURITY_ACCESS))
        ExitGracefully(hr, E_ACCESSDENIED, "No access");

    // Remember the server name
    if (TEXT('\0') != szServer[0])
    {
        hr = LocalAllocString(&m_pszServer, szServer);
        FailGracefully(hr, "LocalAlloc failed");
    }

    // Remember the item path
    DPA_AppendPtr(m_hItemList, pszItem);
    pszItem = NULL;

    if (!(dwAccess & WRITE_DAC))
        m_dwSIFlags |= SI_READONLY;

    if (!(dwAccess & WRITE_OWNER))
    {
        if (!(dwAccess & READ_CONTROL))
            m_dwSIFlags &= ~SI_EDIT_OWNER;
        else
            m_dwSIFlags |= SI_OWNER_READONLY;
    }

    if (!(dwAccess & ACCESS_SYSTEM_SECURITY))
        m_dwSIFlags &= ~SI_EDIT_AUDITS;

    //
    // Check the rest of the selection.  If any part of a multiple
    // selection doesn't support ACLs or the selection isn't homogenous,
    // then we can't create the security page.
    //
    for (i = 2; i <= cItems; i++)
    {
        DWORD dw = 0;

        // We only do multiple selections for files
        TraceAssert(SE_FILE_OBJECT == m_seType);

        pidl = (LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[i]);
        hr = IDA_GetItemName(psf, pidl, &pszItem);
        FailGracefully(hr, "Unable to get item name");

        dwAttr = SFGAO_FOLDER;
        hr = psf->GetAttributesOf(1, &pidl, &dwAttr);
        FailGracefully(hr, "Unable to get item attributes");

        if (dwAttr & SFGAO_FOLDER)
            dw |= DOBJ_RES_CONT;

        if ((dw & DOBJ_RES_CONT) != (dwFlags & DOBJ_RES_CONT))
            ExitGracefully(hr, E_FAIL, "Incompatible multiple selection");

        GetFileInfo(pszItem, &dw, szServer, ARRAYSIZE(szServer), &pszAlternate);
        if (pszAlternate)
        {
            LocalFreeString(&pszItem);
            pszItem = pszAlternate;
            pszAlternate = NULL;
        }

        // Compare against first item.  All flags and the server name
        // must match, otherwise we can't edit the ACLs.
        if (dw == dwFlags &&
            ((NULL == m_pszServer && TEXT('\0') == szServer[0]) ||
             (NULL != m_pszServer && 0 == lstrcmpi(m_pszServer, szServer))))
        {
            // Remember the item path
            DPA_AppendPtr(m_hItemList, pszItem);
            pszItem = NULL;
        }
        else
            ExitGracefully(hr, E_FAIL, "Incompatible multiple selection");
    }

    //
    // If everything has succeeded up to this point, save some flags
    // and the server and object name strings
    //
    if (dwFlags & DOBJ_RES_CONT)
        m_dwSIFlags |= SI_CONTAINER;

    //
    // For Root objects (e.g. "D:\") hide the ACL Protection checkbox,
    // since these objects don't appear to have parents.
    //
    if (dwFlags & DOBJ_RES_ROOT)
        m_dwSIFlags |= SI_NO_ACL_PROTECT;

    // Get the "Normal" display name to use as the object name
    hr = IDA_GetItemName(psf,
                         (LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[1]),
                         szServer,
                         ARRAYSIZE(szServer),
                         SHGDN_NORMAL);
    FailGracefully(hr, "Unable to get item name");
    if (cItems > 1)
    {
        int nLength = lstrlen(szServer);
        LoadString(g_hInstance,
                   IDS_MULTISEL_ELLIPSIS,
                   szServer + nLength,
                   ARRAYSIZE(szServer) - nLength);
    }
    hr = LocalAllocString(&m_pszObject, szServer);

exit_gracefully:

    ReleasePrivileges(hToken);

    DoRelease(psf);

    LocalFreeString(&pszItem);
    LocalFreeString(&pszAlternate);

    TraceLeaveResult(hr);
}


//
//  FUNCTION: CRShellExt::CreateSI(LPSECURITYINFO *)
//
//  PURPOSE: Create a SecurityInformation object of the correct type
//
//  PARAMETERS: ppsi - Location to store ISecurityInformation pointer
//
//  RETURN VALUE: HRESULT signifying success or failure
//
//  COMMENTS:
//
STDMETHODIMP
CRShellExt::CreateSI(LPSECURITYINFO *ppsi)
{
    HRESULT hr;
    CSecurityInformation *psi;

    TraceEnter(TRACE_RSHX32, "CRShellExt::CreateSI");
    TraceAssert(ppsi != NULL);

    *ppsi = NULL;

    switch (m_seType)
    {
    case SE_FILE_OBJECT:
        psi = new CNTFSSecurity(m_seType);  // ref == 1
        break;

    case SE_PRINTER:
        psi = new CPrintSecurity(m_seType); // ref == 1
        break;

    default:
        TraceLeaveResult(E_UNEXPECTED);
    }

    if (psi == NULL)
        TraceLeaveResult(E_OUTOFMEMORY);

    hr = psi->Initialize(m_hItemList,
                         m_dwSIFlags,
                         m_pszServer,
                         m_pszObject);
    if (SUCCEEDED(hr))
    {
        *ppsi = psi;

        // The SecurityInfo object takes responsibility for these
        m_hItemList = NULL;
        m_pszServer = NULL;
        m_pszObject = NULL;
        m_hrSecurityCheck = (HRESULT)-1;
    }
    else
        psi->Release();

    TraceLeaveResult(hr);
}


typedef HPROPSHEETPAGE (WINAPI *PFN_CREATESECPAGE)(LPSECURITYINFO);

HPROPSHEETPAGE
_CreateSecurityPage(LPSECURITYINFO psi)
{
    HPROPSHEETPAGE hPage = NULL;
    const TCHAR szAclui[] = TEXT("aclui.dll");
    const char szCreateSecPage[] = "CreateSecurityPage";

    if (!g_hAclui)
        g_hAclui = LoadLibrary(szAclui);

    if (g_hAclui)
    {
        static PFN_CREATESECPAGE s_pfnCreateSecPage = NULL;

        if (!s_pfnCreateSecPage)
            s_pfnCreateSecPage = (PFN_CREATESECPAGE)GetProcAddress(g_hAclui, szCreateSecPage);

        if (s_pfnCreateSecPage)
            hPage = (*s_pfnCreateSecPage)(psi);
    }

    return hPage;
}

STDMETHODIMP
CRShellExt::AddSecurityPage(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HRESULT hr;
    LPSECURITYINFO psi;

    hr = CreateSI(&psi);            // ref == 1

    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hPermPage = _CreateSecurityPage(psi);

        if (hPermPage)
        {
            if (!lpfnAddPage(hPermPage, lParam))
                DestroyPropertySheetPage(hPermPage);
        }
        else
        {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }

        psi->Release();             // release initial ref
    }
    return hr;
}


//
//  FUNCTION: CRShellExt::IsAddPrinterWizard()
//
//  PURPOSE: Check for the Add Printer wizard
//
//  PARAMETERS: none
//
//  RETURN VALUE: TRUE if the selected object is the Add Printer wizard,
//                FALSE otherwise
//
//  COMMENTS:
//
BOOL
CRShellExt::IsAddPrinterWizard() const
{
    BOOL bRetval = FALSE;
    STGMEDIUM medium;
    FORMATETC fe = { g_cfPrinterGroup, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    TCHAR szFile[MAX_PATH];

    TraceEnter(TRACE_RSHX32, "CRShellExt::IsAddPrinterWizard");
    TraceAssert(m_seType == SE_PRINTER);

    //
    // Fail the call if m_lpdobj is NULL.
    //
    if( m_lpdobj && SUCCEEDED( m_lpdobj->GetData( &fe, &medium ) ) )
    {
#if(_WIN32_WINNT < 0x0500)
        //
        // !!BUGBUG!!
        // In NT 4.0 the printer context menus in the shell failed
        // to handle UNICODE strings properly during a call to DragQueryFile.
        // The strings returned were always ANSI.  This has been fixed
        // in build 1393 and greater.
        //
#ifdef UNICODE
        LPDROPFILES pdf = (LPDROPFILES)GlobalLock(medium.hGlobal);
        pdf->fWide = TRUE;
        GlobalUnlock(medium.hGlobal);
#endif
#endif
        //
        // Get the selected item name.
        //
        if( DragQueryFile( (HDROP)medium.hGlobal, 0, szFile, ARRAYSIZE( szFile ) ) )
        {
            //
            // Check if this is the magic Add Printer Wizard shell object.
            // The check is not case sensitive and the string is not localized.
            //
            if( 0 == lstrcmpi( szFile, TEXT("WinUtils_NewObject") ) )
            {
                TraceMsg("Found Add Printer wizard");
                bRetval = TRUE;
            }
        }

        //
        // Release the storage medium.
        //
        _ReleaseStgMedium( &medium );
    }

    TraceLeaveValue(bRetval);
}


#if(_WIN32_WINNT >= 0x0500)
//
//  FUNCTION: CRShellExt::AddMountedVolumePage()
//
//  PURPOSE: Create Security page for mounted volume properties
//
//  PARAMETERS: lpfnAddPage - pointer to function called to add a page.
//              lParam      - lParam parameter to be passed to lpfnAddPage.
//
//  RETURN VALUE: HRESULT signifying success or failure
//
//  COMMENTS:
//
STDMETHODIMP
CRShellExt::AddMountedVolumePage(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                 LPARAM               lParam)
{
    HRESULT hr = S_OK;
    STGMEDIUM medium = {0};
    FORMATETC fe = { g_cfMountedVolume, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    TCHAR szMountPoint[MAX_PATH];
    TCHAR szVolumeID[MAX_PATH];
    TCHAR szLabel[64];
    LPTSTR pszVolID = NULL;
    DWORD dwVolFlags = 0;
    DWORD dwPrivs[] = { SE_SECURITY_PRIVILEGE, SE_TAKE_OWNERSHIP_PRIVILEGE };
    HANDLE hToken = INVALID_HANDLE_VALUE;
    ACCESS_MASK dwAccess = 0;
    BOOL bHasSecurity = FALSE;

    TraceEnter(TRACE_RSHX32, "CRShellExt::AddMountedVolumePage");
    TraceAssert(m_seType == SE_FILE_OBJECT);
    TraceAssert(m_lpdobj);

    // Try to get the mounted volume host folder path
    hr = m_lpdobj->GetData(&fe, &medium);
    FailGracefully(hr, "Not a mounted volume");

    // Get the host folder path
    if (!DragQueryFile((HDROP)medium.hGlobal, 0, szMountPoint, ARRAYSIZE(szMountPoint)))
        ExitGracefully(hr, E_FAIL, "Can't get mount point from storage medium");

    PathAddBackslash(szMountPoint);

    // Get the volume ID, which looks like
    // "\\?\Volume{9e2df3f5-c7f1-11d1-84d5-000000000000}\"
    if (!GetVolumeNameForVolumeMountPoint(szMountPoint, szVolumeID, ARRAYSIZE(szVolumeID)))
        ExitGracefully(hr, E_FAIL, "GetVolumeNameForVolumeMountPoint failed");

    if (GetVolumeInformation(szMountPoint, //szVolumeID,
                             szLabel,
                             ARRAYSIZE(szLabel),
                             NULL,
                             NULL,
                             &dwVolFlags,
                             NULL,
                             0))
    {
        if (dwVolFlags & FS_PERSISTENT_ACLS)
        {
            bHasSecurity = TRUE;
        }
    }
    else if (GetLastError() == ERROR_ACCESS_DENIED)
    {
        // If we can't get the volume information because we don't have
        // access, then there must be security!
        bHasSecurity = TRUE;
    }

    if (!bHasSecurity)
        ExitGracefully(hr, E_FAIL, "Volume inaccessible or not NTFS");

    hToken = EnablePrivileges(dwPrivs, ARRAYSIZE(dwPrivs));

    hr = CheckFileAccess(szVolumeID, &dwAccess);
    FailGracefully(hr, "Volume inaccessible");

    // If we can't do anything security related, don't continue.
    if (!(dwAccess & ALL_SECURITY_ACCESS))
        ExitGracefully(hr, E_ACCESSDENIED, "No security access");

    if (!(dwAccess & WRITE_DAC))
        m_dwSIFlags |= SI_READONLY;

    if (!(dwAccess & WRITE_OWNER))
    {
        if (!(dwAccess & READ_CONTROL))
            m_dwSIFlags &= ~SI_EDIT_OWNER;
        else
            m_dwSIFlags |= SI_OWNER_READONLY;
    }

    if (!(dwAccess & ACCESS_SYSTEM_SECURITY))
        m_dwSIFlags &= ~SI_EDIT_AUDITS;

    m_dwSIFlags |= SI_CONTAINER | SI_NO_ACL_PROTECT;


    // BUGBUG - get szServer from szMountPoint
    //hr = LocalAllocString(&m_pszServer, szServer);
    //FailGracefully(hr, "LocalAllocString failed");

    if (!FormatStringID(&m_pszObject,
                        g_hInstance,
                        IDS_FMT_VOLUME_DISPLAY,
                        szLabel,
                        szMountPoint))
    {
        LocalAllocString(&m_pszObject, szLabel);
    }

    if (!m_pszObject)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to build volume display string");

    m_hItemList = DPA_Create(1);
    if (!m_hItemList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create item list");

    hr = LocalAllocString(&pszVolID, szVolumeID);
    FailGracefully(hr, "Unable to copy volume ID string");

    DPA_AppendPtr(m_hItemList, pszVolID);
    pszVolID = NULL;

    hr = AddSecurityPage(lpfnAddPage, lParam);

exit_gracefully:

    ReleasePrivileges(hToken);
    LocalFreeString(&pszVolID);
    _ReleaseStgMedium(&medium);
    TraceLeaveResult(hr);
}
#endif  // _WIN32_WINNT >= 0x0500


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous helper functions                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#if(_WIN32_WINNT < 0x0500)

#undef PathIsUNC
STDAPI_(BOOL)
PathIsUNC(LPCTSTR psz)
{
    return (psz && psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}

static const TCHAR c_szColonSlash[] = TEXT(":\\");

#undef PathIsRoot
STDAPI_(BOOL)
PathIsRoot(LPCTSTR pPath)
{
    return (pPath && !lstrcmpi(pPath + 1, c_szColonSlash));
}

#endif // #if(_WIN32_WINNT < 0x0500)

BOOL
IsDfsPath(LPTSTR pszPath,       // in
          LPTSTR pszServer,     // out
          UINT   cchServer,     // in
          LPTSTR pszAltPath)    // out
{
    BOOL bIsDfs = FALSE;
    WCHAR szPath[MAX_PATH];
    PDFS_INFO_3 pDI3 = NULL;
    WCHAR szServer[UNCLEN];
    WCHAR szStorage[MAX_PATH];

    USES_CONVERSION;

    if (!PathIsUNC(pszPath))
        return FALSE;     // local machine

    lstrcpynW(szPath, T2CW(pszPath), ARRAYSIZE(szPath));

    // Check for DFS
    for (;;)
    {
        DWORD dwErr;

        __try
        {
            // This is delay-loaded by the linker, so
            // must wrap with an exception handler.
            dwErr = NetDfsGetClientInfo(szPath,
                                        NULL,
                                        NULL,
                                        3,
                                        (LPBYTE*)&pDI3);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return FALSE;
        }

        if (NERR_Success == dwErr)
        {
            for (ULONG i = 0; i < pDI3->NumberOfStorages; i++)
            {
                if (DFS_STORAGE_STATE_ONLINE & pDI3->Storage[i].State)
                {
                    bIsDfs = TRUE;

                    szServer[0] = L'\\';
                    szServer[1] = L'\\';
                    lstrcpynW(&szServer[2], pDI3->Storage[i].ServerName, ARRAYSIZE(szServer)-2);
                    lstrcpynW(szStorage, szServer, ARRAYSIZE(szStorage));
                    PathAppendW(szStorage, pDI3->Storage[i].ShareName);

                    // If this server is active, quit looking
                    if (DFS_STORAGE_STATE_ACTIVE & pDI3->Storage[i].State)
                        break;
                }
            }
            break;
        }
        else if (NERR_DfsNoSuchVolume == dwErr)
        {
            // If we're at the root, then we can't go any farther.
            if (PathIsRoot(szPath))
                break;

            // Remove the last path element and try again
            PathRemoveFileSpec(szPath);
        }
        else
        {
            // Some other error, bail
            break;
        }
    }

    if (bIsDfs)
    {
        lstrcpyn(pszServer, W2T(szServer), cchServer);

        // Note that EntryPath has only a single leading backslash, hence +1
        LPCTSTR pszEnd = pszPath + lstrlen(pDI3->EntryPath) + 1;
        while (TEXT('\\') == *pszEnd)
            pszEnd++;

        PathCombine(pszAltPath, szStorage, pszEnd);
    }

    if (NULL != pDI3)
        NetApiBufferFree(pDI3);

    return bIsDfs;
}


void
GetVolumeInfo(LPCTSTR pszPath,
              BOOL    bIsFolder,
              LPDWORD pdwFlags,
              LPTSTR  pszVolume,
              ULONG   cchVolume)
{
    TCHAR szVolume[MAX_PATH];
    TCHAR szVolumeID[MAX_PATH];

    //
    // The path can be DFS or contain volume mount points, so start
    // with the full path and try GetVolumeInformation on successively
    // shorter paths until it succeeds or we run out of path.
    //
    // However, if it's a volume mount point, we're interested in the
    // the host folder's volume so back up one level to start.  The
    // child volume is handled separately (see AddMountedVolumePage).
    //

    lstrcpyn(szVolume, pszPath, ARRAYSIZE(szVolume));

    if (!bIsFolder
        || GetVolumeNameForVolumeMountPoint(szVolume, szVolumeID, ARRAYSIZE(szVolumeID)))
    {
        PathRemoveFileSpec(szVolume);
    }

    for (;;)
    {
        PathAddBackslash(szVolume); // GetVolumeInformation likes a trailing '\'

        if (GetVolumeInformation(szVolume,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 pdwFlags,
                                 NULL,
                                 0))
        {
            break;
        }

        // Access denied implies that we've reached the deepest volume
        // in the path; we just can't get the flags.  It also implies
        // security, so assume persistent acls.
        if (ERROR_ACCESS_DENIED == GetLastError())
        {
            *pdwFlags = FS_PERSISTENT_ACLS;
            break;
        }

        // If we're at the root, then we can't go any farther.
        if (PathIsRoot(szVolume))
            break;

        // Remove the last path element and try again
        PathRemoveBackslash(szVolume);
        PathRemoveFileSpec(szVolume);
    }

    if (pszVolume)
    {
        PathRemoveBackslash(szVolume);
        lstrcpyn(pszVolume, szVolume, cchVolume);
    }
}


void
GetFileInfo(LPCTSTR pszPath,
            LPDWORD pdwFileType,
            LPTSTR  pszServer,
            ULONG   cchServer,
            LPTSTR *ppszAlternatePath)
{
    DWORD dwVolumeFlags = 0;
    TCHAR szVolume[MAX_PATH];
    LPTSTR pszUNC = NULL;

    TraceEnter(TRACE_RSHX32, "GetFileInfo");
    TraceAssert(NULL != pszServer);
    TraceAssert(NULL != ppszAlternatePath);

    pszServer[0] = TEXT('\0');
    *ppszAlternatePath = NULL;

    if (!PathIsUNC(pszPath) && S_OK == GetRemotePath(pszPath, &pszUNC))
        pszPath = pszUNC;

    if (PathIsRoot(pszPath))
        *pdwFileType |= DOBJ_RES_ROOT;

    GetVolumeInfo(pszPath,
                  *pdwFileType & DOBJ_RES_CONT,
                  &dwVolumeFlags,
                  szVolume,
                  ARRAYSIZE(szVolume));
    if (dwVolumeFlags & FS_PERSISTENT_ACLS)
    {
        TCHAR szAltVolume[MAX_PATH];

        *pdwFileType |= DOBJ_VOL_NTACLS;

        if (IsDfsPath(szVolume, pszServer, cchServer, szAltVolume))
        {
            LPCTSTR pszEnd = pszPath + lstrlen(szVolume);
            while (TEXT('\\') == *pszEnd)
                pszEnd++;

            *ppszAlternatePath = (LPTSTR)LocalAlloc(LPTR, StringByteSize(szAltVolume)
                                                        + StringByteSize(pszEnd));
            if (*ppszAlternatePath)
            {
                // PathCombine asserts that the buffer is at least MAX_PATH,
                // so don't use it here.  We know the buffer is big enough.
                lstrcpy(*ppszAlternatePath, szAltVolume);
                LPTSTR pszT = PathAddBackslash(*ppszAlternatePath);
                if (pszT)
                    lstrcpy(pszT, pszEnd);
                //PathCombine(*ppszAlternatePath, szAltVolume, pszEnd);
            }
        }
        else if (PathIsUNC(szVolume))
        {
            LPTSTR pSlash = StrChr(&szVolume[2], TEXT('\\'));
            if (pSlash)
                cchServer = min(cchServer, (ULONG)(pSlash - szVolume) + 1);
            lstrcpyn(pszServer, szVolume, cchServer);
        }
    }

    LocalFreeString(&pszUNC);

    TraceLeaveVoid();
}
