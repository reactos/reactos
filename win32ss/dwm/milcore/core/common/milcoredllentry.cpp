// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      milcore initialization and uninitialization
//
//

#include "precomp.hpp"

using namespace dxlayer;


//-----------------------------------------------------------------------------
// Meter declarations

MtDefine(DXInitialization, MILRender, "DX Default Meter");


//
// DLL instance handle
//
extern HINSTANCE g_DllInstance;


//---------------------------------------------------------------------------------
//
// Global composition engine critical section.
//
//---------------------------------------------------------------------------------

extern CCriticalSection g_csCompositionEngine;
extern CCriticalSection g_csGraphicsStream;

//+----------------------------------------------------------------------------
//
//  Member:    MILCoreDllMain
//
//  Synopsis:  This is the very first function call into milcore.dll, and takes
//             place when the DLL is first loaded.  We do some one-time
//             initialization here.
//
//-----------------------------------------------------------------------------
BOOL
MILCoreDllMain(
    __in HINSTANCE   dllHandle,
    ULONG reason
    )
{
    BOOL b = TRUE;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            HRESULT hr = S_OK;

            g_DllInstance = dllHandle;

            // To improve the working set, we tell the system we don't
            // want any DLL_THREAD_ATTACH calls:

            DisableThreadLibraryCalls((HINSTANCE) dllHandle);

            IFC(AvDllInitialize());


#if DBG==1
            // DX is allocating memory on the first call of a DX math function during CPU Optimization checks.
            // This causes a debug break because our meter code asserts that all allocations are done against
            // a valid memory meter. By calling D3DMatrixMultiply we force that this code will execute here under
            // a default meter.
            {
                CSetDefaultMeter mtDefault(Mt(DXInitialization));
                auto i0 = matrix::get_identity();
                auto i1 = matrix::get_identity();
                CMILMatrix r = i0 * i1;
            }
#endif

            //
            // Initialize critical sections used by the composition engine.
            //
            IFC(g_csCompositionEngine.Init());
            IFC(g_csGraphicsStream.Init());
            IFC(RenderOptions::Init());

            IFC(Startup());
            IFC(SwStartup());
            IFC(HwStartup());

            //
            // Register the MIL provider guid with ETW
            //
            // Check the backwards compat reg key
            HKEY hRegAvalonGraphics;
            if (SUCCEEDED(GetAvalonRegistrySettingsKey(&hRegAvalonGraphics, TRUE)))
            {
                DWORD dwTemp = 0;
                if (RegReadDWORD(hRegAvalonGraphics, L"ClassicETW", &dwTemp) && dwTemp != 0)
                {
                    McGenInitTracingSupport();
                    McGenPreVista = TRUE;
                }
            }
            EventRegisterMicrosoft_Windows_WPF();
#if MIL_LOGGER
            IFC(CLogger::Create(&g_pLog));
#endif

#if DBG

            //  Tags for the .dll should be registered before
            //  calling DbgExRestoreDefaultDebugState().  Do this by
            //  declaring each global tag object or by explicitly calling
            //  DbgExTagRegisterTrace.

            DbgExRestoreDefaultDebugState();

#endif // DBG


    Cleanup:

        b = SUCCEEDED(hr);

        break;
        }


    case DLL_PROCESS_DETACH:

        CAssertDllInUse::Check();

#if MIL_LOGGER
        delete g_pLog;
#endif
        //
        // Unregister ETW provider
        //
        EventUnregisterMicrosoft_Windows_WPF();
        HwShutdown();
        SwShutdown();
        Shutdown();
        AvDllShutdown();

#if DBG
        DbgExTraceMemoryLeaks();
#endif

        g_csCompositionEngine.DeInit();
        g_csGraphicsStream.DeInit();
        RenderOptions::DeInit();
        break;
    }

    return(b);
}




