//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       elemdb.hxx
//
//  Contents:   Helper class hiearchy to provide databinding methods to
//              elements.
//
//  Classes:    CDataBindingEvents, CDBindMethods, CDBindMethodsSimple,
//              CDBindMethodsText
//
//------------------------------------------------------------------------

#ifndef I_ELEMDB_HXX_
#define I_ELEMDB_HXX_
#pragma INCMSG("--- Beg 'elemdb.hxx'")

MtExtern(DBMEMBERS);
MtExtern(DBMEMBERS_arydsb_pv);
MtExtern(CDataBindingEvents);
MtExtern(CDataBindingEvents_aryXfer_pv);

class CElement;
class CDataSourceBinder;
class CInstance;
class CDataMemberMgr;

//+---------------------------------------------------------------------------
//
//  enum:       DBFILTER
//
//  Purpose:    When enumerating binding specifications, these flags are
//              used to limit the enumeration to bindings of given category.
//              There are 3 main categories:
//
//              set consumers -- have DATASRC only specified
//              current record bindings -- have DATASRC and DATAFLD
//              table-record bindings -- have DATAFLD only
//
//----------------------------------------------------------------------------
enum DBFILTER
{
    DBIND_SETFILTER = 1,        // enumerate set-bindings
    DBIND_CURRENTFILTER = 2,    // enumerate current-record bindings
    DBIND_TABLEFILTER = 4,      // enumerate table-record bindings
    DBIND_SINGLEFILTER = DBIND_CURRENTFILTER|DBIND_TABLEFILTER,
    DBIND_ALLFILTER = DBIND_SINGLEFILTER|DBIND_SETFILTER,        // all bindings

    // the next filter value is hack used internally for the implementation
    //  of GetDBSpec, which is only interested in one particular ID, not
    //  any necessary succeeding ID.
    DBIND_ONEIDFILTER = 8       // check the next ID only
};

//+---------------------------------------------------------------------------
//
//  struct:     DBSPEC
//
//  Purpose:    Description of one one binding specification.  Although it's
//              a struct, it now has two methods.
//
//----------------------------------------------------------------------------
struct DBSPEC
{
    BOOL FFilter(DWORD dwFlags, BOOL fHackyHierarchicalTableFlag);  // is this desired kind of binding?
    BOOL FHTML();                   // is HTML binding specified?
    BOOL FLocalized();              // is localized-text binding specified?
    
    LPCTSTR  _pStrDataSrc;   // any bindable element may have a datasrc
    LPCTSTR  _pStrDataFld;
    LPCTSTR  _pStrDataFormatAs;
};

//+---------------------------------------------------------------------------
//
//  enum:       DBSPEC
//
//  Purpose:    Describes what kind of binding a given element supports for
//              a given binding id.
//
//----------------------------------------------------------------------------
enum DBIND_KIND
{
    DBIND_NONE,          // this element doesn't support binding,
                         //     or isn't bound
    DBIND_SINGLEVALUE,   // this element can be single-value bound

    // we use negative values for the following so that they compare
    //  arithmetically lower than DBIND_NONE -- they're logically
    //  a discontinguous set

    DBIND_ICURSOR  = -1, // element wants a VB-style ICursor
    DBIND_IROWSET  = -2, // element wants a Nile Rowset
    DBIND_TABLE = -3,    // element is a <TABLE>, wants a DataLayerCursor
    DBIND_IDATASOURCE = -4  // element wants an IDataSource
};

//+---------------------------------------------------------------------------
//
//  enum:       DBIND_TRANSFER
//
//  Purpose:    Flags for describing nature of one binding.
//
//----------------------------------------------------------------------------
enum DBIND_TRANSFER
{
    DBIND_ONEWAY = 0x1,     // only transfer from database to element
    DBIND_HTMLOK = 0x2,     // binding can accept HTML if asked
    DBIND_NOTIFYONLY = 0x4, // don't send data when HROW changes, just notify
    DBIND_TRANSFER_Last_Enum
};

//+---------------------------------------------------------------------------
//
//  struct:     DBINFO
//
//  Purpose:    Description of type and binding behavior for one binding id
//              of one element.
//
//----------------------------------------------------------------------------
struct DBINFO
{
    CVarType _vt;       // data type desired for transfer
    DWORD  _dwTransfer; // restrictions on transfer, see DBIND_TRANSFER
};

//+---------------------------------------------------------------------------
//
//  enum:       ID_DBIND
//
//  Purpose:    All code dealing with binding id's use long values.  (Same
//              convention as DISPIDs.)  This enum is used to define some
//              magic values used in different contexts dealing with
//              binding id's.
//
//----------------------------------------------------------------------------
enum ID_DBIND
{
    ID_DBIND_DEFAULT = 0,   // binding specified on an element's tag
    ID_DBIND_ALL     = -1,  // iterate over all bindings (some methods)
    ID_DBIND_INVALID = -1,  // guaranteed invalid binding id
    ID_DBIND_STARTLOOP = -1,// binding id used to start GetNextDBSpec loop
    ID_DBIND_Last_Enum
};

