//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       BGsound.hxx
//
//  Contents:   CBGsound
//
//----------------------------------------------------------------------------

#ifndef I_EBGSOUND_HXX_
#define I_EBGSOUND_HXX_
#pragma INCMSG("--- Beg 'ebgsound.hxx'")

class CIEMediaPlayer;

#define _hxx_
#include "bgsound.hdl"

MtExtern(CBGsound)

//+---------------------------------------------------------------------------
//
// CBGsound implementation for interfacing to ActiveMovie
//
//----------------------------------------------------------------------------

class CBGsound : public CElement
{
    DECLARE_CLASS_TYPES(CBGsound, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBGsound))

    CBGsound(CDoc *pDoc)
        : CElement(ETAG_BGSOUND, pDoc) {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
    virtual HRESULT Init2(CInit2Context * pContext);
    virtual void    Notify(CNotification *pNF);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);
    virtual void    Passivate(void);

    HRESULT EnterTree();

    void    SetBitsCtx(CBitsCtx * pBitsCtx);
    void    SetAudio();
    void    OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CBGsound *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

    #define _CBGsound_
    #include "bgsound.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    BOOL            _fIsInPlace:1;
    BOOL            _fStopped:1;
    CIEMediaPlayer  *_pSoundObj;
    CBitsCtx        *_pBitsCtx;

    NO_COPY(CBGsound);

};

#pragma INCMSG("--- End 'ebgsound.hxx'")
#else
#pragma INCMSG("*** Dup 'ebgsound.hxx'")
#endif
