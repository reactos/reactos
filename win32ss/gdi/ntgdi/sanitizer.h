#pragma once

BOOL WINAPI IsBadReadPtr(IN LPCVOID lp, IN UINT_PTR ucb);
BOOL NTAPI IsBadWritePtr(IN LPVOID lp, IN UINT_PTR ucb);
BOOL NTAPI IsBadCodePtr(FARPROC lpfn);
BOOL NTAPI IsBadStringPtrA(IN LPCSTR lpsz, IN UINT_PTR ucchMax);
BOOL NTAPI IsBadStringPtrW(IN LPCWSTR lpsz, IN UINT_PTR ucchMax);
