/*
 * MAPI Utility functions
 *
 * Copyright 2004 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "winternl.h"
#include "objbase.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "mapival.h"
#include "xcmc.h"
#include "msi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

static const BYTE digitsToHex[] = {
  0,1,2,3,4,5,6,7,8,9,0xff,0xff,0xff,0xff,0xff,0xff,0xff,10,11,12,13,14,15,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,10,11,12,13,
  14,15 };

/**************************************************************************
 *  ScInitMapiUtil (MAPI32.33)
 *
 * Initialise Mapi utility functions.
 *
 * PARAMS
 *  ulReserved [I] Reserved, pass 0.
 *
 * RETURNS
 *  Success: S_OK. Mapi utility functions may be called.
 *  Failure: MAPI_E_INVALID_PARAMETER, if ulReserved is not 0.
 *
 * NOTES
 *  Your application does not need to call this function unless it does not
 *  call MAPIInitialize()/MAPIUninitialize().
 */
SCODE WINAPI ScInitMapiUtil(ULONG ulReserved)
{
    FIXME("(0x%08x)stub!\n", ulReserved);
    if (ulReserved)
        return MAPI_E_INVALID_PARAMETER;
    return S_OK;
}

/**************************************************************************
 *  DeinitMapiUtil (MAPI32.34)
 *
 * Uninitialise Mapi utility functions.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Your application does not need to call this function unless it does not
 *  call MAPIInitialize()/MAPIUninitialize().
 */
VOID WINAPI DeinitMapiUtil(void)
{
    FIXME("()stub!\n");
}

typedef LPVOID *LPMAPIALLOCBUFFER;

/**************************************************************************
 *  MAPIAllocateBuffer   (MAPI32.12)
 *  MAPIAllocateBuffer@8 (MAPI32.13)
 *
 * Allocate a block of memory.
 *
 * PARAMS
 *  cbSize    [I] Size of the block to allocate in bytes
 *  lppBuffer [O] Destination for pointer to allocated memory
 *
 * RETURNS
 *  Success: S_OK. *lppBuffer is filled with a pointer to a memory block of
 *           length cbSize bytes.
 *  Failure: MAPI_E_INVALID_PARAMETER, if lppBuffer is NULL.
 *           MAPI_E_NOT_ENOUGH_MEMORY, if the memory allocation fails.
 *
 * NOTES
 *  Memory allocated with this function should be freed with MAPIFreeBuffer().
 *  Further allocations of memory may be linked to the pointer returned using
 *  MAPIAllocateMore(). Linked allocations are freed when the initial pointer
 *  is feed.
 */
SCODE WINAPI MAPIAllocateBuffer(ULONG cbSize, LPVOID *lppBuffer)
{
    LPMAPIALLOCBUFFER lpBuff;

    TRACE("(%d,%p)\n", cbSize, lppBuffer);

    if (!lppBuffer)
        return E_INVALIDARG;

    lpBuff = HeapAlloc(GetProcessHeap(), 0, cbSize + sizeof(*lpBuff));
    if (!lpBuff)
        return MAPI_E_NOT_ENOUGH_MEMORY;

    TRACE("initial allocation:%p, returning %p\n", lpBuff, lpBuff + 1);
    *lpBuff++ = NULL;
    *lppBuffer = lpBuff;
    return S_OK;
}

/**************************************************************************
 *  MAPIAllocateMore    (MAPI32.14)
 *  MAPIAllocateMore@12 (MAPI32.15)
 *
 * Allocate a block of memory linked to a previous allocation.
 *
 * PARAMS
 *  cbSize    [I] Size of the block to allocate in bytes
 *  lpOrig    [I] Initial allocation to link to, from MAPIAllocateBuffer()
 *  lppBuffer [O] Destination for pointer to allocated memory
 *
 * RETURNS
 *  Success: S_OK. *lppBuffer is filled with a pointer to a memory block of
 *           length cbSize bytes.
 *  Failure: MAPI_E_INVALID_PARAMETER, if lpOrig or lppBuffer is invalid.
 *           MAPI_E_NOT_ENOUGH_MEMORY, if memory allocation fails.
 *
 * NOTES
 *  Memory allocated with this function and stored in *lppBuffer is freed
 *  when lpOrig is passed to MAPIFreeBuffer(). It should not be freed independently.
 */
