/*
 *	RICHEDIT.H
 *	
 *	Purpose:
 *		RICHEDIT v2.0 public definitions.  Note that there is additional
 *		functionality available for v2.0 that is not in the original
 *		Windows 95 release.
 *	
 *	Copyright (c) 1985-1996, Microsoft Corporation
 */

#ifndef _RICHEDIT_
#define	_RICHEDIT_

#ifdef _WIN32
#include <pshpack4.h>
#elif !defined(RC_INVOKED)
#pragma pack(4)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* To mimic RichEdit 1.0 behavior for Unicode apps, simply set _RICHEDIT_VER to 0x0100 */
#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER	0x0200
#endif

/*
 *	To make some structures which can be passed between 16 and 32 bit windows
 *	almost compatible, padding is introduced to the 16 bit versions of the
 *	structure.
 */
#ifdef _WIN32
#	define	_WPAD	/##/
#else
#	define	_WPAD	WORD
#endif

#define cchTextLimitDefault 32767

/* Richedit2.0 Window Class. */

#define RICHEDIT_CLASSA		"RichEdit20A"
#define RICHEDIT_CLASS10A	"RICHEDIT"			// Richedit 1.0

#ifndef MACPORT
#define RICHEDIT_CLASSW		L"RichEdit20W"
#else	/*----------------------MACPORT */
#define RICHEDIT_CLASSW		TEXT("RichEdit20W")	/* MACPORT change */
#endif /* MACPORT  */

#if (_RICHEDIT_VER >= 0x0200 )
#ifdef UNICODE
#define RICHEDIT_CLASS		RICHEDIT_CLASSW
#else
#define RICHEDIT_CLASS		RICHEDIT_CLASSA
#endif /* UNICODE */
#else
#define RICHEDIT_CLASS		RICHEDIT_CLASS10A
#endif /* _RICHEDIT_VER >= 0x0200 */

/* RichEdit messages */

#ifndef WM_CONTEXTMENU
#define WM_CONTEXTMENU			0x007B
#endif

#ifndef WM_PRINTCLIENT
#define WM_PRINTCLIENT			0x0318
#endif

#ifndef EM_GETLIMITTEXT
#define EM_GETLIMITTEXT			(WM_USER + 37)
#endif

#ifndef EM_POSFROMCHAR	
#define EM_POSFROMCHAR			(WM_USER + 38)
#define EM_CHARFROMPOS			(WM_USER + 39)
#endif

#ifndef EM_SCROLLCARET
#define EM_SCROLLCARET			(WM_USER + 49)
#endif
#define EM_CANPASTE				(WM_USER + 50)
#define EM_DISPLAYBAND			(WM_USER + 51)
#define EM_EXGETSEL				(WM_USER + 52)
#define EM_EXLIMITTEXT			(WM_USER + 53)
#define EM_EXLINEFROMCHAR		(WM_USER + 54)
#define EM_EXSETSEL				(WM_USER + 55)
#define EM_FINDTEXT				(WM_USER + 56)
#define EM_FORMATRANGE			(WM_USER + 57)
#define EM_GETCHARFORMAT		(WM_USER + 58)
#define EM_GETEVENTMASK			(WM_USER + 59)
#define EM_GETOLEINTERFACE		(WM_USER + 60)
#define EM_GETPARAFORMAT		(WM_USER + 61)
#define EM_GETSELTEXT			(WM_USER + 62)
#define EM_HIDESELECTION		(WM_USER + 63)
#define EM_PASTESPECIAL			(WM_USER + 64)
#define EM_REQUESTRESIZE		(WM_USER + 65)
#define EM_SELECTIONTYPE		(WM_USER + 66)
#define EM_SETBKGNDCOLOR		(WM_USER + 67)
#define EM_SETCHARFORMAT		(WM_USER + 68)
#define EM_SETEVENTMASK			(WM_USER + 69)
#define EM_SETOLECALLBACK		(WM_USER + 70)
#define EM_SETPARAFORMAT		(WM_USER + 71)
#define EM_SETTARGETDEVICE		(WM_USER + 72)
#define EM_STREAMIN				(WM_USER + 73)
#define EM_STREAMOUT			(WM_USER + 74)
#define EM_GETTEXTRANGE			(WM_USER + 75)
#define EM_FINDWORDBREAK		(WM_USER + 76)
#define EM_SETOPTIONS			(WM_USER + 77)
#define EM_GETOPTIONS			(WM_USER + 78)
#define EM_FINDTEXTEX			(WM_USER + 79)
#ifdef _WIN32
#define EM_GETWORDBREAKPROCEX	(WM_USER + 80)
#define EM_SETWORDBREAKPROCEX	(WM_USER + 81)
#endif

/* Richedit v2.0 messages */
#define	EM_SETUNDOLIMIT			(WM_USER + 82)
#define EM_REDO					(WM_USER + 84)
#define EM_CANREDO				(WM_USER + 85)
#define EM_GETUNDONAME			(WM_USER + 86)
#define EM_GETREDONAME			(WM_USER + 87)
#define EM_STOPGROUPTYPING		(WM_USER + 88)

