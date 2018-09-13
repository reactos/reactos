// todo:
//
//      delayed keyboard selection so keyboard navigation does not generate a sel change
//
//      Partial expanded nodes in the tree "Tree Down" (TVIS_EXPANDPARTIAL) for net cases
//
//      Programbility:
//          notifies - sel changed, node expanded, verb executed, etc.
//          cmds - do verb, get item, etc.
//      review chrisny:  consolidate state managing stuff for tree control in all areas
//      review chrisny:  more error handling stuff.

#include "priv.h"
#include <shlobjp.h>    
#include "nsc.h"
#include "resource.h"
#include "subsmgr.h"
#include "favorite.h" //for IsSubscribed()
#include "chanmgr.h"
#include "chanmgrp.h"
#include <mstask.h>    // TASK_TRIGGER
#include "dpastuff.h"
#include "nicotask.h"
#include "uemapp.h"
#include <findhlp.h>

#include <mluisupp.h>

#define TF_NSC      0x00002000

#define ID_NSC_SUBCLASS 359
#define ID_NSCTREE  (DWORD)'NSC'

#ifndef UNIX
#define DEFAULT_PATHSTR "C:\\"
#else
#define DEFAULT_PATHSTR "/"
#endif

#define LOGOGAP 2   // all kinds of things 
#define DYITEM  17
#define DXYFRAMESEL 1                             
const DEFAULTORDERPOSITION = 32000;

HRESULT CheckForExpandOnce( HWND hwndTree, HTREEITEM hti );

// from util.cpp
// same guid as in bandisf.cpp
// {F47162A0-C18F-11d0-A3A5-00C04FD706EC}
static const GUID TOID_ExtractImage = { 0xf47162a0, 0xc18f, 0x11d0, { 0xa3, 0xa5, 0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec } };
//from nicotask.cpp
EXTERN_C const GUID TASKID_IconExtraction; // = { 0xeb30900c, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };


BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL);

COLORREF g_clrBk = 0;

typedef struct
{
    DWORD   iIcon     : 12;
    DWORD   iOpenIcon : 12;
    DWORD   nFlags    : 4;
    DWORD   nMagic    : 4;
} NSC_ICONCALLBACKINFO;

//if you don't remove the selection, treeview will expand everything below the current selection
void TreeView_DeleteAllItemsQuickly(HWND hwnd)
{
    TreeView_SelectItem(hwnd, NULL);
    TreeView_DeleteAllItems(hwnd);
}

BOOL IsParentOfItem(HWND hwnd, HTREEITEM htiParent, HTREEITEM htiChild)
{
    for (HTREEITEM hti = htiChild; (hti != TVI_ROOT) && (hti != NULL); hti = TreeView_GetParent(hwnd, hti))
        if (hti == htiParent)
            return TRUE;

    return FALSE;
}

STDAPI CNscTree_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CNscTree * pncst = new CNscTree();
    if (pncst == NULL)
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppunk = SAFECAST(pncst, INSCTree *);
        hr = S_OK;
    }

    return hr;
}


INSCTree *CNscTree_CreateInstance(void)
{
    CNscTree * pncst = new CNscTree();
    if (pncst)
        return SAFECAST(pncst, INSCTree *);
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////

CNscTree::CNscTree() : _cRef(1), _iDragSrc(-1), _iDragDest(-1), _fOnline(!SHIsGlobalOffline())
{
    // This object is a COM object so it will always be on the heap.
    // ASSERT that our member variables were zero initialized.
    ASSERT(!_fInitialized);

    _ulSortCol = _ulDisplayCol = (ULONG)-1;

    // Enable the notifications from wininet that tell us when to gray items 
    // or update a pinned glyph
    _inetNotify.Enable();
}

CNscTree::~CNscTree()
{
    Pidl_Set(&_pidlSelected, NULL);
    ATOMICRELEASE(_pFilter);

    // This needs to be destroyed or we leak the icon handle.
    if (_hicoPinned) 
    {
        DestroyIcon(_hicoPinned);
    }
}

HRESULT CNscTree::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CNscTree, IShellChangeNotify),         // IID_IShellChangeNotify
        QITABENT(CNscTree, INSCTree),                   // IID_INSCTree
        QITABENT(CNscTree, IShellFavoritesNameSpace),   // IID_IShellFavoritesNameSpace
        QITABENT(CNscTree, IWinEventHandler),           // IID_IWinEventHandler
        QITABENT(CNscTree, IDropTarget),                // IID_IDropTarget
        QITABENT(CNscTree, IObjectWithSite),            // IID_IObjectWithSite
        QITABENT(CNscTree, IShellBrowser),              // IID_IShellBrowser
        QITABENT(CNscTree, IShellFolderFilterSite),     // IID_IShellFolderFilterSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CNscTree::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CNscTree::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

void CNscTree::_ReleaseCachedShellFolder()
{
    ATOMICRELEASE(_psfCache);
    ATOMICRELEASE(_psf2Cache);
    _ulSortCol = _ulDisplayCol = (ULONG)-1;
    _htiCache = NULL;
}

#ifdef DEBUG
void CNscTree::TraceHTREE(HTREEITEM hti, LPCTSTR pszDebugMsg)
{
    TCHAR szDebug[MAX_PATH] = TEXT("Root");

    if (hti != TVI_ROOT && hti)
    {
        TVITEM tvi;
        tvi.mask = TVIF_TEXT | TVIF_HANDLE;
        tvi.hItem = hti;
        tvi.pszText = szDebug;
        tvi.cchTextMax = MAX_PATH;
        TreeView_GetItem(_hwndTree, &tvi);
    }

    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebug);
}

void CNscTree::TracePIDL(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg)
{
    TCHAR szDebugName[MAX_URL_STRING] = TEXT("Desktop");
    STRRET str;
    if (_psfCache &&
        SUCCEEDED(_psfCache->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
    {
        StrRetToBuf(&str, pidl, szDebugName, ARRAYSIZE(szDebugName));
    }
    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebugName);
}

void CNscTree::TracePIDLAbs(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg)
{
    TCHAR szDebugName[MAX_URL_STRING] = TEXT("Desktop");
    IEGetDisplayName(pidl, szDebugName, SHGDN_FORPARSING);
    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebugName);
}
#endif

/*****************************************************\
    DESCRIPTION:
        We want to unsubclass/subclass everytime we
    change roots so we get the correct notifications
    for everything in that subtree of the shell
    name space.
\*****************************************************/
void CNscTree::_SubClass(LPCITEMIDLIST pidlRoot)
{
    LPITEMIDLIST pidlToFree = NULL;
    
    if (NULL == pidlRoot)       // (NULL == CSIDL_DESKTOP)
    {
        SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, (LPITEMIDLIST *) &pidlRoot);
        pidlToFree = (LPITEMIDLIST) pidlRoot;
    }
        
    // It's necessary 
    if (EVAL(!_fSubClassed && pidlRoot))
    {
        if (_SubclassWindow(_hwndTree))
        {
            _RegisterWindow(_hwndTree, pidlRoot,
                SHCNE_DRIVEADD|SHCNE_CREATE|SHCNE_MKDIR|SHCNE_DRIVEREMOVED|
                SHCNE_DELETE|SHCNE_RMDIR|SHCNE_RENAMEITEM|SHCNE_RENAMEFOLDER|
                SHCNE_MEDIAINSERTED|SHCNE_MEDIAREMOVED|SHCNE_NETUNSHARE|SHCNE_NETSHARE|
                SHCNE_UPDATEITEM|SHCNE_UPDATEIMAGE|SHCNE_ASSOCCHANGED|
                SHCNE_UPDATEDIR | SHCNE_EXTENDED_EVENT,
                ((_mode & MODE_HISTORY) ? SHCNRF_ShellLevel : SHCNRF_ShellLevel | SHCNRF_InterruptLevel));
        }

        ASSERT(_hwndTree);
        _fSubClassed = SetWindowSubclass(_hwndTree, _SubClassTreeWndProc, 
            ID_NSCTREE, (DWORD_PTR)this);
    }

    if (pidlToFree) // Did we have to alloc our own pidl?
        ILFree(pidlToFree); // Yes.
}


/*****************************************************\
    DESCRIPTION:
        We want to unsubclass/subclass everytime we
    change roots so we get the correct notifications
    for everything in that subtree of the shell
    name space.
\*****************************************************/
void CNscTree::_UnSubClass(void)
{
    if (_fSubClassed)
    {
        _fSubClassed = FALSE;
        RemoveWindowSubclass(_hwndTree, _SubClassTreeWndProc, ID_NSCTREE);
        _UnregisterWindow(_hwndTree);
        _UnsubclassWindow(_hwndTree);
    }
}


void CNscTree::_ReleasePidls(void)
{
    Pidl_Set(&_pidlRoot, NULL);
}


HRESULT CNscTree::ShowWindow(BOOL fShow)
{
    if (fShow)
        _TvOnShow();
    else
        _TvOnHide();

    return S_OK;
}


HRESULT CNscTree::SetSite(IUnknown *punkSite)
{
    if (!punkSite)
    {
        // We need to prepare to go away and squirel
        // away the currently selected pidl(s) because
        // the caller may call INSCTree::GetSelectedItem()
        // after the tree is gone.
        _OnWindowCleanup();
    }

    return CObjectWithSite::SetSite(punkSite);
}

EXTERN_C static const GUID TASKID_IconExtraction;

HRESULT CNscTree::_OnWindowCleanup(void)
{
    // Squirel away the selected pidl in case the caller asks for it after the
    // treeview is gone.
    if (!_fIsSelectionCached)
    {
        _fIsSelectionCached = TRUE;
        Pidl_Set(&_pidlSelected, NULL);
        GetSelectedItem(&_pidlSelected, 0);
    }

    _fClosing = TRUE;
    if (_pTaskScheduler)
        _pTaskScheduler->RemoveTasks(TASKID_IconExtraction, ITSAT_DEFAULT_LPARAM, FALSE);
    ATOMICRELEASE(_pTaskScheduler);

    _TvOnHide();
    ASSERT(IsWindow(_hwndTree));      // window not valid, we need to know about this
    SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0L);
    TreeView_DeleteAllItemsQuickly(_hwndTree);

    _UnSubClass();
    _ReleasePidls();

    ASSERT(_pidlRoot == NULL);
    _ReleaseCachedShellFolder();

    return S_OK;
}

ITEMINFO *CNscTree::_GetTreeItemInfo(HTREEITEM hti)
{
    TV_ITEM tvi;
    
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hti;
    if (!TreeView_GetItem(_hwndTree, &tvi))
        return NULL;
    return (ITEMINFO *)tvi.lParam;
}

PORDERITEM CNscTree::_GetTreeOrderItem(HTREEITEM hti)
{
    ITEMINFO *pii = _GetTreeItemInfo(hti);
    return pii ? pii->poi : NULL;
}

// builds a fully qualified IDLIST from a given tree node by walking up the tree
// be sure to free this when you are done!

LPITEMIDLIST CNscTree::_GetFullIDList(HTREEITEM hti)
{
    LPITEMIDLIST pidl, pidlT = NULL;

    if ((hti == TVI_ROOT) || (hti == NULL)) // evil root
    {
        pidlT = ILClone(_pidlRoot);
        return pidlT;
    }
    // now lets get the information about the item
    PORDERITEM poi = _GetTreeOrderItem(hti);
    if (!poi)
    {
        return NULL;
    }
    
    pidl = ILClone(poi->pidl);
    if (pidl && _pidlRoot)
    {
        while ((hti = TreeView_GetParent(_hwndTree, hti)))
        {
            poi = _GetTreeOrderItem(hti);
            if (!poi)
                return pidl;   // will assume I screwed up...
            
            if (poi->pidl)
                pidlT = ILCombine(poi->pidl, pidl);
            else 
                pidlT = NULL;
            
            ILFree(pidl);
            pidl = pidlT;
            if (pidl == NULL)
                break;          // outta memory
        }
        if (pidl) 
        {
            // MODE_NORMAL has the pidl root in the tree
            if (_mode != MODE_NORMAL)
            {
                pidlT = ILCombine(_pidlRoot, pidl);    // gotta get the silent root
                ILFree(pidl);
            }
            else
                pidlT = pidl;
        }
    }
    return pidlT;
}


BOOL _IsItemFileSystem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    DWORD dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    HRESULT hr = psf->GetAttributesOf(1, &pidl, &dwAttributes);
    return SUCCEEDED(hr) && ((dwAttributes & (SFGAO_FOLDER | SFGAO_FILESYSTEM)) == (SFGAO_FOLDER | SFGAO_FILESYSTEM));
}

// NOTE: takes ownership of pidl 

HTREEITEM CNscTree::_AddItemToTree(HTREEITEM htiParent, LPCITEMIDLIST pidl, 
                                   int cChildren, int iPos, HTREEITEM htiAfter, /* = TVI_LAST*/
                                   BOOL fCheckForDups, /* = TRUE */ BOOL fMarked /*= FALSE */)
{
    HTREEITEM htiRet = NULL;

    BOOL fCached;

    // So we need to cached the shell folder of the parent item. But, this is a little interesting:
    if (_mode == MODE_NORMAL && htiParent == TVI_ROOT)
    {
        // In "Normal" mode, or "Display root in NSC" mode, there is only 1 item that is parented to
        // TVI_ROOT. So when we do an _AddItemToTree, we need the shell folder that contains _pidlRoot or
        // the Parent of TVI_ROOT.
        fCached = (NULL != _CacheParentShellFolder(htiParent, NULL));
    }
    else
    {
        // But, in the "Favorites, Control or History" if htiParent is TVI_ROOT, then we are not adding _pidlRoot,
        // so we actually need the folder that IS TVI_ROOT.
        fCached = _CacheShellFolder(htiParent);
    }

    
    if (fCached)
    {
        PORDERITEM poi = OrderItem_Create((LPITEMIDLIST)pidl, iPos);
        if (poi)
        {
            ITEMINFO *pii = (ITEMINFO *)LocalAlloc(LPTR, sizeof(*pii));
            if (pii)
            {
                pii->poi = poi;

                // For the normal case, we need a relative pidl for this add, but the lParam needs to have a full
                // pidl (This is so that arbitrary mounting works, as well as desktop case).
                if (_mode == MODE_NORMAL && htiParent == TVI_ROOT)
                {
                    pidl = ILFindLastID(pidl);
                }

                if (!fCheckForDups || (NULL == (htiRet = _FindChild(_psfCache, htiParent, pidl))))
                {
                    TV_INSERTSTRUCT tii;
                    // Initialize item to add with callback for everything
                    tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
                    tii.hParent = htiParent;
                    tii.hInsertAfter = htiAfter;
                    tii.item.iImage = I_IMAGECALLBACK;
                    tii.item.iSelectedImage = I_IMAGECALLBACK;
                    tii.item.pszText = LPSTR_TEXTCALLBACK;
                    tii.item.cChildren = cChildren;
                    tii.item.lParam = (LPARAM)pii;
                    tii.item.stateMask = TVIS_STATEIMAGEMASK;
                    tii.item.state = (fMarked ? NSC_TVIS_MARKED : 0);

#ifdef DEBUG
                    TracePIDL(pidl, TEXT("Inserting"));
                    TraceMsg(TF_NSC, "_AddItemToTree(htiParent=%#08lx, htiAfter=%#08lx, fCheckForDups=%d, _psfCache=%#08lx)", 
                                htiParent, htiAfter, fCheckForDups, _psfCache);
                    
#endif // DEBUG

                    pii->fNavigable = !_IsItemFileSystem(_psfCache, pidl);
    
                    htiRet = TreeView_InsertItem(_hwndTree, &tii);
                    if (htiRet)
                    {
                        pii = NULL;     // don't free
                        poi = NULL;     // don't free
                        pidl = NULL;    // don't free
                    }
                }
                if (pii)
                    LocalFree(pii);
            }
            if (poi)
                OrderItem_Free(poi, FALSE);
        }
    }
    if (pidl)
        ILFree((LPITEMIDLIST)pidl);
    return htiRet;
}

HRESULT CNscTree::CreateTree(HWND hwndParent, DWORD dwStyles, HWND *phwnd)
{
    _fIsSelectionCached = FALSE;
    if (*phwnd)
        return S_OK;                                
    
    _style |= (WS_CHILD | TVS_INFOTIP | TVS_FULLROWSELECT | TVS_EDITLABELS
        | TVS_SHOWSELALWAYS | TVS_NONEVENHEIGHT | TVS_NOHSCROLL | dwStyles);

    if (TVS_HASLINES & _style)
        _style &= ~TVS_FULLROWSELECT;       // If it has TVS_HASLINES, it can't have TVS_FULLROWSELECT

    if (_mode != MODE_NORMAL)
    {
        // We don't want track select (underline and blue) for the folder or tree view.
        _style |= TVS_TRACKSELECT;

        //get single expand setting from registry
        DWORD dwValue;
        DWORD dwSize = SIZEOF(dwValue);
        BOOL  fDefault = TRUE;

        SHRegGetUSValue(L"Software\\Microsoft\\Internet Explorer\\Main",
                        L"NscSingleExpand", NULL, (LPBYTE)&dwValue, &dwSize, FALSE,
                        (void *) &fDefault, SIZEOF(fDefault));

        if (dwValue)
        {
            _style |= TVS_SINGLEEXPAND;
            _fSingleExpand = TRUE;
        }
    }
    else
    {
        // According to Bug#241601, Tooltips display too quickly. The problem is
        // the original designer of the InfoTips in the Treeview merged the "InfoTip" tooltip and
        // the "I'm too small to display correctly" tooltips. This is really unfortunate because you
        // cannot control the display of these tooltips independantly. Therefore we are turning off
        // infotips in normal mode. (lamadio) 4.7.99
        _style &= ~TVS_INFOTIP;
    }
    
    _hwndParent = hwndParent;
    *phwnd = _CreateTreeview();
    if (*phwnd == NULL)
    {
        ASSERT(FALSE);
        return E_OUTOFMEMORY;
    }
    ::ShowWindow(_hwndTree, SW_SHOW);
    return S_OK;
}

HWND CNscTree::_CreateTreeview()
{
    ASSERT(_hwndTree == NULL);
    
    if (!_hwndParent)
    {
        TraceMsg(TF_WARNING, "CNscTree::_CreateTreeview has no parent window");
    }
    
    DWORD dwExStyle = 0;
    
    RECT rcParent;
    GetClientRect(_hwndParent, &rcParent);
    
    _style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP;
    _hwndTree = CreateWindowEx(dwExStyle, WC_TREEVIEW, NULL, _style,
        0, 0, rcParent.right, rcParent.bottom, _hwndParent, (HMENU)ID_CONTROL, HINST_THISDLL, NULL);
    
    if (_hwndTree)
    {
        SendMessage(_hwndTree, TVM_SETSCROLLTIME, 100, 0);
        SendMessage(_hwndTree, CCM_SETUNICODEFORMAT, DLL_IS_UNICODE, 0);
    }
    else
    {
        TraceMsg(TF_ERROR, "_hwndTree failed");
    }

    return _hwndTree;
} 

void CNscTree::_TvOnHide()
{
    _DtRevoke();
}

void CNscTree::_TvOnShow()
{
    _DtRegister();
}

HRESULT CNscTree::_HandleWinIniChange()
{
    g_clrBk = GetSysColor(COLOR_WINDOW);

    if (_mode != MODE_NORMAL)
    {
        // make things a bit more spaced out
        int cyItem = TreeView_GetItemHeight(_hwndTree);
        cyItem += LOGOGAP + 1;
        TreeView_SetItemHeight(_hwndTree, cyItem);
    }

    return S_OK;
}

HRESULT CNscTree::Initialize(LPCITEMIDLIST pidlRoot, DWORD grfFlags, DWORD dwFlags)
{
    HRESULT     hres;

    _grfFlags = grfFlags;                          // Filter flags.
    _dwFlags = dwFlags;                            // Behavior Flags

    if (!_fInitialized)
    {
        _fInitialized = TRUE;
    
        SHFILEINFO sfi;
        HIMAGELIST himl = (HIMAGELIST)SHGetFileInfo(TEXT(DEFAULT_PATHSTR), 0, &sfi
            , sizeof(SHFILEINFO),  SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    
        TreeView_SetImageList(_hwndTree, himl, TVSIL_NORMAL);
        _DtRegister();
    
        hres = Init();  // init lock and scroll handles for CDelegateDropTarget
    
        ASSERT(SUCCEEDED(hres));
    
        if (_mode != MODE_NORMAL)
        {
            // set borders and space out for all, much cleaner.
            TreeView_SetBorder(_hwndTree, TVSBF_XBORDER, 2*LOGOGAP, 0);   
        }
    
        //init g_clrBk
        _HandleWinIniChange();

        // pidlRoot may equal NULL because that is equal to CSIDL_DESKTOP.
        if ((LPITEMIDLIST)INVALID_HANDLE_VALUE != pidlRoot)
        {
            _UnSubClass();
            _SetRoot(pidlRoot, 1, NULL, NSSR_CREATEPIDL);
            _SubClass(pidlRoot);
        }
    
        // need top level frame available for D&D if possible.
        IOleWindow *pOleWindow;
    
        _hwndDD = GetParent(_hwndTree);
        if (_punkSite)
        {
            HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IOleWindow, (void **)&pOleWindow);
            if (SUCCEEDED(hr))
            { 
                ASSERT(pOleWindow);
                pOleWindow->GetWindow(&_hwndDD);
                pOleWindow->Release();
            }
        }

        //this is a non-ML resource
        _hicoPinned = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_PINNED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        ASSERT(_hicoPinned);

        //failure ignored intentionally
        THR(CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                             IID_IShellTaskScheduler, (void **)&_pTaskScheduler));
    }
    else
        hres = _ChangePidlRoot(pidlRoot);

    return hres;
}


// BUGBUG: Move to \shlwapi\util.cpp
HRESULT IUnknown_UIActivateIO(IUnknown * punk, BOOL fActivate, LPMSG pMsg)
{
    HRESULT hr = E_INVALIDARG;
    
    if (punk)
    {
        IInputObject * pio;

        hr = punk->QueryInterface(IID_IInputObject, (void **) &pio);
        if (SUCCEEDED(hr))
        {
            hr = pio->UIActivateIO(fActivate, pMsg);
            pio->Release();
        }
    }

    return hr;
}


