// strpool.c

#include <windows.h>
#include <ddk/ntddk.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <strpool.h>
#include <string.h>

typedef struct tagHEAP_STRING_POOLA
{
	char* data;
	struct tagHEAP_STRING_POOLA* next;
} HEAP_STRING_POOLA, *PHEAP_STRING_POOLA;

typedef struct tagHEAP_STRING_POOLW
{
	wchar_t* data;
	struct tagHEAP_STRING_POOLW* next;
} HEAP_STRING_POOLW, *PHEAP_STRING_POOLW;

PHEAP_STRING_POOLA heap_string_Apool = NULL;

PHEAP_STRING_POOLW heap_string_Wpool = NULL;

HANDLE hProcessHeap = NULL;

PVOID
HEAP_alloc ( DWORD len )
{
  return RtlAllocateHeap ( hProcessHeap, 0, len );
}

VOID
HEAP_free ( LPVOID memory )
{
  RtlFreeHeap ( hProcessHeap, 0, memory );
}

LPWSTR
HEAP_strdupW ( LPCWSTR src, DWORD len )
{
  LPWSTR dst;

  if ( !src )
    return NULL;

  dst = HEAP_alloc ( (len+1) * sizeof(WCHAR) );
  if ( !dst )
    return NULL;
  memcpy ( dst, src, (len+1)*sizeof(WCHAR) );
  return dst;
}

NTSTATUS
HEAP_strcpyWtoA ( LPSTR lpszA, LPCWSTR lpszW, DWORD len )
{
  NTSTATUS Status =
    RtlUnicodeToMultiByteN ( lpszA, len, NULL, (LPWSTR)lpszW, len*sizeof(WCHAR) );
  lpszA[len] = '\0';
  return Status;
}

NTSTATUS
HEAP_strcpyAtoW ( LPWSTR lpszW, LPCSTR lpszA, DWORD len )
{
  NTSTATUS Status =
    RtlMultiByteToUnicodeN ( lpszW, len*sizeof(WCHAR), NULL, (LPSTR)lpszA, len ); 
  lpszW[len] = L'\0';
  return Status;
}

NTSTATUS
HEAP_strdupWtoA ( LPSTR* ppszA, LPCWSTR lpszW, DWORD len )
{
  *ppszA = NULL;

  if ( !lpszW )
    return STATUS_SUCCESS;

  *ppszA = HEAP_alloc ( len + 1 );

  if ( !*ppszA )
    return STATUS_NO_MEMORY;

  return HEAP_strcpyWtoA ( *ppszA, lpszW, len );
}

NTSTATUS
HEAP_strdupAtoW ( LPWSTR* ppszW, LPCSTR lpszA, UINT* NewLen )
{
  ULONG len;

  *ppszW = NULL;

  if ( !lpszA )
    return STATUS_SUCCESS;

  len = lstrlenA ( lpszA );

  *ppszW = HEAP_alloc ( (len+1) * sizeof(WCHAR) );

  if ( !*ppszW )
    return STATUS_NO_MEMORY;

  if ( NewLen ) *NewLen = (UINT)len;

  return HEAP_strcpyAtoW ( *ppszW, lpszA, len );
}

char* heap_string_poolA ( const wchar_t* pstrW, DWORD length )
{
  PHEAP_STRING_POOLA pPoolEntry = heap_string_Apool;
  char* pstrA = NULL;
  HEAP_strdupWtoA ( &pstrA, pstrW, length );
  if ( !pstrA )
    return NULL;
  while ( pPoolEntry )
  {
    if ( !strcmp ( pPoolEntry->data, pstrA ) )
    {
      HEAP_free ( pstrA );
      return pPoolEntry->data;
    }
    pPoolEntry = pPoolEntry->next;
  }
  pPoolEntry = (PHEAP_STRING_POOLA)HEAP_alloc ( sizeof(HEAP_STRING_POOLA) );
  pPoolEntry->data = pstrA;

  // IMHO, synchronization is *not* needed here. This data is process-
  // local, so the only possible contention is among threads. If a
  // conflict does occur, the only problem will be a small memory
  // leak that gets cleared up when the heap is destroyed by the
  // process exiting.
  pPoolEntry->next = heap_string_Apool;
  heap_string_Apool = pPoolEntry;
  return pPoolEntry->data;
}

wchar_t* heap_string_poolW ( const wchar_t* pstrWSrc, DWORD length )
{
  PHEAP_STRING_POOLW pPoolEntry = heap_string_Wpool;
  wchar_t* pstrW = NULL;
  pstrW = HEAP_strdupW ( (LPWSTR)pstrWSrc, length );
  if ( !pstrW )
    return NULL;
  while ( pPoolEntry )
  {
    if ( !wcscmp (pPoolEntry->data, pstrW ) )
    {
      HEAP_free ( pstrW );
      return pPoolEntry->data;
    }
    pPoolEntry = pPoolEntry->next;
  }
  pPoolEntry = (PHEAP_STRING_POOLW)HEAP_alloc ( sizeof(HEAP_STRING_POOLW) );
  pPoolEntry->data = pstrW;

  // IMHO, synchronization is *not* needed here. This data is process-
  // local, so the only possible contention is among threads. If a
  // conflict does occur, the only problem will be a small memory
  // leak that gets cleared up when the heap is destroyed by the
  // process exiting.
  pPoolEntry->next = heap_string_Wpool;
  heap_string_Wpool = pPoolEntry;
  return pPoolEntry->data;
}
