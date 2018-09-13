//*************************************************************
//
//  Main entry point
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"


//*************************************************************
//
//  DllMain()
//
//  Purpose:    Main entry point
//
//  Parameters:     hInstance   -   Module instance
//                  dwReason    -   Way this function is being called
//                  lpReseved   -   Reserved
//
//
//  Return:     (BOOL) TRUE if successfully initialized
//                     FALSE if an error occurs
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {

            DisableThreadLibraryCalls (hInstance);
            InitializeGlobals (hInstance);
            InitializeAPIs();
            InitializeNotifySupport();
            InitializeGPOCriticalSection();
            InitializeApiDLLsCritSec();
            InitializePingCritSec();

            {
                TCHAR szProcessName[MAX_PATH];
                DWORD dwLoadFlags = FALSE;
                DWORD WINLOGON_LEN = 12;  // Length of string "winlogon.exe"
                DWORD SETUP_LEN = 9;      // Length of string "setup.exe"

                DWORD dwRet = GetModuleFileName (NULL, szProcessName, ARRAYSIZE(szProcessName));
                if ( dwRet > WINLOGON_LEN ) {

                    if ( CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                         &szProcessName[dwRet-WINLOGON_LEN], -1, L"winlogon.exe", -1 ) == CSTR_EQUAL ) {
                        dwLoadFlags = WINLOGON_LOAD;
                    }
                }
#if 0
                if ( dwRet > SETUP_LEN ) {

                    if ( CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                         &szProcessName[dwRet-SETUP_LEN], -1, L"setup.exe", -1 ) == CSTR_EQUAL ) {
                        dwLoadFlags = SETUP_LOAD;
                    }
                }
#endif

                InitDebugSupport( dwLoadFlags );

                DebugMsg((DM_VERBOSE, TEXT("LibMain: Process Name:  %s"), szProcessName));
            }

            }
            break;


        case DLL_PROCESS_DETACH:
            if (g_hGUIModeSetup) {
                CloseHandle (g_hGUIModeSetup);
                g_hGUIModeSetup = NULL;
            }

            if (g_hProfileSetup) {
                CloseHandle (g_hProfileSetup);
                g_hProfileSetup = NULL;
            }
            
            CloseApiDLLsCritSec();
            ShutdownEvents ();
            ShutdownNotifySupport();
            CloseGPOCriticalSection();
            ClosePingCritSec();
            break;

    }

    return TRUE;
}
