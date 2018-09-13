
#include <windows.h>
#include <ntverp.h>
#include <stdio.h>
#include <windef.h>
#include <wchar.h>

typedef FILE    *PFILE;
typedef WCHAR   *PWCHAR;
typedef USHORT  *PUSHORT;
typedef SHORT   *PSHORT;
typedef UINT    *PUINT;
typedef UCHAR   *PUCHAR;

#include <ctype.h>
#include <errno.h>
#include <io.h>
#include <limits.h>
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mmsystem.h>

#include "charmap.h"
#include "fcntl.h"
#include "getflags.h"
#include "grammar.h"

#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "p0defs.h"

#include "newexe.h"
#include "p1types.h"
#include "rcdefs.h"
#include "rcnewres.h"
#include "rcppx.h"
#include "rcunicod.h"
#include "resfile.h"
#include "strings.h"
#include "trees.h"


#define NULL_FILE ((PFILE) NULL)

/* ----- General symbols ----- */
#define VERSION_DUAL            0x202
#define BUFSIZE                 16384
#define EOF                     (-1)

#define TRUE    1
#define FALSE   0

/* The ResType field of NewHeader identifies the cursor/icon type */
#define  ICONTYPE      1
#define  CURSORTYPE    2

/* Identifies the menu item template version number */
#define OLDMENUITEMTEMPLATEVERSIONNUMBER    0
#define OLDMENUITEMTEMPLATEBYTESINHEADER    0

#define MENUITEMTEMPLATEVERSIONNUMBER       1
#define MENUITEMTEMPLATEBYTESINHEADER       sizeof(DWORD)   //dwHelpID in hdr

#define DIFFERENCE      11

/* Predefined resource types */
#define RT_NEWRESOURCE  0x2000
#define RT_ERROR        0x7fff
#define RT_NEWBITMAP    MAKEINTRESOURCE((DWORD)RT_BITMAP+DIFFERENCE)

// These are 'hidden' resources that users never see/use directly.
#define RT_MENUEX       MAKEINTRESOURCE(15)
#define RT_DIALOGEX     MAKEINTRESOURCE(18)

// These are here only temporary here they should in winuser.w
#define RT_ANICURSOR    MAKEINTRESOURCE(21)
#define RT_ANIICON      MAKEINTRESOURCE(22)

#define RT_LAST         MAKEINTRESOURCE(22)

// AFX resource types (Should these be in winuser.h?)
#define RT_DLGINIT      MAKEINTRESOURCE(240)
#define RT_TOOLBAR      MAKEINTRESOURCE(241)

// BUGBUG only here until they're in winuser.h
#define GFE_ZEROINIT    0x0001
#define GFE_SHORT       0x0002

#ifdef MFR_POPUP
#if (MFR_POPUP != 0x01) && (MFR_END != 0x80)
#error MFR_POPUP or MFR_END definition(s) invalid
#endif
#else
#define MFR_POPUP       0x01
#define MFR_END         0x80
#endif

#define BS_PUSHBOX      0x0AL
// END BUGBUG

/* Single-byte control class IDs */
#define BUTTONCODE      0x80
#define EDITCODE        0x81
#define STATICCODE      0x82
#define LISTBOXCODE     0x83
#define SCROLLBARCODE   0x84
#define COMBOBOXCODE    0x85

/* Translator flag bits */
#define fVIRTKEY                1
#define fNOINVERT               2
#define fSHIFT                  4
#define fCONTROL                8
#define fALT                    16

/* Menu flag bits */
#define OPGRAYED                  0x0001
#define OPINACTIVE                0x0002    // INACTIVE | GRAYED
#define OPBITMAP                  0x0004
#define OPOWNERDRAW               0x0100
#define OPUSECHECKBITMAPS 0x0200
#define OPCHECKED                 0x0008
#define OPPOPUP                   0x0010
#define OPBREAKWBAR               0x0020
#define OPBREAK                   0x0040
#define OPENDMENU                 0x0080
#define OPHELP                    0x4000
#define OPSEPARATOR               0x0800
/*#define OPPOPHELP         0x0004*/


#define wcsdigit(w) (w >= 0x0030 && w <= 0x0039)

/*
** dialog & menu template tokens (these start at 40)
*/

