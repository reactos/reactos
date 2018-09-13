//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       frameset.hxx
//
//  Contents:   CFrameSetSite class definition
//
//----------------------------------------------------------------------------

#ifndef I_FRAMESET_HXX_
#define I_FRAMESET_HXX_
#pragma INCMSG("--- Beg 'frameset.hxx'")

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#define _hxx_
#include "frameset.hdl"

MtExtern(CNoFramesElement)
MtExtern(CFrameSetSite)

class CFrameSetLayout;

//+---------------------------------------------------------------------------
//
//  Class:      CNoFramesElement (nfe)
//
//  Purpose:    Captures the text between NOFRAMES tags
//
//----------------------------------------------------------------------------

class CNoFramesElement : public CElement
{
    friend class CFrameSetSiteParser;

    typedef CElement super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CNoFramesElement))

    CNoFramesElement(CDoc *pDoc)
         : CElement(ETAG_NOFRAMES, pDoc) { _pDoc = pDoc; }

    virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    DECLARE_CLASSDESC_MEMBERS;

protected:
    CDoc *_pDoc;
    CStr  _cstrNoFrames;
};


class CFrameElement;

//+---------------------------------------------------------------------------
//
//  Class:      CFrameSetSite (cfs)
//
//  Purpose:    Site class that implements the <FRAMESET> tag
//
//----------------------------------------------------------------------------

#if defined(DYNAMICARRAY_NOTSUPPORTED)
#define CFRAMESETSITE_ACCELLIST_SIZE    5
#endif

class CFrameSetSite : public CSite
{
    DECLARE_CLASS_TYPES(CFrameSetSite, CSite)

    friend class CFrameSetSiteParser;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameSetSite))

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static int iPixelFrameHighlightWidth;
    static int iPixelFrameHighlightBuffer;

    CFrameSetSite(CDoc *pDoc);
    ~CFrameSetSite() { Assert(!_pNFE); }

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);
    
    virtual void    Notify(CNotification *pNF);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    BOOL IsOverflowFrame(CElement *pElement);

    //-------------------------------------------------------------------------
    //
    // Layout related functions
    //
    //-------------------------------------------------------------------------

    // Create the layout object to be associated with the current element
    //
    DECLARE_LAYOUT_FNS(CFrameSetLayout)
            
    int ExpectedFrames();

    #define _CFrameSetSite_
    #include "frameset.hdl"

    static const CLSID *                s_apclsidPages[];

    static ACCELS s_AccelsFrameSetSiteDesign;
    static ACCELS s_AccelsFrameSetSiteRun;

    DECLARE_CLASSDESC_MEMBERS;

    int GetFrameSpacing();
    void DoNetscapeMappings();

    HRESULT GetOmWindow (LONG nFrame, IHTMLWindow2 ** ppOmWindow);
    BOOL    GetFrameFlat (LONG nIndex, LONG * pcFrames, CFrameElement ** ppFrame);
    HRESULT GetFramesCount (LONG * pcFrames);

    BSTR GetPersistID(BSTR bstrParentName = NULL);

    CColorValue BorderColorAttribute();
    void FrameBorderAttribute(BOOL fFrameBorder, BOOL fDefined = FALSE);
    CUnitValue BorderAttribute();
    CUnitValue FrameSpacingAttribute();

    virtual DWORD GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll);
    virtual HRESULT ApplyDefaultFormat(CFormatInfo * pCFI);

    void WaitForRecalc();

    unsigned             _fFrameBorder : 1;

protected:

    CElement             * _pNFE;    // AddRef'd pointer to NOFRAMES tag
                                     // (if we have one)
};




#pragma INCMSG("--- End 'frameset.hxx'")
#else
#pragma INCMSG("*** Dup 'frameset.hxx'")
#endif
