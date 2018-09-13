#include "shellprv.h"

#include "mmhelper.h"
#include "ids.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "shlwapip.h"
#include "ole2dup.h"

#include "datautil.h"

#include "undo.h"
#include "defview.h"

#define DEF_FOLDERMENU_MAXHKEYS 16

// used with static defcm elements (pointer from mii.dwItemData)
// and find extensions
typedef struct
{
    WCHAR wszMenuText[MAX_PATH];
    WCHAR wszHelpText[MAX_PATH];
    int   iIcon;
} SEARCHEXTDATA;

typedef struct
{
    SEARCHEXTDATA* psed;
    UINT           idCmd;
} SEARCHINFO;

//Defined in fsmenu.obj
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

//
// static verbs, added by getting 
//

const struct {
    LPCTSTR pszCmd;
    WPARAM  idDFMCmd;
    UINT   idDefCmd;
} c_sDFMCmdInfo[] = {
    { c_szDelete,       DFM_CMD_DELETE,         DCMIDM_DELETE },
    { c_szCut,          DFM_CMD_MOVE,           DCMIDM_CUT },
    { c_szCopy,         DFM_CMD_COPY,           DCMIDM_COPY },
    { c_szPaste,        DFM_CMD_PASTE,          DCMIDM_PASTE },
    { c_szLink,         DFM_CMD_LINK,           DCMIDM_LINK },
    { c_szProperties,   DFM_CMD_PROPERTIES,     DCMIDM_PROPERTIES },
    { c_szPaste,        DFM_CMD_PASTE,          0 },
    { c_szPasteLink,    DFM_CMD_PASTELINK,      0 },
    { c_szRename,       DFM_CMD_RENAME,         DCMIDM_RENAME },
};



//=============================================================================
// CDefFolderMenu class
//=============================================================================

class CDefFolderMenu : public IContextMenu3, 
                       public IObjectWithSite, 
                       public IServiceProvider,
                       public ISearchProvider
{
    friend HRESULT CDefFolderMenu_CreateHKeyMenu(HWND hwnd, HKEY hkey, IContextMenu **ppcm);
    friend HRESULT CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                             UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, IContextMenuCB *pcmcb, 
                             UINT nKeys, const HKEY *ahkeyClsKeys, 
                             IContextMenu **ppcm);

public:

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType,
                                UINT *pwRes, LPSTR pszName, UINT cchMax);

    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *pUnkSite);        
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void ** ppvObj);

    // ISearchProvider
    STDMETHOD(GetSearchGUID)(GUID *pGuid);

private:
    CDefFolderMenu(UINT nKeys);
    ~CDefFolderMenu();
    ULONG   _GetAttributes(ULONG dwAttrMask);
    UINT    _Static_Add(HMENU hmenu, UINT idCmd, UINT idCmdLast, HKEY hkey);
    void    _Static_InvokeCommand(UINT iCmd);
    HRESULT _InitDropTarget(DWORD *pdwAttr);
    HRESULT _ProcessEditPaste(BOOL fPasteLink);
    HRESULT _ProcessRename();
    void    _DrawItem(DRAWITEMSTRUCT *pdi);
    LRESULT _MeasureItem(MEASUREITEMSTRUCT *pmi);


private:
    IDropTarget     *_pdtgt;         // Drop target of selected item
    IContextMenuCB  *_pcmcb;         // Callback object
    IDataObject     *_pdtobj;        // Data object
    IShellFolder    *_psf;           // Shell folder
    LONG            _cRef;           // Reference count
    HWND            _hwnd;           // Owner window
    UINT            _idCmdFirst;     // base id
    UINT            _idStdMax;       // standard commands (cut/copy/delete/properties) ID MAX
    UINT            _idFolderMax;    // Folder command ID MAX
    UINT            _idVerbMax;      // Add-in command (verbs) ID MAX
    UINT            _idDelayInvokeMax;// extensiosn loaded at invoke time
    UINT            _idFld2Max;      // 2nd range of Folder command ID MAX
    HDSA            _hdsaStatics;    // For static menu items.
    HDXA            _hdxa;           // Dynamic menu array
    HDSA            _hdsaCustomInfo; // array of SEARCHINFO's
    LPITEMIDLIST    _pidlFolder;
    IUnknown*       _pSite;

    BOOL            _bUnderKeys;     // Data is directly under key, not
                                    // shellex\ContextMenuHandlers
    UINT            _nKeys;          // Number of class keys
    HKEY            _hkeyClsKeys[DEF_FOLDERMENU_MAXHKEYS];  // Class keys

    HMENU           _hmenu;
    BITBOOL         _bFindHack      : 1;
    BITBOOL         _bInitMenuPopup : 1; // true if we received WM_INITMENUPOPUP and _bFindHack = TRUE
    INT             _iStaticInvoked; // index of the invoked static item
};


#define GetFldFirst(this) (_idStdMax + _idCmdFirst)

HRESULT HDXA_FindByCommand(HDXA hdxa, UINT idCmd, REFIID riid, void **ppv);

//----------------------------------------------------------------------------
typedef struct
{
    CLSID clsid;
    UINT idCmd;
    UINT idMenu; // used in cleanup
    GUID  guidSearch; //used with search extensions only
} STATICITEMINFO, *PSTATICITEMINFO;

//----------------------------------------------------------------------------

STDAPI CDefFolderMenu_CreateHKeyMenu(HWND hwnd, HKEY hkey, IContextMenu **ppcm)
{
    HRESULT hres = E_OUTOFMEMORY;
    CDefFolderMenu *pmenu = new CDefFolderMenu(1);
    
    IDLData_InitializeClipboardFormats();

    if (pmenu)
    {
        pmenu->_hwnd = hwnd;
        pmenu->_hdxa = HDXA_Create();
        ASSERT(pmenu->_pidlFolder == NULL);
        ASSERT(pmenu->_pSite == NULL);
        if (pmenu->_hdxa)
        {
            if (hkey) 
            {
                RegOpenKeyEx(hkey, NULL, 0L, MAXIMUM_ALLOWED, &pmenu->_hkeyClsKeys[0]);
                pmenu->_nKeys = 1;
                pmenu->_bUnderKeys = TRUE;
            }
            *ppcm = (IContextMenu *)pmenu;
        }

        hres = NOERROR;
    }
    return hres;
}


//=============================================================================
// CDefFolderMenu : Members
//=============================================================================

STDMETHODIMP CDefFolderMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CDefFolderMenu, IContextMenu, IContextMenu3),
        QITABENTMULTI(CDefFolderMenu, IContextMenu2, IContextMenu3),
        QITABENT(CDefFolderMenu, IContextMenu3), 
        QITABENT(CDefFolderMenu, IObjectWithSite), 
        QITABENT(CDefFolderMenu, IServiceProvider),
        QITABENT(CDefFolderMenu,ISearchProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDefFolderMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

CDefFolderMenu::CDefFolderMenu(UINT nKeys)
{
    _cRef = 1;
    _iStaticInvoked = -1;

    TraceMsg(TF_DOCFIND, "CDefFolderMenu ctor");

    ASSERT(nKeys <= DEF_FOLDERMENU_MAXHKEYS);
}

CDefFolderMenu::~CDefFolderMenu()
{
    UINT nKeys;

    TraceMsg(TF_DOCFIND, "CDefFolderMenu dtor");

    if (_hdxa) 
        HDXA_Destroy(_hdxa);

    ATOMICRELEASE(_psf);

    ATOMICRELEASE(_pSite);

    ATOMICRELEASE(_pdtgt);

    ATOMICRELEASE(_pdtobj);        

    if (_pcmcb) 
    {
        IUnknown_SetSite(_pcmcb, NULL);
        _pcmcb->Release();
    }

    for (nKeys=_nKeys-1; (int)nKeys>=0; --nKeys)
    {
        RegCloseKey(_hkeyClsKeys[nKeys]);
    }

    // if _bInitMenuPopup = true then we changed the dwItemData of the non static items
    // so we have to free them. otherwise don't touch them
    if (_hdsaCustomInfo)
    {
        // remove the custom data structures hanging off mii.dwItemData of static menu items
        // or all items if _bFindHack is true
        int cItems = DSA_GetItemCount(_hdsaCustomInfo);

        for (int i = 0; i < cItems; i++)
        {
            SEARCHINFO* psinfo = (SEARCHINFO*)DSA_GetItemPtr(_hdsaCustomInfo, i);
            ASSERT(psinfo);
            SEARCHEXTDATA* psed = psinfo->psed;

            if (psed)
                LocalFree(psed);
        }
        DSA_Destroy(_hdsaCustomInfo);
    }

    if (_hdsaStatics)
        DSA_Destroy(_hdsaStatics);

    ILFree(_pidlFolder);
}

STDMETHODIMP_(ULONG) CDefFolderMenu::Release()
{
    if (InterlockedDecrement(&(_cRef))) 
        return _cRef;

    if (_pcmcb)
        _pcmcb->CallBack(_psf, _hwnd, NULL, DFM_RELEASE, _idStdMax, 0);

    delete this;
    return 0;
}

int _SHMergePopupMenus(HMENU hmMain, HMENU hmMerge, int idCmdFirst, int idCmdLast)
{
    int i, idMax = idCmdFirst;

    for (i = GetMenuItemCount(hmMerge) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_ID|MIIM_SUBMENU;
        mii.cch = 0;     // just in case

        if (GetMenuItemInfo(hmMerge, i, TRUE, &mii))
        {
            int idTemp = Shell_MergeMenus(_GetMenuFromID(hmMain, mii.wID),
                mii.hSubMenu, (UINT)0, idCmdFirst, idCmdLast,
                MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            if (idMax < idTemp)
                idMax = idTemp;
        }
    }

    return idMax;
}


void CDefFolderMenu_MergeMenu(HINSTANCE hinst, UINT idMainMerge, UINT idPopupMerge, LPQCMINFO pqcm)
{
    UINT idMax = pqcm->idCmdFirst;

    if (idMainMerge)
    {
        HMENU hmMerge = SHLoadPopupMenu(hinst, idMainMerge);
        if (hmMerge)
        {
            idMax = Shell_MergeMenus(
                    pqcm->hmenu, hmMerge, pqcm->indexMenu,
                    pqcm->idCmdFirst, pqcm->idCmdLast,
                    MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);
                
            DestroyMenu(hmMerge);
        }
    }

    if (idPopupMerge)
    {
        HMENU hmMerge = LoadMenu(hinst, MAKEINTRESOURCE(idPopupMerge));
        if (hmMerge)
        {
            UINT idTemp = _SHMergePopupMenus(pqcm->hmenu, hmMerge,
                    pqcm->idCmdFirst, pqcm->idCmdLast);
            if (idMax < idTemp)
                idMax = idTemp;

            DestroyMenu(hmMerge);
        }
    }

    pqcm->idCmdFirst = idMax;
}


ULONG CDefFolderMenu::_GetAttributes(ULONG dwAttrMask)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
    if (pida)
    {
        LPITEMIDLIST *ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * SIZEOF(LPCITEMIDLIST));
        if (ppidl)
        {
            int i;
            BOOL fAllocated;

            for (i = pida->cidl - 1; i >= 0; --i)
            {
                ppidl[i] = (LPITEMIDLIST)IDA_GetRelativeIDListPtr(pida, i, &fAllocated);
            }

            _psf->GetAttributesOf(pida->cidl, (const _ITEMIDLIST **)ppidl, &dwAttrMask);

            if (fAllocated) 
            {
                for (i = pida->cidl - 1; i >= 0; --i)
                {
                    ILFree(ppidl[i]);
                }
            }

            LocalFree((HLOCAL)ppidl);
        }
        else
        {
            dwAttrMask = 0;
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }

    return dwAttrMask;
}


void _DisableRemoveMenuItem(HMENU hmInit, UINT uID, BOOL bAvail, BOOL bRemoveUnavail)
{
    if (bAvail)
    {
        EnableMenuItem(hmInit, uID, MF_ENABLED|MF_BYCOMMAND);
    }
    else if (bRemoveUnavail)
    {
        DeleteMenu(hmInit, uID, MF_BYCOMMAND);
    }
    else
    {
        EnableMenuItem(hmInit, uID, MF_GRAYED|MF_BYCOMMAND);
    }
}


//
// Enable/disable menuitems in the "File" pulldown.
//
void Def_InitFileCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst, BOOL bContext)
{
    idCmdFirst -= SFVIDM_FIRST;

    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_RENAME+idCmdFirst,
        dwAttr & SFGAO_CANRENAME, bContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_DELETE+idCmdFirst,
        dwAttr & SFGAO_CANDELETE, bContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_FILE_LINK+idCmdFirst,
        dwAttr & SFGAO_CANLINK, bContext);

    // Check to see if the folder supports properties on objects, if it is
    // the context menu then we are allowed to remove the item, otherwise just
    // gray it.
    _DisableRemoveMenuItem( hmInit, SFVIDM_FILE_PROPERTIES + idCmdFirst, dwAttr & SFGAO_HASPROPSHEET, bContext);
}

