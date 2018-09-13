/*********

  This file contains the Find Files IShellFolder (CDFFolder) and IShellBrowser 
  (CDFBrowse) implementations

**********/


#include "shellprv.h"
#pragma  hdrstop
extern "C" {
#include <regstr.h>
#include <fsmenu.h>
#include "ids.h"        // Why isn't this gotten from shellprv.h???
#include "findhlp.h"    // "
#include "pidl.h"       // "
#include "shitemid.h"   // "
#include "defview.h"    // "
#include "fstreex.h" // for views.h
#include "views.h"
}

#include "docfind.h"
#include "docfindx.h"
#include "cowsite.h"
#include "exdisp.h"
#include "shguidp.h"
#include "dfcmd.h"
#include "dbgmem.h"
#include "prop.h"           // COL_DATA

//
// HACK!  Listview doesn't support more than 100000000 items, so if our
// client returns more than that, just stop after that point.
//
// Instead of 100000000, we use the next lower 64K boundary.  This keeps
// us away from strange boundary cases (where a +1 might push us over the
// top), and it keeps the Alpha happy.
//
#define MAX_LISTVIEWITEMS  (100000000 & ~0xFFFF)
#define SANE_ITEMCOUNT(c)  ((int)min(c, MAX_LISTVIEWITEMS))

#ifndef DEBUG
#define DF_USE_EXCEPTION
#endif

// NTRAID 277332: To enable the Save Results logic, uncomment this line: [scotthan]
// #define DOCFIND_SAVERESULTS_ENABLED

#ifdef DEBUG // For leak detection
EXTERN_C void GetAndRegisterLeakDetection(void);
EXTERN_C BOOL g_fInitTable;
EXTERN_C LEAKDETECTFUNCS LeakDetFunctionTable;
#endif

#define _FILESEARCHBAND_    // use WIN32 file/folder search band.
#define ID_LISTVIEW 1


#define WM_DF_THREADNOTIFY      (WM_USER + 42)
#define WM_DF_FSNOTIFY          (WM_USER + 43)
#define WM_DF_THREADNOTIFYASYNC (WM_USER + 44)

#define DF_THREADNOTIFY_COMPLETE     ((LPARAM)-1)
#define DF_THREADNOTIFY_NORMAL       ((LPARAM)0)
#define DF_THREADNOTIFY_ASYNC        ((LPARAM)42)
#define DF_THREADNOTIFY_REDRAW       ((LPARAM)43)

#define DF_MAX_MATCHFILES   10000


const UINT c_auDFMenuIDs[] = {
    FCIDM_MENU_FILE,
    FCIDM_MENU_EDIT,
    FCIDM_MENU_VIEW,
    IDM_MENU_OPTIONS,
    FCIDM_MENU_HELP
};

const TCHAR s_szDocFind[]= TEXT("DocFind");
const TCHAR s_szFlags[] = TEXT("Flags");
const TCHAR s_szDocSpecMRU[] = REGSTR_PATH_EXPLORER TEXT("\\Doc Find Spec MRU");

#ifdef WINNT
//
// Unicode descriptor:
//
// Structure written at the end of NT-generated find stream serves dual purpose.
// 1. Contains an NT-specific signature to identify stream as NT-generated.
//    Appears as "NTFF" (NT Find File) in ASCII dump of file.
// 2. Contains an offset to the unicode-formatted criteria section.
//
// The following diagram shows the find criteria/results stream format including
// the NT-specific unicode criteria and descriptor.
//
//          +-----------------------------------------+ --------------
//          |         DFHEADER structure              |   .        .
//          +-----------------------------------------+   .        .
//          |      DF Criteria records (ANSI)         | Win95      .
//          +-----------------------------------------+   .        .
//          |      DF Results (PIDL) [optional]       |   .        NT
//          +-----------------------------------------+ -------    .
//   +----->| DF Criteria records (Unicode) [NT only] |            .
//   |      +-----------------------------------------+            .
//   |      | Unicode Descriptor |                                 .
//   |      +--------------------+  ----------------------------------
//   |     /                      \
//   |    /                         \
//   |   +-----------------+---------+
//   +---| Offset (64-bit) |  "NTFF" |
//       +-----------------+---------+
//
//

const DWORD c_NTsignature = 0x4646544E; // "NTFF" in ASCII file dump.

typedef struct _dfc_unicode_desc {
   ULARGE_INTEGER oUnicodeCriteria;  // Offset of unicode find criteria.
   DWORD NTsignature;               // Signature of NT-generated find file.
} DFC_UNICODE_DESC;

#endif

static const DWORD aFindHelpIDs[] = {
    IDD_PAGELIST,   NO_HELP,
    IDD_ANIMATE,    NO_HELP,
    IDD_STATUS,     NO_HELP,
    IDD_START,      IDH_FINDFILENAME_FINDNOW,
    IDD_STOP,       IDH_FINDFILENAME_STOP,
    IDD_NEWSEARCH,  IDH_FINDFILENAME_NEWSEARCH,
    ID_LISTVIEW,    IDH_FINDFILENAME_STATUSSCREEN,

    0, 0
};

//===========================================================================
// Setup the column number definitions.  - This may need to be converted
// into somethings that is filter related...
//===========================================================================

#ifdef WINNT
const UINT s_auMapDFColToFSCol[] = {0, (UINT)-1, (UINT)-1, 1, 2, 3, 4, 5, 6}; // More items than are needed but...
#else
// no rank column on win9x
const UINT s_auMapDFColToFSCol[] = {0, (UINT)-1, 1, 2, 3, 4, 5, 6}; // More items than are needed but...c
#endif

BOOL DocFind_StartFind(HWND hwndDlg, LPFINDTHREAD pft);
BOOL DocFind_StopFind(HWND hwndDlg);
BOOL DocFind_FindNext(HWND hwndDlg);

//===========================================================================
// IContextMenuCB implementation
//===========================================================================

class CDFContextMenuBase : public IContextMenuCB , public CObjectWithSite
{
public:
    CDFContextMenuBase();
    virtual ~CDFContextMenuBase();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // *** IContextMenuCB methods ***
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg,
        WPARAM wParam, LPARAM lParam) PURE;

private:
    LONG _cRef;
};

CDFContextMenuBase::CDFContextMenuBase()
{
    TraceMsg(TF_DOCFIND, "CDFContextMenuBase ctor");
    _cRef = 1;    
}

CDFContextMenuBase::~CDFContextMenuBase()
{
    TraceMsg(TF_DOCFIND, "CDFContextMenuBase dtor");
}

STDMETHODIMP CDFContextMenuBase::QueryInterface(REFIID riid, void **ppvObj) 
{        
    static const QITAB qit[] = {
        QITABENT(CDFContextMenuBase, IContextMenuCB),           // IID_IContextMenuCB
        QITABENT(CDFContextMenuBase, IObjectWithSite),          // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDFContextMenuBase::AddRef() 
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDFContextMenuBase::Release() 
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


class CDFContextMenuCB : public CDFContextMenuBase
{
public:
    // *** IContextMenuCB methods ***
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg,
        WPARAM wParam, LPARAM lParam);
};


// in fstreex.h
EXTERN_C DWORD CALLBACK _CFSFolder_PropertiesThread(void* pps); // bugbug: not actually a void

// static const QCMINFO_IDMAP 
static const struct {
    UINT max;
    struct {
        UINT id;
        UINT fFlags;
    } list[2];
} idMap = {
    2, 
    {
        {FSIDM_FOLDER_SEP, QCMINFO_PLACE_BEFORE},
        {FSIDM_VIEW_SEP, QCMINFO_PLACE_AFTER},
    },
};

STDMETHODIMP CDFContextMenuCB::CallBack(IShellFolder *psf, HWND hwndOwner,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            if (pdtobj)
            {
                if (!(wParam & CMF_DVFILE))
                {
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_FSVIEW_ITEM, 0, pqcm);
                }
            }
            else
            {
                UINT idStart = pqcm->idCmdFirst;
                UINT idBGMain = 0, idBGPopup = 0;
                IDocFindFolder       *pdfFolder;
                IDocFindFileFilter   *pdfff;
                if (SUCCEEDED(psf->QueryInterface(IID_IDocFindFolder, (void **)&pdfFolder))) 
                {
                    if (SUCCEEDED(pdfFolder->GetDocFindFilter(&pdfff))) 
                    {
                        pdfff->GetFolderMergeMenuIndex(&idBGMain, &idBGPopup);
                        CDefFolderMenu_MergeMenu(HINST_THISDLL, idBGMain, idBGPopup, pqcm);
                        DeleteMenu(pqcm->hmenu, SFVIDM_EDIT_PASTE, MF_BYCOMMAND);
                        DeleteMenu(pqcm->hmenu, SFVIDM_EDIT_PASTELINK, MF_BYCOMMAND);
                        DeleteMenu(pqcm->hmenu, SFVIDM_MISC_REFRESH, MF_BYCOMMAND);

                        IDocFindControllerNotify *pdcn;
                        if (S_OK == pdfFolder->GetControllerNotifyObject(&pdcn))
                        {
                            pdcn->Release();
                        }
                        else
                        {
                            DeleteMenu(pqcm->hmenu, idStart+FSIDM_SAVESEARCH, MF_BYCOMMAND);
                        }
                        
                        pdfff->Release();
                    }
                    pdfFolder->Release();
                }
            }
            pqcm->pIdMap = (QCMINFO_IDMAP *)&idMap;
        }
        break;

    case DFM_INVOKECOMMAND:
        {    
            // Check if this is from item context menu
            if (pdtobj)
            {
                switch(wParam)
                {
                case DFM_CMD_LINK:
                    hres = SHCreateLinks(hwndOwner, NULL, pdtobj, SHCL_USETEMPLATE | SHCL_USEDESKTOP | SHCL_CONFIRM, NULL);
                    break;
    
                case DFM_CMD_DELETE:
// TODO: Do this stuff for CShellBrowser case as well [t-ashram]
                    {
                        IDocFindBrowser *pdfb = NULL;   
        
                        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IDocFindBrowser, (void **)&pdfb))) 
                        {
                            pdfb->DeferProcessUpdateDir();
                        }
                        // convert to DFM_INVOKCOMMANDEX to get flags bits
                        hres = DeleteFilesInDataObject(hwndOwner, 0, pdtobj);

                        if (pdfb) 
                        {
                            pdfb->EndDeferProcessUpdateDir();
                            pdfb->Release();
                        }
                    }
                    break;

                case DFM_CMD_PROPERTIES:
                    // We need to pass an empty IDlist to combine with.
                    SHLaunchPropSheet(_CFSFolder_PropertiesThread, pdtobj,
                                      (LPCTSTR)lParam, NULL,  (void *)&c_idlDesktop);
                    break;

                default:
                    // BUGBUG: if GetAttributesOf did not specify the SFGAO_ bit
                    // that corresponds to this default DFM_CMD, then we should
                    // fail it here instead of returning S_FALSE. Otherwise,
                    // accelerator keys (cut/copy/paste/etc) will get here, and
                    // defcm tries to do the command with mixed results.
                    // BUGBUG: if GetAttributesOf did not specify SFGAO_CANLINK
                    // or SFGAO_CANDELETE or SFGAO_HASPROPERTIES, then the above
                    // implementations of these DFM_CMD commands are wrong...
                    // Let the defaults happen for this object
                    hres = (S_FALSE);
                    break;
                }
            }
            else
            {
                IShellFolderView *psfv;

                // No.
                switch(wParam)
                {
                    case FSIDM_SAVESEARCH:
                    {
                        IDocFindFolder *pdfFolder;
                        if (SUCCEEDED(psf->QueryInterface(IID_IDocFindFolder, (void **)&pdfFolder)))
                        {
                            IDocFindControllerNotify *pdcn;
                            if (S_OK == pdfFolder->GetControllerNotifyObject(&pdcn))
                            {
                                pdcn->SaveSearch();
                                pdcn->Release();
                            }
                            pdfFolder->Release();
                        }
                        break;
                    }

                    case FSIDM_SORTBYNAME:
                    case FSIDM_SORTBYSIZE:
                    case FSIDM_SORTBYTYPE:
                    case FSIDM_SORTBYDATE:
                    case FSIDM_SORTBYLOCATION:
    
                        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_DefView, IID_IShellFolderView, (void **)&psfv))) 
                        {
                            psfv->Rearrange(DFSortIDToICol((int) wParam));
                            psfv->Release();
                        }
                        break;
                default:
                    // This is one of view menu items, use the default code.
                    hres = (S_FALSE);
                    break;
                }
            }
        }
        break;

    case DFM_GETHELPTEXT:  // ansi version
    case DFM_GETHELPTEXTW:
        {
            UINT  id = LOWORD(wParam) + IDS_MH_FSIDM_FIRST;

            if (uMsg == DFM_GETHELPTEXTW)
                LoadStringW(HINST_THISDLL, id, (LPWSTR)lParam, HIWORD(wParam));
            else
                LoadStringA(HINST_THISDLL, id, (LPSTR)lParam, HIWORD(wParam));
        }
        break;
        
    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}


class CDFFolderContextMenuItemCB : public CDFContextMenuBase
{
public:
    // *** IContextMenuCB methods ***
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg,
        WPARAM wParam, LPARAM lParam);

private:
    CDFFolderContextMenuItemCB(IDocFindFolder* pdfFolder);
    ~CDFFolderContextMenuItemCB();
    friend IContextMenu* CDFFolderContextMenuItem_Create(HWND hwndOwner, IDocFindFolder* pdfFolder);

    IDocFindFolder *_pdfFolder;
};

CDFFolderContextMenuItemCB::CDFFolderContextMenuItemCB(IDocFindFolder* pdfFolder)
{
    ASSERT(pdfFolder);
    _pdfFolder = pdfFolder;
    _pdfFolder->AddRef();
}

CDFFolderContextMenuItemCB::~CDFFolderContextMenuItemCB()
{
    _pdfFolder->Release();
}

