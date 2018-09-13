/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWNLS.H
 *  16-bit IME API argument structures
 *
 *  History:
 *  Created 16-Apl-1996 by MSKK
 *
--*/

/* IME API IDs
 */
#define FUN_SENDIMEMESSAGE		6
#define FUN_SENDIMEMESSAGEEX		7

#define	FUN_WINNLSGETIMEHOTKEY		15
#define	FUN_WINNLSENABLEIME		16
#define	FUN_WINNLSGETENABLESTATUS	18

#define	FUN_IMPQUERYIME			22
#define	FUN_IMPGETIME			23
#define	FUN_IMPSETIME			24


#if 0 // NT does not support the following APIs
#define FUN_INQUIREWINNLS		1
#define	FUN_HOOKKEYBOARDMESSAGE		2
#define FUN_CONTROLIMEMESSAGE       	8	// for Taiwan, Prc
#define	FUN_WINNLSSETKEYBOARDHOOK	10
#define	FUN_WINNLSSETIMEHANDLE		11
#define	FUN_WINNLSSETIMESTATUS		12
#define	FUN_WINNLSSETIMEHOTKEY		14
#define	FUN_WINNLSGETKEYSTATE		17
#define	FUN_WINNLSSETKEYSTATE		19
#define	FUN_IMPADDIME			20
#define	FUN_IMPDELETEIME		21
#define	FUN_IMPMODIFYIME		25
#define	FUN_IMPGETDEFAULTIME		26
#define	FUN_IMPSETDEFAULTIME		27
#define	FUN_WINNLSSENDSTRING		30
#define	FUN_WINNLSPOSTAPPMESSAGE	31
#define	FUN_WINNLSSENDAPPMESSAGE	32
#define FUN_WINNLSQUERYIMEINFO      	40     // for Taiwan, Prc
#endif // 0


/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _SENDIMEMESSAGE16 { /* wn6 */
	DWORD	lParam;
	HWND16	hwnd;
} SENDIMEMESSAGE16;
typedef SENDIMEMESSAGE16 UNALIGNED *PSENDIMEMESSAGE16;

typedef struct _SENDIMEMESSAGEEX16 { /* wn7 */
	DWORD	lParam;
	HWND16	hwnd;
} SENDIMEMESSAGEEX16;
typedef SENDIMEMESSAGEEX16 UNALIGNED *PSENDIMEMESSAGEEX16;

typedef struct _WINNLSGETIMEHOTKEY16 { /* wn15 */
	HWND16	hwnd;
} WINNLSGETIMEHOTKEY16;
typedef WINNLSGETIMEHOTKEY16 UNALIGNED *PWINNLSGETIMEHOTKEY16;

typedef struct _WINNLSENABLEIME16 { /* wn16 */
	WORD	fEnabled;
	HWND16	hwnd;
} WINNLSENABLEIME16;
typedef WINNLSENABLEIME16 UNALIGNED *PWINNLSENABLEIME16;

typedef struct _WINNLSGETENABLESTATUS16 { /* wn 18 */
	HWND16	hwnd;
} WINNLSGETENABLESTATUS16;
typedef WINNLSGETENABLESTATUS16 UNALIGNED *PWINNLSGETENABLESTATUS16;

typedef struct _IMPQUERYIME16 { /* wn22 */
        VPVOID  lpIMEPro;
} IMPQUERYIME16;
typedef IMPQUERYIME16 UNALIGNED *PIMPQUERYIME16;

typedef struct _IMPGETIME16 { /* wn23 */
        VPVOID  lpIMEPro;
	HWND16	hwnd;
} IMPGETIME16;
typedef IMPGETIME16 UNALIGNED *PIMPGETIME16;

typedef struct _IMPSETIME16 { /* wn24 */
        VPVOID  lpIMEPro;
	HWND16	hwnd;
} IMPSETIME16;
typedef IMPSETIME16 UNALIGNED *PIMPSETIME16;






#if 0 // NT does not support the following APIs
typedef struct _SETKEYBOARDHOOK16 { /* wn10 */
	WORD    flag;
} SETKEYBOARDHOOK16;
typedef SETKEYBOARDHOOK16 UNALIGNED *PSETKEYBOARDHOOK16;

typedef struct _SETIMEHANDLE16 { /* wn11 */
	VPSTR	lpszName;
	HWND16	hwnd;
} SETIMEHANDLE16;
typedef SETIMEHANDLE16 UNALIGNED *PSETIMEHANDLE16;

