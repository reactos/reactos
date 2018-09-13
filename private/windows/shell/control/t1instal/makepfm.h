// *------------------------------------------------------------------------*
// * makepfm.h
// *------------------------------------------------------------------------*
//
//      Copyright 1990, 1991 -- Adobe Systems, Inc.
//      PostScript is a trademark of Adobe Systems, Inc.
//
// NOTICE:  All information contained herein or attendant hereto is, and
// remains, the property of Adobe Systems, Inc.  Many of the intellectual
// and technical concepts contained herein are proprietary to Adobe Systems,
// Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
// are protected as trade secrets.  Any dissemination of this information or
// reproduction of this material are strictly forbidden unless prior written
// permission is obtained from Adobe Systems, Inc.
//
//---------------------------------------------------------------------------


typedef char *LPSZ;

#define OPEN        FileOpen
#define CLOSE       _lclose
#define READ_BLOCK  _lread
#define WRITE_BLOCK _lwrite
#define STRCPY      lstrcpy
#define STRCAT      lstrcat
#define STRCMP      lstrcmp



/*--------------------------------------------------------------------------*/
#define DEBUG_MODE      0  

typedef struct {      /* A lookup table for converting strings to tokens */
  char *szKey;        /* Ptr to the string */
  int iValue;         /* The corresponding token value */
} KEY;

#define TK_UNDEFINED       0    /* tokens for ReadFontInfo */
#define TK_EOF             1
#define TK_MSMENUNAME      2
#define TK_VPSTYLE         3
#define TK_PI              4
#define TK_SERIF           5
#define TK_PCLSTYLE        6
#define TK_PCLSTROKEWEIGHT 7
#define TK_PCLTYPEFACEID   8
#define TK_INF_CAPHEIGHT   9
#define LAST_FI_TOKEN      9
#define TK_ANGLE           10   // added for ATM ( GetINFFontDescription )
#define TK_PSNAME          11   // added for ATM ( GetINFFontDescription )

/*----------------------------------------------------------------------------*/
/* EM describes the basic character cell dimension (in Adobe units) */
#define EM 1000

/*----------------------------------------------------------------------------*/
#define ANSI_CHARSET   0
#define SYMBOL_CHARSET 2
#define OEM_CHARSET    255
#define PCL_PI_CHARSET 181

#define PS_FONTTYPE    0x0081
#define PCL_FONTTYPE   0x0080

#define FW_NORMAL      400
#define FW_BOLD        700

/* GDI font families. */
#define WIN30
#ifdef WIN30
#define FF_DONTCARE   (0<<4) /* Don't care or don't know. */
#define FF_ROMAN      (1<<4) /* Variable stroke width, serifed. Times Roman, Century Schoolbook, etc. */
#define FF_SWISS      (2<<4) /* Variable stroke width, sans-serifed. Helvetica, Swiss, etc. */
#define FF_MODERN     (3<<4) /* Const stroke width, serifed or sans-serifed. Pica, Elite, Courier, etc. */
#define FF_SCRIPT     (4<<4) /* Cursive, etc. */
#define FF_DECORATIVE (5<<4) /* Old English, etc. */
#endif

typedef struct
{
  SHORT left;
  SHORT bottom;
  SHORT right;
  SHORT top;
} BBOX;

typedef struct {
  SHORT capHeight;
  SHORT xHeight;
  SHORT loAscent;        /* Lower-case ascent */
  SHORT loDescent;       /* Lower-case descent */
  SHORT ulOffset;        /* The underline offset */
  SHORT ulThick;         /* The underline thickness */
  SHORT iSlant;          /* The italic angle */
  BBOX  rcBounds;      /* The font bounding box */
} EMM;

typedef struct {
  char szFont[32];     /* The PostScript font name */
  char szFace[32];     /* The face name of the font */
  BOOL fEnumerate;     /* TRUE if the font should be enumerated */
  BOOL fItalic;        /* TRUE if this is an italic font */
  BOOL fSymbol;        /* TRUE if the font is decorative */
  SHORT iFamily;       /* The fonts family */
  WORD  iWeight;       /* TRUE if this is a bold font */
  SHORT iFirstChar;    /* The first character in the font */
  SHORT iLastChar;     /* The last character in the font */
  SHORT rgWidths[256]; /* Character widths from 0x020 to 0x0ff */
} FONT;

extern void PutByte(SHORT);
extern void PutWord(SHORT);
extern void PutLong(long);

typedef struct
{
  WORD iKey;
  SHORT iKernAmount;
} KX, *PKX;

typedef struct
{
  WORD cPairs;           /* The number of kerning pairs */
  PKX rgPairs;
} KP;

/* The info for a single kern track */
typedef struct
{
  SHORT iDegree;         /* The degree of kerning */
  SHORT iPtMin;          /* The minimum point size */
  SHORT iKernMin;        /* The minimum kern amount */
  SHORT iPtMax;          /* The maximum point size */
  SHORT iKernMax;        /* The maximum kern amount */
} TRACK;

#define MAXTRACKS 16
/* The track kerning table for a font */
typedef struct
{
  SHORT cTracks;              /* The number of kern tracks */
  TRACK rgTracks[MAXTRACKS];  /* The kern track information */
} KT;

/* Character metrics */
typedef struct
{
  BBOX rc;
  SHORT iWidth;
} CM;

