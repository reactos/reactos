
#define BADTYPECAST 101
#define NOROOM      102
#define GEXPRERR    105



int
CPCopyToken(
    LPSTR *lplps,
    LPSTR lpt
    );

int
CPCopyString(
    LPSTR *lplps,
    LPSTR lpT,
    char chEscape,
    BOOL fQuote
    );

LPSTR
CPAdvance(
    char * String,
    char * UserDelim
    );

LPSTR
CPSzToken(
    char ** lplpString,
    char * TokenBuffer
    );

int
CPQueryChar(
    char * String,
    char * UserDelim
    );

int
CPQueryQuoteIndex (
    char * szSrc
    );

LPSTR
CPSkipWhitespace(
    PCSTR String
    );

void
CPRemoveTrailingWhitespace(
    PSTR String
    );

typedef enum {
    CPNOERROR,
    CPNOMEMORY,
    CPGENERAL,
    CPBADADDR,
    CPBADFORMAT,
    CPOVERRUN,
    CPOPTIONAL,
    CPDEFAULT,
    CPNOARGS,
    CPISOPENQUOTE,
    CPISCLOSEQUOTE,
    CPISOPENANDCLOSEQUOTE,
    CPISDELIM,
    CPNOTINQUOTETABLE,
    CPCATASTROPHIC = 0xff
} CPSTATUS;


#define fmtAscii    0
#define fmtInt      1
#define fmtUInt     2
#define fmtFloat    3
#define fmtAddress  4
#define fmtUnicode  5
#define fmtBit      6
#define fmtBasis    0x0f

// override logic to force radix
#define fmtOverRide 0x2000
#define fmtZeroPad  0x4000
#define fmtNat      0x8000

typedef UINT FMTTYPE;


CPSTATUS
CPFormatMemory(
    LPCH    lpchTarget,
    DWORD    cchTarget,
    LPBYTE  lpbSource,
    DWORD    cBits,
    FMTTYPE fmtType,
    DWORD    radix
    );

CPSTATUS
CPUnformatMemory(
    LPBYTE  lpbTarget,
    LPSTR   lpszSource,
    DWORD    cBits,
    FMTTYPE fmtType,
    DWORD    radix
    );

CPSTATUS
CPUnformatAddr(
    LPADDR lpaddr,
    char * lpsz,
    PBOOL pbSegAlwaysZero = NULL
    );

CPSTATUS
CPFormatEnumerate(
    DWORD      iFmt,
    LPDWORD    lpcBits,
    FMTTYPE *  lpFmtType,
    PEERADIX   lpRadix,
    LPDWORD    lpFTwoFields,
    LPDWORD    lpcchMax,
    LPSTR *    lplpszDesc
);

CPSTATUS
CPGetFPNbr(
    LPSTR   lpExpr,
    int     cBits,
    int     nRadix,
    int     fCase,
    PCXF    pCxf,
    LPSTR   lpBuf,
    LPSTR   lpErr
    );

CPSTATUS
CPGetRange(
    LPSTR   lpszExpr,
    LPINT   lpcch,
    LPADDR  lpAddr1,
    LPADDR  lpAddr2,
    PULONG  lpItems,
    EERADIX radix,
    int     cbDefault,
    int     cbSize,
    PCXF    pcxf,
    BOOL    fCase,
    BOOL    fSpecial,
    LPBOOL  lpbSecondParamIsALength
    );

CPSTATUS
CPGetCastNbr(
    char *  szExpr,
    USHORT  type,
    int     radix,
    int     fCase,
    PCXF    pCxf,
    char *  pValue,
    char *  szErrMsg,
    BOOL    fSpecial
    );

CPSTATUS
CPGetAddress(
    LPCSTR      lpExprOrig,
    LPINT       lpcch,
    PADDR       lpAddr,
    EERADIX     radix,
    PCXF        pcxf,
    BOOL        fCase,
    BOOL        fSpecial
    );

long
CPGetNbr(
    char *  szExpr,
    int     radix,
    int     fCase,
    PCXF    pCxf,
    char *  szErrMsg,
    int  *  pErr,
    BOOL    fSpecial
    );

long
CPGetInt(
    char * szExpr,
    int  * pErr,
    int  * cLength
    );

