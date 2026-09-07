// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Abstract:
//
//   This file defines a set of classes to execute "pixel snapping".
//  "Pixel snapping" assumes that points involved in rendering are shifted by
//  small distance in order to place things in better correspondence with pixel
//  locations.
//
//   We need this functionality to suppress undesirable effects caused by the
//  basic Avalon idea of device independent scene construction. The matter is
//  that similar primitives, say edges of buttons, can happen to be positioned
//  different way relative to pixel grid, thus making different blur on these
//  edges, and also visibly different sizes of image's details or gaps between
//  them.
//   
//   When rendering every primitive, the amount of shift is controlled by the
//  set of "guidelines". "Guideline" is vertical or horizontal line, which
//  location is defined during scene construction in local coordinates.
//  After converting to device space, the coordinate of every guideline
//  is snapped to nearest boundary between adjacent pixels. Vertical guidelines
//  (also referred to as X-guidelines) are snapped to left or to right, by the
//  distance up to 1/2 of pixel size. Similarly, horizontal (Y-) guidelines
//  are snapped up or down.
//
//   After this for each guideline we have snap value that's the difference
//  between snapped coordinate and original one. These values are used to snap
//  the points involved in rendering. For every point we detect nearest
//  horizontal and vertical guidelines, and apply their snap values to the
//  point.
//
//   The procedure described above shifts the rectangular areas of final picture.
//  It can improve looking of each rectangles, but it can introduce artifacts
//  on edges of adjucent rectangles that can can overlap or obtain gaps.
//  To work it around, some special techniques are used.
//
//   Guidelines based approach outlines above works mostly for static scenes.
//  Animated ones create additional troubles.
//  We could render a sequence of frames each of which is perfect being properly snapped,
//  but, taken together, they would not compose solid cinema.
//  The locations of some elements that supposed to move slow will be snapped
//  so that they will stay several frames at the same position, then will jump to
//  next pixel, stay another several frames, jump again, and so on.
//  Jumps theirselves are not that bad, but the matter is different elements use
//  to jump at different time, depending on fraction part of coordinate in device space.
//  Random jumps change the distances between elements, and this is percepted as
//  distortion (local slang term for this effect is "dance").
//
//   To work it around, we need to switch off pixel snapping when animation is in progress.
//  Also we need to provide smooth transitions between animated and static state.
//
//   This in turn creates yet another trouble, because it is not an easy thing to get
//  animation start/stop signals from scene constucting code. The matter is many scenarios
//  makes some image element moving implicitly, as an effect of automatic layouting when
//  some other element is changing its size. There are also cases when animations
//  are not controlled programmatically, but instead depend on user's actions like
//  mouse manipulating, and so far this moving are not predictable.
//
//   These troubles are solved by introducing dynamic guidelines.
//  in oppose to static one, dynamic guideline not only holds a coordinate value,
//  but also holds small history block reflecting results of rendering in recent frames.
//  It is used to detect animations and provide smooth transitions between animated
//  and statis states.
//
//      
//-----------------------------------------------------------------------------

#pragma once

class CGuidelineCollection;
class CStaticGuidelineCollection;
class CDynamicGuideline;
class CDynamicGuidelineCollection;

class CSnappingFrame;
class CSnappingFrameEmpty;


MtExtern(CSnappingFrame);
MtExtern(CGuidelineCollection);

enum EGuidelineCollectionType
{
    GCT_Static = 0,
    GCT_Dynamic = 1,

    GCT_Force32bits = 0xFFFFFFFF
};

//+------------------------------------------------------------------------
//
// Class:       CGuidelineCollection
//
// Synopsis:
//  Base class for CStaticGuidelineCollection and CDynamicGuidelineCollection.
//
//  The instance of this class lives as the property of composition node
//  or renderdata of composition node.
//  It represents two arrays of guideline coordinates:
//  X-guidelines (vertical) and Y-guidelines (horizontal).
//
//  CStaticGuidelineCollection is just a holder for coordinate arrays.
//  CDynamicGuidelineCollection also holds coordinate arrays, and also
//  holds small history block for each coordinate used for animation detection
//  and subpixel animation.
//
//  Note:
//   Abstract class technique is not used here on purpose, in favor of performance.
//
//-------------------------------------------------------------------------
class CGuidelineCollection
{
public:

    static HRESULT Create(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount(uCountX + uCountY) const float *prgData,
        bool fDynamic,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );

    static HRESULT CreateFromDoubles(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount_opt(uCountX) const double *prgDataX,
        __in_ecount_opt(uCountY) const double *prgDataY,
        BOOL fDynamic,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );
    // ~CGuidelineCollection(); no destructor for this class

