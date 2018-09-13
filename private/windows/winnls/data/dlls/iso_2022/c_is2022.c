/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_is2022.c

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
//  50220  ISO-2022-JP Japanese JIS X 0202-1984 with no             halfwidth Katakana
//  50221  ISO-2022-JP Japanese JIS X 0202-1984 with <ESC>(I    for halfwidth Katakana
//  50222  ISO-2022-JP Japanese JIS X 0201-1989 with <ESC>(J+SO for halfwidth Katakana
//                                           ;RFC 1468
//
//  50225  ISO-2022-KR Korean KSC-5601-1987  ;RFC 1557
//
//  50227  ISO 2022-CN Traditional Chinese   ;RFC 1922:CNS-11643-1,CNS-11643-2
//  50229  ISO 2022-CN Simplified  Chinese   ;RFC 1922:GB-2312-80
//
//  52936  HZ-GB2312   Simplified  Chinese
//
////////////////////////////////////////////////////////////////////////////



//
//  Include Files.
//

#include <share.h>




//
//  Macro Definitions.
//

#define NLS_CODEPAGE(cp)         (NLS_CP[(cp) % 10])

#define SHIFT_OUT                ((BYTE)0x0E)
#define SHIFT_IN                 ((BYTE)0x0F)
#define ESCAPE                   ((BYTE)0x1B)

#define LEADBYTE_HALFWIDTH       ((BYTE)0x8E)

#define MODE_ASCII               11
#define MODE_HALFWIDTH_KATAKANA  0
#define MODE_JIS_0208            1
#define MODE_JIS_0212            2
#define MODE_KSC_5601            5
#define MODE_HZ                  6
#define MODE_GB_2312             7
#define MODE_CNS_11643_1         9
#define MODE_CNS_11643_2         10




//
//  Global Variables.
//

DWORD NLS_CP[] =
{
   20932,    // 50220  ISO-2022-JP, MODE_HALFWIDTH_KATAKANA
   20932,    // 50221  ISO-2022-JP, MODE_JIS_0208
   20932,    // 50222  ISO-2022-JP, MODE_JIS_0212
   0,
   0,
   20949,    // 50225  ISO-2022-KR, MODE_KSC_5601
   20936,    // 52936  HZ-GB2312,   MODE_HZ
   20936,    // 50227  ISO-2022-CN, MODE_GB_2312
   0,
   20000,    // 50229  ISO-2022-CN, MODE_CNS_11643_1
   20000,    // 50229  ISO-2022-CN, MODE_CNS_11643_2
   0         //                     MODE_ASCII
};




//
//  Forward Declarations.
//

DWORD
ParseMB_CP5022J(
    DWORD CodePage,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount);

DWORD
ParseMB_CP5022_579(
    DWORD CodePage,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount);

DWORD
ParseMB_CP52936(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount);

DWORD
MBToWC_CP5022X(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);

