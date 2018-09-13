//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       frame.hxx
//
//  Contents:   CHTMLAppFrame
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#ifndef __FRAME_HXX__
#define __FRAME_HXX__

// Forward declarations
class CHTMLApp;

class CHTMLAppFrame : public IOleInPlaceFrame
{
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLApp, HTMLApp)
    
public:
    CHTMLAppFrame();

    void Passivate();
    void Resize();

    HRESULT TranslateKeyMsg(MSG *pMsg);
    
    // IOleWindow methods
    STDMETHOD(GetWindow)                (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp)     (BOOL fEnterMode);

    // IOleInPlaceUIWindow methods
    STDMETHOD(GetBorder)                (LPOLERECT lprectBorder);
    STDMETHOD(RequestBorderSpace)       (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetBorderSpace)           (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetActiveObject)          (
                LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
                LPCTSTR                   lpszObjName);

    // IOleInPlaceFrame methods
    STDMETHOD(InsertMenus)              (
                HMENU hmenuShared, 
                LPOLEMENUGROUPWIDTHS 
                lpMenuWidths);
    STDMETHOD(SetMenu)                  (
                HMENU hmenuShared, 
                HOLEMENU holemenu, 
                HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)              (HMENU hmenuShared);
    STDMETHOD(SetStatusText)            (LPCTSTR lpszStatusText);
    STDMETHOD(EnableModeless)           (BOOL fEnable);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg, WORD wID);

private:
    IOleInPlaceActiveObject *   _pActiveObj;

};

#endif  // __FRAME_HXX__
