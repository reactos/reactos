/*
 *  TEXTEDIT.H
 *
 *
 *  Copyright (c) 1985-1996, Microsoft Corporation
 */

#ifndef I_TEXTEDIT_H_
#define I_TEXTEDIT_H_
#pragma INCMSG("--- Beg 'textedit.h'")

#ifdef _WIN32
#include <pshpack4.h>
#elif !defined(RC_INVOKED)
#pragma pack(4)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/*
 *  To make some structures which can be passed between 16 and 32 bit windows
 *  almost compatible, padding is introduced to the 16 bit versions of the
 *  structure.
 */
#ifdef _WIN32
#   define  _WPAD   /##/
#else
#   define  _WPAD   WORD
#endif

#if defined(UNIX) || defined(_MAC)
#  define WPAD1
#  define WPAD2
#  define WPAD3

#else
#  define WPAD1 _WPAD _wPad1;
#  define WPAD2 _WPAD _wPad2;
#  define WPAD3 _WPAD _wPad3;

#endif


#define cchTextLimitDefault 32767

/* Richedit2.0 Window Class. */

#ifndef MACPORTREMOVE   // original version

#define RICHEDIT_CLASSA     "RichEdit20A"
#define RICHEDIT_CLASSW     L"RichEdit20W"

#ifdef UNICODE
#define RICHEDIT_CLASS      RICHEDIT_CLASSW
#else
#define RICHEDIT_CLASS      RICHEDIT_CLASSA
#endif // UNICODE

#else   //MACPORTREMOVE

#define RICHEDIT_CLASSA     "RichEdit20A"
#define RICHEDIT_CLASSW     TEXT("RichEdit20W") // MACPORT change
#define RICHEDIT_CLASS10A   "RICHEDIT"          // MACPORT add
#define RICHEDIT_CLASS10W   TEXT("RICHEDIT")    // MACPORT add

#ifdef UNICODE
 #define RICHEDIT_CLASS     RICHEDIT_CLASSW
 #define RICHEDIT_CLASS10   RICHEDIT_CLASS10W   // MACPORT add
#else
 #define RICHEDIT_CLASS     RICHEDIT_CLASSA
 #define RICHEDIT_CLASS10   RICHEDIT_CLASS10A   // MACPORT add
#endif // UNICODE

#endif // MACPORTREMOVE


/* RichEdit messages */

#ifndef WM_CONTEXTMENU
#define WM_CONTEXTMENU          0x007B
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
#define EM_SCROLLCARET          (WM_USER + 49)
#endif
#define EM_CANPASTE             (WM_USER + 50)
#define EM_DISPLAYBAND          (WM_USER + 51)
#define EM_EXGETSEL             (WM_USER + 52)
#define EM_EXLIMITTEXT          (WM_USER + 53)
#define EM_EXLINEFROMCHAR       (WM_USER + 54)
#define EM_EXSETSEL             (WM_USER + 55)
#define EM_FINDTEXT             (WM_USER + 56)
#define EM_FORMATRANGE          (WM_USER + 57)
#define EM_GETCHARFORMAT        (WM_USER + 58)
#define EM_GETEVENTMASK         (WM_USER + 59)
#define EM_GETOLEINTERFACE      (WM_USER + 60)
#define EM_GETPARAFORMAT        (WM_USER + 61)
#define EM_GETSELTEXT           (WM_USER + 62)
#define EM_HIDESELECTION        (WM_USER + 63)
#define EM_PASTESPECIAL         (WM_USER + 64)
#define EM_REQUESTRESIZE        (WM_USER + 65)
#define EM_SELECTIONTYPE        (WM_USER + 66)
#define EM_SETBKGNDCOLOR        (WM_USER + 67)
#define EM_SETCHARFORMAT        (WM_USER + 68)
#define EM_SETEVENTMASK         (WM_USER + 69)
#define EM_SETOLECALLBACK       (WM_USER + 70)
#define EM_SETPARAFORMAT        (WM_USER + 71)
#define EM_SETTARGETDEVICE      (WM_USER + 72)
#define EM_STREAMIN             (WM_USER + 73)
#define EM_STREAMOUT            (WM_USER + 74)
#define EM_GETTEXTRANGE         (WM_USER + 75)
#define EM_FINDWORDBREAK        (WM_USER + 76)
#define EM_SETOPTIONS           (WM_USER + 77)
#define EM_GETOPTIONS           (WM_USER + 78)
#define EM_FINDTEXTEX           (WM_USER + 79)
#ifdef _WIN32
#define EM_GETWORDBREAKPROCEX   (WM_USER + 80)
#define EM_SETWORDBREAKPROCEX   (WM_USER + 81)
#endif