#define EM_SETTEXTMODE			(WM_USER + 89)
#define EM_GETTEXTMODE			(WM_USER + 90)

/* enum for use with EM_GET/SETTEXTMODE */
typedef enum tagTextMode
{
	TM_PLAINTEXT			= 1,
	TM_RICHTEXT				= 2,	/* default behavior */
	TM_SINGLELEVELUNDO		= 4,
	TM_MULTILEVELUNDO		= 8,	/* default behavior */
	TM_SINGLECODEPAGE		= 16,
	TM_MULTICODEPAGE		= 32	/* default behavior */
} TEXTMODE;

#define EM_AUTOURLDETECT		(WM_USER + 91)
#define EM_GETAUTOURLDETECT		(WM_USER + 92)
#define EM_SETPALETTE			(WM_USER + 93)
#define EM_GETTEXTEX			(WM_USER + 94)
#define EM_GETTEXTLENGTHEX		(WM_USER + 95)

/* Far East specific messages */
#define EM_SETPUNCTUATION		(WM_USER + 100)
#define EM_GETPUNCTUATION		(WM_USER + 101)
#define EM_SETWORDWRAPMODE		(WM_USER + 102)
#define EM_GETWORDWRAPMODE		(WM_USER + 103)
#define EM_SETIMECOLOR			(WM_USER + 104)
#define EM_GETIMECOLOR			(WM_USER + 105)
#define EM_SETIMEOPTIONS		(WM_USER + 106)
#define EM_GETIMEOPTIONS		(WM_USER + 107)
#define EM_CONVPOSITION 		(WM_USER + 108)

#define EM_SETLANGOPTIONS		(WM_USER + 120)
#define EM_GETLANGOPTIONS		(WM_USER + 121)
#define EM_GETIMECOMPMODE		(WM_USER + 122)

/* Options for EM_SETLANGOPTIONS and EM_GETLANGOPTIONS */
#define IMF_AUTOKEYBOARD		0x0001
#define IMF_AUTOFONT			0x0002
#define IMF_IMECANCELCOMPLETE	0x0004	// high completes the comp string when aborting, low cancels.
#define IMF_IMEALWAYSSENDNOTIFY 0x0008

/* Values for EM_GETIMECOMPMODE */
#define ICM_NOTOPEN				0x0000
#define ICM_LEVEL3				0x0001
#define ICM_LEVEL2				0x0002
#define ICM_LEVEL2_5			0x0003
#define ICM_LEVEL2_SUI			0x0004

/* New notifications */

#define EN_MSGFILTER			0x0700
#define EN_REQUESTRESIZE		0x0701
#define EN_SELCHANGE			0x0702
#define EN_DROPFILES			0x0703
#define EN_PROTECTED			0x0704
#define EN_CORRECTTEXT			0x0705			/* PenWin specific */
#define EN_STOPNOUNDO			0x0706
#define EN_IMECHANGE			0x0707			/* Far East specific */
#define EN_SAVECLIPBOARD		0x0708
#define EN_OLEOPFAILED			0x0709
#define EN_OBJECTPOSITIONS		0x070a
#define EN_LINK					0x070b
#define EN_DRAGDROPDONE			0x070c

/* Event notification masks */

#define ENM_NONE				0x00000000
#define ENM_CHANGE				0x00000001
#define ENM_UPDATE				0x00000002
#define ENM_SCROLL				0x00000004
#define ENM_KEYEVENTS			0x00010000
#define ENM_MOUSEEVENTS			0x00020000
#define ENM_REQUESTRESIZE		0x00040000
#define ENM_SELCHANGE			0x00080000
#define ENM_DROPFILES			0x00100000
#define ENM_PROTECTED			0x00200000
#define ENM_CORRECTTEXT			0x00400000		/* PenWin specific */
#define ENM_SCROLLEVENTS		0x00000008
#define ENM_DRAGDROPDONE		0x00000010

/* Far East specific notification mask */
#define ENM_IMECHANGE			0x00800000		/* unused by RE2.0 */
#define ENM_LANGCHANGE			0x01000000
#define ENM_OBJECTPOSITIONS		0x02000000
#define ENM_LINK				0x04000000

/* New edit control styles */

#define ES_SAVESEL				0x00008000
#define ES_SUNKEN				0x00004000
#define ES_DISABLENOSCROLL		0x00002000
/* same as WS_MAXIMIZE, but that doesn't make sense so we re-use the value */
#define ES_SELECTIONBAR			0x01000000
/* same as ES_UPPERCASE, but re-used to completely disable OLE drag'n'drop */
#define ES_NOOLEDRAGDROP		0x00000008

/* New edit control extended style */
#ifdef	_WIN32
#define ES_EX_NOCALLOLEINIT		0x01000000
#endif	

/* These flags are used in FE Windows */
#define ES_VERTICAL				0x00400000
#define	ES_NOIME				0x00080000
#define ES_SELFIME				0x00040000

