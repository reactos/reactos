#pragma once

VOID FASTCALL ValidateReadPtr(IN LPCVOID lp, IN UINT_PTR ucb);
VOID FASTCALL ValidateWritePtr(IN LPVOID lp, IN UINT_PTR ucb);
VOID FASTCALL ValidateStringPtrA(IN LPCSTR lpsz);
VOID FASTCALL ValidateStringPtrW(IN LPCWSTR lpsz);