STDAPI_(BOOL) Def_IsPasteAvailable(IDropTarget *pdtgt, DWORD *pdwEffect)
{
    BOOL fRet = FALSE;

    *pdwEffect = 0;     // assume none

    // Count the number of clipboard formats available, if there are none then there
    // is no point making the clipboard available.
    
    if (CountClipboardFormats() > 0)
    {
        if (pdtgt)
        {
            DECLAREWAITCURSOR;

            SetWaitCursor();

            IDataObject * pdtobj;
            if (SUCCEEDED(OleGetClipboard(&pdtobj)))
            {
                POINTL pt = {0, 0};
                DWORD dwEffectOffered = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY | DROPEFFECT_LINK);

                // Check if we can paste.
                //
                DWORD dwEffect = (dwEffectOffered & (DROPEFFECT_MOVE | DROPEFFECT_COPY));
                if (dwEffect)
                {
                    if (SUCCEEDED(pdtgt->DragEnter(pdtobj, MK_RBUTTON, pt, &dwEffect)))
                    {
                        pdtgt->DragLeave();
                    }
                    else
                    {
                        dwEffect = 0;
                    }
                }

                //
                // Check if we can past-link.
                //
                DWORD dwEffectLink = (dwEffectOffered & DROPEFFECT_LINK);
                if (dwEffectLink)
                {
                    if (SUCCEEDED(pdtgt->DragEnter(pdtobj, MK_RBUTTON, pt, &dwEffectLink)))
                    {
                        pdtgt->DragLeave();
                        dwEffect |= dwEffectLink;
                    }
                }

                fRet = (dwEffect & (DROPEFFECT_MOVE | DROPEFFECT_COPY));
                *pdwEffect = dwEffect;

                pdtobj->Release();
            }
            ResetWaitCursor();
        }
    }

    return fRet;
}

void Def_InitEditCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst, IDropTarget *pdtgt, UINT fContext)
{
    DWORD dwEffect = 0;
    BOOL bEnableUndo;
    TCHAR szMenuText[80];

    idCmdFirst -= SFVIDM_FIRST;

    // enable undo if there's an undo history
    bEnableUndo = IsUndoAvailable();
    if (bEnableUndo)
    {
        GetUndoText(PeekUndoAtom(), szMenuText, ARRAYSIZE(szMenuText), UNDO_MENUTEXT);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_UNDOMENU, szMenuText, ARRAYSIZE(szMenuText));
    }

    // BUGBUG - raymondc - RIPs if the menu doesn't have an Undo command
    if (szMenuText[0]) 
    {
        ModifyMenu(hmInit, SFVIDM_EDIT_UNDO + idCmdFirst,
                   MF_BYCOMMAND | MF_STRING,
                   SFVIDM_EDIT_UNDO + idCmdFirst, szMenuText);
    }
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_UNDO + idCmdFirst,
                           bEnableUndo, fContext);

    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_CUT+idCmdFirst,
        dwAttr & SFGAO_CANMOVE, fContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_COPY+idCmdFirst,
        dwAttr & SFGAO_CANCOPY, fContext);

    //
    //  Enable the "Paste" menuitem if the background drop target can
    // handle what's in the clipboard
    //
    // Never remove the "Paste" command
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_PASTE+idCmdFirst,
                           Def_IsPasteAvailable(pdtgt, &dwEffect), fContext & DIEC_SELECTIONCONTEXT);

    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_PASTELINK+idCmdFirst,
        dwEffect & DROPEFFECT_LINK, fContext & DIEC_SELECTIONCONTEXT);

    // _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_PASTESPECIAL+idCmdFirst,
    //  dwEffect & (DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK), fContext & DIEC_SELECTIONCONTEXT);

    // enable disable move to/copy to items
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_MOVETO+idCmdFirst,
        dwAttr & SFGAO_CANMOVE, fContext);
    _DisableRemoveMenuItem(hmInit, SFVIDM_EDIT_COPYTO+idCmdFirst,
        dwAttr & SFGAO_CANCOPY, fContext);

}


const TCHAR c_szStatic[] = TEXT("Static");

int Static_ExtractIcon(HKEY hkeyMenuItem)
{
    HKEY hkeyDefIcon;
    int iImage = -1;

    if (RegOpenKey(hkeyMenuItem, c_szDefaultIcon, &hkeyDefIcon) == ERROR_SUCCESS)
    {
        TCHAR szDefIcon[MAX_PATH];
        DWORD dwType, cb = SIZEOF(szDefIcon);

        if (SHQueryValueEx(hkeyDefIcon, NULL, NULL, &dwType, (BYTE*)szDefIcon, &cb) == ERROR_SUCCESS)
        {
            iImage = Shell_GetCachedImageIndex(szDefIcon, PathParseIconLocation(szDefIcon), 0);
        }
        RegCloseKey(hkeyDefIcon);
    }
    return iImage;
}

#define SEARCH_GUID   TEXT("SearchGUID")
#define HELP_TEXT     TEXT("HelpText")

#define LAST_ITEM  (int)0x7FFFFFFF

