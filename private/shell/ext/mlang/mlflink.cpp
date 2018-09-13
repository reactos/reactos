// MLFLink.cpp : Implementation of CMLFLink
#include "private.h"
#include "mlmain.h"
#include "codepage.h"

#ifdef UNIX
inline WORD READWINTELWORD(WORD w)
{
  return ( w << 8 | w >> 8 );
}

inline DWORD READWINTELDWORD(DWORD dw)
{
  return READWINTELWORD( (WORD)(dw >> 16 )) | ((DWORD)READWINTELWORD( dw & 0xffff)) << 16;
}
#else
#define READWINTELWORD
#define READWINTELDWORD
#endif

IMLangFontLink *g_pMLFLink = NULL;

CMLFLink::CCodePagesCache* CMLFLink::m_pCodePagesCache = NULL;
CMLFLink::CFontMappingCache* CMLFLink::m_pFontMappingCache = NULL;
CMLFLink2::CFontMappingCache2* CMLFLink2::m_pFontMappingCache2 = NULL;

// Strings to identify regular font
const char szRegular[]    = "Regular";
const char szNormal[]     = "Normal";

// font table
FONTINFO *g_pfont_table = NULL;

// Unicode range table for non Windows code page code points
// Data is provided by NT international group.
URANGEFONT g_urange_table[] = {
{0x0108, 0x010B, 0},
{0x0114, 0x0115, 0},
{0x011C, 0x011D, 0},
{0x0120, 0x0121, 0},
{0x0124, 0x0125, 0},
{0x0128, 0x0129, 0},
{0x012C, 0x012D, 0},
{0x0134, 0x0135, 0},
{0x014E, 0x014F, 0},
{0x015C, 0x015D, 0},
{0x0168, 0x0169, 0},
{0x016C, 0x016D, 0},
{0x0174, 0x0177, 0},
{0x017F, 0x0191, 0},
{0x0193, 0x019F, 0},
{0x01A2, 0x01AE, 0},
{0x01B1, 0x01CD, 0},
{0x01CF, 0x01CF, 0},
{0x01D1, 0x01D1, 0},
{0x01D3, 0x01D3, 0},
{0x01D5, 0x01D5, 0},
{0x01D7, 0x01D7, 0},
{0x01D9, 0x01D9, 0},
{0x01DB, 0x01DB, 0},
{0x01DD, 0x01F5, 0},
{0x01FA, 0x0217, 0},
{0x0250, 0x0250, 0},
{0x0252, 0x0260, 0},
{0x0262, 0x02A8, 0},
{0x02B0, 0x02C5, 0},
{0x02C8, 0x02C8, 0},
{0x02CC, 0x02CC, 0},
{0x02CE, 0x02CF, 0},
{0x02D1, 0x02D7, 0},
{0x02DE, 0x02DE, 0},
{0x02E0, 0x02E9, 0},
{0x0302, 0x0302, 0},
{0x0304, 0x0308, 0},
{0x030A, 0x0322, 0},
{0x0324, 0x0345, 0},
{0x0360, 0x0361, 0},
{0x0374, 0x0375, 0},
{0x037A, 0x037A, 0},
{0x037E, 0x037E, 0},
{0x0387, 0x0387, 0},
{0x03D0, 0x03D6, 0},
{0x03DA, 0x03DA, 0},
{0x03DC, 0x03DC, 0},
{0x03DE, 0x03DE, 0},
{0x03E0, 0x03E0, 0},
{0x03E2, 0x03F3, 0},
{0x0460, 0x0486, 0},
{0x0492, 0x04C4, 0},
{0x04C7, 0x04C8, 0},
{0x04CB, 0x04CC, 0},
{0x04D0, 0x04EB, 0},
{0x04EE, 0x04F5, 0},
{0x04F8, 0x04F9, 0},
{0x0531, 0x0556, 0},
{0x0559, 0x055F, 0},
{0x0561, 0x0587, 0},
{0x0589, 0x0589, 0},
{0x0591, 0x05A1, 0},
{0x05A3, 0x05AF, 0},
{0x05C4, 0x05C4, 0},
{0x0660, 0x066D, 0},
{0x0670, 0x067D, 0},
{0x067F, 0x0685, 0},
{0x0687, 0x0697, 0},
{0x0699, 0x06AE, 0},
{0x06B0, 0x06B7, 0},
{0x06BA, 0x06BE, 0},
{0x06C0, 0x06CE, 0},
{0x06D0, 0x06ED, 0},
{0x06F0, 0x06F9, 0},
{0x0901, 0x0903, 0},
{0x0905, 0x0939, 0},
{0x093C, 0x094D, 0},
{0x0950, 0x0954, 0},
{0x0958, 0x0970, 0},
{0x0981, 0x0983, 0},
{0x0985, 0x098C, 0},
{0x098F, 0x0990, 0},
{0x0993, 0x09A8, 0},
{0x09AA, 0x09B0, 0},
{0x09B2, 0x09B2, 0},
{0x09B6, 0x09B9, 0},
{0x09BC, 0x09BC, 0},
{0x09BE, 0x09C4, 0},
{0x09C7, 0x09C8, 0},
{0x09CB, 0x09CD, 0},
{0x09D7, 0x09D7, 0},
{0x09DC, 0x09DD, 0},
{0x09DF, 0x09E3, 0},
{0x09E6, 0x09FA, 0},
{0x0A02, 0x0A02, 0},
{0x0A05, 0x0A0A, 0},
{0x0A0F, 0x0A10, 0},
{0x0A13, 0x0A28, 0},
{0x0A2A, 0x0A30, 0},
{0x0A32, 0x0A33, 0},
{0x0A35, 0x0A36, 0},
{0x0A38, 0x0A39, 0},
{0x0A3C, 0x0A3C, 0},
{0x0A3E, 0x0A42, 0},
{0x0A47, 0x0A48, 0},
{0x0A4B, 0x0A4D, 0},
{0x0A59, 0x0A5C, 0},
{0x0A5E, 0x0A5E, 0},
{0x0A66, 0x0A74, 0},
{0x0A81, 0x0A83, 0},
{0x0A85, 0x0A8B, 0},
{0x0A8D, 0x0A8D, 0},
{0x0A8F, 0x0A91, 0},
{0x0A93, 0x0AA8, 0},
{0x0AAA, 0x0AB0, 0},
{0x0AB2, 0x0AB3, 0},
{0x0AB5, 0x0AB9, 0},
{0x0ABC, 0x0AC5, 0},
{0x0AC7, 0x0AC9, 0},
{0x0ACB, 0x0ACD, 0},
{0x0AD0, 0x0AD0, 0},
{0x0AE0, 0x0AE0, 0},
{0x0AE6, 0x0AEF, 0},
{0x0B01, 0x0B03, 0},
{0x0B05, 0x0B0C, 0},
{0x0B0F, 0x0B10, 0},
{0x0B13, 0x0B28, 0},
{0x0B2A, 0x0B30, 0},
{0x0B32, 0x0B33, 0},
{0x0B36, 0x0B39, 0},
{0x0B3C, 0x0B43, 0},
{0x0B47, 0x0B48, 0},
{0x0B4B, 0x0B4D, 0},
{0x0B56, 0x0B57, 0},
{0x0B5C, 0x0B5D, 0},
{0x0B5F, 0x0B61, 0},
{0x0B66, 0x0B70, 0},
{0x0B82, 0x0B83, 0},
{0x0B85, 0x0B8A, 0},
{0x0B8E, 0x0B90, 0},
{0x0B92, 0x0B95, 0},
{0x0B99, 0x0B9A, 0},
{0x0B9C, 0x0B9C, 0},
{0x0B9E, 0x0B9F, 0},
{0x0BA3, 0x0BA4, 0},
{0x0BA8, 0x0BAA, 0},
{0x0BAE, 0x0BB5, 0},
{0x0BB7, 0x0BB9, 0},
{0x0BBE, 0x0BC2, 0},
{0x0BC6, 0x0BC8, 0},
{0x0BCA, 0x0BCD, 0},
{0x0BD7, 0x0BD7, 0},
{0x0BE7, 0x0BF2, 0},
{0x0C01, 0x0C03, 0},
{0x0C05, 0x0C0C, 0},
{0x0C0E, 0x0C10, 0},
{0x0C12, 0x0C28, 0},
{0x0C2A, 0x0C33, 0},
{0x0C35, 0x0C39, 0},
{0x0C3E, 0x0C44, 0},
{0x0C46, 0x0C48, 0},
{0x0C4A, 0x0C4D, 0},
{0x0C55, 0x0C56, 0},
{0x0C60, 0x0C61, 0},
{0x0C66, 0x0C6F, 0},
{0x0C82, 0x0C83, 0},
{0x0C85, 0x0C8C, 0},
{0x0C8E, 0x0C90, 0},
{0x0C92, 0x0CA8, 0},
{0x0CAA, 0x0CB3, 0},
{0x0CB5, 0x0CB9, 0},
{0x0CBE, 0x0CC4, 0},
{0x0CC6, 0x0CC8, 0},
{0x0CCA, 0x0CCD, 0},
{0x0CD5, 0x0CD6, 0},
{0x0CDE, 0x0CDE, 0},
{0x0CE0, 0x0CE1, 0},
{0x0CE6, 0x0CEF, 0},
{0x0D02, 0x0D03, 0},
{0x0D05, 0x0D0C, 0},
{0x0D0E, 0x0D10, 0},
{0x0D12, 0x0D28, 0},
{0x0D2A, 0x0D39, 0},
{0x0D3E, 0x0D43, 0},
{0x0D46, 0x0D48, 0},
{0x0D4A, 0x0D4D, 0},
{0x0D57, 0x0D57, 0},
{0x0D60, 0x0D61, 0},
{0x0D66, 0x0D6F, 0},
{0x0E81, 0x0E82, 0},
{0x0E84, 0x0E84, 0},
{0x0E87, 0x0E88, 0},
{0x0E8A, 0x0E8A, 0},
{0x0E8D, 0x0E8D, 0},
{0x0E94, 0x0E97, 0},
{0x0E99, 0x0E9F, 0},
{0x0EA1, 0x0EA3, 0},
{0x0EA5, 0x0EA5, 0},
{0x0EA7, 0x0EA7, 0},
{0x0EAA, 0x0EAB, 0},
{0x0EAD, 0x0EB9, 0},
{0x0EBB, 0x0EBD, 0},
{0x0EC0, 0x0EC4, 0},
{0x0EC6, 0x0EC6, 0},
{0x0EC8, 0x0ECD, 0},
{0x0ED0, 0x0ED9, 0},
{0x0EDC, 0x0EDD, 0},
{0x0F00, 0x0F47, 0},
{0x0F49, 0x0F69, 0},
{0x0F71, 0x0F8B, 0},
{0x0F90, 0x0F95, 0},
{0x0F97, 0x0F97, 0},
{0x0F99, 0x0FAD, 0},
{0x0FB1, 0x0FB7, 0},
{0x0FB9, 0x0FB9, 0},
{0x10A0, 0x10C5, 0},
{0x10D0, 0x10F6, 0},
{0x10FB, 0x10FB, 0},
{0x1100, 0x1159, 0},
{0x115F, 0x11A2, 0},
{0x11A8, 0x11F9, 0},
{0x1E00, 0x1E9B, 0},
{0x1EA0, 0x1EF9, 0},
{0x1F00, 0x1F15, 0},
{0x1F18, 0x1F1D, 0},
{0x1F20, 0x1F45, 0},
{0x1F48, 0x1F4D, 0},
{0x1F50, 0x1F57, 0},
{0x1F59, 0x1F59, 0},
{0x1F5B, 0x1F5B, 0},
{0x1F5D, 0x1F5D, 0},
{0x1F5F, 0x1F7D, 0},
{0x1F80, 0x1FB4, 0},
{0x1FB6, 0x1FC4, 0},
{0x1FC6, 0x1FD3, 0},
{0x1FD6, 0x1FDB, 0},
{0x1FDD, 0x1FEF, 0},
{0x1FF2, 0x1FF4, 0},
{0x1FF6, 0x1FFE, 0},
{0x2000, 0x200B, 0},
{0x2011, 0x2012, 0},
{0x2017, 0x2017, 0},
{0x201B, 0x201B, 0},
{0x201F, 0x201F, 0},
{0x2023, 0x2024, 0},
{0x2028, 0x202E, 0},
{0x2031, 0x2031, 0},
{0x2034, 0x2034, 0},
{0x2036, 0x2038, 0},
{0x203C, 0x2046, 0},
{0x206A, 0x2070, 0},
{0x2075, 0x207E, 0},
{0x2080, 0x2080, 0},
{0x2085, 0x208E, 0},
{0x20A0, 0x20A9, 0},
{0x20D0, 0x20E1, 0},
{0x2100, 0x2102, 0},
{0x2104, 0x2104, 0},
{0x2106, 0x2108, 0},
{0x210A, 0x2112, 0},
{0x2114, 0x2115, 0},
{0x2117, 0x2120, 0},
{0x2123, 0x2125, 0},
{0x2127, 0x212A, 0},
{0x212C, 0x2138, 0},
{0x2155, 0x215A, 0},
{0x215F, 0x215F, 0},
{0x216C, 0x216F, 0},
{0x217A, 0x2182, 0},
{0x219A, 0x21D1, 0},
{0x21D3, 0x21D3, 0},
{0x21D5, 0x21EA, 0},
{0x2201, 0x2201, 0},
{0x2204, 0x2206, 0},
{0x2209, 0x220A, 0},
{0x220C, 0x220E, 0},
{0x2210, 0x2210, 0},
{0x2212, 0x2214, 0},
{0x2216, 0x2219, 0},
{0x221B, 0x221C, 0},
{0x2221, 0x2222, 0},
{0x2224, 0x2224, 0},
{0x2226, 0x2226, 0},
{0x222D, 0x222D, 0},
{0x222F, 0x2233, 0},
{0x2238, 0x223B, 0},
{0x223E, 0x2247, 0},
{0x2249, 0x224B, 0},
{0x224D, 0x2251, 0},
{0x2253, 0x225F, 0},
{0x2262, 0x2263, 0},
{0x2268, 0x2269, 0},
{0x226C, 0x226D, 0},
{0x2270, 0x2281, 0},
{0x2284, 0x2285, 0},
{0x2288, 0x2294, 0},
{0x2296, 0x2298, 0},
{0x229A, 0x22A4, 0},
{0x22A6, 0x22BE, 0},
{0x22C0, 0x22F1, 0},
{0x2300, 0x2300, 0},
{0x2302, 0x2311, 0},
{0x2313, 0x237A, 0},
{0x2400, 0x2424, 0},
{0x2440, 0x244A, 0},
{0x24B6, 0x24CF, 0},
{0x24EA, 0x24EA, 0},
{0x254C, 0x254F, 0},
{0x2575, 0x2580, 0},
{0x2590, 0x2591, 0},
{0x25A2, 0x25A2, 0},
{0x25AA, 0x25B1, 0},
{0x25B4, 0x25B5, 0},
{0x25B8, 0x25BB, 0},
{0x25BE, 0x25BF, 0},
{0x25C2, 0x25C5, 0},
{0x25C9, 0x25CA, 0},
{0x25CC, 0x25CD, 0},
{0x25D2, 0x25E1, 0},
{0x25E6, 0x25EE, 0},
{0x2600, 0x2604, 0},
{0x2607, 0x2608, 0},
{0x260A, 0x260D, 0},
{0x2610, 0x2613, 0},
{0x261A, 0x261B, 0},
{0x261D, 0x261D, 0},
{0x261F, 0x263F, 0},
{0x2641, 0x2641, 0},
{0x2643, 0x265F, 0},
{0x2662, 0x2662, 0},
{0x2666, 0x2666, 0},
{0x266B, 0x266B, 0},
{0x266E, 0x266E, 0},
{0x2701, 0x2704, 0},
{0x2706, 0x2709, 0},
{0x270C, 0x2727, 0},
{0x2729, 0x274B, 0},
{0x274D, 0x274D, 0},
{0x274F, 0x2752, 0},
{0x2756, 0x2756, 0},
{0x2758, 0x275E, 0},
{0x2761, 0x2767, 0},
{0x2776, 0x2794, 0},
{0x2798, 0x27AF, 0},
{0x27B1, 0x27BE, 0},
{0x3004, 0x3004, 0},
{0x3018, 0x301C, 0},
{0x3020, 0x3020, 0},
{0x302A, 0x3037, 0},
{0x303F, 0x303F, 0},
{0x3094, 0x3094, 0},
{0x3099, 0x309A, 0},
{0x30F7, 0x30FA, 0},
{0x312A, 0x312C, 0},
{0x3190, 0x319F, 0},
{0x322A, 0x3230, 0},
{0x3233, 0x3238, 0},
{0x323A, 0x3243, 0},
{0x3280, 0x32A2, 0},
{0x32A9, 0x32B0, 0},
{0x32C0, 0x32CB, 0},
{0x32D0, 0x32FE, 0},
{0x3300, 0x3302, 0},
{0x3304, 0x330C, 0},
{0x330E, 0x3313, 0},
{0x3315, 0x3317, 0},
{0x3319, 0x3321, 0},
{0x3324, 0x3325, 0},
{0x3328, 0x332A, 0},
{0x332C, 0x3335, 0},
{0x3337, 0x333A, 0},
{0x333C, 0x3348, 0},
{0x334B, 0x334C, 0},
{0x334E, 0x3350, 0},
{0x3352, 0x3356, 0},
{0x3358, 0x3376, 0},
{0x337F, 0x337F, 0},
{0x3385, 0x3387, 0},
{0x33CB, 0x33CC, 0},
{0x33D4, 0x33D4, 0},
{0x33D7, 0x33D7, 0},
{0x33D9, 0x33DA, 0},
{0x33E0, 0x33FE, 0},
{0xFB00, 0xFB06, 0},
{0xFB13, 0xFB17, 0},
{0xFB1E, 0xFB36, 0},
{0xFB38, 0xFB3C, 0},
{0xFB3E, 0xFB3E, 0},
{0xFB40, 0xFB41, 0},
{0xFB43, 0xFB44, 0},
{0xFB46, 0xFBB1, 0},
{0xFBD3, 0xFD3F, 0},
{0xFD50, 0xFD8F, 0},
{0xFD92, 0xFDC7, 0},
{0xFDF0, 0xFDFB, 0},
{0xFE20, 0xFE23, 0},
{0xFE32, 0xFE32, 0},
{0xFE58, 0xFE58, 0},
{0xFE70, 0xFE72, 0},
{0xFE74, 0xFE74, 0},
{0xFE76, 0xFEFC, 0},
{0xFEFF, 0xFEFF, 0},
{0xFFA0, 0xFFBE, 0},
{0xFFC2, 0xFFC7, 0},
{0xFFCA, 0xFFCF, 0},
{0xFFD2, 0xFFD7, 0},
{0xFFDA, 0xFFDC, 0},
{0xFFE8, 0xFFEE, 0},
{0xFFFD, 0xFFFD, 0}
};

