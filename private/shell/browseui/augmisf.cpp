//-------------------------------------------------------------------------//
//  
//  AugMisf.cpp  - Augmented Merge IShellFolder class implementation.
//
//-------------------------------------------------------------------------//
#include "priv.h"
#include "augmisf.h"
#include "resource.h"

#include "mluisupp.h"

#define TF_AUGM 0x10000000
//-------------------------------------------------------------------------//
//  BUGBUG: Shell allocator bullchit, inserted here because SHRealloc 
//  isn't imported into browseui, this module's hosting executable.
//  If we get SHRealloc, the following block can be removed:
#define _EXPL_SHELL_ALLOCATOR_

#ifdef  _EXPL_SHELL_ALLOCATOR_

#define SHRealloc( pv, cb )     shrealloc( pv, cb )

void* shrealloc( void* pv,  size_t cb )
{
    IMalloc* pMalloc ;
    void*    pvRet = NULL ;
    if( SUCCEEDED( SHGetMalloc( &pMalloc ) ) )  {
        pvRet = pMalloc->Realloc( pv, cb ) ;
        ATOMICRELEASE( pMalloc ) ;
    }
    return pvRet ;
}

#endif _EXPL_SHELL_ALLOCATOR_

BOOL     AffectAllUsers(HWND hwnd);

// id - verb mappings for IContextMenu impl
const struct
{
    UINT     id;
    LPCSTR   pszVerb;
} c_sIDVerbMap[] = 
{
    {SMIDM_DELETE,     "delete"},
    {SMIDM_RENAME,     "rename"},
    {SMIDM_PROPERTIES, "properties"},
    //{SMIDM_OPEN,       "open"},
    //{SMIDM_EXPLORE,    "explore"},
};

// augmisf context menu

class CAugMergeISFContextMenu : public IContextMenu2
{
public:
    // *** IUnknown methods ***
    STDMETHOD (QueryInterface)(REFIID, void**);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax);

    // *** IContextMenu2 methods ***    
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    CAugMergeISFContextMenu(IShellFolder *psfCommon, LPCITEMIDLIST pidlCommon, 
                            IShellFolder *psfUser,   LPCITEMIDLIST pidlUser, LPITEMIDLIST pidl,
                            HWND hwnd, UINT * prgfInOut);
    ~CAugMergeISFContextMenu();

    friend class CAugmentedMergeISF;
    friend CAugMergeISFContextMenu* CreateMergeISFContextMenu(
                            IShellFolder *psfCommon, LPCITEMIDLIST pidlCommon, 
                            IShellFolder *psfUser,   LPCITEMIDLIST pidlUser, LPITEMIDLIST pidl,
                            HWND hwnd, UINT * prgfInOut);
protected:
    LPITEMIDLIST    _pidlItem;
    IShellFolder *  _psfCommon;
    IShellFolder *  _psfUser;
    IContextMenu *  _pcmCommon;
    IContextMenu *  _pcmUser;
    LPITEMIDLIST    _pidlCommon;
    LPITEMIDLIST    _pidlUser;
    UINT            _idFirst;
    LONG            _cRef;
    HWND            _hwnd;
};

CAugMergeISFContextMenu* CreateMergeISFContextMenu(
                            IShellFolder *psfCommon, LPCITEMIDLIST pidlCommon, 
                            IShellFolder *psfUser,   LPCITEMIDLIST pidlUser, LPITEMIDLIST pidl,
                            HWND hwnd, UINT * prgfInOut)
{
    CAugMergeISFContextMenu* pcm = new CAugMergeISFContextMenu(psfCommon, pidlCommon,
                                                               psfUser, pidlUser,
                                                               pidl, hwnd, prgfInOut);
    if (pcm)
    {
        if (!pcm->_pidlItem)
        {
            delete pcm;
            pcm = NULL;
        }
    }
    return pcm;
}


CAugMergeISFContextMenu::CAugMergeISFContextMenu(IShellFolder *psfCommon, LPCITEMIDLIST pidlCommon,
                                                 IShellFolder *psfUser, LPCITEMIDLIST pidlUser,
                                                 LPITEMIDLIST pidl, HWND hwnd, UINT * prgfInOut)
{
    _cRef = 1;
    HRESULT hres;

    _hwnd = hwnd;
    _psfCommon = psfCommon;
    if (_psfCommon)
    {
        _psfCommon->AddRef();
        hres = _psfCommon->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pidl, IID_IContextMenu, prgfInOut, (void **)&_pcmCommon);

        ASSERT(SUCCEEDED(hres) || !_pcmCommon);
        _pidlCommon = ILClone(pidlCommon);
    }
    _psfUser = psfUser;
    if (_psfUser)
    {
        _psfUser->AddRef();
        hres = _psfUser->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pidl, IID_IContextMenu, prgfInOut, (void **)&_pcmUser);

        ASSERT(SUCCEEDED(hres) || !_pcmUser);
        _pidlUser = ILClone(pidlUser);
    }
    _pidlItem = ILClone(pidl);
    ASSERT(_psfCommon || _psfUser);
}

CAugMergeISFContextMenu::~CAugMergeISFContextMenu()
{
    ATOMICRELEASE(_psfCommon);
    ATOMICRELEASE(_pcmCommon);
    ATOMICRELEASE(_psfUser);
    ATOMICRELEASE(_pcmUser);
    ILFree(_pidlCommon);
    ILFree(_pidlUser);
    ILFree(_pidlItem);
}

STDMETHODIMP CAugMergeISFContextMenu::QueryInterface(REFIID riid, LPVOID *ppvOut)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CAugMergeISFContextMenu, IContextMenu, IContextMenu2),
        QITABENT(CAugMergeISFContextMenu, IContextMenu2),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvOut);
}

STDMETHODIMP_(ULONG) CAugMergeISFContextMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAugMergeISFContextMenu::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

HRESULT CAugMergeISFContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hres = E_INVALIDARG;
    
    if (hmenu)
    {
        HMENU hmContext = LoadMenuPopup(MENU_SM_CONTEXTMENU);
        if (hmContext)
        {
            if (!_psfCommon || !_psfUser)
            {
                DeleteMenu(hmContext, SMIDM_OPENCOMMON, MF_BYCOMMAND);
                DeleteMenu(hmContext, SMIDM_EXPLORECOMMON, MF_BYCOMMAND);
            }

            _idFirst = idCmdFirst;
            Shell_MergeMenus(hmenu, hmContext, -1, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
            DestroyMenu(hmContext);

            // Make it look "Shell Like"
            SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    
    return hres;
}

HRESULT CAugMergeISFContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT    id = -1;
    HRESULT hres = E_FAIL;
    CMINVOKECOMMANDINFO ici = *pici;

    if (pici->cbSize < SIZEOF(CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    if (HIWORD(pici->lpVerb))
    {
        for (int i=0; i < ARRAYSIZE(c_sIDVerbMap); i++)
        {
            if (lstrcmpiA(pici->lpVerb, c_sIDVerbMap[i].pszVerb) == 0)
            {
                id = c_sIDVerbMap[i].id;
                break;
            }
        }
    }
    else
        id = (UINT) PtrToUlong( pici->lpVerb ); // Win64: should be ok since MAKEINTRESOURCE assumed

    switch (id)
    {
        case -1:
            hres = E_INVALIDARG;
            break;

        case SMIDM_OPEN:
        case SMIDM_EXPLORE:
        case SMIDM_OPENCOMMON:
        case SMIDM_EXPLORECOMMON:
            {
                IShellFolder * psf;
                LPITEMIDLIST   pidl;

                if (id == SMIDM_OPEN || id == SMIDM_EXPLORE)
                {
                    if (_psfUser)
                    {
                        psf  = _psfUser;
                        pidl = _pidlUser;
                    }
                    else
                    {
                        psf  = _psfCommon;
                        pidl = _pidlCommon;
                    }
                }
                else
                {
                    psf  = _psfCommon;
                    pidl = _pidlCommon;
                }
                    
                if (psf && pidl)
                {
                    SHELLEXECUTEINFO shei = {0};

                    shei.lpIDList = ILCombine(pidl, _pidlItem);
                    if (shei.lpIDList)
                    {
                        shei.cbSize     = sizeof(shei);
                        shei.fMask      = SEE_MASK_IDLIST;
                        shei.nShow      = SW_SHOWNORMAL;
                        if (id == SMIDM_EXPLORE || id == SMIDM_EXPLORECOMMON)
                            shei.lpVerb = TEXT("explore");
                        else
                            shei.lpVerb = TEXT("open");
                        hres = ShellExecuteEx(&shei) ? S_OK : E_FAIL;
                        ILFree((LPITEMIDLIST)shei.lpIDList);
                    }
                }
            }
            break;

        case SMIDM_PROPERTIES:
            {
                IContextMenu * pcm = _pcmUser ? _pcmUser : _pcmCommon;

                if (pcm)
                {
                    ici.lpVerb = "properties";
                    hres = pcm->InvokeCommand(&ici);
                }
            }
            break;
        case SMIDM_DELETE:
            ici.lpVerb = "delete";
            hres = S_OK;
            
            if (_pcmUser)
            {
                hres = _pcmUser->InvokeCommand(&ici);
            }
            else if (_pcmCommon)
            {
                ici.fMask |= CMIC_MASK_FLAG_NO_UI;
                if (AffectAllUsers(_hwnd))
                    hres = _pcmCommon->InvokeCommand(&ici);   
                else
                    hres = E_FAIL;
            }   
                
            break;
            
        case SMIDM_RENAME:
            ASSERT(0);
            hres = E_NOTIMPL; // sftbar picks this off
            break;
      }
    
    return hres;
}

HRESULT CAugMergeISFContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_NOTIMPL;

    // if hiword in not null then a string is passed to us. we don't handle that case (yet?)
    if (!HIWORD(idCmd) && (uType == GCS_VERBA || uType == GCS_VERBW))
    {
        hres = E_INVALIDARG;

        for (int i = 0; hres != S_OK && i < ARRAYSIZE(c_sIDVerbMap); i++)
        {
            if (c_sIDVerbMap[i].id == idCmd)
            {
                if (uType == GCS_VERBA)
                    lstrcpynA(pszName, c_sIDVerbMap[i].pszVerb, cchMax);
                else
                    SHAnsiToUnicode(c_sIDVerbMap[i].pszVerb, (LPWSTR)pszName, cchMax);
                    
                hres = S_OK;
            }
        }
    }
    return hres;
}

// we need IContextMenu2 although HandleMenuMsg is not impl because of the way sftbar
// works -- it caches only IContextMenu2 so if we don't have ICM2 sftbar will think
// that it does not have context menu so it will eat the messages intended for the hmenu
// that way, with context menu up, if user presses esc it will kill start menu sub menu
// not the context menu.
HRESULT CAugMergeISFContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}
//-------------------------------------------------------------------------//
//  Augmented Merge Shell Folder's pidl wrapper package consists of a versioned 
//  header followed by n 'source namespace' pidl wrappers.
//  Each individual pidl wrapper consists of a header containing a
//  collection lookup index followed by the source pidl itself.  The 
//  source pidl's mkid.cb member is used to seek the next pidl wrap in
//  the package.
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//--- Augmented Merge Shell Folder's pidl wrapper header
typedef struct tagAUGM_IDWRAP {
    USHORT      cb ;         // pidl wrap length 
    USHORT      Reserved ;   // reserved.
    ULONG       tag ;        // AugMergeISF pidl signature
    ULONG       version ;    // AugMergeISF pidl version
    ULONG       cSrcs ;      // Number of source namespace objects backing this composite pidl
} AUGM_IDWRAP;

typedef UNALIGNED AUGM_IDWRAP *PAUGM_IDWRAP;

//--- Source pidl header.  One or more of these records will be concatenated 
//    within the wrap following the wrap header.
typedef struct tagAUGM_IDSRC   {
    UINT        nID     ;     // source namespace index
    BYTE        pidl[0] ;     // source pidl
} AUGM_IDSRC;

typedef UNALIGNED AUGM_IDSRC *PAUGM_IDSRC;

//-------------------------------------------------------------------------//
//  Constants
//-------------------------------------------------------------------------//
#define AUGM_WRAPTAG            MAKELONG( MAKEWORD('A','u'), MAKEWORD('g','M') )
#define AUGM_WRAPVERSION_1_0    MAKELONG( 1, 0 )
#define AUGM_WRAPCURRENTVERSION AUGM_WRAPVERSION_1_0
#define INVALID_NAMESPACE_INDEX ((UINT)-1)
#define CB_IDLIST_TERMINATOR    sizeof(USHORT)


//-------------------------------------------------------------------------//
//  Augmented Merge shell folder pidl wrap utilities
//-------------------------------------------------------------------------//


//-------------------------------------------------------------------------//
//  Resolves the wrap header from the indicated pidl.  
#define AugMergeISF_GetWrap( p ) ((PAUGM_IDWRAP)(p))

//-------------------------------------------------------------------------//
//  Determines whether the indicated pidl is an Augmented Merge 
//  shell folder pidl wrapper.
HRESULT AugMergeISF_IsWrap(
    IN LPCITEMIDLIST pidlTest, 
    IN ULONG nVersion = AUGM_WRAPCURRENTVERSION )
{
    ASSERT(IS_VALID_PIDL( pidlTest ));

    if (pidlTest)
    {
        PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pidlTest ) ;

        return  (pWrap->cb >= sizeof(AUGM_IDWRAP) && 
                pWrap->tag == AUGM_WRAPTAG && 
                pWrap->version == nVersion) ?  
                    S_OK : E_UNEXPECTED ;         //BUGBUG: better error code for version mismatch?
    }
    else
    {
        return E_INVALIDARG;
    }
}


//-------------------------------------------------------------------------//
//  Retrieves the number of source namespace pidls in the wrap.
//  If the pidl was not wrapped, the return value is -1.
ULONG AugMergeISF_GetSourceCount( IN LPCITEMIDLIST pidl )  
{
    ASSERT(SUCCEEDED(AugMergeISF_IsWrap(pidl)));
    return AugMergeISF_GetWrap(pidl)->cSrcs;
}


