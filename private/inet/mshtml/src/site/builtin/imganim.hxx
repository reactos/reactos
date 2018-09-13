//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       imganim.hxx
//
//  Contents:   Manages image animation
//
//----------------------------------------------------------------------------

#ifndef I_IMGANIM_HXX_
#define I_IMGANIM_HXX_
#pragma INCMSG("--- Beg 'imganim.hxx'")

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

CImgAnim * GetImgAnim();
CImgAnim * CreateImgAnim();

MtExtern(CAnimSync)
MtExtern(CAnimSync_aryClients_pv)
MtExtern(CImgAnim)

enum
{
    ANIMSYNC_GETIMGCTX,
    ANIMSYNC_GETHWND,
    ANIMSYNC_TIMER,
    ANIMSYNC_INVALIDATE
};

enum ANIMSTATE
{
    ANIMSTATE_PLAY,
    ANIMSTATE_PAUSE,
    ANIMSTATE_STOP
};

class CAnimSync
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAnimSync))

    CAnimSync() : _aryClients(Mt(CAnimSync_aryClients_pv)) {};

    typedef void (*ASCALLBACK)(void * pvObj, DWORD dwReason,
                               void * pvArg, void **ppvDataOut,
                               IMGANIMSTATE * pAnimState);
    
    BOOL           IsEmpty();
    CImgCtx *      GetImgCtx();

    HRESULT        Register(void * pvObj, DWORD_PTR dwDocId, DWORD_PTR dwImgId,
                            CAnimSync::ASCALLBACK pfnCallback, void * pvArg);
    void           Unregister(void * pvObj);

    void           OnTimer(DWORD * pdwFrameTimeMS);
    void           Update(HWND * pHwnd);
    void           Invalidate();

    IMGANIMSTATE   _imgAnimState;
    DWORD_PTR      _dwDocId;
    DWORD_PTR      _dwImgId;
    ANIMSTATE      _state;
    BOOL           _fInvalidated : 1;

private:

    struct CLIENT {
        void *     pvObj;
        ASCALLBACK pfnCallback;
        void *     pvArg;
    };
    
    CStackDataAry<CLIENT, 1> _aryClients;

};

class CImgAnim
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CImgAnim))

    CImgAnim();
    ~CImgAnim();

    IMGANIMSTATE * GetImgAnimState(LONG lCookie);

    HRESULT        RegisterForAnim(void * pObj, DWORD_PTR dwDocId, DWORD_PTR dwImgId,
                                   CAnimSync::ASCALLBACK callback, void * puData,
                                   LONG * plCookie);
    void           UnregisterForAnim(void * pObj, LONG lCookie);
    void           OnTimer();
    void           ProgAnim(LONG lCookie);

    void           SetAnimState(DWORD_PTR dwDocId, ANIMSTATE state);

    void           StartAnim(LONG lCookie);
    void           StopAnim(LONG lCookie);

private:

    CAnimSync *    GetAnimSync(LONG lCookie);
    void           SetInterval(DWORD dwInterval);

    HRESULT        FindOrCreateAnimSync(DWORD_PTR dwDocId, DWORD_PTR dwImgId, LONG * plCookie, CAnimSync ** ppAnimSync);
    void           CleanupAnimSync(LONG lCookie);


    CPtrAry<CAnimSync *> _aryAnimSync;
    DWORD                _dwInterval;
};

#pragma INCMSG("--- End 'imganim.hxx'")
#else
#pragma INCMSG("*** Dup 'imganim.hxx'")
#endif