/* Edit control options */
#define ECO_AUTOWORDSELECTION	0x00000001
#define ECO_AUTOVSCROLL			0x00000040
#define ECO_AUTOHSCROLL			0x00000080
#define ECO_NOHIDESEL			0x00000100
#define ECO_READONLY			0x00000800
#define ECO_WANTRETURN			0x00001000
#define ECO_SAVESEL				0x00008000
#define ECO_SELECTIONBAR		0x01000000
#define ECO_VERTICAL			0x00400000		/* FE specific */


/* ECO operations */
#define ECOOP_SET				0x0001
#define ECOOP_OR				0x0002
#define ECOOP_AND				0x0003
#define ECOOP_XOR				0x0004

/* new word break function actions */
#define WB_CLASSIFY			3
#define WB_MOVEWORDLEFT		4
#define WB_MOVEWORDRIGHT	5
#define WB_LEFTBREAK		6
#define WB_RIGHTBREAK		7

/* Far East specific flags */
#define WB_MOVEWORDPREV		4
#define WB_MOVEWORDNEXT		5
#define WB_PREVBREAK		6
#define WB_NEXTBREAK		7

#define PC_FOLLOWING		1
#define	PC_LEADING			2
#define	PC_OVERFLOW			3
#define	PC_DELIMITER		4
#define WBF_WORDWRAP		0x010
#define WBF_WORDBREAK		0x020
#define	WBF_OVERFLOW		0x040	
#define WBF_LEVEL1			0x080
#define	WBF_LEVEL2			0x100
#define	WBF_CUSTOM			0x200

/* Far East specific flags */
#define IMF_FORCENONE           0x0001
#define IMF_FORCEENABLE         0x0002
#define IMF_FORCEDISABLE        0x0004
#define IMF_CLOSESTATUSWINDOW   0x0008
#define IMF_VERTICAL            0x0020
#define IMF_FORCEACTIVE         0x0040
#define IMF_FORCEINACTIVE       0x0080
#define IMF_FORCEREMEMBER       0x0100
#define IMF_MULTIPLEEDIT        0x0400

/* Word break flags (used with WB_CLASSIFY) */
#define WBF_CLASS			((BYTE) 0x0F)
#define WBF_ISWHITE			((BYTE) 0x10)
#define WBF_BREAKLINE		((BYTE) 0x20)
#define WBF_BREAKAFTER		((BYTE) 0x40)


/* new data types */

#ifdef _WIN32
/* extended edit word break proc (character set aware) */
typedef LONG (*EDITWORDBREAKPROCEX)(char *pchText, LONG cchText, BYTE bCharSet, INT action);
#endif

/* all character format measurements are in twips */
typedef struct _charformat
{
	UINT		cbSize;
	_WPAD		_wPad1;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	char		szFaceName[LF_FACESIZE];
	_WPAD		_wPad2;
} CHARFORMATA;

typedef struct _charformatw
{
	UINT		cbSize;
	_WPAD		_wPad1;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	WCHAR		szFaceName[LF_FACESIZE];
	_WPAD		_wPad2;
} CHARFORMATW;

#if (_RICHEDIT_VER >= 0x0200)
#ifdef UNICODE
#define CHARFORMAT CHARFORMATW
#else
#define CHARFORMAT CHARFORMATA
#endif /* UNICODE */
#else
#define CHARFORMAT CHARFORMATA
#endif /* _RICHEDIT_VER >= 0x0200 */

/* CHARFORMAT masks */
#define CFM_BOLD		0x00000001
#define CFM_ITALIC		0x00000002
#define CFM_UNDERLINE	0x00000004
#define CFM_STRIKEOUT	0x00000008
#define CFM_PROTECTED	0x00000010
#define CFM_LINK		0x00000020		/* Exchange hyperlink extension */
#define CFM_SIZE		0x80000000
#define CFM_COLOR		0x40000000
#define CFM_FACE		0x20000000
#define CFM_OFFSET		0x10000000
#define CFM_CHARSET		0x08000000

/* CHARFORMAT effects */
#define CFE_BOLD		0x0001
#define CFE_ITALIC		0x0002
#define CFE_UNDERLINE	0x0004
#define CFE_STRIKEOUT	0x0008
#define CFE_PROTECTED	0x0010
#define CFE_LINK		0x0020
#define CFE_AUTOCOLOR	0x40000000		/* NOTE: this corresponds to */
										/* CFM_COLOR, which controls it */
#define yHeightCharPtsMost 1638

/* EM_SETCHARFORMAT wParam masks */
#define SCF_SELECTION	0x0001
#define SCF_WORD		0x0002
#define SCF_DEFAULT		0x0000		// set the default charformat or paraformat
#define SCF_ALL			0x0004		// not valid with SCF_SELECTION or SCF_WORD
#define SCF_USEUIRULES	0x0008		// modifier for SCF_SELECTION; says that
									// the format came from a toolbar, etc. and
									// therefore UI formatting rules should be
									// used instead of strictly formatting the
									// selection.


typedef struct _charrange
{
	LONG	cpMin;
	LONG	cpMax;
} CHARRANGE;

typedef struct _textrange
{
	CHARRANGE chrg;
	LPSTR lpstrText;	/* allocated by caller, zero terminated by RichEdit */
} TEXTRANGEA;

typedef struct _textrangew
{
	CHARRANGE chrg;
	LPWSTR lpstrText;	/* allocated by caller, zero terminated by RichEdit */
} TEXTRANGEW;