//-------------------------------------------------------------------------//
//  Creates an IDLIST wrapper object based on the indicated source pidl.
HRESULT AugMergeISF_CreateWrap( 
    IN LPCITEMIDLIST pidlSrc, 
    IN UINT nSrcID, 
    OUT LPITEMIDLIST* ppidlWrap )
{
    ASSERT( ppidlWrap ) ;
    ASSERT( IS_VALID_PIDL( pidlSrc ) && INVALID_NAMESPACE_INDEX != nSrcID  ) ;

    *ppidlWrap = NULL ;

    //  Allocate a header and terminator
    LPBYTE pBuf = NULL ;
    WORD   cbAlloc = sizeof(AUGM_IDWRAP) + 
                     sizeof(AUGM_IDSRC) + pidlSrc->mkid.cb + 
                     // we need two terminators, one for pidlSrc and one for the wrap
                     // the one for pidlSrc is necessary for the ILClone to work
                     // because it gets confused with the nSrcID that follows the pidl
                     CB_IDLIST_TERMINATOR + 
                     CB_IDLIST_TERMINATOR ;

    if( NULL == (pBuf = (LPBYTE)IEILCreate( cbAlloc )) )
        return E_OUTOFMEMORY ;

    //  Initialize wrap header members
    PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pBuf ) ;
    pWrap->cb       = cbAlloc - CB_IDLIST_TERMINATOR ;
    pWrap->tag      = AUGM_WRAPTAG ;
    pWrap->version  = AUGM_WRAPCURRENTVERSION ;

    if( pidlSrc )
    {
        PAUGM_IDSRC pSrc = (PAUGM_IDSRC)(pBuf + sizeof(AUGM_IDWRAP)) ;
        pSrc->nID = nSrcID ;
        memcpy( pSrc->pidl, pidlSrc, pidlSrc->mkid.cb ) ;
        pWrap->cSrcs = 1 ;
    }

    *ppidlWrap = (LPITEMIDLIST)pWrap ;
    return S_OK ;
}

BOOL WrappedPidlContainsSrcID(LPCITEMIDLIST pidlWrap, UINT uSrcID)
{
    ASSERT(SUCCEEDED(AugMergeISF_IsWrap(pidlWrap)));
    PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pidlWrap ) ;

    if( pWrap->cSrcs > 0 )
    {
        LPBYTE       p     = ((LPBYTE)pWrap) + sizeof(AUGM_IDWRAP) ; // position of first pidl header.
        PAUGM_IDSRC  pSrc  = (PAUGM_IDSRC)p ;                       
        // offset to next pidl header, needs terminator so that ILClone below can work
        UINT         cbPidl= ((LPITEMIDLIST)pSrc->pidl)->mkid.cb + CB_IDLIST_TERMINATOR;

        if (pSrc->nID != uSrcID && pWrap->cSrcs > 1)
        {
            pSrc = (PAUGM_IDSRC)(p + sizeof(AUGM_IDSRC) + cbPidl) ;
        }

        if (pSrc->nID == uSrcID)
            return TRUE;
    }

    return FALSE;
}

