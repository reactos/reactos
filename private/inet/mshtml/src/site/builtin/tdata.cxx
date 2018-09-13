//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tdata.cxx
//
//  Contents:   CTable and related classes.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_SAVER_HXX_
#define X_SAVER_HXX_
#include <saver.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include <download.hxx>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include <othrguid.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include <detail.hxx>
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include <rowbind.hxx>
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

ExternTag(tagTableRecalc);

#ifndef NO_DATABINDING
#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

class CDBindMethodsTable : public CDBindMethods
{
    typedef CDBindMethods super;

public:
    CDBindMethodsTable()    {}
    ~CDBindMethodsTable()   {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const;

    // Notification that data is ready (or not ready)
    virtual void    OnDataReady ( CElement *pElem, BOOL fReady ) const;

    // Notification that the source instance has changed
    virtual HRESULT InstanceChanged(CElement *pElem, CInstance *pSrcInstance) const;

protected:
    virtual DBIND_KIND DBindKindImpl(CElement *pElem,
                                     LONG id,
                                     DBINFO *pdbi) const;
    virtual BOOL    IsReadyImpl(CElement *pElem) const;
};

static const CDBindMethodsTable DBindMethodsTable;

const CDBindMethods *
CTable::GetDBindMethods()
{
    return &DBindMethodsTable;
}

//+----------------------------------------------------------------------------
//
//  Function: DBindKindImpl, CDBindMethods
//
//  Synopsis: Indicate whether or not element is or can be databound, and
//            optionally return additional info about binding -- a DISPID
//            or other ID used for transfer, and the desired data type
//            to be used for transfer.
//  Arguments:
//            [id]    - bound id
//            [pdbi]  - pointer to struct to get data type and ID;
//                      may be NULL
//
//  Returns:  Binding status: one of
//              DBIND_NONE
//              DBIND_SINGLEVALUE
//              DBIND_ICURSOR
//              DBIND_IROWSET
//              DBIND_TABLE
//            For Table, returns DBIND_NONE or DBIND_TABLE.
//
//-----------------------------------------------------------------------------
DBIND_KIND
CDBindMethodsTable::DBindKindImpl(CElement *pElem, LONG id, DBINFO *pdbi) const
{
    DBIND_KIND dbk = DBIND_NONE;    // pdbi, if any, initialized by wrapper

    Assert(pElem->Tag() == ETAG_TABLE);

    if (id == ID_DBIND_DEFAULT)
    {
        dbk = DBIND_TABLE;
        if (pdbi)
        {
            pdbi->_vt = DBTYPE_HCHAPTER;
            pdbi->_dwTransfer = DBIND_NOTIFYONLY | DBIND_ONEWAY;
        }
    }

    return dbk;
}

//+----------------------------------------------------------------------------
//
//  Function: IsReadyImpl, CDBindMethods
//
//  Synopsis: Indicate whether or element is ready to be bound.
//
//  Arguments:  pEleme  table element being queried
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethodsTable::IsReadyImpl(CElement *pElem) const
{
    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function: InstanceChanged, CDBindMethods
//
//  Synopsis: Notification that the source instance has a new HROW.
//
//  Arguments:  pElem           table element being queried
//              pSrcInstance    its source instance (holds the HROW)
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsTable::InstanceChanged(CElement *pElem, CInstance *pSrcInstance) const
{
    HRESULT hr;
    CTable *pTable = DYNCAST(CTable, pElem);

    // We're only supposed to get here for inner tables in a hierarchy
    Assert(!FormsIsEmptyString(pTable->GetAAdataFld()));

    // ReplaceProvider(NULL) just queues up a ReBind request with the dbtask
    hr = THR(pTable->GetDBMembers()->GetBinder(ID_DBIND_DEFAULT)->ReplaceProvider(NULL));

    RRETURN(hr);
}

#endif // ndef NO_DATABINDING

//+------------------------------------------------------------------------
//
//  Member:     CTable::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CTable::Save(CStreamWriteBuff *pStmWrBuff, BOOL fEnd)
{
    HRESULT hr;

    if (!fEnd)
    {
        // remember whether stream is building the repeating template
        _fBuildingTemplate = !!pStmWrBuff->TestFlag(WBF_DATABIND_MODE);

        // turn off "building-template" mode;  this allows nested tables to inject
        // their header/footer/caption into the template
        pStmWrBuff->ClearFlags(WBF_DATABIND_MODE);
    }

    hr = THR(super::Save(pStmWrBuff, fEnd));

    if (fEnd)
    {
        // restore the original value of "building-template" mode
        if (_fBuildingTemplate)
            pStmWrBuff->SetFlags(WBF_DATABIND_MODE);
        else
            pStmWrBuff->ClearFlags(WBF_DATABIND_MODE);
    }

    RRETURN(hr);
}


#ifndef NO_DATABINDING
//+---------------------------------------------------------------------------
//
//  Member:     OnDataReady
//
//  Synopsis:   Notification that datasource is ready to provide data
//
//  Retruns:    nothing
//
//----------------------------------------------------------------------------

void
CDBindMethodsTable::OnDataReady(CElement *pElem, BOOL fReady) const
{
    CTable *pTable = DYNCAST(CTable, pElem);
    CTableLayout *pTableLayout = pTable->Layout();

    
    if (fReady)
    {
        IGNORE_HR(pTableLayout->Populate());
    }
    else
    {
        CDetailGenerator *pDG = pTableLayout->_pDetailGenerator;

        if (pDG)
        {
            if (pTable->_fEnableDatabinding)
            {
                IGNORE_HR(pTableLayout->DeleteGeneratedRows());
                IGNORE_HR(pDG->RestoreTemplate());
            }

            pDG->Detach();
            delete pDG;
            pTableLayout->_pDetailGenerator = NULL;
            pTableLayout->_fRefresh = FALSE;
            pTableLayout->ResetMinMax();
        }
    }
}

HRESULT
CDBindMethodsTable::BoundValueToElement(CElement *, LONG, BOOL, LPVOID) const
{
    Assert(FALSE);
    return E_UNEXPECTED;
}

HRESULT
CDBindMethodsTable::BoundValueFromElement(CElement *, LONG, BOOL, LPVOID) const
{
    Assert(FALSE);
    return E_UNEXPECTED;
}
#endif // ndef NO_DATABINDING

//+---------------------------------------------------------------------------
//
//  Member:     Refresh (regenerate)
//
//  Synopsis:   Populate the table with repeated rows when setting the new
//              RepeatSrc property.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTable::refresh()
{
    HRESULT hr = S_OK;

#ifndef NO_DATABINDING
    // Table delegates refresh to table layout.
    hr = Layout()->refresh();
#endif // ndef NO_DATABINDING
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     nextPage (public, callable from script code)
//
//  Synopsis:   Only for repeated data-bound tables that have a specified
//              pageSize parameter, display the next pageSize data records.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTable::nextPage()
{
#ifndef NO_DATABINDING
    if (Layout()->_pDetailGenerator)
        return Layout()->_pDetailGenerator->nextPage();
    else
#endif
        return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     previousPage (public, callable from script code)
//
//  Synopsis:   Only for repeated data-bound tables that have a specified
//              pageSize parameter, display the previous pageSize data records.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTable::previousPage()
{
#ifndef NO_DATABINDING
    if (Layout()->_pDetailGenerator)
        return Layout()->_pDetailGenerator->previousPage();
    else
#endif
        return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     firstPage (public, callable from script code)
//
//  Synopsis:   Only for repeated data-bound tables that have a specified
//              pageSize parameter, display the first pageSize data records.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTable::firstPage()
{
#ifndef NO_DATABINDING
    if (Layout()->_pDetailGenerator)
        return Layout()->_pDetailGenerator->firstPage();
    else 
#endif
        return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     lastPage (public, callable from script code)
//
//  Synopsis:   Only for repeated data-bound tables that have a specified
//              pageSize parameter, display the last pageSize data records.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTable::lastPage()
{
#ifndef NO_DATABINDING
    if (Layout()->_pDetailGenerator)
        return Layout()->_pDetailGenerator->lastPage();
    else 
#endif
        return E_FAIL;
}



//+------------------------------------------------------------------------
//
//  Member:     put_dataPageSize, IHTMLTable
//
//  Synopsis:   change the page size
//
//  Arguments:  v       new page size
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTable::put_dataPageSize(long v)
{
    HRESULT hr;

    // sanitize
    if (v < 0)
        v = 0;
    
    // set the new page size
    hr = s_propdescCTabledataPageSize.b.SetNumberProperty(v, this, CVOID_CAST(GetAttrArray()));

    // tell the repetition agent
    if (!hr && Layout()->_pDetailGenerator)
        IGNORE_HR(Layout()->_pDetailGenerator->SetPageSize(v));

    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     get_dataPageSize, IHTMLTable
//
//  Synopsis:   return the page size
//
//  Arguments:  v       where to store page size
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTable::get_dataPageSize(long * p)
{
        return s_propdescCTabledataPageSize.b.GetNumberProperty(p, this, CVOID_CAST(GetAttrArray()));
}

//+------------------------------------------------------------------------
//
//  Member:     FindRepeatingContext, public
//
//  Synopsis:   find the enclosing repeated element with the given dataSrc
//              and dataFld attributes
//
//  Arguments:  strDataSrc  look for an element with this dataSrc attribute
//              strDataFld     ... and this dataFld attribute
//              ppElement   enclosing element
//              ppInstance  instance of repetition that contains me
//
//  Returns:    S_OK        found it
//              S_FALSE     found a syntactically enclosing element, but it's not
//                          repeating, *ppInstance will be NULL
//              E_FAIL      couldn't find an enclosing element

HRESULT
CTable::GetInstanceForRow(CTableRow *pRow, CRecordInstance **ppInstance)
{
    HRESULT hr;
    Assert(pRow);
    
#ifndef NO_DATABINDING
    if (Layout()->IsRepeating())
    {
        CDetailGenerator *pDG = Layout()->_pDetailGenerator;
        CRecordInstance *pRecInstance = pDG->GetInstanceForRow(pRow->Layout()->_iRow);

        *ppInstance = pRecInstance;
        hr = S_OK;
    }
    else
#endif
    {
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::IsRepeating
//
//----------------------------------------------------------------------------

BOOL
CTable::IsRepeating()
{
    CDetailGenerator *pDG = Layout()->_pDetailGenerator;
    return pDG ? pDG->IsRepeating() : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::GetDataSrc
//
//----------------------------------------------------------------------------

LPCTSTR
CTable::GetDataSrc()
{
    CDetailGenerator *pDG = Layout()->_pDetailGenerator;
    return pDG ? pDG->GetDataSrc() : NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::GetDataFld
//
//----------------------------------------------------------------------------

LPCTSTR
CTable::GetDataFld()
{
    CDetailGenerator *pDG = Layout()->_pDetailGenerator;
    return pDG ? pDG->GetDataFld() : NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::IsFieldKnown
//
//----------------------------------------------------------------------------

BOOL
CTable::IsFieldKnown(LPCTSTR strField)
{
    CDetailGenerator *pDG = Layout()->_pDetailGenerator;
    return pDG ? pDG->IsFieldKnown(strField) : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::AddDataboundElement
//
//----------------------------------------------------------------------------

HRESULT
CTable::AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining, LPCTSTR strField)
{
    CDetailGenerator *pDG = Layout()->_pDetailGenerator;
    AssertSz(pDG, "Can't add databound element to a non-repeating table");
    return pDG->AddDataboundElement(pElement, id, pRowContaining, strField);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::GetProvider
//
//----------------------------------------------------------------------------

CDataSourceProvider *
CTable::GetProvider()
{
    return GetDBMembers()->GetBinder(ID_DBIND_DEFAULT)->GetProvider();
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CTable::OnReadyStateChange()
{   // do not call super::OnReadyStateChange here - we handle firing the event ourselves
    SetReadyStateTable(_readyStateTable);
}

//+------------------------------------------------------------------------
//
//  Member:     CTable::SetReadyStateTable
//
//  Synopsis:   Use this to set the ready state;
//              it fires onreadystate change if needed.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CTable::SetReadyStateTable(long readyStateTable)
{
    long readyState;

    _readyStateTable = readyStateTable;

    readyState = min ((long)_readyStateTable, super::GetReadyState());

    if ((long)_readyStateFired != readyState)
    {
        _readyStateFired = readyState;

        if (_readyStateTable == READYSTATE_INTERACTIVE)
        {
            // now, we are about to populate the table, we can use incremental recalc
            Assert(HasLayout());
            Layout()->_cCalcedRows = 0;
        }

        Fire_onreadystatechange();
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTable:get_readyState
//
//  Synopsis:  this is for the OM and uses the long _lReadyState
//      to determine the string returned.
//
//+------------------------------------------------------------------------------

HRESULT
CTable::get_readyState(BSTR * p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(s_enumdeschtmlReadyState.StringFromEnum(_readyStateTable, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CTable::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = get_readyState(&V_BSTR(pVarRes));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

