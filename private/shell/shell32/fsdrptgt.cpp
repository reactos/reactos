#include "shellprv.h"
#include "caggunk.h"
#include "datautil.h"
#include "ids.h"
#include "defview.h"
#include "_security.h"
#include "shitemid.h"
#include "idlcomm.h"
#include "cowsite.h"

extern "C"
{
#include "bookmk.h"
#include "fstreex.h"
}

#define TF_DRAGDROP 0x04000000

STDAPI_(BOOL) ExtractImageURLFromCFHTML(LPSTR pszHTML, SIZE_T cbSizeHTML, LPSTR pszImg, DWORD dwSize);
STDAPI_(BOOL) IsFromSneakernetBriefcase(LPCITEMIDLIST pidlSource, LPCTSTR pszTarget);
STDAPI_(BOOL) IsBriefcaseRoot(IDataObject *pDataObj);
STDAPI_(BOOL) DroppingAnyFolders(HDROP hDrop);

typedef struct
{
    HWND    hwnd;
    DWORD   dwFlags;
    POINTL  pt;
    CHAR    szUrl[INTERNET_MAX_URL_LENGTH];
} ADDTODESKTOP;


DWORD CALLBACK AddToActiveDesktopThreadProc(void *pv)
{
    ADDTODESKTOP* pToAD = (ADDTODESKTOP*)pv;
    CHAR szFilePath[MAX_PATH];
    DWORD cchFilePath = SIZECHARS(szFilePath);
    BOOL fAddComp = TRUE;

    if (SUCCEEDED(PathCreateFromUrlA(pToAD->szUrl, szFilePath, &cchFilePath, 0)))
    {
        TCHAR szPath[MAX_PATH];

        SHAnsiToTChar(szFilePath, szPath, ARRAYSIZE(szPath));

        // If the Url is in the Temp directory
        if (PathIsTemporary(szPath))
        {
            if (IDYES == ShellMessageBox(g_hinst, pToAD->hwnd, MAKEINTRESOURCE(IDS_REASONS_URLINTEMPDIR),
                MAKEINTRESOURCE(IDS_AD_NAME), MB_YESNO | MB_ICONQUESTION))
            {
                TCHAR szFilter[64], szTitle[64];
                TCHAR szFilename[MAX_PATH];
                LPTSTR psz;
                OPENFILENAME ofn = { 0 };

                LoadString(g_hinst, IDS_ALLFILESFILTER, szFilter, ARRAYSIZE(szFilter));
                LoadString(g_hinst, IDS_SAVEAS, szTitle, ARRAYSIZE(szTitle));

                psz = szFilter;

                //Strip out the # and make them Nulls for SaveAs Dialog
                while (*psz)
                {
                    if (*psz == (WCHAR)('#'))
                        *psz = (WCHAR)('\0');
                    psz++;
                }

                lstrcpy(szFilename, PathFindFileName(szPath));

                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = pToAD->hwnd;
                ofn.hInstance = g_hinst;
                ofn.lpstrFilter = szFilter;
                ofn.lpstrFile = szFilename;
                ofn.nMaxFile = ARRAYSIZE(szFilename);
                ofn.lpstrTitle = szTitle;
                ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

                if (GetSaveFileName(&ofn))
                {
                    SHFILEOPSTRUCT sfo = { 0 };

                    szPath[lstrlen(szPath) + 1] = 0;
                    ofn.lpstrFile[lstrlen(ofn.lpstrFile) + 1] = 0;

                    sfo.hwnd = pToAD->hwnd;
                    sfo.wFunc = FO_COPY;
                    sfo.pFrom = szPath;
                    sfo.pTo = ofn.lpstrFile;

                    cchFilePath = SIZECHARS(szPath);
                    if (SHFileOperation(&sfo) == 0 &&
                        SUCCEEDED(UrlCreateFromPath(szPath, szPath, &cchFilePath, 0)))
                    {
                        SHTCharToAnsi(szPath, pToAD->szUrl, ARRAYSIZE(pToAD->szUrl));
                    }
                    else
                        fAddComp = FALSE;
                }
                else
                    fAddComp = FALSE;
            }
            else
                fAddComp = FALSE;
        }
    }
    if (fAddComp)
        CreateDesktopComponents(pToAD->szUrl, NULL, pToAD->hwnd, pToAD->dwFlags, pToAD->pt.x, pToAD->pt.y);

    LocalFree((HLOCAL)pToAD);

    return 0;
}

// {F20DA720-C02F-11CE-927B-0800095AE340}
const GUID CLSID_CPackage = {0xF20DA720L, 0xC02F, 0x11CE, 0x92, 0x7B, 0x08, 0x00, 0x09, 0x5A, 0xE3, 0x40};
// old packager guid...
// {0003000C-0000-0000-C000-000000000046}
const GUID CLSID_OldPackage = {0x0003000CL, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46};


typedef struct {
    DWORD        dwDefEffect;
    IDataObject *pdtobj;
    POINTL       pt;
    DWORD *      pdwEffect;
    HKEY         hkeyProgID;
    HKEY         hkeyBase;
    HMENU        hmenu;
    UINT         idCmd;
    DWORD        grfKeyState;
} FSDRAGDROPMENUPARAM;

typedef struct
{
    HMENU   hMenu;
    UINT    uCopyPos;
    UINT    uMovePos;
    UINT    uLinkPos;
} FSMENUINFO;

class CFSDropTarget : public CAggregatedUnknown, CObjectWithSite, public IDropTarget
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // *** IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