const struct {
    int         nCharSet;
    UINT        uCodePage;
    DWORD       dwCodePages;
    SCRIPT_ID   sid[3];
} g_CharSetTransTable[] = 
{
    ANSI_CHARSET,        1252, FS_LATIN1,   sidAsciiLatin,  sidLatin,   sidDefault,  
    EASTEUROPE_CHARSET,  1250, FS_LATIN2,   sidAsciiLatin,  sidLatin,   sidDefault,  
    RUSSIAN_CHARSET,     1251, FS_CYRILLIC, sidCyrillic,    sidDefault, sidDefault,
    GREEK_CHARSET,       1253, FS_GREEK,    sidGreek,       sidDefault, sidDefault,
    TURKISH_CHARSET,     1254, FS_TURKISH,  sidAsciiLatin,  sidLatin,   sidDefault,
    HEBREW_CHARSET,      1255, FS_HEBREW,   sidHebrew,      sidDefault, sidDefault,
    ARABIC_CHARSET,      1256, FS_ARABIC,   sidArabic,      sidDefault, sidDefault,
    BALTIC_CHARSET,      1257, FS_BALTIC,   sidAsciiLatin,  sidLatin,   sidDefault,
    VIETNAMESE_CHARSET,  1258, FS_VIETNAMESE, sidAsciiLatin,  sidLatin, sidDefault,
    THAI_CHARSET,         874, FS_THAI ,    sidThai,        sidDefault, sidDefault,
    SHIFTJIS_CHARSET,     932, FS_JISJAPAN, sidKana,        sidDefault, sidDefault, //sidKana,        sidHan,     sidDefault,
    GB2312_CHARSET,       936, FS_CHINESESIMP,sidHan,       sidDefault, sidDefault, //sidKana,     sidHan,     sidBopomofo,
    HANGEUL_CHARSET,      949, FS_WANSUNG,  sidHangul,      sidDefault, sidDefault, //sidHangul,   sidKana,    sidHan,
    CHINESEBIG5_CHARSET,  950, FS_CHINESETRAD, sidBopomofo, sidDefault, sidDefault, //sidKana,     sidHan,     sidBopomofo,     
    JOHAB_CHARSET,       1361, FS_JOHAB,    sidDefault,     sidDefault, sidDefault,
    DEFAULT_CHARSET,        0, 0,           sidDefault,     sidDefault, sidDefault,
};

// Primary chars for scripts
// Pre-sorted by Unicode characters to speed up CMAP search.
const struct {
    WCHAR       wch;    //Can be extended to a character list
    SCRIPT_ID   sid;
} g_wCharToScript[] =
{
        0x0531, sidArmenian,
        0x0710, sidSyriac,
        0x0780, sidThaana,
        0x0905, sidDevanagari,
        0x0985, sidBengali,
        0x0a05, sidGurmukhi,
        0x0a85, sidGujarati,
        0x0b05, sidOriya,
        0x0b85, sidTamil,
        0x0c05, sidTelugu,
        0x0c85, sidKannada,
        0x0d05, sidMalayalam,
        0x0d85, sidSinhala,
        0x0e81, sidLao,
        0x0f40, sidTibetan,
        0x10a0, sidGeorgian,
        0x1300, sidEthiopic,
        0x1401, sidCanSyllabic,
        0x13a0, sidCherokee,
        0xa000, sidYi,
        0x1680, sidOgham,
        0x16a0, sidRunic,
        0x1700, sidBurmese,
        0x1780, sidKhmer,
        0x2800, sidBraille,
//      0x0020, sidUserDefined
    };

// Script tables ported from Trident
static SCRIPT_ID s_asidUnicodeSubRangeScriptMapping[] =
{
    sidAsciiLatin, sidLatin,      sidLatin,      sidLatin,        // 128-131
    sidLatin,      sidLatin,      0,             sidGreek,        // 132-135
    sidGreek,      sidCyrillic,   sidArmenian,   sidHebrew,       // 136-139
    sidHebrew,     sidArabic,     sidArabic,     sidDevanagari,   // 140-143
    sidBengali,    sidGurmukhi,   sidGujarati,   sidOriya,        // 144-147
    sidTamil,      sidTelugu,     sidKannada,    sidMalayalam,    // 148-151
    sidThai,       sidLao,        sidGeorgian,   sidGeorgian,     // 152-155
    sidHangul,     sidLatin,      sidGreek,      0,               // 156-159
    0,             0,             0,             0,               // 160-163
    0,             0,             0,             0,               // 164-167
    0,             0,             0,             0,               // 168-171
    0,             0,             0,             0,               // 172-175
    sidHan,        sidKana,       sidKana,       sidBopomofo,     // 176-179
    sidHangul,     0,             0,             0,               // 180-183
    sidHangul,     sidHangul,     sidHangul,     sidHan,          // 184-187
    0,             sidHan,        0,             0,               // 188-191
    0,             0,             0,             0,               // 192-195
    0,             0,             sidHangul,     0,               // 196-199
};