// set the root of the name space control.
//
// in:
//  pidlRoot    NULL means the desktop
//    HIWORD 0 -> LOWORD == ID of special folder (CSIDL_* values)
//
//  flags,
//  pidlRoot,       PIDL, NULL for desktop, or CSIDL for shell special folder
//  iExpandDepth,   how many levels to expand the tree
//  pidlExpandTo    NULL, or PIDL to expand to
//

BOOL CNscTree::_SetRoot(LPCITEMIDLIST pidlRoot, int iExpandDepth, LPCITEMIDLIST pidlExpandTo, NSSR_FLAGS flags)
{
    _ReleasePidls();
    // review chrisny:  clean up this psr stuff.
    // HIWORD/LOWORD stuff is to support pidl IDs instead of full pidl here
    if (HIWORD(pidlRoot))
        _pidlRoot = ILClone(pidlRoot);
    else
    {
        SHGetSpecialFolderLocation(NULL, LOWORD(pidlRoot) 
            ? LOWORD(pidlRoot) 
            : CSIDL_DESKTOP, &_pidlRoot);
    }
    if (_pidlRoot)
    {
        HTREEITEM htiRoot = TVI_ROOT;
        if (_mode == MODE_NORMAL)
        {
            // Since we'll be adding this into the tree, we need
            // to clone it: We have a copy for the class, and we
            // have one for the tree itself (Makes life easier so
            // we don't have to special case TVI_ROOT).
            LPITEMIDLIST pidlRoot = ILClone(_pidlRoot);
            if (pidlRoot)
            {
                htiRoot = _AddItemToTree(TVI_ROOT, pidlRoot, 1, 0);
                if (htiRoot)
                {
                    TraceMsg(TF_NSC, "NSCBand: Setting Root to \"Desktop\"");
                    TreeView_Expand(_hwndTree, htiRoot, TVE_EXPAND);
                    TreeView_SelectItem(_hwndTree, htiRoot);
                    return TRUE;
                }
                else
                {
                    ILFree(pidlRoot);
                    htiRoot = TVI_ROOT;
                }
            }
        }

        int cAdded;
        BOOL fOrdered = _fOrdered;
        _LoadSF(htiRoot, _pidlRoot, TRUE, &cAdded, &fOrdered);   // load the roots (actual children of _pidlRoot.
        _fOrdered = BOOLIFY(fOrdered);

#ifdef DEBUG
        TracePIDLAbs(_pidlRoot, TEXT("Setting Root to"));
#endif // DEBUG

        // in organize favorites, select the first item by default
        if (_mode & MODE_CONTROL)
        {
            //yes, this is really necessary. the selectitem scrolls the list down so the 
            //first item is not visible. doing just the select has no effect!
            HTREEITEM htiFirst = TreeView_GetFirstVisible(_hwndTree);
            TreeView_SelectItem(_hwndTree, htiFirst);
            TreeView_Expand(_hwndTree, htiFirst, TVE_COLLAPSE); //just in case it expanded
            TreeView_Select(_hwndTree, htiFirst, TVGN_FIRSTVISIBLE);
        }

        return TRUE;
    }

    TraceMsg(DM_ERROR, "set root failed");
    _ReleasePidls();
    return FALSE;
}


// cache the shell folder for a given tree item
// in:
//  hti tree node to cache shell folder for. this my be
//      NULL indicating the root item.
//

BOOL CNscTree::_CacheShellFolder(HTREEITEM hti)
{
    // in the cache?
    if ((hti != _htiCache) || (_psfCache == NULL))
    {
        // cache miss, do the work
        LPITEMIDLIST pidl;
        BOOL fRet = FALSE;
        
        _fpsfCacheIsTopLevel = FALSE;
        _ReleaseCachedShellFolder();
        
        if ((hti == NULL) || (hti == TVI_ROOT))
        {
            pidl = ILClone(_pidlRoot);
        }
        else
            pidl = _GetFullIDList(hti);
            
        if (pidl)
        {
            if (SUCCEEDED(IEBindToObject(pidl, &_psfCache)))
            {
                ASSERT(_psfCache);
                _htiCache = hti;    // this is for the cache match
                _fpsfCacheIsTopLevel = ( hti == TVI_ROOT || hti == NULL );
                fRet = TRUE;
            }      
            
            ILFree(pidl);
        }
        
        return fRet;
    }
    return TRUE;
}

#define TVI_ROOTPARENT ((HTREEITEM)(ULONG_PTR)-0xF000)

// pidlItem is typically a relative pidl, except in the case of the root where
// it can be a fully qualified pidl

LPITEMIDLIST CNscTree::_CacheParentShellFolder(HTREEITEM hti, LPITEMIDLIST pidl)
{
    // need parent shell folder of TVI_ROOT, special case for drop insert into root level of tree.
    if (hti == TVI_ROOT || 
        hti == NULL || 
        (_mode == MODE_NORMAL &&
        TreeView_GetParent(_hwndTree, hti) == NULL))    // If we have a null parent and we're a normal, 
                                                        // than that's the same as root.
    {
        if (_htiCache != TVI_ROOTPARENT) 
        {
            _ReleaseCachedShellFolder();
            IEBindToParentFolder(_pidlRoot, &_psfCache, NULL);

            if (!ILIsEmpty(_pidlRoot))
                _htiCache = TVI_ROOTPARENT;
        }
        return ILFindLastID(_pidlRoot);
    }

    if (_CacheShellFolder(TreeView_GetParent(_hwndTree, hti)))
    {
        if (pidl == NULL)
        {
            PORDERITEM poi = _GetTreeOrderItem(hti);
            if (!poi)
                return NULL;

            pidl = poi->pidl;
        }
        
        return ILFindLastID(pidl);
    }
    
    return NULL;
}

typedef struct _SORTPARAMS
{
    CNscTree *pnsc;
    IShellFolder *psf;
} SORTPARAMS;

int CALLBACK CNscTree::_TreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    SORTPARAMS *pSortParams = (SORTPARAMS *)lParamSort;
    PORDERITEM poi1 = GetPoi(lParam1), poi2 = GetPoi(lParam2);
    
    HRESULT hres = pSortParams->pnsc->_CompareIDs(pSortParams->psf, poi1->pidl, poi2->pidl);
    return (short)SCODE_CODE(hres);
}

int CALLBACK CNscTree::_TreeOrder(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    HRESULT hres;
    PORDERITEM poi1 = GetPoi(lParam1), poi2 = GetPoi(lParam2);
    
    ASSERT((poi1 != NULL) && (poi1 != NULL));
    if (poi1->nOrder == poi2->nOrder)
        hres = 0;
    else
        // do unsigned compare so -1 goes to end of list
        hres = (poi1->nOrder < poi2->nOrder ? -1 : 1);
    
    return (short)SCODE_CODE(hres);
}
// review chrisny:  instead of sort, insert items on the fly.
void CNscTree::_Sort(HTREEITEM hti, IShellFolder *psf)
{
    TV_SORTCB   scb;
    SORTPARAMS  SortParams = {this, psf};
    BOOL        fOrdering = _IsOrdered(hti);
#ifdef DEBUG
        TraceHTREE(hti, TEXT("Sorting"));
#endif
    
    scb.hParent = hti;
    scb.lpfnCompare = !fOrdering ? _TreeCompare : _TreeOrder;
    
    scb.lParam = (LPARAM)&SortParams;
    TreeView_SortChildrenCB(_hwndTree, &scb, FALSE);
}

BOOL CNscTree::_IsOrdered(HTREEITEM htiRoot)
{
    if ( (htiRoot == TVI_ROOT) || (htiRoot == NULL) )
        return _fOrdered;
    else
    {
        PORDERITEM poi = _GetTreeOrderItem(htiRoot);
        if (poi)
        {
            // LParam Is a Boolean: 
            // TRUE: It has an order.
            // FALSE: It does not have an order.
            // Question: Where is that order stored? _hdpaOrder?
            return poi->lParam;
        }
    }
    return FALSE;
}

//helper function to init _hdpaOrd
//MUST be followed by a call to _FreeOrderList
HRESULT CNscTree::_PopulateOrderList(HTREEITEM htiRoot)
{
    int        i = 0;
    HTREEITEM  hti = NULL;
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Populating Order List from tree node"));
#endif
    
    if (_hdpaOrd)
        DPA_Destroy(_hdpaOrd);
    
    _hdpaOrd = DPA_Create(4);
    if (_hdpaOrd == NULL)
        return E_FAIL;
    
    for (hti = TreeView_GetChild(_hwndTree, htiRoot); hti;
    hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        PORDERITEM poi = _GetTreeOrderItem(hti);
        if (poi)
        {
            poi->nOrder = i;        // reset the positions of the nodes.
            DPA_SetPtr(_hdpaOrd, i++, (void *)poi);
        }
    }
    
    //set the root's ordered flag
    if (htiRoot == TVI_ROOT)
        _fOrdered = TRUE;
    else
    {
        PORDERITEM poi = _GetTreeOrderItem(htiRoot);
        if (poi)
        {
            poi->lParam = TRUE;
        }
    }
    
    return S_OK;
}

//helper function to free _hdpaOrd
//MUST be preceded by a call to _PopulateOrderList

void CNscTree::_FreeOrderList(HTREEITEM htiRoot)
{
    ASSERT(_hdpaOrd);
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Freeing OrderList"));
#endif

    _ReleaseCachedShellFolder();
    
    // Persist the new order out to the registry
    LPITEMIDLIST pidl = _GetFullIDList(htiRoot);
    if (pidl)
    {
        IStream* pstm = GetOrderStream(pidl, STGM_WRITE | STGM_CREATE);
        if (pstm)
        {
            if (_CacheShellFolder(htiRoot))
            {
                OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
                pstm->Release();
                
                // Notify everyone that the order changed
                SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, SHCNF_FLUSH, _pidlRoot);

                TraceMsg(TF_NSC, "NSCBand: Sent SHCNE_EXTENDED_EVENT : SHCNEE_ORDERCHANGED");
                
                // Remove this notify message immediately (so _fDropping is set
                // and we'll ignore this event in above OnChange method)
                //
                // _FlushNotifyMessages(_hwndTree);
            }
            else
                pstm->Release();
        }
        ILFree(pidl);
    }
    
    DPA_Destroy(_hdpaOrd);
    _hdpaOrd = NULL;
}

//removes any order the user has set and goes back to alphabetical sort
HRESULT CNscTree::ResetSort(void)
{
    HRESULT hr = S_OK;
#ifdef UNUSED
    ASSERT(_psfCache);
    ASSERT(_pidlRoot);
    
    int cAdded = 0;
    IStream* pstm = NULL;
    
    _fWeChangedOrder = TRUE;
    if (FAILED(hr = _PopulateOrderList(TVI_ROOT)))
        return hr;
    
    pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, _pidlRoot, REG_SUBKEY_FAVORITESA, STGM_CREATE | STGM_WRITE);
    
    _CacheShellFolder(TVI_ROOT);
    
    if (pstm == NULL || _psfCache == NULL)
    {
        ATOMICRELEASE(pstm);
        _FreeOrderList(TVI_ROOT);
        return S_OK;
    }
    _fOrdered = FALSE;
    
    ORDERINFO   oinfo;
    oinfo.psf = _psfCache;
    (oinfo.psf)->AddRef();
    oinfo.dwSortBy = OI_SORTBYNAME;
    DPA_Sort(_hdpaOrd, OrderItem_Compare,(LPARAM)&oinfo);
    ATOMICRELEASE(oinfo.psf);
    
    OrderList_Reorder(_hdpaOrd);
    
    OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
    ATOMICRELEASE(pstm);
    
    _FreeOrderList(TVI_ROOT);
    Refresh();
    
    _fWeChangedOrder = FALSE;
#endif

    return hr;
}



void CNscTree::MoveItemUpOrDown(BOOL fUp)
{
    HTREEITEM   htiSelected, htiToSwap, htiParent;
    PORDERITEM  poiSelected, poiToSwap;
    
    htiSelected = TreeView_GetSelection(_hwndTree);
    htiToSwap = (fUp) ? TreeView_GetPrevSibling(_hwndTree, htiSelected) : 
                        TreeView_GetNextSibling(_hwndTree, htiSelected);
    htiParent = TreeView_GetParent(_hwndTree, htiSelected);
    if (htiParent == NULL)
        htiParent = TVI_ROOT;
    ASSERT(htiSelected);
    
    _fWeChangedOrder = TRUE;
    if (FAILED(_PopulateOrderList(htiParent)))
        return;
    
    if ( (htiSelected) && (htiToSwap) )
    {
        if ((poiSelected = _GetTreeOrderItem(htiSelected)) &&
            (poiToSwap   = _GetTreeOrderItem(htiToSwap)))
        {
            int iOrder = 0;
            
            iOrder = poiSelected->nOrder;
            poiSelected->nOrder = poiToSwap->nOrder;
            poiToSwap->nOrder   = iOrder;
        }
        
        _CacheShellFolder(htiParent);
        
        if (_psfCache)
            _Sort(htiParent, _psfCache);
    }
    TreeView_SelectItem(_hwndTree, htiSelected);
    
    _FreeOrderList(htiParent);
    _fWeChangedOrder = FALSE;
}



// filter function... let clients filter what gets added here

BOOL CNscTree::_ShouldAdd(LPCITEMIDLIST pidl)
{
    // send notify up to parent to let them filter
    return TRUE;
}


BOOL CNscTree::_OnItemExpandingMsg(NM_TREEVIEW *pnm)
{
    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    BOOL bRet = _OnItemExpanding(pnm->itemNew.hItem, pnm->action, (pnm->itemNew.state & TVIS_EXPANDEDONCE));

    SetCursor(hCursorOld);

    return bRet;
}

//
//  The NSC item is expandable if it is a regular folder and it's not one
//  of those funky non-expandable channel folders.
//
BOOL CNscTree::_IsExpandable(HTREEITEM hti)
{
    BOOL fExpandable = FALSE;
    LPCITEMIDLIST pidlItem = _CacheParentShellFolder(hti, NULL);
    if (pidlItem)
    {
        // make sure item is actually a folder and not a non-expandable channel folder
        // except: in org favs, never expand channel folders
        ULONG ulAttr = SFGAO_FOLDER;
        LPITEMIDLIST pidlTarget = NULL;
        if (SUCCEEDED(_psfCache->GetAttributesOf(1, &pidlItem, &ulAttr)) &&
            (ulAttr & SFGAO_FOLDER) &&
            !(SUCCEEDED(SHGetNavigateTarget(_psfCache, pidlItem, &pidlTarget, &ulAttr)) &&
                  ((_mode & MODE_CONTROL) ?
                        TRUE :
                        !IsExpandableChannelFolder(_psfCache, pidlItem))) )
        {
            fExpandable = TRUE;
        }
        ILFree(pidlTarget);
    }
    return fExpandable;
}

BOOL CNscTree::_OnItemExpanding(HTREEITEM htiToActivate, UINT action, BOOL fExpandedOnce)
{
    int cAdded = 0;
    
    if (action != TVE_EXPAND)
    {
        htiToActivate = TreeView_GetParent(_hwndTree, htiToActivate);
    } 
    else if (fExpandedOnce)
    {
        ; //item has already been expanded, don't add items
    }
    else
    {
        if (_IsExpandable(htiToActivate))
        {
            LPITEMIDLIST pidlParent = _GetFullIDList(htiToActivate);
            if (pidlParent)
            {
                BOOL fOrdered;

                // if we're refreshing, fAddingOnly should false
                _LoadSF(htiToActivate, pidlParent, !_fRefreshing, &cAdded, &fOrdered);
                ILFree(pidlParent);
            }
        }

        // If we did not add anything we should update this item to let
        // the user know something happened.
        //
        if (cAdded == 0)
        {
            TV_ITEM tvi;
            tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children
            tvi.hItem = htiToActivate;
            tvi.cChildren = 0;
            
            TreeView_SetItem(_hwndTree, &tvi);
        }
    }
    
    _UpdateActiveBorder(htiToActivate);
    return TRUE;
}

HTREEITEM CNscTree::_FindFromRoot(HTREEITEM htiRoot, LPCITEMIDLIST pidl)
{
    HTREEITEM    htiRet = NULL;
    LPITEMIDLIST pidlParent, pidlChild;
    BOOL         fFreePidlParent = FALSE;
#ifdef DEBUG
    TracePIDLAbs(pidl, TEXT("Finding this pidl"));
    TraceHTREE(htiRoot, TEXT("from this root"));
#endif
    
    if (!htiRoot) 
    {
        // When in "Normal" mode, we need to use the first child, not the root
        // in order to calculate, because there is no "Invisible" root. On the
        // other hand, History and Favorites have an invisible root: Their
        // parent folder, so they need this fudge.
        htiRoot = (MODE_NORMAL == _mode)?TreeView_GetChild(_hwndTree, 0) : TVI_ROOT;
        pidlParent = _pidlRoot;    // the invisible root.
    }
    else 
    {
        pidlParent      = _GetFullIDList(htiRoot);
        fFreePidlParent = TRUE;
    }
    
    if (pidlParent == NULL)
        return NULL;
    
    if (ILIsEqual(pidlParent, pidl)) 
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return htiRoot;
    }
    
    pidlChild = ILFindChild(pidlParent, pidl);
    if (pidlChild == NULL) 
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return NULL;    // not root match, no hti
    }
    
    // root match, carry on . . .
    
    // Are we rooted under the Desktop (i.e. Empty pidl or ILIsEmpty(_pidlRoot))
    IShellFolder *psf = NULL;
    HRESULT hres = IEBindToObject(pidlParent, &psf);

    if (FAILED(hres))
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return htiRet;
    }
    
    while (htiRoot && psf)
    {
        LPITEMIDLIST pidlItem = ILCloneFirst(pidlChild);
        if (!pidlItem)
            break;
        
        htiRoot = _FindChild(psf, htiRoot, pidlItem);
        IShellFolder *psfNext = NULL;
        hres = psf->BindToObject(pidlItem, NULL, IID_IShellFolder, (void **)&psfNext);
        ILFree(pidlItem);
        if (!htiRoot)
        {
            ATOMICRELEASE(psfNext);
            break;
        }
        psf->Release();
        psf = psfNext;
        pidlChild = _ILNext(pidlChild);
        // if we're down to an empty pidl, we've found it!
        if (ILIsEmpty(pidlChild)) 
        {
            htiRet = htiRoot;
            break;
        }
        if (FAILED(hres))
        {
            ASSERT(psfNext == NULL);
            break;
        }
    }
    if (psf) 
        psf->Release();
    if (fFreePidlParent)
        ILFree(pidlParent);
#ifdef DEBUG
    TraceHTREE(htiRet, TEXT("Found at"));
#endif

    return htiRet;
}

BOOL CNscTree::_FIsItem(IShellFolder * psf, LPCITEMIDLIST pidl, HTREEITEM hti)
{
    PORDERITEM poi = _GetTreeOrderItem(hti);

    if (poi)
    {

        if (poi->pidl && psf->CompareIDs(0, poi->pidl, pidl) == 0)
            return TRUE;
    }
    return FALSE;
}

HRESULT CNscTree::_OnSHNotifyDelete(LPCITEMIDLIST pidl, int *piPosDeleted, HTREEITEM *phtiParent)
{
    HRESULT     hres = S_FALSE;
    HTREEITEM   hti = _FindFromRoot(NULL, pidl);
    
    if (hti == TVI_ROOT)
        return E_INVALIDARG;        // invalid arg, DELETION OF TVI_ROOT
    // need to clear _pidlDrag if the one being deleted is _pidlDrag.
    // handles case where dragging into another folder from within or dragging out.
    if (_pidlDrag)
    {
        LPCITEMIDLIST pidltst = _CacheParentShellFolder(hti, NULL);
        if (pidltst)
        {
            if (!_psfCache->CompareIDs(0, pidltst, _pidlDrag))
                _pidlDrag = NULL;
        }
    }

    if (pidl && (hti != NULL))
    {
        _fIgnoreNextItemExpanding = TRUE;

        HTREEITEM htiParent = TreeView_GetParent(_hwndTree, hti);
        
        if (phtiParent)
            *phtiParent = htiParent;

        //if caller wants the position of the deleted item, don't reorder the other items
        if (piPosDeleted)
        {
            PORDERITEM poi = _GetTreeOrderItem(hti);
            if (poi)
            {
                *piPosDeleted = poi->nOrder;
                hres = S_OK;
            }
            TreeView_DeleteItem(_hwndTree, hti);
        }
        else
        {
            if (htiParent == NULL)
                htiParent = TVI_ROOT;
            if (TreeView_DeleteItem(_hwndTree, hti))
            {
                _ReorderChildren(htiParent);
                hres = S_OK;
            }
        }

        // Update the + next to the parent folder. Note that History and Favorites
        // set ALL of their items to be Folder items, so this is not needed for
        // favorites.
        if (_mode == MODE_NORMAL)
        {
            LPCITEMIDLIST pidl = _CacheParentShellFolder(htiParent, NULL);
            if (pidl && !ILIsEmpty(pidl))
            {
                DWORD dwAttrib = SFGAO_HASSUBFOLDER;
                if (SUCCEEDED(_psfCache->GetAttributesOf(1, &pidl, &dwAttrib)) &&
                    !(dwAttrib & SFGAO_HASSUBFOLDER))
                {
                    TV_ITEM tvi;
                    tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;
                    tvi.hItem = htiParent;
                    tvi.cChildren = 0;
                    TreeView_SetItem(_hwndTree, &tvi);
                }
            }
        }


        _fIgnoreNextItemExpanding = FALSE;

        if (hti == _htiCut)
        {
            _htiCut = NULL;
            _TreeNukeCutState();
        }

    }
    return hres;
}

//
//  Attempt to perform a rename-in-place.  Returns
//
//  S_OK - rename succeeded
//  S_FALSE - original object not found
//  error - rename failed
//

