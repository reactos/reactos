#ifndef __UTILS_H__
#define __UTILS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if 0
VOID
MemInit(IN HANDLE Heap);
#endif

BOOL
MemFree(IN PVOID lpMem);

PVOID
MemAlloc(IN DWORD dwFlags,
         IN DWORD dwBytes);

LPWSTR
FormatDateTime(IN LPSYSTEMTIME pDateTime);

VOID
FreeDateTime(IN LPWSTR lpszDateTime);

LPWSTR
LoadResourceStringEx(IN HINSTANCE hInstance,
                     IN UINT uID,
                     OUT size_t* pSize OPTIONAL);

LPWSTR
LoadConditionalResourceStringEx(IN HINSTANCE hInstance,
                                IN BOOL bCondition,
                                IN UINT uIDifTrue,
                                IN UINT uIDifFalse,
                                IN size_t* pSize OPTIONAL);

#define LoadResourceString(hInst, uID) \
    LoadResourceStringEx((hInst), (uID), NULL)

#define LoadConditionalResourceString(hInst, bCond, uIDifT, uIDifF) \
    LoadConditionalResourceStringEx((hInst), (bCond), (uIDifT), (uIDifF), NULL)

LPWSTR
GetExecutableVendor(IN LPCWSTR lpszFilename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __UTILS_H__

/* EOF */
