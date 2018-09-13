//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       rowbind.hxx
//
//  Contents:   
// 
//  History:
//
//  30-Jul-96   TerryLu     Creation
//
//----------------------------------------------------------------------------

#ifndef I_ROWBIND_HXX_
#define I_ROWBIND_HXX_
#pragma INCMSG("--- Beg 'rowbind.hxx'")

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

MtExtern(CRecordInstance)
MtExtern(CRecordInstance_aryPXfer_pv)

// IRowset data source.
class CRecordInstance : public CInstance
{
public:
    friend class CDataBindWalker;

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRecordInstance))

    CRecordInstance(CDataLayerCursor *pDLC, HROW hRow = 0);
    ~CRecordInstance();

#if DBG == 1
    virtual INST_TYPE Kind() const
        { return IT_ROWSET; }
#endif

    void Detach(BOOL fClear=FALSE);

    HROW GetHRow() const
        { return _hRow; }

    HRESULT OkToChangeHRow();
    HRESULT SetHRow(HROW hRow);

    CDataLayerCursor * GetDLC() const
        { return _pDLC; }
        
    // add a new XFer association
    HRESULT AddBinding(CXfer *pXfer, BOOL fDontTranfer=FALSE,
                        BOOL fTransferOK=TRUE);

    // remove one or all XFer associations with an element
    void    RemoveBinding(CElement *pElement, LONG id = -1);
    
    // Disconnect all Xfer's from CElements.
    void DeleteXfers(BOOL fClear);

    // Notifications
    HRESULT OnFieldsChanged(HROW hrow, DBORDINAL cColumns, DBORDINAL aColumns[]);
    
    DEBUG_METHODS

private:
    CDataLayerCursor           *_pDLC;
    HROW                        _hRow;
    CPtrAry<CXfer *>            _aryPXfer;

    CRecordInstance() : _aryPXfer(Mt(CRecordInstance_aryPXfer_pv)) {}     // Hide default constructor.

    NO_COPY(CRecordInstance);

    // Transfer data to the destination from the source.
    HRESULT TransferToDestination();
};

#pragma INCMSG("--- End 'rowbind.hxx'")
#else
#pragma INCMSG("*** Dup 'rowbind.hxx'")
#endif
