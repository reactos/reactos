// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   country2.c
//
//  PURPOSE:   Console IME control.
//             FarEast country specific module 2 for conime.
//
//  PLATFORMS: Windows NT-FE 3.51
//
//  FUNCTIONS:
//    ImeUIOpenCandidate() - routine for make system line string
//
//  History:
//
//  15.Jul.1996 v-HirShi (Hirotoshi Shimizu)    Created for TAIWAN & KOREA & PRC
//
//  COMMENTS:
//
#include "precomp.h"
#pragma hdrstop


BOOL
ImeUIOpenCandidate(
    HWND hwnd,
    DWORD CandList,
    BOOL OpenFlag
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC  hIMC;

    DBGPRINT(("CONIME: Get IMN_OPENCANDIDATE Message\n"));


    ConTbl = SearchConsole(LastConsole);

    if (ConTbl->fNestCandidate)
        return TRUE;

    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
        return FALSE;

    //
    // Set fInCandidate variables.
    //
    ConTbl->fInCandidate = TRUE;

    switch ( HKL_TO_LANGID(ConTbl->hklActive))
    {
        case    LANG_ID_JAPAN:
            OpenCandidateJapan(hwnd, hIMC, ConTbl, CandList, OpenFlag );
            break;
        case    LANG_ID_TAIWAN:
            OpenCandidateTaiwan(hwnd, hIMC, ConTbl, CandList, OpenFlag );
            break;
        case    LANG_ID_PRC:
            OpenCandidatePRC(hwnd, hIMC, ConTbl, CandList, OpenFlag );
            break;
        case    LANG_ID_KOREA:
            OpenCandidateKorea(hwnd, hIMC, ConTbl, CandList, OpenFlag );
            break;
        default:
            return FALSE;
    }

    ImmReleaseContext( hwnd, hIMC );

    return TRUE;
}


BOOL
OpenCandidateJapan(
    HWND hwnd,
    HIMC  hIMC ,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList,
    BOOL OpenFlag
    )
{
    DWORD dwLength;
    DWORD dwIndex;
    DWORD i;
    DWORD j;
    LPWSTR lpStr;
    DWORD dwDspLen;
    DWORD width;
    DWORD StartIndex;
    DWORD CountDispWidth;
    DWORD AllocLength;
    LPCANDIDATELIST lpCandList;
    LPCONIME_CANDMESSAGE SystemLine;
    DWORD SystemLineSize;
    COPYDATASTRUCT CopyData;
    BOOL EnableCodePoint;

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            dwLength = ImmGetCandidateList(hIMC, dwIndex, NULL, 0);
            if (dwLength == 0)
                return FALSE;
            if ( (ConTbl->CandListMemAllocSize[dwIndex] != dwLength ) &&
                 (ConTbl->lpCandListMem[dwIndex] != NULL)) {
                LocalFree(ConTbl->lpCandListMem[dwIndex]);
                ConTbl->CandListMemAllocSize[dwIndex] = 0;
                ConTbl->lpCandListMem[dwIndex] = NULL;
            }
            if (ConTbl->lpCandListMem[dwIndex] == NULL) {
                ConTbl->lpCandListMem[dwIndex] = LocalAlloc(LPTR, dwLength);
                if (ConTbl->lpCandListMem[dwIndex] == NULL)
                    return FALSE;
                ConTbl->CandListMemAllocSize[dwIndex] = dwLength;
            }
            lpCandList = ConTbl->lpCandListMem[dwIndex];
            ImmGetCandidateList(hIMC, dwIndex, lpCandList, dwLength);

            //
            // check each offset value is not over than buffer size.
            //
            if ((lpCandList->dwCount > 1) &&
                (lpCandList->dwSelection >= dwLength ||
                 lpCandList->dwPageStart >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwSelection] >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwPageStart] >= dwLength    )
               )
                break;

            dwLength = ConTbl->ScreenBufferSize.X;
            dwLength = (dwLength > 128) ? 128 : dwLength ;
            dwLength = ((dwLength < 12) ? 12 : dwLength );

            j = (dwLength-7)/(DELIMITERWIDTH+sizeof(WCHAR));
            j = ((j > 9)?9:j);
            j = lpCandList->dwCount / j + 10;
            AllocLength = (j > DEFAULTCANDTABLE) ? j : DEFAULTCANDTABLE;

            if (lpCandList->dwStyle == IME_CAND_CODE){
                EnableCodePoint = TRUE;
            }
            else{
                EnableCodePoint = FALSE;
            }

            if (EnableCodePoint){
                CountDispWidth = CODEDISPLEN;
            }
            else {
                for (CountDispWidth = 0 ,j = 1; j <= lpCandList->dwCount; CountDispWidth++)
                     j *= 10;
                CountDispWidth *= 2;
                CountDispWidth++;
            }

            if ((ConTbl->CandSepAllocSize != sizeof(DWORD)*AllocLength) &&
                (ConTbl->CandSep != NULL)) {
                LocalFree(ConTbl->CandSep);
                ConTbl->CandSep = NULL;
                ConTbl->CandSepAllocSize = 0;
            }
            if (ConTbl->CandSep == NULL) {
                ConTbl->CandSep= LocalAlloc(LPTR, sizeof(DWORD)*AllocLength);
                if (ConTbl->CandSep == NULL)
                    return FALSE;
                ConTbl->CandSepAllocSize = sizeof(DWORD)*AllocLength;
            }

            if ( EnableCodePoint ){
                j = 0;
                ConTbl->CandSep[j++] = 0;
                if (OpenFlag) {
                    ConTbl->CandOff = lpCandList->dwSelection % 9;
                }
                i = ConTbl->CandOff;
                for (; i < lpCandList->dwCount; i+= 9 ) {
                    ConTbl->CandSep[j++] = i;
                }
                if (i > lpCandList->dwCount) {
                    i = lpCandList->dwCount;
                }
            }
            else{
                j = 0;
                if (OpenFlag) {
                    ConTbl->CandOff = 0;
                }
                ConTbl->CandSep[j++] = ConTbl->CandOff;
                lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ 0 ]);
                dwDspLen = DispLenUnicode( lpStr );
                width = dwDspLen + DELIMITERWIDTH;                  // '1:xxxx 2:xxxx '
                for( i = 1; i < lpCandList->dwCount; i++ ) {
                    lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
                    dwDspLen = DispLenUnicode( lpStr );
                    width += dwDspLen + DELIMITERWIDTH;
                    if ((width > dwLength-CountDispWidth) ||
                        ( i - ConTbl->CandSep[j-1] >= 9)){
                        ConTbl->CandSep[j++] = i;
                        width = dwDspLen + DELIMITERWIDTH;
                    }
                }
            }
            ConTbl->CandSep[j] = i;
            ConTbl->CandMax = j;

            SystemLineSize = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            if (ConTbl->SystemLineSize < SystemLineSize ){
                if (ConTbl->SystemLine != NULL){
                    LocalFree( ConTbl->SystemLine );
                    ConTbl->SystemLine = NULL;
                    ConTbl->SystemLineSize = 0;
                }
                ConTbl->SystemLine = (LPCONIME_CANDMESSAGE)LocalAlloc(LPTR, SystemLineSize );
                if (ConTbl->SystemLine == NULL) {
                    return FALSE;
                }
                ConTbl->SystemLineSize = SystemLineSize;
            }
            SystemLine = ConTbl->SystemLine;

            SystemLine->AttrOff = sizeof(WCHAR) * dwLength + sizeof(DWORD);

            CopyData.dwData = CI_CONIMECANDINFO;
            CopyData.cbData = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            CopyData.lpData = SystemLine;
            StartIndex = GetSystemLineJ( lpCandList,
                           SystemLine->String,
                           (LPSTR)SystemLine + SystemLine->AttrOff,
                           dwLength,
                           CountDispWidth,
                           ConTbl,
                           EnableCodePoint);

            ConTbl->fNestCandidate = TRUE;    // ImmNotyfyIME call back OpenCandidate Message
                                        // by same data.
                                        // so We ignore this mesage.
            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESTART,
                         dwIndex,
                         ConTbl->CandSep[StartIndex]);

            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESIZE,
                         dwIndex,
                         ConTbl->CandSep[StartIndex+1] -
                         ConTbl->CandSep[StartIndex]);
            ConTbl->fNestCandidate = FALSE;

            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                  );
        }
    }
    return TRUE;
}

