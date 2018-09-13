#include "private.h"
#include "detcbase.h"
#include "codepage.h"
#include "detcjpn.h"
#include "detckrn.h"
#include "fechrcnv.h"
#include "ichrcnv.h"
#include "cpdetect.h"


#define CONV_UU     12
#define CONV_UUW    10
#define CONV_UUWI   9
#define CONV_UW     6
#define CONV_UWI    5
#define CONV_WI     3

#define MAX_CHAR_SIZE   4

#define MAPUSERDEF(x) (((x) == 50000) ? 1252 : (x))
#define CONVERT_IS_VALIDCODEPAGE(x) (((x) == CP_USER_DEFINED) ? TRUE: IsValidCodePage(x))
#define CONV_CHK_NLS 0x00000001

struct ENCODINGINFO
{
    DWORD       dwEncoding;
    DWORD       dwCodePage;
    BYTE        bTypeUUIW;
    CP_STATE    nCP_State ;                 // whether this is a valid windows codepage  ?
    DWORD       dwFlags;                    // give us more flexibilities to handle different encodings differently
};

static WCHAR UniocdeSignature = { 0xFFFE } ;

/*
    Bit 4 (16) - Unicode <-> Internet Encoding
    Bit 3 (8) - UTF8, UTF7
    Bit 2 (4) - Unicode
    Bit 1 (2) - Windows CodePage
    Bit 0 (1) - Internet Encoding

     P.S. if bit 4 is set, it means it should convert between Unicode and Internet
     Encoding directly, no intermediate step - Windows CodePage
*/

// these codepages including Unicode need special convertor
static struct ENCODINGINFO aEncodingInfo[] =
{