STDMETHODIMP CDFFolderContextMenuItemCB::CallBack(IShellFolder *psf, HWND hwndOwner,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DOCFIND_ITEM_MERGE, 0, pqcm);
        }
        break;

    case DFM_INVOKECOMMAND:
        switch(wParam)
        {
        case FSIDM_OPENCONTAININGFOLDER:
        {
            IShellFolderView *psfv;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_DefView, IID_IShellFolderView, (void **)&psfv)))
            {
                DFB_HandleOpenContainingFolder(psfv, _pdfFolder);
                psfv->Release();
            }
            break;
        }
        }
        break;

    case DFM_GETHELPTEXT:
    case DFM_GETHELPTEXTW:
        // probably need to implement these...
        
    default:
        hres = E_NOTIMPL;
        break;
    }
    
    return hres;
}

IContextMenu* CDFFolderContextMenuItem_Create(HWND hwndOwner, IDocFindFolder* pdfFolder)
{
    IContextMenu* pcm = NULL;

    // We want a quick IContextMenu implementation -- an empty defcm looks easiest
    IContextMenuCB* pcmcb = new CDFFolderContextMenuItemCB(pdfFolder);
    if (pcmcb)
    {
        CDefFolderMenu_CreateEx((LPCITEMIDLIST)NULL, hwndOwner,
                0, NULL, NULL, pcmcb, NULL, NULL, &pcm);
        pcmcb->Release();
    }

    return pcm;
}

//===========================================================================
// CDFFolder: class definition
//===========================================================================

// in docfind.h


//===========================================================================
// CDFFolder : Helper Functions
//===========================================================================

void cdecl DocFind_SetStatusText(HWND hwndStatus, int iField, UINT ids,...)
{
    TCHAR sz2[MAX_PATH+32];   // leave slop for message + max path name
    va_list ArgList;

    if (hwndStatus)
    {
        va_start(ArgList, ids);
        LPTSTR psz = _ConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(ids), &ArgList);
        if (psz)
        {
            // a-msadek; needed only for BiDi Win95 loc
            // Mirroring will take care of that over NT5 & BiDi Win98
            if(g_bBiDiW95Loc)
            {
                StrCpyN(sz2, psz, ARRAYSIZE(sz2)); // for some reason, this has always copied to sz2
                sz2[0] = sz2[1] = TEXT('\t');      // localizers must be leaving 2 spaces for these tabs...
            }
            else
                StrCpyN(sz2, psz, ARRAYSIZE(sz2));

            LocalFree(psz);
        }
        else
        {
            sz2[0] = 0;
        }
        va_end(ArgList);

        if (iField < 0)
        {
            // a-msadek; needed only for BiDi Win95 loc
            // Mirroring will take care of that over NT5 & BiDi Win98
            if(g_bBiDiW95Loc)
            {
                SendMessage(hwndStatus, SB_SETTEXT, SBT_RTLREADING | SBT_NOBORDERS | 255, (LPARAM)(LPTSTR)sz2);
            }
            else                
                SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)(LPTSTR)sz2);
        }
        else
            SendMessage(hwndStatus, SB_SETTEXT, iField, (LPARAM)(LPTSTR)sz2);

        UpdateWindow(hwndStatus);
    }
}

HRESULT CDFFolder::AddFolderToFolderList(LPITEMIDLIST pidl, BOOL fCheckForDup, int * piFolder)
{
    HRESULT hres = ERROR_SUCCESS;
    DFFolderListItem *pdffli;
    int i;
    int cbPidl;

    cbPidl = ILGetSize(pidl);

    if (fCheckForDup)
    {
        EnterCriticalSection(&_csSearch);
        for (i = DPA_GetPtrCount(this->hdpaPidf)-1; i >= 0; i--)
        {
            pdffli = (DFFolderListItem *)DPA_FastGetPtr( this->hdpaPidf, i);
            if (pdffli && (pdffli->cbPidl == cbPidl) && (ILIsEqual(&pdffli->idl, pidl)))
            {
                LeaveCriticalSection(&_csSearch);
                *piFolder = i;
                return S_OK;
            }
        }
        LeaveCriticalSection(&_csSearch);
    }
    // For now it is path based, but...
    pdffli = (DFFolderListItem *)LocalAlloc(LPTR, SIZEOF(DFFolderListItem) - sizeof(ITEMIDLIST) + cbPidl);
    if (pdffli == NULL)
    {
        *piFolder = -1;
        return (E_OUTOFMEMORY);
    }

    // lpddfli->psf = NULL;
    // pdffli->fUpdateDir = FALSE;
    memcpy(&pdffli->idl, pidl, cbPidl);
    pdffli->iImage = -1;       // Don't know the image yet for this one...
    pdffli->cbPidl = cbPidl;

    EnterCriticalSection(&_csSearch);
    // Now add this item to our DPA...
    i = DPA_AppendPtr( this->hdpaPidf, pdffli);
    LeaveCriticalSection(&_csSearch);
    
    if (i == -1)
    {
        LocalFree((HLOCAL)pdffli);
        hres = E_OUTOFMEMORY;
    }

    *piFolder = i;

    /* If this is a network ID list then register a path -> pidl mapping, therefore
    /  avoiding having to create simple ID lists, which don't work correctly when
    /  being compared against real ID lists. */

    if (IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces))
    {
        TCHAR szPath[ MAX_PATH ];

        SHGetPathFromIDList(pidl, szPath);
        NPTRegisterNameToPidlTranslation(szPath, pidl);
    }
    return hres;
}

void CDFFolder::_AddESFItemToSaveStateList(ESFItem *pesfi)
{
    ESFSaveStateItem essi;
    essi.dwState = pesfi->dwState & CDFITEM_STATE_MASK;
    PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(&pesfi->idl, IDLHID_DOCFINDDATA);
    ASSERT(phdfd && (phdfd->wFlags & DFDF_EXTRADATA));
    essi.dwItemID = phdfd->dwItemID;
    DSA_AppendItem(this->_hdsaSaveStateForIDs, (void *)&essi);
    if (essi.dwState & LVIS_SELECTED)
        _cSaveStateSelected++;
}


HRESULT CDFFolder::RememberSelectedItems()
 {
    int i;
    ESFItem *pesfi;

    EnterCriticalSection(&_csSearch);
    // Currently has list of pidls...
    for (i = DPA_GetPtrCount(this->hdpaItems); i-- > 0; )
    {
        // Pidl at start of structure...
        pesfi = (ESFItem*)DPA_FastGetPtr(this->hdpaItems, i);
        if (pesfi != NULL)
        {
            if (pesfi->dwState & (LVIS_SELECTED|LVIS_FOCUSED))
                _AddESFItemToSaveStateList(pesfi);
        }
    }
    LeaveCriticalSection(&_csSearch);
    return S_OK;
}

STDMETHODIMP CDFFolder::ClearItemList()
{
    int i;
    ESFItem *pesfi;

    // Clear out any async enumerators we may have
    SetAsyncEnum(NULL);
    _cAsyncItems = 0;       // clear out our count of items...
    pdfff->ReleaseQuery();

    // Also tell the filter to release everything...
    EnterCriticalSection(&_csSearch);
    if (this->hdpaItems == NULL)
    {
        LeaveCriticalSection(&_csSearch);
        return S_OK;     // Nothing to do
    }

    // Currently has list of pidls...
    for (i = DPA_GetPtrCount(this->hdpaItems); i-- > 0; )
    {
        // Pidl at start of structure...
        pesfi = (ESFItem*)DPA_FastGetPtr(this->hdpaItems, i);
        if (pesfi != NULL)
            LocalFree((HLOCAL)pesfi);
    }

    _fSearchComplete = FALSE;
    DPA_DeleteAllPtrs(this->hdpaItems);
    LeaveCriticalSection(&_csSearch);
    return S_OK;
}

STDMETHODIMP CDFFolder::ClearFolderList()
{
    int i;
    DFFolderListItem *pdffli;

    EnterCriticalSection(&_csSearch);
    if (this->hdpaPidf == NULL)
    {
        LeaveCriticalSection(&_csSearch);
        return S_OK;     // Nothing to do
    }

    for (i = DPA_GetPtrCount(this->hdpaPidf); i-- > 0; )
    {
        pdffli = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf, i);
        if (pdffli != NULL)
        {
            // Release the IShellFolder if we have one
            if (pdffli->psf != NULL)
                pdffli->psf->Release();

            // And delete the item

            if (LocalFree((HLOCAL)pdffli))
            {
                ASSERT(FALSE);      // Something bad happened!
                return E_FAIL;
            }
        }
    }

    DPA_DeleteAllPtrs(this->hdpaPidf);
    LeaveCriticalSection(&_csSearch);
    
    return S_OK;
}

//
//===========================================================================
// CDFFolder : Constructor
//===========================================================================

CDFFolder::CDFFolder()
{
    _cRef = 1;
    ASSERT(_pidl == NULL);
    _iGetIDList = -1;
    *_szEmptyText = 0;
    
    ASSERT(this->pdfff == NULL);

    // Create the heap for the folder lists.
    this->hdpaPidf = DPA_CreateEx(64, GetProcessHeap());

    // Create the DPA and DSA for the item list.
    this->cbItem = sizeof (ESFItem);
    this->hdpaItems = DPA_CreateEx(64, GetProcessHeap());
    this->_hdsaSaveStateForIDs = DSA_Create(sizeof(ESFSaveStateItem), 16);

    // initialize our LV selection objects...
    _dflvrSel.SetOwner(this, LVIS_SELECTED);
    _dflvrCut.SetOwner(this, LVIS_CUT);

    InitializeCriticalSection(&_csSearch);

    TraceMsg(TF_DOCFIND, "CDFFolder ctor");
}

CDFFolder::~CDFFolder()
{
    ASSERT(_cRef==0);
    
    // We will need to call our function to Free our items in our
    // Folder list.  We will use the same function that we use to
    // clear it when we do a new search

    ClearItemList();
    ClearFolderList();
    ClearSaveStateList();

    EnterCriticalSection(&_csSearch);
    DPA_Destroy(hdpaPidf);
    DPA_Destroy(hdpaItems);
    hdpaPidf = NULL;
    hdpaItems = NULL;
    LeaveCriticalSection(&_csSearch);
    DSA_Destroy(_hdsaSaveStateForIDs);

    if (pdfff)
        pdfff->Release();

    DeleteCriticalSection(&_csSearch);

    TraceMsg(TF_DOCFIND, "CDFFolder dtor");
}

HRESULT CDFFolder_Create(void **ppvOut, IDocFindFileFilter *pdfff)
{
    CDFFolder *pdffNew;
    HRESULT hr = E_OUTOFMEMORY;

    *ppvOut = NULL;
    pdffNew = new CDFFolder();
    if (pdffNew)
    {
        pdffNew->pdfff = pdfff; // Also handle case they do not pass one in...
        if (pdffNew->pdfff == NULL)
            pdffNew->pdfff = CreateDefaultDocFindFilter();
    
        // Check for out of memory conditions...
        if ( (pdffNew->pdfff == NULL) || (pdffNew->hdpaPidf == NULL) || (pdffNew->hdpaItems == NULL))
        {
            if (pdffNew->hdpaPidf)
                DPA_Destroy(pdffNew->hdpaPidf);
            if (pdffNew->hdpaItems)
                DPA_Destroy(pdffNew->hdpaItems);
            delete pdffNew->pdfff;
            delete pdffNew;
            pdffNew = NULL;
        }
        else
        {
            hr = NOERROR;
        }
    }

    *ppvOut = pdffNew;
    return hr;
}

STDAPI CDocFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT   hr;
    CDFFolder *pdff;

    hr = CDFFolder_Create((void **)&pdff, NULL);

    if (SUCCEEDED(hr))
    {
        hr = pdff->QueryInterface(riid, ppv);
        pdff->Release();
    }
    else
    {
        *ppv = NULL;
    }

    return hr;    
}


//Should combine with above
STDAPI CComputerFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT   hr;
    CDFFolder *pdff;
    IDocFindFileFilter *pdfff = CreateDefaultComputerFindFilter();
    if (!pdfff)
        return E_FAIL;

    // Warning This call takes ownership of pdfff - so don't release
    hr = CDFFolder_Create((void **)&pdff, pdfff);

    if (SUCCEEDED(hr))
    {
        hr = pdff->QueryInterface(riid, ppv);
        pdff->Release();
    }
    else
    {
        *ppv = NULL;
    }

    return hr;    
}

//===========================================================================
// CDFFolder : Now part of IDocFindFolder
//===========================================================================
HRESULT CDFFolder::MapFSPidlToDFPidl(LPITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    *ppidl = NULL;
    
    if (pidlParent)
    {
        EnterCriticalSection(&_csSearch);
        // Now loop through our DPA list and see if we can find a matach
        for (int i = 0; i <DPA_GetPtrCount(this->hdpaPidf); i++ )
        {
            DFFolderListItem *pdffli = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf, i);
            if (pdffli != NULL && ILIsEqual(pidlParent, &pdffli->idl))
            {
                // We found the right one
                // so no lets transform the ID into one of our own
                // to return.  Note: we must catch the case where the
                // original one passed in was a simple pidl and do
                // the appropriate thing.
                //
                LPITEMIDLIST pidlReal;

                // If this is not a FS folder, just clone it.
                if (fMapToReal)
                {
                    // We need to make sure we have an IShellFolder to
                    // work with...
                    IShellFolder *psf = DocFind_GetObjectsIFolder(this, pdffli, NULL);

                    if (psf)
                    {
                        SHGetRealIDL(psf, (LPITEMIDLIST)ILFindLastID(pidl), &pidlReal);
                        psf->Release();
                    }
                    else
                    {
                        pidlReal = ILClone((LPITEMIDLIST)ILFindLastID(pidl));
                    }
                }
                else
                    pidlReal = ILClone((LPITEMIDLIST)ILFindLastID(pidl));

                if (pidlReal)
                    //  return the appended
                    *ppidl = DocFind_AppendIFolder(pidlReal, i);

                //  exit regardless of success.
                break;  
            }
        }
        LeaveCriticalSection(&_csSearch);

        ILFree(pidlParent);
    }
    
    return *ppidl ? S_OK : S_FALSE;
}


