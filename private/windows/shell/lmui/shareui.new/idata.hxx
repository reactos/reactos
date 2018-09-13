//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       idata.hxx
//
//  Contents:   IDataObject implementation for path
//
//  History:    27-Oct-95 BruceFo stolen from old sharing tool
//
//--------------------------------------------------------------------------

#include <objbase.h>

//+-------------------------------------------------------------------------
//
//  Class:      CDataObject
//
//  Synopsis:   Standard result data object implementataion.
//
//--------------------------------------------------------------------------

class CDataObject: public IDataObject
{

public:

    CDataObject(VOID);
    ~CDataObject();

    //
    // Second-phase constructor
    //

    HRESULT
    InitInstance(
        IN PWSTR pszItem
        );

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)(VOID);
    STDMETHOD_(ULONG,Release)(VOID);

    //
    // IDataObject methods
    //

    STDMETHOD(GetData)(
            IN LPFORMATETC pFormatEtc,
            OUT LPSTGMEDIUM pStgMedium
            );

    STDMETHOD(GetDataHere)(
            IN LPFORMATETC pformatetc,
            IN OUT LPSTGMEDIUM pmedium
            );

    STDMETHOD(QueryGetData)(
            IN LPFORMATETC pformatetc
            );

    STDMETHOD(GetCanonicalFormatEtc)(
            IN LPFORMATETC pformatetc,
            IN LPFORMATETC pformatetcOut
            );

    STDMETHOD(SetData)(
            IN LPFORMATETC pformatetc,
            IN OUT STGMEDIUM * pmedium,
            IN BOOL fRelease
            );

    STDMETHOD(EnumFormatEtc)(
            IN DWORD dwDirection,
            IN LPENUMFORMATETC * ppenumFormatEtc
            );

    STDMETHOD(DAdvise)(
            IN FORMATETC * pFormatetc,
            IN DWORD advf,
            IN LPADVISESINK pAdvSink,
            IN DWORD * pdwConnection
            );

    STDMETHOD(DUnadvise)(
            IN DWORD dwConnection
            );

    STDMETHOD(EnumDAdvise)(
            IN LPENUMSTATDATA * ppenumAdvise
            );

private:

    ULONG   m_refs;
    PWSTR   m_pszItem;
};
