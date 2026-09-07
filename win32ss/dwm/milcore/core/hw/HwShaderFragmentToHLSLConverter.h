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
//      Contains the declaration for ConvertShaderFragmentsToHLSL
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#define MAX_SHADER_FRAGMENT 99
#define MAX_SHADER_INTERPOLATOR 7
#define MAX_SHADER_SAMPLER 15

struct HwShaderPipelineItem;

__checkReturn HRESULT
ConvertHwShaderFragmentsToHLSL(
    __in_ecount(uNumPipelineItems) HwPipelineItem const * const rgShaderItem,
    __range(1,MAX_SHADER_FRAGMENT) UINT uNumPipelineItems,
    __deref_outro_bcount(cbHLSL) PCSTR &pHLSLOut,
    __out_ecount(1) UINT &cbHLSL
    );