//===========================================================================
// _UpdateItemList()
// Called before saving folder list.  Needed especially in Asynch search, 
// (CI).  We lazily pull item data from RowSet only when list view asks 
// for it.  When we are leaving the search folder, we pull all items
// creating all necessary folder lists.  This ensure when saving  folder
// list, all are included.   
// remark : Fix bug#338714.
//===========================================================================
HRESULT CDFFolder::_UpdateItemList()
{
    ESFItem *pesfi;
    int cItems;
    int i;
    USHORT  cb = 0;

    if (SUCCEEDED(GetItemCount(&cItems))) 
    {
        for (i=0; i < cItems; i++) 
        {
            HRESULT hres = GetItem(i, &pesfi);
            if (hres == DB_S_ENDOFROWSET)
                break;
        }
    }
    return S_OK;
}

//===========================================================================
// CDFFolder : External function to Save results out to file.
// Now part of IDocfindFolder
//===========================================================================
HRESULT CDFFolder::SaveFolderList(IStream *pstm)
{
    // We First pull all the items from RowSet (in Asynch case)
    _UpdateItemList();

    // Then, We serialize all of our PIDLS for each folder in our list
    DFFolderListItem *pdffli;
    USHORT  cb;

    int i;

    EnterCriticalSection(&_csSearch);
    // Now loop through our DPA list and see if we can find a matach
    for (i = 0; i <DPA_GetPtrCount(this->hdpaPidf); i++ )
    {
        pdffli = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf, i);
        if (EVAL(pdffli))
            ILSaveToStream(pstm, &pdffli->idl);
        else
            break;
    }
    LeaveCriticalSection(&_csSearch);
    // Now out a zero size item..
    cb = 0;
    pstm->Write((TCHAR *)&cb, SIZEOF(cb), NULL);

   return(TRUE);
}

//===========================================================================
// CDFFolder : External function to Restore results out to file.
// Now part of IDocfindFolder
//===========================================================================
HRESULT CDFFolder::RestoreFolderList(IStream *pstm)
{
    // loop through and all all of the folders to our list...
    LPITEMIDLIST pidl = NULL;
    HRESULT hres;

    for (;;)
    {
        int i;

        hres = ILLoadFromStream(pstm, &pidl); // frees pidl for us
        
        if (pidl == NULL)
            break;   // end of the list

        AddFolderToFolderList(pidl, FALSE, &i);
    }
    
    ILFree(pidl); // don't forget to free last pidl

    return hres;
}

HRESULT CDFFolder::SaveItemList(IStream *pstm)
{
    // We First serialize all of our PIDLS for each item in our list
    ESFItem *pesfi;
    int cItems;
    int i;
    USHORT  cb = 0;

    if (SUCCEEDED(GetItemCount(&cItems))) 
    {
        // And Save the items that are in the list
        for (i=0; i < cItems; i++) 
        {
            HRESULT hres = GetItem(i, &pesfi);

            if (hres == DB_S_ENDOFROWSET)
                break;
            if (SUCCEEDED(hres) && pesfi)
                ILSaveToStream(pstm, &pesfi->idl);
        }
    }

    // Write out a Trailing NULL size to say end of pidl list...
    pstm->Write(&cb, SIZEOF(cb), NULL);

   return S_OK;
}

HRESULT CDFFolder::RestoreItemList(IStream *pstm, int *pcItems)
{
    // And the pidls that are associated with the object
    int cItems = 0;
    LPITEMIDLIST pidl = NULL;    // don't free previous one
    ESFItem *pesfi;
    for (; ;)
    {
        if (FAILED(ILLoadFromStream(pstm, &pidl)) || (pidl == NULL))
            break;
        
        if (FAILED(AddPidl(cItems, pidl, (UINT)-1, &pesfi)) || !pesfi)
            break;
        cItems++;
    }
    if (pidl)
        ILFree(pidl);       // Free the last one read in

    *pcItems = cItems;
    return S_OK;
}



//===========================================================================
// CDFFolder : External function to Restore results out to file.
// Now part of IDocfindFolder
//===========================================================================
HRESULT CDFFolder::GetParentsPIDL(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent)
{
    PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);

    *ppidlParent = NULL;

    if (phdfd)
    {
        EnterCriticalSection(&_csSearch);
        // Now get to the item associated with that folder out of our dpa.
        ASSERT(hdpaPidf && DPA_GetPtrCount(hdpaPidf) > phdfd->iFolder);
        DFFolderListItem *pdffli = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf, phdfd->iFolder);
        if (pdffli)
        {
            // BUGBUG: we are returning pidl from a structure that may be freed on a different thread. should clone
            *ppidlParent = &pdffli->idl;
            LeaveCriticalSection(&_csSearch);
            return S_OK;
        }
        LeaveCriticalSection(&_csSearch);
    }
    else
        AssertMsg(FALSE, TEXT("CDF::GetParentsPIDL() passed invalid DocFind pidl"));

    return S_FALSE;
}



HRESULT CDFFolder::SetControllerNotifyObject(IDocFindControllerNotify *pdfcn)
{
    ATOMICRELEASE(_pdfcn);
    _pdfcn = pdfcn;
    if (_pdfcn)
    {
        _pdfcn->AddRef();
    }
    return S_OK;
}

HRESULT CDFFolder::GetControllerNotifyObject(IDocFindControllerNotify **ppdfcn)
{
    *ppdfcn = _pdfcn;
    if (_pdfcn)
        _pdfcn->AddRef();
    return _pdfcn ? S_OK : S_FALSE;
}

//===========================================================================
// CDFFolder : Members
//===========================================================================
//
// AddRef
//
STDMETHODIMP_(ULONG) CDFFolder::AddRef()
{
    TraceMsg(TF_DOCFIND, "cdff.AddRef %d",_cRef+1);
    return (++_cRef);
}

//
// Release
//
STDMETHODIMP_(ULONG) CDFFolder::Release()
{
    ASSERT(_cRef >= 1);                    // Don't release more than we can!
    _cRef--;
    TraceMsg(TF_DOCFIND,"cdff.Release %d",_cRef);
    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


STDMETHODIMP CDFFolder::ParseDisplayName(HWND hwndOwner,
    LPBC pbc, LPOLESTR pwzDisplayName,
    ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG * pdwAttributes)
{
    return  E_NOTIMPL;
}

STDMETHODIMP CDFFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    //
    // We do not want the def view to enumerate us, instead we
    // will tell defview to call us...
    //
    *ppenum = NULL;     // No enumerator
    return S_FALSE;     // no enumerator (not error)
}

IShellFolder *DocFind_GetObjectsIFolder(IDocFindFolder *pdff, DFFolderListItem *pdffli, LPCITEMIDLIST pidl)
{
    // First Validate the pidl that was passed in.
    // is what we expect - cb, Folder number, followed by a mkid

    if (pdffli == NULL)
    {
        PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);

        if (phdfd)
            pdff->GetFolderListItem(phdfd->iFolder, &pdffli);
        else
            AssertMsg(FALSE, TEXT("DocFind_GetObjectsIFolder() passed invalid DocFind pidl"));
    }

    // Now see if we need to build an IFolder for this object.
    //
    if (pdffli && pdffli->psf == NULL)
        SHBindToObject(NULL, IID_IShellFolder, &pdffli->idl, (void **)&pdffli->psf);

    if (pdffli && pdffli->psf)
        pdffli->psf->AddRef();
        
    return pdffli ? pdffli->psf : NULL;
}


STDMETHODIMP CDFFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_INVALIDARG;
    //
    // We need to parse the ID and see which sub-folder it belongs to
    // then we will see if we have an IShellFolder for the object.  If not
    // we will construct it and then extract our part off of the IDLIST and
    // then forward it...
    //
    IShellFolder *psfItem = DocFind_GetObjectsIFolder(this, NULL, pidl);
    if (psfItem)
    {
        hr = psfItem->BindToObject(pidl, pbc, riid, ppvOut);
        psfItem->Release();
    }

    return hr;
}


// Little helper function for bellow
HRESULT CDFFolder::_CompareFolderIndexes(int iFolder1, int iFolder2)
{
    TCHAR        szPath1[MAX_PATH];
    TCHAR        szPath2[MAX_PATH];

    EnterCriticalSection(&_csSearch);
    
    DFFolderListItem *pdffli1 = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf,
            iFolder1);
    DFFolderListItem *pdffli2 = (DFFolderListItem *) DPA_FastGetPtr(this->hdpaPidf,
            iFolder2);

    if ((pdffli1 != NULL) && (pdffli2 != NULL))
    {
        SHGetPathFromIDList(&pdffli1->idl, szPath1);
        SHGetPathFromIDList(&pdffli2->idl, szPath2);

        LeaveCriticalSection(&_csSearch);
        
        return(ResultFromShort(lstrcmpi(szPath1, szPath2)));
    }
    LeaveCriticalSection(&_csSearch);
    // If we got here there is an error!
    return (E_INVALIDARG);
}

STDMETHODIMP CDFFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = (E_INVALIDARG);
    IShellFolder *psf1;
    LPITEMIDLIST pidl1Last;
    int iFolder1;
    int iFolder2;

    // Note there are times in the defview that may call us back looking for
    // a fully qualified pidl in pidl1, so we need to detect this case and
    // only process the last part.

    pidl1Last = (LPITEMIDLIST)ILFindLastID((LPITEMIDLIST)pidl1);
    iFolder1 = DF_IFOLDER(pidl1Last);
    iFolder2 = DF_IFOLDER(pidl2);


    // We need to handle the case our self if the sort is by
    // the containing folder. But otherwise we simply forward the informat
    // to the FSFolders compare function.  This will only work if both ids
    // are file system objects, but if this is not the case, then what???
    //
    if (lParam == IDFCOL_PATH)
    {
        if (iFolder1 != iFolder2)
            return _CompareFolderIndexes(iFolder1, iFolder2);

        //
        // If we get here the paths are the same so set the sort order
        // to name...
        lParam = IDFCOL_NAME;
    }
    else if (lParam == IDFCOL_RANK)
    {
        PCHIDDENDOCFINDDATA phdfd1 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl1, IDLHID_DOCFINDDATA);
        PCHIDDENDOCFINDDATA phdfd2 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl2, IDLHID_DOCFINDDATA);

        // Could be mixed if so put ones without rank at the end...
        if (phdfd1 && (phdfd1->wFlags & DFDF_EXTRADATA))
        {
            if (phdfd2 && (phdfd2->wFlags & DFDF_EXTRADATA))
            {
                // Have both...
                if (phdfd1->ulRank < phdfd2->ulRank)
                    return ResultFromShort(-1);
                if (phdfd1->ulRank > phdfd2->ulRank)
                    return ResultFromShort(1);
            }
            else
                return ResultFromShort(1);
        }
        else if (phdfd2)
            return ResultFromShort(-1);

        // Otherwise next sort on name...
        lParam = IDFCOL_NAME;
    }

    // For now we just forward this to the function the that CFSFolder
    // uses.  This is not clean but...
    psf1 = DocFind_GetObjectsIFolder(this, NULL, pidl1Last);
    if (psf1)
    {
        hres = psf1->CompareIDs(s_auMapDFColToFSCol[lParam], pidl1Last, pidl2);
        psf1->Release();

        if ((hres == 0) && (lParam == IDFCOL_NAME) && (iFolder1 != iFolder2))
        {
            // They compared the same and by name so make sure truelly the
            // same path as this is used in processing notifications.
            return _CompareFolderIndexes(iFolder1, iFolder2);
        }
        return hres;
    }
    else
        return E_INVALIDARG;
}

