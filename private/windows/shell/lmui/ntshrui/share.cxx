//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       share.cxx
//
//  Contents:   Shell extension handler for sharing
//
//  Classes:    CShare
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <shrpage.hxx>

#define DONT_WANT_SHELLDEBUG
#include <shlobjp.h>    // SHObjectProperties
#include <shlwapi.h>    // PathCombine

#include "share.hxx"
#include "acl.hxx"
#include "util.hxx"
#include "resource.h"

//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Member:     CShare::CShare
//
//  Synopsis:   Constructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::CShare(VOID) :
    _uRefs(1),
    _fPathChecked(FALSE),
	_fMultipleSharesSelected (FALSE)
{
    INIT_SIG(CShare);
    _szPath[0] = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::~CShare
//
//  Synopsis:   Destructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::~CShare()
{
    CHECK_SIG(CShare);
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::Initialize
//
//  Derivation: IShellExtInit
//
//  Synopsis:   Initialize the shell extension. Stashes away the argument data.
//
//  History:    4-Apr-95    BruceFo  Created
//
//  Notes:      This method can be called more than once.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::Initialize(
    LPCITEMIDLIST   pidlFolder,
    LPDATAOBJECT    pDataObject,
    HKEY            hkeyProgID
    )
{
    CHECK_SIG(CShare);

    if (pDataObject && _szPath[0] == 0)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        HRESULT hr = pDataObject->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))
        {
			// Get the count of shares that have been selected.  Display the page only
			// if 1 share is selected but not for multiple shares.
			UINT	nCntFiles = ::DragQueryFile ((HDROP) medium.hGlobal, -1, _szPath, ARRAYLEN (_szPath));
			if ( nCntFiles > 1 )
				_fMultipleSharesSelected = TRUE;

            DragQueryFile((HDROP)medium.hGlobal, 0, _szPath, ARRAYLEN(_szPath));
            ReleaseStgMedium(&medium);
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::AddPages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (from shlobj.h)
//              "The explorer calls this member function when it finds a
//              registered property sheet extension for a particular type
//              of object. For each additional page, the extension creates
//              a page object by calling CreatePropertySheetPage API and
//              calls lpfnAddPage.
//
//  Arguments:  lpfnAddPage -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    CHECK_SIG(CShare);

    if (_OKToShare() && !_fMultipleSharesSelected )
    {
        appAssert(_szPath[0]);

        //
        //  Create a property sheet page object from a dialog box.
        //

        PWSTR pszPath = NewDup(_szPath);
        if (NULL == pszPath)
        {
            return E_OUTOFMEMORY;
        }

        // Now we have pszPath memory to delete

        CSharingPropertyPage* pPage = new CSharingPropertyPage(pszPath, FALSE);
        if (NULL == pPage)
        {
            delete[] pszPath;
            return E_OUTOFMEMORY;
        }

        // Now the pPage object owns pszPath memory. However, we have
        // pPage memory to delete.

        HRESULT hr = pPage->InitInstance();
        if (FAILED(hr))
        {
            delete pPage;
            return E_OUTOFMEMORY;
        }

        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);    // no extra data.
        psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
        psp.hInstance   = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
        psp.hIcon       = NULL;
        psp.pszTitle    = NULL;
        psp.pfnDlgProc  = CSharingPropertyPage::DlgProcPage;
        psp.lParam      = (LPARAM)pPage;  // transfer ownership
        psp.pfnCallback = CSharingPropertyPage::PageCallback;
        psp.pcRefParent = &g_NonOLEDLLRefs;

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
        if (NULL == hpage)
        {
            // If CreatePropertySheetPage fails, we still have pPage memory
            // to delete.
            delete pPage;
            return E_OUTOFMEMORY;
        }

        BOOL fAdded = (*lpfnAddPage)(hpage, lParam);
        if (!fAdded)
        {
            // At this point, pPage memory, as the lParam of a PROPSHEETPAGE
            // that has been converted into an HPROPSHEETPAGE, is owned by the
            // hpage. Calling DestroyPropertySheetPage will invoke the
            // PageCallback function, subsequently destroying the pPage object,
            // and hence the pszPath object within it. Whew!

            DestroyPropertySheetPage(hpage);
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::ReplacePages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (From shlobj.h)
//              "The explorer never calls this member of property sheet
//              extensions. The explorer calls this member of control panel
//              extensions, so that they can replace some of default control
//              panel pages (such as a page of mouse control panel)."
//
//  Arguments:  uPageID -- Specifies the page to be replaced.
//              lpfnReplace -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM               lParam
    )
{
    CHECK_SIG(CShare);

    appAssert(!"CShare::ReplacePage called, not implemented");
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::QueryContextMenu
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when shell wants to add context menu items.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    CHECK_SIG(CShare);

    if ((hmenu == NULL) || (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY)))
    {
        return S_OK;
    }

    int  cNumberAdded = 0;
    UINT idCmd        = idCmdFirst;

    if (_OKToShare())
    {
        appAssert(_szPath[0]);

        WCHAR szShareMenuItem[50];
        LoadString(g_hInstance, IDS_SHARING, szShareMenuItem, ARRAYLEN(szShareMenuItem));

        if (InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmd++, szShareMenuItem))
        {
            cNumberAdded++;
            InsertMenu(hmenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        }
    }
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)cNumberAdded));
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::InvokeCommand
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to invoke a context menu item.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::InvokeCommand(
    LPCMINVOKECOMMANDINFO pici
    )
{
    CHECK_SIG(CShare);

    HRESULT hr = E_INVALIDARG;  // assume error.

    if (0 == HIWORD(pici->lpVerb))
    {
        appAssert(_szPath[0]);
        TCHAR szShare[50];

        LoadString(g_hInstance, IDS_MSGTITLE, szShare, ARRAYLEN(szShare));

        SHObjectProperties(pici->hwnd, SHOP_FILEPATH, _szPath, szShare);
        hr = S_OK;
    }
    else
    {
        // BUGBUG: compare the strings if not a MAKEINTRESOURCE?
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::GetCommandString
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to get a help string or the
//              menu string.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT*       pwReserved,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    CHECK_SIG(CShare);

    if (uType == GCS_HELPTEXT)
    {
        LoadStringW(g_hInstance, IDS_MENUHELP, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
    else
    {
        LoadStringW(g_hInstance, IDS_SHARING, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::_IsShareableDrive
//
//  Synopsis:   Determines if the drive letter of the current path (_szPath)
//              is shareable. It is if it is local, not remote.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShare::_IsShareableDrive(VOID)
{
    CHECK_SIG(CShare);

    // If this is a regular path it can be shared unless
    // it is redirected.

    if (  (_szPath[0] >= L'A' && _szPath[0] <= L'Z') && _szPath[1] == L':')
    {
        WCHAR szRoot[4];

        szRoot[0] = _szPath[0];
        szRoot[1] = TEXT(':');
        szRoot[2] = TEXT('\\');
        szRoot[3] = 0;

        UINT uType = GetDriveType(szRoot);

        switch (uType)
        {
            case DRIVE_UNKNOWN:
            case DRIVE_NO_ROOT_DIR:
            case DRIVE_REMOTE:
               return FALSE;

            case DRIVE_FIXED:
            case DRIVE_REMOVABLE:
                {
                    WCHAR szDesktopIni[MAX_PATH];
                    PathCombine(szDesktopIni, _szPath, TEXT("desktop.ini"));
                    return GetPrivateProfileInt(TEXT(".ShellClassInfo"), TEXT("Sharing"), TRUE, szDesktopIni);
                }
            default:
               return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::_OKToShare
//
//  Synopsis:   Determine if it is ok to share the current object. It stashes
//              away the current path by querying the cached IDataObject.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShare::_OKToShare(VOID)
{
    CHECK_SIG(CShare);

    if (!g_fSharingEnabled)
    {
        return FALSE;
    }

    if (!_fPathChecked)
    {
        _fPathChecked = TRUE;
        _fOkToSharePath = _IsShareableDrive();
    }

    return _fOkToSharePath;
}


// dummy function to export to get linking to work

HRESULT SharePropDummyFunction()
{
    return S_OK;
}
