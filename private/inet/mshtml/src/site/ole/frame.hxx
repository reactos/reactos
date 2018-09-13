//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       frame.hxx
//
//  Contents:   CFrameElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_FRAME_HXX_
#define I_FRAME_HXX_
#pragma INCMSG("--- Beg 'frame.hxx'")

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#define _hxx_
#include "frmsite.hdl"

#define _hxx_
#include "frame.hdl"

#define _hxx_
#include "iframe.hdl"

MtExtern(CFrameSite)
MtExtern(CFrameElement)
MtExtern(CIFrameElement)

interface ITargetFrame;

//+---------------------------------------------------------------------------
//
// CFrameSite, base class for CFrameElement and CIFrameElement
//
//----------------------------------------------------------------------------

class CFrameSite : public COleSite
{
    DECLARE_CLASS_TYPES(CFrameSite, COleSite)

    friend class CDBindMethodsFrame;
    
private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameSite))

public:

    CFrameSite (ELEMENT_TAG etag, CDoc *pDoc) :
        super(etag, pDoc)
        { _fHoldingIPrint = FALSE; };

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    //
    // CElement/CSite overrides
    //

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);


#ifndef NO_DATABINDING
    // databinding
    virtual const CDBindMethods *GetDBindMethods();
#endif

    // persisting
    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
            HRESULT PersistFavoritesData(INamedPropertyBag * pINPB, 
                                          BSTR bstrSection);
            void    NoteNavEvent() { if(!_fFirstNavigation) _fFirstNavigation = TRUE;
                                        else  _fHasNavigated = TRUE; };


    //
    // Misc. helpers.
    //

    HRESULT CreateObject();
    HRESULT OnPropertyChange_Src();
    HRESULT OnPropertyChange_Scrolling();
    HRESULT OnPropertyChange_NoResize();
    void    CacheTargetFrame();
    virtual void AfterObjectInstantiation();

    HRESULT GetCDoc(CDoc ** ppDoc);
    HRESULT GetIPrintObject(IPrint ** ppPrint);
    HRESULT GetCurrentFrameURL(TCHAR *pchUrl, DWORD cchUrl);
    HRESULT GetOmWindow(IHTMLWindow2 ** ppOmWindow);
    BOOL    NoResize();

    virtual DWORD   GetProgSinkClass() {return PROGSINK_CLASS_FRAME;}
    virtual DWORD   GetProgSinkClassOther() {return PROGSINK_CLASS_NOREMAIN;}

#if DBG==1
    virtual void VerifyReadyState(long lReadyState);
#endif    

    //
    // Data members
    //
    
    ITargetFrame *      _pTargetFrame;
    unsigned int        _fHoldingIPrint:1;
    unsigned int        _fFirstNavigation:1; // _src changed for first assignment, 
    unsigned int        _fHasNavigated:1;    // for persistence, is this frame's URL original?
    unsigned int        _fInstantiate:1;
    unsigned int        _fFrameBorder:1;
    unsigned int        _fTrustedFrame:1;    // Is this a trusted frame?
    unsigned int        _fWheelMsgSentIn:1;  // We just sent in a WM_MOUSEWHEEL mesg and are waiting
                                             // for the SendMessage() to return. 
    
    #define _CFrameSite_
    #include "frmsite.hdl"
};

//+---------------------------------------------------------------------------
//
// CFrameElement; frames living inside framesets
//
//----------------------------------------------------------------------------

class CFrameElement : public CFrameSite
{
    DECLARE_CLASS_TYPES(CFrameElement, CFrameSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameElement))

    CFrameElement(CDoc *pDoc);

    static  HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);

    #define _CFrameElement_
    #include "frame.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CFrameElement);
};

//+---------------------------------------------------------------------------
//
// CIFrameElement; floating frames
//
//----------------------------------------------------------------------------

class CIFrameElement : public CFrameSite
{
    DECLARE_CLASS_TYPES(CIFrameElement, CFrameSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIFrameElement));

    CIFrameElement(CDoc *pDoc);

    static  HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);
    virtual HRESULT ApplyDefaultFormat (CFormatInfo *pCFI);
    
    HRESULT SetContents(CBuffer2 *pbuf2) { RRETURN(pbuf2->SetCStr(&_cstrContents)); }
    HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    #define _CIFrameElement_
    #include "iframe.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CIFrameElement);
    CStr _cstrContents;
};

#pragma INCMSG("--- End 'frame.hxx'")
#else
#pragma INCMSG("*** Dup 'frame.hxx'")
#endif