STDMETHODIMP CDFFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppvOut)
{
    HRESULT hr;

    //
    // We need to parse the ID and see which sub-folder it belongs to
    // then we will see if we have an IShellFolder for the object.  If not
    // we will construct it and then extract our part off of the IDLIST and
    // then forward it...
    //
    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.pshf     = this;
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = DocFind_CreateSFVCB(SAFECAST(this, IShellFolder*), this);
        if (sSFV.psfvcb)
        {
            hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppvOut);

            sSFV.psfvcb->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IContextMenuCB *pcmcb = new CDFContextMenuCB();
        if (pcmcb)
        {
            hr = CDefFolderMenu_CreateEx(NULL, hwnd,
                    0, NULL, this, pcmcb, NULL, NULL, (IContextMenu * *)ppvOut);
            pcmcb->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP CDFFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * prgfInOut)
{
    HRESULT hr = E_FAIL;
    //
    // We need to simply forward this to the the IShellfolder of the
    // first one.  We will pass him all of them as I know he does not
    // process the others...
    // call off to the containing folder
    if (cidl == 0)
    {
        // They are asking for capabilities, so tell them that we support Rename...
        // Review: See what other ones we need to return.
        *prgfInOut = SFGAO_CANRENAME;
        hr = S_OK;
    }
    else
    {
        IShellFolder *psfItem = DocFind_GetObjectsIFolder(this, NULL, ILFindLastID(*apidl));
        if (psfItem)
        {
            hr = psfItem->GetAttributesOf(cidl, apidl, prgfInOut);
            psfItem->Release();
        }
    }
    return hr;
}

// Helper function to Map the Sort Id to IColumn
int DFSortIDToICol(int uCmd)
{
    switch (uCmd)
    {
    case FSIDM_SORTBYNAME:
        return IDFCOL_NAME;
    case FSIDM_SORTBYLOCATION:
        return IDFCOL_PATH;
    default:
#ifdef WINNT
        return uCmd - FSIDM_SORT_FIRST - IDFCOL_NAME + 2;   // make room for location and rank
#else
        // win9x doesn't have rank
        return uCmd - FSIDM_SORT_FIRST - IDFCOL_NAME + 1;   // make room for location
#endif

    }
}

//
// To be called back from within CDefFolderMenuE - Currently only used
//

// Some helper functions                
STDMETHODIMP CDFFolder::SetItemsChangedSinceSort()
{ 
    this->fItemsChangedSinceSort = TRUE;
    return S_OK;
}

STDMETHODIMP CDFFolder::GetItemCount(INT *pcItems)
{ 
    ASSERT(pcItems);
    DBCOUNTITEM cItems = 0;

    EnterCriticalSection(&_csSearch);
    if (this->hdpaItems)
        cItems = DPA_GetPtrCount(this->hdpaItems);
    LeaveCriticalSection(&_csSearch);

    // If async, then we may not have grown our dpa yet... but in mixed case we have so take
    // max of the two...
    if (_pDFEnumAsync)
    {
        if (_cAsyncItems > cItems)
            cItems = _cAsyncItems;
    }

    *pcItems = SANE_ITEMCOUNT(cItems);
    return S_OK;
};


STDMETHODIMP CDFFolder::GetItem(int iItem, ESFItem **ppItem)
{
    HRESULT  hres = E_FAIL; // just to init, use anything
    ESFItem *pesfi;
    IDFEnum *pidfenum;

    GetAsyncEnum(&pidfenum);

    DWORD dwItemID = (UINT)-1;

    EnterCriticalSection(&_csSearch);
    int i = DPA_GetPtrCount(this->hdpaItems);
    pesfi = (ESFItem *) DPA_GetPtr(this->hdpaItems, iItem);
    LeaveCriticalSection(&_csSearch);

    // Mondo hack to better handle Async searching (ROWSET), we are not sure if we
    // can trust the PIDL of the row as new rows may have been inserted...
    // Only do this if we are not looking at the previous item..

    if (pesfi && pidfenum && !_fSearchComplete && (iItem != _iGetIDList))
    {
        PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(&pesfi->idl, IDLHID_DOCFINDDATA);

        // As we can now have mixed results only blow away if this is an async guy...
        if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
        {
            pidfenum->GetItemID(iItem, &dwItemID);
            if (dwItemID != phdfd->dwItemID)
            {
                // Overlload, pass NULL to ADDPIDL to tell system to free that item
                if (pesfi->dwState & (LVIS_SELECTED|LVIS_FOCUSED))
                    _AddESFItemToSaveStateList(pesfi);

                AddPidl(iItem, 0, NULL, NULL);
                pesfi = NULL;
            }
        }
    }
                                                                                   
    _iGetIDList = iItem;   // remember the last one we retrieved...

    if (!pesfi && (iItem >= 0))
    {
        // See if this is the async case
        if (pidfenum)
        {
            LPITEMIDLIST pidlT;

            hres = pidfenum->GetItemIDList(SANE_ITEMCOUNT(iItem), &pidlT);            
            if (SUCCEEDED(hres) && hres != DB_S_ENDOFROWSET)
            {
                AddPidl(iItem, pidlT, dwItemID, &pesfi);
                // See if this item should show up as selected...
                if (dwItemID == (UINT)-1)
                    pidfenum->GetItemID(iItem, &dwItemID);
                GetStateFromSaveStateList(dwItemID, &pesfi->dwState);
            }
        }
    }

    *ppItem = pesfi;

    if (hres != DB_S_ENDOFROWSET)
        hres = pesfi? S_OK : E_FAIL;

    return hres;
}

STDMETHODIMP CDFFolder::DeleteItem(int iItem)
{
    HRESULT hres = E_FAIL;
    
    if (!_fInRefresh)
    {
        ESFItem *pesfi;

        hres = E_INVALIDARG;
        // make sure the item is in dpa (if using cI)
        if (SUCCEEDED(GetItem(iItem, &pesfi)) && pesfi)
        {
            EnterCriticalSection(&_csSearch);
            DPA_DeletePtr(hdpaItems, iItem);
            LeaveCriticalSection(&_csSearch);
            
            PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(&pesfi->idl, IDLHID_DOCFINDDATA);

            if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
            {
                //we are deleting async item...
                _cAsyncItems--;
            }
            
            if (pesfi->dwState &= LVIS_SELECTED)
            {
                // Need to update the count of items selected...
                _dflvrSel.DecrementIncludedCount();
            }
            LocalFree((HLOCAL)pesfi);

            hres = S_OK;
        }
    }
    return hres;
}

STDMETHODIMP CDFFolder::ValidateItems(int iItem, int cItems, BOOL bSearchComplete)
{
    ESFItem *pesfi;
    IDFEnum *pidfenum;
    int cItemsInList;
    int iLVFirst;
    int cLVItems;

    GetAsyncEnum(&pidfenum);

    if (!pidfenum || _fAllAsyncItemsCached)
        return S_OK;    // nothing to validate.

    DWORD dwItemID = (UINT)-1;

    GetItemCount(&cItemsInList);
    // force reload of rows
    pidfenum->Reset();
    iLVFirst = ListView_GetTopIndex(_hwndLV);
    cLVItems = ListView_GetCountPerPage(_hwndLV);

    if (iItem == -1)
    {
        iItem = iLVFirst;
        cItems = cLVItems;
    }

    // to avoid failing to update an item...
    if (bSearchComplete)
        _iGetIDList = -1;
        
    while ((iItem < cItemsInList) && cItems)
    {
        EnterCriticalSection(&_csSearch);
        pesfi = (ESFItem *) DPA_GetPtr(this->hdpaItems, iItem);
        LeaveCriticalSection(&_csSearch);
        if (!pesfi)     // Assume that if we have not gotten this one we are in the clear...
            break;

        PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(&pesfi->idl, IDLHID_DOCFINDDATA);

        if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
        {
            pidfenum->GetItemID(iItem, &dwItemID);
            
            if (dwItemID != phdfd->dwItemID)
            {
                ESFItem *pItem; // dummy to make GetItem happy
                // Oops don't match,
                if (InRange(iItem, iLVFirst, iLVFirst+cLVItems))
                {
                    if (SUCCEEDED(GetItem(iItem, &pItem)))
                    {
                        ListView_RedrawItems(_hwndLV, iItem, iItem);
                    }
                }
                else
                {
                    AddPidl(iItem, NULL, 0, NULL);
                }
            }
        }
        else
        {
            break;  // stop after we reach first non ci item
        }
        iItem++;
        cItems--;
    }

    _fSearchComplete = bSearchComplete;

    DebugMsg(TF_DOCFIND, TEXT("sh TR - CDFFolder::ValidateItems - OK"));
    return S_OK;
}

STDMETHODIMP CDFFolder::AddPidl(int i, LPITEMIDLIST pidl, DWORD dwItemID, ESFItem **ppcdfi)
{
    HRESULT  hr = S_OK;
    ESFItem* pesfi;

    if (!pidl)
    {
        EnterCriticalSection(&_csSearch);
        pesfi = (ESFItem*)DPA_GetPtr(this->hdpaItems, i);
        if (pesfi)
        {
            LocalFree((HLOCAL)pesfi);
            DPA_SetPtr(this->hdpaItems, i, NULL);
        }
        LeaveCriticalSection(&_csSearch);
        if (ppcdfi)
            *ppcdfi = NULL;
        return S_OK;
    }
   
    int cb = ILGetSize(pidl);
    pesfi = (ESFItem*)LocalAlloc(LMEM_FIXED, sizeof(ESFItem) - sizeof(ITEMIDLIST) + cb);
    if (pesfi)
    {
        pesfi->dwMask = 0;
        pesfi->dwState = 0;
        // pesfi->dwItemID = dwItemID;
        pesfi->iIcon = -1;
        memcpy(&pesfi->idl, pidl, cb);

        EnterCriticalSection(&_csSearch);
        BOOL bRet = DPA_SetPtr(this->hdpaItems, i, (void *)pesfi);
        LeaveCriticalSection(&_csSearch);
        if (!bRet)
        {
            LocalFree((HLOCAL)pesfi);
            pesfi = NULL;
            hr = E_FAIL;
        }
    }

    if (ppcdfi)
        *ppcdfi = pesfi;
    
    return hr;
}

STDMETHODIMP CDFFolder::SetAsyncEnum(IDFEnum *pdfEnumAsync)
{
    if (_pDFEnumAsync)
        _pDFEnumAsync->Release();

    _pDFEnumAsync = pdfEnumAsync;
    if (pdfEnumAsync)
        pdfEnumAsync->AddRef();
    return S_OK;
}

STDMETHODIMP CDFFolder::CacheAllAsyncItems()
{
    ESFItem *pesfi;
    IDFEnum *pidfenum;
    int i, maxItems;


    if (_fAllAsyncItemsCached)
        return S_OK;      // Allready done it...  

    GetAsyncEnum(&pidfenum);
    if (!pidfenum)
        return S_FALSE; // nothing to do...


    // Probably the easiest thing to do is to simply walk through all of the items...
    maxItems = SANE_ITEMCOUNT(_cAsyncItems);
    for (i=0;i < maxItems; i++)
    {
        GetItem(i, &pesfi);
    }

    _fAllAsyncItemsCached = TRUE;
    return S_OK;
}


BOOL CDFFolder::AllAsyncItemsCached()
{
    return _fAllAsyncItemsCached;
}

STDMETHODIMP CDFFolder::GetAsyncEnum(IDFEnum **ppdfEnumAsync)
{
    *ppdfEnumAsync = _pDFEnumAsync;
    return S_OK;
}

STDMETHODIMP CDFFolder::SetAsyncCount(DBCOUNTITEM cCount)
{
    _cAsyncItems = cCount;
    _fAllAsyncItemsCached = FALSE;
    return S_OK;
}

STDMETHODIMP CDFFolder::ClearSaveStateList()
{
    DSA_DeleteAllItems(this->_hdsaSaveStateForIDs);
    _cSaveStateSelected = 0;
    return S_OK;
}

STDMETHODIMP CDFFolder::GetStateFromSaveStateList(DWORD dwItemID, DWORD *pdwState)
{
    ESFSaveStateItem *pessi;
    int i;
    for (i = DSA_GetItemCount(this->_hdsaSaveStateForIDs); i-- > 0; )
    {
        // Pidl at start of structure...
        pessi = (ESFSaveStateItem*)DSA_GetItemPtr(this->_hdsaSaveStateForIDs, i);
        if  (pessi->dwItemID == dwItemID)
        {    
            *pdwState = pessi->dwState;
            if (pessi->dwState & LVIS_SELECTED)
            {
                // Remember the counts of items that we have touched...
                _dflvrSel.IncrementIncludedCount();
                _cSaveStateSelected--;
            }

            // Any items we retrieve we can get rid of...
            DSA_DeleteItem(this->_hdsaSaveStateForIDs, i);

            return S_OK;
        }
    }
    return S_FALSE;
}

STDMETHODIMP CDFFolder::GetFolderListItemCount(INT *pcItemCount)
{ 
    *pcItemCount = 0;

    EnterCriticalSection(&_csSearch);
    if (this->hdpaPidf)
        *pcItemCount = DPA_GetPtrCount(this->hdpaPidf);
    LeaveCriticalSection(&_csSearch);
     
    return S_OK;
}

STDMETHODIMP CDFFolder::GetFolderListItem(int iItem, DFFolderListItem **ppdffi)
{ 
    EnterCriticalSection(&_csSearch);
    *ppdffi = (DFFolderListItem *) DPA_GetPtr(this->hdpaPidf, iItem);
    LeaveCriticalSection(&_csSearch);
    return *ppdffi ? S_OK : E_FAIL;
}




// Now define a simple wrapper IContext menu which allows us to catch
// special things like Create Link...
//
// Also this is a DELEGATING icontextmenu implementation. We could rip
// the delegating stuff into a base class which might be useful somewhere...
//
//=============================================================================
// CDefFolderMenu class
//=============================================================================

#define MAX_CM_WRAP 2
class CDFMenuWrap : public IContextMenu3, public CObjectWithSite
{

public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID,void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IContextMenu methods ***
    virtual STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    virtual STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    virtual STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // *** IContextMenu2 methods ***
    virtual STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IContextMenu3 methods ***
    virtual STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    //*** IObjectWithSite ***
    STDMETHOD(SetSite)(IUnknown *punkSite); // override

private:

    // This would be a completely generic IContextMenu delegating object except:
    //   1) There's this silly _pdtobj hack which is the reason it was first written and
    //   2) The constructor function only takes 2 pcms -- easily modifiable however
    //
    friend IContextMenu* DFWrapIContextMenus(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2);
    
    CDFMenuWrap(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2);
    ~CDFMenuWrap();
    void _InsertMenu(IContextMenu* pcm);

    UINT                _cRef;           // Reference count

    IDataObject *       _pdtobj;         // Data object

    UINT                _count;
    UINT                _idFirst;
    UINT                _idOffsets[MAX_CM_WRAP];        // The END of each range (BEGINing of next range is +1)
    IContextMenu        *_pcmItem[MAX_CM_WRAP];         // The contextmenu for the item
    IContextMenu2       *_pcm2Item[MAX_CM_WRAP];        // The contextmenu for the item
    IContextMenu3       *_pcm3Item[MAX_CM_WRAP];        // The contextmenu for the item
};


CDFMenuWrap::CDFMenuWrap(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2)
{
    _cRef = 1;

    ASSERT(pdo);
    pdo->AddRef();
    _pdtobj = pdo;

    _InsertMenu(pcm1);
    _InsertMenu(pcm2);
    
    ASSERT(_count>0); // we better have something or we're worthless
}

void CDFMenuWrap::_InsertMenu(IContextMenu* pcm)
{
    if (pcm)
    {
        if (_count >= MAX_CM_WRAP)
        {
            TraceMsg(TF_ERROR, "Someone's inserting more contextmenus than CDFMenuWrap can handle...");
        }
        else
        {
            pcm->AddRef();
            _pcmItem[_count] = pcm;
            pcm->QueryInterface(IID_IContextMenu2, (void**)&_pcm2Item[_count]);
            pcm->QueryInterface(IID_IContextMenu3, (void**)&_pcm3Item[_count]);
            _count++;
        }
    }
}

CDFMenuWrap::~CDFMenuWrap()
{
    ASSERT(_cRef == 0);

    for (UINT i = 0 ; i < _count ; i++)
    {
        ATOMICRELEASE(_pcmItem[i]);
        ATOMICRELEASE(_pcm2Item[i]);
        ATOMICRELEASE(_pcm3Item[i]);
    }

    if (_pdtobj)
        _pdtobj->Release();
}

STDMETHODIMP CDFMenuWrap::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CDFMenuWrap, IContextMenu, IContextMenu3),    // IID_IContextMenu
        QITABENTMULTI(CDFMenuWrap, IContextMenu2, IContextMenu3),   // IID_IContextMenu2
        QITABENT(CDFMenuWrap, IContextMenu3),                       // IID_IContextMenu3
        QITABENT(CDFMenuWrap, IObjectWithSite),                     // IID_IObjectWithSite
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDFMenuWrap::AddRef()
{
    _cRef++;
    TraceMsg(TF_DOCFIND, "dfmw.AddRef %d",_cRef);
    return _cRef;
}