#if (_RICHEDIT_VER >= 0x0200)
#ifdef UNICODE
#define TEXTRANGE 	TEXTRANGEW
#else
#define TEXTRANGE	TEXTRANGEA
#endif /* UNICODE */
#else
#define TEXTRANGE	TEXTRANGEA
#endif /* _RICHEDIT_VER >= 0x0200 */


typedef DWORD (CALLBACK *EDITSTREAMCALLBACK)(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

typedef struct _editstream
{
	DWORD dwCookie;		/* user value passed to callback as first parameter */
	DWORD dwError;		/* last error */
	EDITSTREAMCALLBACK pfnCallback;
} EDITSTREAM;

/* stream formats */

#define SF_TEXT			0x0001
#define SF_RTF			0x0002
#define SF_RTFNOOBJS	0x0003		/* outbound only */
#define SF_TEXTIZED		0x0004		/* outbound only */
#define SF_UNICODE		0x0010		/* Unicode file of some kind */

/* Flag telling stream operations to operate on the selection only */
/* EM_STREAMIN will replace the current selection */
/* EM_STREAMOUT will stream out the current selection */
#define SFF_SELECTION	0x8000

/* Flag telling stream operations to operate on the common RTF keyword only */
/* EM_STREAMIN will accept the only common RTF keyword */
/* EM_STREAMOUT will stream out the only common RTF keyword */
#define SFF_PLAINRTF	0x4000

typedef struct _findtext
{
	CHARRANGE chrg;
	LPSTR lpstrText;
} FINDTEXTA;

typedef struct _findtextw
{
	CHARRANGE chrg;
	LPWSTR lpstrText;
} FINDTEXTW;

#if (_RICHEDIT_VER >= 0x0200)
#ifdef UNICODE
#define FINDTEXT	FINDTEXTW
#else
#define FINDTEXT	FINDTEXTA
#endif /* UNICODE */
#else
#define FINDTEXT	FINDTEXTA
#endif /* _RICHEDIT_VER >= 0x0200 */

typedef struct _findtextexa
{
	CHARRANGE chrg;
	LPSTR lpstrText;
	CHARRANGE chrgText;
} FINDTEXTEXA;

typedef struct _findtextexw
{
	CHARRANGE chrg;
	LPWSTR lpstrText;
	CHARRANGE chrgText;
} FINDTEXTEXW;

#if (_RICHEDIT_VER >= 0x0200)
#ifdef UNICODE
#define FINDTEXTEX	FINDTEXTEXW
#else
#define FINDTEXTEX	FINDTEXTEXA
#endif /* UNICODE */
#else
#define FINDTEXTEX	FINDTEXTEXA
#endif /* _RICHEDIT_VER >= 0x0200 */


typedef struct _formatrange
{
	HDC hdc;
	HDC hdcTarget;
	RECT rc;
	RECT rcPage;
	CHARRANGE chrg;
} FORMATRANGE;

/* all paragraph measurements are in twips */

#define MAX_TAB_STOPS 32
#define lDefaultTab 720

typedef struct _paraformat
{
	UINT	cbSize;
	_WPAD	_wPad1;
	DWORD	dwMask;
	WORD	wNumbering;
	WORD	wReserved;
	LONG	dxStartIndent;
	LONG	dxRightIndent;
	LONG	dxOffset;
	WORD	wAlignment;
	SHORT	cTabCount;
	LONG	rgxTabs[MAX_TAB_STOPS];
} PARAFORMAT;

/* PARAFORMAT mask values */
#define PFM_STARTINDENT			0x00000001
#define PFM_RIGHTINDENT			0x00000002
#define PFM_OFFSET				0x00000004
#define PFM_ALIGNMENT			0x00000008
#define PFM_TABSTOPS			0x00000010
#define PFM_NUMBERING			0x00000020
#define PFM_OFFSETINDENT		0x80000000

/* PARAFORMAT numbering options */
#define PFN_BULLET		0x0001

/* PARAFORMAT alignment options */
#define PFA_LEFT	0x0001
#define PFA_RIGHT	0x0002
#define PFA_CENTER	0x0003

/* CHARFORMAT2 and PARAFORMAT2 structures */

#ifdef __cplusplus

struct CHARFORMAT2W : _charformatw
{
	WORD		wWeight;			/* Font weight (LOGFONT value)		*/
	SHORT		sSpacing;			/* Amount to space between letters	*/
	COLORREF	crBackColor;		/* Background color					*/
	LCID		lcid;				/* Locale ID						*/
	DWORD		dwReserved;			/* Reserved. Must be 0				*/
	SHORT		sStyle;				/* Style handle						*/
	WORD		wKerning;			/* Twip size above which to kern char pair*/
	BYTE		bUnderlineType;		/* Underline type					*/
	BYTE		bAnimation;			/* Animated text like marching ants */
	BYTE		bRevAuthor;			/* Revision author index			*/
};

struct CHARFORMAT2A : _charformat
{
	WORD		wWeight;			/* Font weight (LOGFONT value)		*/
	SHORT		sSpacing;			/* Amount to space between letters	*/
	COLORREF	crBackColor;		/* Background color					*/
	LCID		lcid;				/* Locale ID						*/
	DWORD		dwReserved;			/* Reserved. Must be 0				*/
	SHORT		sStyle;				/* Style handle						*/
	WORD		wKerning;			/* Twip size above which to kern char pair*/
	BYTE		bUnderlineType;		/* Underline type					*/
	BYTE		bAnimation;			/* Animated text like marching ants	*/
	BYTE		bRevAuthor;			/* Revision author index			*/
};

#else	/* regular C-style  */

typedef struct _charformat2w
{
	UINT		cbSize;
	_WPAD		_wPad1;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;			/* > 0 for superscript, < 0 for subscript */
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	WCHAR		szFaceName[LF_FACESIZE];
	_WPAD		_wPad2;
	WORD		wWeight;			/* Font weight (LOGFONT value)		*/
	SHORT		sSpacing;			/* Amount to space between letters	*/
	COLORREF	crBackColor;		/* Background color					*/
	LCID		lcid;				/* Locale ID						*/
	DWORD		dwReserved;			/* Reserved. Must be 0				*/
	SHORT		sStyle;				/* Style handle						*/
	WORD		wKerning;			/* Twip size above which to kern char pair*/
	BYTE		bUnderlineType;		/* Underline type					*/
	BYTE		bAnimation;			/* Animated text like marching ants	*/
	BYTE		bRevAuthor;			/* Revision author index			*/
	BYTE		bReserved1;
} CHARFORMAT2W;

typedef struct _charformat2a
{
	UINT		cbSize;
	_WPAD		_wPad1;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;			/* > 0 for superscript, < 0 for subscript */
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	char		szFaceName[LF_FACESIZE];
	_WPAD		_wPad2;
	WORD		wWeight;			/* Font weight (LOGFONT value)		*/
	SHORT		sSpacing;			/* Amount to space between letters	*/
	COLORREF	crBackColor;		/* Background color					*/
	LCID		lcid;				/* Locale ID						*/
	DWORD		dwReserved;			/* Reserved. Must be 0				*/
	SHORT		sStyle;				/* Style handle						*/
	WORD		wKerning;			/* Twip size above which to kern char pair*/
	BYTE		bUnderlineType;		/* Underline type					*/
	BYTE		bAnimation;			/* Animated text like marching ants	*/
	BYTE		bRevAuthor;			/* Revision author index			*/
} CHARFORMAT2A;

#endif /* C++ */

#ifdef UNICODE
#define CHARFORMAT2	CHARFORMAT2W
#else
#define CHARFORMAT2 CHARFORMAT2A
#endif

#define CHARFORMATDELTA		(sizeof(CHARFORMAT2) - sizeof(CHARFORMAT))


/* CHARFORMAT and PARAFORMAT "ALL" masks
   CFM_COLOR mirrors CFE_AUTOCOLOR, a little hack to easily deal with autocolor*/

#define CFM_EFFECTS (CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_COLOR | \
					 CFM_STRIKEOUT | CFE_PROTECTED | CFM_LINK)
