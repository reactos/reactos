#ifndef I_TXTDEFS_H_
#define I_TXTDEFS_H_
#pragma INCMSG("--- Beg 'txtdefs.h'")

// BUGBUG: Need to figure out correct public place for this error.
// Note: error code is first outside of range reserved for OLE.
#define S_MSG_KEY_IGNORED \
    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x201)

#define SYS_ALTERNATE		0x20000000
#define SYS_PREVKEYSTATE	0x40000000

enum UN_FLAGS 
{
    UN_NOOBJECTS                = 1,
    UN_CONVERT_WCH_EMBEDDING    = 2
};

#ifdef UNICODE
#define CbOfCch(_x) (DWORD)((_x) * sizeof(WCHAR))
#define CchOfCb(_x) (DWORD)((_x) / sizeof(WCHAR))
#else
#define CbOfCch(_x) (_x)
#define CchOfCb(_x) (_x)
#endif

// Logical unit definition
#define LX_PER_INCH 1440
#define LY_PER_INCH 1440

// Use explicit ASCII values for LF and CR, since MAC compilers
// interchange values of '\r' and '\n'
#define LF          10
#define CR          13
#define FF          TEXT('\f')
#define TAB         TEXT('\t')
#define VT          TEXT('\v')
#define PS          0x2029

inline BOOL IsASCIIEOP(TCHAR ch)
{
    return InRange( ch, LF, CR );
}

BOOL IsEOP(TCHAR ch);
BOOL IsLaunderChar(TCHAR ch);

#define NumOfEOPChar    2

// count of characters in CRLF marker
#define cchCRLF 2
#define cchCR   1

#ifdef DBCS
#define PUNCT_OBJ   ped->lpPunctObj,
#define _FVert      ped->fVertical,
#define _FPed       ped,

#else
#define PUNCT_OBJ
#define _FVert
#define _FPed
#endif

    //
    // This builds an HGLOBAL from a unicode html string
    //

HRESULT HtmlStringToSignaturedHGlobal (
    HGLOBAL * phglobal, const TCHAR * pStr, long cch );

// BUGBUG (cthrash) These two functions have to go.  Some key functionality
// is missing like converting special WCH_ chars (e.g. WORDBREAK).

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, 
        int cwch = -1, UINT codepage = CP_ACP );
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1, UINT uiCodePage = CP_ACP);

HGLOBAL DuplicateHGlobal   (HGLOBAL hglobal);
HGLOBAL TextHGlobalAtoW    (HGLOBAL hglobal);
HGLOBAL TextHGlobalWtoA    (HGLOBAL hglobal);
INT     CountMatchingBits  (const DWORD *a, const DWORD *b, INT total);

//
// Unicode support
//

typedef BYTE CHAR_CLASS;
typedef BYTE SCRIPT_ID;
typedef BYTE DIRCLS;
struct tagUNIPROP
{
	BYTE fNeedsGlyphing : 1;	// Partition needs glyphing
	BYTE fCombiningMark : 1;	// Partition consists of combining marks
	BYTE fZeroWidth		: 1;	// Characters in partition have zero width
	BYTE fUnused		: 5;	// Unused bits
};
typedef tagUNIPROP UNIPROP;

// BUGBUG (cthrash) Need to fix to reflect new partition table
// Selected Partition ids (used by FHangingPunct)
// Partitions are normally NOT named as they are used as transient index into the
// XXXXFromQPid arrays only. These 6 are an exception to avoid a largely redundant table.

#define qpidCommaN  18
#define qpidCommaH  19
#define qpidCommaW  20
#define qpidQuestN  32
#define qpidCenteredN   37
#define qpidPeriodN 41
#define qpidPeriodH 42
#define qpidPeriodW 43

CHAR_CLASS CharClassFromChSlow(WCHAR wch);
SCRIPT_ID ScriptIDFromCharClass(CHAR_CLASS cc);

extern const CHAR_CLASS acc_00[256];
extern const SCRIPT_ID asidAscii[128];
extern const DIRCLS s_aDirClassFromCharClass[];
extern const UNIPROP s_aPropBitsFromCharClass[];

inline CHAR_CLASS
CharClassFromCh(WCHAR ch)
{
    return (ch < 256)
            ? acc_00[ch]
            : CharClassFromChSlow(ch);
}

inline SCRIPT_ID
ScriptIDFromCh(WCHAR ch)
{
    return (ch < 128)
            ? asidAscii[ch]
            : ScriptIDFromCharClass(CharClassFromCh(ch));
}

inline DIRCLS
DirClassFromCh(WCHAR ch)
{
    return s_aDirClassFromCharClass[CharClassFromChSlow(ch)];
}

inline BOOL
IsGlyphableChar(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fNeedsGlyphing;
}

inline BOOL
IsCombiningMark(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fCombiningMark;
}

inline BOOL
IsZeroWidthChar(WCHAR ch)
{
    return s_aPropBitsFromCharClass[CharClassFromChSlow(ch)].fZeroWidth;
}

inline BOOL IsBiDiDiacritic(WCHAR ch)
{
    return (InRange(ch, 0x0591, 0x06ED) && IsCombiningMark(ch));
}

#define WCH_CP1252_MIN WCHAR(0x0152)
#define WCH_CP1252_MAX WCHAR(0x2122)

BYTE InWindows1252ButNotInLatin1Helper(WCHAR ch);

inline BYTE InWindows1252ButNotInLatin1(WCHAR ch)
{
    return InRange(ch, WCH_CP1252_MIN, WCH_CP1252_MAX) ? InWindows1252ButNotInLatin1Helper(ch) : 0;
}
    
#pragma INCMSG("--- End 'txtdefs.h'")
#else
#pragma INCMSG("*** Dup 'txtdefs.h'")
#endif