BOOL
OpenCandidateTaiwan(
    HWND hwnd,
    HIMC  hIMC ,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList,
    BOOL OpenFlag
    )
{
    DWORD dwLength;
    DWORD dwIndex;
    DWORD i;
    DWORD j;
    LPWSTR lpStr;
    DWORD dwDspLen;
    DWORD width;
    DWORD StartIndex;
    DWORD CountDispWidth;
    DWORD AllocLength;
    LPCANDIDATELIST lpCandList;
    LPCONIME_CANDMESSAGE SystemLine;
    DWORD SystemLineSize;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            dwLength = ImmGetCandidateList(hIMC, dwIndex, NULL, 0);
            if (dwLength == 0)
                return FALSE;
            if ( (ConTbl->CandListMemAllocSize[dwIndex] != dwLength ) &&
                 (ConTbl->lpCandListMem[dwIndex] != NULL)) {
                LocalFree(ConTbl->lpCandListMem[dwIndex]);
                ConTbl->CandListMemAllocSize[dwIndex] = 0;
                ConTbl->lpCandListMem[dwIndex] = NULL;
            }
            if (ConTbl->lpCandListMem[dwIndex] == NULL) {
                ConTbl->lpCandListMem[dwIndex] = LocalAlloc(LPTR, dwLength);
                if (ConTbl->lpCandListMem[dwIndex] == NULL)
                    return FALSE;
                ConTbl->CandListMemAllocSize[dwIndex] = dwLength;
            }
            lpCandList = ConTbl->lpCandListMem[dwIndex];
            ImmGetCandidateList(hIMC, dwIndex, lpCandList, dwLength);

            //
            // check each offset value is not over than buffer size.
            //
            if ((lpCandList->dwCount > 1) &&
                (lpCandList->dwSelection >= dwLength ||
                 lpCandList->dwPageStart >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwSelection] >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwPageStart] >= dwLength    )
               )
                break;

            dwLength = ConTbl->ScreenBufferSize.X;
            dwLength = (dwLength > 128) ? 128 : dwLength ;
            dwLength = ((dwLength < 12) ? 12 : dwLength );
#if defined (CANDCOUNTCHT) //for wider candidate list space v-hirshi Oct.16.1996
            dwLength -= 28; // 6+1+4+1+10+1+....+4+1
#else
            dwLength -= IMECNameLength+1+IMECModeFullShapeLen*2+1; // 4+1+2+1+....
