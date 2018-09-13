
#ifndef _EVENT_H_
#define _EVENT_H_

#include "iface.h"          // for IARPEvent


//------------------------------------------------------------------------
//
//  CEventBroker
//
//  This class brokers events sent from the OSP to the OSP listener
//  or the data source listener.
// 
//------------------------------------------------------------------------

class CEventBroker : public IARPEvent
{
public:
    // *** IUnknown ***
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppvObj);

    // *** IARPEvent ***
    STDMETHOD(SetDataSourceListener)(DataSourceListener *);
    STDMETHOD(IsOSPListener)        (OLEDBSimpleProviderListener * posp);
    STDMETHOD(SetOSPListener)       (OLEDBSimpleProviderListener * posp);
    STDMETHOD(AboutToDeleteRows)    (DBROWCOUNT iRowStart, DBROWCOUNT cRows);
    STDMETHOD(DeletedRows)          (DBROWCOUNT iRowStart, DBROWCOUNT cRows);
    STDMETHOD(RowsAvailable)        (DBROWCOUNT iRowStart, DBROWCOUNT cRows);
    STDMETHOD(RowChanged)           (DBROWCOUNT iRow);
    STDMETHOD(LoadCompleted)        (void);
    STDMETHOD(LoadAborted)          (void);
    STDMETHOD(DataSetChanged)       (void);
    
    CEventBroker(LPWSTR pszQualifier);
    ~CEventBroker();

private:
    
    ULONG _cRef;         
    
    DataSourceListener * _pdsl;
    OLEDBSimpleProviderListener *_pospl;

    BSTR _cbstrQualifier;
};

HRESULT CARPEvent_CreateInstance(REFIID riid, LPVOID * ppvObj, LPWSTR pszQualifier);


#endif // _EVENT_H_