// Script table (raw data from MichelSu)
// Rendered by script ID
const SCRIPT ScriptTable[] = 
{

    {sidDefault,    IDS_SIDDEFAULT,     0,  0,  0,  0, SCRIPTCONTF_SCRIPT_SYSTEM},      // 0
    {sidMerge,      IDS_SIDMERGE,       0,  0,  0,  0, SCRIPTCONTF_SCRIPT_SYSTEM},      // 1
    {sidAsciiSym,   IDS_SIDASCIISYM,    0,  0,  0,  0, SCRIPTCONTF_SCRIPT_SYSTEM},      // 2
    {sidAsciiLatin, IDS_SIDASCIILATIN,  1252, 0,      IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP2, SCRIPTCONTF_SCRIPT_USER},  // 3
    {sidLatin,      IDS_SIDLATIN,       1252, 0,      IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP2, SCRIPTCONTF_SCRIPT_HIDE},  // 4
    {sidGreek,      IDS_SIDGREEK,       1253, 0x03AC, IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP2, SCRIPTCONTF_SCRIPT_USER},  // 5
    {sidCyrillic,   IDS_SIDCYRILLIC,    1251, 0x0401, IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP2, SCRIPTCONTF_SCRIPT_USER},  // 6
    {sidArmenian,   IDS_SIDARMENIAN,    0,    0x0531, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 7
/**/{sidHebrew,     IDS_SIDHEBREW,      1255, 0x05D4, IDS_FONT_HEBREW_FIXED,  IDS_FONT_HEBREW_PROP, SCRIPTCONTF_SCRIPT_USER},    // 8
    {sidArabic,     IDS_SIDARABIC,      1256, 0x0627, IDS_FONT_ARABIC_FIXED,  IDS_FONT_ARABIC_PROP, SCRIPTCONTF_SCRIPT_USER},    // 9
    {sidDevanagari, IDS_SIDDEVANAGARI,  0,    0x0905, IDS_FONT_DEVANAGARI_FIXED,IDS_FONT_DEVANAGARI_PROP, SCRIPTCONTF_SCRIPT_USER}, // 10
    {sidBengali,    IDS_SIDBENGALI,     0,    0x0985, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 11
    {sidGurmukhi,   IDS_SIDGURMUKHI,    0,    0x0A05, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 12
    {sidGujarati,   IDS_SIDGUJARATI,    0,    0x0A85, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 13
    {sidOriya,      IDS_SIDORIYA,       0,    0x0B05, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 14
    {sidTamil,      IDS_SIDTAMIL,       0,    0x0B85, IDS_FONT_TAMIL_FIXED, IDS_FONT_TAMIL_PROP, SCRIPTCONTF_SCRIPT_USER},       // 15
    {sidTelugu,     IDS_SIDTELUGU,      0,    0x0C05, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 16
    {sidKannada,    IDS_SIDKANNADA,     0,    0x0C85, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 17
    {sidMalayalam,  IDS_SIDMALAYALAM,   0,    0x0D05, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 18
    {sidThai,       IDS_SIDTHAI,        874,  0x0E01, IDS_FONT_THAI_FIXED, IDS_FONT_THAI_PROP, SCRIPTCONTF_SCRIPT_USER},         // 19
    {sidLao,        IDS_SIDLAO,         0,    0x0E81, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 20
    {sidTibetan,    IDS_SIDTIBETAN,     0,    0x0F40, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 21
    {sidGeorgian,   IDS_SIDGEORGIAN,    0,    0x10A0, 0, 0, SCRIPTCONTF_SCRIPT_USER},                                            // 22
    {sidHangul,     IDS_SIDHANGUL,      949,  0,      IDS_FONT_KOREAN_FIXED,   IDS_FONT_KOREAN_PROP, SCRIPTCONTF_SCRIPT_USER},   // 23
    {sidKana,       IDS_SIDKANA,        932,  0,      IDS_FONT_JAPANESE_FIXED, IDS_FONT_JAPANESE_PROP, SCRIPTCONTF_SCRIPT_USER}, // 24
    {sidBopomofo,   IDS_SIDBOPOMOFO,    950,  0,      IDS_FONT_TAIWAN_FIXED,   IDS_FONT_TAIWAN_PROP, SCRIPTCONTF_SCRIPT_USER},   // 25
    {sidHan,        IDS_SIDHAN,         936,  0,      IDS_FONT_CHINESE_FIXED,  IDS_FONT_CHINESE_PROP, SCRIPTCONTF_SCRIPT_USER},  // 26
    {sidEthiopic,   IDS_SIDETHIOPIC,    0,    0x1300, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 27
    {sidCanSyllabic,IDS_SIDCANSYLLABIC, 0,    0x1401, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 28
    {sidCherokee,   IDS_SIDCHEROKEE,    0,    0x13A0, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 29
    {sidYi,         IDS_SIDYI,          0,    0xA000, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 30
    {sidBraille,    IDS_SIDBRAILLE,     0,    0x2800, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 31
    {sidRunic,      IDS_SIDRUNIC,       0,    0x16A0, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 32
    {sidOgham,      IDS_SIDOGHAM,       0,    0x1680, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 33
    {sidSinhala,    IDS_SIDSINHALA,     0,    0x0D85, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 34
    {sidSyriac,     IDS_SIDSYRIAC,      0,    0x0710, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 35
    {sidBurmese,    IDS_SIDBURMESE,     0,    0x1700, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 36
    {sidKhmer,      IDS_SIDKHMER,       0,    0x1780, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 37
    {sidThaana,     IDS_SIDTHAANA,      0,    0x0780, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 38
    {sidMongolian,  IDS_SIDMONGOLIAN,   0,    0,      0, 0, SCRIPTCONTF_SCRIPT_USER},      // 39
    {sidUserDefined,IDS_SIDUSERDEFINED, 0,    0x0020, 0, 0, SCRIPTCONTF_SCRIPT_USER},      // 40
};



UINT g_cScript = ARRAYSIZE(ScriptTable);

/////////////////////////////////////////////////////////////////////////////
// CMLFLink Free Global Objects

void CMLangFontLink_FreeGlobalObjects()
{
    if (g_pMLFLink)
        g_pMLFLink->Release();
    if (CMLFLink::m_pCodePagesCache)
        delete CMLFLink::m_pCodePagesCache;
    if (CMLFLink::m_pFontMappingCache)
        delete CMLFLink::m_pFontMappingCache;
    if (CMLFLink2::m_pFontMappingCache2)
        delete CMLFLink2::m_pFontMappingCache2;
}

/////////////////////////////////////////////////////////////////////////////
// CMLFLink

CMLFLink::CMLFLink()
{
    EnterCriticalSection(&g_cs);
    if (!m_pCodePagesCache)
        m_pCodePagesCache = new CCodePagesCache;
    if (!m_pFontMappingCache)
        m_pFontMappingCache = new CFontMappingCache;
    LeaveCriticalSection(&g_cs);
    m_pFlinkTable = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CMLFLink : IMLangCodePages

STDMETHODIMP CMLFLink::GetCharCodePages(WCHAR chSrc, DWORD* pdwCodePages)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(pdwCodePages);

    HRESULT hr = S_OK;
    int nLen;
    int iCmd = 0;
    int nPickOffset;
    int nBitOffset = 0;
    int nBitCount = 32;
    DWORD dwDiff = 0xffffffff;
    DWORD dwOr = 0;
    DWORD dwCodePages;
    DWORD adwBitsMap[32];
    const CCodePagesHeader* pHeader;
    const WORD* pwTable;
    int nBlock;
    int nEndLen;
    const BYTE* pbBlock;

    if (m_pCodePagesCache)
        hr = m_pCodePagesCache->Load();
    else
        hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        pHeader = (CCodePagesHeader*)(BYTE*)*m_pCodePagesCache;
        pwTable = (WORD*)(*m_pCodePagesCache + READWINTELDWORD(pHeader->m_dwTableOffset));
        nBlock = chSrc / READWINTELDWORD(pHeader->m_dwBlockSize);
        nEndLen = chSrc % READWINTELDWORD(pHeader->m_dwBlockSize);
        pbBlock = *m_pCodePagesCache + READWINTELWORD(pwTable[nBlock]);
    }

    for (int nDoneLen = 0; SUCCEEDED(hr) && nDoneLen < (int)READWINTELDWORD(pHeader->m_dwBlockSize); nDoneLen += nLen)
    {
        BYTE bCmd = pbBlock[--iCmd];
        if (bCmd < pHeader->m_abCmdCode[1])
        {
            // Flat
            nLen = bCmd + 1;
            nPickOffset = nBitOffset + nBitCount * (nEndLen - nDoneLen);
            nBitOffset += nBitCount * nLen;
        }
        else if (bCmd < pHeader->m_abCmdCode[2])
        {
            // Pack
            nLen = bCmd - pHeader->m_abCmdCode[1] + 2;
            nPickOffset = nBitOffset;
            nBitOffset += nBitCount;
        }
        else if (bCmd < pHeader->m_abCmdCode[4])
        {
            // Diff & Or
            nLen = 0;

            DWORD dw = pbBlock[--iCmd];
            dw <<= 8;
            dw |= pbBlock[--iCmd];
            dw <<= 8;
            dw |= pbBlock[--iCmd];
            dw <<= 8;
            dw |= pbBlock[--iCmd];

            if (bCmd < pHeader->m_abCmdCode[3])
            {
                // Diff
                dwDiff = dw;
                DWORD dwShift = 1;
                nBitCount = 0;
                for (int nBit = 0; nBit < 32; nBit++)
                {
                    if (dwDiff & (1 << nBit))
                    {
                        adwBitsMap[nBit] = dwShift;
                        dwShift <<= 1;
                        nBitCount++;
                    }
                    else
                    {
                        adwBitsMap[nBit] = 0;
                    }
                }
            }
            else
            {
                // Or
                dwOr = dw;
            }
        }
        else
        {
            // Big Pack
            nLen = (bCmd - pHeader->m_abCmdCode[4]) * 0x100 + pbBlock[--iCmd] + pHeader->m_abCmdCode[2] - pHeader->m_abCmdCode[1] + 1 + 1;
            nPickOffset = nBitOffset;
            nBitOffset += nBitCount;
        }

        if (nEndLen < nDoneLen + nLen)
            break;
    }

    if (SUCCEEDED(hr) &&
        nDoneLen < (int)READWINTELDWORD(pHeader->m_dwBlockSize))
    {
        const BYTE* const pbBuf = &pbBlock[nPickOffset / 8];
        DWORD dwCompBits = pbBuf[0] | (DWORD(pbBuf[1]) << 8) | (DWORD(pbBuf[2]) << 16) | (DWORD(pbBuf[3]) << 24);
        dwCompBits >>= nPickOffset % 8;
        if (nBitOffset % 8)
            dwCompBits |= pbBuf[4] << (32 - nBitOffset % 8);

        if (nBitCount < 32)
        {
            dwCompBits &= (1 << nBitCount) - 1;

            dwCodePages = 0;
            for (int nBit = 0; nBit < 32; nBit++)
            {
                if (dwCompBits & adwBitsMap[nBit])
                    dwCodePages |= (1 << nBit);
            }
        }
        else
        {
            dwCodePages = dwCompBits;
        }

        dwCodePages |= dwOr;
    }
    else
    {
        hr = E_FAIL; // Probably Code Pages data is broken.
    }

    if (pdwCodePages)
    {
        if (SUCCEEDED(hr))
        {
            // We invent this new internal charset bit, FS_MLANG_K1HANJA, to support Korean K1 Hanja
            // K1 Hanja is defined in KSC 5657-1991, it contains non-cp949 DBCS characters
            // Currenly, Korean fonts shipped with NT5 and Win98 support K1 Hanja glyphs 
            // and we don't want to switch font to other DBCS fonts in this case.
            if (dwCodePages & FS_MLANG_K1HANJA)
            {
                // Assume Korean font supports K1_HANJA on Win98 and NT5 
                if (g_bIsNT5 || (g_bIsWin98 && CP_KOR_5601 == GetACP()))
                    dwCodePages |= FS_WANSUNG;
                dwCodePages &= ~FS_MLANG_K1HANJA;
            }            
            *pdwCodePages = dwCodePages;
        }
        else
            *pdwCodePages = 0;
    }

    return hr;
}   



STDMETHODIMP CMLFLink::GetStrCodePages(const WCHAR* pszSrc, long cchSrc, DWORD dwPriorityCodePages, DWORD* pdwCodePages, long* pcchCodePages)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pdwCodePages);
    ASSERT_WRITE_PTR_OR_NULL(pcchCodePages);

    HRESULT hr = S_OK;
    long cchCodePages = 0;
    DWORD dwStrCodePages = (DWORD)~0;
    BOOL fInit = FALSE;
    BOOL fNoPri = FALSE;

    if (!pszSrc || cchSrc <= 0) // We can't make dwStrCodePages when cchSrc is zero
        hr = E_INVALIDARG;

    while (SUCCEEDED(hr) && cchSrc > 0)
    {
        DWORD dwCharCodePages;

        if (SUCCEEDED(hr = GetCharCodePages(*pszSrc, &dwCharCodePages)))
        {
            if (!fInit)
            {
                fInit = TRUE;
                fNoPri = !(dwPriorityCodePages & dwCharCodePages);
            }
            else if (fNoPri != !(dwPriorityCodePages & dwCharCodePages))
            {
                break;
            }
            if (!fNoPri)
                dwPriorityCodePages &= dwCharCodePages;

            if (dwCharCodePages && (dwCharCodePages & dwStrCodePages))
                dwStrCodePages &= dwCharCodePages;
            else if (dwCharCodePages) // Don't break if dwCharCodePages is zero
                     break;

            pszSrc++;
            cchSrc--;
            cchCodePages++;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pcchCodePages)
            *pcchCodePages = cchCodePages;
        if (pdwCodePages)
            *pdwCodePages = dwStrCodePages;
    }
    else
    {
        if (pcchCodePages)
            *pcchCodePages = 0;
        if (pdwCodePages)
            *pdwCodePages = 0;
    }

    return hr;
}

STDMETHODIMP CMLFLink::CodePageToCodePages(UINT uCodePage, DWORD* pdwCodePages)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(pdwCodePages);

    for (int iCharSet = 0; g_CharSetTransTable[iCharSet].uCodePage; iCharSet++)
    {
        if (uCodePage == g_CharSetTransTable[iCharSet].uCodePage)
        {
            if (pdwCodePages)
                *pdwCodePages = g_CharSetTransTable[iCharSet].dwCodePages;
            return S_OK;
        }
    }

    if (pdwCodePages)
        *pdwCodePages = 0;

    return E_FAIL; // Unknown code page
}

STDMETHODIMP CMLFLink::CodePagesToCodePage(DWORD dwCodePages, UINT uDefaultCodePage, UINT* puCodePage)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puCodePage);

    HRESULT hr = E_FAIL; // Unknown code pages
    DWORD dwDefaultCodePages;

    if (uDefaultCodePage &&
        SUCCEEDED(hr = CodePageToCodePages(uDefaultCodePage, &dwDefaultCodePages)) &&
        (dwDefaultCodePages & dwCodePages))
    {
        hr = S_OK;
    }
    else
    {
        for (int iCharSet = 0; g_CharSetTransTable[iCharSet].dwCodePages; iCharSet++)
        {
            if (dwCodePages & g_CharSetTransTable[iCharSet].dwCodePages)
            {
                uDefaultCodePage = g_CharSetTransTable[iCharSet].uCodePage;
                hr = S_OK;
                break;
            }
        }
    }

    if (puCodePage)
    {
        if (SUCCEEDED(hr))
            *puCodePage = uDefaultCodePage;
        else
            *puCodePage = 0;
    }

    return hr;
}


#define REGSTR_PATH_FONTLINK TSZMICROSOFTPATH TEXT("\\Windows NT\\CurrentVersion\\FontLink\\SystemLink")

void CMLFLink::FreeFlinkTable(void)
{
    if (m_pFlinkTable)
    {
        for (UINT i=0; i<m_uiFLinkFontNum; i++)
            if (m_pFlinkTable[i].pmszFaceName)
                LocalFree(m_pFlinkTable[i].pmszFaceName);
        LocalFree(m_pFlinkTable);
        m_pFlinkTable = NULL;
        m_uiFLinkFontNum = 0;
    }
}

#define MAX_FONTLINK_BUFFER_SIZE    1024

HRESULT CMLFLink::CreateNT5FontLinkTable(void)
{
    HKEY    hKey = NULL;
    HKEY    hKeyFont = NULL;
    ULONG   ulFonts = 0;
    ULONG   ulFLinkFonts = 0;
    DWORD   dwIndex = 0;
    WCHAR   szFaceName[MAX_PATH];
    LPWSTR  pNewFaceName = NULL;
    DWORD   dwOffset = 0;
    DWORD   dwOffset2 = 0;
    DWORD   dwValue;
    DWORD   dwData;
    WCHAR   szFontFile[LF_FACESIZE];
    DWORD   dwType;
    WCHAR   szFlinkFont[MAX_FONTLINK_BUFFER_SIZE];
    

    // Internal temperate data
    struct tagFontTable
    {
        WCHAR szFontFile[LF_FACESIZE];
        WCHAR szFaceName[LF_FACESIZE];
    }* tmpFontTable = NULL;

    HRESULT hr;

    // Open system font and fontlink key
    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_FONTLINK, 0, KEY_READ, &hKey)) ||
        (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGFONTKEYNT, 0, KEY_READ, &hKeyFont)))
    {
        hr = E_FAIL;
        goto TABLE_DONE;
    }


    // Get number of items
    if ((ERROR_SUCCESS != RegQueryInfoKey(hKey, NULL, NULL, 0, NULL, 
         NULL, NULL, &ulFLinkFonts, NULL, NULL, NULL, NULL) || 0 == ulFLinkFonts) ||            
        (ERROR_SUCCESS != RegQueryInfoKey(hKeyFont, NULL, NULL, 0, NULL, 
            NULL, NULL, &ulFonts, NULL, NULL, NULL, NULL) || 0 == ulFonts))
    {
        hr = E_FAIL;
        goto TABLE_DONE;
    }

    tmpFontTable = (struct tagFontTable *)LocalAlloc(LPTR, sizeof(struct tagFontTable)*ulFonts);

    if (NULL == tmpFontTable)
    {
        hr = E_OUTOFMEMORY;
        goto TABLE_DONE;
    }

    dwValue = dwData = LF_FACESIZE;
    dwType = REG_SZ;
    dwIndex = 0;
    ulFonts = 0;    

    while (ERROR_NO_MORE_ITEMS != RegEnumValueW(
                      hKeyFont,
                      dwIndex++,
                      szFaceName,
                      &dwValue,
                      NULL,
                      &dwType,
                      (LPBYTE)szFontFile,
                      &dwData ))
    {
        //  TTF fonts only, TTC fonts already have face name under fontlink
        if (pNewFaceName = wcsstr(szFaceName, L" & "))
            break;
        pNewFaceName = wcsstr(szFaceName, L" (TrueType)");
        if(pNewFaceName)
        {
           *pNewFaceName = 0;
           MLStrCpyNW(tmpFontTable[ulFonts].szFaceName, szFaceName, LF_FACESIZE);
           MLStrCpyNW(tmpFontTable[ulFonts++].szFontFile, szFontFile, LF_FACESIZE);
        }

        dwValue = dwData = LF_FACESIZE;
        dwType = REG_SZ;
    }

    
    m_pFlinkTable = (PFLINKFONT) LocalAlloc(LPTR, sizeof(FLINKFONT)*ulFLinkFonts);
    if (NULL == m_pFlinkTable)
    {
        hr = E_OUTOFMEMORY;
        goto TABLE_DONE;
    }

    dwValue = LF_FACESIZE;
    dwData = MAX_FONTLINK_BUFFER_SIZE;
    dwType = REG_MULTI_SZ;

    dwIndex = 0;

    while (ERROR_NO_MORE_ITEMS != RegEnumValueW(
                      hKey,
                      dwIndex,
                      m_pFlinkTable[dwIndex].szFaceName,
                      &dwValue,
                      NULL,
                      &dwType,
                      (LPBYTE)szFlinkFont,
                      &dwData ))
    {
        m_pFlinkTable[dwIndex].pmszFaceName = (LPWSTR) LocalAlloc(LPTR, MAX_FONTLINK_BUFFER_SIZE); 
        if (!m_pFlinkTable[dwIndex].pmszFaceName)
        {
            hr = E_OUTOFMEMORY;
            goto TABLE_DONE;
        }
        while (TRUE)
        {
            pNewFaceName = wcsstr(&szFlinkFont[dwOffset], L",");
            
            if (pNewFaceName)   // TTC font, get face name from registry
            {
                MLStrCpyNW(&(m_pFlinkTable[dwIndex].pmszFaceName[dwOffset2]), ++pNewFaceName, LF_FACESIZE);
                dwOffset2 += lstrlenW(pNewFaceName)+1;
            }
            else                // TTF font, search font table for face name            
            {
                if (szFlinkFont[dwOffset])
                    for (UINT i=0; i<ulFonts; i++)
                    {
                        if (!MLStrCmpNIW(&szFlinkFont[dwOffset], tmpFontTable[i].szFontFile, LF_FACESIZE))
                        {
                            MLStrCpyNW(&(m_pFlinkTable[dwIndex].pmszFaceName[dwOffset2]), tmpFontTable[i].szFaceName, LF_FACESIZE);
                            dwOffset2 += lstrlenW(tmpFontTable[i].szFaceName)+1;
                            break;
                        }
                    }
                else            // End of multiple string, break out                    
                    break;
            }

            dwOffset += lstrlenW(&szFlinkFont[dwOffset])+1;

            // Prevent infinitive loop, shouldn't happen
            if (dwOffset >= MAX_FONTLINK_BUFFER_SIZE) 
            {
                break;
            }
        }

        dwValue = LF_FACESIZE;
        dwData = MAX_FONTLINK_BUFFER_SIZE;
        dwType = REG_MULTI_SZ;
        dwOffset = dwOffset2 = 0;
        dwIndex++;
    }

    m_uiFLinkFontNum = ulFLinkFonts;

    hr = S_OK;

TABLE_DONE:
    if (hKey)
        RegCloseKey(hKey);
    if (hKeyFont)
        RegCloseKey(hKeyFont);
    if (tmpFontTable)
        LocalFree(tmpFontTable);
    if ((hr != S_OK) && m_pFlinkTable)
        FreeFlinkTable();

    return hr;
}


HRESULT CMLFLink::GetNT5FLinkFontCodePages(HDC hDC, LOGFONTW* plfEnum, DWORD * lpdwCodePages)
{
    HRESULT hr = S_OK;
    UINT    i;

    if (!EnumFontFamiliesExW(hDC, plfEnum, GetFontCodePagesEnumFontProcW, (LPARAM)lpdwCodePages, 0))
        return E_FAIL;

    if (NULL == m_pFlinkTable)
        CreateNT5FontLinkTable();

    if (m_pFlinkTable)
    {
        for (i=0; i<m_uiFLinkFontNum;i++)
        {
            if (!MLStrCmpNIW(plfEnum->lfFaceName, m_pFlinkTable[i].szFaceName, LF_FACESIZE))
            {
                DWORD dwOffset=0;
                // Internal buffer, we're sure it'll end
                while(TRUE)
                {
                    MLStrCpyNW(plfEnum->lfFaceName, &m_pFlinkTable[i].pmszFaceName[dwOffset], LF_FACESIZE);
                    EnumFontFamiliesExW(hDC, plfEnum, GetFontCodePagesEnumFontProcW, (LPARAM)lpdwCodePages, 0);
                    dwOffset += lstrlenW(&m_pFlinkTable[i].pmszFaceName[dwOffset])+1;
                    // End of multiple string ?
                    if (m_pFlinkTable[i].pmszFaceName[dwOffset] == 0)
                        break;                        
                }
                break;
            }
        }
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CMLFLink : IMLangFontLink
// 1/29/99 - Change HR return
//        Now, we always return S_OK unless system error, caller will
//        check code pages bits in dwCodePages for font code page coverage
STDMETHODIMP CMLFLink::GetFontCodePages(HDC hDC, HFONT hFont, DWORD* pdwCodePages)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(pdwCodePages);

    HRESULT hr = S_OK;
    LOGFONT lfFont;
    DWORD dwCodePages = 0;


    if (!::GetObject(hFont, sizeof(lfFont), &lfFont))
        hr = E_FAIL; // Invalid hFont

    if (SUCCEEDED(hr))
    {
        LOGFONT lfEnum;

        // Enumerates all character sets of given font's facename
        // Then, combines them in dwCodePages

        ::memset(&lfEnum, 0, sizeof(lfEnum));
        lfEnum.lfCharSet = DEFAULT_CHARSET;
        _tcsncpy(lfEnum.lfFaceName, lfFont.lfFaceName, ARRAYSIZE(lfEnum.lfFaceName));


        if (g_bIsNT5)
        {
            LOGFONTW lfEnumW = {0};
            lfEnumW.lfCharSet = DEFAULT_CHARSET;

            if (MultiByteToWideChar(GetACP(), 0, lfFont.lfFaceName, LF_FACESIZE, lfEnumW.lfFaceName, LF_FACESIZE))
                hr = GetNT5FLinkFontCodePages(hDC, &lfEnumW, &dwCodePages);
            else
                if (!::EnumFontFamiliesEx(hDC, &lfEnum, GetFontCodePagesEnumFontProc, (LPARAM)&dwCodePages, 0))
                    hr = E_FAIL; // Invalid hDC

        }
        else
        {
            if (!::EnumFontFamiliesEx(hDC, &lfEnum, GetFontCodePagesEnumFontProc, (LPARAM)&dwCodePages, 0))
                hr = E_FAIL; // Invalid hDC
        }

    }

//############################
//######  MingLiU HACK  ######
//## Fix the bogus font !!! ##
//############################
    if (SUCCEEDED(hr) && ::CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lfFont.lfFaceName, -1, _T("MingLiU"), -1) == 2)
    {
        dwCodePages &= ~FS_LATIN1; // Actually it doesn't have the characters of ANSI_CHARSET.
    }
//############################
#ifdef UNICODE
#define PRC_DEFAULT_GUI_FONT L"\x5b8b\x4f53"
#else
#define PRC_DEFAULT_GUI_FONT "\xcb\xce\xcc\xe5"
#endif

    // PRC Win95 DEFAULT_GUI_FONT HACK !!!
    if (SUCCEEDED(hr) && lfFont.lfCharSet == ANSI_CHARSET && ::CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lfFont.lfFaceName, -1, PRC_DEFAULT_GUI_FONT, -1) == 2)
    {
        dwCodePages &= ~FS_CHINESESIMP; // Actually it doesn't have the characters of GB2321_CHARSET.
    }

    if (pdwCodePages)
    {
        if (SUCCEEDED(hr))
            *pdwCodePages = dwCodePages;
        else
            *pdwCodePages = 0;
    }

    return hr;
}

int CALLBACK CMLFLink::GetFontCodePagesEnumFontProc(const LOGFONT* plf, const TEXTMETRIC*, DWORD FontType, LPARAM lParam)
{
    for (int iCharSet = 0; g_CharSetTransTable[iCharSet].nCharSet != DEFAULT_CHARSET; iCharSet++)
    {
        if (plf->lfCharSet == g_CharSetTransTable[iCharSet].nCharSet)
        {
            if ((FontType == TRUETYPE_FONTTYPE) || 
                (g_CharSetTransTable[iCharSet].uCodePage == GetACP()))
            {
                *((DWORD*)lParam) |= g_CharSetTransTable[iCharSet].dwCodePages;
                break;
            }
        }
    }

    return TRUE;
}

int CALLBACK CMLFLink::GetFontCodePagesEnumFontProcW(const LOGFONTW* plf, const TEXTMETRICW*, DWORD FontType, LPARAM lParam)
{
    for (int iCharSet = 0; g_CharSetTransTable[iCharSet].nCharSet != DEFAULT_CHARSET; iCharSet++)
    {
        if (plf->lfCharSet == g_CharSetTransTable[iCharSet].nCharSet)
        {
            if ((FontType == TRUETYPE_FONTTYPE) || 
                (g_CharSetTransTable[iCharSet].uCodePage == GetACP()))
            {
                *((DWORD*)lParam) |= g_CharSetTransTable[iCharSet].dwCodePages;
                break;
            }
        }
    }

    return TRUE;
}


STDMETHODIMP CMLFLink::MapFont(HDC hDC, DWORD dwCodePages, HFONT hSrcFont, HFONT* phDestFont)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(phDestFont);

    HRESULT hr = S_OK;
    CFontMappingInfo fm; // To accelerate internal subroutine calls

    fm.hDC = hDC;

    // Font mapping cache works only for Display
    BOOL fDisplay = (::GetDeviceCaps(hDC, TECHNOLOGY) == DT_RASDISPLAY);

    dwCodePages &= ~FS_SYMBOL; // We don't map symbol font.

    if (!::GetObject(hSrcFont, sizeof(fm.lfSrcFont), &fm.lfSrcFont))
        hr = E_FAIL; // Invalid hSrcFont

    if (fDisplay)
        EnumFontFamiliesEx(hDC, &fm.lfSrcFont, (FONTENUMPROC)VerifyFontSizeEnumFontProc, (LPARAM)&fm.lfSrcFont, 0);

    // Do two things at same time
    // (1) Find given font in the font mapping cache
    // (2) Build m_auCodePage[] and m_adwCodePages[]
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE; // hr == S_FALSE means that we didn't find the font in the cache
        for (int n = 0; n < 32 && dwCodePages; n++)
        {
            hr = CodePagesToCodePage(dwCodePages, 0, &fm.auCodePage[n]); // Pick one of CodePages

            if (SUCCEEDED(hr))
                hr = CodePageToCodePages(fm.auCodePage[n], &fm.adwCodePages[n]);

            if (SUCCEEDED(hr))
            {
                if (fDisplay && m_pFontMappingCache)
                    hr = m_pFontMappingCache->FindEntry(fm.auCodePage[n], fm.lfSrcFont, &fm.hDestFont);
                else
                    hr = S_FALSE;
            }

            if (hr != S_FALSE)
                break;

            dwCodePages &= ~fm.adwCodePages[n];
        }
        fm.auCodePage[n] = NULL; // End mark
        fm.adwCodePages[n] = 0;
    }

    if (hr == S_FALSE) // Not exist in cache
    {

        hr = MapFontCodePages(fm, GetFaceNameRegistry);

        if (hr == MLSTR_E_FACEMAPPINGFAILURE)
            hr = MapFontCodePages(fm, GetFaceNameGDI);

        // Handle font link failure case for NT5
        if (hr == MLSTR_E_FACEMAPPINGFAILURE && g_bIsNT5)
            hr = MapFontCodePages(fm, GetFaceNameMIME);

        if (SUCCEEDED(hr) && fDisplay && m_pFontMappingCache)
            hr = m_pFontMappingCache->AddEntry(fm.auCodePage[fm.iCP], fm.lfSrcFont, fm.hDestFont);
    }

    if (phDestFont)
    {
        if (SUCCEEDED(hr))
        {            
            *phDestFont = fm.hDestFont;
            fm.hDestFont = NULL; // Avoid being deleted it in destructor
        }
        else
        {
            *phDestFont = NULL;
        }
    }

    return hr;
}

STDMETHODIMP CMLFLink::ReleaseFont(HFONT hFont)
{
    ASSERT_THIS;

    HRESULT hr = S_OK;

    if (!m_pFontMappingCache || FAILED(hr = m_pFontMappingCache->UnlockEntry(hFont)))
    {
        // For non display DC
        if (::DeleteObject(hFont))
            hr = S_OK;
        else
            hr = E_FAIL; // Invalid hFont
    }
    return hr;
}

STDMETHODIMP CMLFLink::ResetFontMapping(void)
{
    ASSERT_THIS;
    HRESULT hr = S_OK;

    if (m_pFontMappingCache)
        hr =  m_pFontMappingCache->FlushEntries();

    return hr;
}

STDMETHODIMP CMLFLink2::ResetFontMapping(void)
{
    ASSERT_THIS;
    HRESULT hr = S_OK;

    if (m_pIMLFLnk)
        hr =  m_pIMLFLnk->ResetFontMapping();

    if (m_pFontMappingCache2)
        hr = (S_OK == m_pFontMappingCache2->EnsureFontTable(FALSE)? hr : E_FAIL);

    return hr;
}

HRESULT CMLFLink::MapFontCodePages(CFontMappingInfo& fm, PFNGETFACENAME pfnGetFaceName)
{
    HRESULT hr = MLSTR_E_FACEMAPPINGFAILURE;    

    for (fm.iCP = 0; fm.auCodePage[fm.iCP]; fm.iCP++)
    {
        fm.lfDestFont.lfCharSet = DEFAULT_CHARSET;

        hr = (this->*pfnGetFaceName)(fm);

        if (SUCCEEDED(hr))
        {
            LOGFONT lf = {0};

            // If face name is from registry or MIMEDB, we set charset to codepage charset.
            if (fm.lfDestFont.lfCharSet == DEFAULT_CHARSET)
            {
                for (int iCharSet = 0; g_CharSetTransTable[iCharSet].uCodePage; iCharSet++)
                {
                    if (fm.auCodePage[fm.iCP] == g_CharSetTransTable[iCharSet].uCodePage)
                    {
                        fm.lfDestFont.lfCharSet = (BYTE)g_CharSetTransTable[iCharSet].nCharSet;
                        break;
                    }
                }
            }

            lf.lfCharSet = DEFAULT_CHARSET;
            MLStrCpyN(lf.lfFaceName, fm.szFaceName, LF_FACESIZE);

            // Retrieve LOGFONT from gotten facename
            fm.lfDestFont.lfFaceName[0] = _T('\0');

            if (!::EnumFontFamiliesEx(fm.hDC, &lf, MapFontEnumFontProc, (LPARAM)&fm.lfDestFont, 0))
                hr = E_FAIL; // Invalid hDC
            else if (fm.lfDestFont.lfFaceName[0] == _T('\0'))
                hr = MLSTR_E_FACEMAPPINGFAILURE;
        }

        if (SUCCEEDED(hr))
        {
            fm.lfDestFont.lfHeight      = fm.lfSrcFont.lfHeight;
            fm.lfDestFont.lfWidth       = fm.lfSrcFont.lfWidth;
            fm.lfDestFont.lfEscapement  = fm.lfSrcFont.lfEscapement;
            fm.lfDestFont.lfOrientation = fm.lfSrcFont.lfOrientation;
            fm.lfDestFont.lfWeight      = fm.lfSrcFont.lfWeight;
            fm.lfDestFont.lfItalic      = fm.lfSrcFont.lfItalic;
            fm.lfDestFont.lfUnderline   = fm.lfSrcFont.lfUnderline;
            fm.lfDestFont.lfStrikeOut   = fm.lfSrcFont.lfStrikeOut;


            HRESULT hrTemp = VerifyFaceMap(fm);
            if (hrTemp == MLSTR_E_FACEMAPPINGFAILURE && fm.lfDestFont.lfWidth)
            {
                fm.lfDestFont.lfWidth = 0; // To recover non-scalable font
                hr = VerifyFaceMap(fm);
            }
            else
            {
                hr = hrTemp;
            }
        }

        if (hr != MLSTR_E_FACEMAPPINGFAILURE)
            break;
    }

    return hr;
}
    
int CALLBACK CMLFLink::MapFontEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC*, DWORD, LPARAM lParam)
{
    LOGFONT* plfDestFont = (LOGFONT*)lParam;
    
    if (!plfDestFont->lfFaceName[0] )
    {
        if (plfDestFont->lfCharSet != DEFAULT_CHARSET)
        {
            if (plfDestFont->lfCharSet == plfFont->lfCharSet)
                *plfDestFont = *plfFont;
        }
        else
              *plfDestFont = *plfFont;
    }

    return TRUE;
}

HRESULT CMLFLink::GetFaceNameRegistry(CFontMappingInfo& fm)
{
    static const TCHAR szRootKey[]       = _T("Software\\Microsoft\\Internet Explorer");
    static const TCHAR szIntlKey[]       = _T("International\\%d");
    static const TCHAR szPropFontName[]  = _T("IEPropFontName");
    static const TCHAR szFixedFontName[] = _T("IEFixedFontName");

    HRESULT hr = S_OK;
    HKEY hKeyRoot;

    if (::RegOpenKeyEx(HKEY_CURRENT_USER, szRootKey, 0, KEY_READ, &hKeyRoot) == ERROR_SUCCESS)
    {
        TCHAR szCodePageKey[ARRAYSIZE(szIntlKey) + 10];
        HKEY hKeySub;

        ::wsprintf(szCodePageKey, szIntlKey, fm.auCodePage[fm.iCP]);
        if (::RegOpenKeyEx(hKeyRoot, szCodePageKey, 0, KEY_READ, &hKeySub) == ERROR_SUCCESS)
        {
            const TCHAR* pszFontNameValue;
            DWORD dwType;
            DWORD dwSize = sizeof(fm.szFaceName);

            if ((fm.lfSrcFont.lfPitchAndFamily & 0x03) == FIXED_PITCH)
                pszFontNameValue = szFixedFontName;
            else
                pszFontNameValue = szPropFontName;

            if (::RegQueryValueEx(hKeySub, pszFontNameValue, 0, &dwType, (LPBYTE)fm.szFaceName, &dwSize) != ERROR_SUCCESS)
                hr = MLSTR_E_FACEMAPPINGFAILURE;

            if (::RegCloseKey(hKeySub) != ERROR_SUCCESS && SUCCEEDED(hr))
                hr = MLSTR_E_FACEMAPPINGFAILURE;
        }
        else
        {
            hr = MLSTR_E_FACEMAPPINGFAILURE;
        }

        if (::RegCloseKey(hKeyRoot) != ERROR_SUCCESS && SUCCEEDED(hr))
            hr = MLSTR_E_FACEMAPPINGFAILURE;
    }
    else
    {
        hr = MLSTR_E_FACEMAPPINGFAILURE;
    }
    return hr;
}

HRESULT CMLFLink::GetFaceNameGDI(CFontMappingInfo& fm)
{
    HRESULT hr = S_OK;

    for (int iCharSet = 0; g_CharSetTransTable[iCharSet].uCodePage; iCharSet++)
    {
        if (fm.auCodePage[fm.iCP] == g_CharSetTransTable[iCharSet].uCodePage)
            break;
    }

    if (g_CharSetTransTable[iCharSet].uCodePage)
    {
        ::memset(&fm.lfDestFont, 0, sizeof(fm.lfDestFont));

        // Specify font weight as NORMAL to avoid NT GDI font mapping bugs
        fm.lfDestFont.lfWeight = FW_NORMAL;
        fm.lfDestFont.lfCharSet = (BYTE)g_CharSetTransTable[iCharSet].nCharSet;
        hr = GetFaceNameRealizeFont(fm);
    }
    else
    {
        hr = E_FAIL; // Unknown code page
    }

    if (SUCCEEDED(hr))
    {
        // Height, CharSet, Pitch and Family
        fm.lfDestFont.lfHeight = fm.lfSrcFont.lfHeight;
        fm.lfDestFont.lfPitchAndFamily = fm.lfSrcFont.lfPitchAndFamily;
        hr = GetFaceNameRealizeFont(fm);

        if (FAILED(hr))
        {
            // CharSet, Pitch and Family
            fm.lfDestFont.lfHeight = 0;
            hr = GetFaceNameRealizeFont(fm);
        }

        if (FAILED(hr))
        {
            // CharSet and Pitch
            fm.lfDestFont.lfPitchAndFamily &= 0x03; // Pitch Mask
            hr = GetFaceNameRealizeFont(fm);
        }

        if (FAILED(hr))
        {
            // CharSet only
            fm.lfDestFont.lfPitchAndFamily = 0;
            hr = GetFaceNameRealizeFont(fm);
        }
    }

    return hr;
}

extern void BuildGlobalObjects(void);

HRESULT CMLFLink::GetFaceNameMIME(CFontMappingInfo& fm)
{
    HRESULT hr = E_FAIL;
    MIMECPINFO cpInfo;    

    if (fm.auCodePage[fm.iCP] == 936)
    {
        MLStrCpyN(fm.szFaceName, TEXT("SimSun"), LF_FACESIZE);
        return S_OK;
    }

    if (!g_pMimeDatabase)
        BuildGlobalObjects();


    if (NULL != g_pMimeDatabase)
    {
        if (SUCCEEDED(g_pMimeDatabase->GetCodePageInfo(fm.auCodePage[fm.iCP], 0x409, &cpInfo)))
        {
            TCHAR szFontFaceName[LF_FACESIZE];
            szFontFaceName[0] = 0;

            if ((fm.lfSrcFont.lfPitchAndFamily & 0x03) == FIXED_PITCH && cpInfo.wszFixedWidthFont[0])
            {
#ifdef UNICODE
                MLStrCpyNW(szFontFaceName, cpInfo.wszFixedWidthFont, LF_FACESIZE);
#else
                WideCharToMultiByte(CP_ACP, 0, cpInfo.wszFixedWidthFont, -1, szFontFaceName, LF_FACESIZE, NULL, NULL);
#endif
            }
            else
                if (cpInfo.wszProportionalFont[0])
                {
#ifdef UNICODE
                    MLStrCpyNW(szFontFaceName, cpInfo.wszProportionalFont, LF_FACESIZE);
#else
                    WideCharToMultiByte(CP_ACP, 0, cpInfo.wszProportionalFont, -1, szFontFaceName, LF_FACESIZE, NULL, NULL);
#endif
                }

            if (szFontFaceName[0])
            {
                MLStrCpyN(fm.szFaceName, szFontFaceName, LF_FACESIZE);
                hr = S_OK;
            }
        }
        else
            hr = MLSTR_E_FACEMAPPINGFAILURE;
    }

    return hr;
}

HRESULT CMLFLink::GetFaceNameRealizeFont(CFontMappingInfo& fm)
{
    HRESULT hr = S_OK;
    HFONT hFont = NULL;
    HFONT hOldFont;
    DWORD dwFontCodePages;

    // First let's get a facename based on the given lfDestFont
    // Then verify if the font of the found facename has the code pages we want.

    hFont = ::CreateFontIndirect(&fm.lfDestFont);
    if (!hFont)
        hr = E_FAIL; // Out of memory or GDI resource

    if (SUCCEEDED(hr))
    {
        hOldFont = (HFONT)::SelectObject(fm.hDC, hFont);
        if (!hOldFont)
            hr = E_FAIL; // Out of memory or GDI resource
    }

    if (SUCCEEDED(hr))
    {
        if (!::GetTextFace(fm.hDC, ARRAYSIZE(fm.szFaceName), fm.szFaceName))
            hr = E_FAIL; // Out of memory or GDI resource

        if (!::SelectObject(fm.hDC, hOldFont) && SUCCEEDED(hr))
            hr = E_FAIL; // Out of memory or GDI resource
    }

    if (hFont)
        ::DeleteObject(hFont);

    if (SUCCEEDED(hr))
    {
        LOGFONT lfTemp;

        lfTemp = fm.lfDestFont;
        _tcsncpy(lfTemp.lfFaceName, fm.szFaceName, ARRAYSIZE(lfTemp.lfFaceName));

        hFont = ::CreateFontIndirect(&lfTemp);
        if (!hFont)
            hr = E_FAIL; // Out of memory or GDI resource

        if (SUCCEEDED(hr = GetFontCodePages(fm.hDC, hFont, &dwFontCodePages)) && !(dwFontCodePages & fm.adwCodePages[fm.iCP]))
                hr = MLSTR_E_FACEMAPPINGFAILURE;

        if (hFont)
            ::DeleteObject(hFont);
    }

    return hr;
}

HRESULT CMLFLink::VerifyFaceMap(CFontMappingInfo& fm)
{
    HRESULT hr = S_OK;
    HFONT hOldFont;

    if (fm.hDestFont)
        ::DeleteObject(fm.hDestFont);

    fm.hDestFont = ::CreateFontIndirect(&fm.lfDestFont);
    if (!fm.hDestFont)
        hr = E_FAIL; // Out of memory or GDI resource

    if (SUCCEEDED(hr))
    {
        hOldFont = (HFONT)::SelectObject(fm.hDC, fm.hDestFont);
        if (!hOldFont)
            hr = E_FAIL; // Out of memory or GDI resource
    }

    if (SUCCEEDED(hr))
    {
        TCHAR szFaceName[LF_FACESIZE];

        if (!::GetTextFace(fm.hDC, ARRAYSIZE(szFaceName), szFaceName))
            hr = E_FAIL; // Out of memory or GDI resource

        if (SUCCEEDED(hr))
        {
            int nRet = ::CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, fm.lfDestFont.lfFaceName, -1, szFaceName, -1);
            if (!nRet)
                hr = E_FAIL; // Unexpected error
            else if (nRet != 2) // Not Equal
                hr = MLSTR_E_FACEMAPPINGFAILURE;
        }

        if (!::SelectObject(fm.hDC, hOldFont) && SUCCEEDED(hr))
            hr = E_FAIL; // Out of memory or GDI resource
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CMLFLink::CFontMappingCache

CMLFLink::CFontMappingCache::CFontMappingCache(void) :
    m_pEntries(NULL),
    m_pFree(NULL),
    m_cEntries(0)
{
    ::InitializeCriticalSection(&m_cs);
}

CMLFLink::CFontMappingCache::~CFontMappingCache(void)
{
    FlushEntries();
    DeleteCriticalSection(&m_cs);
}

HRESULT CMLFLink::CFontMappingCache::FindEntry(UINT uCodePage, const LOGFONT& lfSrcFont, HFONT* phDestFont)
{
    HRESULT hr = S_FALSE;

    ::EnterCriticalSection(&m_cs);

    if (m_pEntries)
    {
        CFontMappingCacheEntry* pEntry = m_pEntries;

        while ((pEntry = pEntry->m_pPrev) != m_pEntries)
        {
            if (uCodePage == pEntry->m_uSrcCodePage &&
                lfSrcFont.lfPitchAndFamily == pEntry->m_bSrcPitchAndFamily &&
                lfSrcFont.lfHeight == pEntry->m_lSrcHeight &&
                lfSrcFont.lfWidth == pEntry->m_lSrcWidth &&
                lfSrcFont.lfEscapement == pEntry->m_lSrcEscapement &&
                lfSrcFont.lfOrientation == pEntry->m_lSrcOrientation &&
                lfSrcFont.lfWeight == pEntry->m_lSrcWeight &&
                lfSrcFont.lfItalic == pEntry->m_bSrcItalic &&
                lfSrcFont.lfUnderline == pEntry->m_bSrcUnderline &&
                lfSrcFont.lfStrikeOut == pEntry->m_bSrcStrikeOut)
            {
                int nRet = ::CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lfSrcFont.lfFaceName, -1, pEntry->m_szSrcFaceName, -1);
                if (!nRet)
                {
                    hr = E_FAIL; // Unexpected error
                    break;
                }
                else if (nRet == 2) // Equal
                {
                    if (phDestFont)
                        *phDestFont = pEntry->m_hDestFont;
                    pEntry->m_nLockCount++;
                    hr = S_OK;
                    break;
                }
            }
        }
    }

    ::LeaveCriticalSection(&m_cs);

    if (phDestFont && hr != S_OK)
        *phDestFont = NULL;

    return hr;
}

HRESULT CMLFLink::CFontMappingCache::UnlockEntry(HFONT hDestFont)
{
    HRESULT hr = E_FAIL; // hDestFont is not found in the cache

    ::EnterCriticalSection(&m_cs);

    if (m_pEntries)
    {
        CFontMappingCacheEntry* pEntry = m_pEntries;

        while ((pEntry = pEntry->m_pPrev) != m_pEntries)
        {
            if (hDestFont == pEntry->m_hDestFont)
            {
                if (pEntry->m_nLockCount - 1 >= 0)
                {
                    pEntry->m_nLockCount--;
                    hr = S_OK;
                }
                break;
            }
        }
    }

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

HRESULT CMLFLink::CFontMappingCache::AddEntry(UINT uCodePage, const LOGFONT& lfSrcFont, HFONT hDestFont)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection(&m_cs);

    if (!m_pEntries) // Need to allocate all the entries
    {
        CFontMappingCacheEntry* pEntries;

        pEntries = new CFontMappingCacheEntry[NUMFONTMAPENTRIES + 1]; // +1 for sentinel

        if (pEntries)
        {
            // Init sentinel
            pEntries[0].m_pPrev = &pEntries[0];
            pEntries[0].m_pNext = &pEntries[0];

            // Init free entries
            for (int n = 0; n < NUMFONTMAPENTRIES; n++)
            {
                const nEnt = n + 1; // + 1 for sentinel

                if (n < NUMFONTMAPENTRIES - 1)
                    pEntries[nEnt].m_pNext = &pEntries[nEnt + 1];
                else
                    pEntries[nEnt].m_pNext = NULL;
            }

            m_pEntries = &pEntries[0];
            m_pFree = &pEntries[1];
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && !m_pFree) // Need to delete oldest entry
    {
        CFontMappingCacheEntry* pOldestEntry = m_pEntries->m_pPrev;

        while (pOldestEntry->m_nLockCount > 0 && pOldestEntry != m_pEntries) // Entry is locked
            pOldestEntry = pOldestEntry->m_pPrev;

        if (pOldestEntry != m_pEntries)
        {
            if (pOldestEntry->m_hDestFont)
                ::DeleteObject(pOldestEntry->m_hDestFont);

            // Delete it from m_pEntries list
            pOldestEntry->m_pPrev->m_pNext = pOldestEntry->m_pNext;
            pOldestEntry->m_pNext->m_pPrev = pOldestEntry->m_pPrev;

            // Insert it into m_pFree list
            pOldestEntry->m_pNext = m_pFree;
            m_pFree = pOldestEntry;
        }
        else // No entry available
        {
            hr = E_FAIL; // Out of cache entries
        }
    }

    if (SUCCEEDED(hr)) // Create new entry and fill it
    {
        CFontMappingCacheEntry* pNewEntry;

        // Delete it from m_pFree list
        pNewEntry = m_pFree; // shouldn't be NULL
        m_pFree = pNewEntry->m_pNext;

        // Insert it into m_pEntries list
        pNewEntry->m_pNext = m_pEntries->m_pNext;
        pNewEntry->m_pPrev = m_pEntries;
        m_pEntries->m_pNext->m_pPrev = pNewEntry;
        m_pEntries->m_pNext = pNewEntry;

        // Fill it
        pNewEntry->m_nLockCount = 1;
        pNewEntry->m_uSrcCodePage = uCodePage;
        pNewEntry->m_lSrcHeight = lfSrcFont.lfHeight;
        pNewEntry->m_lSrcWidth = lfSrcFont.lfWidth;
        pNewEntry->m_lSrcEscapement = lfSrcFont.lfEscapement;
        pNewEntry->m_lSrcOrientation = lfSrcFont.lfOrientation;
        pNewEntry->m_lSrcWeight = lfSrcFont.lfWeight;
        pNewEntry->m_bSrcItalic = lfSrcFont.lfItalic;
        pNewEntry->m_bSrcUnderline = lfSrcFont.lfUnderline;
        pNewEntry->m_bSrcStrikeOut = lfSrcFont.lfStrikeOut;
        pNewEntry->m_bSrcPitchAndFamily = lfSrcFont.lfPitchAndFamily;
        _tcsncpy(pNewEntry->m_szSrcFaceName, lfSrcFont.lfFaceName, ARRAYSIZE(pNewEntry->m_szSrcFaceName));
        pNewEntry->m_hDestFont = hDestFont;
    }

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

HRESULT CMLFLink::CFontMappingCache::FlushEntries(void)
{
    ::EnterCriticalSection(&m_cs);

    if (m_pEntries)
    {
        CFontMappingCacheEntry* pEntry = m_pEntries;

        while ((pEntry = pEntry->m_pPrev) != m_pEntries)
        {
            if (pEntry->m_hDestFont)
                ::DeleteObject(pEntry->m_hDestFont);
        }

        delete[] m_pEntries;

        m_pEntries = NULL;
        m_cEntries = 0;
    }

    ::LeaveCriticalSection(&m_cs);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMLFLink::CCodePagesCache

CMLFLink::CCodePagesCache::CCodePagesCache(void) :
    m_pbBuf(NULL)
{
    ::InitializeCriticalSection(&m_cs);
}

CMLFLink::CCodePagesCache::~CCodePagesCache(void)
{
    DeleteCriticalSection(&m_cs);
}

HRESULT CMLFLink::CCodePagesCache::RealLoad(void)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection(&m_cs);

    if (!m_pbBuf)
    {
        HRSRC hrCodePages;
        HGLOBAL hgCodePages;

        if (SUCCEEDED(hr))
        {
            hrCodePages = ::FindResource(g_hInst, MAKEINTRESOURCE(IDR_CODEPAGES), _T("CODEPAGES"));
            if (!hrCodePages)
                hr = E_FAIL; // Build error?
        }
        if (SUCCEEDED(hr))
        {
            hgCodePages = ::LoadResource(g_hInst, hrCodePages);
            if (!hgCodePages)
                hr = E_FAIL; // Unexpected error
        }
        if (SUCCEEDED(hr))
        {
            m_pbBuf = (BYTE*)::LockResource(hgCodePages);
            if (!m_pbBuf)
                hr = E_FAIL; // Unexpected error
        }
    }

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

extern "C" HRESULT GetGlobalFontLinkObject(IMLangFontLink **ppMLFontLink)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != ppMLFontLink)
    {
        if (NULL == g_pMLFLink)
        {
            EnterCriticalSection(&g_cs);
            if (NULL == g_pMLFLink)
                CComCreator< CComPolyObject< CMLFLink > >::CreateInstance(NULL, IID_IMLangFontLink, (void **)&g_pMLFLink);
            LeaveCriticalSection(&g_cs);
        }
        *ppMLFontLink = g_pMLFLink;
        if (g_pMLFLink)
        {
            g_pMLFLink->AddRef();
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

HRESULT CMLFLink2::CFontMappingCache2::MapFontFromCMAP(HDC hDC, WCHAR wchar, HFONT hSrcFont, HFONT *phDestFont)
{
    BOOL    bFont = FALSE;
    HRESULT hr = E_FAIL;
    int     i,j,k;
    LOGFONT LogFont;

    if (!phDestFont)
        return E_INVALIDARG;

    if (!GetObject(hSrcFont, sizeof(LOGFONT), &LogFont))
        return hr;

    if (!g_pfont_table || !g_pfont_table[0].szFaceName[0])
    {        
        if (FAILED(LoadFontDataFile()))
        {
            return hr;
        }
    }

    i=0;
    j=ARRAYSIZE(g_urange_table);
    k = j/2;

    while (i<=j)
    {

        if (wchar >= g_urange_table[k].wcFrom && wchar <= g_urange_table[k].wcTo)
           break;
        else
           if (wchar < g_urange_table[k].wcFrom)
           {
               j = k -1;
           }
           else
           {
               i = k + 1;
           }
           k = (i+j)/2;
    }

    if (i<=j && g_urange_table[k].nFonts)
    {
        TCHAR szFaceName[LF_FACESIZE];
        GetTextFace(hDC, LF_FACESIZE, szFaceName);

        // Check if it supports the character
        for (i=0; i<g_urange_table[k].nFonts; i++)
        {
            if (!MLStrCmpI(szFaceName,g_pfont_table[*(g_urange_table[k].pFontIndex+i)].lf.lfFaceName))
                break;
        }

        // Current font doesn't support this character
        if (i == g_urange_table[k].nFonts)
        {
            for (i=0; i<g_urange_table[k].nFonts; i++)
            {
                if (LogFont.lfCharSet == g_pfont_table[*(g_urange_table[k].pFontIndex+i)].lf.lfCharSet)
                    break;
            }

            // No font available for current CharSet, then return the first one in the list            
            if (i >= g_urange_table[k].nFonts)
            {
                i = fetchCharSet((BYTE *) &(LogFont.lfCharSet), k);
            }

            MLStrCpyN(LogFont.lfFaceName, g_pfont_table[*(g_urange_table[k].pFontIndex+i)].lf.lfFaceName, LF_FACESIZE);
        }

        if (i < g_urange_table[k].nFonts)
        {
            MLStrCpyN(LogFont.lfFaceName, g_pfont_table[*(g_urange_table[k].pFontIndex+i)].lf.lfFaceName, LF_FACESIZE);
        }
        
        bFont = TRUE;
    } 

    if (bFont && (*phDestFont = CreateFontIndirect(&LogFont)))
    {
        hr = S_OK;       
    }
    else
    {
        *phDestFont = NULL;
    }


    return hr;
}



HRESULT CMLFLink2::CFontMappingCache2::UnicodeRanges(
    LPTSTR  szFont,
    UINT    *puiRanges, 
    UNICODERANGE* pURanges
    )

{
    HRESULT hr = E_FAIL;
    UINT    nURange = 0;
    DWORD   cmap    = 0;
    DWORD   name    = 0;
    HANDLE  hTTF;    
    TCHAR   szFontPath[MAX_PATH];
    static TCHAR s_szFontDir[MAX_PATH] = {0};


    HANDLE  hTTFMap;
    DWORD   dwFileSize;
    LPVOID  lpvFile;
    LPBYTE  lp, lp1, lp2;
    DWORD   Num;
    WORD    i, j, Len;

    if (!s_szFontDir[0])
    {
        MLGetWindowsDirectory(s_szFontDir, MAX_PATH);
        MLPathCombine(s_szFontDir, s_szFontDir, FONT_FOLDER);
    }
    MLPathCombine(szFontPath, s_szFontDir, szFont);

    hTTF = CreateFile(  szFontPath,             // pointer to name of the file
                        GENERIC_READ,           // access (read-write) mode
                        FILE_SHARE_READ,        // share mode
                        NULL,                   // pointer to security attributes
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL);                  // handle to file with attributes to copy;

    if (INVALID_HANDLE_VALUE == hTTF)
        return hr;

    dwFileSize = GetFileSize(hTTF, NULL);

    hTTFMap = CreateFileMapping(
                  hTTF,
                  NULL,
                  PAGE_READONLY,
                  0,
                  dwFileSize,
                  NULL
              );

    if(hTTFMap == NULL)
    {
        goto CloseHandle0;
    }

    lpvFile = MapViewOfFile(
                  hTTFMap,
                  FILE_MAP_READ,
                  0,
                  0,
                  0
              );

    if(lpvFile == NULL)
    {
        goto CloseHandle;
    }

    lp = (LPBYTE)lpvFile;

    // Font table name uses ASCII
    if(strncmp(((TTC_HEAD*)lp)->TTCTag, "ttcf", 4) == 0)   // TTC format
    {
        lp += FOUR_BYTE_NUM(((TTC_HEAD*)lp)->OffsetTTF1);  // points to first TTF
    }

    Num = TWO_BYTE_NUM(((TTF_HEAD*)lp)->NumTables);        // Number of Tables
    lp += sizeof(TTF_HEAD);

    for(i = 0; i < Num; i++)   // go thru all tables to find cmap and name
    {
        if(strncmp( ((TABLE_DIR*)lp)->Tag, "cmap", 4) == 0)
        {
            cmap = FOUR_BYTE_NUM(((TABLE_DIR*)lp)->Offset);
            if (name) break;
        }
        else if(strncmp( ((TABLE_DIR*)lp)->Tag, "name", 4) == 0)
        {
            name = FOUR_BYTE_NUM(((TABLE_DIR*)lp)->Offset);
            if (cmap) break;
        }
        lp += sizeof(TABLE_DIR);
     }

    if((!cmap) || (!name))    // Can't find cmap or name
    {
        goto CloseHandle;
    }

    // Read thru all name records
    // to see if font subfamily name is "Regular"

    lp  = (LPBYTE)lpvFile + name;                   // point to name table
    Num = TWO_BYTE_NUM(((NAME_TABLE*)lp)->NumRec);  // # of name record
    lp1 = lp  + sizeof(NAME_TABLE);                 // point to name record

    for(i = 0; i < Num; i++)
    {
        if(FONT_SUBFAMILY_NAME == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->NameID))
        {
            lp2 = lp +                              // point to string store
                  TWO_BYTE_NUM(((NAME_TABLE* )lp )->Offset) +
                  TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Offset);

            Len = TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Length);

            if(((MICROSOFT_PLATFORM == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Platform)) && 
                (UNICODE_INDEXING == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Encoding)))  ||
               ((APPLE_UNICODE_PLATFORM == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Platform)) && 
                (APPLE_UNICODE_INDEXING == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Encoding))))  
            {
                Len >>= 1;
                const char *pStr = szRegular;

                if (Len == sizeof(szNormal) -1)
                    pStr = szNormal;
                else 
                    if (Len != sizeof(szRegular)-1)
                    {
                        lp1 += sizeof(NAME_RECORD);
                        continue;
                    }

                while(--Len > 0)
                {
                    if(*(lp2+(Len<<1)+1) != pStr[Len])
                    break;
                }

                if (!Len)
                    break;
                else
                {
                    lp1 += sizeof(NAME_RECORD);
                    continue;
                }
            }
            else
            {
                if(strncmp((char*)lp2, szRegular, sizeof(szRegular)-1) != 0 && 
                   strncmp((char*)lp2, szNormal, sizeof(szNormal)-1) != 0)
                {
                    lp1 += sizeof(NAME_RECORD);
                    continue;
                }
                else
                    break;
            }
        }
        lp1 += sizeof(NAME_RECORD);
    }

    // If no regular font, exit
    if (i == Num)
        goto CloseHandle;

    // all non-regular fonts have already been eliminated
    lp1  = (LPBYTE)lpvFile + cmap;                     // point to cmap table
    Num  = TWO_BYTE_NUM(((CMAP_HEAD*)lp1)->NumTables);
    lp1 += sizeof(CMAP_HEAD);

    while(Num >0)
    {

        if(TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Platform) == MICROSOFT_PLATFORM && 
           (TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Encoding) == UNICODE_INDEXING ||
           TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Encoding) == UNICODE_SYMBOL_INDEXING))
        {
            lp = (LPBYTE)lpvFile
                 + cmap
                 + FOUR_BYTE_NUM(((CMAP_TABLE*)lp1)->Offset);

            if(TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->Format) == CMAP_FORMAT_FOUR)
            {
                break;
            }
        }
        Num--;
        lp1 += sizeof(CMAP_TABLE);
    }

    if(Num == 0)                            // can't find Platform:3/Encoding:1 (Unicode)
        goto CloseHandle;

    Num  = TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->SegCountX2);
    lp2  = lp  + sizeof(CMAP_FORMAT);       // lp2 -> first WCHAR of wcTo
    lp1  = lp2 + Num + 2;                   // lp1 -> first WCHAR of wcFrom

    Num /= 2;

    if (pURanges == NULL)
    {
        *puiRanges = Num;
    }
    else
    {
        if (Num > *puiRanges)
            Num = *puiRanges;
        else
            *puiRanges = Num;

        for(i=0, j=0; i < Num; i++, j++, j++)
        {
            pURanges[i].wcFrom = TWO_BYTE_NUM((lp1+j));
            pURanges[i].wcTo   = TWO_BYTE_NUM((lp2+j));
        }
    }

    hr = S_OK;

CloseHandle:
    UnmapViewOfFile(lpvFile);
CloseHandle0:
    CloseHandle(hTTFMap);
    CloseHandle(hTTF);

    return hr;
}

