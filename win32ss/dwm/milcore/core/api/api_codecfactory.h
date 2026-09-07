// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Class Description:
*
*   
*   
*
* Created:
*
*      5/1/2003 ThomasOl
*
**************************************************************************/

//**********************************************************************
// This file is temporarily removed until after the December reverse-integration.
//**********************************************************************
#ifdef COMMENTED_OUT_FOR_DEC_REVERSE_INTEGRATION
#pragma once

typedef HRESULT (WINAPI *MILCreateImagingFactoryProc)(
    UINT SDKVersion,
    __deref_out IMILImagingFactory **ppIImagingFactory);

/**************************************************************************
*
*   
*
*
**************************************************************************/

class CodecDll
{
public:
    static HRESULT WINAPI MILCreateImagingFactory(
        UINT SDKVersion,
        __deref_out IMILImagingFactory **ppIImagingFactory);

private:
    static BOOL EnsureLoaded();
    static MILCreateImagingFactoryProc s_pfnCreateImagingFactory;
    static DllLoadState s_dllState;
};
#endif // COMMENTED_OUT_FOR_DEC_REVERSE_INTEGRATION


