//+----------------------------------------------------------------------------
//
// File:        OLELYT.HXX
//
// Contents:    COleLayout, CFrameLayout
//
// Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_OLELYT_HXX_
#define I_OLELYT_HXX_
#pragma INCMSG("--- Beg 'olelyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

MtExtern(COleLayout)

class COleLayout : public CLayout
{
public:

    typedef CLayout super;

    COleLayout(CElement * pElementLayout) : super(pElementLayout)
    {
        _sizelLast.cx = _sizelLast.cy = -1;
    }
    virtual ~COleLayout() {};

    virtual void    Reset(BOOL fForce);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COleLayout))

    virtual DWORD CalcSize(
                    CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    
    virtual void HandleViewChange(
                    DWORD          flags, 
                    const RECT *   prcClient,
                    const RECT *   prcClip,
                    CDispNode *    pDispNode);
    
    virtual BOOL ProcessDisplayTreeTraversal(void *pClientData);
    
    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual void GetMarginInfo(CParentInfo * ppri,
                               LONG        * plLeftMargin,
                               LONG        * plTopMargin,
                               LONG        * plRightMargin,
                               LONG        * plBottomMargin);

    virtual HRESULT OnFormatsChange(DWORD);

    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    virtual HRESULT DragEnter(
                    IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    virtual HRESULT Drop(
                    IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect);
    virtual HRESULT DragLeave();

#if 0
    virtual void    DragHide();
#endif
    
    CRect           _rcWnd;             // current window rect in global coords.
                                        
    SIZEL           _sizelLast;         // [T] - last sizel sent to control

    unsigned        _fIsInView : 1;     // TRUE if site is in view (really
                                        // visible on screen)

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'olelyt.hxx'")
#else
#pragma INCMSG("*** Dup 'olelyt.hxx'")
#endif