HRESULT CNscTree::_OnSHNotifyRename(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNew)
{
    HTREEITEM   hti, htiParent = NULL;
    int         iPosDeleted = DEFAULTORDERPOSITION;
    HRESULT     hres;

    //
    //  If the source and destination belong to the same folder, then
    //  it's an in-folder rename.
    //
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    LPITEMIDLIST pidlNewParent = ILCloneParent(pidlNew);
    LPCITEMIDLIST pidlNewChild;
    LPITEMIDLIST pidlRealChild;
    IShellFolder *psf = NULL;
    PORDERITEM poi;

    if (pidlParent && pidlNewParent &&
        IEILIsEqual(pidlParent, pidlNewParent, TRUE) &&
        (hti = _FindFromRoot(NULL, pidl)) &&
        (poi = _GetTreeOrderItem(hti)) &&
        SUCCEEDED(_ParentFromItem(pidlNew, &psf, &pidlNewChild)) &&
        SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlNewChild, &pidlRealChild)))
    {
        // Just substitute the final pidl component
        ILFree(poi->pidl);
        poi->pidl = pidlRealChild;
        _TreeInvalidateItemInfo(hti, TVIF_TEXT);

        // BUGBUG If we renamed the item the user is sitting on,
        // SHBrowseForFolder doesn't realize it and doesn't update the
        // edit control.

        hres = S_OK;
    }
    else
    // rename can be a move, so do not depend on the delete happening successfully.
    if ((_OnSHNotifyDelete(pidl, &iPosDeleted, &htiParent) != E_INVALIDARG)   // invalid arg indication of bogus rename, do not continue.
        && ((hti = _FindFromRoot(NULL, pidlNew)) == NULL) 
        && (_OnSHNotifyCreate(pidlNew, iPosDeleted, htiParent) == S_OK))
    {
        hres = S_OK;
    }
    else
    {
        hres = S_FALSE;
    }

    ILFree(pidlParent);
    ILFree(pidlNewParent);
    ATOMICRELEASE(psf);

    return hres;
    
}

//
//  To update an item, just find it and invalidate it.
//
void CNscTree::_OnSHNotifyUpdateItem(LPCITEMIDLIST pidl)
{
    HTREEITEM hti = _FindFromRoot(NULL, pidl);
    if (hti)
        _TreeInvalidateItemInfo(hti, TVIF_TEXT);
}

HRESULT CNscTree::_OnSHNotifyUpdateDir(LPCITEMIDLIST pidl)
{
    HRESULT         hres = S_FALSE;
    HTREEITEM       hti;
    if (((hti = _FindFromRoot(NULL, pidl)) != NULL))
    {   // folder exists in tree refresh folder now if had been loaded by expansion.
        TV_ITEM tvi;
        tvi.mask = TVIF_STATE;
        tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
        tvi.hItem = (HTREEITEM)hti;
        if ((hti == TVI_ROOT) || (TreeView_GetItem(_hwndTree, &tvi) && (tvi.state & TVIS_EXPANDEDONCE)))
            hres = _UpdateDir(hti);
        else if (!(tvi.state & TVIS_EXPANDEDONCE))
        {
            TV_ITEM     tvi;
            tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children so expand below will work.
            tvi.hItem = hti;
            tvi.cChildren = 1;
            TreeView_SetItem(_hwndTree, &tvi);
        }
    }
    return hres;
}

HRESULT CNscTree::_GetEnum(IShellFolder *psf, LPCITEMIDLIST pidlFolder, IEnumIDList **ppenum)
{
    HWND hwnd = NULL;
    DWORD grfFlags = _grfFlags;

    if (_pFilter)
    {
        LPITEMIDLIST pidlFree = NULL;
        if (pidlFolder == NULL)
        {
            SHGetIDListFromUnk(psf, &pidlFree);
            pidlFolder = pidlFree;
        }
        _pFilter->GetEnumFlags(psf, pidlFolder, &hwnd, &grfFlags);

        if (pidlFree)
            ILFree(pidlFree);
    }

    // get the enumerator and add the child items for any given pidl
    // BUGBUG right now, we don't detect if we actually are dealing with a folder (shell32.dll
    // BUGBUG allows you to create an IShellfolder to a non folder object, so we get stupid
    // BUGUBG dialogs, by not passing the hwnd, we don't get the dialogs. we should fix this better. by caching
    // BUGBUG in the tree whether it is a folder or not.
    return psf->EnumObjects(/* _fAutoExpanding ?*/ hwnd, grfFlags, ppenum);
}

BOOL CNscTree::_ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    BOOL bRet = TRUE;
    if (_pFilter)
    {
        LPITEMIDLIST pidlFree = NULL;
        if (pidlFolder == NULL)
        {
            SHGetIDListFromUnk(psf, &pidlFree);
            pidlFolder = pidlFree;
        }
        bRet = (S_OK == _pFilter->ShouldShow(psf, pidlFolder, pidlItem));

        if (pidlFree)
            ILFree(pidlFree);
    }
    return bRet;
}

// review chrisny:  threadize this 
// updates existing dir only.  Not new load.
HRESULT CNscTree::_UpdateDir(HTREEITEM hti)
{
    HTREEITEM       htiTemp;
#ifdef DEBUG
//    TraceHTREE(hti, TEXT("UpdateDIR"));
#endif
    
    // we now know the true state of the folder represented by input hti
    // , so recursively updatedir on all it's children. Nasty. . .
    // review chrisny:  perf on this.
    for (htiTemp = TreeView_GetChild(_hwndTree, hti); htiTemp
        ; htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp)) 
        _UpdateDir(htiTemp);
    if (!_CacheShellFolder(hti))          // immediate failure if not folder causing return.
        return S_FALSE;

    //if this item hasn't been expanded yet (and filled with items), ignore it
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED);
    tvi.hItem = (HTREEITEM)hti;
    if ((hti != TVI_ROOT) && (TreeView_GetItem(_hwndTree, &tvi) && !(tvi.state & TVIS_EXPANDEDONCE)))
        return S_OK;

    HDPA hdpa = DPA_Create(4);
    if (!hdpa)
        return E_OUTOFMEMORY;
    
    LPITEMIDLIST pidlParent = _GetFullIDList(hti);
    if (pidlParent == NULL)
    {
        DPA_Destroy(hdpa);
        return S_FALSE;
    }

    IEnumIDList *penum; 
    if (S_OK == _GetEnum(_psfCache, pidlParent, &penum))
    {
        LPITEMIDLIST pidlChild;
        ULONG celt, i = 0;
        
        while (penum->Next(1, &pidlChild, &celt) == S_OK && celt == 1)
        {
            if (_ShouldShow(_psfCache, pidlParent, pidlChild))
            {
                DPA_SetPtr(hdpa, i++, pidlChild);
            }
            else
                ILFree(pidlChild);
        }
        
        penum->Release();
        
        // set pointers to be kept to NULL in DPA and delete item from tree
        celt = i;
        if (celt && (hti != TVI_ROOT))
        {
            TV_ITEM tvi;
            tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children so expand below will work.
            tvi.hItem = hti;
            tvi.cChildren = celt;
            TreeView_SetItem(_hwndTree, &tvi);
        }
        
        HDPA hDel = DPA_Create(4);
        if (!hDel) 
            return E_FAIL;
        
        i = 0;
        for (htiTemp = TreeView_GetChild(_hwndTree, hti); htiTemp; ) 
        {
            HTREEITEM htiNextChild = TreeView_GetNextSibling(_hwndTree, htiTemp);
            // must delete in this way or break the linkage of tree.
            if (!_FInTree(hdpa, celt, htiTemp)) 
            {
                DPA_SetPtr( hDel, i++, htiTemp );
                //if (!TreeView_DeleteItem(_hwndTree, htiTemp))
                //    ASSERT(FALSE);       // somethings hosed in the tree.
            }
            htiTemp = htiNextChild;
        }
        
        ULONG cDel = DPA_GetPtrCount( hDel );
        for(i = 0;i < cDel; i++ )
        {
            if (!TreeView_DeleteItem(_hwndTree, (HTREEITEM)DPA_FastGetPtr(hDel,i)))
            {
                ASSERT(FALSE);       // somethings hosed in the tree.
            }
        }
        
        DPA_Destroy(hDel);
        
        for (i = 0; i < celt; i++)
        {
            pidlChild = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
            if (pidlChild)
            {
                LPITEMIDLIST pidlFull = ILCombine(pidlParent, pidlChild);
                if (pidlFull)
                {
                    htiTemp = _FindFromRoot(hti, pidlFull);
                    // if we DON'T FIND IT add it to the tree . . .
                    if (!htiTemp)
                    {
                        if (_AddItemToTree(hti, pidlChild, 1, 32000, TVI_LAST, TRUE, _IsMarked(hti)))
                            DPA_SetPtr(hdpa, i, NULL);  // so we don't free below
                    }
                    ILFree(pidlFull);
                }
            }
        }
        // pidls away . . .
        for (i = 0; i < celt; i++)
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
            if (pidl)
                ILFree(pidl);
        }
        EVAL(DPA_Destroy(hdpa));

        // Handle Ordering information.
        HDPA hdpaOrder;
        if (_LoadOrder(hti, pidlParent, _psfCache, &hdpaOrder))
        {
            HTREEITEM htiChild = TreeView_GetChild(_hwndTree, hti);
            // We have to do a manual merge with the items in the listview.
            // Exercise left for the student: Make this fast.

            for (; htiChild; htiChild = TreeView_GetNextSibling(_hwndTree, htiChild)) 
            {
                PORDERITEM poi = _GetTreeOrderItem(htiChild);
                if (poi)
                {
                    int cOrder = DPA_GetPtrCount(hdpaOrder);
                    for (int i = 0; i < cOrder; i++)
                    {
                        PORDERITEM poiOrder = (PORDERITEM)DPA_FastGetPtr(hdpaOrder, i);
                        if (_psfCache->CompareIDs(0, poi->pidl, poiOrder->pidl) == 0)
                        {
                            poi->nOrder = poiOrder->nOrder;
                            OrderItem_Free(poiOrder);
                            DPA_DeletePtr(hdpaOrder, i);
                            break;
                        }
                    }
                }
            }

            OrderList_Destroy(&hdpaOrder, FALSE);
        }

        
        _Sort(hti, _psfCache);
    }
    ILFree(pidlParent);
    return S_OK;
}

BOOL CNscTree::_FInTree(HDPA hdpa, int celt, HTREEITEM hti)
{

    ASSERT(hti);
    PORDERITEM poi = _GetTreeOrderItem(hti);
    if (poi)
    {
        for (int i = 0; i < celt; i++)
        {
            LPITEMIDLIST    pidl;
            if ((pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, i)) == NULL)
                continue;
            if (_psfCache->CompareIDs(0, poi->pidl
                , pidl) == 0)
            {
                ILFree(pidl);
                DPA_SetPtr(hdpa, i++, NULL);
                return TRUE;
            }
        }
    }
    return FALSE;
}

HTREEITEM CNscTree::_FindChild(IShellFolder *psf, HTREEITEM htiParent, LPCITEMIDLIST pidlChild)
{
     HTREEITEM hti;
    
    for (hti = TreeView_GetChild(_hwndTree, htiParent); hti
        ; hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        if (_FIsItem(psf, pidlChild, hti))
            break;
    }
    return hti;
}

void CNscTree::_ReorderChildren(HTREEITEM htiParent)
{
    int i = 0;
    HTREEITEM hti;
    for (hti = TreeView_GetChild(_hwndTree, htiParent); hti
        ; hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        PORDERITEM poi = _GetTreeOrderItem(hti);
        if (poi)
        {
            poi->nOrder = i++;        // reset the positions of the nodes.
        }
    }
}


HRESULT CNscTree::_InsertChild(HTREEITEM htiParent, IShellFolder *psfParent, LPCITEMIDLIST pidlChild, BOOL fExpand, int iPosition)
{
    LPITEMIDLIST pidlReal;
    HRESULT hres = S_OK;
    
    // review chrisny:  no sort here, use compareitems to insert item instead.
    if (SUCCEEDED(_IdlRealFromIdlSimple(psfParent, pidlChild, &pidlReal)))
    {
        HTREEITEM htiAfter = TVI_LAST;
        if (iPosition != DEFAULTORDERPOSITION)
        {
            if (iPosition == 0)
                htiAfter = htiParent;
            else
            {
                for (HTREEITEM hti = TreeView_GetChild(_hwndTree, htiParent); hti
                    ; hti = TreeView_GetNextSibling(_hwndTree, hti))
                {
                    PORDERITEM poi = _GetTreeOrderItem(hti);
                    if (poi)
                    {
                        if (poi->nOrder == iPosition-1)
                        {
                            htiAfter = hti;
#ifdef DEBUG
                            TraceHTREE(htiAfter, TEXT("Inserting After"));
#endif
                            break;
                        }
                    }
                }
            }
        }

        HTREEITEM   htiNew;
        DWORD       dwAttrib = SFGAO_HASSUBFOLDER;
        if ((_FindChild(psfParent, htiParent, pidlReal) == NULL) && 
            SUCCEEDED(psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlReal, &dwAttrib)) &&
            ((htiNew = _AddItemToTree(htiParent, pidlReal, 
                                      (MODE_NORMAL != _mode || dwAttrib & SFGAO_HASSUBFOLDER)? 1 : 0, 
                                      iPosition, htiAfter)) != NULL))
        {
            // if added at the default position OR anywhere when sorted by name,
            //  refresh the order information for the items including the newly added item.
            if ( (iPosition == DEFAULTORDERPOSITION) || !_IsOrdered(htiParent) )
            {
                _ReorderChildren(htiParent);
                _Sort(htiParent, psfParent);
            }
            
            if (fExpand ) 
                TreeView_Expand(_hwndTree, htiParent, TVE_EXPAND);    // force expansion to show new item.
            
            //ensure the item is visible after a rename (or external drop, but that should always be a noop)
            if (iPosition != DEFAULTORDERPOSITION)
                TreeView_EnsureVisible(_hwndTree, htiNew);

            hres = S_OK;
        }
        else
        {
            ILFree(pidlReal);
            hres = S_FALSE;
        }       
    }
    return hres;
}


HRESULT CheckForExpandOnce(HWND hwndTree, HTREEITEM hti)
{
    HRESULT  hres = S_OK;
    TV_ITEM tvi;
    
    // Root node always expanded.
    if( hti == TVI_ROOT )
        return hres;
    
    tvi.mask = TVIF_STATE | TVIF_CHILDREN;
    tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
    tvi.hItem = (HTREEITEM)hti;
    
    if (TreeView_GetItem(hwndTree, &tvi))
    {
        if( !(tvi.state & TVIS_EXPANDEDONCE) && (tvi.cChildren == 0) )
        {
            tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;
            tvi.hItem = hti;
            tvi.cChildren = 1;
            TreeView_SetItem(hwndTree, &tvi);
        }
    }
    
    return hres;
}


HRESULT _InvokeCommandThunk(IContextMenu * pcm, HWND hwndParent)
{
    HRESULT hr;

    if (g_fRunningOnNT)
    {
        CMINVOKECOMMANDINFOEX ici = {0};

        ici.cbSize = sizeof(ici);
        ici.hwnd = hwndParent;
        ici.nShow = SW_NORMAL;
        ici.lpVerb = CMDSTR_NEWFOLDERA;
        ici.fMask = CMIC_MASK_UNICODE;
        ici.lpVerbW = CMDSTR_NEWFOLDERW;

        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
    }
    else
    {
        CMINVOKECOMMANDINFO ici = {0};

        ici.cbSize = sizeof(ici);
        ici.hwnd = hwndParent;
        ici.nShow = SW_NORMAL;
        ici.lpVerb = CMDSTR_NEWFOLDERA;

        // Win95 doesn't work with CMIC_MASK_UNICODE & CMDSTR_NEWFOLDERW
        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
    }

    return hr;
}


// We want to create a new folder in the current directory, so
// find it.
HRESULT CNscTree::NewFolder(void)
{
    return CreateNewFolder(TreeView_GetSelection(_hwndTree));
}


BOOL CNscTree::_IsItemExpanded(HTREEITEM hti)
{
    // if it's not open, then use it's parent
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_EXPANDED;
    tvi.hItem = (HTREEITEM)hti;
    
    return (TreeView_GetItem(_hwndTree, &tvi) && (tvi.state & TVIS_EXPANDED));
}

HRESULT CNscTree::CreateNewFolder(HTREEITEM hti)
{
    HRESULT hr = E_FAIL;

    if (hti)
    {
        // If the user selected a folder item (file), we need
        // to bind set the cache to the parent folder.

        LPITEMIDLIST pidl = _GetFullIDList(hti);

        if (EVAL(pidl))
        {
            ULONG ulAttr = SFGAO_FOLDER;    // make sure item is actually a folder

            if (EVAL(SUCCEEDED(IEGetAttributesOf(pidl, &ulAttr))))
            {
                // Is it a folder?
                if (ulAttr & SFGAO_FOLDER)
                {
                    // non-Normal modes (!MODE_NORMAL) wants the new folder to be created as
                    // a sibling instead of as a child of the selected folder if it's
                    // closed.  I assume their reasoning is that closed folders are often
                    // selected by accident/default because these views are mostly 1 level.
                    // We don't want this functionality for the normal mode.
                    if ((MODE_NORMAL != _mode) && !_IsItemExpanded(hti))
                        _CacheShellFolder(TreeView_GetParent(_hwndTree, hti));
                    else                
                        _CacheShellFolder(hti);     // Yes, so fine.
                }
                else
                    _CacheShellFolder(TreeView_GetParent(_hwndTree, hti));  // No, so bind to the parent.
            }

            ILFree(pidl);
        }
    }

    // If no item is selected, we should still create a folder in whatever
    // the user most recently dinked with.  This is important if the
    // Favorites folder is completely empty.


    if (_psfCache)
    {
        // On Win95 it was only the view background menu that supported NewFolder,
        // On IE4 Shell+, it's the folder background object itself, but the view
        // still works.
        //
        IShellView * psv;
        hr = _psfCache->CreateViewObject(_hwndTree, IID_IShellView, (LPVOID *)&psv);
        if (SUCCEEDED(hr))
        {
            IContextMenu * pcm;

            if (SUCCEEDED(psv->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (LPVOID *)&pcm)))
            {
                IUnknown_SetSite(pcm, psv);

                HMENU hmContext = CreatePopupMenu();
                hr = pcm->QueryContextMenu(hmContext, 0, 1, 256, 0);
                if (SUCCEEDED(hr))
                {
                    _pidlNewFolderParent = _GetFullIDList(_htiCache);
                    hr = _InvokeCommandThunk(pcm, _hwndParent);
                    SHChangeNotifyHandleEvents(); // Flush the events to it doesn't take forever to shift into edit mode
                    ILFree(_pidlNewFolderParent);
                    _pidlNewFolderParent = NULL;
                }

                IUnknown_SetSite(pcm, NULL);
                DestroyMenu(hmContext);
                pcm->Release();
            }

            psv->Release();
        }
    }

    return hr;
}


HRESULT CNscTree::_EnterNewFolderEditMode(LPCITEMIDLIST pidlNewFolder)
{
    HRESULT hres = S_OK;
    HTREEITEM htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    LPITEMIDLIST pidlParent = NULL;
    
    // 1. Flush all the notifications.
    // 2. Find the new dir in the tree.
    //    Expand the parent if needed.
    // 3. Put it into the rename mode.     
    EVAL(SUCCEEDED(SetSelectedItem(pidlNewFolder, FALSE, FALSE, 0)));

    if (htiNewFolder == NULL) 
    {
        pidlParent = ILClone(pidlNewFolder);
        ILRemoveLastID(pidlParent);
        HTREEITEM htiParent = _FindFromRoot(NULL, pidlParent);


        // We are looking for the parent folder. If this is NOT
        // the root, then we need to expand it to show it.

        // NOTE: If it is root, Tree view is pretty stupid and will
        // try and deref TVI_ROOT and faults.
        if (htiParent != TVI_ROOT)
        {
            // Try expanding the parent and finding again.
            CheckForExpandOnce(_hwndTree, htiParent);
            TreeView_SelectItem(_hwndTree, htiParent);
            TreeView_Expand(_hwndTree, htiParent, TVE_EXPAND);
        }
        
        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    }

    if (htiNewFolder == NULL) 
    {
        // Something went very wrong here. We are not able to find newly added node.
        // One last try after refreshing the entire tree. (slow)
        // May be we didn't get notification.
        Refresh();

        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
        if(htiNewFolder && (htiNewFolder != TVI_ROOT))
        {
            HTREEITEM htiParent = _FindFromRoot(NULL, pidlParent);

            // We are looking for the parent folder. If this is NOT
            // the root, then we need to expand it to show it.

            // NOTE: If it is root, Tree view is pretty stupid and will
            // try and deref TVI_ROOT and faults.
            if (htiParent != TVI_ROOT)
            {
                CheckForExpandOnce(_hwndTree, htiParent);
                TreeView_SelectItem(_hwndTree, htiParent);
                TreeView_Expand(_hwndTree, htiParent, TVE_EXPAND);
            }
        }

        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    }

    // Put Edit label on the item for possible renaming by user.
    if (htiNewFolder) 
    {
        _fRclick = TRUE;  //otherwise label editing is canceled
        TreeView_EditLabel(_hwndTree, htiNewFolder);
        _fRclick = FALSE;
    }

    if (pidlParent)
        ILFree(pidlParent);

    return hres;
}


HRESULT CNscTree::_OnSHNotifyCreate(LPCITEMIDLIST pidl, int iPosition, HTREEITEM htiParent)
{
    HRESULT   hres = S_OK;
    HTREEITEM hti = NULL;
    
    if (ILIsParent(_pidlRoot, pidl, FALSE))
    {
        LPITEMIDLIST pidlParent = ILCloneParent(pidl);
        if (pidlParent)
        {
            hti = _FindFromRoot(NULL, pidlParent);
            ILFree(pidlParent);
        }

        if (hti)
        {   // folder exists in tree, if item expanded, load the node, else bag out.
            TV_ITEM tvi;
            if (hti != TVI_ROOT)
            {
                tvi.mask = TVIF_STATE | TVIF_CHILDREN;
                tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                tvi.hItem = (HTREEITEM)hti;
                
                if(!TreeView_GetItem(_hwndTree, &tvi))
                    return hres;
                
                // If we drag and item over to a node which has never beem expanded
                // before we will always fail to add the new node.
                if( !(tvi.state & TVIS_EXPANDEDONCE) ) 
                {
                    CheckForExpandOnce( _hwndTree, hti );
                    
                    tvi.mask = TVIF_STATE;
                    tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                    tvi.hItem = (HTREEITEM)hti;

                    // We need to reset this. This is causing some weird behaviour during drag and drop.
                    _fAsyncDrop = FALSE;
                    
                    if (!TreeView_GetItem(_hwndTree, &tvi))
                        return hres;
                }
            }
            else
                tvi.state = (TVIS_EXPANDEDONCE);    // evil root is always expanded.
            
            if (tvi.state & TVIS_EXPANDEDONCE)
            {
                LPCITEMIDLIST   pidlChild;
                IShellFolder    *psf;
                hres = _ParentFromItem(pidl, &psf, &pidlChild);
                if (_fAsyncDrop)    // inserted via drag/drop
                {
                    int iNewPos =   _fInsertBefore ? (_iDragDest - 1) : _iDragDest;
                    LPITEMIDLIST pidlReal;
                    if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlChild, &pidlReal)))
                    {
                        if (_MoveNode(_hwndTree, _iDragSrc, iNewPos, pidlReal))
                        {
                            TraceMsg(TF_NSC, "NSCBand:  Reordering Item");
                            _fDropping = TRUE;
                            _Dropped();
                            _fAsyncDrop = FALSE;
                            _fDropping = FALSE;
                        }
                    }
                    _htiCur = NULL;
                    _fDragging = _fInserting = _fDropping = FALSE;
                    _iDragDest = _iDragSrc = -1;
                }
                else   // standard shell notify create or drop with no insert, rename.
                {
                    if (SUCCEEDED(hres))
                    {
                        if (_iDragDest >= 0)
                            iPosition = _iDragDest;
                        hres = _InsertChild(hti, psf, pidlChild, BOOLIFY(tvi.state & TVIS_SELECTED), iPosition);
                        if (_iDragDest >= 0 &&
                            SUCCEEDED(_PopulateOrderList(hti)))
                        {
                            _fDropping = TRUE;
                            _Dropped();
                            _fDropping = FALSE;
                        }

                        psf->Release();
                    }
                }  
            }
        }
    }

    //if the item is being moved from a folder and we have it's position, we need to fix up the order in the old folder
    if (iPosition >= 0) //htiParent && (htiParent != hti) && 
    {
        //item was deleted, need to fixup order info
        _ReorderChildren(htiParent);
    }

    _UpdateActiveBorder(_htiActiveBorder);
    return hres;
}

