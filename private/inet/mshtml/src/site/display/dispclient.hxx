//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispclient.hxx
//
//  Contents:   Abstract base class for display tree client objects which
//              perform drawing and fine-grained hit testing.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCLIENT_HXX_
#define I_DISPCLIENT_HXX_
#pragma INCMSG("--- Beg 'dispclient.hxx'")

class CDispNode;
class CDispSurface;
class CDispFilter;

enum VIEWCHANGEFLAGS
{
    VCF_INVIEW          = 0x01,
    VCF_INVIEWCHANGED   = 0x02,
    VCF_POSITIONCHANGED = 0x04,
    VCF_NOREDRAW        = 0x08
};

//+---------------------------------------------------------------------------
//
//  Class:      CDispClient
//
//  Synopsis:   Abstract base class for display tree client objects which
//              perform drawing and fine-grained hit testing.
//
//----------------------------------------------------------------------------

class CDispClient
{
public:
    virtual void            GetOwner(
                                CDispNode *pDispNode,
                                void ** ppv) = 0;
    
    virtual void            DrawClient(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientBackground(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientBorder(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientScrollbar(
                                int whichScrollbar,
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                LONG contentSize,
                                LONG containerSize,
                                LONG scrollAmount,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual void            DrawClientScrollbarFiller(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    virtual BOOL            HitTestContent(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestFuzzy(
                                const POINT *pptHitInParentCoords,
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestScrollbar(
                                int whichScrollbar,
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestScrollbarFiller(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData) = 0;

    virtual BOOL            HitTestBorder(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData) = 0;
                                          
    virtual BOOL            ProcessDisplayTreeTraversal(
                                void *pClientData) = 0;
    
    // called only for z ordered items
    virtual LONG            GetZOrderForSelf() = 0;
    
    virtual LONG            GetZOrderForChild(
                                void *cookie) = 0;

    virtual LONG            CompareZOrder(
                                CDispNode* pDispNode1,
                                CDispNode* pDispNode2) = 0;
    
    // filter interface
    virtual CDispFilter*    GetFilter() = 0;
    
    // called only for "position aware" or "in-view aware" items
    virtual void            HandleViewChange(
                                DWORD flags,
                                const RECT* prcClient,  // global coordinates
                                const RECT* prcClip,    // global coordinates
                                CDispNode* pDispNode) = 0;
    
    // provide opportunity for client to fire_onscroll event
    virtual void            NotifyScrollEvent(
                                RECT *  prcScroll,
                                SIZE *  psizeScrollDelta) = 0;


    // custom drawn layers (in particular, for behaviors)

    virtual DWORD           GetClientLayersInfo(CDispNode *pDispNodeFor) = 0;

    virtual void            DrawClientLayers(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags) = 0;

    // debug stuff

#if DBG==1
    virtual void            DumpDebugInfo(
                                HANDLE hFile,
                                long level,
                                long childNumber,
                                CDispNode *pDispNode,
                                void* cookie) {}
#endif
};



#pragma INCMSG("--- End 'dispclient.hxx'")
#else
#pragma INCMSG("*** Dup 'dispclient.hxx'")
#endif