protected:
    CFSDropTarget(IUnknown* punkOuter);
    ~CFSDropTarget();

    HRESULT v_InternalQueryInterface(REFIID riid, void** ppvObj);
    HRESULT _Init(CFSFolder* pFSFolder, HWND hwnd);
    BOOL _IsDesktopFolder() { return _pFolder->_pidl && ILIsEmpty(_pFolder->_pidl); };
    
    HRESULT _GetDDInfoFileContents(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                   DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoDeskCompHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                    DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                            DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoHIDA(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                           DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoDeskImage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoDeskComp(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                               DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoOlePackage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                 DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoOleObj(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                             DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _GetDDInfoOleLink(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                              DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    
    DWORD _PickDefFSOperation(DWORD dwCurEffectAvail);
    HRESULT _GetPath(LPTSTR pszPath);
    HRESULT _ZoneCheckDataObject(DWORD dwEffect);
    DWORD _LimitDefaultEffect(DWORD dwDefEffect, DWORD dwEffectsAllowed);
    DWORD _GetStdDefEffect(DWORD grfKeyState, DWORD dwCurEffectAvail, DWORD dwAllEffectAvail, DWORD dwOrigDefEffect);
    DWORD _DetermineEffects(DWORD grfKeyState, DWORD *pdwEffectInOut, HMENU hmenu);
    static VOID _AddVerbs(DWORD* pdwEffects,
                          DWORD dwEffectAvail,
                          DWORD dwDefEffect,
                          UINT idCopy,
                          UINT idMove,
                          UINT idLink,
                          DWORD dwForceEffect,
                          FSMENUINFO* pfsMenuInfo);
    HRESULT _DragDropMenu(FSDRAGDROPMENUPARAM *pddm);
    HRESULT _CreatePackage();
    HRESULT _CreateURLDeskComp(int x, int y);
    HRESULT _CreateDeskCompImage(POINTL pt);
    void _GetStateFromSite();

    CFSFolder       *_pFolder;
    HWND            _hwndOwner;             // EVIL: used as a site and UI host
    DWORD           _grfKeyStateLast;       // for previous DragOver/Enter
    IDataObject*    _pdtobj;
    DWORD           _dwEffectLastReturned;  // stashed effect that's returned by base class's dragover
    DWORD           _dwData;                // DTID_*
    DWORD           _dwEffectPreferred;     // if dwData & DTID_PREFERREDEFFECT
    BOOL            _fSameHwnd;             // the drag source and target are the same folder
    BOOL            _fDragDrop;             // 
    BOOL            _fBkDropTarget;
    POINT           _ptDrop;
    
private:
    friend HRESULT CFSDropTarget_CreateInstance(CFSFolder* pFSFolder, HWND hwnd, IDropTarget** ppdt);
};

STDAPI CFSDropTarget_CreateInstance(CFSFolder* pFSFolder, HWND hwnd, IDropTarget** ppdt)
{
    ASSERT(pFSFolder && ppdt);
    HRESULT hr;

    *ppdt = NULL;
    
    CFSDropTarget* pFSDT = new CFSDropTarget(NULL);
    if (pFSDT)
    {
        hr = pFSDT->_Init(pFSFolder, hwnd);
        if (SUCCEEDED(hr))
            pFSDT->QueryInterface(IID_PPV_ARG(IDropTarget, ppdt));
        pFSDT->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

CFSDropTarget::CFSDropTarget(IUnknown* punkOuter) :
    CAggregatedUnknown(punkOuter)
{
    ASSERT(NULL == _hwndOwner);
    ASSERT(0 == _grfKeyStateLast);
    ASSERT(NULL == _pdtobj);
    ASSERT(0 == _dwEffectLastReturned);
    ASSERT(0 == _dwData);
    ASSERT(0 == _dwEffectPreferred);
}

CFSDropTarget::~CFSDropTarget()
{
    if (_pFolder)
        CFSFolder_Release((IShellFolder2 *)&(_pFolder->sf));

    // if we hit this a lot maybe we should just release it
    AssertMsg(_pdtobj == NULL, TEXT("didn't get matching DragLeave. this=%#08lx"), this);
}

HRESULT CFSDropTarget::_Init(CFSFolder* pFSFolder, HWND hwnd)
{
    _pFolder = pFSFolder;
    CFSFolder_AddRef((IShellFolder2 *)&(_pFolder->sf));
    _hwndOwner = hwnd;
    return S_OK;
}

HRESULT CFSDropTarget::v_InternalQueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFSDropTarget, IDropTarget),
        QITABENT(CFSDropTarget, IObjectWithSite),
        QITABENTMULTI2(CFSDropTarget, IID_IDropTargetWithDADSupport, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP CFSDropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    ASSERT(NULL == _pdtobj);    // req DragDrop protocol, someone forgot to call DragLeave

    // init our registerd data formats
    IDLData_InitializeClipboardFormats();

    _grfKeyStateLast = grfKeyState;
    _dwData = 0;
    IUnknown_Set((IUnknown **)&_pdtobj, pDataObj);

    if (pDataObj)
    {
        IEnumFORMATETC *penum;
        HRESULT hres = pDataObj->EnumFormatEtc(DATADIR_GET, &penum);
        if (SUCCEEDED(hres))
        {
            FORMATETC fmte;
            ULONG celt;
            while (penum->Next(1, &fmte, &celt) == S_OK)
            {
                if (fmte.cfFormat == CF_HDROP && (fmte.tymed & TYMED_HGLOBAL))
                    _dwData |= DTID_HDROP;

                if (fmte.cfFormat == g_cfHIDA && (fmte.tymed & TYMED_HGLOBAL))
                    _dwData |= DTID_HIDA;

                if (fmte.cfFormat == g_cfEmbeddedObject && (fmte.tymed & TYMED_ISTORAGE))
                    _dwData |= DTID_EMBEDDEDOBJECT;

                if (fmte.cfFormat == g_cfFileContents && (fmte.tymed & (TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE)))
                    _dwData |= DTID_CONTENTS;
                
                if (fmte.cfFormat == g_cfFileGroupDescriptorA && (fmte.tymed & TYMED_HGLOBAL))
                    _dwData |= DTID_FDESCA;

                if (fmte.cfFormat == g_cfFileGroupDescriptorW && (fmte.tymed & TYMED_HGLOBAL))
                    _dwData |= DTID_FDESCW;

                if ((fmte.cfFormat == g_cfPreferredDropEffect) &&
                    (fmte.tymed & TYMED_HGLOBAL) &&
                    (DROPEFFECT_NONE != (_dwEffectPreferred = DataObj_GetDWORD(pDataObj, g_cfPreferredDropEffect, DROPEFFECT_NONE))))
                {
                    _dwData |= DTID_PREFERREDEFFECT;
                }
#ifdef DEBUG
                TCHAR szFormat[MAX_PATH];
                if (GetClipboardFormatName(fmte.cfFormat, szFormat, ARRAYSIZE(szFormat)))
                {
                    TraceMsg(TF_DRAGDROP, "CFSDropTarget - cf %s, tymed %d", szFormat, fmte.tymed);
                }
                else
                {
                    TraceMsg(TF_DRAGDROP, "CFSDropTarget - cf %d, tymed %d", fmte.cfFormat, fmte.tymed);
                }
#endif // DEBUG
            }
            penum->Release();
        }

        //
        // HACK:
        // Win95 always did the GetData below which can be quite expensive if
        // the data is a directory structure on an ftp server etc.
        // dont check for FD_LINKUI if the data object has a preferred effect
        //
        if ((_dwData & (DTID_PREFERREDEFFECT | DTID_CONTENTS)) == DTID_CONTENTS)
        {
            if (_dwData & DTID_FDESCA)
            {
                FORMATETC fmteRead = {g_cfFileGroupDescriptorA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM medium;
                if (pDataObj->GetData(&fmteRead, &medium) == S_OK)
                {
                    FILEGROUPDESCRIPTORA * pfgd = (FILEGROUPDESCRIPTORA *)GlobalLock(medium.hGlobal);
                    if (pfgd)
                    {
                        if (pfgd->cItems >= 1)
                        {
                            if (pfgd->fgd[0].dwFlags & FD_LINKUI)
                                _dwData |= DTID_FD_LINKUI;
                        }
                        GlobalUnlock(medium.hGlobal);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
            else if (_dwData & DTID_FDESCW)
            {
                FORMATETC fmteRead = {g_cfFileGroupDescriptorW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM medium;
                if (pDataObj->GetData(&fmteRead, &medium)==S_OK)
                {
                    FILEGROUPDESCRIPTORW * pfgd = (FILEGROUPDESCRIPTORW *)GlobalLock(medium.hGlobal);
                    if (pfgd)
                    {
                        if (pfgd->cItems >= 1)
                        {
                            if (pfgd->fgd[0].dwFlags & FD_LINKUI)
                                _dwData |= DTID_FD_LINKUI;
                        }
                        GlobalUnlock(medium.hGlobal);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
        }

        if (OleQueryCreateFromData(pDataObj) == S_OK)
            _dwData |= DTID_OLEOBJ;

        if (OleQueryLinkFromData(pDataObj) == S_OK)
            _dwData |= DTID_OLELINK;

        DebugMsg(DM_TRACE, TEXT("sh TR - CIDL::DragEnter this->dwData = %x"), _dwData);
    }

    // stash this away
    if (pdwEffect)
        _dwEffectLastReturned = *pdwEffect;

    DWORD dwDefault = _DetermineEffects(grfKeyState, pdwEffect, NULL);

    // The cursor always indicates the default action.
    ASSERT(pdwEffect);
    *pdwEffect = dwDefault;

    _dwEffectLastReturned = *pdwEffect;
    TraceMsg(TF_DRAGDROP, "CFSDropTarget::DragEnter() _grfKeyStateLast=%#08lx, dwDefault=%#08lx", _grfKeyStateLast, dwDefault);

    return S_OK;
}

STDMETHODIMP CFSDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    ASSERT(pdwEffect);
    if (_grfKeyStateLast != grfKeyState)
    {
        _grfKeyStateLast = grfKeyState;
        DWORD dwDefault = _DetermineEffects(grfKeyState, pdwEffect, NULL);

        // The cursor always indicates the default action.
        *pdwEffect = dwDefault;
        _dwEffectLastReturned = *pdwEffect;
    }
    else
    {
        *pdwEffect = _dwEffectLastReturned;
    }
    
    TraceMsg(TF_DRAGDROP, "CFSDropTarget::DragOver() this=%#08lx, _pdtobj=%#08lx, _grfKeyStateLast=%#08lx, *pdwEffect=%#08lx", this, _pdtobj, _grfKeyStateLast, *pdwEffect);
    return S_OK;
}

STDMETHODIMP CFSDropTarget::DragLeave()
{
    TraceMsg(TF_DRAGDROP, "CFSDropTarget::DragLeave()  this=%#08lx, _grfKeyStateLast=%#08lx, _pdtobj=%#08lx", this, _grfKeyStateLast, _pdtobj);
    ATOMICRELEASE(_pdtobj);
    return S_OK;
}


// init data from our site that we will need in processing the drop

void CFSDropTarget::_GetStateFromSite()
{
    IShellFolderView *psfv;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_DefView, IID_PPV_ARG(IShellFolderView, &psfv))))
    {
        _fSameHwnd = S_OK == psfv->IsDropOnSource((IDropTarget*)this);
        _fDragDrop = S_OK == psfv->GetDropPoint(&_ptDrop);
        _fBkDropTarget = S_OK == psfv->IsBkDropTarget(NULL);
        psfv->Release();
    }
}

STDMETHODIMP CFSDropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    HRESULT hr;
    BOOL fLinkOnly;

    // OLE may give us a different data object (fully marshalled)
    // from the one we've got on DragEnter (does not seem to happen on Win2k?)

    IUnknown_Set((IUnknown **)&_pdtobj, pDataObj);

    _GetStateFromSite();

    // note, that on the drop the mouse buttons are not down so the grfKeyState
    // is not what we saw on the DragOver/DragEnter, thus we need to cache
    // the grfKeyState to detect left vs right drag
    //
    // ASSERT(this->grfKeyStateLast == grfKeyState);

    HMENU hmenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_TEMPLATEDD);
    DWORD dwDefEffect = _DetermineEffects(grfKeyState, pdwEffect, hmenu);

    if (dwDefEffect == DROPEFFECT_NONE)
    {
        *pdwEffect = DROPEFFECT_NONE;
        DAD_SetDragImage(NULL, NULL);
        hr = S_OK;
        goto DragLeaveAndReturn;
    }

    // Get the hkeyProgID and hkeyBaseProgID
    HKEY hkeyBaseProgID, hkeyProgID;
    SHGetClassKey(_pFolder->_pidl, &hkeyProgID, &hkeyBaseProgID);

    TCHAR szPath[MAX_PATH];
    _GetPath(szPath);

    //
    // Set fLinkOnly if the only option is link and the source hasn't
    //  explicitly told us that it wants only a link created.
    //
    fLinkOnly = ((*pdwEffect == DROPEFFECT_LINK) &&
                 (!(_dwData & DTID_PREFERREDEFFECT) ||
                 (_dwEffectPreferred != DROPEFFECT_LINK)));

    //
    // this doesn't actually do the menu if (grfKeyState MK_LBUTTON)
    //
    FSDRAGDROPMENUPARAM ddm;
    ddm.dwDefEffect = dwDefEffect;
    ddm.pdtobj = pDataObj;
    ddm.pt = pt;
    ddm.pdwEffect = pdwEffect;
    ddm.hkeyProgID = hkeyProgID;
    ddm.hkeyBase = hkeyBaseProgID;
    ddm.hmenu = hmenu;
    ddm.grfKeyState = grfKeyState;

    hr = _DragDropMenu(&ddm);

    SHCloseClassKey(hkeyProgID);
    SHCloseClassKey(hkeyBaseProgID);

    DestroyMenu(hmenu);

    if (hr == S_FALSE)
    {
        // let callers know where this is about to go
        // SHScrap cares because it needs to close the file so we can copy/move it
        DataObj_SetDropTarget(pDataObj, &CLSID_ShellFSFolder);

        switch (ddm.idCmd)
        {
        case DDIDM_CONTENTS_DESKCOMP:
            hr = CreateDesktopComponents(NULL, pDataObj, _hwndOwner, 0, ddm.pt.x, ddm.pt.y);
            break;

        case DDIDM_CONTENTS_DESKURL:
            hr = _CreateURLDeskComp(ddm.pt.x, ddm.pt.y);
            break;

        case DDIDM_CONTENTS_DESKIMG:
            hr = _CreateDeskCompImage(ddm.pt);
            break;

        case DDIDM_CONTENTS_COPY:
        case DDIDM_CONTENTS_MOVE:
        case DDIDM_CONTENTS_LINK:
            hr = FS_AsyncCreateFileFromClip(_hwndOwner, szPath, pDataObj, pt, pdwEffect, _fBkDropTarget);
            break;

        case DDIDM_SCRAP_COPY:
        case DDIDM_SCRAP_MOVE:
        case DDIDM_DOCLINK:
            hr = FS_CreateBookMark(_hwndOwner, szPath, pDataObj, pt, pdwEffect);
            break;

        case DDIDM_OBJECT_COPY:
        case DDIDM_OBJECT_MOVE:
        {
            hr = _CreatePackage();
            if (E_UNEXPECTED == hr)
            {
                // _CreatePackage() can only expand certain types of packages
                // back into files.  For example, it doesn't handle CMDLINK files.
                //
                // If _CreatePackage() didn't recognize the stream format, we fall
                // back to FS_CreateBookMark(), which should create a scrap:
                hr = FS_CreateBookMark(_hwndOwner, szPath, pDataObj, pt, pdwEffect);
            }
            break;
        }

        case DDIDM_COPY:
        case DDIDM_SYNCCOPY:
        case DDIDM_SYNCCOPYTYPE:
        case DDIDM_MOVE:
        case DDIDM_LINK:
        default:
            hr = _ZoneCheckDataObject(*pdwEffect);
                
            if (S_OK == hr)
            {
                FSTHREADPARAM* pfsthp = (FSTHREADPARAM *)LocalAlloc(LPTR, SIZEOF(FSTHREADPARAM));
                if (pfsthp)
                {
                    CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObj, &pfsthp->pstmDataObj);

                    ASSERT(pfsthp->pDataObj == NULL);

                    pfsthp->dwEffect = *pdwEffect;
                    pfsthp->fLinkOnly = fLinkOnly;

                    pfsthp->fSameHwnd = _fSameHwnd;
                    pfsthp->fDragDrop = _fDragDrop;
                    pfsthp->ptDrop = _ptDrop;
                    pfsthp->fBkDropTarget = _fBkDropTarget;

                    pfsthp->bSyncCopy = (DDIDM_SYNCCOPY == ddm.idCmd) || (DDIDM_SYNCCOPYTYPE == ddm.idCmd);
                    pfsthp->idCmd = ddm.idCmd;
                    pfsthp->pidl = ILClone(_pFolder->_pidl);
                    pfsthp->grfKeyState = _grfKeyStateLast;
                    pfsthp->hwndOwner = _hwndOwner;
                    _GetPath(pfsthp->szPath);

                    if (DataObj_CanGoAsync(pDataObj) || DataObj_GoAsyncForCompat(pDataObj))
                    {
                        // create another thread to avoid blocking the source thread.
                        if (SHCreateThread(FileDropTargetThreadProc, pfsthp, CTF_COINIT, NULL))
                        {
                            hr = S_OK;
                        }
                        else
                        {
                            FreeFSThreadParam(pfsthp);  // cleanup
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        FileDropTargetThreadProc(pfsthp);   // synchronously
                    }
                }
                else
                    hr = E_OUTOFMEMORY;
            }

            // in these CF_HDROP cases "Move" is always an optimized move, we delete the
            // source. make sure we don't return DROPEFFECT_MOVE so the source does not 
            // try to do this too... 
            // even if we have not done anything yet since we may have 
            // kicked of a thread to do this
            
            DataObj_SetDWORD(pDataObj, g_cfLogicalPerformedDropEffect, *pdwEffect);            
            if (DROPEFFECT_MOVE == *pdwEffect)
                *pdwEffect = DROPEFFECT_NONE;
            break;
        }
    }

DragLeaveAndReturn:

    DragLeave();

    TraceMsg(TF_DRAGDROP, "CFSDropTarget::Drop(pDataObj=%#08lx) this=%#08lx, _grfKeyStateLast=%#08lx, *pdwEffect=%#08lx, hr=%#08lx", _pdtobj, this, _grfKeyStateLast, *pdwEffect, hr);

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;

    ASSERT(*pdwEffect==DROPEFFECT_COPY || 
           *pdwEffect==DROPEFFECT_LINK || 
           *pdwEffect==DROPEFFECT_MOVE || 
           *pdwEffect==DROPEFFECT_NONE);
    return hr;
}

VOID CFSDropTarget::_AddVerbs(DWORD* pdwEffects,
                              DWORD dwEffectAvail,
                              DWORD dwDefEffect,
                              UINT idCopy,
                              UINT idMove,
                              UINT idLink,
                              DWORD dwForceEffect,
                              FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    MENUITEMINFO mii;
    TCHAR szCmd[MAX_PATH];
    if (NULL != pfsMenuInfo)
    {
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = szCmd;
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.fType = MFT_STRING;
    }
    if ((DROPEFFECT_COPY == (DROPEFFECT_COPY & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_COPY)) || (dwForceEffect & DROPEFFECT_COPY)))
    {
        ASSERT(0 != idCopy);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idCopy + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_COPY == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idCopy;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uCopyPos, TRUE, &mii);
            pfsMenuInfo->uCopyPos++;
            pfsMenuInfo->uMovePos++;
            pfsMenuInfo->uLinkPos++;
        }
    }
    if ((DROPEFFECT_MOVE == (DROPEFFECT_MOVE & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_MOVE)) || (dwForceEffect & DROPEFFECT_MOVE)))
    {
        ASSERT(0 != idMove);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idMove + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_MOVE == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idMove;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uMovePos, TRUE, &mii);
            pfsMenuInfo->uMovePos++;
            pfsMenuInfo->uLinkPos++;
        }
    }
    if ((DROPEFFECT_LINK == (DROPEFFECT_LINK & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_LINK)) || (dwForceEffect & DROPEFFECT_LINK)))
    {
        ASSERT(0 != idLink);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idLink + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_LINK == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idLink;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uLinkPos, TRUE, &mii);
            pfsMenuInfo->uLinkPos++;
        }
    }
    *pdwEffects |= dwEffectAvail;
}

