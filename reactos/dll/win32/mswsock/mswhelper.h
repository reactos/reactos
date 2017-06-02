#ifndef _MSWHELPER_H
#define _MSWHELPER_H

#include <ws2spi.h>

typedef struct {
  DWORD bytesUsed;
  DWORD bytesMax;
  BYTE* buffer;
  BYTE* bufendptr; // Pointer to the first "unused" byte
  BOOL bufok; // FALSE if on mswBuffer-Function fails
} MSW_BUFFER, *PMSW_BUFFER;

void
mswBufferInit(
  _Out_ PMSW_BUFFER mswBuf,
  _In_ BYTE* buffer,
  _In_ DWORD bufferSize);

BOOL
mswBufferCheck(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ DWORD count);

BOOL
mswBufferIncUsed(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ DWORD count);

BYTE*
mswBufferEndPtr(
  _Inout_ PMSW_BUFFER mswBuf);

BOOL
mswBufferAppend(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ void *dataToAppend,
  _In_ DWORD dataSize);

BOOL
mswBufferAppendStrA(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ char* str);

BOOL
mswBufferAppendStrW(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ WCHAR* str);

BOOL
mswBufferAppendPtr(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ void* ptr);

BOOL
mswBufferAppendLst(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ void **lst,
  _In_ DWORD itemByteLength,
  _In_opt_ int deltaofs);

BOOL
mswBufferAppendStrLstA(
  _Inout_ PMSW_BUFFER mswBuf,
  _In_ void **lst,
  _In_opt_ int ptrofs);

BOOL
mswBufferAppendBlob_Hostent(
  _Inout_ PMSW_BUFFER mswBuf,
  _Inout_ LPWSAQUERYSETW lpRes,
    _In_ char** hostAliasesA,
    _In_ char* hostnameA,
  _In_ DWORD ip4addr);

BOOL
mswBufferAppendBlob_Servent(
  _Inout_ PMSW_BUFFER mswBuf,
  _Inout_ LPWSAQUERYSETW lpRes,
  _In_ char* serviceNameA,
  _In_ char** serviceAliasesA,
  _In_ char* protocolNameA,
  _In_ WORD port);

BOOL
mswBufferAppendAddr_AddrInfoW(
  _Inout_ PMSW_BUFFER mswBuf,
  _Inout_ LPWSAQUERYSETW lpRes,
  _In_ DWORD ip4addr);

WCHAR*
StrA2WHeapAlloc(
  _In_opt_ HANDLE hHeap,
  _In_ char* aStr);

char*
StrW2AHeapAlloc(
  _In_opt_ HANDLE hHeap,
  _In_ WCHAR* wStr);

WCHAR*
StrCpyHeapAllocW(
  _In_opt_ HANDLE hHeap,
  _In_ WCHAR* wStr);

char*
StrCpyHeapAllocA(
  _In_opt_ HANDLE hHeap,
  _In_ char* aStr);

/* strary:
   ptr1 ... ptrn \0
   data1 ... datan
*/
char**
StrAryCpyHeapAllocA(
  _In_opt_ HANDLE hHeap,
  _In_ char** aStrAry);

/* strary:
   ptr1 ... ptrn \0
   data1 ... datan
*/
char**
StrAryCpyHeapAllocWToA(
  _In_opt_ HANDLE hHeap,
  _In_ WCHAR** aStrAry);

#endif // _MSWHELPER_H