//----------------------------------------------------------------------------
UINT CDefFolderMenu::_Static_Add(HMENU hmenu, UINT idCmd, UINT idCmdLast, HKEY hkey)
{
    HDSA hdsaStatics;
    HDSA hdsaCustomInfo;

    if (idCmd > idCmdLast)
    {
        DebugMsg(DM_ERROR, TEXT("si_a: Out of command ids!"));
        return idCmd;
    }

    ASSERT(!_hdsaStatics);
    ASSERT(!_hdsaCustomInfo);

    hdsaCustomInfo = DSA_Create(SIZEOF(SEARCHINFO), 1);
    // Create a hdsaStatics.
    hdsaStatics = DSA_Create(SIZEOF(STATICITEMINFO), 1);
    if (hdsaStatics && hdsaCustomInfo)
    {
        HKEY hkeyStatic;
        // Try to open the "Static" subkey.
        if (RegOpenKey(hkey, c_szStatic, &hkeyStatic) == ERROR_SUCCESS)
        {
            TCHAR szClass[MAX_PATH];
            int i;
            BOOL bFindFilesInserted = FALSE;

            // For each subkey of static.
            for (i = 0; RegEnumKey(hkeyStatic, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
            {
                HKEY hkeyClass;

                // Record the GUID.
                if (RegOpenKey(hkeyStatic, szClass, &hkeyClass) == ERROR_SUCCESS)
                {
                    TCHAR szCLSID[MAX_PATH];
                    DWORD dwType, cb = SIZEOF(szCLSID);
                    // HACKHACK: (together with bWebSearchInserted above
                    // we need to have On the Internet as the first menu item
                    // and Find Files or Folders as second
                    BOOL  bWebSearch = lstrcmp(szClass, TEXT("WebSearch")) == 0;
                    BOOL  bFindFiles = FALSE;

                    if (SHQueryValueEx(hkeyClass, NULL, NULL, &dwType, (BYTE*)szCLSID, &cb) == ERROR_SUCCESS)
                    {
                        HKEY hkeyMenuItem;
                        TCHAR szSubKey[32];
                        int iMenuItem;
                        

                        // enum the sub keys 0..N
                        for (iMenuItem = 0; wsprintf(szSubKey, TEXT("%d"), iMenuItem),
                                            RegOpenKey(hkeyClass, szSubKey, &hkeyMenuItem) == ERROR_SUCCESS; 
                            iMenuItem++)
                        {
                            TCHAR szMenuText[MAX_PATH];
                            TCHAR szSearchGUID[MAX_PATH];

                            // Get all the command text.
                            cb = SIZEOF(szMenuText);
                            // dwType = REG_SZ;
                            if (SHQueryValueEx(hkeyMenuItem, NULL, NULL, &dwType, (BYTE*)szMenuText, &cb) == ERROR_SUCCESS)
                            {
                                STATICITEMINFO sii;
                                SEARCHINFO     sinfo;
                                
                                int iIcon = Static_ExtractIcon(hkeyMenuItem);
                                TCHAR szHelpText[MAX_PATH];

                                cb = SIZEOF(szHelpText);
                                if (SHGetValue(hkeyMenuItem, HELP_TEXT, NULL, &dwType, (BYTE*)szHelpText, &cb) != ERROR_SUCCESS)
                                    szHelpText[0] = TEXT('\0');

                                SHCLSIDFromString(szCLSID, &sii.clsid); // store it
                                sii.idCmd = iMenuItem;
                                sii.idMenu = idCmd;

                                // get the search guid if any...
                                cb = SIZEOF(szSearchGUID);
                                if (SHGetValue(hkeyMenuItem, SEARCH_GUID, NULL, &dwType, (BYTE*)szSearchGUID, &cb) == ERROR_SUCCESS)
                                    SHCLSIDFromString(szSearchGUID, &sii.guidSearch);
                                else
                                    sii.guidSearch = GUID_NULL;

                                // HACKHACK: this is a hack. TODO: cleanup -- allow non-static
                                // find extensions to specify a search guid and then we can
                                // remove this static "Find Computer" business...
                                //
                                // if this is FindComputer item and the restriction is not set 
                                // don't add it to the menu
                                if (IsEqualGUID(sii.guidSearch, SRCID_SFindComputer) &&
                                    !SHRestricted(REST_HASFINDCOMPUTERS))
                                    continue;

                                bFindFiles = IsEqualGUID(sii.guidSearch, SRCID_SFileSearch);
                                if (bFindFiles && SHRestricted(REST_NOFIND))
                                    continue;

                                if (IsEqualGUID(sii.guidSearch, SRCID_SFindPrinter))
                                    continue;
                                    
                                DSA_AppendItem(hdsaStatics, &sii);

                                // Create the menu.
                                //AppendMenu(hmenu, MF_STRING, idCmd, szMenuText);

                                // Add the icon if there is one.
                                {
                                    MENUITEMINFO mii;
                                    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)LocalAlloc(LPTR, sizeof(SEARCHEXTDATA));
                                    if (psed)
                                    {
                                        psed->iIcon = iIcon;
                                        SHTCharToUnicode(szHelpText, psed->wszHelpText, ARRAYSIZE(psed->wszHelpText));
                                        SHTCharToUnicode(szMenuText, psed->wszMenuText, ARRAYSIZE(psed->wszMenuText));
                                    }
                                    
                                    mii.cbSize = SIZEOF(MENUITEMINFO);
                                    mii.fMask  = MIIM_DATA | MIIM_TYPE | MIIM_ID;
                                    mii.fType  = MFT_OWNERDRAW;
                                    mii.wID    = idCmd;
                                    mii.dwItemData = (DWORD_PTR)psed; //iIcon;

                                    sinfo.psed = psed;
                                    sinfo.idCmd = idCmd;
                                    if (DSA_AppendItem(hdsaCustomInfo, &sinfo) != -1)
                                    {      
                                        // insert Files or Folders in the first place (see HACKHACK above)
                                        if (!bFindFilesInserted && bFindFiles)
                                            bFindFilesInserted = InsertMenuItem(hmenu, 0, TRUE, &mii);
                                        else
                                        {
                                            UINT uiPos = LAST_ITEM;

                                            // if this is Find Files or Folders insert it after
                                            // On the Internet or in the first place if OtI is
                                            // not inserted yet
                                            if (bWebSearch)
                                                uiPos = bFindFilesInserted ? 1 : 0;
                                            // we don't free psed if Insert fails because it's 
                                            // in dsa and it's going to be freed on destroy
                                            InsertMenuItem(hmenu, uiPos, TRUE, &mii);
                                        }
                                    }
                                }
                                // Next command.
                                idCmd++;
                                if (idCmd > idCmdLast)
                                {
                                    DebugMsg(DM_ERROR, TEXT("si_a: Out of command ids!"));
                                    break;
                                }
                            }
                            RegCloseKey(hkeyMenuItem);
                        }
                    }
                    RegCloseKey(hkeyClass);
                }
            }
            RegCloseKey(hkeyStatic);
        }
        _hdsaStatics = hdsaStatics;
        _hdsaCustomInfo = hdsaCustomInfo;
    }
    return idCmd;
}


#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

void CDefFolderMenu::_Static_InvokeCommand(UINT iCmd)
{
    if (_hdsaStatics)
    {
        PSTATICITEMINFO psii = (PSTATICITEMINFO)DSA_GetItemPtr(_hdsaStatics, iCmd);
        if (psii)
        {
            IContextMenu *pcm;
            if (SUCCEEDED(SHExtCoCreateInstance(NULL, &psii->clsid, NULL,
                IID_PPV_ARG(IContextMenu, &pcm))))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    CMINVOKECOMMANDINFO ici;
                    CHAR                szSearchGUID[GUIDSTR_MAX];
                    LPSTR               psz = NULL;

                    _iStaticInvoked = iCmd;
                    IUnknown_SetSite(pcm, SAFECAST(this, IContextMenu3*));
                    
                    pcm->QueryContextMenu(hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, CMF_NORMAL);
                    ici.cbSize = SIZEOF(CMINVOKECOMMANDINFO);
                    ici.fMask = 0;
                    ici.hwnd = NULL;
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(psii->idCmd);
                    if (!IsEqualGUID(psii->guidSearch, GUID_NULL))
                    {
                        SHStringFromGUIDA(psii->guidSearch, szSearchGUID, ARRAYSIZE(szSearchGUID));
                        psz = szSearchGUID;
                    }
                    ici.lpParameters = psz;
                    ici.lpDirectory = NULL;
                    ici.nShow = SW_NORMAL;
                    pcm->InvokeCommand(&ici);
                    DestroyMenu(hmenu);
                    IUnknown_SetSite(pcm, NULL);
                }
                pcm->Release();
            }
        }
    }
}

HRESULT CDefFolderMenu::_InitDropTarget(DWORD *pdwAttr)
{
    HRESULT hres = E_FAIL;

    if (_pdtgt)    // already have one?
        return NOERROR;

    if (_pdtobj) 
    {
        STGMEDIUM medium;
        DWORD dwAttr = _GetAttributes(
                SFGAO_CANRENAME | SFGAO_CANDELETE | 
                SFGAO_CANLINK   | SFGAO_HASPROPSHEET |
                SFGAO_CANCOPY   | SFGAO_CANMOVE | SFGAO_DROPTARGET);

        LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
        if (pida)
        {
            if (dwAttr & SFGAO_DROPTARGET)
            {
                BOOL fAllocated;

                LPCITEMIDLIST pidl = IDA_GetRelativeIDListPtr(pida, 0, &fAllocated);
                // ok if it fails...  initeditcommands will grey out paste option
                hres = _psf->GetUIObjectOf(_hwnd, 1, &pidl, IID_IDropTarget, 0, (void **)&_pdtgt);

                if (fAllocated)
                    ILFree ((LPITEMIDLIST)pidl);
            }
            HIDA_ReleaseStgMedium(pida, &medium);
        }

        ASSERT(pdwAttr);
        *pdwAttr = dwAttr;
    } 
    else 
    {
        hres = _psf->CreateViewObject(_hwnd, IID_PPV_ARG(IDropTarget, &_pdtgt));
    }
    return hres;
}
 