DWORD CFSDropTarget::_GetStdDefEffect(DWORD grfKeyState, DWORD dwCurEffectAvail, DWORD dwAllEffectAvail, DWORD dwOrigDefEffect)
{
    DWORD dwDefEffect = 0;
    //
    // Alter the default effect depending on modifier keys.
    //
    switch (grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
    case MK_CONTROL:            dwDefEffect = DROPEFFECT_COPY; break;
    case MK_SHIFT:              dwDefEffect = DROPEFFECT_MOVE; break;
    case MK_SHIFT | MK_CONTROL: dwDefEffect = DROPEFFECT_LINK; break;
    case MK_ALT:                dwDefEffect = DROPEFFECT_LINK; break;
    default:
        //
        // no modifier keys:
        // if the data object contains a preferred drop effect, try to use it
        //
        DWORD dwPreferred = DataObj_GetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_NONE) & dwAllEffectAvail;

        if (dwPreferred)
        {
            if (dwPreferred & DROPEFFECT_MOVE)
            {
                dwDefEffect = DROPEFFECT_MOVE;
            }
            else if (dwPreferred & DROPEFFECT_COPY)
            {
                dwDefEffect = DROPEFFECT_COPY;
            }
            else if (dwPreferred & DROPEFFECT_LINK)
            {
                dwDefEffect = DROPEFFECT_LINK;
            }
        }
        else
        {
            dwDefEffect = dwOrigDefEffect;
        }
        break;
    }
    return dwDefEffect & dwCurEffectAvail;
}

