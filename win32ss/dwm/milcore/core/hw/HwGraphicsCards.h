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
//      Contains enumerations for graphics card vendors and individual cards.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

/*=========================================================================*\
    Graphics Card Vendors
\*=========================================================================*/
enum GraphicsCardVendor
{
    GraphicsCardVendorUnknown   = 0,
    GraphicsCardVendorIntel     = 0x8086,
    GraphicsCardVendorNVidia    = 0x10de,

    GRAPHICSCARDVENDOR_FORCEDWORD = MIL_FORCE_DWORD
};

/*=========================================================================*\
    Graphics Cards
\*=========================================================================*/
enum GraphicsCard
{
    GraphicsCardUnknown     = 0,
    GraphicsCardIntel_845G  = 0x2562,
    GraphicsCardNVidia_5900 = 0x330,
    GraphicsCardNVidia_5700 = 0x342,

    GRAPHICSCARD_FORCEDWORD = MIL_FORCE_DWORD
};