HRESULT AugMergeISF_WrapRemovePidl(
    IN LPITEMIDLIST pidlWrap, 
    IN UINT nSrcID, 
    OUT LPITEMIDLIST* ppidlRet )
{
    ASSERT( IS_VALID_WRITE_PTR( ppidlRet, LPITEMIDLIST )) ;
    
    *ppidlRet = NULL ;

    HRESULT hr = AugMergeISF_IsWrap(pidlWrap);
    if (SUCCEEDED(hr))
    {
        PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pidlWrap ) ;

        ASSERT(pWrap->cSrcs > 1);

        LPBYTE       p     = ((LPBYTE)pWrap) + sizeof(AUGM_IDWRAP) ; // position of first pidl header.
        PAUGM_IDSRC  pSrc  = (PAUGM_IDSRC)p ;                       
        // offset to next pidl header, needs terminator so that ILClone below can work
        UINT         cbPidl= ((LPITEMIDLIST)pSrc->pidl)->mkid.cb + CB_IDLIST_TERMINATOR;

        // We want to look for the Other SrcID. So we loop while the source id we're removing is
        // equal. When it's not equal, we've got the ID.
        if (pSrc->nID == nSrcID)
        {
            pSrc = (PAUGM_IDSRC)(p + sizeof(AUGM_IDSRC) + cbPidl) ;
        }

        if (pSrc->nID != nSrcID)
        {
            hr = AugMergeISF_CreateWrap((LPITEMIDLIST)pSrc->pidl, pSrc->nID, ppidlRet);
            ILFree(pidlWrap);
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
//  Adds a source pidl to the indicated pidl wrap.
HRESULT AugMergeISF_WrapAddPidl( 
    IN LPCITEMIDLIST pidlSrc, 
    IN UINT nSrcID, 
    IN OUT LPITEMIDLIST* ppidlWrap )
{
    ASSERT (ppidlWrap && IS_VALID_PIDL( *ppidlWrap ));
    ASSERT (IS_VALID_PIDL( pidlSrc ));
    ASSERT (INVALID_NAMESPACE_INDEX != nSrcID );

    HRESULT hr ;
    if (FAILED((hr = AugMergeISF_IsWrap(*ppidlWrap))))
        return hr ;

    // AHHHHHHHHHHH Rewrite this.
    if (WrappedPidlContainsSrcID(*ppidlWrap, nSrcID))
    {
        if (AugMergeISF_GetSourceCount(*ppidlWrap) > 1)
        {
            hr = AugMergeISF_WrapRemovePidl((LPITEMIDLIST)*ppidlWrap, nSrcID, ppidlWrap);
        }
        else
        {
            ILFree(*ppidlWrap);
            return AugMergeISF_CreateWrap(pidlSrc, nSrcID, ppidlWrap);
        }

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //  Retrieve wrap header
    PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)*ppidlWrap ;
    
    //  Reallocate a block large enough to contain our new record.
    WORD offTerm0 = pWrap->cb,      // offset to end of existing wrap
         offTerm1 = offTerm0 + sizeof(AUGM_IDSRC) + pidlSrc->mkid.cb,  // offset to end of next record
         cbRealloc= offTerm1 + 2*CB_IDLIST_TERMINATOR ;   // total bytes to reallocate

    LPBYTE pRealloc ;
    if( NULL == (pRealloc = (LPBYTE)SHRealloc( pWrap, cbRealloc )) )
        return E_OUTOFMEMORY ;

    //  Adjust our pointers if memory moved
    pWrap = (PAUGM_IDWRAP)pRealloc ;

    //  Initialize new record in the wrap
    UNALIGNED AUGM_IDSRC* pSrc = (PAUGM_IDSRC)(pRealloc + offTerm0 ) ;
    pSrc->nID = nSrcID ;
    memcpy( pSrc->pidl, pidlSrc, pidlSrc->mkid.cb ) ;

    //  Terminate new record 
    ZeroMemory( pRealloc + offTerm1, 2*CB_IDLIST_TERMINATOR ) ; 

    //  Update our header
    pWrap->cb = cbRealloc - CB_IDLIST_TERMINATOR ;
    pWrap->cSrcs++ ;

    *ppidlWrap = (LPITEMIDLIST)pWrap ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
//  Private pidl enumeration block (GetFirst/GetNext)
typedef struct tagAUGM_IDWRAP_ENUM
{
    ULONG           cbStruct ;    // structure size
    PAUGM_IDWRAP    pWrap ;       // wrap header.
    PAUGM_IDSRC     pSrcNext ;    // pointer to next src header
} AUGM_IDWRAP_ENUM, *PAUGM_IDWRAP_ENUM ;

//-------------------------------------------------------------------------//
//  Begins enumeration of source pidls in the indicated pidl wrap.
HANDLE AugMergeISF_EnumFirstSrcPidl( 
    IN LPCITEMIDLIST pidlWrap, 
    OUT UINT* pnSrcID, 
    OUT LPITEMIDLIST* ppidlRet )
{
    ASSERT( IS_VALID_WRITE_PTR( ppidlRet, LPITEMIDLIST ) && IS_VALID_WRITE_PTR( pnSrcID, UINT ) ) ;
    
    PAUGM_IDWRAP_ENUM pEnum = NULL ;
    *ppidlRet = NULL ;
    *pnSrcID  = (UINT)-1 ;

    HRESULT hr = AugMergeISF_IsWrap(pidlWrap);
    if(SUCCEEDED(hr))
    {
        PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pidlWrap ) ;

        if( pWrap->cSrcs > 0 )
        {
            LPBYTE       p     = ((LPBYTE)pWrap) + sizeof(AUGM_IDWRAP) ; // position of first pidl header.
            PAUGM_IDSRC  pSrc  = (PAUGM_IDSRC)p ;                       
            // offset to next pidl header, needs terminator so that ILClone below can work
            UINT         cbPidl= ((LPITEMIDLIST)pSrc->pidl)->mkid.cb + CB_IDLIST_TERMINATOR;
            
            if( NULL != (pEnum = new AUGM_IDWRAP_ENUM) )
            {
                pEnum->cbStruct = sizeof(*pEnum) ;
                pEnum->pWrap    = pWrap ;
                pEnum->pSrcNext = (PAUGM_IDSRC)(p + sizeof(AUGM_IDSRC) + cbPidl) ;
                *pnSrcID = pSrc->nID ;
                *ppidlRet = ILClone( (LPITEMIDLIST)pSrc->pidl ) ;
                if ( NULL == *ppidlRet )
                {
                    delete pEnum;
                    pEnum = NULL;
                }
            }
        }
    }
    return pEnum ;
}

//-------------------------------------------------------------------------//
//  Continues source pidl enumeration
BOOL AugMergeISF_EnumNextSrcPidl( 
    IN HANDLE hEnum, 
    OUT UINT* pnSrcID, 
    OUT LPITEMIDLIST* ppidlRet )
{
    PAUGM_IDWRAP_ENUM pEnum = (PAUGM_IDWRAP_ENUM)hEnum ;
    HRESULT           hr = E_UNEXPECTED ;

    ASSERT( IS_VALID_WRITE_PTR( pEnum, AUGM_IDWRAP_ENUM ) ) ;
    ASSERT( sizeof(*pEnum) == pEnum->cbStruct ) ;
    ASSERT( sizeof(*pEnum) == pEnum->cbStruct );

    *ppidlRet = NULL ;
    *pnSrcID  = (UINT)-1 ;

    if (SUCCEEDED((hr = AugMergeISF_IsWrap((LPCITEMIDLIST)pEnum->pWrap)))) 
    {
        if ((LPBYTE)(pEnum->pWrap) + pEnum->pWrap->cb <= (LPBYTE)pEnum->pSrcNext)
            hr = S_FALSE ;
        else
        {
            UNALIGNED AUGM_IDSRC* pualSrcNext = pEnum->pSrcNext;

            *pnSrcID = pualSrcNext->nID;
            *ppidlRet = ILClone((LPITEMIDLIST)pualSrcNext->pidl);
            
            pEnum->pSrcNext = (PAUGM_IDSRC)(
                                    ((LPBYTE)pualSrcNext) + 
                                    sizeof(AUGM_IDSRC) +
                                    ((LPITEMIDLIST)pualSrcNext->pidl)->mkid.cb + 
                                    CB_IDLIST_TERMINATOR);

            hr = S_OK ;
            return TRUE ;
        }
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Terminates source pidl enumeration
void AugMergeISF_EndEnumSrcPidls( 
    IN OUT HANDLE& hEnum )
{
    PAUGM_IDWRAP_ENUM pEnum = (PAUGM_IDWRAP_ENUM)hEnum ;

    ASSERT( IS_VALID_WRITE_PTR( pEnum, AUGM_IDWRAP_ENUM ) && 
        sizeof(*pEnum) == pEnum->cbStruct  );
    delete pEnum ;
    hEnum = NULL ;
}

//-------------------------------------------------------------------------//
//  Allocates and returns a copy of the specified source pidl 
//  from the wrapped pidl.
HRESULT AugMergeISF_GetSrcPidl( 
    IN LPCITEMIDLIST pidlWrap, 
    IN UINT nSrcID, 
    OUT LPITEMIDLIST* ppidlRet )
{
    ASSERT( ppidlRet ) ;
    *ppidlRet = NULL ;

    HANDLE       hEnum ;
    BOOL         bEnum ;
    UINT         nSrcIDEnum ;
    LPITEMIDLIST pidlEnum ;

    for( hEnum = AugMergeISF_EnumFirstSrcPidl( pidlWrap, &nSrcIDEnum, &pidlEnum ), bEnum = TRUE ;
         hEnum && bEnum ;
         bEnum = AugMergeISF_EnumNextSrcPidl( hEnum, &nSrcIDEnum, &pidlEnum ) )
    {
        if( nSrcIDEnum == nSrcID )
        {
            *ppidlRet = pidlEnum ;
            return S_OK ;
        }

        ILFree( pidlEnum ) ;
    }
    AugMergeISF_EndEnumSrcPidls( hEnum ) ;

    return E_FAIL ;
}

#ifdef DEBUG
BOOL  IsValidWrappedPidl(LPCITEMIDLIST pidlWrap)
{
    BOOL fValid = FALSE;

    if (pidlWrap == NULL)
        return FALSE;

    if (FAILED(AugMergeISF_IsWrap(pidlWrap)))
        return FALSE;


    HANDLE       hEnum ;
    UINT         nSrcIDEnum ;
    LPITEMIDLIST pidlEnum ;

    hEnum = AugMergeISF_EnumFirstSrcPidl( pidlWrap, &nSrcIDEnum, &pidlEnum );
    do 
    {
        fValid = IS_VALID_PIDL(pidlEnum);
        ILFree(pidlEnum);
    }
    while( fValid && AugMergeISF_EnumNextSrcPidl( hEnum, &nSrcIDEnum, &pidlEnum ));
    AugMergeISF_EndEnumSrcPidls( hEnum ) ;

    return fValid;
}
#endif

//-------------------------------------------------------------------------//

int AugmEnumCompare(void *pv1, void *pv2, LPARAM lParam)
{
    CAugISFEnumItem* paugmEnum1 = (CAugISFEnumItem*)pv1;
    CAugISFEnumItem* paugmEnum2 = (CAugISFEnumItem*)pv2;
    int iRet = -1;

    if (paugmEnum1 && paugmEnum2)
    {
        // Are these two items of different types?
        if (BOOLIFY(paugmEnum1->_rgfAttrib & SFGAO_FOLDER) ^ BOOLIFY(paugmEnum2->_rgfAttrib & SFGAO_FOLDER))
        {
            // Yes. Then Folders sort before items.
            iRet = BOOLIFY(paugmEnum1->_rgfAttrib & SFGAO_FOLDER) ? 1 : -1;
        }
        else    // They are of the same type. Then compare by name
        {
            iRet = lstrcmpi(paugmEnum1->_pszDisplayName, paugmEnum2->_pszDisplayName);
        }
    }
        

    return iRet;
}

LPVOID AugmEnumMerge(UINT uMsg, void * pv1, void * pv2, LPARAM lParam)
{    
    void * pvRet = pv1;
    
    switch (uMsg)
    {
        case DPAMM_MERGE:
            {
                HANDLE hEnum;
                UINT   nSrcID;
                LPITEMIDLIST pidl;
                CAugISFEnumItem* paugmeDest = (CAugISFEnumItem*)pv1;
                CAugISFEnumItem* paugmeSrc  = (CAugISFEnumItem*)pv2;

                ASSERT(paugmeDest && paugmeSrc);

                hEnum = AugMergeISF_EnumFirstSrcPidl(paugmeSrc->_pidlWrap, &nSrcID, &pidl);
                if (hEnum)
                {
                    // add pidl from src to dest
                    AugMergeISF_WrapAddPidl(pidl, nSrcID, &paugmeDest->_pidlWrap); 
                    // no longer need hEnum
                    AugMergeISF_EndEnumSrcPidls(hEnum);
                    // this was copied to paugmeDest->_pidlWrap
                    ILFree(pidl);
                }
            }
            break;
        case DPAMM_INSERT:
            {
                CAugISFEnumItem* paugmNew = new CAugISFEnumItem;
                CAugISFEnumItem* paugmSrc = (CAugISFEnumItem*)pv1;

                if (paugmNew)
                {
                    paugmNew->_pidlWrap = ILClone(paugmSrc->_pidlWrap);
                    if (paugmNew->_pidlWrap)
                    {
                        paugmNew->SetDisplayName(paugmSrc->_pszDisplayName);
                        paugmNew->_rgfAttrib = paugmSrc->_rgfAttrib;
                    }
                    else
                    {
                        delete paugmNew;
                        paugmNew = NULL;
                    }
                }
                pvRet = paugmNew;
            }
            break;
        default:
            ASSERT(0);
    }
    return pvRet;
}

typedef struct
{
    LPTSTR pszDisplayName;
    BOOL   fFolder;
} AUGMISFSEARCHFORPIDL;

int CALLBACK AugMISFSearchForOnePidlByDisplayName(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    AUGMISFSEARCHFORPIDL* pSearchFor = (AUGMISFSEARCHFORPIDL*)p1;
    CAugISFEnumItem* paugmEnum  = (CAugISFEnumItem*)p2;

    // Are they of different types?
    if (BOOLIFY(paugmEnum->_rgfAttrib & SFGAO_FOLDER) ^ pSearchFor->fFolder)
    {
        // Yes. 
        return pSearchFor->fFolder ? 1 : -1;
    }
    else    // They are of the same type. Then compare by name
    {
        return StrCmpI(pSearchFor->pszDisplayName, paugmEnum->_pszDisplayName);
    }
}

//-------------------------------------------------------------------------------------------------//
//  DPA utilities
#define DPA_GETPTRCOUNT( hdpa )         ((NULL != (hdpa)) ? DPA_GetPtrCount((hdpa)) : 0)
#define DPA_GETPTR( hdpa, i, type )     ((NULL != (hdpa)) ? (type*)DPA_GetPtr((hdpa), i) : (type*)NULL)
#define DPA_DESTROY( hdpa, pfn )        { if( NULL != hdpa ) \
                                            { DPA_DestroyCallback( hdpa, pfn, NULL ) ; \
                                              hdpa = NULL ; }}

//-------------------------------------------------------------------------------------------------//
//  Forwards...
class CEnum ;
class CChildObject ;


//-------------------------------------------------------------------------------------------------//
//  Augmented Merge Shell Folder source namespace descriptor.
//
//  Objects of class CNamespace are created by CAugmentedMergeISF in 
//  the AddNameSpace() method impl, and are maintained in the collection
//  CAugmentedMergeISF::_hdpaNamespaces.
//
class CNamespace
//-------------------------------------------------------------------------------------------------//
{
public:
    CNamespace( const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib ) ; 
    ~CNamespace() ;

    IShellFolder*   ShellFolder()   { return _psf ; }
    REFGUID         Guid()          { return _guid ; }
    ULONG           Attrib() const  { return _dwAttrib ; }
    LPITEMIDLIST    GetPidl() const { return _pidl; }

    void            Assign( const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib ) ;
    void            Unassign() ;

    HRESULT         RegisterNotify( HWND, UINT, ULONG ) ;
    HRESULT         UnregisterNotify() ;

    BOOL            SetOwner( IUnknown *punk ) ;
    
protected:
    IShellFolder*   _psf ;      // IShellFolder interface pointer
    GUID            _guid ;     // optional GUID for specialized UI handling
    LPITEMIDLIST    _pidl ;     // optional pidl
    ULONG           _dwAttrib ;  // optional flags
    UINT            _uChangeReg ; // Shell change notify registration ID.
} ;

//-------------------------------------------------------------------------------------------------//
inline CNamespace::CNamespace( const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib ) 
    :  _psf(NULL), 
       _pidl(NULL), 
       _guid(GUID_NULL), 
       _dwAttrib(0), 
       _uChangeReg(0)
{
    Assign( pguidUIObject, psf, pidl, dwAttrib ) ;
}

//-------------------------------------------------------------------------------------------------//
inline CNamespace::~CNamespace()  { 
    UnregisterNotify() ;
    Unassign() ;
}

//-------------------------------------------------------------------------------------------------//
//  Assigns data members.
void CNamespace::Assign( const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib )
{
    Unassign() ;
    if( NULL != (_psf = psf) )
        _psf->AddRef() ;

    _pidl       = ILClone( pidl ) ;
    _guid       = pguidUIObject ? *pguidUIObject : GUID_NULL ;
    _dwAttrib   = dwAttrib ;

}

//-------------------------------------------------------------------------------------------------//
//  Unassigns data members.
void CNamespace::Unassign()
{
    ATOMICRELEASE( _psf ) ; 
    ILFree( _pidl ) ;
    _pidl = NULL ;
    _guid = GUID_NULL ;
    _dwAttrib = 0L ;
}

//-------------------------------------------------------------------------------------------------//
//  Register change notification for the namespace
HRESULT CNamespace::RegisterNotify( HWND hwnd, UINT uMsg, ULONG lEvents )
{
    if( 0 == _uChangeReg )
        _uChangeReg = ::RegisterNotify(hwnd,
                                       uMsg,
                                       _pidl,
                                       lEvents,
                                       SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_RecursiveInterrupt,
                                       TRUE);

    return 0 != _uChangeReg ? S_OK : E_FAIL ;
}

//-------------------------------------------------------------------------------------------------//
//  Unregister change notification for the namespace
HRESULT CNamespace::UnregisterNotify()
{
    if( 0 != _uChangeReg )
    {
        UINT uID = _uChangeReg;

        _uChangeReg = 0;
        ::SHChangeNotifyDeregister(uID);
    }
    return S_OK;
}

//-------------------------------------------------------------------------------------------------//
inline BOOL CNamespace::SetOwner(IUnknown *punkOwner)
{
    if (_psf)
    {
        IUnknown_SetOwner(_psf, punkOwner);
        return TRUE ;
    }
    
    return FALSE ;
}

//-------------------------------------------------------------------------//
CAugmentedMergeISF::CAugmentedMergeISF() : _cRef(1)
{
    ASSERT(_hdpaNamespaces == NULL);
    ASSERT(_punkOwner == NULL);
    ASSERT(_pdt == NULL);
    DllAddRef() ;
}

//-------------------------------------------------------------------------//
CAugmentedMergeISF::~CAugmentedMergeISF()
{
    SetOwner(NULL);
    FreeNamespaces();
    DllRelease();
}

//-------------------------------------------------------------------------//
//  CAugmentedMergeISF global CreateInstance method for da class factory
//-------------------------------------------------------------------------//
STDAPI CAugmentedMergeISF_CreateInstance( IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi )
{
    // aggregation checking is handled in class factory
    CAugmentedMergeISF* pObj;

    if( NULL == (pObj = new CAugmentedMergeISF) )
        return E_OUTOFMEMORY ;

    *ppunk = SAFECAST( pObj, IAugmentedShellFolder2 * ) ;
    return S_OK;
}

//-------------------------------------------------------------------------//
//   CAugmentedMergeISF - IUnknown methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI( CAugmentedMergeISF, IShellFolder, IAugmentedShellFolder ),
        QITABENT( CAugmentedMergeISF, IAugmentedShellFolder ),
        QITABENT( CAugmentedMergeISF, IAugmentedShellFolder2 ),
        QITABENT( CAugmentedMergeISF, IShellFolder2 ),
        QITABENT( CAugmentedMergeISF, IShellService ),
        QITABENT( CAugmentedMergeISF, ITranslateShellChangeNotify ),
        QITABENT( CAugmentedMergeISF, IDropTarget ),
        { 0 },
    };
    return QISearch( this, qit, riid, ppvObj ) ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CAugmentedMergeISF::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CAugmentedMergeISF::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

//-------------------------------------------------------------------------//
//   CAugmentedMergeISF - IShellFolder methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    HRESULT hr = E_FAIL;

    if (_hdpaNamespaces)
    {
        // BUGBUG (lamadio): This does not work if you have 2 enumerators. But,
        // when asking for a new enumerator, we should flush the cache.
        FreeObjects();

        *ppenumIDList = new CEnum(this, grfFlags);

        if (NULL == *ppenumIDList)
            return E_OUTOFMEMORY ;
        hr = S_OK ;
    }
    
    return hr;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::BindToObject( LPCITEMIDLIST pidlWrap, LPBC pbc, REFIID riid, LPVOID *ppvOut )
{
    ASSERT(IS_VALID_PIDL( pidlWrap ) && NULL != ppvOut);

    *ppvOut = NULL ;
        
    if (SUCCEEDED(AugMergeISF_IsWrap(pidlWrap)))
    {
        PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap( pidlWrap ) ;
        ASSERT(IsValidWrappedPidl(pidlWrap));
        ASSERT( pWrap ) ;
        ASSERT( pWrap->cSrcs > 0 ) ;    // should never, never happen

        HANDLE           hEnum ;
        BOOL             bEnum ;
        UINT             nIDSrc = -1 ;
        DEBUG_CODE(int   iNumBound = 0);
        LPITEMIDLIST     pidlSrc ;
        HRESULT          hr = E_UNEXPECTED ;
        CNamespace* pSrc = NULL ;
        
        CAugmentedMergeISF* pISF ;
        if (NULL == (pISF = new CAugmentedMergeISF))
            return E_OUTOFMEMORY ;
    
        for (hEnum = AugMergeISF_EnumFirstSrcPidl( pidlWrap, &nIDSrc, &pidlSrc ), bEnum = TRUE ;
             hEnum && bEnum ;
             bEnum = AugMergeISF_EnumNextSrcPidl( hEnum, &nIDSrc, &pidlSrc))
        {
            if (SUCCEEDED((hr = QueryNameSpace(nIDSrc, (PVOID*)&pSrc))) && pSrc)
            {
                IShellFolder *psf;

                hr = S_FALSE;
                if (SUCCEEDED(pSrc->ShellFolder()->BindToObject(pidlSrc, NULL, IID_IShellFolder, (void **)&psf)))
                {
                    LPCITEMIDLIST pidlParent = pSrc->GetPidl();
                    LPITEMIDLIST  pidlFull   = ILCombine(pidlParent, pidlSrc);
                                 
                    hr = pISF->AddNameSpace(NULL, psf, pidlFull, pSrc->Attrib());
#ifdef DEBUG
                    if (SUCCEEDED(hr))
                        iNumBound++;
#endif
                    ILFree(pidlFull);
                    psf->Release();
                }
                ASSERT(SUCCEEDED(hr));
            }
            ILFree(pidlSrc);
        }

             // If this hits, then something is terribly wrong. Either we were unable to bind to the
             // ShellFolders, or the add failed. This could be caused by a bad wrapped pidl.
        ASSERT(iNumBound > 0);

        AugMergeISF_EndEnumSrcPidls( hEnum ) ;
        hr = pISF->QueryInterface(riid, ppvOut);
        pISF->Release();
        return hr ;
    }
    return E_UNEXPECTED ;    
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::BindToStorage( LPCITEMIDLIST, LPBC, REFIID, void ** )
{
    return E_NOTIMPL ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::CompareIDs( 
    LPARAM lParam, 
    LPCITEMIDLIST pidl1, 
    LPCITEMIDLIST pidl2)
{
    IShellFolder    *psf1 = NULL, *psf2 = NULL;
    LPITEMIDLIST    pidlItem1 = NULL, pidlItem2 = NULL;
    int             iRet = 0 ;
    HRESULT         hr1, hr2, hr ;

    hr1 = GetDefNamespace( pidl1, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf1, &pidlItem1 ) ;
    hr2 = GetDefNamespace( pidl2, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf2, &pidlItem2 ) ;

    if( SUCCEEDED( hr1 ) && SUCCEEDED( hr2 ) )
    {
        ULONG dwAttrib1 = SFGAO_FOLDER, dwAttrib2 = SFGAO_FOLDER;
        //  Same namespace? Just forward the request.
        if( psf1 == psf2 )
        {
            hr = psf1->CompareIDs( lParam, pidlItem1, pidlItem2 ) ;
            ILFree( pidlItem1 ) ;
            ILFree( pidlItem2 ) ;
            return hr ;
        }

        hr1 = psf1->GetAttributesOf( 1, (LPCITEMIDLIST*)&pidlItem1, &dwAttrib1 ) ;
        hr2 = psf2->GetAttributesOf( 1, (LPCITEMIDLIST*)&pidlItem2, &dwAttrib2 ) ;

        if( SUCCEEDED( hr1 ) && SUCCEEDED( hr2 ) )
        {
            //  Comparison heuristics:
            //  (1) folders take precedence over nonfolders, (2) alphanum comparison
            if( 0 != (dwAttrib1 & SFGAO_FOLDER) && 
                0 == (dwAttrib2 & SFGAO_FOLDER) )
                iRet = -1 ;
            else if( 0 == (dwAttrib1 & SFGAO_FOLDER) && 
                     0 != (dwAttrib2 & SFGAO_FOLDER) )
                iRet = 1 ;
            else
            {
                STRRET  strName1, strName2;
                HRESULT hres1 = E_FAIL;
                HRESULT hres2 = E_FAIL;
                TCHAR   szName1[MAX_PATH],  szName2[MAX_PATH];
                hr1 = psf1->GetDisplayNameOf(pidlItem1, SHGDN_FORPARSING | SHGDN_INFOLDER, &strName1); 
                hr2 = psf2->GetDisplayNameOf(pidlItem2, SHGDN_FORPARSING | SHGDN_INFOLDER, &strName2);

                if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
                {
                    // must call StrRetToBuf because it frees StrRet strings if allocated
                    hres1 = StrRetToBuf(&strName1, pidlItem1, szName1, ARRAYSIZE(szName1));
                    hres2 = StrRetToBuf(&strName2, pidlItem2, szName2, ARRAYSIZE(szName2));
                }
                // if the names match we return -1 because they are different pidls with
                // the same name

                if (SUCCEEDED(hr1) && SUCCEEDED(hr2) && SUCCEEDED(hres1) && SUCCEEDED(hres2))
                {
                    iRet = lstrcmp(szName1, szName2); // Comparisons are by name with items of the same type.
                }
            }
        }
    }

    hr = FAILED( hr1 ) ? hr1 : 
         FAILED( hr2 ) ? hr2 : 
         S_OK ;
   
    if( pidlItem1 )
        ILFree( pidlItem1 ) ;
    if( pidlItem2 )
        ILFree( pidlItem2 ) ;

    return MAKE_HRESULT( HRESULT_SEVERITY( hr ), HRESULT_FACILITY( hr ), iRet ) ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::CreateViewObject( 
    HWND hwndOwner, 
    REFIID riid, 
    LPVOID * ppvOut )
{
    HRESULT          hr ;
    CNamespace  *pSrc, *pSrc0 ;
    
    pSrc = pSrc0 = NULL ;

    // TODO: Handle IDropTarget here, delegate for all others.
    if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = QueryInterface(riid, ppvOut);
        if (SUCCEEDED(hr))
            _hwnd = hwndOwner;
        return hr;
    }

    //  Search for default namespace for CreateViewObj()
    if( FAILED( (hr = GetDefNamespace( ASFF_DEFNAMESPACE_VIEWOBJ, (PVOID*)&pSrc, NULL, (PVOID*)&pSrc0 )) ) )
        return hr ;

    if( NULL == pSrc ) 
        pSrc = pSrc0 ;

    if( NULL != pSrc )
    {
        ASSERT( pSrc->ShellFolder() ) ;
        hr = pSrc->ShellFolder()->CreateViewObject( hwndOwner, riid, ppvOut ) ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetAttributesOf( 
    UINT cidl, 
    LPCITEMIDLIST * apidl, 
    ULONG * rgfInOut )
{
    IShellFolder* pISF ;
    LPITEMIDLIST  pidlItem ;
    HRESULT       hr ;

    if( cidl > 1 )  // support 1 only.
        return E_NOTIMPL ;
        
    if( !apidl )
        return E_INVALIDARG ;
    
    //  Forward to default namespace for item attributes
    if( FAILED( (hr = GetDefNamespace(  
        apidl[0], ASFF_DEFNAMESPACE_ATTRIB, &pISF, &pidlItem )) ) )
        return hr ;

    hr = pISF->GetAttributesOf( 1, (LPCITEMIDLIST*)&pidlItem, rgfInOut ) ;
    
    ILFree( pidlItem ) ;
    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetUIObjectOf(
    HWND hwndOwner, 
    UINT cidl, 
    LPCITEMIDLIST * apidl, 
    REFIID riid, 
    UINT * prgfInOut, 
    LPVOID * ppvOut )
{
    IShellFolder* pISF ;
    LPITEMIDLIST  pidlItem ;
    HRESULT       hr ;

    if (cidl > 1)  // support 1 only.
        return E_NOTIMPL ;
        
    if (!apidl)
        return E_INVALIDARG ;

    if (IsEqualGUID(riid, IID_IContextMenu))
    {
        hr = _GetContextMenu(hwndOwner, cidl, apidl, prgfInOut, ppvOut);
    }
    else
    {
        //  Forward to default namespace for UI object
        if (FAILED((hr = GetDefNamespace(apidl[0], ASFF_DEFNAMESPACE_UIOBJ, &pISF, &pidlItem))))
            return hr ;

        hr = pISF->GetUIObjectOf(hwndOwner, 1, (LPCITEMIDLIST*)&pidlItem, riid, prgfInOut, ppvOut);
        ILFree(pidlItem);
    }
    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetDisplayNameOf( 
    LPCITEMIDLIST pidl, 
    DWORD grfFlags, 
    LPSTRRET pstrName )
{
    IShellFolder* pISF ;
    LPITEMIDLIST  pidlItem ;
    HRESULT       hr ;

    //  Forward to default namespace for display name
    if (FAILED((hr = GetDefNamespace( 
        pidl, ASFF_DEFNAMESPACE_DISPLAYNAME, &pISF, &pidlItem))))
        return hr ;

    if (SUCCEEDED((hr = pISF->GetDisplayNameOf(pidlItem, grfFlags, pstrName))))
    {
        //  STRRET_OFFSET has no meaning in context of the pidl wrapper.
        //  We can either calculate the offset into the wrapper, or allocate
        //  a wide char for the name.  For expedience, we'll allocate the name.
        
        if (pstrName->uType == STRRET_OFFSET)
        {
            UINT cch = lstrlenA( STRRET_OFFPTR( pidlItem, pstrName ) ) ;
            LPWSTR pwszName = (LPWSTR)SHAlloc( (cch + 1) * sizeof(WCHAR));

            if (NULL !=  pwszName)
            {
                SHAnsiToUnicode( STRRET_OFFPTR( pidlItem, pstrName ), pwszName, cch+1 );
                pwszName[cch] = (WCHAR)0 ;
            }
            pstrName->pOleStr = pwszName ;
            pstrName->uType   = STRRET_WSTR ;
        }

#ifdef DEBUG
        // If the trace flags are set, and this is not comming from an internal query,
        // Then append the location where this name came from
        if (g_dwTraceFlags & TF_AUGM && _fInternalGDNO == FALSE)
        {
            if (pstrName->uType == STRRET_WSTR)
            {
                LPWSTR wszOldName = pstrName->pOleStr;
                UINT cch = lstrlenW(wszOldName);

                pstrName->pOleStr = (LPWSTR)SHAlloc( (cch + 50) * sizeof(WCHAR));

                if (pstrName->pOleStr)
                {
                    StrCpyW(pstrName->pOleStr, wszOldName);

                    if (AugMergeISF_GetSourceCount(pidl) > 1)
                        StrCatW(pstrName->pOleStr, L" (Merged)");
                    else if (WrappedPidlContainsSrcID(pidl, 0))
                        StrCatW(pstrName->pOleStr, L" (1)");
                    else
                        StrCatW(pstrName->pOleStr, L" (2)");

                    SHFree(wszOldName);
                }
                else
                {
                    pstrName->pOleStr = wszOldName;
                }
            }
            else if (pstrName->uType == STRRET_CSTR)
            {
                if (AugMergeISF_GetSourceCount(pidl) > 1)
                    StrCatA(pstrName->cStr, " (Merged)");
                else if (WrappedPidlContainsSrcID(pidl, 0))
                    StrCatA(pstrName->cStr, " (1)");
                else
                    StrCatA(pstrName->cStr, " (2)");
            }
        }

#endif
    }

    ILFree( pidlItem ) ;
    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::ParseDisplayName( 
    HWND hwndOwner, 
    LPBC pbcReserved, 
    LPOLESTR pwszName, 
    ULONG * pchEaten, 
    LPITEMIDLIST * ppidl, 
    ULONG * pdwAttrib )
{
    int iIndex;
    LPITEMIDLIST pidl;

    *ppidl = NULL;
    // This ParseDisplayName should iterate through all our delegates until one works.
    for (iIndex = NamespaceCount() - 1; iIndex >=0 ; iIndex--)
    {
        CNamespace* pSrc = Namespace(iIndex) ;
        if (pSrc)
        {
            if (SUCCEEDED(pSrc->ShellFolder()->ParseDisplayName(hwndOwner, pbcReserved, pwszName, pchEaten,
                                                  &pidl, pdwAttrib)))
            {
                ASSERT(pidl);   // Make sure a valid pidl comes out.
                if (*ppidl == NULL)
                    AugMergeISF_CreateWrap(pidl, iIndex, ppidl);
                else
                    AugMergeISF_WrapAddPidl(pidl, iIndex, ppidl);

                ILFree(pidl);
            }
        }
    }

    return *ppidl? S_OK : E_FAIL;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::SetNameOf( 
    HWND hwndOwner, 
    LPCITEMIDLIST pidl, 
    LPCOLESTR pwszName, 
    DWORD uFlags, 
    LPITEMIDLIST *ppidlOut )
{
    CNamespace*   pnsCommon;
    CNamespace*   pnsUser;
    LPITEMIDLIST  pidlItem;
    HRESULT       hres;
    UINT          uiUser;
    UINT          uiCommon;

    hres = _GetNamespaces(pidl, &pnsCommon, &uiCommon, &pnsUser, &uiUser, &pidlItem, NULL);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidlNew = NULL;
        UINT         uiNamespace;

        if (pnsUser)
        {
            hres = pnsUser->ShellFolder()->SetNameOf(hwndOwner, pidlItem, pwszName, uFlags, &pidlNew);
            uiNamespace = uiUser;
        }
        else if (pnsCommon)
        {
            hres = E_FAIL;

            if (AffectAllUsers(hwndOwner))
            {
                hres = pnsCommon->ShellFolder()->SetNameOf(hwndOwner, pidlItem, pwszName, uFlags, &pidlNew);
                uiNamespace = uiCommon;
            }
        }

        if (ppidlOut)
        {
            *ppidlOut = NULL;
            // wrap the pidl
            if (SUCCEEDED(hres))
                AugMergeISF_CreateWrap(pidlNew, uiNamespace, ppidlOut);
        }
        
        ILFree(pidlNew);
        ILFree(pidlItem);
    }
    return hres;
}

//-------------------------------------------------------------------------//
//  IAugmentedShellFolder methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Adds a source namespace to the Augmented Merge shell folder object.
STDMETHODIMP CAugmentedMergeISF::AddNameSpace( 
    const GUID * pguidObject, 
    IShellFolder * psf, 
    LPCITEMIDLIST pidl, 
    DWORD dwFlags )
{
    ASSERT (IS_VALID_CODE_PTR(psf, IShellFolder*));
    ASSERT (IS_VALID_PIDL(pidl));

    //  Check for duplicate via full display name
    
    for( int i=0, max = NamespaceCount() ; i < max; i++ )
    {
        CNamespace* pSrc = Namespace( i ) ;
        if (pSrc)
        {
            if (ILIsEqual(pSrc->GetPidl(), pidl))
            {
                //  Found!  Reassign attributes
                pSrc->Assign( pguidObject, psf, pidl, dwFlags ) ;
                return S_OK ;
            }
        }
    }

    //  No match; safe to append it to collection, creating DPA if necessary.
    if( NULL == _hdpaNamespaces && 
        NULL == (_hdpaNamespaces= DPA_Create( 2 )) )
        return E_OUTOFMEMORY ;

    CNamespace *pSrc = new CNamespace( pguidObject, psf, pidl, dwFlags );
    if( NULL == pSrc )
        return E_OUTOFMEMORY ;
    
    return DPA_AppendPtr( _hdpaNamespaces, pSrc ) >= 0 ?  S_OK : E_FAIL;
}

//-------------------------------------------------------------------------//
//  Retrieves the primary namespace iid for the wrapped pidl.
STDMETHODIMP CAugmentedMergeISF::GetNameSpaceID( 
    LPCITEMIDLIST pidl, 
    GUID * pguidOut )
{
    HRESULT hr ;
    if (FAILED((hr = AugMergeISF_IsWrap( pidl ))))
        return hr ;

    //  BUGBUG: need to enumerate wrapped source pidls
    return E_NOTIMPL ;
}

//-------------------------------------------------------------------------//
//  Retrieves a pointer to a source namespace descriptor associated with 
//  the specified lookup index.
STDMETHODIMP CAugmentedMergeISF::QueryNameSpace( ULONG nID, PVOID* ppSrc )
{
    if (!ppSrc)
        return E_INVALIDARG;
    *ppSrc = NULL;

    LONG cSrcs;

    if ((cSrcs = NamespaceCount()) <=0)
        return E_FAIL;

    if(nID >= (ULONG)cSrcs) 
        return E_INVALIDARG;

    if (NULL == (*ppSrc = Namespace(nID)))
        return E_UNEXPECTED;

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Retrieves data for the namespace identified by dwID.
STDMETHODIMP CAugmentedMergeISF::QueryNameSpace( 
    ULONG nID, 
    GUID * pguidOut, 
    IShellFolder ** ppsf )
{
    CNamespace* pSrc = NULL ;
    HRESULT          hr = QueryNameSpace( nID, (PVOID*)&pSrc ) ;

    if( pguidOut )  
        *pguidOut = NULL != pSrc ? pSrc->Guid() : GUID_NULL ;

    if( ppsf )
    {      
        if( (*ppsf = (NULL != pSrc) ? pSrc->ShellFolder() : NULL) != NULL )
            (*ppsf)->AddRef() ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::EnumNameSpace( 
    DWORD uNameSpace, 
    DWORD * pdwID )
{
    return E_NOTIMPL ;
}

//-------------------------------------------------------------------------//
//  IAugmentedShellFolder2 methods
//-------------------------------------------------------------------------//

//GetNameSpaceCount and GetIDListWrapCount are not used anywhere
#if 0
//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetNameSpaceCount( OUT LONG* pcNamespaces )
{
    if( !pcNamespaces )
        return E_INVALIDARG ;

    *pcNamespaces = (LONG)NamespaceCount() ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetIDListWrapCount(
    LPCITEMIDLIST pidlWrap, 
    OUT LONG * pcPidls)
{
    if( NULL == pidlWrap || NULL == pcPidls )
        return E_INVALIDARG ;

    *pcPidls = 0 ;

    HRESULT hr ;
    if (SUCCEEDED((hr = AugMergeISF_IsWrap(pidlWrap))))
    {
        PAUGM_IDWRAP pWrap = AugMergeISF_GetWrap(pidlWrap);
        *pcPidls = pWrap->cSrcs;
        hr = S_OK;
    }
    return hr;
}
#endif // #if 0
//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::UnWrapIDList(
    LPCITEMIDLIST pidlWrap, 
    LONG cPidls, 
    IShellFolder** apsf, 
    LPITEMIDLIST* apidlFolder, 
    LPITEMIDLIST* apidlItems, 
    LONG* pcFetched )
{
    HRESULT         hr ;
    HANDLE          hEnum ;
    BOOL            bEnum = TRUE ;
    UINT            nSrcID ;
    LPITEMIDLIST    pidlItem ;
    LONG            cFetched = 0;

    if (NULL == pidlWrap || cPidls <= 0)
        return E_INVALIDARG ;

    if (FAILED((hr = AugMergeISF_IsWrap(pidlWrap))))
        return hr ;

    //  Enumerate pidls in wrap
    for (hEnum = AugMergeISF_EnumFirstSrcPidl( pidlWrap, &nSrcID, &pidlItem);
         cFetched < cPidls && hEnum && bEnum ;
         bEnum = AugMergeISF_EnumNextSrcPidl( hEnum, &nSrcID, &pidlItem))
    {
        //  Retrieve namespace data
        CNamespace* pSrc ;
        if (SUCCEEDED((hr = QueryNameSpace(nSrcID, (PVOID*)&pSrc))))
        {
            if (apsf)
            {
                apsf[cFetched] = pSrc->ShellFolder() ;
                if (apsf[cFetched])
                    apsf[cFetched]->AddRef();
            }
            if (apidlFolder)
                apidlFolder[cFetched] = ILClone(pSrc->GetPidl());
            if (apidlItems)
            {
                apidlItems[cFetched] = pidlItem;
                pidlItem = NULL; // paranoia -- just making sure we, somehow, don't free this guy at the end of the for loop
            }
            cFetched++ ;
        }
        else
        {
            ILFree( pidlItem ) ;
        }
    }
    ILFree(pidlItem); // AugMergeISF_EnumNextSrcPidl is called (if there are 2 wrapped pidls and we ask for only one)
                      // right before we exit the for loop so we have to free pidl if allocated.
    if (hEnum)
        AugMergeISF_EndEnumSrcPidls( hEnum );

    if( pcFetched )
        *pcFetched = cFetched ;
    
    return cFetched == cPidls ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::SetOwner( IUnknown* punkOwner )
{
    HRESULT hr = S_OK ;
    
    int cSrcs = NamespaceCount() ;

    if( cSrcs > 0 )
        DPA_EnumCallback( _hdpaNamespaces, SetOwnerProc, NULL ) ;

    ATOMICRELEASE( _punkOwner ) ;

    if( punkOwner )
    {
        hr = punkOwner->QueryInterface(IID_IUnknown, (LPVOID *)&_punkOwner ) ;
        
        if( cSrcs )
            DPA_EnumCallback( _hdpaNamespaces, SetOwnerProc, (void *)_punkOwner);
    }

    return hr ;
}

//-------------------------------------------------------------------------//
int CAugmentedMergeISF::SetOwnerProc( LPVOID pv, LPVOID pvParam )
{
    CNamespace* pSrc = (CNamespace*) pv ;
    ASSERT( pSrc ) ;

    return pSrc->SetOwner( (IUnknown*)pvParam ) ;
}

//-------------------------------------------------------------------------//
//  ITranslateShellChangeNotify methods
//-------------------------------------------------------------------------//

LPITEMIDLIST ILCombineBase(LPCITEMIDLIST pidlContainingBase, LPCITEMIDLIST pidlRel)
{
    // This routine differs from ILCombine in that it takes the First pidl's base, and
    // cats on the last id of the second pidl. We need this so Wrapped pidls
    // end up with the same base, and we get a valid full pidl.
    LPITEMIDLIST pidlRet = NULL;
    LPITEMIDLIST pidlBase = ILClone(pidlContainingBase);
    if (pidlBase)
    {
        ILRemoveLastID(pidlBase);

        pidlRet = ILCombine(pidlBase, pidlRel);

        ILFree(pidlBase);
    }

    return pidlRet;
}

BOOL IsFolderEvent(LONG lEvent)
{
    return lEvent == SHCNE_MKDIR || lEvent == SHCNE_RMDIR || lEvent == SHCNE_RENAMEFOLDER;
}

#ifdef DEBUG
void CAugmentedMergeISF::DumpObjects()
{
    if (g_dwDumpFlags & TF_AUGM)
    {
        ASSERT(_hdpaObjects);
        int iObjectCount = DPA_GetPtrCount(_hdpaObjects);
        TraceMsg(TF_AUGM, "CAugMISF::DumpObjects: Number of items: %d", iObjectCount);

        CNamespace* pns = (CNamespace *)DPA_FastGetPtr(_hdpaNamespaces, 0);
        if (pns)
            DebugDumpPidl(TF_AUGM, TEXT("CAugMISF::DumpObjects Namespace 1"), pns->GetPidl());

        pns = (CNamespace *)DPA_FastGetPtr(_hdpaNamespaces, 1);
        if (pns)
            DebugDumpPidl(TF_AUGM, TEXT("CAugMISF::DumpObjects Namespace 2"), pns->GetPidl());

        for (int i = 0; i < iObjectCount; i++)
        {
            CAugISFEnumItem* pEnumItem = (CAugISFEnumItem*)DPA_FastGetPtr(_hdpaObjects, i);
            TraceMsg(TF_ALWAYS, "CAugMISF::DumpObjects: %s, Folder: %s Merged: %s",
                pEnumItem->_pszDisplayName, 
                BOOLIFY(pEnumItem->_rgfAttrib & SFGAO_FOLDER) ? TEXT("Yes") : TEXT("No"),
                (AugMergeISF_GetSourceCount(pEnumItem->_pidlWrap) > 1)? TEXT("Yes") : TEXT("No")); 
        }
    }
}
#endif

BOOL GetRealPidlFromSimple(LPCITEMIDLIST pidlSimple, LPITEMIDLIST* ppidlReal)
{
    // Similar to SHGetRealIDL in Function, but SHGetRealIDL does SHGDN_FORPARSING | INFOLDER.
    // I need the parsing name. I can't rev SHGetRealIDL very easily, so here's this one!
    TCHAR szFullName[MAX_PATH];
    if (SUCCEEDED(SHGetNameAndFlags(pidlSimple, SHGDN_FORPARSING, szFullName, SIZECHARS(szFullName), NULL)))
    {
        *ppidlReal = ILCreateFromPath(szFullName);
    }

    if (*ppidlReal == NULL) // Unable to create? Then use the simple pidl. This is because it does not exist any more
    {                       // For say, a Delete Notify
        *ppidlReal = ILClone(pidlSimple);
    }

    return *ppidlReal != NULL;
}



//-------------------------------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::TranslateIDs( 
    LONG *plEvent, 
    LPCITEMIDLIST pidl1, 
    LPCITEMIDLIST pidl2, 
    LPITEMIDLIST * ppidlOut1, 
    LPITEMIDLIST * ppidlOut2,
    LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
    LPITEMIDLIST *ppidlOut2Event2)
{
    HRESULT hres = E_FAIL;

    switch (*plEvent)
    {
    case SHCNE_EXTENDED_EVENT:
    case SHCNE_ASSOCCHANGED:
    case SHCNE_UPDATEIMAGE:
        return S_OK;

    case SHCNE_UPDATEDIR:
        FreeObjects();
        return S_OK;
    }

    ASSERT(ppidlOut1);
    ASSERT(ppidlOut2);
    LONG lEvent = *plEvent;

    *plEvent2 = (LONG)-1;
    *ppidlOut1Event2 = NULL;
    *ppidlOut2Event2 = NULL;

    
    *ppidlOut1 = (LPITEMIDLIST)pidl1;
    *ppidlOut2 = (LPITEMIDLIST)pidl2;

    if (!plEvent)
        return E_FAIL;

    // If they are already wrapped, don't wrap twice.
    if ((pidl1 && SUCCEEDED(AugMergeISF_IsWrap(ILFindLastID(pidl1)))) ||
        (pidl2 && SUCCEEDED(AugMergeISF_IsWrap(ILFindLastID(pidl2)))))
    {
        // We don't want to wrap twice.
        return E_FAIL;
    }

    if (!_hdpaNamespaces)
        return E_FAIL;

    if (!_hdpaObjects)
        return E_FAIL;

    CAugISFEnumItem* pEnumItem;

    int iIndex;
    int iShellFolder1 = -1;
    int iShellFolder2 = -1;
    IShellFolder* psf1 = NULL;
    IShellFolder* psf2 = NULL;
    LPITEMIDLIST pidlReal1 = NULL;
    LPITEMIDLIST pidlReal2 = NULL;
    LPITEMIDLIST pidlRealRel1 = NULL;
    LPITEMIDLIST pidlRealRel2 = NULL;
    BOOL fFolder = IsFolderEvent(*plEvent);

    // Get the information about these Simple pidls: Are they our Children? If so, what namespace?
    BOOL fChild1 = IsChildIDInternal(pidl1, TRUE, &iShellFolder1);
    BOOL fChild2 = IsChildIDInternal(pidl2, TRUE, &iShellFolder2);

    // Is either a child?
    if (!(fChild1 || fChild2))
        return hres;

    // Ok, pidl1 is a child, can we get the Real pidl from the simple one?
    if (pidl1 && !GetRealPidlFromSimple(pidl1, &pidlReal1))
        goto Cleanup;

    // Ok, pidl2 is a child, can we get the Real pidl from the simple one?
    if (pidl2 && !GetRealPidlFromSimple(pidl2, &pidlReal2))
        goto Cleanup;

    // These are for code clarity later on. We deal with Relative pidls from here until the very end,
    // when we combine the base of the in pidls with the outgoing wrapped pidls.
    if (pidlReal1)
        pidlRealRel1 = ILFindLastID(pidlReal1);

    if (pidlReal2)
        pidlRealRel2 = ILFindLastID(pidlReal2);

    // Is Pidl1 in our namespaces?
    if (iShellFolder1 != -1)
    {
        // Yes, lets get the non-refcounted shell folder that know's about this pidl.
        CNamespace * pns = (CNamespace *)DPA_GetPtr(_hdpaNamespaces, iShellFolder1);
        psf1 = pns->ShellFolder();  // Non ref counted.
    }

    // Is Pidl2 in our namespaces?
    if (iShellFolder2 != -1)
    {
        // Yes, lets get the non-refcounted shell folder that know's about this pidl.
        CNamespace * pns = (CNamespace *)DPA_GetPtr(_hdpaNamespaces, iShellFolder2);
        psf2 = pns->ShellFolder();  // Non ref counted.
    }

    hres = S_OK;

    DEBUG_CODE(_fInternalGDNO = TRUE);

    switch(*plEvent)
    {
    case 0: // Just look up the pidls and return.
        {
            DWORD rgfAttrib = SFGAO_FOLDER;
            if (iShellFolder1 != -1)
            {
                psf1->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlRealRel1, &rgfAttrib);
                if (S_OK == _SearchForPidl(psf1, pidlRealRel1, BOOLIFY(rgfAttrib & SFGAO_FOLDER), &iIndex, &pEnumItem))
                {
                    *ppidlOut1 = ILCombineBase(pidlReal1, pEnumItem->_pidlWrap);
                    if (!*ppidlOut1)
                        hres = E_OUTOFMEMORY;
                }
            }

            rgfAttrib = SFGAO_FOLDER;
            if (iShellFolder2 != -1 && SUCCEEDED(hres))
            {
                psf2->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlRealRel2, &rgfAttrib);
                if (S_OK == _SearchForPidl(psf2, pidlRealRel2, BOOLIFY(rgfAttrib & SFGAO_FOLDER), &iIndex, &pEnumItem))
                {
                    *ppidlOut2 = ILCombineBase(pidlReal2, pEnumItem->_pidlWrap);
                    if (!*ppidlOut2)
                        hres = E_OUTOFMEMORY;
                }
            }
        }

        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s", fFolder? 
                TEXT("SHCNE_MKDIR") : TEXT("SHCNE_CREATE")); 
            // Is there a thing of this name already?
            if (S_OK == _SearchForPidl(psf1, pidlRealRel1, fFolder, &iIndex, &pEnumItem))
            {
                TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s needs to be merged. Converting to Rename", pEnumItem->_pszDisplayName);
                // Yes; Then we need to merge this new pidl into the wrapped pidl, and change this
                // to a rename, passing the Old wrapped pidl as the first arg, and the new wrapped pidl
                // as the second arg. I have to be careful about the freeing:
                // Free *ppidlOut1
                // Clone pEnumItem->_pidlWrap -> *ppidlOut1.
                // Add pidl1 to pEnumItem->_pidlWrap.
                // Clone new pEnumItem->_pidlWrap -> *ppidlOut2.  ASSERT(*ppidlOut2 == NULL)

                *ppidlOut1 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);
                if (*ppidlOut1)
                {
                    AugMergeISF_WrapAddPidl(pidlRealRel1, iShellFolder1, &pEnumItem->_pidlWrap); 
                    *ppidlOut2 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);

                    if (!*ppidlOut2)
                        TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl2");

                    *plEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                }
                else
                {
                    TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl1");
                }

            }
            else
            {
                LPITEMIDLIST pidlWrap;
                CAugISFEnumItem* paugmEnum = new CAugISFEnumItem;
                if (paugmEnum)
                {
                    if (SUCCEEDED(AugMergeISF_CreateWrap(pidlRealRel1, (UINT)iShellFolder1, &pidlWrap)) &&
                        paugmEnum->InitWithWrappedToOwn(SAFECAST(this, IAugmentedShellFolder2*), 
                                                        iShellFolder1, pidlWrap))
                    {
                        AUGMISFSEARCHFORPIDL AugMSearch;
                        AugMSearch.pszDisplayName = paugmEnum->_pszDisplayName;
                        AugMSearch.fFolder = fFolder;

                        int iInsertIndex = DPA_Search(_hdpaObjects, (LPVOID)&AugMSearch, 0,
                                AugMISFSearchForOnePidlByDisplayName, NULL, DPAS_SORTED | DPAS_INSERTAFTER);

                        TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Creating new unmerged %s at %d", 
                            paugmEnum->_pszDisplayName, iInsertIndex);

                        if (iInsertIndex < 0)
                            iInsertIndex = DA_LAST;

                        if (DPA_InsertPtr(_hdpaObjects, iInsertIndex, paugmEnum) == -1)
                        {
                            TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Was unable to add %s for some reason. Bailing", 
                                paugmEnum->_pszDisplayName);
                            DestroyObjectsProc(paugmEnum, NULL);
                        }
                        else
                        {
                            *ppidlOut1 = ILCombineBase(pidl1, paugmEnum->_pidlWrap);
                        }
                    }
                    else
                        DestroyObjectsProc(paugmEnum, NULL);
                }
            }

        }
        break;

    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        {
            TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s", fFolder? 
                TEXT("SHCNE_RMDIR") : TEXT("SHCNE_DELETE")); 
            int iDeleteIndex;
            // Is there a folder of this name already?
            if (S_OK == _SearchForPidl(psf1, pidlRealRel1, 
                fFolder, &iDeleteIndex, &pEnumItem))
            {
                TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Found %s checking merge state.", pEnumItem->_pszDisplayName); 
                // Yes; Then we need to unmerge this pidl from the wrapped pidl, and change this
                // to a rename, passing the Old wrapped pidl as the first arg, and the new wrapped pidl
                // as the second arg. I have to be careful about the freeing:
                // Free *ppidlOut1
                // Clone pEnumItem->_pidlWrap -> *ppidlOut1.
                // Remove pidl1 from pEnumItem->_pidlWrap
                // Convert to rename, pass new wrapped as second arg. 

                if (AugMergeISF_GetSourceCount( pEnumItem->_pidlWrap )  > 1 )
                {
                    TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is Merged. Removing pidl, convert to rename", pEnumItem->_pszDisplayName); 
                    *ppidlOut1 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);
                    if (*ppidlOut1)
                    {
                        EVAL(SUCCEEDED(AugMergeISF_WrapRemovePidl(pEnumItem->_pidlWrap, 
                            iShellFolder1, &pEnumItem->_pidlWrap)));

                        *ppidlOut2 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);

                        if (!*ppidlOut2)
                            TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl2");

                        *plEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                    }
                    else
                    {
                        TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl1");
                    }
                }
                else
                {
                    TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is not Merged. deleteing", pEnumItem->_pszDisplayName); 
                    pEnumItem = (CAugISFEnumItem*)DPA_DeletePtr(_hdpaObjects, iDeleteIndex);

                    if (EVAL(pEnumItem))
                    {
                        *ppidlOut1 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);
                        DestroyObjectsProc(pEnumItem, NULL);
                    }
                    else
                    {
                        TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to get %d from DPA", iDeleteIndex);
                    }
                }

            }

        }
        break;

    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        {
            // BUGBUG (lamadio): When renaming an item in the menu, this code will split it into
            // a Delete and a Create. We need to detect this situation and convert it to 1 rename. This
            // will solve the problem of the lost order during a rename....
            BOOL fEvent1Set = FALSE;
            BOOL fFirstPidlInNamespace = FALSE;
            TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s", fFolder? 
                TEXT("SHCNE_RENAMEFOLDER") : TEXT("SHCNE_RENAMEITEM")); 

            // Is this item being renamed FROM the Folder?
            if (iShellFolder1 != -1 &&          // Is this pidl a child of the Folder?
                S_OK == _SearchForPidl(psf1, pidlRealRel1, 
                fFolder, &iIndex, &pEnumItem))  // Is it found?
            {
                TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Old pidl %s is in the Folder", pEnumItem->_pszDisplayName); 
                // Yes.
                // Then we need to see if the item that it's being renamed from was Merged

                // Need this for reentrancy
                if (WrappedPidlContainsSrcID(pEnumItem->_pidlWrap, iShellFolder1))
                {
                    // Was it merged?
                    if (AugMergeISF_GetSourceCount(pEnumItem->_pidlWrap) > 1)    // Case 3)
                    {
                        TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is Merged. Removing pidl. Convert to rename for event 1", 
                            pEnumItem->_pszDisplayName); 
                        // Yes;
                        // Then we need to unmerge that item.
                        *ppidlOut1 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);
                        if (*ppidlOut1)
                        {
                            // UnWrap
                            AugMergeISF_WrapRemovePidl(pEnumItem->_pidlWrap, iShellFolder1, &pEnumItem->_pidlWrap); 

                            *ppidlOut2 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);

                            if (!*ppidlOut2)
                                TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl2");

                            // This We need to "Rename" the old wrapped pidl, to this new one
                            // that does not contain the old item.
                            fEvent1Set = TRUE;
                        }
                        else
                        {
                            TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to create new pidl1");
                        }
                    }
                    else
                    {
                        TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is not merged. Nuking item Convert to Delete for event 1.", 
                            pEnumItem->_pszDisplayName); 
                        // No, This was not a wrapped pidl. Then, convert to a delete:
                        pEnumItem = (CAugISFEnumItem*)DPA_DeletePtr(_hdpaObjects, iIndex);

                        if (EVAL(pEnumItem))
                        {
                            // If we're renaming from this folder, into this folder, Then the first event stays a rename.
                            if (iShellFolder2 == -1)
                            {
                                fEvent1Set = TRUE;
                                *plEvent = fFolder? SHCNE_RMDIR : SHCNE_DELETE;
                            }
                            else
                            {
                                fFirstPidlInNamespace = TRUE;
                            }
                            *ppidlOut1 = ILCombineBase(pidl1, pEnumItem->_pidlWrap);
                            DestroyObjectsProc(pEnumItem, NULL);
                        }
                        else
                        {
                            TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Failure. Was unable to find Item at %d", iIndex);
                        }

                    }
                }
                else
                {
                    TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Skipping this because we already processed it."
                        "Dragging To Desktop?");
                    hres = E_FAIL;
                }

            }

            // Is this item is being rename INTO the Start Menu?
            if (iShellFolder2 != -1)
            {
                TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: New pidl is in the Folder"); 
                LPITEMIDLIST* ppidlNewWrapped1 = ppidlOut1;
                LPITEMIDLIST* ppidlNewWrapped2 = ppidlOut2;
                LONG* plNewEvent = plEvent;

                if (fEvent1Set)
                {
                    plNewEvent = plEvent2;
                    ppidlNewWrapped1 = ppidlOut1Event2;
                    ppidlNewWrapped2 = ppidlOut2Event2;
                }

                if (S_OK == _SearchForPidl(psf2, pidlRealRel2, 
                    fFolder, &iIndex, &pEnumItem))
                {

                    // If we're renaming from this folder, into this folder, Check to see if the destination has a
                    // conflict. If there is a confict (This case), then convert first event to a remove, 
                    // and the second event to the rename.
                    if (fFirstPidlInNamespace)
                    {
                        fEvent1Set = TRUE;
                        *plEvent = fFolder? SHCNE_RMDIR : SHCNE_DELETE;
                        plNewEvent = plEvent2;
                        ppidlNewWrapped1 = ppidlOut1Event2;
                        ppidlNewWrapped2 = ppidlOut2Event2;
                    }
                    
                    
                    TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is in Folder", pEnumItem->_pszDisplayName);
                    TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Adding pidl to %s. Convert to Rename for event %s", 
                        pEnumItem->_pszDisplayName, fEvent1Set? TEXT("2") : TEXT("1"));

                    // Then the destination needs to be merged.
                    *ppidlNewWrapped1 = ILCombineBase(pidl2, pEnumItem->_pidlWrap);
                    if (*ppidlNewWrapped1)
                    {
                        TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Successfully created out pidl1");
                        AugMergeISF_WrapAddPidl(pidlRealRel2, iShellFolder2, &pEnumItem->_pidlWrap); 

                        *ppidlNewWrapped2 = ILCombineBase(pidl2, pEnumItem->_pidlWrap);

                        *plNewEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                    }
                }
                else
                {
                    LPITEMIDLIST pidlWrap;
                    CAugISFEnumItem* paugmEnum = new CAugISFEnumItem;

                    if (paugmEnum)
                    {
                        if (SUCCEEDED(AugMergeISF_CreateWrap(pidlRealRel2, (UINT)iShellFolder2, &pidlWrap)) &&
                            paugmEnum->InitWithWrappedToOwn(SAFECAST(this, IAugmentedShellFolder2*), 
                                                            iShellFolder2, pidlWrap))
                        {
                            AUGMISFSEARCHFORPIDL AugMSearch;
                            AugMSearch.pszDisplayName = paugmEnum->_pszDisplayName;
                            AugMSearch.fFolder = BOOLIFY(paugmEnum->_rgfAttrib & SFGAO_FOLDER);

                            int iInsertIndex = DPA_Search(_hdpaObjects, &AugMSearch, 0,
                                    AugMISFSearchForOnePidlByDisplayName, NULL, DPAS_SORTED | DPAS_INSERTAFTER);

                            TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: %s is a new item. Converting to Create", 
                                paugmEnum->_pszDisplayName);

                            if (iInsertIndex < 0)
                                iInsertIndex = DA_LAST;

                            if (DPA_InsertPtr(_hdpaObjects, iInsertIndex, paugmEnum) == -1)
                            {
                                TraceMsg(TF_ERROR, "CAugMISF::TranslateIDs: Was unable to add %s for some reason. Bailing", 
                                    paugmEnum->_pszDisplayName);
                                DestroyObjectsProc(paugmEnum, NULL);
                            }
                            else
                            {
                                TraceMsg(TF_AUGM, "CAugMISF::TranslateIDs: Creating new item %s at %d for event %s", 
                                    paugmEnum->_pszDisplayName, iInsertIndex,  fEvent1Set? TEXT("2") : TEXT("1"));

                                // If we're renaming from this folder, into this folder, Then the first event stays
                                // a rename.
                                if (!fFirstPidlInNamespace)
                                {
                                    *plNewEvent = fFolder ? SHCNE_MKDIR : SHCNE_CREATE;
                                    *ppidlNewWrapped1 = ILCombineBase(pidl2, pidlWrap);
                                    *ppidlNewWrapped2 = NULL;
                                }
                                else
                                    *ppidlOut2 = ILCombineBase(pidl2, pidlWrap);

                            }
                        }
                        else
                            DestroyObjectsProc(paugmEnum, NULL);

                    }
                }
            }
        }
        break;

    default:
        break;
    }

