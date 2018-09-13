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

#define CANCELLED(result)       ((result) == S_FALSE)

// CRowPosition implements IRowPosition
class CRowPosition: public CBase, public IRowPosition
{
    typedef CBase super;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRowPosition))
//    void *          operator new(size_t cb) { return MemAllocClear(cb); }
    CRowPosition ()
    {
        // 0-initialized by operator new; we'll put some assertions here
        //   double-check, although we could check all members. 
        Assert(_pRowset == NULL);
        Assert(_pRPCSink == NULL);
        Assert(_pCP == NULL);
        _dwPositionFlags = DBPOSITION_NOROW;
        _pUnkOuter = getpIUnknown();
    }

    //
    // IUnknown members
    //
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    DECLARE_AGGREGATED_IUNKNOWN(CRowPosition)

    IUnknown * PunkOuter() { return _pUnkOuter; }
    IUnknown *              _pUnkOuter;

    IUnknown * getpIUnknown() const
    {
        return (IUnknown *)(IPrivateUnknown *)this;
    }

    //
    // CBase members
    //
    virtual const CBase::CLASSDESC *GetClassDesc() const;
    void Passivate();

    //
    // IRowPosition members
    // 
    STDMETHOD(ClearRowPosition)();
    STDMETHOD(GetRowPosition)(HCHAPTER * phChapter, HROW * phRow, DBPOSITIONFLAGS * pdwPositionFlags);
    STDMETHOD(GetRowset)(REFIID riid, LPUNKNOWN *ppRowset);
    STDMETHOD(Initialize)(IUnknown * pRowset);   
    STDMETHOD(SetRowPosition)(HCHAPTER hChapter, HROW hRow, DBPOSITIONFLAGS pdwPositionFlags);

protected:
#ifdef ROWPOSITION_DELETE

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
    } _RowsetNotify;

    friend class MyRowsetNotify;

    // IRowsetLocate -- we can't do anything if this isn't available.
    IRowsetLocate *     _pRowsetLocate;

#endif  // ROWPOSITION_DELETE

    static const CBase::CLASSDESC         s_classdesc;    // IRowPosition classDesc
    static const CONNECTION_POINT_INFO    s_acpi[2];      // IRowPositionChange


    //
    // Notification stuff.
    //

    // Pointer to our sink (i.e. object that cares about our events).
    IRowPositionChange *_pRPCSink;      // IRowPositionChange sink
#   define DBEVENTPHASE_MAX ((DWORD) DBEVENTPHASE_DIDEVENT)
#   define DBREASON_MAX     ((DWORD) DBREASON_ROW_ASYNCHINSERT) // for now
    typedef unsigned char   NOTIFY_STATE;
    NOTIFY_STATE           _aNotifyInProgress[DBREASON_MAX + 1];

    // Helper function for multiplexing notifications.
    HRESULT             FireRowPositionChange(DBREASON,
                                              DBEVENTPHASE,
                                              BOOL,
                                              int *);
    NOTIFY_STATE EnterNotify(DBREASON reason, DBEVENTPHASE phase)
    {
        NOTIFY_STATE ns = _aNotifyInProgress[reason];
        Assert(reason <= DBREASON_MAX);
        Assert(phase <= DBEVENTPHASE_MAX);

        _aNotifyInProgress[reason] |= (1 << phase);
        return (ns);
    };

    void LeaveNotify(DBREASON reason, DBEVENTPHASE, NOTIFY_STATE ns)
    {
        Assert(reason <= DBREASON_MAX);
        _aNotifyInProgress[reason] = ns;
    }

#ifdef ROWPOSITITION_DELETE
    // Bookmark stuff
    HRESULT             CreateBookmarkAccessor();
    void                ClearBookmark();
    void                ReleaseBookmarkAccessor();
    IAccessor *         _pAccessor;     // IAccessor for IRowset
    HACCESSOR           _hAccessorBookmark;
    DBVECTOR            _Bookmark;      // has size & ptr members
#endif

    // Data members
    IRowset            *_pRowset;       // The rowset we're tracking
    IChapteredRowset   *_pChapRowset;   // its chapter interface.
    IConnectionPoint   *_pCP;           // So we can Unadvise when we're done,
    DWORD               _wAdviseCookie; // ditto..
    HCHAPTER            _hChapter;      // current chapter
    HROW                _hRow;          // current hRow
    DBPOSITIONFLAGS     _dwPositionFlags;   // Position flags
    boolean             _fCleared;      // state

    // helper functions
    void                ReleaseResources();
};