/* Richedit v2.0 messages */
#define EM_SETUNDOLIMIT         (WM_USER + 82)
#define EM_REDO                 (WM_USER + 84)
#define EM_CANREDO              (WM_USER + 85)
#define EM_GETUNDONAME          (WM_USER + 86)
#define EM_GETREDONAME          (WM_USER + 87)
#define EM_STOPGROUPTYPING      (WM_USER + 88)

#define EM_SETTEXTMODE          (WM_USER + 89)
#define EM_GETTEXTMODE          (WM_USER + 90)

/* enum for use with EM_GET/SETTEXTMODE */
typedef enum tagTextMode
{
    TM_PLAINTEXT    =   1,
    TM_RICHTEXT     =   2
} TEXTMODE;


/* Far East specific messages */
#define EM_SETPUNCTUATION       (WM_USER + 100)
#define EM_GETPUNCTUATION       (WM_USER + 101)
#define EM_SETWORDWRAPMODE      (WM_USER + 102)
#define EM_GETWORDWRAPMODE      (WM_USER + 103)
#define EM_SETIMECOLOR          (WM_USER + 104)
#define EM_GETIMECOLOR          (WM_USER + 105)
#define EM_SETIMEOPTIONS        (WM_USER + 106)
#define EM_GETIMEOPTIONS        (WM_USER + 107)
#define EM_CONVPOSITION         (WM_USER + 108)

#define EM_SETLANGOPTIONS       (WM_USER + 120)
#define EM_GETLANGOPTIONS       (WM_USER + 121)
#define EM_GETIMECOMPMODE       (WM_USER + 122)

/* Options for EM_SETLANGOPTIONS and EM_GETLANGOPTIONS */
#define IMF_AUTOKEYBOARD        0x0001
#define IMF_AUTOFONT            0x0002

/* Values for EM_GETIMECOMPMODE */
#define ICM_NOTOPEN             0x0000
#define ICM_LEVEL3              0x0001
#define ICM_LEVEL2              0x0002
#define ICM_LEVEL2_5            0x0003
#define ICM_LEVEL2_SUI          0x0004

/* New notifications */

#define EN_MSGFILTER            0x0700
#define EN_REQUESTRESIZE        0x0701
#define EN_SELCHANGE            0x0702
#define EN_DROPFILES            0x0703
#define EN_PROTECTED            0x0704
#define EN_CORRECTTEXT          0x0705          /* PenWin specific */
#define EN_STOPNOUNDO           0x0706
#define EN_IMECHANGE            0x0707          /* Far East specific */
#define EN_SAVECLIPBOARD        0x0708
#define EN_OLEOPFAILED          0x0709
#define EN_OBJECTPOSITIONS      0x070a
#define EN_LINK                 0x070b

/* Event notification masks */

#define ENM_NONE                0x00000000
#define ENM_CHANGE              0x00000001
#define ENM_UPDATE              0x00000002
#define ENM_SCROLL              0x00000004
#define ENM_KEYEVENTS           0x00010000
#define ENM_MOUSEEVENTS         0x00020000
#define ENM_REQUESTRESIZE       0x00040000
#define ENM_SELCHANGE           0x00080000
#define ENM_DROPFILES           0x00100000
#define ENM_PROTECTED           0x00200000
#define ENM_CORRECTTEXT         0x00400000      /* PenWin specific */

/* Far East specific notification mask */
#define ENM_IMECHANGE           0x00800000
#define ENM_LANGCHANGE          0x01000000
#define ENM_OBJECTPOSITIONS     0x02000000
#define ENM_LINK                0x04000000

/* New edit control styles */

#define ES_SAVESEL              0x00008000
#define ES_SUNKEN               0x00004000
#define ES_DISABLENOSCROLL      0x00002000
/* same as WS_MAXIMIZE, but that doesn't make sense so we re-use the value */
#define ES_SELECTIONBAR         0x01000000

/* New edit control extended style */
#ifdef  _WIN32
#define ES_EX_NOCALLOLEINIT     0x01000000
#endif

/* These flags are used in FE Windows */
#define ES_VERTICAL             0x00400000
#define ES_NOIME                0x00080000
#define ES_SELFIME              0x00040000

/* Edit control options */
#define ECO_AUTOWORDSELECTION   0x00000001
#define ECO_AUTOVSCROLL         0x00000040
#define ECO_AUTOHSCROLL         0x00000080
#define ECO_NOHIDESEL           0x00000100
#define ECO_READONLY            0x00000800
#define ECO_WANTRETURN          0x00001000
#define ECO_SAVESEL             0x00008000
#define ECO_SELECTIONBAR        0x01000000
#define ECO_VERTICAL            0x00400000      /* FE specific */


/* ECO operations */
#define ECOOP_SET               0x0001
#define ECOOP_OR                0x0002
#define ECOOP_AND               0x0003
#define ECOOP_XOR               0x0004