Cleanup:
    ILFree(pidlReal1);
    ILFree(pidlReal2);

#ifdef DEBUG
    DumpObjects();
    _fInternalGDNO = FALSE;
#endif


    return hres;
}

BOOL CAugmentedMergeISF::IsChildIDInternal(LPCITEMIDLIST pidlKid, BOOL fImmediate, int* piShellFolder)
{
    // This is basically the same Method as the interface method, but returns the shell folder
    // that it came from.
    BOOL fChild = FALSE;

    //At this point we should have a translated pidl
    if (pidlKid)
    {
        if (SUCCEEDED(AugMergeISF_IsWrap(pidlKid)))
        {
            LPCITEMIDLIST pidlRelKid = ILFindLastID(pidlKid);
            if (pidlRelKid)
            {
                UINT   uiId;
                LPITEMIDLIST pidl;
                HANDLE hEnum = AugMergeISF_EnumFirstSrcPidl(pidlRelKid, &uiId, &pidl);

                if (hEnum)
                {
                    do
                    {
                        ILFree(pidl);

                        for (int i = 0; fChild == FALSE && i < DPA_GetPtrCount(_hdpaNamespaces); i++)
                        {
                            CNamespace * pns = (CNamespace *)DPA_GetPtr(_hdpaNamespaces, i);
                            // reuse pidl
                            if (pns && (pidl = pns->GetPidl()) != NULL)
                            {
                                if (ILIsParent(pidl, pidlKid, fImmediate) &&
                                    !ILIsEqual(pidl, pidlKid))
                                {
                                    fChild = TRUE;
                                    if (piShellFolder)
                                        *piShellFolder = i;
                                }
                            }
                        }
                    }
                    while (fChild == FALSE && AugMergeISF_EnumNextSrcPidl(hEnum, &uiId, &pidl));

                    AugMergeISF_EndEnumSrcPidls(hEnum);
                }
            }
        }
        else
        {
            int cSrcs = NamespaceCount();

            for(int i = 0; fChild == FALSE && i < cSrcs ; i++)
            {
                CNamespace* pSrc = Namespace(i);
                if (pSrc && ILIsParent(pSrc->GetPidl(), pidlKid, fImmediate) && 
                    !ILIsEqual(pSrc->GetPidl(), pidlKid))
                {
                    fChild = TRUE;
                    if (piShellFolder)
                        *piShellFolder = i;
                }
            }
        }
    }

    return fChild;
}