STDMETHODIMP_(ULONG) CDFMenuWrap::Release()
{
    ASSERT(_cRef >= 1);                    // Don't release more than we can!
    _cRef--;
    TraceMsg(TF_DOCFIND, "dfmw.Release %d",_cRef);

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;

    return 0;

}

STDMETHODIMP CDFMenuWrap::SetSite(IUnknown *punkSite)
{
    // let all the kids know
    for (UINT i = 0 ; i < _count ; i++)
    {
        IUnknown_SetSite(_pcmItem[i], punkSite);
    }

    return CObjectWithSite::SetSite(punkSite);
}

STDMETHODIMP CDFMenuWrap::QueryContextMenu(
        HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    HRESULT hres;
    
    // simply foward this to the one we are wrapping...
    TraceMsg(TF_DOCFIND,"dfmw.queryContextMenu, iMenu=%d, iFirst=%d, iLast=%d",
            indexMenu, idCmdFirst, idCmdLast);

    _idFirst = idCmdFirst;
    for (UINT i = 0 ; i < _count  && idCmdFirst < idCmdLast; i++)
    {
        hres = _pcmItem[i]->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
        if (SUCCEEDED(hres))
        {
            _idOffsets[i] = idCmdFirst - _idFirst + (UINT)ShortFromResult(hres);
            idCmdFirst = idCmdFirst + (UINT)ShortFromResult(hres) + 1;
        }
        else
        {
            if (0 == i)
                _idOffsets[i] = 0;
            else
                _idOffsets[i] = _idOffsets[i-1];
        }
    }
    
    return hres;

}

STDMETHODIMP CDFMenuWrap::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hres;
    
    TraceMsg(TF_DOCFIND, "dfmw.InvokeCommand, verbstr=%lx", lpici->lpVerb);

    // This is sortof Gross, but we will attempt to pickoff the Link command
    // which looks like the pcmitem will be SHARED_FILE_LINK....
    // NOTE: This assumes the filesys context menu is FIRST in our array
    if ((UINT_PTR)lpici->lpVerb == SHARED_FILE_LINK ||
        (!IS_INTRESOURCE(lpici->lpVerb) && (lstrcmpiA(lpici->lpVerb, "link") == 0)))
    {
        // Note: old code used to check pdtobj, but we don't create this
        //       object unless we get one of them, so why check?
        ASSERT(_pdtobj);
        return SHCreateLinks(lpici->hwnd, NULL, _pdtobj,
                SHCL_USETEMPLATE | SHCL_USEDESKTOP | SHCL_CONFIRM, NULL);
    }

    for (UINT i = 0 ; i < _count ; i++)
    {
        if (IS_INTRESOURCE(lpici->lpVerb))
        {
            UINT idCmd = (UINT)LOWORD((DWORD_PTR)lpici->lpVerb);
            if (idCmd <= _idOffsets[i])
            {
                // adjust id to be in proper range for this pcm
                if (i>0)
                {
                    lpici->lpVerb = MAKEINTRESOURCEA(idCmd - _idOffsets[i-1] - 1);
                }
                hres = _pcmItem[i]->InvokeCommand(lpici);
                return hres;
            }
        }
        else
        {
            // I guess we try until it works
            hres = _pcmItem[i]->InvokeCommand(lpici);
            if (SUCCEEDED(hres))
                return hres;
        }
    }
    
    TraceMsg(TF_ERROR, "Someone's passing CDFMenuWrap::InvokeCommand an id we didn't insert...");
    return E_FAIL;
}

STDMETHODIMP CDFMenuWrap::GetCommandString(
        UINT_PTR idCmd, UINT wFlags, UINT * pmf,
        LPSTR pszName, UINT  cchMax)
{
    for (UINT i = 0 ; i < _count ; i++)
    {
        if (idCmd <= _idOffsets[i])
        {
            // adjust id to be in proper range for this pcm
            if (i>0)
            {
                idCmd = idCmd - _idOffsets[i-1] - 1;
            }
        
            return _pcmItem[i]->GetCommandString(idCmd, wFlags, pmf, pszName, cchMax);
        }
    }

    TraceMsg(TF_ERROR, "Someone's passing CDFMenuWrap::GetCommandString an id we didn't insert...");
    return E_FAIL;
}

STDMETHODIMP CDFMenuWrap::HandleMenuMsg(
        UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

STDMETHODIMP CDFMenuWrap::HandleMenuMsg2(
        UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hres = E_FAIL;
    UINT idCmd;

    // Find the menu command id -- it's packed differently depending on the message
    //
    switch (uMsg) {
    case WM_MEASUREITEM:
        idCmd = ((MEASUREITEMSTRUCT *)lParam)->itemID;
        break;

    case WM_DRAWITEM:
        idCmd = ((LPDRAWITEMSTRUCT)lParam)->itemID;
        break;

    case WM_INITMENUPOPUP:
        idCmd = GetMenuItemID((HMENU)wParam, 0);
        break;

    case WM_MENUSELECT:
        idCmd = (UINT) LOWORD(wParam);
        break;

    case WM_MENUCHAR:
    case WM_NEXTMENU:
        idCmd = GetMenuItemID((HMENU)lParam, 0);
        break;

    default:
        return E_FAIL;
    }

    // make sure it's in our range
    if (idCmd >= _idFirst)
    {
        idCmd -= _idFirst;
        
        for (UINT i = 0 ; i < _count ; i++)
        {
            if (idCmd <= _idOffsets[i])
            {
                if (_pcm3Item[i])
                    hres = _pcm3Item[i]->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
                else if (_pcm2Item[i] && NULL == plResult)
                    hres = _pcm2Item[i]->HandleMenuMsg(uMsg, wParam, lParam);
                break;
            }
        }
    }

    return hres;
}

HRESULT DFWrapIContextMenu(HWND hwndOwner, IShellFolder *psfItem, LPCITEMIDLIST pidl,
                           IContextMenu* pcmExtra, void **ppvInOut)
{
    HRESULT hres;
    IContextMenu *pcmWrap = NULL;
    IContextMenu *pcmFree = (IContextMenu*)*ppvInOut;

    IDataObject* pdo;
    hres = psfItem->GetUIObjectOf(hwndOwner,
            1, &pidl, IID_IDataObject, NULL, (void **) &pdo);
    if (SUCCEEDED(hres))
    {
        pcmWrap = DFWrapIContextMenus(pdo, pcmFree, pcmExtra);
        if (!pcmWrap)
            hres = E_OUTOFMEMORY;
        pdo->Release();
    }

    pcmFree->Release();
    *ppvInOut = pcmWrap;
    
    return hres;
}

IContextMenu* DFWrapIContextMenus(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2)
{
    CDFMenuWrap * pcm = new CDFMenuWrap(pdo, pcm1, pcm2);

    return SAFECAST(pcm, IContextMenu*);
}


STDMETHODIMP CDFFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDFFolder, IShellFolder2),          //IID_ISHELLFolder2
        QITABENTMULTI(CDFFolder, IShellFolder, IShellFolder2),   // IID_IShellFolder
        QITABENT(CDFFolder, IDocFindFolder),        //IID_IDocFindFolder
        QITABENT(CDFFolder, IShellIcon),            //IID_IShellIcon
        QITABENT(CDFFolder, IPersistFolder2),       //IID_IPersistFolder2
        QITABENTMULTI(CDFFolder, IPersistFolder, IPersistFolder2), //IID_IPersistFolder
        QITABENTMULTI(CDFFolder, IPersist, IPersistFolder2),      //IID_IPersist
        QITABENT(CDFFolder, IShellIconOverlay),     //IID_IShellIconOverlay
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}
    
// IPersistFolder2 implementation
STDMETHODIMP CDFFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DocFindFolder;
    return S_OK;
}

STDMETHODIMP CDFFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (_pidl)
        ILFree(_pidl);

    return SHILClone(pidl, &_pidl);
}

STDMETHODIMP CDFFolder::GetCurFolder(LPITEMIDLIST *ppidl) 
{    
    return GetCurFolderImpl(_pidl, ppidl);
}


// helper function to sort the selected ID list by something that
// makes file operations work reasonably OK, when both an object and it's
// parent is in the list...
//
int CALLBACK CDFFolder_SortForFileOp(void *lp1, void *lp2, LPARAM lparam)
{
    // Since I do recursion, If I get the Folder index number from the
    // last element of each and sort by them such that the higher numbers
    // come first, should solve the problem fine...
    LPITEMIDLIST pidl1 = (LPITEMIDLIST)ILFindLastID((LPITEMIDLIST)lp1);
    LPITEMIDLIST pidl2 = (LPITEMIDLIST)ILFindLastID((LPITEMIDLIST)lp2);

    return DF_IFOLDER(pidl2) - DF_IFOLDER(pidl1);
}

LPITEMIDLIST CDFFolder::_GetFullPidlForItem(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlParent;
    if (S_OK == GetParentsPIDL(pidl, &pidlParent))
    {
        return ILCombine(pidlParent, pidl);
    }
    return NULL;
}

STDMETHODIMP CDFFolder::GetUIObjectOf(HWND hwndOwner,
                                      UINT cidl, LPCITEMIDLIST * apidl,
                                      REFIID riid, UINT * prgfInOut, void **ppvOut)
{
    HRESULT hres = E_INVALIDARG;

    *ppvOut = NULL;

    // If Count of items passed in is == 1 simply pass to the appropriate
    // folder
    if (cidl == 1)
    {
        // Note we may have been passed in a complex item so find the last
        // id.

        ASSERT(ILIsEmpty(_ILNext(*apidl)));  // should be a single level PIDL!

        LPCITEMIDLIST pidl = ILFindLastID(*apidl);  // BUGBUG, this is bogus
        IShellFolder *psfItem = DocFind_GetObjectsIFolder(this, NULL, pidl);
        if (psfItem)
        {
            hres = psfItem->GetUIObjectOf(hwndOwner, 1, &pidl, riid, prgfInOut, ppvOut);

            // if we are doing context menu, then we will wrap this
            // interface in a wrapper object, that we can then pick
            // off commands like link to process specially
            if (SUCCEEDED(hres) && IsEqualIID(riid, IID_IContextMenu))
            {
                // we also let the net/file guy add in a context menu if they want to
                IContextMenu* pcmExtra = NULL;
                pdfff->GetItemContextMenu(hwndOwner, SAFECAST(this, IDocFindFolder*), &pcmExtra);
                
                hres = DFWrapIContextMenu(hwndOwner, psfItem, pidl, pcmExtra, ppvOut);

                ATOMICRELEASE(pcmExtra);
            }
            psfItem->Release();
        }
        return hres;   // error...
    }

    if (IsEqualIID(riid, IID_IContextMenu))
    {
        // Is there any selection?
        if (cidl == 0)
        {
            hres = E_INVALIDARG;
        }
        else
        {
            IContextMenuCB *pcmcb;

            // So we have at least two items in the list.

            // Try to create a menu object that we process ourself
            // Yes, do context menu.
            HKEY ahkeys[3] = {0, 0, 0};

            LPITEMIDLIST pidlFull = _GetFullPidlForItem(apidl[0]);
            if (pidlFull)
            {
                // Get the hkeyProgID and hkeyBaseProgID from the first item.
                SHGetClassKey(pidlFull, &ahkeys[0], &ahkeys[1]);
                ILFree(pidlFull);
            }
            
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("AllFilesystemObjects"), &ahkeys[2]);
            
            pcmcb = new CDFContextMenuCB();
            if (pcmcb)
            {
                hres = CDefFolderMenu_Create2Ex(NULL, hwndOwner,
                                cidl, apidl, this, pcmcb,
                                ARRAYSIZE(ahkeys), ahkeys,
                                (IContextMenu **)ppvOut);
                pcmcb->Release();
            }

            SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
        }
    }
    else if (cidl && IsEqualIID(riid, IID_IDataObject))
    {
        // We need to generate a data object that each item as being
        // fully qualified.  This is a pain, but...
        // This is a really gross use of memory!

        UINT i;
        HDPA hdpa = DPA_Create(0);
        if (!hdpa)
            return hres;

        if (!DPA_Grow(hdpa, cidl))
        {
            DPA_Destroy(hdpa);
            return hres;
        }
        for (i = 0; i < cidl; i++)
        {
            LPITEMIDLIST pidl = _GetFullPidlForItem(apidl[i]);
            if (pidl)
                DPA_InsertPtr(hdpa, i, pidl);
        }

        // In order to make file manipulation functions work properly we
        // need to sort the elements to make sure if an element and one
        // of it's parents are in the list, that the element comes
        // before it's parents...
        DPA_Sort(hdpa, CDFFolder_SortForFileOp, 0);

        hres = CFSFolder_CreateDataObject(&c_idlDesktop, cidl, (LPCITEMIDLIST*)DPA_GetPtrPtr(hdpa),
                                           (IDataObject **)ppvOut);
        //
        // now to cleanup what we created.
        //
        for (i = 0; i < cidl; i++)
        {
            ILFree((LPITEMIDLIST)DPA_FastGetPtr(hdpa, i));
        }
        DPA_Destroy(hdpa);
    }

    return hres;
}

STDMETHODIMP CDFFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwRes, LPSTRRET pStrRet)
{
    HRESULT hr = E_INVALIDARG;
    IShellFolder *psfItem = DocFind_GetObjectsIFolder(this, NULL, pidl);
    if (psfItem)
    {
        hr = psfItem->GetDisplayNameOf(pidl, dwRes, pStrRet);
        psfItem->Release();
    }
    return hr;
}

STDMETHODIMP CDFFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                                  LPCOLESTR lpszName, DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = E_INVALIDARG;
    IShellFolder *psfItem = DocFind_GetObjectsIFolder(this, NULL, pidl);
    if (psfItem)
    {
        hr = psfItem->SetNameOf(hwnd, pidl, lpszName, dwRes, ppidlOut);
        psfItem->Release();
    }
    return hr;
}


STDMETHODIMP CDFFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    return pdfff->GetDefaultSearchGUID(SAFECAST(this, IShellFolder2*), lpGuid);
}

STDMETHODIMP CDFFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    return pdfff->EnumSearches(SAFECAST(this, IShellFolder2*), ppenum);
}

HRESULT CDFFolder::_QueryItemInterface(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    IShellFolder *psf = DocFind_GetObjectsIFolder(this, NULL, pidl);
    if (psf)
    {
        hr = psf->QueryInterface(riid, ppv);
        psf->Release();
    }
    return hr;
}