/* buttons */
#define TKRADIOBUTTON       40
#define TKCHECKBOX          41
#define TKPUSHBUTTON        42
#define TKDEFPUSHBUTTON     43
#define TKAUTOCHECK         44
#define TK3STATE            45
#define TKAUTO3             46
#define TKUSERBUTTON        47
#define TKAUTORADIO         48
#define TKOWNERDRAW         50
#define TKGROUPBOX          51
#define TKPUSHBOX           52

/* static/edit */
#define TKBEDIT         57
#define TKHEDIT         58
#define TKIEDIT         59
#define TKEDITTEXT      60
#define TKLTEXT         61
#define TKRTEXT         62
#define TKCTEXT         63
#define TKEDIT          64
#define TKSTATIC        65
#define TKICON          66
#define TKBITMAP        67

/* menu stuff */
#define TKMENU          70
#define TKMENUITEM      71
#define TKSEPARATOR     72
#define TKCHECKED       73
#define TKGRAYED        74
#define TKINACTIVE      75
#define TKBREAKWBAR     76
#define TKBREAK         77
#define TKPOPUP         78
#define TKHELP          79

/* other controls */
#define TKLISTBOX       90
#define TKCOMBOBOX      91
#define TKRCDATA        92
#define TKSCROLLBAR     93
#define TKFONT          94
#define TKBUTTON        95
#define TKMESSAGETABLE  96

/* math expression tokens */
#define TKCLASS         100
#define TKPLUS          101
#define TKMINUS         102
#define TKNOINVERT      103
#define TKNOT           104
#define TKKANJI         105
#define TKSHIFT         106

/* Accel table */
#define TKALT           110
#define TKASCII         111
#define TKVIRTKEY       112
#define TKVALUE         113
#define TKBLOCK         114

/* verison */
#define TKFILEVERSION   120
#define TKPRODUCTVERSION        121
#define TKFILEFLAGSMASK 122
#define TKFILEFLAGS     123
#define TKFILEOS        124
#define TKFILETYPE      125
#define TKFILESUBTYPE   126

/* misc */
#define TKCHARACTERISTICS   130
#define TKLANGUAGE          131
#define TKVERSION           132
#define TKSTYLE             133
#define TKCONTROL           134
#define TKCAPTION           135
#define TKDLGINCLUDE        136
#define TKLSTR              137
#define TKDLGINIT           138
#define TKEXSTYLE           0xf7  /* so as not to conflict with x-coordinate */

/* memory and load flags */
#define TKFIXED         0xfff0
#define TKMOVEABLE      0xfff1
#define TKDISCARD       0xfff2
#define TKLOADONCALL    0xfff3
#define TKPRELOAD       0xfff4
#define TKPURE          0xfff5
#define TKIMPURE        0xfff6



/* special tokens */
#define CHCARRIAGE      L'\r'
#define CHSPACE         L' '
#define CHNEWLINE       L'\n'
//??#define CHTAB            9
#define CHTAB           L'\t'
#define CHDIRECTIVE     L'#'
#define CHQUOTE         L'"'
#define CHEXTENSION     L'.'
#define CHCSOURCE       L'c'
#define CHCHEADER       L'h'
#define CHCOMMENT       L';'

#define DEBUGLEX    1
#define DEBUGPAR    2
#define DEBUGGEN    4

/* End of file character/token */
#define EOFMARK         127

/* Single character keywords that we ignore */
#define LPAREN          1      /* ( */
#define RPAREN          2      /* ) */

/* multiple character keywords */
#define FIRSTKWD        11             /* for adding to table indices */

#define OR          FIRSTKWD+1
#define BEGIN       FIRSTKWD+2
#define END         FIRSTKWD+3
#define COMMA       FIRSTKWD+4
#define TILDE       FIRSTKWD+5
#define AND         FIRSTKWD+6
#define EQUAL       FIRSTKWD+7  // AFX
#define LASTKWD     FIRSTKWD+8  // 19

/* Token types */
#define NUMLIT      LASTKWD+1   // 20
#define STRLIT      LASTKWD+2
#define CHARLIT     LASTKWD+3
#define LSTRLIT     LASTKWD+4   // AFX

/* The following switches, when defined enable various options
**  #define DEBUG enables debugging output.  Use one or more of the
**  values defined above to enable debugging output for different modules */

