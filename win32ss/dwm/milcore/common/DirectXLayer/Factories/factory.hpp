// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

// This is the main and only header that will be included to obtain access to 
// D3DX9 or DirectXMath specific types
// The main project/solution should define either D3DX9 or DIRECTXMATH (but not both).

#if (defined(D3D9EXTENSIONS) && defined(DIRECTXMATH))
#error "Both D3DX9 and DIRECTXMATH are defined. Only one of those definitions is expected"
#elif defined(D3D9EXTENSIONS)
#include "factory9.hpp"
#elif defined (DIRECTXMATH)
#include "xmfactory.hpp"
#else 
#error "One of D3D9EXTENSIONS and DIRECTXMATH should be defined" 
#endif 
