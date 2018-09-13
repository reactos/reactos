//+----------------------------------------------------------------------------
//
//  Class CRowPosition
//
//  Purpose:
//      Implementation of IRowPosition, IRowPositionSource, and IRowPositionChange
//  interfaces on OLE-DB.  These may one day be part of OLE-DB, but until then we
//  wrap our own implementation.  Until then, we also provide the helper function,
//  CreateRowPositionOnRowset to conveniently create the IRowPosition object for
//  a given rowset.
//

#include <cdbase.hxx>

EXTERN_C const IID IID_IRowPosition;
EXTERN_C const IID IID_IRowPositionChange;
EXTERN_C const IID IID_IRowPositionSource;

#include <rowpos.h>

// CRowPosition implements IRowPosition
class CRowPosition: public CBase, public IRowPosition
{
    typedef CBase super;

private:    
    CRowPosition();
    void *          operator new(size_t cb) { return MemAllocClear(cb); }
    HRESULT     Init(IRowset *pRowset);

public:
    //
    // IUnknown members
    //
    DECLARE_FORMS_DELEGATING_IUNKNOWN(CRowPosition);
    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);    

    //
    // CBase members
    //
    virtual CBase::CLASSDESC *GetClassDesc() const;
    void Passivate();

    // STATIC member that creates instances of this class
    HRESULT static Create(/* [in]  */ IUnknown *punkOuter,
                          /* [in]  */ IRowset *pRowset,
                          /* [out] */ IRowPosition **ppRowPosition);

    //
    // IRowPosition members
    // 
    STDMETHODIMP    GetRowset(REFIID riid, LPUNKNOWN *ppRowset);
    STDMETHODIMP    SetRowPosition(HROW hRow);
    STDMETHODIMP    GetRowPosition(HROW *phRow);

protected:
    //
    // Class for "Row Position" Rowset Notify
    //
    class MyRowsetNotify: public IRowsetNotify
    {
        public:
            //  IUnknown members
            DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CRowPosition);

            //  IRowsetNotify members
            STDMETHODIMP OnFieldChange (IRowset *pRowset,
                                       HROW hRow,
                                       ULONG cColumns,
                                       ULONG aColumns[],
                                       DBREASON eReason,
                                       DBEVENTPHASE ePhase,
                                       BOOL fCantDeny);
            STDMETHODIMP OnRowChange (IRowset *pRowset,
                                      ULONG cRows,
                                      const HROW ahRows[],
                                      DBREASON eReason,
                                      DBEVENTPHASE ePhase,
                                      BOOL fCantDeny);
            STDMETHODIMP OnRowsetChange (IRowset *pRowset,
                                         DBREASON eReason,
                                         DBEVENTPHASE ePhase,
                                         BOOL fCantDeny);
         private:
            HROW    _hRow;          // HROW of active notification cycle
    } _RowsetNotify;

    friend class MyRowsetNotify;

    static CBase::CLASSDESC         s_classdesc;    // IRowPosition classDesc
    static CONNECTION_POINT_INFO    s_acpi[1];      // IRowPositionChange
    
    // IRowsetLocate -- we can't do anything if this isn't available.
    IRowsetLocate *     _pRowsetLocate;

    //
    // Notification stuff.
    //

    // Pointer to our sink (i.e. object that cares about our events).
    IRowPositionChange *_pRPCSink;      // IRowPositionChange sink
    // Helper function for multiplexing notifications.
    HRESULT             FireRowPositionChange(DBREASON,
                                              DBEVENTPHASE,
                                              BOOL,
                                              int *);
    // Bookmark stuff
    HRESULT             CreateBookmarkAccessor();
    void                ClearBookmark();
    void                ReleaseBookmarkAccessor();
    IAccessor *         _pAccessor;     // IAccessor for IRowset
    HACCESSOR           _hAccessorBookmark;
    DBVECTOR            _Bookmark;      // has size & ptr members

    // Data members
    IRowset            *_pRowset;       // The rowset we're tracking
    IConnectionPoint   *_pCP;           // So we can Unadvise when we're done,
    DWORD               _wAdviseCookie; // ditto..
    HROW                _hRow;          // current hRow

    // helper functions
    void                ReleaseResources();
};

