/***************************************************************************
 *  FILERES.C
 *
 *              File resource extraction routines.
 *
 ***************************************************************************/
//
//  BUGBUG - GetVerInfoSize plays tricks and tells the caller to allocate
//  some extra slop at the end of the buffer in case we need to thunk all
//  the strings to ANSI.  The bug is that it only tells it to allocate
//  one extra ANSI char (== BYTE) for each Unicode char.  This is not correct
//  in the DBCS case (since one Unicode char can equal a two byte DBCS char)
//
//  We should change GetVerInfoSize return the Unicode size * 2 (instead
//  of (Unicode size * 1.5) and then change VerQueryInfoA to also use the
//  * 2 computation instead of * 1.5 (== x + x/2)
//
//  23-May-1996 JonPa
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "verpriv.h"
#include <memory.h>

#define DWORDUP(x) (((x)+3)&~03)

typedef struct tagVERBLOCK {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;
    WCHAR szKey[1];
} VERBLOCK ;

typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;


typedef struct tagVERBLOCK16 {
    WORD wTotLen;
    WORD wValLen;
    CHAR szKey[1];
} VERBLOCK16 ;

typedef struct tagVERHEAD16 {
    WORD wTotLen;
    WORD wValLen;
    CHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;      // same as win31
} VERHEAD16 ;

DWORD VER2_SIG='X2EF';


extern WCHAR szTrans[];

/* ----- Functions ----- */
DWORD
MyExtractVersionResource16W (
    LPCWSTR  lpwstrFilename,
    LPHANDLE hVerRes
    )
{
    DWORD dwTemp = 0;
    DWORD (__stdcall *pExtractVersionResource16W)(LPCWSTR, LPHANDLE);
    HINSTANCE hShell32 = LoadLibraryW(L"shell32.dll");

    if (hShell32) {
        pExtractVersionResource16W = (DWORD(__stdcall *)(LPCWSTR, LPHANDLE))
                                     GetProcAddress(hShell32, "ExtractVersionResource16W");
        if (pExtractVersionResource16W) {
            dwTemp = pExtractVersionResource16W( lpwstrFilename, hVerRes );
        } else {
            dwTemp = 0;
        }
        FreeLibrary(hShell32);
    }
    return dwTemp;
}


/* GetFileVersionInfoSize
 * Gets the size of the version information; notice this is quick
 * and dirty, and the handle is just the offset
 *
 * Returns size of version info in bytes
 * lpwstrFilename is the name of the file to get version information from
 * lpdwHandle is outdated for the Win32 api and is set to zero.
 */