SCODE WINAPI MAPIAllocateMore(ULONG cbSize, LPVOID lpOrig, LPVOID *lppBuffer)
{
    LPMAPIALLOCBUFFER lpBuff = lpOrig;

    TRACE("(%d,%p,%p)\n", cbSize, lpOrig, lppBuffer);

    if (!lppBuffer || !lpBuff || !--lpBuff)
        return E_INVALIDARG;

    /* Find the last allocation in the chain */
    while (*lpBuff)
    {
        TRACE("linked:%p->%p\n", lpBuff, *lpBuff);
        lpBuff = *lpBuff;
    }

    if (SUCCEEDED(MAPIAllocateBuffer(cbSize, lppBuffer)))
    {
        *lpBuff = ((LPMAPIALLOCBUFFER)*lppBuffer) - 1;
        TRACE("linking %p->%p\n", lpBuff, *lpBuff);
    }
    return *lppBuffer ? S_OK : MAPI_E_NOT_ENOUGH_MEMORY;
}

/**************************************************************************
 *  MAPIFreeBuffer   (MAPI32.16)
 *  MAPIFreeBuffer@4 (MAPI32.17)
 *
 * Free a block of memory and any linked allocations associated with it.
 *
 * PARAMS
 *  lpBuffer [I] Memory to free, returned from MAPIAllocateBuffer()
 *
 * RETURNS
 *  S_OK.
 */
ULONG WINAPI MAPIFreeBuffer(LPVOID lpBuffer)
{
    LPMAPIALLOCBUFFER lpBuff = lpBuffer;

    TRACE("(%p)\n", lpBuffer);

    if (lpBuff && --lpBuff)
    {
        while (lpBuff)
        {
            LPVOID lpFree = lpBuff;

            lpBuff = *lpBuff;

            TRACE("linked:%p->%p, freeing %p\n", lpFree, lpBuff, lpFree);
            HeapFree(GetProcessHeap(), 0, lpFree);
        }
    }
    return S_OK;
}

/**************************************************************************
 *  WrapProgress@20 (MAPI32.41)
 */
HRESULT WINAPI WrapProgress(PVOID unk1, PVOID unk2, PVOID unk3, PVOID unk4, PVOID unk5)
{
    /* Native does not implement this function */
    return MAPI_E_NO_SUPPORT;
}

/*************************************************************************
 * HrThisThreadAdviseSink@8 (MAPI32.42)
 *
 * Ensure that an advise sink is only notified in its originating thread.
 *
 * PARAMS
 *  lpSink     [I] IMAPIAdviseSink interface to be protected
 *  lppNewSink [I] Destination for wrapper IMAPIAdviseSink interface
 *
 * RETURNS
 * Success: S_OK. *lppNewSink contains a new sink to use in place of lpSink.
 * Failure: E_INVALIDARG, if any parameter is invalid.
 */
HRESULT WINAPI HrThisThreadAdviseSink(LPMAPIADVISESINK lpSink, LPMAPIADVISESINK* lppNewSink)
{
    FIXME("(%p,%p)semi-stub\n", lpSink, lppNewSink);

    if (!lpSink || !lppNewSink)
        return E_INVALIDARG;

    /* Don't wrap the sink for now, just copy it */
    *lppNewSink = lpSink;
    IMAPIAdviseSink_AddRef(lpSink);
    return S_OK;
}

/*************************************************************************
 * FBinFromHex (MAPI32.44)
 *
 * Create an array of binary data from a string.
 *
 * PARAMS
 *  lpszHex [I] String to convert to binary data
 *  lpOut   [O] Destination for resulting binary data
 *
 * RETURNS
 *  Success: TRUE. lpOut contains the decoded binary data.
 *  Failure: FALSE, if lpszHex does not represent a binary string.
 *
 * NOTES
 *  - lpOut must be at least half the length of lpszHex in bytes.
 *  - Although the Mapi headers prototype this function as both
 *    Ascii and Unicode, there is only one (Ascii) implementation. This
 *    means that lpszHex is treated as an Ascii string (i.e. a single NUL
 *    character in the byte stream terminates the string).
 */
