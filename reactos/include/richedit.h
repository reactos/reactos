/*
 * Copyright (C) 2000 Jean-Claude Batista
 * Copyright (C) 2002 Andriy Palamarchuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_RICHEDIT_H
#define __WINE_RICHEDIT_H

#include "pshpack4.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER   0x0210
#endif /* _RICHEDIT_VER */

#define cchTextLimitDefault 0x7fff

#define RICHEDIT_CLASS20A	"RichEdit20A"
#if defined(__GNUC__)
# define RICHEDIT_CLASS20W (const WCHAR []){ 'R','i','c','h','E','d','i','t','2','0','W',0 }
#elif defined(_MSC_VER)
# define RICHEDIT_CLASS20W      L"RichEdit20W"
#else
static const WCHAR RICHEDIT_CLASS20W[] = { 'R','i','c','h','E','d','i','t','2','0','W',0 };
#endif
#define RICHEDIT_CLASS10A	"RICHEDIT"

#if (_RICHEDIT_VER >= 0x0200 )
#define RICHEDIT_CLASS		WINELIB_NAME_AW(RICHEDIT_CLASS20)
#else
#define RICHEDIT_CLASS		RICHEDIT_CLASS10A
#endif

#define EM_CANPASTE		(WM_USER + 50)
#define EM_DISPLAYBAND		(WM_USER + 51)
#define EM_EXGETSEL		(WM_USER + 52)
#define EM_EXLIMITTEXT		(WM_USER + 53)
#define EM_EXLINEFROMCHAR	(WM_USER + 54)
#define EM_EXSETSEL		(WM_USER + 55)
#define EM_FINDTEXT		(WM_USER + 56)
#define EM_FORMATRANGE		(WM_USER + 57)
#define EM_GETCHARFORMAT	(WM_USER + 58)
#define EM_GETEVENTMASK		(WM_USER + 59)
#define EM_GETOLEINTERFACE	(WM_USER + 60)
#define EM_GETPARAFORMAT	(WM_USER + 61)
#define EM_GETSELTEXT		(WM_USER + 62)
#define EM_HIDESELECTION	(WM_USER + 63)
#define EM_PASTESPECIAL		(WM_USER + 64)
#define EM_REQUESTRESIZE	(WM_USER + 65)
#define EM_SELECTIONTYPE	(WM_USER + 66)
#define EM_SETBKGNDCOLOR	(WM_USER + 67)
#define EM_SETCHARFORMAT	(WM_USER + 68)
#define EM_SETEVENTMASK		(WM_USER + 69)
#define EM_SETOLECALLBACK	(WM_USER + 70)
#define EM_SETPARAFORMAT	(WM_USER + 71)
#define EM_SETTARGETDEVICE	(WM_USER + 72)
#define EM_STREAMIN		(WM_USER + 73)
#define EM_STREAMOUT		(WM_USER + 74)
#define EM_GETTEXTRANGE		(WM_USER + 75)
#define EM_FINDWORDBREAK	(WM_USER + 76)
#define EM_SETOPTIONS		(WM_USER + 77)
#define EM_GETOPTIONS		(WM_USER + 78)
#define EM_FINDTEXTEX		(WM_USER + 79)
#define EM_GETWORDBREAKPROCEX	(WM_USER + 80)
#define EM_SETWORDBREAKPROCEX	(WM_USER + 81)

#define EM_SETUNDOLIMIT		(WM_USER + 82)
#define EM_REDO			(WM_USER + 84)
#define EM_CANREDO		(WM_USER + 85)
#define EM_GETUNDONAME		(WM_USER + 86)
#define EM_GETREDONAME		(WM_USER + 87)
#define EM_STOPGROUPTYPING	(WM_USER + 88)

#define EM_SETTEXTMODE		(WM_USER + 89)
#define EM_GETTEXTMODE		(WM_USER + 90)
#define EM_AUTOURLDETECT	(WM_USER + 91)
#define EM_GETAUTOURLDETECT	(WM_USER + 92)
#define EM_SETPALETTE		(WM_USER + 93)
#define EM_GETTEXTEX		(WM_USER + 94)
#define EM_GETTEXTLENGTHEX	(WM_USER + 95)
#define EM_SHOWSCROLLBAR	(WM_USER + 96)
#define EM_SETTEXTEX		(WM_USER + 97)