DWORD
MBToWC_CP52936(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);





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
    DWORD NlsCodePage = NLS_CODEPAGE(CodePage);

    if (!IsValidCodePage(NlsCodePage))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    switch (dwFlags)
    {
        case ( NLS_CP_CPINFO ) :
        {
           memset(lpCPInfo, 0, sizeof(CPINFO));

           lpCPInfo->MaxCharSize    = 5;
           lpCPInfo->DefaultChar[0] = 0x3f;

           //
           //  The lead-byte does not apply here, leave them all NULL.
           //
           return (TRUE);
        }
        case ( NLS_CP_MBTOWC ) :
        {
            if (cchMultiByte == -1)
            {
                cchMultiByte = strlen(lpMultiByteStr) + 1;
            }

            switch (CodePage)
            {
                case (50220) :
                case (50221) :
                case (50222) :
                case (50225) :
                case (50227) :
                case (50229) :
                {
                    return (MBToWC_CP5022X( lpMultiByteStr,
                                            cchMultiByte,
                                            lpWideCharStr,
                                            cchWideChar ));
                }
                case (52936) :
                {
                    return (MBToWC_CP52936( lpMultiByteStr,
                                            cchMultiByte,
                                            lpWideCharStr,
                                            cchWideChar ));
                }
            }
            break;
        }
        case ( NLS_CP_WCTOMB ) :
        {
            int cchMBCount;
            LPSTR lpMBNoEscStr;

            if (cchWideChar == -1)
            {
                cchWideChar = wcslen(lpWideCharStr) + 1;
            }

            lpMBNoEscStr = (LPSTR)NLS_ALLOC_MEM(cchWideChar * sizeof(WCHAR));
            if (lpMBNoEscStr == NULL)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return (0);
            }

            cchMBCount = WideCharToMultiByte( NlsCodePage,
                                              WC_NO_BEST_FIT_CHARS,
                                              lpWideCharStr,
                                              cchWideChar,
                                              lpMBNoEscStr,
                                              cchMultiByte,
                                              NULL,
                                              NULL );
            if (cchMBCount != 0)
            {
                switch (CodePage)
                {
                    case (50220) :
                    case (50221) :
                    case (50222) :
                    {
                        cchMBCount = ParseMB_CP5022J( CodePage,
                                                      lpMultiByteStr,
                                                      cchMultiByte,
                                                      lpMBNoEscStr,
                                                      cchMBCount );
                        break;
                    }
                    case (50225) :
                    case (50227) :
                    case (50229) :
                    {
                        cchMBCount = ParseMB_CP5022_579( CodePage,
                                                         lpMultiByteStr,
                                                         cchMultiByte,
                                                         lpMBNoEscStr,
                                                         cchMBCount );
                        break;
                    }
                    case (52936) :
                    {
                        cchMBCount = ParseMB_CP52936( lpMultiByteStr,
                                                      cchMultiByte,
                                                      lpMBNoEscStr,
                                                      cchMBCount );
                        break;
                    }
                }
            }

            NLS_FREE_MEM (lpMBNoEscStr);

            return (cchMBCount);
        }
    }

    //
    //  This shouldn't happen since this function gets called by
    //  the NLS API routines.
    //
    SetLastError(ERROR_INVALID_PARAMETER);
    return (0);
}





//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  MBToWC_CP5022X
//
//  This routine does the translations from ISO-2022 to Unicode.
//
////////////////////////////////////////////////////////////////////////////

