/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_eucdb.c

Abstract:

    This file contains the main functions for this module.

    External Routines in this file:
      DllEntry
      NlsDllCodePageTranslation

Revision History:

    10-30-96    JulieB    Created.

--*/



////////////////////////////////////////////////////////////////////////////
//
//  EUC DBCS<->Unicode conversions :
//
//  51932 (Japanese) ............................. calls c_20932.nls
//  51949 (Korean) ............................... calls c_20949.nls
//  51950 (Taiwanese Traditional Chinese) ........ calls c_20950.nls
//  51936 (Chinese   Simplified  Chinese) ........ calls c_20936.nls
//
////////////////////////////////////////////////////////////////////////////



//
//  Include Files.
//

#include <share.h>




//
//  Constant Declarations.
//

#define EUC_J  51932
#define INTERNAL_CODEPAGE(cp)  ((cp) - 31000)





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
    int ctr;
    int cchMBTemp, cchMBCount;
    LPSTR lpMBTempStr;

    //
    //  Error out if internally needed c_*.nls file is not installed.
    //
    if (!IsValidCodePage(INTERNAL_CODEPAGE(CodePage)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    switch (dwFlags)
    {
        case ( NLS_CP_CPINFO ) :
        {
            GetCPInfo(CodePage, lpCPInfo);

            if (CodePage == EUC_J)
            {
                lpCPInfo->MaxCharSize = 3;
            }

            return (TRUE);
        }
        case ( NLS_CP_MBTOWC ) :
        {
            if (CodePage != EUC_J)
            {
                return (MultiByteToWideChar( INTERNAL_CODEPAGE(CodePage),
                                             0,
                                             lpMultiByteStr,
                                             cchMultiByte,
                                             lpWideCharStr,
                                             cchWideChar ));
            }

            //
            //  CodePage == EUC_J
            //
            //  JIS X 0212-1990
            //  0x8F is the first-byte of a 3-byte char :
            //    Remove 0x8F
            //    If there is no third byte
            //       remove the second byte as well,
            //    else
            //       leave the second byte unchanged.
            //    Mask off MSB of the third byte (Byte3 & 0x7F).
            //    Example : 0x8FA2EF -> 0xA26F
            //
            if (cchMultiByte == -1)
            {
                cchMultiByte = strlen(lpMultiByteStr) + 1;
            }

            lpMBTempStr = (LPSTR)NLS_ALLOC_MEM(cchMultiByte);
            if (lpMBTempStr == NULL)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return (0);
            }

            for (ctr = 0, cchMBTemp = 0; ctr < cchMultiByte; ctr++, cchMBTemp++)
            {
                if (lpMultiByteStr[ctr] == (char)0x8F)
                {
                    ctr++;
                    if (ctr >= (cchMultiByte - 1))
                    {
                        //
                        //  Missing second or third byte.
                        //
                        break;
                    }

                    lpMBTempStr[cchMBTemp++] = lpMultiByteStr[ctr++];
                    lpMBTempStr[cchMBTemp]   = (lpMultiByteStr[ctr] & 0x7F);
                }
                else
                {
                    lpMBTempStr[cchMBTemp] = lpMultiByteStr[ctr];
                }
            }

            cchMBCount = MultiByteToWideChar( INTERNAL_CODEPAGE(CodePage),
                                              0,
                                              lpMBTempStr,
                                              cchMBTemp,
                                              lpWideCharStr,
                                              cchWideChar );
            NLS_FREE_MEM(lpMBTempStr);

            return (cchMBCount);
        }
        case ( NLS_CP_WCTOMB ) :
        {
            if (CodePage != EUC_J)
            {
                return (WideCharToMultiByte( INTERNAL_CODEPAGE(CodePage),
                                             WC_NO_BEST_FIT_CHARS,
                                             lpWideCharStr,
                                             cchWideChar,
                                             lpMultiByteStr,
                                             cchMultiByte,
                                             NULL,
                                             NULL ));
            }

            //
            //  CodePage == EUC_J
            //
            //  Check char for JIS X 0212-1990
            //  if a lead-byte (>= 0x80) followed by a trail-byte (< 0x80)
            //  then
            //    insert 0x8F which is the first byte of a 3-byte char
            //    lead-byte becomes the second byte
            //    turns on MSB of trail-byte which becomes the third byte
            //    Example : 0xA26F -> 0x8FA2EF
            //
            if (cchWideChar == -1)
            {
                cchWideChar = wcslen(lpWideCharStr);
            }

            cchMBTemp = cchWideChar * (sizeof(WCHAR) + 1) + 1;
            lpMBTempStr = (LPSTR)NLS_ALLOC_MEM(cchMBTemp);
            if (lpMBTempStr == NULL)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return (0);
            }

            cchMBCount = WideCharToMultiByte( INTERNAL_CODEPAGE(CodePage),
                                              WC_NO_BEST_FIT_CHARS,
                                              lpWideCharStr,
                                              cchWideChar,
                                              lpMBTempStr,
                                              cchMBTemp,
                                              NULL,
                                              NULL );

            for (ctr = 0, cchMBTemp = 0;
                 ctr < cchMBCount, cchMBTemp < cchMultiByte;
                 ctr++, cchMBTemp++)
            {
                if (lpMBTempStr[ctr] & 0x80)
                {
                    //
                    //  It's a lead byte.
                    //
                    if (lpMBTempStr[ctr + 1] & 0x80)
                    {
                        //
                        //  It's a non JIS X 0212-1990 char.
                        //
                        if (cchMultiByte)
                        {
                            if (cchMBTemp < (cchMultiByte - 1))
                            {
                                lpMultiByteStr[cchMBTemp]     = lpMBTempStr[ctr];
                                lpMultiByteStr[cchMBTemp + 1] = lpMBTempStr[ctr + 1];
                            }
                            else
                            {
                                //
                                //  No room for trail byte.
                                //
                                lpMultiByteStr[cchMBTemp++] = '?';
                                break;
                            }
                        }
                    }
                    else
                    {
                        //
                        //  It's a JIS X 0212-1990 char.
                        //
                        if (cchMultiByte)
                        {
                            if (cchMBTemp < (cchMultiByte - 2))
                            {
                                lpMultiByteStr[cchMBTemp]     = (char) 0x8F;
                                lpMultiByteStr[cchMBTemp + 1] = lpMBTempStr[ctr];
                                lpMultiByteStr[cchMBTemp + 2] = (lpMBTempStr[ctr + 1] | 0x80);
                            }
                            else
                            {
                                //
                                //  No room for two trail bytes.
                                //
                                lpMultiByteStr[cchMBTemp++] = '?';
                                break;
                            }
                        }
                        cchMBTemp++;
                    }
                    cchMBTemp++;
                    ctr++;
                }
                else
                {
                    if (cchMultiByte && (cchMBTemp < cchMultiByte))
                    {
                        lpMultiByteStr[cchMBTemp] = lpMBTempStr[ctr];
                    }
                }
            }

            //
            //  See if the output buffer is too small.
            //
            if (cchMultiByte && (cchMBTemp >= cchMultiByte))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            NLS_FREE_MEM (lpMBTempStr);

            return (cchMBTemp);
        }
    }

    //
    //  This shouldn't happen since this gets called by the NLS APIs.
    //
    SetLastError(ERROR_INVALID_PARAMETER);
    return (0);
}
