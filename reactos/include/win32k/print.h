
#ifndef __WIN32K_PRINT_H
#define __WIN32K_PRINT_H

INT
STDCALL
W32kAbortDoc(HDC  hDC);

INT
STDCALL
W32kEndDoc(HDC  hDC);

INT
STDCALL
W32kEndPage(HDC  hDC);

INT
STDCALL
W32kEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData);

INT
STDCALL
W32kExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData);

INT
STDCALL
W32kSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc);

INT
STDCALL
W32kStartDoc(HDC  hDC,
                  CONST LPDOCINFO  di);

INT
STDCALL
W32kStartPage(HDC  hDC);

#endif