//BUGBUG make this void
HRESULT CNscTree::_OnDeleteItem(NM_TREEVIEW *pnm)
{
    if (_htiActiveBorder == pnm->itemOld.hItem)
        _htiActiveBorder = NULL;

    ITEMINFO *  pii = (ITEMINFO *) pnm->itemOld.lParam;
    pnm->itemOld.lParam = NULL;

//    TraceMsg(TF_ALWAYS, "Nsc_Del(htiItem=%#08lx, pii=%#08lx)", pnm->itemOld.hItem, pii);

    OrderItem_Free(pii->poi, TRUE);
    LocalFree(pii);

    return S_OK;
}

void CNscTree::_GetDefaultIconIndex(LPCITEMIDLIST pidl, ULONG ulAttrs, TVITEM *pitem, BOOL fFolder)
{
    if (_iDefaultFavoriteIcon == 0)
    {
        WCHAR psz[MAX_PATH];
        DWORD cbSize = ARRAYSIZE(psz);
        int iTemp = 0;

        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, L"InternetShortcut\\DefaultIcon", NULL, NULL, (LPBYTE)&psz, &cbSize))
        {
            iTemp = PathParseIconLocation(psz);
        }

        _iDefaultFavoriteIcon = Shell_GetCachedImageIndex(psz, iTemp, 0);

        cbSize = ARRAYSIZE(psz);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, L"Folder\\DefaultIcon", NULL, NULL, (LPBYTE)&psz, &cbSize))
            iTemp = PathParseIconLocation(psz);

        _iDefaultFolderIcon = Shell_GetCachedImageIndex(psz, iTemp, 0);
    }

    pitem->iImage = pitem->iSelectedImage = (fFolder) ? _iDefaultFolderIcon : _iDefaultFavoriteIcon;
}

BOOL CNscTree::_LoadOrder(HTREEITEM hti, LPCITEMIDLIST pidl, IShellFolder* psf, HDPA* phdpa)
{
    BOOL fOrdered = FALSE;

    // there is no SFGAO_ bit that includes non folders so we need to enum
    // if we are showing non folders we have to do an enum to peek down at items below
    if (_hdpaOrd == NULL)
        _hdpaOrd = DPA_Create(2);
    
    if (_hdpaOrd)
    {
        HDPA hdpaOrder = NULL;

        IStream *pstm = GetOrderStream(pidl, STGM_READ);
        if (pstm)
        {
            OrderList_LoadFromStream(pstm, &hdpaOrder, psf);
            pstm->Release();
        }
        if ((hdpaOrder == NULL) || (DPA_GetPtrCount(hdpaOrder) == 0))
            fOrdered = FALSE;
        else
            fOrdered = TRUE;

        //set the tree item's ordered flag
        PORDERITEM poi;
        if (hti == TVI_ROOT)
        {
            _fOrdered = TRUE;
        }
        else if ((poi = _GetTreeOrderItem(hti)) != NULL)
        {
            poi->lParam = fOrdered;
        }

        *phdpa = hdpaOrder;
    }

    return fOrdered;
}

// load shell folder and deal with persisted ordering.
HRESULT CNscTree::_LoadSF(HTREEITEM htiRoot, LPCITEMIDLIST pidl, BOOL fAddingOnly, int * pcAdded, BOOL * pfOrdered)
{
    ASSERT(pcAdded);
    ASSERT(pfOrdered);
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Loading the Shell Folder for"));
#endif

    if (_CacheShellFolder(htiRoot))
    {    
        HRESULT hres;
        HDPA hdpaOrder = NULL;
        IShellFolder *psfItem = _psfCache;

        psfItem->AddRef();  // hang on as adding items may change the cached psfCache
    
        *pfOrdered = _LoadOrder(htiRoot, pidl, psfItem, &hdpaOrder);
        if (_hdpaOrd)
        {
            IEnumIDList *penum;
            hres = _GetEnum(psfItem, pidl, &penum);
            if (S_OK == hres)
            {
                ULONG celt;
                LPITEMIDLIST pidlTemp;
                while (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
                {
                    if (_ShouldShow(_psfCache, pidl, pidlTemp))
                    {
                        if (!OrderList_Append(_hdpaOrd, pidlTemp, -1))
                        {
                            ILFree(pidlTemp);
                            penum->Release();
                            goto LGone;
                        }
                    }
                    else
                        ILFree(pidlTemp);
                }
                penum->Release();
            }
            else
                hres = S_FALSE;
            
            ORDERINFO oinfo;
            oinfo.psf = psfItem;
            oinfo.dwSortBy = OI_SORTBYNAME; // merge depends on by name.
            if (*pfOrdered)
                OrderList_Merge(_hdpaOrd, hdpaOrder,  -1, (LPARAM)&oinfo, NULL, NULL);
            oinfo.dwSortBy = (*pfOrdered ? OI_SORTBYORDINAL : OI_SORTBYNAME);
            
            DPA_Sort(_hdpaOrd, OrderItem_Compare, (LPARAM)&oinfo);
            
            OrderList_Reorder(_hdpaOrd);

            BOOL fParentMarked = _IsMarked(htiRoot);
            BOOL fCheckForDups = !fAddingOnly;
            
            // load merged item list into tree.
            *pcAdded = DPA_GetPtrCount(_hdpaOrd);
            for (int i = 0; i < *pcAdded; i++)
            {
                DWORD dwAttrib = SFGAO_HASSUBFOLDER;
                PORDERITEM pitoi = (PORDERITEM)DPA_FastGetPtr(_hdpaOrd, i);
                if (pitoi == NULL)
                    break;
                
                psfItem->GetAttributesOf(1, (LPCITEMIDLIST*)&pitoi->pidl, &dwAttrib);
                    
                // If this is a normal NSC, we need to display the plus sign correctly.
                HTREEITEM hti = _AddItemToTree(htiRoot, pitoi->pidl, 
                                               (MODE_NORMAL != _mode || dwAttrib & SFGAO_HASSUBFOLDER)? 1 : 0, 
                                               pitoi->nOrder, TVI_LAST, fCheckForDups, fParentMarked);
                if (hti == NULL)
                    break;
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
LGone:
        OrderList_Destroy(&_hdpaOrd, FALSE);
        OrderList_Destroy(&hdpaOrder, TRUE);        // calls DPA_Destroy(hdpaOrder)
        psfItem->Release();
        
        return hres;
    }
    return S_FALSE;
}

// review chrisny:  get rid of this function.
int CNscTree::_GetChildren(IShellFolder *psf, LPCITEMIDLIST pidl, ULONG ulAttrs)
{
    int cChildren = 0;  // assume none
    
    if (ulAttrs & SFGAO_FOLDER)
    {
        if (_grfFlags & SHCONTF_NONFOLDERS)
        {
            // there is no SFGAO_ bit that includes non folders so we need to enum
            IShellFolder *psfItem;
            if (SUCCEEDED(psf->BindToObject(pidl, NULL, IID_IShellFolder, (void **)&psfItem)))
            {
                // if we are showing non folders we have to do an enum to peek down at items below
                IEnumIDList *penum;
                if (S_OK == _GetEnum(psfItem, NULL, &penum))
                {
                    ULONG celt;
                    LPITEMIDLIST pidlTemp;
                    
                    if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
                    {
                        if (_ShouldShow(psfItem, NULL, pidlTemp))
                        {
                            cChildren = 1;
                        }

                        ILFree(pidlTemp);
                    }
                    penum->Release();
                }
                psfItem->Release();
            }
        }
        else
        {
            // if just folders we can peek at the attributes
            ULONG ulAttrs = SFGAO_HASSUBFOLDER;
            psf->GetAttributesOf(1, &pidl, &ulAttrs);
            
            cChildren = (ulAttrs & SFGAO_HASSUBFOLDER) ? 1 : 0;
        }
    }
    return cChildren;
}

void CNscTree::_OnGetDisplayInfo(TV_DISPINFO *pnm)
{
    PORDERITEM poi = GetPoi(pnm->item.lParam);
    LPCITEMIDLIST pidl = _CacheParentShellFolder(pnm->item.hItem, poi->pidl);
    ASSERT(pidl);
    if (pidl == NULL)
        return;
    ASSERT(_psfCache);
    ASSERT(pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN));
    if (pnm->item.mask & TVIF_TEXT)
    {
        SHELLDETAILS details;
        if (SUCCEEDED(_GetDisplayNameOf(_psfCache, pidl, SHGDN_INFOLDER, &details)))
            StrRetToBuf(&details.str, pidl, pnm->item.pszText, pnm->item.cchTextMax);
    }
    // make sure we set the attributes for those flags that need them
    if (pnm->item.mask & (TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE))
    {
        ULONG ulAttrs = SFGAO_FOLDER | SFGAO_NEWCONTENT;
        _psfCache->GetAttributesOf(1, &pidl, &ulAttrs);
        // review chrisny:  still need to handle notify of changes from
        //  other navs.
        
        // HACKHACK!!!  we're using the TVIS_FOCUSED bit to stored whether there's
        // new content or not. 
        if (ulAttrs & SFGAO_NEWCONTENT)
        {
            pnm->item.mask |= TVIF_STATE;
            pnm->item.stateMask = TVIS_FOCUSED;  // init state mask to bold
            pnm->item.state = TVIS_FOCUSED;  // init state mask to bold
        }
        // Also see if this guy has any child folders
        if (pnm->item.mask & TVIF_CHILDREN)
            pnm->item.cChildren = _GetChildren(_psfCache, pidl, ulAttrs);
        
        if (pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE))
            // We now need to map the item into the right image index.
            _GetDefaultIconIndex(pidl, ulAttrs, &pnm->item, (ulAttrs & SFGAO_FOLDER));

        _UpdateItemDisplayInfo(pnm->item.hItem);
    }
    // force the treeview to store this so we don't get called back again
    pnm->item.mask |= TVIF_DI_SETITEM;
}

#define SZ_CUTA                 "cut"
#define SZ_CUT                  TEXT(SZ_CUTA)
#define SZ_RENAMEA              "rename"
#define SZ_RENAME               TEXT(SZ_RENAMEA)

void CNscTree::_ApplyCmd(HTREEITEM hti, IContextMenu *pcm, UINT idCmd)
{
    TCHAR szCommandString[40];
    BOOL fHandled = FALSE;
    BOOL fCutting = FALSE;
    
    // We need to special case the rename command
    if (SUCCEEDED(ContextMenu_GetCommandStringVerb(pcm, idCmd, szCommandString, ARRAYSIZE(szCommandString))))
    {
        if (StrCmpI(szCommandString, SZ_RENAME)==0) 
        {
            TreeView_EditLabel(_hwndTree, hti);
            fHandled = TRUE;
        } 
        else if (!StrCmpI(szCommandString, SZ_CUT)) 
        {
            fCutting = TRUE;
        }
    }
    
    if (!fHandled)
    {
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
                0L,
                _hwndTree,
                MAKEINTRESOURCEA(idCmd),
                NULL, NULL,
                SW_NORMAL,
        };
        
        HRESULT hres = pcm->InvokeCommand(&ici);
        if (fCutting && SUCCEEDED(hres))
        {
            TV_ITEM tvi;
            tvi.mask = TVIF_STATE;
            tvi.stateMask = TVIS_CUT;
            tvi.state = TVIS_CUT;
            tvi.hItem = hti;
            TreeView_SetItem(_hwndTree, &tvi);
            
            // _hwndNextViewer = SetClipboardViewer(_hwndTree);
            // _htiCut = hti;
        }
        
        //hack to force a selection update, so oc can update it's status text
        if (_mode & MODE_CONTROL)
        {
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            
            SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);
            TreeView_SelectItem(_hwndTree, NULL);
            
            //only select the item if the handle is still valid
            if (hti)
                TreeView_SelectItem(_hwndTree, hti);
            SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);
        }
    }
}


// perform actions like they were chosen from the context menu, but without showing the menu
HRESULT CNscTree::InvokeContextMenuCommand(BSTR strCommand)
{
    ASSERT(strCommand);
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    
    if (htiSelected)
    {
        if (StrCmpIW(strCommand, L"rename") == 0) 
        {
            _fRclick = TRUE;  //otherwise label editing is canceled
            TreeView_EditLabel(_hwndTree, htiSelected);
            _fRclick = FALSE;
        }
        else
        {
            LPCITEMIDLIST pidl = _CacheParentShellFolder(htiSelected, NULL);
            if (pidl)
            {
                IContextMenu *pcm;
                
                if (SUCCEEDED(_psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, IID_IContextMenu, NULL, (void **)&pcm)))
                {
                    CHAR szCommand[MAX_PATH];
                    SHUnicodeToAnsi(strCommand, szCommand, ARRAYSIZE(szCommand));
                    
                    // QueryContextMenu, even though unused, initializes the folder properly (fixes delete subscription problems)
                    HMENU hmenu = CreatePopupMenu();
                    if (hmenu)
                        pcm->QueryContextMenu(hmenu, 0, 0, 0x7fff, CMF_NORMAL);

                    /* Need to try twice, in case callee is ANSI-only */
                    CMINVOKECOMMANDINFOEX ici = 
                    {
                        CMICEXSIZE_NT4,         /* Be NT4-compat */
                        CMIC_MASK_UNICODE,
                        _hwndTree,
                        szCommand,
                        NULL, NULL,
                        SW_NORMAL,
                        0, NULL,
                        NULL,
                        strCommand,
                        NULL, NULL,
                        NULL,
                    };
                    
                    HRESULT hres = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    if (hres == E_INVALIDARG) 
                    {
                        // Recipient didn't like the unicode command; send an ANSI one
                        ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                        ici.fMask &= ~CMIC_MASK_UNICODE;
                        pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    }

                    // do any visuals for cut state
                    if (SUCCEEDED(hres) && StrCmpIW(strCommand, L"cut") == 0) 
                    {
                        HTREEITEM hti = TreeView_GetSelection(_hwndTree);
                        if (hti) 
                        {
                            _TreeSetItemState(hti, TVIS_CUT, TVIS_CUT);
                            ASSERT(!_hwndNextViewer);
                            _hwndNextViewer = SetClipboardViewer(_hwndTree);
                            _htiCut = hti;
                        }
                    }
                    if (hmenu)
                        DestroyMenu(hmenu);
                    pcm->Release();
                }
            }
        }
        
        //if properties was invoked, who knows what might have changed, so force a reselect
        if (StrCmpNW(strCommand, L"properties", 10) == 0)
        {
            TreeView_SelectItem(_hwndTree, htiSelected);
        }
    }

    return S_OK;
}

//
//  pcm = IContextMenu for the item the user selected
//  hti = the item the user selected
//
//  Okay, this menu thing is kind of funky.
//
//  If "Favorites", then everybody gets "Create new folder".
//
//  If expandable:
//      Show "Expand" or "Collapse"
//      (accordingly) and set it as the default.
//
//  If not expandable:
//      The default menu of the underlying context menu is
//      used as the default; or use the first item if nobody
//      picked a default.
//
//      We replace the existing "Open" command with our own.
//

HMENU CNscTree::_CreateContextMenu(IContextMenu *pcm, HTREEITEM hti)
{
    BOOL fExpandable = _IsExpandable(hti);
    HRESULT hres;

    HMENU hmenu = CreatePopupMenu();

    if (hmenu)
    {
        pcm->QueryContextMenu(hmenu, 0, idCmdStart, 0x7fff,
            CMF_EXPLORE | CMF_CANRENAME);

        //
        //  Always delete "Create shortcut" from the context menu.
        //
        //  Sometimes we need to delete "Open":
        //
        //  History mode always.  The context menu for history mode folders
        //  has "Open" but it doesn't work, so we need to replace it with
        //  Expand/Collapse.  And the context menu for history mode items
        //  has "Open" but it opens in a new window.  We want to navigate.
        //
        //  Favorites mode, expandable:  Leave "Open" alone -- it will open
        //  the expandable thing in a new window.
        //
        //  Favorites mode, non-expandable: Delete the original "Open" and
        //  replace it with ours that does a navigate.
        //

        BOOL fReplaceOpen = FALSE;
        if (_mode & MODE_HISTORY)
            fReplaceOpen = TRUE;
        else if (!fExpandable && (_mode & MODE_FAVORITES))
            fReplaceOpen = TRUE;

        UINT ilast = GetMenuItemCount(hmenu);
        for (UINT ipos = 0; ipos < ilast; ipos++)
        {
            UINT idCmd = GetMenuItemID(hmenu, ipos);

            if (idCmd >= idCmdStart && idCmd <= 0x7fff)
            {
                TCHAR szVerb[40];
                hres = ContextMenu_GetCommandStringVerb(pcm, idCmd-idCmdStart, szVerb, ARRAYSIZE(szVerb));

                if (SUCCEEDED(hres))
                {
                    if ((fReplaceOpen && 0 == StrCmpI(szVerb, TEXT("open"))) ||
                                         0 == StrCmpI(szVerb, TEXT("link")))
                        DeleteMenu(hmenu, idCmd, MF_BYCOMMAND );
                }
            }
        }

        // Load the NSC part of the context menu and part on it separately.
        // By doing this, we save the trouble of having to do a SHPrettyMenu
        // after we dork it -- Shell_MergeMenus does all the prettying
        // automatically.

        HMENU hmenuctx = LoadMenuPopup_PrivateNoMungeW(POPUP_CONTEXT_NSC);

        if (hmenuctx)
        {

            // create new folder doesn't make sense outside of favorites
            // (actually, it does, but there's no interface to it)
            if (!(_mode & MODE_FAVORITES))
                DeleteMenu(hmenuctx, RSVIDM_NEWFOLDER, MF_BYCOMMAND);

            //
            //  Of "Expand", "Collapse", or "Open", we will keep at most one of
            //  them.  idmKeep is the one we choose to keep.
            //
            UINT idmKeep;

            if (fExpandable)
            {
                // Even if the item has no children, we still show Expand.
                // The reason is that an item that has never been expanded
                // is marked as "children: unknown" so we show an Expand
                // and then the user picks it and nothing expands.  And then
                // the user clicks it again and the Expand option is gone!
                // (Because the second time, we know that the item isn't
                // expandable.)
                //
                // Better to be consistently wrong than randomly wrong.
                //
                if (_IsItemExpanded(hti))
                    idmKeep = RSVIDM_COLLAPSE;
                else
                    idmKeep = RSVIDM_EXPAND;


            }
            else if (!(_mode & MODE_CONTROL))
            {
                idmKeep = RSVIDM_OPEN;
            }
            else
            {
                idmKeep = 0;
            }

            //
            //  Now go decide which of RSVIDM_COLLAPSE, RSVIDM_EXPAND, or
            //  RSVIDM_OPEN we want to keep.
            //
            if (idmKeep != RSVIDM_EXPAND)
                DeleteMenu(hmenuctx, RSVIDM_EXPAND,   MF_BYCOMMAND);
            if (idmKeep != RSVIDM_COLLAPSE)
                DeleteMenu(hmenuctx, RSVIDM_COLLAPSE, MF_BYCOMMAND);
            if (idmKeep != RSVIDM_OPEN)
                DeleteMenu(hmenuctx, RSVIDM_OPEN,     MF_BYCOMMAND);

            Shell_MergeMenus(hmenu, hmenuctx, 0, 0, 0xFFFF, fReplaceOpen ? 0 : MM_ADDSEPARATOR);

            DestroyMenu(hmenuctx);

            if (idmKeep)
                SetMenuDefaultItem(hmenu, idmKeep, MF_BYCOMMAND);
        }
        _SHPrettyMenu(hmenu);
    }
    return hmenu;
}

