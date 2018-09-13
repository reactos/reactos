#include <windows.h>

#include "keys.h"

//--------------------------------------------------------------------------
// Description:
//   This implements lstrcat except that we always only cat up to the
//   passed on maxDest length.  This prevents cases where we cat past
//   the end of the destination buffer.
//
// Arguments:
//   pDest   - destination string to append to
//   pSrc    - src string to append 
//   maxDest - the maxuium number of characters of the destination buffer
//
// Returns: the destination buffer or NULL on error.  
//          GetLastError() will return the reason for the failure.
// 
//--------------------------------------------------------------------------
LPTSTR
lstrcatn(LPTSTR pDest, LPTSTR pSrc, int maxDest)
{
    int destLen;

    destLen=lstrlen(pDest);

    if (destLen < maxDest)
        return (lstrcpyn(pDest+destLen,pSrc,maxDest-destLen) ? pDest : NULL);

    //
    // if the buffer is the exact length and we have nothing to append
    // then this is ok, just return the destination buffer.
    //
    if ((destLen == maxDest) && ((NULL == pSrc) || (*pSrc == TEXT('\0'))))
        return pDest;

    //
    // the destination buffer is too small, so return an error.
    //
    SetLastError(ERROR_INSUFFICIENT_BUFFER);

    return NULL;
}

