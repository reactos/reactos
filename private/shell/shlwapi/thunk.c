//============================================================================
//
// Unicode-to-Ansi thunks
//
//============================================================================

#include "priv.h"


/*----------------------------------------------------------
 Purpose: This function converts a wide-char string to a multi-byte
 string.

 If pszBuf is non-NULL and the converted string can fit in
 pszBuf, then *ppszAnsi will point to the given buffer.
 Otherwise, this function will allocate a buffer that can
 hold the converted string.

 If pszWide is NULL, then *ppszAnsi will be freed.  Note
 that pszBuf must be the same pointer between the call
 that converted the string and the call that frees the
 string.

 Returns: TRUE
 FALSE (if out of memory)

 Cond:    --
 */
BOOL
AnsiFromUnicode(
    LPSTR * ppszAnsi,
    LPCWSTR pwszWide,        // NULL to clean up
    LPSTR pszBuf,
    int cchBuf)
{
    BOOL bRet;

    // Convert the string?
    if (pwszWide)
    {
        // Yes; determine the converted string length
        int cch;
        LPSTR psz;

        cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, NULL, 0, NULL, NULL);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pszBuf)
        {
            // Yes; allocate space
            cchBuf = cch + 1;
            psz = (LPSTR)LocalAlloc(LPTR, CbFromCchA(cchBuf));
        }
        else
        {
            // No; use the provided buffer
            ASSERT(pszBuf);
            psz = pszBuf;
        }

        if (psz)
        {
            // Convert the string
            cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, psz, cchBuf, NULL, NULL);
            bRet = (0 < cch);
        }
        else
        {
            bRet = FALSE;
        }

        *ppszAnsi = psz;
    }
    else
    {
        // No; was this buffer allocated?
        if (*ppszAnsi && pszBuf != *ppszAnsi)
        {
            // Yes; clean up
            LocalFree((HLOCAL)*ppszAnsi);
            *ppszAnsi = NULL;
        }
        bRet = TRUE;
    }

    return bRet;
}