    CStaticGuidelineCollection* CastToStatic();
    CDynamicGuidelineCollection* CastToDynamic();

protected:
    CGuidelineCollection(
        EGuidelineCollectionType type,
        UINT16 uCountX,
        UINT16 uCountY
        )
        : m_type(type)
        , m_uCountX(uCountX)
        , m_uCountY(uCountY)
    {}

public:

    //
    // public data accessors
    //
    UINT16 CountX() const { return m_uCountX; }
    UINT16 CountY() const { return m_uCountY; }

private:
    const EGuidelineCollectionType m_type;

protected:
    const UINT16 m_uCountX;
    const UINT16 m_uCountY;
};

//+------------------------------------------------------------------------
//
// Class:       CStaticGuidelineCollection
//
// Synopsis:    Holder for two sorted arrays of floating point numbers.
//
//  The instance of this class lives as the property of composition node.
//  It represents two arrays of guideline coordinates:
//  X-guidelines (vertical) and Y-guidelines (horizontal).
//  The content and life time of the instance is controlled externally.
//
//-------------------------------------------------------------------------
class CStaticGuidelineCollection : public CGuidelineCollection
{
public:
    static HRESULT Create(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount(uCountX + uCountY) const float *prgData,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );

    static HRESULT CreateFromDoubles(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount_opt(uCountX) const double *prgDataX,
        __in_ecount_opt(uCountY) const double *prgDataY,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );
    // ~CGuidelineCollection(); no destructor for this class

private:

    CStaticGuidelineCollection(UINT16 uCountX, UINT16 uCountY)
        : CGuidelineCollection(GCT_Static, uCountX, uCountY)
    {}

    HRESULT StoreRange(
        UINT uIndex,
        UINT uCount,
        __in_ecount(uCount) const float *prgSrc
        );

    HRESULT StoreRangeFromDoubles(
        UINT uIndex,
        UINT uCount,
        __in_ecount(uCount) const double *prgSrc
        );

    //
    // Methods to access data that resides in common memory
    // slot together with the class instance.
    //
    __outro_xcount(m_uCountX + m_uCountY) const float* Data() const { return reinterpret_cast<const float*>(this+1); }
      __out_xcount(m_uCountX + m_uCountY)       float* Data()       { return reinterpret_cast<      float*>(this+1); }

public:

    //
    // public data accessors
    //
    __outro_xcount(m_uCountX) const float* GuidesX() const { return Data()            ; }
    __outro_xcount(m_uCountY) const float* GuidesY() const { return Data() + m_uCountX; }
};


enum EAnimationPhase
{
    APH_Start       = 0,
    APH_Quiet       = 1,
    APH_Animation   = 2,
    APH_Landing     = 3,
    APH_Flight      = 4,
};

//+------------------------------------------------------------------------
//
// Class:       CDynamicGuideline
//
//
//
// Synopsis:    entry in arrays of CDynamicGuidelineCollection.
//
//-------------------------------------------------------------------------
class CDynamicGuideline
{
private:
    friend class CDynamicGuidelineCollection;

    CDynamicGuideline(float rCoord, float rShift)
        : m_rCoord(rCoord)
        , m_rShift(rShift)
    {
        m_uTimeAndPhase = 0;

        // When m_eAnimationPhase == APH_Start, remaining data are considered
        // unknown. Put zeros here to appease prefast.
        m_rLastGivenCoord = 0;
        m_rLastOffset = 0;
    }

    void SubpixelAnimationCorrection(
        float rScale,
        float rOffset,
        UINT32 uCurrentTime,
        __out_ecount(1) bool & fNeedMoreCycles
        );
    void NotifyNonSnappableState(UINT32 uCurrentTime);

public:
    float GetLocalCoordinate() const { return m_rCoord; }
    float GetGivenCoordinate() const {return m_rLastGivenCoord; }
    float GetSnappingOffset() const {return m_rLastOffset; }
    float GetShift() const { return m_rShift; }

private:
    UINT32 GetBumpTime() const
    {
        return m_uTimeAndPhase & sc_uTimeMask;
    }

    void SetBumpTime(UINT32 uTime)
    {
        m_uTimeAndPhase = (m_uTimeAndPhase & ~sc_uTimeMask)
                        | (uTime           &  sc_uTimeMask);
    }

    void SetAnimationPhase(UINT32 uPhase)
    {
        m_uTimeAndPhase = (m_uTimeAndPhase & sc_uTimeMask)
                        | (uPhase << sc_uBitsForTime);
    }

    UINT32 GetAnimationPhase() const
    {
        return m_uTimeAndPhase >> sc_uBitsForTime;
    }

