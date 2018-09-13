/*****************************************************************************
 *
 *	fndcm.c - IContextMenu interface
 *
 *****************************************************************************/

#include "fnd.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflCm

/*****************************************************************************
 *
 *	PICI
 *
 *	I'm getting lazy.
 *
 *****************************************************************************/

typedef LPCMINVOKECOMMANDINFO PICI;

/*****************************************************************************
 *
 *	Declare the interfaces we will be providing.
 *
 *	We must implement an IShellExtInit so the shell
 *	will know that we are ready for action.
 *
 *****************************************************************************/

  Primary_Interface(CFndCm, IContextMenu);
Secondary_Interface(CFndCm, IShellExtInit);

/*****************************************************************************
 *
 *	CFndCm
 *
 *	The context menu extension for "Find .... On the &Internet".
 *
 *****************************************************************************/

typedef struct CFndCm {

    /* Supported interfaces */
    IContextMenu 	cm;
    IShellExtInit	sxi;

} CFndCm, FCM, *PFCM;

typedef IContextMenu CM, *PCM;
typedef IShellExtInit SXI, *PSXI;
typedef IDataObject DTO, *PDTO;

/*****************************************************************************
 *
 *	CFndCm_QueryInterface (from IUnknown)
 *
 *	We need to check for our additional interfaces before falling
 *	through to Common_QueryInterface.
 *
 *****************************************************************************/

STDMETHODIMP
CFndCm_QueryInterface(PCM pcm, RIID riid, PPV ppvObj)
{
    PFCM this = IToClass(CFndCm, cm, pcm);
    HRESULT hres;
    if (IsEqualIID(riid, &IID_IShellExtInit)) {
	*ppvObj = &this->sxi;
	Common_AddRef(this);
	hres = NOERROR;
    } else {
	hres = Common_QueryInterface(this, riid, ppvObj);
    }
    AssertF(fLimpFF(FAILED(hres), *ppvObj == 0));
    return hres;
}

/*****************************************************************************
 *
 *	CFndCm_AddRef (from IUnknown)
 *	CFndCm_Release (from IUnknown)
 *
 *****************************************************************************/

#ifdef DEBUG
Default_AddRef(CFndCm)
Default_Release(CFndCm)
#else
#define CFndCm_AddRef Common_AddRef
#define CFndCm_Release Common_Release
#endif

/*****************************************************************************
 *
 *	CFndCm_Finalize (from Common)
 *
 *	Release the resources of an CFndCm.
 *
 *****************************************************************************/

void EXTERNAL
CFndCm_Finalize(PV pv)
{
    PFCM this = pv;

    EnterProc(CFndCm_Finalize, (_ "p", pv));

    ExitProc();
}


/*****************************************************************************
 *
 *	CFndCm_QueryContextMenu (From IContextMenu)
 *
 *	Given an existing context menu hmenu, insert new context menu
 *	items at location imi (imi = index to menu imi), returning the
 *	number of menu items added.
 *
 *	Our job is to add the "Find ... On the Internet" menu option.
 *
 *	hmenu     - destination menu
 *	imi	  - location at which menu items should be inserted
 *	idcMin	  - first available menu identifier
 *	idcMax    - first unavailable menu identifier
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszMyself[] = TEXT(".{37865980-75d1-11cf-bfc7-444553540000}");

#pragma END_CONST_DATA

STDMETHODIMP
CFndCm_QueryContextMenu(PCM pcm, HMENU hmenu, UINT imi,
			UINT idcMin, UINT idcMax, UINT uFlags)
{
    PFCM this = IToClass(CFndCm, cm, pcm);
    HRESULT hres;
    MENUITEMINFO mii;
    TCHAR tsz[256];
    SHFILEINFO sfi;
    EnterProc(CFndCm_QueryContextMenu, (_ "pu", pcm, idcMin));

    LoadString(g_hinst, IDS_ONTHEINTERNET, tsz, cA(tsz));

    SHGetFileInfo(c_tszMyself, FILE_ATTRIBUTE_DIRECTORY, &sfi, cbX(sfi),
		  SHGFI_SMALLICON |
		  SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_UNCHECKED;
    mii.wID = idcMin;
    mii.dwItemData = sfi.iIcon;
    mii.dwTypeData = tsz;

    InsertMenuItem(hmenu, imi, TRUE, &mii);

    hres = hresUs(1);

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	_CFndCm_InvokeFind
 *
 *	Go find it by launching the search URL.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszIEMain[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
TCHAR c_tszSearch[] = TEXT("Search Page");
TCHAR c_tszDefSearch[] = TEXT("http://http://www.msn.com/access/allinone.htm");

#pragma END_CONST_DATA

STDMETHODIMP
_CFndCm_InvokeFind(void)
{
    HRESULT hres;
    HKEY hk;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_tszIEMain,
		     0, KEY_ALL_ACCESS, &hk) == 0) {
	TCHAR tsz[MAX_PATH];
	LPCTSTR ptsz;
	DWORD cb = cbX(tsz);
	if (RegQueryValueEx(hk, c_tszSearch, 0, 0, tsz, &cb) == 0) {
	    ptsz = tsz;
	} else {
	    ptsz = c_tszDefSearch;
	}
	if (ShellExecute(0, 0, tsz, 0, 0, SW_NORMAL)) {
	    hres = S_OK;
	} else {
	    hres = E_FAIL;
	}
	RegCloseKey(hk);
    } else {
	hres = E_FAIL;
    }
    return hres;
}

/*****************************************************************************
 *
 *	CFndCm_InvokeCommand (from IContextMenu)
 *
 *	We have only one command, called "find".
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszFind[] = TEXT("find");

#pragma END_CONST_DATA

STDMETHODIMP
CFndCm_InvokeCommand(PCM pcm, PICI pici)
{
    PFCM this = IToClass(CFndCm, cm, pcm);
    HRESULT hres;
    EnterProc(CFndCm_InvokeCommand,
	    (_ HIWORD(pici->lpVerb) ? "pA" : "pu", pcm, pici->lpVerb));

    if (pici->cbSize >= sizeof(*pici)) {
	if (
#ifdef	SHELL32_IS_BUG_FREE //;Internal
	(HIWORD(pici->lpVerb) && lstrcmpi(c_tszFind, pici->lpVerb) == 0) || //;Internal
	     pici->lpVerb == 0 //;Internal
#else //;Internal
	fLimpFF(HIWORD(pici->lpVerb), lstrcmpi(c_tszFind, pici->lpVerb) == 0)
#endif //;Internal
	    ) {
	    hres = _CFndCm_InvokeFind();
	} else {
	    hres = E_INVALIDARG;
	}
    } else {
	hres = E_INVALIDARG;
    }
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CFndCm_GetCommandString (from IContextMenu)
 *
 *	Somebody wants to convert a command id into a string of some sort.
 *
 *****************************************************************************/