#endif

            j = (dwLength-7)/(DELIMITERWIDTH+sizeof(WCHAR));
            j = ((j > 9)?9:j);
            j = lpCandList->dwCount / j + 10;
            AllocLength = (j > DEFAULTCANDTABLE) ? j : DEFAULTCANDTABLE;

            for (CountDispWidth = 0 ,j = 1; j <= lpCandList->dwCount; CountDispWidth++)
                 j *= 10;
            CountDispWidth *= 2;
            CountDispWidth++;

            if ((ConTbl->CandSepAllocSize != sizeof(DWORD)*AllocLength) &&
                (ConTbl->CandSep != NULL)) {
                LocalFree(ConTbl->CandSep);
                ConTbl->CandSep = NULL;
                ConTbl->CandSepAllocSize = 0;
            }
            if (ConTbl->CandSep == NULL) {
                ConTbl->CandSep= LocalAlloc(LPTR, sizeof(DWORD)*AllocLength);
                if (ConTbl->CandSep == NULL)
                    return FALSE;
                ConTbl->CandSepAllocSize = sizeof(DWORD)*AllocLength;
            }

            j = 0;
            if (OpenFlag) {
                ConTbl->CandOff = 0;
            }
            ConTbl->CandSep[j++] = ConTbl->CandOff;
            lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ 0 ]);
            dwDspLen = DispLenUnicode( lpStr );
            width = dwDspLen + DELIMITERWIDTH;                  // '1:xxxx 2:xxxx '
            for( i = 1; i < lpCandList->dwCount; i++ ) {
                lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
                dwDspLen = DispLenUnicode( lpStr );
                width += dwDspLen + DELIMITERWIDTH;
                if ((width > dwLength-CountDispWidth) ||
                    ( i - ConTbl->CandSep[j-1] >= 9)){
                    ConTbl->CandSep[j++] = i;
                    width = dwDspLen + DELIMITERWIDTH;
                }
            }
            ConTbl->CandSep[j] = i;
            ConTbl->CandMax = j;

            SystemLineSize = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            if (ConTbl->SystemLineSize < SystemLineSize ){
                if (ConTbl->SystemLine != NULL){
                    LocalFree( ConTbl->SystemLine );
                    ConTbl->SystemLine = NULL;
                    ConTbl->SystemLineSize = 0;
                }
                ConTbl->SystemLine = (LPCONIME_CANDMESSAGE)LocalAlloc(LPTR, SystemLineSize );
                if (ConTbl->SystemLine == NULL) {
                    return FALSE;
                }
                ConTbl->SystemLineSize = SystemLineSize;
            }
            SystemLine = ConTbl->SystemLine;

            SystemLine->AttrOff = sizeof(WCHAR) * dwLength + sizeof(DWORD);

            StartIndex = GetSystemLineT( lpCandList,
                           SystemLine->String,
                           (LPSTR)SystemLine + SystemLine->AttrOff,
                           dwLength,
                           CountDispWidth,
                           ConTbl
                           );

            ConTbl->fNestCandidate = TRUE;    // ImmNotyfyIME call back OpenCandidate Message
                                        // by same data.
                                        // so We ignore this mesage.
            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESTART,
                         dwIndex,
                         ConTbl->CandSep[StartIndex]);

            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESIZE,
                         dwIndex,
                         ConTbl->CandSep[StartIndex+1] -
                         ConTbl->CandSep[StartIndex]);
            ConTbl->fNestCandidate = FALSE;

            CopyData.dwData = CI_CONIMEMODEINFO;
            CopyData.cbData = sizeof(CONIME_UIMODEINFO);
            CopyData.lpData = lpModeInfo;
            if (MakeInfoStringTaiwan(ConTbl, lpModeInfo) ) {
                ConsoleImeSendMessage( ConTbl->hWndCon,
                                       (WPARAM)hwnd,
                                       (LPARAM)&CopyData
                                     );
            }
        }
    }
    LocalFree( lpModeInfo );
    return TRUE;
}

BOOL
OpenCandidatePRC(
    HWND hwnd,
    HIMC  hIMC ,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList,
    BOOL OpenFlag
    )
{
    DWORD dwLength;
    DWORD dwIndex;
    DWORD i;
    DWORD j;
    LPWSTR lpStr;
    DWORD dwDspLen;
    DWORD width;
    DWORD StartIndex;
    DWORD CountDispWidth;
    DWORD AllocLength;
    LPCANDIDATELIST lpCandList;
    LPCONIME_CANDMESSAGE SystemLine;
    DWORD SystemLineSize;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            dwLength = ImmGetCandidateList(hIMC, dwIndex, NULL, 0);
            if (dwLength == 0)
                return FALSE;
            if ( (ConTbl->CandListMemAllocSize[dwIndex] != dwLength ) &&
                 (ConTbl->lpCandListMem[dwIndex] != NULL)) {
                LocalFree(ConTbl->lpCandListMem[dwIndex]);
                ConTbl->CandListMemAllocSize[dwIndex] = 0;
                ConTbl->lpCandListMem[dwIndex] = NULL;
            }
            if (ConTbl->lpCandListMem[dwIndex] == NULL) {
                ConTbl->lpCandListMem[dwIndex] = LocalAlloc(LPTR, dwLength);
                if (ConTbl->lpCandListMem[dwIndex] == NULL)
                    return FALSE;
                ConTbl->CandListMemAllocSize[dwIndex] = dwLength;
            }
            lpCandList = ConTbl->lpCandListMem[dwIndex];
            ImmGetCandidateList(hIMC, dwIndex, lpCandList, dwLength);

            //
            // check each offset value is not over than buffer size.
            //
            if ((lpCandList->dwCount > 1) &&
                (lpCandList->dwSelection >= dwLength ||
                 lpCandList->dwPageStart >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwSelection] >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwPageStart] >= dwLength    )
               )
                break;

            dwLength = ConTbl->ScreenBufferSize.X;
            dwLength = (dwLength > 128) ? 128 : dwLength ;
            dwLength = ((dwLength < 12) ? 12 : dwLength );
#if defined (CANDCOUNTPRC) //for wider candidate list space v-hirshi Oct.16.1996
            dwLength -= (20 + PRCCOMPWIDTH); //(8+1+4+1+PRCCOMPWIDTH+1+...+5)
#else
            dwLength -= (15 + PRCCOMPWIDTH); //(8+1+4+1+PRCCOMPWIDTH+1+...)
#endif

            j = (dwLength-7)/(DELIMITERWIDTH+sizeof(WCHAR));
            j = ((j > 9)?9:j);
            j = lpCandList->dwCount / j + 10;
            AllocLength = (j > DEFAULTCANDTABLE) ? j : DEFAULTCANDTABLE;