    bool BumpedRecently(UINT32 uCurrentTime) const
    {
        // Use only least sc_uBitsForTime bits of time values.
        // It will cause wrapping around approximately every two weeks.
        // So far, chances to get wrong return are negligible.
        // Even if it'll happen, the worst thing that we'll get is a stutter.
        return ((uCurrentTime - m_uTimeAndPhase) & sc_uTimeMask) < sc_uCriticalTime;
    }

private:
    float const m_rCoord;       // in local space
    float const m_rShift;       // in local space

    //
    // subpixel animation variables
    //

    //
    // Time when guideline coordinate was changed, in milliseconds.
    // We store only least significant bits, so it wraps around every
    // 2^29 msec = 6.21 days.
    //
    // In theory, this infers possible failure in animation detection.
    // If some primitive will stay immovable during 6.2137837 days,
    // then within next sc_uCriticalTime (200 msec) will be displaced
    // by not more than sc_rBigJumpThreshold (3 pixels), this will be
    // mistakenly accepted as animation, so that the primitive will
    // get some blur for next half a second or so.
    //
    // In practice, the probability of such an event is very small
    // and the resulted harm is negligible. We could improve this by
    // spending more bits to store time, or maybe using more sophisticated
    // logic. Not going this way in favor of code simplicity and performance.
    //
    UINT32 m_uTimeAndPhase;


    float m_rLastGivenCoord;    // in device space
    float m_rLastOffset;        // in device space

    static const UINT sc_uBitsForTime = 29;
    static const UINT32 sc_uTimeMask  = (1 << sc_uBitsForTime) - 1;

    static const UINT sc_uCriticalTime = 200; // msec

public:
    static const UINT sc_uTimeDelta = 50; // reschedule period, msec
};

//+------------------------------------------------------------------------
//
//  Class:
//      CDynamicGuidelineCollection
//
//  Synopsis:
//      Holder for two sorted arrays of CDynamicGuideline instances.
//
//  The instance of this class lives as the property of composition node.
//  It represents two arrays of guideline coordinates:
//  X-guidelines (vertical) and Y-guidelines (horizontal).
//  The content and life time of the instance is controlled externally.
//
//-------------------------------------------------------------------------
class CDynamicGuidelineCollection  : public CGuidelineCollection
{
public:
    static HRESULT Create(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount(uCountX + uCountY) const float *prgData,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );

    static HRESULT CreateFromDoubles(
        UINT16 uCountX,
        UINT16 uCountY,
        __in_ecount_opt(uCountX) const double *prgDataX,
        __in_ecount_opt(uCountY) const double *prgDataY,
        __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
        );
    // ~CDynamicGuidelineCollection(); no destructor for this class

    void SubpixelAnimationCorrection(
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat,
        UINT uCurrentTime,
        __out_ecount(1) bool & fNeedMoreCycles
        );

    void NotifyNonSnappableState(UINT uCurrentTime);

private:

    CDynamicGuidelineCollection(UINT16 uCountX, UINT16 uCountY)
        : CGuidelineCollection(GCT_Dynamic, uCountX, uCountY)
    {}

    HRESULT StoreRange(
        UINT16 uIndex,
        UINT16 uCount,
        __in_ecount(2*uCount) const float *prgSrc
        );

    HRESULT StoreRangeFromDoubles(
        UINT16 uIndex,
        UINT16 uCount,
        __in_ecount(2*uCount) const double *prgSrc
        );

    //
    // Methods to access data that resides in common memory
    // slot together with the class instance.
    //
    __outro_xcount(m_uCountX + m_uCountY) const CDynamicGuideline* Data() const { return reinterpret_cast<const CDynamicGuideline*>(this+1); }
      __out_xcount(m_uCountX + m_uCountY)       CDynamicGuideline* Data()       { return reinterpret_cast<      CDynamicGuideline*>(this+1); }

public:

    //
    // public data accessors
    //
    __outro_xcount(m_uCountX) const CDynamicGuideline* GuidesX() const { return Data()            ; }
    __outro_xcount(m_uCountY) const CDynamicGuideline* GuidesY() const { return Data() + m_uCountX; }
};

MIL_FORCEINLINE CStaticGuidelineCollection*
CGuidelineCollection::CastToStatic()
{
    return m_type == GCT_Static
        ? static_cast<CStaticGuidelineCollection*>(this)
        : NULL;
}

MIL_FORCEINLINE CDynamicGuidelineCollection*
CGuidelineCollection::CastToDynamic()
{
    return m_type == GCT_Dynamic
        ? static_cast<CDynamicGuidelineCollection*>(this)
        : NULL;
}

