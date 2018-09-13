//-------------------------------------------------------------------------//
//  
//  AugMisf.h  - Augmented Merge IShellFolder class declaration.
//
//-------------------------------------------------------------------------//

#ifndef __AUGMISF_H__
#define __AUGMISF_H__

class CAugISFEnumItem
{
public:
    CAugISFEnumItem()   {};
    BOOL Init(IShellFolder* psf, int iShellFolder, LPCITEMIDLIST pidl);
    BOOL InitWithWrappedToOwn(IShellFolder* psf, int iShellFolder, LPITEMIDLIST pidl);

    ~CAugISFEnumItem()
    {
        Str_SetPtr(&_pszDisplayName, NULL);
        ILFree(_pidlWrap);
    }
    void SetDisplayName(LPTSTR pszDispName)
    {
        Str_SetPtr(&_pszDisplayName, pszDispName);
    }
    ULONG          _rgfAttrib;
    LPTSTR         _pszDisplayName;
    LPITEMIDLIST   _pidlWrap;
};


//  Forwards:
//-------------//
class CNamespace ;
STDAPI CAugmentedMergeISF_CreateInstance( IUnknown*, IUnknown**, LPCOBJECTINFO );  

class IAugmentedMergedShellFolderInternal : public IUnknown
{
public:
    STDMETHOD(GetPidl)(THIS_ int* /*in|out*/ piPos, DWORD grfEnumFlags, LPITEMIDLIST* ppidl) PURE;
};

//-------------------------------------------------------------------------//
//  Supports hierarchically merged shell namespaces
class CAugmentedMergeISF : public IAugmentedShellFolder2,
                           public IShellFolder2,
                           public IShellService,
                           public ITranslateShellChangeNotify,
                           public IDropTarget,
                           public IAugmentedMergedShellFolderInternal
//-------------------------------------------------------------------------//
{
public:
    // *** IUnknown methods ***
    STDMETHOD ( QueryInterface )    ( REFIID, void ** ) ;
    STDMETHOD_( ULONG, AddRef )     ( ) ;
    STDMETHOD_( ULONG, Release )    ( ) ;
    
    // *** IShellFolder methods ***
    STDMETHOD( BindToObject )       ( LPCITEMIDLIST, LPBC, REFIID, LPVOID * ) ;
    STDMETHOD( BindToStorage )      ( LPCITEMIDLIST, LPBC, REFIID, LPVOID * ) ;
    STDMETHOD( CompareIDs )         ( LPARAM, LPCITEMIDLIST, LPCITEMIDLIST) ;
    STDMETHOD( CreateViewObject )   ( HWND, REFIID, LPVOID * ) ;
    STDMETHOD( EnumObjects )        ( HWND, DWORD, LPENUMIDLIST * ) ;
    STDMETHOD( GetAttributesOf )    ( UINT, LPCITEMIDLIST * , ULONG * ) ;
    STDMETHOD( GetDisplayNameOf )   ( LPCITEMIDLIST , DWORD , LPSTRRET ) ;
    STDMETHOD( GetUIObjectOf )      ( HWND, UINT, LPCITEMIDLIST *, REFIID, UINT *, LPVOID * ) ;
    STDMETHOD( ParseDisplayName )   ( HWND, LPBC, LPOLESTR, ULONG *, LPITEMIDLIST *, ULONG * ) ;
    STDMETHOD( SetNameOf )          ( HWND, LPCITEMIDLIST, LPCOLESTR, DWORD, LPITEMIDLIST *) ;

    // *** IShellFolder2 methods ***
    // stub implementation to indicate we support CompareIDs() for identity
    STDMETHOD( GetDefaultSearchGUID)( LPGUID ) { return E_NOTIMPL; }
    STDMETHOD( EnumSearches )       ( LPENUMEXTRASEARCH *pe) { *pe = NULL; return E_NOTIMPL; }
    STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) { return E_NOTIMPL; };
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pbState) { return E_NOTIMPL; };
    STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) { return E_NOTIMPL; };
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails){ return E_NOTIMPL; };
    STDMETHOD(MapColumnToSCID)(UINT iCol, SHCOLUMNID *pscid) { return E_NOTIMPL; };

    // *** IAugmentedShellFolder methods ***
    STDMETHOD( AddNameSpace )       ( const GUID * pguidObject, IShellFolder * psf, LPCITEMIDLIST pidl, DWORD dwFlags ) ;
    STDMETHOD( GetNameSpaceID )     ( LPCITEMIDLIST pidl, GUID * pguidOut ) ;
    STDMETHOD( QueryNameSpace )     ( DWORD dwID, GUID * pguidOut, IShellFolder ** ppsf ) ;
    STDMETHOD( EnumNameSpace )      ( DWORD cNameSpaces, DWORD * pdwID ) ;

    // *** IAugmentedShellFolder2 methods ***
    // not used anywhere
    //STDMETHOD( GetNameSpaceCount )  ( OUT LONG* pcNamespaces ) ;
    //STDMETHOD( GetIDListWrapCount)  ( LPCITEMIDLIST pidlWrap, OUT LONG * pcPidls) ;
    STDMETHOD( UnWrapIDList)        ( LPCITEMIDLIST pidlWrap, LONG cPidls, IShellFolder** apsf, LPITEMIDLIST* apidlFolder, LPITEMIDLIST* apidlItems, LONG* pcFetched ) ;

    // *** IShellService methods ***
    STDMETHOD( SetOwner )           ( IUnknown * punkOwner ) ;

    // *** ITranslateShellChangeNotify methods ***
    STDMETHOD( TranslateIDs )       ( LONG *plEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
                                      LONG *plEvent2, LPITEMIDLIST * ppidlOut1Event2, LPITEMIDLIST * ppidlOut2Event2);
    STDMETHOD( IsChildID )          ( LPCITEMIDLIST pidlKid, BOOL fImmediate );
    STDMETHOD( IsEqualID )          ( LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2 );
    STDMETHOD( Register )           ( HWND hwnd, UINT uMsg, long lEvents );
    STDMETHOD( Unregister )         ( void );

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)(void);
    STDMETHOD(Drop)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IAugmentedMergedShellFolderInternal ***
    STDMETHODIMP GetPidl(int* piPos, DWORD grfEnumFlags, LPITEMIDLIST* ppidl);

