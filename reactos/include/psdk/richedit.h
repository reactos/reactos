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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_RICHEDIT_H
#define __WINE_RICHEDIT_H

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER   0x0210
#endif /* _RICHEDIT_VER */

#define cchTextLimitDefault 0x7fff

#if defined(__GNUC__)
# define MSFTEDIT_CLASS (const WCHAR []){ 'R','i','c','h','E','d','i','t','5','0','W',0 }
#elif defined(_MSC_VER)
# define MSFTEDIT_CLASS L"RichEdit50W"
#else
static const WCHAR MSFTEDIT_CLASS[] = { 'R','i','c','h','E','d','i','t','5','0','W',0 };
#endif

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

#ifndef WM_NOTIFY
#define WM_NOTIFY               0x004e
#endif
#ifndef WM_CONTEXTMENU
#define WM_CONTEXTMENU          0x007b
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR              0x0109
#endif
#ifndef WM_PRINTCLIENT
#define WM_PRINTCLIENT          0x0318
#endif

#ifndef EM_GETLIMITTEXT
#define EM_GETLIMITTEXT         (WM_USER + 37)
#endif
#ifndef EM_POSFROMCHAR
#define EM_POSFROMCHAR          (WM_USER + 38)
#define EM_CHARFROMPOS          (WM_USER + 39)
#endif
#ifndef EM_SCROLLCARET
#define EM_SCROLLCARET		(WM_USER + 49)
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
#define EM_GETVIEWKIND          (WM_USER + 226)
#define EM_SETVIEWKIND          (WM_USER + 227)

#define EM_GETPAGE              (WM_USER + 228)
#define EM_SETPAGE              (WM_USER + 229)
#define EM_GETHYPHENATEINFO     (WM_USER + 230)
#define EM_SETHYPHENATEINFO     (WM_USER + 231)
#define EM_GETPAGEROTATE        (WM_USER + 235)
#define EM_SETPAGEROTATE        (WM_USER + 236)
#define EM_GETCTFMODEBIAS       (WM_USER + 237)
#define EM_SETCTFMODEBIAS       (WM_USER + 238)
#define EM_GETCTFOPENSTATUS     (WM_USER + 240)
#define EM_SETCTFOPENSTATUS     (WM_USER + 241)
#define EM_GETIMECOMPTEXT       (WM_USER + 242)
#define EM_ISIME                (WM_USER + 243)
#define EM_GETIMEPROPERTY       (WM_USER + 244)
#define EM_GETQUERYRTFOBJ       (WM_USER + 269)
#define EM_SETQUERYRTFOBJ       (WM_USER + 270)

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
#define EN_PAGECHANGE                   0x070e
#define EN_LOWFIRTF                     0x070f
#define EN_ALIGNLTR			0x0710
#define EN_ALIGNRTL			0x0711


typedef DWORD (CALLBACK * EDITSTREAMCALLBACK)( DWORD_PTR, LPBYTE, LONG, LONG * );


#define yHeightCharPtsMost    1638
#define lDefaultTab           720

/* tab stops number limit */
#define MAX_TAB_STOPS         0x00000020

#define MAX_TABLE_CELLS       63

/* Rich edit control styles */
#define ES_NOOLEDRAGDROP      0x00000008
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
#define SCF_ASSOCIATEFONT     0x00000010
#define SCF_NOKBUPDATE        0x00000020
#define SCF_ASSOCIATEFONT2    0x00000040

#ifndef WM_NOTIFY
typedef struct _nmhdr
{
    HWND       hwndFrom;
    UINT       idFrom;
    UINT       code;
} NMHDR;
#endif

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

#define CHARFORMATDELTA       (sizeof(CHARFORMAT2) - sizeof(CHARFORMAT))

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
#define CFM_UNDERLINETYPE     0x00800000
#define CFM_LCID              0x02000000
#define CFM_BACKCOLOR         0x04000000
#define CFM_CHARSET           0x08000000
#define CFM_OFFSET            0x10000000
#define CFM_FACE              0x20000000
#define CFM_COLOR             0x40000000
#define CFM_SIZE              0x80000000

#define CFM_EFFECTS           (CFM_BOLD | \
                               CFM_ITALIC | \
                               CFM_UNDERLINE | \
                               CFM_COLOR | \
                               CFM_STRIKEOUT | \
                               CFE_PROTECTED | \
                               CFM_LINK)

