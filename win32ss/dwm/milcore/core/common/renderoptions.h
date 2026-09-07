// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//----------------------------------------------------------------------------

namespace RenderOptions
{
    // These two methods are not thread safe and should only be called at DLL load/unload
    HRESULT Init();
    void DeInit();
    
    void ForceSoftwareRenderingForProcess(BOOL fForce);

    BOOL IsSoftwareRenderingForcedForProcess();
};


