
#ifndef __WIN32K_PRINT_H
#define __WIN32K_PRINT_H

INT
STDCALL
NtGdiAbortDoc(HDC  hDC);

INT
STDCALL
NtGdiEndDoc(HDC  hDC);

INT
STDCALL
NtGdiEndPage(HDC  hDC);

INT
STDCALL
NtGdiEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData);

INT
STDCALL
NtGdiExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData);

INT
STDCALL
NtGdiSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc);

INT
STDCALL
NtGdiStartDoc(HDC  hDC,
                  CONST LPDOCINFOW  di);

INT
STDCALL
NtGdiStartPage(HDC  hDC);

#endif