typedef struct
{
  WORD  iPtSize;
  SHORT iFirstChar;
  SHORT iLastChar;
  SHORT iAvgWidth;
  SHORT iMaxWidth;
  SHORT iItalicAngle;
  SHORT iFamily;
  SHORT ulOffset;
  SHORT ulThick;
  SHORT iAscent;
  SHORT iDescent;
  BOOL fVariablePitch;
  BOOL fWasVariablePitch;
  char szFile[MAX_PATH + 4]; // +1 for nul term, +3 for alignment.
  char szFont[80];
  char szFace[80];
  SHORT iWeight;
  KP kp;
  KT kt;
  BBOX rcBBox;
  CM rgcm[256];        /* The character metrics */
} AFM;

/*----------------------------------------------------------------------------*/

typedef struct
{
  SHORT iSize;
  SHORT iPointSize;
  SHORT iOrientation;
  SHORT iMasterHeight;
  SHORT iMinScale;
  SHORT iMaxScale;
  SHORT iMasterUnits;
  SHORT iCapHeight;
  SHORT iXHeight;
  SHORT iLowerCaseAscent;
  SHORT iLowerCaseDescent;
  SHORT iSlant;
  SHORT iSuperScript;
  SHORT iSubScript;
  SHORT iSuperScriptSize;
  SHORT iSubScriptSize;
  SHORT iUnderlineOffset;
  SHORT iUnderlineWidth;
  SHORT iDoubleUpperUnderlineOffset;
  SHORT iDoubleLowerUnderlineOffset;
  SHORT iDoubleUpperUnderlineWidth;
  SHORT iDoubleLowerUnderlineWidth;
  SHORT iStrikeOutOffset;
  SHORT iStrikeOutWidth;
  WORD nKernPairs;
  WORD nKernTracks;
} ETM;

/*----------------------------------------------------------------------------*/

typedef struct
{
  WORD iVersion;
  DWORD iSize;
  CHAR szCopyright[60];
  WORD iType;
  WORD iPoints;
  WORD iVertRes;
  WORD iHorizRes;
  WORD iAscent;
  WORD iInternalLeading;
  WORD iExternalLeading;
  BYTE iItalic;
  BYTE iUnderline;
  BYTE iStrikeOut;
  WORD iWeight;
  BYTE iCharSet;
  WORD iPixWidth;
  WORD iPixHeight;
  BYTE iPitchAndFamily;
  WORD iAvgWidth;
  WORD iMaxWidth;
  BYTE iFirstChar;
  BYTE iLastChar;
  BYTE iDefaultChar;
  BYTE iBreakChar;
  WORD iWidthBytes;
  DWORD oDevice;
  DWORD oFace;
  DWORD oBitsPointer;
  DWORD oBitsOffset;
} PFM;

typedef struct
{
  WORD oSizeFields;
  DWORD oExtMetricsOffset;
  DWORD oExtentTable;
  DWORD oOriginTable;
  DWORD oPairKernTable;
  DWORD oTrackKernTable;
  DWORD oDriverInfo;
  DWORD iReserved;
} PFMEXT;

/*----------------------------------------------------------------------------*/

typedef enum    {
        epsymUserDefined,
        epsymRoman8,
        epsymKana8,
        epsymMath8,
        epsymUSASCII,
        epsymLineDraw,
        epsymMathSymbols,
        epsymUSLegal,
        epsymRomanExt,
        epsymISO_DenNor,
        epsymISO_UK,
        epsymISO_France,
        epsymISO_German,
        epsymISO_Italy,
        epsymISO_SwedFin,
        epsymISO_Spain,
        epsymGENERIC7,
        epsymGENERIC8,
        epsymECMA94
} SYMBOLSET;

typedef struct
        {
        SYMBOLSET symbolSet;            /* kind of translation table */
        DWORD offset;                           /* location of user-defined table */
        WORD len;                                       /* length (in bytes) of table */
        BYTE firstchar, lastchar;       /* table ranges from firstchar to lastchar */
        } TRANSTABLE;

typedef struct
        {
        WORD epSize;                            /* size of this data structure */
        WORD epVersion;                         /* number indicating version of struct */
        DWORD epMemUsage;                       /* amt of memory font takes up in printer */
        DWORD epEscape;                         /* pointer to escape that selects the font */
        TRANSTABLE xtbl;                        /* character set translation info */
        } DRIVERINFO;

/*----------------------------------------------------------------------------*/

#define POSTSCRIPT  (1)
#define PCL         (2)

/*----------------------------------------------------------------------------*/

typedef enum    { PORTRAIT, LANDSCAPE } ORIENTATION;

#define ASCII_SET   ("0U")
#define ROMAN8_SET  ("8U")
#define WINANSI_SET ("9U")
#define PI_SET      ("15U")

typedef struct
{
  ORIENTATION orientation;
  char symbolsetStr[4];
  SYMBOLSET symbolsetNum;
  SHORT style;
  SHORT strokeWeight;
  SHORT typefaceLen;
  WORD typeface;
  char *epEscapeSequence;       /* escape sequence that selects the font */
} PCLINFO;

/*--------------------------------------------------------------------------*/
#define EOS        '\0'
#define FNAMEMAX   (80)

/*--------------------------------------------------------------------------*/

#define BUFFLEN 80
#define MANDATORY       1
#define CookedReadMode  "r"
#define FATALEXIT  (2)