#define CFM_ALL (CFM_EFFECTS | CFM_SIZE | CFM_FACE | CFM_OFFSET | CFM_CHARSET)

#define	PFM_ALL (PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_OFFSET	| \
				 PFM_ALIGNMENT   | PFM_TABSTOPS    | PFM_NUMBERING | \
				 PFM_OFFSETINDENT)

/* New masks and effects -- a parenthesized asterisk indicates that
   the data is stored by RichEdit2.0, but not displayed */

#define CFM_SMALLCAPS		0x0040			/* (*)	*/
#define	CFM_ALLCAPS			0x0080			/* (*)	*/
#define	CFM_HIDDEN			0x0100			/* (*)	*/
#define	CFM_OUTLINE			0x0200			/* (*)	*/
#define	CFM_SHADOW			0x0400			/* (*)	*/
#define	CFM_EMBOSS			0x0800			/* (*)	*/
#define	CFM_IMPRINT			0x1000			/* (*)	*/
#define CFM_DISABLED		0x2000
#define	CFM_REVISED			0x4000

#define CFM_BACKCOLOR		0x04000000
#define CFM_LCID			0x02000000
#define	CFM_UNDERLINETYPE	0x00800000		/* (*)	*/
#define	CFM_WEIGHT			0x00400000
#define CFM_SPACING			0x00200000		/* (*)	*/
#define CFM_KERNING			0x00100000		/* (*)	*/
#define CFM_STYLE			0x00080000		/* (*)	*/
#define CFM_ANIMATION		0x00040000		/* (*)	*/
#define CFM_REVAUTHOR		0x00008000

#define CFE_SUBSCRIPT		0x00010000		/* Superscript and subscript are */
#define CFE_SUPERSCRIPT		0x00020000		/*  mutually exclusive			 */

#define CFM_SUBSCRIPT		CFE_SUBSCRIPT | CFE_SUPERSCRIPT
#define CFM_SUPERSCRIPT		CFM_SUBSCRIPT