/* Version number.  VERSION and REVISION are used to set the API number
** in an RCed file.  SIGNON_* are used just to print the signon banner.
** Changing VERSION and REVISION means that applications RCed with this
** version will not run with earlier versions of Windows.  */

//#define VERSION     2
#define REVISION    03
#define SIGNON_VER  4
#define SIGNON_REV  00

/* GetToken() flags */
#define TOKEN_NOEXPRESSION 0x8000

/* Current token structure */
#define MAXSTR (4096+1)     // AFX defines this as 255
#define MAXTOKSTR (256+1)

// New Menu Template Parsing structures/definitions

// New menu template format
//
//      MenuName MENUEX
//      BEGIN
//          [MENUITEM "text" [, [id] [, [type] [, [state]]]]]
//          [POPUP    "text" [, [id] [, [type] [, [state] [, [help id]]]]]
//          BEGIN
//              [MENUITEM "text" [, [id] [, [type] [, [state]]]]]
//              ...
//          END]
//          ...
//      END

typedef struct tagMENU
{
    DWORD   dwID;
    DWORD   dwType;
    DWORD   dwState;
    DWORD   dwHelpID;
    WORD    wResInfo;
    WCHAR   szText[MAXTOKSTR];
} MENU, *PMENU;


typedef enum
{
    PT_WORD = 0,
    PT_DWORD,
    PT_TEXT
}   PARSETYPE;

typedef enum
{
    PTO_WORD = 0x80,
    PTO_DWORD,
    PTO_TEXT
}   OPARSETYPE;

typedef enum
{
    PAR_POPUP = 0,
    PAR_MENUITEM,
    PAR_MENU
}   PARCELTYPE;

#define PT_OPTIONAL 0x80

typedef struct tagPARCEL
{
    WORD    *pwEnd;
    BYTE    *pwParms;
}   PARCEL;

typedef struct tagKEY
{
    PWCHAR  kwd;
    WORD    kwdval;
} KEY, *PKEY;

typedef struct tagSKEY
{
    WCHAR   skwd;
    UINT    skwdval; /* changed from a char */
} SKEY, *PSKEY;

#pragma pack(2)
typedef struct tagSYMINFO
{
    WCHAR   name[MAX_SYMBOL + 1];/* symbol for the resource if available */
    WCHAR   file[_MAX_PATH];
    WORD    line;
    WORD    nID;
} SYMINFO, *PSYMINFO;

typedef struct tagTOKEN
{
    LONG        longval;
    int         row;                    /* line number of current token */
    int         col;                    /* column number of current token */
    BOOL        flongval;               /* is parsed number a long? */
    USHORT      val;
    UCHAR       type;
    UCHAR       realtype;
    SYMINFO     sym;
} TOKEN, *PTOKEN;

typedef struct tagFONTDIR
{
    USHORT              ordinal;
    USHORT              nbyFont;
    struct tagFONTDIR   *next;
} FONTDIR, *PFONTDIR;

typedef struct tagOBJLST
{
    struct tagOBJLST    *next;
    DWORD               nObj;         /* objecty number */
    DWORD               cb;           /* number of bytes used */
    DWORD               cpg;          /* number of pages used */
    DWORD               flags;        /* object memory flags */
} OBJLST, *POBJLST;

typedef struct tagCTRL
{
    SHORT   x;
    SHORT   y;
    SHORT   cx;
    SHORT   cy;
    WCHAR   fOrdinalText;
    WCHAR   text[MAXTOKSTR];
    DWORD   id;
    WCHAR   Class[MAXTOKSTR];
    DWORD   dwStyle;
    DWORD   dwExStyle;
    DWORD   dwHelpID;
} CTRL, *PCTRL;

typedef struct tagDLGHDR
{
    SHORT   x;
    SHORT   y;
    SHORT   cx;
    SHORT   cy;
    UCHAR   fOrdinalMenu;
    UCHAR   fClassOrdinal;
    DWORD   dwStyle;
    DWORD   dwExStyle;
    WORD    bNumberOfItems;
    WCHAR   Title[MAXTOKSTR];
    WCHAR   MenuName[MAXTOKSTR];
    WCHAR   Class[MAXTOKSTR];
    WCHAR   Font[MAXTOKSTR];
    WORD    pointsize;
    WORD    wWeight;
    BYTE    bItalic;
    BYTE    bCharSet;
    DWORD   dwHelpID;
} DLGHDR, *PDLGHDR;

