// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"

//**********************************************************************
// This file is temporarily removed until after the December reverse-integration.
// Until then (after the RI) all codec calls from mil\core and udwm will go
// through helper functions located in mil\core (MILCreateBitmap, etc).
//**********************************************************************
#ifdef COMMENTED_OUT_FOR_DEC_REVERSE_INTEGRATION

/**************************************************************************
*
* Statics
*
**************************************************************************/

MILCreateImagingFactoryProc CodecDll::s_pfnCreateImagingFactory = NULL;
DllLoadState CodecDll::s_dllState = Uninitialized;

/**************************************************************************
*
* BOOL ICMModule::EnsureLoaded()
*
* Function Description:
*
* Created:
*
*      4/18/2003 ThomasOl
*
**************************************************************************/

BOOL CodecDll::EnsureLoaded()
{
    if (Uninitialized == s_dllState)
    {
        s_dllState = LoadFailed;
        HMODULE hDLL = NULL;
        {
            FPUStateSandbox oGuard;
            hDLL = LoadLibraryA("WindowsCodecs.dll");
        }

        if (hDLL)
        {
           s_pfnCreateImagingFactory =
               (MILCreateImagingFactoryProc)GetProcAddress(hDLL, "MILCreateImagingFactory");

            if (s_pfnCreateImagingFactory)
            {
                s_dllState = Loaded;
            }
        }
    }
    return (s_dllState == Loaded);
}

HRESULT WINAPI CodecDll::MILCreateImagingFactory(
    UINT SDKVersion,
    __deref_out IMILImagingFactory **ppIImagingFactory)
{
    HRESULT hr = E_FAIL;
    if (EnsureLoaded())
    {
        hr = (*s_pfnCreateImagingFactory)(SDKVersion,
                                        ppIImagingFactory);
    }
    return hr;
}

STDMETHODIMP CMILFactory::GetImagingFactory(
    OUT IMILImagingFactory **ppIImagingFactory
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppIImagingFactory)
    {
        if (!m_pIImagingFactory)
        {
            MIL_THR(CodecDll::MILCreateImagingFactory(
                WINCODEC_SDK_VERSION_WPF,
                &m_pIImagingFactory));
        }
        else
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            *ppIImagingFactory = m_pIImagingFactory;
            m_pIImagingFactory->AddRef();
        }
    }

    return hr;
}

#endif // COMMENTED_OUT_FOR_DEC_REVERSE_INTEGRATION


