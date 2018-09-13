/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1994, Microsoft Corporation
 *
 *  WWMMAN.C
 *  WOW32 16-bit WifeMan API support (manually-coded thunks)
 *
 *  History:
 *  Created 17-May-1994 by hiroh
 *  Rewrote 12-May-1995 by hideyukn
 *
--*/
#include "precomp.h"
#pragma hdrstop

#ifdef FE_SB

#include "wowwife.h"
#include "wwmman.h"

MODNAME(wwmman.c);

STATIC LPSTR SkipSpaces(LPSTR lpch)
{
    if (lpch == NULL) return(NULL);

    for ( ; ; lpch++ ) {
        switch (*lpch) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;

        case '\0':
        // fall through...
        default:
            return(lpch);
        }
    }
}

#define EUDC_RANGE_KEY \
   (LPSTR) "SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage\\EUDCCodeRange"

ULONG FASTCALL WWM32MiscGetEUDCLeadByteRange(PVDMFRAME pFrame)
{
    unsigned short usEUDCRange = 0;
    unsigned char  chEUDCStart = 0, chEUDCEnd = 0;

    HKEY   hKey;
    ULONG  ulRet;
    CHAR   achACP[10];
    CHAR   achRange[256];

    DWORD  dwType;
    DWORD  dwRangeSize = sizeof(achRange);

    UNREFERENCED_PARAMETER(pFrame);

    //
    // Get EUDC Range
    //
    // In Win32, We support multiple EUDC Range, but for
    // Win16, we just return only first EUDC range to
    // keep backward compatibility...
    //

    ulRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, EUDC_RANGE_KEY,
                          (DWORD) 0, KEY_QUERY_VALUE, &hKey);
 
    if (ulRet != ERROR_SUCCESS) {
        #if DBG
        LOGDEBUG(0,("WOW32:RegOpenKeyExA(EUDC_RANGE_KEY) fail\n"));
        #endif
        return 0;
    }

    //
    // Convert ACP to string..
    //
    RtlIntegerToChar(GetACP(),10,sizeof(achACP),achACP);

    ulRet = RegQueryValueExA(hKey, achACP, (LPDWORD)NULL, (LPDWORD)&dwType,
                             (LPBYTE)achRange, &dwRangeSize);

    if (ulRet != ERROR_SUCCESS) {
        #if DBG
        LOGDEBUG(0,("WOW32:RegQueryValueExA(CP_ACP) fail\n"));
        #endif
        RegCloseKey(hKey);
        return 0;
    }

    RegCloseKey(hKey);

    //
    // Perse the data.
    //
    {
        LPSTR  pszData = achRange;
        USHORT usStart, usEnd;

        pszData = SkipSpaces(pszData);

        if ((*pszData) == '\0') {
            #if DBG
            LOGDEBUG(0,("WOW32:Parse First Data fail\n"));
            #endif
            return 0;
        }

        usStart = (USHORT)strtoul(pszData,&pszData,16);

        if ((pszData = WOW32_strchr(pszData,'-')) == NULL) {
            #if DBG
            LOGDEBUG(0,("WOW32:Find End Data fail\n"));
            #endif
            return 0;
        }

        //
        // Skip '-'..
        //
        pszData++;

        pszData = SkipSpaces(pszData);

        if ((*pszData) == '\0') {
            #if DBG
            LOGDEBUG(0,("WOW32:Parse End Data fail\n"));
            #endif
            return 0;
        }

        usEnd = (USHORT)strtoul(pszData,&pszData,16);

        //
        // Confirm the data sort order is correct
        //
        if (usStart > usEnd) {
            #if DBG
            LOGDEBUG(0,("WOW32:Invalid EUDC Range Order\n"));
            #endif
            return 0;
        }

        //
        // Get EUDC Start, End LeadByte...
        //
        chEUDCStart = HIBYTE(usStart);
        chEUDCEnd   = HIBYTE(usEnd);
    }

    //
    // Setup return value.
    //

    usEUDCRange = ((unsigned short)chEUDCEnd << 8) |
                  ((unsigned short)chEUDCStart     );

    #if DBG
    LOGDEBUG(10,("WOW32:EUDC Range = %x\n",usEUDCRange));
    #endif

    RETURN(usEUDCRange); 
}
#endif // FE_SB
