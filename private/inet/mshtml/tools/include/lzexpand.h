/*
** lzdos.h - Public interface to LZEXP?.LIB.
*/

#ifndef _LZEXPAND_
#define _LZEXPAND_

#ifdef __cplusplus
extern "C" {
#endif

/*
** Error Return Codes
*/

#define LZERROR_BADINHANDLE   (-1)  /* invalid input handle */
#define LZERROR_BADOUTHANDLE  (-2)  /* invalid output handle */
#define LZERROR_READ          (-3)  /* corrupt compressed file format */
#define LZERROR_WRITE         (-4)  /* out of space for output file */
#define LZERROR_GLOBALLOC     (-5)  /* insufficient memory for LZFile struct */
#define LZERROR_GLOBLOCK      (-6)  /* bad global handle */
#define LZERROR_BADVALUE      (-7)  /* input parameter out of acceptable range*/
#define LZERROR_UNKNOWNALG    (-8)  /* compression algorithm not recognized */


/*
** Prototypes
*/

INT
APIENTRY
LZStart(
	VOID
	);

VOID
APIENTRY
LZDone(
	VOID
	);

LONG
APIENTRY
CopyLZFile(
	INT,
	INT
	);

LONG
APIENTRY
LZCopy(
	INT,
	INT
	);

INT
APIENTRY
LZInit(
	INT
	);

INT
APIENTRY
GetExpandedNameA(
	LPSTR,
	LPSTR
	);
INT
APIENTRY
GetExpandedNameW(
	LPWSTR,
	LPWSTR
	);
#ifdef UNICODE
#define GetExpandedName  GetExpandedNameW
#else
#define GetExpandedName  GetExpandedNameA
#endif // !UNICODE

INT
APIENTRY
LZOpenFileA(
	LPSTR,
	LPOFSTRUCT,
	WORD
	);
INT
APIENTRY
LZOpenFileW(
	LPWSTR,
	LPOFSTRUCT,
	WORD
	);
#ifdef UNICODE
#define LZOpenFile  LZOpenFileW
#else
#define LZOpenFile  LZOpenFileA
#endif // !UNICODE

LONG
APIENTRY
LZSeek(
	INT,
	LONG,
	INT
	);

INT
APIENTRY
LZRead(
	INT,
	LPSTR,
	INT
	);

VOID
APIENTRY
LZClose(
	INT
	);

#ifdef __cplusplus
}
#endif


#endif // _LZEXPAND_