//-------------------------------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::IsChildID( LPCITEMIDLIST pidlKid, BOOL fImmediate)
{
    return IsChildIDInternal(pidlKid, fImmediate, NULL) ? S_OK : S_FALSE;
}

//-------------------------------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::IsEqualID( LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2 )
{
    // This used to return E_NOTIMPL. I'm kinda overloading the interface to mean:
    // is this equal tp any of your namespaces.
    HRESULT hres = S_FALSE;
    int cSrcs = NamespaceCount();

    for(int i = 0; hres == S_FALSE && i < cSrcs ; i++)
    {
        CNamespace* pSrc = Namespace(i);
        if (pidl1)
        {
            if (pSrc && ILIsEqual(pSrc->GetPidl(), pidl1))
                hres = S_OK;
        }
        else if (pidl2) // If you pass a pidl2 it means: Is pidl2 a parent of one of my namespaces?
        {
            if (pSrc && ILIsParent(pidl2, pSrc->GetPidl(), FALSE))
                hres = S_OK;
        }

    }
    return hres;
}

//-------------------------------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::Register( 
    HWND hwnd, 
    UINT uMsg, 
    long lEvents )
{
    int i, cSrcs ;

    if( 0 >= (cSrcs = NamespaceCount()) )
        return E_FAIL ;
    
    for( i = 0; i < cSrcs ; i++ )
    {
        CNamespace* pSrc ;
        if( NULL != (pSrc = Namespace( i )) )
            pSrc->RegisterNotify( hwnd, uMsg, lEvents ) ;
    }
    return S_OK ;
}