int CMLFLink2::CFontMappingCache2::fetchCharSet(BYTE *pCharset, int iURange)
{
    int i,j;

    //Check if current charset valid for the font
    for (i=0; i<g_urange_table[iURange].nFonts; i++)
    {
        for (j=0;(j<32) && g_CharSetTransTable[j].uCodePage;j++)
        {
            if (g_pfont_table[*(g_urange_table[iURange].pFontIndex+i)].dwCodePages[0] & g_CharSetTransTable[j].dwCodePages)
                if (*pCharset == g_CharSetTransTable[j].nCharSet)
                    return i;
        }
    }

    //If invalid, fetch first valid one.
    for (i=0;(i<32) && g_CharSetTransTable[i].uCodePage;i++)
    {
        if (g_pfont_table[*(g_urange_table[iURange].pFontIndex)].dwCodePages[0] & g_CharSetTransTable[i].dwCodePages)
        {
           *pCharset = (BYTE)g_CharSetTransTable[i].nCharSet;
           break;
        }
    }

    return 0;
}


BOOL CMLFLink2::CFontMappingCache2::GetNonCpFontUnicodeRanges(TCHAR *szFontName, int iFontIndex)
{
    LONG    nURange = 0;
    DWORD   cmap    = 0;
    DWORD   name    = 0;
    DWORD   os2     = 0;

    HANDLE  hTTFMap;
    DWORD   dwFileSize;
    LPVOID  lpvFile;
    LPBYTE  lp, lp1, lp2;
    DWORD   Num;
    int     i, j, k, m;
    WORD    Len;
    HANDLE  hTTF;
    BOOL    bRet = FALSE;


    hTTF = CreateFile(  szFontName,             // pointer to name of the file
                        GENERIC_READ,           // access (read-write) mode
                        FILE_SHARE_READ,        // share mode
                        NULL,                   // pointer to security attributes
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL);                  // handle to file with attributes to copy;

    if (hTTF == INVALID_HANDLE_VALUE)
        return FALSE;

    dwFileSize = GetFileSize(hTTF, NULL);

    hTTFMap = CreateFileMapping(
                  hTTF,
                  NULL,
                  PAGE_READONLY,
                  0,
                  dwFileSize,
                  NULL
              );

    if(hTTFMap == NULL)
    {
        goto CloseHandle01;
    }

    lpvFile = MapViewOfFile(
                  hTTFMap,
                  FILE_MAP_READ,
                  0,
                  0,
                  0
              );

    if(lpvFile == NULL)
    {
        goto CloseHandle00;
    }

    lp = (LPBYTE)lpvFile;

    if(strncmp(((TTC_HEAD*)lp)->TTCTag, "ttcf", 4) == 0)   // TTC format
    {
        lp += FOUR_BYTE_NUM(((TTC_HEAD*)lp)->OffsetTTF1);  // points to first TTF
    }

    Num = TWO_BYTE_NUM(((TTF_HEAD*)lp)->NumTables);        // Number of Tables
    {
      // if SearchRange != (Maximum power of 2 <= Num)*16,
      // then this is not a TTF file
      DWORD  wTmp = 1;

      while(wTmp <= Num)
      {
        wTmp <<= 1;
      }
      wTmp <<= 3;          // (wTmp/2)*16

      if(wTmp != (DWORD)TWO_BYTE_NUM(((TTF_HEAD*)lp)->SearchRange))
      {
        goto CloseHandle00;
      }

      // if RangeShift != (Num*16) - SearchRange,
      // then this is not a TTF file
      wTmp = (Num<<4) - wTmp;
      if(wTmp != (DWORD)TWO_BYTE_NUM(((TTF_HEAD*)lp)->RangeShift))
      {
        goto CloseHandle00;
      }
    }

    lp += sizeof(TTF_HEAD);

    for(i = 0; i < (int)Num; i++)   // go thru all tables to find cmap and name
    {
        if(strncmp( ((TABLE_DIR*)lp)->Tag, "cmap", 4) == 0)
        {
            cmap = FOUR_BYTE_NUM(((TABLE_DIR*)lp)->Offset);
            if (name && os2) break;
        }
        else if(strncmp( ((TABLE_DIR*)lp)->Tag, "name", 4) == 0)
        {
            name = FOUR_BYTE_NUM(((TABLE_DIR*)lp)->Offset);
            if (cmap && os2) break;
        }
        else if(strncmp( ((TABLE_DIR*)lp)->Tag, "OS/2", 4) == 0)
        {
            os2 = FOUR_BYTE_NUM(((TABLE_DIR*)lp)->Offset);
            if (cmap && name) break;
        }

        lp += sizeof(TABLE_DIR);
     }

    if((!cmap) || (!name) || (!os2))    // Can't find cmap or name
    {
        goto CloseHandle00;
    }

    // Read thru all name records
    // to see if font subfamily name is "Regular"

    lp  = (LPBYTE)lpvFile + name;                   // point to name table
    Num = TWO_BYTE_NUM(((NAME_TABLE*)lp)->NumRec);  // # of name record
    lp1 = lp  + sizeof(NAME_TABLE);                 // point to name record

    for(i = 0; i < (int)Num; i++)
    {
        if(FONT_SUBFAMILY_NAME == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->NameID))
        {
            lp2 = lp +                              // point to string store
                  TWO_BYTE_NUM(((NAME_TABLE* )lp )->Offset) +
                  TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Offset);

            Len = TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Length);

            if(UNICODE_INDEXING == TWO_BYTE_NUM(((NAME_RECORD*)lp1)->Encoding))
            {
                Len >>= 1;
                while(--Len > 0)
                {
                    if(*(lp2+(Len<<1)+1) != szRegular[Len])
                    goto CloseHandle00;
                }
                break;
            }
            else
            {
                if(strncmp((char*)lp2, szRegular, Len) != 0)
                    goto CloseHandle00;
                else
                    break;
            }
        } 

        lp1 += sizeof(NAME_RECORD);
    }

    // all non-regular fonts have already been eliminated

    lp1  = (LPBYTE)lpvFile + cmap;                     // point to cmap table
    Num  = TWO_BYTE_NUM(((CMAP_HEAD*)lp)->NumTables);
    lp1 += sizeof(CMAP_HEAD);

    while(Num >0)
    {
        if(TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Platform) == MICROSOFT_PLATFORM && 
           TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Encoding) == UNICODE_INDEXING)
        {
            lp = (LPBYTE)lpvFile
                 + cmap
                 + FOUR_BYTE_NUM(((CMAP_TABLE*)lp1)->Offset);

            if(TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->Format) == CMAP_FORMAT_FOUR)
            {
                break;
            }
        }
        Num--;
        lp1 += sizeof(CMAP_TABLE);
    }

    if(Num == 0)                   // can't find Platform:3/Encoding:1 (Unicode)
        goto CloseHandle00;


    Num  = TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->SegCountX2) ;

    m = ARRAYSIZE(g_urange_table);

    lp2  = lp  + sizeof(CMAP_FORMAT);     // lp2 -> first WCHAR of wcTo
    lp1  = lp2 + Num + 2;                 // lp1 -> first WCHAR of wcFrom


    // Fast parse !!!
    while (--m)
    {
        // URANGE binary search
        i=0;
        j= (int) Num - 2;
        k=j/2;
        while (i<=j)
        {
            if (k % 2) 
                k++;

            if (g_urange_table[m].wcFrom >= TWO_BYTE_NUM((lp1+k)) && g_urange_table[m].wcTo <= TWO_BYTE_NUM((lp2+k)))
            {
                EnterCriticalSection(&g_cs);
                if (!g_urange_table[m].pFontIndex)
                    g_urange_table[m].pFontIndex = (int *)LocalAlloc(LPTR, sizeof(int)* MAX_FONT_INDEX);
                if (!g_urange_table[m].pFontIndex)
                {
                    goto CloseHandle00;
                }

                if (g_urange_table[m].nFonts >= MAX_FONT_INDEX)
                {
                    break;
                }

                g_urange_table[m].pFontIndex[g_urange_table[m].nFonts] = iFontIndex;
                g_urange_table[m].nFonts++;

                // Fill in font code page signature
                g_pfont_table[iFontIndex].dwCodePages[0] = FOUR_BYTE_NUM(((BYTE *)lpvFile+os2+OFFSET_OS2CPRANGE));
                g_pfont_table[iFontIndex].dwCodePages[1] = FOUR_BYTE_NUM(((BYTE *)lpvFile+os2+OFFSET_OS2CPRANGE+1));                
                LeaveCriticalSection(&g_cs);
                break;
            }
            else
            {
                if (g_urange_table[m].wcFrom < TWO_BYTE_NUM((lp1+k)))
                {
                    j = k-2;
                }
                else
                {
                    i = k+2;
                }
                k = (i+j)/2;
            }
        }
    }
    
    bRet = TRUE;

