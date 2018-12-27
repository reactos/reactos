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


//
// apply error codes
//
#define ERROR_PATCH_DECODE_FAILURE  0xC00E4101
#define ERROR_PATCH_CORRUPT         0xC00E4102
#define ERROR_PATCH_NEWER_FORMAT    0xC00E4103
#define ERROR_PATCH_WRONG_FILE      0xC00E4104
#define ERROR_PATCH_NOT_NECESSARY   0xC00E4105
#define ERROR_PATCH_NOT_AVAILABLE   0xC00E4106


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


typedef WINBOOL (CALLBACK PATCH_PROGRESS_CALLBACK)(PVOID CallbackContext, ULONG CurrentPosition, ULONG MaximumPosition);
typedef PATCH_PROGRESS_CALLBACK *PPATCH_PROGRESS_CALLBACK;


BOOL WINAPI TestApplyPatchToFileA(LPCSTR PatchFileName, LPCSTR OldFileName, ULONG ApplyOptionFlags);
BOOL WINAPI TestApplyPatchToFileW(LPCWSTR PatchFileName, LPCWSTR OldFileName, ULONG ApplyOptionFlags);
#define     TestApplyPatchToFile WINELIB_NAME_AW(TestApplyPatchToFile)
BOOL WINAPI TestApplyPatchToFileByHandles(HANDLE PatchFileHandle, HANDLE OldFileHandle, ULONG ApplyOptionFlags);


BOOL WINAPI ApplyPatchToFileA(LPCSTR PatchFileName, LPCSTR OldFileName, LPCSTR NewFileName, ULONG ApplyOptionFlags);
BOOL WINAPI ApplyPatchToFileW(LPCWSTR PatchFileName, LPCWSTR OldFileName, LPCWSTR NewFileName, ULONG ApplyOptionFlags);
#define     ApplyPatchToFile WINELIB_NAME_AW(ApplyPatchToFile)
BOOL WINAPI ApplyPatchToFileByHandles(HANDLE PatchFileHandle, HANDLE OldFileHandle, HANDLE NewFileHandle, ULONG ApplyOptionFlags);


BOOL WINAPI GetFilePatchSignatureA(LPCSTR FileName, ULONG OptionFlags, PVOID OptionData, ULONG IgnoreRangeCount,
                                   PPATCH_IGNORE_RANGE IgnoreRangeArray, ULONG RetainRangeCount,
                                   PPATCH_RETAIN_RANGE RetainRangeArray, ULONG SignatureBufferSize,
                                   PVOID SignatureBuffer);
BOOL WINAPI GetFilePatchSignatureW(LPCWSTR FileName, ULONG OptionFlags, PVOID OptionData, ULONG IgnoreRangeCount,
                                   PPATCH_IGNORE_RANGE IgnoreRangeArray, ULONG RetainRangeCount,
                                   PPATCH_RETAIN_RANGE RetainRangeArray, ULONG SignatureBufferSize,
                                   PVOID SignatureBuffer);
#define     GetFilePatchSignature WINELIB_NAME_AW(GetFilePatchSignature)
BOOL WINAPI GetFilePatchSignatureByHandle(HANDLE FileHandle, ULONG OptionFlags, PVOID OptionData, ULONG IgnoreRangeCount,
                                          PPATCH_IGNORE_RANGE IgnoreRangeArray, ULONG RetainRangeCount,
                                          PPATCH_RETAIN_RANGE RetainRangeArray, ULONG SignatureBufferSize,
                                          PVOID SignatureBuffer);

#ifdef __cplusplus
}
#endif

#endif /* _PATCHAPI_H_ */