//+---------------------------------------------------------------------------
//
//  CDataBindingEvents
//
//  This now-poorly named class contains all databinding method calls initiated
//  by the bound element.
//
//----------------------------------------------------------------------------
class CDataBindingEvents
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataBindingEvents))
    CDataBindingEvents       ( )    : _fInSave(0), _fInErrorUpdate(0) { }
    HRESULT SaveIfChanged    ( CElement * pElement, LONG id, BOOL fLoud, BOOL fForceIsCurrent); 
    HRESULT CheckSrcWritable ( CElement * pElement, LONG id );
    HRESULT CompareWithSrc   ( CElement * pElement, LONG id );
    HRESULT TransferFromSrc  ( CElement * pElement, LONG id );
    HRESULT ValueFromSrc     ( CElement * pElement, LONG id, LPVOID lpv );
    BOOL    FBoundID         ( CElement * pElement, LONG id )
        { return(FindPXfer(pElement, id) != NULL); }
        
    void    DetachBinding    ( CElement * pElement, LONG id );
    HRESULT ClearValue       ( CElement * pElement, LONG id );
    HRESULT EnableTransfers  ( CElement * pElement, LONG id, BOOL fTransferOK );
#if DBG == 1
    BOOL    IsEmpty          ( )    { return _aryXfer.Size() == 0; }
#endif

private:
    friend class CXfer;
    
    HRESULT AddXfer          ( CXfer *pXfer )
        { RRETURN(_aryXfer.Append(pXfer)); }
        
    CXfer ** FindPXfer         (CElement * pElement, LONG id );

    // I'd actually prefer to use a TIntrusiveDblLinkedList<CXfer>,
    //  but hxx-file dependency tangles prevent me from doing so.
    DECLARE_CStackPtrAry(CAryXfer, CXfer *, 1, Mt(Mem), Mt(CDataBindingEvents_aryXfer_pv))
    CAryXfer                    _aryXfer;

    // flags to detect and handle recursive save calls
    unsigned                    _fInSave:1;
    unsigned                    _fInErrorUpdate:1;

};

//+---------------------------------------------------------------------------
//
//  Class:      DBMEMBERS
//
//  Purpose:    Holds all databinding information/state for one element
//              instance.  Not allocated until we have reason to believe
//              that a given element is involved in databinding.
//
//              Formerly, a simple struct.  Now it has a bunch of methods,
//              which either used to be on CElement, or which inline forward
//              to its members.
//
//              BUGBUG: change to class
//
//----------------------------------------------------------------------------
struct DBMEMBERS
{
    DECLARE_MEMALLOC_NEW_DELETE(Mt(DBMEMBERS));
    DECLARE_CPtrAry(CAryDsb, CDataSourceBinder *, Mt(Mem), Mt(DBMEMBERS_arydsb_pv));


    CDataBindingEvents  _dbe;       // all bindings attached to this element
    CAryDsb             _arydsb;    // all binders for this element
    CDataMemberMgr *    _pDMembMgr; // element's data member manager

    DBMEMBERS(): _pDMembMgr(NULL) {}
    ~DBMEMBERS()
        {
            Assert(_arydsb.Size() == 0);
            Assert(_dbe.IsEmpty());
            Assert(_pDMembMgr == NULL);
        }
    CDataSourceBinder * GetBinder(LONG id);
    HRESULT             SetBinder(CDataSourceBinder *pdsbBinder)
        { Assert(pdsbBinder); RRETURN(_arydsb.Append(pdsbBinder)); }
    CDataBindingEvents * GetDataBindingEvents ( )
        { return &_dbe; }
    HRESULT SaveIfChanged ( CElement * pElement, LONG id, BOOL fLoud, BOOL fForceIsCurrent )
        { RRETURN1(_dbe.SaveIfChanged (pElement, id, fLoud, fForceIsCurrent), S_FALSE); }
    HRESULT CheckSrcWritable ( CElement * pElement, LONG id )
        { RRETURN1(_dbe.CheckSrcWritable(pElement, id), S_FALSE); }
    HRESULT CompareWithSrc   ( CElement * pElement, LONG id )
        { RRETURN1(_dbe.CompareWithSrc(pElement, id), S_FALSE); }
    HRESULT TransferFromSrc  ( CElement * pElement, LONG id )
        { RRETURN(_dbe.TransferFromSrc(pElement, id)); }
    HRESULT ValueFromSrc     ( CElement * pElement, LONG id, LPVOID lpv )
        { RRETURN(_dbe.ValueFromSrc(pElement, id, lpv)); }
    BOOL    FBoundID         ( CElement * pElement, LONG id )
        { return _dbe.FBoundID(pElement, id); }
        
    void    DetachBinding    ( CElement * pElement, LONG id )
            { _dbe.DetachBinding(pElement, id); }
    HRESULT ClearValue       ( CElement * pElement, LONG id )
        { return _dbe.ClearValue(pElement, id); }
    HRESULT EnableTransfers  ( CElement * pElement, LONG id, BOOL fTransferOK )
        { RRETURN(_dbe.EnableTransfers(pElement, id, fTransferOK)); }

