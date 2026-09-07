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
//          Contains class declaration for CAdjustBitmapSource class used in the
//      implementation of the meta render target
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAdjustBitmapSource
//
//  Synopsis:
//      Adjusts bitmap source, replacing intermediate render targets with their
//      device specific bitmaps.
//
//------------------------------------------------------------------------------

class CAdjustBitmapSource : public CAdjustObject<CAdjustBitmapSource>
{
public:
    MIL_FORCEINLINE CAdjustBitmapSource(
        __in_ecount_opt(1) IWGXBitmapSource **ppIBitmapSource
        );
    MIL_FORCEINLINE ~CAdjustBitmapSource();

    MIL_FORCEINLINE HRESULT BeginPrimitiveAdjustInternal(
        __out_ecount(1) bool *pfRequiresAdjustment
        );
    MIL_FORCEINLINE HRESULT BeginDeviceAdjustInternal(
        __in_ecount(idx+1) MetaData *prgMetaData,
        UINT idx
        );
    MIL_FORCEINLINE void EndPrimitiveAdjustInternal();

private:

    IWGXBitmapSource **m_ppIBitmapSource;;
    CMetaBitmapRenderTarget *m_pMetaBitmapRT;

#if DBG
public:
    // inlined to avoid cpp file
    inline void DbgSaveState();
    inline void DbgCheckState() const;

private:
    IWGXBitmapSource *m_pDBGBitmapSourceNoRef;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::CAdjustBitmapSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CAdjustBitmapSource::CAdjustBitmapSource(
    __in_ecount_opt(1) IWGXBitmapSource **ppIBitmapSource
    )
{
    m_ppIBitmapSource = ppIBitmapSource;
    m_pMetaBitmapRT = NULL;

#if DBG
    m_pDBGBitmapSourceNoRef = NULL;
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::~CAdjustBitmapSource
//
//  Synopsis:
//      dctor
//
//------------------------------------------------------------------------------
CAdjustBitmapSource::~CAdjustBitmapSource()
{
    EndPrimitiveAdjust();

    Assert(m_pMetaBitmapRT == NULL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::BeginPrimitiveAdjustInternal
//
//  Synopsis:
//      saves the bitmap source present in the brush
//
//------------------------------------------------------------------------------
HRESULT CAdjustBitmapSource::BeginPrimitiveAdjustInternal(
    __out_ecount(1) bool *pfRequiresAdjustment
    )
{
    HRESULT hr = S_OK;

    if (m_ppIBitmapSource != NULL &&
        *m_ppIBitmapSource != NULL)
    {
        // Figure out if the bitmap supports meta-RT internal bitmaps.
        hr = (*m_ppIBitmapSource)->QueryInterface(
            IID_CMetaBitmapRenderTarget,
            (void **)&m_pMetaBitmapRT
            );
    
        if (FAILED(hr))
        {
            m_pMetaBitmapRT = NULL;
            if (hr == E_NOINTERFACE)
            {
                hr = S_OK;
            }
            else
            {
                // If we fail a QI here and get a strange HR
                // this object is likely not built correctly.
                // Fail here early rather than late.
                IFC(hr);
            }
        }
    }

    *pfRequiresAdjustment = (m_pMetaBitmapRT != NULL);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::BeginDeviceAdjustInternal
//
//  Synopsis:
//      Replaces the bitmap source with the device specific bitmap Modifications
//      will be undone in the deconstructor
//
//------------------------------------------------------------------------------
HRESULT CAdjustBitmapSource::BeginDeviceAdjustInternal(
    __in_ecount(idx+1) MetaData *prgMetaData,
    UINT idx
    )
{
    HRESULT hr = S_OK;
    IWGXBitmapSource *pILocalBitmap = NULL;

    UNREFERENCED_PARAMETER(prgMetaData);
    
    // get the bitmap specific to this RT.
    idx = m_pMetaBitmapRT->m_rgMetaData[idx].uIndexOfRealRTBitmap;
    Assert(m_pMetaBitmapRT->m_rgMetaData[idx].fEnable);
    Assert(m_pMetaBitmapRT->m_rgMetaData[idx].pIRTBitmap);
    IFC(m_pMetaBitmapRT->m_rgMetaData[idx].pIRTBitmap->GetBitmapSource(
        &pILocalBitmap
        ));

    (*m_ppIBitmapSource)->Release();
    // steal reference
    *m_ppIBitmapSource = pILocalBitmap;
    pILocalBitmap = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pILocalBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::EndPrimitiveAdjustInternal
//
//  Synopsis:
//      Restores the bitmap source
//
//------------------------------------------------------------------------------
void CAdjustBitmapSource::EndPrimitiveAdjustInternal()
{
    Assert(m_pMetaBitmapRT);

    (*m_ppIBitmapSource)->Release();
    // steal reference
    *m_ppIBitmapSource = m_pMetaBitmapRT;
    m_pMetaBitmapRT = NULL;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::DbgSaveState
//
//  Synopsis:
//      Saves the bitmap source to another location for verification purposes
//
//------------------------------------------------------------------------------
void CAdjustBitmapSource::DbgSaveState()
{
    m_pDBGBitmapSourceNoRef = *m_ppIBitmapSource;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAdjustBitmapSource::DbgCheckState
//
//  Synopsis:
//      Check to make sure the bitmap source was not changed by any other class
//
//------------------------------------------------------------------------------
void CAdjustBitmapSource::DbgCheckState() const
{
    Assert(m_pDBGBitmapSourceNoRef == *m_ppIBitmapSource);
}
#endif