BOOL WINAPI FBinFromHex(LPWSTR lpszHex, LPBYTE lpOut)
{
    LPSTR lpStr = (LPSTR)lpszHex;

    TRACE("(%p,%p)\n", lpszHex, lpOut);

    while (*lpStr)
    {
        if (lpStr[0] < '0' || lpStr[0] > 'f' || digitsToHex[lpStr[0] - '0'] == 0xff ||
            lpStr[1] < '0' || lpStr[1] > 'f' || digitsToHex[lpStr[1] - '0'] == 0xff)
            return FALSE;

        *lpOut++ = (digitsToHex[lpStr[0] - '0'] << 4) | digitsToHex[lpStr[1] - '0'];
        lpStr += 2;
    }
    return TRUE;
}

/*************************************************************************
 * HexFromBin (MAPI32.45)
 *
 * Create a string from an array of binary data.
 *
 * PARAMS
 *  lpHex   [I] Binary data to convert to string
 *  iCount  [I] Length of lpHex in bytes
 *  lpszOut [O] Destination for resulting hex string
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  - lpszOut must be at least 2 * iCount + 1 bytes characters long.
 *  - Although the Mapi headers prototype this function as both
 *    Ascii and Unicode, there is only one (Ascii) implementation. This
 *    means that the resulting string is not properly NUL terminated
 *    if the caller expects it to be a Unicode string.
 */
void WINAPI HexFromBin(LPBYTE lpHex, int iCount, LPWSTR lpszOut)
{
    static const char hexDigits[] = { "0123456789ABCDEF" };
    LPSTR lpStr = (LPSTR)lpszOut;

    TRACE("(%p,%d,%p)\n", lpHex, iCount, lpszOut);

    while (iCount-- > 0)
    {
        *lpStr++ = hexDigits[*lpHex >> 4];
        *lpStr++ = hexDigits[*lpHex & 0xf];
        lpHex++;
    }
    *lpStr = '\0';
}

/*************************************************************************
 * SwapPlong@8 (MAPI32.47)
 *
 * Swap the bytes in a ULONG array.
 *
 * PARAMS
 *  lpData [O] Array to swap bytes in
 *  ulLen  [I] Number of ULONG element to swap the bytes of
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SwapPlong(PULONG lpData, ULONG ulLen)
{
    ULONG i;

    for (i = 0; i < ulLen; i++)
        lpData[i] = RtlUlongByteSwap(lpData[i]);
}

/*************************************************************************
 * SwapPword@8 (MAPI32.48)
 *
 * Swap the bytes in a USHORT array.
 *
 * PARAMS
 *  lpData [O] Array to swap bytes in
 *  ulLen  [I] Number of USHORT element to swap the bytes of
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SwapPword(PUSHORT lpData, ULONG ulLen)
{
    ULONG i;

    for (i = 0; i < ulLen; i++)
        lpData[i] = RtlUshortByteSwap(lpData[i]);
}

/**************************************************************************
 *  MNLS_lstrlenW@4 (MAPI32.62)
 *
 * Calculate the length of a Unicode string.
 *
 * PARAMS
 *  lpszStr [I] String to calculate the length of
 *
 * RETURNS
 *  The length of lpszStr in Unicode characters.
 */
ULONG WINAPI MNLS_lstrlenW(LPCWSTR lpszStr)
{
    TRACE("(%s)\n", debugstr_w(lpszStr));
    return strlenW(lpszStr);
}

/*************************************************************************
 * MNLS_lstrcmpW@8 (MAPI32.63)
 *
 * Compare two Unicode strings.
 *
 * PARAMS
 *  lpszLeft  [I] First string to compare
 *  lpszRight [I] Second string to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszLeft is less than, the same, or greater than lpszRight.
 */
INT WINAPI MNLS_lstrcmpW(LPCWSTR lpszLeft, LPCWSTR lpszRight)
{
    TRACE("(%s,%s)\n", debugstr_w(lpszLeft), debugstr_w(lpszRight));
    return strcmpW(lpszLeft, lpszRight);
}

