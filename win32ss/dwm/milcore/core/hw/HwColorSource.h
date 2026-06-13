// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains definition for HW Color Source base class, CHwColorSource
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CHwPipelineShader;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwColorSource
//
//  Synopsis:
//      This class defines the common interface for the HW Pipeline to call HW
//      Color Sources.
//
//  Responsibilites:
//      - Prepare device independent color data for use by a specific device
//        - Caching
//        - Scaling
//        - Wrapping
//      - Prepare mapping from basic vertex info to color source space (as needed)
//      - Setting texture and sample settings for a given stage
//
//  Not responsible for:
//      - Selecting texture stage/sampler
//
//  Inputs required:
//      - Device independent color data (bitmap, brush, mesh, video, ...)
//      - Texture sampler and device state manager
//
//------------------------------------------------------------------------------

class CHwColorSource : public CMILRefCountBase
{
public:

    enum TypeFlagsEnum
    {
        Constant                = 1, // Color is the same over entire primitive area
        Texture                 = 2, // Color is sampled from a texture
        PrecomputedComponent    = 4, // Color is supplied as data in each vertex
        Programmatic            = 8, // Color is generated per vertex or destination coord
    };

    typedef TMILFlagsEnum<TypeFlagsEnum> TypeFlags;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetSourceType
    //
    //  Synopsis:
    //      This method returns the type of color source.
    //
    //--------------------------------------------------------------------------

    virtual TypeFlags GetSourceType(
        ) const PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsOpaque
    //
    //  Synopsis:
    //      Does the source contain alpha?  This method tells you.
    //
    //--------------------------------------------------------------------------

    virtual bool IsOpaque(
        ) const PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsAlphaScalable
    //
    //  Synopsis:
    //      Returns true if AlphaScale functionality is available.
    //
    //--------------------------------------------------------------------------

    virtual bool IsAlphaScalable() const { return false; }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      AlphaScale
    //
    //  Synopsis:
    //      Scale (multiply) the current alpha value by the given scale
    //
    //  Note:
    //      Only valid to call if IsAlphaScalable returns true
    //
    //--------------------------------------------------------------------------

    virtual void AlphaScale(
        FLOAT alphaScale
        )
    {
        UNREFERENCED_PARAMETER(alphaScale);
        return;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendVertexMapping
    //
    //  Synopsis:
    //      This method sends the information needed by the vertex builder to
    //      generate vertex fields for this color source.
    //
    //--------------------------------------------------------------------------

    virtual HRESULT SendVertexMapping(
        __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
        MilVertexFormatAttribute mvfaLocation
        ) PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      Realize
    //
    //  Synopsis:
    //      This method realizes the device consumable resources for this color
    //      source.
    //
    //--------------------------------------------------------------------------

    virtual HRESULT Realize(
        ) PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendDeviceStates
    //
    //  Synopsis:
    //      This method sends the render/stage/sampler states specific to this
    //      color source to the given device.
    //
    //--------------------------------------------------------------------------

    virtual HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        ) PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      ResetForPipelineReuse
    //
    //  Synopsis:
    //      This method resets any device/vertex/shader mappings that may still
    //      be stored from the last render.
    //
    //--------------------------------------------------------------------------

    virtual void ResetForPipelineReuse() PURE;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendShaderData
    //
    //  Synopsis:
    //      This method sends all relevant data to the constant storage of a
    //      shader.
    //
    //--------------------------------------------------------------------------

    virtual HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        ) PURE;
};



