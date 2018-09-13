//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       regkey.hxx
//
//  Contents:   CServerObject
//
//-------------------------------------------------------------------------

#ifndef __SERVER_HXX__
#define __SERVER_HXX__

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

class CServerObject : public IPersistMoniker, IOleObject
{
public:

    CServerObject(CHTMLApp *pApp);
    
    DECLARE_FORMS_STANDARD_IUNKNOWN(CServerObject)

    // IPersist methods
    STDMETHOD(GetClassID)(CLSID *);

    // IPersistMoniker methods
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(BOOL, IMoniker *, LPBC, DWORD );
    STDMETHOD(Save)(IMoniker *, LPBC, BOOL);
    STDMETHOD(SaveCompleted)(IMoniker *, LPBC);
    STDMETHOD(GetCurMoniker)(IMoniker **);

    // IOleObject methods
    
    STDMETHOD(SetClientSite)(IOleClientSite *);
    STDMETHOD(GetClientSite)(IOleClientSite **);
    STDMETHOD(SetHostNames)(LPCOLESTR, LPCOLESTR);
    STDMETHOD(Close)(DWORD);
    STDMETHOD(SetMoniker)(DWORD , IMoniker *);
    STDMETHOD(GetMoniker)(DWORD, DWORD, IMoniker **);
    STDMETHOD(InitFromData)(IDataObject *, BOOL, DWORD);
    STDMETHOD(GetClipboardData)(DWORD, IDataObject **);
    STDMETHOD(DoVerb)(LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCOLERECT);
    STDMETHOD(EnumVerbs)(IEnumOLEVERB **);
    STDMETHOD(Update)();
    STDMETHOD(IsUpToDate)();
    STDMETHOD(GetUserClassID)(CLSID *);
    STDMETHOD(GetUserType)(DWORD, LPOLESTR *);
    STDMETHOD(SetExtent)(DWORD, SIZEL *);
    STDMETHOD(GetExtent)(DWORD, SIZEL *);
    STDMETHOD(Advise)(IAdviseSink *, DWORD *);
    STDMETHOD(Unadvise)(DWORD);
    STDMETHOD(EnumAdvise)(IEnumSTATDATA **);
    STDMETHOD(GetMiscStatus)(DWORD, DWORD *);
    STDMETHOD(SetColorScheme)(LOGPALETTE  *pLogpal);

private:
    CHTMLApp *   _pApp;
};

#endif // __SERVER_HXX__


