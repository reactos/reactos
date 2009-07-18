/*
 * X11 physical font objects
 *
 * Copyright 1997 Alex Korobka
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
 *
 * TODO: Mapping algorithm tweaks, FO_SYNTH_... flags (ExtTextOut() will
 *	 have to be changed for that), dynamic font loading (FreeType).
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winreg.h"
#include "x11font.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

#define X_PFONT_MAGIC		(0xFADE0000)
#define X_FMC_MAGIC		(0x0000CAFE)

#define MAX_FONTS	        1024*16
#define MAX_LFD_LENGTH		256
#define TILDE                   '~'
#define HYPHEN                  '-'

#define DEF_POINT_SIZE          8      /* CreateFont(0 .. ) gets this */
#define DEF_SCALABLE_HEIGHT	100    /* pixels */
#define MIN_FONT_SIZE           2      /* Min size in pixels */
#define MAX_FONT_SIZE           1000   /* Max size in pixels */

#define REMOVE_SUBSETS		1
#define UNMARK_SUBSETS		0

#define FONTCACHE               32     /* dynamic font cache size */

#define FF_FAMILY       (FF_MODERN | FF_SWISS | FF_ROMAN | FF_DECORATIVE | FF_SCRIPT)

typedef struct __fontAlias
{
  LPSTR			faTypeFace;
  LPSTR			faAlias;
  struct __fontAlias*	next;
} fontAlias;

static fontAlias *aliasTable = NULL;

static const char INIFontMetrics[] = "cachedmetrics.";
static const char INIFontSection[] = "Software\\Wine\\X11 Driver\\Fonts";
static const char INIAliasSection[] = "Alias";
static const char INIIgnoreSection[] = "Ignore";
static const char INIDefault[] = "Default";
static const char INIDefaultFixed[] = "DefaultFixed";
static const char INIGlobalMetrics[] = "FontMetrics";
static const char INIDefaultSerif[] = "DefaultSerif";
static const char INIDefaultSansSerif[] = "DefaultSansSerif";


/* FIXME - are there any more Latin charsets ? */
/* FIXME - RUSSIAN, ARABIC, GREEK, HEBREW are NOT Latin */
#define IS_LATIN_CHARSET(ch) \
  ((ch)==ANSI_CHARSET ||\
   (ch)==EE_CHARSET ||\
   (ch)==ISO3_CHARSET ||\
   (ch)==ISO4_CHARSET ||\
   (ch)==RUSSIAN_CHARSET ||\
   (ch)==ARABIC_CHARSET ||\
   (ch)==GREEK_CHARSET ||\
   (ch)==HEBREW_CHARSET ||\
   (ch)==TURKISH_CHARSET ||\
   (ch)==ISO10_CHARSET ||\
   (ch)==BALTIC_CHARSET ||\
   (ch)==CELTIC_CHARSET)

/* suffix-charset mapping tables - must be less than 254 entries long */

typedef struct __sufch
{
  LPCSTR        psuffix;
  WORD          charset; /* hibyte != 0 means *internal* charset */
  WORD          codepage;
  WORD          cptable;
} SuffixCharset;

