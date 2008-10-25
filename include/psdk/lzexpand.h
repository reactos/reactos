#ifndef _LZEXPAND_H
#define _LZEXPAND_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define LZERROR_BADINHANDLE	(-1)
#define LZERROR_BADOUTHANDLE	(-2)
#define LZERROR_READ	(-3)
#define LZERROR_WRITE	(-4)
#define LZERROR_GLOBALLOC	(-5)
#define LZERROR_GLOBLOCK	(-6)
#define LZERROR_BADVALUE	(-7)
#define LZERROR_UNKNOWNALG	(-8)
LONG WINAPI CopyLZFile(INT,INT);
INT WINAPI GetExpandedNameA(LPSTR,LPSTR);
INT WINAPI GetExpandedNameW(LPWSTR,LPWSTR);
VOID APIENTRY LZClose(INT);
LONG APIENTRY LZCopy(INT,INT);
VOID WINAPI LZDone(VOID);
INT WINAPI LZInit(INT);
INT WINAPI LZOpenFileA(LPSTR,LPOFSTRUCT,WORD);
INT WINAPI LZOpenFileW(LPWSTR,LPOFSTRUCT,WORD);
INT WINAPI LZRead(INT,LPSTR,INT);
LONG WINAPI LZSeek(INT,LONG,INT);
INT WINAPI LZStart(VOID);
#ifdef UNICODE
#define GetExpandedName GetExpandedNameW
#define LZOpenFile  LZOpenFileW
#else
#define GetExpandedName GetExpandedNameA
#define LZOpenFile  LZOpenFileA
#endif
#ifdef __cplusplus
}
#endif
#endif