#if defined (CANDCOUNTPRC) //for wider candidate list space v-hirshi Oct.16.1996
            for (CountDispWidth = 0 ,j = 1; j <= lpCandList->dwCount; CountDispWidth++)
                 j *= 10;
            CountDispWidth *= 2;
            CountDispWidth++;
#else
            CountDispWidth = 0;
#endif

            if ((ConTbl->CandSepAllocSize != sizeof(DWORD)*AllocLength) &&
                (ConTbl->CandSep != NULL)) {
                LocalFree(ConTbl->CandSep);
                ConTbl->CandSep = NULL;
                ConTbl->CandSepAllocSize = 0;
            }
            if (ConTbl->CandSep == NULL) {
                ConTbl->CandSep= LocalAlloc(LPTR, sizeof(DWORD)*AllocLength);
                if (ConTbl->CandSep == NULL)
                    return FALSE;
                ConTbl->CandSepAllocSize = sizeof(DWORD)*AllocLength;
            }

            j = 0;
            if (OpenFlag) {
                ConTbl->CandOff = 0;
            }
            ConTbl->CandSep[j++] = ConTbl->CandOff;
            lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ 0 ]);
            dwDspLen = DispLenUnicode( lpStr );
            width = dwDspLen + DELIMITERWIDTH;                  // '1:xxxx 2:xxxx '
            for( i = 1; i < lpCandList->dwCount; i++ ) {
                lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
                dwDspLen = DispLenUnicode( lpStr );
                width += dwDspLen + DELIMITERWIDTH;
                if ((width > dwLength-CountDispWidth) ||
                    ( i - ConTbl->CandSep[j-1] >= 9)){
                    ConTbl->CandSep[j++] = i;
                    width = dwDspLen + DELIMITERWIDTH;
                }
            }
            ConTbl->CandSep[j] = i;
            ConTbl->CandMax = j;

            SystemLineSize = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            if (ConTbl->SystemLineSize < SystemLineSize ){
                if (ConTbl->SystemLine != NULL){
                    LocalFree( ConTbl->SystemLine );
                    ConTbl->SystemLine = NULL;
                    ConTbl->SystemLineSize = 0;
                }
                ConTbl->SystemLine = (LPCONIME_CANDMESSAGE)LocalAlloc(LPTR, SystemLineSize );
                if (ConTbl->SystemLine == NULL) {
                    return FALSE;
                }
                ConTbl->SystemLineSize = SystemLineSize;
            }
            SystemLine = ConTbl->SystemLine;

            SystemLine->AttrOff = sizeof(WCHAR) * dwLength + sizeof(DWORD);

            StartIndex = GetSystemLineP( lpCandList,
                           SystemLine->String,
                           (LPSTR)SystemLine + SystemLine->AttrOff,
                           dwLength,
                           CountDispWidth,
                           ConTbl
                           );

            ConTbl->fNestCandidate = TRUE;    // ImmNotyfyIME call back OpenCandidate Message
                                        // by same data.
                                        // so We ignore this mesage.
            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESTART,
                         dwIndex,
                         ConTbl->CandSep[StartIndex]);

            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESIZE,
                         dwIndex,
                         ConTbl->CandSep[StartIndex+1] -
                         ConTbl->CandSep[StartIndex]);
            ConTbl->fNestCandidate = FALSE;

            CopyData.dwData = CI_CONIMEMODEINFO;
            CopyData.cbData = sizeof(CONIME_UIMODEINFO);
            CopyData.lpData = lpModeInfo;
            if (MakeInfoStringPRC(ConTbl, lpModeInfo) ) {
                ConsoleImeSendMessage( ConTbl->hWndCon,
                                       (WPARAM)hwnd,
                                       (LPARAM)&CopyData
                                     );
            }
        }
    }
    LocalFree( lpModeInfo );
    return TRUE;
}