STDMETHODIMP CDefFolderMenu::QueryContextMenu(HMENU hmenu,
        UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };
    DECLAREWAITCURSOR;
    HRESULT hres;
    BOOL fUseDefExt;

    SetWaitCursor();

    _idCmdFirst = idCmdFirst;
    _hmenu = hmenu;
    _bFindHack = BOOLIFY(uFlags & CMF_FINDHACK);
    _bInitMenuPopup = FALSE;

    // first add in the folder commands like cut/copy/paste
    if (_pdtobj && !(uFlags & (CMF_VERBSONLY | CMF_DVFILE)))
    {
        ULONG dwAttr;

        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DCM_ITEM, 0, &qcm);

        //
        // If there is previously got drop target, release it.
        //
        if (_pdtgt) 
        {
            _pdtgt->Release();
            _pdtgt = NULL;
        }

        _InitDropTarget(&dwAttr);
        if (!(uFlags & CMF_CANRENAME))
            dwAttr &= ~(SFGAO_CANRENAME);

        Def_InitFileCommands(dwAttr, hmenu, idCmdFirst, TRUE);
        Def_InitEditCommands(dwAttr, hmenu, idCmdFirst, _pdtgt, DIEC_SELECTIONCONTEXT);

    }
    _idStdMax = qcm.idCmdFirst - idCmdFirst;

    //
    // DFM_MERGECONTEXTMENU returns (S_FALSE) if we should not
    // add any verbs.
    //
    if (_pcmcb) 
    {
        hres = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MERGECONTEXTMENU, uFlags, (LPARAM)&qcm);
        fUseDefExt = (hres == NOERROR);
        if (_pdtobj && (hres == ResultFromShort(-1)) && !(uFlags & CMF_NODEFAULT) &&
            (GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0) == -1)) 
        {
            SetMenuDefaultItem(hmenu, (idCmdFirst - SFVIDM_FIRST) + SFVIDM_FILE_PROPERTIES, MF_BYCOMMAND);
        }
    }
    else 
    {
        fUseDefExt = FALSE;
    }

    _idFolderMax = qcm.idCmdFirst - idCmdFirst;
    // add registry verbs
    if ((!(uFlags & CMF_NOVERBS)) ||
        (!_pdtobj && !_psf && _nKeys)) // this second case is for find extensions
    {


        //
        // Put the separator between container menuitems and object menuitems
        //  only if we don't have the separator at the insertion point.
        //
        MENUITEMINFO mii;
        HDCA hdca;

        //HACK: Default Extenstions EXPECT a selection, Let's hope all don't
        if(!_pdtobj)
            fUseDefExt = FALSE;

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_TYPE;
        mii.cch = 0;            // WARNING: We must put 0 here!!!!
        mii.dwTypeData = NULL;
        mii.fType = MFT_SEPARATOR;              // to avoid ramdom result.
        if (GetMenuItemInfo(hmenu, indexMenu, TRUE, &mii) && !(mii.fType & MFT_SEPARATOR))
        {
            InsertMenu(hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);
        }

        hdca = DCA_Create();
        if (hdca)
        {
            UINT nKeys;
            //
            //  Add default extensions, only if the folder callback returned
            // NOERROR. The Printer and Control folder returns S_FALSE
            // indicating that they don't need any default extension.
            //
            if (fUseDefExt)
            {
                // Always add this default extention at the top.
                DCA_AddItem(hdca, CLSID_ShellFileDefExt);
            }

             // Append menus for all extensions
            for (nKeys = 0; nKeys < _nKeys; ++nKeys)
            {
                DCA_AddItemsFromKey(hdca, _hkeyClsKeys[nKeys],
                        _bUnderKeys ? NULL : STRREG_SHEX_MENUHANDLER);
            }
            // BUGBUG:
            // first time we call this _hdxa is empty            
            // after that it has the same items as before but will not add any new ones
            // if user keeps right clicking we will eventually run out of menu item ids
            // read comment in HDXA_AppendMenuItems2. to prevent it we empty _hdxa
            HDXA_DeleteAll(_hdxa);

            // BUGBUG (lamadio) For background context menu handlers, the pidlFolder 
            // should be a valid pidl, but, for backwards compatilility, this 
            // parameter should be NULL, if the Dataobject is NOT NULL.
            // We need to rewrite the background CMs to unpack the dataobject to get the
            // Folder Pidl.
            qcm.idCmdFirst = HDXA_AppendMenuItems2(_hdxa, _pdtobj,
                            _nKeys, _hkeyClsKeys,
                            (!_pdtobj)?(_pidlFolder):NULL, 
                            &qcm, uFlags, hdca, _pSite);

            //
            //  if no default menu got set, choose the first one.
            //
            if (_pdtobj && !(uFlags & CMF_NODEFAULT) &&
                    GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1)
            {
                UINT idStatic;

                //
                // we are about to set the default menu id, give the callback a chance
                // to override and set one of the static entries instead of the
                // first entry in the menu.
                //

                if (_pcmcb && SUCCEEDED(_pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                                                         DFM_GETDEFSTATICID, 
                                                         0, (LPARAM)&idStatic)))
                {
                    for (int i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
                    {
                        if (idStatic == c_sDFMCmdInfo[i].idDFMCmd)
                        {
                            SetMenuDefaultItem(hmenu, idCmdFirst+c_sDFMCmdInfo[i].idDefCmd, MF_BYCOMMAND);
                            break;
                        }
                    }
                }

                if ( GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1 )
                    SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
            }

            DCA_Destroy(hdca);
        }

        _idVerbMax = qcm.idCmdFirst - idCmdFirst;

        // menu extensions that are loaded at invoke time
        if (uFlags & CMF_INCLUDESTATIC)
        {
            qcm.idCmdFirst = _Static_Add(hmenu, qcm.idCmdFirst, idCmdLast, _hkeyClsKeys[0]);
        }
        _idDelayInvokeMax = qcm.idCmdFirst - idCmdFirst;

        //
        // Remove the separator if we did not add any.
        //
        if ((_idDelayInvokeMax == _idFolderMax))
        {
            if(GetMenuState(hmenu, 0, MF_BYPOSITION) & MF_SEPARATOR)
                DeleteMenu(hmenu, 0, MF_BYPOSITION);
        }
    }

    // And now we give the callback the option to put (more) commands on top
    // of everything else
    if (_pcmcb)
        _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_MERGECONTEXTMENU_TOP, uFlags, (LPARAM)&qcm);

    _idFld2Max = qcm.idCmdFirst - idCmdFirst;

    _SHPrettyMenu(hmenu);

    ResetWaitCursor();

    return ResultFromShort(_idFld2Max);
}

HRESULT CDefFolderMenu::_ProcessEditPaste(BOOL fPasteLink)
{
    DWORD dwAttr;
    DECLAREWAITCURSOR;

    SetWaitCursor();

    HRESULT hr = _InitDropTarget(&dwAttr);
    if (SUCCEEDED(hr))
    {
        IDataObject *pdtobj;
        hr = OleGetClipboard(&pdtobj);
        if (SUCCEEDED(hr))
        {
            DWORD grfKeyState, dwEffect;
            DWORD dwEffectIn = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY | DROPEFFECT_LINK);

            if (fPasteLink) 
            {
                // MK_FAKEDROP to avoid drag/drop pop up menu
                grfKeyState = MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_FAKEDROP;
                dwEffectIn &= DROPEFFECT_LINK;
            } 
            else
            {
                grfKeyState = MK_LBUTTON;
                dwEffectIn &= ~DROPEFFECT_LINK;
            }

            IUnknown_SetSite(_pdtgt, _pSite);

            // simulate the drag drop protocol
            dwEffect = dwEffectIn;
            hr = SHSimulateDrop(_pdtgt, pdtobj, grfKeyState, NULL, &dwEffect);

            IUnknown_SetSite(_pdtgt, NULL);

            if (SUCCEEDED(hr))
            {
                // these formats are put into the data object by the drop target code. this
                // requires the data object support ::SetData() for arbitrary data formats
                //
                // g_cfPerformedDropEffect effect is the reliable version of dwEffect (some targets 
                // return dwEffect == DROPEFFECT_MOVE always)
                //
                // g_cfLogicalPerformedDropEffect indicates the logical action so we can tell the
                // difference between optmized and non optimized move

                DWORD dwPerformedEffect        = DataObj_GetDWORD(pdtobj, g_cfPerformedDropEffect, DROPEFFECT_NONE);
                DWORD dwLogicalPerformedEffect = DataObj_GetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, DROPEFFECT_NONE);

                if ((DROPEFFECT_MOVE == dwLogicalPerformedEffect) ||
                    (DROPEFFECT_MOVE == dwEffect && DROPEFFECT_MOVE == dwPerformedEffect))
                {
                    // communicate back the source data object 
                    // so they can complete the "move" if necessary

                    DataObj_SetDWORD(pdtobj, g_cfPasteSucceeded, dwEffect);

                    // if we just did a paste and we moved the files we cant paste
                    // them again (because they moved!) so empty the clipboard

                    OleSetClipboard(NULL);
                }
            }
            pdtobj->Release();
        }
    }
    ResetWaitCursor();

    if (FAILED(hr))
        MessageBeep(0);

    return hr;
}

//
// BUGBUG:: This is complete trash, the view added the Rename command (sortof), but we have
// no way to actually try to execute it.  
HRESULT CDefFolderMenu::_ProcessRename()
{
    IContextMenuSite *pcms;
    HRESULT hr = IUnknown_QueryService(_pSite, SID_SContextMenuSite, IID_PPV_ARG(IContextMenuSite, &pcms));
    if (SUCCEEDED(hr))
    {
        hr = pcms->DoRename();
        pcms->Release();
    }
    return hr;
}

// deal with the versioning of this structure...

void CopyInvokeInfo(CMINVOKECOMMANDINFOEX *pici, const CMINVOKECOMMANDINFO *piciIn)
{
    ASSERT(piciIn->cbSize >= SIZEOF(CMINVOKECOMMANDINFO));

    ZeroMemory(pici, SIZEOF(CMINVOKECOMMANDINFOEX));
    memcpy(pici, piciIn, min(SIZEOF(CMINVOKECOMMANDINFOEX), piciIn->cbSize));
    pici->cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
}

#ifdef UNICODE        
#define IS_UNICODE_ICI(pici) ((pici->cbSize >= CMICEXSIZE_NT4) && ((pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE))
#else
#define IS_UNICODE_ICI(pici) (FALSE)
#endif