#define CFM_EFFECTS2          (CFM_EFFECTS | \
                               CFM_DISABLED | \
                               CFM_SMALLCAPS | \
                               CFM_ALLCAPS | \
                               CFM_HIDDEN | \
                               CFM_OUTLINE | \
                               CFM_SHADOW | \
                               CFM_EMBOSS | \
                               CFM_IMPRINT | \
                               CFM_DISABLED | \
                               CFM_REVISED | \
                               CFM_SUBSCRIPT | \
                               CFM_SUPERSCRIPT | \
                               CFM_BACKCOLOR)

#define CFM_ALL               (CFM_EFFECTS | \
                               CFM_SIZE | \
                               CFM_FACE | \
                               CFM_OFFSET | \
                               CFM_CHARSET)

#define CFM_ALL2              (CFM_ALL | \
                               CFM_EFFECTS2 | \
                               CFM_BACKCOLOR | \
                               CFM_LCID | \
                               CFM_UNDERLINETYPE | \
                               CFM_WEIGHT | \
                               CFM_REVAUTHOR | \
                               CFM_SPACING | \
                               CFM_KERNING | \
                               CFM_STYLE | \
                               CFM_ANIMATION)

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
#define CFE_AUTOBACKCOLOR     CFM_BACKCOLOR

#define CFU_UNDERLINENONE             0x00
#define CFU_UNDERLINE                 0x01
#define CFU_UNDERLINEWORD             0x02
#define CFU_UNDERLINEDOUBLE           0x03
#define CFU_UNDERLINEDOTTED           0x04
#define CFU_UNDERLINEDASH             0x05
#define CFU_UNDERLINEDASHDOT          0x06
#define CFU_UNDERLINEDASHDOTDOT       0x07
#define CFU_UNDERLINEWAVE             0x08
#define CFU_UNDERLINETHICK            0x09
#define CFU_UNDERLINEHAIRLINE         0x0a
#define CFU_UNDERLINEDOUBLEWAVE       0x0b
#define CFU_UNDERLINEHEAVYWAVE        0x0c
#define CFU_UNDERLINELONGDASH         0x0d
#define CFU_UNDERLINETHICKDASH        0x0e
#define CFU_UNDERLINETHICKDASHDOT     0x0f
#define CFU_UNDERLINETHICKDASHDOTDOT  0x10
#define CFU_UNDERLINETHICKDOTTED      0x11
#define CFU_UNDERLINETHICKLONGDASH    0x12
#define CFU_INVERT                    0xFE
#define CFU_CF1UNDERLINE              0xFF

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
#define ENM_SCROLLEVENTS      0x00000008
#define ENM_DRAGDROPDONE      0x00000010
#define ENM_PARAGRAPHEXPANDED 0x00000020
#define ENM_PAGECHANGE        0x00000040
#define ENM_KEYEVENTS         0x00010000
#define ENM_MOUSEEVENTS       0x00020000
#define ENM_REQUESTRESIZE     0x00040000
#define ENM_SELCHANGE         0x00080000
#define ENM_DROPFILES         0x00100000
#define ENM_PROTECTED         0x00200000
#define ENM_CORRECTTEXT       0x00400000
#define ENM_IMECHANGE         0x00800000
#define ENM_LANGCHANGE        0x01000000
#define ENM_OBJECTPOSITIONS   0x02000000
#define ENM_LINK              0x04000000
#define ENM_LOWFIRTF          0x08000000

typedef struct _bidioptions
{
    UINT    cbSize;
    WORD    wMask;
    WORD    wEffects;
} BIDIOPTIONS;

#ifndef __RICHEDIT_CHARRANGE_DEFINED
#define __RICHEDIT_CHARRANGE_DEFINED

typedef struct _charrange
{
    LONG    cpMin;
    LONG    cpMax;
} CHARRANGE;

#endif /* __RICHEDIT_CHARRANGE_DEFINED */

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
    DWORD_PTR		dwCookie;
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

typedef struct _enlowfirtf {
    NMHDR      nmhdr;
    char       *szControl;
} ENLOWFIRTF;

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
    LPCSTR     lpstrText;
} FINDTEXTA;

typedef struct _findtextW {
    CHARRANGE  chrg;
    LPCWSTR    lpstrText;
} FINDTEXTW;