//-------------------------------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::Unregister ()
{
    int i, cSrcs = NamespaceCount() ;
    
    if( cSrcs <= 0 )
        return E_FAIL ;
    
    for( i = 0; i < cSrcs ; i++ )
    {
        CNamespace* pSrc ;
        if( NULL != (pSrc = Namespace( i )) )
            pSrc->UnregisterNotify() ;
    }
    return S_OK ;
}

// *** IDropTarget methods ***
#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

HRESULT CAugmentedMergeISF::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ASSERT(!_fCommon);
    ASSERT(_pdt == NULL);
    if (pDataObj)
    {        
        InitClipboardFormats();
        
        FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;

        medium.pUnkForRelease = NULL;
        medium.hGlobal = NULL;

        if (SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
        {
            LPIDA pida = (LPIDA)GlobalLock(medium.hGlobal);
            
            if (pida)
            {
                LPCITEMIDLIST pidlItem = HIDA_GetPIDLFolder(pida);

                _fCommon = BOOLIFY(_IsCommonPidl(pidlItem));
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }

        CNamespace *pSrc = NULL;
        ULONG gdnsAttribs = 0;
        
        if (!_fCommon)
            gdnsAttribs = ASFF_DEFNAMESPACE_ALL;
            
        if (SUCCEEDED(GetDefNamespace(gdnsAttribs, (PVOID*)&pSrc, NULL, NULL)))
        {
            if (SUCCEEDED(pSrc->ShellFolder()->CreateViewObject(_hwnd, IID_IDropTarget, (void **)&_pdt)))
            {
                _pdt->DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
            }
        }        
    }

    _grfDragEnterKeyState = grfKeyState;
    _dwDragEnterEffect = *pdwEffect;

    return S_OK;
}

HRESULT CAugmentedMergeISF::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres = S_OK;

    if (_pdt)
        hres = _pdt->DragOver(grfKeyState, pt, pdwEffect);
        
    return hres;
}

