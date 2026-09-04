// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Contains SW Startup routine declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

HRESULT SwStartup();
void SwShutdown();

extern bool g_fUseMMX;
extern bool g_fUseSSE2;

void HwShutdown();



