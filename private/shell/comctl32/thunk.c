#include "ctlspriv.h"
#include <limits.h>

/*
 * Creates a buffer for a unicode string, and then copies the ANSI text
 * into it (converting it to unicode in the process)
 *
 * The returned pointer should be freed with LocalFree after use.
 */
LPWSTR ProduceWFromA( UINT uiCodePage, LPCSTR psz ) {
    LPWSTR pszW;
    int cch;

    if (psz == NULL || psz == LPSTR_TEXTCALLBACKA)
        return (LPWSTR)psz;

    // The old code would call lstrlen and lstrcpy which would fault internal to the
    // api, this should do about the same...
    if (IsBadReadPtr(psz,1))
        return NULL;    // For now lets try not setting a string...

    cch = MultiByteToWideChar(uiCodePage, 0, psz, -1, NULL, 0);

    if (cch == 0)
        cch = 1;

    pszW = LocalAlloc( LMEM_FIXED, cch * sizeof(WCHAR) );

    if (pszW != NULL ) {
         if (MultiByteToWideChar( uiCodePage, MB_PRECOMPOSED, psz, -1, pszW,
                cch ) == FALSE) {
            LocalFree(pszW);
            pszW = NULL;
        }
    }

    return pszW;

}


/*
 * Creates a buffer for a unicode string, and then copies the ANSI text
 * into it (converting it to unicode in the process)
 *
 * The returned pointer should be freed with LocalFree after use.
 */
LPSTR ProduceAFromW( UINT uiCodePage, LPCWSTR psz ) {
    LPSTR pszA;
    int cch;

    if (psz == NULL || psz == LPSTR_TEXTCALLBACKW)
        return (LPSTR)psz;

    cch = WideCharToMultiByte(uiCodePage, 0, psz, -1, NULL, 0, NULL, NULL);

    if (cch == 0)
        cch = 1;

    pszA = LocalAlloc( LMEM_FIXED, cch * sizeof(char) );

    if (pszA != NULL ) {
         if (WideCharToMultiByte(uiCodePage, 0, psz, -1, pszA, cch, NULL, NULL) ==
                                                                       FALSE) {
            LocalFree(pszA);
            pszA = NULL;
        }
    }

    return pszA;

}