HRESULT CAugmentedMergeISF::DragLeave(void)
{
    HRESULT hres = S_OK;

    _fCommon = 0;
    if (_pdt)
    {
        hres = _pdt->DragLeave();
        ATOMICRELEASE(_pdt);
    }
    
    return hres;
}

HRESULT CAugmentedMergeISF::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres = S_OK;
    BOOL    bNoUI = !_fCommon;
    BOOL    bConfirmed = !_fCommon;

    if (!_pdt && pDataObj)
    {
        LPITEMIDLIST pidlParent = NULL,
                     pidlOther  = NULL;
        
        int csidl = _fCommon ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU,
            csidlOther = _fCommon ? CSIDL_STARTMENU : CSIDL_COMMON_STARTMENU;

        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidlParent)) &&
            SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidlOther, &pidlOther)))
        {
            FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;
            
            medium.pUnkForRelease = NULL;
            medium.hGlobal = NULL;

            if (SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
            {
                LPIDA pida = (LPIDA)GlobalLock(medium.hGlobal);
                
                if (pida)
                {
                    IShellFolder *psfParent = NULL,
                                 *psfOther  = NULL;

                    if (SUCCEEDED(IEBindToObject(pidlParent, &psfParent)) &&
                        SUCCEEDED(IEBindToObject(pidlOther,  &psfOther)))
                    {
                        LPCITEMIDLIST pidlItem, pidl;
                        LPITEMIDLIST  pidlRel;
                                      
                        pidlItem   = HIDA_GetPIDLItem(pida, 0);

                        // we came here because we don't have pdt which means that
                        // there is only one folder in our namespace and that's not
                        // the one we have to drop IDataObj on.
                        pidl = Namespace(0)->GetPidl();
                        pidlRel = ILClone(ILFindChild(pidlOther, pidl));
                        
                        STRRET strret;
                        TCHAR  szDir[MAX_PATH];

                        strret.uType = STRRET_CSTR;
                        if (SUCCEEDED(psfParent->GetDisplayNameOf(pidlRel, SHGDN_FORPARSING, &strret)) &&
                            SUCCEEDED(StrRetToBuf(&strret, pidlRel, szDir, ARRAYSIZE(szDir))))
                        {
                            if (_fCommon)
                            {
                                bConfirmed = AffectAllUsers(_hwnd);
                                bNoUI = TRUE;
                            }

                            if (bConfirmed)
                            {
                                BOOL bCreated = FALSE;

                                switch (SHCreateDirectory(_hwnd, szDir))
                                {
                                case ERROR_FILENAME_EXCED_RANGE:
                                case ERROR_FILE_EXISTS:
                                case ERROR_ALREADY_EXISTS:
                                case 0: // It was created successfully.
                                    bCreated = TRUE;
                                }

                                if (bCreated)
                                {
                                    IShellFolder *psf;
                                    
                                    if (SUCCEEDED(psfParent->BindToObject(pidlRel, NULL, IID_IShellFolder, (void **)&psf)))
                                    {
                                        psf->CreateViewObject(_hwnd, IID_IDropTarget, (void **)&_pdt);
                                        // we're going to call drop on it, call dragenter first
                                        if (_pdt)
                                            _pdt->DragEnter(pDataObj, _grfDragEnterKeyState, pt, &_dwDragEnterEffect);
                                        psf->Release();
                                    }
                                }
                            }
                        }
                          
                        ILFree(pidlRel);
                    }
                    if (psfParent)
                        psfParent->Release();
                    if (psfOther)
                        psfOther->Release();

                    GlobalUnlock(medium.hGlobal);
                }
                ReleaseStgMedium(&medium);
            }
        }
        ILFree(pidlParent);
        ILFree(pidlOther);
    }

    if (_pdt)
    {
        hres = E_FAIL;
        
        if ((bNoUI || (bConfirmed = AffectAllUsers(_hwnd))) && bConfirmed)
            hres = _pdt->Drop(pDataObj, grfKeyState, pt, pdwEffect);
        else
            hres = _pdt->DragLeave();

        ATOMICRELEASE(_pdt);
    }
    _fCommon = 0;

    return hres;
}

//-------------------------------------------------------------------------------------------------//
LPITEMIDLIST CAugmentedMergeISF::GetNativePidl(LPCITEMIDLIST pidlWrap, LPARAM nSrcID /*int nID*/)
{
    LPITEMIDLIST pidlRet = NULL;

    if (SUCCEEDED(AugMergeISF_GetSrcPidl(pidlWrap, (UINT)nSrcID, &pidlRet)))
        return pidlRet ;

    // not wrapped by me.
    return NULL;
}

BOOL AffectAllUsers(HWND hwnd)
{
    TCHAR szMessage[255];
    TCHAR szTitle[20];
    BOOL  bRet = FALSE;

    if (MLLoadShellLangString(IDS_ALLUSER_WARNING, szMessage, ARRAYSIZE(szMessage)) > 0 &&
        MLLoadShellLangString(IDS_ALLUSER_WARNING_TITLE, szTitle, ARRAYSIZE(szTitle)) > 0)
    {
        bRet = IDYES == MessageBox(hwnd, szMessage, szTitle, MB_YESNO | MB_ICONINFORMATION);
    }
    return bRet;
}

BOOL CAugmentedMergeISF::_IsCommonPidl(LPCITEMIDLIST pidlItem)
{
    BOOL bRet = FALSE;
    LPITEMIDLIST pidlCommon;
    
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }
    if (!bRet &&
        SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }
    if (!bRet &&
        SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }

    return bRet;
}

HRESULT CAugmentedMergeISF::_SearchForPidl(IShellFolder* psf, LPCITEMIDLIST pidl, BOOL fFolder, int* piIndex, CAugISFEnumItem** ppEnumItem)
{
    STRRET str;
    TCHAR szDisplayName[MAX_PATH];
    int iIndex = -1;

    *ppEnumItem = NULL;

    if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, &str)) &&
        SUCCEEDED(StrRetToBuf(&str, pidl, szDisplayName, ARRAYSIZE(szDisplayName))))
    {
        AUGMISFSEARCHFORPIDL SearchFor;
        SearchFor.pszDisplayName = szDisplayName;
        SearchFor.fFolder = fFolder;

        iIndex = DPA_Search(_hdpaObjects, (LPVOID)&SearchFor, 0,
                AugMISFSearchForOnePidlByDisplayName, NULL, DPAS_SORTED);

        if (iIndex >= 0)
        {
            *ppEnumItem = DPA_GETPTR( _hdpaObjects, iIndex, CAugISFEnumItem);
        }
    }

    if (piIndex)
        *piIndex = iIndex;

    if (*ppEnumItem)
        return S_OK;

    return S_FALSE;
}


// given a wrapped pidl
// f-n returns common and user namespaces (if they are in wrapped pidl) -- note that they are not addrefed
// unwrapped pidl, and if the unwrapped pidl is folder or not
HRESULT CAugmentedMergeISF::_GetNamespaces(LPCITEMIDLIST pidlWrap, 
                                           CNamespace** ppnsCommon, 
                                           UINT* pnCommonID,
                                           CNamespace** ppnsUser, 
                                           UINT* pnUserID,
                                           LPITEMIDLIST* ppidl, 
                                           BOOL *pbIsFolder)
{    
    HRESULT      hres;
    UINT         nSrcID;
    CNamespace * pns;
    int          cWrapped;
    HANDLE       hEnum;
    
    ASSERT(ppnsCommon && ppnsUser && ppidl);

    *ppnsCommon = NULL;
    *ppnsUser   = NULL;
    *ppidl      = NULL;

    ASSERT(SUCCEEDED(AugMergeISF_IsWrap(pidlWrap)));
        
    cWrapped = AugMergeISF_GetSourceCount(pidlWrap);

    if (NULL == _hdpaNamespaces || 0 >= cWrapped || 
        NULL == (hEnum = AugMergeISF_EnumFirstSrcPidl(pidlWrap, &nSrcID, ppidl)))
        return E_FAIL;
        
    hres = QueryNameSpace(nSrcID, (void **)&pns);
    if (EVAL(SUCCEEDED(hres)))
    {
        IShellFolder * psf;
        ULONG rgf = SFGAO_FOLDER;

        psf = pns->ShellFolder(); // no addref
        ASSERT(psf);
        if (SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST*)ppidl, &rgf)))
        {
            if (pbIsFolder)
                *pbIsFolder = rgf & SFGAO_FOLDER;
                
            LPITEMIDLIST   pidlItem;
            UINT           nCommonID;
            CNamespace*    pnsCommonTemp;

            // get common namespace (attribs = 0)
            hres = GetDefNamespace(0, (void **)&pnsCommonTemp, &nCommonID, NULL);
            ASSERT(NamespaceCount() == 2 && SUCCEEDED(hres) || NamespaceCount() == 1);
            if (FAILED(hres))
                nCommonID = 1;

            if (nCommonID == nSrcID)
            {
                *ppnsCommon = pns;
                if (pnCommonID)
                    *pnCommonID = nCommonID;
            }
            else
            {
                *ppnsUser = pns;
                if (pnUserID)
                    *pnUserID = nSrcID;
            }
            
            if (AugMergeISF_EnumNextSrcPidl(hEnum, &nSrcID, &pidlItem))
            {
                ASSERT(ILIsEqual(*ppidl, pidlItem));
                ILFree(pidlItem);
                if (SUCCEEDED(QueryNameSpace(nSrcID, (void **)&pns)))
                {
                    ASSERT(pns);
                    if (nCommonID == nSrcID)
                    {
                        *ppnsCommon = pns;
                        if (pnCommonID)
                            *pnCommonID = nCommonID;
                    }
                    else
                    {
                        *ppnsUser = pns;
                        if (pnUserID)
                            *pnUserID = nSrcID;
                    }
                }
            }

            hres = S_OK;
        }
    }
    AugMergeISF_EndEnumSrcPidls(hEnum);

    return hres;
}

