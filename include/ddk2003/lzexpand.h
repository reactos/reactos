
#ifndef _LZEXPAND_
#define _LZEXPAND_

#ifdef __cplusplus
extern "C" {
#endif

#define LZERROR_BADINHANDLE     (-1)
#define LZERROR_BADOUTHANDLE    (-2)
#define LZERROR_READ            (-3)
#define LZERROR_WRITE           (-4)
#define LZERROR_GLOBALLOC       (-5)
#define LZERROR_GLOBLOCK        (-6)
#define LZERROR_BADVALUE        (-7)
#define LZERROR_UNKNOWNALG      (-8)


LONG
APIENTRY
LZCopy(
    INT,
    INT);

LONG
APIENTRY
CopyLZFile(
    INT,
    INT);

INT
APIENTRY
GetExpandedNameA(
    LPSTR,
    LPSTR);

INT
APIENTRY
GetExpandedNameW(
    LPWSTR,
    LPWSTR);

VOID
APIENTRY
LZClose(INT);

INT
APIENTRY
LZInit(INT);

INT
APIENTRY
LZStart(VOID);

VOID
APIENTRY
LZDone(VOID);


INT
APIENTRY
LZOpenFileA(
    LPSTR,
    LPOFSTRUCT,
    WORD);

INT
APIENTRY
LZOpenFileW(
    LPWSTR,
    LPOFSTRUCT,
    WORD);

INT
APIENTRY
LZRead(
    INT,
    LPSTR,
    INT);

LONG
APIENTRY
LZSeek(
    INT,
    LONG,
    INT);

#ifdef UNICODE
#define GetExpandedName  GetExpandedNameW
#define LZOpenFile  LZOpenFileW
#else
#define GetExpandedName  GetExpandedNameA
#define LZOpenFile  LZOpenFileA
#endif

#ifdef __cplusplus
}
#endif


#endif