LRESULT CNscTree::_OnContextMenu(short x, short y)
{
    HTREEITEM hti;
    POINT ptPopup;  // in screen coordinate

    //assert that the SetFocus() below won't be ripping focus away from anyone
    ASSERT((_mode & MODE_CONTROL) ? (GetFocus() == _hwndTree) : TRUE);

    if (x == -1 && y == -1)
    {
        // Keyboard-driven: Get the popup position from the selected item.
        hti = TreeView_GetSelection(_hwndTree);
        if (hti)
        {
            RECT rc;
            //
            // Note that TV_GetItemRect returns it in client coordinate!
            //
            TreeView_GetItemRect(_hwndTree, hti, &rc, TRUE);
            ptPopup.x = (rc.left + rc.right) / 2;
            ptPopup.y = (rc.top + rc.bottom) / 2;
            MapWindowPoints(_hwndTree, HWND_DESKTOP, &ptPopup, 1);
        }
        //so we can go into rename mode
        _fRclick = TRUE;
    }
    else
    {
        TV_HITTESTINFO tvht;

        // Mouse-driven: Pick the treeitem from the position.
        ptPopup.x = x;
        ptPopup.y = y;

        tvht.pt = ptPopup;
        ScreenToClient(_hwndTree, &tvht.pt);

        hti = TreeView_HitTest(_hwndTree, &tvht);
    }

    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(hti, NULL);
        if (pidl)
        {
            IContextMenu *pcm;

            TreeView_SelectDropTarget(_hwndTree, hti);

            if (SUCCEEDED(_psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, IID_IContextMenu, NULL, (void **)&pcm)))
            {
                pcm->QueryInterface(IID_IContextMenu2, (void **)&_pcmSendTo);

                HMENU hmenu = _CreateContextMenu(pcm, hti);
                if (hmenu)
                {
                    UINT idCmd;

                    _pcm = pcm; // for IContextMenu2 code

                    // use _hwnd so menu msgs go there and I can forward them
                    // using IContextMenu2 so "Sent To" works

                    // review chrisny:  useTrackPopupMenuEx for clipping etc.  
                    idCmd = TrackPopupMenu(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        ptPopup.x, ptPopup.y, 0, _hwndTree, NULL);
                    // Note:  must requery selected item to verify that the hti is good.  This
                    // solves the problem where the hti was deleted, hence pointed to something
                    // bogus, then we write to it causing heap corruption, while the menu was up.  
                    TV_HITTESTINFO tvht;
                    tvht.pt = ptPopup;
                    ScreenToClient(_hwndTree, &tvht.pt);
                    hti = TreeView_HitTest(_hwndTree, &tvht);
                    if (hti && idCmd)
                    {
                        switch (idCmd)
                        {
                        case RSVIDM_OPEN:
                        case RSVIDM_EXPAND:
                        case RSVIDM_COLLAPSE:
                            TreeView_SelectItem(_hwndTree, hti);
                            //  turn off flag, so select will have an effect.
                            _fRclick = FALSE;
                            _OnSelChange();     // selection has changed, force the navigation.
                            //  SelectItem may not expand (if was closed and selected)
                            TreeView_Expand(_hwndTree, hti, idCmd == RSVIDM_COLLAPSE ? TVE_COLLAPSE : TVE_EXPAND);
                            break;

                        // This WAS unix only, now win32 does it too
                        // IEUNIX : We allow new folder creation from context menu. since
                        // this control was used to organize favorites in IEUNIX4.0
                        case RSVIDM_NEWFOLDER:
                            CreateNewFolder(hti);
                            break;

                        default:
                            _ApplyCmd(hti, pcm, idCmd-idCmdStart);
                            break;
                        }

                        //we must have had focus before (asserted above), but we might have lost it after a delete.
                        //get it back.
                        //this is only a problem in the nsc oc.
                        if ((_mode & MODE_CONTROL) && !_fInLabelEdit)
                            SetFocus(_hwndTree);
                    }
                    ATOMICRELEASE(_pcmSendTo);
                    DestroyMenu(hmenu);
                    _pcm = NULL;
                }
                pcm->Release();
            }
            TreeView_SelectDropTarget(_hwndTree, NULL);
        }
    }

    if (x == -1 && y == -1)
        _fRclick = FALSE;

    return S_FALSE;       // So WM_CONTEXTMENU message will not come.
}


HRESULT CNscTree::_QuerySelection(IContextMenu **ppcm, HTREEITEM *phti)
{
    HRESULT hres = E_FAIL;
    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(hti, NULL);
        if (pidl)
        {
            if (ppcm)
            {
                hres = _psfCache->GetUIObjectOf(_hwndTree, 
                    1, &pidl, IID_IContextMenu, NULL, (void **)ppcm);
            }
            else
            {
                hres = S_OK;
            }
        }
    }
    
    if (phti)
        *phti = hti;
    
    return hres;
}

LRESULT NSCEditBoxSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uIdSubclass == ID_NSC_SUBCLASS && uMsg == WM_GETDLGCODE)
    {
        return DLGC_WANTMESSAGE;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CNscTree::_OnBeginLabelEdit(TV_DISPINFO *ptvdi)
{
    BOOL fCantRename = TRUE;
    
    LPCITEMIDLIST pidl = _CacheParentShellFolder(ptvdi->item.hItem, NULL);
    if (pidl)
    {
        DWORD dwAttribs = SFGAO_CANRENAME;
        _psfCache->GetAttributesOf(1, &pidl, &dwAttribs);
        if (dwAttribs & SFGAO_CANRENAME)
            fCantRename = FALSE;
    }

    HWND hwndEdit = (HWND)SendMessage(_hwndTree, TVM_GETEDITCONTROL, 0, 0);
    if (hwndEdit)
    {
        SetWindowSubclass(hwndEdit, NSCEditBoxSubclassWndProc, ID_NSC_SUBCLASS, NULL);
    }
    
    _fInLabelEdit = !fCantRename;
    if (_fInLabelEdit)
        _htiRenaming = ptvdi->item.hItem;
        
    return fCantRename;
}

LRESULT CNscTree::_OnEndLabelEdit(TV_DISPINFO *ptvdi)
{

    HWND hwndEdit = (HWND)SendMessage(_hwndTree, TVM_GETEDITCONTROL, 0, 0);
    if (hwndEdit)
    {
        RemoveWindowSubclass(hwndEdit, NSCEditBoxSubclassWndProc, ID_NSC_SUBCLASS);
    }

#ifdef UNIX
    // IEUNIX (BUGBUG): If we lose activation in the  middle of rename operation
    // and we have invalid name in the   edit box, rename operation will popup 
    // a message box which causes IE on unix to go into infinite focus changing
    // loop. To workaround this problem, we are considering the operation as 
    // cancelled and we copy the original value into the buffer.

    BOOL fHasActivation = FALSE;

    if(GetActiveWindow() && IsChild(GetActiveWindow(), _hwndTree))
        fHasActivation = TRUE;

    if (!fHasActivation)
    {
       
        TV_ITEM tvi;
        tvi.mask = TVIF_TEXT;
        tvi.hItem = (HTREEITEM)ptvdi->item.hItem;
        tvi.pszText = ptvdi->item.pszText;  
        tvi.cchTextMax = ptvdi->item.cchTextMax;
        TreeView_GetItem(_hwndTree, &tvi);
    }
#endif

    if (ptvdi->item.pszText != NULL)
    {
        ASSERT(ptvdi->item.hItem);
        
        LPCITEMIDLIST pidl = _CacheParentShellFolder(ptvdi->item.hItem, NULL);
        if (pidl)
        {
            WCHAR wszName[MAX_PATH - 5]; //-5 to work around nt4 shell32 bug
            SHTCharToUnicode(ptvdi->item.pszText, wszName, ARRAYSIZE(wszName));
            
            if (SUCCEEDED(_psfCache->SetNameOf(_hwndTree, pidl, wszName, 0, NULL)))
            {
                // NOTES: pidl is no longer valid here.
                
                // Set the handle to NULL in the notification to let
                // the system know that the pointer is probably not
                // valid anymore.
                ptvdi->item.hItem = NULL;
                _FlushNotifyMessages(_hwndTree);    // do this last, else we get bad results
                _fInLabelEdit = FALSE;
#ifdef UNIX
                SHChangeNotifyHandleEvents();
#endif
            }
            else
            {
                // not leaving label edit mode here, so do not set _fInLabelEdit to FALSE or we
                // will not get ::TranslateAcceleratorIO() and backspace, etc, will not work.
                _fRclick = TRUE;  //otherwise label editing is canceled
                SendMessage(_hwndTree, TVM_EDITLABEL, (WPARAM)ptvdi->item.pszText, (LPARAM)ptvdi->item.hItem);
                _fRclick = FALSE;
            }
        }
    }
    else
        _fInLabelEdit = FALSE;

    if (!_fInLabelEdit)
        _htiRenaming = NULL;
        
    //else user cancelled, nothing to do here.
    return 0;   // We always return 0, "we handled it".
}
    
    
void CNscTree::_OnBeginDrag(NM_TREEVIEW *pnmhdr)
{
    LPCITEMIDLIST pidl = _CacheParentShellFolder(pnmhdr->itemNew.hItem, NULL);
    _htiDragging = pnmhdr->itemNew.hItem;   // item we are dragging.
    if (pidl)
    {
        if (_pidlDrag)
        {
            ILFree(_pidlDrag);
            _pidlDrag = NULL;
        }

        DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
        
        _psfCache->GetAttributesOf(1, &pidl, &dwEffect);
        
        dwEffect &= DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
        
        if (dwEffect)
        {
            IDataObject *pdtobj;
            HRESULT hres = _psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, 
                IID_IDataObject, NULL, (void **)&pdtobj);
            if (SUCCEEDED(hres))
            {
                HWND hwndTT;
                
                _fDragging = TRUE;
                if (hwndTT = TreeView_GetToolTips(_hwndTree))
                    SendMessage(hwndTT, TTM_POP, (WPARAM) 0, (LPARAM) 0);
                PORDERITEM poi = _GetTreeOrderItem(pnmhdr->itemNew.hItem);
                if (poi)
                {
                    _iDragSrc = poi->nOrder;
                    TraceMsg(TF_NSC, "NSCBand: Starting Drag");
                    _pidlDrag = ILClone(poi->pidl);
                    _htiFolderStart = TreeView_GetParent(_hwndTree, pnmhdr->itemNew.hItem);
                    if (_htiFolderStart == NULL)
                        _htiFolderStart = TVI_ROOT;
                }
                else
                {
                    _iDragSrc = -1;
                    _pidlDrag = NULL;
                    _htiFolderStart = NULL;
                }

                //
                // Don't allow drag and drop of channels if
                // REST_NoRemovingChannels is set.
                //
                if (!SHRestricted2(REST_NoRemovingChannels, NULL, 0) ||
                    !_IsChannelFolder(_htiDragging))
                {
                    HIMAGELIST himlDrag;
                
                    SHLoadOLE(SHELLNOTIFY_OLELOADED); // Browser Only - our shell32 doesn't know ole has been loaded

                    _fStartingDrag = TRUE;
                    IDragSourceHelper* pdsh = NULL;
                    if (SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IDragSourceHelper, (void**)&pdsh)))
                    {
                        pdsh->InitializeFromWindow(_hwndTree, &pnmhdr->ptDrag, pdtobj);
                        _fStartingDrag = FALSE;
                    }
                    else
                    {
                        himlDrag = TreeView_CreateDragImage(_hwndTree, pnmhdr->itemNew.hItem);
                        _fStartingDrag = FALSE;
                
                        if (himlDrag) 
                        {
                            DAD_SetDragImage(himlDrag, NULL);
                        }
                    }

                            
                    SHDoDragDrop(_hwndTree, pdtobj, NULL, dwEffect, &dwEffect);
                    // nothing happened when the d&d terminated, so clean up you fool.
                    if (dwEffect == DROPEFFECT_NONE)
                    {
                        ILFree(_pidlDrag);
                        _pidlDrag = NULL;
                    }

                    if (pdsh)
                    {
                        pdsh->Release();

                    }
                    else
                    {
                        DAD_SetDragImage((HIMAGELIST)-1, NULL);
                        ImageList_Destroy(himlDrag);
                    }
                }

                _iDragSrc = -1;
                pdtobj->Release();
            }
        }
    }
    _htiDragging = NULL;
}

BOOL CNscTree::IsExpandableChannelFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (WhichPlatform() == PLATFORM_INTEGRATED)
        return SHIsExpandableFolder(psf, pidl);

    ASSERT(pidl);
    ASSERT(psf);

    BOOL          fExpand = FALSE;
    IShellFolder* psfChannelFolder;

    if (pidl && psf && SUCCEEDED(SHBindToObject(psf, IID_IShellFolder, pidl, (void**)&psfChannelFolder)))
    {
        IEnumIDList *penum;
        if (S_OK == _GetEnum(psfChannelFolder, NULL, &penum))
        {
            ULONG celt;
            LPITEMIDLIST pidlTemp;

            if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
            {
                ILFree(pidlTemp);
                fExpand = FALSE;
            }
            if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
            {
                ILFree(pidlTemp);
                fExpand = TRUE;
            }
            penum->Release();
        }
        psfChannelFolder->Release();
    }

    return fExpand;
}


BOOL CNscTree::_OnSelChange()
{
    BOOL fExpand = TRUE;
    HTREEITEM hti = TreeView_GetSelection(_hwndTree);

    //if we're in control mode (where pswProxy always null), never navigate
    if (hti)
    {
        LPCITEMIDLIST pidlItem = _CacheParentShellFolder(hti, NULL);
        if (pidlItem)
        {
            IShellBrowser *psb;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SProxyBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
            {
                LPITEMIDLIST pidlTarget;
                ULONG ulAttrs = SFGAO_FOLDER | SFGAO_NEWCONTENT;
                HRESULT hr = SHGetNavigateTarget(_psfCache, pidlItem, &pidlTarget, &ulAttrs);
                if (SUCCEEDED(hr))
                {
                    if ( !( (ulAttrs & SFGAO_FOLDER) && IsExpandableChannelFolder(_psfCache, pidlItem)))
                    {
                        if (!_fInSelectPidl)
                        {
                            BOOL fNavigateDone = FALSE;

                            fExpand = FALSE;

                            if (_mode & MODE_FAVORITES)
                            {
                                TCHAR szPath[MAX_PATH];
                                fNavigateDone = SUCCEEDED(GetPathForItem(_psfCache, pidlItem, szPath, NULL)) &&
                                                SUCCEEDED(NavFrameWithFile(szPath, psb));
                            }

                            if (!fNavigateDone)
                                psb->BrowseObject(pidlTarget, (_dwFlags & NSS_NOHISTSELECT) ? SBSP_NOAUTOSELECT : 0);

                            // whether we succeed or not... (BUGBUG?)
                            UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_NAVIGATE, 
                                _mode==MODE_FAVORITES ? UIBL_NAVFAVS : _mode==MODE_HISTORY ? UIBL_NAVHIST : UIBL_NAVOTHER);

                            if (_mode & MODE_FAVORITES)
                            {
                                LPITEMIDLIST pidlFull = _GetFullIDList(hti);
                                if (pidlFull)
                                {
                                    UEMFireEvent(&UEMIID_BROWSER, UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)_psfCache, (LPARAM)pidlItem);
                                    SHSendChangeMenuNotify(NULL, SHCNEE_PROMOTEDITEM, 0, pidlFull);
                                    ILFree(pidlFull);
                                }
                            }
                        }
                    }
                    else    //it's a channel folder that can be expanded
                    {
                        fExpand = TRUE;
                    }

                    ILFree(pidlTarget);

                    ulAttrs |= SFGAO_FOLDER;   // don't try _InvokeCM below

                    // review chrisny:  still need to handle notify of changes from
                    //  other navs.
                    if (ulAttrs & SFGAO_NEWCONTENT)
                    {
                        TV_ITEM tvi;
                        tvi.hItem = hti;
                        tvi.mask = TVIF_STATE | TVIF_HANDLE;
                        tvi.stateMask = TVIS_FOCUSED;  // the BOLD bit is to be
                        tvi.state = 0;              // cleared
                    
                        TreeView_SetItem(_hwndTree, &tvi);
                    }
                }
                else
                {
                    ulAttrs = SFGAO_FOLDER;
                    _psfCache->GetAttributesOf(1, &pidlItem, &ulAttrs);
                }

                if ( FAILED(hr) && !(ulAttrs & SFGAO_FOLDER) )
                {
                    SHInvokeCommand(_hwndTree, _psfCache, pidlItem, NULL);
                }

                psb->Release();
            }
        }
    }

    if (!_fSingleExpand && 
        fExpand && 
        (_mode != MODE_NORMAL))
    {
        TreeView_Expand(_hwndTree, hti, TVE_TOGGLE);
    }

    _UpdateActiveBorder(hti);

    return TRUE;
}

void CNscTree::_OnGetInfoTip(NMTVGETINFOTIP* pnm)
{
    // No info tip operation on drag/drop
    if (_fDragging || _fDropping || _fClosing || _fHandlingShellNotification || _fInSelectPidl)
        return;

    PORDERITEM poi = GetPoi(pnm->lParam);
    if (poi)
    {
        LPITEMIDLIST pidl = _CacheParentShellFolder(pnm->hItem, poi->pidl);
        if (EVAL(pidl))
            GetInfoTip(_psfCache, pidl, pnm->pszText, pnm->cchTextMax);
    }
}

LRESULT CNscTree::_OnSetCursor(NMMOUSE* pnm)
{
    if (!pnm->dwItemData)
        return 0;        // ot on an item.  return.  (bogus, why am I even getting called?)

    if (!(_mode & MODE_CONTROL) && (_mode != MODE_NORMAL))
    {
        ITEMINFO* pii = GetPii(pnm->dwItemData);
        if (pii) 
        {
            if (!pii->fNavigable)
            {
                //folders always get the arrow
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            else
            {
                //favorites always get some form of the hand
                HCURSOR hCursor = pii->fGreyed ? (HCURSOR)LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_OFFLINE_HAND)) :
                                         LoadHandCursor(0);
                if (hCursor)
                    SetCursor(hCursor);
            }
        }
    }
    else
    {
        //always show the arrow in org favs
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    
    return 1; // 1 if We handled it, 0 otherwise
}

BOOL CNscTree::_IsTopParentItem(HTREEITEM hti)
{
    return (hti && (!TreeView_GetParent(_hwndTree, hti)) );
}

LRESULT CNscTree::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;
    
    switch (pnm->code) 
    {
    case NM_CUSTOMDRAW:
        return _OnCDNotify((LPNMCUSTOMDRAW)pnm);

    case TVN_GETINFOTIP:
        // no info tips on drag/drop ops
        // According to Bug#241601, Tooltips display too quickly. The problem is
        // the original designer of the InfoTips in the Treeview merged the "InfoTip" tooltip and
        // the "I'm too small to display correctly" tooltips. This is really unfortunate because you
        // cannot control the display of these tooltips independantly. Therefore we are turning off
        // infotips in normal mode.
        if (!_fInLabelEdit && _mode != MODE_NORMAL)
            _OnGetInfoTip((NMTVGETINFOTIP*)pnm);
        else 
            return FALSE;
        break;
        
    case NM_SETCURSOR:
        lres = _OnSetCursor((NMMOUSE*)pnm);
        break;
        
    case NM_SETFOCUS:
        _fHasFocus = TRUE;
        break;

    case NM_KILLFOCUS:
        {
            _fHasFocus = FALSE;

            //invalidate the item because tabbing away doesn't
            RECT rc;

            // Tree can focus and not have any items.
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            if (hti)
            {
                TreeView_GetItemRect(_hwndTree, hti, &rc, FALSE);
                //does this need to be UpdateWindow? only if focus rect gets left behind.
                InvalidateRect(_hwndTree, &rc, FALSE);
            }
        }
        break;

    case TVN_KEYDOWN:
        {
            TV_KEYDOWN *ptvkd = (TV_KEYDOWN *) pnm;
            switch (ptvkd->wVKey)
            {
            case VK_RETURN:
            case VK_SPACE:
                lres = _OnSelChange();
                break;

            case VK_DELETE:
                if ( !((_mode & MODE_HISTORY) && IsInetcplRestricted(L"History")) )
                {
                    _fIgnoreNextSelChange = TRUE;
                    InvokeContextMenuCommand(L"delete");
                }
                break;

            case VK_UP:
            case VK_DOWN:
                //VK_MENU == VK_ALT
                if ((_mode != MODE_HISTORY) && (GetKeyState(VK_MENU) < 0))
                {
                    MoveItemUpOrDown(ptvkd->wVKey == VK_UP);
                    lres = 0;
                    _fIgnoreNextSelChange = TRUE;
                }
                break;
            
            case VK_F2:
                //only do this in org favs, because the band accel handler usually processes this
                if (_mode & MODE_CONTROL)
                    InvokeContextMenuCommand(L"rename");
                break;

            default:
                break;
            }
                
            if (!_fSingleExpand)
                _UpdateActiveBorder(TreeView_GetSelection(_hwndTree));
        }
        break;

    case TVN_SELCHANGINGA:
    case TVN_SELCHANGING:
        {
            //hack because treeview keydown ALWAYS does it's default processing
            if (_fIgnoreNextSelChange)
            {
                _fIgnoreNextSelChange = FALSE;
                return TRUE;
            }

            NM_TREEVIEW * pnmtv = (NM_TREEVIEW *) pnm;

            //if it's coming from somewhere weird (like a WM_SETFOCUS), don't let it select
            return (pnmtv->action != TVC_BYKEYBOARD) && (pnmtv->action != TVC_BYMOUSE) && (pnmtv->action != TVC_UNKNOWN);
        }
        break;
        
    case TVN_SELCHANGEDA:
    case TVN_SELCHANGED:

#ifdef DEBUG
        {
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            LPITEMIDLIST pidl = _GetFullIDList(hti);
            if (pidl)
            {
                TCHAR sz[MAX_PATH];
                SHGetNameAndFlags(pidl, SHGDN_NORMAL, sz, SIZECHARS(sz), NULL);
                TraceMsg(TF_NSC, "NSCBand: Selecting %s", sz);
                //
                // On NT4 and W95 shell this call will miss the history
                // shell extension junction point.  It will then deref into
                // history pidls as if they were shell pidls.  On short
                // history pidls this would fault on a debug version of the OS.
                //
                // SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, sz, SIZECHARS(sz), NULL);
                //TraceMsg(TF_NSC, "displayname = %s", sz);
                ILFree(pidl);
            }
        }
#endif  // DEBUG
        break;

#if 0
        // no navigation when dropping or closing or prev selected in other folder.
        if (_fRefreshing || _fDropping || _fClosing)
            return TRUE;

        return (((NM_TREEVIEW *)pnm)->action == TVC_BYKEYBOARD) ? TRUE : _OnSelChange();
#endif   
    case TVN_GETDISPINFO:
        _OnGetDisplayInfo((TV_DISPINFO *)pnm);
        break;

    case TVN_ITEMEXPANDING: 
        TraceMsg(TF_NSC, "NSCBand: Expanding");
        if (!_fIgnoreNextItemExpanding)
            _OnItemExpandingMsg((LPNM_TREEVIEW)pnm);
        else
            lres = TRUE;
        break;
        
    case TVN_DELETEITEM:
        _OnDeleteItem((LPNM_TREEVIEW)pnm);
        break;
        
    case TVN_BEGINDRAG:
    case TVN_BEGINRDRAG:
        _OnBeginDrag((NM_TREEVIEW *)pnm);
        break;
        
    case TVN_BEGINLABELEDIT:

        // This really should be renamed to _fOkToRename
        if (!_fRclick)
            return 1;

        lres = _OnBeginLabelEdit((TV_DISPINFO *)pnm);

        if (_punkSite)
            IUnknown_UIActivateIO(_punkSite, TRUE, NULL);
        break;
        
    case TVN_ENDLABELEDIT:
        lres = _OnEndLabelEdit((TV_DISPINFO *)pnm);
        break;
        
    case TVN_SINGLEEXPAND:
    case NM_DBLCLK:
        break;
        
    case NM_CLICK:
    {
        //if someone clicks on the selected item, force a selection change (to force a navigate)
        DWORD dwPos = GetMessagePos();
        TV_HITTESTINFO tvht;
        HTREEITEM hti;
        tvht.pt.x = GET_X_LPARAM(dwPos);
        tvht.pt.y = GET_Y_LPARAM(dwPos);
        ScreenToClient(_hwndTree, &tvht.pt);
        hti = TreeView_HitTest(_hwndTree, &tvht);

        // But not if they click on the button, since that means that they
        // are merely expanding/contracting and not selecting
        if (hti && !(tvht.flags & TVHT_ONITEMBUTTON))
        {
            TreeView_SelectItem(_hwndTree, hti);
            _OnSelChange();
        }
        break;
    }
        
    case NM_RCLICK:
    {
        DWORD dwPos = GetMessagePos();
        _fRclick = TRUE;
        lres = _OnContextMenu(GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos));
        _fRclick = FALSE;
        break;
    }
        
    default:
        break;
    }
    return lres;
}