#define	CFM_EFFECTS2 (CFM_EFFECTS | CFM_DISABLED | CFM_SMALLCAPS | CFM_ALLCAPS \
					| CFM_HIDDEN  | CFM_OUTLINE | CFM_SHADOW | CFM_EMBOSS \
					| CFM_IMPRINT | CFM_DISABLED | CFM_REVISED \
					| CFM_SUBSCRIPT | CFM_SUPERSCRIPT | CFM_BACKCOLOR)

#define CFM_ALL2	 (CFM_ALL | CFM_EFFECTS2 | CFM_BACKCOLOR | CFM_LCID \
					| CFM_UNDERLINETYPE | CFM_WEIGHT | CFM_REVAUTHOR \
					| CFM_SPACING | CFM_KERNING | CFM_STYLE | CFM_ANIMATION)

#define	CFE_SMALLCAPS		CFM_SMALLCAPS
#define	CFE_ALLCAPS			CFM_ALLCAPS
#define	CFE_HIDDEN			CFM_HIDDEN
#define	CFE_OUTLINE			CFM_OUTLINE
#define	CFE_SHADOW			CFM_SHADOW
#define	CFE_EMBOSS			CFM_EMBOSS
#define	CFE_IMPRINT			CFM_IMPRINT
#define	CFE_DISABLED		CFM_DISABLED
#define	CFE_REVISED			CFM_REVISED

/* NOTE: CFE_AUTOCOLOR and CFE_AUTOBACKCOLOR correspond to CFM_COLOR and
   CFM_BACKCOLOR, respectively, which control them */
#define CFE_AUTOBACKCOLOR	CFM_BACKCOLOR

/* Underline types */
#define CFU_CF1UNDERLINE	0xFF	/* map charformat's bit underline to CF2.*/
#define CFU_INVERT			0xFE	/* For IME composition fake a selection.*/	
#define	CFU_UNDERLINEDOTTED	0x4		/* (*) displayed as ordinary underline	*/
#define	CFU_UNDERLINEDOUBLE	0x3		/* (*) displayed as ordinary underline	*/
#define CFU_UNDERLINEWORD	0x2		/* (*) displayed as ordinary underline	*/
#define CFU_UNDERLINE		0x1
#define CFU_UNDERLINENONE	0

#ifdef __cplusplus
struct PARAFORMAT2 : _paraformat
{
	LONG	dySpaceBefore;			/* Vertical spacing before para			*/
	LONG	dySpaceAfter;			/* Vertical spacing after para			*/	
	LONG	dyLineSpacing;			/* Line spacing depending on Rule		*/
	SHORT	sStyle;					/* Style handle							*/
	BYTE	bLineSpacingRule;		/* Rule for line spacing (see tom.doc)	*/
	BYTE	bCRC;					/* Reserved for CRC for rapid searching	*/
	WORD	wShadingWeight;			/* Shading in hundredths of a per cent	*/
	WORD	wShadingStyle;			/* Nibble 0: style, 1: cfpat, 2: cbpat	*/
	WORD	wNumberingStart;		/* Starting value for numbering			*/
	WORD	wNumberingStyle;		/* Alignment, roman/arabic, (), ), ., etc.*/
	WORD	wNumberingTab;			/* Space bet FirstIndent and 1st-line text*/
	WORD	wBorderSpace;			/* Space between border and text (twips)*/
	WORD	wBorderWidth;			/* Border pen width (twips)				*/
	WORD	wBorders;				/* Byte 0: bits specify which borders	*/
									/* Nibble 2: border style, 3: color index*/
};

#else	/* regular C-style	*/

typedef struct _paraformat2
{
	UINT	cbSize;
	_WPAD	_wPad1;
	DWORD	dwMask;
	WORD	wNumbering;
	WORD	wReserved;
	LONG	dxStartIndent;
	LONG	dxRightIndent;
	LONG	dxOffset;
	WORD	wAlignment;
	SHORT	cTabCount;
	LONG	rgxTabs[MAX_TAB_STOPS];
 	LONG	dySpaceBefore;			/* Vertical spacing before para			*/
	LONG	dySpaceAfter;			/* Vertical spacing after para			*/
	LONG	dyLineSpacing;			/* Line spacing depending on Rule		*/
	SHORT	sStyle;					/* Style handle							*/
	BYTE	bLineSpacingRule;		/* Rule for line spacing (see tom.doc)	*/
	BYTE	bCRC;					/* Reserved for CRC for rapid searching	*/
	WORD	wShadingWeight;			/* Shading in hundredths of a per cent	*/
	WORD	wShadingStyle;			/* Nibble 0: style, 1: cfpat, 2: cbpat	*/
	WORD	wNumberingStart;		/* Starting value for numbering			*/	
	WORD	wNumberingStyle;		/* Alignment, roman/arabic, (), ), ., etc.*/
	WORD	wNumberingTab;			/* Space bet 1st indent and 1st-line text*/
	WORD	wBorderSpace;			/* Space between border and text (twips)*/
	WORD	wBorderWidth;			/* Border pen width (twips)				*/
	WORD	wBorders;				/* Byte 0: bits specify which borders	*/
									/* Nibble 2: border style, 3: color index*/
} PARAFORMAT2;