HRESULT CFSDropTarget::_GetDDInfoDeskCompHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                               DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(_dwData & DTID_HDROP);
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (!SHRestricted(REST_NOACTIVEDESKTOP) &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        hr = IsDeskCompHDrop(_pdtobj);
        if (S_OK == hr)
        {
            DWORD dwDefEffect = 0;
            DWORD dwEffectAdd = dwEffectsAvail & DROPEFFECT_LINK;
            if (NULL != pdwDefaultEffect)
            {
                dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
                *pdwDefaultEffect = dwDefEffect;
            }
            
            _AddVerbs(pdwEffects,
                      dwEffectAdd,
                      dwDefEffect,
                      0,
                      0,
                      DDIDM_CONTENTS_DESKCOMP,
                      DROPEFFECT_LINK, // force add the DDIDM_CONTENTS_DESKCOMP verb
                      pfsMenuInfo);
        }
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                           DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(_dwData & DTID_HDROP);
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    // Is this the sneakernet case?
    ASSERT(_pdtobj);
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
    if (pida)
    {
        LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
        if (pidlParent)
        {
            TCHAR szTargetFolder[MAX_PATH];

            _GetPath(szTargetFolder);

            if (IsFromSneakernetBriefcase(pidlParent, szTargetFolder))
            {
                // Yes; show the non-default briefcase cm
                FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM mediumT;

                if (SUCCEEDED(_pdtobj->GetData(&fmte, &mediumT)))
                {
                    BOOL fSyncCopyType = DroppingAnyFolders((HDROP) mediumT.hGlobal);

                    DWORD dwDefEffect = 0;
                    DWORD dwEffectAdd = DROPEFFECT_COPY & dwEffectsAvail;
                    if (NULL != pdwDefaultEffect)
                    {
                        dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
                        *pdwDefaultEffect = dwDefEffect;
                    }
                    
                    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_SYNCCOPY, 0, 0, 0, pfsMenuInfo);

                    // Call _AddVerbs() again to force "Sync Copy of Type" as a 2nd DROPEFFECT_COPY verb:
                    if (fSyncCopyType && (DROPEFFECT_COPY & dwEffectsAvail))
                    {
                        _AddVerbs(pdwEffects,
                                  DROPEFFECT_COPY,
                                  0,
                                  DDIDM_SYNCCOPYTYPE,
                                  0,
                                  0,
                                  DROPEFFECT_COPY,
                                  pfsMenuInfo);
                    }
                    
                    ReleaseStgMedium(&mediumT);
                }
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                       DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(_dwData & DTID_HDROP);
    ASSERT(pdwEffects);

    DWORD dwDefEffect = 0;
    DWORD dwEffectAdd = dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE);
    if (NULL != pdwDefaultEffect)
    {
        dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, _PickDefFSOperation(dwEffectAdd));
        *pdwDefaultEffect = dwDefEffect;
    }
    
    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_COPY, DDIDM_MOVE, 0, 0, pfsMenuInfo);

    return S_OK;
}

