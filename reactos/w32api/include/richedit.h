#ifndef _RICHEDIT_H
#define _RICHEDIT_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push,4)

#ifdef UNICODE 
#define RICHEDIT_CLASS L"RichEdit20W"
#else
#define RICHEDIT_CLASS "RichEdit20A"
#endif
#define RICHEDIT_CLASS10A    "RICHEDIT"
#define CF_RTF TEXT("Rich Text Format")
#define CF_RTFNOOBJS TEXT("Rich Text Format Without Objects")
#define CF_RETEXTOBJ TEXT("RichEdit Text and Objects")
#define CFM_BOLD	1
#define CFM_ITALIC	2
#define CFM_UNDERLINE	4
#define CFM_STRIKEOUT	8
#define CFM_PROTECTED	16
#define CFM_LINK	32
#define CFM_SIZE	0x80000000
#define CFM_COLOR	0x40000000
#define CFM_FACE	0x20000000
#define CFM_OFFSET	0x10000000
#define CFM_CHARSET	0x08000000
#define CFM_SUBSCRIPT	0x00030000
#define CFM_SUPERSCRIPT	0x00030000
#define CFM_EFFECTS	(CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_COLOR | CFM_STRIKEOUT | CFE_PROTECTED | CFM_LINK)
#define CFE_BOLD	1
#define CFE_ITALIC	2
#define CFE_UNDERLINE	4
#define CFE_STRIKEOUT	8
#define CFE_PROTECTED	16
#define CFE_AUTOCOLOR	0x40000000
#define CFE_SUBSCRIPT	0x00010000
#define CFE_SUPERSCRIPT	0x00020000
#define IMF_FORCENONE	1
#define IMF_FORCEENABLE	2
#define IMF_FORCEDISABLE	4
#define IMF_CLOSESTATUSWINDOW	8
#define IMF_VERTICAL	32
#define IMF_FORCEACTIVE	64
#define IMF_FORCEINACTIVE	128
#define IMF_FORCEREMEMBER	256
#define SEL_EMPTY       0
#define SEL_TEXT        1
#define SEL_OBJECT      2
#define SEL_MULTICHAR   4
#define SEL_MULTIOBJECT 8
#define MAX_TAB_STOPS 32
#define PFM_ALIGNMENT 8
#define PFM_NUMBERING 32
#define PFM_OFFSET 4
#define PFM_OFFSETINDENT 0x80000000
#define PFM_RIGHTINDENT 2
#define PFM_STARTINDENT 1
#define PFM_TABSTOPS 16
#define PFM_BORDER 2048
#define PFM_LINESPACING 256
#define PFM_NUMBERINGSTART 32768
#define PFM_NUMBERINGSTYLE 8192
#define PFM_NUMBERINGTAB 16384
#define PFM_SHADING 4096
#define PFM_SPACEAFTER 128
#define PFM_SPACEBEFORE 64
#define PFM_STYLE 1024
#define PFM_DONOTHYPHEN 4194304
#define PFM_KEEP 131072
#define PFM_KEEPNEXT 262144
#define PFM_NOLINENUMBER 1048576
#define PFM_NOWIDOWCONTROL 2097152
#define PFM_PAGEBREAKBEFORE 524288
#define PFM_RTLPARA 65536
#define PFM_SIDEBYSIDE 8388608
#define PFM_TABLE 1073741824
#define PFN_BULLET 1
#define PFE_DONOTHYPHEN 64
#define PFE_KEEP 2
#define PFE_KEEPNEXT 4
#define PFE_NOLINENUMBER 16
#define PFE_NOWIDOWCONTROL 32
#define PFE_PAGEBREAKBEFORE 8
#define PFE_RTLPARA 1
#define PFE_SIDEBYSIDE 128
#define PFE_TABLE 16384
#define PFA_LEFT 1
#define PFA_RIGHT 2
#define PFA_CENTER 3
#define PFA_JUSTIFY 4
#define PFA_FULL_INTERWORD 4
#define SF_TEXT	1
#define SF_RTF	2
#define SF_RTFNOOBJS	3
#define SF_TEXTIZED	4
#define SF_UNICODE	16
#define SF_USECODEPAGE	32
#define SF_NCRFORNONASCII	64
#define SF_RTFVAL	0x0700
#define SFF_PWD	0x0800
#define SFF_KEEPDOCINFO	0x1000
#define SFF_PERSISTVIEWSCALE	0x2000
#define SFF_PLAINRTF	0x4000
#define SFF_SELECTION	0x8000
#define WB_CLASSIFY	3
#define WB_MOVEWORDLEFT	4
#define WB_MOVEWORDRIGHT	5
#define WB_LEFTBREAK	6
#define WB_RIGHTBREAK	7
#define WB_MOVEWORDPREV	4
#define WB_MOVEWORDNEXT	5
#define WB_PREVBREAK	6
#define WB_NEXTBREAK	7
#define WBF_WORDWRAP	16
#define WBF_WORDBREAK	32
#define WBF_OVERFLOW	64
#define WBF_LEVEL1	128
#define WBF_LEVEL2	256
#define WBF_CUSTOM	512
#define ES_DISABLENOSCROLL	8192
#define ES_EX_NOCALLOLEINIT 16777216
#define ES_NOIME 524288
#define ES_NOOLEDRAGDROP 8
#define ES_SAVESEL	32768
#define ES_SELECTIONBAR 16777216
#define ES_SELFIME 262144
#define ES_SUNKEN 16384
#define ES_VERTICAL 4194304
#define EM_CANPASTE	(WM_USER+50)
#define EM_DISPLAYBAND	(WM_USER+51)
#define EM_EXGETSEL	(WM_USER+52)
#define EM_EXLIMITTEXT	(WM_USER+53)
#define EM_EXLINEFROMCHAR	(WM_USER+54)
#define EM_EXSETSEL	(WM_USER+55)
#define EM_FINDTEXT	(WM_USER+56)
#define EM_FORMATRANGE	(WM_USER+57)
#define EM_GETCHARFORMAT	(WM_USER+58)
#define EM_GETEVENTMASK	(WM_USER+59)
#define EM_GETOLEINTERFACE	(WM_USER+60)
#define EM_GETPARAFORMAT	(WM_USER+61)
#define EM_GETSELTEXT	(WM_USER+62)
#define EM_HIDESELECTION	(WM_USER+63)
#define EM_PASTESPECIAL	(WM_USER+64)
#define EM_REQUESTRESIZE	(WM_USER+65)
#define EM_SELECTIONTYPE	(WM_USER+66)
#define EM_SETBKGNDCOLOR	(WM_USER+67)
#define EM_SETCHARFORMAT	(WM_USER+68)
#define EM_SETEVENTMASK	(WM_USER+69)
#define EM_SETOLECALLBACK	(WM_USER+70)
#define EM_SETPARAFORMAT	(WM_USER+71)
#define EM_SETTARGETDEVICE	(WM_USER+72)
#define EM_STREAMIN	(WM_USER+73)
#define EM_STREAMOUT	(WM_USER+74)
#define EM_GETTEXTRANGE	(WM_USER+75)
#define EM_FINDWORDBREAK	(WM_USER+76)
#define EM_SETOPTIONS	(WM_USER+77)
#define EM_GETOPTIONS	(WM_USER+78)
#define EM_FINDTEXTEX	(WM_USER+79)
#define EM_GETWORDBREAKPROCEX	(WM_USER+80)
#define EM_SETWORDBREAKPROCEX	(WM_USER+81)
/* RichEdit 2.0 messages */
#define EM_SETUNDOLIMIT	(WM_USER+82)
#define EM_REDO	(WM_USER+84)
#define EM_CANREDO	(WM_USER+85)
#define EM_GETUNDONAME	(WM_USER+86)
#define EM_GETREDONAME	(WM_USER+87)
#define EM_STOPGROUPTYPING	(WM_USER+88)
#define EM_SETTEXTMODE	(WM_USER+89)
#define EM_GETTEXTMODE	(WM_USER+90)
#define EM_AUTOURLDETECT	(WM_USER+91)
#define EM_GETAUTOURLDETECT	(WM_USER + 92)
#define EM_SETPALETTE	(WM_USER + 93)
#define EM_GETTEXTEX	(WM_USER+94)
#define EM_GETTEXTLENGTHEX	(WM_USER+95)
#define EM_SHOWSCROLLBAR	(WM_USER+96)
#define EM_SETTEXTEX	(WM_USER + 97)
#define EM_SETPUNCTUATION	(WM_USER + 100)
#define EM_GETPUNCTUATION	(WM_USER + 101)
#define EM_SETWORDWRAPMODE	(WM_USER + 102)
#define EM_GETWORDWRAPMODE	(WM_USER + 103)
#define EM_SETIMECOLOR	(WM_USER + 104)
#define EM_GETIMECOLOR	(WM_USER + 105)
#define EM_SETIMEOPTIONS	(WM_USER + 106)
#define EM_GETIMEOPTIONS	(WM_USER + 107)
#define EM_SETLANGOPTIONS	(WM_USER+120)
#define EM_GETLANGOPTIONS	(WM_USER+121)
#define EM_GETIMECOMPMODE	(WM_USER+122)
#define EM_FINDTEXTW	(WM_USER + 123)
#define EM_FINDTEXTEXW	(WM_USER + 124)
#define EM_RECONVERSION	(WM_USER + 125)
#define EM_SETBIDIOPTIONS	(WM_USER + 200)
#define EM_GETBIDIOPTIONS	(WM_USER + 201)
#define EM_SETTYPOGRAPHYOPTIONS	(WM_USER+202)
#define EM_GETTYPOGRAPHYOPTIONS	(WM_USER+203)
#define EM_SETEDITSTYLE	(WM_USER + 204)
#define EM_GETEDITSTYLE	(WM_USER + 205)
#define EM_GETSCROLLPOS	(WM_USER+221)
#define EM_SETSCROLLPOS	(WM_USER+222)
#define EM_SETFONTSIZE	(WM_USER+223)
#define EM_GETZOOM	(WM_USER+224)
#define EM_SETZOOM	(WM_USER+225)

