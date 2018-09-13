//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispitemplus.hxx
//
//  Contents:   A complex display item supporting background, border, and
//              user clip.
//
//----------------------------------------------------------------------------

#ifndef I_DISPITEMPLUS_HXX_
#define I_DISPITEMPLUS_HXX_
#pragma INCMSG("--- Beg 'dispitemplus.hxx'")

#ifndef X_DISPLEAF_HXX_
#define X_DISPLEAF_HXX_
#include "displeaf.hxx"
#endif

class CDispClipInfo;

MtExtern(CDispItemPlus)


//+---------------------------------------------------------------------------
//
//  Class:      CDispItemPlus
//
//  Synopsis:   A complex display item supporting background, border, and
//              user clip.
//
//----------------------------------------------------------------------------

class CDispItemPlus :
    public CDispLeafNode
{
    DECLARE_DISPNODE(CDispItemPlus, CDispLeafNode, DISPNODETYPE_ITEMPLUS)
    
    friend class CDispNode;
    friend class CDispInteriorNode;
    friend class CDispContainerPlus;
    
protected:
    CDispClient*    _pDispClient;
    LONG            _extra[UNSIZED_ARRAY];  // variable length, must be last
    // 44-84 bytes (8-48 bytes + 36 bytes for CDispLeafNode)
    
protected:
    // object can be created only by CDispRoot, and destructed only from
    // special methods
    void* operator new(size_t cb) {Assert(FALSE); return NULL;}    // do not use
    void* operator new(size_t cb, size_t cbExtra)
            {return(MemAllocClear(Mt(CDispItemPlus), cb + cbExtra));}
    void  operator delete(void* pv) {MemFree(pv);}

                            CDispItemPlus(CDispClient* pDispClient)
                                : CDispLeafNode() 
                                    {Assert(pDispClient != NULL);
                                    _pDispClient = pDispClient;}
                            ~CDispItemPlus() {}

public:
    void                    SetRightToLeft(BOOL fRightToLeft)
                                    {SetBoolean(CDispFlags::s_rightToLeft,fRightToLeft);}
    
    // CDispNode overrides
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll);
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual LONG            GetZOrder() const;
    virtual CDispClient*    GetDispClient() const {return _pDispClient;}

protected:
    // CDispNode overrides
    virtual BOOL            CalculateInView(
                                CDispContext* pContext,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild);
    virtual BOOL            HitTestPoint(CDispHitContext* pContext) const;
    virtual void            GetNodeTransform(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            GetNodeTransform(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            CalcDispInfo(
                                const CRect& rcClip,
                                CDispInfo* pdi) const;
    virtual CDispExtras*    GetExtras() const {return (CDispExtras*) _extra;}
    virtual BOOL            ScrollRectIntoView(
                                const CRect& rc,
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz,
                                COORDINATE_SYSTEM coordSystem,
                                BOOL fScrollBits);
    
    // CDispLeafNode overrides
    virtual void            NotifyInViewChange(
                                CDispContext* pContext,
                                BOOL fResolvedVisible,
                                BOOL fWasResolvedVisible,
                                BOOL fNoRedraw);
    
private:
    void                    DrawBorder(CDispDrawContext* pContext, CDispInfo* pDI);
    void                    DrawBackground(CDispDrawContext* pContext, CDispInfo* pDI);
    void                    DrawContent(CDispDrawContext* pContext, CDispInfo* pDI);
    void                    DrawClientLayer(
                                CDispDrawContext* pContext,
                                const CDispInfo& di,
                                const CRect& rcClip,
                                DWORD dwClientLayer);
    
#if DBG==1
protected:
    virtual void            DumpInfo(HANDLE hFile, long level, long childNumber);
#endif
};


#pragma INCMSG("--- End 'dispitemplus.hxx'")
#else
#pragma INCMSG("*** Dup 'dispitemplus.hxx'")
#endif

