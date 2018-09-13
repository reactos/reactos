//
//  ITBDROP.CPP
//  routines for implementing OLE drop target capability
//  within the internet toolbar control
//
//  History:
//      07/13/96 t-mkim     Created
//      10/13/96 chrisg     massive cleanup
//

#include "priv.h"
#include "itbdrop.h"
#include "sccls.h"

#include "resource.h"

#include "mluisupp.h"

#ifdef UNIX

#ifdef SIZEOF
#undef SIZEOF
#endif
#define SIZEOF(x)   sizeof(x)       // has been checked for UNICODE correctness

#endif

#define MAX_NAME_QUICKLINK 40

// Data type of the incoming data object.
#define CITBDTYPE_NONE      0
#define CITBDTYPE_HDROP     1
#define CITBDTYPE_URL       2
#define CITBDTYPE_TEXT      3



//  get an IDropTarget for shell special folders
//
HRESULT _GetSpecialDropTarget(UINT csidl, IDropTarget **ppdtgt)
{
    IShellFolder *psfDesktop;

    *ppdtgt = NULL;

    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidl;
        hres = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
        if (SUCCEEDED(hres))
        {
            IShellFolder *psf;
            hres = psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (void **)&psf);
            if (SUCCEEDED(hres))
            {
                hres = psf->CreateViewObject(NULL, IID_IDropTarget, (void **)ppdtgt);
                psf->Release();
            }

            ILFree(pidl);
        }
        psfDesktop->Release();
    }
    return hres;
}

//  Takes a variety of inputs and returns a string for drop targets.
//  szUrl:    the URL
//  szName:   the name (for quicklinks and the confo dialog boxes)
//  returns:  NOERROR if succeeded
//
HRESULT _GetURLData(IDataObject *pdtobj, int iDropType, TCHAR *pszUrl, DWORD cchUrl,  TCHAR *pszName)
{
    HRESULT hRes = NOERROR;
    STGMEDIUM stgmedium;
    UINT cfFormat;

    *pszName = 0;
    *pszUrl = 0;

    switch (iDropType)
    {
    case CITBDTYPE_HDROP:
        cfFormat = CF_HDROP;
        break;

    case CITBDTYPE_URL:
        InitClipboardFormats();
        cfFormat = g_cfURL;
        break;

    case CITBDTYPE_TEXT:
        cfFormat = CF_TEXT;
        break;

    default:
        return E_UNEXPECTED;
    }

    // Get the parse string
    LPCSTR pszURL = (LPCSTR)DataObj_GetDataOfType(pdtobj, cfFormat, &stgmedium);
    if (pszURL)
    {
        if (iDropType == CITBDTYPE_HDROP)
        {
            TCHAR szPath[MAX_PATH];
            SHFILEINFO sfi;
            DWORD_PTR bGotInfo;

            ASSERT(stgmedium.tymed == TYMED_HGLOBAL);

            DragQueryFile((HDROP)stgmedium.hGlobal, 0, szPath, SIZEOF(szPath));

            // defaults...
            lstrcpyn(pszUrl, szPath, MAX_URL_STRING);
            lstrcpyn(pszName, szPath, MAX_NAME_QUICKLINK);

            bGotInfo = SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES);
            if (bGotInfo)
                lstrcpyn(pszName, sfi.szDisplayName, MAX_NAME_QUICKLINK);

            if (bGotInfo && (sfi.dwAttributes & SFGAO_LINK))
            {
                LPITEMIDLIST pidl;
                if (SUCCEEDED(GetLinkTargetIDList(szPath, pszUrl, cchUrl, &pidl)))
                {
                    // we only care about the name... thanks anyway.
                    ILFree(pidl);
                }
            }
        }
        else
        {
#ifdef UNICODE
            WCHAR wszURL[MAX_URL_STRING];
            SHAnsiToUnicode(pszURL, wszURL, ARRAYSIZE(wszURL));
            LPTSTR pszURLData = wszURL;
#else
            LPTSTR pszURLData = pszURL;
#endif
            if (iDropType == CITBDTYPE_URL)
            {
                // defaults
                lstrcpyn(pszUrl,  pszURLData, MAX_URL_STRING);
                lstrcpyn(pszName, pszURLData, MAX_NAME_QUICKLINK);

                WCHAR szPath[MAX_PATH];

                if (SUCCEEDED(DataObj_GetNameFromFileDescriptor(pdtobj, szPath, ARRAYSIZE(szPath))))
                    PathToDisplayNameW(szPath, pszName, MAX_NAME_QUICKLINK);
                    
            }
            else // if (iDropType == CITBDTYPE_TEXT)
            {
                ASSERT(iDropType == CITBDTYPE_TEXT);

                lstrcpyn(pszUrl, pszURLData, MAX_URL_STRING);
                lstrcpyn(pszName, pszURLData, MAX_NAME_QUICKLINK);
            }
        }
    
        ReleaseStgMediumHGLOBAL(&stgmedium);
    }
    else
    {
        hRes = E_FAIL;
    }

    return hRes;
}

