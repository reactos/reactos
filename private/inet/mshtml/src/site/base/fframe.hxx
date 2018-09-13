//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       fframe.hxx
//
//  Contents:   Fake frame
//
//  Classes:    CFakeFrame
//
//----------------------------------------------------------------------------

#ifndef I_FFRAME_HXX_
#define I_FFRAME_HXX_
#pragma INCMSG("--- Beg 'fframe.hxx'")

class CFormInPlace;

class CFakeUIWindow : public IOleInPlaceFrame
{
public:
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);
    STDMETHOD(GetBorder)(LPOLERECT lprectBorder);
    STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS pborderwidths);
    STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS pborderwidths);
    STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR pszObjName);
    STDMETHOD(InsertMenus)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenu)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)(HMENU hmenuShared);
    STDMETHOD(SetStatusText)(LPCOLESTR pszStatusText);
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg, WORD wID);

    virtual IOleInPlaceUIWindow * RealInPlaceUIWindow() = 0;
    virtual CDoc * Doc() = 0;

    CFormInPlace * InPlace(); 
};

class CFakeDocUIWindow : public CFakeUIWindow
{
    virtual IOleInPlaceUIWindow * RealInPlaceUIWindow();
    virtual CDoc * Doc();
};

class CFakeInPlaceFrame : public CFakeUIWindow
{
    virtual IOleInPlaceUIWindow * RealInPlaceUIWindow();
    virtual CDoc * Doc();
};

#pragma INCMSG("--- End 'fframe.hxx'")
#else
#pragma INCMSG("*** Dup 'fframe.hxx'")
#endif