#define EN_CORRECTTEXT 1797
#define EN_DROPFILES 1795
#define EN_IMECHANGE 1799
#define EN_LINK 1803
#define EN_MSGFILTER 1792
#define EN_OLEOPFAILED 1801
#define EN_PROTECTED 1796
#define EN_REQUESTRESIZE 1793
#define EN_SAVECLIPBOARD 1800
#define EN_SELCHANGE 1794
#define EN_STOPNOUNDO 1798
#define ENM_NONE 0
#define ENM_CHANGE 1
#define ENM_CORRECTTEXT 4194304
#define ENM_DRAGDROPDONE	16
#define ENM_DROPFILES 1048576
#define ENM_IMECHANGE 8388608
#define ENM_KEYEVENTS 65536
#define ENM_LANGCHANGE 16777216
#define ENM_LINK 67108864
#define ENM_MOUSEEVENTS 131072
#define ENM_OBJECTPOSITIONS 33554432
#define ENM_PROTECTED 2097152
#define ENM_REQUESTRESIZE 262144
#define ENM_SCROLL 4
#define ENM_SCROLLEVENTS 8
#define ENM_SELCHANGE 524288
#define ENM_UPDATE 2
#define ECO_AUTOWORDSELECTION	1
#define ECO_AUTOVSCROLL	64
#define ECO_AUTOHSCROLL	128
#define ECO_NOHIDESEL	256
#define ECO_READONLY	2048
#define ECO_WANTRETURN	4096
#define ECO_SAVESEL	0x8000
#define ECO_SELECTIONBAR	0x1000000
#define ECO_VERTICAL	0x400000
#define ECOOP_SET	1
#define ECOOP_OR	2
#define ECOOP_AND	3
#define ECOOP_XOR	4
#define SCF_DEFAULT	0
#define SCF_SELECTION	1
#define SCF_WORD	2
#define SCF_ALL	4
#define SCF_USEUIRULES	8
#define TM_PLAINTEXT	1
#define TM_RICHTEXT	2
#define TM_SINGLELEVELUNDO	4
#define TM_MULTILEVELUNDO	8
#define TM_SINGLECODEPAGE	16
#define TM_MULTICODEPAGE	32
#define GT_DEFAULT	0
#define GT_USECRLF	1
#define yHeightCharPtsMost 1638
#define lDefaultTab 720