#define EM_SETPUNCTUATION	(WM_USER + 100)
#define EM_GETPUNCTUATION	(WM_USER + 101)
#define EM_SETWORDWRAPMODE	(WM_USER + 102)
#define EM_GETWORDWRAPMODE	(WM_USER + 103)
#define EM_SETIMECOLOR		(WM_USER + 104)
#define EM_GETIMECOLOR		(WM_USER + 105)
#define EM_SETIMEOPTIONS	(WM_USER + 106)
#define EM_GETIMEOPTIONS	(WM_USER + 107)
#define EM_CONVPOSITION		(WM_USER + 108)

#define EM_SETLANGOPTIONS	(WM_USER + 120)
#define EM_GETLANGOPTIONS	(WM_USER + 121)
#define EM_GETIMECOMPMODE	(WM_USER + 122)

#define EM_SETLANGOPTIONS	(WM_USER + 120)
#define EM_GETLANGOPTIONS	(WM_USER + 121)
#define EM_GETIMECOMPMODE	(WM_USER + 122)

#define EM_FINDTEXTW		(WM_USER + 123)
#define EM_FINDTEXTEXW		(WM_USER + 124)

#define EM_RECONVERSION		(WM_USER + 125)
#define EM_SETIMEMODEBIAS	(WM_USER + 126)
#define EM_GETIMEMODEBIAS	(WM_USER + 127)

#define EM_SETBIDIOPTIONS	(WM_USER + 200)
#define EM_GETBIDIOPTIONS	(WM_USER + 201)

#define EM_SETTYPOGRAPHYOPTIONS (WM_USER + 202)
#define EM_GETTYPOGRAPHYOPTIONS (WM_USER + 203)

#define EM_SETEDITSTYLE		(WM_USER + 204)
#define EM_GETEDITSTYLE		(WM_USER + 205)

#define EM_OUTLINE              (WM_USER + 220)

#define EM_GETSCROLLPOS         (WM_USER + 221)
#define EM_SETSCROLLPOS         (WM_USER + 222)

#define EM_SETFONTSIZE          (WM_USER + 223)
#define EM_GETZOOM		(WM_USER + 224)
#define EM_SETZOOM		(WM_USER + 225)

/* New notifications */
#define EN_MSGFILTER                    0x0700
#define EN_REQUESTRESIZE                0x0701
#define EN_SELCHANGE                    0x0702
#define EN_DROPFILES                    0x0703
#define EN_PROTECTED                    0x0704
#define EN_CORRECTTEXT                  0x0705
#define EN_STOPNOUNDO                   0x0706
#define EN_IMECHANGE                    0x0707
#define EN_SAVECLIPBOARD                0x0708
#define EN_OLEOPFAILED                  0x0709
#define EN_OBJECTPOSITIONS              0x070a
#define EN_LINK				0x070b
#define EN_DRAGDROPDONE                 0x070c
#define EN_PARAGRAPHEXPANDED		0x070d
#define EN_ALIGNLTR			0x0710
#define EN_ALIGNRTL			0x0711


typedef DWORD (CALLBACK * EDITSTREAMCALLBACK)( DWORD, LPBYTE, LONG, LONG * );

/* tab stops number limit */
#define MAX_TAB_STOPS         0x00000020

/* Rich edit control styles */
#define ES_DISABLENOSCROLL    0x00002000
#define ES_SUNKEN             0x00004000
#define ES_SAVESEL            0x00008000
#define ES_SELFIME            0x00040000
#define ES_NOIME              0x00080000
#define ES_VERTICAL           0x00400000
#define ES_SELECTIONBAR       0x01000000
#define ES_EX_NOCALLOLEINIT   0x01000000

/* the character formatting options */
#define SCF_DEFAULT           0x00000000
#define SCF_SELECTION         0x00000001
#define SCF_WORD              0x00000002
#define SCF_ALL               0x00000004
#define SCF_USEUIRULES        0x00000008

/* CHARFORMAT structure */
typedef struct _charformat
{
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    char       szFaceName[LF_FACESIZE];
} CHARFORMATA;

typedef struct _charformatw
{
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    WCHAR      szFaceName[LF_FACESIZE];
} CHARFORMATW;

DECL_WINELIB_TYPE_AW(CHARFORMAT)

typedef struct _charformat2a {
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    char       szFaceName[LF_FACESIZE];
    WORD       wWeight;
    SHORT      sSpacing;
    COLORREF   crBackColor;
    LCID       lcid;
    DWORD      dwReserved;
    SHORT      sStyle;
    WORD       wKerning;
    BYTE       bUnderlineType;
    BYTE       bAnimation;
    BYTE       bRevAuthor;
} CHARFORMAT2A;

