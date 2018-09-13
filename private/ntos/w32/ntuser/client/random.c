/****************************** Module Header ******************************\
* Module Name: random.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This file contains global function pointers that are called trough to get
* to either a client or a server function depending on which side we are on
*
* History:
* 10-Nov-1993 MikeKe    Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*
* History:
* 10-Nov-1993 MikeKe    Created
\***************************************************************************/

HBRUSH                      ghbrWhite = NULL;
HBRUSH                      ghbrBlack = NULL;

/***************************************************************************\
* GetSysColorBrush
*
* Retrieves the system-color-brush.
*
\***************************************************************************/
HBRUSH WINAPI GetSysColorBrush(
    int nIndex)
{
    if ((nIndex < 0) || (nIndex >= COLOR_MAX))
        return NULL;

    return SYSHBRUSH(nIndex);
}

/***************************************************************************\
* SetSysColorTemp
*
* Sets the global system colors all at once.  Also remembers the old colors
* so they can be reset.
*
* Sets/Resets the color and brush arrays for user USER drawing.
* lpRGBs and lpBrushes are pointers to arrays paralleling the argbSystem and
* gpsi->hbrSystem arrays.  wCnt is a sanity check so that this does the "right"
* thing in a future windows version.  The current argbSystem and hbrSystem
* arrays are saved off, and a handle to those saved arrays is returned.
*
* To reset the arrays, pass in NULL for lpRGBs, NULL for lpBrushes, and the
* handle (from the first set) for wCnt.
*
* History:
* 18-Sep-1995   JohnC   Gave this miserable function a life
\***************************************************************************/

LPCOLORREF gpOriginalRGBs = NULL;
UINT       gcOriginalRGBs = 0;

WINUSERAPI HANDLE WINAPI SetSysColorsTemp(
    CONST COLORREF *lpRGBs,
    CONST HBRUSH   *lpBrushes,
    UINT_PTR       cBrushes)      // Count of brushes or handle
{
    UINT cbRGBSize;
    UINT i;
    UINT abElements[COLOR_MAX];

    /*
     * See if we are resetting the colors back to a saved state
     */
    if (lpRGBs == NULL) {

        /*
         * When restoring cBrushes is really a handle to the old global
         * handle.  Make sure that is true.  Also lpBrushes is unused
         */
        UNREFERENCED_PARAMETER(lpBrushes);
        UserAssert(lpBrushes == NULL);
        UserAssert(cBrushes == (ULONG_PTR)gpOriginalRGBs);

        if (gpOriginalRGBs == NULL) {
            RIPMSG0(RIP_ERROR, "SetSysColorsTemp: Can not restore if not saved");
            return NULL;
        }

        /*
         * reset the global Colors
         */
        UserAssert((sizeof(abElements)/sizeof(abElements[0])) >= gcOriginalRGBs);
        for (i = 0; i < gcOriginalRGBs; i++)
            abElements[i] = i;

        NtUserSetSysColors(gcOriginalRGBs, abElements, gpOriginalRGBs, 0);

        UserLocalFree(gpOriginalRGBs);

        gpOriginalRGBs = NULL;
        gcOriginalRGBs = 0;

        return (HANDLE)TRUE;
    }

    /*
     * Make sure we aren't trying to set too many colors
     * If we allow more then COLOR_MAX change the abElements array
     */
    if (cBrushes > COLOR_MAX) {
        RIPMSG1(RIP_ERROR, "SetSysColorsTemp: trying to set too many colors %lX", cBrushes);
        return NULL;
    }

    /*
     * If we have already a saved state then don't let them save it again
     */
    if (gpOriginalRGBs != NULL) {
        RIPMSG0(RIP_ERROR, "SetSysColorsTemp: temp colors already set");
        return NULL;
    }

    /*
     * If we are here then we must be setting the new temp colors
     *
     * First save the old colors
     */
    cbRGBSize = sizeof(COLORREF) * (UINT)cBrushes;

    UserAssert(sizeof(COLORREF) == sizeof(int));
    gpOriginalRGBs = UserLocalAlloc(HEAP_ZERO_MEMORY, cbRGBSize);

    if (gpOriginalRGBs == NULL) {
        RIPMSG0(RIP_WARNING, "SetSysColorsTemp: unable to alloc temp colors buffer");
    }

    RtlCopyMemory(gpOriginalRGBs, gpsi->argbSystem, cbRGBSize);

    /*
     * Now set the new colors.
     */
    UserAssert( (sizeof(abElements)/sizeof(abElements[0])) >= cBrushes);

    for (i = 0; i < cBrushes; i++)
        abElements[i] = i;

    NtUserSetSysColors((UINT)cBrushes, abElements, lpRGBs, 0);

    gcOriginalRGBs = (UINT)cBrushes;

    return gpOriginalRGBs;
}

