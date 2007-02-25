// strpool.h

#ifndef __USER32_INTERNAL_STRING_POOL_H
#define __USER32_INTERNAL_STRING_POOL_H

extern HANDLE hProcessHeap;

PVOID
HEAP_alloc ( DWORD len );

VOID
HEAP_free ( LPVOID memory );

LPWSTR
HEAP_strdupW ( LPCWSTR src, DWORD len );

NTSTATUS
HEAP_strcpyWtoA ( LPSTR lpszA, LPCWSTR lpszW, DWORD len );

NTSTATUS
HEAP_strcpyAtoW ( LPWSTR lpszW, LPCSTR lpszA, DWORD len );

NTSTATUS
HEAP_strdupWtoA ( LPSTR* ppszA, LPCWSTR lpszW, DWORD len );

NTSTATUS
HEAP_strdupAtoW ( LPWSTR* ppszW, LPCSTR lpszA, UINT* NewLen );

char*
heap_string_poolA ( const wchar_t* pstrW, DWORD length );

wchar_t*
heap_string_poolW ( const wchar_t* pstrWSrc, DWORD length );

#endif//__USER32_INTERNAL_STRING_POOL_H