typedef struct _charformat2w {
    UINT       cbSize;
    DWORD      dwMask;
    DWORD      dwEffects;
    LONG       yHeight;
    LONG       yOffset;
    COLORREF   crTextColor;
    BYTE       bCharSet;
    BYTE       bPitchAndFamily;
    WCHAR      szFaceName[LF_FACESIZE];
    WORD       wWeight;
    SHORT      sSpacing;
    COLORREF   crBackColor;
    LCID       lcid;
    DWORD      dwReserved;
    SHORT      sStyle;
    WORD       wKerning;
    BYTE       bUnderlineType;
    BYTE       bAnimation;
    BYTE       bRevAuthor;
} CHARFORMAT2W;

DECL_WINELIB_TYPE_AW(CHARFORMAT2)

/* CHARFORMAT masks */
#define CFM_BOLD              0x00000001
#define CFM_ITALIC            0x00000002
#define CFM_UNDERLINE         0x00000004
#define CFM_STRIKEOUT         0x00000008
#define CFM_PROTECTED         0x00000010
#define CFM_LINK              0x00000020
#define CFM_SMALLCAPS         0x00000040
#define CFM_ALLCAPS           0x00000080
#define CFM_HIDDEN            0x00000100
#define CFM_OUTLINE           0x00000200
#define CFM_SHADOW            0x00000400
#define CFM_EMBOSS            0x00000800
#define CFM_IMPRINT           0x00001000
#define CFM_DISABLED          0x00002000
#define CFM_REVISED           0x00004000
#define CFM_REVAUTHOR         0x00008000
#define CFM_SUBSCRIPT         0x00030000
#define CFM_SUPERSCRIPT       0x00030000
#define CFM_ANIMATION         0x00040000
#define CFM_STYLE             0x00080000
#define CFM_KERNING           0x00100000
#define CFM_SPACING           0x00200000
#define CFM_WEIGHT            0x00400000
#define CFM_LCID              0x02000000
#define CFM_BACKCOLOR         0x04000000
#define CFM_CHARSET           0x08000000
#define CFM_OFFSET            0x10000000
#define CFM_FACE              0x20000000
#define CFM_COLOR             0x40000000
#define CFM_SIZE              0x80000000
#define CFM_EFFECTS	      (CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_COLOR | CFM_STRIKEOUT | CFE_PROTECTED | CFM_LINK)

/* CHARFORMAT effects */
#define CFE_BOLD              0x00000001
#define CFE_ITALIC            0x00000002
#define CFE_UNDERLINE         0x00000004
#define CFE_STRIKEOUT         0x00000008
#define CFE_PROTECTED         0x00000010
#define CFE_LINK              0x00000020
#define CFE_SUBSCRIPT         0x00010000
#define CFE_SUPERSCRIPT       0x00020000
#define CFE_AUTOCOLOR         0x40000000

#define CFE_SMALLCAPS         CFM_SMALLCAPS
#define CFE_ALLCAPS           CFM_ALLCAPS
#define CFE_HIDDEN            CFM_HIDDEN
#define CFE_OUTLINE           CFM_OUTLINE
#define CFE_SHADOW            CFM_SHADOW
#define CFE_EMBOSS            CFM_EMBOSS
#define CFE_IMPRINT           CFM_IMPRINT
#define CFE_DISABLED          CFM_DISABLED
#define CFE_REVISED           CFM_REVISED

#define CFU_CF1UNDERLINE      0xFF
#define CFU_INVERT            0xFE
#define CFU_UNDERLINEDOTTED   0x04
#define CFU_UNDERLINEDOUBLE   0x03
#define CFU_UNDERLINEWORD     0x02
#define CFU_UNDERLINE         0x01
#define CFU_UNDERLINENONE     0x00

/* ECO operations */
#define ECOOP_SET             0x0001
#define ECOOP_OR              0x0002
#define ECOOP_AND             0x0003
#define ECOOP_XOR             0x0004

/* edit control options */
#define ECO_AUTOWORDSELECTION 0x00000001
#define ECO_AUTOVSCROLL       0x00000040
#define ECO_AUTOHSCROLL       0x00000080
#define ECO_NOHIDESEL         0x00000100
#define ECO_READONLY          0x00000800
#define ECO_WANTRETURN        0x00001000
#define ECO_SAVESEL           0x00008000
#define ECO_SELECTIONBAR      0x01000000
#define ECO_VERTICAL          0x00400000

