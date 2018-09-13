//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CHtmlDlgHelper
//
//  Contents:   DlgHelpr OC which gets embedded in design time dialogs
//
//  History:    12-Mar-98   raminh  Created
//----------------------------------------------------------------------------
#ifndef _DLGHELPR_H_
#define _DLGHELPR_H_

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

MtExtern(CHtmlDlgHelper)

#define SetErrorInfo( x )   x

MtExtern(CFontNameOptions)
MtExtern(CFontNameOptions_aryFontNames_pv)


class ATL_NO_VTABLE CFontNameOptions : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CFontNameOptions, &IID_IHtmlFontNamesCollection>,
    public IDispatchImpl<IHtmlFontNamesCollection, &IID_IHtmlFontNamesCollection, &LIBID_OPTSHOLDLib>
{
public:
    CFontNameOptions()  { }
    ~CFontNameOptions();
    
    DECLARE_REGISTRY_RESOURCEID(IDR_FONTSOPTION)
    DECLARE_NOT_AGGREGATABLE(CFontNameOptions)

    BEGIN_COM_MAP(CFontNameOptions) 
        COM_INTERFACE_ENTRY(IHtmlFontNamesCollection)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    // IHtmlFontNamesCollection
	STDMETHOD(get_length)(/*[retval, out]*/ long * p);
	STDMETHOD(item)(/*[in]*/ long index, /*[retval, out]*/ BSTR* pBstr);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFontNameOptions));

    // helper and builder functions
    HRESULT   AddName (TCHAR * strFontNamee);
    void      SetSize(long lSize) { _aryFontNames.SetSize(lSize); };

private:
    DECLARE_CDataAry(CAryFontNames, CStr, Mt(Mem), Mt(CFontNameOptions_aryFontNames_pv))
    CAryFontNames _aryFontNames;
};


class ATL_NO_VTABLE CHtmlDlgHelper :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHtmlDlgHelper,&CLSID_HtmlDlgHelper>,
    public CComControl<CHtmlDlgHelper>,
    public IDispatchImpl<IHtmlDlgHelper, &IID_IHtmlDlgHelper, &LIBID_OPTSHOLDLib>,
    public IOleControlImpl<CHtmlDlgHelper>,
    public IOleObjectImpl<CHtmlDlgHelper>,
    public IOleInPlaceActiveObjectImpl<CHtmlDlgHelper>,
    public IOleInPlaceObjectWindowlessImpl<CHtmlDlgHelper>,
    public ISupportErrorInfo
{
public:
    CHtmlDlgHelper()
    {
    	Assert(_pFontNameObj == NULL); // zero based allocator
    }

    ~CHtmlDlgHelper()
    {
    	ReleaseInterface(_pFontNameObj);
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_HTMLDLGHELPER)

    BEGIN_COM_MAP(CHtmlDlgHelper) 
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IHtmlDlgHelper)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()

    BEGIN_MSG_MAP(CHtmlDlgHelper)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    END_MSG_MAP()

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // IHtmlDlgHelper
    STDMETHOD(get_fonts)(/*[retval, out]*/ IHtmlFontNamesCollection* * p);
    STDMETHOD(getCharset)(/*[in]*/ BSTR fontName,/*[retval, out]*/ long* charset);
    STDMETHOD(choosecolordlg)(/*[optional, in]*/ VARIANT initColor,/*[retval, out]*/ long* rgbColor);
    STDMETHOD(savefiledlg)(/*[optional, in]*/ VARIANT initFile,/*[optional, in]*/ VARIANT initDir,/*[optional, in]*/ VARIANT filter,/*[optional, in]*/ VARIANT title,/*[retval, out]*/ BSTR* pathName);
    STDMETHOD(openfiledlg)(/*[optional, in]*/ VARIANT initFile,/*[optional, in]*/ VARIANT initDir,/*[optional, in]*/ VARIANT filter,/*[optional, in]*/ VARIANT title,/*[retval, out]*/ BSTR* pathName);
    STDMETHOD(get_document)(/*[out, retval]*/ LPDISPATCH *pVal);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlDlgHelper));

private:
    VOID        EnsureWrappersLoaded();	

    HRESULT     OpenSaveFileDlg( VARIANTARG initFile, VARIANTARG initDir, 
                                 VARIANTARG filter, VARIANTARG title, 
                                 BSTR *pathName, BOOL fSaveFile, HWND hwndInPlace);

    CComObject<CFontNameOptions> * _pFontNameObj;   // pointer to font name object
};

#endif //_DLGHELPR_H_
