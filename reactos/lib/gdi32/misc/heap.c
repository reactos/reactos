// heap.c

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <debug.h>

// global variables in a dll are process-global
HANDLE hProcessHeap = NULL;

PVOID
HEAP_alloc ( DWORD len )
{
  /* make sure hProcessHeap gets initialized by GdiProcessSetup before we get here */
  assert(hProcessHeap);
  return RtlAllocateHeap ( hProcessHeap, 0, len );
}

NTSTATUS
HEAP_strdupA2W ( LPWSTR* ppszW, LPCSTR lpszA )
{
  ULONG len;
  NTSTATUS Status;

  *ppszW = NULL;
  if ( !lpszA )
    return STATUS_SUCCESS;
  len = lstrlenA(lpszA);

  *ppszW = HEAP_alloc ( (len+1) * sizeof(WCHAR) );
  if ( !*ppszW )
    return STATUS_NO_MEMORY;
  Status = RtlMultiByteToUnicodeN ( *ppszW, len*sizeof(WCHAR), NULL, (PCHAR)lpszA, len );
  (*ppszW)[len] = L'\0';
  return Status;
}


VOID
HEAP_free ( LPVOID memory )
{
  /* make sure hProcessHeap gets initialized by GdiProcessSetup before we get here */
  assert(hProcessHeap);

  RtlFreeHeap ( hProcessHeap, 0, memory );
}