DECL_WINELIB_TYPE_AW(FINDTEXT)

typedef struct _findtextexA {
    CHARRANGE  chrg;
    LPCSTR     lpstrText;
    CHARRANGE  chrgText;
} FINDTEXTEXA;

typedef struct _findtextexW {
    CHARRANGE  chrg;
    LPCWSTR    lpstrText;
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

typedef enum tagKHYPH
{
    khyphNil           = 0,
    khyphNormal        = 1,
    khyphAddBefore     = 2,
    khyphChangeBefore  = 3,
    khyphDeleteBefore  = 4,
    khyphChangeAfter   = 5,
    khyphDelAndChange  = 6
} KHYPH;

typedef struct hyphresult
{
    KHYPH      khyph;
    long       ichHyph;
    WCHAR      chHyph;
} HYPHRESULT;

typedef struct tagHyphenateInfo
{
    SHORT      cbSize;
    SHORT      dxHyphenateZone;
    void       (WINAPI* pfnHyphenate)(WCHAR*, LANGID, long, HYPHRESULT*);
} HYPHENATEINFO;

typedef struct _msgfilter {
    NMHDR      nmhdr;
    UINT       msg;
    WPARAM     wParam;
    LPARAM     lParam;
} MSGFILTER;

typedef struct _objectpositions {
    NMHDR      nmhdr;
    LONG       cObjectCount;
    LONG       *pcpPositions;
} OBJECTPOSITIONS;

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

typedef struct _paraformat2 {
    UINT       cbSize;
    DWORD      dwMask;
    WORD       wNumbering;
    WORD       wEffects;
    LONG       dxStartIndent;
    LONG       dxRightIndent;
    LONG       dxOffset;
    WORD       wAlignment;
    SHORT      cTabCount;
    LONG       rgxTabs[MAX_TAB_STOPS];
    LONG       dySpaceBefore, dySpaceAfter, dyLineSpacing;
    SHORT      sStyle;
    BYTE       bLineSpacingRule, bOutlineLevel;
    WORD       wShadingWeight, wShadingStyle;
    WORD       wNumberingStart, wNumberingStyle, wNumberingTab;
    WORD       wBorderSpace, wBorderWidth, wBorders;
} PARAFORMAT2;

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
    LPBOOL     lpUsedDefChar;
} GETTEXTEX;

typedef struct _imecomptext {
    LONG       cb;
    DWORD      flags;
} IMECOMPTEXT;

void WINAPI HyphenateProc(WCHAR*, LANGID, long, HYPHRESULT*);

#define SF_TEXT		      0x00000001
#define SF_RTF		      0x00000002
#define SF_RTFNOOBJS	      0x00000003
#define SF_TEXTIZED	      0x00000004
#define SF_UNICODE            0x00000010
#define SF_USECODEPAGE        0x00000020
#define SF_NCRFORNONASCII     0x00000040
#define SF_RTFVAL             0x00000700

/* BIDIOPTIONS.wMask flag values */
#define BOM_DEFPARADIR        0x00000001
#define BOM_PLAINTEXT         0x00000002
#define BOM_NEUTRALOVERRIDE   0x00000004
#define BOM_CONTEXTREADING    0x00000008
#define BOM_CONTEXTALIGNMENT  0x00000010
#define BOM_LEGACYBIDICLASS   0x00000040

/* BIDIOPTIONS.wEffects flag values */
#define BOE_RTLDIR            0x00000001
#define BOE_PLAINTEXT         0x00000002
#define BOE_NEUTRALOVERRIDE   0x00000004
#define BOE_CONTEXTREADING    0x00000008
#define BOE_CONTEXTALIGNMENT  0x00000010
#define BOE_LEGACYBIDICLASS   0x00000040

/* Clipboard formats */
#define CF_RTF          TEXT("Rich Text Format")
#define CF_RTFNOOBJS    TEXT("Rich Text Format Without Objects")
#define CF_RETEXTOBJ    TEXT("RichEdit Text and Objects")