    {  CP_JPN_SJ,            932,       0x02,   INVALID_CP,     0 }, // W-Japanese Shift JIS
    {  CP_CHN_GB,            936,       0x02,   INVALID_CP,     0 }, // W-Simplified Chinese
    {  CP_KOR_5601,          949,       0x02,   INVALID_CP,     0 }, // W-Krean Unified Hangul
    {  CP_TWN,               950,       0x02,   INVALID_CP,     0 }, // W-Traditional Chinese
    {  CP_UCS_2,               0,       0x04,   INVALID_CP,     0 }, // U-Unicode 
    {  CP_UCS_2_BE,            0,       0x04,   INVALID_CP,     0 }, // U-Unicode Big Endian
    {  CP_1252,             1252,       0x02,   INVALID_CP,     0 }, // W-Latin 1
    {  CP_20127,            1252,       0x11,   INVALID_CP,     CONV_CHK_NLS }, // US ASCII
    {  CP_ISO_8859_1,       1252,       0x11,   INVALID_CP,     CONV_CHK_NLS }, // I-ISO 8859-1 Latin 1 
    {  CP_ISO_8859_15,      1252,       0x11,   INVALID_CP,     CONV_CHK_NLS }, // I-ISO 8859-1 Latin 1 
    {  CP_AUTO,             1252,       0x01,   INVALID_CP,     0 }, // General auto detect 
    {  CP_ISO_2022_JP,       932,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-JP No Halfwidth Katakana 
    {  CP_ISO_2022_JP_ESC,   932,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-JP w/esc Halfwidth Katakana 
    {  CP_ISO_2022_JP_SIO,   932,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-JP w/sio Halfwidth Katakana 
    {  CP_ISO_2022_KR,       949,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-KR
    {  CP_ISO_2022_TW,       950,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-TW
    {  CP_ISO_2022_CH,       936,       0x01,   INVALID_CP,     0 }, // I-ISO 2022-CH
    {  CP_JP_AUTO,           932,       0x01,   INVALID_CP,     0 }, // JP auto detect 
    {  CP_CHS_AUTO,          936,       0x01,   INVALID_CP,     0 }, // Simplified Chinese auto detect 
    {  CP_KR_AUTO,           949,       0x01,   INVALID_CP,     0 }, // KR auto detect 
    {  CP_CHT_AUTO,          950,       0x01,   INVALID_CP,     0 }, // Traditional Chinese auto detect 
    {  CP_CYRILLIC_AUTO,    1251,       0x01,   INVALID_CP,     0 }, // Cyrillic auto detect 
    {  CP_GREEK_AUTO,       1253,       0x01,   INVALID_CP,     0 }, // Greek auto detect 
    {  CP_ARABIC_AUTO,      1256,       0x01,   INVALID_CP,     0 }, // Arabic auto detect 
    {  CP_EUC_JP,            932,       0x01,   INVALID_CP,     0 }, // EUC Japanese 
    {  CP_EUC_CH,            936,       0x01,   INVALID_CP,     0 }, // EUC Chinese 
    {  CP_EUC_KR,            949,       0x01,   INVALID_CP,     0 }, // EUC Korean
    {  CP_EUC_TW,            950,       0x01,   INVALID_CP,     0 }, // EUC Taiwanese 
    {  CP_CHN_HZ,            936,       0x01,   INVALID_CP,     0 }, // Simplify Chinese HZ-GB 
    {  CP_UTF_7,               0,       0x08,   INVALID_CP,     0 }, // U-UTF7 
    {  CP_UTF_8,               0,       0x08,   INVALID_CP,     0 }, // U-UTF8 
};


// HTML name entity table for Latin-1 Supplement - from 0x00A0-0x00FF

#define NAME_ENTITY_OFFSET  0x00A0
#define NAME_ENTITY_MAX     0x00FF
#define NAME_ENTITY_ENTRY   96

static CHAR *g_lpstrNameEntity[NAME_ENTITY_ENTRY] =
{
    "&nbsp;",   // "&#160;" -- no-break space = non-breaking space,
    "&iexcl;",  // "&#161;" -- inverted exclamation mark, U+00A1 ISOnum -->
    "&cent;",   // "&#162;" -- cent sign, U+00A2 ISOnum -->
    "&pound;",  // "&#163;" -- pound sign, U+00A3 ISOnum -->
    "&curren;", // "&#164;" -- currency sign, U+00A4 ISOnum -->
    "&yen;",    // "&#165;" -- yen sign = yuan sign, U+00A5 ISOnum -->
    "&brvbar;", // "&#166;" -- broken bar = broken vertical bar,
    "&sect;",   // "&#167;" -- section sign, U+00A7 ISOnum -->
    "&uml;",    // "&#168;" -- diaeresis = spacing diaeresis,
    "&copy;",   // "&#169;" -- copyright sign, U+00A9 ISOnum -->
    "&ordf;",   // "&#170;" -- feminine ordinal indicator, U+00AA ISOnum -->
    "&laquo;",  // "&#171;" -- left-pointing double angle quotation mark
    "&not;",    // "&#172;" -- not sign = discretionary hyphen,
    "&shy;",    // "&#173;" -- soft hyphen = discretionary hyphen,
    "&reg;",    // "&#174;" -- registered sign = registered trade mark sign,
    "&macr;",   // "&#175;" -- macron = spacing macron = overline
    "&deg;",    // "&#176;" -- degree sign, U+00B0 ISOnum -->
    "&plusmn;", // "&#177;" -- plus-minus sign = plus-or-minus sign,
    "&sup2;",   // "&#178;" -- superscript two = superscript digit two
    "&sup3;",   // "&#179;" -- superscript three = superscript digit three
    "&acute;",  // "&#180;" -- acute accent = spacing acute,
    "&micro;",  // "&#181;" -- micro sign, U+00B5 ISOnum -->
    "&para;",   // "&#182;" -- pilcrow sign = paragraph sign,
    "&middot;", // "&#183;" -- middle dot = Georgian comma
    "&cedil;",  // "&#184;" -- cedilla = spacing cedilla, U+00B8 ISOdia -->
    "&sup1;",   // "&#185;" -- superscript one = superscript digit one,
    "&ordm;",   // "&#186;" -- masculine ordinal indicator,
    "&raquo;",  // "&#187;" -- right-pointing double angle quotation mark
    "&frac14;", // "&#188;" -- vulgar fraction one quarter
    "&frac12;", // "&#189;" -- vulgar fraction one half
    "&frac34;", // "&#190;" -- vulgar fraction three quarters
    "&iquest;", // "&#191;" -- inverted question mark
    "&Agrave;", // "&#192;" -- latin capital letter A with grave
    "&Aacute;", // "&#193;" -- latin capital letter A with acute,
    "&Acirc;",  // "&#194;" -- latin capital letter A with circumflex,
    "&Atilde;", // "&#195;" -- latin capital letter A with tilde,
    "&Auml;",   // "&#196;" -- latin capital letter A with diaeresis,
    "&Aring;",  // "&#197;" -- latin capital letter A with ring above
    "&AElig;",  // "&#198;" -- latin capital letter AE
    "&Ccedil;", // "&#199;" -- latin capital letter C with cedilla,
    "&Egrave;", // "&#200;" -- latin capital letter E with grave,
    "&Eacute;", // "&#201;" -- latin capital letter E with acute,
    "&Ecirc;",  // "&#202;" -- latin capital letter E with circumflex,
    "&Euml;",   // "&#203;" -- latin capital letter E with diaeresis,
    "&Igrave;", // "&#204;" -- latin capital letter I with grave,
    "&Iacute;", // "&#205;" -- latin capital letter I with acute,
    "&Icirc;",  // "&#206;" -- latin capital letter I with circumflex,
    "&Iuml;",   // "&#207;" -- latin capital letter I with diaeresis,
    "&ETH;",    // "&#208;" -- latin capital letter ETH, U+00D0 ISOlat1 -->
    "&Ntilde;", // "&#209;" -- latin capital letter N with tilde,
    "&Ograve;", // "&#210;" -- latin capital letter O with grave,
    "&Oacute;", // "&#211;" -- latin capital letter O with acute,
    "&Ocirc;",  // "&#212;" -- latin capital letter O with circumflex,
    "&Otilde;", // "&#213;" -- latin capital letter O with tilde,
    "&Ouml;",   // "&#214;" -- latin capital letter O with diaeresis,
    "&times;",  // "&#215;" -- multiplication sign, U+00D7 ISOnum -->
    "&Oslash;", // "&#216;" -- latin capital letter O with stroke
    "&Ugrave;", // "&#217;" -- latin capital letter U with grave,
    "&Uacute;", // "&#218;" -- latin capital letter U with acute,
    "&Ucirc;",  // "&#219;" -- latin capital letter U with circumflex,
    "&Uuml;",   // "&#220;" -- latin capital letter U with diaeresis,
    "&Yacute;", // "&#221;" -- latin capital letter Y with acute,
    "&THORN;",  // "&#222;" -- latin capital letter THORN,
    "&szlig;",  // "&#223;" -- latin small letter sharp s = ess-zed,
    "&agrave;", // "&#224;" -- latin small letter a with grave
    "&aacute;", // "&#225;" -- latin small letter a with acute,
    "&acirc;",  // "&#226;" -- latin small letter a with circumflex,
    "&atilde;", // "&#227;" -- latin small letter a with tilde,
    "&auml;",   // "&#228;" -- latin small letter a with diaeresis,
    "&aring;",  // "&#229;" -- latin small letter a with ring above
    "&aelig;",  // "&#230;" -- latin small letter ae
    "&ccedil;", // "&#231;" -- latin small letter c with cedilla,
    "&egrave;", // "&#232;" -- latin small letter e with grave,
    "&eacute;", // "&#233;" -- latin small letter e with acute,
    "&ecirc;",  // "&#234;" -- latin small letter e with circumflex,
    "&euml;",   // "&#235;" -- latin small letter e with diaeresis,
    "&igrave;", // "&#236;" -- latin small letter i with grave,
    "&iacute;", // "&#237;" -- latin small letter i with acute,
    "&icirc;",  // "&#238;" -- latin small letter i with circumflex,
    "&iuml;",   // "&#239;" -- latin small letter i with diaeresis,
    "&eth;",    // "&#240;" -- latin small letter eth, U+00F0 ISOlat1 -->
    "&ntilde;", // "&#241;" -- latin small letter n with tilde,
    "&ograve;", // "&#242;" -- latin small letter o with grave,
    "&oacute;", // "&#243;" -- latin small letter o with acute,
    "&ocirc;",  // "&#244;" -- latin small letter o with circumflex,
    "&otilde;", // "&#245;" -- latin small letter o with tilde,
    "&ouml;",   // "&#246;" -- latin small letter o with diaeresis,
    "&divide;", // "&#247;" -- division sign, U+00F7 ISOnum -->
    "&oslash;", // "&#248;" -- latin small letter o with stroke,
    "&ugrave;", // "&#249;" -- latin small letter u with grave,
    "&uacute;", // "&#250;" -- latin small letter u with acute,
    "&ucirc;",  // "&#251;" -- latin small letter u with circumflex,
    "&uuml;",   // "&#252;" -- latin small letter u with diaeresis,
    "&yacute;", // "&#253;" -- latin small letter y with acute,
    "&thorn;",  // "&#254;" -- latin small letter thorn with,
    "&yuml;",   // "&#255;" -- latin small letter y with diaeresis,
};


#ifdef MORE_NAME_ENTITY   // in case we decide to do more name entity latter
// Additional HTML 4.0 name entity table for CP 1252 extension character set
#define CP1252EXT_BASE  (UINT)0x0080
#define CP1252EXT_MAX   (UINT)0x009F
#define NONUNI          0xFFFF
#define UNDEFCHAR       "???????"
#define CP1252EXT_NCR_SIZE  7

struct NAME_ENTITY_EXT
{
    UWORD     uwUniCode;
    LPCTSTR   lpszNameEntity;
};

static struct NAME_ENTITY_EXT aNameEntityExt[] =
{
//      UniCode  NCR_Enty          Name_Enty        CP1252Ext  Comment
    {   0x20AC,  "&#8364;"  },  // "&euro;"    },  // &#128;  #EURO SIGN
//  {   NONUNI,  UNDEFCHAR  },  // "&;"        },  // &#129;  #UNDEFINED
    {   0x201A,  "&#8218;"  },  // "&sbquo;"   },  // &#130;  #SINGLE LOW-9 QUOTATION MARK
    {   0x0192,  "&#0402;"  },  // "&fnof;"    },  // &#131;  #LATIN SMALL LETTER F WITH HOOK
    {   0x201E,  "&#8222;"  },  // "&bdquo;"   },  // &#132;  #DOUBLE LOW-9 QUOTATION MARK
    {   0x2026,  "&#8230;"  },  // "&hellip;"  },  // &#133;  #HORIZONTAL ELLIPSIS
    {   0x2020,  "&#8224;"  },  // "&dagger;"  },  // &#134;  #DAGGER
    {   0x2021,  "&#8225;"  },  // "&Dagger;"  },  // &#135;  #DOUBLE DAGGER
    {   0x02C6,  "&#0710;"  },  // "&circ;"    },  // &#136;  #MODIFIER LETTER CIRCUMFLEX ACCENT
    {   0x2030,  "&#8240;"  },  // "&permil;"  },  // &#137;  #PER MILLE SIGN
    {   0x0160,  "&#0352;"  },  // "&Scaron;"  },  // &#138;  #LATIN CAPITAL LETTER S WITH CARON
    {   0x2039,  "&#8249;"  },  // "&lsaquo;"  },  // &#139;  #SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    {   0x0152,  "&#0338;"  },  // "&OElig;"   },  // &#140;  #LATIN CAPITAL LIGATURE OE
//  {   NONUNI,  UNDEFCHAR  },  // "&;"        },  // &#141;  #UNDEFINED
    {   0x017D,  "&#0381;"  },  // "&;"        },  // &#142;  #LATIN CAPITAL LETTER Z WITH CARON, ***no name entity defined in HTML 4.0***
//  {   NONUNI,  UNDEFCHAR  },  // "&;"        },  // &#143;  #UNDEFINED
//  {   NONUNI,  UNDEFCHAR  },  // "&;"        },  // &#144;  #UNDEFINED
    {   0x2018,  "&#8216;"  },  // "&lsquo;"   },  // &#145;  #LEFT SINGLE QUOTATION MARK
    {   0x2019,  "&#8217;"  },  // "&rsquo;"   },  // &#146;  #RIGHT SINGLE QUOTATION MARK
    {   0x201C,  "&#8220;"  },  // "&ldquo;"   },  // &#147;  #LEFT DOUBLE QUOTATION MARK
    {   0x201D,  "&#8221;"  },  // "&rdquo;"   },  // &#148;  #RIGHT DOUBLE QUOTATION MARK
    {   0x2022,  "&#8226;"  },  // "&bull;"    },  // &#149;  #BULLET
    {   0x2013,  "&#8211;"  },  // "&ndash;"   },  // &#150;  #EN DASH
    {   0x2014,  "&#8212;"  },  // "&mdash;"   },  // &#151;  #EM DASH
    {   0x20DC,  "&#0732;"  },  // "&tilde;"   },  // &#152;  #SMALL TILDE
    {   0x2122,  "&#8482;"  },  // "&trade;"   },  // &#153;  #TRADE MARK SIGN
    {   0x0161,  "&#0353;"  },  // "&scaron;"  },  // &#154;  #LATIN SMALL LETTER S WITH CARON
    {   0x203A,  "&#8250;"  },  // "&rsaquo;"  },  // &#155;  #SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    {   0x0153,  "&#0339;"  },  // "&oelig;"   },  // &#156;  #LATIN SMALL LIGATURE OE
//  {   NONUNI,  UNDEFCHAR  },  // "&;"        },  // &#157;  #UNDEFINED
    {   0x017E,  "&#0382;"  },  // "&;"        },  // &#158;  #LATIN SMALL LETTER Z WITH CARON, ***no name entity defined in HTML 4.0***
    {   0x0178,  "&#0376;"  },  // "&Yuml;"    },  // &#159;  #LATIN CAPITAL LETTER Y WITH DIAERESIS
};
#endif


HRESULT WINAPI DoConvertINetString(LPDWORD lpdwMode, BOOL fInbound, UINT uCodePage, int nCodeSet, LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDestStr, int cchDest, LPINT lpnSize);


/******************************************************************************
*****************************   U T I L I T I E S   ***************************
******************************************************************************/
void DataByteSwap(LPSTR DataBuf, int len )
{
    int i ;
    UCHAR tmpData ;

    if ( len )
        for ( i = 0 ; i < len-1 ; i+=2 )
        {
            tmpData = DataBuf[i] ;
            DataBuf[i] = DataBuf[i+1] ;
            DataBuf[i+1] = tmpData ;
        }

    return ;
}

void CheckUnicodeDataType(DWORD dwDstEncoding, LPSTR DataBuf, int len )
{
    
    if ( DataBuf && len )
    {
        if ( dwDstEncoding == CP_UCS_2_BE )
            DataByteSwap(DataBuf,len);
    }
    return ;
}

/******************************************************************************
******************   C O N V E R T   I N E T   S T R I N G   ******************
******************************************************************************/
HRESULT CICharConverter::UnicodeToMultiByteEncoding(DWORD dwDstEncoding, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{

    int nBuffSize = 0, i ;
    BOOL UseDefChar = FALSE ;
    LPSTR lpDefFallBack = NULL ;
    UCHAR DefaultCharBuff[3]; // possible DBCS + null    
    HRESULT hr = E_FAIL;
    int _nDstSize = *lpnDstSize;    

    if ( _dwUnicodeEncoding == CP_UCS_2_BE && _cvt_count == 0 )
    {
       if ( _lpUnicodeStr = (LPSTR)LocalAlloc(LPTR, *lpnSrcSize ) )
       {
          MoveMemory(_lpUnicodeStr, lpSrcStr, *lpnSrcSize ) ;
          lpSrcStr = _lpUnicodeStr ;
       }
       else
       {
          hr = E_OUTOFMEMORY;
          goto EXIT;
       }
    }

    CheckUnicodeDataType(_dwUnicodeEncoding, (LPSTR) lpSrcStr, *lpnSrcSize);

    if ( *lpnSrcSize )
        nBuffSize = *lpnSrcSize / sizeof(WCHAR);

    // We force to use MLang NO_BEST_FIT_CHAR check on ISCII encoding since system don't accept default chars
    if (IS_ISCII_CP(dwDstEncoding) && (dwFlag & MLCONVCHARF_USEDEFCHAR))
        dwFlag |= MLCONVCHARF_NOBESTFITCHARS;

    if ( lpFallBack && ( dwFlag & MLCONVCHARF_USEDEFCHAR ))
    {
        // only take SBCS, no DBCS character
        if ( 1 == WideCharToMultiByte(MAPUSERDEF(dwDstEncoding), 0,
                               (LPCWSTR)lpFallBack, 1,
                               (LPSTR)DefaultCharBuff, sizeof(DefaultCharBuff), NULL, NULL ))
            lpDefFallBack = (LPSTR) DefaultCharBuff;        
    }

    if(!(*lpnDstSize = WideCharToMultiByte(MAPUSERDEF(dwDstEncoding), 0,
                                           (LPCWSTR)lpSrcStr, nBuffSize,
                                           lpDstStr, *lpnDstSize, IS_ISCII_CP(dwDstEncoding)? NULL:(LPCSTR)lpDefFallBack, IS_ISCII_CP(dwDstEncoding)? NULL:&UseDefChar)))
    {
        hr = E_FAIL;
        goto EXIT;
    }

    if ( !_cvt_count ) // save SrcSize if it is the first time conversion
        _nSrcSize = nBuffSize * sizeof(WCHAR);

    if (*lpnDstSize)
    {
        if (dwFlag & ( MLCONVCHARF_NCR_ENTITIZE | MLCONVCHARF_NAME_ENTITIZE | MLCONVCHARF_NOBESTFITCHARS ))
        {
            char    *lpDstStrTmp = lpDstStr;
            WCHAR   *lpwStrTmp = NULL;
            WCHAR   *lpwStrTmpSave = NULL;
            char    *lpDstStrTmp2 = NULL;
            char    *lpDstStrTmp2Save = NULL;
            int     cCount, ConvCount = 0, nCount = 0;
            WCHAR   *lpwSrcStrTmp = (WCHAR *)lpSrcStr;
            int     *lpBCharOffset = NULL;
            int     *lpBCharOffsetSave = NULL;

            if (!(lpwStrTmpSave = lpwStrTmp = (WCHAR *)LocalAlloc(LPTR, _nSrcSize)))
            {
                hr = E_OUTOFMEMORY;
                goto ENTITIZE_DONE;
            }

            // Make sure we have real converted buffer to check BEST_FIT_CHAR and DEFAULT_CHAR
            if (!_nDstSize)
            {
                lpDstStrTmp2Save = lpDstStrTmp2 = (char *)LocalAlloc(LPTR, *lpnDstSize);
                if (lpDstStrTmp2)
                {
                    WideCharToMultiByte(MAPUSERDEF(dwDstEncoding), 0,
                               (LPCWSTR)lpSrcStr, nBuffSize,
                               lpDstStrTmp2, *lpnDstSize, NULL, NULL );
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto ENTITIZE_DONE;
                }
            }

//BUGBUG: dest. buffer size is too big
            if (nBuffSize == 
                MultiByteToWideChar(MAPUSERDEF(dwDstEncoding), 0, _nDstSize? lpDstStr : lpDstStrTmp2, *lpnDstSize, lpwStrTmp, _nSrcSize))
            {
                // Pre scan to get number of best fit chars.
                for (i=0; i<nBuffSize; i++)
                {
                    // make special case for ?(yen sign) in Shift-JIS
                    if (*lpwStrTmp++ != *lpwSrcStrTmp++)
                    {
                        if ((dwDstEncoding == CP_JPN_SJ) && (*(lpwSrcStrTmp - 1) == 0x00A5))
                            *(lpwStrTmp - 1) = 0x00A5;
                        else
                            nCount ++;
                    }
                }

                lpwSrcStrTmp -= nBuffSize;
                lpwStrTmp -= nBuffSize;

                if (nCount)
                {
                    int j = 0;

                    if (!(dwFlag & ( MLCONVCHARF_NCR_ENTITIZE | MLCONVCHARF_NAME_ENTITIZE | MLCONVCHARF_USEDEFCHAR)))
                    {
                        hr = E_FAIL;
                        goto ENTITIZE_DONE;
                    }

                    if (!(lpBCharOffsetSave = lpBCharOffset = (int *) LocalAlloc(LPTR, nCount*sizeof(int))))
                    {
                        hr = E_OUTOFMEMORY;
                        goto ENTITIZE_DONE;
                    }

                    // Record the offset position of each best fit char.
                    for (i=0; i<nBuffSize; i++)
                    {
                        if (*lpwStrTmp++ != *lpwSrcStrTmp++)
                        {
                            *lpBCharOffset = i-j;
                            lpBCharOffset++;
                            j = i+1;
                        }
                    }

                    lpBCharOffset -= nCount;
                    lpwSrcStrTmp -= nBuffSize;
                    lpwStrTmp -= nBuffSize;

                    for (i=0; i<nCount; i++)
                    {
                        BOOL bIsSurrogatePair = FALSE;

                        if (*lpBCharOffset)
                        {
//BUGBUG: dest. buffer size is too big
                            cCount = WideCharToMultiByte(MAPUSERDEF(dwDstEncoding), 0,
                                   (LPCWSTR)lpwSrcStrTmp, *lpBCharOffset,
                                   lpDstStrTmp,  _nDstSize ? (*lpBCharOffset)*2 : 0, NULL, NULL );

                            ConvCount += cCount;
                            if (_nDstSize)
                            {
                                lpDstStrTmp += cCount;
                            }
                            lpwSrcStrTmp += *lpBCharOffset;
                        }

                        BOOL fConverted = FALSE;

                        // check if unconvertable character falls in NAME ENTITY area
                        if (dwFlag & MLCONVCHARF_NAME_ENTITIZE)
                        {
                            // for beta2, make assmption that name entity implys NCR.
                            dwFlag |= MLCONVCHARF_NCR_ENTITIZE;

#ifdef MORE_NAME_ENTITY   // in case we decide do more name entity latter
                            BOOL      fDoNEnty = FALSE;
                            LPCTSTR   lpszNEnty = NULL;

                            // check if character is in the Latin-1 Supplement range
                            if ((*lpwSrcStrTmp >= NAME_ENTITY_OFFSET) && (*lpwSrcStrTmp <= NAME_ENTITY_MAX ))
                            {
                                fDoNEnty = TRUE;
                                lpszNEnty = g_lpstrNameEntity[(*lpwSrcStrTmp) - NAME_ENTITY_OFFSET];
                            }

                            // check if character is in the additional name entity table for CP 1252 extension
                            if (!fDoNEnty)
                            {
                                for (int idx = 0; idx < ARRAYSIZE(aNameEntityExt); idx++)
                                    if (*lpwSrcStrTmp == aNameEntityExt[idx].uwUniCode)
                                    {
                                        fDoNEnty = TRUE;
                                        lpszNEnty = aNameEntityExt[idx].lpszNameEntity;
                                        break;
                                    }
                            }

                            if (fDoNEnty)
                            {
                                cCount = lstrlenA(lpszNEnty);
                                if (_nDstSize)
                                {
                                    CopyMemory(lpDstStrTmp, lpszNEnty, cCount);
                                    lpDstStrTmp += cCount ;
                                }

                                ConvCount += cCount;
                                fConverted = TRUE;
                            }
#else
                            // check if character is in the Latin-1 Supplement range
                            if ((*lpwSrcStrTmp >= NAME_ENTITY_OFFSET)
                                && (*lpwSrcStrTmp < ARRAYSIZE(g_lpstrNameEntity)+NAME_ENTITY_OFFSET))
                                
                            {
                                LPCTSTR   lpszNEnty = NULL;

                                if (!(lpszNEnty = g_lpstrNameEntity[(*lpwSrcStrTmp) - NAME_ENTITY_OFFSET]))
                                {
#ifdef DEBUG
                                    AssertMsg((BOOL)FALSE, "Name entity table broken"); 
#endif
                                    hr = E_FAIL;
                                    goto ENTITIZE_DONE;
                                }

                                    cCount = lstrlenA(lpszNEnty);
                                    if (_nDstSize)
                                    {
                                        CopyMemory(lpDstStrTmp, lpszNEnty, cCount);
                                        lpDstStrTmp += cCount ;
                                    }
                                
                                ConvCount += cCount;
                                fConverted = TRUE;
                            }
#endif
                        }

                        // check if NCR requested
                        if ((!fConverted) && (dwFlag & MLCONVCHARF_NCR_ENTITIZE))
                        {
                            if ((nCount-i >= 2) &&
                                (*lpwSrcStrTmp >= 0xD800 && *lpwSrcStrTmp <= 0xDBFF) &&
                                (*(lpwSrcStrTmp+1) >= 0xDC00 && *(lpwSrcStrTmp+1) <= 0xDFFF))
                                bIsSurrogatePair = TRUE;
                            else
                                bIsSurrogatePair = FALSE;
                          
                            if (_nDstSize)
                            {
                                lpDstStrTmp[0] = '&' ;
                                lpDstStrTmp[1] = '#' ;
                                lpDstStrTmp += 2 ;
                                // If it is a Unicode surrogates pair, we convert it to real Unicode value
                                if (bIsSurrogatePair)
                                {
                                    DWORD dwUnicode = ((*lpwSrcStrTmp - 0xD800) << 10) + *(lpwSrcStrTmp+1) - 0xDC00 + 0x10000;
                                    _ultoa( dwUnicode, (char*)lpDstStrTmp, 10);
                                }
                                else
                                    _ultoa( *lpwSrcStrTmp, (char*)lpDstStrTmp, 10);
                                cCount = lstrlenA(lpDstStrTmp);
                                lpDstStrTmp += cCount;
                                ConvCount += cCount;
                                *(lpDstStrTmp++) = ';' ;
                            }
                            else
                            {
                                char szTmpString[10];
                                if (bIsSurrogatePair)
                                {
                                    DWORD dwUnicode = ((*lpwSrcStrTmp - 0xD800) << 10) + *(lpwSrcStrTmp+1) - 0xDC00 + 0x10000;
                                    _ultoa( dwUnicode, szTmpString, 10);
                                }
                                else
                                    _ultoa( *lpwSrcStrTmp, szTmpString, 10);
                                ConvCount += lstrlenA(szTmpString);
                            }
                        
                            fConverted = TRUE;
                            ConvCount += 3;                    
                        }

                        // handle MLCONVCHARF_USEDEFCHAR here - less priority and default method
                        if (!fConverted)
                        {
                            if (_nDstSize)
                            {
                                *lpDstStrTmp = lpDefFallBack ? *lpDefFallBack : '?';
                                lpDstStrTmp++;
                            }

                            ConvCount++;
                            if (!UseDefChar)
                                UseDefChar = TRUE;
                        }

                        lpBCharOffset++;
                        lpwSrcStrTmp++;
                        // Skip next character if it is a Unicode surrogates pair
                        if (bIsSurrogatePair)
                        {
                            lpBCharOffset++;
                            lpwSrcStrTmp++;
                            i++;
                        }
                    }
                    lpBCharOffset -= nCount ;
                }

                int nRemain = (*lpnSrcSize - (int)((char*)lpwSrcStrTmp - (char *)lpSrcStr))/sizeof(WCHAR);

//BUGBUG: dest. buffer size is too big
                ConvCount += WideCharToMultiByte(MAPUSERDEF(dwDstEncoding), 0,
                                   (LPCWSTR)lpwSrcStrTmp, nRemain,
                                   lpDstStrTmp, _nDstSize ? nRemain*2 : 0, NULL, NULL );

                *lpnDstSize = ConvCount ;

                hr = S_OK;
            } 
            else
            {
                hr = E_FAIL;
            }

ENTITIZE_DONE:
            if (lpwStrTmpSave)
                LocalFree(lpwStrTmpSave);
            if (lpDstStrTmp2Save)
                LocalFree(lpDstStrTmp2Save);
            if (lpBCharOffsetSave)
                LocalFree(lpBCharOffsetSave);
        }
        else
        {
            hr = S_OK;
        }       

        if (S_OK == hr && UseDefChar)
            hr = S_FALSE;
    }
    else
    {
        hr = E_FAIL;
    }

EXIT:
    return hr;
}

HRESULT CICharConverter::UTF78ToUnicode(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize)
{
    HRESULT hr ;

    hr = DoConvertINetString(lpdwMode, TRUE, CP_UCS_2, _dwUTFEncoding, lpSrcStr, lpnSrcSize, lpDstStr, *lpnDstSize, lpnDstSize);

    if ( !_cvt_count ) // save SrcSize if it is the first time conversion
        _nSrcSize = *lpnSrcSize ;

    CheckUnicodeDataType(_dwUnicodeEncoding, lpDstStr, *lpnDstSize);

    return hr ;
}

HRESULT CICharConverter::UnicodeToUTF78(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize)
{
    HRESULT hr ;

    if ( _dwUnicodeEncoding == CP_UCS_2_BE && _cvt_count == 0 )
    {
       if ( _lpUnicodeStr = (LPSTR)LocalAlloc(LPTR, *lpnSrcSize ) )
       {
          MoveMemory(_lpUnicodeStr, lpSrcStr, *lpnSrcSize ) ;
          lpSrcStr = _lpUnicodeStr ;
       }
       else
        return E_OUTOFMEMORY ;
    }

    CheckUnicodeDataType(_dwUnicodeEncoding, (LPSTR) lpSrcStr, *lpnSrcSize);

    hr = DoConvertINetString(lpdwMode, FALSE, CP_UCS_2, _dwUTFEncoding, lpSrcStr, lpnSrcSize, lpDstStr, *lpnDstSize, lpnDstSize);
    if ( !_cvt_count ) // save SrcSize if it is the first time conversion
        _nSrcSize = *lpnSrcSize ;


    return hr ;
}

HRESULT CICharConverter::UnicodeToWindowsCodePage(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;

    hr = UnicodeToMultiByteEncoding(_dwWinCodePage,lpSrcStr,lpnSrcSize,lpDstStr,lpnDstSize,dwFlag,lpFallBack);

    return hr ;
}

HRESULT CICharConverter::UnicodeToInternetEncoding(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;

    hr = UnicodeToMultiByteEncoding(_dwInternetEncoding,lpSrcStr,lpnSrcSize,lpDstStr,lpnDstSize,dwFlag,lpFallBack);

    return hr ;
}

HRESULT CICharConverter::InternetEncodingToUnicode(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize)
{
    int cch = 0 ;
    int cb = *lpnSrcSize;

    if ( !_cvt_count )
    {
        // If we have a multibyte character encoding, we are at risk of splitting
        // some characters at the read boundary.  We must Make sure we have a
        // discrete number of characters first.

        UINT uMax = MAX_CHAR_SIZE ;
        cb++; // pre-increment
        do
        {
            cch = MultiByteToWideChar( MAPUSERDEF(_dwInternetEncoding),
                                        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
                                        lpSrcStr, --cb,
                                        NULL, 0 );
            --uMax;
        } while (!cch && uMax && cb);
    }

    if ( cb == (*lpnSrcSize - MAX_CHAR_SIZE +1 ))  // if conversion problem isn't at the end of the string
        cb = *lpnSrcSize ; // restore orginal value

    *lpnDstSize = MultiByteToWideChar( MAPUSERDEF(_dwInternetEncoding), 0,
                               lpSrcStr, cb,
                               (LPWSTR)lpDstStr, *lpnDstSize/sizeof(WCHAR) );
    *lpnDstSize = *lpnDstSize * sizeof(WCHAR);
    if ( !_cvt_count ) // save SrcSize if it is the first time conversion
        _nSrcSize = cb ;

    CheckUnicodeDataType(_dwUnicodeEncoding, lpDstStr, *lpnDstSize);

    if (*lpnDstSize==0 && cb)
        return E_FAIL ;
    else
        return S_OK ;
}

HRESULT CICharConverter::WindowsCodePageToUnicode(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize)
{

    int cch = 0 ;
    int cb = *lpnSrcSize;

    if ( !_cvt_count )
    {
        UINT uMax = MAX_CHAR_SIZE ;
        cb++; // pre-increment
        do
        {
            cch = MultiByteToWideChar( MAPUSERDEF(_dwWinCodePage),
                                        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
                                        lpSrcStr, --cb,
                                        NULL, 0 );
            --uMax;
        } while (!cch && uMax && cb);
    }

    if ( cb == (*lpnSrcSize - MAX_CHAR_SIZE +1 ))  // if conversion problem isn't at the end of the string
        cb = *lpnSrcSize ; // restore orginal value

    *lpnDstSize = MultiByteToWideChar( MAPUSERDEF(_dwWinCodePage), 0,
                               lpSrcStr, cb,
                               (LPWSTR)lpDstStr, *lpnDstSize/sizeof(WCHAR) );
    *lpnDstSize = *lpnDstSize * sizeof(WCHAR);
    if ( !_cvt_count ) // save SrcSize if it is the first time conversion
        _nSrcSize = cb ;

    CheckUnicodeDataType(_dwUnicodeEncoding, lpDstStr, *lpnDstSize);

    if (*lpnDstSize==0 && cb)
        return E_FAIL ;
    else
        return S_OK ;
}

HRESULT CICharConverter::WindowsCodePageToInternetEncoding(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;

    // check if the conversion should go through Unicode indirectly
    if ( _dwConvertType & 0x10 )
        hr = WindowsCodePageToInternetEncodingWrap(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);
    else
    {

        hr = DoConvertINetString(lpdwMode, FALSE, _dwWinCodePage, _dwInternetEncoding, lpSrcStr, lpnSrcSize, lpDstStr, *lpnDstSize, lpnDstSize);

        if ( !_cvt_count ) // save SrcSize if it is the first time conversion
            _nSrcSize = *lpnSrcSize ;
    }
    return hr ;
}

HRESULT CICharConverter::InternetEncodingToWindowsCodePage(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;

    // check if the conversion should go through Unicode indirectly
    if ( _dwConvertType & 0x10 )
        hr = InternetEncodingToWindowsCodePageWrap(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);
    else
    {
        hr = DoConvertINetString(lpdwMode, TRUE, _dwWinCodePage, _dwInternetEncoding, lpSrcStr, lpnSrcSize, lpDstStr, *lpnDstSize, lpnDstSize);

        if ( !_cvt_count ) // save SrcSize if it is the first time conversion
            _nSrcSize = *lpnSrcSize ;
    }
    return hr ;
}

HRESULT CICharConverter::WindowsCodePageToInternetEncodingWrap(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    int nBuffSize = 0 ;
    int cb = *lpnSrcSize;
    UINT uMax = MAX_CHAR_SIZE ;
    BOOL UseDefChar = FALSE ;
    HRESULT hr = S_OK;

    if ( !_cvt_count )
    {
        cb++; // pre-increment
        do
        {
            nBuffSize = MultiByteToWideChar( MAPUSERDEF(_dwWinCodePage),
                                        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
                                        lpSrcStr, --cb,
                                        NULL, 0 );
            --uMax;
        } while (!nBuffSize && uMax && cb);
    }

    if ( cb == (*lpnSrcSize - MAX_CHAR_SIZE +1 ))  // if conversion problem isn't at the end of the string
        cb = *lpnSrcSize ; // restore orginal value

    if (!nBuffSize)  // in case there are illeage characters
        nBuffSize = cb ;

    if ( _lpInterm1Str = (LPSTR) LocalAlloc(LPTR, (nBuffSize * sizeof(WCHAR))))
    {
        nBuffSize = MultiByteToWideChar(MAPUSERDEF(_dwWinCodePage), 0,
                        lpSrcStr, cb, (LPWSTR)_lpInterm1Str, nBuffSize );

        int iSrcSizeTmp = nBuffSize * sizeof(WCHAR);
        hr = UnicodeToMultiByteEncoding(MAPUSERDEF(_dwInternetEncoding), (LPCSTR)_lpInterm1Str, &iSrcSizeTmp,
                                        lpDstStr, lpnDstSize, dwFlag, lpFallBack);
//        *lpnDstSize = WideCharToMultiByte( MAPUSERDEF(_dwInternetEncoding), 0,
//                        (LPCWSTR)_lpInterm1Str, nBuffSize, lpDstStr, *lpnDstSize, NULL, &UseDefChar );

        if ( !_cvt_count ) // save SrcSize if it is the first time conversion
            _nSrcSize = cb ;
    }
    else        
        hr = E_FAIL;

    if (hr == S_OK)
    {
        if (*lpnDstSize==0 && cb)
            hr = E_FAIL ;
        else 
        {
            if ( UseDefChar )
                return S_FALSE ;
            else
                return S_OK ;
        }
    }

    return hr;
}

HRESULT CICharConverter::InternetEncodingToWindowsCodePageWrap(LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{

    int nBuffSize = 0 ;
    int cb = *lpnSrcSize;
    UINT uMax = MAX_CHAR_SIZE ;
    BOOL UseDefChar = FALSE ;
    HRESULT hr = S_OK;

    if ( !_cvt_count )
    {
        cb++; // pre-increment
        do
        {
            nBuffSize = MultiByteToWideChar( MAPUSERDEF(_dwInternetEncoding),
                                        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
                                        lpSrcStr, --cb,
                                        NULL, 0 );
            --uMax;
        } while (!nBuffSize && uMax && cb);
    }

    if ( cb == (*lpnSrcSize - MAX_CHAR_SIZE +1 ))  // if conversion problem isn't at the end of the string
        cb = *lpnSrcSize ; // restore orginal value

    if (!nBuffSize)  // in case there are illeage characters
        nBuffSize = cb ;

    if ( _lpInterm1Str = (LPSTR) LocalAlloc(LPTR,nBuffSize * sizeof (WCHAR) ))
    {
        nBuffSize = MultiByteToWideChar( MAPUSERDEF(_dwInternetEncoding), 0,
                        lpSrcStr, cb, (LPWSTR)_lpInterm1Str, nBuffSize );

        int iSrcSizeTmp = nBuffSize * sizeof(WCHAR);
        hr = UnicodeToMultiByteEncoding(MAPUSERDEF(_dwWinCodePage), (LPCSTR)_lpInterm1Str, &iSrcSizeTmp,
                                        lpDstStr, lpnDstSize, dwFlag, lpFallBack);
//        *lpnDstSize = WideCharToMultiByte( MAPUSERDEF(_dwWinCodePage), 0,
//                        (LPCWSTR)_lpInterm1Str, nBuffSize, lpDstStr, *lpnDstSize, NULL, &UseDefChar );

        if ( !_cvt_count ) // save SrcSize if it is the first time conversion
            _nSrcSize = cb ;
    }
    else
        hr = E_FAIL;

    if (hr == S_OK)
    {
        if (*lpnDstSize==0 && cb)
            hr = E_FAIL ;
        else 
        {
            if ( UseDefChar )
                return S_FALSE ;
            else
                return S_OK ;
        }
    }

    return hr;
}

HRESULT CICharConverter::ConvertIWUU(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    int nBuffSize = 0 ;
    HRESULT hr = S_OK ;
    HRESULT hrWarnings = S_OK ;

    // InternetEncodingToWindowsCodePage
    if ( _dwConvertType % 2 && _dwConvertType < 21 ) /* start from Internet Encoding */
    {
        if ( _dwConvertType == 5 || _dwConvertType == 9 ) /* use interm buffer */
        {
            hr = InternetEncodingToWindowsCodePage(lpdwMode, lpSrcStr, lpnSrcSize, NULL, &nBuffSize, dwFlag, lpFallBack);
            if ( _lpInterm1Str = (LPSTR) LocalAlloc(LPTR,nBuffSize) )
            {
                hr = InternetEncodingToWindowsCodePage(lpdwMode, lpSrcStr, lpnSrcSize, _lpInterm1Str, &nBuffSize, dwFlag, lpFallBack);
                lpSrcStr = _lpInterm1Str ;
                *lpnSrcSize = nBuffSize ;
            }
            else
                goto fail ;
        }
        else
            hr = InternetEncodingToWindowsCodePage(lpdwMode, lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);
        _cvt_count ++ ;
    }

    if ( hr != S_OK )
        hrWarnings = hr ;
        
    // WindowsCodePageToUnicode or InternetEncodingToUnicode
    if ( _dwConvertType == 21 || _dwConvertType == 25 )
    {
        if ( _dwConvertType == 21 )
            hr = InternetEncodingToUnicode(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize);
        else // _dwConvertType == 25 
        {
            hr = InternetEncodingToUnicode(lpSrcStr, lpnSrcSize, NULL, &nBuffSize);
            if ( _lpInterm1Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
            {
                hr = InternetEncodingToUnicode(lpSrcStr, lpnSrcSize, _lpInterm1Str, &nBuffSize);
                lpSrcStr = _lpInterm1Str ;
                *lpnSrcSize = nBuffSize ;
            }
            else
                goto fail ;
        }
        _cvt_count ++ ;
    }
    else if ( _dwConvertType >= 4 && _dwConvertType <= 10 )
    {
        if ( _dwConvertType > 8 )
        {
            nBuffSize = 0 ;
            hr = WindowsCodePageToUnicode(lpSrcStr, lpnSrcSize, NULL, &nBuffSize);
            if ( _cvt_count )
            {
                if ( _lpInterm2Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
                {
                    hr = WindowsCodePageToUnicode(lpSrcStr, lpnSrcSize, _lpInterm2Str, &nBuffSize);
                    lpSrcStr = _lpInterm2Str ;
                    *lpnSrcSize = nBuffSize ;
                }
                else
                    goto fail ;

            }
            else
            {
                if ( _lpInterm1Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
                {
                    hr = WindowsCodePageToUnicode(lpSrcStr, lpnSrcSize, _lpInterm1Str, &nBuffSize);
                    lpSrcStr = _lpInterm1Str ;
                    *lpnSrcSize = nBuffSize ;
                }
                else
                    goto fail ;
            }
        }
        else
            hr = WindowsCodePageToUnicode(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize);
        _cvt_count ++ ;
    }

    if ( hr != S_OK )
        hrWarnings = hr ;

    // UnicodeToUTF78
    if ( _dwConvertType & 0x08 )
#ifndef UNIX
        hr = UnicodeToUTF78(lpdwMode, lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize);
#else
        {
        /* we now hack the lpSrcStr to be the same as 2 byte Unicode so mlang
         * lowlevel code can work right.
         */
        LPWSTR lpwSrcStr = (LPWSTR)lpSrcStr;
        INT tmpSize = *lpnSrcSize/sizeof(WCHAR);
        UCHAR *pTmp = new UCHAR[(tmpSize+1)*2];
        if(pTmp) {
            for(int i = 0; i < tmpSize; i++) {
                pTmp[i*2] = *lpwSrcStr++;
                pTmp[i*2+1] = 0x00;
            }
            pTmp[i*2] = pTmp[i*2+1] = 0x00;
            tmpSize *= 2;
            hr = UnicodeToUTF78(lpdwMode, (LPCSTR)pTmp, &tmpSize, lpDstStr, lpnDstSize);
        }
        else
            hr = E_FAIL;
        delete [] pTmp;
        }
#endif /* UNIX */

    return ( hr == S_OK ? hrWarnings : hr ) ;

fail :
    return E_FAIL ;
}

HRESULT CICharConverter::ConvertUUWI(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    int nBuffSize = 0 ;
    HRESULT hr = S_OK ;
    HRESULT hrWarnings = S_OK ;

    // UTF78ToUnicode
    if ( _dwConvertType & 0x08 )
    {
        if ( _dwConvertType == 12 ) /* convert UTF78 -> Unicode only */
            hr = UTF78ToUnicode(lpdwMode, lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize);
        else /* use interm buffer, type = 10 or 9 */
        {
            hr = UTF78ToUnicode(lpdwMode, lpSrcStr, lpnSrcSize, NULL, &nBuffSize);
            if ( _lpInterm1Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
            {
                hr = UTF78ToUnicode(lpdwMode, lpSrcStr, lpnSrcSize, _lpInterm1Str, &nBuffSize);
                lpSrcStr = _lpInterm1Str ;
                *lpnSrcSize = nBuffSize ;
            }
            else
                goto fail ;
        }
        _cvt_count ++ ;
    }

    if ( hr != S_OK )
        hrWarnings = hr ;

    // UnicodeToWindowsCodePage or UnicodeToInternetEncoding
    if ( _dwConvertType == 21 || _dwConvertType == 25 )
    {
        hr = UnicodeToInternetEncoding(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);
        _cvt_count ++ ;
    }
    else if ( _dwConvertType >= 4 && _dwConvertType <= 10 )
    {
        if ( _dwConvertType % 2 ) /* use interm buffer */
        {
            nBuffSize = 0 ;
            hr = UnicodeToWindowsCodePage(lpSrcStr, lpnSrcSize, NULL, &nBuffSize, dwFlag, lpFallBack);
            if ( _cvt_count )
            {
                if ( _lpInterm2Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
                {
                    hr = UnicodeToWindowsCodePage(lpSrcStr, lpnSrcSize, _lpInterm2Str, &nBuffSize, dwFlag, lpFallBack);
                    lpSrcStr = _lpInterm2Str ;
                    *lpnSrcSize = nBuffSize ;
                }
                else
                    goto fail ;
            }
            else
            {
                if ( _lpInterm1Str= (LPSTR)LocalAlloc(LPTR, nBuffSize) )
                {
                    hr = UnicodeToWindowsCodePage(lpSrcStr, lpnSrcSize, _lpInterm1Str, &nBuffSize, dwFlag, lpFallBack);
                    lpSrcStr = _lpInterm1Str ;
                    *lpnSrcSize = nBuffSize ;
                }
                else
                    goto fail ;
            }
        }
        else
            hr = UnicodeToWindowsCodePage(lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);
        _cvt_count ++ ;
    }

    if ( hr != S_OK )
        hrWarnings = hr ;

    // WindowsCodePageToInternetEncoding
    if ( _dwConvertType % 2 && _dwConvertType < 21 )
        hr = WindowsCodePageToInternetEncoding(lpdwMode, lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize, dwFlag, lpFallBack);

    return ( hr == S_OK ? hrWarnings : hr ) ;

fail :
    return E_FAIL ;
}

#if 0
struct CODEPAGEINFO
{
    UINT        uCodePage ;
    CP_STATE    nCP_State ;    // whether this is a valid windows codepage  ?
};

// ValidCodepageInfo is used to cache whether a codepage is a vaild code
// It uses circular-FIFO cache algorithm
#define MAX_CP_CACHE    32
static int cp_cache_count = 0 ;
static int cp_cache_ptr = 0 ;
static struct CODEPAGEINFO ValidCodepageInfo[MAX_CP_CACHE];

// ValidCodepageInfo is used to cache whether a codepage is a vaild codepage
// It uses circular-FIFO cache algorithm

BOOL CheckIsValidCodePage (UINT uCodePage)
{
    if ( uCodePage == 50000 ) // User defined
        return TRUE ;

    int i ;
    BOOL bRet ;

    for ( i = 0 ; i < cp_cache_count ; i++ )
    {
        if ( uCodePage == ValidCodepageInfo[i].uCodePage )
        {
            if ( ValidCodepageInfo[i].nCP_State == VALID_CP )
                return TRUE ;
            else
                return FALSE ;
        }
    }

    // not found, call IsValidCodePage and cache the return value
    bRet = IsValidCodePage(uCodePage);

    EnterCriticalSection(&g_cs);
    ValidCodepageInfo[cp_cache_ptr].uCodePage = uCodePage ;
    if (bRet)
        ValidCodepageInfo[cp_cache_ptr].nCP_State = VALID_CP ;
    else
        ValidCodepageInfo[cp_cache_ptr].nCP_State = INVALID_CP ;
    if ( cp_cache_count < MAX_CP_CACHE )
        cp_cache_count++ ;
    cp_cache_ptr = ( ++cp_cache_ptr ) % MAX_CP_CACHE ;
    LeaveCriticalSection(&g_cs);

    return bRet ;
}
#endif

/*
    Conversion Flag:

    Bit 7 - Convert Direction.

    Bit 4 (16) - Unicode <-> Internet Encoding
    Bit 3 (8) - UTF8, UTF7
    Bit 2 (4) - Unicode
    Bit 1 (2) - Windows CodePage
    Bit 0 (1) - Internet Encoding

    12, 6, 3 (19) - one step convert
    10, 5 (21)  - two steps convert
    9 (25) - three steps convert

*/

int GetWindowsEncodingIndex(DWORD dwEncoding)
{
    int nr = sizeof (aEncodingInfo) / sizeof(ENCODINGINFO) ;
    int i, half = nr / 2, index = -1 ;

    if (aEncodingInfo[half].dwEncoding > dwEncoding )
    {
        for ( i = 0 ; i < half ; i++ )
            if (aEncodingInfo[i].dwEncoding == dwEncoding )
                index = i ;

    }
    else if (aEncodingInfo[half].dwEncoding < dwEncoding )
    {
        for ( i = half + 1 ; i < nr ; i++ )
            if (aEncodingInfo[i].dwEncoding == dwEncoding )
                index = i ;
    }
    else
        index = half ;

    if (index>=0) // found
    {
        if ( aEncodingInfo[index].nCP_State != VALID_CP &&
                aEncodingInfo[index].dwCodePage )
        {

            if ( aEncodingInfo[index].dwCodePage == 50000 || IsValidCodePage(aEncodingInfo[index].dwCodePage ) ) // 50000 means user defined
                aEncodingInfo[index].nCP_State = VALID_CP ;
            else
                aEncodingInfo[index].nCP_State = INVALID_CP ;

            if ((aEncodingInfo[index].nCP_State == VALID_CP) &&
                (aEncodingInfo[index].dwFlags & CONV_CHK_NLS) &&
                !IsValidCodePage(aEncodingInfo[index].dwEncoding))
                aEncodingInfo[index].nCP_State = INVALID_CP ;
        }
    }

    return index ;
}

HRESULT CICharConverter::ConvertSetup(DWORD dwSrcEncoding, DWORD dwDstEncoding)
{
    DWORD SrcFlag = 0, DstFlag = 0 ;
    int index, unknown = 0 ;

    /* check source & destination encoding type */
    index = GetWindowsEncodingIndex(dwSrcEncoding);
    if ( index >=0 )
    {
        SrcFlag = (DWORD) aEncodingInfo[index].bTypeUUIW ;
        if ( aEncodingInfo[index].dwCodePage )
        {
            _dwWinCodePage = (DWORD) aEncodingInfo[index].dwCodePage ;
            if (aEncodingInfo[index].nCP_State == INVALID_CP )
                goto fail ;
        }
        if ( SrcFlag & 0x08 )
            _dwUTFEncoding = dwSrcEncoding ;
        if ( SrcFlag & 0x01 )
            _dwInternetEncoding = dwSrcEncoding ;
        if ( SrcFlag & 0x04 )
            _dwUnicodeEncoding = dwSrcEncoding ;
    }
    // assume it is a unknown Window Codepage
    else
    {
        if ( !CONVERT_IS_VALIDCODEPAGE(dwSrcEncoding))
            goto fail ;

        SrcFlag = 0x02 ;
        _dwWinCodePage = dwSrcEncoding ;

        unknown ++ ;
    }

    index = GetWindowsEncodingIndex(dwDstEncoding);
    if ( index >=0 )
    {
        // check if two codepages are compatiable
        if ( _dwWinCodePage && aEncodingInfo[index].dwCodePage )
        {
            if (_dwWinCodePage != (DWORD) aEncodingInfo[index].dwCodePage )
                goto fail ;
        }
        DstFlag = (DWORD) aEncodingInfo[index].bTypeUUIW ;
        if ( aEncodingInfo[index].dwCodePage )
        {
            _dwWinCodePage = (DWORD) aEncodingInfo[index].dwCodePage ;
            if (aEncodingInfo[index].nCP_State == INVALID_CP )
                goto fail ;
        }
        if ( DstFlag & 0x08 )
            _dwUTFEncoding = dwDstEncoding ;
        if ( DstFlag & 0x01 )
            _dwInternetEncoding = dwDstEncoding ;
        if ( DstFlag & 0x04 )
            _dwUnicodeEncoding = dwDstEncoding ;
    }
    // 1) First time unknown, assume it is a unknown Window Codepage
    //    the conversion become UTF78 <-> Unicode <-> Window Codepage
    // 2) Second time unknown, assume it is a unknown Internet Encoding
    //    the conversion become Windows Codepage <-> Unicode <-> Internet Encoding
    else
    {
        if ( !CONVERT_IS_VALIDCODEPAGE(dwDstEncoding))
            goto fail ;

        if ( unknown == 0 )
        {
            if ( _dwWinCodePage )
            {
                if (_dwWinCodePage != dwDstEncoding )
                    goto fail ;
            }

            DstFlag = 0x02 ;
            _dwWinCodePage = dwDstEncoding ;
        }
        else
        {
            DstFlag = 0x11 ;
            _dwInternetEncoding = dwDstEncoding ;
        }
    }

    if ( !SrcFlag | !DstFlag )
        goto fail ;

    if ( SrcFlag == DstFlag && dwSrcEncoding != dwDstEncoding && ( 4 != SrcFlag ))
        goto fail ;

    _dwConvertType = SrcFlag | DstFlag ;

    _bConvertDirt = ( SrcFlag & 0x0f ) > ( DstFlag & 0x0f )  ;

    // if code convertor has been allocated, deallocate it
    if (_hcins)
    {
        delete _hcins ;
        _hcins = NULL ;
    }

    return S_OK ;

fail :
    return S_FALSE ;
}


HRESULT CICharConverter::DoCodeConvert(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
    LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr = S_OK ;

    if ( 4 == _dwConvertType ) // CP_UCS_2 <-> CP_UCS_2_BE 
    {
        if (!lpDstStr)
        {   
            _nSrcSize = *lpnDstSize = *lpnSrcSize ;
        }
        else
        {
            int nSize = min(*lpnDstSize,*lpnSrcSize);

            _nSrcSize = *lpnSrcSize ;
            if ( lpDstStr && nSize > 0 )
            {
                MoveMemory(lpDstStr, lpSrcStr, nSize );
                DataByteSwap(lpDstStr, nSize );
                _nSrcSize = nSize ;
                *lpnDstSize = nSize ;
            }
        }
    }
    else if ( _bConvertDirt )
        hr = ConvertUUWI(lpdwMode, lpSrcStr,lpnSrcSize,lpDstStr,lpnDstSize, dwFlag, lpFallBack);
    else
        hr = ConvertIWUU(lpdwMode, lpSrcStr,lpnSrcSize,lpDstStr,lpnDstSize, dwFlag, lpFallBack);

    return hr ;
}

BOOL CICharConverter::ConvertCleanUp()
{
    if (_lpInterm1Str)
    {
         LocalFree(_lpInterm1Str);
         _lpInterm1Str = NULL ;
    }
    if (_lpInterm2Str)
    {
         LocalFree(_lpInterm2Str);
         _lpInterm2Str = NULL ;
    }
    if (_lpUnicodeStr)
    {
         LocalFree(_lpUnicodeStr);
         _lpUnicodeStr = NULL ;
    }
    _cvt_count = 0 ;
    _nSrcSize = 0 ;

    return TRUE ;
}

CICharConverter::CICharConverter()
{
    _lpInterm1Str = NULL ;
    _lpInterm2Str = NULL ;
    _lpUnicodeStr = NULL ;
    _hcins = NULL ;
    _cvt_count = 0 ;
    _dwWinCodePage = 0;
    _dwInternetEncoding = 0;
    _dwUTFEncoding = 0;
    _dwUnicodeEncoding = 0;
    _dwConvertType = 0;
    _nSrcSize = 0 ;
    _hcins_dst = 0 ;

    return ;
}

CICharConverter::CICharConverter(DWORD dwFlag, WCHAR *lpFallBack)
{
    _lpInterm1Str = NULL ;
    _lpInterm2Str = NULL ;
    _lpUnicodeStr = NULL ;
    _hcins = NULL ;
    _cvt_count = 0 ;
    _dwWinCodePage = 0;
    _dwInternetEncoding = 0;
    _dwUTFEncoding = 0;
    _dwUnicodeEncoding = 0;
    _dwConvertType = 0;
    _nSrcSize = 0 ;
    _hcins_dst = 0 ;
    _dwFlag = dwFlag;
    _lpFallBack = lpFallBack;

    return ;
}


CICharConverter::~CICharConverter()
{
    if (_lpInterm1Str)
    {
         LocalFree(_lpInterm1Str);
         _lpInterm1Str = NULL ;
    }
    if (_lpInterm2Str)
    {
         LocalFree(_lpInterm2Str);
         _lpInterm2Str = NULL ;
    }
    if (_lpUnicodeStr)
    {
         LocalFree(_lpUnicodeStr);
         _lpUnicodeStr = NULL ;
    }
    if (_hcins)
    {
        delete _hcins ;
        _hcins = NULL ;
    }
}

CICharConverter::CICharConverter(DWORD dwSrcEncoding, DWORD dwDstEncoding)
{
    _lpInterm1Str = NULL ;
    _lpInterm2Str = NULL ;
    _lpUnicodeStr = NULL ;
    _hcins = NULL ;
    _cvt_count = 0 ;
    _dwWinCodePage = 0;
    _dwInternetEncoding = 0;
    _dwUTFEncoding = 0;
    _dwUnicodeEncoding = 0;
    _dwConvertType = 0;
    _nSrcSize = 0 ;
    _hcins_dst = 0 ;
    
    ConvertSetup(dwSrcEncoding,dwDstEncoding);
    return ;
}

HRESULT WINAPI IsConvertINetStringAvailable(DWORD dwSrcEncoding, DWORD dwDstEncoding)
{
    HRESULT hr;
    CICharConverter * INetConvert = new CICharConverter ;

    if (!INetConvert)
        return E_OUTOFMEMORY;

    hr = INetConvert->ConvertSetup(dwSrcEncoding, dwDstEncoding);
    delete INetConvert;

    return hr ;
}

#define DETECTION_BUFFER_NUM    3

// BUGBUG (weiwu)
// In CP_AUTO and detection result is UTF7 case, private converter might use high word of *lpdwMode to store internal data, but we need 
// to use it to notify Trident the detection result, currently, we're bias to returning correct detection result.
// After IE5 release, we have to re-prototype conversion object and resovle this issue
HRESULT WINAPI ConvertINetStringEx(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    CICharConverter * INetConvert;
    int nSrcSize;
    int nDstSize;
    DWORD   dwMode = 0 ;
    // dwDetectResult 
    // CP_UNDEFINED :Fail to detect
    //      0       :Not a auto-detect scenario
    // Others       :Detected encoding
    DWORD   dwDetectResult = CP_UNDEFINED;
    HRESULT hr ;

    if(lpnSrcSize)
    {
        nSrcSize = *lpnSrcSize;
    }
    else
        nSrcSize = -1;

    if ( lpSrcStr && nSrcSize == -1 ) // Get length of lpSrcStr if not given, assuming lpSrcStr is a zero terminate string.
    {
        if ( dwSrcEncoding == CP_UCS_2 )
            nSrcSize = (lstrlenW((WCHAR*)lpSrcStr) << 1) ;
        else
            nSrcSize = lstrlenA(lpSrcStr) ;
    }

    if (!nSrcSize)
    {
        if (lpnDstSize)
           *lpnDstSize = 0;
        return S_OK;
    }

    INetConvert = new CICharConverter(dwFlag, lpFallBack) ;    

    if (!INetConvert)
        return E_OUTOFMEMORY;

    // ASSERT(CP_AUTO != dwDstEncoding);

    // if null specified at dst buffer we'll get the size of required buffer.
    if(!lpDstStr)
        nDstSize = 0;
    else if (lpnDstSize)
        nDstSize = *lpnDstSize;
    else 
        nDstSize = 0;

    if (lpdwMode)
        dwMode = *lpdwMode ;

    // In real world, clients uses 28591 as 1252, 28599 as 1254, 
    // To correctly convert those extended characters to Unicode,
    // We internally replace it with 1252 
    if (dwDstEncoding == CP_UCS_2 || dwDstEncoding == CP_UCS_2_BE)
    {
        if ((dwSrcEncoding == CP_ISO_8859_1) && _IsValidCodePage(CP_1252))
            dwSrcEncoding = CP_1252;

        if ((dwSrcEncoding == CP_ISO_8859_9) && _IsValidCodePage(CP_1254))
            dwSrcEncoding = CP_1254;
    }

    if ((dwDstEncoding == CP_1252) && (dwSrcEncoding == CP_ISO_8859_1))
    {
        dwSrcEncoding = CP_1252;
    }

    if ((dwDstEncoding == CP_1254) && (dwSrcEncoding == CP_ISO_8859_9))
    {
        dwSrcEncoding = CP_1254;
    }


    if ( dwSrcEncoding == CP_JP_AUTO ) // Auto Detection for Japan
    {
        CIncdJapanese DetectJapan;
        UINT uiCodePage ;

        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
        {
            dwSrcEncoding = uiCodePage ;
            dwDetectResult = 0;
        }
        else
        {
            dwSrcEncoding = DetectJapan.DetectStringA(lpSrcStr, *lpnSrcSize);
            // if dwSrcEncoding is zero means there is an ambiguity, we don't return
            // the detected codepage to caller, instead we defaut its codepage internally
            // to SJIS
            if (dwSrcEncoding)
            {
                dwDetectResult = dwSrcEncoding << 16 ;
            }
            else
                dwSrcEncoding = CP_JPN_SJ;
        }
    }
    // BUGBUG: bug #43190, we auto-detect again for euc-kr page because IMN ver 1.0
    // mislabel an ISO-KR page as a ks_c_5601-1987 page. This is the only way 
    // we can fix that stupid mistake. 
    else if ( dwSrcEncoding == CP_KR_AUTO || dwSrcEncoding == CP_KOR_5601 ||
        dwSrcEncoding == CP_EUC_KR )
    {
        CIncdKorean DetectKorean;
        UINT uiCodePage ;

        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
        {
            dwSrcEncoding = uiCodePage ;
            dwDetectResult = 0;
        }
        else
        {
            dwSrcEncoding = DetectKorean.DetectStringA(lpSrcStr, *lpnSrcSize);
            if (dwSrcEncoding)
            {
                dwDetectResult = dwSrcEncoding << 16 ;
            }
            else
                dwSrcEncoding = CP_KOR_5601;
        }

    }
    else if ( dwSrcEncoding == CP_AUTO ) // General Auto Detection for all code pages
    {
        nSrcSize = DETECTION_MAX_LEN < *lpnSrcSize ?  DETECTION_MAX_LEN : *lpnSrcSize;
        int nScores = DETECTION_BUFFER_NUM;
        DetectEncodingInfo Encoding[DETECTION_BUFFER_NUM];
        UINT uiCodePage ;


        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
        {
            dwSrcEncoding = uiCodePage ;
            dwDetectResult = 0;
        }
        else
        {
            dwSrcEncoding = GetACP();
            if ( S_OK == _DetectInputCodepage(MLDETECTCP_HTML, CP_AUTO, (char *)lpSrcStr, &nSrcSize, &Encoding[0], &nScores))
            {
                MIMECPINFO cpInfo;

                if (Encoding[0].nCodePage == CP_20127)
                    Encoding[0].nCodePage = dwSrcEncoding;

                if (NULL != g_pMimeDatabase)
                {
                    if (SUCCEEDED(g_pMimeDatabase->GetCodePageInfo(Encoding[0].nCodePage, 0x409, &cpInfo)) && 
                        (cpInfo.dwFlags & MIMECONTF_VALID))
                    {
                        dwSrcEncoding = Encoding[0].nCodePage;     
                        dwDetectResult = dwSrcEncoding << 16 ;  
                    }
                }
            }

            // If we failed in general detection and system locale is Jpn, we try harder 
            // with our Japanese detection engine
            if (dwSrcEncoding == CP_JPN_SJ && dwDetectResult == CP_UNDEFINED)
            {
                CIncdJapanese DetectJapan;
                DWORD dwSrcEncodingJpn = DetectJapan.DetectStringA(lpSrcStr, *lpnSrcSize);
                if (dwSrcEncodingJpn)
                {
                    // We only change conversion encoding without returnning this result to browser 
                    // if it is in the middle of detection, this is to prevent other encodings been mis-detected as Jpn encodings.
                    dwSrcEncoding = dwSrcEncodingJpn;   
                    
                    // Set search range for end tag as 10 bytes
                    if (*lpnSrcSize >= 10)
                    {
                        char szTmpStr[11] = {0};
                        char *lpTmpStr = szTmpStr;
                        MLStrCpyN(szTmpStr, (char *)&lpSrcStr[*lpnSrcSize-10], 10);                        

                        //ToLower
                        while(*lpTmpStr)
                        {
                            if (*lpTmpStr >= 'A' && *lpTmpStr <= 'W')
                                *lpTmpStr += 0x20;
                            lpTmpStr++;
                        }

                        // If end of page, return this result
                        if (MLStrStr(szTmpStr, "</html>"))
                            dwDetectResult = dwSrcEncoding << 16 ;  
                    }

                }
            }
            //aEncodingInfo[GetWindowsEncodingIndex(CP_AUTO)].dwCodePage = dwSrcEncoding;         
        }     
    }
    else
    {
        // Not a auto-detect scenario
        dwDetectResult = 0;
    }

    if ( S_OK == ( hr = INetConvert->ConvertSetup(dwSrcEncoding,dwDstEncoding )))
    {
       if ( dwSrcEncoding != dwDstEncoding )
       {
            // if high word of dwMode is CP_UTF_7, it must be detection result, don't pass it to UTF7 converter
            if ( dwSrcEncoding == CP_UTF_7 && (dwMode >> 16) == CP_UTF_7)
                dwMode &= 0xFFFF;
            // ASSERT(!((IS_ENCODED_ENCODING(dwSrcEncoding) || IS_ENCODED_ENCODING(dwDstEncoding)) && (NULL == lpdwMode)));
            hr = INetConvert->DoCodeConvert(&dwMode, lpSrcStr, &nSrcSize, lpDstStr, &nDstSize, dwFlag, lpFallBack);

            // return the number of bytes processed for the source. 
            if (lpnSrcSize)
                *lpnSrcSize = INetConvert->_nSrcSize ;
            INetConvert->ConvertCleanUp();
       }
       else
       {
            int nSize, i ;
            hr = S_OK ;
            BOOL bLeadByte = FALSE ;

            // only check for windows codepage
            if ( INetConvert->_dwConvertType == 02 && lpSrcStr )
            { 
                for ( i=0; i<nSrcSize; i++)
                {
                   if (bLeadByte)
                       bLeadByte = FALSE ;
                   else if (IsDBCSLeadByteEx(dwSrcEncoding,lpSrcStr[i]))
                       bLeadByte = TRUE ;
                }
                if (bLeadByte)
                    nSrcSize-- ;
            }
            // set input size
            if (lpnSrcSize)
                *lpnSrcSize = nSrcSize ;
            // set output size and copy if we need to
            if (lpDstStr && *lpnDstSize)
            {
                nSize = min(*lpnDstSize,nSrcSize);
                MoveMemory(lpDstStr, lpSrcStr, nSize);
                nDstSize = nSize ;
            }
            else
                nDstSize = nSrcSize ;
       }
    }
    else
            nDstSize = 0 ;

    delete INetConvert;

    // return the number of bytes copied for the destination,
    if (lpnDstSize)
        *lpnDstSize = nDstSize;

    if (lpdwMode && lpDstStr)
    {        
        if (dwDetectResult)                     // CP_AUTO conversion
        {
            dwMode &= 0xFFFF;                   // Clear HIGHWORD in case private converter set it
            // If we have detection result, return it in HIGHWORD
            // BUGBUG: in the case of UTF7 conversion, private converter might use high word to store internal data,
            // this will conflict with our logic of returning detection result in high word, it is a design flaw, 
            // currently, we ignore conversion setting and give detection result more priority
            if (dwDetectResult != CP_UNDEFINED) 
                dwMode |= dwDetectResult;
        }
        *lpdwMode = dwMode ;
    }

    return hr ;
}

HRESULT WINAPI ConvertINetReset(void)
{
    // BUGBUG: We will remove this API soon.
    return S_OK ;
}

HRESULT WINAPI ConvertINetMultiByteToUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;
    int nByteCountSize = 0;

    if (lpnWideCharCount)
    {
        nByteCountSize = *lpnWideCharCount * sizeof(WCHAR);
    }

#ifdef UNIX
   int saved_nByteCountSize = nByteCountSize;
#endif /* UNIX */

    hr = ConvertINetStringEx(lpdwMode,dwEncoding, CP_UCS_2, lpSrcStr, lpnMultiCharCount, (LPSTR)lpDstStr, &nByteCountSize, dwFlag, lpFallBack) ;

#ifdef UNIX
    if(dwEncoding == 1200 || dwEncoding == 65000 || dwEncoding == 65001 ||
       (dwEncoding == 50001 && !_IsValidCodePage(dwEncoding)) )
    {
        /*
         * On unix we need to convert the little endian mode 2 byte unicode
         * format to unix mode 4 byte wChars.
         */
        if(lpDstStr && (saved_nByteCountSize < (nByteCountSize/2)*sizeof(WCHAR)))
            hr = E_FAIL;
        else
        {
            /*
             * Use a temporary array to do the 2byte -> 4byte conversion
             */
            LPSTR pTmp = (LPSTR) lpDstStr;
            LPWSTR pw4 = NULL;

            if(pTmp) /* allocate only if we have a lpDstStr */
                pw4 = new WCHAR[nByteCountSize/2];
            if(pw4)
            {
                int i = 0;
                LPWSTR pw4Tmp = pw4;
                for(; i < nByteCountSize/2; i++)
                    *pw4Tmp++ = (UCHAR)pTmp[i*2];
                pw4Tmp = pw4;
                for(i = 0; i < nByteCountSize/2; i++)
                    *lpDstStr++ = *pw4Tmp++;
            }
            if(!pw4 && pTmp) /* if lpDstStr and allocate fails bail out */
                hr = E_FAIL;
            delete [] pw4;
        }
        nByteCountSize *= 2; // Expand twice as we have 4 byte wchars.
    }
#endif
    *lpnWideCharCount = nByteCountSize / sizeof(WCHAR);

    return hr ;
}


HRESULT WINAPI ConvertINetUnicodeToMultiByteEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr ;
    int nByteCountSize=-1;

    if(lpnWideCharCount && *lpnWideCharCount != -1) 
        nByteCountSize = *lpnWideCharCount * sizeof(WCHAR);

    hr = ConvertINetStringEx(lpdwMode,CP_UCS_2, dwEncoding, (LPCSTR) lpSrcStr, &nByteCountSize, lpDstStr, lpnMultiCharCount, dwFlag, lpFallBack);

#ifdef UNIX
    if(dwEncoding == 1200 || dwEncoding == 65000 || dwEncoding == 65001) {
        nByteCountSize *= 2; // Expand twice as we have 4 byte wchars.
    }
#endif /* UNIX */

    if (lpnWideCharCount)
        *lpnWideCharCount = nByteCountSize / sizeof(WCHAR);

    return hr ;
}

HRESULT WINAPI ConvertINetString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDstStr, LPINT lpnDstSize)
{
    HRESULT hr ;

    hr = ConvertINetStringEx(lpdwMode,dwSrcEncoding,dwDstEncoding,lpSrcStr,lpnSrcSize,lpDstStr,lpnDstSize, 0, NULL);

    return hr ;
}

HRESULT WINAPI ConvertINetUnicodeToMultiByte(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount)
{
    HRESULT hr ;
    DWORD dwFlag = 0 ;

    if ( lpdwMode )
        dwFlag |= ( *lpdwMode & 0x00008000 ) ? MLCONVCHARF_ENTITIZE : 0 ;

    hr = ConvertINetUnicodeToMultiByteEx(lpdwMode,dwEncoding,lpSrcStr,lpnWideCharCount,lpDstStr,lpnMultiCharCount,dwFlag,NULL);

    return hr ;
}

HRESULT WINAPI ConvertINetMultiByteToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount)
{
    HRESULT hr ;

    hr = ConvertINetMultiByteToUnicodeEx(lpdwMode,dwEncoding,lpSrcStr,lpnMultiCharCount,lpDstStr,lpnWideCharCount, 0, NULL);

    return hr ;
}

#define STR_BUFFER_SIZE 2048

HRESULT _ConvertINetStringInIStream(CICharConverter * INetConvert, LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut, DWORD dwFlag, WCHAR *lpFallBack)
{
    DWORD   dwMode, dwModeTemp ;
    HRESULT hr= S_OK, hrWarnings=S_OK;
    LPSTR lpstrIn = NULL, lpstrOut = NULL; 
    ULONG nSrcSize, nSrcUsed, nSrcLeft, nDstSize, _nDstSize, nOutBuffSize ;

    if (lpdwMode)
        dwMode = *lpdwMode ;

    // allocate a temp input buffer - 2K in size
    if ( (lpstrIn = (LPSTR) LocalAlloc(LPTR, STR_BUFFER_SIZE )) == NULL )
    {
        hrWarnings = E_OUTOFMEMORY ;
        goto exit;
    }

    if ( (lpstrOut = (LPSTR) LocalAlloc(LPTR, STR_BUFFER_SIZE * 2 )) == NULL )
    {
        hrWarnings = E_OUTOFMEMORY ;
        goto exit;
    }

    nOutBuffSize = STR_BUFFER_SIZE * 2 ;
    nSrcLeft = 0 ;

    // In real world, clients uses 28591 as 1252, 28599 as 1254, 
    // To correctly convert those extended characters to Unicode,
    // We internally replace it with 1252 
    if (dwDstEncoding == CP_UCS_2 || dwDstEncoding == CP_UCS_2_BE)
    {
        if ((dwSrcEncoding == CP_ISO_8859_1) && _IsValidCodePage(CP_1252))
            dwSrcEncoding = CP_1252;

        if ((dwSrcEncoding == CP_ISO_8859_9) && _IsValidCodePage(CP_1254))
            dwSrcEncoding = CP_1254;
    }

    if ((dwDstEncoding == CP_1252) && (dwSrcEncoding == CP_ISO_8859_1))
    {
        dwSrcEncoding = CP_1252;
    }

    if ((dwDstEncoding == CP_1254) && (dwSrcEncoding == CP_ISO_8859_9))
    {
        dwSrcEncoding = CP_1254;
    }


    if ( dwSrcEncoding == CP_JP_AUTO ) // Auto Detection for Japan
    {
        CIncdJapanese DetectJapan;
        UINT uiCodePage ;
        LARGE_INTEGER   li;

        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
            dwSrcEncoding = uiCodePage ;
        else
        {
            LISet32(li, 0);

            hr = pstmIn->Read(lpstrIn, STR_BUFFER_SIZE , &nSrcSize);
            if (S_OK != hr)
                hrWarnings = hr;
            hr = pstmIn->Seek(li,STREAM_SEEK_SET, NULL);
            if (S_OK != hr)
                hrWarnings = hr;

            dwSrcEncoding = DetectJapan.DetectStringA(lpstrIn, nSrcSize);
            // if dwSrcEncoding is zero means there is an ambiguity, we don't return
            // the detected codepage to caller, instead we defaut its codepage internally
            // to SJIS
            if (dwSrcEncoding)
            {
                dwMode &= 0x0000ffff ;
                dwMode |= dwSrcEncoding << 16 ; 
            }
            else
                dwSrcEncoding = CP_JPN_SJ;
        }
    }
    // BUGBUG: bug #43190, we auto-detect again for euc-kr page because IMN ver 1.0
    // mislabel an ISO-KR page as a ks_c_5601-1987 page. This is the only way 
    // we can fix that stupid mistake. 
    else if ( dwSrcEncoding == CP_KR_AUTO || dwSrcEncoding == CP_KOR_5601 ||
        dwSrcEncoding == CP_EUC_KR )
    {
        CIncdKorean DetectKorean;
        UINT uiCodePage ;
        LARGE_INTEGER   li;

        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
            dwSrcEncoding = uiCodePage ;
        else
        {
            LISet32(li, 0);
            
            hr = pstmIn->Read(lpstrIn, STR_BUFFER_SIZE, &nSrcSize);
            if (S_OK != hr)
                hrWarnings = hr;
            hr = pstmIn->Seek(li,STREAM_SEEK_SET, NULL);
            if (S_OK != hr)
                hrWarnings = hr;
            dwSrcEncoding = DetectKorean.DetectStringA(lpstrIn, nSrcSize);
            if (dwSrcEncoding)
            {
                dwMode &= 0x0000ffff ;
                dwMode |= dwSrcEncoding << 16 ; 
            }
            else
                dwSrcEncoding = CP_KOR_5601;
        }
    }
    else if ( dwSrcEncoding == CP_AUTO ) // General Auto Detection for all code pages
    {
        INT nScores = 1;
        DWORD dwSrcEncoding ;
        DetectEncodingInfo Encoding;
        UINT uiCodePage ;
        LARGE_INTEGER   li;

        uiCodePage = ( dwMode >> 16 ) & 0xffff ;
        if ( uiCodePage )
            dwSrcEncoding = uiCodePage ;
        else
        {
            LISet32(li, 0);

            hr = pstmIn->Read(lpstrIn, STR_BUFFER_SIZE , &nSrcSize);
            if (S_OK != hr)
                hrWarnings = hr;
            hr = pstmIn->Seek(li,STREAM_SEEK_SET, NULL);
            if (S_OK != hr)
                hrWarnings = hr;

            if (DETECTION_MAX_LEN < nSrcSize)
                nSrcSize =  DETECTION_MAX_LEN;

            if ( S_OK == _DetectInputCodepage(MLDETECTCP_HTML, 1252, lpstrIn, (int *)&nSrcSize, &Encoding, &nScores))
            {
                dwSrcEncoding = Encoding.nCodePage;
                dwMode &= 0x0000ffff ;
                dwMode |= dwSrcEncoding << 16 ; 
            }
            else
            {
                dwSrcEncoding = CP_ACP;
            }
            aEncodingInfo[GetWindowsEncodingIndex(CP_AUTO)].dwCodePage = dwSrcEncoding;
        }
    }

    if ( S_OK == ( hr = INetConvert->ConvertSetup(dwSrcEncoding,dwDstEncoding )))
    {
        // Loop for ever
        while(1)
        {
            // Read a buffer
            hr = pstmIn->Read(&lpstrIn[nSrcLeft], STR_BUFFER_SIZE-nSrcLeft, &nSrcSize);
            if (S_OK != hr)
                hrWarnings = hr;

            // Done
            if (0 == nSrcSize)
                break;

            nSrcSize += nSrcLeft ;
            nSrcUsed = nSrcSize ;
            dwModeTemp = dwMode ;
            nDstSize = 0 ;

            // get the size of output buffer
            hr = INetConvert->DoCodeConvert(&dwModeTemp, (LPCSTR)lpstrIn, (LPINT)&nSrcUsed, NULL, (LPINT)&nDstSize, dwFlag, lpFallBack);
            if (S_OK != hr)
                hrWarnings = hr;

            // Reallocate output buffer if so
            if ( nDstSize > nOutBuffSize )
            {
                LPSTR psz = (LPSTR) LocalReAlloc(lpstrOut, nDstSize, LMEM_ZEROINIT|LMEM_MOVEABLE);
                if (psz == NULL)
                {
                    hrWarnings = E_OUTOFMEMORY ;
                    goto exit;
                }
                lpstrOut = psz;
                nOutBuffSize = nDstSize ;
            }
            _nDstSize = nDstSize;

            // Due to multi_stage conversion, this is the actual size is used
            nSrcUsed = INetConvert->_nSrcSize ;
            nSrcLeft = nSrcSize - nSrcUsed ;

#if 0
            // restore Src size
            nSrcUsed = nSrcSize ;
#endif
            // do conversion
            hr = INetConvert->DoCodeConvert(&dwMode, (LPCSTR)lpstrIn, (LPINT)&nSrcUsed, lpstrOut, (LPINT)&_nDstSize, dwFlag, lpFallBack);
            if (S_OK != hr)
                hrWarnings = hr;

            // Write It
            hr = pstmOut->Write(lpstrOut, nDstSize, &nDstSize);
            if (S_OK != hr)
                hrWarnings = hr;

            if (nSrcLeft )
                MoveMemory(lpstrIn, &lpstrIn[nSrcSize-nSrcLeft],nSrcLeft);

            INetConvert->ConvertCleanUp();
        }
    }

    if (nSrcLeft )
    {
        LARGE_INTEGER   li;

        LISet32(li, -(LONG)nSrcLeft );
        hr = pstmIn->Seek(li,STREAM_SEEK_CUR, NULL);
    }

    if (lpdwMode)
        *lpdwMode = dwMode ;

exit :
    if (lpstrIn)
        LocalFree(lpstrIn);
    if (lpstrOut)
        LocalFree(lpstrOut);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}


HRESULT WINAPI ConvertINetStringInIStream(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut, DWORD dwFlag, WCHAR *lpFallBack)
{
    HRESULT hr;
    CICharConverter * INetConvert = new CICharConverter(dwFlag, lpFallBack) ;

    if (!INetConvert)
        return E_OUTOFMEMORY;

    hr = _ConvertINetStringInIStream(INetConvert,lpdwMode,dwSrcEncoding,dwDstEncoding,pstmIn,pstmOut,dwFlag,lpFallBack);

    delete INetConvert;

    return hr ;
}