/* new word break function actions */
// Left must we even, right must be odd
#define WB_CLASSIFY         3
#define WB_MOVEWORDLEFT     4
#define WB_MOVEWORDRIGHT    5
#define WB_LEFTBREAK        6
#define WB_RIGHTBREAK       7
#define WB_MOVEURLLEFT      10
#define WB_MOVEURLRIGHT     11

/* Far East specific flags */
#define WB_MOVEWORDPREV     4
#define WB_MOVEWORDNEXT     5
#define WB_PREVBREAK        6
#define WB_NEXTBREAK        7

#define PC_FOLLOWING        1
#define PC_LEADING          2
#define PC_OVERFLOW         3
#define PC_DELIMITER        4
#define WBF_WORDWRAP        0x010
#define WBF_WORDBREAK       0x020
#define WBF_OVERFLOW        0x040
#define WBF_LEVEL1          0x080
#define WBF_LEVEL2          0x100
#define WBF_CUSTOM          0x200

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
#define WBF_CLASS           ((BYTE) 0x0F)
#define WBF_WHITE           ((BYTE) 0x10)
#define WBF_BREAKAFTER      ((BYTE) 0x20)
#define WBF_EATWHITE        ((BYTE) 0x40)

/* new data types */

#ifdef _WIN32
/* extended edit word break proc (character set aware) */
typedef LONG (*EDITWORDBREAKPROCEX)(char *pchText, LONG cchText, BYTE bCharSet, INT action);
#endif


#define yHeightCharPtsMost 1638

/* EM_SETCHARFORMAT wParam masks */
#define SCF_SELECTION   0x0001
#define SCF_WORD        0x0002
#define SCF_DEFAULT     0x0000      // set the default charformat or paraformat
#define SCF_ALL         0x0004      // not valid with SCF_SELECTION or SCF_WORD


typedef struct _charrange
{
    LONG    cpMin;
    LONG    cpMax;
} CHARRANGE;

typedef struct _textrange
{
    CHARRANGE chrg;
    LPSTR lpstrText;    /* allocated by caller, zero terminated by RichEdit */
} TEXTRANGEA;

typedef struct _textrangew
{
    CHARRANGE chrg;
    LPWSTR lpstrText;   /* allocated by caller, zero terminated by RichEdit */
} TEXTRANGEW;

#ifdef UNICODE
#define TEXTRANGE   TEXTRANGEW
#else
#define TEXTRANGE   TEXTRANGEA
#endif // UNICODE