DWORD
APIENTRY
GetFileVersionInfoSizeW(
                       LPWSTR lpwstrFilename,
                       LPDWORD lpdwHandle
                       )
{
    DWORD dwTemp;
    VERHEAD *pVerHead;
    HANDLE hMod;
    HANDLE hVerRes;
    HANDLE h;
    DWORD dwError;

    if (lpdwHandle != NULL)
        *lpdwHandle = 0;

    dwTemp = SetErrorMode(SEM_FAILCRITICALERRORS);
    hMod = LoadLibraryEx(lpwstrFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);
    SetErrorMode(dwTemp);

    pVerHead = NULL;
    if (!hMod) {
        hVerRes = NULL;
        __try
        {
            dwTemp = MyExtractVersionResource16W( lpwstrFilename, &hVerRes );

            dwError = ERROR_SUCCESS;
            if (!dwTemp) {
                __leave;
            }

            if (!(pVerHead = GlobalLock(hVerRes)) || (pVerHead->wTotLen > dwTemp)) {
                dwError = ERROR_INVALID_DATA;
                dwTemp = 0;
                __leave;
            }
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            dwError = ERROR_INVALID_DATA;
            dwTemp = 0 ;
        }

        if (pVerHead)
            GlobalUnlock(hVerRes);

        if (hVerRes)
            GlobalFree(hVerRes);

        SetLastError(dwError);

        return dwTemp ? dwTemp * 3 : 0;     // 3x == 1x for ansi input, 2x for unicode convert space
    }

    __try {
        dwError = ERROR_SUCCESS;
        if ((hVerRes = FindResource(hMod, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO)) == NULL) {
            dwTemp = 0;
            __leave;
        }

        if ((dwTemp=SizeofResource(hMod, hVerRes)) == 0) {
            dwTemp = 0;
            __leave;
        }

        if ((h = LoadResource(hMod, hVerRes)) == NULL) {
            dwTemp = 0;
            __leave;
        }

        if ((pVerHead = (VERHEAD*)LockResource(h)) == NULL) {
            dwTemp = 0;
            __leave;
        }

        if ((DWORD)pVerHead->wTotLen > dwTemp) {
            dwError = ERROR_INVALID_DATA;
            dwTemp = 0;
            __leave;
        }

        dwTemp = (DWORD)pVerHead->wTotLen;

        dwTemp = DWORDUP(dwTemp);

        if (pVerHead->vsf.dwSignature != VS_FFI_SIGNATURE) {
            dwError = ERROR_INVALID_DATA;
            dwTemp = 0;
            __leave;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
        dwTemp = 0;
    }

    if (pVerHead)
        UnlockResource(h);

    FreeLibrary(hMod);

    SetLastError(dwError);

    //
    // dwTemp should be evenly divisible by two since not single
    // byte components at all (also DWORDUP for safety above):
    // alloc space for ansi components
    //

    //
    // Keep space for DBCS chars.
    //
    return dwTemp ? (dwTemp * 2) + sizeof(VER2_SIG) : 0;
}


/* GetFileVersionInfo
 * Gets the version information; fills in the structure up to
 * the size specified by the dwLen parameter (since Control Panel
 * only cares about the version numbers, it won't even call
 * GetFileVersionInfoSize).  Notice this is quick and dirty
 * version, and dwHandle is just the offset (or NULL).
 *
 * lpwstrFilename is the name of the file to get version information from.
 * dwHandle is the handle filled in from the GetFileVersionInfoSize call.
 * dwLen is the length of the buffer to fill.
 * lpData is the buffer to fill.
 */
BOOL
APIENTRY
GetFileVersionInfoW(
                   LPWSTR lpwstrFilename,
                   DWORD dwHandle,
                   DWORD dwLen,
                   LPVOID lpData
                   )
{
    VERHEAD *pVerHead;
    VERHEAD16 *pVerHead16;
    HANDLE hMod;
    HANDLE hVerRes;
    HANDLE h;
    UINT   dwTemp;
    BOOL bTruncate, rc;

    UNREFERENCED_PARAMETER(dwHandle);

    // Check minimum size to prevent access violations

    if (dwLen < sizeof(((VERHEAD*)lpData)->wTotLen)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (FALSE);
    }

    dwTemp = SetErrorMode(SEM_FAILCRITICALERRORS);
    hMod = LoadLibraryEx(lpwstrFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);
    SetErrorMode(dwTemp);

    if (hMod == NULL) {

        // Allow 16bit stuff

        __try {
            dwTemp = MyExtractVersionResource16W( lpwstrFilename, &hVerRes );
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            dwTemp = 0 ;
        }

        if (!dwTemp)
            return (FALSE);

        if (!(pVerHead16 = GlobalLock(hVerRes))) {

            SetLastError(ERROR_INVALID_DATA);
            GlobalFree(hVerRes);
            return (FALSE);
        }

        __try {
            dwTemp = (DWORD)pVerHead16->wTotLen;

            if ((dwTemp * 3) > dwLen) {

                //
                // We are forced to truncate.
                //
                dwTemp = dwLen/3;

                bTruncate = TRUE;

            } else {

                bTruncate = FALSE;
            }

            // Now mem copy only the real size of the resource.  (We alloced
            // extra space for unicode)

            memcpy((PVOID)lpData, (PVOID)pVerHead16, dwTemp);
            if (bTruncate) {

                // If we truncated above, then we must set the new
                // size of the block so that we don't overtraverse.

                ((VERHEAD16*)lpData)->wTotLen = (WORD)dwTemp;
            }
            rc = TRUE;
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            rc = FALSE;
        }

        GlobalUnlock(hVerRes);
        GlobalFree(hVerRes);

        return rc;
    }

    if (((hVerRes = FindResource(hMod, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO)) == NULL) ||
        ((pVerHead = LoadResource(hMod, hVerRes)) == NULL)) {
        rc = FALSE;
    } else {
        __try {
            dwTemp = (DWORD)pVerHead->wTotLen;

            if (((dwTemp * 2) + sizeof(VER2_SIG)) > dwLen) {

                // We are forced to truncate.

                //
                // dwLen = UnicodeBuffer + AnsiBuffer.
                //
                // if we try to "memcpy" with "(dwLen/3) * 2" size, pVerHead
                // might not have such a big data...
                //
                dwTemp = (dwLen / 2) - sizeof(VER2_SIG);

                bTruncate = TRUE;
            } else {
                bTruncate = FALSE;
            }

            // Now mem copy only the real size of the resource.  (We alloced
            // extra space for ansi)

            memcpy((PVOID)lpData, (PVOID)pVerHead, dwTemp);

            // Store a sig between the raw data and the ANSI translation area so we know
            // how much space we have available in VerQuery for ANSI translation.
            *((PDWORD)((ULONG_PTR)lpData + dwTemp)) = VER2_SIG;
            if (bTruncate) {
                // If we truncated above, then we must set the new
                // size of the block so that we don't overtraverse.

                ((VERHEAD*)lpData)->wTotLen = (WORD)dwTemp;
            }

            rc = TRUE;
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            rc = FALSE;
        }
    }

    FreeLibrary(hMod);

    return (rc);
}


BOOL
VerpQueryValue16(
                const LPVOID pb,
                LPVOID lpSubBlockX,
                INT    nIndex,
                LPVOID *lplpKey,
                LPVOID *lplpBuffer,
                PUINT puLen,
                BOOL    bUnicodeNeeded
                )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    LPSTR lpSubBlock;
    LPSTR lpSubBlockOrg;
    NTSTATUS Status;

    VERBLOCK16 *pBlock = (VERBLOCK16*)pb;
    LPSTR lpStart, lpEndBlock, lpEndSubBlock;
    CHAR cTemp, cEndBlock;
    BOOL bLastSpec;
    DWORD dwHeadLen, dwTotBlockLen;
    INT  nCmp;

    BOOL bThunkNeeded;

    /*
     * If needs unicode, then we must thunk the input parameter
     * to ansi.  If it's ansi already, we make a copy so we can
     * modify it.
     */

    if (bUnicodeNeeded) {

        //
        // Thunk is not needed if lpSubBlockX == \VarFileInfo\Translation
        // or if lpSubBlockX == \
        //
        bThunkNeeded = (BOOL)((*(LPTSTR)lpSubBlockX != 0) &&
                              (lstrcmp(lpSubBlockX, TEXT("\\")) != 0) &&
                              (lstrcmpi(lpSubBlockX, szTrans) != 0));

        RtlInitUnicodeString(&UnicodeString, lpSubBlockX);
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

        if (!NT_SUCCESS(Status)) {
            SetLastError(Status);
            return FALSE;
        }
        lpSubBlock = AnsiString.Buffer;

    } else {
        lpSubBlockOrg = (LPSTR)LocalAlloc(LPTR,(lstrlenA(lpSubBlockX)+1)*sizeof(CHAR));
        if (lpSubBlockOrg == NULL ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        lstrcpyA(lpSubBlockOrg,lpSubBlockX);
        lpSubBlock = lpSubBlockOrg;
    }

    *puLen = 0;

    /* Ensure that the total length is less than 32K but greater than the
     * size of a block header; we will assume that the size of pBlock is at
     * least the value of this first INT.
     */
    if ((INT)pBlock->wTotLen < sizeof(VERBLOCK16))
        goto Fail;

    /*
     * Put a '\0' at the end of the block so that none of the lstrlen's will
     * go past then end of the block.  We will replace it before returning.
     */
    lpEndBlock = ((LPSTR)pBlock) + pBlock->wTotLen - 1;
    cEndBlock = *lpEndBlock;
    *lpEndBlock = '\0';

    bLastSpec = FALSE;

    while ((*lpSubBlock || nIndex != -1)) {
        //
        // Ignore leading '\\'s
        //
        while (*lpSubBlock == '\\')
            ++lpSubBlock;

        if ((*lpSubBlock || nIndex != -1)) {
            /* Make sure we still have some of the block left to play with
             */
            dwTotBlockLen = (DWORD)(lpEndBlock - ((LPSTR)pBlock) + 1);
            if ((INT)dwTotBlockLen<sizeof(VERBLOCK16) ||
                pBlock->wTotLen>dwTotBlockLen)

                goto NotFound;

            /* Calculate the length of the "header" (the two length WORDs plus
             * the identifying string) and skip past the value
             */

            dwHeadLen = sizeof(WORD)*2 + DWORDUP(lstrlenA(pBlock->szKey)+1)
                        + DWORDUP(pBlock->wValLen);

            if (dwHeadLen > pBlock->wTotLen)
                goto NotFound;
            lpEndSubBlock = ((LPSTR)pBlock) + pBlock->wTotLen;
            pBlock = (VERBLOCK16 FAR *)((LPSTR)pBlock+dwHeadLen);

            /* Look for the first sub-block name and terminate it
             */
            for (lpStart=lpSubBlock; *lpSubBlock && *lpSubBlock!='\\';
                lpSubBlock=CharNextA(lpSubBlock))
                /* find next '\\' */ ;
            cTemp = *lpSubBlock;
            *lpSubBlock = '\0';

            /* Continue while there are sub-blocks left
             * pBlock->wTotLen should always be a valid pointer here because
             * we have validated dwHeadLen above, and we validated the previous
             * value of pBlock->wTotLen before using it
             */

            nCmp = 1;
            while ((INT)pBlock->wTotLen>sizeof(VERBLOCK16) &&
                   (INT)(lpEndSubBlock-((LPSTR)pBlock))>=(INT)pBlock->wTotLen) {

                //
                // Index functionality: if we are at the end of the path
                // (cTemp == 0 set below) and nIndex is NOT -1 (index search)
                // then break on nIndex zero.  Else do normal wscicmp.
                //
                if (bLastSpec && nIndex != -1) {

                    if (!nIndex) {

                        if (lplpKey) {
                            *lplpKey = pBlock->szKey;
                        }
                        nCmp=0;

                        //
                        // Index found, set nInde to -1
                        // so that we exit this loop
                        //
                        nIndex = -1;
                        break;
                    }

                    nIndex--;

                } else {

                    //
                    // Check if the sub-block name is what we are looking for
                    //

                    if (!(nCmp=lstrcmpiA(lpStart, pBlock->szKey)))
                        break;
                }

                /* Skip to the next sub-block
                 */
                pBlock=(VERBLOCK16 FAR *)((LPSTR)pBlock+DWORDUP(pBlock->wTotLen));
            }

            /* Restore the char NULLed above and return failure if the sub-block
             * was not found
             */
            *lpSubBlock = cTemp;
            if (nCmp)
                goto NotFound;
        }
        bLastSpec = !cTemp;
    }

    /* Fill in the appropriate buffers and return success
     */
    *puLen = pBlock->wValLen;

    *lplpBuffer = (LPSTR)pBlock + 4 + DWORDUP(lstrlenA(pBlock->szKey) + 1);

    //
    // Shouldn't need zero-length value check since win31 compatible.
    //

    *lpEndBlock = cEndBlock;

    /*
     * Must free string we allocated above
     */

    if (bUnicodeNeeded) {
        RtlFreeAnsiString(&AnsiString);
    } else {
        LocalFree(lpSubBlockOrg);
    }


    /*----------------------------------------------------------------------
     * thunk the results
     *
     * Must always thunk key, always ??? value
     *
     * We have no way of knowing if the resource info is binary or strings
     * Version stuff is usually string info, so thunk.
     *
     * The best we can do is assume that everything is a string UNLESS
     * we are looking at \VarFileInfo\Translation or at \.
     *
     * This is acceptable because the documenation of VerQueryValue
     * indicates that this is used only for strings (except these cases.)
     *----------------------------------------------------------------------*/

    if (bUnicodeNeeded) {

        //
        // Do thunk only if we aren't looking for \VarFileInfo\Translation or \
        //
        if (bThunkNeeded) {

            AnsiString.Length = AnsiString.MaximumLength = (SHORT)*puLen;
            AnsiString.Buffer = *lplpBuffer;

            //
            // Do the string conversion in the second half of the buffer
            // Assumes wTotLen is first filed in VERHEAD
            //
            UnicodeString.Buffer = (LPWSTR)((PBYTE)pb + DWORDUP(*((WORD*)pb)) +
                                            (DWORD)((PBYTE)*lplpBuffer - (PBYTE)pb)*2);

            UnicodeString.MaximumLength = (SHORT)((*puLen+1) * sizeof(WCHAR));
            RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);

            *lplpBuffer = UnicodeString.Buffer;
        }

        if (lplpKey) {

            //
            // Thunk the key
            //

            dwHeadLen = lstrlenA(*lplpKey);
            AnsiString.Length = AnsiString.MaximumLength = (SHORT)dwHeadLen;
            AnsiString.Buffer = *lplpKey;

            UnicodeString.Buffer = (LPWSTR) ((PBYTE)pb + DWORDUP(*((WORD*)pb)) +
                                             (DWORD)((PBYTE)*lplpKey - (PBYTE)pb)*2);

            UnicodeString.MaximumLength = (SHORT)((dwHeadLen+1) * sizeof(WCHAR));
            RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);

            *lplpKey = UnicodeString.Buffer;
        }
    }

    return (TRUE);



    NotFound:

    /* Restore the char we NULLed above
     */
    *lpEndBlock = cEndBlock;

    Fail:

    if (bUnicodeNeeded) {
        RtlFreeAnsiString(&AnsiString);
    } else {
        LocalFree(lpSubBlockOrg);
    }

    return (FALSE);
}