typedef struct _charformat {
	UINT cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG yHeight;
	LONG yOffset;
	COLORREF crTextColor;
	BYTE bCharSet;
	BYTE bPitchAndFamily;
	char szFaceName[LF_FACESIZE];
} CHARFORMATA;
typedef struct _charformatw {
	UINT cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG yHeight;
	LONG yOffset;
	COLORREF crTextColor;
	BYTE bCharSet;
	BYTE bPitchAndFamily;
	WCHAR szFaceName[LF_FACESIZE];
} CHARFORMATW;
typedef struct _charformat2a {
	UINT cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG yHeight;
	LONG yOffset;
	COLORREF crTextColor;
	BYTE bCharSet;
	BYTE bPitchAndFamily;
	char szFaceName[LF_FACESIZE];
	WORD wWeight;
	SHORT sSpacing;
	COLORREF crBackColor;
	LCID lcid;
	DWORD dwReserved;
	SHORT sStyle;
	WORD wKerning;
	BYTE bUnderlineType;
	BYTE bAnimation;
	BYTE bRevAuthor;
} CHARFORMAT2A;
typedef struct _charformat2w {
	UINT cbSize;
	DWORD dwMask;
	DWORD dwEffects;
	LONG yHeight;
	LONG yOffset;
	COLORREF crTextColor;
	BYTE bCharSet;
	BYTE bPitchAndFamily;
	WCHAR szFaceName[LF_FACESIZE];
	WORD wWeight;
	SHORT sSpacing;
	COLORREF crBackColor;
	LCID lcid;
	DWORD dwReserved;
	SHORT sStyle;
	WORD wKerning;
	BYTE bUnderlineType;
	BYTE bAnimation;
	BYTE bRevAuthor;
} CHARFORMAT2W;
typedef struct _charrange {
	LONG cpMin;
	LONG cpMax;
} CHARRANGE;
typedef struct _compcolor {
	COLORREF crText;
	COLORREF crBackground;
	DWORD dwEffects;
} COMPCOLOR;
typedef DWORD(CALLBACK *EDITSTREAMCALLBACK)(DWORD,PBYTE,LONG,LONG*);
typedef struct _editstream {
	DWORD dwCookie;
	DWORD dwError;
	EDITSTREAMCALLBACK pfnCallback;
} EDITSTREAM;
typedef struct _encorrecttext {
	NMHDR nmhdr;
	CHARRANGE chrg;
	WORD seltyp;
} ENCORRECTTEXT;
typedef struct _endropfiles {
	NMHDR nmhdr;
	HANDLE hDrop;
	LONG cp;
	BOOL fProtected;
} ENDROPFILES;
typedef struct _enlink {
	NMHDR nmhdr;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	CHARRANGE chrg;
} ENLINK;
typedef struct {
	NMHDR nmhdr;
	LONG iob;
	LONG lOper;
	HRESULT hr;
} ENOLEOPFAILED;
typedef struct _enprotected {
	NMHDR nmhdr;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
	CHARRANGE chrg;
} ENPROTECTED,*LPENPROTECTED;
typedef struct _ensaveclipboard {
	NMHDR nmhdr;
	LONG cObjectCount;
	LONG cch;
} ENSAVECLIPBOARD;
typedef struct _findtextA {
	CHARRANGE chrg;
	LPSTR lpstrText;
} FINDTEXTA;
typedef struct _findtextW {
	CHARRANGE chrg;
	LPWSTR lpstrText;
} FINDTEXTW;
typedef struct _findtextexA {
	CHARRANGE chrg;
	LPSTR lpstrText;
	CHARRANGE chrgText;
} FINDTEXTEXA;
typedef struct _findtextexW {
	CHARRANGE chrg;
	LPWSTR lpstrText;
	CHARRANGE chrgText;
} FINDTEXTEXW;
typedef struct _formatrange {
	HDC hdc;
	HDC hdcTarget;
	RECT rc;
	RECT rcPage;
	CHARRANGE chrg;
} FORMATRANGE;
typedef struct _msgfilter {
	NMHDR nmhdr;
    UINT msg;
	WPARAM wParam;
	LPARAM lParam;
} MSGFILTER;
typedef struct _paraformat {
	UINT cbSize;
	DWORD dwMask;
	WORD wNumbering;
	WORD wReserved;
	LONG dxStartIndent;
	LONG dxRightIndent;
	LONG dxOffset;
	WORD wAlignment;
	SHORT cTabCount;
	LONG rgxTabs[MAX_TAB_STOPS];
} PARAFORMAT;
typedef struct _paraformat2 {
	UINT cbSize;
	DWORD dwMask;
	WORD wNumbering;
	WORD wEffects;
	LONG dxStartIndent;
	LONG dxRightIndent;
	LONG dxOffset;
	WORD wAlignment;
	SHORT cTabCount;
	LONG rgxTabs[MAX_TAB_STOPS];
	LONG dySpaceBefore;
	LONG dySpaceAfter;
	LONG dyLineSpacing;
	SHORT sStype;
	BYTE bLineSpacingRule;
	BYTE bOutlineLevel;
	WORD wShadingWeight;
	WORD wShadingStyle;
	WORD wNumberingStart;
	WORD wNumberingStyle;
	WORD wNumberingTab;
	WORD wBorderSpace;
	WORD wBorderWidth;
	WORD wBorders;
} PARAFORMAT2;
typedef struct _selchange {
	NMHDR nmhdr;
	CHARRANGE chrg;
	WORD seltyp;
} SELCHANGE;
typedef struct _textrange {
	CHARRANGE chrg;
	LPSTR lpstrText;
} TEXTRANGEA;
typedef struct _textrangew {
	CHARRANGE chrg;
	LPWSTR lpstrText;
} TEXTRANGEW;
typedef struct _reqresize {
	NMHDR nmhdr;
	RECT rc;
} REQRESIZE;
typedef struct _repastespecial {
	DWORD dwAspect;
	DWORD dwParam;
} REPASTESPECIAL;
typedef struct _punctuation {
	UINT iSize;
	LPSTR szPunctuation;
} PUNCTUATION;
typedef struct _gettextex {
	DWORD cb;
	DWORD flags;
	UINT codepage;
	LPCSTR lpDefaultChar;
	LPBOOL lpUsedDefaultChar;
} GETTEXTEX;
typedef LONG (*EDITWORDBREAKPROCEX)(char*,LONG,BYTE,INT);
/* Defines for EM_SETTYPOGRAPHYOPTIONS */
#define	TO_ADVANCEDTYPOGRAPHY	1
#define	TO_SIMPLELINEBREAK	2
/* Defines for GETTEXTLENGTHEX */
#define GTL_DEFAULT	0
#define GTL_USECRLF	1
#define GTL_PRECISE	2
#define GTL_CLOSE	4
#define GTL_NUMCHARS	8
#define GTL_NUMBYTES	16
typedef struct _gettextlengthex {
	DWORD flags;
	UINT codepage;
} GETTEXTLENGTHEX;
#ifdef UNICODE
typedef CHARFORMATW CHARFORMAT;
typedef CHARFORMAT2W CHARFORMAT2;
typedef FINDTEXTW FINDTEXT;
typedef FINDTEXTEXW FINDTEXTEX;
typedef TEXTRANGEW TEXTRANGE;
#else
typedef CHARFORMATA CHARFORMAT;
typedef CHARFORMAT2A CHARFORMAT2;
typedef FINDTEXTA FINDTEXT;
typedef FINDTEXTEXA FINDTEXTEX;
typedef TEXTRANGEA TEXTRANGE;
#endif
#pragma pack(pop)
#ifdef __cplusplus
}
#endif
#endif
