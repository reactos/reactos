/*
 * MAPI Utility functions
 *
 * Copyright 2004 Jon Griffiths
 * Copyright 2009 Owen Rudge for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "winternl.h"
#include "objbase.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "mapival.h"
#include "xcmc.h"
#include "msi.h"
#include "util.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

static const BYTE digitsToHex[] = {
  0,1,2,3,4,5,6,7,8,9,0xff,0xff,0xff,0xff,0xff,0xff,0xff,10,11,12,13,14,15,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,10,11,12,13,
  14,15 };

MAPI_FUNCTIONS mapiFunctions;

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
    if (mapiFunctions.ScInitMapiUtil)
        return mapiFunctions.ScInitMapiUtil(ulReserved);

    FIXME("(0x%08lx)stub!\n", ulReserved);
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
    if (mapiFunctions.DeinitMapiUtil)
        mapiFunctions.DeinitMapiUtil();
    else
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

    TRACE("(%ld,%p)\n", cbSize, lppBuffer);

    if (mapiFunctions.MAPIAllocateBuffer)
        return mapiFunctions.MAPIAllocateBuffer(cbSize, lppBuffer);

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

    TRACE("(%ld,%p,%p)\n", cbSize, lpOrig, lppBuffer);

    if (mapiFunctions.MAPIAllocateMore)
        return mapiFunctions.MAPIAllocateMore(cbSize, lpOrig, lppBuffer);

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

    if (mapiFunctions.MAPIFreeBuffer)
        return mapiFunctions.MAPIFreeBuffer(lpBuffer);

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
 * HrDispatchNotifications@4 (MAPI32.239)
 */
HRESULT WINAPI HrDispatchNotifications(ULONG flags)
{
    FIXME("(%08lx)\n", flags);
    return S_OK;
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
    if (mapiFunctions.HrThisThreadAdviseSink)
        return mapiFunctions.HrThisThreadAdviseSink(lpSink, lppNewSink);

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
    return lstrlenW(lpszStr);
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
    return lstrcmpW(lpszLeft, lpszRight);
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
    len = (lstrlenW(lpszSrc) + 1) * sizeof(WCHAR);
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

    TRACE("0x%08lx,%s,%s\n", dwCp, debugstr_w(lpszLeft), debugstr_w(lpszRight));
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
        return !lstrcmpW(lpName1->Kind.lpwstrName, lpName2->Kind.lpwstrName);

    return lpName1->Kind.lID == lpName2->Kind.lID;
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

    TRACE("(%p,%p,0x%08lx,%s,%s,%p)\n", lpAlloc, lpFree, ulFlags,
          debugstr_a((LPSTR)lpszPath), debugstr_a((LPSTR)lpszPrefix), lppStream);

    if (mapiFunctions.OpenStreamOnFile)
        return mapiFunctions.OpenStreamOnFile(lpAlloc, lpFree, ulFlags, lpszPath, lpszPrefix, lppStream);

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

    TRACE("%s %s %p %lu %d\n", component, qualifier, dll_path, dll_path_length, install);

    if (mapiFunctions.FGetComponentPath)
        return mapiFunctions.FGetComponentPath(component, qualifier, dll_path, dll_path_length, install);

    dll_path[0] = 0;

    hmsi = LoadLibraryA("msi.dll");
    if (hmsi)
    {
        UINT (WINAPI *pMsiProvideQualifiedComponentA)(LPCSTR, LPCSTR, DWORD, LPSTR, LPDWORD);

        pMsiProvideQualifiedComponentA = (void *)GetProcAddress(hmsi, "MsiProvideQualifiedComponentA");
        if (pMsiProvideQualifiedComponentA)
        {
            static const char * const fmt[] = { "%d\\NT", "%d\\95", "%d" };
            char lcid_ver[20];
            UINT i;

            for (i = 0; i < ARRAY_SIZE(fmt); i++)
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

/**************************************************************************
 *  HrQueryAllRows   (MAPI32.75)
 */
HRESULT WINAPI HrQueryAllRows(LPMAPITABLE lpTable, LPSPropTagArray lpPropTags,
    LPSRestriction lpRestriction, LPSSortOrderSet lpSortOrderSet,
    LONG crowsMax, LPSRowSet *lppRows)
{
    if (mapiFunctions.HrQueryAllRows)
        return mapiFunctions.HrQueryAllRows(lpTable, lpPropTags, lpRestriction, lpSortOrderSet, crowsMax, lppRows);

    FIXME("(%p, %p, %p, %p, %ld, %p): stub\n", lpTable, lpPropTags, lpRestriction, lpSortOrderSet, crowsMax, lppRows);
    *lppRows = NULL;
    return MAPI_E_CALL_FAILED;
}

/**************************************************************************
 *  WrapCompressedRTFStream   (MAPI32.186)
 */
HRESULT WINAPI WrapCompressedRTFStream(LPSTREAM compressed, ULONG flags, LPSTREAM *uncompressed)
{
    if (mapiFunctions.WrapCompressedRTFStream)
        return mapiFunctions.WrapCompressedRTFStream(compressed, flags, uncompressed);

    FIXME("(%p, 0x%08lx, %p): stub\n", compressed, flags, uncompressed);
    return MAPI_E_NO_SUPPORT;
}

static HMODULE mapi_provider;
static HMODULE mapi_ex_provider;

/**************************************************************************
 *  load_mapi_provider
 *
 * Attempts to load a MAPI provider from the specified registry key.
 *
 * Returns a handle to the loaded module in `mapi_provider' if successful.
 */
static void load_mapi_provider(HKEY hkeyMail, LPCWSTR valueName, HMODULE *mapi_provider)
{
    DWORD dwType, dwLen = 0;
    LPWSTR dllPath;

    /* Check if we have a value set for DLLPath */
    if ((RegQueryValueExW(hkeyMail, valueName, NULL, &dwType, NULL, &dwLen) == ERROR_SUCCESS) &&
        ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ)) && (dwLen > 0))
    {
        dllPath = HeapAlloc(GetProcessHeap(), 0, dwLen);

        if (dllPath)
        {
            RegQueryValueExW(hkeyMail, valueName, NULL, NULL, (LPBYTE)dllPath, &dwLen);

            /* Check that this value doesn't refer to mapi32.dll (eg, as Outlook does) */
            if (lstrcmpiW(dllPath, L"mapi32.dll") != 0)
            {
                if (dwType == REG_EXPAND_SZ)
                {
                    DWORD dwExpandLen;
                    LPWSTR dllPathExpanded;

                    /* Expand the path if necessary */
                    dwExpandLen = ExpandEnvironmentStringsW(dllPath, NULL, 0);
                    dllPathExpanded = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * dwExpandLen + 1);

                    if (dllPathExpanded)
                    {
                        ExpandEnvironmentStringsW(dllPath, dllPathExpanded, dwExpandLen + 1);

                        HeapFree(GetProcessHeap(), 0, dllPath);
                        dllPath = dllPathExpanded;
                    }
                }

                /* Load the DLL */
                TRACE("loading %s\n", debugstr_w(dllPath));
                *mapi_provider = LoadLibraryW(dllPath);
            }

            HeapFree(GetProcessHeap(), 0, dllPath);
        }
    }
}