/***************************************************************************\
* TextAlloc
*
* History:
* 25-Oct-1990   MikeHar     Wrote.
* 09-Nov-1990   DarrinM     Fixed.
* 13-Jan-1992   GregoryW    Neutralized.
\***************************************************************************/

LPWSTR TextAlloc(
    LPCWSTR lpszSrc)
{
    LPWSTR pszT;
    DWORD  cbString;

    if (lpszSrc == NULL)
        return NULL;

    cbString = (wcslen(lpszSrc) + 1) * sizeof(WCHAR);

    if (pszT = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cbString)) {

        RtlCopyMemory(pszT, lpszSrc, cbString);
    }

    return pszT;
}

#if DBG
/***************************************************************************\
* CheckCurrentDesktop
*
* Ensure that the pointer is valid for the current desktop.
*
* History:
* 10-Apr-1995   JimA    Created.
\***************************************************************************/

VOID CheckCurrentDesktop(
    PVOID p)
{
    UserAssert(p >= GetClientInfo()->pDeskInfo->pvDesktopBase &&
               p < GetClientInfo()->pDeskInfo->pvDesktopLimit);
}
#endif


/***************************************************************************\
* SetLastErrorEx
*
* Sets the last error, ignoring dwtype.
\***************************************************************************/

VOID WINAPI SetLastErrorEx(
    DWORD dwErrCode,
    DWORD dwType
    )
{
    UNREFERENCED_PARAMETER(dwType);

    SetLastError(dwErrCode);
}

#if defined(_X86_)
/***************************************************************************\
* InitializeWin32EntryTable
*
* Initializes a Win32 entry table so our test apps will know which entry
* points to avoid. This should be removed before we ship.
\***************************************************************************/

static CONST PROC FunctionsToSkip[] = {
    NtUserWaitMessage,
    NtUserLockWorkStation,
};

UINT InitializeWin32EntryTable(
    PBOOLEAN pbEntryTable)
{
#if DBG
    // We'll only define this on free systems for now. Checked systems
    // will hit too many asserts.
    UNREFERENCED_PARAMETER(pbEntryTable);
    return 0;
#else
    UINT i;
    PBYTE pb;

    if (pbEntryTable) {
        for (i = 0; i < ARRAY_SIZE(FunctionsToSkip); i++) {
            pb = (PBYTE)FunctionsToSkip[i];
            pbEntryTable[*((WORD *)(pb+1)) - 0x1000] = TRUE;
        }

    }

    return gDispatchTableValues;
#endif
}
#endif
/***************************************************************************\
* GetLastInputInfo
*
* Retrieves information about the last input event
*
* 05/30/07  GerardoB    Created
\***************************************************************************/
BOOL GetLastInputInfo (PLASTINPUTINFO plii)
{
    VALIDATIONFNNAME(GetLastInputInfo);

    if (plii->cbSize != sizeof(LASTINPUTINFO)) {
        VALIDATIONFAIL(plii->cbSize);
    }

    plii->dwTime = gpsi->dwLastRITEventTickCount;

    return TRUE;
    VALIDATIONERROR(FALSE);
}

