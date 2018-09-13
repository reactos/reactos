//+-----------------------------------------------------------------------
//
//  IShellFolder Data Provider Data Object
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       DataPrv.h
//
//  Contents:   Declaration of the CShellFolderData COM object.
//
//------------------------------------------------------------------------

#ifndef _DATAPRV_H_
#define _DATAPRV_H_
#include "simpdata.h"

//------------------------------------------------------------------------
//
//  Class:     CShellFolderData
//
//  Synopsis:  This is the data source object that works from any 
//             IShellFolder.
//
//------------------------------------------------------------------------

class CShellFolderData : public OLEDBSimpleProvider
{
public:
    ~CShellFolderData();
    HRESULT SetListener(OLEDBSimpleProviderListener **ppListener);
    
    // IUnknown

    STDMETHOD(QueryInterface)(REFIID, LPVOID FAR*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //  The STD methods are passed directly to the attached TDCArr object
    //
    STDMETHOD(getRowCount)(DBROWCOUNT *pcRows);
    STDMETHOD(getColumnCount)(DB_LORDINAL *pcColumns);
    STDMETHOD(getRWStatus)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus);
    STDMETHOD(getVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol,
                                 OSPFORMAT fmt, VARIANT *pVar);
    STDMETHOD(setVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol,
                                 OSPFORMAT fmt, VARIANT Var);
    STDMETHOD(getLocale)(BSTR *pbstrLocale);
    STDMETHOD(deleteRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    STDMETHOD(insertRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);
    STDMETHOD(find)(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val,
              OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound);
    STDMETHOD(addOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(removeOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(getEstimatedRows)(DBROWCOUNT *pcRows);
    STDMETHOD(isAsync)(BOOL *pbAsync);
    STDMETHOD(stopTransfer)();

public:
    HRESULT SetShellFolder(IShellFolder *psf);

private:
    HRESULT _DoEnum();

private:
    OLEDBSimpleProviderListener  **_ppListener;
    IUnknown                     *_pListenerOwner;
    IShellFolder                 *_psf;
    HDPA                        _hdpa;
};


#endif _DATAPRV_H_