typedef struct tagMENUHDR
{
    USHORT   menuTemplateVersionNumber;
    USHORT   menuTemplateBytesInHeader;
} MENUHDR, *PMENUHDR;

typedef struct tagMENUITEM
{
    SHORT       id;
    WCHAR       szText[ MAXTOKSTR ];
    WORD        OptFlags;
    WORD        PopFlag;
} MENUITEM, *PMENUITEM;

#define BLOCKSIZE 16
typedef struct tagRCSTRING
{
    struct tagRCSTRING *next;
    DWORD       version;
    DWORD       characteristics;
    USHORT      hibits;
    SHORT       flags;
    WORD        language;
    PWCHAR      rgsz[ BLOCKSIZE ];
    PSYMINFO    rgsym[ BLOCKSIZE ];
} RCSTRING, *PRCSTRING;

typedef struct tagRCACCEL
{
    WORD        flags;
    WCHAR       ascii;
    USHORT      id;
    USHORT      unused;
} RCACCEL, *PRCACCEL;

typedef struct tagRESINFO
{
    DWORD       version;
    DWORD       characteristics;
    LONG        exstyleT;
    LONG        BinOffset;
    LONG        size;
    struct tagRESINFO *next;
    WORD        *poffset;
    PWCHAR      name;
    POBJLST     pObjLst;
    WORD        language;
    SHORT       flags;
    USHORT      nameord;
    USHORT      cLang;
    SYMINFO     sym;
} RESINFO, *PRESINFO;

typedef struct tagTYPEINFO
{
    struct tagTYPEINFO *next;
    PRESINFO    pres;
    PWCHAR      type;
    USHORT      typeord;
    USHORT      cTypeStr;
    USHORT      cNameStr;
    SHORT       nres;
} TYPEINFO, *PTYPEINFO;

#pragma pack()


/* ----- Global variables ----- */
extern  SHORT       ResCount;
extern  PTYPEINFO   pTypInfo;
extern  UINT        uiDefaultCodePage;
extern  UINT        uiCodePage;
extern  SHORT       nFontsRead;
extern  PFONTDIR    pFontList;
extern  PFONTDIR    pFontLast;
extern  TOKEN       token;
extern  int         errorCount;
extern  WCHAR       tokenbuf[MAXSTR + 1]; // +1 is to allow double sz termination
extern  UCHAR       exename[_MAX_PATH], fullname[_MAX_PATH];
extern  UCHAR       curFile[_MAX_PATH];
extern  WORD        language;
extern  LONG        version;
extern  LONG        characteristics;

extern  PDLGHDR     pLocDlg;
extern  UINT        mnEndFlagLoc;   /* patch location for end of a menu. */
                                    /* we set the high order bit there    */
extern  BOOL        fVerbose;       /* verbose mode (-v) */
extern  BOOL        fAFXSymbols;
extern  BOOL        fMacRsrcs;
extern  BOOL        fAppendNull;
extern  BOOL        fWarnInvalidCodePage;
extern  long        lOffIndex;
extern  WORD        idBase;
extern  PCHAR       szTempFileName;
extern  PCHAR       szTempFileName2;
extern  CHAR        inname[_MAX_PATH];
extern  PFILE       fhBin;
extern  PFILE       fhInput;
extern  PFILE       fhCode;
extern  PCHAR       pchInclude;
extern  SHORT       k1,k2,k3,k4;
extern  PRESINFO    pResString;

extern  HINSTANCE   hInstance;
extern  HANDLE      hHeap;

