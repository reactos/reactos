//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// itemmenu.cpp 
//
//   IConextMenu for folder items.
//
//   History:
//
//       3/26/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "itemmenu.h"
#include "dll.h"
#include "resource.h"

#include <mluisupp.h>

//  In Shdocvw: shbrowse.cpp
#ifndef UNIX
extern HRESULT CDDEAuto_Navigate(BSTR str, HWND *phwnd, long);
#else
extern "C" HRESULT CDDEAuto_Navigate(BSTR str, HWND *phwnd, long);
#endif /* UNIX */
//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::CContextMenu ***
//
//    Constructor for IContextMenu.
//
////////////////////////////////////////////////////////////////////////////////
CContextMenu::CContextMenu (
	PCDFITEMIDLIST* apcdfidl,
    LPITEMIDLIST pidlPath,
    UINT nCount
)
: m_cRef(1)
{
    //
    // Copy the pcdfidls.
    //

    ASSERT(apcdfidl || 0 == nCount);

    ASSERT(NULL == m_apcdfidl);
    ASSERT(NULL == m_pidlPath);

    //
    // In low memory situations pidlPath may be NULL.
    //

    if (pidlPath)
        m_pidlPath = ILClone(pidlPath);

    IMalloc* pIMalloc;

    if (SUCCEEDED(SHGetMalloc(&pIMalloc)))
    {
        ASSERT(pIMalloc);

        m_apcdfidl = (PCDFITEMIDLIST*)pIMalloc->Alloc(
                                               nCount * sizeof(PCDFITEMIDLIST));

        if (m_apcdfidl)
        {
            for (UINT i = 0, bOutOfMem = FALSE; (i < nCount) && !bOutOfMem; i++)
            {
                ASSERT(CDFIDL_IsValid(apcdfidl[i]));
                ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)apcdfidl[i])));

                m_apcdfidl[i] = (PCDFITEMIDLIST)ILClone(
                                                     (LPITEMIDLIST)apcdfidl[i]);

                if (bOutOfMem = (NULL == m_apcdfidl[i]))
                {
                    while (i--)
                        pIMalloc->Free(m_apcdfidl[i]);

                    pIMalloc->Free(m_apcdfidl);
                    m_apcdfidl = NULL;
                }
                else
                {
                    ASSERT(CDFIDL_IsValid(m_apcdfidl[i]));
                }
            }
        }

        pIMalloc->Release();
    }

    m_nCount = m_apcdfidl ? nCount : 0;

    //
    // Don't allow the DLL to unload.
    //

    TraceMsg(TF_OBJECTS, "+ IContextMenu");

    DllAddRef();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::~CContextMenu ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CContextMenu::~CContextMenu (
	void
)
{
    ASSERT(0 == m_cRef);

    //
    // Free the locally stored cdfidls.
    //

    if (m_apcdfidl)
    {
        IMalloc* pIMalloc;

        if (SUCCEEDED(SHGetMalloc(&pIMalloc)))
        {
            ASSERT(pIMalloc);

            while (m_nCount--)
            {
                ASSERT(CDFIDL_IsValid(m_apcdfidl[m_nCount]));
                ASSERT(pIMalloc->DidAlloc(m_apcdfidl[m_nCount]));

                pIMalloc->Free(m_apcdfidl[m_nCount]);
            }

            ASSERT(pIMalloc->DidAlloc(m_apcdfidl));

            pIMalloc->Free(m_apcdfidl);

            pIMalloc->Release();
        }
    }

    if (m_pidlPath)
        ILFree(m_pidlPath);

    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IContextMenu");

    DllRelease();

	return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::CContextMenu ***
//
//    CExtractIcon QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CContextMenu::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IContextMenu2 == riid)
    {
        *ppv = (IContextMenu2*)this;
    }
    else if (IID_IContextMenu == riid)
    {
        *ppv = (IContextMenu*)this;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    
    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::AddRef ***
//
//    CContextMenu AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CContextMenu::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::Release ***
//
//    CContextMenu Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CContextMenu::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}


//
// IContextMenu methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::QueryContextMenu ***
//
//
// Description:
//     Adds menu items to the given item's context menu.
//
// Parameters:
//     [In Out]  hmenu      - A handle to the menu.  New items are inserted into
//                            this menu  
//     [In]      indexMenu  - Zero-based position at which to insert the first
//                            menu item.
//     [In]      idCmdFirst - Minimum value that can be used for a new menu item
//                            identifier. 
//     [In]      idCmdLast  - Maximum value the can be used for a menu item id.
//     [In]      uFlags     - CMF_DEFAULTONLY, CMF_EXPLORE, CMF_NORMAL or
//                            CMF_VERBSONLY.
//
// Return:
//     On success the scode contains the the menu identifier offset of the last
//     menu item added plus one.
//
// Comments:
//     CMF_DEFAULTONLY flag indicates the user double-clicked on the item.  In
//     this case no menu is displayed.  The shell is simply querying for the ID
//     of the default action.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CContextMenu::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
)
{
    ASSERT(hmenu);
    ASSERT(idCmdFirst < idCmdLast);

    HRESULT hr;

    if (CMF_DEFAULTONLY & uFlags)
    {
        ASSERT(idCmdFirst + IDM_OPEN < idCmdLast);

        InsertMenu(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst + IDM_OPEN,
                   TEXT(""));

        SetMenuDefaultItem(hmenu, idCmdFirst + IDM_OPEN, FALSE);

        hr = S_OK;
    }
    else
    {
        ASSERT(idCmdFirst + IDM_PROPERTIES < idCmdLast);

        HMENU hmenuParent = LoadMenu(MLGetHinst(),
                                     MAKEINTRESOURCE(IDM_CONTEXTMENU));

        if (hmenuParent)
        {
            HMENU hmenuContext = GetSubMenu(hmenuParent, 0);

            if (hmenuContext)
            {
                ULONG idNew = Shell_MergeMenus(hmenu, hmenuContext, indexMenu,
                                               idCmdFirst, idCmdLast,
                                               MM_ADDSEPARATOR |
                                               MM_SUBMENUSHAVEIDS);

                SetMenuDefaultItem(hmenu, idCmdFirst + IDM_OPEN, FALSE);

                hr = 0x000ffff & idNew;

                DestroyMenu(hmenuContext);
            }
            else
            {
                hr = E_FAIL;
            }

            DestroyMenu(hmenuParent);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::InvokeCommand ***
//
//
// Description:
//     Carries out the command for the given menu item id.
//
// Parameters:
//     [In]  lpici - Structure containing the verb, hwnd, menu id, etc.
//
// Return:
//     S_OK if the command was successful.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CContextMenu::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
)
{
    ASSERT(lpici);

    HRESULT hr;

    if (HIWORD(lpici->lpVerb))
    {
        hr = E_NOTIMPL;
    }
    else
    {
        WORD wCmd = LOWORD(lpici->lpVerb);

        switch(wCmd)
        {
        case IDM_OPEN:
            hr = DoOpen(lpici->hwnd, lpici->nShow);
            break;

        case IDM_PROPERTIES:
            hr = DoProperties(lpici->hwnd);
            break;

        default:
            hr = E_NOTIMPL;
            break;
        }
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::GetCommandString ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//      note -- return an ANSI command string
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CContextMenu::GetCommandString(
    UINT_PTR idCommand,
    UINT uFlags,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax
)
{
    HRESULT hr = E_FAIL;

    if ((uFlags == GCS_VERB) && (idCommand == IDM_OPEN))
    {
        StrCpyN((LPTSTR)pszName, TEXT("open"), cchMax);
        hr = NOERROR;
    }
#ifdef UNICODE
    else if ((uFlags == GCS_VERBA) && (idCommand == IDM_OPEN))
    {
        StrCpyNA(pszName, "open", cchMax);
        hr = NOERROR;
    }
#endif

    return hr;
}


//
// IContextMenu2 methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::HandleMenuMsg ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CContextMenu::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    return S_OK;
}


//
// Helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::DoOpen ***
//
//
// Description:
//     Command handler for IDM_OPEN.
//
// Parameters:
//     [In]  hwnd  - Parent window.  Used for dialogs etc.
//     [In]  nShow - ShowFlag use in ShowWindow command.
//
// Return:
//     S_OK if the command executed.
//     E_FAIL if the command iddn't execute.
//     E_OUTOFMEMORY if there wasn't enough memory to execute the command.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CContextMenu::DoOpen(
    HWND hwnd,
    int  nShow
)
{
    HRESULT hr;

    if (m_apcdfidl)
    {
        ASSERT(CDFIDL_IsValid(m_apcdfidl[0]));
        ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)m_apcdfidl[0])));

        if (CDFIDL_IsFolder(m_apcdfidl[0]))
        {
            hr = DoOpenFolder(hwnd, nShow);
        }
        else
        {
            hr = DoOpenStory(hwnd, nShow);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::DoOpenFolder ***
//
//
// Description:
//     Open command for folders.
//
// Parameters:
//     [In]  hwnd  - Parent window.  Used for dialogs etc.
//     [In]  nShow - ShowFlag use in ShowWindow command.
//
// Return:
//     S_OK if the command executed.
//     E_FAIL if the command iddn't execute.
//     E_OUTOFMEMORY if there wasn't enough memory to execute the command.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CContextMenu::DoOpenFolder(
    HWND hwnd,
    int  nShow
)
{
    HRESULT hr;

    if (m_pidlPath)
    {
        ASSERT(m_apcdfidl);
        ASSERT(CDFIDL_IsValid(m_apcdfidl[0]));
        ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)m_apcdfidl[0])));

        LPITEMIDLIST pidlFull = ILCombine(m_pidlPath,
                                          (LPITEMIDLIST)m_apcdfidl[0]);

        if (pidlFull)
        {
            SHELLEXECUTEINFO ei = {0};

            ei.cbSize   = sizeof(SHELLEXECUTEINFO);
            ei.fMask    = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
            ei.hwnd     = hwnd;
            ei.lpVerb   = TEXT("Open");
            ei.nShow    = nShow;
            ei.lpIDList = (LPVOID)pidlFull;
            ei.lpClass  = TEXT("Folder");

            hr = ShellExecuteEx(&ei) ? S_OK : E_FAIL;

            ILFree(pidlFull);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::DoOpenStory ***
//
//
// Description:
//     Open command for stories (internet links).
//
// Parameters:
//     [In]  hwnd  - Parent window.  Used for dialogs etc.
//     [In]  nShow - ShowFlag use in ShowWindow command.
//
// Return:
//     S_OK if ShellExecuteEx succeeded.
//     E_FAIL if ShellExecuteEx didn't succeed.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CContextMenu::DoOpenStory(
    HWND hwnd,
    int  nShow
)
{
    ASSERT(m_apcdfidl);
    ASSERT(CDFIDL_IsValid(m_apcdfidl[0]));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)m_apcdfidl[0])));

    HRESULT hr = E_FAIL;

    LPTSTR pszURL = CDFIDL_GetURL(m_apcdfidl[0]); 

    if (PathIsURL(pszURL))
    {
    
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
        HWND hwnd = (HWND)-1;
        BSTR bstrURL;

        SHTCharToUnicode(pszURL, wszURL, ARRAYSIZE(wszURL));
        bstrURL = SysAllocString(wszURL);
        if (bstrURL)
        {
            hr = CDDEAuto_Navigate(bstrURL, &hwnd, 0);
            SysFreeString(bstrURL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::DoProperties ***
//
//
// Description:
//     Command handler for IDM_PROPERTIES.
//
// Parameters:
//     [In]  hwnd  - Parent window.  Used for dialogs etc.
//
// Return:
//     S_OK if the command executed.
//     E_FAIL if the command iddn't execute.
//     E_OUTOFMEMORY if there wasn't enough memory to execute the command.
//
// Comments:
//     Uses the property pages of the InternetShortcut object.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CContextMenu::DoProperties(
    HWND hwnd
)
{
    HRESULT hr;

    if (m_apcdfidl)
    {
        ASSERT(CDFIDL_IsValid(m_apcdfidl[0]));
        ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)m_apcdfidl[0])));

        IShellPropSheetExt* pIShellPropSheetExt;

        hr = QueryInternetShortcut(m_apcdfidl[0], IID_IShellPropSheetExt,
                                   (void**)&pIShellPropSheetExt);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIShellPropSheetExt);

            PROPSHEETHEADER psh = {0};
            HPROPSHEETPAGE  ahpage[MAX_PROP_PAGES];

            psh.dwSize     = sizeof(PROPSHEETHEADER);
            psh.dwFlags    = PSH_NOAPPLYNOW;
            psh.hwndParent = hwnd;
            psh.hInstance  = MLGetHinst();
            psh.pszCaption = CDFIDL_GetName(m_apcdfidl[0]);
            psh.phpage     = ahpage;

            hr = pIShellPropSheetExt->AddPages(AddPages_Callback, (LPARAM)&psh);

            if (SUCCEEDED(hr))
            {
                //
                // Property sheets are currently disabled.  This is the only
                // API called in comctl32.dll so remove it to avoid a
                //dependency.

                //hr = (-1 == PropertySheet(&psh)) ? E_FAIL : S_OK;
            }

            pIShellPropSheetExt->Release();
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** AddPages_Callback ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
AddPages_Callback(
    HPROPSHEETPAGE hpage,
    LPARAM lParam
)
{
    ASSERT(hpage);
    ASSERT(lParam);

    BOOL                bRet;
    PROPSHEETHEADER*    ppsh = (PROPSHEETHEADER*)lParam;

    if (ppsh->nPages < MAX_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

//
//
//

HRESULT CALLBACK
MenuCallBack(
    IShellFolder* pIShellFolder,
    HWND hwndOwner,
    LPDATAOBJECT pdtobj,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::QueryInternetShortcut ***
//
//
// Description:
//     Sets up an internet shorcut object for the given pidl.
//
// Parameters:
//     [In]  pcdfidl - The shortcut object is created for the URL stored in this
//                     cdf item id list.
//     [In]  riid    - The requested interface on the shortcut object.
//     [Out] ppvOut  - A pointer that receives the interface.
//
// Return:
//     S_OK if the object is created and the interface is found.
//     A COM error code otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CContextMenu::QueryInternetShortcut(
    PCDFITEMIDLIST pcdfidl,
    REFIID riid,
    void** ppvOut
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)pcdfidl)));
    ASSERT(ppvOut);

    HRESULT hr;

    *ppvOut = NULL;

    //
    // Only create a shell link object if the CDF contains an URL
    //
    if (*(CDFIDL_GetURL(pcdfidl)) != 0)
    {
        IShellLinkA * pIShellLink;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IShellLinkA, (void**)&pIShellLink);
        if (SUCCEEDED(hr))
        {
            ASSERT(pIShellLink);

#ifdef UNICODE
            CHAR szUrlA[INTERNET_MAX_URL_LENGTH];

            SHTCharToAnsi(CDFIDL_GetURL(pcdfidl), szUrlA, ARRAYSIZE(szUrlA));
            hr = pIShellLink->SetPath(szUrlA);
#else
            hr = pIShellLink->SetPath(CDFIDL_GetURL(pcdfidl));
#endif
            if (SUCCEEDED(hr))
            {
                //
                // The description ends up being the file name created.
                //

                TCHAR szPath[MAX_PATH];
#ifdef UNICODE
                CHAR  szPathA[MAX_PATH];
#endif

                StrCpyN(szPath, CDFIDL_GetName(pcdfidl), ARRAYSIZE(szPath) - 5);
                StrCat(szPath, TEXT(".url"));
#ifdef UNICODE
                SHTCharToAnsi(szPath, szPathA, ARRAYSIZE(szPathA));
                pIShellLink->SetDescription(szPathA);
#else
                pIShellLink->SetDescription(szPath);
#endif
                hr = pIShellLink->QueryInterface(riid, ppvOut);
            }

            pIShellLink->Release();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ASSERT((SUCCEEDED(hr) && *ppvOut) || (FAILED(hr) && NULL == *ppvOut));

    return hr;
}
