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
//      Contains CStateTable implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(StateTableMemory, MILRawMemory, "StateTableMemory");

// Explicit template instantiation

template class CStateTable<IDirect3DBaseTexture9 *>;

template class CStateTable<IDirect3DVertexShader9 *>;
template class CStateTable<IDirect3DPixelShader9 *>;

template class CStateTable<IDirect3DIndexBuffer9 *>;
template class CStateTable<IDirect3DVertexBuffer9 *>;
template class CStateTable<IDirect3DSurface9 *>;



