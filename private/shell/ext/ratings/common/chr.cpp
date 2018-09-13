#include "npcommon.h"

// strchrf(str, ch)
//
// Returns a pointer to the first occurrence of ch in str.
// Returns NULL if not found.
// May search for a double-byte character.

LPSTR WINAPI strchrf(LPCSTR lpString, UINT ch)
{
    while (*lpString) {
        if (ch == (IS_LEAD_BYTE(*lpString) ? *(LPWORD)lpString : *lpString))
            return (LPSTR)lpString;
        ADVANCE(lpString);
    }
    return NULL;
}


// strrchrf(str, ch)
//
// Returns a pointer to the last occurrence of ch in str.
// Returns NULL if not found.
// May search for a double-byte character.

LPSTR WINAPI strrchrf(LPCSTR lpString, UINT ch)
{
    LPSTR lpLast = NULL;

    while (*lpString) {
        if (ch == (IS_LEAD_BYTE(*lpString) ? *(LPWORD)lpString : *lpString))
            lpLast = (LPSTR)lpString;
        ADVANCE(lpString);
    }
    return lpLast;
}