STDMETHODIMP CDefFolderMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres = NOERROR;
    WPARAM idCmd = (WPARAM)-1;
    WPARAM idCmdLocal;  // this is used within each if block for the local idCmd value
    LPCMINVOKECOMMANDINFOEX picix = (LPCMINVOKECOMMANDINFOEX)pici; // This value is only usable when fCmdInfoEx is true

    BOOL fUnicode = IS_UNICODE_ICI(pici);

    if (pici->cbSize < SIZEOF(CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    if (!IS_INTRESOURCE(pici->lpVerb))
    {
        LPCTSTR pszVerb;
#ifdef UNICODE
        WCHAR szVerb[MAX_PATH];

        if (!fUnicode || picix->lpVerbW == NULL)
        {
            SHAnsiToUnicode(picix->lpVerb, szVerb, ARRAYSIZE(szVerb));
            pszVerb = szVerb;
        }
        else
            pszVerb = picix->lpVerbW;
#else
        pszVerb = pici->lpVerb;
#endif
        idCmdLocal = idCmd;

        for (int i = 0; i < ARRAYSIZE(c_sDFMCmdInfo) ; i++)
        {
            if (lstrcmpi(pszVerb, c_sDFMCmdInfo[i].pszCmd)==0) 
            {
                idCmdLocal = c_sDFMCmdInfo[i].idDFMCmd;
                // We need to use goto because idFolderMax might not be initialized
                // yet (QueryContextMenu might have not been called).
                goto ProcessCommand;
            }
        }

        // see if this is a command provided by name by the callback
        if (*pszVerb && SUCCEEDED(_pcmcb->CallBack(_psf, _hwnd,
            _pdtobj, DFM_MAPCOMMANDNAME, (WPARAM)&idCmdLocal,
            (LPARAM)pszVerb)))
        {
            goto ProcessCommand;
        }

        // we need to give the verbs a chance in case they asked for it by string
        goto ProcessVerb;
    }
    else
    {
        idCmd = LOWORD((UINT_PTR)pici->lpVerb);
    }

    if (idCmd < _idStdMax)
    {
        idCmdLocal = idCmd;

        for (int i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
        {
            if (idCmdLocal == c_sDFMCmdInfo[i].idDefCmd)
            {
                idCmdLocal = c_sDFMCmdInfo[i].idDFMCmd;
                goto ProcessCommand;
            }
        }

        hres = E_INVALIDARG;
    }
    else if (idCmd < _idFolderMax)
    {
        DFMICS dfmics;
        LPARAM lParam;
#ifdef UNICODE
        WCHAR szLParamBuffer[MAX_PATH];
#endif

        idCmdLocal = idCmd - _idStdMax;
ProcessCommand:


#ifdef UNICODE
        if (!fUnicode || picix->lpParametersW == NULL)
        {
            if (pici->lpParameters == NULL)
            {
                lParam = (LPARAM)NULL;
            }
            else
            {
                SHAnsiToUnicode(pici->lpParameters, szLParamBuffer, ARRAYSIZE(szLParamBuffer));
                lParam = (LPARAM)szLParamBuffer;
            }
        }
        else
            lParam = (LPARAM)picix->lpParametersW;
#else
        lParam = (LPARAM)pici->lpParameters;
#endif

        switch (idCmdLocal) 
        {
        case DFM_CMD_LINK:
#ifdef UNICODE
            if (!fUnicode || picix->lpDirectoryW == NULL)
            {
                if (pici->lpDirectory == NULL)
                {
                    lParam = (LPARAM)NULL;
                }
                else
                {
                    SHAnsiToUnicode(pici->lpDirectory, szLParamBuffer, ARRAYSIZE(szLParamBuffer));
                    lParam = (LPARAM)szLParamBuffer;
                }
            }
            else
                lParam = (LPARAM)picix->lpDirectoryW;
#else
            lParam = (LPARAM)pici->lpDirectory;
#endif
            break;

        case DFM_CMD_PROPERTIES:
             if (SHRestricted(REST_NOVIEWCONTEXTMENU))
             {
                // This is what the NT4 QFE returned, but I wonder
                // if HRESULT_FROM_WIN32(E_ACCESSDENIED) would be better?
                return hres;
             }
             break;
        }

        //
        // call the callback
        //
        // try to use a DFM_INVOKECOMMANDEX first so the callback can see
        // the INVOKECOMMANDINFO struct (for stuff like the 'no ui' flag)
        //
        dfmics.cbSize = sizeof(dfmics);
        dfmics.fMask = pici->fMask;
        dfmics.lParam = lParam;
        dfmics.idCmdFirst = _idCmdFirst;
        dfmics.idDefMax = _idStdMax;
        dfmics.pici = pici;

        // HACK alert: (dli) This is a hack for the property pages to show up right at
        // the POINT where they were activated. 
        if ((idCmdLocal == DFM_CMD_PROPERTIES) && (pici->fMask & CMIC_MASK_PTINVOKE) && _pdtobj)
        {
            HRESULT hres;
            ASSERT(pici->cbSize >= SIZEOF(CMINVOKECOMMANDINFOEX));
            POINT *ppt = (POINT *)GlobalAlloc(GPTR, SIZEOF(POINT));
            if (ppt)
            {
                *ppt = picix->ptInvoke;
                hres = DataObj_SetGlobal(_pdtobj, g_cfOFFSETS, ppt);
                if (FAILED(hres))
                    GlobalFree(ppt);
            }
        }

        hres = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_INVOKECOMMANDEX, 
            idCmdLocal, (LPARAM)&dfmics);
        if (hres == E_NOTIMPL)
        {
            // the callback didn't understand the DFM_INVOKECOMMANDEX
            // fall back to a regular DFM_INVOKECOMMAND instead
            //
            hres = _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_INVOKECOMMAND, idCmdLocal, lParam);
        }

        // Check if we need to execute the default code.
        if (hres == S_FALSE)
        {
            hres = NOERROR;     // assume no error

            if (_pdtobj)
            {
                switch (idCmdLocal) 
                {
                case DFM_CMD_MOVE:
                case DFM_CMD_COPY:
                    DataObj_SetDWORD(_pdtobj, g_cfPreferredDropEffect, 
                        (idCmdLocal == DFM_CMD_MOVE) ?
                        DROPEFFECT_MOVE : (DROPEFFECT_COPY | DROPEFFECT_LINK));

                    ShellFolderView_SetPoints(_hwnd, _pdtobj);
                    OleSetClipboard(_pdtobj);
                    // this needs to be done after the set clipboard so
                    // that the hwndView chain will be set right
                    ShellFolderView_SetClipboard(_hwnd, idCmdLocal);
                    break;

                case DFM_CMD_LINK:
                    SHCreateLinks(pici->hwnd, NULL, _pdtobj, lParam ? SHCL_USETEMPLATE | SHCL_USEDESKTOP : SHCL_USETEMPLATE, NULL);
                    break;

                case DFM_CMD_PASTE:
                case DFM_CMD_PASTELINK:
                    hres = _ProcessEditPaste(idCmdLocal == DFM_CMD_PASTELINK);
                    break;

                case DFM_CMD_RENAME:
                    hres = _ProcessRename();
                    break;

                default:
                    DebugMsg(TF_WARNING, TEXT("DefCM command not processed in %s at %d (%x)"),
                                    __FILE__, __LINE__, idCmdLocal);
                    break;
                }
            }
            else
            {
                // This is a background menu. Process common command ids.
                switch(idCmdLocal)
                {
                case DFM_CMD_PASTE:
                case DFM_CMD_PASTELINK:
                    hres = _ProcessEditPaste(idCmdLocal == DFM_CMD_PASTELINK);
                    break;

                default:
                    // Only our commands should come here
                    break;
                }
            }
        }
    }
    else if (idCmd < _idVerbMax)
    {
        idCmdLocal = idCmd - _idFolderMax;
ProcessVerb:
        {
            CMINVOKECOMMANDINFOEX ici;
            UINT_PTR idCmdSave;

            CopyInvokeInfo(&ici, pici);

            if (IS_INTRESOURCE(pici->lpVerb))
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmdLocal);

            // One of extension menu is selected.
            idCmdSave = (UINT_PTR)ici.lpVerb;
            UINT_PTR idCmd = 0;

            hres = HDXA_LetHandlerProcessCommandEx(_hdxa, &ici, &idCmd);
            if (SUCCEEDED(hres) && (idCmd == idCmdSave))
            {
                // hdxa failed to handle it
                hres = E_INVALIDARG;
            }
        }
    }
    else if (idCmd < _idDelayInvokeMax)
    {
        _Static_InvokeCommand((UINT)(idCmd-_idVerbMax));
    }
    else if (idCmd < _idFld2Max)
    {
        idCmdLocal = idCmd - _idDelayInvokeMax;
        goto ProcessCommand;
    }
    else
    {
        hres = E_INVALIDARG;
    }

    return hres;
}