/*************************************************************************
 * MNLS_lstrcpyW@8 (MAPI32.64)
 *
 * Copy a Unicode string to another string.
 *
 * PARAMS
 *  lpszDest [O] Destination string
 *  lpszSrc  [I] Source string
 *
 * RETURNS
 *  The length lpszDest in Unicode characters.
 */
ULONG WINAPI MNLS_lstrcpyW(LPWSTR lpszDest, LPCWSTR lpszSrc)
{
    ULONG len;

    TRACE("(%p,%s)\n", lpszDest, debugstr_w(lpszSrc));
    len = (strlenW(lpszSrc) + 1) * sizeof(WCHAR);
    memcpy(lpszDest, lpszSrc, len);
    return len;
}

/*************************************************************************
 * MNLS_CompareStringW@12 (MAPI32.65)
 *
 * Compare two Unicode strings.
 *
 * PARAMS
 *  dwCp      [I] Code page for the comparison
 *  lpszLeft  [I] First string to compare
 *  lpszRight [I] Second string to compare
 *
 * RETURNS
 *  CSTR_LESS_THAN, CSTR_EQUAL or CSTR_GREATER_THAN, indicating that
 *  lpszLeft is less than, the same, or greater than lpszRight.
 */
INT WINAPI MNLS_CompareStringW(DWORD dwCp, LPCWSTR lpszLeft, LPCWSTR lpszRight)
{
    INT ret;

    TRACE("0x%08x,%s,%s\n", dwCp, debugstr_w(lpszLeft), debugstr_w(lpszRight));
    ret = MNLS_lstrcmpW(lpszLeft, lpszRight);
    return ret < 0 ? CSTR_LESS_THAN : ret ? CSTR_GREATER_THAN : CSTR_EQUAL;
}

/**************************************************************************
 *  FEqualNames@8 (MAPI32.72)
 *
 * Compare two Mapi names.
 *
 * PARAMS
 *  lpName1 [I] First name to compare to lpName2
 *  lpName2 [I] Second name to compare to lpName1
 *
 * RETURNS
 *  TRUE, if the names are the same,
 *  FALSE, Otherwise.
 */
BOOL WINAPI FEqualNames(LPMAPINAMEID lpName1, LPMAPINAMEID lpName2)
{
    TRACE("(%p,%p)\n", lpName1, lpName2);

    if (!lpName1 || !lpName2 ||
        !IsEqualGUID(lpName1->lpguid, lpName2->lpguid) ||
        lpName1->ulKind != lpName2->ulKind)
        return FALSE;

    if (lpName1->ulKind == MNID_STRING)
        return !strcmpW(lpName1->Kind.lpwstrName, lpName2->Kind.lpwstrName);

    return lpName1->Kind.lID == lpName2->Kind.lID ? TRUE : FALSE;
}

/**************************************************************************
 *  IsBadBoundedStringPtr@8 (MAPI32.71)
 *
 * Determine if a string pointer is valid.
 *
 * PARAMS
 *  lpszStr [I] String to check
 *  ulLen   [I] Maximum length of lpszStr
 *
 * RETURNS
 *  TRUE, if lpszStr is invalid or longer than ulLen,
 *  FALSE, otherwise.
 */
BOOL WINAPI IsBadBoundedStringPtr(LPCSTR lpszStr, ULONG ulLen)
{
    if (!lpszStr || IsBadStringPtrA(lpszStr, -1) || strlen(lpszStr) >= ulLen)
        return TRUE;
    return FALSE;
}

/**************************************************************************
 *  FtAddFt@16 (MAPI32.121)
 *
 * Add two FILETIME's together.
 *
 * PARAMS
 *  ftLeft  [I] FILETIME to add to ftRight
 *  ftRight [I] FILETIME to add to ftLeft
 *
 * RETURNS
 *  The sum of ftLeft and ftRight
 */
LONGLONG WINAPI MAPI32_FtAddFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONGLONG *pl = (LONGLONG*)&ftLeft, *pr = (LONGLONG*)&ftRight;

    return *pl + *pr;
}