CloseHandle00:
    UnmapViewOfFile(lpvFile);
CloseHandle01:
    CloseHandle(hTTF);
    CloseHandle(hTTFMap);

    return bRet;
}

HRESULT GetRegFontKey(HKEY *phKey, DWORD *pdwValues)
{
    HRESULT hr = E_FAIL;

    if (ERROR_SUCCESS == (g_bIsNT? 
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGFONTKEYNT, 0, KEY_READ, phKey):
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGFONTKEY95, 0, KEY_READ, phKey)))
    {
        if (ERROR_SUCCESS == RegQueryInfoKey(*phKey, NULL, NULL, 0, NULL, 
            NULL, NULL, pdwValues, NULL, NULL, NULL, NULL))
        {
            hr = S_OK;
        }        
    }

    return hr;
}

BOOL CMLFLink2::CFontMappingCache2::GetFontURangeBits(TCHAR *szFontFile, DWORD * pdwURange)
{
    // We can make use of font Unicode range signature if needed.
    return TRUE;    
}

BOOL CMLFLink2::CFontMappingCache2::SetFontScripts(void)
{

    LOGFONT lf;
    int     i,j;
    HWND    hWnd = GetTopWindow(GetDesktopWindow());
    HDC     hDC = GetDC(hWnd);

    if (!g_pfont_table)
        return FALSE;

    // Process code page based scripts (g_CharSetTransTable.sid)
    for (i = 0; g_CharSetTransTable[i].nCharSet != DEFAULT_CHARSET; i++)
    {
        j = 0;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfCharSet = (BYTE)g_CharSetTransTable[i].nCharSet;

        while (g_CharSetTransTable[i].sid[j] != sidDefault)
        {
            EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)SetFontScriptsEnumFontProc, (LPARAM)g_CharSetTransTable[i].sid[j], 0);
            j++;
        }
    }

    if (hDC)
        ReleaseDC(hWnd, hDC);

    // Process Unicode subrange based scripts (not implemented)
    // Skip this part since we need to access font CMAP anyway

    // Process char based scripts (g_wCharToScript)

    for (i=1; i<= (int)g_pfont_table[0].dwCodePages[0]; i++)
    {
        UINT uiRanges = 0;
        UNICODERANGE* pURanges = NULL;
        SCRIPT_IDS  scripts;

        if (SUCCEEDED(m_pFontMappingCache2->UnicodeRanges(g_pfont_table[i].szFileName, &uiRanges, pURanges)))
        {
            if (uiRanges)
            {
                int         l, m, n;                

                pURanges = (UNICODERANGE *)LocalAlloc(LPTR, sizeof(UNICODERANGE) * uiRanges);

                if (!pURanges)
                    return E_OUTOFMEMORY;

                m_pFontMappingCache2->UnicodeRanges(g_pfont_table[i].szFileName, &uiRanges, pURanges);
                for (j=0; j< ARRAYSIZE(g_wCharToScript); j++)
                {

                    l = 0;
                    m = uiRanges;
                    n = m/2;
                    while (l <= m)
                    {
                        if ((g_wCharToScript[j].wch >= pURanges[n].wcFrom) && (g_wCharToScript[j].wch <= pURanges[n].wcTo))
                        {
                            scripts = 1;
                            scripts <<= g_wCharToScript[j].sid;
                            g_pfont_table[i].scripts |= scripts;
                            break;
                        }
                        else
                        {
                            if (g_wCharToScript[j].wch < pURanges[n].wcFrom)
                                m = n-1;
                            else
                                l = n+1;
                            n = (m+l)/2;
                        }
                    }
                }

                LocalFree(pURanges);
                pURanges = NULL;
            }
        }

        // sidUserDefined should contain all valid regular TrueType fonts
        if (!MLStrStr(g_pfont_table[i].szFaceName, TEXT("Bold")) && !MLStrStr(g_pfont_table[i].szFaceName, TEXT("Italic")))
        {
            scripts = 1;
            scripts <<= sidUserDefined;
            g_pfont_table[i].scripts |= scripts;
        }
    }

    //GetFontScriptFromCMAP(szFont, &(g_pfont_table[i].scripts));

    return TRUE;
}

