/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_other.c

Abstract:

    This file contains the main functions for this module.

    External Routines in this file:
      DllEntry
      NlsDllCodePageTranslation

Revision History:

    10-30-96    JulieB    Created.

--*/



//
//  Include Files.
//

#include <share.h>




//
//  Global Variables.
//




//
//  Forward Declarations.
//

DWORD
TranslateCP_50000(
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo);





//-------------------------------------------------------------------------//
//                             DLL ENTRY POINT                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  DllEntry
//
//  DLL Entry initialization procedure.
//
//  10-30-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL DllEntry(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    switch (dwReason)
    {
        case ( DLL_THREAD_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_THREAD_DETACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            return (TRUE);
        }
    }

    return (FALSE);
    hModule;
    lpRes;
}





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NlsDllCodePageTranslation
//
//  This routine is the main exported procedure for the functionality in
//  this DLL.  All calls to this DLL must go through this function.
//
//  10-30-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

DWORD NlsDllCodePageTranslation(
    DWORD CodePage,
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo)
{
    switch (CodePage)
    {
        case ( 50000 ) :
        {
            return ( TranslateCP_50000( dwFlags,
                                        lpMultiByteStr,
                                        cchMultiByte,
                                        lpWideCharStr,
                                        cchWideChar,
                                        lpCPInfo ) );
        }
        default :
        {
            //
            //  Invalid code page value.
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }
}





//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  TranslateCP_50000
//
//  This routine does the translations for code page 50000.
//
//      ****  This is a BOGUS routine - for testing purposes only.  ****
//
//  10-30-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

DWORD TranslateCP_50000(
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo)
{
    int ctr;

    switch (dwFlags)
    {
        case ( NLS_CP_CPINFO ) :
        {
            lpCPInfo->MaxCharSize = 1;

            lpCPInfo->DefaultChar[0] = '?';
            lpCPInfo->DefaultChar[1] = (BYTE)0;

            for (ctr = 0; ctr < MAX_LEADBYTES; ctr++)
            {
                lpCPInfo->LeadByte[ctr] = 0;
            }

            return (TRUE);
        }
        case ( NLS_CP_MBTOWC ) :
        {
            if (cchWideChar == 0)
            {
                return (cchMultiByte);
            }

            for (ctr = 0; (ctr < cchMultiByte) && (ctr < cchWideChar); ctr++)
            {
                lpWideCharStr[ctr] = (WORD)(lpMultiByteStr[ctr]);
            }

            return (ctr);
        }
        case ( NLS_CP_WCTOMB ) :
        {
            if (cchMultiByte == 0)
            {
                return (cchWideChar);
            }

            for (ctr = 0; (ctr < cchWideChar) && (ctr < cchMultiByte); ctr++)
            {
                lpMultiByteStr[ctr] = LOBYTE(lpWideCharStr[ctr]);
            }

            return (ctr);
        }
        default :
        {
            //
            //  This shouldn't happen since this function gets called by
            //  the NLS API routines.
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }
}