/* Mode bias wParam values for EM_SETCTFMODEBIAS message */
#define CTFMODEBIAS_DEFAULT                0x00000000
#define CTFMODEBIAS_FILENAME               0x00000001
#define CTFMODEBIAS_NAME                   0x00000002
#define CTFMODEBIAS_READING                0x00000003
#define CTFMODEBIAS_DATETIME               0x00000004
#define CTFMODEBIAS_CONVERSATION           0x00000005
#define CTFMODEBIAS_NUMERIC                0x00000006
#define CTFMODEBIAS_HIRAGANA               0x00000007
#define CTFMODEBIAS_KATAKANA               0x00000008
#define CTFMODEBIAS_HANGUL                 0x00000009
#define CTFMODEBIAS_HALFWIDTHKATAKANA      0x0000000a
#define CTFMODEBIAS_FULLWIDTHALPHANUMERIC  0x0000000b
#define CTFMODEBIAS_HALFWIDTHALPHANUMERIC  0x0000000c

#define EMO_EXIT              0x00000000
#define EMO_ENTER             0x00000001
#define EMO_PROMOTE           0x00000002
#define EMO_EXPAND            0x00000003
#define EMO_MOVESELECTION     0x00000004
#define EMO_GETVIEWMODE       0x00000005

#define EMO_EXPANDSELECTION   0x00000000
#define EMO_EXPANDDOCUMENT    0x00000001

/* Page Rotate values used in wParam of EM_SETPAGEROTATE message */
#define EPR_0                 0x00000000
#define EPR_270               0x00000001
#define EPR_180               0x00000002
#define EPR_90                0x00000003

/* Find flags for wParam of EM_FINDTEXT message */
#define FR_MATCHDIAC          0x20000000
#define FR_MATCHKASHIDA       0x40000000
#define FR_MATCHALEFHAMZA     0x80000000

/* IME Compatibility Mode return values for EM_GETIMECOMPMODE message */
#define ICM_NOTOPEN           0x00000000
#define ICM_LEVEL3            0x00000001
#define ICM_LEVEL2            0x00000002
#define ICM_LEVEL2_5          0x00000003
#define ICM_LEVEL2_SUI        0x00000004
#define ICM_CTF               0x00000005

/* Flags value for IMECOMPTEXT structure */
#define ICT_RESULTREADSTR     0x00000001

/* Input Method Flags used in EM_SETLANGOPTIONS message */
#define IMF_AUTOKEYBOARD        0x00000001
#define IMF_AUTOFONT            0x00000002
#define IMF_IMECANCELCOMPLETE   0x00000004
#define IMF_IMEALWAYSSENDNOTIFY 0x00000008
#define IMF_AUTOFONTSIZEADJUST  0x00000010
#define IMF_UIFONTS             0x00000020
#define IMF_DUALFONT            0x00000080

/* Parameters values for the EM_SETIMEMODEBIAS message */
#define IMF_SMODE_PLAURALCLAUSE 0x00000001
#define IMF_SMODE_NONE          0x00000002

/* Parameters of the EM_SETIMEOPTIONS message */
#define IMF_FORCENONE         0x00000001
#define IMF_FORCEENABLE       0x00000002
#define IMF_FORCEDISABLE      0x00000004
#define IMF_CLOSESTATUSWINDOW 0x00000008
#define IMF_VERTICAL          0x00000020
#define IMF_FORCEACTIVE       0x00000040
#define IMF_FORCEINACTIVE     0x00000080
#define IMF_FORCEREMEMBER     0x00000100
#define IMF_MULTIPLEEDIT      0x00000400

/* return values of the EM_SELECTION_TYPE message */
#define SEL_EMPTY             0x00000000
#define SEL_TEXT              0x00000001
#define SEL_OBJECT            0x00000002
#define SEL_MULTICHAR         0x00000004
#define SEL_MULTIOBJECT       0x00000008

/* ENOLEOPFAILED.lOper value that indicates operation failure */
#define OLEOP_DOVERB          0x00000001

/* punctionation type values for wParam of EM_SETPUNCTUATION message */
#define PC_FOLLOWING          0x00000001
#define PC_LEADING            0x00000002
#define PC_OVERFLOW           0x00000003
#define PC_DELIMITER          0x00000004

/* mask values in the PARAFORMAT structure */
#define PFM_STARTINDENT       0x00000001
#define PFM_RIGHTINDENT       0x00000002
#define PFM_OFFSET            0x00000004
#define PFM_ALIGNMENT         0x00000008
#define PFM_TABSTOPS          0x00000010
#define PFM_NUMBERING         0x00000020
#define PFM_OFFSETINDENT      0x80000000

