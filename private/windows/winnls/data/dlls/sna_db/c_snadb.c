/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_snadb.c

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
// IBM EBCDIC DBCS from/to Unicode conversions for SNA
//
//  CP#   = Single Byte              + Double Byte
//  ----- = -----------              + -----------
//  50930 =  290 (Katakana Extended) + 300 (Japanese)            calls 20930
//  50931 =  037 (US/Canada)         + 300 (Japanese)            calls 20931
//  50933 =  833 (Korean Extended)   + 834 (Korean)              calls 20933
//  50935 =  836 (Simp-Chinese Ext.) + 837 (Simplified  Chinese) calls 20935
//  50937 =  037 (US/Canada)         + 835 (Traditional Chinese) calls 20937
//  50939 = 1027 (Latin Extended)    + 300 (Japanese)            calls 20939
//
////////////////////////////////////////////////////////////////////////////



//
//  Include Files.
//

#include <share.h>




//
//  Constant Declarations.
//

#define SHIFTOUT       0x0e       // from SBCS to DBCS
#define SHIFTIN        0x0f       // from DBCS to SBCS

#define BOGUSLEADBYTE  0x3f       // prefix SBC to make it DBC

#define INTERNAL_CODEPAGE(cp)  ((cp) - 30000)





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
    LPSTR lpMBNoEscStr;
    int cchMBEscStr = 0;
    int ctr, cchMBTemp, cchMBCount, cchWCCount;
    BOOL IsDBCS = FALSE;

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
            memset(lpCPInfo, 0, sizeof(CPINFO));

            lpCPInfo->MaxCharSize    = 3;
            lpCPInfo->DefaultChar[0] = 0x3f;

            //
            //  Lead byte does not apply here, leave them all NULL.
            //
            return (TRUE);
        }
        case ( NLS_CP_MBTOWC ) :
        {
            if (cchMultiByte == -1)
            {
                cchMultiByte = strlen(lpMultiByteStr) + 1;
            }

            //
            //  Each single byte char becomes 2 bytes, so we need a
            //  temporary buffer twice as big.
            //
            if ((lpMBNoEscStr = (LPSTR)NLS_ALLOC_MEM(cchMultiByte << 1)) == NULL)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return (0);
            }

            //
            //  Remove all Shift-In & Shift-Out.
            //
            for (ctr = 0, cchMBTemp = 0; ctr < cchMultiByte; ctr++)
            {
                if (lpMultiByteStr[ctr] == SHIFTOUT)
                {
                    IsDBCS = TRUE;
                }
                else if (lpMultiByteStr[ctr] == SHIFTIN)
                {
                    IsDBCS = FALSE;
                }
                else
                {
                    if (IsDBCS)
                    {
                        //
                        //  Double byte char.
                        //
                        if (ctr < (cchMultiByte - 1))
                        {
                            lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr++];
                            lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr];
                        }
                        else
                        {
                            //
                            //  Last char is a lead-byte with no trail-byte,
                            //  so let MultiByteToWideChar take care of it.
                            //
                            break;
                        }
                    }
                    else
                    {
                        //
                        //  Single byte char.
                        //  Prefix it with a bogus lead byte to make it a
                        //  double byte char.  The internal table has been
                        //  arranged accordingly.
                        //
                        lpMBNoEscStr[cchMBTemp++] = BOGUSLEADBYTE;
                        lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr];
                    }
                }
            }

            cchWCCount = MultiByteToWideChar( INTERNAL_CODEPAGE(CodePage),
                                              0,
                                              lpMBNoEscStr,
                                              cchMBTemp,
                                              lpWideCharStr,
                                              cchWideChar );
            if (cchWCCount == 0)
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            }

            NLS_FREE_MEM(lpMBNoEscStr);

            return (cchWCCount);
        }
        case ( NLS_CP_WCTOMB ) :
        {
            if (cchWideChar == -1)
            {
                cchWideChar = wcslen(lpWideCharStr) + 1;
            }

            cchMBTemp = cchWideChar * sizeof(WCHAR);
            lpMBNoEscStr = (LPSTR)NLS_ALLOC_MEM(cchMBTemp);
            if (lpMBNoEscStr == NULL)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return (0);
            }

            //
            //  Convert to an MB string without Shift-In/Out first.
            //
            cchMBCount = WideCharToMultiByte( INTERNAL_CODEPAGE(CodePage),
                                              WC_NO_BEST_FIT_CHARS,
                                              lpWideCharStr,
                                              cchWideChar,
                                              lpMBNoEscStr,
                                              cchMBTemp,
                                              NULL,
                                              NULL );

            /*
               what if (cchMBCount == 0) ?
               might need to add error checking later
            */

            //
            //  Insert Shift-In and Shift-Out as needed and
            //  remove BOGUSLEADBYTE.
            //
            ctr = 0;
            while (ctr < cchMBCount)
            {
                //
                //  See if it's a single byte char.
                //
                if (lpMBNoEscStr[ctr] == BOGUSLEADBYTE)
                {
                    //
                    //  It's a single byte char.
                    //
                    ctr++;
                    if (IsDBCS)
                    {
                        if (cchMultiByte)
                        {
                            if (cchMBEscStr < cchMultiByte)
                            {
                                lpMultiByteStr[cchMBEscStr] = SHIFTIN;
                            }
                            else
                            {
                                //
                                //  Output buffer is too small.
                                //
                                break;
                            }
                        }
                        cchMBEscStr++;
                        IsDBCS = FALSE;
                    }

                    if (cchMultiByte)
                    {
                        if (cchMBEscStr < cchMultiByte)
                        {
                            lpMultiByteStr[cchMBEscStr] = lpMBNoEscStr[ctr];
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            break;
                        }
                    }
                    cchMBEscStr++;
                    ctr++;
                }
                else
                {
                    //
                    //  It's a double byte char.
                    //
                    if (!IsDBCS)
                    {
                        if (cchMultiByte)
                        {
                            if (cchMBEscStr < cchMultiByte)
                            {
                                lpMultiByteStr[cchMBEscStr] = SHIFTOUT;
                            }
                            else
                            {
                                //
                                //  Output buffer is too small.
                                //
                                break;
                            }
                        }
                        cchMBEscStr++;
                        IsDBCS = TRUE;
                    }

                    if (ctr >= (cchMBCount - 1))
                    {
                        //
                        //  Missing trail byte.
                        //
                        break;
                    }

                    if (cchMultiByte)
                    {
                        if (cchMBEscStr < (cchMultiByte - 1))
                        {
                            lpMultiByteStr[cchMBEscStr]     = lpMBNoEscStr[ctr];
                            lpMultiByteStr[cchMBEscStr + 1] = lpMBNoEscStr[ctr + 1];
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            break;
                        }
                    }
                    cchMBEscStr += 2;
                    ctr += 2;
                }
            }

            NLS_FREE_MEM(lpMBNoEscStr);

            //
            //  See if the output buffer is too small.
            //
            if ((cchMultiByte > 0) && (cchMBEscStr > cchMultiByte))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            return (cchMBEscStr);
        }
    }

    //
    //  This shouldn't happen since this is called by the NLS APIs.
    //
    SetLastError(ERROR_INVALID_PARAMETER);
    return (0);
}