BOOL
OpenCandidateKorea(
    HWND hwnd,
    HIMC  hIMC ,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList,
    BOOL OpenFlag
    )
{
    DWORD dwLength;
    DWORD dwIndex;
    DWORD i;
    DWORD j;
    LPWSTR lpStr;
    DWORD dwDspLen;
    DWORD width;
    DWORD StartIndex;
    DWORD CountDispWidth;
    DWORD AllocLength;
    LPCANDIDATELIST lpCandList;
    LPCONIME_CANDMESSAGE SystemLine;
    DWORD SystemLineSize;
    COPYDATASTRUCT CopyData;
    BOOL EnableCodePoint;

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            dwLength = ImmGetCandidateList(hIMC, dwIndex, NULL, 0);
            if (dwLength == 0)
                return FALSE;
            if ( (ConTbl->CandListMemAllocSize[dwIndex] != dwLength ) &&
                 (ConTbl->lpCandListMem[dwIndex] != NULL)) {
                LocalFree(ConTbl->lpCandListMem[dwIndex]);
                ConTbl->CandListMemAllocSize[dwIndex] = 0;
                ConTbl->lpCandListMem[dwIndex] = NULL;
            }
            if (ConTbl->lpCandListMem[dwIndex] == NULL) {
                ConTbl->lpCandListMem[dwIndex] = LocalAlloc(LPTR, dwLength);
                if (ConTbl->lpCandListMem[dwIndex] == NULL)
                    return FALSE;
                ConTbl->CandListMemAllocSize[dwIndex] = dwLength;
            }
            lpCandList = ConTbl->lpCandListMem[dwIndex];
            ImmGetCandidateList(hIMC, dwIndex, lpCandList, dwLength);

            //
            // check each offset value is not over than buffer size.
            //
            if ((lpCandList->dwCount > 1) &&
                (lpCandList->dwSelection >= dwLength ||
                 lpCandList->dwPageStart >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwSelection] >= dwLength ||
                 lpCandList->dwOffset[lpCandList->dwPageStart] >= dwLength    )
               )
                break;

            dwLength = ConTbl->ScreenBufferSize.X;
            dwLength = (dwLength > 128) ? 128 : dwLength ;
            dwLength = ((dwLength < 12) ? 12 : dwLength );

            j = (dwLength-7)/(DELIMITERWIDTH+sizeof(WCHAR));
            j = ((j > 9)?9:j);
            j = lpCandList->dwCount / j + 10;
            AllocLength = (j > DEFAULTCANDTABLE) ? j : DEFAULTCANDTABLE;

            if (lpCandList->dwStyle == IME_CAND_CODE){
                EnableCodePoint = TRUE;
            }
            else{
                EnableCodePoint = FALSE;
            }

            if (EnableCodePoint){
                CountDispWidth = CODEDISPLEN;
            }
            else {
                for (CountDispWidth = 0 ,j = 1; j <= lpCandList->dwCount; CountDispWidth++)
                     j *= 10;
                CountDispWidth *= 2;
                CountDispWidth++;
            }

            if ((ConTbl->CandSepAllocSize != sizeof(DWORD)*AllocLength) &&
                (ConTbl->CandSep != NULL)) {
                LocalFree(ConTbl->CandSep);
                ConTbl->CandSep = NULL;
                ConTbl->CandSepAllocSize = 0;
            }
            if (ConTbl->CandSep == NULL) {
                ConTbl->CandSep= LocalAlloc(LPTR, sizeof(DWORD)*AllocLength);
                if (ConTbl->CandSep == NULL)
                    return FALSE;
                ConTbl->CandSepAllocSize = sizeof(DWORD)*AllocLength;
            }

            if ( EnableCodePoint ){
                j = 0;
                ConTbl->CandSep[j++] = 0;
                if (OpenFlag) {
                    ConTbl->CandOff = lpCandList->dwSelection % 9;
                }
                i = ConTbl->CandOff;
                for (; i < lpCandList->dwCount; i+= 9 ) {
                    ConTbl->CandSep[j++] = i;
                }
                if (i > lpCandList->dwCount) {
                    i = lpCandList->dwCount;
                }
            }
            else{
                j = 0;
                if (OpenFlag) {
                    ConTbl->CandOff = 0;
                }
                ConTbl->CandSep[j++] = ConTbl->CandOff;
                lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ 0 ]);
                dwDspLen = DispLenUnicode( lpStr );
                width = dwDspLen + DELIMITERWIDTH;                  // '1:xxxx 2:xxxx '
                for( i = 1; i < lpCandList->dwCount; i++ ) {
                    lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
                    dwDspLen = DispLenUnicode( lpStr );
                    width += dwDspLen + DELIMITERWIDTH;
                    if ((width > dwLength-CountDispWidth) ||
                        ( i - ConTbl->CandSep[j-1] >= 9)){
                        ConTbl->CandSep[j++] = i;
                        width = dwDspLen + DELIMITERWIDTH;
                    }
                }
            }
            ConTbl->CandSep[j] = i;
            ConTbl->CandMax = j;

            SystemLineSize = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            if (ConTbl->SystemLineSize < SystemLineSize ){
                if (ConTbl->SystemLine != NULL){
                    LocalFree( ConTbl->SystemLine );
                    ConTbl->SystemLine = NULL;
                    ConTbl->SystemLineSize = 0;
                }
                ConTbl->SystemLine = (LPCONIME_CANDMESSAGE)LocalAlloc(LPTR, SystemLineSize );
                if (ConTbl->SystemLine == NULL) {
                    return FALSE;
                }
                ConTbl->SystemLineSize = SystemLineSize;
            }
            SystemLine = ConTbl->SystemLine;

            SystemLine->AttrOff = sizeof(WCHAR) * dwLength + sizeof(DWORD);

            CopyData.dwData = CI_CONIMECANDINFO;
            CopyData.cbData = (sizeof(WCHAR)+sizeof(UCHAR))*dwLength + sizeof(DWORD);
            CopyData.lpData = SystemLine;
            StartIndex = GetSystemLineJ( lpCandList,
                           SystemLine->String,
                           (LPSTR)SystemLine + SystemLine->AttrOff,
                           dwLength,
                           CountDispWidth,
                           ConTbl,
                           EnableCodePoint);

            ConTbl->fNestCandidate = TRUE;    // ImmNotyfyIME call back OpenCandidate Message
                                        // by same data.
                                        // so We ignore this mesage.
            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESTART,
                         dwIndex,
                         ConTbl->CandSep[StartIndex]);

            ImmNotifyIME(hIMC,
                         NI_SETCANDIDATE_PAGESIZE,
                         dwIndex,
                         ConTbl->CandSep[StartIndex+1] -
                         ConTbl->CandSep[StartIndex]);
            ConTbl->fNestCandidate = FALSE;

            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                 );
        }
    }
    return TRUE;
}

DWORD
DispLenUnicode(
    LPWSTR lpString )
{
    DWORD i;
    DWORD Length;

    Length = 0;

    for ( i = 0; lpString[i] != 0; i++) {
        Length += IsUnicodeFullWidth(lpString[i]) ? 2 : 1;
    }
    return Length;
}