BOOL CMLFLink2::CFontMappingCache2::IsFontUpdated(void)
{
    HKEY    hkey;
    DWORD   dwFonts = 0;
    BOOL    bRet = FALSE;


    if (g_pfont_table)
    {
        if (S_OK == GetRegFontKey(&hkey, &dwFonts))
        {
            if (g_pfont_table[0].dwCodePages[1] != dwFonts)
                bRet = TRUE;
            RegCloseKey(hkey);
        }
    }
    else
    {
        // font table not created yet, need to update
        bRet = TRUE;
    }

    return bRet;
}
    
// Make sure we have font data table available and it is updated
HRESULT CMLFLink2::CFontMappingCache2::EnsureFontTable(BOOL bUpdateURangeTable)    
{
    if (IsFontUpdated())
    {
        if (g_pfont_table)
        {
            if (g_pfont_table[0].szFaceName[0])
            {
                bUpdateURangeTable = TRUE;
            }
            LocalFree(g_pfont_table);
            g_pfont_table = NULL;
        }

        if (!SetFontTable())
            return E_OUTOFMEMORY;

        if (bUpdateURangeTable)
        {
            for (int i = 0; i < ARRAYSIZE(g_urange_table); i++)
            {
                if (g_urange_table[i].nFonts)
                {
                    LocalFree(g_urange_table[i].pFontIndex);
                    g_urange_table[i].pFontIndex = NULL;
                    g_urange_table[i].nFonts = 0;
                }            
            }

            if (S_OK != SetFontUnicodeRanges())
                return E_OUTOFMEMORY;

            SaveFontDataFile();
        }
    }

    // All tables created successfully
    return S_OK;
}