/**************************************************************************
 *  FtSubFt@16 (MAPI32.123)
 *
 * Subtract two FILETIME's together.
 *
 * PARAMS
 *  ftLeft  [I] Initial FILETIME
 *  ftRight [I] FILETIME to subtract from ftLeft
 *
 * RETURNS
 *  The remainder after ftRight is subtracted from ftLeft.
 */
LONGLONG WINAPI MAPI32_FtSubFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONGLONG *pl = (LONGLONG*)&ftLeft, *pr = (LONGLONG*)&ftRight;

    return *pr - *pl;
}

/**************************************************************************
 *  FtMulDw@12 (MAPI32.124)
 *
 * Multiply a FILETIME by a DWORD.
 *
 * PARAMS
 *  dwLeft  [I] DWORD to multiply with ftRight
 *  ftRight [I] FILETIME to multiply with dwLeft
 *
 * RETURNS
 *  The product of dwLeft and ftRight
 */
LONGLONG WINAPI MAPI32_FtMulDw(DWORD dwLeft, FILETIME ftRight)
{
    LONGLONG *pr = (LONGLONG*)&ftRight;

    return (LONGLONG)dwLeft * (*pr);
}

/**************************************************************************
 *  FtMulDwDw@8 (MAPI32.125)
 *
 * Multiply two DWORD, giving the result as a FILETIME.
 *
 * PARAMS
 *  dwLeft  [I] DWORD to multiply with dwRight
 *  dwRight [I] DWORD to multiply with dwLeft
 *
 * RETURNS
 *  The product of ftMultiplier and ftMultiplicand as a FILETIME.
 */
LONGLONG WINAPI MAPI32_FtMulDwDw(DWORD dwLeft, DWORD dwRight)
{
    return (LONGLONG)dwLeft * (LONGLONG)dwRight;
}

/**************************************************************************
 *  FtNegFt@8 (MAPI32.126)
 *
 * Negate a FILETIME.
 *
 * PARAMS
 *  ft [I] FILETIME to negate
 *
 * RETURNS
 *  The negation of ft.
 */
LONGLONG WINAPI MAPI32_FtNegFt(FILETIME ft)
{
    LONGLONG *p = (LONGLONG*)&ft;

    return - *p;
}

/**************************************************************************
 *  UlAddRef@4 (MAPI32.128)
 *
 * Add a reference to an object.
 *
 * PARAMS
 *  lpUnk [I] Object to add a reference to.
 *
 * RETURNS
 *  The new reference count of the object, or 0 if lpUnk is NULL.
 *
 * NOTES
 * See IUnknown_AddRef.
 */
ULONG WINAPI UlAddRef(void *lpUnk)
{
    TRACE("(%p)\n", lpUnk);

    if (!lpUnk)
        return 0UL;
    return IUnknown_AddRef((LPUNKNOWN)lpUnk);
}

/**************************************************************************
 *  UlRelease@4 (MAPI32.129)
 *
 * Remove a reference from an object.
 *
 * PARAMS
 *  lpUnk [I] Object to remove reference from.
 *
 * RETURNS
 *  The new reference count of the object, or 0 if lpUnk is NULL. If lpUnk is
 *  non-NULL and this function returns 0, the object pointed to by lpUnk has
 *  been released.
 *
 * NOTES
 * See IUnknown_Release.
 */
ULONG WINAPI UlRelease(void *lpUnk)
{
    TRACE("(%p)\n", lpUnk);

    if (!lpUnk)
        return 0UL;
    return IUnknown_Release((LPUNKNOWN)lpUnk);
}

/**************************************************************************
 *  UFromSz@4 (MAPI32.133)
 *
 * Read an integer from a string
 *
 * PARAMS
 *  lpszStr [I] String to read the integer from.
 *
 * RETURNS
 *  Success: The integer read from lpszStr.
 *  Failure: 0, if the first character in lpszStr is not 0-9.
 *
 * NOTES
 *  This function does not accept whitespace and stops at the first non-digit
 *  character.
 */
