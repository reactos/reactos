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
#include <shsemip.h>

#include "share.hxx"
#include "acl.hxx"
#include "util.hxx"
#include "resource.h"

//--------------------------------------------------------------------------

typedef
BOOL
(WINAPI *SHOBJECTPROPERTIES)(
    HWND  hwndOwner,
    DWORD dwType,
    LPCTSTR lpObject,
    LPCTSTR lpPage
    );

SHOBJECTPROPERTIES g_pSHObjectProperties = NULL;

BOOL
LoadShellDllEntries(
    VOID
    );

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

CShare::CShare(
    VOID
    )
    :
    _uRefs(0),
    _pDataObject(NULL),
    _hkeyProgID(NULL),
    _pszPath(NULL),
    _fPathChecked(FALSE)
{
    INIT_SIG(CShare);

    AddRef(); // give it the correct initial reference count. add to the DLL reference count
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

    if (_pDataObject)
    {
        _pDataObject->Release();
    }

    if (_hkeyProgID)
    {
        LONG l = RegCloseKey(_hkeyProgID);
        if (l != ERROR_SUCCESS)
        {
            appDebugOut((DEB_ERROR, "CShare::destructor. Error closing registry key, 0x%08lx\n", l));
        }
        _hkeyProgID = NULL;
    }

    delete[] _pszPath;
    _pszPath = NULL;

    // force path to be checked again, but never need to re-check the server
    _fPathChecked = FALSE;
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

    if (!LoadShellDllEntries())
    {
        appDebugOut((DEB_ERROR, "CShare::Initialize. Couldn't load shell32.dll entrypoints\n"));
        return E_FAIL;
    }

    CShare::~CShare();

    // Duplicate the pDataObject pointer
    _pDataObject = pDataObject;
    if (pDataObject)
    {
        pDataObject->AddRef();
    }

    // Duplicate the handle
    if (hkeyProgID)
    {
        LONG l = RegOpenKeyEx(hkeyProgID, NULL, 0L, MAXIMUM_ALLOWED, &_hkeyProgID);
        if (l != ERROR_SUCCESS)
        {
            appDebugOut((DEB_ERROR, "CShare::Initialize. Error duplicating registry key, 0x%08lx\n", l));
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

    if (_OKToShare())
    {
        appAssert(NULL != _pszPath);

        //
        //  Create a property sheet page object from a dialog box.
        //

		int size = sizeof(SHARE_PAGE_INFO) + (wcslen(_pszPath) + 1) * sizeof(WCHAR);
		if (_bRemote)
		{
			size += (wcslen(_szServer) + 1 + wcslen(_szShare) + 1 + wcslen(_szRemotePath) + 1) * sizeof(WCHAR);
		}
		LPBYTE pBuf = new BYTE[size];
        if (NULL == pBuf)
        {
            return E_OUTOFMEMORY;
        }

		SHARE_PAGE_INFO* pInfo = (SHARE_PAGE_INFO*)pBuf;
		PWSTR pszStrings = (PWSTR)((LPBYTE)pInfo + sizeof(SHARE_PAGE_INFO));

		pInfo->pszPath = pszStrings;
		wcscpy(pInfo->pszPath, _pszPath);
		pszStrings += wcslen(_pszPath) + 1;

		pInfo->bRemote = _bRemote;
		if (_bRemote)
		{
			pInfo->pszServer = pszStrings;
			wcscpy(pInfo->pszServer, _szServer);
			pszStrings += wcslen(_szServer) + 1;

			pInfo->pszShare = pszStrings;
			wcscpy(pInfo->pszShare, _szShare);
			pszStrings += wcslen(_szShare) + 1;

			pInfo->pszRemotePath = pszStrings;
			wcscpy(pInfo->pszRemotePath, _szRemotePath);

			pszStrings += wcslen(_szRemotePath) + 1;

			appAssert((LPBYTE)pszStrings == (LPBYTE)pInfo + size);
		}
		else
		{
			pInfo->pszServer     = NULL;
			pInfo->pszShare      = NULL;
			pInfo->pszRemotePath = NULL;
		}

        appDebugOut((DEB_TRACE,
            "SHARE_PAGE_INFO: pInfo(0x%x), path(0x%x) %ws, remote %ws, server(0x%x) %ws, share(0x%x) %ws, remote path(0x%x) %ws\n",
			pInfo,
			pInfo->pszPath,
			pInfo->pszPath,
			pInfo->bRemote ? L"TRUE" : L"FALSE",
			pInfo->pszServer,
			pInfo->pszServer,
			pInfo->pszShare,
			pInfo->pszShare,
			pInfo->pszRemotePath,
			pInfo->pszRemotePath
            ));

        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);    // no extra data.
        psp.dwFlags     = PSP_USEREFPARENT;
        psp.hInstance   = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
        psp.hIcon       = NULL;
        psp.pszTitle    = NULL;
        psp.pfnDlgProc  = CSharingPropertyPage::DlgProcPage;
        psp.lParam      = (LPARAM)pInfo;  // transfer ownership
        psp.pfnCallback = NULL;
        psp.pcRefParent = &g_NonOLEDLLRefs;

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
        if (NULL != hpage)
        {
            if (!lpfnAddPage(hpage, lParam))
            {
				delete[] (BYTE*)pInfo;
                DestroyPropertySheetPage(hpage);
            }
        }
		else
		{
			delete[] (BYTE*)pInfo;
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

    if ((hmenu == NULL)
        || (uFlags & CMF_DEFAULTONLY)
        || (uFlags & CMF_VERBSONLY))
    {
        return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 0));
    }

    int  cNumberAdded = 0;
    UINT idCmd        = idCmdFirst;

    if (_OKToShare())
    {
        appAssert(NULL != _pszPath);

        WCHAR szShareMenuItem[50];
        LoadString(g_hInstance, IDS_SHARING, szShareMenuItem, ARRAYLEN(szShareMenuItem));

        if (InsertMenu(
                hmenu,
                indexMenu,
                MF_STRING | MF_BYPOSITION,
                idCmd++,
                szShareMenuItem))
        {
            cNumberAdded++;
            InsertMenu(hmenu, indexMenu, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
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

    HWND hwnd = pici->hwnd;
    LPCSTR pszCmd = pici->lpVerb;

    HRESULT hr = ResultFromScode(E_INVALIDARG);  // assume error.

    if (0 == HIWORD(pszCmd))
    {
        if (NULL != g_pSHObjectProperties)
        {
            appAssert(NULL != _pszPath);

            (*g_pSHObjectProperties)(hwnd, SHOP_FILEPATH, _pszPath, g_szShare);
            hr = S_OK;
        }
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
    UINT        idCmd,
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
//  Synopsis:   Determines if the drive letter of the current path (_pszPath)
//              is shareable. It is if it is local, not remote.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShare::_IsShareableDrive(
    VOID
    )
{
    CHECK_SIG(CShare);
    appAssert(_pszPath != NULL);

    // If this is a regular path it can be shared unless
    // it is redirected.

	_bRemote = FALSE;

    if (  (_pszPath[0] >= L'A' && _pszPath[0] <= L'Z')
		&& _pszPath[1] == L':'
		&& _pszPath[2] == L'\\'
		)
    {
        WCHAR szRoot[] = L"X:\\";
        szRoot[0] = _pszPath[0];
        switch (GetDriveType(szRoot))
        {
            case DRIVE_REMOTE:
			{
                appDebugOut((DEB_TRACE,
                       	"Remote drive letter %wc:\n",
                       	_pszPath[0]));

				_bRemote = TRUE;

				BOOL bReturn = FALSE;

			    // we'll allow remote drives so long as the user has permission
			    // to NetShareGetInfo level 502 to the root.
			    WCHAR szLocalName[3];
			    szLocalName[0] = _pszPath[0];
			    szLocalName[1] = L':';
			    szLocalName[2] = L'\0';
				BYTE buf[sizeof(WNET_CONNECTIONINFO) + MAX_PATH * 2];
				LPWNET_CONNECTIONINFO pInfo = (LPWNET_CONNECTIONINFO)buf;
			    DWORD bufSize = ARRAYLEN(buf);
			    DWORD err = WNetGetConnection2(szLocalName, buf, &bufSize);
			    if (err == NO_ERROR)
			    {
                  	appDebugOut((DEB_TRACE,
                        	"Remote drive letter maps to path %ws, provider %ws\n",
                        	pInfo->lpRemoteName, pInfo->lpProvider));

					// is it the Microsoft network provider?
					DWORD dwNetType;
					err = WNetGetProviderType(pInfo->lpProvider, &dwNetType);
					if (err == NO_ERROR)
					{
						if (HIWORD(dwNetType) == HIWORD(WNNC_NET_LANMAN))
						{
							// ok, it's lanman. Parse out the share name.
							LPWSTR psz = pInfo->lpRemoteName;
							LPWSTR pszT;
							if (NULL != psz
								&& (psz[0] == L'\\')
								&& (psz[1] == L'\\')
								&& (psz[2] != L'\\' && psz[2] != L'\0')
								&& (NULL != (pszT = wcschr(&psz[3], L'\\')))
								&& (++pszT, *pszT != L'\\' && *pszT != L'\0')
								)
							{
								// ok, now pszT points to the share name.
								int serverLen = (pszT - 1) - (psz + 2);
								int shareLen;
								LPWSTR pszT2 = wcschr(pszT, L'\\');
								if (NULL == pszT2)
								{
									shareLen = wcslen(pszT);
								}
								else
								{
									shareLen = pszT2 - pszT;
								}

								if (serverLen < ARRAYLEN(_szServer)
									&& shareLen < ARRAYLEN(_szShare)
									)
								{
									wcsncpy(_szServer, psz + 2, serverLen);
									_szServer[serverLen] = L'\0';

									wcsncpy(_szShare, pszT, shareLen);
									_szShare[shareLen] = L'\0';

                    				appDebugOut((DEB_TRACE,
                        				"Remote directory, server %ws, share %ws\n",
                        				_szServer, _szShare));

									SHARE_INFO_502* pShareInfo;
									NET_API_STATUS status = NetShareGetInfo(
																_szServer,
																_szShare,
																502,
																(LPBYTE*)&pShareInfo);
									if (NERR_Success == status)
									{
										// ok, we've got a path like
										// "x:\foobar", which is a redirected
										// drive. In UNC form, we've got
										// \\server\share\foobar. But, we need
										// to know the path to this object on
										// the remote machine so we can pass
										// it back to NetShareAdd, etc. So
										// save it.

										// BUGBUG: assuming it's not too big.
										wcscpy(_szRemotePath, pShareInfo->shi502_path);
										// make sure there is a separating
										// backslash
										int rlen = wcslen(_szRemotePath);
										if (_szRemotePath[rlen - 1] != L'\\')
										{
											// no trailing backslash
											wcscat(_szRemotePath, L"\\");
										}
										wcscat(_szRemotePath, &_pszPath[3]);

                    					appDebugOut((DEB_TRACE,
                        					"Remote path %ws\n",
                        					_szRemotePath));

										bReturn = TRUE;			// YES!!!!!!!
										NetApiBufferFree(pShareInfo);
									}
								}
							}
						}
					}
			    }
                return bReturn;
			}

            case DRIVE_FIXED:
            case DRIVE_REMOVABLE:
            case DRIVE_CDROM:
               return TRUE;

			default:
				return FALSE;
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
CShare::_OKToShare(
    VOID
    )
{
    CHECK_SIG(CShare);

    if (!g_fSharingEnabled)
    {
        return FALSE;
    }

    if (!_fPathChecked)
    {
        _fPathChecked = TRUE;
        _fOkToSharePath = FALSE;

        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        appAssert(NULL != _pDataObject);
        HRESULT hr = _pDataObject->GetData(&fmte, &medium);
        CHECK_HRESULT(hr);
        if (SUCCEEDED(hr))
        {
            UINT cObjects = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
            if (1 == cObjects)
            {
                WCHAR szPath[LM20_PATHLEN + 3];

                HDROP hdrop = (HDROP)medium.hGlobal;
                WCHAR wszPath[MAX_PATH];
                DragQueryFile(hdrop, 0, wszPath, ARRAYLEN(wszPath));

                _pszPath = NewDup(wszPath);
                if (NULL != _pszPath)
                {
                    _fOkToSharePath = _IsShareableDrive();

                    appDebugOut((DEB_TRACE,
                        "ok to share %ws?: %ws\n",
                        _pszPath, _fOkToSharePath ? L"yes" : L"no"));
                }
            }
            else
            {
                appDebugOut((DEB_TRACE,"_OKToShare: Got %d objects, disallowing sharing\n", cObjects));
            }

            ReleaseStgMedium(&medium);
        }
        else
        {
            appDebugOut((DEB_TRACE,
                "_OKToShare: IDataObject::GetData failed, 0x%08lx\n",
                hr));
        }
    }

    return _fOkToSharePath;
}


//+-------------------------------------------------------------------------
//
//  Function:   LoadShellDllEntries
//
//  Synopsis:   Get addresses of functions in shell32.dll
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
LoadShellDllEntries(
    VOID
    )
{
    static BOOL s_fEntrypointsChecked = FALSE;

    if (!s_fEntrypointsChecked)
    {
        // only check once!
        s_fEntrypointsChecked = TRUE;

        HINSTANCE hShellLibrary = LoadLibrary(L"shell32.dll");
        if (NULL != hShellLibrary)
        {
            g_pSHObjectProperties =
                (SHOBJECTPROPERTIES)GetProcAddress(hShellLibrary,
                            (LPCSTR)(MAKELONG(SHObjectPropertiesORD, 0)) );
        }
    }

    return (NULL != g_pSHObjectProperties);
}


// dummy function to export to get linking to work

HRESULT SharePropDummyFunction()
{
    return S_OK;
}
