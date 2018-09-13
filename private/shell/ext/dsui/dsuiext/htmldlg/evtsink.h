//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      evtsink.h
//
//  Contents:  CSiteObj class definition
//
//  History:   22-Jan-97 EricB      Created
//
//-----------------------------------------------------------------------------

#ifndef _EVT_SINK_H
#define _EVT_SINK_H

#include "siteobj.h"

//class CInputEventSink : public HTMLInputTextEvents
class CInputEventSink : public HTMLDocumentEvents
{
public:

#ifdef _DEBUG
    char szClass[16];
#endif

    CInputEventSink(LPSITE pSite);
    ~CInputEventSink();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR ** rgszNames,
                             UINT cNames, LCID lcid,
                             DISPID* rgdispid);

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid,
                      WORD wFlags, DISPPARAMS* pdispparams,
                      VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
                      UINT* puArgErr);

private:
    ULONG   m_refs;
    LPSITE  m_pSite;
};

#endif