/* Event notification masks */
#define ENM_NONE              0x00000000
#define ENM_CHANGE            0x00000001
#define ENM_UPDATE            0x00000002
#define ENM_SCROLL            0x00000004
#define ENM_KEYEVENTS         0x00010000
#define ENM_MOUSEEVENTS       0x00020000
#define ENM_REQUESTRESIZE     0x00040000
#define ENM_SELCHANGE         0x00080000
#define ENM_DROPFILES         0x00100000
#define ENM_PROTECTED         0x00200000
#define ENM_CORRECTTEXT       0x00400000
#define ENM_IMECHANGE         0x00800000
#define ENM_LINK              0x04000000

typedef struct _charrange
{
    LONG    cpMin;
    LONG    cpMax;
} CHARRANGE;

typedef struct _textrange
{
    CHARRANGE chrg;
    LPSTR     lpstrText;
} TEXTRANGEA;

typedef struct _textrangew
{
    CHARRANGE chrg;
    LPWSTR     lpstrText;
} TEXTRANGEW;

DECL_WINELIB_TYPE_AW(TEXTRANGE)

typedef struct _editstream
{
    DWORD		dwCookie;
    DWORD		dwError;
    EDITSTREAMCALLBACK	pfnCallback;
} EDITSTREAM;

typedef struct _compcolor {
    COLORREF   crText;
    COLORREF   crBackground;
    DWORD      dwEffects;
} COMPCOLOR;

typedef struct _encorrecttext {
    NMHDR      nmhdr;
    CHARRANGE  chrg;
    WORD       seltyp;
} ENCORRECTTEXT;

typedef struct _endropfiles {
    NMHDR      nmhdr;
    HANDLE     hDrop;
    LONG       cp;
    BOOL       fProtected;
} ENDROPFILES;

typedef struct _enlink {
    NMHDR      nmhdr;
    UINT       msg;
    WPARAM     wParam;
    LPARAM     lParam;
    CHARRANGE  chrg;
} ENLINK;

typedef struct {
    NMHDR      nmhdr;
    LONG       iob;
    LONG       lOper;
    HRESULT    hr;
} ENOLEOPFAILED;

typedef struct _enprotected {
    NMHDR      nmhdr;
    UINT       msg;
    WPARAM     wParam;
    LPARAM     lParam;
    CHARRANGE  chrg;
} ENPROTECTED, *LPENPROTECTED;

typedef struct _ensaveclipboard {
    NMHDR      nmhdr;
    LONG       cObjectCount;
    LONG       cch;
} ENSAVECLIPBOARD;

typedef struct _findtextA {
    CHARRANGE  chrg;
    LPSTR      lpstrText;
} FINDTEXTA;

typedef struct _findtextW {
    CHARRANGE  chrg;
    LPWSTR     lpstrText;
} FINDTEXTW;

DECL_WINELIB_TYPE_AW(FINDTEXT)

typedef struct _findtextexA {
    CHARRANGE  chrg;
    LPSTR      lpstrText;
    CHARRANGE  chrgText;
} FINDTEXTEXA;

typedef struct _findtextexW {
    CHARRANGE  chrg;
    LPWSTR     lpstrText;
    CHARRANGE  chrgText;
} FINDTEXTEXW;

DECL_WINELIB_TYPE_AW(FINDTEXTEX)

typedef struct _formatrange {
    HDC        hdc;
    HDC        hdcTarget;
    RECT       rc;
    RECT       rcPage;
    CHARRANGE  chrg;
} FORMATRANGE;

typedef struct _msgfilter {
    NMHDR      nmhdr;
    UINT       msg;
    WPARAM     wParam;
    LPARAM     lParam;
} MSGFILTER;

typedef struct _paraformat {
    UINT       cbSize;
    DWORD      dwMask;
    WORD       wNumbering;
    WORD       wReserved;
    LONG       dxStartIndent;
    LONG       dxRightIndent;
    LONG       dxOffset;
    WORD       wAlignment;
    SHORT      cTabCount;
    LONG       rgxTabs[MAX_TAB_STOPS];
} PARAFORMAT;

typedef struct _selchange {
    NMHDR      nmhdr;
    CHARRANGE  chrg;
    WORD       seltyp;
} SELCHANGE;

typedef struct _reqresize {
    NMHDR      nmhdr;
    RECT       rc;
} REQRESIZE;

typedef struct _repastespecial {
    DWORD      dwAspect;
    DWORD      dwParam;
} REPASTESPECIAL;

typedef struct _punctuation {
    UINT       iSize;
    LPSTR      szPunctuation;
} PUNCTUATION;

typedef struct _gettextex {
    DWORD      cb;
    DWORD      flags;
    UINT       codepage;
    LPCSTR     lpDefaultChar;
    LPBOOL     lpUsedDefaultChar;
} GETTEXTEX;