DWORD
GetSystemLineJ(
    LPCANDIDATELIST lpCandList ,
    LPWSTR String,
    LPSTR Attr,
    DWORD dwLength,
    DWORD CountDispWidth,
    PCONSOLE_TABLE FocusedConsole,
    BOOL EnableCodePoint)
{
    DWORD dwStrLen;
    DWORD dwDspLen;
    DWORD i;
    DWORD j;
    DWORD SepIndex;
    DWORD SelCount;
    DWORD Length;
    DWORD dwWholeLen;
    BOOL lfBreak = FALSE;
    LPWSTR StrToWrite;
    LPSTR AttrToSel;
    LPWSTR lpStr;
    USHORT MultiChar;
    USHORT TempMulti;

    if ((lpCandList->dwSelection > lpCandList->dwCount)||
        (lpCandList->dwSelection < 0)) {
        lpCandList->dwSelection  = 0;
    }

    for ( SepIndex = FocusedConsole->CandMax; SepIndex > 0; SepIndex--) {
        if (lpCandList->dwSelection >= FocusedConsole->CandSep[SepIndex])
            break;
    }
    if (SepIndex == FocusedConsole->CandMax)
        SepIndex = 0;

    for ( i = 0; i < dwLength; i++) {
        Attr[i] = 0x0000;
    }
    StrToWrite = String;
    AttrToSel = Attr;
    dwWholeLen = 0;
#if 1
    // HACK HACK ntbug #69699
    // MS-IME97 & MS-IME97A does not return correct value for IME_PROP_CANDLIST_START_FROM_1.
    // These always return its starting from 0.
    // Currently there is not IME starting from 0. So we hack.
    // Actually IME should be fixed.
    SelCount = 1;
#else
    if (FocusedConsole->ImmGetProperty & IME_PROP_CANDLIST_START_FROM_1)
        SelCount = 1;
    else
        SelCount = 0;
#endif

    if (EnableCodePoint){
        lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[lpCandList->dwSelection]);
        WideCharToMultiByte(CP_OEMCP, 0, lpStr, 1, (PBYTE)&TempMulti, 2, NULL, NULL);
        *StrToWrite = UNICODE_LEFT;
        StrToWrite++;
        MultiChar = (USHORT)(HIBYTE(TempMulti)| LOBYTE(TempMulti)<<8);
        for (i = 0; i < 4; i++) {
            j = (MultiChar & 0xf000 ) >> 12;
            if ( j <= 9)
                *StrToWrite = (USHORT)(j + UNICODE_ZERO);
            else
                *StrToWrite = (USHORT)(j + UNICODE_HEXBASE);
            StrToWrite++;
            MultiChar = (USHORT)(MultiChar << 4);
        }
        *StrToWrite = UNICODE_RIGHT;
        StrToWrite++;
        *StrToWrite = UNICODE_SPACE;
        StrToWrite++;
        AttrToSel += CountDispWidth;
        dwWholeLen += CountDispWidth;
        CountDispWidth = 0;
    }

    for (i = FocusedConsole->CandSep[SepIndex]; i < FocusedConsole->CandSep[SepIndex+1]; i++) {
        //
        // check each offset value is not over than buffer size.
        //
        if (lpCandList->dwOffset[i] >= lpCandList->dwSize)
            break;

        lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
        dwStrLen = lstrlenW( lpStr );
        dwDspLen = DispLenUnicode( lpStr );

        if ( dwWholeLen + dwDspLen + DELIMITERWIDTH  > dwLength - CountDispWidth ){
            Length = 0;
            lfBreak = TRUE;
            for (j = 0; j < dwStrLen; j++ ){
                Length += IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                if (dwWholeLen + Length > dwLength - (CountDispWidth + DELIMITERWIDTH)){
                    dwStrLen = j-1;
                    dwDspLen = Length - IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                    break;
                }
            }
        }
        if ((dwWholeLen + dwDspLen + DELIMITERWIDTH + CountDispWidth ) <= dwLength )      // if minus value
            dwWholeLen += (dwDspLen + DELIMITERWIDTH);
        else {
            break;
        }

        if (i == lpCandList->dwSelection) {
            for (j = 0; j < dwStrLen+2; j++)
                *(AttrToSel+j) = 1;
        }
        *StrToWrite = (USHORT)(SelCount + UNICODE_ZERO);
        StrToWrite++;
        *StrToWrite = UNICODE_COLON;
        StrToWrite++;
        CopyMemory(StrToWrite, lpStr, dwStrLen * sizeof(WCHAR));
        StrToWrite += dwStrLen;
        *StrToWrite = UNICODE_SPACE;
        StrToWrite++;
        AttrToSel += dwStrLen+DELIMITERWIDTH;
        SelCount++;
        if (lfBreak)
            break;
    }
    *StrToWrite = 0;
    dwDspLen = DispLenUnicode( String );
    if (dwDspLen > (dwLength - CountDispWidth))
        return SepIndex;

    if (EnableCodePoint){
        for (i = dwDspLen; i < dwLength; i++) {
            *StrToWrite = UNICODE_SPACE;
            StrToWrite++;
        }
    }
    else {
        for (i = dwDspLen; i < (dwLength - CountDispWidth); i++) {
            *StrToWrite = UNICODE_SPACE;
            StrToWrite++;
        }

        i = (CountDispWidth-1) / 2;
        NumString(StrToWrite,lpCandList->dwSelection+1,i);
        StrToWrite+=i;
        *StrToWrite = UNICODE_SLASH;
        StrToWrite++;
        NumString(StrToWrite,lpCandList->dwCount, i);
        StrToWrite+=i;
    }
    *StrToWrite = 0;

    return SepIndex;
}

