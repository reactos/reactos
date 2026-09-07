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
//          Contains class declaration for CAdjustObject class used in the
//      implementation of the meta render target
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAdjustObject
//
//  Synopsis:
//      Base class for adjustment classes used in the meta render target
//      implementation.
//
//------------------------------------------------------------------------------

template <class TAdjustor>
class CAdjustObject
{
protected:
    MIL_FORCEINLINE CAdjustObject();
    MIL_FORCEINLINE ~CAdjustObject();

public:

    MIL_FORCEINLINE HRESULT BeginPrimitiveAdjust(
        __out_ecount(1) bool *pfRequiresAdjustment
        );
    MIL_FORCEINLINE HRESULT BeginDeviceAdjust(
        __in_ecount(idx+1) MetaData *prgMetaData,
        UINT idx
        );

    // These methods call the subclass implementation in DBG and
    // do nothing in a fre build
    MIL_FORCEINLINE void DbgSaveState();
    MIL_FORCEINLINE void DbgCheckState();

    bool GetEndPrimitiveNeeded() const
        { return m_fEndPrimitiveNeeded; }

protected:
    MIL_FORCEINLINE void EndPrimitiveAdjust();

private:
    bool m_fEndPrimitiveNeeded;
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::CAdjustObject
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
template <class TAdjustor>
CAdjustObject<TAdjustor>::CAdjustObject()
{
    m_fEndPrimitiveNeeded = FALSE;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::~CAdjustObject
//
//  Synopsis:
//      dctor
//
//------------------------------------------------------------------------------
template <class TAdjustor>
CAdjustObject<TAdjustor>::~CAdjustObject()
{
    Assert(!m_fEndPrimitiveNeeded);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::BeginPrimitiveAdjust
//
//  Synopsis:
//      Begins the adjustment process, saving any data that will need to be
//      restored later in member variables.
//
//------------------------------------------------------------------------------
template <class TAdjustor>
HRESULT CAdjustObject<TAdjustor>::BeginPrimitiveAdjust(
    __out_ecount(1) bool *pfRequiresAdjustment
    )
{
    HRESULT hr = S_OK;

    TAdjustor *pAdjustor = static_cast<TAdjustor *>(this);

    IFC(pAdjustor->BeginPrimitiveAdjustInternal(
        pfRequiresAdjustment
        ));

    if (*pfRequiresAdjustment)
    {
        DbgSaveState();

        m_fEndPrimitiveNeeded = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::BeginDeviceAdjust
//
//  Synopsis:
//      Performs the adjustment. Any data that is changed here should be
//      restored in EndPrimitiveAdjust
//
//------------------------------------------------------------------------------
template <class TAdjustor>
HRESULT CAdjustObject<TAdjustor>::BeginDeviceAdjust(
    __in_ecount(idx+1) MetaData *prgMetaData,
    UINT idx
    )
{
    HRESULT hr = S_OK;

    TAdjustor *pAdjustor = static_cast<TAdjustor *>(this);

    Assert(m_fEndPrimitiveNeeded);

    DbgCheckState();

    IFC(pAdjustor->BeginDeviceAdjustInternal(prgMetaData, idx));
    
    DbgSaveState();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::EndPrimitiveAdjust
//
//  Synopsis:
//      Restores the adjusted variables back to their orginal values
//
//------------------------------------------------------------------------------
template <class TAdjustor>
void CAdjustObject<TAdjustor>::EndPrimitiveAdjust()
{
    if (m_fEndPrimitiveNeeded)
    {
        TAdjustor *pAdjustor = static_cast<TAdjustor *>(this);

        DbgCheckState();
    
        pAdjustor->EndPrimitiveAdjustInternal();

        m_fEndPrimitiveNeeded = FALSE;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::DbgSaveState
//
//  Synopsis:
//      calls the save state function on the adjustor class
//
//------------------------------------------------------------------------------
template <class TAdjustor>
void CAdjustObject<TAdjustor>::DbgSaveState()
{
#if DBG
    TAdjustor *pAdjustor = static_cast<TAdjustor *>(this);

    pAdjustor->DbgSaveState();
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustObject::DbgCheckState
//
//  Synopsis:
//      calls the check state function on the adjustor class
//
//------------------------------------------------------------------------------
template <class TAdjustor>
void CAdjustObject<TAdjustor>::DbgCheckState()
{
#if DBG
    TAdjustor *pAdjustor = static_cast<TAdjustor *>(this);

    pAdjustor->DbgCheckState();
#endif
}





