/* $Id: errormsg.c,v 1.1 2002/12/06 13:14:14 robd Exp $
 *
 * reactos/lib/kernel32/misc/errormsg.c
 *
 */
#include <ddk/ntddk.h>

// #define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* INTERNAL */


/* EXPORTED */

DWORD
STDCALL
FormatMessageW(
    DWORD    dwFlags,
    LPCVOID  lpSource,
    DWORD    dwMessageId,
    DWORD    dwLanguageId,
    LPWSTR   lpBuffer,
    DWORD    nSize,
    va_list* Arguments)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
FormatMessageA(
    DWORD    dwFlags,
    LPCVOID  lpSource,
    DWORD    dwMessageId,
    DWORD    dwLanguageId,
    LPSTR    lpBuffer,
    DWORD    nSize,
    va_list* Arguments)
{
    HLOCAL pBuf = NULL;
    //LPSTR pBuf = NULL;

#define MAX_MSG_STR_LEN 200

    if (lpBuffer != NULL) {

        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
            pBuf = LocalAlloc(LPTR, max(nSize, MAX_MSG_STR_LEN));
            if (pBuf == NULL) {
                return 0;
            }
            *(LPSTR*)lpBuffer = pBuf;
        } else {
            pBuf = *(LPSTR*)lpBuffer;
        }

        if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        } else {
        }

//FORMAT_MESSAGE_IGNORE_INSERTS
//FORMAT_MESSAGE_FROM_STRING
//FORMAT_MESSAGE_FROM_HMODULE
//FORMAT_MESSAGE_FROM_SYSTEM
//FORMAT_MESSAGE_ARGUMENT_ARRAY 

    }
/*
        if (FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
          0,
          error,
          MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
          (PTSTR)&msg,
          0,
          NULL)
        )
 */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* EOF */
