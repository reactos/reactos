/****************************** Module Header ******************************\
* Module Name: random.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains a random collection of support routines for the User
* API functions.  Many of these functions will be moved to more appropriate
* files once we get our act together.
*
* History:
* 10-17-90 DarrinM      Created.
* 02-06-91 IanJa        HWND revalidation added (none required)
\***************************************************************************/


/***************************************************************************\
* RtlGetExpWinVer
*
* Returns the expected windows version, in the same format as Win3.1's
* GetExpWinVer(). This takes it out of the module header. As such, this
* api cannot be called from the server context to get version info for
* a client process - instead that information needs to be queried ahead
* of time and passed with any client/server call.
*
* 03-14-92 ScottLu      Created.
\***************************************************************************/

DWORD RtlGetExpWinVer(
    HANDLE hmod)
{
    PIMAGE_NT_HEADERS pnthdr;
    DWORD dwMajor = 3;
    DWORD dwMinor = 0xA;

    /*
     * If it doesn't look like a valid 32bit hmod, use the default
     *  (i.e., assuming all 16bit hmods are 0x30a)
     */
    if ((hmod != NULL) && (LOWORD(HandleToUlong(hmod)) == 0)) {
        try {
            pnthdr = RtlImageNtHeader((PVOID)hmod);
            dwMajor = pnthdr->OptionalHeader.MajorSubsystemVersion;
            /*
             * Still need this hack 'cuz the linker still puts
             * version 1.00 in the header of some things.
             */
            if (dwMajor == 1) {
                dwMajor = 0x3;
            } else {
                dwMinor = pnthdr->OptionalHeader.MinorSubsystemVersion;
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            dwMajor = 3;        // just to be safe
            dwMinor = 0xA;
        }
    }


    /*
     * Return this is a win3.1 compatible format:
     *
     * 0x030A == win3.1
     * 0x0300 == win3.0
     * 0x0200 == win2.0, etc.
     */

    return (DWORD)MAKELONG(MAKEWORD((BYTE)dwMinor, (BYTE)dwMajor), 0);
}

/***************************************************************************\
* FindCharPosition
*
* Finds position of character ch in lpString.  If not found, the length
* of the string is returned.
*
* History:
*   11-13-90 JimA                Created.
\***************************************************************************/

DWORD FindCharPosition(
    LPWSTR lpString,
    WCHAR ch)
{
    DWORD dwPos = 0L;

    while (*lpString && *lpString != ch) {
        ++lpString;
        ++dwPos;
    }
    return dwPos;
}


/***************************************************************************\
* TextCopy
*
* Returns: number of characters copied not including the NULL
*
* History:
* 10-25-90 MikeHar      Wrote.
* 11-09-90 DarrinM      Rewrote with a radically new algorithm.
* 01-25-91 MikeHar      Fixed the radically new algorithm.
* 02-01-91 DarrinM      Bite me.
* 11-26-91 DarrinM      Ok, this time it's perfect (except NLS, probably).
* 01-13-92 GregoryW     Now it's okay for Unicode.
\***************************************************************************/

UINT TextCopy(
    PLARGE_UNICODE_STRING pstr,
    LPWSTR pszDst,
    UINT cchMax)
{
    if (cchMax != 0) {
        cchMax = min(pstr->Length / sizeof(WCHAR), cchMax - 1);
        RtlCopyMemory(pszDst, (PVOID)pstr->Buffer, cchMax * sizeof(WCHAR));
        pszDst[cchMax] = 0;
    }

    return cchMax;
}

/***************************************************************************\
* DWORD wcsncpycch(dest, source, count) - copy no more than n wide chars
*
* Purpose:
*       Copies no more than count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is NOT padded
*       with more than 1 null character.
*
*
* Entry:
*       LPWSTR dest - pointer to destination
*       LPWSTR source - source string for copy
*       DWORD count - max number of characters to copy
*
* Exit:
*       returns number of characters copied into dest, including the null
*   terminator, if any.
*
* Exceptions:
*
****************************************************************************/

DWORD wcsncpycch (
        LPWSTR dest,
        LPCWSTR source,
        DWORD count
        )
{
        LPWSTR start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        return (DWORD)(dest - start);
}

/***************************************************************************\
* DWORD strncpycch(dest, source, count) - copy no more than n characters
*
* Purpose:
*       Copies no more than count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is NOT padded
*       with more than 1 null character.
*
*
* Entry:
*       LPSTR dest - pointer to destination
*       LPSTR source - source string for copy
*       DWORD count - max number of characters to copy
*
* Exit:
*       returns number of characters copied into dest, including the null
*   terminator, if any.
*
* Exceptions:
*
*******************************************************************************/

DWORD strncpycch (
        LPSTR dest,
        LPCSTR source,
        DWORD count
        )
{
        LPSTR start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        return (DWORD)(dest - start);
}