STDMETHODIMP
CFndCm_GetCommandString(PCM pcm, UINT_PTR idCmd, UINT uFlags, UINT *pwRsv,
			LPSTR pszName, UINT cchMax)
{
    PFCM this = IToClass(CFndCm, cm, pcm);
    HRESULT hres;
    EnterProc(CFndCm_GetCommandString, (_ "uu", idCmd, uFlags));

    if (idCmd == 0) {
	switch (uFlags) {
	case GCS_HELPTEXT:
	    if (cchMax) {
		pszName[0] = '\0';
		if (LoadString(g_hinst, IDS_FINDHELP, pszName, cchMax)) {
		    hres = NOERROR;
		} else {
		    hres = E_INVALIDARG;
		}
	    } else {
		hres = E_INVALIDARG;
	    }
	    break;

	case GCS_VALIDATE:
	    hres = NOERROR;
	    break;

	case GCS_VERB:
	    lstrcpyn(pszName, c_tszFind, cchMax);
	    hres = NOERROR;
	    break;

	default:
	    hres = E_NOTIMPL;
	    break;
	}
    } else {
	hres = E_INVALIDARG;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CFndCm_SXI_Initialize (from IShellExtension)
 *
 *****************************************************************************/

STDMETHODIMP
CFndCm_SXI_Initialize(PSXI psxi, PCIDL pidlFolder, PDTO pdto, HKEY hk)
{
    PFCM this = IToClass(CFndCm, sxi, psxi);
    HRESULT hres;
    EnterProc(CFndCm_SXI_Initialize, (_ ""));

    hres = S_OK;
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CFndCm_New (from IClassFactory)
 *
 *	Note that we release the pfcm that Common_New created, because we
 *	are done with it.  The real refcount is handled by the
 *	CFndCm_QueryInterface.
 *
 *****************************************************************************/

STDMETHODIMP
CFndCm_New(RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(CFndCm_New, (_ "G", riid));

    *ppvObj = 0;
    hres = Common_New(CFndCm, ppvObj);
    if (SUCCEEDED(hres)) {
	PFCM pfcm = *ppvObj;
	pfcm->sxi.lpVtbl = Secondary_Vtbl(CFndCm, IShellExtInit);
	hres = CFndCm_QueryInterface(&pfcm->cm, riid, ppvObj);
	Common_Release(pfcm);
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	The long-awaited vtbls
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

Primary_Interface_Begin(CFndCm, IContextMenu)
	CFndCm_QueryContextMenu,
	CFndCm_InvokeCommand,
	CFndCm_GetCommandString,
Primary_Interface_End(CFndCm, IContextMenu)

Secondary_Interface_Begin(CFndCm, IShellExtInit, sxi)
 	CFndCm_SXI_Initialize,
Secondary_Interface_End(CFndCm, IShellExtInit, sxi)