HRESULT CNscTree::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // review chrisny:  better error return here.
    _fHandlingShellNotification = TRUE;
    _OnChangeNotify(lEvent, pidl1, pidl2);
    _fHandlingShellNotification = FALSE;
    return S_OK;
}

void CNscTree::_SelectPidl(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert)
{
    HTREEITEM       htiParent;
    HTREEITEM       hti = NULL;
    LPITEMIDLIST    pidlItem = NULL
        , pidlParent;
    LPCITEMIDLIST   pidlTemp = NULL;
    TV_ITEM         tvi;
    IShellFolder    *psf = NULL;
    IShellFolder    *psfNext = NULL;
    HRESULT         hres = S_OK;

#ifdef DEBUG
    TracePIDLAbs(pidl, TEXT("Attempting to select"));
#endif
    
    // We need to do this so items that are rooted at the Desktop, are found 
    // correctly.
    htiParent = (_mode == MODE_NORMAL)? TreeView_GetRoot(_hwndTree) : TVI_ROOT;
    ASSERT((_hwndTree != NULL) && (pidl != NULL));

    if (_hwndTree == NULL) 
        goto LGone;

    // We should unify the "FindFromRoot" code path and this one.
    pidlParent = _pidlRoot;

    if (ILIsEmpty(pidlParent))
    {
        pidlTemp = pidl;
        SHGetDesktopFolder(&psf);
    }
    else
    {
        if ((pidlTemp = ILFindChild(pidlParent, pidl)) == NULL)
        {
            goto LGone;    // not root match, no hti
        }

        // root match, carry on . . .   
        hres = IEBindToObject(pidlParent, &psf);
    }

    if (FAILED(hres))
    {
        goto LGone;
    }
    
    while(!ILIsEmpty(pidlTemp))
    {
        if ((pidlItem = ILCloneFirst(pidlTemp)) == NULL)
            goto LGone;
        pidlTemp = _ILNext(pidlTemp);


        // Since we are selecting a pidl, we need to make sure it's parent is visible.
        // We do it this before the insert, so that we don't have to check for duplicates.
        // when enumerating NTDev it goes from about 10min to about 8 seconds.
        if (htiParent != TVI_ROOT)
        {
            // Check to see if it's expanded.
            tvi.mask = TVIF_STATE;
            tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
            tvi.hItem = (HTREEITEM)htiParent;
            if (!TreeView_GetItem(_hwndTree, &tvi))
            {
                goto LGone;
            }

            // If not, Expand it.
            if (!(tvi.state & TVIS_EXPANDED))
            {
                TreeView_Expand(_hwndTree, htiParent, TVE_EXPAND);
            }
        }
        
        // Now that we have it enumerated, check to see if the child if there.
        hti = _FindChild(psf, htiParent, pidlItem);

        // fReinsert will allow us to force the item to be reinserted
        if (hti && fReinsert) 
        {
            ASSERT(fCreate);
            TreeView_DeleteItem(_hwndTree, hti);
            hti = NULL;
        }

        // Do we have a child in the newly expanded tree?
        if (NULL == hti)
        {
            // No. We must have to create it.
            if (!fCreate)
            {
                // But, we're not allowed to... Shoot.
                goto LGone;
            }

            // Get a real pidl.
            LPITEMIDLIST pidlReal;
            if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlItem, &pidlReal)))
            {
                // Figure out if it has a submenu.
                DWORD dwAttrib = SFGAO_HASSUBFOLDER;
                psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlReal, &dwAttrib);

                // Add to the tree. NOTE: If we're in "Normal" mode, we only have the + if it has sub menus
                // but in other modes we always have the +. Why? who knows.
                hti = _AddItemToTree(htiParent, pidlReal, 
                                        (MODE_NORMAL != _mode || dwAttrib & SFGAO_HASSUBFOLDER)? 1 : 0, 
                                        DEFAULTORDERPOSITION, 
                                        TVI_LAST, TRUE, FALSE);

                // Once we add something, reorder the children.
                _ReorderChildren(htiParent);
            }

            // Unable to add?
            if (NULL == hti) 
            {
                // Cleanup and return.
                ILFree(pidlReal);
                goto LGone;
            }
        }

        // we don't need to bind if its the last one
        //   -- a half-implemented ISF might not like this bind...
        if (!ILIsEmpty(pidlTemp))
            hres = psf->BindToObject(pidlItem, NULL, IID_IShellFolder, (void **) &psfNext);

        ILFree(pidlItem);
        pidlItem = NULL;
        if (FAILED(hres))
            goto LGone;

        htiParent = hti;
        psf->Release();
        psf = psfNext;
        psfNext = NULL;
    }
LGone:
    if (hti != NULL)
    {
        TreeView_SelectItem(_hwndTree, hti);
#ifdef DEBUG
        TraceHTREE(hti, TEXT("Found"));
#endif
    }
    
    if (psf != NULL)
        psf->Release();
    if (psfNext != NULL)
        psfNext->Release();
    if (pidlItem != NULL)
        ILFree(pidlItem);
}


HRESULT CNscTree::GetSelectedItem(LPITEMIDLIST * ppidl, int nItem)
{
    HRESULT hr = E_INVALIDARG;

    // nItem will be used in the future when we support multiple selections.
    // GetSelectedItem() returns S_FALSE and (NULL == *ppidl) if not that many
    // items are selected.  Not yet implemented.
    if (nItem > 0)
    {
        *ppidl = NULL;
        return S_FALSE;
    }

    if (ppidl)
    {
        *ppidl = NULL;
        // Is the ListView still there?
        if (_fIsSelectionCached)
        {
            // No, so get the selection that was saved before
            // the listview was destroyed.
            if (_pidlSelected)
            {
                *ppidl = ILClone(_pidlSelected);
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
        else
        {
            HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);

            if (htiSelected)
            {
                *ppidl = _GetFullIDList(htiSelected);
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
    }

    return hr;
}


HRESULT CNscTree::SetSelectedItem(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert, int nItem)
{
    DWORD dwAttributes = SFGAO_VALIDATE;

    // nItem will be used in the future when we support multiple selections.
    // Not yet implemented.
    if (nItem > 0)
    {
        return S_FALSE;
    }
    
    //  Override fCreate if the object no longer exists
    fCreate = fCreate && SUCCEEDED(IEGetAttributesOf(pidl, &dwAttributes));
    
    //  We probably haven't seen the ChangeNotify yet, so we tell
    //  _SelectPidl to create any folders that are there
    //  Then select the pidl, expanding as necessary
    _fInSelectPidl = TRUE;
    _SelectPidl(pidl, fCreate, fReinsert);
    _fInSelectPidl = FALSE;

    return S_OK;
}


//***   CNscTree::IWinEventHandler
HRESULT CNscTree::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch (uMsg) 
    {
    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        return S_OK;
        
    case WM_PALETTECHANGED:
        _OnPaletteChanged(wParam, lParam);
        break;

    case WM_NCDESTROY:
        // This is not normally sent to IWinEventHandler::OnWinEvent
        SetSite(NULL);
        break;

    default:
        break;
    }
    return E_FAIL;
}


void CNscTree::_OnChangeNotify(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    switch (lEvent)
    {
    case SHCNE_RENAMEFOLDER:
    case SHCNE_RENAMEITEM:
        if (pidl && pidlExtra)
            _OnSHNotifyRename(pidl, pidlExtra);
        else
            ASSERT(FALSE);
        
        break;
        
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        if (pidl)
            _OnSHNotifyDelete(pidl, NULL, NULL);
        else
            ASSERT(FALSE);
        break;
        
        
    case SHCNE_UPDATEITEM:
        if (EVAL(pidl))
            _OnSHNotifyUpdateItem(pidl);
        break;

        //    case 0:         // review chrisny:  what the hell does 0 mean.
        //    case SHCNE_DRIVEADD:
        //    case SHCNE_DRIVEREMOVED:    // review chrisny:  this does not seem to fit with add.

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        if (pidl)
        {
            _OnSHNotifyCreate(pidl, DEFAULTORDERPOSITION, NULL);
            if (SHCNE_MKDIR == lEvent &&
                _pidlNewFolderParent &&
                ILIsParent(_pidlNewFolderParent, pidl, TRUE)) // TRUE = immediate parent only
            {
                EVAL(SUCCEEDED(_EnterNewFolderEditMode(pidl)));
            }
        }
        break;
    case SHCNE_UPDATEDIR:
        if (pidl)
            _OnSHNotifyUpdateDir(pidl);
        break;

    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_DRIVEADDGUI:
    case SHCNE_NETSHARE:
    case SHCNE_SERVERDISCONNECT:
    case SHCNE_ASSOCCHANGED:
        break;

    case SHCNE_UPDATEIMAGE:
        if (pidl) 
        {
            int iIndex;
            if (pidlExtra )
            {   // new style update image notification.....
                iIndex = SHHandleUpdateImage( pidlExtra );
                if ( iIndex == -1 )
                    break;
            }
            else
                iIndex = *(int UNALIGNED *)((BYTE*)pidl + 2);
            
            _InvalidateImageIndex(NULL, iIndex);
        }
        break;
    case SHCNE_EXTENDED_EVENT:
        {
#ifndef UNIX
            SHChangeDWORDAsIDList* pdwidl = (SHChangeDWORDAsIDList UNALIGNED *)pidl;
#else
            SHChangeDWORDAsIDList* pdwidl = (SHChangeDWORDAsIDList *)pidl;
#endif
            
            INT_PTR iEvent = pdwidl->dwItem1;

            switch (iEvent)
            {
            case SHCNEE_ORDERCHANGED:
                if (EVAL(pidl))
                {
                    if (_fDropping ||                           // If WE are dropping.
                        _fInLabelEdit ||                        // We're editing a name (Kicks us out)
                        SHChangeMenuWasSentByMe(this, pidl)  || // Ignore if we sent it.
                        (_mode == MODE_HISTORY))                // Always ignore history changes
                    {
                        TraceMsg(TF_NSC, "NSCBand: Ignoring Change Notify: We sent");
                        //ignore the notification                    
                    }
                    else
                    {
                        TraceMsg(TF_BAND, "NSCBand: OnChange SHCNEE_ORDERCHANGED accepted");

                        HTREEITEM htiRoot = _FindFromRoot(TVI_ROOT, pidlExtra);
                        if (htiRoot != NULL)
                            _UpdateDir(htiRoot);
                    }
                }
                break;
            case SHCNEE_WININETCHANGED:
                {
                    if (pdwidl->dwItem2 & (CACHE_NOTIFY_SET_ONLINE | CACHE_NOTIFY_SET_OFFLINE))
                    {
                        BOOL fOnline = !SHIsGlobalOffline();
                        if ((fOnline && !_fOnline) || (!fOnline && _fOnline))
                        {
                            // State changed
                            _fOnline = fOnline;
                            _OnSHNotifyOnlineChange(TVI_ROOT, _fOnline);
                        }
                    }
                    
                    if (pdwidl->dwItem2 & (CACHE_NOTIFY_ADD_URL |
                        CACHE_NOTIFY_DELETE_URL |   
                        CACHE_NOTIFY_DELETE_ALL |
                        CACHE_NOTIFY_URL_SET_STICKY |
                        CACHE_NOTIFY_URL_UNSET_STICKY))
                    {
                        // Something in the cache changed
                        _OnSHNotifyCacheChange(TVI_ROOT, pdwidl->dwItem2);
                    }
                    break;
                }
            }
            break;
        }
        break;
    }
    return;
}

// note, this duplicates SHGetRealIDL() so we work in non integrated shell mode
// BUGBUG: if it is not a file system pidl SFGAO_FILESYSTEM, we don't need to do this...
// review chrisny:  but this is only called in the case of SHCNE_CREATE for shell notify
// and all shell notify pidls are SFGAO_FILESYSTEM
HRESULT CNscTree::_IdlRealFromIdlSimple(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST *ppidlReal)
{
    STRRET str;
    HRESULT hres = psf->GetDisplayNameOf(pidlSimple, SHGDN_FORPARSING | SHGDN_INFOLDER, &str);
    if (SUCCEEDED(hres))
    {
        WCHAR wszPath[MAX_PATH];
        ULONG cbEaten;
        
        if (FAILED(hres = StrRetToBufW(&str, pidlSimple, wszPath, ARRAYSIZE(wszPath))) ||
            FAILED(hres = psf->ParseDisplayName(NULL, NULL, wszPath, &cbEaten, ppidlReal, NULL )))
        {
            *ppidlReal = ILClone(pidlSimple);   // we don't own the lifetime of pidlSimple
            hres = *ppidlReal ? S_OK : E_OUTOFMEMORY;
        }
    }
    return hres;    
}


HRESULT CNscTree::Refresh(void)
{
    ASSERT(!_fRefreshing);
    _bSynchId++;
    _fRefreshing = TRUE;

    TraceMsg(TF_NSC, "Expensive Refresh of tree");
    _htiActiveBorder = NULL;
    LPITEMIDLIST pidlRoot = CSIDL_DESKTOP;
    if (MODE_NORMAL != _mode)
        pidlRoot = ILClone(_pidlRoot);    // Need to do this because it's freed
    if (pidlRoot)
    {
        _ChangePidlRoot(pidlRoot);
        if (MODE_NORMAL != _mode)
            ILFree(pidlRoot);
    }

    _fRefreshing = FALSE;

    return S_OK;
}

void CNscTree::_CacheDetails(IShellFolder *psf)
{
    if (_ulDisplayCol == (ULONG)-1)
    {
        ASSERT(_psf2Cache == NULL);
        
        _ulSortCol = _ulDisplayCol = 0;
        
        if (SUCCEEDED(psf->QueryInterface(IID_IShellFolder2, (void **)&_psf2Cache)))
        {
            _psf2Cache->GetDefaultColumn(0, &_ulSortCol, &_ulDisplayCol);
        }
    }
}

HRESULT CNscTree::_GetDisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD uFlags, 
                                    LPSHELLDETAILS pdetails)
{
    _CacheDetails(psf);
    if (_ulDisplayCol)
        return _psf2Cache->GetDetailsOf(pidl, _ulDisplayCol, pdetails);
    return psf->GetDisplayNameOf(pidl, uFlags, &pdetails->str);
}

// if fSort, then compare for sort, else compare for existence.
HRESULT CNscTree::_CompareIDs(IShellFolder *psf, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    _CacheDetails(psf);

    return psf->CompareIDs(_ulSortCol, pidl1, pidl2);
}

HRESULT CNscTree::_ParentFromItem(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild)
{
    return IEBindToParentFolder(pidl, ppsfParent, ppidlChild);
} 

LRESULT CNscTree::_OnCDNotify(LPNMCUSTOMDRAW pnm)
{
    LRESULT     lres = CDRF_DODEFAULT;

    if (MODE_NORMAL == _mode)
        return 0;
    
    switch (pnm->dwDrawStage) 
    {
    case CDDS_PREPAINT:
        if (NSS_BROWSERSELECT & _dwFlags)
            lres = CDRF_NOTIFYITEMDRAW;
        break;
        
    case CDDS_ITEMPREPAINT:
        {
            //BUGBUG davemi: why is comctl giving us empty rects?
            if (IsRectEmpty(&(pnm->rc)))
                break;
            PORDERITEM poi = GetPoi(pnm->lItemlParam);
            DWORD dwFlags = 0;
            COLORREF    clrBk, clrText;
            LPNMTVCUSTOMDRAW pnmtv = (LPNMTVCUSTOMDRAW)pnm;             
            TV_ITEM tvi;
            TCHAR sz[MAX_URL_STRING];
            tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
            tvi.stateMask = TVIS_EXPANDED | TVIS_STATEIMAGEMASK | TVIS_DROPHILITED;
            tvi.pszText = sz;
            tvi.cchTextMax = MAX_URL_STRING;
            tvi.hItem = (HTREEITEM)pnm->dwItemSpec;
            if (!TreeView_GetItem(_hwndTree, &tvi))
                break;
            //
            //  See if we have fetched greyed/pinned information for this item yet 
            //
            ITEMINFO * pii = GetPii(pnm->lItemlParam);
            pii->fFetched = TRUE;

            if (pii->fGreyed && !(_mode & MODE_CONTROL))
                dwFlags |= DIGREYED;
            if (pii->fPinned)
                dwFlags |= DIPINNED;

            if (!pii->fNavigable)
                dwFlags |= DIFOLDER;
            
            dwFlags |= DIICON;

            clrBk   = g_clrBk;
            clrText = GetSysColor(COLOR_WINDOWTEXT);

            //if we're renaming an item, don't draw any text for it (otherwise it shows behind the item)
            if (tvi.hItem == _htiRenaming)
                sz[0] = 0;

            if (tvi.state & TVIS_EXPANDED)
                dwFlags |= DIFOLDEROPEN;

            if (tvi.state & NSC_TVIS_MARKED)
                dwFlags |= DIACTIVEBORDER;
            
            if ( (pnm->uItemState & CDIS_SELECTED) || (tvi.state & TVIS_DROPHILITED) )
            {
                if (_fHasFocus || tvi.state & TVIS_DROPHILITED)
                {
                    clrBk = GetSysColor(COLOR_HIGHLIGHT);
                    clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                }
                else
                    clrBk = GetSysColor(COLOR_BTNFACE);
//                    dwFlags |= DIFOCUSRECT;
            }

            if (pnm->uItemState & CDIS_HOT)
            {
                if (!(_mode & MODE_CONTROL))
                    dwFlags |= DIHOT;
                clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);

                if (clrText == clrBk)
                    clrText = GetSysColor(COLOR_HIGHLIGHT);
            }

            if (tvi.state & NSC_TVIS_MARKED)
            {
                //top level item
                if (_IsTopParentItem((HTREEITEM)pnm->dwItemSpec)) 
                {
                    dwFlags |= DISUBFIRST;
                    if (!(tvi.state & TVIS_EXPANDED))
                        dwFlags |= DISUBLAST;
                }
                else    // lower level items
                {                                                                
                    HTREEITEM hti;

                    dwFlags |= DISUBITEM;
                    if (((HTREEITEM)pnm->dwItemSpec) == _htiActiveBorder)
                        dwFlags |= DISUBFIRST;
                    
                    hti = TreeView_GetNextVisible(_hwndTree, (HTREEITEM)pnm->dwItemSpec);
                    if ( (hti && !_IsMarked(hti)) || (hti == NULL) )
                        dwFlags |= DISUBLAST;
                }
            }

            _DrawItem((HTREEITEM)pnm->dwItemSpec, sz, pnm->hdc, &(pnm->rc), dwFlags, pnmtv->iLevel, clrBk, clrText);
            lres = CDRF_SKIPDEFAULT;
            break;
        }
    case CDDS_POSTPAINT:
        break;
    }
    
    return lres;
} 

// *******droptarget implementation.
void CNscTree::_DtRevoke()
{
    if (_fDTRegistered)
    {
        RevokeDragDrop(_hwndTree);
        _fDTRegistered = FALSE;
    }
}

// review chrisny:  return hresult here.
void CNscTree::_DtRegister()
{
    if (!_fDTRegistered && (_dwFlags & NSS_DROPTARGET))
    {
        if (IsWindow(_hwndTree))
        {
            HRESULT hr = THR(RegisterDragDrop(_hwndTree, SAFECAST(this, IDropTarget*)));
            _fDTRegistered = BOOLIFY(SUCCEEDED(hr));
        }
        else
            ASSERT(FALSE);
    }
}

HRESULT CNscTree::GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll)
{
    if (!IsWindow(_hwndTree))
    {
        ASSERT(FALSE);
        return S_FALSE;
    }
    *phwndLock = /*_hwndDD*/_hwndTree;
    *phwndScroll = _hwndTree;
    return S_OK;
}
const int iInsertThresh = 6;