extern  int         nBogusFontNames;
extern  WCHAR      *pszBogusFontNames[16];
extern  WCHAR       szSubstituteFontName[MAXTOKSTR];

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rcl.c                                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int     FileChar();
USHORT  FindKwd(PWCHAR);
WCHAR   GetCharFTB();
PWSTR   GetWord(PWSTR);
LONG    GetDNum();
LONG    GetExpression();
PWCHAR  GetGenText();
int     GetKwd(int);
int     GetNameOrd();
VOID    GetNum();
VOID    GetNumFTB();
VOID    GetNumNoExpression();
LONG    GetONum();
LONG    GetOperand();
int     GetOperator(PWCHAR pOperator);
VOID    GetStr();
int     GetToken(int);
LONG    GetXNum();
void    LexError1(int iMsg);
void    LexError2(int iMsg, PCHAR str);
int     LexInit(PFILE);
WCHAR   LitChar();
WCHAR   OurGetChar();
VOID    SkipWhitespace();

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rcp.c                                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID        AddBinEntry(PTYPEINFO, PRESINFO, PCHAR , int, LONG);
VOID        AddDefaultTypes();
VOID        AddFontDir();
VOID        AddResToResFile (PTYPEINFO, PRESINFO, PCHAR, int, LONG);
PTYPEINFO   AddResType (PWCHAR, LPWSTR);
VOID        AddStringToBin(USHORT, PWCHAR);
int         DGetMemFlags(PRESINFO);
LONG        GetFileName (VOID);
VOID        ParseError1(int);
VOID        ParseError2(int, PWCHAR); // AFX has 1&2 -- NT just has ParseError
VOID        ParseError3(int);
int         ReadRF(VOID);
WORD        GetLanguage(VOID);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rcstring.c                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID        WriteTable (PRESINFO);
int         GetAccelerators (PRESINFO);
PRESINFO    GetTable (PRESINFO);
PCHAR       MyFAlloc(UINT, PCHAR);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rctg.c                                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

DWORD   GetNewBitmap(void);
BOOL    AddFontRes(PRESINFO);
VOID    AppendString(PWCHAR, BOOL);
VOID    CtlAlloc();
PFILE   CtlFile(PFILE);
VOID    CtlFree();
VOID    CtlInit();
VOID    FixMenuPatch();
VOID    GenWarning2(int iMsg, PCHAR arg);
VOID    GenWarning4(int iMsg, PCHAR arg1, PCHAR arg2, PCHAR arg3);
VOID    GenError1(int iMsg);
VOID    GenError2(int iMsg, PCHAR arg);
WORD    GetBufferLen();
void    SetItemCount(int Index, USHORT wCount);
USHORT  GetItemCount(int Index);
DWORD   GetIcon(LONG);
DWORD   GetAniIconsAniCursors(LONG nbyFile);
DWORD	FileIsAnimated(LONG nbyFile);
VOID    GetNewIconsCursors(PTYPEINFO, PRESINFO, LPWSTR);
WORD    GetRCData(PRESINFO);
PCHAR   GetSpace(WORD cb);
VOID    IncItemCount();
VOID    SwapItemCount(VOID);
void    FixMenuPatch    (WORD);
void    FixOldMenuPatch (WORD);
VOID    MarkAccelFlagsByte();
VOID    MarkMenuPatch();
VOID    PatchAccelEnd();
LONG    ResourceSize();
VOID    SaveResFile(PTYPEINFO, PRESINFO);
void    SetItemExtraCount(WORD, BOOL);
void    SetUpDlg        (PDLGHDR, BOOL);
void    SetUpItem       (PCTRL,   BOOL);
WORD    SetUpMenu       (PMENU);
WORD    SetUpOldMenu    (PMENUITEM);
VOID    WriteAlign();
VOID    WriteBuffer(PCHAR, USHORT);
int     WriteControl(PFILE, PCHAR, int, LONG);
VOID    WriteString(PWCHAR, BOOL);
VOID    WriteMacString(PWCHAR sz, BOOL fMacCP, BOOL fPascal);
int     ExpandString(BYTE* pb, int cb, BYTE* pbExpand);
WORD    SwapWord(WORD w);
DWORD   SwapLong(DWORD dw);

#define SwappedWord(w)  (fMacRsrcs ? SwapWord(w) : (w))
#define SwappedLong(dw) (fMacRsrcs ? SwapLong(dw) : (dw))

#define WriteByte(b)    (*GetSpace(sizeof(CHAR)) = (b))
#define WriteLong(dw)   (*((DWORD UNALIGNED*) GetSpace(sizeof(DWORD))) = SwappedLong(dw))
#define WriteWord(w)    (*((WORD UNALIGNED*)  GetSpace(sizeof(WORD))) = SwappedWord(w))


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rctp.c                                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define GFE_ZEROINIT    0x0001
#define GFE_SHORT       0x0002