#define SF_TEXT		      0x00000001
#define SF_RTF		      0x00000002
#define SF_RTFNOOBJS	      0x00000003
#define SF_TEXTIZED	      0x00000004
#define SF_UNICODE            0x00000010
#define SF_USECODEPAGE        0x00000020
#define SF_NCRFORNONASCII     0x00000040
#define SF_RTFVAL             0x00000700

/* Clipboard formats */
#define CF_RTF          TEXT("Rich Text Format")
#define CF_RTFNOOBJS    TEXT("Rich Text Format Without Objects")
#define CF_RETEXTOBJ    TEXT("RichEdit Text and Objects")


/* Parameters of the EM_SETIMEOPTIONS message */
#define IMF_FORCENONE         0x00000001
#define IMF_FORCEENABLE       0x00000002
#define IMF_FORCEDISABLE      0x00000004
#define IMF_CLOSESTATUSWINDOW 0x00000008
#define IMF_VERTICAL          0x00000020
#define IMF_FORCEACTIVE       0x00000040
#define IMF_FORCEINACTIVE     0x00000080
#define IMF_FORCEREMEMBER     0x00000100

/* return values of the EM_SELECTION_TYPE message */
#define SEL_EMPTY             0x00000000
#define SEL_TEXT              0x00000001
#define SEL_OBJECT            0x00000002
#define SEL_MULTICHAR         0x00000004
#define SEL_MULTIOBJECT       0x00000008

/* mask values in the PARAFORMAT2 structure */
#define PFM_STARTINDENT       0x00000001
#define PFM_RIGHTINDENT       0x00000002
#define PFM_OFFSET            0x00000004
#define PFM_ALIGNMENT         0x00000008
#define PFM_TABSTOPS          0x00000010
#define PFM_NUMBERING         0x00000020
#define PFM_OFFSETINDENT      0x80000000

/* numbering option */
#define PFN_BULLET            0x00000001

/* paragraph alignment */
#define PFA_LEFT              0x00000001
#define PFA_RIGHT             0x00000002
#define PFA_CENTER            0x00000003

/* streaming flags */
#define SFF_PWD               0x00000800
#define SFF_KEEPDOCINFO       0x00001000
#define SFF_PERSISTVIEWSCALE  0x00002000
#define SFF_PLAINRTF          0x00004000
#define SFF_SELECTION         0x00008000

typedef LONG (*EDITWORDBREAKPROCEX)(char*,LONG,BYTE,INT);

/* options of the EM_FINDWORDBREAK message */
#define WB_CLASSIFY           0x00000003
#define WB_MOVEWORDLEFT       0x00000004
#define WB_MOVEWORDPREV       0x00000004
#define WB_MOVEWORDRIGHT      0x00000005
#define WB_MOVEWORDNEXT       0x00000005
#define WB_LEFTBREAK          0x00000006
#define WB_PREVBREAK          0x00000006
#define WB_RIGHTBREAK         0x00000007
#define WB_NEXTBREAK          0x00000007

/* options of the EM_SETWORDWRAPMODE message */
#define WBF_WORDWRAP          0x00000010
#define WBF_WORDBREAK         0x00000020
#define WBF_OVERFLOW          0x00000040
#define WBF_LEVEL1            0x00000080
#define WBF_LEVEL2            0x00000100
#define WBF_CUSTOM            0x00000200

/* options of the EM_SETTEXTMODE message */
#define TM_PLAINTEXT          0x00000001
#define TM_RICHTEXT           0x00000002
#define TM_SINGLELEVELUNDO    0x00000004
#define TM_MULTILEVELUNDO     0x00000008
#define TM_SINGLECODEPAGE     0x00000010
#define TM_MULTICODEPAGE      0x00000020

/* GETTEXT structure flags */
#define GT_DEFAULT            0x00000000
#define GT_USECRLF            0x00000001

/* Options of the EM_SETTYPOGRAPHYOPTIONS message */
#define TO_ADVANCEDTYPOGRAPHY 0x00000001
#define TO_SIMPLELINEBREAK    0x00000002

typedef struct _gettextlengthex {
    DWORD      flags;
    UINT       codepage;
} GETTEXTLENGTHEX;

/* Flags of the GETTEXTLENGTHEX structure */
#define GTL_DEFAULT           0x00000000
#define GTL_USECRLF           0x00000001
#define GTL_PRECISE           0x00000002
#define GTL_CLOSE             0x00000004
#define GTL_NUMCHARS          0x00000008
#define GTL_NUMBYTES          0x00000010

#ifdef __cplusplus
}
#endif

#include "poppack.h"

#endif /* __WINE_RICHEDIT_H */