DWORD
GetSystemLineT(
    LPCANDIDATELIST lpCandList ,
    LPWSTR String,
    LPSTR Attr,
    DWORD dwLength,
    DWORD CountDispWidth,
    PCONSOLE_TABLE FocusedConsole
    )
{
    DWORD dwStrLen;
    DWORD dwDspLen;
    DWORD i;
    DWORD j;
    DWORD SepIndex;
    DWORD SelCount;
    DWORD Length;
    DWORD dwWholeLen;
    BOOL lfBreak = FALSE;
    LPWSTR StrToWrite;
    LPSTR AttrToSel;
    LPWSTR lpStr;
    USHORT MultiChar;
    USHORT TempMulti;

    if ((lpCandList->dwSelection > lpCandList->dwCount)||
        (lpCandList->dwSelection < 0)) {
        lpCandList->dwSelection  = 0;
    }

    for ( SepIndex = FocusedConsole->CandMax; SepIndex > 0; SepIndex--) {
        if (lpCandList->dwSelection >= FocusedConsole->CandSep[SepIndex])
            break;
    }
    if (SepIndex == FocusedConsole->CandMax)
        SepIndex = 0;

    for ( i = 0; i < dwLength; i++) {
        Attr[i] = 0x0000;
    }
    StrToWrite = String;
    AttrToSel = Attr;
    dwWholeLen = 0;
    if (FocusedConsole->ImmGetProperty & IME_PROP_CANDLIST_START_FROM_1)
        SelCount = 1;
    else
        SelCount = 0;


    for (i = FocusedConsole->CandSep[SepIndex]; i < FocusedConsole->CandSep[SepIndex+1]; i++) {
        lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
        dwStrLen = lstrlenW( lpStr );
        dwDspLen = DispLenUnicode( lpStr );

        if ( dwWholeLen + dwDspLen + DELIMITERWIDTH  > dwLength - CountDispWidth ){
            Length = 0;
            lfBreak = TRUE;
            for (j = 0; j < dwStrLen; j++ ){
                Length += IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                if (dwWholeLen + Length > dwLength - (CountDispWidth + DELIMITERWIDTH)){
                    dwStrLen = j-1;
                    dwDspLen = Length - IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                    break;
                }
            }
        }
        if ((dwWholeLen + dwDspLen + DELIMITERWIDTH + CountDispWidth ) <= dwLength )      // if minus value
            dwWholeLen += (dwDspLen + DELIMITERWIDTH);
        else {
            break;
        }

        if (i == lpCandList->dwSelection) {
            for (j = 0; j < dwStrLen+2; j++)
                *(AttrToSel+j) = 1;
        }
        *StrToWrite = (USHORT)(SelCount + UNICODE_ZERO);
        StrToWrite++;
        *StrToWrite = UNICODE_COLON;
        StrToWrite++;
        CopyMemory(StrToWrite, lpStr, dwStrLen * sizeof(WCHAR));
        StrToWrite += dwStrLen;
        *StrToWrite = UNICODE_SPACE;
        StrToWrite++;
        AttrToSel += dwStrLen+DELIMITERWIDTH;
        SelCount++;
        if (lfBreak)
            break;
    }
    *StrToWrite = 0;
    dwDspLen = DispLenUnicode( String );
    if (dwDspLen > (dwLength - CountDispWidth))
        return SepIndex;

    *StrToWrite = UNICODE_SPACE;
    StrToWrite++;

    i = (CountDispWidth-1) / 2;
    NumString(StrToWrite,lpCandList->dwSelection+1,i);
    StrToWrite+=i;
    *StrToWrite = UNICODE_SLASH;
    StrToWrite++;
    NumString(StrToWrite,lpCandList->dwCount, i);
    StrToWrite+=i;
    *StrToWrite = 0;

    return SepIndex;
}

DWORD
GetSystemLineP(
    LPCANDIDATELIST lpCandList ,
    LPWSTR String,
    LPSTR Attr,
    DWORD dwLength,
    DWORD CountDispWidth,
    PCONSOLE_TABLE FocusedConsole
    )
{
    DWORD dwStrLen;
    DWORD dwDspLen;
    DWORD i;
    DWORD j;
    DWORD SepIndex;
    DWORD SelCount;
    DWORD Length;
    DWORD dwWholeLen;
    BOOL lfBreak = FALSE;
    LPWSTR StrToWrite;
    LPSTR AttrToSel;
    LPWSTR lpStr;
    USHORT MultiChar;
    USHORT TempMulti;

    if ((lpCandList->dwSelection > lpCandList->dwCount)||
        (lpCandList->dwSelection < 0)) {
        lpCandList->dwSelection  = 0;
    }

    for ( SepIndex = FocusedConsole->CandMax; SepIndex > 0; SepIndex--) {
        if (lpCandList->dwSelection >= FocusedConsole->CandSep[SepIndex])
            break;
    }
    if (SepIndex == FocusedConsole->CandMax)
        SepIndex = 0;

    for ( i = 0; i < dwLength; i++) {
        Attr[i] = 0x0000;
    }
    StrToWrite = String;
    AttrToSel = Attr;
    dwWholeLen = 0;
    if (FocusedConsole->ImmGetProperty & IME_PROP_CANDLIST_START_FROM_1)
        SelCount = 1;
    else
        SelCount = 0;

    for (i = FocusedConsole->CandSep[SepIndex]; i < FocusedConsole->CandSep[SepIndex+1]; i++) {
        lpStr = (LPWSTR)((LPSTR)lpCandList + lpCandList->dwOffset[ i ]);
        dwStrLen = lstrlenW( lpStr );
        dwDspLen = DispLenUnicode( lpStr );

        if ( dwWholeLen + dwDspLen + DELIMITERWIDTH  > dwLength - CountDispWidth ){
            Length = 0;
            lfBreak = TRUE;
            for (j = 0; j < dwStrLen; j++ ){
                Length += IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                if (dwWholeLen + Length > dwLength - (CountDispWidth + DELIMITERWIDTH)){
                    dwStrLen = j-1;
                    dwDspLen = Length - IsUnicodeFullWidth(lpStr[j]) ? 2 : 1;
                    break;
                }
            }
        }
        if ((dwWholeLen + dwDspLen + DELIMITERWIDTH + CountDispWidth ) <= dwLength )      // if minus value
            dwWholeLen += (dwDspLen + DELIMITERWIDTH);
        else {
            break;
        }

        if (i == lpCandList->dwSelection) {
            for (j = 0; j < dwStrLen+2; j++)
                *(AttrToSel+j) = 1;
        }
        *StrToWrite = (USHORT)(SelCount + UNICODE_ZERO);
        StrToWrite++;
        *StrToWrite = UNICODE_COLON;
        StrToWrite++;
        CopyMemory(StrToWrite, lpStr, dwStrLen * sizeof(WCHAR));
        StrToWrite += dwStrLen;
        *StrToWrite = UNICODE_SPACE;
        StrToWrite++;
        AttrToSel += dwStrLen+DELIMITERWIDTH;
        SelCount++;
        if (lfBreak)
            break;
    }
    *StrToWrite = 0;
    dwDspLen = DispLenUnicode( String );
    if (dwDspLen > (dwLength - CountDispWidth))
        return SepIndex;

#if defined (CANDCOUNTPRC) //for wider candidate list space v-hirshi Oct.16.1996
    *StrToWrite = UNICODE_SPACE;
    StrToWrite++;

    i = (CountDispWidth-1) / 2;
    NumString(StrToWrite,lpCandList->dwSelection+1,i);
    StrToWrite+=i;
    *StrToWrite = UNICODE_SLASH;
    StrToWrite++;
    NumString(StrToWrite,lpCandList->dwCount, i);
    StrToWrite+=i;
#endif

    *StrToWrite = 0;

    return SepIndex;
}