//+------------------------------------------------------------------------
//
// Class:       CSnappingFrame
//
// Synopsis: Unfolded representation of CGuidelineCollection that
//           implements pixel snapping procedure for points.
//
//   The instance of this class exists in the stack attached to rendering
//  context. When some guideline collection enters into play, we create
//  new frame and push it into stack so that it becomes current one.
//  When this collection goes out of scope, we pop the stack, thus
//  restoring previous state.
//
//   The data in the frame are taken from CGuidelineCollection and
//  transformed to device space. Some redundant data is stored here
//  to optimize calculations.
//
//-------------------------------------------------------------------------
class CSnappingFrame
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSnappingFrame));

    static HRESULT PushFrame(
        __inout_ecount_opt(1) CGuidelineCollection *pGuidelineCollection,
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat,
        UINT uCurrentTime,
        __out_ecount(1) bool & fNeedMoreCycles,
        bool fSuppressAnimation,
        __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
        );

    static void PopFrame(
        __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
        );

    bool IsEmpty() const
    {
        return m_uCountX == 0 && m_uCountY == 0;
    }

    CSnappingFrameEmpty* CastToEmpty();

    bool IsSimple() const
    {
        return m_uCountX <= 1 && m_uCountY <= 1;
    }

    void SnapPoint(
        __inout_ecount(1) MilPoint2F &point
        );

    void SnapTransform(
        __inout_ecount(1) CBaseMatrix &mat
        );

private:
    friend class CSnappingFrameEmpty;

    CSnappingFrame(UINT16 uCountX, UINT16 uCountY)
        : m_uCountX(uCountX)
        , m_uCountY(uCountY)
    {
        m_pNext = NULL;
    }

    ~CSnappingFrame()
    {
        Assert(m_pNext == NULL);
    }

    void HookupToStack(
        __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
        )
    {
        Assert(m_pNext == NULL);
        m_pNext = *ppSnappingStack;
        *ppSnappingStack = this;
    }

    void UnhookFromStack(
        __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
        )
    {
        Assert(*ppSnappingStack == this);
        *ppSnappingStack = m_pNext;
        m_pNext = NULL;
    }

    void PushFrameStatic(
        __inout_ecount(1) CStaticGuidelineCollection *pGuidelineCollection,
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat
        );

    void PushFrameDynamic(
        __inout_ecount(1) CDynamicGuidelineCollection *pGuidelineCollection,
        bool fSuppressAnimation,
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat
        );

    static void StoreRangeStatic(
        __out_ecount(2*uCount) float *prgDst,
        __in_ecount(uCount) const float *prgSrc,
        __range(>=, 1) UINT16 uCount,
        float rScale,
        float rOffset
        );

    static void StoreRangeDynamic(
        __out_ecount(2*uCount) float *prgDst,
        __range(>=, 1) UINT16 uCount,
        __in_ecount(uCount) const CDynamicGuideline *prgSrc,
        bool fSuppressAnimation,
        float rScale,
        float rOffset
        );

    static void SnapCoordinate(
        __inout_ecount(1) float &z,
        UINT uCount,
        __in_ecount(2*uCount) const float* prgGuides
        );

    //
    // Methods to access data that resides in common memory
    // slot together with the class instance.
    //

    __out_xcount(2*(m_uCountX + m_uCountY)) float* Data() { return reinterpret_cast<float*>(this+1); }

    __out_xcount(2*m_uCountX) float* GuidesX()  { return Data(); }
    __out_xcount(2*m_uCountY) float* GuidesY()  { return Data() + 2*static_cast<UINT32>(m_uCountX); }

private:
    const UINT16 m_uCountX;
    const UINT16 m_uCountY;

    // pointer to next frame in stack
    CSnappingFrame *m_pNext;

    // The threshold, in pixels, to consider neighboring guideline
    // to be a pair that should be snapped synchronously.
    static const UINT sc_uPairingThreshold = 3;
};


//+------------------------------------------------------------------------
//
// Class:       CSnappingFrameEmpty
//
// Synopsis:
//  The special form of CSnappingFrame to represent empty set of guidelines.
//
//  The optimization idea is to avoid allocating memory for many empty
//  frames, instead create just one with built-in counter.
//
//  The instance of CSnappingFrameEmpty can be distinguished from
//  CSnappingFrame by zeros in both m_uCountX and m_uCountY.
//
//-------------------------------------------------------------------------
class CSnappingFrameEmpty : public CSnappingFrame
{
private:
    friend class CSnappingFrame;

    CSnappingFrameEmpty() : CSnappingFrame(0,0)
    {
        m_uIdlePushCount = 1;
    }

    static HRESULT PushFrame(
        __deref_opt_inout_ecount(1) CSnappingFrame **ppSnappingStack
        );

private:
    UINT m_uIdlePushCount;
};

MIL_FORCEINLINE CSnappingFrameEmpty*
CSnappingFrame::CastToEmpty()
{
    Assert(IsEmpty());
    return static_cast<CSnappingFrameEmpty*>(this);
}


