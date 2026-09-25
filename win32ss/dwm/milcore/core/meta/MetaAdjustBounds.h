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
//      Definition for bounds adjustment class CAjustBounds
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      CAdjustBounds
//
//  Synopsis:
//      Sets up device specific bounds per device
//
//------------------------------------------------------------------------------

class CAdjustBounds
{
public:
    MIL_FORCEINLINE CAdjustBounds(
        __deref_opt_inout_ecount(1) CMilRectF const **ppBoundsToAdjust
        );

    MIL_FORCEINLINE ~CAdjustBounds();

    MIL_FORCEINLINE bool BeginPrimitiveAdjust(
        ) const;

    MIL_FORCEINLINE void BeginDeviceAdjust(
        __in_ecount(idx+1) MetaData const *prgMetaData,
        UINT idx
        );

private:

    bool const m_fBoundsNeedAdjustment;  // TRUE if there are bounds to be
                                         // adjusted

    CMilRectF const m_boundsOrig;      // Copy of bounds to adjust

    CMilRectF m_boundsForDevice;      // Rect to store adjustments in --
                                         // It's address is handed back to
                                         // caller to be use for each primitive
                                         // call.

};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBounds::CAdjustBounds
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CAdjustBounds::CAdjustBounds(
    __deref_opt_inout_ecount(1) CMilRectF const **ppBoundsToAdjust
    ) :
    m_fBoundsNeedAdjustment(
           ppBoundsToAdjust != NULL && !(*ppBoundsToAdjust)->IsEmpty()
           ),
    // Remember original bounds
    m_boundsOrig(m_fBoundsNeedAdjustment ?
                 **ppBoundsToAdjust :
                 m_boundsForDevice.sc_rcEmpty)
{
//    m_fBoundsNeedAdjustment = (ppBoundsToAdjust != NULL);

    if (m_fBoundsNeedAdjustment)
    {
        Assert((*ppBoundsToAdjust)->HasValidValues());

//        // Remember original bounds
//        m_boundsOrig = **ppBoundsToAdjust;

        // Hand back address to use per-primitive
        *ppBoundsToAdjust = &m_boundsForDevice;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBounds::~CAdjustBounds
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CAdjustBounds::~CAdjustBounds()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBounds::BeginPrimitiveAdjust
//
//  Synopsis:
//      returns whether BeginDeviceAdjust needs called
//
//------------------------------------------------------------------------------
bool CAdjustBounds::BeginPrimitiveAdjust(
    ) const
{
    return m_fBoundsNeedAdjustment;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBounds::BeginDeviceAdjust
//
//  Synopsis:
//      Sets up the bounds for a specific device
//
//------------------------------------------------------------------------------
void CAdjustBounds::BeginDeviceAdjust(
    __in_ecount(idx+1) MetaData const *prgMetaData,
    UINT idx
    )
{
    // Create monitor rectangle that is relative to the client
    // rectangle.  This is in meta render target space.

    CMilRectF rcMonitor(
        static_cast<FLOAT>(prgMetaData[idx].rcLocalDeviceRenderBounds.left),
        static_cast<FLOAT>(prgMetaData[idx].rcLocalDeviceRenderBounds.top),
        static_cast<FLOAT>(prgMetaData[idx].rcLocalDeviceRenderBounds.right),
        static_cast<FLOAT>(prgMetaData[idx].rcLocalDeviceRenderBounds.bottom),
        LTRB_Parameters
        );

    //
    // Intersect monitor bounds with original bounds
    //

    m_boundsForDevice = m_boundsOrig;

    if (m_boundsForDevice.Intersect(rcMonitor))
    {
        // Translate the rectangle from meta render target space into
        // the internal render target's coordinate space
        m_boundsForDevice.left   -= prgMetaData[idx].ptInternalRTOffset.x;
        m_boundsForDevice.top    -= prgMetaData[idx].ptInternalRTOffset.y;
        m_boundsForDevice.right  -= prgMetaData[idx].ptInternalRTOffset.x;
        m_boundsForDevice.bottom -= prgMetaData[idx].ptInternalRTOffset.y;
    }
}




