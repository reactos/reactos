/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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

#ifndef _PATCHAPI_H_
#define _PATCHAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define APPLY_OPTION_FAIL_IF_EXACT  0x00000001
#define APPLY_OPTION_FAIL_IF_CLOSE  0x00000002
#define APPLY_OPTION_TEST_ONLY      0x00000004
#define APPLY_OPTION_VALID_FLAGS    0x00000007

typedef struct _PATCH_IGNORE_RANGE
{
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
} PATCH_IGNORE_RANGE, *PPATCH_IGNORE_RANGE;

typedef struct _PATCH_RETAIN_RANGE
{
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
    ULONG OffsetInNewFile;
} PATCH_RETAIN_RANGE, *PPATCH_RETAIN_RANGE;

BOOL WINAPI ApplyPatchToFileA(LPCSTR,LPCSTR,LPCSTR,ULONG);
BOOL WINAPI ApplyPatchToFileW(LPCWSTR,LPCWSTR,LPCWSTR,ULONG);
#define     ApplyPatchToFile WINELIB_NAME_AW(ApplyPatchToFile)

#ifdef __cplusplus
}
#endif

#endif /* _PATCHAPI_H_ */