HRESULT CNscTree::HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD *pdwId, DWORD * pdwDropEffect)
{                                              
    switch (nEvent)
    {
    case HTDDT_ENTER:
        break;
        
    case HTDDT_LEAVE:
    {
        _fDragging = FALSE; 
        _fDropping = FALSE; 
        DAD_ShowDragImage(FALSE);
        TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);
        TreeView_SelectDropTarget(_hwndTree, NULL);
        DAD_ShowDragImage(TRUE);
        break;
    }
        
    case HTDDT_OVER: 
        {
            // review chrisny:  make function TreeView_InsertMarkHittest!!!!!
            RECT rc;
            TV_HITTESTINFO tvht;
            HTREEITEM htiOver;     // item to insert before or after.
            BOOL fWasInserting = BOOLIFY(_fInserting);
            BOOL fOldInsertBefore = BOOLIFY(_fInsertBefore);
            TV_ITEM tvi;
            PORDERITEM poi = NULL;
            IDropTarget     *pdtgt = NULL;
            HRESULT         hres;
            LPITEMIDLIST    pidl;
        
            _fDragging = TRUE;
            *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
            tvht.pt = *ppt;
            htiOver = TreeView_HitTest(_hwndTree, &tvht);
            // if no hittest assume we are dropping on the evil root.
            if (htiOver != NULL)
            {
                TreeView_GetItemRect(_hwndTree, (HTREEITEM)htiOver, &rc, TRUE);
                tvi.mask = TVIF_STATE | TVIF_PARAM | TVIF_HANDLE;
                tvi.stateMask = TVIS_EXPANDED;
                tvi.hItem = (HTREEITEM)htiOver;
                if (TreeView_GetItem(_hwndTree, &tvi))
                    poi = GetPoi(tvi.lParam);
                if (poi == NULL)
                {
                    ASSERT(FALSE);
                    return S_FALSE;
                }
            }
            else 
                htiOver = TVI_ROOT;

            // NO DROPPY ON HISTORY
            if (_mode & MODE_HISTORY)   
            {
                *pdwId = PtrToUlong(htiOver);
                *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
                return S_OK;
            }

            pidl = (poi == NULL) ? _pidlRoot : poi->pidl;
            pidl = _CacheParentShellFolder(htiOver, pidl);
            if (pidl)
            {
                // Is this the desktop pidl?
                if (ILIsEmpty(pidl))
                {
                    // Desktop's GetUIObject does not support the Empty pidl, so
                    // create the view object.
                    hres = _psfCache->CreateViewObject(_hwndTree, IID_IDropTarget, (void **)&pdtgt);
                }
                else
                    hres = _psfCache->GetUIObjectOf(_hwndTree, 1, (LPCITEMIDLIST *)&pidl, IID_IDropTarget, NULL, (void **)&pdtgt);
            }

            _fInserting = ((htiOver != TVI_ROOT) && ((ppt->y < (rc.top + iInsertThresh) 
                || (ppt->y > (rc.bottom - iInsertThresh)))  || !pdtgt));
            // review chrisny:  do I need folderstart == folder over?
            // If in normal mode, we never want to insert before, always _ON_...
            if (_mode != MODE_NORMAL && _fInserting)
            {
                ASSERT(poi);
                _iDragDest = poi->nOrder;   // index of item within folder pdwId
                if ((ppt->y < (rc.top + iInsertThresh)) || !pdtgt)
                    _fInsertBefore = TRUE;
                else
                {
                    ASSERT (ppt->y > (rc.bottom - iInsertThresh));
                    _fInsertBefore = FALSE;
                }
                if (_iDragSrc != -1)
                    *pdwDropEffect = DROPEFFECT_MOVE;   // moving from within.
                else
                    *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
                // inserting, drop target is actually parent folder of this item
                if (_fInsertBefore || ((htiOver != TVI_ROOT) && !(tvi.state & TVIS_EXPANDED)))
                {
                    _htiDropInsert = TreeView_GetParent(_hwndTree, (HTREEITEM)htiOver);
                }
                else
                    _htiDropInsert = htiOver;
                if (_htiDropInsert == NULL)
                    _htiDropInsert = TVI_ROOT;
                *pdwId = PtrToUlong(_htiDropInsert);
            }
            else
            {
                _htiDropInsert = htiOver;
                *pdwId =PtrToUlong(htiOver);
                _iDragDest = -1;     // no insertion point.
                *pdwDropEffect = DROPEFFECT_NONE;
            }

            // if we're over the item we're dragging, don't allow drop here
            if ( (_htiDragging == htiOver) || (IsParentOfItem(_hwndTree, _htiDragging, htiOver)) )
            {
                *pdwDropEffect = DROPEFFECT_NONE;
                *pdwId = (DWORD)-1;
                _fInserting = FALSE;
                ATOMICRELEASE(pdtgt);
            }

            // update UI
            if (_htiCur != (HTREEITEM)htiOver || fWasInserting != BOOLIFY(_fInserting) || fOldInsertBefore != BOOLIFY(_fInsertBefore))
            {
                // change in target
                _dwLastTime = GetTickCount();     // keep track for auto-expanding the tree
                DAD_ShowDragImage(FALSE);
                if (_fInserting)
                {
                    TraceMsg(TF_NSC, "NSCBand: drop insert now");
                    if (htiOver != TVI_ROOT)
                    {
                        if (_mode != MODE_NORMAL)
                        {
                            TreeView_SelectDropTarget(_hwndTree, NULL);
                            TreeView_SetInsertMark(_hwndTree, htiOver, !_fInsertBefore);
                        }
                    }
                }
                else
                {
                    TraceMsg(TF_NSC, "NSCBand: drop select now");
                    if (_mode != MODE_NORMAL)
                        TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);

                    if (htiOver != TVI_ROOT)
                    {
                        if (pdtgt)
                        {
                            TreeView_SelectDropTarget(_hwndTree, htiOver);       
                        }
                        else if (_mode != MODE_NORMAL)
                        {
                            // We do not want to select the drop target in normal mode
                            // because it causes a weird flashing of some item unrelated
                            // to the drag and drop when the drop is not supported.
                            TreeView_SelectDropTarget(_hwndTree, NULL);
                        }
                    }
                }
                UpdateWindow(_hwndTree);
                DAD_ShowDragImage(TRUE);
            }
            else
            {
                // No target change
                // auto expand the tree
                if (_htiCur)
                {
                    DWORD dwNow = GetTickCount();
                    if ((dwNow - _dwLastTime) >= 1000)
                    {
                        _dwLastTime = dwNow;
                        DAD_ShowDragImage(FALSE);
                        _fAutoExpanding = TRUE;
                        if (_htiCur != TVI_ROOT)
                            TreeView_Expand(_hwndTree, _htiCur, TVE_EXPAND);
                        _fAutoExpanding = FALSE;
                        UpdateWindow(_hwndTree);
                        DAD_ShowDragImage(TRUE);
                    }
                }
            }
            _htiCur = (HTREEITEM)htiOver; 
            ATOMICRELEASE(pdtgt);
        }
        break;
    }
    
    return S_OK;
}


HRESULT CNscTree::GetObjectDDT(DWORD dwId, REFIID riid, void ** ppvObj)
{
    HRESULT hres = S_FALSE;

    // We use -1 as the sentinal "This is where you started stupid"
    if (dwId != -1)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder((HTREEITEM)dwId, NULL);
        if (pidl)
        {
            if (ILIsEmpty(pidl))
                hres = _psfCache->CreateViewObject(_hwndTree, riid, (void **)ppvObj);
            else
                hres = _psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, riid, NULL, (void **)ppvObj);
        }
    }
    return hres;
}

HRESULT CNscTree::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;
    
    _fAsyncDrop = FALSE;                //ASSUME
    _fDropping = TRUE;

    // move within same folder, else let Drop() handle it.
    if (_iDragSrc >= 0)
    {
        if (_htiFolderStart == _htiDropInsert && _mode != MODE_NORMAL)
        {
            if (_iDragSrc != _iDragDest)    // no moving needed
            {
                int iNewPos =   _fInsertBefore ? (_iDragDest - 1) : _iDragDest;
                if (_MoveNode(_hwndTree, _iDragSrc, iNewPos, _pidlDrag))
                {
                    TraceMsg(TF_NSC, "NSCBand:  Reordering");
                    _fDropping = TRUE;
                    _Dropped();
                    // Remove this notify message immediately (so _fDropping is set
                    // and we'll ignore this event in above OnChange method)
                    //
                    _FlushNotifyMessages(_hwndTree);
                    _fDropping = FALSE;
                }
            }
            DragLeave();
            _htiCur = _htiFolderStart = NULL;
            _htiDropInsert =  (HTREEITEM)-1;
            _fDragging = _fInserting = _fDropping = FALSE;
            _iDragDest = -1;
            hres = S_FALSE;     // handled 
        }
    }
    else
    {
        // the item will get created in SHNotifyCreate()
        TraceMsg(TF_NSC, "NSCBand:  Dropped and External Item");

        BOOL         fSafe = TRUE;
        LPITEMIDLIST pidl;

        if (SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {
            fSafe = IEIsLinkSafe(_hwndParent, pidl, ILS_ADDTOFAV);
            ILFree(pidl);
        }

        if (fSafe)
        {
            _fAsyncDrop = TRUE;
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }

        // Since we're not freeing this in _MoveNode, we need to free this. This also cleans
        // up a problem where dragging after a completed drag fails.
        if (_pidlDrag)
            ILFree(_pidlDrag);
    }

    TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);
    TreeView_SelectDropTarget(_hwndTree, NULL);
    _pidlDrag = NULL;

    return hres;
}

IStream * CNscTree::GetOrderStream(LPCITEMIDLIST pidl, DWORD grfMode)
{
    // only do this for favorites
    if (!ILIsEmpty(pidl) && (_mode & MODE_FAVORITES) )
        return OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, pidl, REG_SUBKEY_FAVORITESA, grfMode);
    return NULL;
}

BOOL CNscTree::_MoveNode(HWND _hwndTree, int iDragSrc, int iNewPos, LPCITEMIDLIST pidl)
{
    int i = 0;
    HTREEITEM hti, htiAfter = TVI_LAST, htiDel = NULL;
    
    // if we are not moving and not dropping directly on a folder with no insert.
    if ((iDragSrc == iNewPos) && (iNewPos != -1))
        return FALSE;       // no need to move
    for (hti = TreeView_GetChild(_hwndTree, _htiDropInsert); hti; hti = TreeView_GetNextSibling(_hwndTree, hti)) 
    {
        if (i == iDragSrc)
            htiDel = hti;       // save node to be deleted, can't deelete it while enumerating
        // cuz the treeview will go down the tubes.  
        if (i == iNewPos)
            htiAfter = hti;
        i++;
    }
    if (iNewPos == -1)  // must be the first item
        htiAfter = TVI_FIRST;
    // add before delete to handle add after deleteable item case.
    _AddItemToTree(_htiDropInsert, pidl, I_CHILDRENCALLBACK, _iDragDest, htiAfter, FALSE);
    if (htiDel)
        TreeView_DeleteItem(_hwndTree, htiDel);

    _PopulateOrderList(_htiDropInsert);

    _fWeChangedOrder = TRUE;
    return TRUE;
}

void CNscTree::_Dropped(void)
{
    // Persist the new order out to the registry
    LPITEMIDLIST pidl = _GetFullIDList(_htiDropInsert);
    if (pidl)
    {
        IStream* pstm = GetOrderStream(pidl, STGM_WRITE | STGM_CREATE);
        if (pstm)
        {
            if (_CacheShellFolder(_htiDropInsert))
            {
                OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
                // remember we are now ordered.
                if (_htiDropInsert == TVI_ROOT)
                    _fOrdered = TRUE;
                else
                {
                    PORDERITEM poi = _GetTreeOrderItem(_htiDropInsert);
                    if (poi)
                    {
                        poi->lParam = (DWORD)FALSE;
                    }
                }
                // Notify everyone that the order changed
                SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, 0, pidl);
            }
            pstm->Release();
        }
        ILFree(pidl);
    }
    
    DPA_Destroy(_hdpaOrd);
    _hdpaOrd = NULL;

    _UpdateActiveBorder(_htiDropInsert);
}

CNscTree::CSelectionContextMenu::~CSelectionContextMenu()
{
    ATOMICRELEASE(_pcmSelection);
    ATOMICRELEASE(_pcm2Selection);
}

HRESULT CNscTree::CSelectionContextMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CNscTree::CSelectionContextMenu, IContextMenu2),                      // IID_IContextMenu2
        QITABENTMULTI(CNscTree::CSelectionContextMenu, IContextMenu, IContextMenu2),   // IID_IContextMenu
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}


ULONG CNscTree::CSelectionContextMenu::AddRef(void)
{
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    _ulRefs++;
    return pnsc->AddRef();
}

ULONG CNscTree::CSelectionContextMenu::Release(void)
{
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    ASSERT(_ulRefs > 0);
    _ulRefs--;
    if (0 == _ulRefs)
    {
        ATOMICRELEASE(_pcmSelection);
        ATOMICRELEASE(_pcm2Selection);
    }
    return pnsc->Release();
}

HRESULT CNscTree::CSelectionContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, 
                                                          UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (NULL == _pcmSelection)
    {
        return E_FAIL;
    }
    else
    {
        return _pcmSelection->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }
}

HRESULT CNscTree::CSelectionContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
    HTREEITEM hti;
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    UINT idCmd;
    
    if (!HIWORD(pici->lpVerb))
    {
        idCmd = LOWORD(pici->lpVerb);
    }
    else
    {
        return E_FAIL;
    }
    
    hres = pnsc->_QuerySelection(NULL, &hti);
    if (SUCCEEDED(hres))
    {
        pnsc->_ApplyCmd(hti, _pcmSelection, idCmd);
    }
    return hres;
}

HRESULT CNscTree::CSelectionContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (NULL == _pcm2Selection)
    {
        return E_FAIL;
    }
    else
    {
        //  HACK alert.  Work around bug in win95 user code for WM_DRAWITEM that sign extends
        //  itemID
        if (!g_fRunningOnNT && WM_DRAWITEM == uMsg)
        {
            LPDRAWITEMSTRUCT lpDraw = (LPDRAWITEMSTRUCT)lParam;
            
            if (0xFFFF0000 == (lpDraw->itemID & 0xFFFF0000) &&
                (lpDraw->itemID & 0xFFFF) >= FCIDM_BROWSERFIRST &&
                (lpDraw->itemID & 0xFFFF) <= FCIDM_BROWSERLAST)
            {
                lpDraw->itemID = lpDraw->itemID & 0xFFFF;
            }
        }
        
        return _pcm2Selection->HandleMenuMsg(uMsg,wParam,lParam);
    }
}

IContextMenu *CNscTree::CSelectionContextMenu::_QuerySelection()
{
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    
    ATOMICRELEASE(_pcmSelection);
    ATOMICRELEASE(_pcm2Selection);
    
    pnsc->_QuerySelection(&_pcmSelection, NULL);
    if (_pcmSelection)
    {
        _pcmSelection->QueryInterface(IID_IContextMenu2, (void **) &_pcm2Selection);
        AddRef();
        return SAFECAST(this, IContextMenu*);
    }
    return NULL;
}

LRESULT CALLBACK CNscTree::_SubClassTreeWndProc(
                                  HWND hwnd, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LRESULT lres = 0;
    CNscTree* pns = (CNscTree*)dwRefData; 

    ASSERT(pns);
    if (pns == NULL)
        return 0;
    
    switch (uMsg)
    {
    case WM_COMMAND:
        lres = pns->_OnCommand(wParam, lParam);
        break;

    case WM_SIZE:
        // if the width changes, we need to invalidate to redraw the ...'s at the end of the lines
        if (GET_X_LPARAM(lParam) != pns->_cxOldWidth) {
            //BUGBUG: be a bit more clever and only inval the right part where the ... can be
            InvalidateRect(pns->_hwndTree, NULL, FALSE);
            pns->_cxOldWidth = GET_X_LPARAM(lParam);
        }
        break;
        
    case WM_CONTEXTMENU:
        pns->_OnContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;
        
    case WM_INITMENUPOPUP:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
        if (pns->_pcmSendTo)
        {
            pns->_pcmSendTo->HandleMenuMsg(uMsg, wParam, lParam);
            return TRUE;
        }
        break;

    case WM_NSCUPDATEICONINFO:
        {
            NSC_ICONCALLBACKINFO nici = {(DWORD) (lParam&0x00000FFF),
                                         (DWORD) ((lParam&0x00FFF000) >> 12),
                                         (DWORD) ((lParam&0x0F000000) >> 24),
                                         (DWORD) ((lParam&0xF0000000) >> 28) };
            // make sure the magic numbers match
            if (nici.nMagic == pns->_bSynchId)
            {
                TVITEM    tvi;
                tvi.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                tvi.hItem = (HTREEITEM)wParam;

                // This can fail if the item was moved before the async icon
                // extraction finished for that item.
                if (TreeView_GetItem(pns->_hwndTree, &tvi))
                {
                    ITEMINFO* pii = GetPii(tvi.lParam);

                    pii->fGreyed      = BOOLIFY(nici.nFlags & NSCICON_GREYED);
                    pii->fPinned      = BOOLIFY(nici.nFlags & NSCICON_PINNED);
                    pii->fDontRefetch = BOOLIFY(nici.nFlags & NSCICON_DONTREFETCH);

                    tvi.iImage         = nici.iIcon;
                    tvi.iSelectedImage = nici.iOpenIcon;

                    TreeView_SetItem(pns->_hwndTree, &tvi);
                }
            }
        }
        break;

    // UGLY: Win95/NT4 shell DefView code sends this msg and does not deal
    // with the failure case. other ISVs do the same so this needs to stay forever
    case CWM_GETISHELLBROWSER:
        if (EVAL(pns))
            return (LRESULT)SAFECAST(pns, IShellBrowser*);  // not ref counted!
        break;

    case WM_HELP:
        {
            const static DWORD aBrowseHelpIDs[] = 
            {  // Context Help IDs
                ID_CONTROL,         IDH_BROWSELIST,
                0,                  0
            };
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
        }
        break;

    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
        // _HandleWinIniChange does an item height calculation that
        // depends on treeview having computed the default item height
        // already.  So we need to let treeview handle the settings
        // change before calling _HandleWinIniChange.  Also, we need
        // to reset the height to default so that treeview will
        // calculate a new default.
        TreeView_SetItemHeight(hwnd, -1);
        lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);
        pns->_HandleWinIniChange();
        break;

    default:
        break;
    }
    
    if (lres == 0)
       lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);

    return lres;
}

HRESULT CNscTree::_OnPaletteChanged(WPARAM wParam, LPARAM lParam)
{
    // forward this to our child view by invalidating their window (they should never realize their palette
    // in the foreground so they don't need the message parameters.) ...
    RECT rc;
    GetClientRect( _hwndTree, &rc );
    InvalidateRect( _hwndTree, &rc, FALSE );
    
    return NOERROR;
}

void CNscTree::_InvalidateImageIndex(HTREEITEM hItem, int iImage)
{
    HTREEITEM hChild;
    TV_ITEM tvi;
    
    if (hItem)
    {
        tvi.mask = TVIF_SELECTEDIMAGE | TVIF_IMAGE;
        tvi.hItem = hItem;
        
        TreeView_GetItem(_hwndTree, &tvi);
        if (iImage == -1 || tvi.iImage == iImage || tvi.iSelectedImage == iImage) 
            _TreeInvalidateItemInfo(hItem, 0);
    }
    
    hChild = TreeView_GetChild(_hwndTree, hItem);
    if (!hChild)
        return;
    
    for ( ; hChild; hChild = TreeView_GetNextSibling(_hwndTree, hChild))
        _InvalidateImageIndex(hChild, iImage);
}

void CNscTree::_TreeInvalidateItemInfo(HTREEITEM hItem, UINT mask)
{
    TV_ITEM tvi;
    
    tvi.mask =  mask | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem = hItem;
    tvi.cChildren = I_CHILDRENCALLBACK;
    tvi.iImage = I_IMAGECALLBACK;
    tvi.iSelectedImage = I_IMAGECALLBACK;
    tvi.pszText = LPSTR_TEXTCALLBACK;
    TreeView_SetItem(_hwndTree, &tvi);
}

void CNscTree::_DrawActiveBorder(HDC hdc, LPRECT prc)
{
    MoveToEx(hdc, prc->left, prc->top, NULL);
    LineTo(hdc, prc->right, prc->bottom);
}


#define DXLEFT      8
#define MAGICINDENT 3
void CNscTree::_DrawIcon(HTREEITEM hti, HDC hdc, int iLevel, RECT *prc, DWORD dwFlags)
{
    HIMAGELIST  himl = TreeView_GetImageList(_hwndTree, TVSIL_NORMAL);
    TV_ITEM     tvi;
    int         dx, dy, x, y;
    
    tvi.mask = TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE;
    tvi.hItem = hti;
    if (TreeView_GetItem(_hwndTree, &tvi))
    {
        ImageList_GetIconSize(himl, &dx, &dy);    
        if (!_fStartingDrag)
            x = DXLEFT;
        else
            x = 0;
        x += (iLevel * TreeView_GetIndent(_hwndTree)); // - ( (dwFlags & DIFOLDEROPEN) ? 1 : 0);
        y = prc->top + (((prc->bottom - prc->top) - dy) >> 1);
        int iImage = (dwFlags & DIFOLDEROPEN) ? tvi.iSelectedImage : tvi.iImage;
        ImageList_DrawEx(himl, iImage, hdc, x, y, 0, 0, CLR_NONE, GetSysColor(COLOR_WINDOW), (dwFlags & DIGREYED) ? ILD_BLEND50 : ILD_TRANSPARENT); 
        
        if (dwFlags & DIPINNED)
        {
            ASSERT(_hicoPinned);    
            DrawIconEx(hdc, x, y, _hicoPinned, 16, 16, 0, NULL, DI_NORMAL);
        }
    }
    return;
}

#define TreeView_GetFont(hwnd)  (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0)