/* mask values in the PARAFORMAT2 structure */
#define PFM_SPACEBEFORE       0x00000040
#define PFM_SPACEAFTER        0x00000080
#define PFM_LINESPACING       0x00000100
#define PFM_STYLE             0x00000400
#define PFM_BORDER            0x00000800
#define PFM_SHADING           0x00001000
#define PFM_NUMBERINGSTYLE    0x00002000
#define PFM_NUMBERINGTAB      0x00004000
#define PFM_NUMBERINGSTART    0x00008000
#define PFM_RTLPARA           0x00010000
#define PFM_KEEP              0x00020000
#define PFM_KEEPNEXT          0x00040000
#define PFM_PAGEBREAKBEFORE   0x00080000
#define PFM_NOLINENUMBER      0x00100000
#define PFM_NOWIDOWCONTROL    0x00200000
#define PFM_DONOTHYPHEN       0x00400000
#define PFM_SIDEBYSIDE        0x00800000
#define PFM_COLLAPSED         0x01000000
#define PFM_OUTLINELEVEL      0x02000000
#define PFM_BOX               0x04000000
#define PFM_RESERVED2         0x08000000
#define PFM_TABLEROWDELIMITER 0x10000000
#define PFM_TEXTWRAPPINGBREAK 0x20000000
#define PFM_TABLE             0x40000000

#define PFM_ALL               (PFM_STARTINDENT | \
                               PFM_RIGHTINDENT | \
                               PFM_OFFSET | \
                               PFM_ALIGNMENT | \
                               PFM_TABSTOPS | \
                               PFM_NUMBERING | \
                               PFM_OFFSETINDENT | \
                               PFM_RTLPARA)

#define PFM_EFFECTS           (PFM_RTLPARA | \
                               PFM_KEEP | \
                               PFM_KEEPNEXT | \
                               PFM_PAGEBREAKBEFORE | \
                               PFM_NOLINENUMBER | \
                               PFM_NOWIDOWCONTROL | \
                               PFM_DONOTHYPHEN | \
                               PFM_SIDEBYSIDE | \
                               PFM_TABLEROWDELIMITER | \
                               PFM_TABLE)

#define PFM_ALL2              (PFM_ALL | \
                               PFM_EFFECTS | \
                               PFM_SPACEBEFORE | \
                               PFM_SPACEAFTER | \
                               PFM_LINESPACING | \
                               PFM_STYLE | \
                               PFM_BORDER | \
                               PFM_SHADING | \
                               PFM_NUMBERINGSTYLE | \
                               PFM_NUMBERINGTAB | \
                               PFM_NUMBERINGSTART)

/* numbering option */
#define PFN_BULLET            0x00000001
#define PFN_ARABIC            0x00000002
#define PFN_LCLETTER          0x00000003
#define PFN_UCLETTER          0x00000004
#define PFN_LCROMAN           0x00000005
#define PFN_UCROMAN           0x00000006

/* paragraph format numbering styles */
#define PFNS_PAREN            0x00000000
#define PFNS_PARENS           0x00000100
#define PFNS_PERIOD           0x00000200
#define PFNS_PLAIN            0x00000300
#define PFNS_NONUMBER         0x00000400
#define PFNS_NEWNUMBER        0x00008000

/* paragraph alignment */
#define PFA_LEFT              0x00000001
#define PFA_RIGHT             0x00000002
#define PFA_CENTER            0x00000003
#define PFA_JUSTIFY           0x00000004
#define PFA_FULL_INTERWORD    0x00000004
#define PFA_FULL_INTERLETTER  0x00000005
#define PFA_FULL_SCALED       0x00000006
#define PFA_FULL_GLYPHS       0x00000007
#define PFA_SNAP_GRID         0x00000008

/* paragraph effects */
#define PFE_RTLPARA           0x00000001
#define PFE_KEEP              0x00000002
#define PFE_KEEPNEXT          0x00000004
#define PFE_PAGEBREAKBEFORE   0x00000008
#define PFE_NOLINENUMBER      0x00000010
#define PFE_NOWIDOWCONTROL    0x00000020
#define PFE_DONOTHYPHEN       0x00000040
#define PFE_SIDEBYSIDE        0x00000080
#define PFE_COLLAPSED         0x00000100
#define PFE_BOX               0x00000400
#define PFE_TABLEROWDELIMITER 0x00001000
#define PFE_TEXTWRAPPINGBREAK 0x00002000
#define PFE_TABLE             0x00004000