STDMETHODIMP CDefFolderMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_INVALIDARG;
    UINT_PTR idCmdLocal;
    int i;

    if (!IS_INTRESOURCE(idCmd))
    {
        // This must be a string
        LPTSTR pCmd = (LPTSTR)idCmd;
        // BUGBUG raymondc GCS_UNICODE

        if (HDXA_GetCommandString(_hdxa, idCmd, uType, pwReserved, pszName, cchMax) == NOERROR)
        {
            return NOERROR;
        }

        // Convert the string into an ID
        for (i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
        {
            if (!lstrcmpi(pCmd, c_sDFMCmdInfo[i].pszCmd))
            {
                idCmdLocal = (UINT) c_sDFMCmdInfo[i].idDFMCmd;
                goto ProcessCommand;
            }
        }
        return E_INVALIDARG;
    }

    if (idCmd < _idStdMax)
    {
        idCmdLocal = idCmd;

        switch (uType)
        {
        case GCS_HELPTEXTA:
            // HACK: DCM commands are in the same order as SFV commands
            return(LoadStringA(HINST_THISDLL,
                (UINT) idCmdLocal + (UINT)(SFVIDM_FIRST + SFVIDS_MH_FIRST),
                (LPSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
            break;

        case GCS_HELPTEXTW:
            // HACK: DCM commands are in the same order as SFV commands
            return(LoadStringW(HINST_THISDLL,
                (UINT) idCmdLocal + (UINT)(SFVIDM_FIRST + SFVIDS_MH_FIRST),
                (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY);
            break;

        case GCS_VERBA:
        case GCS_VERBW:
            for (i = 0; i < ARRAYSIZE(c_sDFMCmdInfo); i++)
            {
                if (idCmdLocal == c_sDFMCmdInfo[i].idDefCmd)
                {
                    if (uType == GCS_VERBW)
                        SHTCharToUnicode(c_sDFMCmdInfo[i].pszCmd, (LPWSTR)pszName, cchMax);
                    else
                        SHTCharToAnsi(c_sDFMCmdInfo[i].pszCmd, (LPSTR)pszName, cchMax);
                    return NOERROR;
                }
            }

            return E_INVALIDARG;


        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            // BUGBUG: We should do something here, but I am too lazy

        default:
            return E_NOTIMPL;
        }
    } 
    else if (idCmd < _idFolderMax)
    {
        idCmdLocal = idCmd - _idStdMax;
ProcessCommand:
        if (!_pcmcb)
            return E_NOTIMPL;   // REVIEW: If no callback, how can idFolderMax be > 0?

        // This is a folder menu
        switch (uType)
        {
        case GCS_HELPTEXTA:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_GETHELPTEXT,
                      (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_HELPTEXTW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, DFM_GETHELPTEXTW,
                      (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_VALIDATECMD, idCmdLocal, 0);

        case GCS_VERBA:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_GETVERBA, (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        case GCS_VERBW:
            return _pcmcb->CallBack(_psf, _hwnd, _pdtobj,
                DFM_GETVERBW, (WPARAM)MAKELONG(idCmdLocal, cchMax), (LPARAM)pszName);

        default:
            return E_NOTIMPL;
        }
    }
    else if (idCmd < _idVerbMax)
    {
        idCmdLocal = idCmd - _idFolderMax;
        // One of extension menu is selected.
        hres = HDXA_GetCommandString(_hdxa, idCmdLocal, uType, pwReserved, pszName, cchMax);
    }
    else if (idCmd < _idDelayInvokeMax)
    {
        // menu extensions that are loaded at invoke time don't support this
    }
    else if (idCmd < _idFld2Max)
    {
        idCmdLocal = idCmd - _idDelayInvokeMax;
        goto ProcessCommand;
    }

    return hres;
}

STDMETHODIMP CDefFolderMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, 
                                           LPARAM lParam,LRESULT* plResult)
{
    IContextMenu3 *pcmItem;
    UINT uMsgFld;
    WPARAM wParamFld;       // map the folder call back params to these
    LPARAM lParamFld;
    UINT idCmd;
    UINT id; //temp var

    switch (uMsg) {
    case WM_MEASUREITEM:
        idCmd = GET_WM_COMMAND_ID(((MEASUREITEMSTRUCT *)lParam)->itemID, 0);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if ((_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)        
        {
            _MeasureItem((MEASUREITEMSTRUCT *)lParam);
            return S_OK;
        }
        
        uMsgFld = DFM_WM_MEASUREITEM;
        wParamFld = GetFldFirst(this);
        lParamFld = lParam;
        break;

    case WM_DRAWITEM:
        idCmd = GET_WM_COMMAND_ID(((LPDRAWITEMSTRUCT)lParam)->itemID, 0);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if ((_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)
        {
            _DrawItem((LPDRAWITEMSTRUCT)lParam);
            return S_OK;
        }

        uMsgFld = DFM_WM_DRAWITEM;
        wParamFld = GetFldFirst(this);
        lParamFld = lParam;
        break;

    case WM_INITMENUPOPUP:
        idCmd = GetMenuItemID((HMENU)wParam, 0);
        if (_bFindHack)
        {
            int i;
            HMENU hmenu = (HMENU)wParam;
            int cItems = GetMenuItemCount(hmenu);
            
            _bInitMenuPopup = TRUE;
            if (!_hdsaCustomInfo)
                _hdsaCustomInfo = DSA_Create(SIZEOF(SEARCHINFO), 1);

            if (_hdsaCustomInfo && cItems > 0)
            {
                // need to go bottom up because we may delete some items
                for (i=cItems-1; i >= 0; i--)
                {
                    MENUITEMINFO mii;
                    TCHAR        szMenuText[MAX_PATH];

                    mii.cbSize = SIZEOF(mii);
                    mii.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    mii.dwTypeData = szMenuText;
                    mii.cch = ARRAYSIZE(szMenuText);
                    
                    if (GetMenuItemInfo(hmenu, i, TRUE, &mii))
                    {
                        SEARCHINFO sinfo;
                        // static items already have correct dwItemData (pointer to SEARCHEXTDATA added in _Static_Add)
                        // we now have to change other find extension's dwItemData from having an index into the icon
                        // cache to pointer to SEARCHEXTDATA
                        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
                        id = mii.wID-_idCmdFirst;
                        if (!(_hdsaStatics && _idVerbMax <= id && id < _idDelayInvokeMax))
                        {
                            UINT   iIcon = (UINT) mii.dwItemData;
                            SEARCHEXTDATA *psed = (SEARCHEXTDATA *)LocalAlloc(LPTR, sizeof(SEARCHEXTDATA));
                            if (psed)
                            {
                                psed->iIcon = iIcon;
                                SHTCharToUnicode(szMenuText, psed->wszMenuText, ARRAYSIZE(psed->wszMenuText));
                            }
                            mii.fMask = MIIM_DATA | MIIM_TYPE;
                            mii.fType = MFT_OWNERDRAW;
                            mii.dwItemData = (DWORD_PTR)psed;

                            sinfo.psed = psed;
                            sinfo.idCmd = mii.wID;
                            if (DSA_AppendItem(_hdsaCustomInfo, &sinfo) == -1)
                            {
                                DeleteMenu(hmenu, i, MF_BYPOSITION);
                                if (psed)
                                    LocalFree(psed);
                            }
                            else
                                SetMenuItemInfo(hmenu, i, TRUE, &mii);
                        }
                    }
                }
            }
            else if (!_hdsaCustomInfo)
            {
                // we could not allocate space for _hdsaCustomInfo
                // delete all items because there will be no pointer hanging off dwItemData
                // so start | search will fault
                for (i=0; i < cItems; i++)
                    DeleteMenu(hmenu, i, MF_BYPOSITION);
            }
        }
        
        uMsgFld = DFM_WM_INITMENUPOPUP;
        wParamFld = wParam;
        lParamFld = GetFldFirst(this);
        break;

    case WM_MENUSELECT:
        idCmd = (UINT) LOWORD(wParam);
        // cannot use InRange because _idVerbMax can be equal to _idDelayInvokeMax
        id = idCmd-_idCmdFirst;
        if (_pSite && (_bInitMenuPopup || (_hdsaStatics && _idVerbMax <= id)) && id < _idDelayInvokeMax)
        {
            IShellBrowser *psb;

            if (SUCCEEDED(IUnknown_QueryService(_pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
            {
                MENUITEMINFO mii;

                mii.cbSize = SIZEOF(mii);
                mii.fMask = MIIM_DATA;
                mii.cch = 0; //just in case
                if (GetMenuItemInfo(_hmenu, idCmd, FALSE, &mii))
                {
                    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)mii.dwItemData;
                    psb->SetStatusTextSB(psed->wszHelpText);
                }
                psb->Release();
            }
        }
        return S_OK;
        
      
    case WM_MENUCHAR:
        if (_bFindHack && _hdsaCustomInfo)
        {
            int cItems = DSA_GetItemCount(_hdsaCustomInfo);
            
            for (int i=0; i < cItems; i++)
            {
                SEARCHINFO* psinfo = (SEARCHINFO*)DSA_GetItemPtr(_hdsaCustomInfo, i);
                ASSERT(psinfo);
                SEARCHEXTDATA* psed = psinfo->psed;
                
                if (psed)
                {
                    TCHAR  szMenu[MAX_PATH];

                    SHUnicodeToTChar(psed->wszMenuText, szMenu, ARRAYSIZE(szMenu));
                
                    if (_MenuCharMatch(szMenu,(TCHAR)LOWORD(wParam), FALSE))
                    {
                        if (plResult) 
                            *plResult = MAKELONG(GetMenuPosFromID((HMENU)lParam, psinfo->idCmd), MNC_EXECUTE);
                        return S_OK;
                    }                            
                }
            }
            if (plResult) 
                *plResult = MAKELONG(0, MNC_IGNORE);
                
            return S_FALSE;
        }
        else
            idCmd = GetMenuItemID((HMENU)lParam, 0);

        break;
        
    case WM_NEXTMENU:
        idCmd = GetMenuItemID((HMENU)lParam, 0);
        break;
    default:
        return E_FAIL;
    }

    // bias this down to the extension range (comes right after the folder range)

    idCmd -= _idCmdFirst + _idFolderMax;

    // Only forward along on IContextMenu3 as some shell extensions say they support
    // IContextMenu2, but fail and bring down the shell...
    if (SUCCEEDED(HDXA_FindByCommand(_hdxa, idCmd, IID_PPV_ARG(IContextMenu3, &pcmItem))))
    {
        HRESULT hres;
        hres = pcmItem->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
        pcmItem->Release();
        return hres;
    }

    // redirect to the folder callback
    if (_pcmcb)
        return _pcmcb->CallBack(_psf, _hwnd, _pdtobj, uMsgFld, wParamFld, lParamFld);

    return E_FAIL;
}

STDMETHODIMP CDefFolderMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

 
STDMETHODIMP CDefFolderMenu::GetSite(REFIID riid, void** ppvSite)
{
    if (_pSite)
        return _pSite->QueryInterface(riid, ppvSite);
    else
    {
        *ppvSite = NULL;
        return E_FAIL; // OLE spec says to return E_FAIL if there is no site
    }
}

STDMETHODIMP CDefFolderMenu::SetSite(IUnknown* pUnk)
{
    IUnknown_Set(&_pSite, pUnk);

    IUnknown_SetSite(_pcmcb, pUnk);

    for (int icmi = 0; icmi < DSA_GetItemCount(_hdxa); icmi++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(_hdxa, icmi);
        // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
        if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_LIMITEDQI))
            IUnknown_SetSite((IUnknown*)pcmi->pcm, pUnk);
    }
    return NOERROR;
}

STDMETHODIMP CDefFolderMenu::QueryService(REFGUID guidService, REFIID riid, void ** ppvObj)
{
    return IUnknown_QueryService(_pSite, guidService, riid, ppvObj);
}

STDMETHODIMP CDefFolderMenu::GetSearchGUID(GUID *pGuid)
{
    HRESULT hres = E_FAIL;
    
    if (_iStaticInvoked != -1)
    {
        PSTATICITEMINFO psii = (PSTATICITEMINFO)DSA_GetItemPtr(_hdsaStatics, _iStaticInvoked);

        if (psii)
        {
            *pGuid = psii->guidSearch;
            hres = S_OK;
        }
    }

    return hres;
}

//=============================================================================
// HDXA stuff
//=============================================================================
//
//  This function enumerate all the context menu handlers and let them
// append menuitems. Each context menu handler will create an object
// which support IContextMenu interface. We call QueryContextMenu()
// member function of all those IContextMenu object to let them append
// menuitems. For each IContextMenu object, we create ContextMenuInfo
// struct and append it to hdxa (which is a dynamic array of ContextMenuInfo).
//
//  The caller will release all those IContextMenu objects, by calling
// its Release() member function.
//
// Arguments:
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  pdata           -- Specifies the selected items (files)
//  hkeyShellEx     -- Specifies the reg.dat class we should enumurate handlers
//  hkeyProgID      -- Specifies the program identifier of the selected file/directory
//  pszHandlerKey   -- Specifies the reg.dat key to the handler list
//  pidlFolder      -- Specifies the folder (drop target)
//  hmenu           -- Specifies the menu to be modified
//  uInsert         -- Specifies the position to be insert menuitems
//  idCmdFirst      -- Specifies the first menuitem ID to be used
//  idCmdLast       -- Specifies the last menuitem ID to be used
//
// Returns:
//  The first menuitem ID which is not used.
//
// History:
//  02-25-93 SatoNa     Created
//
//  06-30-97 lAmadio    Modified to add ID mapping support.

UINT HDXA_AppendMenuItems(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        HMENU hmenu, UINT uInsert,
                        UINT  idCmdFirst,  UINT idCmdLast,
                        UINT fFlags,
                        HDCA hdca)
{
    QCMINFO qcm = {hmenu,uInsert,idCmdFirst,idCmdLast,NULL};
    return HDXA_AppendMenuItems2(hdxa, pdtobj,nKeys,ahkeyClsKeys,pidlFolder,&qcm,fFlags,hdca,NULL);
}

UINT HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        QCMINFO* pqcm,
                        UINT fFlags,
                        HDCA hdca,
                        IUnknown* pSite)
{
    int idca;
    const UINT idCmdBase = pqcm->idCmdFirst;
    UINT idCmdFirst = pqcm->idCmdFirst;
    ASSERT(pqcm != NULL);

    // Apparently, somebody has already called into here with this object.  We
    // need to keep the ID ranges separate, so we'll put the new ones at the
    // end.
    // BUGBUG: If QueryContextMenu is called too many times, we will run out of
    // ID range and not add anything.  We could try storing the information
    // used to create each pcm (HKEY, GUID, and fFlags) and reuse some of them,
    // but then we would have to worry about what if the number of commands
    // grows and other details; this is just not worth the effort since
    // probably nobody will ever have a problem.  The rule of thumb is to
    // create an IContextMenu, do the QueryContextMenu and InvokeCommand, and
    // then Release it.
    idca = DSA_GetItemCount(hdxa);
    if (idca > 0)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, idca-1);
        idCmdFirst += pcmi->idCmdMax;
    }

    //
    // Note that we need to reverse the order because each extension
    // will intert menuitems "above" uInsert.
    //
    for (idca = DCA_GetItemCount(hdca) - 1; idca >= 0; idca--)
    {
        int nCurKey;
        IShellExtInit *psei = NULL;
        IContextMenu *pcm = NULL;
        IObjectWithSite* pows = NULL;

        TCHAR szCLSID[GUIDSTR_MAX];
        TCHAR szRegKey[GUIDSTR_MAX + 40];
        DWORD dwType;
        DWORD dwSize;
        DWORD dwExtType;

        const CLSID* pclsid = DCA_GetItem(hdca, idca);
        SHStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID));
        //
        // Let's avoid creating an instance (loading the DLL) when:
        //  1. fFlags has CMF_DEFAULTONLY and
        //  2. CLSID\clsid\MayChangeDefault does not exist
        //
        if (fFlags & CMF_DEFAULTONLY)
        {
            if (pclsid && (*pclsid) != CLSID_ShellFileDefExt)
            {
                wsprintf(szRegKey, TEXT("CLSID\\%s\\shellex\\MayChangeDefaultMenu"), szCLSID);

                if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegKey, NULL, NULL) != ERROR_SUCCESS)
                {
                    DebugMsg(TF_MENU, TEXT("HDXA_AppendMenuItems skipping %s"), szCLSID);
                    continue;
                }
            }
        }

        for (nCurKey = 0; nCurKey < (int)nKeys; nCurKey++)
        {
            UINT citems;
            DWORD dwCompat = SHGetObjectCompatFlags(NULL, pclsid);

            if (!psei && FAILED(DCA_CreateInstance(hdca, idca, IID_PPV_ARG(IShellExtInit, &psei))))
                break;

            // Try all the class keys in order
            HRESULT hres = psei->Initialize(pidlFolder, pdtobj, ahkeyClsKeys[nCurKey]);
            if (FAILED(hres))
            {
                continue;
            }

            if (!(dwCompat & OBJCOMPATF_CTXMENU_LIMITEDQI))
                IUnknown_SetSite((IUnknown *)psei, pSite);

            // Only get the pcm after initializing
            if (!pcm && FAILED(psei->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm))))
            {
                continue;   // break?
            }
            
            wsprintf(szRegKey, TEXT("CLSID\\%s"), szCLSID);
            dwSize = SIZEOF(DWORD);

            if (SHGetValue(HKEY_CLASSES_ROOT, szRegKey, TEXT("flags"),&dwType, (BYTE*)&dwExtType, &dwSize) == ERROR_SUCCESS &&
                dwType == REG_DWORD &&
                pqcm->pIdMap != NULL &&
                dwExtType < pqcm->pIdMap->nMaxIds)
            {
                //Explanation:
                //Here we are trying to add a context menu extension to an already 
                //existing menu, owned by the sister object of DefView. We used the callback
                //to get a list of extension "types" and their place withing the menu, relative
                //to IDs that the sister object inserted already. That object also told us 
                //where to put extensions, before or after the ID. Since they are IDs and not
                //positions, we have to convert using GetMenuPosFromID.
                hres = pcm->QueryContextMenu(
                    pqcm->hmenu, 
                    GetMenuPosFromID(pqcm->hmenu,pqcm->pIdMap->pIdList[dwExtType].id) +
                    ((pqcm->pIdMap->pIdList[dwExtType].fFlags & QCMINFO_PLACE_AFTER)? 1: 0),  
                    idCmdFirst, 
                    pqcm->idCmdLast, fFlags);
            }
            else
                hres = pcm->QueryContextMenu(pqcm->hmenu, pqcm->indexMenu, idCmdFirst, pqcm->idCmdLast, fFlags);

            if (0 == pqcm->indexMenu && (0 == GetMenuDefaultItem(pqcm->hmenu, TRUE, 0)))
                pqcm->indexMenu++;
                
            citems = HRESULT_CODE(hres);

            if (SUCCEEDED(hres) && citems)
            {
                ContextMenuInfo cmi;
                cmi.pcm = pcm;
                cmi.dwCompat = dwCompat;
                cmi.idCmdFirst = idCmdFirst - idCmdBase;
                cmi.idCmdMax   = cmi.idCmdFirst + citems;

                if (DSA_AppendItem(hdxa, &cmi) == -1)
                {
                    // There is no "clean" way to remove menu items, so
                    // we should check the add to the DSA before adding the
                    // menu items
                    DebugMsg(DM_ERROR, TEXT("filemenu.c ERROR: DSA_GetItemPtr failed (memory overflow)"));
                }
                else
                {
                    pcm->AddRef();
                }
                idCmdFirst += citems;

                FullDebugMsg(TF_MENU, TEXT("HDXA_Append: %d, %d"), idCmdFirst, citems);

                //
                //  keep going if it is our internal handler
                //
                if ((*(CLSID*)DCA_GetItem(hdca, idca)) != CLSID_ShellFileDefExt)
                    break;      // not out handler stop

                pcm->Release();
                pcm = NULL;

                psei->Release();
                psei = NULL;

                continue;       // next hkey
            }
        }

        if (pcm)
            pcm->Release();

        if (psei)
            psei->Release();
    }

    return idCmdFirst;
}