HRESULT CFSDropTarget::_GetDDInfoFileContents(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                              DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    
    if ((_dwData & (DTID_CONTENTS | DTID_FDESCA)) == (DTID_CONTENTS | DTID_FDESCA) ||
        (_dwData & (DTID_CONTENTS | DTID_FDESCW)) == (DTID_CONTENTS | DTID_FDESCW))
    {
        DWORD dwEffectAdd, dwSuggestedEffect;
        //
        // HACK: if there is a preferred drop effect and no HIDA
        // then just take the preferred effect as the available effects
        // this is because we didn't actually check the FD_LINKUI bit
        // back when we assembled dwData! (performance)
        //
        if ((_dwData & (DTID_PREFERREDEFFECT | DTID_HIDA)) == DTID_PREFERREDEFFECT)
        {
            dwEffectAdd = _dwEffectPreferred;
            dwSuggestedEffect = _dwEffectPreferred;
        }
        else if (_dwData & DTID_FD_LINKUI)
        {
            dwEffectAdd = DROPEFFECT_LINK;
            dwSuggestedEffect = DROPEFFECT_LINK;
        }
        else
        {
            dwEffectAdd = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            dwSuggestedEffect = DROPEFFECT_COPY;
        }
        dwEffectAdd &= dwEffectsAvail;

        DWORD dwDefEffect = 0;
        if (NULL != pdwDefaultEffect)
        {
            dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, dwSuggestedEffect);
            *pdwDefaultEffect = dwDefEffect;
        }

        _AddVerbs(pdwEffects,
                  dwEffectAdd,
                  dwDefEffect,
                  DDIDM_CONTENTS_COPY,
                  DDIDM_CONTENTS_MOVE,
                  DDIDM_CONTENTS_LINK,
                  0,
                  pfsMenuInfo);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT CFSDropTarget::_GetDDInfoHIDA(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                      DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(_dwData & DTID_HIDA);
    ASSERT(pdwEffects);

    DWORD dwDefEffect = 0;
    DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
    // NOTE: we only add a HIDA default effect if HDROP isn't going to add a default
    // effect.  This preserves shell behavior with file system data objects without
    // requiring us to change the enumerator order in CIDLData.  When we do change
    // the enumerator order, we can remove this special case:
    if ((NULL != pdwDefaultEffect) &&
        ((0 == (_dwData & DTID_HDROP)) ||
         (0 == _GetStdDefEffect(grfKeyFlags,
                                dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE),
                                dwEffectsAvail,
                                _PickDefFSOperation(dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE))))))
    {
        dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
        *pdwDefaultEffect = dwDefEffect;
    }
    
    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, 0, 0, DDIDM_LINK, 0, pfsMenuInfo);

    return S_OK;
}

