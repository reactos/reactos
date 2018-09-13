//+-----------------------------------------------------------------------
//
//  event.cpp:  Implementation of the CEventBroker class.
//              This class translates internal ADC/OSP events into
//              appropriate notifications for the external world.
//
//------------------------------------------------------------------------

#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "event.h"
//#include "dump.h"


// constructor
CEventBroker::CEventBroker(LPWSTR pszQualifier) : _cRef(1)
{
    ASSERT(NULL == _pospl);
    ASSERT(NULL == _pdsl);

    TraceAddRef(CEventBroker, _cRef);
    _cbstrQualifier = SysAllocString(pszQualifier);
}


// destructor
CEventBroker::~CEventBroker()
{
    TraceMsg(TF_OBJLIFE, "(EventBroker) destroying");
    
    SetDataSourceListener(NULL);
    SetOSPListener(NULL);
    if (_cbstrQualifier)
        SysFreeString(_cbstrQualifier);
}



/*--------------------------------------------------------------------
Purpose: IUnknown::QueryInterface
*/
STDMETHODIMP CEventBroker::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CEventBroker, IARPEvent),
        { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CEventBroker::AddRef()
{
    ++_cRef;
    TraceAddRef(CEventBroker, _cRef);
    return _cRef;
}


STDMETHODIMP_(ULONG) CEventBroker::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    TraceRelease(CEventBroker, _cRef);
    
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::SetDataSourceListener

         Sets the Data Source listener.

         If pdsl is NULL, no notifcations are sent.
*/
STDMETHODIMP CEventBroker::SetDataSourceListener(DataSourceListener * pdsl)
{
    // If we've changed/reset the data source listener, make sure we don't
    // think we've fired dataMemberChanged on it yet.
    ATOMICRELEASE(_pdsl);

    _pdsl = pdsl;
    if (_pdsl)
        _pdsl->AddRef();

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::SetOSPListener

         Sets the OSP listener.

         If pospl is NULL, no notifications are sent.
*/
STDMETHODIMP CEventBroker::SetOSPListener(OLEDBSimpleProviderListener *pospl)
{
    ATOMICRELEASE(_pospl);

    _pospl = pospl;
    if (_pospl)
        _pospl->AddRef();
        
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::IsOSPListener

         Returns S_OK if the given listener is the current OSP listener.
*/
STDMETHODIMP CEventBroker::IsOSPListener(OLEDBSimpleProviderListener *pospl)
{
    return (pospl == _pospl) ? S_OK : S_FALSE;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::RowChanged

         Fired when the OSP updated some fields in a row.
*/
STDMETHODIMP CEventBroker::RowChanged(DBROWCOUNT iRow)
{
    ASSERT(iRow >= 0);
    
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) rowChanged %d", iRow);
    
        _pospl->cellChanged(iRow, -1);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::AboutToDeleteRows

         Fired when the OSP is going to delete some rows.
*/
STDMETHODIMP CEventBroker::AboutToDeleteRows(DBROWCOUNT iRowStart, DBROWCOUNT cRows)
{
    ASSERT(iRowStart >= 0);
    ASSERT(cRows > 0);
    
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) AboutToDeleteRows(%d, %d)", iRowStart, cRows);
    
        _pospl->aboutToDeleteRows(iRowStart, cRows);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::DeletedRows

         Fired when the OSP has deleted some rows.
*/
STDMETHODIMP CEventBroker::DeletedRows(DBROWCOUNT iRowStart, DBROWCOUNT cRows)
{
    ASSERT(iRowStart >= 0);
    ASSERT(cRows > 0);
    
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) DeletedRows(%d, %d)", iRowStart, cRows);
    
        _pospl->deletedRows(iRowStart, cRows);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::RowsAvailable

         Fired when the OSP has enumerated some rows.
*/
STDMETHODIMP CEventBroker::RowsAvailable(DBROWCOUNT iRowStart, DBROWCOUNT cRows)
{
    ASSERT(iRowStart >= 0);
    ASSERT(cRows > 0);
    
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) RowsAvailable(%d, %d)", iRowStart, cRows);
        
        _pospl->rowsAvailable(iRowStart, cRows);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::LoadCompleted

         Fired when the OSP has finished enumerating its data.
*/
STDMETHODIMP CEventBroker::LoadCompleted(void)
{
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) LoadCompleted");

        _pospl->transferComplete(OSPXFER_COMPLETE);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::LoadAborted

         Fired when the OSP has aborted its enumeration.
*/
STDMETHODIMP CEventBroker::LoadAborted(void)
{
    // Right now, any error results in not returning an SP object,
    // therefore we should not fire transfer complete.
    if (_pospl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) LoadAborted");
    
        _pospl->transferComplete(OSPXFER_ABORT);
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IARPEvent::DataSetChanged

         Fired when the OSP's data set has changed (resorted, refiltered, etc.)
*/
STDMETHODIMP CEventBroker::DataSetChanged(void)
{
    if (_pdsl)
    {
        TraceMsg(TF_DSO, "(CEventBroker) DataSetChanged");
        
        _pdsl->dataMemberChanged(_cbstrQualifier);
    }
    return S_OK;
}


/*----------------------------------------------------------
Purpose: Create-instance function for CARPEvent

*/
HRESULT CARPEvent_CreateInstance(REFIID riid, LPVOID * ppvObj, LPWSTR pszQualifier)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppvObj = NULL;
    
    CEventBroker * pObj = new CEventBroker(pszQualifier);
    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }

    return hres;
}

#endif //DOWNLEVEL_PLATFORM