UINT WINAPI UFromSz(LPCSTR lpszStr)
{
    ULONG ulRet = 0;

    TRACE("(%s)\n", debugstr_a(lpszStr));

    if (lpszStr)
    {
        while (*lpszStr >= '0' && *lpszStr <= '9')
        {
            ulRet = ulRet * 10 + (*lpszStr - '0');
            lpszStr++;
        }
    }
    return ulRet;
}

/*************************************************************************
 * OpenStreamOnFile@24 (MAPI32.147)
 *
 * Create a stream on a file.
 *
 * PARAMS
 *  lpAlloc    [I] Memory allocation function
 *  lpFree     [I] Memory free function
 *  ulFlags    [I] Flags controlling the opening process
 *  lpszPath   [I] Path of file to create stream on
 *  lpszPrefix [I] Prefix of the temporary file name (if ulFlags includes SOF_UNIQUEFILENAME)
 *  lppStream  [O] Destination for created stream
 *
 * RETURNS
 * Success: S_OK. lppStream contains the new stream object
 * Failure: E_INVALIDARG if any parameter is invalid, or an HRESULT error code
 *          describing the error.
 */
HRESULT WINAPI OpenStreamOnFile(LPALLOCATEBUFFER lpAlloc, LPFREEBUFFER lpFree,
                                ULONG ulFlags, LPWSTR lpszPath, LPWSTR lpszPrefix,
                                LPSTREAM *lppStream)
{
    WCHAR szBuff[MAX_PATH];
    DWORD dwMode = STGM_READWRITE, dwAttributes = 0;
    HRESULT hRet;

    TRACE("(%p,%p,0x%08x,%s,%s,%p)\n", lpAlloc, lpFree, ulFlags,
          debugstr_a((LPSTR)lpszPath), debugstr_a((LPSTR)lpszPrefix), lppStream);

    if (lppStream)
        *lppStream = NULL;

    if (ulFlags & SOF_UNIQUEFILENAME)
    {
        FIXME("Should generate a temporary name\n");
        return E_INVALIDARG;
    }

    if (!lpszPath || !lppStream)
        return E_INVALIDARG;

    /* FIXME: Should probably munge mode and attributes, and should handle
     *        Unicode arguments (I assume MAPI_UNICODE is set in ulFlags if
     *        we are being passed Unicode strings; MSDN doesn't say).
     *        This implementation is just enough for Outlook97 to start.
     */
    MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpszPath, -1, szBuff, MAX_PATH);
    hRet = SHCreateStreamOnFileEx(szBuff, dwMode, dwAttributes, TRUE,
                                  NULL, lppStream);
    return hRet;
}

/*************************************************************************
 * UlFromSzHex@4 (MAPI32.155)
 *
 * Read an integer from a hexadecimal string.
 *
 * PARAMS
 *  lpSzHex [I] String containing the hexadecimal number to read
 *
 * RETURNS
 * Success: The number represented by lpszHex.
 * Failure: 0, if lpszHex does not contain a hex string.
 *
 * NOTES
 *  This function does not accept whitespace and stops at the first non-hex
 *  character.
 */
ULONG WINAPI UlFromSzHex(LPCWSTR lpszHex)
{
    LPCSTR lpStr = (LPCSTR)lpszHex;
    ULONG ulRet = 0;

    TRACE("(%s)\n", debugstr_a(lpStr));

    while (*lpStr)
    {
        if (lpStr[0] < '0' || lpStr[0] > 'f' || digitsToHex[lpStr[0] - '0'] == 0xff ||
            lpStr[1] < '0' || lpStr[1] > 'f' || digitsToHex[lpStr[1] - '0'] == 0xff)
            break;

        ulRet = ulRet * 16 + ((digitsToHex[lpStr[0] - '0'] << 4) | digitsToHex[lpStr[1] - '0']);
        lpStr += 2;
    }
    return ulRet;
}

/************************************************************************
 * FBadEntryList@4 (MAPI32.190)
 *
 * Determine is an entry list is invalid.
 *
 * PARAMS
 *  lpEntryList [I] List to check
 *
 * RETURNS
 *  TRUE, if lpEntryList is invalid,
 *  FALSE, otherwise.
 */