#endif /* C++	*/

/* this is a hack to make PARAFORMAT code more readable.
   it applies to the wReserved field in PARAFORMAT, which
   in PARAFORMAT2 is now used. */

#define	wEffects			wReserved

/* PARAFORMAT 2.0 masks and effects */

#define PFM_SPACEBEFORE			0x00000040
#define PFM_SPACEAFTER			0x00000080
#define PFM_LINESPACING			0x00000100
#define	PFM_STYLE				0x00000400
#define PFM_BORDER				0x00000800	/* (*)	*/
#define PFM_SHADING				0x00001000	/* (*)	*/
#define PFM_NUMBERINGSTYLE		0x00002000	/* (*)	*/
#define PFM_NUMBERINGTAB		0x00004000	/* (*)	*/
#define PFM_NUMBERINGSTART		0x00008000	/* (*)	*/

#define PFM_RTLPARA				0x00010000
#define PFM_KEEP				0x00020000	/* (*)	*/
#define PFM_KEEPNEXT			0x00040000	/* (*)	*/
#define PFM_PAGEBREAKBEFORE		0x00080000	/* (*)	*/
#define PFM_NOLINENUMBER		0x00100000	/* (*)	*/
#define PFM_NOWIDOWCONTROL		0x00200000	/* (*)	*/
#define PFM_DONOTHYPHEN			0x00400000	/* (*)	*/
#define PFM_SIDEBYSIDE			0x00800000	/* (*)	*/

#define PFM_TABLE				0xc0000000	/* (*)	*/

/* Note: PARAFORMAT has no effects */
#define PFM_EFFECTS  (PFM_RTLPARA | PFM_KEEP | PFM_KEEPNEXT | PFM_TABLE \
					| PFM_PAGEBREAKBEFORE | PFM_NOLINENUMBER  \
					| PFM_NOWIDOWCONTROL | PFM_DONOTHYPHEN | PFM_SIDEBYSIDE \
					| PFM_TABLE)

#define PFM_ALL2	(PFM_ALL | PFM_EFFECTS | PFM_SPACEBEFORE | PFM_SPACEAFTER \
					| PFM_LINESPACING | PFM_STYLE | PFM_SHADING | PFM_BORDER \
					| PFM_NUMBERINGTAB | PFM_NUMBERINGSTART | PFM_NUMBERINGSTYLE)

#define PFE_RTLPARA				(PFM_RTLPARA		 >> 16)
#define PFE_KEEP				(PFM_KEEP			 >> 16)	/* (*)	*/
#define PFE_KEEPNEXT			(PFM_KEEPNEXT		 >> 16)	/* (*)	*/
#define PFE_PAGEBREAKBEFORE		(PFM_PAGEBREAKBEFORE >> 16)	/* (*)	*/
#define PFE_NOLINENUMBER		(PFM_NOLINENUMBER	 >> 16)	/* (*)	*/
#define PFE_NOWIDOWCONTROL		(PFM_NOWIDOWCONTROL	 >> 16)	/* (*)	*/
#define PFE_DONOTHYPHEN			(PFM_DONOTHYPHEN 	 >> 16)	/* (*)	*/
#define PFE_SIDEBYSIDE			(PFM_SIDEBYSIDE		 >> 16)	/* (*)	*/

#define PFE_TABLEROW			0xc000		/* These 3 options are mutually	*/
#define PFE_TABLECELLEND		0x8000		/*  exclusive and each imply	*/
#define PFE_TABLECELL			0x4000		/*  that para is part of a table*/

/*
 *	PARAFORMAT numbering options (values for wNumbering):
 *
 *		Numbering Type		Value	Meaning
 *		tomNoNumbering		  0		Turn off paragraph numbering
 *		tomNumberAsLCLetter	  1		a, b, c, ...
 *		tomNumberAsUCLetter	  2		A, B, C, ...
 *		tomNumberAsLCRoman	  3		i, ii, iii, ...
 *		tomNumberAsUCRoman	  4		I, II, III, ...
 *		tomNumberAsSymbols	  5		default is bullet
 *		tomNumberAsNumber	  6		0, 1, 2, ...
 *		tomNumberAsSequence	  7		tomNumberingStart is first Unicode to use
 *
 *	Other valid Unicode chars are Unicodes for bullets.
 */


#define	PFA_JUSTIFY			4	/* New paragraph-alignment option 2.0 (*)


/* notification structures */

#ifndef WM_NOTIFY
#define WM_NOTIFY				0x004E

typedef struct _nmhdr
{
	HWND	hwndFrom;
	_WPAD	_wPad1;
	UINT	idFrom;
	_WPAD	_wPad2;
	UINT	code;
	_WPAD	_wPad3;
} NMHDR;
#endif  /* !WM_NOTIFY */

typedef struct _msgfilter
{
	NMHDR	nmhdr;
	UINT	msg;
	_WPAD	_wPad1;
	WPARAM	wParam;
	_WPAD	_wPad2;
	LPARAM	lParam;
} MSGFILTER;

typedef struct _reqresize
{
	NMHDR nmhdr;
	RECT rc;
} REQRESIZE;