void    DGetClassName   (PDLGHDR);
void    DGetFont        (PDLGHDR, BOOL);
void    DGetMenuName    (PDLGHDR);
void    DGetTitle       (PDLGHDR);
BOOL    DLexOptionalArgs(PRESINFO, PDLGHDR, BOOL);
void    DlgIncludeParse (PRESINFO); // new for NT
WORD    DoMenuItem      (int);
void    GetCoords       (PSHORT, PSHORT, PSHORT, PSHORT);
VOID    GetCtlCoords    (PCTRL);
VOID    GetCtlID        (PCTRL, BOOL);
VOID    GetCtlText      (PCTRL);
int     GetDlg          (PRESINFO, PDLGHDR, BOOL);
int     GetDlgItems     (BOOL);
BOOL    GetFullExpression(void *pval, WORD wFlags);
USHORT  GetTokenNoComma (USHORT wFlags);
USHORT  ICGetTok        ();
int     IsmnOption      (UINT, PMENUITEM);
VOID    ParseCtl        (PCTRL, BOOL);
int     ParseMenu       (int, PRESINFO);
int     ParseOldMenu    (int, PRESINFO);
int     VersionBlockStruct(PWCHAR pstrKey, PCHAR pstrValue, USHORT LenValue);
int     VersionBlockVariable(PWCHAR pstrKey);
VOID    VersionGet4Words(PDWORD pdw);
VOID    VersionGetDWord(PDWORD pdw);
int     VersionParse();
USHORT  VersionParseBlock();
int     VersionParseFixed();
USHORT  VersionParseValue(int IndexType); // void arg list in AFX
int     GetToolbar (PRESINFO);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rcutil.c                                                                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID    ExtractFileName(PWCHAR, PWCHAR);
int     iswhite (WCHAR);
BOOL    IsSwitchChar(CHAR);
UINT    MyAlign(PFILE);
PVOID   MyAlloc(UINT);
int     MyCopy (PFILE, PFILE, DWORD);
int     MyCopyAll (PFILE, PFILE);
PVOID   MyFree (PVOID);
PWCHAR  MyMakeStr(PWCHAR);
UINT    MyRead (PFILE, PVOID, UINT);
LONG    MySeek (PFILE, LONG, int);
UINT    MyWrite (PFILE, PVOID, UINT);
VOID    PreBeginParse(PRESINFO, int);
VOID    quit (PSTR);
VOID    searchenv(PCHAR, PCHAR, PCHAR);
int     strpre (PWCHAR, PWCHAR);
DWORD   wcsatoi(PWCHAR);
PWCHAR  wcsitow(LONG v, PWCHAR s, DWORD r);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rcfutil.c                                                                */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int     fgetl (PWCHAR, int, BOOL, PFILE);


/* ----- AFX Functionality ----- */
#include "rcppx.h"

LONG    MakeFontDir();              // AFX only

void SendWarning(PSTR str);
void SendError(PSTR str);

extern RC_PARSE_CALLBACK lpfnParseCallback;
extern RC_MESSAGE_CALLBACK  lpfnMessageCallback;
extern HWND         hWndCaller;

#define fflush(errFile)

void GetSymbolDef(int fReportError, WCHAR curChar);
void GetSymbol(int fReportError, WCHAR curChar);
BOOL InitSymbolInfo();
BOOL TermSymbolInfo(PFILE fhresFile);
void WriteSymbolUse(PSYMINFO sym);
void WriteSymbolDef(PWCHAR name, PWCHAR value, PWCHAR file, WORD line, char flags);
void WriteFileInfo(PRESINFO pRes, PTYPEINFO pType, PWCHAR szFileName);
void WriteResInfo(PRESINFO pRes, PTYPEINFO pType, BOOL bWriteMapEntry);
void WriteResIndex(PRESINFO pRes, PTYPEINFO pType);
void UpdateStatus(unsigned nCode, unsigned long dwStatus);

// offset in a macintosh resource file of the start of the resource data
#define MACDATAOFFSET 256

void GetMacIcon(TYPEINFO *pType, RESINFO *pRes);
void GetMacCursor(TYPEINFO *pType, RESINFO *pRes);

void WriteMacMap(void);
BOOL IsIcon(TYPEINFO* ptype);

#define res_type(ch1,ch2,ch3,ch4) (((unsigned long)(unsigned char)(ch1)<<24)| \
                                ((unsigned long)(unsigned char)(ch2)<<16)| \
                                ((unsigned long)(unsigned char)(ch3)<<8)| \
                                ((unsigned long)(unsigned char)(ch4)))
