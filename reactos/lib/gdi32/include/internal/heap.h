// heap.h

#ifndef GDI32_INTERNAL_HEAP_H
#define GDI32_INTERNAL_HEAP_H

extern HANDLE hProcessHeap;

PVOID
HEAP_alloc ( DWORD len );

NTSTATUS
HEAP_strdupA2W ( LPWSTR* ppszW, LPCSTR lpszA );

VOID
HEAP_free ( LPVOID memory );

#endif//GDI32_INTERNAL_HEAP_H