typedef struct _selchange
{
	NMHDR nmhdr;
	CHARRANGE chrg;
	WORD seltyp;
} SELCHANGE;

#define SEL_EMPTY		0x0000
#define SEL_TEXT		0x0001
#define SEL_OBJECT		0x0002
#define SEL_MULTICHAR	0x0004
#define SEL_MULTIOBJECT	0x0008

/* used with IRichEditOleCallback::GetContextMenu, this flag will be
   passed as a "selection type".  It indicates that a context menu for
   a right-mouse drag drop should be generated.  The IOleObject parameter
   will really be the IDataObject for the drop
 */
#define GCM_RIGHTMOUSEDROP  0x8000

typedef struct _endropfiles
{
	NMHDR nmhdr;
	HANDLE hDrop;
	LONG cp;
	BOOL fProtected;
} ENDROPFILES;

typedef struct _enprotected
{
	NMHDR nmhdr;
	UINT msg;
	_WPAD	_wPad1;
	WPARAM wParam;
	_WPAD	_wPad2;
	LPARAM lParam;
	CHARRANGE chrg;
} ENPROTECTED;

typedef struct _ensaveclipboard
{
	NMHDR nmhdr;
	LONG cObjectCount;
    LONG cch;
} ENSAVECLIPBOARD;

#ifndef MACPORT
typedef struct _enoleopfailed
{
	NMHDR nmhdr;
	LONG iob;
	LONG lOper;
	HRESULT hr;
} ENOLEOPFAILED;
#endif

#define	OLEOP_DOVERB	1

typedef struct _objectpositions
{
    NMHDR nmhdr;
    LONG cObjectCount;
    LONG *pcpPositions;
} OBJECTPOSITIONS;

typedef struct _enlink
{
    NMHDR nmhdr;
    UINT msg;
    _WPAD   _wPad1;
    WPARAM wParam;
    _WPAD   _wPad2;
    LPARAM lParam;
    CHARRANGE chrg;
} ENLINK;

/* PenWin specific */
typedef struct _encorrecttext
{
	NMHDR nmhdr;
	CHARRANGE chrg;
	WORD seltyp;
} ENCORRECTTEXT;

/* Far East specific */
typedef struct _punctuation
{
	UINT	iSize;
	LPSTR	szPunctuation;
} PUNCTUATION;

/* Far East specific */
typedef struct _compcolor
{
	COLORREF crText;
	COLORREF crBackground;
	DWORD dwEffects;
}COMPCOLOR;


/* clipboard formats - use as parameter to RegisterClipboardFormat() */
#define CF_RTF 			TEXT("Rich Text Format")
#define CF_RTFNOOBJS 	TEXT("Rich Text Format Without Objects")
#define CF_RETEXTOBJ 	TEXT("RichEdit Text and Objects")

/* Paste Special */
typedef struct _repastespecial
{
	DWORD	dwAspect;
	DWORD	dwParam;
} REPASTESPECIAL;

/*	UndoName info */
typedef enum _undonameid
{
    UID_UNKNOWN     = 0,
	UID_TYPING		= 1,
	UID_DELETE 		= 2,
	UID_DRAGDROP	= 3,
	UID_CUT			= 4,
	UID_PASTE		= 5
} UNDONAMEID;

/* flags for the GETEXTEX data structure */
#define GT_DEFAULT		0
#define GT_USECRLF		1

/* EM_GETTEXTEX info; this struct is passed in the wparam of the message */
typedef struct _gettextex
{
	DWORD	cb;				/* count of bytes in the string				*/
	DWORD	flags;			/* flags (see the GT_XXX defines			*/
	UINT	codepage;		/* code page for translation (CP_ACP for default,
						       1200 for Unicode							*/
	LPCSTR	lpDefaultChar;	/* replacement for unmappable chars			*/
	LPBOOL	lpUsedDefChar;	/* pointer to flag set when def char used	*/
} GETTEXTEX;

/* flags for the GETTEXTLENGTHEX data structure							*/
#define GTL_DEFAULT		0	/* do the default (return # of chars)		*/
#define GTL_USECRLF		1	/* compute answer using CRLFs for paragraphs*/
#define GTL_PRECISE		2	/* compute a precise answer					*/
#define GTL_CLOSE		4	/* fast computation of a "close" answer		*/
#define GTL_NUMCHARS	8	/* return the number of characters			*/
#define GTL_NUMBYTES	16	/* return the number of _bytes_				*/

/* EM_GETTEXTLENGTHEX info; this struct is passed in the wparam of the msg */
typedef struct _gettextlengthex
{
	DWORD	flags;			/* flags (see GTL_XXX defines)				*/
	UINT	codepage;		/* code page for translation (CP_ACP for default,
							   1200 for Unicode							*/
} GETTEXTLENGTHEX;
	
	
/* UNICODE embedding character */
#ifndef WCH_EMBEDDING
#define WCH_EMBEDDING (WCHAR)0xFFFC
#endif /* WCH_EMBEDDING */
		

#undef _WPAD

#ifdef _WIN32
#include <poppack.h>
#elif !defined(RC_INVOKED)
#pragma pack()
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* !_RICHEDIT_ */

