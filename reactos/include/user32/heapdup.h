
int lstrcpynWtoA( LPSTR ptr1, LPWSTR ptr2, int n );
int lstrcpynAtoW( LPWSTR ptr1, LPSTR ptr2, int n );
int lstrcpyWtoA( LPSTR ptr1, LPWSTR ptr2 );
int lstrcpyAtoW( LPWSTR ptr1, LPSTR ptr2 );
int lpstrncpyA( LPSTR ptr1,LPSTR ptr2, int nMaxCount);
int lpstrncpyW( LPWSTR ptr1,LPWSTR ptr2, int nMaxCount);
LPVOID HEAP_strdupAtoW(HANDLE  hHeap,DWORD  dwFlags,	LPCSTR lpszAsciiString );
LPVOID HEAP_strdupWtoA(HANDLE  hHeap,DWORD  dwFlags,	LPCWSTR lpszUnicodeString );
LPSTR HEAP_strdupA(HANDLE  hHeap,DWORD  dwFlags, LPCSTR ptr);
LPWSTR HEAP_strdupW(HANDLE  hHeap,DWORD  dwFlags, LPCWSTR ptr);
int HEAP_memset( void *d,int c ,int count);