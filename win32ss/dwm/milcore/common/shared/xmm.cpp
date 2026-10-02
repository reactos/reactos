// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Classes to execute fast calculations on integer data
//      using SSE2 instruction set extention.
//

#include "precomp.hpp"

#if defined(_BUILD_SSE_)

const CXmmValue::QWords CXmmValue::sc_Zero = {0, 0};

const CXmmWords::Words CXmmWords::sc_Half8dot8 =
{
    0x0080, 0x0080, 0x0080, 0x0080,
    0x0080, 0x0080, 0x0080, 0x0080
};

#endif //_BUILD_SSE_


