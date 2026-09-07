// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_meta
//      $Keywords:
//
//  $Description:
//          Contains class declaration for the render target iterator used in the
//      implementation of the meta render target primitive drawing functions.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CMetaIterator
{
public:
    CMetaIterator(
        __in_ecount(cRT) MetaData *prgMetaData,
        UINT cRT,
        UINT idxFirstEnabledRT,
        bool fUseRTOffset,
        __in_ecount_opt(1) CDisplaySet const *pDisplaySet,
        __in_ecount_opt(1) CAliasedClip *pAliasedClip,
        __deref_opt_inout_ecount(1) CMilRectF const **ppBoundsToAdjust,
        __in_ecount_opt(1) CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> *pTransform,
        __in_ecount_opt(1) CContextState *pContextState,
        __in_ecount_opt(1) IWGXBitmapSource **ppIBitmapSource
        );

    ~CMetaIterator();

    HRESULT PrepareForIteration();

    HRESULT SetupForNextInternalRT(
        __deref_out_ecount(1) IRenderTargetInternal **ppRTInternalNoAddRef
        );

    MIL_FORCEINLINE UINT CurrentRT() const { return m_idxCurrent; }

    MIL_FORCEINLINE bool MoreIterationsNeeded();

private:

    MIL_FORCEINLINE HRESULT BeginDeviceAdjust(
        UINT idx
        );

private:
    bool m_fMoreIterationsNeeded;

    CContextState *m_pContextState;
    CDisplaySet const *m_pDisplaySet;

    CAdjustTransform m_firstTransformAdjustor;
    CAdjustBounds m_boundsAdjustor;
    CAdjustAliasedClip m_aliasedClipAdjustor;
    CAdjustBitmapSource m_bitmapSourceAdjustor;

    CAdjustTransform *m_pFirstTransformAdjustor;
    bool m_fAdjustBounds;
    CAdjustAliasedClip *m_pAliasedClipAdjustor;
    CAdjustBitmapSource *m_pBitmapSourceAdjustor;

    MetaData *m_prgMetaData;
    UINT m_cRT;

    UINT m_idxCurrent;

#if DBG_ANALYSIS
    CMultiOutSpaceMatrix<CoordinateSpace::Variant> * const m_pDbgToPageOrDeviceTransform;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::MoreIterationsNeeded
//
//  Synopsis:
//      Sets m_idxCurrent to the index of the next enabled RT
//
//------------------------------------------------------------------------------
bool CMetaIterator::MoreIterationsNeeded()
{ 
    do
    {
        m_idxCurrent++;
    } while (m_idxCurrent < m_cRT &&
             !m_prgMetaData[m_idxCurrent].fEnable
             );

    return m_idxCurrent < m_cRT;
}