//
// Function:    HDXA_LetHandlerProcessCommandEx, public (not exported)
//
//  This function is called after the user select one of add-in menu items.
// This function calls IncokeCommand method of corresponding context menu
// object.
//
//  hdxa            -- Handler of the dynamic ContextMenuInfo struct array
//  idCmd           -- Specifies the menu item ID
//  hwndParent      -- Specifies the parent window.
//  pszWorkingDir   -- Specifies the working directory.
//
// Returns:
//  IDCMD_PROCESSED, if InvokeCommand method is called; idCmd, otherwise
//
// History:
//  03-03-93 SatoNa     Created
//
HRESULT HDXA_LetHandlerProcessCommandEx(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici, UINT_PTR * pidCmd)
{
    HRESULT hr = S_OK;
    UINT_PTR idCmd;

    if (!pidCmd)
        pidCmd = &idCmd;
        
    *pidCmd = (UINT_PTR)pici->lpVerb;

    //
    // One of add-in menuitems is selected. Let the context
    // menu handler process it.
    //
    for (int icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);
        //
        // Check if it is for this context menu handler.
        //
        // Notes: We can't use InRange macro because idCmdFirst might
        //  be equal to idCmdLast.
        // if (InRange(*pidCmd, pcmi->idCmdFirst, pcmi->idCmdMax-1))
        if (!IS_INTRESOURCE(pici->lpVerb))
        {
            //
            //  some ctx menu extension always succeed regardless
            //  if it is theirs or not.  better to never pass them a string
            //
            if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_NOVERBS))
            {
                hr = pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)pici);
                if (SUCCEEDED(hr))
                {
                    *pidCmd = IDCMD_PROCESSED;
                    break;
                }
            }
            else
                hr = E_FAIL;
        }
        else if ((*pidCmd >= pcmi->idCmdFirst) && (*pidCmd < pcmi->idCmdMax))
        {
            //
            // Yes, it is. Let it handle this menuitem.
            //
            CMINVOKECOMMANDINFOEX ici;

            CopyInvokeInfo(&ici, (CMINVOKECOMMANDINFO *)pici);

            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(*pidCmd - pcmi->idCmdFirst);

#ifdef DEBUG
#ifdef SZKEYDEBUG
            // The keydebug data is not set anywhere causes beeps...
            DebugMsg(TF_MENU, TEXT("HDXA_LetHandleProcessCommand *pidCmd=%d %s:(%d,%d)"),
                     *pidCmd, pcmi->szKeyDebug, pcmi->idCmdFirst, pcmi->idCmdMax);
#else
            DebugMsg(TF_MENU, TEXT("HDXA_LetHandleProcessCommand *pidCmd=%d:(%d,%d)"),
                     *pidCmd, pcmi->idCmdFirst, pcmi->idCmdMax);
#endif
#endif

            if (SUCCEEDED(pcmi->pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici)))
            {
                *pidCmd = IDCMD_PROCESSED;
            }
            break;
        }
    }

    // It's OK if (idCmd != IDCMD_PROCESSED) because some callers will try to use several
    // IContextMenu implementations in order to get the IContextMenu for the selected items,
    // the IContextMenu for the background, etc.  CBackgrndMenu::InvokeCommand() does this.
    // -BryanSt (04/29/1999)
    return hr;
}


