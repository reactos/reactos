
#ifndef __WIN32K_PRINT_H
#define __WIN32K_PRINT_H

INT  W32kAbortDoc(HDC  hDC);

INT  W32kEndDoc(HDC  hDC);

INT  W32kEndPage(HDC  hDC);

INT  W32kEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData);

INT  W32kExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData);

INT  W32kSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc);

INT  W32kStartDoc(HDC  hDC,
                  CONST PDOCINFO  di);

INT  W32kStartPage(HDC  hDC);

#endif