#ifdef UNIX
typedef struct tagTable_info{
    int count;
    int table_size;
    } Table_info;

int UnixGetAllFontsProc(ENUMLOGFONTEX* plfFont, NEWTEXTMETRICEX* lpntm, int iFontType, LPARAM lParam)
{
    LOGFONT *lplf;
    int *pcount = &((Table_info*)lParam)->count;
    int *ptable_size = &((Table_info*)lParam)->table_size;

    lplf = &(plfFont->elfLogFont);
    // We don't use non TrueType fonts
    if (iFontType == DEVICE_FONTTYPE || iFontType == RASTER_FONTTYPE)
        return 1;   // keep going but don't use this font

    // We don't use the SYMBOL, Mac Charset fonts
    if(lplf->lfCharSet == SYMBOL_CHARSET || lplf->lfCharSet == MAC_CHARSET)
        return 1;

    // We don't handle vertical fonts
    if (TEXT('@') == lplf->lfFaceName[0])
        return 1;

    // Now update the font-table
    // Does UNIX use TTF? // if (FontType == TRUETYPE_FONTTYPE)
    {
        CopyMemory(&g_pfont_table[*pcount].lf, lplf, sizeof(LOGFONT));
        MLStrCpyN(g_pfont_table[*pcount].szFaceName, lplf->lfFaceName, LF_FACESIZE);
        (*pcount)++;
    }

    if (*pcount >= *ptable_size)
    {
        FONTINFO * pfont_table = NULL;

        *ptable_size += FONT_TABLE_INIT_SIZE;
        pfont_table = (FONTINFO *) LocalReAlloc(g_pfont_table, 
                                        sizeof(FONTINFO) * *ptable_size,
                                        LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (NULL == pfont_table)
        {
            return 0; // Stop enum. 
        }
        else
        {
            g_pfont_table = pfont_table;
        }
    }
 
    return 1;       // Keep enum. 
}
#endif

BOOL CMLFLink2::CFontMappingCache2::SetFontTable(void)
{
    BOOL    bRet = TRUE;
    TCHAR   szFaceName[MAX_PATH];

    DWORD   dwValue;
    TCHAR   szFontFile[MAX_FONT_FILE_NAME];
    DWORD   dwData;
    DWORD   dwType = REG_SZ;
    DWORD   dwFonts;
    int     i, table_size = FONT_TABLE_INIT_SIZE;
    LPTSTR  pNewFaceName = NULL;
    HKEY    hkey = NULL;
    static int count;

    HDC     hDC = NULL;
    HWND    hWnd = NULL;
    
    if (!g_pfont_table)
    {
        EnterCriticalSection(&g_cs);
        g_pfont_table = (FONTINFO *)LocalAlloc(LPTR, sizeof(FONTINFO) * FONT_TABLE_INIT_SIZE);
        LeaveCriticalSection(&g_cs);
        if (!g_pfont_table)
        {
            bRet = FALSE;
            goto SETFONT_DONE;
        }        
    }
    else
    {
        return TRUE;
    }
    
#ifndef UNIX
    if (S_OK != GetRegFontKey(&hkey, &dwFonts))
    {
        return FALSE;
    }    

    count = 1;
    hWnd = GetTopWindow(GetDesktopWindow());
    hDC = GetDC(hWnd);

    for (i=0; ;i++)
    {
        dwValue = sizeof(szFaceName);
        dwData  = sizeof(szFontFile);

        if (ERROR_NO_MORE_ITEMS == RegEnumValue(
                      hkey,
                      i,
                      szFaceName,
                      &dwValue,
                      NULL,
                      &dwType,
                      (LPBYTE)szFontFile,
                      &dwData ))
        {
            break;
        }
        DWORD dwOffset = 0;
FIND_NEXT_FACENAME:
        
        pNewFaceName = MLStrStr(&szFaceName[dwOffset], TEXT(" & "));
        if (pNewFaceName)
        {
           *pNewFaceName = 0;
           // Skip " & ", look for next font face name
           pNewFaceName+=3;
        }
        else
        {
            pNewFaceName = MLStrStr(&szFaceName[dwOffset], TEXT("(TrueType)"));
            if(pNewFaceName)
            {
                // Ignor the space between face name and "(TrueTye)" signature
                if ((pNewFaceName > szFaceName) && (*(pNewFaceName-1) == 0x20))
                    pNewFaceName--;
                *pNewFaceName = 0;
            }
        }


        EnterCriticalSection(&g_cs);
        if (pNewFaceName && !EnumFontFamilies(hDC, &szFaceName[dwOffset], MapFontExEnumFontProc, (LPARAM)&count))   //TrueType font
        {
            if (count >= table_size)
            {
                FONTINFO * _pfont_table = NULL;

                table_size += FONT_TABLE_INIT_SIZE;
                _pfont_table = (FONTINFO *) LocalReAlloc(g_pfont_table, sizeof(FONTINFO) * table_size,
                                    LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (NULL == _pfont_table)
                {
                    bRet = FALSE;
                    goto SETFONT_DONE;
                }
                else
                {
                    g_pfont_table = _pfont_table;
                }
            }

            GetFontURangeBits(szFontFile, &(g_pfont_table[count-1].dwUniSubRanges[0]));
            MLStrCpyN(g_pfont_table[count-1].szFaceName, &szFaceName[dwOffset], LF_FACESIZE);
            MLStrCpyN(g_pfont_table[count-1].szFileName, szFontFile, LF_FACESIZE);

        }
        LeaveCriticalSection(&g_cs);
        if (pNewFaceName && (*pNewFaceName))
        {
            dwOffset = (DWORD)(pNewFaceName - &szFaceName[0]);
            goto FIND_NEXT_FACENAME;
        }
    }
#else
    // For UNIX, we don't have registry font information,
    // Let's create font table through EnumFontFamiliesEx.
    Table_info table_info;
    table_info.count = 1;
    table_info.table_size = table_size;

    int iRet;
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET; // give me all fonts
    lf.lfFaceName[0] = _T('\0');
    lf.lfPitchAndFamily = 0;
        
    hWnd = GetTopWindow(GetDesktopWindow());
    hDC = GetDC(hWnd);

    EnterCriticalSection(&g_cs);
    iRet = EnumFontFamiliesEx(hDC, // Enum all fonts
                     &lf,
                     (FONTENUMPROC)UnixGetAllFontsProc,
                     (LPARAM)&table_info,
                     0);
    LeaveCriticalSection(&g_cs);
    count = table_info.count;
    if (iRet == 0) // abort
    {
        bRet = FALSE;
        goto SETFONT_DONE;    
    }
#endif // UNIX

    // Release un-used memory
    g_pfont_table = (FONTINFO *)LocalReAlloc(g_pfont_table, (count)*sizeof(FONTINFO), LMEM_MOVEABLE);

    // Save TrueType font number
    g_pfont_table[0].dwCodePages[0] = count-1;

#ifndef UNIX
    // Unix doesn't have this number.
    // Save total font number for font change verification 
    g_pfont_table[0].dwCodePages[1] = dwFonts;

    RegCloseKey(hkey);
#endif

    if (count > 1)
        SetFontScripts();

SETFONT_DONE:    

    if (hDC)
        ReleaseDC(hWnd, hDC);

    if (count <= 1)
    {
        if (g_pfont_table)
            LocalFree(g_pfont_table);
        bRet = FALSE;
    }

    return bRet;

}


HRESULT CMLFLink2::CFontMappingCache2::SaveFontDataFile(void)
{
    FONTDATAHEADER fileHeader;
    HRESULT hr = E_FAIL;
    int     *pTmpBuf = NULL;
    HANDLE  hFile = NULL;
    int     i, j, Count = 0;
    DWORD   dwSize;
    FONTDATATABLE fontInfoTable, fontIndexTable;


    hFile = CreateFile( szFontDataFilePath,         
                        GENERIC_WRITE,          
                        0,                      
                        NULL,                   
                        CREATE_ALWAYS,          
                        FILE_ATTRIBUTE_HIDDEN,
                        NULL);                 

    if (hFile == INVALID_HANDLE_VALUE)
    {
        goto SAVE_FONT_DATA_DONE;
    }


    for (i = 0; i < ARRAYSIZE(g_urange_table); i++)
    {
        Count += (g_urange_table[i].nFonts+1);
    }


    // Create file header
    lstrcpyA(fileHeader.FileSig, FONT_DATA_SIGNATURE);
    fileHeader.dwVersion = 0x00010000;

    // Use file size as CheckSum
    fileHeader.dwCheckSum = sizeof(FONTINFO)*(g_pfont_table[0].dwCodePages[0]+1)+Count*sizeof(int)+
         + sizeof(FONTDATAHEADER) + sizeof(FONTDATATABLE)*FONTDATATABLENUM;
    fileHeader.nTable = FONTDATATABLENUM;


    pTmpBuf = (int *)LocalAlloc(LPTR, Count*sizeof(int));

    // Get font index data
    for (i = 0; i < ARRAYSIZE(g_urange_table); i++)
    {
        *pTmpBuf++ = g_urange_table[i].nFonts;

        if (g_urange_table[i].nFonts)
        {
            for (j = 0; j< g_urange_table[i].nFonts; j++)
            {
                *pTmpBuf++ = *(g_urange_table[i].pFontIndex+j);
            }
        }
    }

    pTmpBuf -= Count;

    // Create Dir tables
    lstrcpyA(fontInfoTable.szName, "fnt");
    fontInfoTable.dwOffset = sizeof(FONTDATAHEADER) + sizeof(FONTDATATABLE)*FONTDATATABLENUM;
    fontInfoTable.dwSize = sizeof(FONTINFO)*(g_pfont_table[0].dwCodePages[0]+1);

    lstrcpyA(fontIndexTable.szName, "idx");
    fontIndexTable.dwOffset = fontInfoTable.dwSize+fontInfoTable.dwOffset;
    fontIndexTable.dwSize = Count*sizeof(int);

    if (WriteFile(hFile, &fileHeader, sizeof(FONTDATAHEADER), &dwSize, NULL) &&
        WriteFile(hFile, &fontInfoTable, sizeof(FONTDATATABLE), &dwSize, NULL) &&
        WriteFile(hFile, &fontIndexTable, sizeof(FONTDATATABLE), &dwSize, NULL) &&
        WriteFile(hFile, g_pfont_table, fontInfoTable.dwSize, &dwSize, NULL) &&
        WriteFile(hFile, pTmpBuf, fontIndexTable.dwSize, &dwSize, NULL))
    {
        hr = S_OK;
    }

SAVE_FONT_DATA_DONE:
    if (hFile)
        CloseHandle(hFile);
    if (pTmpBuf)
        LocalFree(pTmpBuf);

    return hr;
}

HRESULT CMLFLink2::CFontMappingCache2::LoadFontDataFile(void)
{
    HANDLE  hFontData = NULL;
    HANDLE  hFileMap = NULL;
    LPVOID  lpvFile;
    int *   lp;
    HRESULT hr = E_FAIL;
    DWORD   dwFileSize;

    int     i, j;
    HKEY    hKey = NULL;
    DWORD   nFonts;
    FONTDATAHEADER *pHeader;
    FONTDATATABLE *pfTable;


    hFontData = CreateFile(szFontDataFilePath,  
                        GENERIC_READ,           
                        FILE_SHARE_READ,        
                        NULL,                   
                        OPEN_EXISTING,          
                        FILE_ATTRIBUTE_NORMAL,  
                        NULL);                  

    if (hFontData == INVALID_HANDLE_VALUE)
        return EnsureFontTable(TRUE);

    dwFileSize = GetFileSize(hFontData, NULL);

    hFileMap = CreateFileMapping(
                  hFontData,
                  NULL,
                  PAGE_READONLY,
                  0,
                  dwFileSize,
                  NULL
              );

    if(hFileMap == NULL)
    {
        goto Load_File_Done;
    }

    lpvFile = MapViewOfFile(
                  hFileMap,
                  FILE_MAP_READ,
                  0,
                  0,
                  0
              );

    if (lpvFile == NULL)
    {        
        goto Load_File_Done;
    }

    pHeader = (FONTDATAHEADER *)lpvFile;

    // Check mlang font cache file by signature and checksum
    if (lstrcmpA(pHeader->FileSig, FONT_DATA_SIGNATURE) || pHeader->dwCheckSum != dwFileSize)
    {
        goto Load_File_Done;
    }


    if (S_OK != GetRegFontKey(&hKey, &nFonts))
    {
        goto Load_File_Done;
    }

    pfTable = (FONTDATATABLE *) ((LPBYTE)lpvFile + sizeof(FONTDATAHEADER));

    //!BUGBUG, Check if there is any font change (no guarantee, but works in most cases)
    if (nFonts != ((FONTINFO*)((LPBYTE)lpvFile + pfTable[0].dwOffset))->dwCodePages[1])
    {
        // If there is a change in system font number, we reload everything
        UnmapViewOfFile(lpvFile);
        CloseHandle(hFileMap);
        CloseHandle(hFontData);
        RegCloseKey(hKey);
        return EnsureFontTable(TRUE);
    }

    EnterCriticalSection(&g_cs);
    // Reset cache information
    if (g_pfont_table)
    {
        
        LocalFree(g_pfont_table);
        g_pfont_table = NULL;
        for (i = 0; i < ARRAYSIZE(g_urange_table); i++)
        {
            if (g_urange_table[i].nFonts)
            {
                LocalFree(g_urange_table[i].pFontIndex);
                g_urange_table[i].pFontIndex = NULL;
                g_urange_table[i].nFonts = 0;
            }
        }        
    }


    if(!(g_pfont_table = (FONTINFO *) (LocalAlloc(LPTR, pfTable[0].dwSize))))
    {
        hr = E_OUTOFMEMORY;
        goto Load_File_Done;
    }

    CopyMemory(g_pfont_table, (LPBYTE)lpvFile + pfTable[0].dwOffset, pfTable[0].dwSize);

    lp = (int *)((LPBYTE)lpvFile + pfTable[1].dwOffset);

    for (i = 0; i < ARRAYSIZE(g_urange_table); i++)
    {
        if (g_urange_table[i].nFonts = *lp++)
        {
            //g_urange_table[i].nFonts = *lp++;
            g_urange_table[i].pFontIndex = (int *)LocalAlloc(LPTR, sizeof(int)*g_urange_table[i].nFonts);
            for (j = 0; j<  g_urange_table[i].nFonts; j++)
            {
                g_urange_table[i].pFontIndex[j] = *lp++;
            }
        }
    }

    LeaveCriticalSection(&g_cs);

    hr = S_OK;

Load_File_Done:
    if (lpvFile)
        UnmapViewOfFile(lpvFile);
    if (hFileMap)
        CloseHandle(hFileMap);
    if (hFontData)
        CloseHandle(hFontData);
    if (hKey)
        RegCloseKey(hKey);

    return hr;

}


HRESULT CMLFLink2::CFontMappingCache2::SetFontUnicodeRanges(void)
{
    TCHAR   szFontPath[MAX_PATH];
    TCHAR   szFont[MAX_PATH];
    HRESULT hr = S_OK;
    int     i;
    

    EnterCriticalSection(&g_cs);
    g_pfont_table[0].szFaceName[0] = 1;
    LeaveCriticalSection(&g_cs);
    
    MLGetWindowsDirectory(szFontPath, MAX_PATH);
    MLPathCombine(szFontPath, szFontPath, FONT_FOLDER);

    for (i=1; i<= (int)g_pfont_table[0].dwCodePages[0]; i++)
    {
        MLPathCombine(szFont, szFontPath, g_pfont_table[i].szFileName);
        GetNonCpFontUnicodeRanges(szFont, i);
    }

    // Release un-used memory
    for (i=0; i< ARRAYSIZE(g_urange_table); i++)
    {
        if (g_urange_table[i].nFonts)
            g_urange_table[i].pFontIndex = (int *)LocalReAlloc(g_urange_table[i].pFontIndex, g_urange_table[i].nFonts*sizeof(int), LMEM_MOVEABLE);
    }

    return hr;
}

STDMETHODIMP CMLFLink2::GetStrCodePages(const WCHAR* pszSrc, long cchSrc, DWORD dwPriorityCodePages, DWORD* pdwCodePages, long* pcchCodePages)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pdwCodePages);
    ASSERT_WRITE_PTR_OR_NULL(pcchCodePages);

    HRESULT hr = S_OK;
    long cchCodePages = 0;
    DWORD dwStrCodePages = (DWORD)~0;
    BOOL fInit = FALSE;
    BOOL fNoPri = FALSE;

    if (!pszSrc || cchSrc <= 0) // We can't make dwStrCodePages when cchSrc is zero
        hr = E_INVALIDARG;

    if (!m_pIMLFLnk)
        return E_OUTOFMEMORY;

    while (SUCCEEDED(hr) && cchSrc > 0)
    {
        DWORD dwCharCodePages;

        if (SUCCEEDED(hr = m_pIMLFLnk->GetCharCodePages(*pszSrc, &dwCharCodePages)))
        {
            if (!fInit)
            {
                fInit = TRUE;
                fNoPri = !(dwPriorityCodePages & dwCharCodePages);
            }
            else if (fNoPri != !(dwPriorityCodePages & dwCharCodePages))
            {
                break;
            }
            if (!fNoPri)
                dwPriorityCodePages &= dwCharCodePages;

            if (dwCharCodePages && (dwCharCodePages & dwStrCodePages))
                dwStrCodePages &= dwCharCodePages;
            else
                break;

            pszSrc++;
            cchSrc--;
            cchCodePages++;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!cchCodePages)
        {
            dwStrCodePages = 0;
            cchCodePages++;
        }
        if (pcchCodePages)
            *pcchCodePages = cchCodePages;
        if (pdwCodePages)
            *pdwCodePages = dwStrCodePages;
    }
    else
    {
        if (pcchCodePages)
            *pcchCodePages = 0;
        if (pdwCodePages)
            *pdwCodePages = 0;
    }

    return hr;
}