BOOL WINAPI FBadEntryList(LPENTRYLIST lpEntryList)
{
    ULONG i;

    if (IsBadReadPtr(lpEntryList, sizeof(*lpEntryList)) ||
        IsBadReadPtr(lpEntryList->lpbin,
                     lpEntryList->cValues * sizeof(*lpEntryList->lpbin)))
        return TRUE;

    for (i = 0; i < lpEntryList->cValues; i++)
        if(IsBadReadPtr(lpEntryList->lpbin[i].lpb, lpEntryList->lpbin[i].cb))
            return TRUE;

    return FALSE;
}

/*************************************************************************
 * CbOfEncoded@4 (MAPI32.207)
 *
 * Return the length of an encoded string.
 *
 * PARAMS
 *  lpSzEnc [I] Encoded string to get the length of.
 *
 * RETURNS
 * The length of the encoded string in bytes.
 */
ULONG WINAPI CbOfEncoded(LPCSTR lpszEnc)
{
    ULONG ulRet = 0;

    TRACE("(%s)\n", debugstr_a(lpszEnc));

    if (lpszEnc)
        ulRet = (((strlen(lpszEnc) | 3) >> 2) + 1) * 3;
    return ulRet;
}

/*************************************************************************
 * cmc_query_configuration (MAPI32.235)
 *
 * Retrieves the configuration information for the installed CMC
 *
 * PARAMS
 *  session          [I]   MAPI session handle
 *  item             [I]   Enumerated variable that identifies which 
 *                         configuration information is being requested
 *  reference        [O]   Buffer where configuration information is written
 *  config_extensions[I/O] Path of file to create stream on
 *
 * RETURNS
 * A CMD define
 */
CMC_return_code WINAPI cmc_query_configuration(
  CMC_session_id session,
  CMC_enum item,
  CMC_buffer reference,
  CMC_extension  *config_extensions)
{
	FIXME("stub\n");
	return CMC_E_NOT_SUPPORTED;
}

/**************************************************************************
 *  FGetComponentPath   (MAPI32.254)
 *  FGetComponentPath@20 (MAPI32.255)
 *
 * Return the installed component path, usually to the private mapi32.dll.
 *
 * PARAMS
 *  component       [I] Component ID
 *  qualifier       [I] Application LCID
 *  dll_path        [O] returned component path
 *  dll_path_length [I] component path length
 *  install         [I] install mode
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 *
 * NOTES
 *  Previously documented in Q229700 "How to locate the correct path
 *  to the Mapisvc.inf file in Microsoft Outlook".
 */
BOOL WINAPI FGetComponentPath(LPCSTR component, LPCSTR qualifier, LPSTR dll_path,
                              DWORD dll_path_length, BOOL install)
{
    BOOL ret = FALSE;
    HMODULE hmsi;

    TRACE("%s %s %p %u %d\n", component, qualifier, dll_path, dll_path_length, install);

    dll_path[0] = 0;

    hmsi = LoadLibraryA("msi.dll");
    if (hmsi)
    {
        FARPROC pMsiProvideQualifiedComponentA = GetProcAddress(hmsi, "MsiProvideQualifiedComponentA");

        if (pMsiProvideQualifiedComponentA)
        {
            static const char * const fmt[] = { "%d\\NT", "%d\\95", "%d" };
            char lcid_ver[20];
            UINT i;

            for (i = 0; i < sizeof(fmt)/sizeof(fmt[0]); i++)
            {
                /* FIXME: what's the correct behaviour here? */
                if (!qualifier || qualifier == lcid_ver)
                {
                    sprintf(lcid_ver, fmt[i], GetUserDefaultUILanguage());
                    qualifier = lcid_ver;
                }

                if (pMsiProvideQualifiedComponentA(component, qualifier,
                        install ? INSTALLMODE_DEFAULT : INSTALLMODE_EXISTING,
                        dll_path, &dll_path_length) == ERROR_SUCCESS)
                {
                    ret = TRUE;
                    break;
                }

                if (qualifier != lcid_ver) break;
            }
        }
        FreeLibrary(hmsi);
    }
    return ret;
}