/**************************************************************************
 *  load_mapi_providers
 *
 * Scans the registry for MAPI providers and attempts to load a Simple and
 * Extended MAPI library.
 *
 * Returns TRUE if at least one library loaded, FALSE otherwise.
 */
void load_mapi_providers(void)
{
    static const WCHAR regkey_mail[] = L"Software\\Clients\\Mail";

    HKEY hkeyMail;
    DWORD dwType, dwLen = 0;
    LPWSTR appName = NULL, appKey = NULL;

    TRACE("()\n");

    /* Open the Mail key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regkey_mail, 0, KEY_READ, &hkeyMail) != ERROR_SUCCESS)
        return;

    /* Check if we have a default value set, and the length of it */
    if ((RegQueryValueExW(hkeyMail, NULL, NULL, &dwType, NULL, &dwLen) != ERROR_SUCCESS) ||
        !((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ)) || (dwLen == 0))
        goto cleanUp;

    appName = HeapAlloc(GetProcessHeap(), 0, dwLen);

    if (!appName)
        goto cleanUp;

    /* Get the value, and get the path to the app key */
    RegQueryValueExW(hkeyMail, NULL, NULL, NULL, (LPBYTE)appName, &dwLen);

    TRACE("appName: %s\n", debugstr_w(appName));

    appKey = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (lstrlenW(regkey_mail) +
        lstrlenW(L"\\") + lstrlenW(appName) + 1));

    if (!appKey)
        goto cleanUp;

    lstrcpyW(appKey, regkey_mail);
    lstrcatW(appKey, L"\\");
    lstrcatW(appKey, appName);

    RegCloseKey(hkeyMail);

    TRACE("appKey: %s\n", debugstr_w(appKey));

    /* Open the app's key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, appKey, 0, KEY_READ, &hkeyMail) != ERROR_SUCCESS)
        goto cleanUp;

    /* Try to load the providers */
    load_mapi_provider(hkeyMail, L"DLLPath", &mapi_provider);
    load_mapi_provider(hkeyMail, L"DLLPathEx", &mapi_ex_provider);

    /* Now try to load our function pointers */
    ZeroMemory(&mapiFunctions, sizeof(mapiFunctions));

    /* Simple MAPI functions */
    if (mapi_provider)
    {
        mapiFunctions.MAPIAddress = (void*) GetProcAddress(mapi_provider, "MAPIAddress");
        mapiFunctions.MAPIDeleteMail = (void*) GetProcAddress(mapi_provider, "MAPIDeleteMail");
        mapiFunctions.MAPIDetails = (void*) GetProcAddress(mapi_provider, "MAPIDetails");
        mapiFunctions.MAPIFindNext = (void*) GetProcAddress(mapi_provider, "MAPIFindNext");
        mapiFunctions.MAPILogoff = (void*) GetProcAddress(mapi_provider, "MAPILogoff");
        mapiFunctions.MAPILogon = (void*) GetProcAddress(mapi_provider, "MAPILogon");
        mapiFunctions.MAPIReadMail = (void*) GetProcAddress(mapi_provider, "MAPIReadMail");
        mapiFunctions.MAPIResolveName = (void*) GetProcAddress(mapi_provider, "MAPIResolveName");
        mapiFunctions.MAPISaveMail = (void*) GetProcAddress(mapi_provider, "MAPISaveMail");
        mapiFunctions.MAPISendDocuments = (void*) GetProcAddress(mapi_provider, "MAPISendDocuments");
        mapiFunctions.MAPISendMail = (void*) GetProcAddress(mapi_provider, "MAPISendMail");
        mapiFunctions.MAPISendMailW = (void*) GetProcAddress(mapi_provider, "MAPISendMailW");
    }

    /* Extended MAPI functions */
    if (mapi_ex_provider)
    {
        mapiFunctions.MAPIInitialize = (void*) GetProcAddress(mapi_ex_provider, "MAPIInitialize");
        mapiFunctions.MAPILogonEx = (void*) GetProcAddress(mapi_ex_provider, "MAPILogonEx");
        mapiFunctions.MAPIUninitialize = (void*) GetProcAddress(mapi_ex_provider, "MAPIUninitialize");

        mapiFunctions.DeinitMapiUtil = (void*) GetProcAddress(mapi_ex_provider, "DeinitMapiUtil@0");
        mapiFunctions.DllCanUnloadNow = (void*) GetProcAddress(mapi_ex_provider, "DllCanUnloadNow");
        mapiFunctions.DllGetClassObject = (void*) GetProcAddress(mapi_ex_provider, "DllGetClassObject");
        mapiFunctions.FGetComponentPath = (void*) GetProcAddress(mapi_ex_provider, "FGetComponentPath");
        mapiFunctions.HrThisThreadAdviseSink = (void*) GetProcAddress(mapi_ex_provider, "HrThisThreadAdviseSink@8");
        mapiFunctions.HrQueryAllRows = (void*) GetProcAddress(mapi_ex_provider, "HrQueryAllRows@24");
        mapiFunctions.MAPIAdminProfiles = (void*) GetProcAddress(mapi_ex_provider, "MAPIAdminProfiles");
        mapiFunctions.MAPIAllocateBuffer = (void*) GetProcAddress(mapi_ex_provider, "MAPIAllocateBuffer");
        mapiFunctions.MAPIAllocateMore = (void*) GetProcAddress(mapi_ex_provider, "MAPIAllocateMore");
        mapiFunctions.MAPIFreeBuffer = (void*) GetProcAddress(mapi_ex_provider, "MAPIFreeBuffer");
        mapiFunctions.MAPIGetDefaultMalloc = (void*) GetProcAddress(mapi_ex_provider, "MAPIGetDefaultMalloc@0");
        mapiFunctions.MAPIOpenLocalFormContainer = (void *) GetProcAddress(mapi_ex_provider, "MAPIOpenLocalFormContainer");
        mapiFunctions.OpenStreamOnFile = (void*) GetProcAddress(mapi_ex_provider, "OpenStreamOnFile@24");
        mapiFunctions.ScInitMapiUtil = (void*) GetProcAddress(mapi_ex_provider, "ScInitMapiUtil@4");
        mapiFunctions.WrapCompressedRTFStream = (void*) GetProcAddress(mapi_ex_provider, "WrapCompressedRTFStream@12");
    }

cleanUp:
    RegCloseKey(hkeyMail);
    HeapFree(GetProcessHeap(), 0, appKey);
    HeapFree(GetProcessHeap(), 0, appName);
}

/**************************************************************************
 *  unload_mapi_providers
 *
 * Unloads any loaded MAPI libraries.
 */
void unload_mapi_providers(void)
{
    TRACE("()\n");

    FreeLibrary(mapi_provider);
    FreeLibrary(mapi_ex_provider);
}