/* Set Edit Style flags for EM_SETEDITSTYLE message */
#define SES_EMULATESYSEDIT      0x00000001
#define SES_BEEPONMAXTEXT       0x00000002
#define SES_EXTENDBACKCOLOR     0x00000004
#define SES_MAPCPS              0x00000008
#define SES_EMULATE10           0x00000010
#define SES_USECRLF             0x00000020
#define SES_NOXLTSYMBOLRANGE    0x00000020
#define SES_USEAIMM             0x00000040
#define SES_NOIME               0x00000080
#define SES_ALLOWBEEPS          0x00000100
#define SES_UPPERCASE           0x00000200
#define SES_LOWERCASE           0x00000400
#define SES_NOINPUTSEQUENCECHK  0x00000800
#define SES_BIDI                0x00001000
#define SES_SCROLLONKILLFOCUS   0x00002000
#define SES_XLTCRCRLFTOCR       0x00004000
#define SES_DRAFTMODE           0x00008000
#define SES_USECTF              0x00010000
#define SES_HIDEGRIDLINES       0x00020000
#define SES_USEATFONT           0x00040000
#define SES_CUSTOMLOOK          0x00080000
#define SES_LBSCROLLNOTIFY      0x00100000
#define SES_CTFALLOWEMBED       0x00200000
#define SES_CTFALLOWSMARTTAG    0x00400000
#define SES_CTFALLOWPROOFING    0x00800000

/* streaming flags */
#define SFF_WRITEXTRAPAR      0x00000080
#define SFF_PWD               0x00000800
#define SFF_KEEPDOCINFO       0x00001000
#define SFF_PERSISTVIEWSCALE  0x00002000
#define SFF_PLAINRTF          0x00004000
#define SFF_SELECTION         0x00008000

typedef enum _undonameid
{
    UID_UNKNOWN     = 0,
    UID_TYPING      = 1,
    UID_DELETE      = 2,
    UID_DRAGDROP    = 3,
    UID_CUT         = 4,
    UID_PASTE       = 5,
    UID_AUTOCORRECT = 6
} UNDONAMEID;

typedef LONG (*EDITWORDBREAKPROCEX)(char*,LONG,BYTE,INT);

#define VM_OUTLINE            0x00000002
#define VM_NORMAL             0x00000004
#define VM_PAGE               0x00000009

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

#define WBF_CLASS             ((BYTE) 0x0F)
#define WBF_ISWHITE           ((BYTE) 0x10)
#define WBF_BREAKLINE         ((BYTE) 0x20)
#define WBF_BREAKAFTER        ((BYTE) 0x40)

/* Placeholder unicode character for an embedded object */
#ifndef WCH_EMBEDDING
#define WCH_EMBEDDING         (WCHAR)0xFFFC
#endif

/* options of the EM_SETTEXTMODE message */
#define TM_PLAINTEXT          0x00000001
#define TM_RICHTEXT           0x00000002
#define TM_SINGLELEVELUNDO    0x00000004
#define TM_MULTILEVELUNDO     0x00000008
#define TM_SINGLECODEPAGE     0x00000010
#define TM_MULTICODEPAGE      0x00000020

/* GETTEXTEX structure flags */
#define GT_DEFAULT            0x00000000
#define GT_USECRLF            0x00000001
#define GT_SELECTION          0x00000002
#define GT_RAWTEXT            0x00000004
#define GT_NOHIDDENTEXT       0x00000008

/* Options of the EM_SETTYPOGRAPHYOPTIONS message */
#define TO_ADVANCEDTYPOGRAPHY   0x00000001
#define TO_SIMPLELINEBREAK      0x00000002
#define TO_DISABLECUSTOMTEXTOUT 0x00000004
#define TO_ADVANCEDLAYOUT       0x00000008

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

#define GCM_RIGHTMOUSEDROP    0x8000

/* Options of the EM_SETTEXTEX message */
typedef struct _settextex {
    DWORD	flags;
    UINT	codepage;
} SETTEXTEX;

/* Flags of the EM_SETTEXTEX message */
#define ST_DEFAULT	      0x00000000	
#define ST_KEEPUNDO           0x00000001
#define ST_SELECTION          0x00000002

#define ST_NEWCHARS           0x00000004

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_RICHEDIT_H */
