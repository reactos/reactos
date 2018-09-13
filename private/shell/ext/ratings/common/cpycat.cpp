#include "npcommon.h"

// strncpyf(dest, src, cb)
//
// Always stores cb bytes to dest.  If total characters copied
// ends up less than cb bytes, zero-fills dest.
// If strlen(src) >= cb, dest is NOT null-terminated.
// Returns dest.

LPSTR WINAPI strncpyf(LPSTR lpDest, LPCSTR lpSrc, UINT cbCopy)
{
    LPCSTR lpChr = lpSrc;
    UINT cbToCopy = 0;

    // find ptr past last char to copy
    while (*lpChr) {
        if (cbToCopy + (IS_LEAD_BYTE(*lpChr) ? 2 : 1) > cbCopy)
            break;  // copying this char would run over the limit
        cbToCopy += IS_LEAD_BYTE(*lpChr) ? 2 : 1;
        ADVANCE(lpChr);
    }

    // copy that many bytes
    memcpyf(lpDest, lpSrc, cbToCopy);
    memsetf(lpDest + cbToCopy, '\0', cbCopy - cbToCopy);

    return lpDest;
}


// strncatf(dest, src, cb)
//
// Concatenates at most cb bytes of src onto the end of dest.
// Unlike strncpyf, does not pad with extra nulls, but does
// guarantee a null-terminated destination.
// Returns dest.

LPSTR WINAPI strncatf(LPSTR lpDest, LPCSTR lpSrc, UINT cbCopy)
{
    LPCSTR lpChr = lpSrc;
    UINT cbToCopy = 0;

    // find ptr past last char to copy
    while (*lpChr) {
        if (cbToCopy + (IS_LEAD_BYTE(*lpChr) ? 2 : 1) > cbCopy)
            break;  // copying this char would run over the limit
        cbToCopy += IS_LEAD_BYTE(*lpChr) ? 2 : 1;
        ADVANCE(lpChr);
    }

    // copy that many bytes
    memcpyf(lpDest, lpSrc, cbToCopy);
    lpDest[cbToCopy] = '\0';

    return lpDest;
}

