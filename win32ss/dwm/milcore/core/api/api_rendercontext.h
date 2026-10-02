// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Contains declaration of CContextState
//

#pragma once

class CRenderState;
class CSnappingFrame;

struct CContextState
{
private:
    // Display set from which display settings 
    // are extracted. 
    // Note: 
    //  A reference to the display set is maintained
    //  to enable us to release it from within the destructor. 
    //  This ensures that DisplaySettings::pIDWriteRenderingParams 
    //  does not get inadvertently released elsewhere while 
    //  we still have use of it.
    CDisplaySet const* m_pDisplaySet = nullptr;

    DisplaySettings  const* m_pDisplaySettings = nullptr;

    // 
    // DPIProvider passed from CSlaveHWndRenderContext
    // via CDrawingContext
    // This is consumed by CGlyphResource::GetAvailableScale
    // to compute monitor-DPI specific realization scale
    //
    // This is a weak pointer to an ancestor object 
    // and thus should not be AddRef'd etc.
    IDpiProvider* m_pDpiProvider = nullptr;

public:
    //
    // Unit Transform kept by the render context (U)
    // Reflects the selected unit for the world space.
    //

    CMatrix<CoordinateSpace::PageInUnits,CoordinateSpace::Inches> UnitTransform;
    MilUnit::Enum PageUnit;

    //
    // Transforms for 3D rendering
    // These are currently used only on 3D objects
    // and are the only transforms applied to those
    // objects.  To be used they require the In3D
    // flag to be on.
    //

    CMILMatrix WorldTransform3D;
    CMILMatrix ViewTransform3D;
    CMILMatrix ProjectionTransform3D;
    CMultiOutSpaceMatrix<CoordinateSpace::Projection3D> ViewportProjectionModifier3D;
    CMultiSpaceRectF<CoordinateSpace::PageInPixels, CoordinateSpace::Device> UnclippedProjectedMeshBounds;

    bool In3D;
    D3DCMPFUNC DepthBufferFunction3D;
    D3DCULL CullMode3D;
    
    //
    // This is the clip state of the current context.
    //

    CAliasedClip AliasedClip;

    //
    // The data from below is only valid inside
    // a DrawXXX call. It is initialized by the
    // render context before calling the render target
    // The reason it is kept here is that we do not want
    // to be copying state between different state structures
    //

    CRenderState *RenderState;

    //
    // W * U * D if the Unit is not unit pixel
    // W if the Unit is pixel.
    //

    //   Refactor CContextState transforms
    //   The transform-related state in this struct is not used as documented -
    //   and some of it is not used at all.
    //
    //   We need to decide what features belong at this level. My hunch from
    //   GDI/GDI+, is that we want to model world, container, page, and unit
    //   space at this level. Another point of view is that there should just be
    //   a single transform at this level.

    CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> WorldToDevice;

    DWORD CurrentTime;

    CMILLightData LightData;
    
    CSnappingFrame * m_pSnappingStack;

    CContextState(BOOL f2DInitOnly = FALSE) :
        AliasedClip(NULL)
    {
        PageUnit = MilUnit::Pixel;
        RenderState = NULL;
        In3D = false;

        UnitTransform = CMatrix<CoordinateSpace::PageInUnits,CoordinateSpace::Inches>::refIdentity();
        WorldToDevice.SetToIdentity();
        WorldToDevice.DbgChangeToSpace<
            CoordinateSpace::LocalRendering,
            CoordinateSpace::PageInPixels
            >();

        if (!f2DInitOnly)
        {
            WorldTransform3D = dxlayer::matrix::get_identity();
            ViewTransform3D = dxlayer::matrix::get_identity();
            ProjectionTransform3D = dxlayer::matrix::get_identity();
            ViewportProjectionModifier3D.SetToIdentity();
            // Change Out space to Page as that is the common working Out space.
            // Setting to any other space will have to be explicit.
            ViewportProjectionModifier3D.DbgChangeToSpace<
                CoordinateSpace::Projection3D,
                CoordinateSpace::PageInPixels
                >();

            // Cull mode and zfunc defaults
            CullMode3D = D3DCULL_NONE;
            DepthBufferFunction3D = D3DCMP_LESSEQUAL;
        }

        CurrentTime = 0;

        // m_pSnappingStack is not owned by this class
        // so we don't care about its life time.
        m_pSnappingStack = NULL;
    }
    
    ~CContextState()
    {
        ReleaseInterface(m_pDisplaySet);
        // m_pDpiProvider is a weak-pointer to an
        // ancestor. Do not attempt to release it here.
    }

    //+----------------------------------------------------------------------------
    //
    //  Member:    
    //      GetSamplingSourceCoordSpace
    //
    //  Synopsis:  
    //      2D and 3D handle texture coordinates differently. 2D computes texture
    //      coordinates from device space and 3D computes it from brush coordinate
    //      space. (The coordinates in brush coordinate space are also called
    //      texture coordinates, making things doubly difficult.) This function can
    //      be called to help abstract the two cases, declaring the space we care
    //      about in more familiar terms... either as SampleSpace or WorldSpace
    //
    //-----------------------------------------------------------------------------

    CoordinateSpaceId::Enum GetSamplingSourceCoordSpace(
        ) const
    {
        //
        // In 3D we transform from brush coordinates to texture coordinate space
        // Brush coordinate space is the same as World Sampling Space
        //
        // In 2D we transform from device space to texture coordinate space

        return (In3D)
                ? CoordinateSpaceId::BaseSampling
                : CoordinateSpaceId::Device;
    }

    // Gets the current or default display settings
    //  If there is no cached (i.e, current) DisplaySettings object, then 
    //  acquire the default display settings  from the current 
    //  display set.
    DisplaySettings const * GetCurrentOrDefaultDisplaySettings()
    {
        if (m_pDisplaySettings == nullptr)
        {
            if (m_pDisplaySet == nullptr)
            {
                g_DisplayManager.GetCurrentDisplaySet(&m_pDisplaySet);
                Assert(m_pDisplaySet);
            }

            m_pDisplaySettings = m_pDisplaySet->GetDefaultDisplaySettings();
            Assert(m_pDisplaySettings);
        }

        return m_pDisplaySettings;
    }

    // Gets display settings from the given display set
    //  The current display set will be replaced with the one 
    //  supplied by the caller here. The settings index is not 
    //  preserved - instead the relevant display settings object
    //  is cached. 
    DisplaySettings const * GetDisplaySettingsFromDisplaySet(CDisplaySet const* pDisplaySet, UINT index)
    {
        Assert (pDisplaySet != nullptr);
        if (pDisplaySet != nullptr)
        {
            ReplaceInterface(m_pDisplaySet, pDisplaySet);
            Assert(m_pDisplaySet != nullptr);
        }

        m_pDisplaySettings = m_pDisplaySet->GetDisplaySettings(index);
        Assert(m_pDisplaySettings);

        return m_pDisplaySettings;
    }

    // Returns the current instance of the DpiProvider
    // object. This is nullptr if no DpiProvider
    // was registered during the creation CDrawingContext
    IDpiProvider const* GetDpiProvider() const
    {
        return m_pDpiProvider;
    }
    
    // Sets the DpiProvider instance after releasing
    // the previous instance
    void SetDpiProvider(IDpiProvider* pDpiProvider)
    {
        // This is a weak pointer to an ancestor 
        // instance. Do not AddRef here. 
        m_pDpiProvider = pDpiProvider;
    }
};


/*=========================================================================*/