void CNscTree::_DrawItem(HTREEITEM hti, TCHAR * psz, HDC hdc
                         , LPRECT prc, DWORD dwFlags, int iLevel, COLORREF clrbk, COLORREF clrtxt)
{
    SIZE        size;
    HIMAGELIST  himl = TreeView_GetImageList(_hwndTree, TVSIL_NORMAL);
    HFONT       hfont = NULL;
    HFONT       hfontOld = NULL;
    int         x, y, dx, dy;
    LOGFONT     lf;
    
    COLORREF clrGreyed = GetSysColor(COLOR_BTNSHADOW);
    if ((dwFlags & DIGREYED) && (clrbk != clrGreyed))
    {
        clrtxt = clrGreyed;
    }

    // For the history and favorites bars, we use the default
    // font (for UI consistency with the folders bar).

    if (_mode != MODE_FAVORITES && _mode != MODE_HISTORY)
        hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if ((dwFlags & DIHOT) && !(dwFlags & DIFOLDER))
    {
        if (!hfont)
            hfont = TreeView_GetFont(_hwndTree);

        // create the underline font
        GetObject(hfont, sizeof(lf), &lf);
        lf.lfUnderline = TRUE;
        hfont = CreateFontIndirect(&lf);
    }
    
    if (hfont)
        hfontOld = (HFONT)SelectObject(hdc, hfont);
    GetTextExtentPoint32(hdc, psz, lstrlen(psz), &size);
    if (himl)
        ImageList_GetIconSize(himl, &dx, &dy);    
    else 
    {
        dx = 0;
        dy = 0;
    }
    x = prc->left + ((dwFlags & DIICON) ? (iLevel * TreeView_GetIndent(_hwndTree) + dx + DXLEFT + MAGICINDENT) : DXLEFT);
    if (_fStartingDrag)
        x -= DXLEFT;
    y = prc->top + (((prc->bottom - prc->top) - size.cy) >> 1);

    UINT eto = ETO_CLIPPED;
    RECT rc;
    rc.left = prc->left + 2;
    rc.top = prc->top;
    rc.bottom = prc->bottom;
    rc.right = prc->right - 2;

    SetBkColor(hdc, clrbk);
    eto |= ETO_OPAQUE;
    ExtTextOut(hdc, 0, 0, eto, &rc, NULL, 0, NULL);

    SetTextColor(hdc, clrtxt);
    rc.left = x;
    rc.top = y;
    rc.bottom = rc.top + size.cy;
  
    DrawTextWrap(hdc, psz, lstrlen(psz), &rc, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (dwFlags & DIICON)
        _DrawIcon(hti, hdc, iLevel, prc, dwFlags);
    if (hfontOld)
        SelectObject(hdc, hfontOld);

    if (dwFlags & DIACTIVEBORDER)
    {
        if (dwFlags & DIFIRST)
        {
            rc = *prc;
            rc.left += 2;
            rc.bottom = rc.top + 1;
            rc.right -= 2;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
        if (dwFlags & DISUBITEM)
        {
            rc = *prc;
            rc.left += 2;
            rc.right = rc.left + 1;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
            rc.right = prc->right - 2;
            rc.left = rc.right - 1;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
        if (dwFlags & DILAST)
        {
            rc = *prc;
            rc.left += 2;
            rc.top = rc.bottom - 1;
            rc.right -= 2;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
    }
    
#if 0
    //focus is currently shown by drawing the selection with a different color
    // (in default scheme, it's blue when has focus, gray when not)
    if (dwFlags & DIFOCUSRECT)
    {
        rc = *prc;
        InflateRect(&rc, -1, -1);
        DrawFocusRect(hdc, &rc);
    }
#endif

    if (hfont)
        DeleteObject(hfont);
}

//+-------------------------------------------------------------------------
// If going online, ungreys all items that were unavailable.  If going
// offline, refreshes all items to see if they are still available.
//--------------------------------------------------------------------------
void CNscTree::_OnSHNotifyOnlineChange(HTREEITEM htiRoot, BOOL fGoingOnline)
{
    HTREEITEM hItem;

    for (hItem = TreeView_GetChild(_hwndTree, htiRoot); hItem
        ; hItem = TreeView_GetNextSibling(_hwndTree, hItem)) 
    {
        ITEMINFO *pii = _GetTreeItemInfo(hItem);
        if (pii)
        {
            if (fGoingOnline)
            {
                // Going online, if previously greyed then ungrey it
                if (pii->fGreyed)
                {
                    pii->fGreyed = FALSE;
                    _UpdateItemDisplayInfo(hItem);
                }
            }
            else
            {
                // Recheck each item to see if they should be greyed
                if (pii->fFetched && !pii->fDontRefetch)
                {
                    pii->fFetched = FALSE;
                    _UpdateItemDisplayInfo(hItem);
                }
            }
        }
        // Inform children too
        _OnSHNotifyOnlineChange(hItem, fGoingOnline);
    }
}

//+-------------------------------------------------------------------------
// Force items to recheck to see if the should be pinned or greyed
//--------------------------------------------------------------------------
void CNscTree::_OnSHNotifyCacheChange
(
 HTREEITEM htiRoot,      // recurse through all children
 DWORD_PTR dwFlags       // CACHE_NOTIFY_* flags
 )
{
    HTREEITEM hItem;

    for (hItem = TreeView_GetChild(_hwndTree, htiRoot); hItem
        ; hItem = TreeView_GetNextSibling(_hwndTree, hItem)) 
    {
        ITEMINFO *pii = _GetTreeItemInfo(hItem);
        if (pii)
        {
            // If we have cached info for this item, refresh it if it's state may have toggled
            if ((pii->fFetched && !pii->fDontRefetch) &&
                ((pii->fGreyed && (dwFlags | CACHE_NOTIFY_ADD_URL)) ||
                
                // We only need to check ungreyed items for changes to the 
                // stickey bit in the cache!
                (!pii->fGreyed &&
                ((dwFlags & (CACHE_NOTIFY_DELETE_URL | CACHE_NOTIFY_DELETE_ALL))) ||
                (!pii->fPinned && (dwFlags & CACHE_NOTIFY_URL_SET_STICKY)) ||
                (pii->fPinned && (dwFlags & CACHE_NOTIFY_URL_UNSET_STICKY))
                )
                ))
            {
                pii->fFetched = FALSE;
                _UpdateItemDisplayInfo(hItem);
            }
        }
        
        // Do it's children too
        _OnSHNotifyCacheChange(hItem, dwFlags);
    }
}

//
// Calls the appropriate routine in shdocvw to favorites import or export on
// the currently selected item
//
HRESULT CNscTree::DoImportOrExport(BOOL fImport)
{
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    LPITEMIDLIST pidl = _GetFullIDList(htiSelected);
    if (pidl)
    {
        //
        // If current selection is not a folder get the parent pidl
        //
        if (!ILIsFileSysFolder(pidl))
            ILRemoveLastID(pidl);
    
        //
        // Create the actual routine in shdocvw to do the import/export work
        //
        IShellUIHelper *pShellUIHelper;
        HRESULT hr = CoCreateInstance(CLSID_ShellUIHelper, NULL, CLSCTX_INPROC_SERVER,  IID_IShellUIHelper,  (void **)&pShellUIHelper);
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL vbImport = fImport ? VARIANT_TRUE : VARIANT_FALSE;
            WCHAR wszPath[MAX_PATH];

            SHGetPathFromIDListW(pidl, wszPath);
        
            hr = pShellUIHelper->ImportExportFavorites(vbImport, wszPath);
            if (SUCCEEDED(hr) && fImport)
            {
                //
                // Successfully imported favorites so need to update view
                // BUGBUG ie5 24973 - flicker alert, should optimize to just redraw selected
                //
                Refresh();
                //TreeView_SelectItem(_hwndTree, htiSelected);
            }
        
            pShellUIHelper->Release();
        }
        ILFree(pidl);
    }
    return S_OK;
}


HRESULT CNscTree::GetSelectedItemName(LPWSTR pszName, DWORD cchName)
{
    HRESULT hr = E_FAIL;
    TCHAR szPath[MAX_PATH];
    TV_ITEM tvi;

    tvi.hItem = TreeView_GetSelection(_hwndTree);
    if (tvi.hItem != NULL)
    {
        tvi.mask = TVIF_HANDLE | TVIF_TEXT;
        tvi.pszText = szPath;
        tvi.cchTextMax = ARRAYSIZE(szPath);
        
        if (TreeView_GetItem(_hwndTree, &tvi))
        {
            SHTCharToUnicode(szPath, pszName, cchName);
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT CNscTree::BindToSelectedItemParent(REFIID riid, void **ppv, LPITEMIDLIST *ppidl)
{
    HRESULT hres = E_FAIL;
    if (!_fClosing)
    {
        LPCITEMIDLIST pidlItem = _CacheParentShellFolder(TreeView_GetSelection(_hwndTree), NULL);
        if (pidlItem)
        {
            hres = _psfCache->QueryInterface(riid, ppv);
            if (SUCCEEDED(hres) && ppidl)
            {
                *ppidl = ILClone(pidlItem);
                if (*ppidl == NULL)
                {
                    hres = E_OUTOFMEMORY;
                    ((IUnknown *)*ppv)->Release();
                    *ppv = NULL;
                }
            }
        }
    }

    return hres;
}

HRESULT CNscTree::MoveSelectionTo(void)
{
    return MoveItemsIntoFolder(GetParent(_hwndParent)) ? S_OK : S_FALSE;
}

BOOL CNscTree::MoveItemsIntoFolder(HWND hwndParent)
{
    BOOL         fSuccess = FALSE;
    BROWSEINFO   browse = {0};
    TCHAR        szDisplayName[MAX_PATH];
    TCHAR        szInstructionString[MAX_PATH];
    LPITEMIDLIST pidlDest = NULL, pidlSelected = NULL;
    HTREEITEM    htiSelected = NULL;
    
    //Initialize the BROWSEINFO struct.
    browse.pidlRoot = ILClone(_pidlRoot);
    if (!browse.pidlRoot)
        return FALSE;
    
    htiSelected = TreeView_GetSelection(_hwndTree);
    pidlSelected = _GetFullIDList(htiSelected);
    if (!pidlSelected)
    {
        ILFree((LPITEMIDLIST)browse.pidlRoot);
        return FALSE;
    }
    
    MLLoadShellLangString(IDS_FAVORITEBROWSE, szInstructionString, ARRAYSIZE(szInstructionString));
    
    browse.pszDisplayName = szDisplayName;
    browse.hwndOwner = hwndParent;
    browse.lpszTitle = szInstructionString;
    browse.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    browse.lpfn = NULL;
    browse.lParam = 0;
    browse.iImage = 0;
    
    pidlDest = SHBrowseForFolder(&browse);
    if (pidlDest)
    {
        TCHAR szFrom[MAX_PATH+1];  // +1 for double null
        TCHAR szDest[MAX_PATH+1];
        SHGetPathFromIDList(pidlDest, szDest);
        SHGetPathFromIDList(pidlSelected, szFrom);
        
        ASSERT(szDest[0]);  // must be a file system thing...
        ASSERT(szFrom[0]);
        
        szDest[lstrlen(szDest) + 1] = 0;   // double null
        szFrom[lstrlen(szFrom) + 1] = 0;   // double null
        
        
        SHFILEOPSTRUCT  shop = {hwndParent, FO_MOVE, szFrom, szDest, 0, };
        SHFileOperation(&shop);
        
        fSuccess = TRUE;
        
        ILFree(pidlDest);
    }
    ILFree((LPITEMIDLIST)browse.pidlRoot);
    ILFree(pidlSelected);
    
    return fSuccess;
}

// the following guid goo and IsChannelFolder are mostly lifted from cdfview
#define     GUID_STR_LEN            80
const GUID  CLSID_CDFINI = {0xf3aa0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x34}};
// {f3aa0dc0-9cc8-11d0-a599-00c04fd64434}

// BUGBUG: total hack. looks into the desktop.ini for this guy
//
// pwzChannelURL is assumed to be INTERNET_MAX_URL_LENGTH
BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL)
{
    ASSERT(pwzPath);
    
    BOOL fRet = FALSE;
    
    WCHAR wzFolderGUID[GUID_STR_LEN];
    WCHAR wzIniFile[MAX_PATH];
    
    if (!PathCombineW(wzIniFile, pwzPath, L"desktop.ini"))
        return FALSE;
    
    if (GetPrivateProfileString(L".ShellClassInfo", L"CLSID", L"", wzFolderGUID, ARRAYSIZE(wzFolderGUID), wzIniFile))
    {
        WCHAR wzChannelGUID[GUID_STR_LEN];
        
        //it's only a channel if it's got the right guid and an url
        if (SHStringFromGUID(CLSID_CDFINI, wzChannelGUID, ARRAYSIZE(wzChannelGUID)))
        {
            fRet = (StrCmpN(wzFolderGUID, wzChannelGUID, ARRAYSIZE(wzChannelGUID)) == 0);
            if (fRet && pwzChannelURL)
            {
                fRet = (SHGetIniStringW(L"Channel", L"CDFURL", pwzChannelURL, INTERNET_MAX_URL_LENGTH, wzIniFile) != 0);
            }
        }
    }
    
    return fRet;
}

BOOL CNscTree::_IsChannelFolder(HTREEITEM hti)
{
    BOOL fRet = FALSE;

    LPITEMIDLIST pidl = _GetFullIDList(hti);

    if (pidl)
    {
        WCHAR szPath[MAX_PATH];
    
        if (SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath,
                                        ARRAYSIZE(szPath), NULL)))
        {
            fRet = IsChannelFolder(szPath, NULL);
        }

        ILFree(pidl);
    }

    return fRet;
}


HRESULT CNscTree::CreateSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool)
{
    HRESULT hr = DoSubscriptionForSelection(TRUE);
    
    if (pBool)
        *pBool = (SUCCEEDED(hr) ? TRUE : FALSE);

    return hr;
}


HRESULT CNscTree::DeleteSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool)
{
    HRESULT hr = DoSubscriptionForSelection(FALSE);
    
    if (pBool)
        *pBool = (SUCCEEDED(hr) ? TRUE : FALSE);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DoSubscriptionForSelection
//
// 1. get the selected item
// 2. get it's name
// 3. get it's url
// 4. create a Subscription manager and do the right thing for channels
// 5. return Subscription manager's hresult
HRESULT CNscTree::DoSubscriptionForSelection(BOOL fCreate)
{

#ifndef DISABLE_SUBSCRIPTIONS

    HRESULT hr = E_FAIL;
    WCHAR wzUrl[MAX_URL_STRING];
    WCHAR wzName[MAX_PATH];
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    if (htiSelected == NULL)
        return E_FAIL;
    
    TV_ITEM tvi;
    
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = htiSelected;
    tvi.pszText = wzName;
    tvi.cchTextMax = ARRAYSIZE(wzName);
    
    TreeView_GetItem(_hwndTree, &tvi);
    
    WCHAR wzPath[MAX_PATH];
    
    LPITEMIDLIST pidlItem = _CacheParentShellFolder(htiSelected, NULL);
    if (pidlItem)
    {
        GetPathForItem(_psfCache, pidlItem, wzPath, NULL);
        hr = GetNavTargetName(_psfCache, pidlItem, wzUrl, ARRAYSIZE(wzUrl));
    }
    
    if (FAILED(hr))     //if we couldn't get an url, not much to do
        return hr;
    
    ISubscriptionMgr *psm;
    
    hr = JITCoCreateInstance(CLSID_SubscriptionMgr, NULL,
        CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
        (void**)&psm, _hwndTree, FIEF_FLAG_FORCE_JITUI);
    if (SUCCEEDED(hr))
    {
        HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
        //IsChannelFolder will fixup wzUrl if it's a channel
        BOOL fChannel = IsChannelFolder(wzPath, wzUrl);
        
        if (fCreate)
        {
            SUBSCRIPTIONINFO si = { sizeof(SUBSCRIPTIONINFO) };
            TASK_TRIGGER tt;
            BOOL bIsSoftware = FALSE;
            
            if (fChannel)
            {
                IChannelMgrPriv *pChannelMgrPriv;
                
                hr = JITCoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IChannelMgrPriv, (void **)&pChannelMgrPriv,
                    _hwndTree, FIEF_FLAG_PEEK);

                if (SUCCEEDED(hr))
                {
                    WCHAR wszTitle[MAX_PATH];
                    
                    si.fUpdateFlags |= SUBSINFO_SCHEDULE;
                    si.schedule     = SUBSSCHED_AUTO;
                    si.pTrigger     = (LPVOID)&tt;
                    
                    hr = pChannelMgrPriv->DownloadMinCDF(_hwndTree, wzUrl, 
                        wszTitle, ARRAYSIZE(wszTitle), 
                        &si, &bIsSoftware);
                    pChannelMgrPriv->Release();
                }
            }

            if (SUCCEEDED(hr))
            {
                DWORD dwFlags = CREATESUBS_NOUI | CREATESUBS_FROMFAVORITES | 
                    ((!bIsSoftware) ? 0 : CREATESUBS_SOFTWAREUPDATE);
                
                hr = psm->CreateSubscription(_hwndTree, wzUrl, wzName, dwFlags, 
                    (fChannel ? SUBSTYPE_CHANNEL : SUBSTYPE_URL), 
                    &si);
            }
        }
        else
        {
            hr = psm->DeleteSubscription(wzUrl, NULL);
        }
        
        BOOL bSubscribed;
        
        //  This is in case subscribing or unsubscribing return a failed result even
        //  though the action succeeded from our standpoint (ie. item was subscribed
        //  successfully but creating a schedule failed or the item was unsubscribed 
        //  successfully but we couldn't abort a running download in syncmgr).
        
        psm->IsSubscribed(wzUrl, &bSubscribed);
        
        hr = ((fCreate && bSubscribed) || (!fCreate && !bSubscribed)) ? S_OK : E_FAIL;
        
        psm->Release();
        
        SetCursor(hCursorOld);
    }
    
    return hr;

#else  /* !DISABLE_SUBSCRIPTIONS */

    return E_FAIL;

#endif /* !DISABLE_SUBSCRIPTIONS */

}


// Causes NSC to re-root on a different pidl --
HRESULT CNscTree::_ChangePidlRoot(LPCITEMIDLIST pidl)
{
    _fClosing = TRUE;
    SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0L);
    _bSynchId++;
    if (_bSynchId >= 16)
        _bSynchId = 0;
    TreeView_DeleteAllItemsQuickly(_hwndTree);
    _htiActiveBorder = NULL;
    _fClosing = FALSE;
    if (_psfCache)
        _ReleaseCachedShellFolder();

    // We do this even for (NULL == pidl) because (CSIDL_DESKTOP == NULL)
    if ((LPCITEMIDLIST)INVALID_HANDLE_VALUE != pidl)
    {
        _UnSubClass();
        _SetRoot(pidl, 3/*random*/, NULL, NSSR_CREATEPIDL);
        _SubClass(pidl);
    }
    //Refresh();
    SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0L);

    return S_OK;
}

BOOL CNscTree::_IsMarked(HTREEITEM hti)
{
    if ( (hti == NULL) || (hti == TVI_ROOT) )
        return FALSE;
        
    TVITEM tvi;
    tvi.mask = TVIF_HANDLE | TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.state = 0;
    tvi.hItem = hti;
    TreeView_GetItem(_hwndTree, &tvi);

    return BOOLIFY(tvi.state & NSC_TVIS_MARKED);
}

void CNscTree::_MarkChildren(HTREEITEM htiParent, BOOL fOn)
{
    TVITEM tvi;
    tvi.mask = TVIF_HANDLE | TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.state = (fOn ? NSC_TVIS_MARKED : 0);
    tvi.hItem = htiParent;
    TreeView_SetItem(_hwndTree, &tvi);

    for (HTREEITEM htiTemp = TreeView_GetChild(_hwndTree, htiParent); htiTemp; htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp)) 
    {
        tvi.hItem = htiTemp;
        TreeView_SetItem(_hwndTree, &tvi);
    
        _MarkChildren(htiTemp, fOn);
    }
}

//Updates the tree and internal state for the active border (the 1 pixel line)
// htiSelected is the item that was just clicked on/selected
void CNscTree::_UpdateActiveBorder(HTREEITEM htiSelected)
{
    HTREEITEM htiNewBorder;
    if (MODE_NORMAL == _mode)
        return;

    //if an item is a folder, then it should have the border
    if (htiSelected != TVI_ROOT)
    {
        if (TreeView_GetChild(_hwndTree, htiSelected))
            htiNewBorder = htiSelected;
        else
            htiNewBorder = TreeView_GetParent(_hwndTree, htiSelected);
    }
    else
        htiNewBorder = NULL;
        
    //clear the old state
    if ( (_htiActiveBorder != TVI_ROOT) && (_htiActiveBorder != NULL) && (htiNewBorder != _htiActiveBorder))
        _MarkChildren(_htiActiveBorder, FALSE);
   
    //set the new state
    if ( (htiNewBorder != TVI_ROOT) && (htiNewBorder != NULL) )
        _MarkChildren(htiNewBorder, TRUE);

    //treeview knows to invalidate itself

    _htiActiveBorder = htiNewBorder;
}

void CNscTree::_UpdateItemDisplayInfo(HTREEITEM hti)
{
    LPITEMIDLIST pidl = _GetFullIDList(hti);
    if (_GetTreeItemInfo(hti) && _pTaskScheduler && pidl)
    {
        AddNscIconTask(_pTaskScheduler, pidl, &s_NscIconCallback, this, (UINT_PTR) hti, (UINT)_bSynchId);
    }
    else
    {
        ILFree(pidl);
    }
    //pidl gets freed by CNscIconTask
}

void CNscTree::s_NscIconCallback(LPVOID pvData, UINT_PTR uId, int iIcon, int iIconOpen, DWORD dwFlags, UINT uMagic)
{
    ASSERT(pvData);
    ASSERT(uId);
    CNscTree* pns = (CNscTree*)pvData;

    //this function gets called on a background thread, so use PostMessage to do treeview ops
    //on the main thread only.

    //assert that wacky packing is going to work
    ASSERT( (iIcon < 4096) && (iIconOpen < 4096) && (dwFlags < 16) && (uMagic < 16));

    LPARAM lParam = (uMagic << 28) + (dwFlags << 24) + (iIconOpen << 12) + iIcon;

    if (uMagic == pns->_bSynchId)
        PostMessage(pns->_hwndTree, WM_NSCUPDATEICONINFO, (WPARAM)uId, lParam);
}

LRESULT CNscTree::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    switch(idCmd)
    {
    case FCIDM_MOVE:
        InvokeContextMenuCommand(L"cut");
        break;

    case FCIDM_COPY:
        InvokeContextMenuCommand(L"copy");
        break;

    case FCIDM_PASTE:
        InvokeContextMenuCommand(L"paste");
        break;

    case FCIDM_LINK:
        InvokeContextMenuCommand(L"link");
        break;

    case FCIDM_DELETE:
        InvokeContextMenuCommand(L"delete");
        if (_hwndTree) 
        {
            SHChangeNotifyHandleEvents();
        }
        break;

    case FCIDM_PROPERTIES:
        InvokeContextMenuCommand(L"properties");
        break;

    case FCIDM_RENAME:
        {
            // HACKHACK (lamadio): This is to hack around tree view renaming on click and hover
            _fRclick = TRUE;
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            if (hti)
                TreeView_EditLabel(_hwndTree, hti);
            _fRclick = FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void CNscTree::_TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state)
{
    if (hti) 
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_STATE;
        tvi.stateMask = stateMask;
        tvi.hItem = hti;
        tvi.state = state;
        TreeView_SetItem(_hwndTree, &tvi);
    }

}

void CNscTree::_TreeNukeCutState()
{
    _TreeSetItemState(_htiCut, TVIS_CUT, 0);
    _htiCut = NULL;

    ChangeClipboardChain(_hwndTree, _hwndNextViewer);
    _hwndNextViewer = NULL;
}

    // *** IShellFolderFilterSite methods ***
HRESULT CNscTree::SetFilter(IUnknown* punk)
{
    HRESULT hres = S_OK;
    ATOMICRELEASE(_pFilter);

    if (punk)
        hres = punk->QueryInterface(IID_IShellFolderFilter, (void**)&_pFilter);

    return hres;
}
