/*
 * Copyright (C) 2014 Martin Storsjo
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

#ifndef __WINSTRING_H_
#define __WINSTRING_H_

#include <hstring.h>

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI WindowsCompareStringOrdinal(HSTRING str1, HSTRING str2, INT32 *order);
HRESULT WINAPI WindowsConcatString(HSTRING str1, HSTRING str2, HSTRING *out);
HRESULT WINAPI WindowsCreateString(LPCWSTR ptr, UINT32 len, HSTRING *out);
HRESULT WINAPI WindowsCreateStringReference(LPCWSTR ptr, UINT32 len,
                                            HSTRING_HEADER *header, HSTRING *out);
HRESULT WINAPI WindowsDeleteString(HSTRING str);
HRESULT WINAPI WindowsDeleteStringBuffer(HSTRING_BUFFER buf);
HRESULT WINAPI WindowsDuplicateString(HSTRING str, HSTRING *out);
UINT32  WINAPI WindowsGetStringLen(HSTRING str);
LPCWSTR WINAPI WindowsGetStringRawBuffer(HSTRING str, UINT32 *len);
BOOL    WINAPI WindowsIsStringEmpty(HSTRING str);
HRESULT WINAPI WindowsPreallocateStringBuffer(UINT32 len, WCHAR **outptr, HSTRING_BUFFER *out);
HRESULT WINAPI WindowsPromoteStringBuffer(HSTRING_BUFFER buf, HSTRING *out);
HRESULT WINAPI WindowsReplaceString(HSTRING haystack, HSTRING needle, HSTRING replacement,
                                    HSTRING *out);
HRESULT WINAPI WindowsStringHasEmbeddedNull(HSTRING str, BOOL *out);
HRESULT WINAPI WindowsSubstring(HSTRING str, UINT32 pos, HSTRING *out);
HRESULT WINAPI WindowsSubstringWithSpecifiedLength(HSTRING str, UINT32 pos,
                                                   UINT32 len, HSTRING *out);
HRESULT WINAPI WindowsTrimStringEnd(HSTRING str, HSTRING charstr, HSTRING *out);
HRESULT WINAPI WindowsTrimStringStart(HSTRING str, HSTRING charstr, HSTRING *out);

#ifdef __cplusplus
}
#endif

#endif  /* __WINSTRING_H_ */