STDMETHODIMP CDFFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDFFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDFFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    IShellFolder2 *psf2;
    HRESULT hr = _QueryItemInterface(pidl, IID_IShellFolder2, (void **)&psf2);
    if (SUCCEEDED(hr))
    {
        hr = psf2->GetDetailsEx(pidl, pscid, pv);
        psf2->Release();
    }
    return hr;
}

STDMETHODIMP CDFFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    // first let the filters have a crack at this...
    IDocFindFileFilter *pdfff;
    HRESULT hres = GetDocFindFilter(&pdfff);
    if (SUCCEEDED(hres)) 
    {
        EnterCriticalSection(&_csSearch);
        hres = pdfff->GetDetailsOf(hdpaPidf, pidl, &iColumn, pdi);
        LeaveCriticalSection(&_csSearch);
        pdfff->Release();

        // special case to go to async guy...
#ifdef IF_ADD_MORE_COLS
        if (hres == S_FALSE)
        {
            if (_pDFEnumAsync)
            {
                // OK lets ask it for the information...
                // First we need to get which row it is in.
                CDFItem *pesfi;
                if (SUCCEEDED(GetItem(_iGetIDList, &pesfi)) 
                    && pesfi
                    && ((&pesfi->idl == pidl) || ILIsEqual(pidl, &pesfi->idl)))
                {
                    // Yep it was ours so return the index quickly..
                    return _pDFEnumAsync->GetExtendedDetailsOf(pidl, iColumn, pdi);
                }
            }
            else
                hres = S_OK;  // No extended data so ignore it for now...
        }
#endif
    }
    return hres;
}

STDMETHODIMP CDFFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    return E_NOTIMPL;
}


STDMETHODIMP CDFFolder::SetDocFindFilter(IDocFindFileFilter *pdfffNew)
{
    // TODO: add type checked version of IUnknown_Set to avoid code like this [t-ashram]
    if (pdfff) 
        pdfff->Release();

    pdfff = pdfffNew;

    if (pdfff) 
        pdfff->AddRef();

    return S_OK;
}

STDMETHODIMP CDFFolder::GetDocFindFilter(IDocFindFileFilter **ppdfff)
{
    *ppdfff = pdfff;
    if (pdfff)
        pdfff->AddRef();
    
    return pdfff ? S_OK : E_FAIL;
}

//===========================================================================
// CDFFolder::IShellIcon : Members
//===========================================================================

//
// GetIconOf
//
STDMETHODIMP CDFFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    IShellIcon * psiItem;
    HRESULT hres = _QueryItemInterface(pidl, IID_IShellIcon, (void **)&psiItem);
    if (SUCCEEDED(hres))
    {
        hres = psiItem->GetIconOf(pidl, flags, piIndex);
        psiItem->Release();
    }
    return hres;
}

//===========================================================================
// CDFFolder::IShellIconOverlay : Members
//===========================================================================


STDMETHODIMP CDFFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int * pIndex)
{
    IShellIconOverlay * psioItem;
    HRESULT hres = _QueryItemInterface(pidl, IID_IShellIconOverlay, (void **)&psioItem);
    if (SUCCEEDED(hres))
    {
        hres = psioItem->GetOverlayIndex(pidl, pIndex);
        psioItem->Release();
    }
    return hres;
}

STDMETHODIMP CDFFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int * pIndex)
{
    return E_NOTIMPL;
}


STDMETHODIMP CDFFolder::RestoreSearchFromSaveFile(LPITEMIDLIST pidlSaveFile, IShellFolderView *psfv)
{
    // See if we can restore most of the search from here...
    IStream *pstm;
    ULONG cbRead;
    TCHAR szPathName[MAX_PATH];
    LARGE_INTEGER dlibMove = {0, 0};
    DFHEADER dfh;
    int cItems = 0;
    HRESULT hr;

    //
    // First try to open the file
    //

    SHGetPathFromIDList(pidlSaveFile, szPathName);
    if (FAILED(hr = SHCreateStreamOnFile(szPathName, STGM_READ, &pstm)))
        return hr;

    // Now Read in the header
    // Note: in theory I should test the size read by the size of the
    // smaller headers, but if the number of bytes read is smaller than
    // the few new things added then there is nothing to restore anyway...

    // Note: Win95/NT4 incorrectly failed newer versions of this structure.
    // Which is bogus since the struct was backward compatible (that's what
    // the offsets are for).  We fix for NT5 and beyond, but downlevel
    // systems are forever broken.  Hopefully this feature is rarely enough
    // used (and never mailed) that nobody will notice we're broken.
    //
    if (FAILED(pstm->Read(&dfh, SIZEOF(DFHEADER), &cbRead)) || 
        (cbRead != SIZEOF(DFHEADER)) || (dfh.wSig != DOCFIND_SIG))
    {
        // Not the correct format...
        pstm->Release();
        return E_FAIL;
    }

#if 0
    //BUGBUG:: need to add support for remembering which view we are in and then restoring it.
    if (dfh.wVer < 3)
    {
        // The ViewMode was added in version 3.
        dfh.ViewMode = FVM_DETAILS;
    }
#endif
    // Now try to read in the criteria...
#ifdef WINNT
    DFC_UNICODE_DESC desc;
    WORD fCharType = 0;

    //
    // Check the stream's signature to see if it was generated by Win95 or NT.
    //
    dlibMove.QuadPart = 0 - SIZEOF(desc);
    pstm->Seek(dlibMove, STREAM_SEEK_END, NULL);
    pstm->Read(&desc, SIZEOF(desc), &cbRead);
    if (cbRead > 0 && desc.NTsignature == c_NTsignature)
    {
       //
       // NT-generated stream.  Read in Unicode criteria.
       //
       fCharType         = DFC_FMT_UNICODE;
       dlibMove.QuadPart = desc.oUnicodeCriteria.QuadPart;
    }
    else
    {
       //
       // Win95-generated stream.  Read in ANSI criteria.
       //
       fCharType        = DFC_FMT_ANSI;
       dlibMove.LowPart = dfh.oCriteria;
    }
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    pdfff->RestoreCriteria(pstm, dfh.cCriteria, fCharType);

#else
    //
    // Now Read in the criteria
    //
    dlibMove.LowPart = dfh.oCriteria;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

    pdfff->RestoreCriteria(pstm, dfh.cCriteria, DFC_FMT_ANSI);
#endif

    // Now read in the results
    dlibMove.LowPart = dfh.oResults;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);


    if (dfh.wVer >  1)
    {
        // only restore this way if version 2 data....
        // Now Restore away the folder list
        RestoreFolderList(pstm);
        RestoreItemList(pstm, &cItems);
        if (cItems > 0)
            psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
    }

    //
    // and close the file
    //
    pstm->Release();

    return S_OK;
}

STDMETHODIMP CDFFolder::SetEmptyText(LPCTSTR pszText)
{
    if (pszText && 0 == lstrcmp(_szEmptyText, pszText))
        return S_OK ;

    if (pszText)
        lstrcpyn(_szEmptyText, pszText, ARRAYSIZE(_szEmptyText));
    else
        *_szEmptyText = 0 ;
    

    if (IsWindow(_hwndLV))
        SendMessage(_hwndLV, LVM_RESETEMPTYTEXT, 0, 0L);

    return S_OK ;
}

BOOL CDFFolder::IsSlow()
{
    BOOL bRet = FALSE;

    if (IsWindow(_hwndLV))
    {
        LONG lStyle = GetWindowLong(_hwndLV, GWL_STYLE) & LVS_TYPEMASK;

        bRet = lStyle == LVS_ICON || lStyle == LVS_SMALLICON;
    }
    return bRet;
}

// this is the same as shfindfiles except without the gross hack
//
BOOL RealFindFiles(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile)
{
    HRESULT       hres;
    IWebBrowser2* pwb2 = NULL;

    // First create the top level browser...
    hres = CoCreateInstance(CLSID_ShellBrowserWindow, NULL, CLSCTX_LOCAL_SERVER,
                            IID_IWebBrowser2, (void **)&pwb2);

    if (SUCCEEDED( hres ) && pwb2)
    {   
        SA_BSTR bstrClsid;
        VARIANT varClsid;
        VARIANT varEmpty = {0};

        SHStringFromGUIDW(CLSID_FileSearchBand, bstrClsid.wsz, ARRAYSIZE(bstrClsid.wsz));
        bstrClsid.cb = lstrlenW(bstrClsid.wsz) * sizeof(WCHAR);
        varClsid.vt = VT_BSTR;
        varClsid.bstrVal = bstrClsid.wsz;

        // show a search bar
        hres = pwb2->ShowBrowserBar(&varClsid, &varEmpty, &varEmpty);
        if (SUCCEEDED(hres))
        {
            //  Grab the band's IUnknown from browser property.
            VARIANT varFsb ;
            BSTR    bstrProperty = NULL ;
            WCHAR   wszProperty[GUIDSTR_MAX+1] ;
            
            EVAL( SUCCEEDED( SHStringFromGUIDW( CLSID_FileSearchBand, wszProperty, ARRAYSIZE(wszProperty) ) ) ) ;
            VariantInit( &varFsb ) ;

            if( (bstrProperty = SysAllocString( wszProperty )) != NULL &&
                SUCCEEDED( pwb2->GetProperty( bstrProperty, &varFsb ) ) )
            {
                if( VT_UNKNOWN == varFsb.vt && varFsb.punkVal )
                {
                    //  QI for IFileSearchBand, which we'll use to program the search band's
                    //  search type (files or folders), inititial scope, and/or saved query file.
                    IFileSearchBand* pfsb ;
                    if( SUCCEEDED( varFsb.punkVal->QueryInterface( IID_IFileSearchBand, (PVOID*)&pfsb ) ) )
                    {
                        WCHAR   wszSearch[GUIDSTR_MAX+1] ;
                        BSTR    bstrSearch ;
                        
                        //  Get the File/Folders search guid in string form
                        EVAL( SUCCEEDED( SHStringFromGUIDW( SRCID_SFileSearch, wszSearch, ARRAYSIZE(wszSearch) ) ) ) ;
                        
                        if( (bstrSearch = SysAllocString( wszSearch )) != NULL )
                        {                           
                            VARIANT varQueryFile ;
                            VARIANT varScope ;
                            VariantInit( &varQueryFile ) ;
                            VariantInit( &varScope ) ;

                            //  assign initial scope
                            if( pidlFolder )
                                InitVariantFromIDList( &varScope, pidlFolder ) ;

                            //  assign query file from which to restore search
                            else if( pidlSaveFile )
                                InitVariantFromIDList( &varQueryFile, pidlSaveFile ) ;

                            pfsb->SetSearchParameters( &bstrSearch, VARIANT_TRUE, &varScope, &varQueryFile ) ;
                        }
                        pfsb->Release() ;
                    }
                }
                VariantClear( &varFsb ) ;
            }
            SysFreeString( bstrProperty ) ;

            if (SUCCEEDED(hres))
                hres = pwb2->put_Visible(TRUE);
        }
        pwb2->Release();
    }
    else
        hres = E_FAIL;

    return hres;
}

//==========================================================================
//
// This is the main external entry point to start a search.  This will
// create a new thread to process the
//
STDAPI_(BOOL) SHFindFiles(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile)
{
    // are we allowed?
    if (SHRestricted(REST_NOFIND))
        return FALSE;
        
    // We Need a hack to allow Find to work for cases like
    // Rest of network and workgroups to map to find computer instead
    // This is rather gross, but what the heck.  It is also assumed that
    // the pidl is of the type that we know about (either File or network)
    if (pidlFolder)
    {
        BYTE bType;
        bType = SIL_GetType(ILFindLastID(pidlFolder));
        if ((bType == SHID_NET_NETWORK) ||
               (bType == SHID_NET_RESTOFNET) ||
               (bType == SHID_NET_DOMAIN))
            return SHFindComputer(pidlFolder, pidlSaveFile);
    }

    return RealFindFiles(pidlFolder, pidlSaveFile);
}


//==========================================================================
//
// This is the main external entry point to start a search.  This will
// create a new thread to process the
//
STDAPI_(BOOL) SHFindComputer(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile)
{
    IContextMenu *pcm;
    HRESULT hres = CoCreateInstance(CLSID_ShellSearchExt, NULL, CLSCTX_INPROC_SERVER, IID_IContextMenu, (void **)&pcm);
    if (SUCCEEDED(hres))
    {
        CMINVOKECOMMANDINFO ici = {0};

        ici.cbSize = SIZEOF(ici);
        ici.lpParameters = "{996E1EB1-B524-11d1-9120-00A0C98BA67D}"; // Search Guid of Find Computers
        ici.nShow  = SW_NORMAL;

        hres = pcm->InvokeCommand(&ici);

        pcm->Release();
    }
    return SUCCEEDED(hres);
}

HRESULT SelectPidlInSFV(IShellFolderViewDual *psfv, LPCITEMIDLIST pidl, DWORD dwOpts)
{
    VARIANT var;
    HRESULT hres = InitVariantFromIDList(&var, pidl);
    if (SUCCEEDED(hres)) 
    {
        hres = psfv->SelectItem(&var, dwOpts);
        VariantClear(&var);
    }
    return hres;
}


HRESULT OpenContainingFolderAndGetShellFolderView(LPCITEMIDLIST pidlFolder, IShellFolderViewDual **ppsfv)
{
    *ppsfv = NULL;

    IWebBrowserApp *pauto;
    HRESULT hres = SHGetIDispatchForFolder(pidlFolder, &pauto);
    if (SUCCEEDED(hres))
    {
        // WARNING!  This needs to change to LONG_PTR once the MIDL
        // compiler supports it.
        HWND hwnd;
        if (SUCCEEDED(pauto->get_HWND((LONG*)&hwnd)))
        {
            // Make sure we make this the active window
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOWNORMAL);
        }

        // We have IDispatch for window, now try to get one for
        // the folder object...
        IDispatch *pautoDoc;
        hres = pauto->get_Document(&pautoDoc);
        if (SUCCEEDED(hres))
        {
            hres = pautoDoc->QueryInterface(IID_IShellFolderViewDual, (void **)ppsfv);
            pautoDoc->Release();
        }
        pauto->Release();
    }
    return hres;
}