HRESULT HDXA_GetCommandString(HDXA hdxa, UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_INVALIDARG;
    LPTSTR pCmd = (LPTSTR)idCmd;

    if (!hdxa)
        return E_INVALIDARG;

    //
    // One of add-in menuitems is selected. Let the context
    // menu handler process it.
    //
    for (int icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
    {
        ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);

        if (!IS_INTRESOURCE(idCmd))
        {
            // This must be a string command; see if this handler wants it
            if (pcmi->pcm->GetCommandString(idCmd, uType,
                                            pwReserved, pszName, cchMax) == NOERROR)
            {
                return NOERROR;
            }
        }
        //
        // Check if it is for this context menu handler.
        //
        // Notes: We can't use InRange macro because idCmdFirst might
        //  be equal to idCmdLast.
        // if (InRange(idCmd, pcmi->idCmdFirst, pcmi->idCmdMax-1))
        else if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
        {
            //
            // Yes, it is. Let it handle this menuitem.
            //
            hres = pcmi->pcm->GetCommandString(idCmd-pcmi->idCmdFirst, uType, pwReserved, pszName, cchMax);
            break;
        }
    }

    return hres;
}

HRESULT HDXA_FindByCommand(HDXA hdxa, UINT idCmd, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;    // bug nt power toy does not properly null out in error cases...

    if (hdxa)
    {
        for (int icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);

            if (idCmd >= pcmi->idCmdFirst && idCmd < pcmi->idCmdMax)
            {
                // APPCOMPAT: PGP50 can only be QIed for IContextMenu, IShellExtInit, and IUnknown.
                if (!(pcmi->dwCompat & OBJCOMPATF_CTXMENU_LIMITEDQI))
                    hr = pcmi->pcm->QueryInterface(riid, ppv);
                else
                    hr = E_FAIL;
                break;
            }
        }
    }
    return hr;
}

//
// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo,
//
void HDXA_DeleteAll(HDXA hdxa)
{
    if (hdxa)
    {
        //  Release all the IContextMenu objects, then destroy the DSA.
        for (int icmi = 0; icmi < DSA_GetItemCount(hdxa); icmi++)
        {
            ContextMenuInfo *pcmi = (ContextMenuInfo *)DSA_GetItemPtr(hdxa, icmi);
            IContextMenu *pcm = pcmi->pcm;
            if (pcm)
            {
                pcm->Release();
            }
        }
        DSA_DeleteAllItems(hdxa);
    }
}

// This function releases all the IContextMenu objects in the dynamic
// array of ContextMenuInfo, then destroys the dynamic array.

void HDXA_Destroy(HDXA hdxa)
{
    if (hdxa)
    {
        HDXA_DeleteAll(hdxa);
        DSA_Destroy(hdxa);
    }
}


//=============================================================================
// class CContextMenuCBImpl
//=============================================================================


class CContextMenuCBImpl : public IContextMenuCB 
{
public:
    CContextMenuCBImpl(LPFNDFMCALLBACK lpfn) : m_pfn(lpfn), m_cRef(1) {}

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv) 
    {
        static const QITAB qit[] = {
            QITABENT(CContextMenuCBImpl, IContextMenuCB), // IID_IContextMenuCB
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    STDMETHOD_(ULONG,AddRef)() 
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHOD_(ULONG,Release)() 
    {
        if (InterlockedDecrement(&m_cRef)) 
            return m_cRef;

        delete this;
        return 0;
    }

    // IContextMenuCB
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return m_pfn ? m_pfn(psf, hwnd, pdtobj, uMsg, wParam, lParam) : E_FAIL;
    }

private:
    LPFNDFMCALLBACK m_pfn;
    LONG m_cRef;
};


//=============================================================================
// CDefFolderMenu_Create methods
//=============================================================================

STDAPI CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                                UINT cidl, LPCITEMIDLIST *apidl,
                                IShellFolder *psf, IContextMenuCB *pcmcb, 
                                UINT nKeys, const HKEY *ahkeyClsKeys, 
                                IContextMenu **ppcm)
{
    HRESULT hres = E_OUTOFMEMORY;

    IDLData_InitializeClipboardFormats();

    CDefFolderMenu *pmenu = new CDefFolderMenu(nKeys);
    if (pmenu)
    {
        pmenu->_hwnd = hwnd;
        pmenu->_pcmcb = pcmcb;
        pcmcb->AddRef();

        pmenu->_psf = psf;
        ASSERT(pmenu->_pSite == NULL);
        pmenu->_pidlFolder = ILClone(pidlFolder);
        if (psf)
            psf->AddRef();

        if (cidl)
        {
            hres = psf->GetUIObjectOf(hwnd, cidl, apidl, IID_IDataObject, 
                NULL, (void **)&pmenu->_pdtobj);
        }
        else
        {
            hres = NOERROR;
        }

        pmenu->_hdxa = HDXA_Create();
        if (pmenu->_hdxa)
        {
            if (SUCCEEDED(hres))
            {
                UINT i;
                for (i = 0; i < nKeys; ++i)
                {
                    if (ahkeyClsKeys[i])
                    {
                        // Make a copy of the key for menu's use
                        if (RegOpenKeyEx(ahkeyClsKeys[i], NULL, 0L, MAXIMUM_ALLOWED,
                                &pmenu->_hkeyClsKeys[pmenu->_nKeys]) == ERROR_SUCCESS)
                        {
                            pmenu->_nKeys++;
                        }
                    }
                }
                *ppcm = SAFECAST(pmenu, IContextMenu *);
            }
        }

        if (SUCCEEDED(hres))
        {
            hres = pcmcb->CallBack(psf, hwnd, NULL, DFM_ADDREF, 0, 0);
            if (hres == E_NOTIMPL)
            {
                // I guess there was no initialization to do
                hres = NOERROR;
            }
        }
        if (FAILED(hres))
        {
            delete pmenu;
        }
    }
    return hres;
}

STDAPI CDefFolderMenu_CreateEx(LPCITEMIDLIST pidlFolder,
                               HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                               IShellFolder *psf, IContextMenuCB *pcmcb, 
                               HKEY hkeyProgID, HKEY hkeyBaseProgID,
                               IContextMenu **ppcm)
{
    HKEY ahkeyClsKeys[2];

    ahkeyClsKeys[0] = hkeyProgID;
    ahkeyClsKeys[1] = hkeyBaseProgID;

    // Note that Create2 will remove NULL and duplicate HKEY's
    return CDefFolderMenu_Create2Ex(pidlFolder, hwnd, cidl, apidl, psf, pcmcb, ARRAYSIZE(ahkeyClsKeys), ahkeyClsKeys, ppcm);
}


//
// old style CDefFolderMenu_Create and CDefFolderMenu_Create2
//

STDAPI CDefFolderMenu_Create(LPCITEMIDLIST pidlFolder,
                             HWND hwndOwner,
                             UINT cidl, LPCITEMIDLIST * apidl,
                             IShellFolder *psf,
                             LPFNDFMCALLBACK lpfn,
                             HKEY hkeyProgID, HKEY hkeyBaseProgID,
                             IContextMenu **ppcm)
{
    IContextMenuCB *pcmcb = new CContextMenuCBImpl(lpfn);
    if (!pcmcb) 
    {
        *ppcm = NULL;
        return E_OUTOFMEMORY;
    }

    HRESULT hr = CDefFolderMenu_CreateEx(pidlFolder, hwndOwner, cidl, apidl, psf, pcmcb, 
                                 hkeyProgID, hkeyBaseProgID, ppcm);
    pcmcb->Release();
    return hr;
}

STDAPI CDefFolderMenu_Create2(LPCITEMIDLIST pidlFolder, HWND hwnd,
                             UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, LPFNDFMCALLBACK lpfn,
                             UINT nKeys, const HKEY *ahkeyClsKeys,
                             IContextMenu **ppcm)
{
    IContextMenuCB *pcmcb = new CContextMenuCBImpl(lpfn);
    if (!pcmcb) 
    {
        *ppcm = NULL;
        return E_OUTOFMEMORY;
    }

    HRESULT hr = CDefFolderMenu_Create2Ex(pidlFolder, hwnd, cidl, apidl, psf, pcmcb, 
                                  nKeys, ahkeyClsKeys, ppcm);
    pcmcb->Release();
    return hr;
}

#define CXIMAGEGAP      6

void CDefFolderMenu::_DrawItem(DRAWITEMSTRUCT *pdi)
{
    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)pdi->itemData;
    if (psed)
    {
        TCHAR szMenuText[MAX_PATH];
        SHUnicodeToTChar(psed->wszMenuText, szMenuText, ARRAYSIZE(szMenuText));
        DrawMenuItem(pdi, szMenuText, psed->iIcon);
    }        
}

LRESULT CDefFolderMenu::_MeasureItem(MEASUREITEMSTRUCT *pmi)
{
    SEARCHEXTDATA *psed = (SEARCHEXTDATA *)pmi->itemData;
    if (psed)
    {
        TCHAR szMenuText[MAX_PATH];
        SHUnicodeToTChar(psed->wszMenuText, szMenuText, ARRAYSIZE(szMenuText));
        return MeasureMenuItem(pmi, szMenuText);
    }
    return FALSE;        
}