//  Displays a dialog asking for confirmation of drop-set operations.
//  Returns: User's response to the dialog box: YES = TRUE, NO = FALSE
//
BOOL _ConfirmChangeQuickLink(HWND hwndParent, TCHAR *pszName, int iTarget)
{
    MSGBOXPARAMS mbp;
    TCHAR szHeader[64];
    TCHAR szBuffer [MAX_NAME_QUICKLINK + 64];
    TCHAR szCaption [MAX_NAME_QUICKLINK + 64];
    UINT titleID, textID, iconID;

    switch (iTarget)
    {
    case TBIDM_HOME:
        titleID = IDS_SETHOME_TITLE;
        textID = IDS_SETHOME_TEXT;
        iconID = IDI_HOMEPAGE;
        break;
#if 0
    case TBIDM_SEARCH:
        titleID = IDS_SETSEARCH_TITLE;
        textID = IDS_SETSEARCH_TEXT;
        iconID = IDI_FRAME; // Warning if you unif0 this: IDI_FRAME is not in this dll
        break;
#endif

    default:
        return FALSE;           // We should never get here!
    }
    mbp.cbSize = sizeof (MSGBOXPARAMS);
    mbp.hwndOwner = hwndParent;
    mbp.hInstance = HinstShdocvw();
    mbp.dwStyle = MB_YESNO | MB_USERICON;
    MLLoadString(titleID, szCaption, ARRAYSIZE (szCaption));
    mbp.lpszCaption = szCaption;
    mbp.lpszIcon = MAKEINTRESOURCE (iconID);
    mbp.dwContextHelpId = 0;
    mbp.lpfnMsgBoxCallback = NULL;
    mbp.dwLanguageId = LANGIDFROMLCID (g_lcidLocale);

    MLLoadString(textID, szHeader, ARRAYSIZE (szHeader));
    wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szHeader, pszName);
    mbp.lpszText = szBuffer;

    return MessageBoxIndirect(&mbp) == IDYES;
}

//  Creates an instance of CITBarDropTarget. ptr is a pointer to the parent
//  CInternetToolbar.
//
CITBarDropTarget::CITBarDropTarget(HWND hwnd, int iTarget) : 
    _cRef(1), _iDropType(CITBDTYPE_NONE), _hwndParent(hwnd), _iTarget(iTarget)
{
}

STDMETHODIMP CITBarDropTarget::QueryInterface(REFIID iid, void **ppvObj)
{
    if (IsEqualIID (iid, IID_IUnknown) || IsEqualIID (iid, IID_IDropTarget))
    { 
        *ppvObj = SAFECAST(this, IDropTarget*);
        AddRef(); 
        return NOERROR; 
    } 

    *ppvObj = NULL; 
    return E_NOINTERFACE; 
}

STDMETHODIMP_(ULONG) CITBarDropTarget::AddRef()
{
    _cRef++;
    return _cRef;
}

STDMETHODIMP_(ULONG) CITBarDropTarget::Release()
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

typedef struct
{
    int iTarget;
    int iDropType;
    HWND hwnd;
    TCHAR szUrl [MAX_URL_STRING];
    TCHAR szName [MAX_NAME_QUICKLINK];
} DROPDATA;


DWORD CALLBACK ITBarDropThreadProc(void *pv)
{
    DROPDATA *pdd = (DROPDATA *)pv;

    switch (pdd->iTarget)
    {
    case TBIDM_HOME:

        if (pdd->iDropType != CITBDTYPE_TEXT)
        {
            if (_ConfirmChangeQuickLink(pdd->hwnd, pdd->szName, pdd->iTarget)) {
                ASSERT(pdd->iTarget == TBIDM_HOME);
                // currently don't support pdd->itarget == TBIDM_SEARCH
                //(pdd->iTarget == TBIDM_HOME) ? DVIDM_GOHOME : DVIDM_GOSEARCH);
                _SetStdLocation(pdd->szUrl, DVIDM_GOHOME);
            }
        }
        break;

    case TBIDM_SEARCH:
        ASSERT(0);
        break;
    }

    LocalFree(pdd);

    return 0;
}