void DFB_HandleOpenContainingFolder(IShellFolderView* psfv, IDocFindFolder* pdff)
{
    LPCITEMIDLIST *ppidls;    // pointer to a list of pidls.
    UINT cpidls;             // Count of pidls that were returned.

    // Ok lets ask the view for the list of selected objects.
    psfv->GetSelectedObjects(&ppidls, &cpidls);
    if (cpidls > 0)
    {
        UINT i;
        for (i = 0; i < cpidls; i++)
        {
            LPITEMIDLIST pidl;
            LPITEMIDLIST pidlT;
            IShellFolderViewDual *psfvDual;

            // See if we have already processed this one.
            if (ppidls[i] == NULL)
                continue;

            // Now get the parent of it.
            pdff->GetParentsPIDL(ppidls[i], &pidl);

            if (SUCCEEDED(OpenContainingFolderAndGetShellFolderView(pidl, &psfvDual)))
            {
                UINT j;
                SelectPidlInSFV(psfvDual, ppidls[i], SVSI_SELECT | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE);

                // Now see if there are any other items selected in
                // this same cabinet.  If so we might as well process
                // them here and save the work of trying to open it
                // again and the like!
                for (j=i+1; j < cpidls; j++)
                {
                    if (ppidls[j] == NULL)
                        continue;

                    // Now see if it has the same parent as we
                    // are processing...
                    pdff->GetParentsPIDL(ppidls[j], &pidlT);
                    if (pidlT == pidl)
                    {
                        SelectPidlInSFV(psfvDual, ppidls[j], SVSI_SELECT);
                        ppidls[j] = NULL;   // dont process again.
                    }
                }

                psfvDual->Release();
            }
        }
    }

    // Free the memory associated with the item list.
    if (ppidls != NULL)
        LocalFree((HLOCAL)ppidls); 
}

#ifdef DOCFIND_SAVERESULTS_ENABLED
// Hook procedure to allow us to our Save Results checkbox to the Save file dialog
UINT_PTR APIENTRY DocFindSaveDialogHookProc( 
    HWND hWnd, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam)
{
    DFBSAVEINFO * pdfb;
    switch ( msg )
    {
    case WM_INITDIALOG:
        pdfb = (DFBSAVEINFO*)((LPOPENFILENAME)lParam)->lCustData;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pdfb);
        CheckDlgButton( hWnd, IDD_SAVERESULTS, 
                        (pdfb->dwFlags & DFOO_SAVERESULTS) ? BST_CHECKED : BST_UNCHECKED);
        break;
    case WM_NOTIFY:         
        switch (((LPOFNOTIFY)lParam)->hdr.code)
        {
        case CDN_FILEOK: 
            {
                // See if the Save results option changed.
                BOOL fChecked;
                pdfb = (DFBSAVEINFO*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                fChecked = (BOOL) SendDlgItemMessage( hWnd, IDD_SAVERESULTS, BM_GETCHECK, 0, 0 );  
                if ((fChecked && ((pdfb->dwFlags & DFOO_SAVERESULTS) == 0)) ||
                    (!fChecked && (pdfb->dwFlags & DFOO_SAVERESULTS)))
                {
                    pdfb->dwFlags ^= DFOO_SAVERESULTS;
                    HKEY hkeyExp = SHGetExplorerHkey(HKEY_LOCAL_MACHINE, TRUE);
                    if (hkeyExp) 
                        SHSetValue(hkeyExp, s_szDocFind, s_szFlags, REG_BINARY, &pdfb->dwFlags, SIZEOF(pdfb->dwFlags));
                }
            }

            // Returning zero signifies that it is ok to exit the common dialog...
            return FALSE;
         } 
        break;
    }
    return ( FALSE );
}
#endif //DOCFIND_SAVERESULTS_ENABLED

//==========================================================================
//
// DFBSave
//      Save away the current search to a file on the desktop.
//      For now the name will be automatically generated.
//
void DFB_Save(IDocFindFileFilter* pdfff, HWND hwndOwner, DFBSAVEINFO * pSaveInfo,
              IShellView* psv, IDocFindFolder* pDocfindFolder, IUnknown *pObject)
{
    TCHAR szFilePath[MAX_PATH];
    IStream * pstm;
    DFHEADER dfh;
    TCHAR szTemp[MAX_PATH];
    SHORT cb;
    HRESULT hres;
    LARGE_INTEGER dlibMove = {0, 0};
    ULARGE_INTEGER libCurPos;
    FOLDERSETTINGS fs;

    LPCTSTR pszLastPath = c_szNULL;        // Null string to begin with.

    //
    // See if the search already has a file name associated with it.  If so
    // we will save it in it, else we will create a new file on the desktop
    if (pdfff->FFilterChanged() == S_FALSE)
    {
        // Lets blow away the save file
        ILFree(pSaveInfo->pidlSaveFile);
        pSaveInfo->pidlSaveFile = NULL;
    }

    // If it still looks like we want to continue to use a save file then
    // continue.
    if (pSaveInfo->pidlSaveFile)
    {
        SHGetPathFromIDList(pSaveInfo->pidlSaveFile, szFilePath);
    }
    else
    {
        LPTSTR lpsz;
        LPTSTR pszTitle;
        TCHAR  szShortName[12];

        // First get the path name to the Desktop.
        SHGetSpecialFolderPath(NULL, szFilePath, CSIDL_PERSONAL, TRUE);

        // and update the title
        // we now do this before getting a filename because we generate
        // the file name from the title

        pdfff->GenerateTitle(&pszTitle, TRUE);
        if (pszTitle)
        {
            // Now add on the extension.
            lstrcpyn(szTemp, pszTitle, MAX_PATH - (lstrlen(szFilePath) + 1 + 4 + 1+3));
            lstrcat(szTemp, TEXT(".fnd"));

            LocalFree(pszTitle);     // And free the title string.
        }


        // Now loop through and replace all of the invalid characters with _'s
        // we special case a few of the characters...
        for (lpsz = szTemp;  *lpsz; lpsz = CharNext(lpsz))
        {
            if (PathGetCharType(*lpsz) & (GCT_INVALID|GCT_WILD|GCT_SEPARATOR))
            {
                switch (*lpsz) {
                case TEXT(':'):
                    *lpsz = TEXT('-');
                    break;
                case TEXT('*'):
                    *lpsz = TEXT('@');
                    break;
                case TEXT('?'):
                    *lpsz = TEXT('!');
                    break;
                default:
                    *lpsz = TEXT('_');
                }
            }
        }

        LoadString(HINST_THISDLL, IDS_FIND_SHORT_NAME, szShortName, ARRAYSIZE(szShortName));
        if (!PathYetAnotherMakeUniqueName(szFilePath, szFilePath, szShortName, szTemp))
            return;
    }
    
    // Now lets bring up the save as dialog...
    TCHAR szFilter[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szFilename[MAX_PATH];
    LPTSTR psz;
    OPENFILENAME ofn = { 0 };

    LoadString(g_hinst, IDS_FINDFILESFILTER, szFilter, MAX_PATH);
    LoadString(g_hinst, IDS_FINDSAVERESULTSTITLE, szTitle, MAX_PATH);


    //Strip out the # and make them Nulls for SaveAs Dialog
    psz = szFilter;
    while (*psz)
    {
        if (*psz == TEXT('#'))
            *psz = TEXT('\0');
        psz++;
    }

    lstrcpy(szFilename, PathFindFileName(szFilePath));
    PathRemoveFileSpec(szFilePath);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hinst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = szFilePath;
    ofn.lpstrTitle = szTitle;
    ofn.lpstrDefExt = TEXT("fnd");
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | 
        OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

#ifdef DOCFIND_SAVERESULTS_ENABLED
    ofn.lpTemplateName= MAKEINTRESOURCE(DLG_DOCFIND_SAVE);
    ofn.lpfnHook= DocFindSaveDialogHookProc;
    ofn.lCustData = (LPARAM)pSaveInfo;
    ofn.Flags |= (OFN_ENABLETEMPLATE | OFN_ENABLEHOOK) ;
#else //DOCFIND_SAVERESULTS_ENABLED
    ofn.lpTemplateName = NULL;
    ofn.lpfnHook= NULL;
    ofn.lCustData = NULL;
#endif //DOCFIND_SAVERESULTS_ENABLED

    if (!GetSaveFileName(&ofn))
        return;

    if (FAILED(SHCreateStreamOnFile(szFilename, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstm)))
        return;

    // remember the file that we saved away to...
    ILFree(pSaveInfo->pidlSaveFile);
    pSaveInfo->pidlSaveFile = ILCreateFromPath(szFilename);

    // Now setup and write out header information
    ZeroMemory(&dfh, sizeof(dfh));
    dfh.wSig = DOCFIND_SIG;
    dfh.wVer = DF_CURFILEVER;
    dfh.dwFlags =  pSaveInfo->dwFlags;
    dfh.wSortOrder = (WORD)pSaveInfo->SortMode;
    dfh.wcbItem = SIZEOF(DFITEM);
    dfh.oCriteria = SIZEOF(dfh);
    // dfh.cCriteria = sizeof(s_aIndexes) / sizeof(SHORT);
    // dfh.oResults =;

    // Not used anymore...
    dfh.cResults = -1;

    //
    // Note: Later we may convert this to DOCFILE where the
    // criteria is stored as properties.
    //

    // Get the current Folder Settings
    if (SUCCEEDED(psv->GetCurrentInfo(&fs)))
        dfh.ViewMode = fs.ViewMode;
    else
        dfh.ViewMode = FVM_DETAILS;

    //
    // Now call the filter object to save out his own set of criterias

    dlibMove.LowPart = dfh.oCriteria;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    hres = pdfff->SaveCriteria(pstm, DFC_FMT_ANSI);
    if (SUCCEEDED(hres))
        dfh.cCriteria = GetScode(hres);

    // Now setup to output the results
    dlibMove.LowPart = 0;
    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos); // Get current pos
    dfh.oResults = libCurPos.LowPart;
    //
    // Now Let our file folder serialize his results out here also...
    // But only if the option is set to do so...
    //
    cb = 0;

#ifdef DOCFIND_SAVERESULTS_ENABLED
    if (pSaveInfo->dwFlags & DFOO_SAVERESULTS)
    {
        pDocfindFolder->SaveFolderList(pstm);       
        pDocfindFolder->SaveItemList(pstm);       
    }
    else
#endif DOCFIND_SAVERESULTS_ENABLED
    {
        // Write out a Trailing NULL for Folder list
        pstm->Write(&cb, SIZEOF(cb), NULL);
        // And item list.
        pstm->Write(&cb, SIZEOF(cb), NULL);
    }

    //
    // END of DFHEADER_WIN95 information
    // BEGIN of NT5 information:
    //

    // Now setup to output the history stream
    if (pObject)
    {
        dlibMove.LowPart = 0;
        pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos); // Get current pos
        dfh.oHistory = libCurPos.LowPart;

        if (FAILED(SavePersistHistory(pObject, pstm)))
        {
            // On failure we might as well just pretend we didn't save this bit of data.
            // Do we need an error message -- the ui won't be right when relaunched...
            //
            dfh.oHistory = 0;
            dlibMove.LowPart = libCurPos.LowPart;
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
        }
    }


// In NT the below was done AT THE END OF THE STREAM instead of
// revving the DFHEADER struct.  (Okay, DFHEADEREX, since Win95
// already broke DFHEADER back compat by in improper version check)
// This could have been done by putting a propery signatured
// DFHEADEREX that had proper versioning so we could add information
// to.  Unfortunately another hardcoded struct was tacked on to
// the end of the stream...   Next time, please fix the problem
// instead of work around it.
//
// What this boils down to is we cannot put any information
// after the DFC_UNICODE_DESC section, so might as well
// always do this SaveCriteria section last...
#ifdef WINNT
    {
       //
       // See comment at top of file for DFC_UNICODE_DESC.
       //
       DFC_UNICODE_DESC desc;

       //
       // Get the current location in stream.  This is the offset where
       // we'll write the unicode find criteria.  Save this
       // value (along with NT-specific signature) in the descriptor
       //
       dlibMove.LowPart = 0;
       pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos);

       desc.oUnicodeCriteria.QuadPart = libCurPos.QuadPart;
       desc.NTsignature               = c_NTsignature;

       // Append the Unicode version of the find criteria.
       hres = pdfff->SaveCriteria(pstm, DFC_FMT_UNICODE);

       // Append the unicode criteria descriptor to the end of the file.
       pstm->Write(&desc, SIZEOF(desc), NULL);
   }
#endif
    //
    // don't put any code between the above DFC_UNICDE_DESC section
    // and this back-patch of the dfh header...
    //
    // Finally output the header information at the start of the file
    // and close the file
    //
    pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
    pstm->Write((LPTSTR)&dfh, SIZEOF(dfh), NULL);
    pstm->Release();


    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pSaveInfo->pidlSaveFile, NULL);
    SHChangeNotify(SHCNE_FREESPACE, SHCNF_IDLIST, pSaveInfo->pidlSaveFile, NULL);

}

void DocFind_SetObjectCount(IShellView *psv, int cItems, DWORD dwFlags)
{
    IShellFolderView *psfv;

    if (SUCCEEDED(psv->QueryInterface(IID_IShellFolderView, (void **)&psfv))) 
    {
        psfv->SetObjectCount(cItems, dwFlags);
        psfv->Release();
    }
}