STDMETHODIMP CMLFLink2::MapFont(HDC hDC, DWORD dwCodePages, WCHAR wchar, HFONT* phDestFont)
{
    HFONT hSrcFont = NULL;

    if (NULL == (hSrcFont = (HFONT) GetCurrentObject(hDC, OBJ_FONT)))
        return E_FAIL;

    if (dwCodePages)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->MapFont(hDC, dwCodePages, hSrcFont, phDestFont);
        return E_OUTOFMEMORY;
    }
    else
    {
        if (!m_pFontMappingCache2)
            m_pFontMappingCache2 = new CFontMappingCache2;
        if (m_pFontMappingCache2)
            return m_pFontMappingCache2->MapFontFromCMAP(hDC, wchar, hSrcFont, phDestFont);
        else
            return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CMLFLink2::GetFontUnicodeRanges(HDC hDC, UINT *puiRanges, UNICODERANGE* pURanges)
{
    int     i;
    LOGFONT lf;
    HRESULT hr = E_FAIL;
    HFONT   hFont = NULL;

    if (!puiRanges)
        return E_INVALIDARG;

    if (!m_pFontMappingCache2)
        m_pFontMappingCache2 = new CFontMappingCache2;
    if (!m_pFontMappingCache2)
        return E_OUTOFMEMORY;

    if (!(hFont = (HFONT)GetCurrentObject(hDC, OBJ_FONT)))
        return hr;

    if (FAILED(m_pFontMappingCache2->EnsureFontTable(FALSE)))
        return hr;

    if (!GetObject(hFont, sizeof(LOGFONT), &lf))
        return hr;

    for (i=1; i<= (int) g_pfont_table[0].dwCodePages[0]; i++)
    {
        if (!lstrcmp(lf.lfFaceName, g_pfont_table[i].szFaceName))
            break;
    }

    if (i > (int) g_pfont_table[0].dwCodePages[0])
        return hr;

    return m_pFontMappingCache2->UnicodeRanges(g_pfont_table[i].szFileName, puiRanges, pURanges);

}

STDMETHODIMP CMLFLink2::GetScriptFontInfo(SCRIPT_ID sid, DWORD dwFlags, UINT *puiFonts, SCRIPTFONTINFO* pScriptFont)
{
    HRESULT hr = E_FAIL;
    UINT    uiNum;
    BYTE    bPitch = dwFlags & SCRIPTCONTF_FIXED_FONT? FIXED_PITCH:VARIABLE_PITCH;

    if (!m_pFontMappingCache2)
        m_pFontMappingCache2 = new CFontMappingCache2;

    if (m_pFontMappingCache2)
        m_pFontMappingCache2->EnsureFontTable(FALSE);

    if (!g_pfont_table)
        return hr;


    if (!pScriptFont)
    {
        uiNum = g_pfont_table[0].dwCodePages[0];
    }
    else
    {
        uiNum = *puiFonts;    
    }

    *puiFonts = 0;

    // Binary search font table to match script id.
    for (UINT i=1; i<= g_pfont_table[0].dwCodePages[0]; i++)
    {
        // Check font pitch
        if (!(g_pfont_table[i].lf.lfPitchAndFamily & bPitch))
            continue;

        // Get sid bit mask
        SCRIPT_IDS sids = 1;
        sids <<= sid;

        if (sids & g_pfont_table[i].scripts)
        {
            // Bail out is required number reached
            if (*puiFonts >= uiNum)
            {
                break;
            }
            if (pScriptFont)
            {
                MultiByteToWideChar(CP_ACP, 0, g_pfont_table[i].szFaceName, -1, (pScriptFont + *puiFonts)->wszFont, MAX_MIMEFACE_NAME);
                (pScriptFont + *puiFonts)->scripts = g_pfont_table[i].scripts;
            }
            (*puiFonts)++;
        }
    }

    return S_OK;

}

// Map Windows code page to script id 
// if multiple script id exist, we'll return the default one
STDMETHODIMP CMLFLink2::CodePageToScriptID(UINT uiCodePage, SCRIPT_ID *pSid)
{
    MIMECPINFO  cpInfo;
    HRESULT     hr = E_FAIL;

    if (!pSid)
        return E_INVALIDARG;

    if (NULL != g_pMimeDatabase)
    {
        if (SUCCEEDED(g_pMimeDatabase->GetCodePageInfo(uiCodePage, 0x409, &cpInfo)))
        {
            if (cpInfo.uiFamilyCodePage == CP_USER_DEFINED)
            {
                *pSid = sidUserDefined;
                hr = S_OK; 
            }
            else
                for (int i = 0; g_CharSetTransTable[i].uCodePage; i++)
                {
                    if (cpInfo.uiFamilyCodePage == g_CharSetTransTable[i].uCodePage)
                    {
                        *pSid = g_CharSetTransTable[i].sid[0];
                        hr = S_OK;
                        break;
                    }
                }            
        }
    }

    return hr;
}
        
CMLFLink2::CFontMappingCache2::CFontMappingCache2(void)
{
    GetSystemDirectory(szFontDataFilePath, MAX_PATH);
    MLPathCombine(szFontDataFilePath, szFontDataFilePath, FONT_DATA_FILE_NAME);
}

CMLFLink2::CFontMappingCache2::~CFontMappingCache2(void)
{
    if (g_pfont_table) 
    {
        LocalFree(g_pfont_table); 
        g_pfont_table = NULL;
    }

    if (g_urange_table)
    {
        for (int i=0; i< ARRAYSIZE(g_urange_table); i++)
        if (g_urange_table[i].nFonts)
        {
            LocalFree(g_urange_table[i].pFontIndex);
        }
    }
}

int CALLBACK CMLFLink2::CFontMappingCache2::MapFontExEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC*, DWORD FontType, LPARAM lParam)
{  
    if (FontType == TRUETYPE_FONTTYPE && plfFont->lfFaceName[0] != TEXT('@') )
    {
        CopyMemory(&g_pfont_table[*(int *)lParam].lf, plfFont, sizeof(LOGFONT));
        (*(int *)lParam)++;
        return 0;
    }
    return 1;
}

int CALLBACK CMLFLink2::CFontMappingCache2::SetFontScriptsEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC*, DWORD FontType, LPARAM lParam)
{      
    if (FontType == TRUETYPE_FONTTYPE)
    {
        if (g_pfont_table)
        {
            for (int i=1; i<= (int)g_pfont_table[0].dwCodePages[0]; i++)
                if (!MLStrCmpNI(plfFont->lfFaceName, g_pfont_table[i].szFaceName, LF_FACESIZE))
                {
                    SCRIPT_IDS scripts = 1;
                    scripts <<= lParam;
                    g_pfont_table[i].scripts |= scripts;
                    break;
                }

            if (i > (int)g_pfont_table[0].dwCodePages[0] && plfFont->lfFaceName[0] != TEXT('@'))        // GDI font not in current font table?
            {
                FONTINFO * pfont_table = NULL;

                pfont_table = (FONTINFO *) LocalReAlloc(g_pfont_table, 
                                        sizeof(FONTINFO) * (g_pfont_table[0].dwCodePages[0]+2),
                                        LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (NULL != pfont_table)
                {
                    g_pfont_table = pfont_table;
                    g_pfont_table[0].dwCodePages[0]++;
                    MLStrCpyN(g_pfont_table[i].szFaceName, (char *)plfFont->lfFaceName, LF_FACESIZE);
                    CopyMemory(&g_pfont_table[i].lf, plfFont, sizeof(LOGFONT));

                    SCRIPT_IDS scripts = 1;
                    scripts <<= lParam;
                    g_pfont_table[i].scripts |= scripts;                    
                }
            }
        }
    }
    return 1;
}

int CALLBACK CMLFLink::VerifyFontSizeEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC* ptm, DWORD FontType, LPARAM lParam)
{
    LOGFONT* plfSrcFont = (LOGFONT*)lParam;

    if (FontType != TRUETYPE_FONTTYPE)
    {
        LONG lHeight = ptm->tmInternalLeading - ptm->tmHeight;
        // Match source font's lfHeight to physical bitmap font's lfHeight
        if (lHeight < 0 && plfSrcFont->lfHeight < 0 && lHeight < plfSrcFont->lfHeight)
        {
            plfSrcFont->lfHeight = lHeight ;
        }
    }

    return 0;
}
            
