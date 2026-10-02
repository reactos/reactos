// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Simple instrumentation to investigate performance of code fragments. See
//      header file for details.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.h"

#ifndef ENABLE_SIMPLE_PERF
void DumpInstrumentationData() {}

#else //#ifdef ENABLE_SIMPLE_PERF

CPerfAcc::CDumper CPerfAcc::m_dumper;

void DumpInstrumentationData()
{
    CPerfAcc::m_dumper.Dump();
}
#endif //ENABLE_SIMPLE_PERF