//  Construction, Destruction
protected:
    CAugmentedMergeISF() ;
    virtual ~CAugmentedMergeISF() ;

//  Miscellaneous helpers
protected:
    STDMETHOD( QueryNameSpace )     ( DWORD dwID, OUT PVOID* ppvNameSpace ) ;
    
    //  pidl crackers
    STDMETHOD_( LPITEMIDLIST, GetNativePidl )( LPCITEMIDLIST pidl, LPARAM lParam /*int nID*/) ;

    BOOL     _IsCommonPidl(LPCITEMIDLIST pidlItem);

    HRESULT _SearchForPidl(IShellFolder* psf, LPCITEMIDLIST pidl, BOOL fFolder, int* piIndex, CAugISFEnumItem** ppEnumItem);
    HRESULT  _GetNamespaces(LPCITEMIDLIST pidlWrap, CNamespace** ppnsCommon, UINT* pnCommonID,
                                                    CNamespace** ppnsUser, UINT* pnUserID,
                                                    LPITEMIDLIST* ppidl, BOOL *pbIsFolder);
    HRESULT  _GetContextMenu(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, 
                                        UINT * prgfInOut, LPVOID* ppvOut);
    BOOL     _AffectAllUsers();
    //  namespace utility methods
    STDMETHOD( GetDefNamespace )    ( LPCITEMIDLIST pidl, ULONG, OUT IShellFolder** ppsf, OUT LPITEMIDLIST* ppv) ;
    STDMETHOD( GetDefNamespace )    ( ULONG dwAttrib, OUT PVOID* ppv, OUT UINT *pnID, OUT PVOID* ppv0) ;
    CNamespace* Namespace( int iNamespace ) ;
    int         NamespaceCount() const ;
    void        FreeNamespaces() ;

    int                 AcquireObjects() ;
    void                FreeObjects() ;
    static int CALLBACK DestroyObjectsProc( LPVOID pv, LPVOID pvData ) ;
    friend int CALLBACK AugMISFSearchForWrappedPidl(LPVOID p1, LPVOID p2, LPARAM lParam);
    BOOL IsChildIDInternal(LPCITEMIDLIST pidl, BOOL fImmediate, int* iShellFolder);

#ifdef DEBUG
    void DumpObjects();
#endif
    
//  Callback routines
private:
    static      int SetOwnerProc( LPVOID, LPVOID ) ;
    static      int DestroyNamespacesProc(LPVOID pv, LPVOID pvData) ;

//  Data
protected:
    HDPA          _hdpaNamespaces ;     // source namespace collection
    LPUNKNOWN     _punkOwner ;          // owner object
    LONG          _cRef;                // reference count.
    IDropTarget*  _pdt;
    HWND          _hwnd;
    BITBOOL       _fCommon : 1;         // is _pdt a common programs folder (or its child)
#ifdef DEBUG
    BITBOOL       _fInternalGDNO:1 ;
#endif
    HDPA          _hdpaObjects;
    int           _count;
    DWORD         _grfDragEnterKeyState;
    DWORD         _dwDragEnterEffect;

    friend HRESULT CAugmentedMergeISF_CreateInstance( IUnknown*, IUnknown**, LPCOBJECTINFO );  
} ;

//-------------------------------------------------------------------------//
//  inline implementation 
inline int CAugmentedMergeISF::NamespaceCount() const {
    return _hdpaNamespaces ? DPA_GetPtrCount( _hdpaNamespaces ) : 0 ;
}
inline CNamespace* CAugmentedMergeISF::Namespace( int iNamespace )  {
    return _hdpaNamespaces ? 
        (CNamespace*)DPA_GetPtr( _hdpaNamespaces, iNamespace ) : NULL ;
}

//-------------------------------------------------------------------------//
//  CAugmentedMergeISF enumerator object.
class CEnum : public IEnumIDList
//-------------------------------------------------------------------------//
{
//  Public interface methods
public:
    // *** IUnknown methods ***
    STDMETHOD ( QueryInterface ) (REFIID, void ** ) ;
    STDMETHOD_( ULONG,AddRef )  () ;
    STDMETHOD_( ULONG,Release ) () ;

    // *** IEnumIDList methods ***
    STDMETHOD( Next )  (ULONG, LPITEMIDLIST*, ULONG* ) ;
    STDMETHOD( Skip )  (ULONG) ;
    STDMETHOD( Reset ) ();
    STDMETHOD( Clone ) (IEnumIDList** ) ;

// Construction, destruction, assignment:
public:
    CEnum(IAugmentedMergedShellFolderInternal* psmsfi, DWORD grfEnumFlags, int iPos = 0);
    ~CEnum() ;

    //  Miscellaneous methods, data
protected:
    IAugmentedMergedShellFolderInternal* _psmsfi;

    DWORD _grfEnumFlags;
    int _cRef;
    int _iPos;
} ;



#endif __AUGMISF_H__
