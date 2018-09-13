//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       pluginst.hxx
//
//  Contents:   CPluginSite
//
//  This class encapsulates a Netscape Plugin element.  It works very much
//  like the COleSite stuff, but happens to know exactly what the classID
//  of the ocx is and a few other custom behaviors in its dealing with the
//  plugin.ocx.  The plugin.ocx is an ActiveX control which actually loads
//  and communicates with the plugin code according to the Netscape plugin
//  architecture.
//
//----------------------------------------------------------------------------

#ifndef I_PLUGINST_HXX_
#define I_PLUGINST_HXX_
#pragma INCMSG("--- Beg 'pluginst.hxx'")

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#define _hxx_
#include "pluginst.hdl"

MtExtern(CPluginSite)

//+---------------------------------------------------------------------------
//
// CPluginSite
//
//----------------------------------------------------------------------------

class CPluginSite : public COleSite
{
    DECLARE_CLASS_TYPES(CPluginSite, COleSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPluginSite))

    CPluginSite (CDoc *pDoc)
      : super(ETAG_EMBED, pDoc) {}

    //
    // CElement/CSite overrides.
    //
    
    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    //
    // Misc. helpers.
    //

    HRESULT CreateObject();

    virtual void Passivate();

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    virtual HRESULT PostLoad();
    
    #define _CPluginSite_
    #include "pluginst.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    LPTSTR      _pszFullUrl;
    BOOL        _fUsingActiveXControl;
    NO_COPY(CPluginSite);
};

#pragma INCMSG("--- End 'pluginst.hxx'")
#else
#pragma INCMSG("*** Dup 'pluginst.hxx'")
#endif