VOID
NumString(
    LPWSTR StrToWrite,
    DWORD NumToDisp,
    DWORD CountDispWidth)
{
    DWORD i;
    DWORD k;
    k = 1;
    for (i = 1; i < CountDispWidth; i++)
        k *= 10;
    for (i = k; i > 0; i /= 10){
        k = (NumToDisp / i);
        *StrToWrite = (USHORT)(k + UNICODE_ZERO);
        if ((*StrToWrite == UNICODE_ZERO) &&
            ((*(StrToWrite-1) == UNICODE_SPACE)||(*(StrToWrite-1) == UNICODE_SLASH)) )
            *StrToWrite = UNICODE_SPACE;
        StrToWrite++;
        NumToDisp -= (i*k);
    }
}

BOOL
ImeUICloseCandidate(
   HWND hwnd,
   DWORD CandList
   )
{
    HIMC  hIMC;
    PCONSOLE_TABLE ConTbl;

    DBGPRINT(("CONIME: Get IMN_CLOSECANDIDATE Message\n"));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
        return FALSE;

    //
    // Reset fInCandidate variables.
    //
    ConTbl->fInCandidate = FALSE;

    switch ( HKL_TO_LANGID(ConTbl->hklActive))
    {
        case    LANG_ID_JAPAN:
            CloseCandidateJapan(hwnd, hIMC, ConTbl, CandList );
            break;
        case    LANG_ID_TAIWAN:
            CloseCandidateTaiwan(hwnd, hIMC, ConTbl, CandList );
            break;
        case    LANG_ID_PRC:
            CloseCandidatePRC(hwnd, hIMC, ConTbl, CandList );
            break;
        case    LANG_ID_KOREA:
            CloseCandidateKorea(hwnd, hIMC, ConTbl, CandList );
            break;
        default:
            return FALSE;
            break;
    }
    ImmReleaseContext( hwnd, hIMC );

    return TRUE;
}

BOOL
CloseCandidateJapan(
    HWND hwnd,
    HIMC hIMC,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList
   )
{
    DWORD dwIndex;
    COPYDATASTRUCT CopyData;

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            if (ConTbl->lpCandListMem[dwIndex] != NULL ) {
                 LocalFree(ConTbl->lpCandListMem[dwIndex]);
                 ConTbl->lpCandListMem[dwIndex] = NULL;
                 ConTbl->CandListMemAllocSize[dwIndex] = 0;
            }
        }
    }

    CopyData.dwData = CI_CONIMECANDINFO;
    CopyData.cbData = 0;
    CopyData.lpData = NULL;
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );

    return TRUE;
}

BOOL
CloseCandidateTaiwan(
    HWND hwnd,
    HIMC hIMC,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList
   )
{
    DWORD dwIndex;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;
    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            if (ConTbl->lpCandListMem[dwIndex] != NULL ) {
                 LocalFree(ConTbl->lpCandListMem[dwIndex]);
                 ConTbl->lpCandListMem[dwIndex] = NULL;
                 ConTbl->CandListMemAllocSize[dwIndex] = 0;
            }
        }
    }

    CopyData.dwData = CI_CONIMEMODEINFO;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO);
    CopyData.lpData = lpModeInfo;
    if (MakeInfoStringTaiwan(ConTbl, lpModeInfo) ) {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                             );
    }

    LocalFree( lpModeInfo );
    return TRUE;

}

BOOL
CloseCandidatePRC(
    HWND hwnd,
    HIMC hIMC,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList
   )
{
    DWORD dwIndex;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;
    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            if (ConTbl->lpCandListMem[dwIndex] != NULL ) {
                 LocalFree(ConTbl->lpCandListMem[dwIndex]);
                 ConTbl->lpCandListMem[dwIndex] = NULL;
                 ConTbl->CandListMemAllocSize[dwIndex] = 0;
            }
        }
    }

    CopyData.dwData = CI_CONIMEMODEINFO;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO);
    CopyData.lpData = lpModeInfo;
    if (MakeInfoStringPRC(ConTbl, lpModeInfo) ) {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                             );
    }

    LocalFree( lpModeInfo );
    return TRUE;

}

BOOL
CloseCandidateKorea(
    HWND hwnd,
    HIMC hIMC,
    PCONSOLE_TABLE ConTbl,
    DWORD CandList
   )
{
    DWORD dwIndex;
    COPYDATASTRUCT CopyData;

    for (dwIndex = 0; dwIndex < MAX_LISTCAND ; dwIndex ++ ) {
        if ( CandList & ( 1 << dwIndex ) ) {
            if (ConTbl->lpCandListMem[dwIndex] != NULL ) {
                 LocalFree(ConTbl->lpCandListMem[dwIndex]);
                 ConTbl->lpCandListMem[dwIndex] = NULL;
                 ConTbl->CandListMemAllocSize[dwIndex] = 0;
            }
        }
    }

    CopyData.dwData = CI_CONIMECANDINFO;
    CopyData.cbData = 0;
    CopyData.lpData = NULL;
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );

    return TRUE;

}
