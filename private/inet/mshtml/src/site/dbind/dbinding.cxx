//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       dbinding.cxx
//
//  Contents:   HTML Elements Data Binding Extensions
//
//  class CDataBinder  -  knows what we are binding (column/property).
//                        Has a protocol for Setting/Getting data
//                        to/from the HTML elements.
//
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include <elemdb.hxx>
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include <mshtmlrc.h>
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include "rowbind.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_SIMPDC_H_
#define X_SIMPDC_H_
#include "simpdc.h"
#endif

DeclareTag(tagCRecordInstance, "CRecordInstance", "Row Instance");
PerfDbgTag(tagDataTransfer,   "Databinding", "Data transfer");
MtDefine(CXfer, DataBind, "CXfer")
MtDefine(CXferThunk, DataBind, "CXferThunk")
MtDefine(CHRowAccessor, DataBind, "CHRowAccessor")
MtDefine(CRecordInstance, DataBind, "CRecordInstance")
MtDefine(CRecordInstance_aryPXfer_pv, CRecordInstance, "CRecordInstance::_aryPXfer::_pv")

////////////////////////////////////////////////////////////////////////////////
//
//  CRecordInstance
//
////////////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     constructor
//
//-------------------------------------------------------------------------
CRecordInstance::CRecordInstance(CDataLayerCursor *pDLC, HROW hRow) :
    _pDLC(pDLC),
    _hRow(0),    // so that SetHRow() call won't have suprise side-effect
    _aryPXfer(Mt(CRecordInstance_aryPXfer_pv))
{
    TraceTag((tagCRecordInstance,
              "CRecordInstance::constructor() -> %p", this ));

    _pDLC->AddRef();
    IGNORE_HR(SetHRow(hRow));

    Assert(_aryPXfer.Size() == 0);

    IS_VALID(this);
}


