// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       conmain.cxx
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL __cdecl mainCRTStartup();
extern HRESULT AvCreateProcessHeap();
extern HRESULT AvDestroyProcessHeap();

int __cdecl CON_MAIN_FUNCTION_NAME()
{
    int retcode = 0;

    if (FAILED(AvCreateProcessHeap())) {
        retcode = FALSE;
    } else {
        CON_MAIN_PRE_MAIN

        // Initialize the CRT and have it call into our DllMain for us
        // This will never return
        retcode = mainCRTStartup();


        if (FAILED(AvDestroyProcessHeap())) {
            retcode = FALSE;
        }
    }

    return retcode;
}

#ifdef __cplusplus
}
#endif