DWORD MBToWC_CP5022X(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    int ctr, cchMBTemp = 0, cchWCCount = 0;
    LPSTR lpMBTempStr,lpMBNoEscStr, lpMBStrStart;
    WORD wMode, wModePrev, wModeSO;
    LPWSTR lpWCTempStr;

    //
    //  Allocate a buffer of the appropriate size.
    //  Use sizeof(WCHAR) because size could potentially double if
    //  the buffer contains all halfwidth Katakanas
    //
    lpMBStrStart = (LPSTR)NLS_ALLOC_MEM(cchMultiByte * sizeof(WCHAR));
    if (lpMBStrStart == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (0);
    }

    lpWCTempStr = (LPWSTR)NLS_ALLOC_MEM(cchMultiByte  * sizeof(WCHAR));
    if (lpWCTempStr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (0);
    }

    if (cchWideChar)
    {
        *lpWideCharStr = 0;
    }

    lpMBTempStr = lpMBNoEscStr = lpMBStrStart;
    wModePrev = wMode = wModeSO = MODE_ASCII;

    //
    //  Remove esc sequence, then convert to Unicode.
    //
    for (ctr = 0; ctr < cchMultiByte;)
    {
        if ((BYTE)lpMultiByteStr[ctr] ==  ESCAPE)
        {
            wMode = wModeSO = MODE_ASCII;
            if (ctr >= (cchMultiByte - 2))
            {
                //
                //  Incomplete escape sequence.
                //
            }
            else if ((BYTE)lpMultiByteStr[ctr + 1] == '(')
            {
                if ((BYTE)lpMultiByteStr[ctr + 2] == 'B')       // <esc>(B
                {
                    wMode = wModeSO = MODE_ASCII;
                    ctr += 3;
                }
                else if ((BYTE)lpMultiByteStr[ctr + 2] == 'J')  // <esc>(J
                {
                    wMode = MODE_ASCII;
                    wModeSO = MODE_HALFWIDTH_KATAKANA;
                    ctr += 3;
                }
                else if ((BYTE)lpMultiByteStr[ctr + 2] == 'I')  // <esc>(I
                {
                    wMode = wModeSO = MODE_HALFWIDTH_KATAKANA;
                    ctr += 3;
                }
            }
            else if ((BYTE)lpMultiByteStr[ctr + 1] == '$')
            {
                if (((BYTE)lpMultiByteStr[ctr + 2] == '@') ||   // <esc>$@
                    ((BYTE)lpMultiByteStr[ctr + 2] == 'B'))     // <esc>$B
                {
                    wMode = wModeSO = MODE_JIS_0208;
                    ctr += 3;
                }
                else
                {
                    if (ctr >= (cchMultiByte - 3))
                    {
                        //
                        //  Imcomplete escape sequence.
                        //
                    }
                    else if ((BYTE)lpMultiByteStr[ctr + 2] == '(')
                    {
                        if (((BYTE)lpMultiByteStr[ctr + 3] == '@') ||  // <esc>$(@
                            ((BYTE)lpMultiByteStr[ctr + 3] == 'B'))    // <esc>$(B
                        {
                            wMode = wModeSO = MODE_JIS_0208;
                            ctr += 4;
                        }
                        else if ((BYTE)lpMultiByteStr[ctr + 3] == 'D') // <esc>$(D
                        {
                            wMode = wModeSO = MODE_JIS_0212;
                            ctr += 4;
                        }
                    }
                    else if ((BYTE)lpMultiByteStr[ctr + 2] == ')')
                    {
                        if ((BYTE)lpMultiByteStr[ctr + 3] == 'C')      // <esc>$)C
                        {
                            wMode = wModeSO = MODE_KSC_5601;
                            ctr += 4;
                        }
                        else if ((BYTE)lpMultiByteStr[ctr + 3] == 'A') // <esc>$)A
                        {
                            wMode = wModeSO = MODE_GB_2312;
                            ctr += 4;
                        }
                        else if ((BYTE)lpMultiByteStr[ctr + 3] == 'G') // <esc>$)G
                        {
                            wMode = wModeSO = MODE_CNS_11643_1;
                            ctr += 4;
                        }
                    }
                    else if (((BYTE)lpMultiByteStr[ctr + 2] == '*') && // <esc>$*H
                             ((BYTE)lpMultiByteStr[ctr + 3] == 'H'))
                    {
                        wMode = wModeSO = MODE_CNS_11643_2;
                        ctr += 4;
                    }
                }
            }
            else if (lpMultiByteStr[ctr + 1] == '&')
            {
                if (ctr >= (cchMultiByte - 5))
                {
                    //
                    //  Incomplete escape sequence.
                    //
                }
                else if (((BYTE)lpMultiByteStr[ctr + 2] == '@')     &&
                         ((BYTE)lpMultiByteStr[ctr + 3] ==  ESCAPE) &&
                         ((BYTE)lpMultiByteStr[ctr + 4] == '$')     &&
                         ((BYTE)lpMultiByteStr[ctr + 5] == 'B'))
                {
                    wMode = wModeSO = MODE_JIS_0208;
                    ctr += 6;
                }
            }
        }
        else if ((BYTE)lpMultiByteStr[ctr] == SHIFT_OUT)
        {
           wMode = wModeSO;
           ctr++;
        }
        else if ((BYTE)lpMultiByteStr[ctr] == SHIFT_IN)
        {
           wMode = MODE_ASCII;
           ctr++;
        }

        switch (wMode)
        {
            case ( MODE_JIS_0208 ) :
            case ( MODE_KSC_5601 ) :
            case ( MODE_GB_2312 ) :
            case ( MODE_CNS_11643_1 ) :
            {
                //
                //  To handle errors, we need to check:
                //    1. if trailbyte is there
                //    2. if code is valid
                //
                while (lpMultiByteStr[ctr] == SHIFT_OUT)
                {
                    ctr++;
                }

                while ((ctr < (cchMultiByte - 1))      &&
                       (lpMultiByteStr[ctr] != ESCAPE) &&
                       (lpMultiByteStr[ctr] != SHIFT_IN))
                {
                    *lpMBTempStr++ = lpMultiByteStr[ctr++] | 0x80;
                    *lpMBTempStr++ = lpMultiByteStr[ctr++] | 0x80;
                    cchMBTemp += 2;
                }

                break;
            }
            case ( MODE_JIS_0212 ) :
            case ( MODE_CNS_11643_2 ) :
            {
                while (lpMultiByteStr[ctr] == SHIFT_OUT)
                {
                    ctr++;
                }

                while ((ctr < (cchMultiByte - 1))      &&
                       (lpMultiByteStr[ctr] != ESCAPE) &&
                       (lpMultiByteStr[ctr] != SHIFT_IN))
                {
                    *lpMBTempStr++ = lpMultiByteStr[ctr++] | 0x80;
                    *lpMBTempStr++ = lpMultiByteStr[ctr++];
                    cchMBTemp += 2;
                }

                break;
            }
            case ( MODE_HALFWIDTH_KATAKANA ) :
            {
                while (lpMultiByteStr[ctr] == SHIFT_OUT)
                {
                    ctr++;
                }

                while ((ctr < cchMultiByte)            &&
                       (lpMultiByteStr[ctr] != ESCAPE) &&
                       (lpMultiByteStr[ctr] != SHIFT_IN))
                {
                    *lpMBTempStr++ = (BYTE)0x8E;
                    *lpMBTempStr++ = lpMultiByteStr[ctr++] | 0x80;
                    cchMBTemp += 2;
                }

                break;
            }
            default :                  // MODE_ASCII
            {
                while (lpMultiByteStr[ctr] == SHIFT_IN)
                {
                    ctr++;
                }

                while ((ctr < cchMultiByte)            &&
                       (lpMultiByteStr[ctr] != ESCAPE) &&
                       (lpMultiByteStr[ctr] != SHIFT_OUT))
                {
                    *lpMBTempStr++ = lpMultiByteStr[ctr++];
                    cchMBTemp++;
                }
            }
        }

        if (cchMBTemp == 0)
        {
            break;
        }

        cchWCCount += MultiByteToWideChar( NLS_CP[wMode],
                                           0,
                                           lpMBNoEscStr,
                                           cchMBTemp,
                                           lpWCTempStr,
                                           cchWideChar );
        if (cchWideChar)
        {
            if (cchWCCount > cchWideChar)
            {
                //
                //  Output buffer is too small.
                //
                break;
            }
            else
            {
                wcscat(lpWideCharStr, lpWCTempStr);
            }
        }
        lpMBNoEscStr += cchMBTemp;
        cchMBTemp = 0;
    }

    NLS_FREE_MEM(lpMBStrStart);
    NLS_FREE_MEM(lpWCTempStr);

    if (cchWideChar && (cchWCCount > cchWideChar))
    {
        //
        //  Output buffer is too small.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    return (cchWCCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMB_CP5022J
//
//  --> ISO-2022-JP
//
//  for 50220 : convert all halfwidth katakana to fullwidth
//      50221 : use <esc>(I     for halfwidth katakana
//      50222 : use <esc>(J<SO> for halfwidth katakana
//
////////////////////////////////////////////////////////////////////////////

DWORD ParseMB_CP5022J(
    DWORD CodePage,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount)
{
    int ctr, cchMBTemp = 0;
    WORD wMode, wModeSO;
    LPSTR lpMBTempStr;

    static WORD HalfToFullWidthKanaTable[] =
    {
        0xa1a3, // 0x8ea1 : Halfwidth Ideographic Period
        0xa1d6, // 0x8ea2 : Halfwidth Opening Corner Bracket
        0xa1d7, // 0x8ea3 : Halfwidth Closing Corner Bracket
        0xa1a2, // 0x8ea4 : Halfwidth Ideographic Comma
        0xa1a6, // 0x8ea5 : Halfwidth Katakana Middle Dot
        0xa5f2, // 0x8ea6 : Halfwidth Katakana Wo
        0xa5a1, // 0x8ea7 : Halfwidth Katakana Small A
        0xa5a3, // 0x8ea8 : Halfwidth Katakana Small I
        0xa5a5, // 0x8ea9 : Halfwidth Katakana Small U
        0xa5a7, // 0x8eaa : Halfwidth Katakana Small E
        0xa5a9, // 0x8eab : Halfwidth Katakana Small O
        0xa5e3, // 0x8eac : Halfwidth Katakana Small Ya
        0xa5e5, // 0x8ead : Halfwidth Katakana Small Yu
        0xa5e7, // 0x8eae : Halfwidth Katakana Small Yo
        0xa5c3, // 0x8eaf : Halfwidth Katakana Small Tu
        0xa1bc, // 0x8eb0 : Halfwidth Katakana-Hiragana Prolonged Sound Mark
        0xa5a2, // 0x8eb1 : Halfwidth Katakana A
        0xa5a4, // 0x8eb2 : Halfwidth Katakana I
        0xa5a6, // 0x8eb3 : Halfwidth Katakana U
        0xa5a8, // 0x8eb4 : Halfwidth Katakana E
        0xa5aa, // 0x8eb5 : Halfwidth Katakana O
        0xa5ab, // 0x8eb6 : Halfwidth Katakana Ka
        0xa5ad, // 0x8eb7 : Halfwidth Katakana Ki
        0xa5af, // 0x8eb8 : Halfwidth Katakana Ku
        0xa5b1, // 0x8eb9 : Halfwidth Katakana Ke
        0xa5b3, // 0x8eba : Halfwidth Katakana Ko
        0xa5b5, // 0x8ebb : Halfwidth Katakana Sa
        0xa5b7, // 0x8ebc : Halfwidth Katakana Si
        0xa5b9, // 0x8ebd : Halfwidth Katakana Su
        0xa5bb, // 0x8ebe : Halfwidth Katakana Se
        0xa5bd, // 0x8ebf : Halfwidth Katakana So
        0xa5bf, // 0x8ec0 : Halfwidth Katakana Ta
        0xa5c1, // 0x8ec1 : Halfwidth Katakana Ti
        0xa5c4, // 0x8ec2 : Halfwidth Katakana Tu
        0xa5c6, // 0x8ec3 : Halfwidth Katakana Te
        0xa5c8, // 0x8ec4 : Halfwidth Katakana To
        0xa5ca, // 0x8ec5 : Halfwidth Katakana Na
        0xa5cb, // 0x8ec6 : Halfwidth Katakana Ni
        0xa5cc, // 0x8ec7 : Halfwidth Katakana Nu
        0xa5cd, // 0x8ec8 : Halfwidth Katakana Ne
        0xa5ce, // 0x8ec9 : Halfwidth Katakana No
        0xa5cf, // 0x8eca : Halfwidth Katakana Ha
        0xa5d2, // 0x8ecb : Halfwidth Katakana Hi
        0xa5d5, // 0x8ecc : Halfwidth Katakana Hu
        0xa5d8, // 0x8ecd : Halfwidth Katakana He
        0xa5db, // 0x8ece : Halfwidth Katakana Ho
        0xa5de, // 0x8ecf : Halfwidth Katakana Ma
        0xa5df, // 0x8ed0 : Halfwidth Katakana Mi
        0xa5e0, // 0x8ed1 : Halfwidth Katakana Mu
        0xa5e1, // 0x8ed2 : Halfwidth Katakana Me
        0xa5e2, // 0x8ed3 : Halfwidth Katakana Mo
        0xa5e4, // 0x8ed4 : Halfwidth Katakana Ya
        0xa5e6, // 0x8ed5 : Halfwidth Katakana Yu
        0xa5e8, // 0x8ed6 : Halfwidth Katakana Yo
        0xa5e9, // 0x8ed7 : Halfwidth Katakana Ra
        0xa5ea, // 0x8ed8 : Halfwidth Katakana Ri
        0xa5eb, // 0x8ed9 : Halfwidth Katakana Ru
        0xa5ec, // 0x8eda : Halfwidth Katakana Re
        0xa5ed, // 0x8edb : Halfwidth Katakana Ro
        0xa5ef, // 0x8edc : Halfwidth Katakana Wa
        0xa5f3, // 0x8edd : Halfwidth Katakana N
        0xa1ab, // 0x8ede : Halfwidth Katakana Voiced Sound Mark
        0xa1ac  // 0x8edf : Halfwidth Katakana Semi-Voiced Sound Mark
    };

    wMode = wModeSO = MODE_ASCII;

    //
    //  Code page 50220 does not use halfwidth Katakana.
    //  Convert to fullwidth.
    //
    if (CodePage == 50220)
    {
        int ctr;

        for (ctr = 0; ctr < cchMBCount; ctr++)
        {
            WORD wFWKana;

            if ((BYTE)lpMBNoEscStr[ctr] == LEADBYTE_HALFWIDTH)
            {
                wFWKana = HalfToFullWidthKanaTable[(BYTE)lpMBNoEscStr[ctr + 1] - 0xA1];
                lpMBNoEscStr[ctr++] = HIBYTE(wFWKana);
                lpMBNoEscStr[ctr]   = LOBYTE(wFWKana);
            }
        }
    }

    lpMBTempStr = lpMultiByteStr;

    for (ctr = 0; ctr < cchMBCount; ctr++)
    {
        if ((BYTE)lpMBNoEscStr[ctr] == LEADBYTE_HALFWIDTH)
        {
            //
            //  It's halfwidth Katakana.
            //
            ctr++;
            if (CodePage == 50222)
            {
                if (wMode != MODE_HALFWIDTH_KATAKANA)
                {
                    if (wModeSO != MODE_HALFWIDTH_KATAKANA)
                    {
                        if (cchMultiByte)
                        {
                            if (cchMBTemp < (cchMultiByte - 2))
                            {
                                *lpMBTempStr++ = ESCAPE;
                                *lpMBTempStr++ = '(';
                                *lpMBTempStr++ = 'J';
                            }
                            else
                            {
                                //
                                //  Output buffer is too small.
                                //
                                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                                return (0);
                            }
                        }
                       cchMBTemp += 3;
                       wModeSO = MODE_HALFWIDTH_KATAKANA;
                    }
                    *lpMBTempStr++ = SHIFT_OUT;
                    cchMBTemp += 1;
                    wMode = MODE_HALFWIDTH_KATAKANA;
                }
            }
            else                  // CodePage = 50221
            {
                if (wMode != MODE_HALFWIDTH_KATAKANA)
                {
                    if (cchMultiByte)
                    {
                        if (cchMBTemp < (cchMultiByte - 2))
                        {
                            *lpMBTempStr++ = ESCAPE;
                            *lpMBTempStr++ = '(';
                            *lpMBTempStr++ = 'I';
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return (0);
                        }
                    }
                    cchMBTemp += 3;
                    wMode = MODE_HALFWIDTH_KATAKANA;
                }
            }

            if (cchMultiByte)
            {
                if (cchMBTemp < cchMultiByte)
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr] & 0x7F;
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp++;
        }
        else if (IsDBCSLeadByteEx(20932, lpMBNoEscStr[ctr]))
        {
            //
            //  It's a double byte character.
            //
            if (lpMBNoEscStr[ctr + 1] & 0x80)  // JIS X 0208
            {
                if (wMode != MODE_JIS_0208)
                {
                    if (cchMultiByte)
                    {
                        if (cchMBTemp < (cchMultiByte - 2))
                        {
                            *lpMBTempStr++ = ESCAPE;
                            *lpMBTempStr++ = '$';
                            *lpMBTempStr++ = 'B';
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return (0);
                        }
                    }
                    cchMBTemp += 3;
                    wMode = MODE_JIS_0208;
                }
            }
            else                              // JIS X 0212
            {
                if (wMode != MODE_JIS_0212)
                {
                    if (cchMultiByte)
                    {
                        if (cchMBTemp < (cchMultiByte - 3))
                        {
                            *lpMBTempStr++ = ESCAPE;
                            *lpMBTempStr++ = '$';
                            *lpMBTempStr++ = '(';
                            *lpMBTempStr++ = 'D';
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return (0);
                        }
                    }
                    cchMBTemp += 4;
                    wMode = MODE_JIS_0212;
                }
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
                if(cchMBTemp < (cchMultiByte - 1))
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr++] & 0x7F;
                    *lpMBTempStr++ = lpMBNoEscStr[ctr]   & 0x7F;
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp += 2;
        }
        else                      // Single byte Char
        {
            if (wMode != MODE_ASCII)
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp < (cchMultiByte - 2))
                    {
                        *lpMBTempStr++ = ESCAPE;
                        *lpMBTempStr++ = '(';
                        *lpMBTempStr++ = 'B';
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                cchMBTemp += 3;
                wMode = MODE_ASCII;
            }

            if (cchMultiByte)
            {
                if (cchMBTemp < cchMultiByte)
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr];
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp++;
        }
    }

    if (cchMultiByte && (cchMBTemp > cchMultiByte))
    {
        //
        //  Output buffer is too small.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    return (cchMBTemp);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMB_CP5022_579
//
//  KSC --> ISO-2022-KR (CP-50225)
//  GB  --> ISO-2022-CN (CP-50227)
//  CNS --> ISO-2022-CN (CP-50229)
//
////////////////////////////////////////////////////////////////////////////

DWORD ParseMB_CP5022_579(
    DWORD CodePage,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount)
{
    int ctr, cchMBTemp = 0;
    WORD wMode, wModeSO, wModeCP;
    char EscChar;
    LPSTR lpMBTempStr;

    lpMBTempStr = lpMultiByteStr;
    wMode = wModeSO = MODE_ASCII;
    wModeCP = (WORD)(CodePage % 10);
    EscChar = ( wModeCP == MODE_KSC_5601 ? 'C' :
               (wModeCP == MODE_GB_2312  ? 'A' : 'G'));

    for (ctr = 0; ctr < cchMBCount; ctr++)
    {
        if (IsDBCSLeadByteEx(NLS_CODEPAGE(CodePage), lpMBNoEscStr[ctr]))
        {
            //
            //  It's a double byte character.
            //
            if (lpMBNoEscStr[ctr + 1] & 0x80)         // KSC, GB or CNS-1
            {
                if (wModeSO != wModeCP)
                {
                    if (cchMultiByte)
                    {
                        if (cchMBTemp < (cchMultiByte - 3))
                        {
                            *lpMBTempStr++ = ESCAPE;
                            *lpMBTempStr++ = '$';
                            *lpMBTempStr++ = ')';
                            *lpMBTempStr++ = EscChar;
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return (0);
                        }
                    }
                    cchMBTemp += 4;
                    wModeSO = wModeCP;
                }
            }
            else
            {
                //
                //  lpMBNoEscStr[ctr + 1] & 0x80 == 0 indicates CNS-2
                //
                if (wModeSO != MODE_CNS_11643_2)
                {
                    if (cchMultiByte)
                    {
                        if (cchMBTemp<(cchMultiByte-3))
                        {
                            *lpMBTempStr++ = ESCAPE;
                            *lpMBTempStr++ = '$';
                            *lpMBTempStr++ = '*';
                            *lpMBTempStr++ = 'H';
                        }
                        else
                        {
                            //
                            //  Output buffer is too small.
                            //
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return (0);
                        }
                    }
                    cchMBTemp += 4;
                    wModeSO = MODE_CNS_11643_2;
                }
            }

            if (wMode == MODE_ASCII)
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp<cchMultiByte)
                    {
                        *lpMBTempStr++ = SHIFT_OUT;
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                cchMBTemp++;
                wMode = wModeSO;
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
                if (cchMBTemp < (cchMultiByte - 1))
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr]   & 0x7F;
                    *lpMBTempStr++ = lpMBNoEscStr[++ctr] & 0x7F;
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp += 2;
        }
        else
        {
            //
            //  It's a single byte character.
            //
            if (wMode != MODE_ASCII)
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp<cchMultiByte)
                    {
                        *lpMBTempStr++ = SHIFT_IN;
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                cchMBTemp++;
                wMode = MODE_ASCII;
            }

            if (cchMultiByte)
            {
                if (cchMBTemp<cchMultiByte)
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr];
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp++;
        }
    }

    if (cchMultiByte && (cchMBTemp > cchMultiByte))
    {
        //
        //  Output buffer is too small.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    return (cchMBTemp);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMB_CP52936
//
//  GB-2312 --> HZ (CP-52936)
//
////////////////////////////////////////////////////////////////////////////

DWORD ParseMB_CP52936(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPSTR lpMBNoEscStr,
    int cchMBCount)
{
    int ctr, cchMBTemp = 0;
    WORD wMode;
    LPSTR lpMBTempStr;

    lpMBTempStr = lpMultiByteStr;
    wMode = MODE_ASCII;

    for (ctr = 0; ctr < cchMBCount; ctr++)
    {
        if (lpMBNoEscStr[ctr] & 0x80)
        {
            if (wMode != MODE_HZ)
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp < (cchMultiByte - 1))
                    {
                        *lpMBTempStr++ = '~';
                        *lpMBTempStr++ = '{';
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                wMode = MODE_HZ;
                cchMBTemp += 2;
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
                if (cchMBTemp < (cchMultiByte - 1))
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr++] & 0x7F;
                    *lpMBTempStr++ = lpMBNoEscStr[ctr]   & 0x7F;
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp += 2;
        }
        else
        {
            if (wMode != MODE_ASCII)
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp < (cchMultiByte - 1))
                    {
                        *lpMBTempStr++ = '~';
                        *lpMBTempStr++ = '}';
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                wMode = MODE_ASCII;
                cchMBTemp += 2;
            }

            if ((BYTE)lpMBNoEscStr[ctr] == '~')
            {
                if (cchMultiByte)
                {
                    if (cchMBTemp < cchMultiByte)
                    {
                        *lpMBTempStr++ = '~';
                    }
                    else
                    {
                        //
                        //  Output buffer is too small.
                        //
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return (0);
                    }
                }
                cchMBTemp++;
            }

            if (cchMultiByte)
            {
                if (cchMBTemp < cchMultiByte)
                {
                    *lpMBTempStr++ = lpMBNoEscStr[ctr];
                }
                else
                {
                    //
                    //  Output buffer is too small.
                    //
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
            }
            cchMBTemp++;
        }
    }

    if (cchMultiByte && (cchMBTemp > cchMultiByte))
    {
        //
        //  Output buffer is too small.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    return (cchMBTemp);
}


////////////////////////////////////////////////////////////////////////////
//
//  MBToWC_CP52936
//
//  HZ (CP-52936) --> Unicode
//
////////////////////////////////////////////////////////////////////////////

DWORD MBToWC_CP52936(
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    int ctr, cchMBTemp, cchWCCount;
    WORD wMode;
    LPSTR lpMBNoEscStr;

    lpMBNoEscStr = (LPSTR)NLS_ALLOC_MEM(cchMultiByte * sizeof(WCHAR));
    if (lpMBNoEscStr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (0);
    }

    cchMBTemp = 0;
    wMode = MODE_ASCII;

    for (ctr = 0; ctr < cchMultiByte; ctr++)
    {
        if (((BYTE)lpMultiByteStr[ctr] == '~') && (ctr < (cchMultiByte - 1)))
        {
            if ((BYTE)lpMultiByteStr[ctr + 1] == '{')
            {
                wMode = MODE_HZ;
                ctr += 2;
            }
            else if ((BYTE)lpMultiByteStr[ctr + 1] == '}')
            {
                wMode = MODE_ASCII;
                ctr += 2;
            }
            else if ((BYTE)lpMultiByteStr[ctr + 1] == '~')
            {
                ctr++;
            }
            else if (((BYTE)lpMultiByteStr[ctr + 1] == '\\') &&
                     (ctr < (cchMultiByte - 2))              &&
                     (((BYTE)lpMultiByteStr[ctr + 2] == 'n')  ||
                      ((BYTE)lpMultiByteStr[ctr + 2] == 'N' )))
            {
                ctr += 2;
            }
        }

        if (wMode == MODE_HZ)
        {
            if (ctr < (cchMultiByte - 1))
            {
                lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr++] | 0x80;
                lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr]   | 0x80;
            }
        }
        else
        {
            if (ctr < cchMultiByte)
            {
                lpMBNoEscStr[cchMBTemp++] = lpMultiByteStr[ctr];
            }
        }
    }

    cchWCCount = MultiByteToWideChar ( 20936,
                                       0,
                                       lpMBNoEscStr,
                                       cchMBTemp,
                                       lpWideCharStr,
                                       cchWideChar );
    NLS_FREE_MEM(lpMBNoEscStr);

    return (cchWCCount);
}