//+------------------------------------------------------------------------
//
//  Member:     destructor
//
//-------------------------------------------------------------------------
CRecordInstance::~CRecordInstance()
{
    TraceTag((tagCRecordInstance,
              "CRecordInstance::constructor() -> %p", this ));

    IS_VALID(this);

    Assert(!_hRow                                &&
           (_aryPXfer.Size() == 0)               && "Detach not called.");

    if (_pDLC)
    {
        _pDLC->Release();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detaches RecordInstance and all Xfer's associated w/ this
//              RecordInstance.  Likewise, change all CElements pointed to
//              to no longer point back to the Xfer.
//-------------------------------------------------------------------------
void
CRecordInstance::Detach (BOOL fClear)
{
    TraceTag((tagCRecordInstance, "CRecordInstance::Detach() -> %p", this));

    IS_VALID(this);

    // DeleteXFers before discarding HROW, so that we don't try to stuff
    //  null information into all of the bound elements.
    DeleteXfers(fClear);                        // Remove all XFers.

    IGNORE_HR(SetHRow(0));                      // Release the HROW
}


//+------------------------------------------------------------------------
//
//  Member:     OkToChangeHRow
//
//  Synopsis:   See if it's OK to change my HROW association right now.
//              May validate-and-save the current site on the page.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CRecordInstance::OkToChangeHRow()
{
    TraceTag((tagCRecordInstance, "CRecordInstance::OkToChangeHRow() -> %p ()",
             this ));
             
    HRESULT hr = S_OK;
    const int cXfers = _aryPXfer.Size();
    int i;

    // it's always OK to move off the null HROW
    if (_hRow == DB_NULL_HROW)
        goto Cleanup;

    for (i=0; !hr && i<cXfers; ++i)
    {
        CXfer *pXfer = _aryPXfer[i];
        CElement *pElem;

        if (!pXfer)
            continue;

        if (pXfer->FOneWay())
            continue;
        
        pElem = pXfer->GetDestElement();


        // For Sites, we need to possibly check DISPID_VALID, and we need to
        //  save via a virtual method on the site which takes care of raising
        //  events if the site is current, cancelling posted messages, etc.
        // For elements, we do the save
        //  ourselves, because we've resisted putting a SaveDataIfChanged
        //  method on CElement becuase of VTable bloat.
        // BUGBUG: much of the logic here duplicates that found elsewhere;
        //  this redundancy will lead to maintenance problems.  (JJD)
        //  The DISPID_VALID stuff duplicates RequestYieldCurrency, but
        //  we don't call it ourselves because we support binding to arbitrary
        /// params.
        if (pElem->NeedsLayout())
        {
            LONG idBound = pXfer->IdElem();
            
            if (idBound == 0)   // BUGBUG: what's the right check to decide if
                                //  we need to the the DISPID_VALID check,
                                //  normally done by
                                //  CSite::RequestYieldCurrency()?
            {
                CElement *pElemCurrent = pElem->Doc()->_pElemCurrent;

                if (pElemCurrent->NeedsLayout() &&
                    pElemCurrent->GetElementDataBound() == pElem)
                {
                    if (!pElemCurrent->IsValid())
                    {
                        // BUGBUG   Need to display error message here.
                        //          May not be able to handle synchronous
                        //          display, so consider deferred display.
                        hr = E_FAIL;
                        continue;
                    }
                }
            }

            hr = pElem->SaveDataIfChanged(idBound, /* fLoud */ TRUE);
            if (hr == S_FALSE)
            {
                hr = S_OK;      // if user reverted to database, that's just fine
            }
        }
        else
        {
#if DBG == 1
            DBINFO dbi;

            Verify(DBIND_SINGLEVALUE == CDBindMethods::DBindKind(pElem,
                                                                 pXfer->IdElem(),
                                                                 &dbi));
#endif
        }
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     SetHRow
//
//  Synopsis:   Assign a new HRow to this Instance, releasing the old HROW
//              and addrefing the new HROW..
//
//  Arguments:  [hRow]      new HROW
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CRecordInstance::SetHRow (HROW hRow)
{
    TraceTag((tagCRecordInstance, "CRecordInstance::SetHRow() -> %p (%ul)",
             this, hRow ));

    IS_VALID(this);

    HRESULT             hr = S_OK;

    // Valid RecordInstance data?

    if (_hRow || hRow)
    {
        // AddRef the new hrow we now hold onto.
        if (hRow)
        {
            _pDLC->AddRefRows(1, &hRow);
        }

        // Release the hRow we're holding onto.
        if (_hRow)
        {
            _pDLC->ReleaseRows(1, &_hRow);
        }

        _hRow = hRow;
        IGNORE_HR(TransferToDestination ());
    }
    else
    {
        // Had better be zero already if the cookie is invalid.
        Assert(hRow == 0);
        Assert(_hRow == hRow);
    }

    RRETURN(hr);
}


HRESULT
CRecordInstance::AddBinding(CXfer *pXfer, BOOL fDontTransfer,
                                BOOL fTransferOK)
{
    HRESULT hr;

    pXfer->EnableTransfers(fTransferOK);

    hr = _aryPXfer.Append(pXfer);
    if (hr)
    {
        goto Error;
    }

    if (_hRow && !fDontTransfer)
    {
        IGNORE_HR(pXfer->TransferFromSrc());
    }

Cleanup:
    RRETURN(hr);

Error:
    pXfer->Detach();
    delete pXfer;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     RemoveBinding
//
//  Synopsis:   Remove one or all Xfers associated with this element.
//
//  Arguments:  [pElement] - element whose binding(s) are being removed
//              [id]       - either individual binding ID being removed,
//                           or else ID_DBIND_ALL, to remove all Xfers
//                           associated with the element
//
//-------------------------------------------------------------------------
void
CRecordInstance::RemoveBinding (CElement *pElement, LONG id)
{
    Assert("valid element required" && pElement);
    TraceTag((tagCRecordInstance,
              "CRecordInstance::DeleteXfers() -> %p", this));

    IS_VALID(this);

    for (int i = _aryPXfer.Size() - 1; i >= 0; i--)
    {
        if (_aryPXfer[i] && pElement==_aryPXfer[i]->GetDestElement())
        {
            if (id == ID_DBIND_ALL || id == _aryPXfer[i]->IdElem())
            {
                // Disconnect CElement from Xfer (destination no longer bound).
                _aryPXfer[i]->Detach();
                delete _aryPXfer[i];
                _aryPXfer.Delete(i);
            }
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     DeleteXfers
//
//  Synopsis:   Remove all Xfers associated with this row instance.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
void
CRecordInstance::DeleteXfers (BOOL fClear)
{
    TraceTag((tagCRecordInstance,
              "CRecordInstance::DeleteXfers() -> %p", this));

    IS_VALID(this);

    for (int i = _aryPXfer.Size() - 1; i >= 0; i--)
    {
        if (_aryPXfer[i])
        {
            // clear the element, if requested
            if (fClear)
                _aryPXfer[i]->ClearValue();

            // Disconnect CElement from Xfer (destination no longer bound).
            _aryPXfer[i]->Detach();
            delete _aryPXfer[i];
        }
    }

    // Remove the Xfer slot
    _aryPXfer.DeleteAll();
}


//+------------------------------------------------------------------------
//
//  Member:     TransferToDestination
//
//  Synopsis:   Process each Xfer associated with this RecordInstance
//              transfering data from the source to the destination.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CRecordInstance::TransferToDestination ()
{
    TraceTag((tagCRecordInstance,
              "CRecordInstance::TransferToDestination() -> %p", this));

    IS_VALID(this);

    HRESULT     hr = S_OK;
    HRESULT     hrT;
    int         iMaxIdx;

    iMaxIdx = _aryPXfer.Size() - 1;
    for (int i = 0; i <= iMaxIdx; i++)
    {
        if (!_aryPXfer[i])
            continue;

        hrT = _aryPXfer[i]->TransferFromSrc();

        // don't let one failure abort additional transfers, but
        //  return the first failure.
        if (hrT && !hr)
        {
            hr = hrT;
        }
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
// Member:      On Fields Changed (public)
//
// Synopsis:    notify my Xfer's that columns have changed
//
// Arguments:   cColumns        count of changed columns
//              aColumns        index of each changed column
//
// Returns:     S_OK

HRESULT
CRecordInstance::OnFieldsChanged(HROW hrow, DBORDINAL cColumns, DBORDINAL aColumns[])
{
    if (_pDLC->IsSameRow(hrow, _hRow))
    {
        const int cXfers = _aryPXfer.Size();
        for (int i=0; i<cXfers; ++i)
        {
            if (_aryPXfer[i])
                _aryPXfer[i]->ColumnsChanged(cColumns, aColumns);
        }
    }
    return S_OK;
}


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     IsValidObject
//
//  Synopsis:   Validation method, called by macro IS_VALID(p)
//
BOOL
CRecordInstance::IsValidObject ()
{
    Assert(this);
    Assert(_pDLC);

    return TRUE;
}


char s_achCRecordInstance[] = "CRecordInstance";

//+---------------------------------------------------------------------------
//
//  Member:     Dump
//
//  Synopsis:   Dump function, called by macro DUMP(p,dc)
//
void
CRecordInstance::Dump (CDumpContext&)
{
    IS_VALID(this);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetClassName
//
//  Synopsis:   GetClassName function. (virtual)
//
char *
CRecordInstance::GetClassName ()
{
    return s_achCRecordInstance;
}
#endif  // DBG == 1



////////////////////////////////////////////////////////////////////////////////
//
//  CXfer
//
////////////////////////////////////////////////////////////////////////////////

DeclareTag(tagCXfer, "CXfer", "CXfer Instance");


//+------------------------------------------------------------------------
//
//  Member:     CreateBinding (static)
//
//-------------------------------------------------------------------------

HRESULT
CXfer::CreateBinding(CElement *pElement,  LONG id, LPCTSTR strDataFld,
                    CDataSourceProvider *pProvider,
                    CRecordInstance *pSrcInstance,
                    CXfer **ppXfer /* = NULL */,
                    BOOL fDontTransfer /* = FALSE */)
{
    HRESULT hr = S_OK;
    DBIND_KIND dbk;
    DBINFO  dbi;
    DBSPEC dbs;
    CXferThunk *pXT = NULL;
    CXfer *pXfer = NULL;

    // Do all checks before memory allocation, for simple error handling.
    dbk = CDBindMethods::DBindKind(pElement, id, &dbi);
    if (dbk != DBIND_SINGLEVALUE && dbk != DBIND_TABLE)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // we will need to find out if DATAFORMATAS=HTML was specified
    Verify(!CDBindMethods::GetDBSpec(pElement, id, &dbs));

    // if dataformatas=localized-text is set (and is appropriate), mark the target type
    if (dbs.FLocalized() && dbi._vt == VT_BSTR)
    {
        dbi._vt.SetLocalized(TRUE);
    }
    
    // get a thunk from the provider
    if (strDataFld == NULL)
    {
        strDataFld = dbs._pStrDataFld;
    }
    pXT = pProvider->GetXT(strDataFld, dbi._vt);
    if (pXT == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // create a CXfer to govern the binding
    pXfer = new CXfer;
    if (pXfer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // initialize it
    hr = pXfer->Init(pElement, pSrcInstance, pXT, id, dbs.FHTML(), dbi._dwTransfer);

    // add it to the instance's notify list
    if (!hr)
        hr = pSrcInstance->AddBinding(pXfer, fDontTransfer);
    if (hr)
        goto Cleanup;
    
Cleanup:
    if (ppXfer)
        *ppXfer = pXfer;
    
    RRETURN1(hr, S_FALSE);
}


HRESULT
CXfer::ConnectXferToElement()
{
    HRESULT hr  = S_OK;
    DBMEMBERS *pdbm;

    if (!_pDestElem)
        goto Cleanup;

    hr  = _pDestElem->EnsureDBMembers();
    if (hr)
        goto Cleanup;

    pdbm = _pDestElem->GetDBMembers();

    hr = pdbm->GetDataBindingEvents()->AddXfer(this);

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Init
//
//-------------------------------------------------------------------------

HRESULT
CXfer::Init(CElement *pElem, CInstance *pSrcInstance, CXferThunk *pXT,
                LONG id, BOOL fHTML, DWORD dwTransfer)
{
    HRESULT hr;

    _pDestElem = pElem;
    _pSrcInstance = pSrcInstance;
    _pXT = pXT;
    _idElem = id;
    _fHTMLSpecified = fHTML;
    _dwTransfer = dwTransfer;

    _pXT->AddRef();
    hr = ConnectXferToElement();

    IS_VALID(this);
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     destructor
//
//-------------------------------------------------------------------------
CXfer::~CXfer ()
{
    TraceTag((tagCXfer, "CXfer::destructor() -> %p", this));

    IS_VALID(this);

    Assert(!_pDestElem && !_pSrcInstance && !_pXT && "Detach not called.");
}


//+------------------------------------------------------------------------
//
//  Member:     SetDestElement
//
//  Synopsis:   Change the CElement associated with this Xfer.
//
//  Arguments:  [pDestElem]     new CElement associated w/ Xfer.
//
//  Returns:    none
//
//-------------------------------------------------------------------------
HRESULT
CXfer::SetDestElement (CElement *pDestElem)
{
    TraceTag((tagCXfer, "CXfer::SetDestElement() -> %p (%p)", this, pDestElem));

    IS_VALID(this);

    DisonnectXferToElement();

    _pDestElem = pDestElem;

    return ConnectXferToElement();
}


//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detach the Xfer from the CElement.
//
//  Arguments:  none
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CXfer::Detach ()
{
    TraceTag((tagCXfer, "CXfer::Detach() -> %p", this));

    IS_VALID(this);

    DisonnectXferToElement();

    _pSrcInstance = NULL;
    ClearInterface(&_pXT);
}


//+------------------------------------------------------------------------
//
//  Member:     DisonnectXferToElement
//
//  Synopsis:   Disconnect CElement and Xfer connection to each other.
//
//  Arguments:  none
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CXfer::DisonnectXferToElement ()
{
    TraceTag((tagCXfer, "CXfer::Detach() -> %p", this));

    IS_VALID(this);

    if (_pDestElem)
    {
        // Sever CElement connection to an XFer.
        _pDestElem->GetDBMembers()->GetDataBindingEvents()->_aryXfer.DeleteByValue(this);

        // Sever Xfer connections to a CElement.
        _pDestElem = NULL;
    }
}


HRESULT
CXfer::TransferToSrc (DWORD *pdwStatus)
{
    HRESULT hr = S_OK;
    VARIANT     var;
    VOID        *pData;
    BOOL fWasTransferring;

    Assert(*pdwStatus == DBSTATUS_S_OK);    // caller must initialize
    
    // Note that it is up to callers to avoid calling us when _fTransferringToDest.
    // This division of responsibility is necessary because we sometime decide to
    //  save synchronously, but don't actually do the save asynchronously.
    if (_pDestElem == NULL || _pSrcInstance == NULL || _pXT == NULL
            || _fTransferringToDest)
    {
        Assert(FALSE);
        hr = E_UNEXPECTED;
        goto Done;
    }

    if (_fDontTransfer)
    {
        hr = S_OK;
        goto Done;
    }

    // read-only binding always "succeeds"
    if (_dwTransfer & DBIND_ONEWAY)
        goto Done;

    fWasTransferring = _fTransferringToSrc;
    _fTransferringToSrc = TRUE;

    // prepare the variant which will receive the element's value.
    pData = _pXT->PvInitVar(&var);

    // get the value from the element
    hr = THR(_pDestElem->GetDBindMethods()->BoundValueFromElement(
                _pDestElem,
                IdElem(),
                FHTML(),
                pData ) );

    // set the status to indicate if it's null
    if (pData == &var.bstrVal && V_VT(&var) == VT_BSTR && V_BSTR(&var) == NULL)
    {
        *pdwStatus = DBSTATUS_S_ISNULL;
    }
    
    // transfer the data to the Rowset
    if (!hr)
    {
        hr = THR(_pXT->ValueToSrc(_pSrcInstance, pData, pdwStatus, _pDestElem));
    }
    
    _fTransferringToSrc = fWasTransferring;
    if (hr)
        goto Cleanup;

    // store the database value back into the element (so we get the
    // provider's converions, if any)
    IGNORE_HR(TransferFromSrc());

Cleanup:
    VariantClear(&var);
Done:
    RRETURN(hr);
}
    

HRESULT
CXfer::TransferFromSrc ()
{
    HRESULT         hr = S_OK;
    BOOL            fWasTransferring;
    WHEN_DBG(ULONG ulRefElement;)

    if (_fDontTransfer || _fTransferringToSrc)
    {
        goto Cleanup;
    }

    if (_pDestElem == NULL || _pSrcInstance == NULL || _pXT == NULL)
    {
        Assert(FALSE);
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    _pDestElem->AddRef();           // keep element alive while we change it
    
    fWasTransferring = _fTransferringToDest;
    _fTransferringToDest = TRUE;

    if (_dwTransfer & DBIND_NOTIFYONLY) // element just wants notification, not data
    {
        hr = THR(_pDestElem->GetDBindMethods()->InstanceChanged(_pDestElem, _pSrcInstance));
    }

    else                        // element wants data
    {
        HRESULT hrT;
        VARIANT     var;
        VOID       *pData;

        pData = _pXT->PvInitVar(&var);

        PerfDbgLog1(tagDataTransfer, _pDestElem, "+DataTransfer to %ls",
                    _pDestElem->TagName());
        
        hr = THR(_pXT->ValueFromSrc(_pSrcInstance, pData, _pDestElem));

        PerfDbgLog(tagDataTransfer, _pDestElem, "got DB value, sending to element");

        // even on failure to fetch data from source, we want to put
        //  SOMETHING -- our empty-equivalent data -- into the element
        hrT = THR(_pDestElem->GetDBindMethods()->BoundValueToElement(
                    _pDestElem,
                    IdElem(),
                    FHTML(),
                    pData ) );
        if (!hr)
        {
            hr = hrT;
        }

        PerfDbgLog(tagDataTransfer, _pDestElem, "-DataTransfer");

        VariantClear(&var);

        // Databinding should not save undo information
        _pDestElem->Doc()->FlushUndoData();
    }
    
    // remember whether or not we achieved success
    _fError = (hr != S_OK);

    _fTransferringToDest = fWasTransferring;
    WHEN_DBG(ulRefElement = ) _pDestElem->Release();
    AssertSz(ulRefElement>0, "element unexpectedly destroyed during data transfer");

Cleanup:
    RRETURN(hr);
}


HRESULT
CXfer::ClearValue()
{
    HRESULT hr = S_OK;
    VARIANT var;
    VOID *pNull;
    
    Assert(_pDestElem && _pXT);
    
    if (_dwTransfer & DBIND_NOTIFYONLY) // element just wants notification, not data
        goto Cleanup;
    
    // set up null value
    pNull = _pXT->PvInitVar(&var);
    
    // transfer the null value
    hr = THR( _pDestElem->GetDBindMethods()->BoundValueToElement(
                _pDestElem,
                IdElem(),
                FHTML(),
                pNull ) );

    // Databinding should not save undo information
    _pDestElem->Doc()->FlushUndoData();

Cleanup:
    RRETURN(hr);
}


HRESULT
CXfer::CompareWithSrc ()
{
    HRESULT         hr = S_OK;
    
    VARIANT     var;
    VOID        *pData;
    
    VARIANT     varElem;
    VOID        *pElemData;

    // if we are comparing and we had a previous error, we do nothing and
    //  report a match with S_OK
    if (_fError)
        goto Done;

    // if we are comparing, and we are in the act of transferring back
    //  to the element, then we want to do nothing and return S_OK.
    if (_fTransferringToDest)
        goto Done;
    
    if (_pDestElem == NULL || _pSrcInstance == NULL || _pXT == NULL)
    {
        Assert(FALSE);
        hr = E_UNEXPECTED;
        goto Done;
    }

    Assert(CTypeCoerce::FVariantType(_vtElem.BaseType()));
    
    // compare on a read-only binding always returns true
    if (_dwTransfer & DBIND_ONEWAY)
        goto Done;
   
    pData = _pXT->PvInitVar(&var);    
    pElemData = _pXT->PvInitVar(&varElem);

    // get element's value
    hr = _pDestElem->GetDBindMethods()->BoundValueFromElement(_pDestElem,
                                                           IdElem(),
                                                           FHTML(),
                                                           pElemData);
    if (hr)
    {
        // S_FALSE is an expected return, which means the element is not in
        // a state where it has reasonable data to compare to.  An example
        // of this is a radio button with no buttons pressed, a state that
        // can be achieved if the databinding value matched no buttons.
        // We must suppress the S_FALSE here because our callers become upset
        // on any non-S_OK hr's.
        if (hr==S_FALSE)
        {
            hr = S_OK;
        }
        goto Cleanup;
    }

    // get database value
    hr = _pXT->ValueFromSrc(_pSrcInstance, pData, _pDestElem);
    if (hr)
    {
        if (hr == DB_E_DELETEDROW)
        {
            // moving away from a deleted record - the move-away code wants
            // to compare the current value with the (unavailable) database
            // value.  This condition would never occur if our data sources
            // gave us good deleted-rows notifications, but until ADC does,
            // we have to take some action.  We report that any value
            // matches the contents of a deleted-row, so that we won't
            // trigger save logic.
            hr = S_OK;
        }
        goto Cleanup;
    }

    // compare the values
    hr = CTypeCoerce::IsEqual(VT_VARIANT,
                              &varElem,
                              &var );

    // if we we have a mis-match, and we know that the source is read-only,
    //  let's report E_ACCESSDENIED now.
    if (hr == S_FALSE && !_pXT->IsWritable(_pSrcInstance))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

Cleanup:
    VariantClear(&var);
    VariantClear(&varElem);
Done:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
// Member:      Columns Changed (public)
//
// Synopsis:    if my column has changed, get the new data
//
// Arguments:   cColumns    count of changed columns
//              aColumns    index of each changed column
//
// Returns:     S_OK

HRESULT
CXfer::ColumnsChanged(DBORDINAL cColumns, DBORDINAL aColumns[])
{
    HRESULT hr = S_OK;
    HRESULT hrTemp;

    if (_pXT)
    {
        _pDestElem->AddRef();       // keep element alive while we change it
        for (ULONG k=0; k<cColumns; ++k)
        {
            if (_pXT->IdSrc() == aColumns[k])
            {
                hrTemp = TransferFromSrc();
                if (hrTemp)                     // return last error (arbitrarily)
                    hr = hrTemp;
            }
        }
        _pDestElem->Release();
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     ShowDiscardMessage
//
//  Synopsis:   Build and show the right message to prompt the user
//              if she wants changed discarded, and return results.
//
//  Arguments:  hrError  - error code of the failure
//              dwStatus - for DB_E_ERRORSOCCURRED only, status code
//              pnResult - where to put the value return by MessageBox.
//                         It is up to caller to initialize with sensible
//                         value in case error occurs.
//
//  Returns:    HRESULT
//

HRESULT
CXfer::ShowDiscardMessage (HRESULT hrError, DWORD dwStatus, int *pnResult)
{
    TCHAR   achBufError[FORMS_BUFLEN];
    int     errorDescr = 0;
    DBSPEC  dbs = {NULL, NULL, NULL};
    HRESULT hr;

    Verify(!CDBindMethods::GetDBSpec(PElemOwner(), IdElem(), &dbs, DBIND_SINGLEFILTER));
    Assert(dbs._pStrDataFld);

    switch (hrError)
    {
    case OLE_E_CANTCONVERT:
    case DB_E_UNSUPPORTEDCONVERSION:
    case DB_E_CANTCONVERTVALUE:
        errorDescr = IDS_EE_DB_COERCE;
        break;
        
    case DB_E_DELETEDROW:
        errorDescr = IDS_EE_DB_DELROW;
        break;
        
    case E_ACCESSDENIED:
    case DB_E_READONLYACCESSOR:
    case DB_SEC_E_PERMISSIONDENIED:
        errorDescr = IDS_EE_DB_READ;
        break;
        
    case DB_E_INTEGRITYVIOLATION:
    case DB_E_SCHEMAVIOLATION:
        errorDescr = IDS_EE_DB_SCHEMA;
        break;

    case DB_E_CANCELED:
        errorDescr = IDS_EE_DB_CANCELED;
        break;

    case DB_E_ERRORSOCCURRED:
        switch (dwStatus)
        {
        case DBSTATUS_E_CANTCONVERTVALUE:
        case DBSTATUS_E_DATAOVERFLOW:
        case DBSTATUS_E_SIGNMISMATCH:
            errorDescr = IDS_EE_DB_COERCE;
            break;

        case DBSTATUS_E_PERMISSIONDENIED:
            errorDescr = IDS_EE_DB_READ;
            break;

        case DBSTATUS_E_SCHEMAVIOLATION:
        case DBSTATUS_E_INTEGRITYVIOLATION:
            errorDescr = IDS_EE_DB_SCHEMA;
            break;

        default:
            errorDescr = IDS_EE_DB_OTHER_STATUS;
            break;
        }
        break;
        
    }

    if (errorDescr == 0)
    {
        hr = THR(GetErrorText(hrError, achBufError, ARRAY_SIZE(achBufError)));
    }
    else
    {
        hr = THR(Format(0,
                        achBufError,
                        ARRAY_SIZE(achBufError),
                        MAKEINTRESOURCE(errorDescr),
                        hrError,
                        dwStatus));
    }
    if (hr)
        goto Cleanup;

    hr = THR(_pDestElem->ShowMessage(pnResult,
                                MB_YESNO|MB_ICONEXCLAMATION|/*MB_HELP|*/MB_DEFBUTTON1|
                                MB_APPLMODAL,
                                0,             // help context..
                                IDS_MSG_DB_CANTSAVE,
                                dbs._pStrDataFld,
                                achBufError));
Cleanup:
    RRETURN(hr);
}

#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     IsValidObject
//
//  Synopsis:   Validation method, called by macro IS_VALID(p)
//
BOOL
CXfer::IsValidObject ()
{
    Assert(this);

    return TRUE;
}


char s_achCXfer[] = "CXfer";

//+---------------------------------------------------------------------------
//
//  Member:     Dump
//
//  Synopsis:   Dump function, called by macro DUMP(p,dc)
//
void
CXfer::Dump (CDumpContext&)
{
    IS_VALID(this);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetClassName
//
//  Synopsis:   GetClassName function. (virtual)
//
char *
CXfer::GetClassName ()
{
    return s_achCXfer;
}
#endif  // DBG == 1


////////////////////////////////////////////////////////////////////////////////
//
//  CDataBindingEvents
//
//  This now-poorly named class contains all databinding method calls initiated
//  by the bound element.
//
////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::SaveIfChanged
//
//  Synopsis:   If appropriate, save value back to data source for
//              either one binding, or all bindings.  Fires any
//              events appropriate.
//              
//
//  Arguments:  [pElement] - Source element
//              [id]       - The ID of the particular binding
//                           within pElement.  ID_DBIND_ALL indicates
//                           all bindings.
//              [fLoud]    - Should a failure bring an alert up
//                           on the user's screen?
//              [fForceIsCurrent]    - Treat pElement as if it were the
//                           current focus element, even if it isn't
//
//  Returns:    S_OK       - Success (or no actual binding)
//              S_FALSE    - user reverted value to database version
//              E_ABORT    - user cancelled update
//              E_*        - failure
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::SaveIfChanged(CElement * pElement, LONG id, BOOL fLoud,
                                    BOOL fForceIsCurrent)
{
    CXfer     **ppXfer;
    int         i;
    DWORD       dwCookie;
    
    BOOL        fWasInSave = _fInSave;
    
    _fInSave = TRUE;                // recursion detection
    
    CDoc       *pDoc = pElement->Doc();
    BOOL        fCurrent = fForceIsCurrent ||
                            (pElement->NeedsLayout() &&
                            pDoc->_pElemCurrent == pElement);

    BOOL        fBeforeFired = FALSE;
    BOOL        fErrorFired = FALSE;
    BOOL        fAlerted = FALSE;
    BOOL        fRevert = FALSE;    // initialization not really needed
    BOOL        fDoDefault = TRUE;  // initialization not really needed

    // BUGBUG: are Lock and BlockNewUndoUnits really needed on save TO src?
    CDoc::CLock Lock(pDoc, SERVERLOCK_TRANSITION);
    pElement->AddRef();             // stabilize pointer
    HRESULT hr = pDoc->BlockNewUndoUnits(&dwCookie);
    
    if (hr)
        goto Cleanup;

    // set up loop variables to either do one ID, or else all of them
    if (id == ID_DBIND_ALL)
    {
        ppXfer = _aryXfer;
        i = _aryXfer.Size();
    }
    else
    {
        ppXfer = FindPXfer(pElement, id);

        // if this ID isn't bound just report sucess
        if (!ppXfer)
            goto Unblock;

        i = 1;
    }

    for (/* i, ppXfer initialized above */ ;
         i > 0;
         i--, ppXfer++)
    {
        CXfer *pXfer = *ppXfer;
        DWORD dwStatus = DBSTATUS_S_OK;

        Assert(!hr);    // we quit on failure
        
        hr = pXfer->CompareWithSrc();   //  includes IsWritable check
        if (!hr)
            continue;

        // even if we know that update will fail (that is, even if we got
        //  an error code from the comparison), we might still fire
        //  onbeforeupdate.
        
        if (!fBeforeFired && (fCurrent && !fWasInSave))
        {
            // either set fBeforeFired to TRUE, or bail out leaving
            //  it false, which will cause AfterUpdate to not be fired.
            fBeforeFired = pElement->Fire_onbeforeupdate();
            if (!fBeforeFired)
            {
                hr = E_ABORT;
                goto Unblock;
            }

            // don't crash if bindings yanked from underneath us by script
            if (ppXfer < (CXfer **) _aryXfer
                || ppXfer >= &_aryXfer[_aryXfer.Size()]
                || *ppXfer != pXfer)
            {
                goto Unblock;
            }
        }

        // try to write the new value, if it looks like writing is allowed
        if (!FAILED(hr))
        {
            hr = pXfer->TransferToSrc(&dwStatus);
            if (!hr)
                continue;
        }

        // something went wrong.  We'll either fail now, or else
        //  we'll consider what an <OnErrorUpdate and alert> might
        //  indicate.
        if (fLoud || (fCurrent && !fWasInSave))
        {
            if (!fErrorFired)
            {
                // in the unlikely case that we're nested inside another
                //  onerrorUpdate invocation, bail out rather than risk
                //  infinite recursion.
                if (_fInErrorUpdate)
                    goto Unblock;
                    
                _fInErrorUpdate = TRUE;
                fDoDefault = pElement->Fire_onerrorupdate();
                _fInErrorUpdate = FALSE;
                
                fErrorFired = TRUE;
                
                // don't crash if bindings yanked from underneath us by script
                if (ppXfer < (CXfer **) _aryXfer
                    || ppXfer >= &_aryXfer[_aryXfer.Size()]
                    || *ppXfer != pXfer)
                {
                    goto Unblock;
                }

                if (!pXfer->CompareWithSrc())
                {
                    hr = S_OK;
                    continue;
                }
            }

            Assert(hr);     // we're still working with failure
            
            if (fDoDefault && fLoud)
            {
                if (!fAlerted)
                {
                    int nResult = IDYES; // insurance that on failure, we'll revert
                        
                    // revert value if ShowDiscardMessage fails, or if
                    // user picks revert option
                    fRevert =  (pXfer->ShowDiscardMessage (hr, dwStatus, &nResult)
                                || IDYES == nResult );
                    fAlerted = TRUE;
                }


                if (fRevert)
                {
                    hr = S_FALSE;
                    IGNORE_HR(pXfer->TransferFromSrc());
                    continue;
                }
            }
        }

        // nobody fixed our failure
        Assert(hr);
        goto Unblock;
    }

Unblock:
    if (fBeforeFired && !fErrorFired)
    {
        pElement->Fire_onafterupdate();
    }
    pDoc->UnblockNewUndoUnits(dwCookie);

Cleanup:
    _fInSave = fWasInSave;
    pElement->Release();        // remove stabilizing reference
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::TransferFromSrc
//
//  Synopsis:   Copy the database value back into the element.
//              Used to revert changes the user may have made.
//              
//
//  Arguments:  [pElement] - Destination element
//              [id]       - The ID for the particular binding
//                           destination within pElement.
//
//  Returns:    S_OK       - Success (or no actual binding)
//              E_*        - failure
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::TransferFromSrc(CElement *pElement, LONG id)
{
    HRESULT     hr = S_OK;
    CXfer       **ppXfer;

    if (id == ID_DBIND_ALL)
    {
        HRESULT hrt;
        int i;
        for (i=0, ppXfer=&_aryXfer[0]; i<_aryXfer.Size(); ++i, ++ppXfer)
        {
            Assert(pElement == (*ppXfer)->GetDestElement());
            hrt = (*ppXfer)->TransferFromSrc();
            if (!hr)
                hr = hrt;       // remember first error
        }
    }
    else
    {
        ppXfer = FindPXfer(pElement, id);
        if (!ppXfer)
            goto Cleanup;

        Assert(pElement == (*ppXfer)->GetDestElement());
        hr = (*ppXfer)->TransferFromSrc();
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::ValueFromSrc
//
//  Synopsis:   Fetch the database value which would be put into the
//              element if a full transfer were accomplished.  Up to
//              caller to initialize receiving memory, and make sure
//              it is of right size to accommodate the element's
//              data type.
//              
//
//  Arguments:  [pElement] - Destination element
//              [id]       - The ID for the particular binding
//                           destination within pElement.
//              [lpv]      - where to put the value
//
//  Returns:    S_OK       - Success (or no actual binding)
//              E_*        - failure
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::ValueFromSrc(CElement *pElement, LONG id, LPVOID lpv)
{
    HRESULT     hr = S_OK;
    CXfer       **ppXfer;

    ppXfer = FindPXfer(pElement, id);
    if (!ppXfer)
        goto Cleanup;

    Assert(pElement == (*ppXfer)->GetDestElement());

    hr = (*ppXfer)->ValueFromSrc(lpv);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::CheckSrcWritable
//
//  Synopsis:   Check whether not we could propagate a change from
//              the given ID of the given element to its data source.
//              
//
//  Arguments:  [pElement] - Destination element
//              [id]       - The ID for the particular binding
//                           destination of pElement.
//
//  Returns:    S_OK       - Not bound or OK to write
//              S_FALSE    - Write-to-source is suppressed because we
//                           are in act of propgating value from source
//                           to destination, and don't want to ping-pong
//                           the value between source and destination.
//              E_*        - error
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::CheckSrcWritable(CElement *pElement, LONG id)
{
    HRESULT     hr = S_OK;
    CXfer       **ppXfer;
    CXfer       *pXfer;
    CXferThunk  *pXT;

    ppXfer = FindPXfer(pElement, id);

    if (!ppXfer)
        goto Cleanup;

    pXfer = *ppXfer;

    Assert(pElement == pXfer->GetDestElement());

    if (pXfer->FTransferringToDest())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

        
    pXT = pXfer->GetXT();

    if (!pXT->IsWritable(pXfer->GetSrcInstance()))
    {
        hr = E_ACCESSDENIED;    // BUGBUG: is this the right code?
        goto Cleanup;
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::CompareWithSrc
//
//  Synopsis:   Check whether not the data source and element
//              contents hold equal values.
//              
//
//  Arguments:  [pElement] - Destination element tempate
//              [id]       - The ID for the particular binding
//                           destination of pElement.
//
//  Returns:    S_OK       - Values match.
//              S_FALSE    - Non-match.
//              E_*        - error
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::CompareWithSrc(CElement *pElement, LONG id)
{
    HRESULT     hr = S_OK;
    CXfer       **ppXfer;
    
    ppXfer = FindPXfer(pElement, id);

    if (!ppXfer)
        goto Cleanup;

    Assert(pElement == (*ppXfer)->GetDestElement());

    hr = (*ppXfer)->CompareWithSrc();

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::Detach
//
//  Synopsis:   Detach the element from the event sink.
//
//  Arguments:  [pElement] - Source element
//              [id]       - The ID for the particular binding
//                           source within pElement.  Or ID_DBIND_ALL
//                           to indicate all bindings
//
//  Returns:    S_OK       - Success (or no actual binding)
//              E_*        - failure
//
//
//---------------------------------------------------------------

void
CDataBindingEvents::DetachBinding(CElement *pElement, LONG id)
{
    int i;
    CXfer **ppXfer;

    // we loop ppXfer backwards to avoid unnecessary memory copying as
    //  we delete from _aryXfer
    if (id == ID_DBIND_ALL)
    {
        i = _aryXfer.Size();
        ppXfer = &_aryXfer[i-1];
    }
    else
    {
        ClearValue(pElement, id);
        ppXfer = FindPXfer(pElement, id);
        i =  ppXfer ? 1 : 0;
    }

    for ( ; i> 0; i--, ppXfer--)
    {
        CRecordInstance *pRecInstance = DYNCAST(CRecordInstance, (*ppXfer)->GetSrcInstance());
        LONG idElem = (*ppXfer)->IdElem();

        Assert(pElement == (*ppXfer)->GetDestElement());

        _aryXfer.Delete(ppXfer-(CXfer **)_aryXfer);
        pRecInstance->RemoveBinding(pElement, idElem);  // deletes Xfer
    }
}


//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::ClearValue
//
//  Synopsis:   Clear the value in the element to a null-equivalent.
//
//  Arguments:  [pElement] - Source element
//              [id]       - The ID for the particular binding
//                           source within pElement.  ID_DBIND_ALL
//                           is not allowed.
//
//  Returns:    S_OK       - Success (or no actual binding)
//              E_*        - failure
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::ClearValue(CElement *pElement, LONG id)
{
    Assert(id != ID_DBIND_ALL);

    HRESULT hr = S_OK;
    CXfer **ppXfer = FindPXfer(pElement, id);

    if (pElement->Doc()->TestLock(FORMLOCK_LOADING | FORMLOCK_UNLOADING))
        goto Cleanup;

    if (ppXfer == NULL)
        goto Cleanup;

    Assert(pElement == (*ppXfer)->GetDestElement());
    hr = (*ppXfer)->ClearValue();

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::EnableTransfers
//
//  Synopsis:   Enable data transfer for this binding.  Called when an
//              object becomes ready.
//
//  Arguments:  [pElement] - Source element
//              [id]       - The ID for the particular binding.
//
//  Returns:    S_OK       - Success (or no actual binding)
//              E_*        - failure
//
//
//---------------------------------------------------------------

HRESULT
CDataBindingEvents::EnableTransfers(CElement *pElement, LONG id, BOOL fTransferOK)
{
    HRESULT     hr = S_OK;
    CXfer       **ppXfer;
    int         i;

    // initialize variables for loop below
    if (id == ID_DBIND_ALL)
    {
        i = _aryXfer.Size();
        ppXfer = _aryXfer;
    }
    else
    {
        i = 1;
        ppXfer = FindPXfer(pElement, id);
    }

    if (ppXfer == NULL)
        goto Cleanup;
    
    for (/* i, ppXfer */; i>0; --i, ++ppXfer)
    {
        Assert(pElement == (*ppXfer)->GetDestElement());
        (*ppXfer)->EnableTransfers(fTransferOK);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDataBindingEvents::FindPXfer, private
//
//  Synopsis:   Find the slot in _aryXfer which holds binding to the
//              given ID,if any.
//              
//
//  Arguments:  [pElement] - Source element
//              [id]       - The ID for the particular binding
//                           source within pElement.
//
//  Returns:    ppXfer or NULL
//
//---------------------------------------------------------------
CXfer **
CDataBindingEvents::FindPXfer(CElement *pElement, LONG id)
{
    CXfer **ppXfer;
    int    i;

    Assert(id != ID_DBIND_ALL);
    
    for (i = _aryXfer.Size(), ppXfer = _aryXfer;
         i > 0;
         i--, ppXfer++)
    {
        if ((*ppXfer)->IdElem() == id)
        {
            return ppXfer;
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CXferThunk
//
////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------
//
//  Member:     CXferThunk::Create, static
//
//  Synopsis:   Create and init CXferThunk with all information necessary
//              for eventual resolution of the source reference, and
//              instantiation of mechanism for accomplishing transfer.
//
//
//  Arguments:  [pElement] - Destination element tempate
//              [id]       - binding ID in the destination element
//              [pdlas]    - data layer accessor source to be used for
//                           source resolution.  BUGBUG: consider
//                           removing in the future
//              [ppXT]     - Where to return a pointer to the XferThunk
//
//  Returns:    S_OK       - XferThunk successfully created
//              S_FALSE    - no XferThunk needed for this element
//              E_*        - error
//
//
//---------------------------------------------------------------

HRESULT
CXferThunk::Create(CDataLayerCursor *pDLC, LPCTSTR strField, CVarType vtDest,
                    ISimpleDataConverter *pSDC, CXferThunk **ppXT)
{
    CXferThunk *pXT = NULL;
    CTypeCoerce::CAN_TYPE cantDest;
    HRESULT hr;
    DBORDINAL uColID;

    // initilize in case of failure
    *ppXT = NULL;

    hr = CTypeCoerce::CanonicalizeType(vtDest.BaseType(), cantDest);
    if (hr)
        goto Cleanup;

    // allocate the memory
    pXT = new CXferThunk();
    if (pXT == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // initialize with one reference
    pXT->_ulRefCount = 1;

    hr = THR(pDLC->GetColumnNumberFromName(strField, uColID));
    if (hr)
        goto Cleanup;

    pXT->_idSrc = uColID;

    pXT->_vtDest = vtDest;
    pXT->_cantDest = cantDest;
    Assert(pXT->_fAccessorError == FALSE);
    Assert(!pXT->_pAccessor);
    Assert(pXT->_dbtSrc == VT_EMPTY);
    pXT->_fWriteSrc = TRUE;
    pXT->_pSDC = pSDC;

Cleanup:
    if (!hr)
    {
        *ppXT = pXT;
    }
    else
    {
        delete pXT;
    }
    
    RRETURN(hr);
}


HRESULT
CXferThunk::QueryInterface (REFIID riid, LPVOID * ppv)
{
    HRESULT hr;

    Assert(ppv);

    if (riid == IID_IUnknown)
    {
        *ppv = this;
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}


ULONG
CXferThunk::AddRef (void)
{
    return ++_ulRefCount;
}


ULONG
CXferThunk::Release (void)
{

    if (--_ulRefCount == 0)
    {
        _ulRefCount = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }

    return _ulRefCount;
}


CXferThunk::~CXferThunk()
{
    if (_pAccessor)
    {
        _pAccessor->Detach();
        _pAccessor = NULL;
    }
}

//+---------------------------------------------------------------
//
//  Member:     CXferThunk::EnsureAccessor, private
//
//  Synopsis:   If we haven't ever tried to get an accessor before, try
//              to get one now.  If it fails for a non-recoverable reason
//              set _fAccessorError flag so we don't keep trying.
//
//  Arguments:  [pSrc]     - Source instance.  Concrete type must
//                           match that expected my the CXferThunk.
//
//  Returns:    S_OK:       We got an accessor.
//              S_FALSE:    We couldn't get an accessor due to a persistent
//                          error (e.g. the column name referenced in the
//                          DATAFLD doesn't exist).
//              E_*         Error of some sort 
//
//                         * * * * WARNING * * * * 
//  NOTE THAT THIS FUNCTION CAN RETURN S_OK AND YET STILL LEAVE _pAccessor NULL!
//
//---------------------------------------------------------------
HRESULT
CXferThunk::EnsureAccessor(CInstance *pSrc)
{
    HRESULT hr = S_OK;

    if (!_pAccessor && !_fAccessorError)
    {
        // BUGBUG: for now, we assume that we will be fetching
        //  from HROWs.  When we add IDispatch-based sources,
        //  we'll need to do better.
        CHRowAccessor *pHRowAccessor = new CHRowAccessor;
        if (!pHRowAccessor)
        {
            hr = E_OUTOFMEMORY;            
            goto Cleanup;
        }

        hr = pHRowAccessor->Init(this, DYNCAST(CRecordInstance, pSrc)->GetDLC());
        if (hr)
        {
            delete pHRowAccessor;
            // BUGBUG:: We really should report this error someday, someway,
            // and differentiate from recoverable and non-recoverable errors.
            // For now, we presume all accessor creation failures are non-recov.
            hr = S_OK;                  // look like we succeeded
            _fAccessorError = TRUE;
        }
        else
        {
            _fAccessorError = FALSE;
            _pAccessor = pHRowAccessor;
        }
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CXferThunk::ValueFromSrc, public
//
//  Synopsis:   Fetch a bound value suiable for stuffing into a destination
//              element.  It is up to caller to ensure that receiving
//              memory buffer is suitable for receiving the element's
//              desired type, and to whatever memory initialization is
//              required.
//
//  Arguments:  [pSrc]     - Source instance.  Concrete type must
//                           match that expected my the CXferThunk.
//              [lpv]      - destination memory buffer
//
//  Returns:    S_OK:       All OK.
//              E_*         Error of some sort
//
//---------------------------------------------------------------

HRESULT
CXferThunk::ValueFromSrc (CInstance *pSrc, LPVOID lpv, CElement *pElem)
{
    HRESULT hr;

    hr = EnsureAccessor(pSrc);
    if (!_pAccessor)
        goto Cleanup;

    hr = THR(_pAccessor->ValueFromSrc(this, pSrc, lpv, pElem));
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CXferThunk::TransferToSrc, public
//
//  Synopsis:   Accomplish a transfer from a destination element
//              back to its source instance.
//
//  Arguments:  [pSrc]     - Source instance.  Concrete type must
//                           match that expected my the CXferThunk.
//              [pCEDest]  - destination element instance
//              [pdwStatus] - used to pass pack status information
//                            from OLE DB
//
//---------------------------------------------------------------

HRESULT
CXferThunk::ValueToSrc(CInstance *pSrc, LPVOID lpv, DWORD *pdwStatus, CElement *pElem)
{
    HRESULT hr;

    hr = EnsureAccessor(pSrc);
    if (!_pAccessor)
        goto Cleanup;

    hr = THR(_pAccessor->ValueToSrc(this, pSrc, lpv, pdwStatus, pElem));
    
Cleanup:
    RRETURN(hr);
}

BOOL
CXferThunk::IsWritable(CInstance *pSrc)
{
    BOOL fRet = FALSE;

    IGNORE_HR(EnsureAccessor(pSrc));
    if (_pAccessor)
    {
        if (!_fWriteSrc)
        {
            goto Cleanup;
        }
        fRet = _pAccessor->IsSrcWritable(pSrc);
    }

Cleanup:
    return fRet;
}

//+---------------------------------------------------------------
//
//  Member:     CXferThunk::PvInitVar, public
//
//  Synopsis:   Initialize variant based on the type desired by an
//              XferThunk's element, and return a pointer either to
//              beginning of the variant or to the data-portion,
//              depending on what the element wants.
//
//  Arguments:  [pvar]     - variant to be initalized
//
//  Returns:    Pointer to beginning or data portion of variant.
//
//  Notes:
//      Arranges that:
//      1) If the variant isn't further manipulated, VariantClear
//         will be harmless.
//      2) If we are doing some cheating where the data fetch is going
//         simulate a variant, or we are otherwise going to rely
//         on VariantClear to release a value, the VT will have been
//         set correctly
//      3) If things go wrong, we'll pass an appropriate empty-equivalent
//         value
//
//---------------------------------------------------------------

VOID *
CXferThunk::PvInitVar(VARIANT *pvar, CVarType vt)
{
    VOID *pv = pvar;
    
#ifdef UNIX
    CY_INT64(&pvar->cyVal) = 0;
#else
    pvar->cyVal.int64 = 0;
#endif
    if (vt.BaseType() == VT_VARIANT)
    {
        pvar->vt = VT_EMPTY;
    }
    else
    {
        pvar->vt = vt.BaseType();
        if (!vt.FInVariant())
        {
            pv = &pvar->iVal;
        }
    }
    return pv;
}

VOID *
CXferThunk::PvInitVar(VARIANT *pvar) const
{
    return PvInitVar(pvar, _vtDest);
}

// DBData type used to fetch and set database data
// we may fetch the data into the beginning of the variant, or
// else directly into the data portion, depending on the datatype
struct DBData {
    DWORD   dwStatus;
    VARIANT var_data;
};

//+---------------------------------------------------------------
//
//  Member:     CHRowAccessor::Init, CAccessor
//
//  Synopsis:   Initialize a helper object for a CXferThunk for
//              accomplishing a transfer from HROW sources.
//
//  Arguments:  [pXT]      - The owning CXferThunk.
//
//---------------------------------------------------------------

HRESULT
CXferThunk::CHRowAccessor::Init(CXferThunk *pXT, CDataLayerCursor *pDLC)
{
    IS_VALID(pXT);

    CVarType  vtDest = pXT->_vtDest;
    DBBINDING binding;
    DBTYPE    dbType;
    HRESULT   hr;
    const CDataLayerCursor::ColumnInfo *pCI;
    size_t    oIn = 0;

    Assert(vtDest != VT_EMPTY);
    Assert(pDLC);

    hr = THR(pDLC->GetPColumnInfo(pXT->IdSrc(), &pCI));
    if (hr)
    {
        goto Cleanup;
    }

    dbType = pCI->dwType;

    // Let's plan our access and conversion strategies.  First, we figure out
    //  how to interact with the database.

    // under certain special circumstances, we don't use the provider's data
    //  type for the Rowset accessor.
    
    Assert(vtDest.BaseType() != DBTYPE_WSTR && vtDest.BaseType() != DBTYPE_STR);
    if (dbType != vtDest.BaseType())        // A type coercion is necessary.
    {
        // see if DSO wants us to use its SimpleDataConverter
        Assert(!pXT->_fUseSDC);
        if (pXT->_pSDC)
        {
            DBTYPE dbTypeTemp;
            
            switch (dbType)
            {
            // map date-like types into DBTYPE_DATE
            case DBTYPE_DATE:
            case DBTYPE_DBDATE:
            case DBTYPE_DBTIME:
            case DBTYPE_DBTIMESTAMP:
                dbTypeTemp = DBTYPE_DATE;
                break;

            // map fixed-point numeric types into DBTYPE_DECIMAL
            case DBTYPE_NUMERIC:
                dbTypeTemp = DBTYPE_DECIMAL;
                break;

            default:
                dbTypeTemp = dbType;
                break;
            }
            
            if (S_OK == pXT->_pSDC->CanConvertData(dbTypeTemp, vtDest.BaseType()))
            {
                pXT->_fUseSDC = TRUE;
                dbType = dbTypeTemp;
            }
        }

        if (!pXT->_fUseSDC)
        {
            // If control wants a string, let's
            //  Assume that provider can deal with WSTRs and BSTRs.

            // Note that all of our conversion code requires BSTRs.
            switch(vtDest.BaseType())
            {
            case DBTYPE_BSTR:
                dbType = DBTYPE_BSTR;
                break;

            case DBTYPE_VARIANT:
                // if the control wants a variant and the database has a non-variant
                // type, get the provider to convert to BSTR first.
                if (!CTypeCoerce::FVariantType(dbType))
                    dbType = DBTYPE_BSTR;
                break;
                
            default:
                if (dbType == DBTYPE_STR || dbType == DBTYPE_WSTR)
                {
                    dbType = DBTYPE_BSTR;
                }
                break;
            }
        }
    }
    Assert(pXT->_fUseSDC || (dbType != DBTYPE_WSTR && dbType != DBTYPE_STR));
    Assert(!pXT->_fUseSDC || CTypeCoerce::FVariantType(dbType));

    
    Assert(pXT->_dbtSrc == dbType || pXT->_dbtSrc == VT_EMPTY);
    pXT->_dbtSrc = dbType;

    // now figure out how we will convert what we fetch from the database
    //  to what the consumer desires

    // following values 0-filled at "new" time
    Assert(pXT->_cantSrc == CTypeCoerce::TYPE_NULL);  // used as conversion flg

    // when exchanging values with the HROW, should we pass a pointer into the
    //  data portion of a variant, or to the variant as a whole?
    // (Note: VT_DECIMAL overlays the entire variant)
    if (dbType != VT_VARIANT && dbType != VT_DECIMAL)
    {
        oIn = FIELD_OFFSET(VARIANT, iVal);
    }
        
    // If a type coercsion will be required, lay the groundwork
    if (dbType != vtDest.BaseType())
    {
        if (dbType == VT_NULL)
        {
            hr = OLE_E_CANTCONVERT;
        }
        else if (pXT->_fUseSDC)
        {
            hr = S_OK;
            pXT->_cantSrc = dbType;
        }
        else
        {
            hr = CTypeCoerce::CanonicalizeType(dbType, pXT->_cantSrc);

            // we use _cantSrc to indicate whether or not coercion needed
            Assert(hr || pXT->_cantSrc != CTypeCoerce::TYPE_NULL);
        }
        if (hr)
        {
            goto Cleanup;
        }
    }

    binding.iOrdinal = pXT->IdSrc();
    binding.obStatus = FIELD_OFFSET(DBData, dwStatus);
    binding.obValue = FIELD_OFFSET(DBData, var_data) + oIn;
    binding.pBindExt = 0;
    binding.cbMaxLen = 0;
    binding.dwFlags = 0;
    binding.wType = dbType;
    binding.eParamIO = DBPARAMIO_NOTPARAM;
    binding.dwPart = DBPART_STATUS | DBPART_VALUE;
    binding.dwMemOwner = DBMEMOWNER_CLIENTOWNED;    // BUGBUG is this right?
    binding.bPrecision = 0;         // BUGBUG what goes here?
    binding.bScale = 0;             // BUGBUG what goes here?


    hr = THR_NOTRACE(pDLC->CreateAccessor(_hAccessor,
                        DBACCESSOR_ROWDATA, &binding, 1 ));
    if (hr)
    {
        goto Cleanup;
    }

    // remember whether the column is writeable
    pXT->_fWriteSrc = ((pCI->dwFlags &
                        (DBCOLUMNFLAGS_WRITE | DBCOLUMNFLAGS_WRITEUNKNOWN)) != 0);

    // if we succeeded, keep DLCursor alive during my lifetime
    _pDLC = pDLC;
    _pDLC->AddRef();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CHRowAccessor::Detach, CAccessor
//
//  Synopsis:   Destroy a CHRowAccessor, releasing any resources it
//              owns, along with its own memory.
//
//---------------------------------------------------------------

void
CXferThunk::CHRowAccessor::Detach()
{
    if (_hAccessor)
    {
        Assert(_pDLC);
        _pDLC->ReleaseAccessor(_hAccessor);
        Assert(_hAccessor == NULL);
    }
    
    if (_pDLC)
        _pDLC->Release();
    
    delete this;
}

//+---------------------------------------------------------------
//
//  Member:     CHRowAccessor::ValueFromSrc, CAccessor
//
//  Synopsis:   Accomplish the actual transfer from the HROW source,
//              to a memory location expecting the bound-element's native
//              data type.
//
//  Arguments:  [pXT]      - The owning CXferThunk.
//              [pSrc]     - source instance
//              [lpvData]  - where to put the data.  It is up to caller
//                           to 0 initialize!
//
//  Returns:    S_OK:       All OK..
//              E_*         Error of some sort
//
//---------------------------------------------------------------

HRESULT
CXferThunk::CHRowAccessor::ValueFromSrc (const CXferThunk *pXT,
                                         CInstance *pSrc,
                                         LPVOID lpvData,
                                         CElement *pElemBound) const
{
    // shouldn't need this const_cast, but the IS_VALID macros
    //  weren't designed to take into account const methods.
    Assert(IS_VALID(const_cast<CXferThunk::CHRowAccessor *>(this)));
    
    HRESULT     hr = S_OK;
    CRecordInstance *pRecInstance;
    HROW        hRow;

    pRecInstance = DYNCAST(CRecordInstance, pSrc);

    hRow = pRecInstance->GetHRow();

    // if we have a NULL hrow, we skip any fetch from the Rowset, and
    //  rely on the fact that we've already initialized pOut in a manner
    //  consisting with pushing empty data into the destination element.
    if (hRow != DB_NULL_HROW)
    {
        DBData      dbValue;
        VOID       *pSrcData;
        
        // prepare the variant which will receive the database value.
        pSrcData = CXferThunk::PvInitVar(&dbValue.var_data, pXT->_dbtSrc);

        hr = THR(_pDLC->GetData(hRow, _hAccessor, (void *) &dbValue));
        Assert(FAILED(hr) || hr == S_OK);

        // VT_DECIMAL overlaps VARIANT completely.  Some providers write
        // into the DECIMAL.wReserved field, wiping out the VARIANT.vt field
        // that we carefully set up in PvInitVar.  Protect against such badness.
        if (pXT->_dbtSrc == VT_DECIMAL)
        {
            dbValue.var_data.vt = VT_DECIMAL;
        }
        
        if (!hr)
        {
            CVarType vtDest = pXT->_vtDest;
            
            if (dbValue.dwStatus == DBSTATUS_S_ISNULL)
            {
                // We got a NULL; rely on pre-clearing of lpvData by caller.
                // However, if destination wants VT_NULL, then he has
                //  pre-initialized vt to VT_EMPTY, and we want VT_NULL.
                if (vtDest.BaseType() == VT_VARIANT)
                {
                    ((VARIANT *) lpvData)->vt = VT_NULL;
                }
            }
            else
            {
                BYTE *pDest = (BYTE *) lpvData;
                CTypeCoerce::CAN_TYPE cantSrc = pXT->_cantSrc;
                CTypeCoerce::CAN_TYPE cantDest = pXT->_cantDest;
                
                if (vtDest.FInVariant())
                {
                    pDest += FIELD_OFFSET(VARIANT, iVal);
                }
                
                if (cantSrc == CTypeCoerce::TYPE_NULL)
                {
                    // no conversion required, just copy the data bits to caller's
                    //  buffer
                    memcpy(pDest, pSrcData, CTypeCoerce::MemSize(cantDest));
                    // owner of lpvData now has reponsibility for freeing
                    //  any BSTR's etc.
                }
                else if (pXT->_fUseSDC)
                {
                    // SimpleDataConverter converts between variants, but
                    // pDest usually doesn't point into a variant.  So we
                    // convert into a temp variant, then copy the value.
                    VARIANT varTemp;
                    IHTMLElement *pHTMLElement = NULL;
                    
                    VariantInit(&varTemp);
                    if (pElemBound)
                    {
                        IGNORE_HR(pElemBound->QueryInterface(IID_IHTMLElement,
                                                        (void**)&pHTMLElement));
                    }

                    hr = pXT->_pSDC->ConvertData(dbValue.var_data,
                                                        vtDest.BaseType(),
                                                        pHTMLElement,
                                                        &varTemp);
                    if (!hr)
                    {
                        memcpy(pDest, &varTemp.iVal, CTypeCoerce::MemSize(cantDest));
                    }
                    else
                    {
                        dbValue.dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
                        hr = DB_E_CANTCONVERTVALUE;
                    }
                    VariantClear(&dbValue.var_data);
                    ReleaseInterface(pHTMLElement);
                }
                else
                {
                    // conversion required, do it
                    hr = THR(CTypeCoerce::ConvertData(cantSrc,
                                                      pSrcData,
                                                      cantDest,
                                                      pDest ) );
                    // caller is not repsonsible for our variant data;
                    //  free it.
                    VariantClear(&dbValue.var_data);
                }
            }
        }
        else
        {
            // if what went wrong was missing data, we treat it like
            // VT_EMPTY success.  We rely on caller pre-initializing
            // his receiving buffer.
            if (hr == DB_E_ERRORSOCCURRED)
            {
                if (dbValue.dwStatus == DBSTATUS_E_UNAVAILABLE)
                {
                    hr= S_OK;
                    // proceed with normal comparison or transfer
                }
            }
        }
    }
    

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CHRowAccessor::ValueToSrc, CAccessor
//
//  Synopsis:   Accomplish the actual transfer to the HROW source,
//              on behalf of the owning CXferThunk
//
//  Arguments:  [pXT]      - The owning CXferThunk.
//              [pSrc]     - source instance
//              [lpvData]  - pointer to data
//              [pdwStatus] - place for OLE DB status
//
//---------------------------------------------------------------

HRESULT
CXferThunk::CHRowAccessor::ValueToSrc(const CXferThunk *pXT,
                                      CInstance *pSrc,
                                      LPVOID lpvData,
                                      DWORD *pdwStatus,
                                      CElement *pElemBound) const
{
    HRESULT     hr;
    CRecordInstance *pRecInstance;
    HROW        hRow;
    DBData      dbValue;
    VOID       *pSrcData;
    
    CTypeCoerce::CAN_TYPE cantSrc = pXT->_cantSrc;
    DBTYPE      dbtSrc = pXT->_dbtSrc;
    CVarType    vtDest = pXT->_vtDest;
    
    // shouldn't need this const_cast, but the IS_VALID macros
    //  weren't designed to take into account const methods.
    Assert(IS_VALID(const_cast<CXferThunk::CHRowAccessor *>(this)));

    Assert(*pdwStatus == DBSTATUS_S_OK || *pdwStatus == DBSTATUS_S_ISNULL);    // caller-initialized
    dbValue.dwStatus = *pdwStatus;      // initialize for normal transfer

    pRecInstance = DYNCAST(CRecordInstance, pSrc);
    
    hRow = pRecInstance->GetHRow();

    // if NULL HROW, we can't transfer any data to it
    if (hRow == DB_NULL_HROW)
    {
        hr = E_FAIL;
        goto Done;
    }

    // BUGBUG: Just because we think the element likes to give us, say
    //  strings in a variant, doesn't mean that it really does!
    if (vtDest.FInVariant() && ((VARIANT *) lpvData)->vt != vtDest.BaseType())
    {
        Assert(!"bound parameter of inconsistent datatype");
        hr = E_FAIL;    // BUGBUG: error code?
        goto Done;
    }
    
    // prepare the structure which we will use to exchange data with
    //  the database.
    pSrcData = CXferThunk::PvInitVar(&dbValue.var_data, dbtSrc);
    
    // Determine if we want NULL-equivalent data in the source column,
    //  or a more "normal" transfer.
    if (vtDest.BaseType() == VT_VARIANT
        && (((VARIANT *) lpvData)->vt == VT_NULL
            || (dbtSrc != VT_VARIANT
                && ((VARIANT *) lpvData)->vt == VT_EMPTY ) ) )
    {
        dbValue.dwStatus = DBSTATUS_S_ISNULL;
        if (dbtSrc == VT_VARIANT)
        {
            dbValue.var_data.vt = VT_NULL;
        }
    }
    else
    {
        // locals are not really named intuitively -- we are transferring
        //  from Dest to Src
        BYTE *pDest = (BYTE *) lpvData;
        CTypeCoerce::CAN_TYPE cantDest = pXT->_cantDest;
        
        if (vtDest.FInVariant())
        {
            pDest += FIELD_OFFSET(VARIANT, iVal);
        }
        
        if (cantSrc == CTypeCoerce::TYPE_NULL)
        {
            // no conversion required, just copy the data bits to caller's
            //  buffer
            memcpy(pSrcData, pDest, CTypeCoerce::MemSize(cantDest));
        }
        else if (pXT->_fUseSDC)
        {
            // SimpleDataConverter uses variants, so copy into a temp
            // variant, then convert.
            VARIANT varTemp;
            IHTMLElement *pHTMLElement = NULL;
            
            V_VT(&varTemp) = vtDest.BaseType();
            memcpy(&varTemp.iVal, pDest, CTypeCoerce::MemSize(cantDest));

            if (pElemBound)
            {
                IGNORE_HR(pElemBound->QueryInterface(IID_IHTMLElement,
                                                (void**)&pHTMLElement));
            }

            hr = pXT->_pSDC->ConvertData(varTemp, dbtSrc,
                                            pHTMLElement, &dbValue.var_data);
            ReleaseInterface(pHTMLElement);
            
            if (hr)
            {
                dbValue.dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
                hr = DB_E_CANTCONVERTVALUE;
                goto Cleanup;
            }
        }
        else
        {
            // conversion required, do it
            hr = THR(CTypeCoerce::ConvertData(cantDest,
                                              pDest,
                                              cantSrc,
                                              pSrcData ) );
            if (hr)
            {
                goto Cleanup;
            }
        }
    }
        
    hr = THR(_pDLC->SetData(hRow, _hAccessor, (void *) &dbValue));
    if (cantSrc != CTypeCoerce::TYPE_NULL)
    {
        VariantClear(&dbValue.var_data);
    }
    
Cleanup:
    *pdwStatus = dbValue.dwStatus;
    
Done:
    RRETURN(hr);
}

BOOL
CXferThunk::CHRowAccessor::IsSrcWritable(CInstance *pSrc) const
{
    return (DYNCAST(CRecordInstance, pSrc)->GetHRow() != NULL);
}


#if DBG==1
char s_achCXferThunk[] = "CXferThunk";

char*
CXferThunk::GetClassName()
{
    return s_achCXferThunk;
}

BOOL
CXferThunk::IsValidObject ()
{
    Assert(this);
    Assert(_vtDest.BaseType() != VT_EMPTY);
    Assert(!_pAccessor || _dbtSrc != VT_EMPTY);
    Assert(!_pAccessor || _pAccessor->IsValidObject ());
    return TRUE;
}

void
CXferThunk::Dump (CDumpContext&)
{
    IS_VALID(this);
}

char s_achCHRowAccessor[] = "CHRowAccessor";

char*
CXferThunk::CHRowAccessor::GetClassName()
{
    return s_achCHRowAccessor;
}

BOOL
CXferThunk::CHRowAccessor::IsValidObject ()
{
    Assert(this);
    return TRUE;
}

void
CXferThunk::CHRowAccessor::Dump (CDumpContext&)
{
    IS_VALID(this);
}
#endif // DBG==1