STDMETHODIMP CITBarDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    ASSERT(pdtobj);
    _DragEnter(_hwndParent, ptl, pdtobj);
    if (_iTarget == TBIDM_FAVORITES)
    {
        if (SUCCEEDED(_GetSpecialDropTarget(CSIDL_FAVORITES, &_pdrop)))
            _pdrop->DragEnter(pdtobj, grfKeyState, ptl, pdwEffect);
        else
            *pdwEffect = DROPEFFECT_NONE;
        return NOERROR;
    }
    else if (_iTarget == TBIDM_HOME)
    {
        HKEY                hkeyRest = 0;
        DWORD               dwValue = 0;
        DWORD               dwLen = sizeof(DWORD);

        // Check if setting home page is restricted
    
        if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SET_HOMEPAGE_RESTRICTION, 0,
                         KEY_READ, &hkeyRest) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hkeyRest, REGVAL_HOMEPAGE_RESTRICTION, NULL, NULL,
                                (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS
                && dwValue)
            {
                return E_ACCESSDENIED;
            }
    
            RegCloseKey(hkeyRest);
        }
    }

    
    InitClipboardFormats();

    // Find the drop object's data format.
    FORMATETC fe = {g_cfURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (NOERROR == pdtobj->QueryGetData (&fe))
    {
        _iDropType = CITBDTYPE_URL;
    }
    else if (fe.cfFormat = CF_HDROP, NOERROR == pdtobj->QueryGetData (&fe))
    {
        _iDropType = CITBDTYPE_HDROP;
    }
    else if (fe.cfFormat = CF_TEXT, NOERROR == pdtobj->QueryGetData (&fe))
    {
        _iDropType = CITBDTYPE_TEXT;
        // We want to eventually pick through the text for an
        // URL, but right now we just leave it unmolested.
    }
    DragOver (grfKeyState, ptl, pdwEffect);
    return NOERROR;
}

STDMETHODIMP CITBarDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DWORD dwEffectAvail;
    _DragMove(_hwndParent, ptl);
    if (_iTarget == TBIDM_FAVORITES)
    {
        if (_pdrop)
            return _pdrop->DragOver(grfKeyState, ptl, pdwEffect);

        *pdwEffect = DROPEFFECT_NONE;
        return NOERROR;
    }
    ASSERT(!_pdrop);

    if (_iDropType == CITBDTYPE_NONE)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return NOERROR;
    }

    dwEffectAvail = DROPEFFECT_NONE;
    switch (_iTarget)
    {
        case TBIDM_HOME:
        case TBIDM_SEARCH:
            if (_iDropType == CITBDTYPE_TEXT)
            {
                // CF_TEXT doesn't do link.
            }
            else
                dwEffectAvail = DROPEFFECT_LINK;
            break;
    }
    *pdwEffect &= dwEffectAvail;
    return NOERROR;
}

STDMETHODIMP CITBarDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    BOOL fSafe = TRUE;
    LPITEMIDLIST pidl;

    if (_pdrop)
    {
        ASSERT(_iTarget == TBIDM_FAVORITES);

        //
        // Force a linking since we are passing straight through to the folder.
        // This avoids confusion when dragging to the toolbar button.
        //
        // BUGBUG: this should really go through the "Add to Favorites" UI
        //

        // When forcing a link, make sure that you can move it. If you cannot move it,
        // then we rely on the prefered effect of the data object. Why? Well, the history
        // folder only allows a copy. If you just whack this to LINK, the shell folder hoses
        // the drag images (Does a DAD_SetDragImage(NULL), blowing away the information about
        // the last locked window, without unlocking it.). So, if you can move the item, 
        // you can link to it (I guess), but if you cannot move it, do whatever. 
        //     - (lamadio) 1.3.99
        if (*pdwEffect & DROPEFFECT_MOVE)
            *pdwEffect = DROPEFFECT_LINK;

        if (TBIDM_FAVORITES == _iTarget &&
            SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {
            fSafe = IEIsLinkSafe(_hwndParent, pidl, ILS_ADDTOFAV);
            ILFree(pidl);
        }

        if (fSafe)
        {
            _pdrop->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        }
        else
        {
            pdtobj->Release();  // Match Release called in _pdrop->Drop.
        }
       
        DAD_DragLeave();

        _pdrop->Release();
        _pdrop = NULL;
    }
    else
    {
        if (TBIDM_HOME == _iTarget &&
            SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {
            fSafe = IEIsLinkSafe(_hwndParent, pidl, ILS_HOME);
            ILFree(pidl);
        }

        if (fSafe)
        {
            DROPDATA *pdd = (DROPDATA *)LocalAlloc (LPTR, sizeof(DROPDATA));
            if (pdd)
            {
                pdd->iTarget = _iTarget;
                pdd->iDropType = _iDropType;
                pdd->hwnd = _hwndParent;

                // do this async so we don't block the source of the drag durring our UI
                if (FAILED(_GetURLData(pdtobj, _iDropType, pdd->szUrl, ARRAYSIZE(pdd->szUrl), pdd->szName)) ||
                    !SHCreateThread(ITBarDropThreadProc, pdd, 0, NULL))
                    LocalFree(pdd);
            }
        }
        DragLeave();
    }
    return NOERROR;
}

STDMETHODIMP CITBarDropTarget::DragLeave(void)
{
    DAD_DragLeave();
    // Check if we should to pass to the favorites dt.
    if (_pdrop)
    {
        ASSERT(_iTarget == TBIDM_FAVORITES);
        _pdrop->DragLeave();
        _pdrop->Release();
        _pdrop = NULL;
    }
    _iDropType = CITBDTYPE_NONE;

    return NOERROR;
}