// Broke out from class to share with old and new code
BOOL DFB_handleUpdateDir(IDocFindFolder *pdfFolder, LPITEMIDLIST pidl, BOOL fCheckSubDirs)
{
    // 1. Start walk through list of dirs.  Find list of directories effected
    //    and mark them
    // 2. Walk the list of items that we have and mark each of the items that
    //    that are in our list of directories and then do a search...
    int iPidf;
    BOOL fCurrentItemsMayBeImpacted = FALSE;
    DFFolderListItem *pdffli;
    int iItem;
    INT cPidf;
    ESFItem *pesfi;
    IShellFolder *psfDesktop;

    SHGetDesktopFolder(&psfDesktop);

    // First see which directories are effected...
    pdfFolder->GetFolderListItemCount(&cPidf);
    for (iPidf = 0; iPidf < cPidf; iPidf++ )
    {        
        if (SUCCEEDED(pdfFolder->GetFolderListItem(iPidf, &pdffli)) 
            && !pdffli->fUpdateDir) // We may have already impacted these...
        {
            pdffli->fUpdateDir = ILIsParent(pidl, &pdffli->idl, FALSE);
            fCurrentItemsMayBeImpacted |= pdffli->fUpdateDir;
        }
    }

    if (fCurrentItemsMayBeImpacted)
    {
        // Now we need to walk through the whole list and remove any entries
        // that are no longer there...
        //
        if (SUCCEEDED(pdfFolder->GetItemCount(&iItem))) 
        {
            for (--iItem; iItem >= 0; iItem--)
            {
                WORD iFolder;
                if (FAILED(pdfFolder->GetItem(iItem, &pesfi)) || pesfi == NULL)
                    continue;
                iFolder = DF_IFOLDER(&pesfi->idl);
                
                // See if item may be impacted...
                if (SUCCEEDED(pdfFolder->GetFolderListItem(iFolder, &pdffli)) && pdffli->fUpdateDir)
                    pesfi->dwState |= CDFITEM_STATE_MAYBEDELETE;
            }
        }
    }

    return fCurrentItemsMayBeImpacted;
}

void DFB_UpdateOrMaybeAddPidl(IDocFindFolder *pdfFolder, IShellFolderView *psfv, int code, 
        LPITEMIDLIST pidl, LPITEMIDLIST pidlOld)
{
    IShellFolder *psf;
    LPITEMIDLIST pidlT;
    BOOL fMatch = FALSE;
    UINT iItem;
    ESFItem *pesfi;
    HRESULT hres;

    // First see if we should try to do an update...
    if (pidlOld)
    {
        if (pdfFolder->MapFSPidlToDFPidl(pidl, TRUE, &pidlT) == S_OK)
        {
            // SFV owns apidl[1] after this
            pdfFolder->SetItemsChangedSinceSort();
            hres = psfv->UpdateObject(pidlOld, pidlT, &iItem);

            ILFree(pidlT);  // In either case simply blow away our generated pidl...
            if (SUCCEEDED(hres))
                return;
        }
    }

    if (SUCCEEDED(SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, (LPCITEMIDLIST *) &pidlT)))
    {
        // See if this item matches the filter...
        IDocFindFileFilter  *pdfff;
        if (SUCCEEDED(pdfFolder->GetDocFindFilter(&pdfff))) {
            fMatch = pdfff->FDoesItemMatchFilter(NULL, NULL, psf, pidl) != 0;
            pdfff->Release();
        }

        psf->Release();

        if (fMatch)
        {
            LPITEMIDLIST pidlT;
            if (pdfFolder->MapFSPidlToDFPidl(pidl, TRUE, &pidlT) != S_OK)
            {
                // The folder has not been added before now...
                int iFolder;
                LPITEMIDLIST pidlFolder;
                TCHAR szPath[MAX_PATH];     // Don't add stuff in trashcan...
                SHGetPathFromIDList(pidl, szPath);
                if (IsFileInBitBucket(szPath))
                    return;     // yes, don't add it to the list...

                PathRemoveFileSpec(szPath);
                pidlFolder = ILCreateFromPath(szPath);
                if (!pidlFolder)
                    return;

                hres = pdfFolder->AddFolderToFolderList(pidlFolder, TRUE, &iFolder);
                ILFree(pidlFolder);

                if (FAILED(hres))
                    return;

                if (pdfFolder->MapFSPidlToDFPidl(pidl, TRUE, &pidlT) != S_OK)
                    return;
            }

            // There are times we get notified twice.  To handle this
            // see if the item is already in our list.  If so punt...
            // SFV owns apidl[1] after this
            pdfFolder->SetItemsChangedSinceSort();
            if (SUCCEEDED(psfv->UpdateObject(pidlT, pidlT, &iItem)))
            {
                ILFree(pidlT);
                return;     // We were able to add the item without problem...
            }

            // Normal case would be here to add the object
            // We need to add this to our dpa and dsa...
            if (SUCCEEDED(pdfFolder->GetItemCount((INT *)&iItem))) {
                pdfFolder->AddPidl(iItem, pidlT, (UINT)-1, &pesfi);
                ILFree(pidlT);
                if (pesfi)
                    psfv->SetObjectCount(++iItem, SFVSOC_NOSCROLL);
            }
        }
    }
}


void DFB_handleRMDir(IDocFindFolder *pdfFolder, IShellFolderView *psfv, LPITEMIDLIST pidl)
{
    BOOL fCurrentItemsMayBeImpacted = FALSE;
    DFFolderListItem *pdffli;
    int iItem;
    INT cItems;
    ESFItem *pesfi;


    // First see which directories are effected...
    pdfFolder->GetFolderListItemCount(&cItems);
    for (iItem = 0; iItem < cItems; iItem++ )
    {         
        if (SUCCEEDED(pdfFolder->GetFolderListItem(iItem, &pdffli)))
        {
            pdffli->fDeleteDir = ILIsParent(pidl, &pdffli->idl, FALSE);
            fCurrentItemsMayBeImpacted |= pdffli->fDeleteDir;
        }
        else 
        {
#ifdef DEBUG
            INT cItem;
            pdfFolder->GetFolderListItemCount(&cItem);
            TraceMsg(TF_WARNING, "NULL pdffli in _handleRMDir (iItem == %d, ItemCount()==%d)!!!", iItem, cItems);
#endif
        }
    }

    if (fCurrentItemsMayBeImpacted)
    {
        // Now we need to walk through the whole list and remove any entries
        // that are no longer there...
        //
        if (SUCCEEDED(pdfFolder->GetItemCount(&iItem))) {
            for (--iItem; iItem >= 0; iItem--)
            {
                WORD iFolder;
                if (FAILED(pdfFolder->GetItem(iItem, &pesfi)) || pesfi == NULL)
                    continue;
                iFolder = DF_IFOLDER(&pesfi->idl);

                // See if item may be impacted...
                if (SUCCEEDED(pdfFolder->GetFolderListItem(iFolder, &pdffli)) 
                    && pdffli->fDeleteDir) {

                    psfv->RemoveObject(&pesfi->idl, (UINT*)&cItems);
                }
            }
        }
    }
}





UINT GetControlCharWidth(HWND hwnd)
{
    SIZE siz;
    HDC hdc = GetDC(HWND_DESKTOP);
    HFONT hfOld = SelectFont(hdc, FORWARD_WM_GETFONT(hwnd, SendMessage));
    
    GetTextExtentPoint(hdc, TEXT("0"), 1, &siz);

    SelectFont(hdc, hfOld);
    ReleaseDC(HWND_DESKTOP, hdc);

    return siz.cx;
}

//////////////////////////////////////////////
////
////
////  Context menu stuff
////

TCHAR const c_szFindExtensions[] = TEXT("FindExtensions");

IContextMenu * WINAPI SHFind_InitMenuPopup(HMENU hmenu, HWND hwndOwner, UINT idCmdFirst, UINT idCmdLast)
{
    IContextMenu * pcm = NULL;
    HKEY hkFind = SHGetExplorerSubHkey(HKEY_LOCAL_MACHINE, c_szFindExtensions, FALSE);

    if (hkFind) {

        if (SUCCEEDED(CDefFolderMenu_CreateHKeyMenu(hwndOwner, hkFind, &pcm))) {
            int iItems = GetMenuItemCount(hmenu);
            // nuke all old entries
            while (iItems--) {
                DeleteMenu(hmenu, iItems, MF_BYPOSITION);
            }

            pcm->QueryContextMenu(hmenu, 0, idCmdFirst, idCmdLast, CMF_NODEFAULT|CMF_INCLUDESTATIC|CMF_FINDHACK);
            iItems = GetMenuItemCount(hmenu);
            if (!iItems) {
                TraceMsg(TF_DOCFIND, "no menus in find extension, blowing away context menu");
                pcm->Release();
                pcm = NULL;
            }
        }
        RegCloseKey(hkFind);
    }

    return pcm;
}

// This whole class is so "Find Computer" can redirect from a static item to this one.
// The FindFile stuff below is DEAD CODE.  BUT, it may be resurrected (see HACKHACK
// comment in defcm's _Static_Add function.
//
class CShellFindExt : public IShellExtInit, public IContextMenu
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(THIS_ REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // *** IShellExtInit methods ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, 
        HKEY hkeyProgID);

    // *** IContextMenu methods ***
    virtual STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast,
        UINT uFlags);
    virtual STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    virtual STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwReserved, LPSTR pszName,
        UINT cchMax);

    // constructor
    CShellFindExt();

private:    
    UINT                cRef;
    LPITEMIDLIST        pidl;
};

typedef CShellFindExt* LPSHELLFINDEXT;


STDMETHODIMP CShellFindExt::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellFindExt, IShellExtInit),         //IID_IDocFindFolder
        QITABENT(CShellFindExt, IContextMenu),          //IID_IContextMenu
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShellFindExt::Release()
{
    ASSERT(this->cRef >= 1);                    // Don't release more than we can!

    this->cRef--;
    if (this->cRef > 0)
    {
        return this->cRef;
    }
    
    ILFree(this->pidl);
    delete this;
    return 0;
}

STDMETHODIMP_(ULONG) CShellFindExt::AddRef()
{
    this->cRef++;
    return this->cRef;
}

CShellFindExt::CShellFindExt()
{
    this->cRef = 1;
}

STDMETHODIMP CShellFindExt::Initialize(LPCITEMIDLIST pidlFolder,
                                       IDataObject *pdtobj,
                                       HKEY hkeyProgID)
{
    return NOERROR;
}


STDMETHODIMP CShellFindExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, 
                                             UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int iCommands = 0;
    int idMax = idCmdFirst;
    HMENU hmMerge = SHLoadPopupMenu(HINST_THISDLL, POPUP_FINDEXT_POPUPMERGE);

    if (hmMerge) {
        MENUITEMINFO mii;

        mii.cbSize = SIZEOF(MENUITEMINFO);
        mii.fMask = MIIM_DATA;
        mii.dwItemData = Shell_GetCachedImageIndex(c_szShell32Dll, EIRESID(IDI_DOCFIND), 0);

        // setting the icon index
        TraceMsg(TF_DOCFIND, "CSFE::QueryContextMenu: %d is the icon index being set", mii.dwItemData);

        if (mii.dwItemData != -1)
            SetMenuItemInfo(hmMerge, FSIDM_FINDFILES, FALSE, &mii);

        if (!(GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS) ||
                            !SHRestricted(REST_HASFINDCOMPUTERS) ) {
            // Need to remove the Find Computer as we dont have a network
            DeleteMenu(hmMerge, FSIDM_FINDCOMPUTER, MF_BYCOMMAND);
        } else {

            mii.dwItemData = Shell_GetCachedImageIndex(c_szShell32Dll, EIRESID(IDI_COMPFIND),0);
            if (mii.dwItemData != -1)
                SetMenuItemInfo(hmMerge, FSIDM_FINDCOMPUTER, FALSE, &mii);

        }
        idMax = Shell_MergeMenus(hmenu, hmMerge, indexMenu,
                                 idCmdFirst, idCmdLast,
                                 0);
        DestroyMenu(hmMerge);
    }

    return ResultFromShort(idMax - idCmdFirst);
}

STDMETHODIMP CShellFindExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    LPITEMIDLIST pidl;
    UINT_PTR     id;

    TraceMsg(TF_DOCFIND, "CSFE::InvokeCommand %d", pici->lpVerb);

    id = (UINT_PTR)pici->lpVerb;
    if (pici->lpParameters)
    {
        CLSID guid;
        if (SUCCEEDED(GUIDFromStringA(pici->lpParameters, &guid)) && IsEqualGUID(guid, SRCID_SFindComputer))
            id = FSIDM_FINDCOMPUTER;
    }
    
    switch (id) {
    case FSIDM_FINDFILES:
        if (pici->lpDirectory)
        {
#ifdef UNICODE
            WCHAR szDirectory[MAX_PATH];
            LPCWSTR lpDirectory;

            if (pici->cbSize < CMICEXSIZE_NT4
                || (pici->fMask & CMIC_MASK_UNICODE) != CMIC_MASK_UNICODE)
            {
                MultiByteToWideChar(CP_ACP, 0,
                                    pici->lpDirectory, -1,
                                    szDirectory, ARRAYSIZE(szDirectory));
                lpDirectory = szDirectory;
            }
            else
            {
                lpDirectory = ((LPCMINVOKECOMMANDINFOEX)pici)->lpDirectoryW;
            }
            pidl = ILCreateFromPath(lpDirectory);
#else
            pidl = ILCreateFromPath(pici->lpDirectory);
#endif
        }
        else
        {
            pidl = NULL;
        }
        RealFindFiles(pidl, NULL);
        ILFree(pidl);
        break;

    case FSIDM_FINDCOMPUTER:
        SHFindComputer(NULL, NULL);
        break;
    }
    return NOERROR;
}

STDMETHODIMP CShellFindExt::GetCommandString(
                                        UINT_PTR    idCmd,
                                        UINT        wFlags,
                                        UINT *  pwReserved,
                                        LPSTR       pszName,
                                        UINT        cchMax)
{
    TraceMsg(TF_DOCFIND, "CSFE::GetCommandString idCmd = %d", idCmd);

    if (wFlags & GCS_HELPTEXTA)
    {
        UINT cch;

        if ((wFlags & GCS_HELPTEXTW) == GCS_HELPTEXTW)
            cch = LoadStringW(HINST_THISDLL,
                               (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                               (LPWSTR)pszName, cchMax);
        else
            cch = LoadStringA(HINST_THISDLL,
                               (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                               pszName, cchMax);
        if (cch)
            return NOERROR;
        else
            return E_OUTOFMEMORY;
    }
    else
        return E_NOTIMPL;
}

STDAPI CShellFindExt_CreateInstance(LPUNKNOWN, REFIID , void **);

HRESULT CALLBACK CShellFindExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    LPSHELLFINDEXT psfe;
    HRESULT hres;

    psfe = new CShellFindExt; 
    if (psfe)
    {
        hres = psfe->QueryInterface(riid, ppv);
        psfe->Release();
    }
    else
    {
        *ppv = NULL;
        hres = E_OUTOFMEMORY;
    }
    
    return hres;
}