    void    MarkReadyToBind();

    // Data Member Manager methods
    HRESULT EnsureDataMemberManager(CElement *pElement);
    void    ReleaseDataMemberManager(CElement *pElement);
    CDataMemberMgr * GetDataMemberManager() { return _pDMembMgr; }
};


//
//  Class CDBindMethods
//
//  Abstract class to encapsulate methods for inquiring about databinding
//  status of elements, and for setting and getting the data bound value.
//
//  Having this class hierarchy separate from CElement replaces multiple
//  vtable slots in CElement and its derived class with one:
//  GetDBindMethods().  Furthermore, we reduce the amount of recompilation
//  every time that we touch databinding.
//
class NOVTABLE CDBindMethods
{
public:
    
    static HRESULT GetNextDBSpec(CElement *pElem,
                                 LONG *pid,
                                 DBSPEC *pdbs,
                                 DWORD dwFilter = DBIND_ALLFILTER);
    
    static HRESULT GetDBSpec(CElement *pElem,
                             LONG id,
                             DBSPEC *pdbs,
                             DWORD dwFilter = DBIND_ALLFILTER)
    {
        HRESULT hr;
        
        id -= 1;

        hr = GetNextDBSpec(pElem, &id, pdbs, dwFilter | DBIND_ONEIDFILTER);

        RRETURN1(hr, S_FALSE);
    }

    static BOOL FDataSrcValid(CElement *pElem);
    static BOOL FDataFldValid(CElement *pElem);
    static BOOL FDataFormatAsValid(CElement *pElem);
    
    static DBIND_KIND DBindKind(CElement *pElem,
                                LONG id, DBINFO *pdbi = NULL);

    static BOOL IsReadyToBind(CElement *pElem);
    
    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const = 0;
                                        
    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const = 0;

    // Notification that data is ready (or not ready)
    virtual void    OnDataReady ( CElement *pElem, BOOL fReady ) const { }

    // Notification that the source instance has changed
    virtual HRESULT InstanceChanged(CElement *pElem, CInstance *pSrcInstance) const
    {
        AssertSz(0, "Should only be called for hierarchical tables");
        return S_OK;
    }
    

protected:
    CDBindMethods()     {}
    ~CDBindMethods()    {}
    
    virtual DBIND_KIND DBindKindImpl(CElement *pElem,
                                     LONG id,
                                     DBINFO *pdbi) const = 0;
    virtual BOOL IsReadyImpl(CElement *pElem) const = 0;
    
    virtual BOOL FDataSrcValidImpl(CElement *pElem) const;
    virtual BOOL FDataFldValidImpl(CElement *pElem) const;
    virtual BOOL FDataFormatAsImpl() const    {return FALSE; }
    virtual HRESULT GetNextDBSpecCustom(CElement *pElem,
                                        LONG *pid,
                                        DBSPEC *pdbs) const
    {
        return S_FALSE;
    }
};

#ifndef NO_DATABINDING
//
//  class CDBindMethodsSimple
//
//  Semi-concrete base class for providing simple implementation of DBindKind
//  for elements which support a single simple binding.
//
//  Users of this class derive from it, and implement their own
//  BoundValueToElement() and BoundValueFromElement().  They in turn instantiate
//  a concrete instance of their derived class with correct _vt and _dwTransfer.
//
class NOVTABLE CDBindMethodsSimple : public CDBindMethods
{
    typedef CDBindMethods super;

public:
    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const;
         
protected:
    CDBindMethodsSimple(VARTYPE vt, DWORD dwTransfer = 0)
        : super()   {_vt = vt; _dwTransfer=dwTransfer; }
    ~CDBindMethodsSimple()  {}
    
    virtual DBIND_KIND DBindKindImpl(CElement *pElem,
                                     LONG id,
                                     DBINFO *pdbi) const;

    virtual BOOL IsReadyImpl(CElement *pElem) const { return TRUE; }
    
    virtual BOOL FDataFormatAsImpl() const
        { return (_dwTransfer & DBIND_HTMLOK) != 0; }
                                     
    VARTYPE _vt;
    DWORD   _dwTransfer;

private:
    CDBindMethodsSimple();      // no implementation
};

//
//  class CDBindMethodsText
//
//  Concrete class for bound textual elements.
//  We have a static instance of this class for each combination of rw-status
//  and HTML-support required.
//
class CDBindMethodsText : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;
    
public:
    CDBindMethodsText(DWORD dwTransfer)
        : super(VT_BSTR, dwTransfer) { }
    ~CDBindMethodsText()    {}
        
    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;
                                        
    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const;

private:
    CDBindMethodsText();      // no implementation
};

extern const CDBindMethodsText DBindMethodsTextRichRO;
#endif // ndef NO_DATABINDING

#pragma INCMSG("--- End 'elemdb.hxx'")
#else
#pragma INCMSG("*** Dup 'elemdb.hxx'")
#endif