static const SuffixCharset sufch_ansi[] = {
    {  "0", ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS },
    { NULL, ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_iso646[] = {
    { "irv", ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS },
    { NULL, ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_iso8859[] = {
    {  "1", ANSI_CHARSET, 28591, X11DRV_CPTABLE_SBCS },
    {  "2", EE_CHARSET, 28592, X11DRV_CPTABLE_SBCS },
    {  "3", ISO3_CHARSET, 28593, X11DRV_CPTABLE_SBCS },
    {  "4", ISO4_CHARSET, 28594, X11DRV_CPTABLE_SBCS },
    {  "5", RUSSIAN_CHARSET, 28595, X11DRV_CPTABLE_SBCS },
    {  "6", ARABIC_CHARSET, 28596, X11DRV_CPTABLE_SBCS },
    {  "7", GREEK_CHARSET, 28597, X11DRV_CPTABLE_SBCS },
    {  "8", HEBREW_CHARSET, 28598, X11DRV_CPTABLE_SBCS },
    {  "9", TURKISH_CHARSET, 28599, X11DRV_CPTABLE_SBCS },
    { "10", ISO10_CHARSET, 28600, X11DRV_CPTABLE_SBCS },
    { "11", THAI_CHARSET, 874, X11DRV_CPTABLE_SBCS }, /* FIXME */
    { "12", SYMBOL_CHARSET, CP_SYMBOL, X11DRV_CPTABLE_SBCS },
    { "13", BALTIC_CHARSET, 28603, X11DRV_CPTABLE_SBCS },
    { "14", CELTIC_CHARSET, 28604, X11DRV_CPTABLE_SBCS },
    { "15", ANSI_CHARSET, 28605, X11DRV_CPTABLE_SBCS },
    { "16", ISO3_CHARSET, 28606, X11DRV_CPTABLE_SBCS },
    { NULL, ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_microsoft[] = {
    { "cp1250", EE_CHARSET, 1250, X11DRV_CPTABLE_SBCS },
    { "cp1251", RUSSIAN_CHARSET, 1251, X11DRV_CPTABLE_SBCS },
    { "cp1252", ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS },
    { "cp1253", GREEK_CHARSET, 1253, X11DRV_CPTABLE_SBCS },
    { "cp1254", TURKISH_CHARSET, 1254, X11DRV_CPTABLE_SBCS },
    { "cp1255", HEBREW_CHARSET, 1255, X11DRV_CPTABLE_SBCS },
    { "cp1256", ARABIC_CHARSET, 1256, X11DRV_CPTABLE_SBCS },
    { "cp1257", BALTIC_CHARSET, 1257, X11DRV_CPTABLE_SBCS },
    { "fontspecific", SYMBOL_CHARSET, CP_SYMBOL, X11DRV_CPTABLE_SYMBOL },
    { "symbol", SYMBOL_CHARSET, CP_SYMBOL, X11DRV_CPTABLE_SYMBOL },
    {   NULL,   ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_tcvn[] = {
    {  "0", TCVN_CHARSET, 1252, X11DRV_CPTABLE_SBCS }, /* FIXME */
    { NULL, TCVN_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_tis620[] = {
    {  "0", THAI_CHARSET, 874, X11DRV_CPTABLE_SBCS }, /* FIXME */
    { NULL, THAI_CHARSET, 874, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_viscii[] = {
    {  "1", VISCII_CHARSET, 1252, X11DRV_CPTABLE_SBCS }, /* FIXME */
    { NULL, VISCII_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_windows[] = {
    { "1250", EE_CHARSET, 1250, X11DRV_CPTABLE_SBCS },
    { "1251", RUSSIAN_CHARSET, 1251, X11DRV_CPTABLE_SBCS },
    { "1252", ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS },
    { "1253", GREEK_CHARSET, 1253, X11DRV_CPTABLE_SBCS },
    { "1254", TURKISH_CHARSET, 1254, X11DRV_CPTABLE_SBCS },
    { "1255", HEBREW_CHARSET, 1255, X11DRV_CPTABLE_SBCS },
    { "1256", ARABIC_CHARSET, 1256, X11DRV_CPTABLE_SBCS },
    { "1257", BALTIC_CHARSET, 1257, X11DRV_CPTABLE_SBCS },
    {  NULL,  ANSI_CHARSET, 1252, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_koi8[] = {
    { "r", RUSSIAN_CHARSET, 20866, X11DRV_CPTABLE_SBCS },
    { "ru", RUSSIAN_CHARSET, 21866, X11DRV_CPTABLE_SBCS },
    { "u", RUSSIAN_CHARSET, 21866, X11DRV_CPTABLE_SBCS },
    { NULL, RUSSIAN_CHARSET, 20866, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_jisx0201[] = {
    { "0", X11FONT_JISX0201_CHARSET, 932, X11DRV_CPTABLE_SBCS },
    { NULL, X11FONT_JISX0201_CHARSET, 932, X11DRV_CPTABLE_SBCS }};

static const SuffixCharset sufch_jisx0208[] = {
    { "0", SHIFTJIS_CHARSET, 932, X11DRV_CPTABLE_CP932 },
    { NULL, SHIFTJIS_CHARSET, 932, X11DRV_CPTABLE_CP932 }};

static const SuffixCharset sufch_jisx0212[] = {
    { "0", X11FONT_JISX0212_CHARSET, 932, X11DRV_CPTABLE_CP932 },
    { NULL, X11FONT_JISX0212_CHARSET, 932, X11DRV_CPTABLE_CP932 }};

static const SuffixCharset sufch_ksc5601[] = {
    { "0", HANGEUL_CHARSET, 949, X11DRV_CPTABLE_CP949 },
    { NULL, HANGEUL_CHARSET, 949, X11DRV_CPTABLE_CP949 }};

static const SuffixCharset sufch_gb2312[] = {
    { "0", GB2312_CHARSET, 936, X11DRV_CPTABLE_CP936 },
    { NULL, GB2312_CHARSET, 936, X11DRV_CPTABLE_CP936 }};

static const SuffixCharset sufch_big5[] = {
    { "0", CHINESEBIG5_CHARSET, 950, X11DRV_CPTABLE_CP950 },
    { NULL, CHINESEBIG5_CHARSET, 950, X11DRV_CPTABLE_CP950 }};

static const SuffixCharset sufch_unicode[] = {
    { "0", DEFAULT_CHARSET, 0, X11DRV_CPTABLE_UNICODE },
    { NULL, DEFAULT_CHARSET, 0, X11DRV_CPTABLE_UNICODE }};

static const SuffixCharset sufch_iso10646[] = {
    { "1", DEFAULT_CHARSET, 0, X11DRV_CPTABLE_UNICODE },
    { NULL, DEFAULT_CHARSET, 0, X11DRV_CPTABLE_UNICODE }};

static const SuffixCharset sufch_dec[] = {
    { "dectech", SYMBOL_CHARSET, CP_SYMBOL, X11DRV_CPTABLE_SBCS },
    { NULL, 0, 0, X11DRV_CPTABLE_SBCS }};

/* Each of these must be matched explicitly */
static const SuffixCharset sufch_any[] = {
    { "fontspecific", SYMBOL_CHARSET, CP_SYMBOL, X11DRV_CPTABLE_SBCS },
    { NULL, 0, 0, X11DRV_CPTABLE_SBCS }};


typedef struct __fet
{
  LPCSTR	 prefix;
  const SuffixCharset* sufch;
  const struct __fet*  next;
} fontEncodingTemplate;

/* Note: we can attach additional encoding mappings to the end
 *       of this table at runtime.
 */
static const fontEncodingTemplate fETTable[] = {
			{ "ansi",         sufch_ansi,         &fETTable[1] },
			{ "ascii",        sufch_ansi,         &fETTable[2] },
			{ "iso646.1991",  sufch_iso646,       &fETTable[3] },
			{ "iso8859",      sufch_iso8859,      &fETTable[4] },
			{ "microsoft",    sufch_microsoft,    &fETTable[5] },
			{ "tcvn",         sufch_tcvn,         &fETTable[6] },
			{ "tis620.2533",  sufch_tis620,       &fETTable[7] },
			{ "viscii1.1",    sufch_viscii,       &fETTable[8] },
			{ "windows",      sufch_windows,      &fETTable[9] },
			{ "koi8",         sufch_koi8,         &fETTable[10]},
			{ "jisx0201.1976",sufch_jisx0201,     &fETTable[11]},
			{ "jisc6226.1978",sufch_jisx0208,     &fETTable[12]},
			{ "jisx0208.1983",sufch_jisx0208,     &fETTable[13]},
			{ "jisx0208.1990",sufch_jisx0208,     &fETTable[14]},
			{ "jisx0212.1990",sufch_jisx0212,     &fETTable[15]},
			{ "ksc5601.1987", sufch_ksc5601,      &fETTable[16]},
			{ "gb2312.1980",  sufch_gb2312,       &fETTable[17]},
			{ "big5",	  sufch_big5,         &fETTable[18]},
			{ "unicode",      sufch_unicode,      &fETTable[19]},
			{ "iso10646",     sufch_iso10646,     &fETTable[20]},
			{ "cp",           sufch_windows,      &fETTable[21]},
			{ "dec",          sufch_dec,          &fETTable[22]},
			/* NULL prefix matches anything so put it last */
			{   NULL,         sufch_any,          NULL },
};

/* a charset database for known facenames */
struct CharsetBindingInfo
{
	const char*	pszFaceName;
	BYTE		charset;
};
static const struct CharsetBindingInfo charsetbindings[] =
{
	/* special facenames */
	{ "System", DEFAULT_CHARSET },
	{ "FixedSys", DEFAULT_CHARSET },

	/* known facenames */
	{ "MS Serif", ANSI_CHARSET },
	{ "MS Sans Serif", ANSI_CHARSET },
	{ "Courier", ANSI_CHARSET },
	{ "Symbol", SYMBOL_CHARSET },

	{ "Arial", ANSI_CHARSET },
	{ "Arial Greek", GREEK_CHARSET },
	{ "Arial Tur", TURKISH_CHARSET },
	{ "Arial Baltic", BALTIC_CHARSET },
	{ "Arial CE", EASTEUROPE_CHARSET },
	{ "Arial Cyr", RUSSIAN_CHARSET },
	{ "Courier New", ANSI_CHARSET },
	{ "Courier New Greek", GREEK_CHARSET },
	{ "Courier New Tur", TURKISH_CHARSET },
	{ "Courier New Baltic", BALTIC_CHARSET },
	{ "Courier New CE", EASTEUROPE_CHARSET },
	{ "Courier New Cyr", RUSSIAN_CHARSET },
	{ "Times New Roman", ANSI_CHARSET },
	{ "Times New Roman Greek", GREEK_CHARSET },
	{ "Times New Roman Tur", TURKISH_CHARSET },
	{ "Times New Roman Baltic", BALTIC_CHARSET },
	{ "Times New Roman CE", EASTEUROPE_CHARSET },
	{ "Times New Roman Cyr", RUSSIAN_CHARSET },

	{ "\x82\x6c\x82\x72 \x83\x53\x83\x56\x83\x62\x83\x4e",
			SHIFTJIS_CHARSET }, /* MS gothic */
	{ "\x82\x6c\x82\x72 \x82\x6f\x83\x53\x83\x56\x83\x62\x83\x4e",
			SHIFTJIS_CHARSET }, /* MS P gothic */
	{ "\x82\x6c\x82\x72 \x96\xbe\x92\xa9",
			SHIFTJIS_CHARSET }, /* MS mincho */
	{ "\x82\x6c\x82\x72 \x82\x6f\x96\xbe\x92\xa9",
			SHIFTJIS_CHARSET }, /* MS P mincho */
	{ "GulimChe", HANGEUL_CHARSET },
	{ "\xcb\xce\xcc\xe5", GB2312_CHARSET }, /* SimSun */
	{ "\xba\xda\xcc\xe5", GB2312_CHARSET }, /* SimHei */
	{ "\xb7\x73\xb2\xd3\xa9\xfa\xc5\xe9", CHINESEBIG5_CHARSET },/*MS Mingliu*/
	{ "\xb2\xd3\xa9\xfa\xc5\xe9", CHINESEBIG5_CHARSET },

	{ NULL, 0 }
};


static int		DefResolution = 0;

static CRITICAL_SECTION crtsc_fonts_X11;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &crtsc_fonts_X11,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": crtsc_fonts_X11") }
};
static CRITICAL_SECTION crtsc_fonts_X11 = { &critsect_debug, -1, 0, 0, 0, 0 };

static fontResource*	fontList = NULL;
static fontObject*      fontCache = NULL;		/* array */
static unsigned int     fontCacheSize = FONTCACHE;
static int		fontLF = -1, fontMRU = -1;	/* last free, most recently used */

#define __PFONT(pFont)     ( fontCache + ((UINT)(pFont) & 0x0000FFFF) )
#define CHECK_PFONT(pFont) ( (((UINT)(pFont) & 0xFFFF0000) == X_PFONT_MAGIC) &&\
			     (((UINT)(pFont) & 0x0000FFFF) < fontCacheSize) )

/***********************************************************************
 *           Helper macros from X distribution
 */

#define CI_NONEXISTCHAR(cs) (((cs)->width == 0) && \
			     (((cs)->rbearing|(cs)->lbearing| \
			       (cs)->ascent|(cs)->descent) == 0))

#define CI_GET_CHAR_INFO(fs,col,def,cs) \
{ \
    cs = def; \
    if (col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) { \
	if (fs->per_char == NULL) { \
	    cs = &fs->min_bounds; \
	} else { \
	    cs = &fs->per_char[(col - fs->min_char_or_byte2)]; \
	    if (CI_NONEXISTCHAR(cs)) cs = def; \
	} \
    } \
}

#define CI_GET_DEFAULT_INFO(fs,cs) \
  CI_GET_CHAR_INFO(fs, fs->default_char, NULL, cs)


/***********************************************************************
 *           is_stock_font
 */
static inline BOOL is_stock_font( HFONT font )
{
    int i;
    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
    {
        if (i != DEFAULT_PALETTE && font == GetStockObject(i)) return TRUE;
    }
    return FALSE;
}


static void FONT_LogFontWTo16( const LOGFONTW* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    WideCharToMultiByte( CP_ACP, 0, font32->lfFaceName, -1,
                         font16->lfFaceName, LF_FACESIZE, NULL, NULL );
    font16->lfFaceName[LF_FACESIZE-1] = 0;
}


/***********************************************************************
 *           Checksums
 */
static UINT16   __lfCheckSum( const LOGFONT16 *plf )
{
    CHAR        font[LF_FACESIZE];
    UINT16      checksum = 0;
    const UINT16 *ptr;
    int i;

    ptr = (const UINT16 *)plf;
    for (i = 0; i < 9; i++) checksum ^= *ptr++;
    for (i = 0; i < LF_FACESIZE; i++)
    {
        font[i] = tolower(plf->lfFaceName[i]);
        if (!font[i] || font[i] == ' ') break;
    }
    for (ptr = (UINT16 *)font, i >>= 1; i > 0; i-- ) checksum ^= *ptr++;
   return checksum;
}

static UINT16   __genericCheckSum( const void *ptr, int size )
{
   unsigned int checksum = 0;
   const char *p = ptr;
   while (size-- > 0)
     checksum ^= (checksum << 3) + (checksum >> 29) + *p++;

   return checksum & 0xffff;
}

/*************************************************************************
 *           LFD parse/compose routines
 *
 * NB. LFD_Parse will use lpFont for its own ends, so if you want to keep it
 *     make a copy first
 *
 * These functions also do TILDE to HYPHEN conversion
 */
static BOOL LFD_Parse(LPSTR lpFont, LFD *lfd)
{
    char *lpch = lpFont, *lfd_fld[LFD_FIELDS], *field_start;
    int i;
    if (*lpch != HYPHEN)
    {
        WARN("font '%s' doesn't begin with '%c'\n", lpFont, HYPHEN);
        return FALSE;
    }

    field_start = ++lpch;
    for( i = 0; i < LFD_FIELDS; )
    {
	if (*lpch == HYPHEN)
	{
	    *lpch = '\0';
	    lfd_fld[i] = field_start;
	    i++;
	    field_start = ++lpch;
	}
	else if (!*lpch)
	{
	    lfd_fld[i] = field_start;
	    i++;
	    break;
	}
	else if (*lpch == TILDE)
	{
	    *lpch = HYPHEN;
	    ++lpch;
	}
	else
	    ++lpch;
    }
    /* Fill in the empty fields with NULLS */
    for (; i< LFD_FIELDS; i++)
	lfd_fld[i] = NULL;
    if (*lpch)
	WARN("Extra ignored in font '%s'\n", lpFont);

    lfd->foundry = lfd_fld[0];
    lfd->family = lfd_fld[1];
    lfd->weight = lfd_fld[2];
    lfd->slant = lfd_fld[3];
    lfd->set_width = lfd_fld[4];
    lfd->add_style = lfd_fld[5];
    lfd->pixel_size = lfd_fld[6];
    lfd->point_size = lfd_fld[7];
    lfd->resolution_x = lfd_fld[8];
    lfd->resolution_y = lfd_fld[9];
    lfd->spacing = lfd_fld[10];
    lfd->average_width = lfd_fld[11];
    lfd->charset_registry = lfd_fld[12];
    lfd->charset_encoding = lfd_fld[13];
    return TRUE;
}


static void LFD_UnParse(LPSTR dp, UINT buf_size, LFD* lfd)
{
    const char* lfd_fld[LFD_FIELDS];
    int i;

    if (!buf_size)
	return; /* Don't be silly */

    lfd_fld[0]  = lfd->foundry;
    lfd_fld[1]	= lfd->family;
    lfd_fld[2]	= lfd->weight ;
    lfd_fld[3]	= lfd->slant ;
    lfd_fld[4]  = lfd->set_width ;
    lfd_fld[5]	= lfd->add_style;
    lfd_fld[6]	= lfd->pixel_size;
    lfd_fld[7]	= lfd->point_size;
    lfd_fld[8]	= lfd->resolution_x;
    lfd_fld[9]	= lfd->resolution_y ;
    lfd_fld[10] = lfd->spacing ;
    lfd_fld[11] = lfd->average_width ;
    lfd_fld[12] = lfd->charset_registry ;
    lfd_fld[13] = lfd->charset_encoding ;

    buf_size--; /* Room for the terminator */

    for (i = 0; i < LFD_FIELDS; i++)
    {
	const char* sp = lfd_fld[i];
	if (!sp || !buf_size)
	    break;

	*dp++ = HYPHEN;
	buf_size--;
	while (buf_size > 0 && *sp)
	{
	    *dp = (*sp == HYPHEN) ? TILDE : *sp;
	    buf_size--;
	    dp++; sp++;
	}
    }
    *dp = '\0';
}


static void LFD_GetWeight( fontInfo* fi, LPCSTR lpStr)
{
    int j = strlen(lpStr);
    if( j == 1 && *lpStr == '0')
	fi->fi_flags |= FI_POLYWEIGHT;
    else if( j == 4 )
    {
	if( !strcasecmp( "bold", lpStr) )
	    fi->df.dfWeight = FW_BOLD;
	else if( !strcasecmp( "demi", lpStr) )
	{
	    fi->fi_flags |= FI_FW_DEMI;
	    fi->df.dfWeight = FW_DEMIBOLD;
	}
	else if( !strcasecmp( "book", lpStr) )
	{
	    fi->fi_flags |= FI_FW_BOOK;
	    fi->df.dfWeight = FW_REGULAR;
	}
    }
    else if( j == 5 )
    {
	if( !strcasecmp( "light", lpStr) )
	    fi->df.dfWeight = FW_LIGHT;
	else if( !strcasecmp( "black", lpStr) )
	    fi->df.dfWeight = FW_BLACK;
    }
    else if( j == 6 && !strcasecmp( "medium", lpStr) )
	fi->df.dfWeight = FW_REGULAR;
    else if( j == 8 && !strcasecmp( "demibold", lpStr) )
	fi->df.dfWeight = FW_DEMIBOLD;
    else
	fi->df.dfWeight = FW_DONTCARE; /* FIXME: try to get something
					* from the weight property */
}

static BOOL LFD_GetSlant( fontInfo* fi, LPCSTR lpStr)
{
    int l = strlen(lpStr);
    if( l == 1 )
    {
	switch( tolower( *lpStr ) )
	{
	    case '0':  fi->fi_flags |= FI_POLYSLANT;	/* haven't seen this one yet */
	    default:
	    case 'r':  fi->df.dfItalic = 0;
		       break;
	    case 'o':
		       fi->fi_flags |= FI_OBLIQUE;
	    case 'i':  fi->df.dfItalic = 1;
		       break;
	}
	return FALSE;
    }
    return TRUE;
}

static void LFD_GetStyle( fontInfo* fi, LPCSTR lpstr, int dec_style_check)
{
    int j = strlen(lpstr);
    if( j > 3 )	/* find out is there "sans" or "script" */
    {
	j = 0;

	if( strstr(lpstr, "sans") )
	{
	    fi->df.dfPitchAndFamily |= FF_SWISS;
	    j = 1;
	}
	if( strstr(lpstr, "script") )
	{
	    fi->df.dfPitchAndFamily |= FF_SCRIPT;
	    j = 1;
	}
	if( !j && dec_style_check )
	    fi->df.dfPitchAndFamily |= FF_DECORATIVE;
   }
}

/*************************************************************************
 *           LFD_InitFontInfo
 *
 * INIT ONLY
 *
 * Fill in some fields in the fontInfo struct.
 */
static int LFD_InitFontInfo( fontInfo* fi, const LFD* lfd, LPCSTR fullname )
{
   int    	i, j, dec_style_check, scalability;
   const fontEncodingTemplate* boba;
   const char* ridiculous = "font '%s' has ridiculous %s\n";
   const char* lpstr;

   if (!lfd->charset_registry)
   {
       WARN("font '%s' does not have enough fields\n", fullname);
       return FALSE;
   }

   memset(fi, 0, sizeof(fontInfo) );

/* weight name - */
   LFD_GetWeight( fi, lfd->weight);

/* slant - */
   dec_style_check = LFD_GetSlant( fi, lfd->slant);

/* width name - */
   lpstr = lfd->set_width;
   if( strcasecmp( "normal", lpstr) )	/* XXX 'narrow', 'condensed', etc... */
       dec_style_check = TRUE;
   else
       fi->fi_flags |= FI_NORMAL;

/* style - */
   LFD_GetStyle(fi, lfd->add_style, dec_style_check);

/* pixel & decipoint height, and res_x & y */

   scalability = 0;

   j = strlen(lfd->pixel_size);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "pixel_size");
       return FALSE;
   }
   if( !(fi->lfd_height = atoi(lfd->pixel_size)) )
       scalability++;

   j = strlen(lfd->point_size);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "point_size");
       return FALSE;
   }
   if( !(atoi(lfd->point_size)) )
       scalability++;

   j = strlen(lfd->resolution_x);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "resolution_x");
       return FALSE;
   }
   if( !(fi->lfd_resolution = atoi(lfd->resolution_x)) )
       scalability++;

   j = strlen(lfd->resolution_y);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "resolution_y");
       return FALSE;
   }
   if( !(atoi(lfd->resolution_y)) )
       scalability++;

   /* Check scalability */
   switch (scalability)
   {
   case 0: /* Bitmap */
       break;
   case 4: /* Scalable */
       fi->fi_flags |= FI_SCALABLE;
       break;
   case 2:
       /* #$%^!!! X11R6 mutant garbage (scalable bitmap) */
       TRACE("Skipping scalable bitmap '%s'\n", fullname);
       return FALSE;
   default:
       WARN("Font '%s' has weird scalability\n", fullname);
       return FALSE;
   }

/* spacing - */
   lpstr = lfd->spacing;
   switch( *lpstr )
   {
     case '\0':
	 WARN("font '%s' has no spacing\n", fullname);
	 return FALSE;

     case 'p': fi->fi_flags |= FI_VARIABLEPITCH;
	       break;
     case 'c': fi->df.dfPitchAndFamily |= FF_MODERN;
	       fi->fi_flags |= FI_FIXEDEX;
	       /* fall through */
     case 'm': fi->fi_flags |= FI_FIXEDPITCH;
	       break;
     default:
               /* Of course this line does nothing */
	       fi->df.dfPitchAndFamily |= DEFAULT_PITCH | FF_DONTCARE;
   }

/* average width - */
   lpstr = lfd->average_width;
   j = strlen(lpstr);
   if( j == 0 || j > 3 )
   {
       WARN(ridiculous, fullname, "average_width");
       return FALSE;
   }

   if( !(atoi(lpstr)) && !scalability )
   {
       WARN("font '%s' has average_width 0 but is not scalable!!\n", fullname);
       return FALSE;
   }

/* charset registry, charset encoding - */
   lpstr = lfd->charset_registry;
   if( strstr(lpstr, "ksc") ||
       strstr(lpstr, "gb2312") ||
       strstr(lpstr, "big5") )
   {
       FIXME("DBCS fonts like '%s' are not working correctly now.\n", fullname);
   }

   fi->df.dfCharSet = ANSI_CHARSET;

   for( i = 0, boba = &fETTable[0]; boba; boba = boba->next, i++ )
   {
       if (!boba->prefix || !strcasecmp(lpstr, boba->prefix))
       {
	   if (lfd->charset_encoding)
	   {
	       for( j = 0; boba->sufch[j].psuffix; j++ )
	       {
		   if( !strcasecmp(lfd->charset_encoding, boba->sufch[j].psuffix ))
		   {
		       fi->df.dfCharSet = (BYTE)(boba->sufch[j].charset & 0xff);
		       fi->internal_charset = boba->sufch[j].charset;
		       fi->codepage = boba->sufch[j].codepage;
		       fi->cptable = boba->sufch[j].cptable;
		       goto done;
		   }
	       }

               fi->df.dfCharSet = (BYTE)(boba->sufch[j].charset & 0xff);
               fi->internal_charset = boba->sufch[j].charset;
               fi->codepage = boba->sufch[j].codepage;
               fi->cptable = boba->sufch[j].cptable;
               if (boba->prefix)
               {
                  FIXME("font '%s' has unknown character encoding '%s' in known registry '%s'\n",
                       fullname, lfd->charset_encoding, boba->prefix);
                  j = 254;
               }
               else
               {
                  FIXME("font '%s' has unknown registry '%s' and character encoding '%s'\n",
                       fullname, lfd->charset_registry, lfd->charset_encoding);
                  j = 255;
               }

               WARN("Defaulting to: df.dfCharSet = %d,  internal_charset = %d, codepage = %d, cptable = %d\n",
                    fi->df.dfCharSet,fi->internal_charset, fi->codepage, fi->cptable);
               goto done;
	   }
	   else if (boba->prefix)
	   {
               WARN("font '%s' has known registry '%s' and no character encoding\n",
                    fullname, lpstr);
	       for( j = 0; boba->sufch[j].psuffix; j++ )
		   ;
	       fi->df.dfCharSet = (BYTE)(boba->sufch[j].charset & 0xff);
	       fi->internal_charset = boba->sufch[j].charset;
	       fi->codepage = boba->sufch[j].codepage;
	       fi->cptable = boba->sufch[j].cptable;
	       j = 255;
	       goto done;
	   }
       }
   }
   WARN("font '%s' has unknown character set '%s'\n", fullname, lpstr);
   return FALSE;

done:
   /* i - index into fETTable
    * j - index into suffix array for fETTable[i]
    *     except:
    *     254 - found encoding prefix, unknown suffix
    *     255 - no encoding match at all.
    */
   fi->fi_encoding = 256 * (UINT16)i + (UINT16)j;

   return TRUE;
}


/*************************************************************************
 *           LFD_AngleMatrix
 *
 * make a matrix suitable for LFD based on theta radians
 */
static void LFD_AngleMatrix( char* buffer, int h, double theta)
{
    double matrix[4];
    matrix[0] = h*cos(theta);
    matrix[1] = h*sin(theta);
    matrix[2] = -h*sin(theta);
    matrix[3] = h*cos(theta);
    sprintf(buffer, "[%+f%+f%+f%+f]", matrix[0], matrix[1], matrix[2], matrix[3]);
}

/*************************************************************************
 *           LFD_ComposeLFD
 *
 * Note: uRelax is a treatment not a cure. Font mapping algorithm
 *       should be bulletproof enough to allow us to avoid hacks like
 *	 this despite LFD being so braindead.
 */
static BOOL LFD_ComposeLFD( const fontObject* fo,
			    INT height, LPSTR lpLFD, UINT uRelax )
{
   int		i, h;
   const char   *any = "*";
   char         h_string[64], resx_string[64], resy_string[64];
   LFD          aLFD;
   const fontEncodingTemplate* boba = &fETTable[0];

/* Get the worst case over with first */

/* RealizeFont() will call us repeatedly with increasing uRelax
 * until XLoadFont() succeeds.
 * to avoid an infinite loop; these will always match
 */
   if (uRelax >= 6)
   {
       if (uRelax == 6)
	   sprintf( lpLFD, "-*-*-*-*-*-*-*-*-*-*-*-*-iso8859-1" );
       else
	   sprintf( lpLFD, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*" );
       return TRUE;
   }

/* read foundry + family from fo */
   aLFD.foundry = fo->fr->resource->foundry;
   aLFD.family  = fo->fr->resource->family;

/* add weight */
   switch( fo->fi->df.dfWeight )
   {
        case FW_BOLD:
		aLFD.weight = "bold"; break;
	case FW_REGULAR:
		if( fo->fi->fi_flags & FI_FW_BOOK )
		    aLFD.weight = "book";
		else
		    aLFD.weight = "medium";
		break;
	case FW_DEMIBOLD:
		aLFD.weight = "demi";
		if( !( fo->fi->fi_flags & FI_FW_DEMI) )
		     aLFD.weight = "bold";
		break;
	case FW_BLACK:
		aLFD.weight = "black"; break;
	case FW_LIGHT:
		aLFD.weight = "light"; break;
	default:
		aLFD.weight = any;
   }

/* add slant */
   if( fo->fi->df.dfItalic )
       if( fo->fi->fi_flags & FI_OBLIQUE )
	   aLFD.slant = "o";
       else
	   aLFD.slant = "i";
   else
       aLFD.slant = (uRelax <= 1) ? "r" : any;

/* add width */
   if( fo->fi->fi_flags & FI_NORMAL )
       aLFD.set_width = "normal";
   else
       aLFD.set_width = any;

/* ignore style */
   aLFD.add_style = any;

/* add pixelheight
 *
 * FIXME: fill in lpXForm and lpPixmap for rotated fonts
 */
   if( fo->fo_flags & FO_SYNTH_HEIGHT )
       h = fo->fi->lfd_height;
   else
   {
       h = (fo->fi->lfd_height * height + (fo->fi->df.dfPixHeight>>1));
       h /= fo->fi->df.dfPixHeight;
   }
   if (h < MIN_FONT_SIZE) /* Resist rounding down to 0 */
       h = MIN_FONT_SIZE;
   else if (h > MAX_FONT_SIZE) /* Sanity check as huge fonts can lock up the font server */
   {
       WARN("Huge font size %d pixels has been reduced to %d\n", h, MAX_FONT_SIZE);
       h = MAX_FONT_SIZE;
   }

   if (uRelax <= 3)
       /* handle rotated fonts */
       if (fo->lf.lfEscapement) {
	   /* escapement is in tenths of degrees, theta is in radians */
	   double theta = M_PI*fo->lf.lfEscapement/1800.;
	   LFD_AngleMatrix(h_string, h, theta);
       }
       else
       {
	   sprintf(h_string, "%d", h);
       }
   else
       sprintf(h_string, "%d", fo->fi->lfd_height);

   aLFD.pixel_size = h_string;
   aLFD.point_size = any;

/* resolution_x,y, average width */
   /* FIXME - Why do some font servers ignore average width ?
    * so that you have to mess around with res_y
    */
   aLFD.average_width = any;
   if (uRelax <= 4)
   {
       sprintf(resx_string, "%d", fo->fi->lfd_resolution);
       aLFD.resolution_x = resx_string;

       strcpy(resy_string, resx_string);
       if( uRelax == 0  && text_caps & TC_SF_X_YINDEP )
       {
	   if( fo->lf.lfWidth && !(fo->fo_flags & FO_SYNTH_WIDTH))
	   {
	       int resy = 0.5 + fo->fi->lfd_resolution *
		   (fo->fi->df.dfAvgWidth * height) /
		   (fo->lf.lfWidth * fo->fi->df.dfPixHeight) ;
	       sprintf(resy_string,  "%d", resy);
	   }
	   else
	   {
	       /* FIXME - synth width */
           }
       }
       aLFD.resolution_y = resy_string;
   }
   else
   {
       aLFD.resolution_x = aLFD.resolution_y = any;
   }

/* spacing */
   {
       const char* w;

       if( fo->fi->fi_flags & FI_FIXEDPITCH )
	   w = ( fo->fi->fi_flags & FI_FIXEDEX ) ? "c" : "m";
       else
	   w = ( fo->fi->fi_flags & FI_VARIABLEPITCH ) ? "p" : any;

       aLFD.spacing = (uRelax <= 2) ? w : any;
   }

/* encoding */

   for(i = fo->fi->fi_encoding >> 8; i; i--) boba = boba->next;
   aLFD.charset_registry = boba->prefix ? boba->prefix : any;

   i = fo->fi->fi_encoding & 255;
   switch( i ) {
   default:
       aLFD.charset_encoding = boba->sufch[i].psuffix;
       break;

   case 254:
       aLFD.charset_encoding = any;
       break;

   case 255: /* no suffix - it ends, e.g., "-ascii" */
       aLFD.charset_encoding = NULL;
       break;
   }

   LFD_UnParse(lpLFD, MAX_LFD_LENGTH, &aLFD);

   TRACE("\tLFD(uRelax=%d): %s\n", uRelax, lpLFD );
   return TRUE;
}


/***********************************************************************
 *		X Font Resources
 */
static void XFONT_GetLeading( const IFONTINFO16 *pFI, const XFontStruct* x_fs,
			      INT16* pIL, INT16* pEL, const XFONTTRANS *XFT )
{
    unsigned long height;
    unsigned min = (unsigned char)pFI->dfFirstChar;
    unsigned max = (unsigned char)pFI->dfLastChar;
    BOOL bIsLatin = IS_LATIN_CHARSET(pFI->dfCharSet);

    if( pEL ) *pEL = 0;

    if(XFT) {
        wine_tsx11_lock();
	if(XGetFontProperty((XFontStruct*)x_fs, x11drv_atom(RAW_CAP_HEIGHT), &height))
	    *pIL = XFT->ascent -
                            (INT)(XFT->pixelsize / 1000.0 * height);
	else
	    *pIL = 0;
        wine_tsx11_unlock();
	return;
    }

    wine_tsx11_lock();
    if( XGetFontProperty((XFontStruct*)x_fs, XA_CAP_HEIGHT, &height) == FALSE )
    {
        if( x_fs->per_char )
	    if( bIsLatin && ((unsigned char)'X' <= (max - min)) )
		    height = x_fs->per_char['X' - min].ascent;
	    else
		if (x_fs->ascent >= x_fs->max_bounds.ascent)
		    height = x_fs->max_bounds.ascent;
		else
		{
		    height = x_fs->ascent;
		    if( pEL )
			*pEL = x_fs->max_bounds.ascent - height;
		}
	else
	    height = x_fs->min_bounds.ascent;
    }
    wine_tsx11_unlock();

    *pIL = x_fs->ascent - height;
}

/***********************************************************************
 *           XFONT_CharWidth
 */
static int XFONT_CharWidth(const XFontStruct* x_fs,
			   const XFONTTRANS *XFT, int ch)
{
    if(!XFT)
	return x_fs->per_char[ch].width;
    else
	return x_fs->per_char[ch].attributes * XFT->pixelsize / 1000.0;
}

/***********************************************************************
 *           XFONT_GetAvgCharWidth
 */
static INT XFONT_GetAvgCharWidth( LPIFONTINFO16 pFI, const XFontStruct* x_fs,
				    const XFONTTRANS *XFT)
{
    unsigned min = (unsigned char)pFI->dfFirstChar;
    unsigned max = (unsigned char)pFI->dfLastChar;

    INT avg;

    if( x_fs->per_char )
    {
	unsigned int width = 0, chars = 0, j;
	if( (IS_LATIN_CHARSET(pFI->dfCharSet) ||
	    pFI->dfCharSet == DEFAULT_CHARSET) &&
            (max - min) >= (unsigned char)'z' )
	{
	    /* FIXME - should use a weighted average */
	    for( j = 0; j < 26; j++ )
		width += XFONT_CharWidth(x_fs, XFT, 'a' + j - min) +
		         XFONT_CharWidth(x_fs, XFT, 'A' + j - min);
	    chars = 52;
	}
	else /* unweighted average of everything */
	{
	    for( j = 0,  max -= min; j <= max; j++ )
		if( !CI_NONEXISTCHAR(x_fs->per_char + j) )
		{
		    width += XFONT_CharWidth(x_fs, XFT, j);
		    chars++;
		}
	}
	if (chars) avg = (width + (chars-1))/ chars; /* always round up*/
	else       avg = 0; /* No characters exist at all */
    }
    else /* uniform width */
	avg = x_fs->min_bounds.width;

    TRACE(" retuning %d\n",avg);
    return avg;
}

/***********************************************************************
 *           XFONT_GetMaxCharWidth
 */
static INT XFONT_GetMaxCharWidth(const XFontStruct* xfs, const XFONTTRANS *XFT)
{
    unsigned min = (unsigned char)xfs->min_char_or_byte2;
    unsigned max = (unsigned char)xfs->max_char_or_byte2;
    unsigned int maxwidth, j;

    if(!XFT || !xfs->per_char)
        return abs(xfs->max_bounds.width);

    for( j = 0, maxwidth = 0, max -= min; j <= max; j++ )
	if( !CI_NONEXISTCHAR(xfs->per_char + j) )
	    if(maxwidth < xfs->per_char[j].attributes)
		maxwidth = xfs->per_char[j].attributes;

    maxwidth *= XFT->pixelsize / 1000.0;
    return maxwidth;
}

/***********************************************************************
 *              XFONT_SetFontMetric
 *
 * INIT ONLY
 *
 * Initializes IFONTINFO16.
 */
static void XFONT_SetFontMetric(fontInfo* fi, const fontResource* fr, XFontStruct* xfs)
{
    unsigned min, max;
    fi->df.dfFirstChar = (BYTE)(min = xfs->min_char_or_byte2);
    fi->df.dfLastChar = (BYTE)(max = xfs->max_char_or_byte2);

    fi->df.dfDefaultChar = (BYTE)xfs->default_char;
    fi->df.dfBreakChar = (BYTE)(( ' ' < min || ' ' > max) ? xfs->default_char: ' ');

    fi->df.dfPixHeight = (INT16)((fi->df.dfAscent = (INT16)xfs->ascent) + xfs->descent);
    fi->df.dfPixWidth = (xfs->per_char) ? 0 : xfs->min_bounds.width;

    XFONT_GetLeading( &fi->df, xfs, &fi->df.dfInternalLeading, &fi->df.dfExternalLeading, NULL );
    fi->df.dfAvgWidth = (INT16)XFONT_GetAvgCharWidth(&fi->df, xfs, NULL );
    fi->df.dfMaxWidth = (INT16)XFONT_GetMaxCharWidth(xfs, NULL);

    if( xfs->min_bounds.width != xfs->max_bounds.width )
        fi->df.dfPitchAndFamily |= TMPF_FIXED_PITCH; /* au contraire! */
    if( fi->fi_flags & FI_SCALABLE )
    {
	fi->df.dfType = DEVICE_FONTTYPE;
        fi->df.dfPitchAndFamily |= TMPF_DEVICE;
    }
    else if( fi->fi_flags & FI_TRUETYPE )
	fi->df.dfType = TRUETYPE_FONTTYPE;
    else
	fi->df.dfType = RASTER_FONTTYPE;

    fi->df.dfFace = fr->lfFaceName;
}

/***********************************************************************
 *              XFONT_GetFontMetric
 *
 * Retrieve font metric info (enumeration).
 */
static UINT XFONT_GetFontMetric( const fontInfo* pfi,
				 LPENUMLOGFONTEXW pLF,
				 NEWTEXTMETRICEXW *pTM )
{
    memset( pLF, 0, sizeof(*pLF) );
    memset( pTM, 0, sizeof(*pTM) );

#define plf ((LPLOGFONTW)pLF)
#define ptm ((LPNEWTEXTMETRICW)pTM)
    plf->lfHeight    = ptm->tmHeight       = pfi->df.dfPixHeight;
    plf->lfWidth     = ptm->tmAveCharWidth = pfi->df.dfAvgWidth;
    plf->lfWeight    = ptm->tmWeight       = pfi->df.dfWeight;
    plf->lfItalic    = ptm->tmItalic       = pfi->df.dfItalic;
    plf->lfUnderline = ptm->tmUnderlined   = pfi->df.dfUnderline;
    plf->lfStrikeOut = ptm->tmStruckOut    = pfi->df.dfStrikeOut;
    plf->lfCharSet   = ptm->tmCharSet      = pfi->df.dfCharSet;

    /* convert pitch values */

    ptm->tmPitchAndFamily = pfi->df.dfPitchAndFamily;
    plf->lfPitchAndFamily = (pfi->df.dfPitchAndFamily & 0xF1) + 1;

    MultiByteToWideChar(CP_ACP, 0, pfi->df.dfFace, -1,
			plf->lfFaceName, LF_FACESIZE);

    /* FIXME: fill in rest of plF values */
    strcpyW(pLF->elfFullName, plf->lfFaceName);
    MultiByteToWideChar(CP_ACP, 0, "Regular", -1,
			pLF->elfStyle, LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, plf->lfCharSet == SYMBOL_CHARSET ?
			"Symbol" : "Roman", -1,
			pLF->elfScript, LF_FACESIZE);

#undef plf

    ptm->tmAscent = pfi->df.dfAscent;
    ptm->tmDescent = ptm->tmHeight - ptm->tmAscent;
    ptm->tmInternalLeading = pfi->df.dfInternalLeading;
    ptm->tmMaxCharWidth = pfi->df.dfMaxWidth;
    ptm->tmDigitizedAspectX = pfi->df.dfHorizRes;
    ptm->tmDigitizedAspectY = pfi->df.dfVertRes;

    ptm->tmFirstChar = pfi->df.dfFirstChar;
    ptm->tmLastChar = pfi->df.dfLastChar;
    ptm->tmDefaultChar = pfi->df.dfDefaultChar;
    ptm->tmBreakChar = pfi->df.dfBreakChar;

    TRACE("Calling Enum proc with FaceName %s FullName %s\n",
	  debugstr_w(pLF->elfLogFont.lfFaceName),
	  debugstr_w(pLF->elfFullName));

   TRACE("CharSet = %d type = %d\n", ptm->tmCharSet, pfi->df.dfType);
    /* return font type */
    return pfi->df.dfType;
#undef ptm
}


/***********************************************************************
 *           XFONT_FixupFlags
 *
 * INIT ONLY
 *
 * dfPitchAndFamily flags for some common typefaces.
 */
static BYTE XFONT_FixupFlags( LPCSTR lfFaceName )
{
   switch( lfFaceName[0] )
   {
        case 'a':
        case 'A': if(!strncasecmp(lfFaceName, "Arial", 5) )
                    return FF_SWISS;
                  break;
        case 'h':
        case 'H': if(!strcasecmp(lfFaceName, "Helvetica") )
                    return FF_SWISS;
                  break;
        case 'c':
        case 'C': if(!strncasecmp(lfFaceName, "Courier", 7))
	            return FF_MODERN;

	          if (!strcasecmp(lfFaceName, "Charter") )
		      return FF_ROMAN;
		  break;
        case 'p':
        case 'P': if( !strcasecmp(lfFaceName,"Palatino") )
                    return FF_ROMAN;
                  break;
        case 't':
        case 'T': if(!strncasecmp(lfFaceName, "Times", 5) )
                    return FF_ROMAN;
                  break;
        case 'u':
        case 'U': if(!strcasecmp(lfFaceName, "Utopia") )
                    return FF_ROMAN;
                  break;
        case 'z':
        case 'Z': if(!strcasecmp(lfFaceName, "Zapf Dingbats") )
                    return FF_DECORATIVE;
   }
   return 0;
}

/***********************************************************************
 *           XFONT_SameFoundryAndFamily
 *
 * INIT ONLY
 */
static BOOL XFONT_SameFoundryAndFamily( const LFD* lfd1, const LFD* lfd2 )
{
    return ( !strcasecmp( lfd1->foundry, lfd2->foundry ) &&
	     !strcasecmp( lfd1->family,  lfd2->family ) );
}

/***********************************************************************
 *           XFONT_InitialCapitals
 *
 * INIT ONLY
 *
 * Upper case first letters of words & remove multiple spaces
 */
static void XFONT_InitialCapitals(LPSTR lpch)
{
    int i;
    BOOL up;
    char* lpstr = lpch;

    for( i = 0, up = TRUE; *lpch; lpch++, i++ )
    {
	if( isspace(*lpch) )
	{
	    if (!up)  /* Not already got one */
	    {
		*lpstr++ = ' ';
		up = TRUE;
	    }
	}
	else if( isalpha(*lpch) && up )
	{
	    *lpstr++ = toupper(*lpch);
	    up = FALSE;
	}
	else
	{
	    *lpstr++ = *lpch;
	    up = FALSE;
	}
    }

    /* Remove possible trailing space */
    if (up && i > 0)
	--lpstr;
    *lpstr = '\0';
}


/***********************************************************************
 *           XFONT_WindowsNames
 *
 * INIT ONLY
 *
 * Build generic Windows aliases for X font names.
 *
 * -misc-fixed- -> "Fixed"
 * -sony-fixed- -> "Sony Fixed", etc...
 */
static void XFONT_WindowsNames(void)
{
    fontResource* fr;

    for( fr = fontList; fr ; fr = fr->next )
    {
	fontResource* pfr;

	if( fr->fr_flags & FR_NAMESET ) continue;     /* skip already assigned */

	for( pfr = fontList; pfr != fr ; pfr = pfr->next )
	    if( pfr->fr_flags & FR_NAMESET )
	    {
		if (!strcasecmp( pfr->resource->family, fr->resource->family))
		    break;
	    }

	snprintf( fr->lfFaceName, sizeof(fr->lfFaceName), "%s %s",
					  /* prepend vendor name */
					  (pfr==fr) ? "" : fr->resource->foundry,
					  fr->resource->family);
	XFONT_InitialCapitals(fr->lfFaceName);
	{
	    BYTE bFamilyStyle = XFONT_FixupFlags( fr->lfFaceName );
	    if( bFamilyStyle)
	    {
		fontInfo* fi;
		for( fi = fr->fi ; fi ; fi = fi->next )
		    fi->df.dfPitchAndFamily |= bFamilyStyle;
	    }
	}

	TRACE("typeface '%s'\n", fr->lfFaceName);

	fr->fr_flags |= FR_NAMESET;
    }
}

/***********************************************************************
 *           XFONT_LoadDefaultLFD
 *
 * Move lfd to the head of fontList to make it more likely to be matched
 */
static void XFONT_LoadDefaultLFD(LFD* lfd, LPCSTR fonttype)
{
    {
	fontResource *fr, *pfr;
	for( fr = NULL, pfr = fontList; pfr; pfr = pfr->next )
	{
	    if( XFONT_SameFoundryAndFamily(pfr->resource, lfd) )
	    {
		if( fr )
		{
		    fr->next = pfr->next;
		    pfr->next = fontList;
		    fontList = pfr;
		}
		break;
	    }
	    fr = pfr;
	}
	if (!pfr)
	    WARN("Default %sfont '-%s-%s-' not available\n", fonttype,
		 lfd->foundry, lfd->family);
    }
}

/***********************************************************************
 *           XFONT_LoadDefault
 */
static void XFONT_LoadDefault( HKEY hkey, LPCSTR ini, LPCSTR fonttype)
{
    char buffer[MAX_LFD_LENGTH];
    DWORD type, count = sizeof(buffer);

    buffer[0] = 0;
    RegQueryValueExA(hkey, ini, 0, &type, (LPBYTE)buffer, &count);

    if (*buffer)
    {
        LFD lfd;
        char* pch = buffer;
        while( *pch && isspace(*pch) ) pch++;

        TRACE("Using '%s' as default %sfont\n", pch, fonttype);
        if (LFD_Parse(pch, &lfd) && lfd.foundry && lfd.family)
            XFONT_LoadDefaultLFD(&lfd, fonttype);
        else
            WARN("Ini section [%s]%s is malformed\n", INIFontSection, ini);
    }
}

/***********************************************************************
 *           XFONT_LoadDefaults
 */
static void XFONT_LoadDefaults( HKEY hkey )
{
    XFONT_LoadDefault( hkey, INIDefaultFixed, "fixed ");
    XFONT_LoadDefault( hkey, INIDefault, "");
}

/***********************************************************************
 *           XFONT_CreateAlias
 */
static fontAlias* XFONT_CreateAlias( LPCSTR lpTypeFace, LPCSTR lpAlias )
{
    int j;
    fontAlias *pfa, *prev = NULL;

    for(pfa = aliasTable; pfa; pfa = pfa->next)
    {
	/* check if we already got one */
	if( !strcasecmp( pfa->faTypeFace, lpAlias ) )
	{
	    TRACE("redundant alias '%s' -> '%s'\n",
		  lpAlias, lpTypeFace );
	    return NULL;
	}
	prev = pfa;
    }

    j = strlen(lpTypeFace) + 1;
    pfa = HeapAlloc( GetProcessHeap(), 0, sizeof(fontAlias) +
			       j + strlen(lpAlias) + 1 );
    if (pfa)
    {
        if (!prev)
	    aliasTable = pfa;
	else
	    prev->next = pfa;

	pfa->next = NULL;
	pfa->faTypeFace = (LPSTR)(pfa + 1);
	strcpy( pfa->faTypeFace, lpTypeFace );
	pfa->faAlias = pfa->faTypeFace + j;
	strcpy( pfa->faAlias, lpAlias );

        TRACE("added alias '%s' for '%s'\n", lpAlias, lpTypeFace );

	return pfa;
    }
    return NULL;
}


/***********************************************************************
 *           XFONT_LoadAlias
 */
static void XFONT_LoadAlias(const LFD* lfd, LPCSTR lpAlias, BOOL bSubst)
{
    fontResource *fr, *frMatch = NULL;
    if (!lfd->foundry || ! lfd->family)
    {
	WARN("Malformed font resource for alias '%s'\n", lpAlias);
	return;
    }
    for (fr = fontList; fr ; fr = fr->next)
    {
        if(!strcasecmp(fr->resource->family, lpAlias))
	{
	    /* alias is not needed since the real font is present */
	    TRACE("Ignoring font alias '%s' as it is already available as a real font\n", lpAlias);
	    return;
	}
	if( XFONT_SameFoundryAndFamily( fr->resource, lfd ) )
	{
	    frMatch = fr;
	    break;
	}
    }

    if( frMatch )
    {
        if( bSubst )
	{
	    fontAlias *pfa, *prev = NULL;

	    for(pfa = aliasTable; pfa; pfa = pfa->next)
	    {
	        /* Remove lpAlias from aliasTable - we should free the old entry */
	        if(!strcmp(lpAlias, pfa->faAlias))
		{
		    if(prev)
		        prev->next = pfa->next;
		    else
		        aliasTable = pfa->next;
		}

		/* Update any references to the substituted font in aliasTable */
		if(!strcmp(frMatch->lfFaceName, pfa->faTypeFace))
                {
                    pfa->faTypeFace = HeapAlloc( GetProcessHeap(), 0, strlen(lpAlias)+1 );
                    strcpy( pfa->faTypeFace, lpAlias );
                }
		prev = pfa;
	    }

	    TRACE("\tsubstituted '%s' with '%s'\n", frMatch->lfFaceName, lpAlias );

	    lstrcpynA( frMatch->lfFaceName, lpAlias, LF_FACESIZE );
	    frMatch->fr_flags |= FR_NAMESET;
	}
	else
	{
	    /* create new entry in the alias table */
	    XFONT_CreateAlias( frMatch->lfFaceName, lpAlias );
	}
    }
    else
    {
	WARN("Font alias '-%s-%s-' is not available\n", lfd->foundry, lfd->family);
    }
}

/***********************************************************************
 *  Just a copy of PROFILE_GetStringItem
 *
 *  Convenience function that turns a string 'xxx, yyy, zzz' into
 *  the 'xxx\0 yyy, zzz' and returns a pointer to the 'yyy, zzz'.
 */
static char *XFONT_GetStringItem( char *start )
{
#define XFONT_isspace(c) (isspace(c) || (c == '\r') || (c == 0x1a))
    char *lpchX, *lpch;

    for (lpchX = start, lpch = NULL; *lpchX != '\0'; lpchX++ )
    {
        if( *lpchX == ',' )
        {
            if( lpch ) *lpch = '\0'; else *lpchX = '\0';
            while( *(++lpchX) )
                if( !XFONT_isspace(*lpchX) ) return lpchX;
        }
	else if( XFONT_isspace( *lpchX ) && !lpch ) lpch = lpchX;
	     else lpch = NULL;
    }
    if( lpch ) *lpch = '\0';
    return NULL;
#undef XFONT_isspace
}

/***********************************************************************
 *           XFONT_LoadAliases
 *
 * INIT ONLY
 *
 * Create font aliases for some standard windows fonts using user's
 * default choice of (sans-)serif fonts
 *
 * Read user-defined aliases from config file. Format is as follows
 *
 * Alias# = [Windows font name],[LFD font name], <substitute original name>
 *
 * Example:
 *   Alias0 = Arial, -adobe-helvetica-
 *   Alias1 = Times New Roman, -bitstream-courier-, 1
 *   ...
 *
 * Note that from 970817 and on we have built-in alias templates that take
 * care of the necessary Windows typefaces.
 */
typedef struct
{
  LPSTR                 fatResource;
  LPSTR                 fatAlias;
} aliasTemplate;

static void XFONT_LoadAliases( HKEY hkey )
{
    char *lpResource;
    char buffer[MAX_LFD_LENGTH];
    int i = 0;
    LFD lfd;

    /* built-ins first */
    strcpy(buffer, "-bitstream-charter-");
    if (hkey)
    {
	DWORD type, count = sizeof(buffer);
	RegQueryValueExA(hkey, INIDefaultSerif, 0, &type, (LPBYTE)buffer, &count);
    }
    TRACE("Using '%s' as default serif font\n", buffer);
    if (LFD_Parse(buffer, &lfd))
    {
        /* NB XFONT_InitialCapitals should not change these standard aliases */
        XFONT_LoadAlias( &lfd, "Tms Roman", FALSE);
        XFONT_LoadAlias( &lfd, "MS Serif", FALSE);
        XFONT_LoadAlias( &lfd, "Times New Roman", FALSE);

        XFONT_LoadDefaultLFD( &lfd, "serif ");
    }

    strcpy(buffer, "-adobe-helvetica-");
    if (hkey)
    {
	DWORD type, count = sizeof(buffer);
	RegQueryValueExA(hkey, INIDefaultSansSerif, 0, &type, (LPBYTE)buffer, &count);
    }
    TRACE("Using '%s' as default sans serif font\n", buffer);
    if (LFD_Parse(buffer, &lfd))
    {
        XFONT_LoadAlias( &lfd, "Helv", FALSE);
        XFONT_LoadAlias( &lfd, "MS Sans Serif", FALSE);
        XFONT_LoadAlias( &lfd, "MS Shell Dlg", FALSE);
        XFONT_LoadAlias( &lfd, "System", FALSE);
        XFONT_LoadAlias( &lfd, "Arial", FALSE);

        XFONT_LoadDefaultLFD( &lfd, "sans serif ");
    }

    /* then user specified aliases */
    do
    {
        BOOL bSubst;
	char subsection[32];
        snprintf( subsection, sizeof(subsection), "%s%i", INIAliasSection, i++ );

	buffer[0] = 0;
        if (hkey)
	{
	    DWORD type, count = sizeof(buffer);
	    RegQueryValueExA(hkey, subsection, 0, &type, (LPBYTE)buffer, &count);
	}

	if (!buffer[0])
	    break;

	XFONT_InitialCapitals(buffer);
	lpResource = XFONT_GetStringItem( buffer );
	bSubst = (XFONT_GetStringItem( lpResource )) ? TRUE : FALSE;
	if( lpResource && *lpResource )
	{
            if (LFD_Parse(lpResource, &lfd)) XFONT_LoadAlias(&lfd, buffer, bSubst);
	}
	else
	    WARN("malformed font alias '%s'\n", buffer );
    }
    while(TRUE);
}

/***********************************************************************
 *           XFONT_UnAlias
 *
 * Convert an (potential) alias into a real windows name
 *
 */
static LPCSTR XFONT_UnAlias(char* font)
{
    if (font[0])
    {
	fontAlias* fa;
	XFONT_InitialCapitals(font); /* to remove extra white space */

	for( fa = aliasTable; fa; fa = fa->next )
	    /* use case insensitive matching to handle, e.g., "MS Sans Serif" */
	    if( !strcasecmp( fa->faAlias, font ) )
	    {
		TRACE("found alias '%s'->%s'\n", font, fa->faTypeFace );
		strcpy(font, fa->faTypeFace);
		return fa->faAlias;
	    }
    }
    return NULL;
}

/***********************************************************************
 *           XFONT_RemoveFontResource
 *
 * Caller should check if the font resource is in use. If it is it should
 * set FR_REMOVED flag to delay removal until the resource is not in use
 * any more.
 */
static void XFONT_RemoveFontResource( fontResource** ppfr )
{
    fontResource* pfr = *ppfr;
#if 0
    fontInfo* pfi;

    /* FIXME - if fonts were read from a cache, these HeapFrees will fail */
    while( pfr->fi )
    {
	pfi = pfr->fi->next;
	HeapFree( GetProcessHeap(), 0, pfr->fi );
	pfr->fi = pfi;
    }
    HeapFree( GetProcessHeap(), 0, pfr );
#endif
    *ppfr = pfr->next;
}

/***********************************************************************
 *           XFONT_LoadIgnores
 *
 * INIT ONLY
 *
 * Removes specified fonts from the font table to prevent Wine from
 * using it.
 *
 * Ignore# = [LFD font name]
 *
 * Example:
 *   Ignore0 = -misc-nil-
 *   Ignore1 = -sun-open look glyph-
 *   ...
 *
 */
static void XFONT_LoadIgnore(char* lfdname)
{
    fontResource** ppfr;
    LFD lfd;

    if (LFD_Parse(lfdname, &lfd) && lfd.foundry && lfd.family)
    {
	for( ppfr = &fontList; *ppfr ; ppfr = &((*ppfr)->next))
	{
	    if( XFONT_SameFoundryAndFamily( (*ppfr)->resource, &lfd) )
	    {
		TRACE("Ignoring '-%s-%s-'\n",
		      (*ppfr)->resource->foundry, (*ppfr)->resource->family  );

		XFONT_RemoveFontResource( ppfr );
		break;
	    }
	}
    }
    else
	WARN("Malformed font resource\n");
}

static void XFONT_LoadIgnores( HKEY hkey )
{
    int i = 0;
    char  subsection[32];
    char buffer[MAX_LFD_LENGTH];

    /* Standard one that no one wants */
    strcpy(buffer, "-misc-nil-");
    XFONT_LoadIgnore(buffer);

    /* Others from INI file */
    if (!hkey) return;
    do
    {
	DWORD type, count = sizeof(buffer);
	sprintf( subsection, "%s%i", INIIgnoreSection, i++ );

	if (!RegQueryValueExA(hkey, subsection, 0, &type, (LPBYTE)buffer, &count))
	{
	    char* pch = buffer;
	    while( *pch && isspace(*pch) ) pch++;
	    XFONT_LoadIgnore(pch);
	}
	else
	    break;
    } while(TRUE);
}


/***********************************************************************
 *           XFONT_UserMetricsCache
 *
 * Returns expanded name for the cachedmetrics file.
 * Now it also appends the current value of the $DISPLAY variable.
 */
static char* XFONT_UserMetricsCache( char* buffer, int* buf_size )
{
    const char *confdir = wine_get_config_dir();
    const char *display_name = XDisplayName(NULL);
    int len = strlen(confdir) + strlen(INIFontMetrics) + strlen(display_name) + 8;
    int display = 0;
    int screen = 0;
    char *p, *ext;

    /*
    **  Normalize the display name, since on Red Hat systems, DISPLAY
    **      is commonly set to one of either 'unix:0.0' or ':0' or ':0.0'.
    **      after this code, all of the above will resolve to ':0.0'.
    */
    if (!strncmp( display_name, "unix:", 5 )) display_name += 4;
    p = strchr(display_name, ':');
    if (p) sscanf(p + 1, "%d.%d", &display, &screen);

    if ((len > *buf_size) &&
        !(buffer = HeapReAlloc( GetProcessHeap(), 0, buffer, *buf_size = len )))
    {
        ERR("out of memory\n");
        ExitProcess(1);
    }
    sprintf( buffer, "%s/%s", confdir, INIFontMetrics );

    ext = buffer + strlen(buffer);
    strcpy( ext, display_name );

    if (!(p = strchr( ext, ':' ))) p = ext + strlen(ext);
    sprintf( p, ":%d.%d", display, screen );
    return buffer;
}


/***********************************************************************
 *           X Font Matching
 *
 * Compare two fonts (only parameters set by the XFONT_InitFontInfo()).
 */
static INT XFONT_IsSubset(const fontInfo* match, const fontInfo* fi)
{
  INT           m;

  /* 0 - keep both, 1 - keep match, -1 - keep fi */

  /* Compare dfItalic, Underline, Strikeout, Weight, Charset */
  m = (const BYTE*)&fi->df.dfPixWidth - (const BYTE*)&fi->df.dfItalic;
  if( memcmp(&match->df.dfItalic, &fi->df.dfItalic, m )) return 0;

  if( (!((fi->fi_flags & FI_SCALABLE) + (match->fi_flags & FI_SCALABLE))
       && fi->lfd_height != match->lfd_height) ||
      (!((fi->fi_flags & FI_POLYWEIGHT) + (match->fi_flags & FI_POLYWEIGHT))
       && fi->df.dfWeight != match->df.dfWeight) ) return 0;

  m = (int)(match->fi_flags & (FI_POLYWEIGHT | FI_SCALABLE)) -
      (int)(fi->fi_flags & (FI_SCALABLE | FI_POLYWEIGHT));

  if( m == (FI_POLYWEIGHT - FI_SCALABLE) ||
      m == (FI_SCALABLE - FI_POLYWEIGHT) ) return 0;	/* keep both */
  else if( m >= 0 ) return 1;	/* 'match' is better */

  return -1;			/* 'fi' is better */
}

/***********************************************************************
 *            XFONT_CheckFIList
 *
 * REMOVE_SUBSETS - attach new fi and purge subsets
 * UNMARK_SUBSETS - remove subset flags from all fi entries
 */
static void XFONT_CheckFIList( fontResource* fr, fontInfo* fi, int action)
{
  int		i = 0;
  fontInfo*     pfi, *prev;

  for( prev = NULL, pfi = fr->fi; pfi; )
  {
    if( action == REMOVE_SUBSETS )
    {
        if( pfi->fi_flags & FI_SUBSET )
        {
	    fontInfo* subset = pfi;

	    i++;
	    fr->fi_count--;
            if( prev ) prev->next = pfi = pfi->next;
            else fr->fi = pfi = pfi->next;
	    HeapFree( GetProcessHeap(), 0, subset );
	    continue;
        }
    }
    else pfi->fi_flags &= ~FI_SUBSET;

    prev = pfi;
    pfi = pfi->next;
  }

  if( action == REMOVE_SUBSETS )	/* also add the superset */
  {
    if( fi->fi_flags & FI_SCALABLE )
    {
        fi->next = fr->fi;
        fr->fi = fi;
    }
    else if( prev ) prev->next = fi; else fr->fi = fi;
    fr->fi_count++;
  }

  if( i ) TRACE("\t    purged %i subsets [%i]\n", i , fr->fi_count);
}

/***********************************************************************
 *            XFONT_FindFIList
 */
static fontResource* XFONT_FindFIList( fontResource* pfr, const char* pTypeFace )
{
  while( pfr )
  {
    if( !strcasecmp( pfr->lfFaceName, pTypeFace ) ) break;
    pfr = pfr->next;
  }
  /* Give the app back the font name it asked for. Encarta checks this. */
  if (pfr) strcpy(pfr->lfFaceName,pTypeFace);
  return pfr;
}

/***********************************************************************
 *           XFONT_FixupPointSize
 */
static void XFONT_FixupPointSize(fontInfo* fi)
{
#define df (fi->df)
    df.dfHorizRes = df.dfVertRes = fi->lfd_resolution;
    df.dfPoints = (INT16)
	(((INT)(df.dfPixHeight - df.dfInternalLeading) * 72 + (df.dfVertRes >> 1)) /
	 df.dfVertRes );
#undef df
}

/***********************************************************************
 *           XFONT_BuildMetrics
 *
 * Build font metrics from X font
 */
static int XLoadQueryFont_ErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

/* XLoadQueryFont has the bad habit of crashing when loading a bad font... */
static XFontStruct *safe_XLoadQueryFont(Display *display, char *name)
{
    XFontStruct *ret;

    X11DRV_expect_error(display, XLoadQueryFont_ErrorHandler, NULL);
    ret = XLoadQueryFont(display, name);
    if (X11DRV_check_error()) ret = NULL;
    return ret;
}


static int XFONT_BuildMetrics(char** x_pattern, int res, unsigned x_checksum, int x_count)
{
    int		  i;
    fontInfo*	  fi = NULL;
    fontResource* fr, *pfr;
    int           n_ff = 0;

    MESSAGE("Building font metrics. This may take some time...\n");
    for( i = 0; i < x_count; i++ )
    {
	char*         typeface;
	LFD           lfd;
	int           j;
	char          buffer[MAX_LFD_LENGTH];
	char*	      lpstr;
	XFontStruct*  x_fs;
	fontInfo*    pfi;

	if (!(typeface = HeapAlloc(GetProcessHeap(), 0, strlen(x_pattern[i])+1))) break;
	strcpy( typeface, x_pattern[i] );
	if (i % 10 == 0) MESSAGE("Font metrics: %.1f%% done\n", 100.0 * i / x_count);

        if (!LFD_Parse(typeface, &lfd))
	{
	    HeapFree(GetProcessHeap(), 0, typeface);
	    continue;
	}

	/* find a family to insert into */

	for( pfr = NULL, fr = fontList; fr; fr = fr->next )
	{
	    if( XFONT_SameFoundryAndFamily(fr->resource, &lfd))
		break;
	    pfr = fr;
	}

	if( !fi ) fi = HeapAlloc(GetProcessHeap(), 0, sizeof(fontInfo));

	if( !LFD_InitFontInfo( fi, &lfd, x_pattern[i]) )
	    goto nextfont;

	if( !fr ) /* add new family */
	{
	    n_ff++;
	    fr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(fontResource));
	    if (fr)
	    {
		fr->resource = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LFD));

		TRACE("family: -%s-%s-\n", lfd.foundry, lfd.family );
		fr->resource->foundry = HeapAlloc(GetProcessHeap(), 0, strlen(lfd.foundry)+1);
		strcpy( (char *)fr->resource->foundry, lfd.foundry );
		fr->resource->family = HeapAlloc(GetProcessHeap(), 0, strlen(lfd.family)+1);
		strcpy( (char *)fr->resource->family, lfd.family );
		fr->resource->weight = "";

		if( pfr ) pfr->next = fr;
		else fontList = fr;
	    }
	    else
		WARN("Not enough memory for a new font family\n");
	}

	/* check if we already have something better than "fi" */

	for( pfi = fr->fi, j = 0; pfi && j <= 0; pfi = pfi->next )
	    if( (j = XFONT_IsSubset( pfi, fi )) < 0 )
		pfi->fi_flags |= FI_SUBSET; /* superseded by "fi" */
	if( j > 0 ) goto nextfont;

	/* add new font instance "fi" to the "fr" font resource */

	if( fi->fi_flags & FI_SCALABLE )
	{
	    LFD lfd1;
	    char pxl_string[4], res_string[4];
	    /* set scalable font height to get a basis for extrapolation */

	    fi->lfd_height = DEF_SCALABLE_HEIGHT;
	    fi->lfd_resolution = res;

	    sprintf(pxl_string, "%d", fi->lfd_height);
	    sprintf(res_string, "%d", fi->lfd_resolution);

	    lfd1 = lfd;
	    lfd1.pixel_size = pxl_string;
	    lfd1.point_size = "*";
	    lfd1.resolution_x = res_string;
	    lfd1.resolution_y = res_string;

	    LFD_UnParse(buffer, sizeof(buffer), &lfd1);

	    lpstr = buffer;
	}
	else lpstr = x_pattern[i];

	/* X11 may return an error on some bad fonts... So be prepared to handle these. */
	if ((x_fs = safe_XLoadQueryFont(gdi_display, lpstr)) != 0)
	{
	    XFONT_SetFontMetric( fi, fr, x_fs );
            wine_tsx11_lock();
            XFreeFont( gdi_display, x_fs );
            wine_tsx11_unlock();

	    XFONT_FixupPointSize(fi);

	    TRACE("\t[% 2ipt] '%s'\n", fi->df.dfPoints, x_pattern[i] );

	    XFONT_CheckFIList( fr, fi, REMOVE_SUBSETS );
	    fi = NULL;	/* preventing reuse */
	}
	else
	{
	    ERR("failed to load %s\n", lpstr );

	    XFONT_CheckFIList( fr, fi, UNMARK_SUBSETS );
	}
    nextfont:
	HeapFree(GetProcessHeap(), 0, typeface);
    }
    HeapFree(GetProcessHeap(), 0, fi);

    /* Scan through the font list and remove FontResource(s) (fr)
     * that have no associated Fontinfo(s) (fi).
     * This code is necessary because XFONT_ReadCachedMetrics
     * assumes that there is at least one fi associated with a fr.
     * This assumption is invalid for TT font
     *  -altsys-ms outlook-medium-r-normal--0-0-0-0-p-0-microsoft-symbol.
     */

    fr = fontList;

    while (!fr->fi_count)
    {
       fontList = fr->next;

       HeapFree(GetProcessHeap(), 0, fr->resource);
       HeapFree(GetProcessHeap(), 0, fr);

       fr = fontList;
       n_ff--;
    }

    fr = fontList;

    while (fr->next)
    {
      	if (!fr->next->fi_count)
	{
	    pfr = fr->next;
	    fr->next = fr->next->next;

	    HeapFree(GetProcessHeap(), 0, pfr->resource);
	    HeapFree(GetProcessHeap(), 0, pfr);

	    n_ff--;
	}
	else
	    fr = fr->next;
    }

    MESSAGE("Font metrics: 100.0%% done\n");
    return n_ff;
}

/***********************************************************************
 *           XFONT_ReadCachedMetrics
 *
 * INIT ONLY
 */
static BOOL XFONT_ReadCachedMetrics( int fd, int res, unsigned x_checksum, int x_count )
{
    if( fd >= 0 )
    {
	unsigned u;
	int i, j;

	/* read checksums */
	read( fd, &u, sizeof(unsigned) );
	read( fd, &i, sizeof(int) );

	if( u == x_checksum && i == x_count )
	{
	    off_t length, offset = 3 * sizeof(int);

            /* read total size */
	    read( fd, &i, sizeof(int) );
	    length = lseek( fd, 0, SEEK_END );

	    if( length == (i + offset) )
	    {
		lseek( fd, offset, SEEK_SET );
		fontList = HeapAlloc( GetProcessHeap(), 0, i);
		if( fontList )
		{
		    fontResource* 	pfr = fontList;
		    fontInfo* 		pfi = NULL;

		    TRACE("Reading cached font metrics:\n");

		    read( fd, fontList, i); /* read all metrics at once */
		    while( offset < length )
		    {
			offset += sizeof(fontResource) + sizeof(fontInfo);
			pfr->fi = pfi = (fontInfo*)(pfr + 1);
			j = 1;
			while( TRUE )
			{
			   if( offset > length ||
			       pfi->cptable >= (UINT16)X11DRV_CPTABLE_COUNT ||
			       PtrToInt(pfi->next) != j++ )
			   {
			       TRACE("error: offset=%ld length=%ld cptable=%d pfi->next=%p j=%d\n",
                                      (long)offset, (long)length, pfi->cptable, pfi->next, j-1);
			       goto fail;
			   }

			   if( pfi->df.dfPixHeight == 0 )
			   {
			       TRACE("error: dfPixHeight==0\n");
			       goto fail;
			   }

			   pfi->df.dfFace = pfr->lfFaceName;
			   if( pfi->fi_flags & FI_SCALABLE )
			   {
			       /* we can pretend we got this font for any resolution */
			       pfi->lfd_resolution = res;
			       XFONT_FixupPointSize(pfi);
			   }
			   pfi->next = pfi + 1;

			   if( j > pfr->fi_count ) break;

			   pfi = pfi->next;
			   offset += sizeof(fontInfo);
			}
			pfi->next = NULL;
			if( pfr->next )
			{
			    pfr->next = (fontResource*)(pfi + 1);
			    pfr = pfr->next;
			}
			else break;
		    }
		    if( pfr->next == NULL &&
			*(int*)(pfi + 1) == X_FMC_MAGIC )
		    {
			/* read LFD stubs */
			char* lpch = (char*)((int*)(pfi + 1) + 1);
			offset += sizeof(int);
			for( pfr = fontList; pfr; pfr = pfr->next )
			{
			    size_t len = strlen(lpch) + 1;
			    TRACE("\t%s, %i instances\n", lpch, pfr->fi_count );
			    pfr->resource = HeapAlloc(GetProcessHeap(),0,sizeof(LFD));
                            if (!LFD_Parse(lpch, pfr->resource))
                            {
                                HeapFree( GetProcessHeap(), 0, pfr->resource );
                                pfr->resource = NULL;
                            }
			    lpch += len;
			    offset += len;
			    if (offset > length)
			    {
			        TRACE("error: offset=%ld length=%ld\n",(long)offset,(long)length);
			        goto fail;
			    }
			}
			close( fd );
			return TRUE;
		    }
		}
	    } else {
	        TRACE("Wrong length: %ld!=%ld\n",(long)length,(long)(i+offset));
	    }
	} else {
	    TRACE("Checksum (%x vs. %x) or count (%d vs. %d) mismatch\n",
	          u,x_checksum,i,x_count);
	}
fail:
        HeapFree( GetProcessHeap(), 0, fontList );
	fontList = NULL;
	close( fd );
    }
    return FALSE;
}

/***********************************************************************
 *           XFONT_WriteCachedMetrics
 *
 * INIT ONLY
 */
static BOOL XFONT_WriteCachedMetrics( int fd, unsigned x_checksum, int x_count, int n_ff )
{
    fontResource* pfr;
    fontInfo* pfi;

    if( fd >= 0 )
    {
        int  i, j, k;
	char buffer[MAX_LFD_LENGTH];

        /* font metrics file:
         *
         * +0000 x_checksum
         * +0004 x_count
         * +0008 total size to load
         * +000C prepackaged font metrics
	 * ...
	 * +...x 	X_FMC_MAGIC
	 * +...x + 4 	LFD stubs
         */

	write( fd, &x_checksum, sizeof(unsigned) );
	write( fd, &x_count, sizeof(int) );

	for( j = i = 0, pfr = fontList; pfr; pfr = pfr->next )
	{
	    LFD_UnParse(buffer, sizeof(buffer), pfr->resource);
	    i += strlen( buffer) + 1;
	    j += pfr->fi_count;
	}
        i += n_ff * sizeof(fontResource) + j * sizeof(fontInfo) + sizeof(int);
	write( fd, &i, sizeof(int) );

	TRACE("Writing font cache:\n");

	for( pfr = fontList; pfr; pfr = pfr->next )
	{
	    fontInfo fi;

	    TRACE("\t-%s-%s-, %i instances\n", pfr->resource->foundry, pfr->resource->family, pfr->fi_count );

	    i = write( fd, pfr, sizeof(fontResource) );
	    if( i == sizeof(fontResource) )
	    {
		for( k = 1, pfi = pfr->fi; pfi; pfi = pfi->next )
		{
		    fi = *pfi;

		    fi.df.dfFace = NULL;
		    fi.next = IntToPtr(k);	/* loader checks this */

		    j = write( fd, &fi, sizeof(fi) );
		    k++;
		}
	        if( j == sizeof(fontInfo) ) continue;
	    }
	    break;
	}
        if( i == sizeof(fontResource) && j == sizeof(fontInfo) )
	{
	    i = j = X_FMC_MAGIC;
	    write( fd, &i, sizeof(int) );
	    for( pfr = fontList; pfr && i == j; pfr = pfr->next )
	    {
		LFD_UnParse(buffer, sizeof(buffer), pfr->resource);
	        i = strlen( buffer ) + 1;
		j = write( fd, buffer, i );
	    }
	}
	close( fd );
	return ( i == j );
    }
    return FALSE;
}

/***********************************************************************
 *           XFONT_GetDefResolution()
 *
 * INIT ONLY
 *
 * Here we initialize DefResolution which is used in the
 * XFONT_Match() penalty function, based on the values of log_pixels
 */
static int XFONT_GetDefResolution( int log_pixels_x, int log_pixels_y )
{
    int i, j, num = 3;
    int allowed_xfont_resolutions[3] = { 72, 75, 100 };
    int best = 0, best_diff = 65536;

    /* FIXME We can only really guess at a best DefResolution
     * - this should be configurable
     */
    for( i = best = 0; i < num; i++ )
    {
	j = abs( log_pixels_x - allowed_xfont_resolutions[i] );
	if( j < best_diff )
	{
	    best = i;
	    best_diff = j;
	}
    }
    DefResolution = allowed_xfont_resolutions[best];
    return DefResolution;
}


/***********************************************************************
 *            XFONT_Match
 *
 * Compute the matching score between the logical font and the device font.
 *
 * contributions from highest to lowest:
 *      charset
 *      fixed pitch
 *      height
 *      family flags (only when the facename is not present)
 *      width
 *      weight, italics, underlines, strikeouts
 *
 * NOTE: you can experiment with different penalty weights to see what happens.
 */
static UINT XFONT_Match( fontMatch* pfm )
{
   fontInfo*    pfi = pfm->pfi;         /* device font to match */
   LPLOGFONT16  plf = pfm->plf;         /* wanted logical font */
   UINT       penalty = 0;
   BOOL       bR6 = pfm->flags & FO_MATCH_XYINDEP;    /* from text_caps */
   BOOL       bScale = pfi->fi_flags & FI_SCALABLE;
   int d = 0, height;

   TRACE("\t[ %-2ipt h=%-3i w=%-3i %s%s]\n", pfi->df.dfPoints,
		 pfi->df.dfPixHeight, pfi->df.dfAvgWidth,
		(pfi->df.dfWeight > FW_NORMAL) ? "Bold " : "Normal ",
		(pfi->df.dfItalic) ? "Italic" : "" );

   pfm->flags &= FO_MATCH_MASK;

/* Charset */
   /* pfm->internal_charset: given(required) charset */
   /* pfi->internal_charset: charset of this font */
   if (pfi->internal_charset == DEFAULT_CHARSET)
   {
      /* special case(unicode font) */
      /* priority: unmatched charset < unicode < matched charset */
      penalty += 0x50;
   }
   else
   {
     if( pfm->internal_charset == DEFAULT_CHARSET )
     {
	/*
         if (pfi->internal_charset != ANSI_CHARSET)
	    penalty += 0x200;
	*/
	if ( pfi->codepage != GetACP() )
	    penalty += 0x200;
     }
     else if (pfm->internal_charset != pfi->internal_charset)
     {
       if ( pfi->internal_charset & 0xff00 )
         penalty += 0x1000; /* internal charset - should not be used */
       else
         penalty += 0x200;
     }
   }

/* Height */
   height = -1;
   {
       if( plf->lfHeight > 0 )
       {
	   int h = pfi->df.dfPixHeight;
	   d = h - plf->lfHeight;
	   height = plf->lfHeight;
       }
       else
       {
	   int h = pfi->df.dfPixHeight - pfi->df.dfInternalLeading;
	   if (h)
	   {
	       d = h + plf->lfHeight;
	       height = (-plf->lfHeight * pfi->df.dfPixHeight) / h;
	   }
	   else
	   {
	       ERR("PixHeight == InternalLeading\n");
	       penalty += 0x1000; /* don't want this */
	   }
       }
   }

   if( height == 0 )
       pfm->height = 1; /* Very small */
   else if( d )
   {
       if( bScale )
	   pfm->height = height;
       else if( (plf->lfQuality != PROOF_QUALITY) && bR6 )
       {
	   if( d > 0 ) 	/* do not shrink raster fonts */
	   {
	       pfm->height = pfi->df.dfPixHeight;
	       penalty += (pfi->df.dfPixHeight - height) * 0x4;
	   }
	   else 	/* expand only in integer multiples */
	   {
	       pfm->height = height - height%pfi->df.dfPixHeight;
	       penalty += (height - pfm->height + 1) * height / pfi->df.dfPixHeight;
	   }
       }
       else /* can't be scaled at all */
       {
	   if( plf->lfQuality != PROOF_QUALITY) pfm->flags |= FO_SYNTH_HEIGHT;
	   pfm->height = pfi->df.dfPixHeight;
	   penalty += (d > 0)? d * 0x8 : -d * 0x10;
       }
   }
   else
       pfm->height = pfi->df.dfPixHeight;

/* Pitch and Family */
   if( pfm->flags & FO_MATCH_PAF ) {
       int family = plf->lfPitchAndFamily & FF_FAMILY;

       /* TMPF_FIXED_PITCH means exactly the opposite */
       if( plf->lfPitchAndFamily & FIXED_PITCH ) {
	   if( pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH ) penalty += 0x100;
       } else /* Variable is the default */
	   if( !(pfi->df.dfPitchAndFamily & TMPF_FIXED_PITCH) ) penalty += 0x2;

       if (family != FF_DONTCARE && family != (pfi->df.dfPitchAndFamily & FF_FAMILY) )
	   penalty += 0x10;
   }

/* Width */
   if( plf->lfWidth )
   {
       int h;
       if( bR6 || bScale ) h = 0;
       else
       {
	   /* FIXME: not complete */

	   pfm->flags |= FO_SYNTH_WIDTH;
	   h = abs(plf->lfWidth - (pfm->height * pfi->df.dfAvgWidth)/pfi->df.dfPixHeight);
       }
       penalty += h * ( d ) ? 0x2 : 0x1 ;
   }
   else if( !(pfi->fi_flags & FI_NORMAL) ) penalty++;

/* Weight */
   if( plf->lfWeight != FW_DONTCARE )
   {
       penalty += abs(plf->lfWeight - pfi->df.dfWeight) / 40;
       if( plf->lfWeight > pfi->df.dfWeight ) pfm->flags |= FO_SYNTH_BOLD;
   } else if( pfi->df.dfWeight >= FW_BOLD ) penalty++;	/* choose normal by default */

/* Italic */
   if( plf->lfItalic != pfi->df.dfItalic )
   {
       penalty += 0x4;
       pfm->flags |= FO_SYNTH_ITALIC;
   }
/* Underline */
   if( plf->lfUnderline ) pfm->flags |= FO_SYNTH_UNDERLINE;

/* Strikeout */
   if( plf->lfStrikeOut ) pfm->flags |= FO_SYNTH_STRIKEOUT;


   if( penalty && !bScale && pfi->lfd_resolution != DefResolution )
       penalty++;

   TRACE("  returning %i\n", penalty );

   return penalty;
}

/***********************************************************************
 *            XFONT_MatchFIList
 *
 * Scan a particular font resource for the best match.
 */
static UINT XFONT_MatchFIList( fontMatch* pfm )
{
  BOOL        skipRaster = (pfm->flags & FO_MATCH_NORASTER);
  UINT        current_score, score = (UINT)(-1);
  fontMatch   fm = *pfm;

  for( fm.pfi = pfm->pfr->fi; fm.pfi && score; fm.pfi = fm.pfi->next)
  {
     if( skipRaster && !(fm.pfi->fi_flags & FI_SCALABLE) )
         continue;

     current_score = XFONT_Match( &fm );
     if( score > current_score )
     {
	*pfm = fm;
        score = current_score;
     }
  }
  return score;
}

/***********************************************************************
 *	XFONT_is_ansi_charset
 *
 *  returns TRUE if the given charset is ANSI_CHARSET.
 */
static BOOL XFONT_is_ansi_charset( UINT charset )
{
	return ((charset == ANSI_CHARSET) ||
		(charset == DEFAULT_CHARSET && GetACP() == 1252));
}

/***********************************************************************
 *          XFONT_MatchDeviceFont
 *
 * Scan font resource tree.
 *
 */
static void XFONT_MatchDeviceFont( fontResource* start, fontMatch* pfm)
{
    fontMatch           fm = *pfm;
    UINT          current_score, score = (UINT)(-1);
    fontResource**	ppfr;

    TRACE("(%u) '%s' h=%i weight=%i %s\n",
	  pfm->plf->lfCharSet, pfm->plf->lfFaceName, pfm->plf->lfHeight,
	  pfm->plf->lfWeight, (pfm->plf->lfItalic) ? "Italic" : "" );

    pfm->pfi = NULL;

    /* the following hard-coded font binding assumes 'ANSI_CHARSET'. */
    if( !fm.plf->lfFaceName[0] &&
	XFONT_is_ansi_charset(fm.plf->lfCharSet) )
    {
        switch(fm.plf->lfPitchAndFamily & 0xf0)
	{
	case FF_MODERN:
 	    strcpy(fm.plf->lfFaceName, "Courier New");
	    break;
	case FF_ROMAN:
	    strcpy(fm.plf->lfFaceName, "Times New Roman");
	    break;
	case FF_SWISS:
	    strcpy(fm.plf->lfFaceName, "Arial");
	    break;
	default:
	    if((fm.plf->lfPitchAndFamily & 0x0f) == FIXED_PITCH)
	        strcpy(fm.plf->lfFaceName, "Courier New");
	    else
	        strcpy(fm.plf->lfFaceName, "Arial");
	    break;
	}
    }

    if( fm.plf->lfFaceName[0] )
    {
        fm.pfr = XFONT_FindFIList( start, fm.plf->lfFaceName);
	if( fm.pfr ) /* match family */
	{
	    TRACE("found facename '%s'\n", fm.pfr->lfFaceName );

	    if( fm.pfr->fr_flags & FR_REMOVED )
		fm.pfr = 0;
	    else
	    {
		XFONT_MatchFIList( &fm );
		*pfm = fm;
		if (pfm->pfi)
		    return;
	    }
	}

	/* get charset if lfFaceName is one of known facenames. */
	{
	    const struct CharsetBindingInfo* pcharsetbindings;

	    pcharsetbindings = &charsetbindings[0];
	    while ( pcharsetbindings->pszFaceName != NULL )
	    {
		if ( !strcmp( pcharsetbindings->pszFaceName,
			      fm.plf->lfFaceName ) )
		{
		    fm.internal_charset = pcharsetbindings->charset;
		    break;
		}
		pcharsetbindings ++;
	    }
	    TRACE( "%s charset %u\n", fm.plf->lfFaceName, fm.internal_charset );
	}
    }

    /* match all available fonts */

    fm.flags |= FO_MATCH_PAF;
    for( ppfr = &fontList; *ppfr && score; ppfr = &(*ppfr)->next )
    {
	if( (*ppfr)->fr_flags & FR_REMOVED )
	{
	    if( (*ppfr)->fo_count == 0 )
		XFONT_RemoveFontResource( ppfr );
	    continue;
	}

	fm.pfr = *ppfr;

	TRACE("%s\n", fm.pfr->lfFaceName );

	current_score = XFONT_MatchFIList( &fm );
	if( current_score < score )
	{
	    score = current_score;
	    *pfm = fm;
	}
    }
}


/***********************************************************************
 *           X Font Cache
 */
static void XFONT_GrowFreeList(int start, int end)
{
   /* add all entries from 'start' up to and including 'end' */

   memset( fontCache + start, 0, (end - start + 1) * sizeof(fontObject) );

   fontCache[end].lru = fontLF;
   fontCache[end].count = -1;
   fontLF = start;
   while( start < end )
   {
	fontCache[start].count = -1;
	fontCache[start].lru = start + 1;
	start++;
   }
}

static fontObject* XFONT_LookupCachedFont( const LOGFONT16 *plf, UINT16* checksum )
{
    UINT16	cs = __lfCheckSum( plf );
    int		i = fontMRU, prev = -1;

   *checksum = cs;
    while( i >= 0 )
    {
	if( fontCache[i].lfchecksum == cs &&
	  !(fontCache[i].fo_flags & FO_REMOVED) )
	{
	    /* FIXME: something more intelligent here ? */

	    if( !memcmp( plf, &fontCache[i].lf,
			 sizeof(LOGFONT16) - LF_FACESIZE ) &&
		!strcmp( plf->lfFaceName, fontCache[i].lf.lfFaceName) )
	    {
		/* remove temporarily from the lru list */
		if( prev >= 0 )
		    fontCache[prev].lru = fontCache[i].lru;
		else
		    fontMRU = (INT16)fontCache[i].lru;
		return (fontCache + i);
	    }
	}
	prev = i;
	i = (INT16)fontCache[i].lru;
    }
    return NULL;
}

static fontObject* XFONT_GetCacheEntry(void)
{
    int		i;

    if( fontLF == -1 )
    {
	int	prev_i, prev_j, j;

	TRACE("font cache is full\n");

	/* lookup the least recently used font */

	for( prev_i = prev_j = j = -1, i = fontMRU; i >= 0; i = (INT16)fontCache[i].lru )
	{
	    if( fontCache[i].count <= 0 &&
	      !(fontCache[i].fo_flags & FO_SYSTEM) )
	    {
		prev_j = prev_i;
		j = i;
	    }
	    prev_i = i;
	}

	if( j >= 0 )	/* unload font */
	{
	    /* detach from the lru list */

	    TRACE("\tfreeing entry %i\n", j );

	    fontCache[j].fr->fo_count--;

	    if( prev_j >= 0 )
		fontCache[prev_j].lru = fontCache[j].lru;
	    else fontMRU = (INT16)fontCache[j].lru;

	    /* FIXME: lpXForm, lpPixmap */
            HeapFree( GetProcessHeap(), 0, fontCache[j].lpX11Trans );

            wine_tsx11_lock();
            XFreeFont( gdi_display, fontCache[j].fs );
            wine_tsx11_unlock();

	    memset( fontCache + j, 0, sizeof(fontObject) );
	    return (fontCache + j);
	}
	else		/* expand cache */
	{
	    fontObject*	newCache;

	    prev_i = fontCacheSize + FONTCACHE;

	    TRACE("\tgrowing font cache from %i to %i\n", fontCacheSize, prev_i );

	    if( (newCache = HeapReAlloc(GetProcessHeap(), 0, fontCache, prev_i * sizeof(fontObject))) )
	    {
		i = fontCacheSize;
		fontCacheSize  = prev_i;
		fontCache = newCache;
		XFONT_GrowFreeList( i, fontCacheSize - 1);
	    }
	    else return NULL;
	}
    }

    /* detach from the free list */

    i = fontLF;
    fontLF = (INT16)fontCache[i].lru;
    fontCache[i].count = 0;
    return (fontCache + i);
}

static int XFONT_ReleaseCacheEntry(const fontObject* pfo)
{
    UINT	u = (UINT)(pfo - fontCache);
    int 	i;
    int 	ret;

    if( u < fontCacheSize )
    {
	ret = --fontCache[u].count;
	if ( ret == 0 )
	{
	    for ( i = 0; i < X11FONT_REFOBJS_MAX; i++ )
	    {
		if( CHECK_PFONT(pfo->prefobjs[i]) )
		    XFONT_ReleaseCacheEntry(__PFONT(pfo->prefobjs[i]));
	    }
	}

	return ret;
    }

    return -1;
}

/***********************************************************************
 *           X11DRV_FONT_InitX11Metrics
 *
 * Initialize font resource list and allocate font cache.
 */
static void X11DRV_FONT_InitX11Metrics( void )
{
  char**    x_pattern;
  unsigned  x_checksum;
  int       i, x_count, fd, buf_size;
  char      *buffer;
  HKEY hkey;


  wine_tsx11_lock();
  x_pattern = XListFonts(gdi_display, "*", MAX_FONTS, &x_count );
  wine_tsx11_unlock();

  TRACE("Font Mapper: initializing %i x11 fonts\n", x_count);
  if (x_count == MAX_FONTS)
      MESSAGE("There may be more fonts available - try increasing the value of MAX_FONTS\n");

  for( i = x_checksum = 0; i < x_count; i++ )
  {
     int j;
#if 0
     printf("%i\t: %s\n", i, x_pattern[i] );
#endif

     j = strlen( x_pattern[i] );
     if( j ) x_checksum ^= __genericCheckSum( x_pattern[i], j );
  }
  x_checksum |= X_PFONT_MAGIC;
  buf_size = 128;
  buffer = HeapAlloc( GetProcessHeap(), 0, buf_size );

  /* deal with systemwide font metrics cache */

  buffer[0] = 0;
  /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver\Fonts */
  if (RegOpenKeyA(HKEY_CURRENT_USER, INIFontSection, &hkey)) hkey = 0;
  if (hkey)
  {
	DWORD type, count = buf_size;
	RegQueryValueExA(hkey, INIGlobalMetrics, 0, &type, (LPBYTE)buffer, &count);
  }

  if( buffer[0] )
  {
      fd = open( buffer, O_RDONLY );
      XFONT_ReadCachedMetrics(fd, DefResolution, x_checksum, x_count);
  }
  if (fontList == NULL)
  {
      /* try per-user */
      buffer = XFONT_UserMetricsCache( buffer, &buf_size );
      if( buffer[0] )
      {
	  fd = open( buffer, O_RDONLY );
	  XFONT_ReadCachedMetrics(fd, DefResolution, x_checksum, x_count);
      }
  }

  if( fontList == NULL )	/* build metrics from scratch */
  {
      int n_ff = XFONT_BuildMetrics(x_pattern, DefResolution, x_checksum, x_count);
      if( buffer[0] )	 /* update cached metrics */
      {
	  fd = open( buffer, O_CREAT | O_TRUNC | O_RDWR, 0644 ); /* -rw-r--r-- */
	  if( XFONT_WriteCachedMetrics( fd, x_checksum, x_count, n_ff ) == FALSE )
	  {
	      WARN("Unable to write to fontcache '%s'\n", buffer);
	      if( fd >= 0) remove( buffer );	/* couldn't write entire file */
	  }
      }
  }

  wine_tsx11_lock();
  XFreeFontNames(x_pattern);

  /* check if we're dealing with X11 R6 server */
  {
      XFontStruct*  x_fs;
      strcpy(buffer, "-*-*-*-*-normal-*-[12 0 0 12]-*-72-*-*-*-iso8859-1");
      if( (x_fs = safe_XLoadQueryFont(gdi_display, buffer)) )
      {
	  text_caps |= TC_SF_X_YINDEP;
	  XFreeFont(gdi_display, x_fs);
      }
  }
  wine_tsx11_unlock();

  HeapFree(GetProcessHeap(), 0, buffer);

  XFONT_WindowsNames();
  XFONT_LoadAliases( hkey );
  if (hkey) XFONT_LoadDefaults( hkey );
  XFONT_LoadIgnores( hkey );

  /* fontList initialization is over, allocate X font cache */

  fontCache = HeapAlloc(GetProcessHeap(), 0, fontCacheSize * sizeof(fontObject));
  XFONT_GrowFreeList(0, fontCacheSize - 1);

  TRACE("done!\n");
  if (hkey) RegCloseKey(hkey);
}

/***********************************************************************
 *           X11DRV_FONT_Init
 */
void X11DRV_FONT_Init( int log_pixels_x, int log_pixels_y )
{
    XFONT_GetDefResolution( log_pixels_x, log_pixels_y );

    if(using_client_side_fonts)
        text_caps |= TC_VA_ABLE;

    return;
}

/**********************************************************************
 *	XFONT_SetX11Trans
 */
static BOOL XFONT_SetX11Trans( fontObject *pfo )
{
  char *fontName;
  Atom nameAtom;
  LFD lfd;

  wine_tsx11_lock();
  XGetFontProperty( pfo->fs, XA_FONT, &nameAtom );
  fontName = XGetAtomName( gdi_display, nameAtom );
  if (!LFD_Parse(fontName, &lfd) || lfd.pixel_size[0] != '[')
  {
      XFree(fontName);
      wine_tsx11_unlock();
      return FALSE;
  }
#define PX pfo->lpX11Trans

  sscanf(lfd.pixel_size, "[%f%f%f%f]", &PX->a, &PX->b, &PX->c, &PX->d);
  XFree(fontName);

  XGetFontProperty( pfo->fs, x11drv_atom(RAW_ASCENT), &PX->RAW_ASCENT );
  XGetFontProperty( pfo->fs, x11drv_atom(RAW_DESCENT), &PX->RAW_DESCENT );
  wine_tsx11_unlock();

  PX->pixelsize = hypot(PX->a, PX->b);
  PX->ascent = PX->pixelsize / 1000.0 * PX->RAW_ASCENT;
  PX->descent = PX->pixelsize / 1000.0 * PX->RAW_DESCENT;

  TRACE("[%f %f %f %f] RA = %ld RD = %ld\n",
	PX->a, PX->b, PX->c, PX->d,
	PX->RAW_ASCENT, PX->RAW_DESCENT);

#undef PX
  return TRUE;
}

/***********************************************************************
 *           X Device Font Objects
 */
static X_PHYSFONT XFONT_RealizeFont( LPLOGFONT16 plf,
				     LPCSTR* faceMatched, BOOL bSubFont,
				     WORD internal_charset,
				     WORD* pcharsetMatched )
{
    UINT16	checksum;
    INT         index = 0;
    fontObject* pfo;

    pfo = XFONT_LookupCachedFont( plf, &checksum );
    if( !pfo )
    {
	fontMatch	fm;
	INT		i;

	fm.pfr = NULL;
	fm.pfi = NULL;
	fm.height = 0;
	fm.flags = 0;
	fm.plf = plf;
	fm.internal_charset = internal_charset;

	if( text_caps & TC_SF_X_YINDEP ) fm.flags = FO_MATCH_XYINDEP;

	/* allocate new font cache entry */

	if( (pfo = XFONT_GetCacheEntry()) )
	{
	    /* initialize entry and load font */
	    char lpLFD[MAX_LFD_LENGTH];
	    UINT uRelaxLevel = 0;

	    if(abs(plf->lfHeight) > MAX_FONT_SIZE) {
		ERR(
  "plf->lfHeight = %d, Creating a 100 pixel font and rescaling metrics\n",
		    plf->lfHeight);
		pfo->rescale = fabs(plf->lfHeight / 100.0);
		if(plf->lfHeight > 0) plf->lfHeight = 100;
		else plf->lfHeight = -100;
	    } else
		pfo->rescale = 1.0;

	    XFONT_MatchDeviceFont( fontList, &fm );
	    pfo->fr = fm.pfr;
	    pfo->fi = fm.pfi;
	    pfo->fr->fo_count++;
	    pfo->fo_flags = fm.flags & ~FO_MATCH_MASK;

	    pfo->lf = *plf;
	    pfo->lfchecksum = checksum;

	    do
	    {
		LFD_ComposeLFD( pfo, fm.height, lpLFD, uRelaxLevel++ );
		if( (pfo->fs = safe_XLoadQueryFont( gdi_display, lpLFD )) ) break;
	    } while( uRelaxLevel );


	    if(pfo->lf.lfEscapement != 0) {
		pfo->lpX11Trans = HeapAlloc(GetProcessHeap(), 0, sizeof(XFONTTRANS));
		if(!XFONT_SetX11Trans( pfo )) {
		    HeapFree(GetProcessHeap(), 0, pfo->lpX11Trans);
		    pfo->lpX11Trans = NULL;
		}
	    }
	    XFONT_GetLeading( &pfo->fi->df, pfo->fs,
			      &pfo->foInternalLeading, NULL, pfo->lpX11Trans );
	    pfo->foAvgCharWidth = (INT16)XFONT_GetAvgCharWidth(&pfo->fi->df, pfo->fs, pfo->lpX11Trans );
	    pfo->foMaxCharWidth = (INT16)XFONT_GetMaxCharWidth(pfo->fs, pfo->lpX11Trans);

	    /* FIXME: If we've got a soft font or
	     * there are FO_SYNTH_... flags for the
	     * non PROOF_QUALITY request, the engine
	     * should rasterize characters into mono
	     * pixmaps and store them in the pfo->lpPixmap
	     * array (pfo->fs should be updated as well).
	     * array (pfo->fs should be updated as well).
	     * X11DRV_ExtTextOut() must be heavily modified
	     * to support pixmap blitting and FO_SYNTH_...
	     * styles.
	     */

	    pfo->lpPixmap = NULL;

	    for ( i = 0; i < X11FONT_REFOBJS_MAX; i++ )
		pfo->prefobjs[i] = 0xffffffff; /* invalid value */

            /* special treatment for DBCS that needs multiple fonts */
            /* All member of pfo must be set correctly. */
	    if ( bSubFont == FALSE )
	    {
		WORD charset_sub;
		WORD charsetMatchedSub;
		LOGFONT16 lfSub;
		LPCSTR faceMatchedSub;

		for ( i = 0; i < X11FONT_REFOBJS_MAX; i++ )
		{
		    charset_sub = X11DRV_cptable[pfo->fi->cptable].
				penum_subfont_charset( i );
		    if ( charset_sub == DEFAULT_CHARSET ) break;

		    lfSub = *plf;
		    lfSub.lfWidth = 0;
                   lfSub.lfHeight=plf->lfHeight;
		    lfSub.lfCharSet = (BYTE)(charset_sub & 0xff);
		    lfSub.lfFaceName[0] = '\0'; /* FIXME? */
		    /* this font has sub font */
                    if ( i == 0 ) pfo->prefobjs[0] = 0;
		    pfo->prefobjs[i] =
			XFONT_RealizeFont( &lfSub, &faceMatchedSub,
					   TRUE, charset_sub,
					   &charsetMatchedSub );
		    /* FIXME: check charsetMatchedSub */
		}
	    }
	}

	if( !pfo ) /* couldn't get a new entry, get one of the cached fonts */
	{
	    UINT		current_score, score = (UINT)(-1);

	    i = index = fontMRU;
	    fm.flags |= FO_MATCH_PAF;
            do
            {
		pfo = fontCache + i;
		fm.pfr = pfo->fr; fm.pfi = pfo->fi;

                current_score = XFONT_Match( &fm );
                if( current_score < score ) index = i;

	        i =  pfo->lru;
            } while( i >= 0 );
	    pfo = fontCache + index;
	    goto END;
	}
    }

    /* attach at the head of the lru list */
    pfo->lru = fontMRU;
    index = fontMRU = (pfo - fontCache);

END:
    pfo->count++;

    TRACE("physfont %i\n", index);
    *faceMatched = pfo->fi->df.dfFace;
    *pcharsetMatched = pfo->fi->internal_charset;

    return X_PFONT_MAGIC | index;
}

/***********************************************************************
 *           XFONT_GetFontObject
 */
fontObject* XFONT_GetFontObject( X_PHYSFONT pFont )
{
    if( CHECK_PFONT(pFont) ) return __PFONT(pFont);
    return NULL;
}

/***********************************************************************
 *           XFONT_GetFontStruct
 */
XFontStruct* XFONT_GetFontStruct( X_PHYSFONT pFont )
{
    if( CHECK_PFONT(pFont) ) return __PFONT(pFont)->fs;
    return NULL;
}



/* X11DRV Interface ****************************************************
 *                                                                     *
 * Exposed via the dc->funcs dispatch table.                           *
 *                                                                     *
 ***********************************************************************/
/***********************************************************************
 *           SelectFont   (X11DRV.@)
 */
HFONT CDECL X11DRV_SelectFont( X11DRV_PDEVICE *physDev, HFONT hfont, HANDLE gdiFont )
{
    LOGFONTW logfont;
    LOGFONT16 lf;

    TRACE("hdc=%p, hfont=%p\n", physDev->hdc, hfont);

    if (!GetObjectW( hfont, sizeof(logfont), &logfont )) return HGDI_ERROR;

    TRACE("gdiFont = %p\n", gdiFont);

    if(gdiFont && using_client_side_fonts) {
        X11DRV_XRender_SelectFont(physDev, hfont);
        physDev->has_gdi_font = TRUE;
	return FALSE;
    }

    EnterCriticalSection( &crtsc_fonts_X11 );

    if(fontList == NULL) X11DRV_FONT_InitX11Metrics();

    if( CHECK_PFONT(physDev->font) )
        XFONT_ReleaseCacheEntry( __PFONT(physDev->font) );

    FONT_LogFontWTo16(&logfont, &lf);

    /* stock fonts ignore the mapping mode */
    if (!is_stock_font( hfont ))
    {
        /* Make sure we don't change the sign when converting to device coords */
        /* FIXME - check that the other drivers do this correctly */
        if (lf.lfWidth)
        {
            INT width = X11DRV_XWStoDS( physDev, lf.lfWidth );
            lf.lfWidth = (lf.lfWidth < 0) ? -abs(width) : abs(width);
            if (lf.lfWidth == 0)
                lf.lfWidth = 1; /* Minimum width */
        }
        if (lf.lfHeight)
        {
            INT height = X11DRV_YWStoDS( physDev, lf.lfHeight );
            lf.lfHeight = (lf.lfHeight < 0) ? -abs(height) : abs(height);
            if (lf.lfHeight == 0)
                lf.lfHeight = MIN_FONT_SIZE;
        }
    }

    if (!lf.lfHeight)
        lf.lfHeight = -(DEF_POINT_SIZE * GetDeviceCaps(physDev->hdc,LOGPIXELSY) + (72>>1)) / 72;

    {
	/* Fixup aliases before passing to RealizeFont */
        /* alias = Windows name in the alias table */
	LPCSTR alias = XFONT_UnAlias( lf.lfFaceName );
	LPCSTR faceMatched;
	WORD charsetMatched;

	TRACE("hfont=%p\n", hfont); /* to connect with the trace from RealizeFont */
	physDev->font = XFONT_RealizeFont( &lf, &faceMatched,
					   FALSE, lf.lfCharSet,
					   &charsetMatched );

	/* set face to the requested facename if it matched
	 * so that GetTextFace can get the correct face name
	 */
	if (alias && !strcmp(faceMatched, lf.lfFaceName))
	    MultiByteToWideChar(CP_ACP, 0, alias, -1,
				logfont.lfFaceName, LF_FACESIZE);
	else
	    MultiByteToWideChar(CP_ACP, 0, faceMatched, -1,
			      logfont.lfFaceName, LF_FACESIZE);

	/*
	 * In X, some encodings may have the same lfFaceName.
	 * for example:
	 *   -misc-fixed-*-iso8859-1
	 *   -misc-fixed-*-jisx0208.1990-0
	 * so charset should be saved...
	 */
	logfont.lfCharSet = charsetMatched;
    }

    LeaveCriticalSection( &crtsc_fonts_X11 );

    physDev->has_gdi_font = FALSE;
    return (HFONT)1; /* Use a device font */
}


/***********************************************************************
 *
 *           X11DRV_EnumDeviceFonts
 */
BOOL CDECL X11DRV_EnumDeviceFonts( X11DRV_PDEVICE *physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp )
{
    ENUMLOGFONTEXW	lf;
    NEWTEXTMETRICEXW	tm;
    fontResource*	pfr = fontList;
    BOOL	  	b, bRet = 0;
    LOGFONTW lfW;

    /* don't enumerate x11 fonts if we're using client side fonts */
    if (physDev->has_gdi_font) return FALSE;

    if (!plf)
    {
        lfW.lfCharSet = DEFAULT_CHARSET;
        lfW.lfPitchAndFamily = 0;
        lfW.lfFaceName[0] = 0;
        plf = &lfW;
    }

    if( plf->lfFaceName[0] )
    {
        char facename[LF_FACESIZE+1];
        WideCharToMultiByte( CP_ACP, 0, plf->lfFaceName, -1,
                             facename, sizeof(facename), NULL, NULL );
	/* enum all entries in this resource */
	pfr = XFONT_FindFIList( pfr, facename );
	if( pfr )
	{
	    fontInfo*	pfi;
	    for( pfi = pfr->fi; pfi; pfi = pfi->next )
	    {
		/* Note: XFONT_GetFontMetric() will have to
		   release the crit section, font list will
		   have to be retraversed on return */
	        if(plf->lfCharSet == DEFAULT_CHARSET ||
		   plf->lfCharSet == pfi->df.dfCharSet) {
		    UINT xfm = XFONT_GetFontMetric( pfi, &lf, &tm );

		    if( (b = (*proc)( &lf.elfLogFont, (TEXTMETRICW *)&tm, xfm, lp )) )
		        bRet = b;
		    else break;
		}
	    }
	}
    }
    else /* enum first entry in each resource */
	for( ; pfr ; pfr = pfr->next )
	{
            if(pfr->fi)
            {
	        UINT xfm = XFONT_GetFontMetric( pfr->fi, &lf, &tm );

	        if( (b = (*proc)( &lf.elfLogFont, (TEXTMETRICW *)&tm, xfm, lp )) )
		    bRet = b;
		else break;
            }
	}
    return bRet;
}


/***********************************************************************
 *           X11DRV_GetTextMetrics
 */
BOOL CDECL X11DRV_GetTextMetrics(X11DRV_PDEVICE *physDev, TEXTMETRICW *metrics)
{
    if( CHECK_PFONT(physDev->font) )
    {
	fontObject* pfo = __PFONT(physDev->font);
	X11DRV_cptable[pfo->fi->cptable].pGetTextMetricsW( pfo, metrics );
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           X11DRV_GetCharWidth
 */
BOOL CDECL X11DRV_GetCharWidth( X11DRV_PDEVICE *physDev, UINT firstChar, UINT lastChar,
                                  LPINT buffer )
{
    fontObject* pfo = XFONT_GetFontObject( physDev->font );

    if( pfo )
    {
	unsigned int i;

	if (pfo->fs->per_char == NULL)
	    for (i = firstChar; i <= lastChar; i++)
  	        if(pfo->lpX11Trans)
		    *buffer++ = pfo->fs->min_bounds.attributes *
		      pfo->lpX11Trans->pixelsize / 1000.0 * pfo->rescale;
		else
		    *buffer++ = pfo->fs->min_bounds.width * pfo->rescale;
	else
	{
	    XCharStruct *cs, *def;
	    static XCharStruct	__null_char = { 0, 0, 0, 0, 0, 0 };

	    CI_GET_CHAR_INFO(pfo->fs, pfo->fs->default_char, &__null_char,
			     def);

	    for (i = firstChar; i <= lastChar; i++)
	    {
		WCHAR wch = i;
		BYTE ch;
		UINT ch_f; /* character code in the font encoding */
		WideCharToMultiByte( pfo->fi->codepage, 0, &wch, 1, (LPSTR)&ch, 1, NULL, NULL );
		ch_f = ch;
		if (ch_f >= pfo->fs->min_char_or_byte2 &&
		    ch_f <= pfo->fs->max_char_or_byte2)
		{
		    cs = &pfo->fs->per_char[(ch_f - pfo->fs->min_char_or_byte2)];
		    if (CI_NONEXISTCHAR(cs)) cs = def;
  		} else cs = def;
		if(pfo->lpX11Trans)
		    *buffer++ = max(cs->attributes, 0) *
		      pfo->lpX11Trans->pixelsize / 1000.0 * pfo->rescale;
		else
		    *buffer++ = max(cs->width, 0 ) * pfo->rescale;
	    }
	}

	return TRUE;
    }
    return FALSE;
}