typedef struct _SETIMESTATUS16 { /* wn12 */
	WORD	status;
	HWND16	hwnd;
} SETIMESTATUS16;
typedef SETIMESTATUS16 UNALIGNED *PSETIMESTATUS16;

typedef struct _SETIMEHOTKEY16 { /* wn14 */
	WORD	key;
	HWND16	hwnd;
} SETIMEHOTKEY16;
typedef SETIMEHOTKEY16 UNALIGNED *PSETIMEHOTKEY16;
#endif // 0

typedef struct _IMESTRUCT16 { /* imest */
	WORD	fnc;
	WORD	wParam;
	WORD	wCount;
	WORD	dchSource;
	WORD	dchDest;
	LONG	lParam1;
	LONG	lParam2;
	LONG	lParam3;
} IMESTRUCT16;
typedef IMESTRUCT16 UNALIGNED *PIMESTRUCT16;

#if 0
typedef struct _SETKEYSTATE16 { /* wn19 */
	HWND16	hwnd;
	WORD	wState;
} SETKEYSTATE16;
typedef SETKEYSTATE16 UNALIGNED *PSETKEYSTATE16;
#endif // 0

typedef struct _DATETIME16 { /* dt */
    WORD	year;
    WORD	month;
    WORD	day;
    WORD	hour;
    WORD	min;
    WORD	sec;
} DATETIME16;

typedef struct _IMEPRO16 { /* pro */
	HWND16	hWnd;
	DATETIME16 InstDate;
	WORD	wVersion;
	BYTE	szDescription[50];
	BYTE	szName[80];
	BYTE	szOptions[30];
} IMEPRO16;
typedef IMEPRO16 UNALIGNED *PIMEPRO16;

typedef struct _UNDETERMINESTRUCT16 { /* ud16 */
    DWORD    dwSize;
    INT16     uDefIMESize;
    INT16     uDefIMEPos;
    INT16     uUndetTextLen;
    INT16     uUndetTextPos;
    INT16     uUndetAttrPos;
    INT16     uCursorPos;
    INT16     uDeltaStart;
    INT16     uDetermineTextLen;
    INT16     uDetermineTextPos;
    INT16     uDetermineDelimPos;
    INT16     uYomiTextLen;
    INT16     uYomiTextPos;
    INT16     uYomiDelimPos;
} UNDETERMINESTRUCT16;
typedef UNDETERMINESTRUCT16 UNALIGNED *PUNDETERMINESTRUCT16;

typedef struct _STRINGEXSTRUCT16 { /* sx16 */
    DWORD    dwSize;
    INT16     uDeterminePos;
    INT16     uDetermineDelimPos;
    INT16     uYomiPos;
    INT16     uYomiDelimPos;
} STRINGEXSTRUCT16;
typedef STRINGEXSTRUCT16 UNALIGNED *PSTRINGEXSTRUCT16;




/////////// tmp /////////
#if 0
#define IR_DBCSCHAR             0x160   /* New for 3.1 */
#define IR_UNDETERMINE          0x170   /* New for 3.1 */
#define IR_STRINGEX             0x180   /* New for 3.1 */

typedef struct UNDETERMINESTRUCT32 { /* ud32 */
    DWORD    dwSize;
    UINT     uDefIMESize;
    UINT     uDefIMEPos;
    UINT     uUndetTextLen;
    UINT     uUndetTextPos;
    UINT     uUndetAttrPos;
    UINT     uCursorPos;
    UINT     uDeltaStart;
    UINT     uDetermineTextLen;
    UINT     uDetermineTextPos;
    UINT     uDetermineDelimPos;
    UINT     uYomiTextLen;
    UINT     uYomiTextPos;
    UINT     uYomiDelimPos;
} UNDETERMINESTRUCT32;
typedef UNDETERMINESTRUCT32 UNALIGNED *PUNDETERMINESTRUCT32;

typedef struct STRINGEXSTRUCT32 { /* sx32 */
    DWORD    dwSize;
    UINT     uDeterminePos;
    UINT     uDetermineDelimPos;
    UINT     uYomiPos;
    UINT     uYomiDelimPos;
} STRINGEXSTRUCT32;
typedef STRINGEXSTRUCT32 UNALIGNED *PSTRINGEXSTRUCT32;
#endif // 0
//////// end of tmp ///////////

/* XLATOFF */
#pragma pack()
/* XLATON */