/* VerpQueryValue
 * Given a pointer to a branch of a version info tree and the name of a
 * sub-branch (as in "sub\subsub\subsubsub\..."), this fills in a pointer
 * to the specified value and a word for its length.  Returns TRUE on success,
 * FALSE on failure.
 *
 * Note that a subblock name may start with a '\\', but it will be ignored.
 * To get the value of the current block, use lpSubBlock=""
 */
BOOL
APIENTRY
VerpQueryValue(
              const LPVOID pb,
              LPVOID lpSubBlockX,    // can be ansi or unicode
              INT    nIndex,
              LPVOID *lplpKey,
              LPVOID *lplpBuffer,
              PUINT puLen,
              BOOL    bUnicodeNeeded
              )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    LPWSTR lpSubBlockOrg;
    LPWSTR lpSubBlock;
    NTSTATUS Status;

    VERBLOCK *pBlock = (PVOID)pb;
    LPWSTR lpStart, lpEndBlock, lpEndSubBlock;
    WCHAR cTemp, cEndBlock;
    DWORD dwHeadLen, dwTotBlockLen;
    BOOL bLastSpec;
    INT nCmp;
    BOOL bString;

    *puLen = 0;

    /*
     * Major hack: wType is 0 for win32 versions, but holds 56 ('V')
     * for win16.
     */

    if (((VERHEAD*)pb)->wType)
        return VerpQueryValue16(pb,
                                lpSubBlockX,
                                nIndex,
                                lplpKey,
                                lplpBuffer,
                                puLen,
                                bUnicodeNeeded);

    /*
     * If doesnt need unicode, then we must thunk the input parameter
     * to unicode.
     */

    if (!bUnicodeNeeded) {

        RtlInitAnsiString(&AnsiString, (LPSTR)lpSubBlockX);
        Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

        if (!NT_SUCCESS(Status)) {
            SetLastError(Status);
            return FALSE;
        }
        lpSubBlock = UnicodeString.Buffer;

    } else {
        lpSubBlockOrg = (LPWSTR)LocalAlloc(LPTR,(lstrlen(lpSubBlockX)+1)*sizeof(WCHAR));
        if (lpSubBlockOrg == NULL ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        lstrcpy(lpSubBlockOrg,lpSubBlockX);
        lpSubBlock = lpSubBlockOrg;
    }



    /* Ensure that the total length is less than 32K but greater than the
     * size of a block header; we will assume that the size of pBlock is at
     * least the value of this first int.
     * Put a '\0' at the end of the block so that none of the wcslen's will
     * go past then end of the block.  We will replace it before returning.
     */
    if ((int)pBlock->wTotLen < sizeof(VERBLOCK))
        goto Fail;

    lpEndBlock = (LPWSTR)((LPSTR)pBlock + pBlock->wTotLen - sizeof(WCHAR));
    cEndBlock = *lpEndBlock;
    *lpEndBlock = 0;
    bString = FALSE;
    bLastSpec = FALSE;

    while ((*lpSubBlock || nIndex != -1)) {
        //
        // Ignore leading '\\'s
        //
        while (*lpSubBlock == TEXT('\\'))
            ++lpSubBlock;

        if ((*lpSubBlock || nIndex != -1)) {
            /* Make sure we still have some of the block left to play with
             */
            dwTotBlockLen = (DWORD)((LPSTR)lpEndBlock - (LPSTR)pBlock + sizeof(WCHAR));
            if ((int)dwTotBlockLen < sizeof(VERBLOCK) ||
                pBlock->wTotLen > (WORD)dwTotBlockLen)
                goto NotFound;

            /* Calculate the length of the "header" (the two length WORDs plus
             * the data type flag plus the identifying string) and skip
             * past the value.
             */
            dwHeadLen = DWORDUP(sizeof(VERBLOCK) - sizeof(WCHAR) +
                                (wcslen(pBlock->szKey) + 1) * sizeof(WCHAR)) +
                        DWORDUP(pBlock->wValLen);
            if (dwHeadLen > pBlock->wTotLen)
                goto NotFound;
            lpEndSubBlock = (LPWSTR)((LPSTR)pBlock + pBlock->wTotLen);
            pBlock = (VERBLOCK*)((LPSTR)pBlock+dwHeadLen);

            /* Look for the first sub-block name and terminate it
             */
            for (lpStart=lpSubBlock; *lpSubBlock && *lpSubBlock!=TEXT('\\');
                lpSubBlock++)
                /* find next '\\' */ ;
            cTemp = *lpSubBlock;
            *lpSubBlock = 0;

            /* Continue while there are sub-blocks left
             * pBlock->wTotLen should always be a valid pointer here because
             * we have validated dwHeadLen above, and we validated the previous
             * value of pBlock->wTotLen before using it
             */
            nCmp = 1;
            while ((int)pBlock->wTotLen > sizeof(VERBLOCK) &&
                   (int)pBlock->wTotLen <= (LPSTR)lpEndSubBlock-(LPSTR)pBlock) {

                //
                // Index functionality: if we are at the end of the path
                // (cTemp == 0 set below) and nIndex is NOT -1 (index search)
                // then break on nIndex zero.  Else do normal wscicmp.
                //
                if (bLastSpec && nIndex != -1) {

                    if (!nIndex) {

                        if (lplpKey) {
                            *lplpKey = pBlock->szKey;
                        }
                        nCmp=0;

                        //
                        // Index found, set nInde to -1
                        // so that we exit this loop
                        //
                        nIndex = -1;
                        break;
                    }

                    nIndex--;

                } else {

                    //
                    // Check if the sub-block name is what we are looking for
                    //

                    if (!(nCmp=_wcsicmp(lpStart, pBlock->szKey)))
                        break;
                }

                /* Skip to the next sub-block
                 */
                pBlock=(VERBLOCK*)((LPSTR)pBlock+DWORDUP(pBlock->wTotLen));
            }

            /* Restore the char NULLed above and return failure if the sub-block
             * was not found
             */
            *lpSubBlock = cTemp;
            if (nCmp)
                goto NotFound;
        }
        bLastSpec = !cTemp;
    }

    /* Fill in the appropriate buffers and return success
     */

    *puLen = pBlock->wValLen;

    /* Add code to handle the case of a null value.
     *
     * If zero-len, then return the pointer to the null terminator
     * of the key.  Remember that this is thunked in the ansi case.
     *
     * We can't just look at pBlock->wValLen.  Check if it really is
     * zero-len by seeing if the end of the key string is the end of the
     * block (i.e., the val string is outside of the current block).
     */

    lpStart = (LPWSTR)((LPSTR)pBlock+DWORDUP((sizeof(VERBLOCK)-sizeof(WCHAR))+
                                             (wcslen(pBlock->szKey)+1)*sizeof(WCHAR)));

    *lplpBuffer = lpStart < (LPWSTR)((LPBYTE)pBlock+pBlock->wTotLen) ?
                  lpStart :
                  (LPWSTR)(pBlock->szKey+wcslen(pBlock->szKey));

    bString = pBlock->wType;

    *lpEndBlock = cEndBlock;

    /*
     * Must free string we allocated above
     */

    if (!bUnicodeNeeded) {
        RtlFreeUnicodeString(&UnicodeString);
    } else {
        LocalFree(lpSubBlockOrg);
    }

    /*----------------------------------------------------------------------
     * thunk the results
     *
     * Must always thunk key, sometimes (if bString true) value
     *----------------------------------------------------------------------*/

    if (!bUnicodeNeeded) {

        // See if we're looking at a V1 or a V2 input block so we know how much space we
        // have for decoding the strings.
        BOOL fV2 = *(PDWORD)((PBYTE)pb + DWORDUP(*((WORD*)pb))) == VER2_SIG ? TRUE : FALSE;

        DWORD cbAnsiTranslateBuffer;
        if (fV2) {
            cbAnsiTranslateBuffer = DWORDUP(*((WORD *)pb));
        } else {
            cbAnsiTranslateBuffer = DWORDUP(*((WORD *)pb)) / 2;
        }

        if (bString && *puLen != 0) {
            DWORD cb, cb2;

            //
            // Must multiply length by two (first subtract 1 since puLen includes the null terminator)
            //
            UnicodeString.Length = UnicodeString.MaximumLength = (SHORT)((*puLen - 1) * 2);
            UnicodeString.Buffer = *lplpBuffer;

            //
            // Do the string conversion in the second half of the buffer
            // Assumes wTotLen is first filed in VERHEAD
            //

            // cb = offset in buffer to beginning of string
            cb = (DWORD)((PBYTE)*lplpBuffer - (PBYTE)pb);

            // cb2 = offset in translation area for this string
            if (fV2) {
                cb2 = cb + sizeof(VER2_SIG);
            } else {
                cb2 = cb / 2;
            }

            AnsiString.Buffer = (PBYTE)pb + DWORDUP(*((WORD*)pb)) + cb2;

            AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(&UnicodeString);
            if ( AnsiString.MaximumLength > MAXUSHORT ) {
                goto Fail;
                }

            AnsiString.MaximumLength = (USHORT)(__min((DWORD)AnsiString.MaximumLength,
                                                      (DWORD)(cbAnsiTranslateBuffer-cb2)));

            RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

            *lplpBuffer = AnsiString.Buffer;
            *puLen = AnsiString.Length + 1;

        }

        if (lplpKey) {

            DWORD cb, cb2;

            //
            // Thunk the key
            //
            dwHeadLen = wcslen(*lplpKey);
            UnicodeString.Length = UnicodeString.MaximumLength = (SHORT)(dwHeadLen * sizeof(WCHAR));
            UnicodeString.Buffer = *lplpKey;

            // cb2 = offset in translation area for this string

            cb = (DWORD)((PBYTE)*lplpKey - (PBYTE)pb);
            if (fV2) {
                cb2 = cb + sizeof(VER2_SIG);
            } else {
                cb2 = cb / 2;
            }

            AnsiString.Buffer = (PBYTE)pb + DWORDUP(*((WORD*)pb)) + cb2;

            AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(&UnicodeString);
            if ( AnsiString.MaximumLength > MAXUSHORT ) {
                goto Fail;
                }

            AnsiString.MaximumLength = (USHORT)(__min((DWORD)AnsiString.MaximumLength,
                                                      (DWORD)(cbAnsiTranslateBuffer-cb2)));
            RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

            *lplpKey = AnsiString.Buffer;
            *puLen = AnsiString.Length+1;
        }
    }

    return (TRUE);


    NotFound:
    /* Restore the char we NULLed above
     */
    *lpEndBlock = cEndBlock;

    Fail:

    if (!bUnicodeNeeded) {

        RtlFreeUnicodeString(&UnicodeString);
    } else {
        LocalFree(lpSubBlockOrg);
    }

    return (FALSE);
}
