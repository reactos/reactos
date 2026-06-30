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
//      Contains HW Primary Color Source interface
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Interface:
//      IHwPrimaryColorSource
//
//  Synopsis:
//      Common interface provided by all primitives for the pipeline to request
//      the information it needs
//
//      This interface should be supported by brushes for DrawPath and an object
//      provided by each of the other DrawXxx primitives using the pipeline.
//
//  Responsibilities:
//      - Prepare device independent brush data for use by a specific device
//        - Breaking down complex brushes into simpler hardware color sources
//        - Caching
//        - Scaling
//        - Wrapping
//      - Specifying primary blending argument(s) and operation(s) to pipeline
//
//  Not responsible for:
//      - Selecting texture stage
//      - Determining vertex format in vertex buffer
//
//  Inputs required:
//      - Device independent color data (bitmap, brush, mesh, video, ...)
//
//------------------------------------------------------------------------------

class CHwSurfaceRenderTargetSharedData;

interface IHwPrimaryColorSource
{

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendOperations
    //
    //  Synopsis:
    //      Send primary blend operations and color source(s) to builder
    //
    //--------------------------------------------------------------------------

    virtual HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        ) PURE;
};



