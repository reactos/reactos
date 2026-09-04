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
//          Contains class declaration for CAdjustAliasedClip class used in the
//      implementation of the meta render target
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAdjustAliasedClip
//
//  Synopsis:
//      Sets up the realized clip in the context state
//
//------------------------------------------------------------------------------

class CAdjustAliasedClip : public CAdjustObject<CAdjustAliasedClip>
{
public:
    MIL_FORCEINLINE CAdjustAliasedClip(
        __inout_ecount_opt(1) CAliasedClip *pAliasedClip
        );

    MIL_FORCEINLINE ~CAdjustAliasedClip();

    MIL_FORCEINLINE HRESULT BeginPrimitiveAdjustInternal(
        __out_ecount(1) bool *pfRequiresAdjustment
        );
    MIL_FORCEINLINE HRESULT BeginDeviceAdjustInternal(
        __in_ecount(idx+1) MetaData *prgMetaData,
        UINT idx
        );
    MIL_FORCEINLINE void EndPrimitiveAdjustInternal();


private:
    CAliasedClip *m_pClipToAdjust;

    CAliasedClip m_aliasedClipOrig;

#if DBG
public:
    // inlined to avoid cpp file
    inline void DbgSaveState();
    inline void DbgCheckState() const;

private:
    CAliasedClip m_DBGAliasedClipSaved;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::CAdjustAliasedClip
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CAdjustAliasedClip::CAdjustAliasedClip(
    __inout_ecount_opt(1) CAliasedClip *pAliasedClip
    ) : m_aliasedClipOrig(NULL)
#if DBG
    , m_DBGAliasedClipSaved(NULL)
#endif
{
    m_pClipToAdjust = pAliasedClip;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::~CAdjustAliasedClip
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CAdjustAliasedClip::~CAdjustAliasedClip()
{
    EndPrimitiveAdjust();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::BeginPrimitiveAdjustInternal
//
//  Synopsis:
//      realizes the clip data and stores the render target clip
//
//------------------------------------------------------------------------------
HRESULT CAdjustAliasedClip::BeginPrimitiveAdjustInternal(
    __out_ecount(1) bool *pfRequiresAdjustment
    )
{
    HRESULT hr = S_OK;

    if (   m_pClipToAdjust
        && !m_pClipToAdjust->IsNullClip()
        && !m_pClipToAdjust->IsEmptyClip()
        )
    {
        m_aliasedClipOrig = *m_pClipToAdjust;
    
        *pfRequiresAdjustment = true;
    }
    else
    {
        *pfRequiresAdjustment = false;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::BeginDeviceAdjustInternal
//
//  Synopsis:
//      Modifies the render target clip to the realized clip data. 
//      Modifications will be undone in EndPrimitiveAdjustInternal
//
//------------------------------------------------------------------------------
HRESULT CAdjustAliasedClip::BeginDeviceAdjustInternal(
    __in_ecount(idx+1) MetaData *prgMetaData,
    UINT idx
    )
{
    Assert(!m_aliasedClipOrig.IsNullClip());
    Assert(!m_aliasedClipOrig.IsEmptyClip());

    CMilRectF rcAliasedClipF;
    m_aliasedClipOrig.GetAsCMilRectF(&rcAliasedClipF);

    rcAliasedClipF.Offset(static_cast<FLOAT>(-prgMetaData[idx].ptInternalRTOffset.x),
                          static_cast<FLOAT>(-prgMetaData[idx].ptInternalRTOffset.y));

    *m_pClipToAdjust = CAliasedClip(&rcAliasedClipF);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::EndPrimitiveAdjustInternal
//
//  Synopsis:
//      Restores render target clip back to its original value
//
//------------------------------------------------------------------------------
void CAdjustAliasedClip::EndPrimitiveAdjustInternal()
{
    *m_pClipToAdjust = m_aliasedClipOrig;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::DbgSaveState
//
//  Synopsis:
//      Make sure that we adjusted the clip state
//
//------------------------------------------------------------------------------
void CAdjustAliasedClip::DbgSaveState()
{
    m_DBGAliasedClipSaved = *m_pClipToAdjust;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustAliasedClip::DbgCheckState
//
//  Synopsis:
//      Check to make sure that the clip state was not changed by the internal
//      render target and friends
//
//------------------------------------------------------------------------------
void CAdjustAliasedClip::DbgCheckState() const
{
    Assert(memcmp(m_pClipToAdjust, &m_DBGAliasedClipSaved, sizeof(m_DBGAliasedClipSaved)) == 0);
}
#endif