typedef DWORD (CALLBACK *EDITSTREAMCALLBACK)(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

typedef struct _editstream
{
    DWORD dwCookie;     /* user value passed to callback as first parameter */
    DWORD dwError;      /* last error */
    EDITSTREAMCALLBACK pfnCallback;
} EDITSTREAM;

/* stream formats */

#define SF_TEXT         0x0001
#define SF_RTF          0x0002
#define SF_RTFNOOBJS    0x0003      /* outbound only */
#define SF_TEXTIZED     0x0004      /* outbound only */
#define SF_UNICODE      0x0010      /* Unicode file of some kind */

/* Flag telling stream operations to operate on the selection only */
/* EM_STREAMIN will replace the current selection */
/* EM_STREAMOUT will stream out the current selection */
#define SFF_SELECTION   0x8000

/* Flag telling stream operations to operate on the common RTF keyword only */
/* EM_STREAMIN will accept the only common RTF keyword */
/* EM_STREAMOUT will stream out the only common RTF keyword */
#define SFF_PLAINRTF    0x4000

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

#ifdef UNICODE
#define FINDTEXT    FINDTEXTW
#else
#define FINDTEXT    FINDTEXTA
#endif // UNICODE

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

#ifdef UNICODE
#define FINDTEXTEX  FINDTEXTEXW
#else
#define FINDTEXTEX  FINDTEXTEXA
#endif // UNICODE


typedef struct _formatrange
{
    HDC hdc;
    HDC hdcTarget;
    RECT rc;
    RECT rcPage;
    CHARRANGE chrg;
} FORMATRANGE;

/* all paragraph measurements are in twips */

// BUGBUG (cthrash) There's really no point in statically allocating an
// array of MAX_TAB_STOPS tabs, since currently we have no mechanism for
// changing them.  In the future, though, we may have via stylesheets a
// way of setting them.  Until then, let's not waste memory in PFs.

#define MAX_TAB_STOPS 1 //32
#define lDefaultTab 960 //720

/* PARAFORMAT numbering options */

struct CListing
{
public:

    enum LISTING_CONSTS { MAXLEVELS = 256 };

    enum LISTING_TYPE
    {
        NONE = 0,
        BULLET = 1,
        NUMBERING = 2,
        IMAGE = 3,
        DEFINITION = 4,
        LAST
    };

private:

    WORD    wBits;
    WORD    wStyle;

    enum LISTING_MASKS
    {
        TYPE       = 0x0007,       // bits 0-2
        GETVALUE   = 0x0008,       // bit 3
        GETSTYLE   = 0x0010,       // bit 4
        INLIST     = 0x0020,       // bit 5
        UNUSED     = 0x00c0,       // bits 6-7
        VALUE      = 0xff00        // bits 8-15
    };

public:

    void         Reset() { wBits = wStyle = 0; }
    BOOL         IsReset() const { return(wBits == 0 && wStyle == 0); }

    LISTING_TYPE GetType() const { return (LISTING_TYPE)(wBits & TYPE); }
    void         SetType(LISTING_TYPE lt)
                 {
                     Assert( lt > 0 && lt < LAST );
                     wBits &= ~TYPE;
                     wBits |= (WORD)lt;
                 }

    int          GetLevel() const { return (int)(wBits >> 8); }
    void         SetLevel( int level )
                 {
                     Assert( level >= 0 && level < 256 );
                     wBits &= ~VALUE;
                     wBits |= (WORD)level<<8;
                 }

    BOOL         HasAdornment() const
                 {
                     int type = (wBits & TYPE);

                     return type == BULLET ||
                            type == NUMBERING ||
                            type == IMAGE;
                 }

    WORD         IsInList() const     { return wBits & INLIST; }
    void         SetInList()          { wBits |= INLIST; }
    void         SetNotInList()       { wBits &= ~INLIST; }
    WORD         IsValueValid() const { return wBits & GETVALUE; }
    void         SetValueValid()      { wBits |= GETVALUE; }
    WORD         IsStyleValid() const { return wBits & GETSTYLE; }
    void         SetStyleValid()      { wBits |= GETSTYLE; }

    enum _styleListStyleType GetStyle() const
            { return (enum _styleListStyleType)wStyle; }
    void SetStyle(enum _styleListStyleType style) { wStyle = (WORD)style; }
};

/* Underline types */
#define CFU_OVERLINE_BITS   0xf0
#define CFU_UNDERLINE_BITS  0x0f
#define CFU_OVERLINE        0x10
#define CFU_STRIKE          0x20
#define CFU_SWITCHSTYLE     0x40
#define CFU_INVERT          0x5 /* For IME composition fake a selection.*/
#define CFU_CF1UNDERLINE    0x4 /* map charformat's bit underline to CF2.*/
#define CFU_UNDERLINEDOTTED 0x3     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINEDOUBLE 0x2     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINEWORD   0x1     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINE       0x0

/* notification structures */

#ifndef WM_NOTIFY
#define WM_NOTIFY               0x004E

typedef struct _nmhdr
{
    HWND    hwndFrom;
    WPAD1
    UINT    idFrom;
    WPAD2
    UINT    code;
    WPAD3
} NMHDR;
#endif  /* !WM_NOTIFY */

typedef struct _msgfilter
{
    NMHDR   nmhdr;
    UINT    msg;
    WPAD1
    WPARAM  wParam;
    WPAD2
    LPARAM  lParam;
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

#define SEL_EMPTY       0x0000
#define SEL_TEXT        0x0001
#define SEL_OBJECT      0x0002
#define SEL_MULTICHAR   0x0004
#define SEL_MULTIOBJECT 0x0008

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
    WPAD1
    WPARAM wParam;
    WPAD2
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

#define OLEOP_DOVERB    1

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
    WPAD1
    WPARAM wParam;
    WPAD2
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
    UINT    iSize;
    LPSTR   szPunctuation;
} PUNCTUATION;

/* Far East specific */
typedef struct _compcolor
{
    COLORREF crText;
    COLORREF crBackground;
    DWORD dwEffects;
}COMPCOLOR;


/* clipboard formats - use as parameter to RegisterClipboardFormat() */
#define CF_RTF          TEXT("Rich Text Format")
#define CF_RTFNOOBJS    TEXT("Rich Text Format Without Objects")
#define CF_RETEXTOBJ    TEXT("RichEdit Text and Objects")

/* Paste Special */
typedef struct _repastespecial
{
    DWORD   dwAspect;
    DWORD   dwParam;
} REPASTESPECIAL;

/*  UndoName info */
typedef enum _undonameid
{
    UID_UNKNOWN     = 0,
    UID_TYPING      = 1,
    UID_DELETE      = 2,
    UID_DRAGDROP    = 3,
    UID_CUT         = 4,
    UID_PASTE       = 5
} UNDONAMEID;

#undef _WPAD

#ifdef _WIN32
#include <poppack.h>
#elif !defined(RC_INVOKED)
#pragma pack()
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma INCMSG("--- End 'textedit.h'")
#else
#pragma INCMSG("*** Dup 'textedit.h'")
#endif