HRESULT CFSDropTarget::_GetDDInfoOlePackage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                            DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(_dwData & DTID_EMBEDDEDOBJECT);
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (NULL != pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    FORMATETC fmte = {g_cfObjectDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    ASSERT(NULL != _pdtobj);
    if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
    {
        // we've got an object descriptor
        OBJECTDESCRIPTOR* pOD = (OBJECTDESCRIPTOR*) GlobalLock(medium.hGlobal);
        if (pOD)
        {
            if (IsEqualCLSID(CLSID_OldPackage, pOD->clsid) ||
                IsEqualCLSID(CLSID_CPackage, pOD->clsid))
            {
                // This is a package - proceed
                DWORD dwDefEffect = 0;
                DWORD dwEffectAdd = (DROPEFFECT_COPY | DROPEFFECT_MOVE) & dwEffectsAvail;
                if (NULL != pdwDefaultEffect)
                {
                    dwDefEffect = _GetStdDefEffect(grfKeyFlags,
                                                  dwEffectAdd,
                                                  dwEffectsAvail,
                                                  DROPEFFECT_COPY);
                    *pdwDefaultEffect = dwDefEffect;
                }
                
                _AddVerbs(pdwEffects,
                          dwEffectAdd,
                          dwDefEffect,
                          DDIDM_OBJECT_COPY,
                          DDIDM_OBJECT_MOVE,
                          0,
                          0,
                          pfsMenuInfo);

                hr = S_OK;
            }
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoDeskImage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                           DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (NULL != pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    if (!SHRestricted(REST_NOACTIVEDESKTOP) &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        FORMATETC fmte = {g_cfHTML, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            //DANGER WILL ROBINSON:
            //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
            // it as is it were ANSI. Find a way to escape the sequences...
            CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
            if (pszData)
            {
                CHAR szUrl[MAX_URL_STRING];
                if (ExtractImageURLFromCFHTML(pszData, GlobalSize(medium.hGlobal), szUrl, ARRAYSIZE(szUrl)))
                {
                    // The HTML contains an image tag - carry on...
                    DWORD dwDefEffect = 0;
                    DWORD dwEffectAdd = DROPEFFECT_LINK; // NOTE: ignoring dwEffectsAvail!
                    if (NULL != pdwDefaultEffect)
                    {
                        dwDefEffect = _GetStdDefEffect(grfKeyFlags,
                                                       dwEffectAdd,
                                                       dwEffectsAvail | DROPEFFECT_LINK,
                                                       DROPEFFECT_LINK);
                        *pdwDefaultEffect = dwDefEffect;
                    }
                    
                    _AddVerbs(pdwEffects,
                              dwEffectAdd,
                              dwDefEffect,
                              0,
                              0,
                              DDIDM_CONTENTS_DESKIMG,
                              0,
                              pfsMenuInfo);

                    hr = S_OK;
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoDeskComp(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                          DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (NULL != pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    if (!SHRestricted(REST_NOACTIVEDESKTOP) &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            // DANGER WILL ROBINSON:
            // HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
            // it as is it were ANSI. Find a way to escape the sequences...
            CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
            if (pszData)
            {
                int nScheme = GetUrlSchemeA(pszData);
                if ((nScheme != URL_SCHEME_INVALID) && (nScheme != URL_SCHEME_FTP))
                {
                    // This is an internet scheme - carry on...
                    DWORD dwDefEffect = 0;
                    DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
                    if (NULL != pdwDefaultEffect)
                    {
                        dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
                        *pdwDefaultEffect = dwDefEffect;
                    }
                    
                    _AddVerbs(pdwEffects,
                              dwEffectAdd,
                              dwDefEffect,
                              0,
                              0,
                              DDIDM_CONTENTS_DESKURL,
                              DROPEFFECT_LINK, // force add this verb
                              pfsMenuInfo);

                    hr = S_OK;
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoOleObj(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                        DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (_dwData & DTID_OLEOBJ)
    {
        DWORD dwDefEffect = 0;
        DWORD dwEffectAdd = (DROPEFFECT_COPY | DROPEFFECT_MOVE) & dwEffectsAvail;
        if (NULL != pdwDefaultEffect)
        {
            dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
            *pdwDefaultEffect = dwDefEffect;
        }
    
        _AddVerbs(pdwEffects,
                  dwEffectAdd,
                  dwDefEffect,
                  DDIDM_SCRAP_COPY,
                  DDIDM_SCRAP_MOVE,
                  0,
                  0,
                  pfsMenuInfo);

        hr = S_OK;
    }
    return hr;
}

HRESULT CFSDropTarget::_GetDDInfoOleLink(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                         DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (_dwData & DTID_OLELINK)
    {
        DWORD dwDefEffect = 0;
        DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
        if (NULL != pdwDefaultEffect)
        {
            dwDefEffect = _GetStdDefEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
            *pdwDefaultEffect = dwDefEffect;
        }
    
        _AddVerbs(pdwEffects,
                  dwEffectAdd,
                  dwDefEffect,
                  0,
                  0,
                  DDIDM_DOCLINK,
                  0,
                  pfsMenuInfo);

        hr = S_OK;
    }
    return hr;
}

HRESULT CFSDropTarget::_CreateURLDeskComp(int x, int y)
{
    // This code should only be entered if DDIDM_CONTENTS_DESKURL was added to the menu,
    // and it has these checks:
    ASSERT(!SHRestricted(REST_NOACTIVEDESKTOP) &&
           !SHRestricted(REST_NOADDDESKCOMP) &&
           _IsDesktopFolder());
           
    STGMEDIUM medium;
    FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = _pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        //DANGER WILL ROBINSON:
        //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
        // it as is it were ANSI. Find a way to escape the sequences...
        CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
        if (pszData)
        {
            int nScheme = GetUrlSchemeA(pszData);
            if ((nScheme != URL_SCHEME_INVALID) && (nScheme != URL_SCHEME_FTP))
            {
                // This is an internet scheme - URL

                hr = CreateDesktopComponents(pszData, NULL, _hwndOwner, DESKCOMP_URL, x, y);
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_FAIL;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}

HRESULT CFSDropTarget::_CreateDeskCompImage(POINTL pt)
{
    ASSERT(!SHRestricted(REST_NOACTIVEDESKTOP) &&
           !SHRestricted(REST_NOADDDESKCOMP) &&
           _IsDesktopFolder());
           
    FORMATETC fmte = {g_cfHTML, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    HRESULT hres = _pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hres))
    {
        //DANGER WILL ROBINSON:
        //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
        // it as is it were ANSI. Find a way to escape the sequences...
        CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
        if (pszData)
        {
            CHAR szUrl[MAX_URL_STRING];
            if (ExtractImageURLFromCFHTML(pszData, GlobalSize(medium.hGlobal), szUrl, ARRAYSIZE(szUrl)))
            {
                // The HTML contains an image tag - carry on...
                ADDTODESKTOP *pToAD = (ADDTODESKTOP*)LocalAlloc(LPTR, sizeof(*pToAD));
                if (pToAD)
                {
                    pToAD->hwnd = _hwndOwner;
                    lstrcpyA(pToAD->szUrl, szUrl);
                    pToAD->dwFlags = DESKCOMP_IMAGE;
                    pToAD->pt = pt;

                    if (SHCreateThread(AddToActiveDesktopThreadProc, pToAD, CTF_COINIT, NULL))
                    {
                        hres = NOERROR;
                    }
                    else
                    {
                        LocalFree(pToAD);
                        hres = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                hres = E_FAIL;
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hres = E_FAIL;
        }
        ReleaseStgMedium(&medium);
    }
    return hres;
}


//
// read byte by byte until we hit the null terminating char
// return: the number of bytes read
//
STDAPI StringReadFromStream(IStream* pstm, LPSTR pszBuf, UINT cchBuf)
{
    UINT cch = 0;
    
    do {
        pstm->Read(pszBuf, sizeof(CHAR), NULL);
        cch++;
    } while (*pszBuf++ && cch <= cchBuf);  
    return cch;
} 

STDAPI CopyStreamToFile(IStream* pstmSrc, LPCTSTR pszFile) 
{
    IStream *pstmFile;
    HRESULT hr = SHCreateStreamOnFile(pszFile, OF_CREATE | OF_WRITE | OF_SHARE_DENY_WRITE, &pstmFile);
    if (SUCCEEDED(hr))
    {
        hr = CopyStreamUI(pstmSrc, pstmFile, NULL);
        pstmFile->Release();
    }
    return hr;
}   

HRESULT CFSDropTarget::_CreatePackage()
{
    ILockBytes* pLockBytes;
    HRESULT hr = CreateILockBytesOnHGlobal(NULL, TRUE, &pLockBytes);
    if (SUCCEEDED(hr))
    {
        STGMEDIUM medium;
        medium.tymed = TYMED_ISTORAGE;
        hr = StgCreateDocfileOnILockBytes(pLockBytes,
                                        STGM_DIRECT | STGM_READWRITE | STGM_CREATE |
                                        STGM_SHARE_EXCLUSIVE, 0, &medium.pstg);
        if (SUCCEEDED(hr))
        {
            FORMATETC fmte = {g_cfEmbeddedObject, NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE};
            ASSERT(NULL != _pdtobj);
            hr = _pdtobj->GetDataHere(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                IStream* pstm;
#ifdef DEBUG
                STATSTG stat;
                if (SUCCEEDED(medium.pstg->Stat(&stat, STATFLAG_NONAME)))
                {
                    ASSERT(IsEqualCLSID(CLSID_OldPackage, stat.clsid) ||
                           IsEqualCLSID(CLSID_CPackage, stat.clsid));
                }
#endif // DEBUG                        
                #define PACKAGER_ICON           2
                #define PACKAGER_CONTENTS       L"\001Ole10Native"
                #define PACKAGER_EMBED_TYPE     3
                hr = medium.pstg->OpenStream(PACKAGER_CONTENTS, 0,
                                               STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                               0, &pstm);
                if (SUCCEEDED(hr))
                {
                    DWORD dw;
                    WORD w;
                    CHAR szName[MAX_PATH];
                    CHAR szTemp[MAX_PATH];
                    if (SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)) && // pkg size
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // pkg appearance
                        (PACKAGER_ICON == w) &&
                        SUCCEEDED(StringReadFromStream(pstm, szName, ARRAYSIZE(szName))) &&
                        SUCCEEDED(StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp))) && // icon path
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // icon index
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // panetype
                        (PACKAGER_EMBED_TYPE == w) &&
                        SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)) && // filename size
                        SUCCEEDED(pstm->Read(szTemp, dw, NULL)) &&      // filename
                        SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)))   // get file size
                    {
                        // The rest of the stream is the file contents
                        TCHAR szPath[MAX_PATH], szBase[MAX_PATH], szDest[MAX_PATH];
                        _GetPath(szPath);

                        SHAnsiToTChar(szName, szBase, ARRAYSIZE(szBase));
                        PathAppend(szPath, szBase);
                        PathYetAnotherMakeUniqueName(szDest, szPath, NULL, szBase);
                        TraceMsg(TF_GENERAL, "CFSIDLDropTarget pkg: %s", szDest);

                        hr = CopyStreamToFile(pstm, szDest);

                        if (SUCCEEDED(hr))
                        {
                            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szDest, NULL);
                            if (_fBkDropTarget && _hwndOwner)
                            {
                                FS_PositionFileFromDrop(_hwndOwner, szDest, NULL);
                            }
                        }
                    }
                    else
                    {
                        hr = E_UNEXPECTED;
                    }
                    pstm->Release();
                }
            }
            medium.pstg->Release();
        }
        pLockBytes->Release();
    }
    return hr;
}

HRESULT CFSDropTarget::_GetPath(LPTSTR pszPath)
{
    return CFSFolder_GetPath(_pFolder, pszPath);
}

STDAPI_(BOOL) AllRegisteredPrograms(HDROP hDrop);

//
// Returns:
//  If the data object does NOT contain HDROP -> "none"
//  else if the source is root or registered progam -> "link"
//   else if this is within a volume   -> "move"
//   else if this is a briefcase       -> "move"
//   else                              -> "copy"
//
DWORD CFSDropTarget::_PickDefFSOperation(DWORD dwCurEffectAvail)
{
    ASSERT(_pdtobj);
    DWORD dwDefEffect = 0;      // assume no HDROP

    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
    {
        TCHAR szPath[MAX_PATH], szFolder[MAX_PATH];

        _GetPath(szFolder);
        DragQueryFile((HDROP) medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)); // focused item

        // Determine the default operation depending on the item.
        if (PathIsRoot(szPath) || AllRegisteredPrograms((HDROP) medium.hGlobal))
        {
            dwDefEffect = DROPEFFECT_LINK;
        }
        else if (PathIsSameRoot(szPath, szFolder))
        {
            dwDefEffect = DROPEFFECT_MOVE;
        }
        else if (IsBriefcaseRoot(_pdtobj))
        {
            // briefcase default to move even accross volumes
            dwDefEffect = DROPEFFECT_MOVE;
        }
        else
        {
            dwDefEffect = DROPEFFECT_COPY;
        }
        ReleaseStgMedium(&medium);
    }
    else
    {
        // GetData failed. Let's see if QueryGetData failed or not.
        if (SUCCEEDED(_pdtobj->QueryGetData(&fmte)))
        {
            // this means this data object has HDROP but can't
            // provide it until it is dropped. Let's assume we are copying.
            dwDefEffect = DROPEFFECT_COPY;
        }
    }
    // Switch default verb if the dwCurEffectAvail hint suggests that we picked an
    // unavailable effect (this code applies to MOVE and COPY only):
    dwCurEffectAvail &= (DROPEFFECT_MOVE | DROPEFFECT_COPY);
    if ((DROPEFFECT_MOVE == dwDefEffect) && (DROPEFFECT_COPY == dwCurEffectAvail))
    {
        // If we were going to return MOVE, and only COPY is available, return COPY:
        dwDefEffect = DROPEFFECT_COPY;
    }
    else if ((DROPEFFECT_COPY == dwDefEffect) && (DROPEFFECT_MOVE == dwCurEffectAvail))
    {
        // If we were going to return COPY, and only MOVE is available, return MOVE:
        dwDefEffect = DROPEFFECT_MOVE;
    }
    return dwDefEffect;
}

HRESULT CFSDropTarget::_ZoneCheckDataObject(DWORD dwEffect)
{
    ASSERT(NULL != _pdtobj);
    
    CHAR szUrl[INTERNET_MAX_URL_LENGTH];
    szUrl[0] = 0;

    // Grab a URL and use it for the zone check if possible:
    if (!SHRestricted(REST_NOACTIVEDESKTOP) &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        FORMATETC fmte = {g_cfHTML, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            CHAR* pszData = (CHAR*) GlobalLock(medium.hGlobal);
            if (pszData)
            {
                ExtractImageURLFromCFHTML(pszData, GlobalSize(medium.hGlobal), szUrl, sizeof(szUrl));
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
        else
        {
            fmte.cfFormat = g_cfShellURL;
            if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
            {
                CHAR* pszData = (CHAR*) GlobalLock(medium.hGlobal);
                if (pszData)
                {
                    lstrcpynA(szUrl, (LPCSTR)pszData, ARRAYSIZE(szUrl));
                    GlobalUnlock(medium.hGlobal);
                }
                ReleaseStgMedium(&medium);
            }
        }
    }

    if (szUrl[0])
    {
        return ZoneCheckUrlA(szUrl,
                             URLACTION_SHELL_MOVE_OR_COPY,
                             PUAF_FORCEUI_FOREGROUND | PUAF_WARN_IF_DENIED | PUAF_CHECK_TIFS,
                             NULL);
    }
    else
    {
        return ZoneCheckHDrop(_pdtobj,
                              dwEffect,
                              URLACTION_SHELL_MOVE_OR_COPY,
                              PUAF_FORCEUI_FOREGROUND | PUAF_WARN_IF_DENIED | PUAF_CHECK_TIFS,
                              NULL);
    }
}

//
// make sure that the default effect is among the allowed effects
//
DWORD CFSDropTarget::_LimitDefaultEffect(DWORD dwDefEffect, DWORD dwEffectsAllowed)
{
    if (dwDefEffect & dwEffectsAllowed)
        return dwDefEffect;

    if (dwEffectsAllowed & DROPEFFECT_COPY)
        return DROPEFFECT_COPY;

    if (dwEffectsAllowed & DROPEFFECT_MOVE)
        return DROPEFFECT_MOVE;

    if (dwEffectsAllowed & DROPEFFECT_LINK)
        return DROPEFFECT_LINK;

    return DROPEFFECT_NONE;
}

typedef struct {
    HRESULT (CFSDropTarget::*pfnGetDragDropInfo)(
                                  IN FORMATETC* pfmte,
                                  IN DWORD grfKeyFlags,
                                  IN DWORD dwEffectsAvail,
                                  IN OUT DWORD* pdwEffectsUsed,
                                  OUT DWORD* pdwDefaultEffect,
                                  IN OUT FSMENUINFO* pfsMenuInfo);
    FORMATETC fmte;
} FS_DATA_HANDLER;

#define CFFMTE(cf, tymed) {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, tymed}
//
// This function returns the default effect.
// This function also modifies *pdwEffectInOut to indicate "available" operations.
//
DWORD CFSDropTarget::_DetermineEffects(DWORD grfKeyState,
                                       DWORD *pdwEffectInOut,
                                       HMENU hmenu)
{
    DWORD dwDefEffect = DROPEFFECT_NONE;
    DWORD dwEffectsUsed = DROPEFFECT_NONE;

    // NOTE: the order is important (particularly for multiple entries with the same FORMATETC)
    FS_DATA_HANDLER rg_data_handlers[] = {
        _GetDDInfoFileContents,     CFFMTE(g_cfFileGroupDescriptorW, TYMED_HGLOBAL),
        _GetDDInfoFileContents,     CFFMTE(g_cfFileGroupDescriptorA, TYMED_HGLOBAL),
        _GetDDInfoFileContents,     CFFMTE(g_cfFileContents, TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE),
        _GetDDInfoBriefcase,        CFFMTE(CF_HDROP, TYMED_HGLOBAL),
        _GetDDInfoHDROP,            CFFMTE(CF_HDROP, TYMED_HGLOBAL),
        _GetDDInfoDeskCompHDROP,    CFFMTE(CF_HDROP, TYMED_HGLOBAL),
        _GetDDInfoHIDA,             CFFMTE(g_cfHIDA, TYMED_HGLOBAL),
        _GetDDInfoOlePackage,       CFFMTE(g_cfEmbeddedObject, TYMED_ISTORAGE),
        _GetDDInfoDeskImage,        CFFMTE(g_cfHTML, TYMED_HGLOBAL),
        _GetDDInfoDeskComp,         CFFMTE(g_cfShellURL, TYMED_HGLOBAL),
        _GetDDInfoOleObj,           CFFMTE(0, TYMED_HGLOBAL),
        _GetDDInfoOleLink,          CFFMTE(0, TYMED_HGLOBAL),
    };
    // Loop through formats, factoring in both the order of the enumerator and
    // the order of our rg_data_handlers to determine the default effect
    // (and possibly, to create the drop context menu)
    FSMENUINFO fsmi = { hmenu, 0, 0, 0 };
    IEnumFORMATETC *penum;
    AssertMsg((NULL != _pdtobj), TEXT("CFSDropTarget::_DetermineEffects() _pdtobj is NULL but we need it.  this=%#08lx"), this);
    if (_pdtobj && SUCCEEDED(_pdtobj->EnumFormatEtc(DATADIR_GET, &penum)))
    {
        FORMATETC fmte;
        ULONG celt;
        while (penum->Next(1, &fmte, &celt) == S_OK)
        {
            for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                if (rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat &&
                    rg_data_handlers[i].fmte.dwAspect == fmte.dwAspect &&
                    (0 != (rg_data_handlers[i].fmte.tymed & fmte.tymed)))
                {
                    (this->*(rg_data_handlers[i].pfnGetDragDropInfo))(
                                                           &fmte,
                                                           grfKeyState,
                                                           *pdwEffectInOut,
                                                           &dwEffectsUsed,
                                                           (DROPEFFECT_NONE == dwDefEffect) ?
                                                            &dwDefEffect : NULL,
                                                           (NULL != hmenu) ? &fsmi : NULL);
                }
            }
        }
        penum->Release();
    }
    // Loop through the rg_data_handlers that don't have an associated clipboard format last
    for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
    {
        if (0 == rg_data_handlers[i].fmte.cfFormat)
        {
            (this->*(rg_data_handlers[i].pfnGetDragDropInfo))(
                                                   NULL,
                                                   grfKeyState,
                                                   *pdwEffectInOut,
                                                   &dwEffectsUsed,
                                                   (DROPEFFECT_NONE == dwDefEffect) ?
                                                    &dwDefEffect : NULL,
                                                   (NULL != hmenu) ? &fsmi : NULL);
        }
    }

    *pdwEffectInOut &= dwEffectsUsed;

    dwDefEffect = _LimitDefaultEffect(dwDefEffect, *pdwEffectInOut);

    DebugMsg(TF_FSTREE, TEXT("CFSDT::GetDefaultEffect dwDef=%x, dwEffUsed=%x, *pdw=%x"),
             dwDefEffect, dwEffectsUsed, *pdwEffectInOut);

    return dwDefEffect;
}

struct FS_EFFECT {
    UINT uID;
    DWORD dwEffect;
};

// This is used to map command id's back to dropeffect's:

const FS_EFFECT c_IDFSEffects[] = {
    DDIDM_COPY,         DROPEFFECT_COPY,
    DDIDM_MOVE,         DROPEFFECT_MOVE,
    DDIDM_CONTENTS_DESKCOMP,     DROPEFFECT_LINK,
    DDIDM_LINK,         DROPEFFECT_LINK,
    DDIDM_SCRAP_COPY,   DROPEFFECT_COPY,
    DDIDM_SCRAP_MOVE,   DROPEFFECT_MOVE,
    DDIDM_DOCLINK,      DROPEFFECT_LINK,
    DDIDM_CONTENTS_COPY, DROPEFFECT_COPY,
    DDIDM_CONTENTS_MOVE, DROPEFFECT_MOVE,
    DDIDM_CONTENTS_LINK, DROPEFFECT_LINK,
    DDIDM_CONTENTS_DESKIMG,     DROPEFFECT_LINK,
    DDIDM_SYNCCOPYTYPE, DROPEFFECT_COPY,        // (order is important)
    DDIDM_SYNCCOPY,     DROPEFFECT_COPY,
    DDIDM_OBJECT_COPY,  DROPEFFECT_COPY,
    DDIDM_OBJECT_MOVE,  DROPEFFECT_MOVE,
    DDIDM_CONTENTS_DESKURL,  DROPEFFECT_LINK,
};

HRESULT CFSDropTarget::_DragDropMenu(FSDRAGDROPMENUPARAM *pddm)
{
    HRESULT hres = E_OUTOFMEMORY;       // assume error
    DWORD dwEffectOut = 0;                              // assume no-ope.
    if (pddm->hmenu)
    {
        int nItem;
        UINT idCmd;
        UINT idCmdFirst = DDIDM_EXTFIRST;
        HDXA hdxa = HDXA_Create();
        HDCA hdca = DCA_Create();
        if (hdxa && hdca)
        {
            // BUGBUG (toddb): Even though pddm->hkeyBase does not have the same value as
            // pddm->hkeyProgID they can both be the same registry key (HKCR\FOLDER, for example).
            // As a result we sometimes enumerate this key twice looking for the same data.  As
            // this is sometimes a slow operation we should avoid this.  The comparision
            // done below was never valid on NT and might not be valid on win9x.

            //
            // Add extended menu for "Base" class.
            //
            if (pddm->hkeyBase && pddm->hkeyBase != pddm->hkeyProgID)
                DCA_AddItemsFromKey(hdca, pddm->hkeyBase, STRREG_SHEX_DDHANDLER);

            //
            // Enumerate the DD handlers and let them append menu items.
            //
            if (pddm->hkeyProgID)
                DCA_AddItemsFromKey(hdca, pddm->hkeyProgID, STRREG_SHEX_DDHANDLER);

            idCmdFirst = HDXA_AppendMenuItems(hdxa, pddm->pdtobj, 1,
                &pddm->hkeyProgID, _pFolder->_pidl, pddm->hmenu, 0,
                DDIDM_EXTFIRST, DDIDM_EXTLAST, 0, hdca);
        }

        // If this dragging is caused by the left button, simply choose
        // the default one, otherwise, pop up the context menu.  If there
        // is no key state info and the original effect is the same as the
        // current effect, choose the default one, otherwise pop up the
        // context menu.  
        if ((_grfKeyStateLast & MK_LBUTTON) ||
             (!_grfKeyStateLast && (*(pddm->pdwEffect) == pddm->dwDefEffect)))
        {
            idCmd = GetMenuDefaultItem(pddm->hmenu, MF_BYCOMMAND, 0);

            //
            // This one MUST be called here. Please read its comment block.
            //
            DAD_DragLeave();

            if (_hwndOwner)
                SetForegroundWindow(_hwndOwner);
        }
        else
        {
            //
            // Note that SHTrackPopupMenu calls DAD_DragLeave().
            //
            idCmd = SHTrackPopupMenu(pddm->hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pddm->pt.x, pddm->pt.y, 0, _hwndOwner, NULL);


        }

        //
        // We also need to call this here to release the dragged image.
        //
        DAD_SetDragImage(NULL, NULL);

        //
        // Check if the user selected one of add-in menu items.
        //
        if (idCmd == 0)
        {
            hres = S_OK;        // Canceled by the user, return S_OK
        }
        else if (InRange(idCmd, DDIDM_EXTFIRST, DDIDM_EXTLAST))
        {
            //
            // Yes. Let the context menu handler process it.
            //
            CMINVOKECOMMANDINFOEX ici = {
                SIZEOF(CMINVOKECOMMANDINFOEX),
                0L,
                _hwndOwner,
                (LPSTR)MAKEINTRESOURCE(idCmd - DDIDM_EXTFIRST),
                NULL, NULL,
                SW_NORMAL,
            };

            // record if the shift/control keys were down at the time of the drop
            if (_grfKeyStateLast & MK_SHIFT)
            {
                ici.fMask |= CMIC_MASK_SHIFT_DOWN;
            }

            if (_grfKeyStateLast & MK_CONTROL)
            {
                ici.fMask |= CMIC_MASK_CONTROL_DOWN;
            }

            // We may not want to ignore the error code. (Can happen when you use the context menu
            // to create new folders, but I don't know if that can happen here.).
            HDXA_LetHandlerProcessCommandEx(hdxa, &ici, NULL);
            hres = S_OK;
        }
        else
        {
            for (nItem = 0; nItem < ARRAYSIZE(c_IDFSEffects); ++nItem)
            {
                if (idCmd == c_IDFSEffects[nItem].uID)
                {
                    dwEffectOut = c_IDFSEffects[nItem].dwEffect;
                    break;
                }
            }

            // if hmenuReplace had menu commands other than DDIDM_COPY,
            // DDIDM_MOVE, DDIDM_LINK, and that item was selected,
            // this assert will catch it.  (dwEffectOut is 0 in this case)
            ASSERT(nItem < ARRAYSIZE(c_IDFSEffects));

            hres = S_FALSE;
        }

        if (hdca)
            DCA_Destroy(hdca);

        if (hdxa)
            HDXA_Destroy(hdxa);

        pddm->idCmd = idCmd;
    }

    *(pddm->pdwEffect) = dwEffectOut;

    return hres;
}
