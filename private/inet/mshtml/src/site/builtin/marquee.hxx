//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       marquee.hxx
//
//  Contents:   CMarquee.
//
//----------------------------------------------------------------------------

#ifndef I_MARQUEE_HXX_
#define I_MARQUEE_HXX_
#pragma INCMSG("--- Beg 'marquee.hxx'")

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#define _hxx_
#include "marquee.hdl"

class CMarquee;

MtExtern(CMarquee)
MtExtern(CMarqueeTask)

//+---------------------------------------------------------------------------
// CMarqueeTsk
//----------------------------------------------------------------------------
class CMarqueeTask : public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarqueeTask))

    CMarqueeTask(CMarquee* pMarquee);

    virtual void OnRun(DWORD dwTimeout);
    virtual void OnTerminate() {};

private:

    CMarquee * _pMarquee;
};

//+---------------------------------------------------------------------------
//
// CMarquee
//
//----------------------------------------------------------------------------

class CMarquee: public CTxtSite
{

    DECLARE_CLASS_TYPES(CMarquee, CTxtSite)
    friend class CMarqueeTask;
    friend class CDBindMethodsMarquee;
    friend class CMarqueeLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarquee))

    DECLARE_LAYOUT_FNS(CMarqueeLayout)

    CMarquee (ELEMENT_TAG etag, CDoc *pDoc): CTxtSite(etag, pDoc)
    {
#ifdef WIN16
            m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }
    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);

    virtual BOOL    GetAutoSize() const { return TRUE; }

    void InitScrollParams(void);

    //+---------------------------------------------------------------------
    // CTask hook
    //----------------------------------------------------------------------
    void OnRun(DWORD dwTimeout);


    void ExitTree();

    //+-----------------------------------------------------------------------
    // CElement methods
    //------------------------------------------------------------------------
    virtual void    Passivate();
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    //+------------------------------------------------------------------------
    // CElement methods
    //-----------------------------------------------------------------------
    HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
#ifndef NO_DATABINDING
    virtual const CDBindMethods *GetDBindMethods();
#endif

    //+-----------------------------------------------------------------------
    // Helper methods
    //------------------------------------------------------------------------
    void SetMarqueeTask(void);

    //+-----------------------------------------------------------------------
    //  Interface methods
    //------------------------------------------------------------------------
//    HRESULT BUGCALL stop();
//    HRESULT BUGCALL start();
    static const CLSID *            s_apclsidPages[];

    #define _CMarquee_
    #include "marquee.hdl"

    long _lXMargin;
    long _lYMargin;

protected:

    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CMarquee);
    SIZE _sizeScroll;
    htmlMarqueeDirection  _direction;
    long _lXPos;
    long _lYPos;
    long _lLoop;
    long _lScrollBy;
    long _lScrollDelay;
    DWORD _dwOldTimeout;
    BOOL _fStop:1;
    BOOL _fIsVisible:1;
    BOOL _fInVisibleRange:1;
    BOOL _fDone:1;
    BOOL _fSwitch:1;
    BOOL _fSlide:1;
    BOOL _fFirstRun:1;
    BOOL _fToBigForSwitch:1;
    BOOL _fTrueSpeed:1;
    BOOL _fRightToLeft: 1;  // This flag is for the text flow direction.
                            // Languages like Hebrew and Arabic will have this on.

    CMarqueeTask *_pMarqueeTask;
};

#pragma INCMSG("--- End 'marquee.hxx'")
#else
#pragma INCMSG("*** Dup 'marquee.hxx'")
#endif