HRESULT CAugmentedMergeISF::_GetContextMenu(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, 
                                            UINT * prgfInOut, void ** ppvOut)
{
    HRESULT      hres;
    LPITEMIDLIST pidl;
    BOOL         bIsFolder;
    CNamespace * pnsCommon;
    CNamespace * pnsUser;

    ASSERT(cidl == 1);

    // psfCommon and psfUser are not addrefed
    hres = _GetNamespaces(apidl[0], &pnsCommon, NULL, &pnsUser, NULL, &pidl, &bIsFolder);
    if (SUCCEEDED(hres))
    {
        ASSERT(pnsCommon || pnsUser);
        if (bIsFolder)
        {
            // folder? need our context menu
            IShellFolder * psfCommon = NULL;
            IShellFolder * psfUser = NULL;
            LPCITEMIDLIST  pidlCommon = NULL;
            LPCITEMIDLIST  pidlUser = NULL;

            if (pnsCommon)
            {
                psfCommon  = pnsCommon->ShellFolder();
                pidlCommon = pnsCommon->GetPidl();
            }
            if (pnsUser)
            {
                psfUser    = pnsUser->ShellFolder();
                pidlUser   = pnsUser->GetPidl();
            }
            CAugMergeISFContextMenu * pcm = CreateMergeISFContextMenu(psfCommon, pidlCommon, 
                                                                      psfUser, pidlUser,
                                                                      pidl, hwnd, prgfInOut);

            if (pcm)
            {
                hres = pcm->QueryInterface(IID_IContextMenu, ppvOut);
                pcm->Release();
            }
            else
                hres = E_OUTOFMEMORY;
        }
        else
        {   // it's not a folder
            // delegate to the isf
            IShellFolder * psf = pnsUser ? pnsUser->ShellFolder() : pnsCommon->ShellFolder();

            hres = psf->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pidl, IID_IContextMenu, prgfInOut, ppvOut);
        }
        ILFree(pidl);
    }

    return hres;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAugmentedMergeISF::GetDefNamespace( 
    LPCITEMIDLIST pidlWrap, 
    ULONG dwAttrib, 
    OUT IShellFolder** ppsf,
    OUT LPITEMIDLIST* ppidlItem )
{
    HRESULT          hr ;
    LPITEMIDLIST     pidl ;
    CNamespace* pSrc ;
    ULONG            dwDefAttrib = dwAttrib & ASFF_DEFNAMESPACE_ALL ;
    int              cWrapped ;
    UINT             nSrcID ;
    HANDLE           hEnum ;

    ASSERT( ppsf ) ;
    
    *ppsf = NULL ;
    if (ppidlItem) 
        *ppidlItem = NULL ;

    if (FAILED((hr = AugMergeISF_IsWrap( pidlWrap ))))
        return hr ;
    cWrapped = AugMergeISF_GetSourceCount( pidlWrap ) ;

    //  No namespaces?
    if (NULL == _hdpaNamespaces || 0 >= cWrapped || 
        NULL == (hEnum = AugMergeISF_EnumFirstSrcPidl( pidlWrap, &nSrcID, &pidl)))
        return E_FAIL ;

    //  Only one namespace in wrap? Give up the shell folder and item ID.
    if (1 == cWrapped || 0==dwDefAttrib)
    {
        AugMergeISF_EndEnumSrcPidls( hEnum ) ; // no need to go further

        //  Retrieve the namespace object identified by nSrcID.
        if( SUCCEEDED( (hr = QueryNameSpace( nSrcID, (PVOID*)&pSrc )) ) ) 
        {
            *ppsf = pSrc->ShellFolder() ;
            if( ppidlItem )
                *ppidlItem = pidl ;
            return S_OK ;
        }

        ILFree( pidl ) ;
        return hr ;
    }

    //  More than one namespace in wrap?
    if( cWrapped > 1 )
    {
        LPITEMIDLIST   pidl0   = NULL ;
        CNamespace*    pSrc0   = NULL ;  // get this below.

        for (BOOL bEnum = TRUE ; bEnum ; 
             bEnum = AugMergeISF_EnumNextSrcPidl(hEnum, &nSrcID,  &pidl))
        {
            if (SUCCEEDED((hr = QueryNameSpace(nSrcID, (PVOID*)&pSrc)))) 
            {
                if (dwDefAttrib & pSrc->Attrib())
                {
                    //  Matched attributes; we're done.
                    AugMergeISF_EndEnumSrcPidls(hEnum);
                    *ppsf = pSrc->ShellFolder() ;
                    if (ppidlItem)
                        *ppidlItem = pidl;
                    if(pidl0) 
                        ILFree(pidl0);
                    return S_OK ;
                }

                //  Stash first namespace object and item pidl.  
                //  We'll default to these if                
                if( NULL == pSrc0 )
                {
                    pSrc0 = pSrc ;
                    pidl0 = ILClone( pidl ) ;
                }
            }
            ILFree( pidl ) ;
        }
        AugMergeISF_EndEnumSrcPidls( hEnum ) ;
        
        //  Default to first namespace
        if( pSrc0 && pidl0 )
        {
            *ppsf       = pSrc0->ShellFolder() ;
            if( ppidlItem )
                *ppidlItem  = pidl0 ;
            return S_OK ;
        }
    }
    
    return E_UNEXPECTED ;
}

//-------------------------------------------------------------------------//
//  Retrieves the default namespaces for the indicated attibutes.
//  The dwAttrib arg must be initialized prior to function entry,
STDMETHODIMP CAugmentedMergeISF::GetDefNamespace( 
    ULONG dwAttrib,
    OUT   PVOID* ppSrc, UINT *pnSrcID, PVOID* ppSrc0 )
{
    CNamespace* pSrc ;
    ULONG       dwDefAttrib = dwAttrib & ASFF_DEFNAMESPACE_ALL ;

    // this is an internal helper so we better make sure we pass the correct params!
    //if (NULL == ppSrc)
    //    return E_INVALIDARG ;
    *ppSrc = NULL ;
    if( ppSrc0 ) 
        *ppSrc0 = NULL ;

    for( int i = 0, cSrcs = NamespaceCount(); i < cSrcs ; i++ )
    {
        if( NULL != (pSrc = Namespace( i )) )
        {
            if( 0 == i && ppSrc0 )
                *ppSrc0 = pSrc ;

            if( dwDefAttrib & pSrc->Attrib() || 
                dwDefAttrib == 0 && !(pSrc->Attrib() & ASFF_DEFNAMESPACE_ALL))
            {
                *ppSrc = pSrc;
                if (NULL != pnSrcID)
                    *pnSrcID = i;
                return S_OK ;
            }
        }
    }

    return E_FAIL ;
}

// BUGBUG(lamadio): Move this to a better location, This is a nice generic function
#ifdef DEBUG
BOOL DPA_VerifySorted(HDPA hdpa, PFNDPACOMPARE pfn, LPARAM lParam)
{
    if (!EVAL(hdpa))
        return FALSE;

    for (int i = 0; i < DPA_GetPtrCount(hdpa) - 1; i++)
    {
        if (pfn(DPA_FastGetPtr(hdpa, i), DPA_FastGetPtr(hdpa, i + 1), lParam) > 0)
            return FALSE;
    }

    return TRUE;
}
#else
#define DPA_VerifySorted(hdpa, pfn, lParam)
#endif

int CAugmentedMergeISF::AcquireObjects()
{
    HDPA hdpa2 = NULL;

    DEBUG_CODE(_fInternalGDNO = TRUE);
    
    for (int i = 0; i < DPA_GETPTRCOUNT(_hdpaNamespaces); i++)
    {
        CNamespace * pns = DPA_GETPTR(_hdpaNamespaces, i, CNamespace);
        IShellFolder * psf;
        IEnumIDList  * peid;
        HDPA         * phdpa;

        ASSERT(pns);
        psf = pns->ShellFolder(); // no addref here!

        if (i == 0)
        {
            phdpa = &_hdpaObjects;
            _hdpaObjects = DPA_Create(4);   // We should always create the DPA
        }
        else
        {
            ASSERT(i == 1); // no support for more than 2 isf's
            phdpa = &hdpa2;
        }
        
        HRESULT hres = psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &peid);
        if (SUCCEEDED(hres))
        {
            if (!*phdpa)
                *phdpa = DPA_Create(4);

            if (*phdpa)
            {
                LPITEMIDLIST pidl;
                ULONG        cEnum;
                
                while (SUCCEEDED(peid->Next(1, &pidl, &cEnum)) && 1 == cEnum)
                {
                    CAugISFEnumItem* paugmEnum = new CAugISFEnumItem;

                    if (paugmEnum)
                    {
                        
                        if (paugmEnum->Init(SAFECAST(this, IAugmentedShellFolder2*), i, pidl))
                        {
                            if (DPA_AppendPtr(*phdpa, paugmEnum) == -1)
                                DestroyObjectsProc(paugmEnum, NULL);
                        }
                        else
                            delete paugmEnum;
                    }
                    ILFree(pidl);
                }
            }
            peid->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "CAugMISF::AcquireObjects: Failed to get enumerator 0x%x", hres);

        }
    }

    // now that we have both hdpa's (or one) let's merge them.
    if (DPA_GETPTRCOUNT(_hdpaNamespaces) == 2 && hdpa2)
    {
        DPA_Merge(_hdpaObjects, hdpa2, DPAM_UNION, AugmEnumCompare, AugmEnumMerge, (LPARAM)0);
        DPA_DESTROY(hdpa2, DestroyObjectsProc);
    }
    else
    {
        DPA_Sort(_hdpaObjects, AugmEnumCompare, 0);
    }

    ASSERT(DPA_VerifySorted(_hdpaObjects, AugmEnumCompare, 0));

    DEBUG_CODE(_fInternalGDNO = FALSE);


#ifdef DEBUG
    TraceMsg(TF_AUGM, "CAugMISF::AcquireObjects");
    DumpObjects();
#endif

    _count = DPA_GETPTRCOUNT(_hdpaObjects);
    
    return _count;
}

//-------------------------------------------------------------------------//
void CAugmentedMergeISF::FreeObjects()
{
    DPA_DESTROY( _hdpaObjects, DestroyObjectsProc ) ;
    _hdpaObjects = NULL;
    _count = 0 ;
}

//-------------------------------------------------------------------------//
int CAugmentedMergeISF::DestroyObjectsProc( LPVOID pv, LPVOID pvData )
{
    CAugISFEnumItem* paugmEnum = (CAugISFEnumItem*)pv;

    if (EVAL(NULL != paugmEnum))
    {
        delete paugmEnum;
    }
    return TRUE ;
}

//-------------------------------------------------------------------------//
void CAugmentedMergeISF::FreeNamespaces()
{
    DPA_DESTROY( _hdpaNamespaces, DestroyNamespacesProc ) ;
}

//-------------------------------------------------------------------------//
int CAugmentedMergeISF::DestroyNamespacesProc( LPVOID pv, LPVOID pvData )
{
    CNamespace* p ;
    if( NULL != (p = (CNamespace*)pv) )
        delete p ;
    return TRUE ;
}


STDMETHODIMP CAugmentedMergeISF::GetPidl(int* piPos, DWORD grfEnumFlags, LPITEMIDLIST* ppidl)
{
    *ppidl = NULL;

    if (_hdpaObjects == NULL)
        AcquireObjects();

    if (_hdpaObjects == NULL)
        return E_OUTOFMEMORY;

    BOOL   fWantFolders    = 0 != (grfEnumFlags & SHCONTF_FOLDERS),
           fWantNonFolders = 0 != (grfEnumFlags & SHCONTF_NONFOLDERS),
           fWantHidden     = 0 != (grfEnumFlags & SHCONTF_INCLUDEHIDDEN) ;

    while (*piPos < _count)
    {
        CAugISFEnumItem* paugmEnum = DPA_GETPTR( _hdpaObjects, *piPos, CAugISFEnumItem);
        if ( NULL != paugmEnum )
        {
            BOOL fFolder         = 0 != (paugmEnum->_rgfAttrib & SFGAO_FOLDER),
                 fHidden         = 0 != (paugmEnum->_rgfAttrib & SFGAO_HIDDEN);
             
            if ((!fHidden || fWantHidden) && 
                ((fFolder && fWantFolders) || (!fFolder && fWantNonFolders)))
            {
                //  Copy out the pidl ;
                *ppidl = ILClone(paugmEnum->_pidlWrap);
                break;
            }
            else
            {
                (*piPos)++;
            }
        }
    }

    if (*ppidl)
        return S_OK;

    return S_FALSE;
}


//-------------------------------------------------------------------------//
CEnum::CEnum(IAugmentedMergedShellFolderInternal* psmsfi, DWORD grfEnumFlags, int iPos) : 
        _cRef(1), 
        _iPos(iPos),
        _psmsfi(psmsfi),
        _grfEnumFlags(grfEnumFlags)

{ 
    _psmsfi->AddRef();
}

CEnum::~CEnum()
{
    ATOMICRELEASE(_psmsfi);
}

//-------------------------------------------------------------------------//
//  class CEnum - IUnknown methods 
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CEnum::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    static const QITAB qit[] = { 
        QITABENT(CEnum, IEnumIDList), 
        { 0 } 
    };
    return QISearch(this, qit, riid, ppvObj);
}
//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CEnum::AddRef ()
{
    return InterlockedIncrement((LONG*)&_cRef);
}
//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CEnum::Release ()
{
    if (InterlockedDecrement((LONG*)&_cRef)) 
        return _cRef;
    
    delete this ;
    return 0;
}

//-------------------------------------------------------------------------//
//  class CEnum - IEnumIDList methods 
//-------------------------------------------------------------------------//
STDMETHODIMP CEnum::Next( 
    ULONG celt,
    LPITEMIDLIST *rgelt,
    ULONG *pceltFetched )
{
    int iStart = _iPos;
    int cFetched = 0;
    HRESULT hres = S_OK;

    if( !(celt > 0 && rgelt) || (NULL == pceltFetched && celt > 1 ) )
        return E_INVALIDARG ;
    
    *rgelt = 0;

    while(hres == S_OK && (_iPos - iStart) < (int)celt)
    {
        LPITEMIDLIST pidl;
        hres = _psmsfi->GetPidl(&_iPos, _grfEnumFlags, &pidl);
        if (hres == S_OK)
        {
            rgelt[cFetched] = pidl;
            cFetched++ ;
        }
        _iPos++;
    }
    
    if( pceltFetched )
        *pceltFetched = cFetched ;
    
    return celt == (ULONG)cFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnum::Skip(ULONG celt)
{
    _iPos += celt;
    return S_OK ;
}
//-------------------------------------------------------------------------//
STDMETHODIMP CEnum::Reset()
{
    _iPos = 0;
    return S_OK ;
}
//-------------------------------------------------------------------------//
// REVIEW: Can probably be E_NOTIMPL
STDMETHODIMP CEnum::Clone( IEnumIDList **ppenum )
{
    if( NULL == (*ppenum = new CEnum( _psmsfi, _grfEnumFlags, _iPos )) )
        return E_OUTOFMEMORY;

    return S_OK;
}



BOOL CAugISFEnumItem::Init(IShellFolder* psf, int iShellFolder, LPCITEMIDLIST pidl)
{
    // This is ok, the memory just gets written to twice. 
    if (SUCCEEDED(AugMergeISF_CreateWrap(pidl, iShellFolder, &_pidlWrap)))
    {
        // Takes ownership of passed in pidl.
        return InitWithWrappedToOwn(psf, iShellFolder, _pidlWrap);
    }

    return FALSE;
}

BOOL CAugISFEnumItem::InitWithWrappedToOwn(IShellFolder* psf, int iShellFolder, LPITEMIDLIST pidl)
{
    BOOL fRet = FALSE;
    STRRET str;
    TCHAR  szDisplayName[MAX_PATH];

    _pidlWrap = pidl;
    
    _rgfAttrib = SFGAO_FOLDER | SFGAO_HIDDEN;

    psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &_rgfAttrib);

    if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, &str)) &&
        SUCCEEDED(StrRetToBuf(&str, pidl, szDisplayName, ARRAYSIZE(szDisplayName))))
    {
        SetDisplayName(szDisplayName);
        fRet = TRUE;
    }
    return fRet;
}
