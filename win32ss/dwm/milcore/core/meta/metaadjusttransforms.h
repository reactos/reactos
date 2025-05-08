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
//          Contains class declaration for CAdjustTransform class used in the
//      implementation of the meta render target
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      PVariantInMultiOutSpaceMatrix
//
//  Synopsis:
//      Helper class to act as CMultiOutSpaceMatrix<CoordinateSpace::Variant> *.
//      It will allow assignment from any CMultiOutSpaceMatrix<> *.  This class
//      Is needed because C++ doesn't allow classes to define cast operators for
//      class pointers (as far as JasonHa knows).
//
//------------------------------------------------------------------------------

class PVariantInMultiOutSpaceMatrix
{
public:
    template<typename AnyCoordSpace>
    PVariantInMultiOutSpaceMatrix (CMultiOutSpaceMatrix<AnyCoordSpace> *pGenericMultiOutSpaceMatrix)
    {
        C_ASSERT(sizeof(CMultiOutSpaceMatrix<AnyCoordSpace>) ==
                 sizeof(CMultiOutSpaceMatrix<CoordinateSpace::Variant>));
        m_pmatVariantInMultiOutSpace =
            reinterpret_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> *>
            (pGenericMultiOutSpaceMatrix);
    }

    operator CMultiOutSpaceMatrix<CoordinateSpace::Variant> * () const
    {
        return m_pmatVariantInMultiOutSpace;
    }

private:
    CMultiOutSpaceMatrix<CoordinateSpace::Variant> * m_pmatVariantInMultiOutSpace;
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CAdjustTransform
//
//  Synopsis:
//      Adjusts a transform by the amount necessary to translate from from meta
//      rt space to internal rt space.
//
//------------------------------------------------------------------------------

class CAdjustTransform : public CAdjustObject<CAdjustTransform>
{
public:
    MIL_FORCEINLINE CAdjustTransform(
        __in_ecount_opt(1) PVariantInMultiOutSpaceMatrix pTransform
        );

    MIL_FORCEINLINE ~CAdjustTransform();

    MIL_FORCEINLINE HRESULT BeginPrimitiveAdjustInternal(
        __out_ecount(1) bool * const pfRequiresAdjustment
        );
    MIL_FORCEINLINE HRESULT BeginDeviceAdjustInternal(
        __in_ecount(idx+1) MetaData const *prgMetaData,
        UINT idx
        );
    MIL_FORCEINLINE void EndPrimitiveAdjustInternal();

private:
    CMultiOutSpaceMatrix<CoordinateSpace::Variant> * const m_pTransform;

    MilPoint2F m_ptTranslate;

#if DBG
public:
    // inlined to avoid cpp file
    inline void DbgSaveState();
    inline void DbgCheckState();

private:
    MilPoint2F m_ptDBGTranslate;
#endif
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::CAdjustTransform
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CAdjustTransform::CAdjustTransform(
    __in_ecount_opt(1) PVariantInMultiOutSpaceMatrix pTransform
    )
    : m_pTransform(pTransform)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::CAdjustTransform
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CAdjustTransform::~CAdjustTransform()
{
    EndPrimitiveAdjust();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::BeginPrimitiveAdjustInternal
//
//  Synopsis:
//      saves the transform to member variables
//
//------------------------------------------------------------------------------
HRESULT CAdjustTransform::BeginPrimitiveAdjustInternal(
    __out_ecount(1) bool * const pfRequiresAdjustment
    )
{
    if (m_pTransform)
    {
        m_ptTranslate.X = m_pTransform->GetDx();
        m_ptTranslate.Y = m_pTransform->GetDy();

        m_pTransform->DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();

        *pfRequiresAdjustment = true;
    }
    else
    {
        *pfRequiresAdjustment = false;
    }

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::BeginDeviceAdjustInternal
//
//  Synopsis:
//      Modifies the transform Modifications will be undone in
//      EndPrimitiveAdjustInternal
//
//------------------------------------------------------------------------------
HRESULT CAdjustTransform::BeginDeviceAdjustInternal(
    __in_ecount(idx+1) MetaData const *prgMetaData,
    UINT idx
    )
{
    m_pTransform->SetDx(m_ptTranslate.X - prgMetaData[idx].ptInternalRTOffset.x);
    m_pTransform->SetDy(m_ptTranslate.Y - prgMetaData[idx].ptInternalRTOffset.y);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::EndPrimitiveAdjustInternal
//
//  Synopsis:
//      Restores transform back to original values
//
//------------------------------------------------------------------------------
void CAdjustTransform::EndPrimitiveAdjustInternal()
{
    m_pTransform->SetDx(m_ptTranslate.X);
    m_pTransform->SetDy(m_ptTranslate.Y);
    m_pTransform->DbgChangeToSpace<CoordinateSpace::Device,CoordinateSpace::PageInPixels>();
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::DbgSaveState
//
//  Synopsis:
//      Saves the transform to another location for verification purposes
//
//------------------------------------------------------------------------------
void CAdjustTransform::DbgSaveState()
{
    m_ptDBGTranslate.X = m_pTransform->GetDx();
    m_ptDBGTranslate.Y = m_pTransform->GetDy();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustTransform::DbgCheckState
//
//  Synopsis:
//      Check to make sure the transform was not changed by any other class
//
//------------------------------------------------------------------------------
void CAdjustTransform::DbgCheckState()
{
    // Assert that none of the rendering calls modify the transform
    Assert(BitwiseEquals(m_ptDBGTranslate.X, m_pTransform->GetDx()));
    Assert(BitwiseEquals(m_ptDBGTranslate.Y, m_pTransform->GetDy()));
}
#endif




