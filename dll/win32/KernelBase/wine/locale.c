/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2002, 2019 Alexandre Julliard
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

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define WINNORMALIZEAPI
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winuser.h"
#include "winternl.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

#define CALINFO_MAX_YEAR 2029

static HMODULE kernelbase_handle;

struct registry_entry
{
    const WCHAR                         *value;
    const WCHAR                         *subkey;
    enum { NOT_CACHED, CACHED, MISSING } status;
    WCHAR                                data[80];
};

static const WCHAR world_subkey[] = { 0xd83c, 0xdf0e, 0xd83c, 0xdf0f, 0xd83c, 0xdf0d, 0 }; /* 🌎🌏🌍 */

static struct registry_entry entry_icalendartype      = { L"iCalendarType" };
static struct registry_entry entry_icountry           = { L"iCountry" };
static struct registry_entry entry_icurrdigits        = { L"iCurrDigits" };
static struct registry_entry entry_icurrency          = { L"iCurrency" };
static struct registry_entry entry_idigits            = { L"iDigits" };
static struct registry_entry entry_idigitsubstitution = { L"NumShape" };
static struct registry_entry entry_ifirstdayofweek    = { L"iFirstDayOfWeek" };
static struct registry_entry entry_ifirstweekofyear   = { L"iFirstWeekOfYear" };
static struct registry_entry entry_ilzero             = { L"iLZero" };
static struct registry_entry entry_imeasure           = { L"iMeasure" };
static struct registry_entry entry_inegcurr           = { L"iNegCurr" };
static struct registry_entry entry_inegnumber         = { L"iNegNumber" };
static struct registry_entry entry_ipapersize         = { L"iPaperSize" };
static struct registry_entry entry_s1159              = { L"s1159" };
static struct registry_entry entry_s2359              = { L"s2359" };
static struct registry_entry entry_scurrency          = { L"sCurrency" };
static struct registry_entry entry_sdecimal           = { L"sDecimal" };
static struct registry_entry entry_sgrouping          = { L"sGrouping" };
static struct registry_entry entry_sintlsymbol        = { L"Currencies", world_subkey };
static struct registry_entry entry_slist              = { L"sList" };
static struct registry_entry entry_slongdate          = { L"sLongDate" };
static struct registry_entry entry_smondecimalsep     = { L"sMonDecimalSep" };
static struct registry_entry entry_smongrouping       = { L"sMonGrouping" };
static struct registry_entry entry_smonthousandsep    = { L"sMonThousandSep" };
static struct registry_entry entry_snativedigits      = { L"sNativeDigits" };
static struct registry_entry entry_snegativesign      = { L"sNegativeSign" };
static struct registry_entry entry_spositivesign      = { L"sPositiveSign" };
static struct registry_entry entry_sshortdate         = { L"sShortDate" };
static struct registry_entry entry_sshorttime         = { L"sShortTime" };
static struct registry_entry entry_sthousand          = { L"sThousand" };
static struct registry_entry entry_stimeformat        = { L"sTimeFormat" };
static struct registry_entry entry_syearmonth         = { L"sYearMonth" };


static const struct { UINT cp; const WCHAR *name; } codepage_names[] =
{
    { 37,    L"IBM EBCDIC US Canada" },
    { 424,   L"IBM EBCDIC Hebrew" },
    { 437,   L"OEM United States" },
    { 500,   L"IBM EBCDIC International" },
    { 708,   L"Arabic ASMO" },
    { 720,   L"Arabic (Transparent ASMO)" },
    { 737,   L"OEM Greek 437G" },
    { 775,   L"OEM Baltic" },
    { 850,   L"OEM Multilingual Latin 1" },
    { 852,   L"OEM Slovak Latin 2" },
    { 855,   L"OEM Cyrillic" },
    { 856,   L"Hebrew PC" },
    { 857,   L"OEM Turkish" },
    { 860,   L"OEM Portuguese" },
    { 861,   L"OEM Icelandic" },
    { 862,   L"OEM Hebrew" },
    { 863,   L"OEM Canadian French" },
    { 864,   L"OEM Arabic" },
    { 865,   L"OEM Nordic" },
    { 866,   L"OEM Russian" },
    { 869,   L"OEM Greek" },
    { 874,   L"ANSI/OEM Thai" },
    { 875,   L"IBM EBCDIC Greek" },
    { 878,   L"Russian KOI8" },
    { 932,   L"ANSI/OEM Japanese Shift-JIS" },
    { 936,   L"ANSI/OEM Simplified Chinese GBK" },
    { 949,   L"ANSI/OEM Korean Unified Hangul" },
    { 950,   L"ANSI/OEM Traditional Chinese Big5" },
    { 1006,  L"IBM Arabic" },
    { 1026,  L"IBM EBCDIC Latin 5 Turkish" },
    { 1250,  L"ANSI Eastern Europe" },
    { 1251,  L"ANSI Cyrillic" },
    { 1252,  L"ANSI Latin 1" },
    { 1253,  L"ANSI Greek" },
    { 1254,  L"ANSI Turkish" },
    { 1255,  L"ANSI Hebrew" },
    { 1256,  L"ANSI Arabic" },
    { 1257,  L"ANSI Baltic" },
    { 1258,  L"ANSI/OEM Viet Nam" },
    { 1361,  L"Korean Johab" },
    { 10000, L"Mac Roman" },
    { 10001, L"Mac Japanese" },
    { 10002, L"Mac Traditional Chinese" },
    { 10003, L"Mac Korean" },
    { 10004, L"Mac Arabic" },
    { 10005, L"Mac Hebrew" },
    { 10006, L"Mac Greek" },
    { 10007, L"Mac Cyrillic" },
    { 10008, L"Mac Simplified Chinese" },
    { 10010, L"Mac Romanian" },
    { 10017, L"Mac Ukrainian" },
    { 10021, L"Mac Thai" },
    { 10029, L"Mac Latin 2" },
    { 10079, L"Mac Icelandic" },
    { 10081, L"Mac Turkish" },
    { 10082, L"Mac Croatian" },
    { 20127, L"US-ASCII (7bit)" },
    { 20866, L"Russian KOI8" },
    { 20932, L"EUC-JP" },
    { 20949, L"Korean Wansung" },
    { 21866, L"Ukrainian KOI8" },
    { 28591, L"ISO 8859-1 Latin 1" },
    { 28592, L"ISO 8859-2 Latin 2 (East European)" },
    { 28593, L"ISO 8859-3 Latin 3 (South European)" },
    { 28594, L"ISO 8859-4 Latin 4 (Baltic old)" },
    { 28595, L"ISO 8859-5 Cyrillic" },
    { 28596, L"ISO 8859-6 Arabic" },
    { 28597, L"ISO 8859-7 Greek" },
    { 28598, L"ISO 8859-8 Hebrew" },
    { 28599, L"ISO 8859-9 Latin 5 (Turkish)" },
    { 28600, L"ISO 8859-10 Latin 6 (Nordic)" },
    { 28601, L"ISO 8859-11 Latin (Thai)" },
    { 28603, L"ISO 8859-13 Latin 7 (Baltic)" },
    { 28604, L"ISO 8859-14 Latin 8 (Celtic)" },
    { 28605, L"ISO 8859-15 Latin 9 (Euro)" },
    { 28606, L"ISO 8859-16 Latin 10 (Balkan)" },
    { 65000, L"65000 (UTF-7)" },
    { 65001, L"65001 (UTF-8)" }
};

/* Unicode expanded ligatures */
static const WCHAR ligatures[][5] =
{
    { 0x00c6,  'A','E',0 },
    { 0x00de,  'T','H',0 },
    { 0x00df,  's','s',0 },
    { 0x00e6,  'a','e',0 },
    { 0x00fe,  't','h',0 },
    { 0x0132,  'I','J',0 },
    { 0x0133,  'i','j',0 },
    { 0x0152,  'O','E',0 },
    { 0x0153,  'o','e',0 },
    { 0x01c4,  'D',0x017d,0 },
    { 0x01c5,  'D',0x017e,0 },
    { 0x01c6,  'd',0x017e,0 },
    { 0x01c7,  'L','J',0 },
    { 0x01c8,  'L','j',0 },
    { 0x01c9,  'l','j',0 },
    { 0x01ca,  'N','J',0 },
    { 0x01cb,  'N','j',0 },
    { 0x01cc,  'n','j',0 },
    { 0x01e2,  0x0100,0x0112,0 },
    { 0x01e3,  0x0101,0x0113,0 },
    { 0x01f1,  'D','Z',0 },
    { 0x01f2,  'D','z',0 },
    { 0x01f3,  'd','z',0 },
    { 0x01fc,  0x00c1,0x00c9,0 },
    { 0x01fd,  0x00e1,0x00e9,0 },
    { 0x05f0,  0x05d5,0x05d5,0 },
    { 0x05f1,  0x05d5,0x05d9,0 },
    { 0x05f2,  0x05d9,0x05d9,0 },
    { 0xfb00,  'f','f',0 },
    { 0xfb01,  'f','i',0 },
    { 0xfb02,  'f','l',0 },
    { 0xfb03,  'f','f','i',0 },
    { 0xfb04,  'f','f','l',0 },
    { 0xfb05,  0x017f,'t',0 },
    { 0xfb06,  's','t',0 },
};

struct calendar
{
    USHORT icalintvalue;        /* 00 */
    USHORT itwodigityearmax;    /* 02 */
    UINT   sshortdate;          /* 04 */
    UINT   syearmonth;          /* 08 */
    UINT   slongdate;           /* 0c */
    UINT   serastring;          /* 10 */
    UINT   iyearoffsetrange;    /* 14 */
    UINT   sdayname;            /* 18 */
    UINT   sabbrevdayname;      /* 1c */
    UINT   smonthname;          /* 20 */
    UINT   sabbrevmonthname;    /* 24 */
    UINT   scalname;            /* 28 */
    UINT   smonthday;           /* 2c */
    UINT   sabbreverastring;    /* 30 */
    UINT   sshortestdayname;    /* 34 */
    UINT   srelativelongdate;   /* 38 */
    UINT   unused[3];           /* 3c */
};

static const struct geo_id
{
    GEOID    id;
    WCHAR    latitude[12];
    WCHAR    longitude[12];
    GEOCLASS class;
    GEOID    parent;
    WCHAR    iso2[4];
    WCHAR    iso3[4];
    USHORT   uncode;
    USHORT   dialcode;
    WCHAR    currcode[4];
    WCHAR    currsymbol[8];
} *geo_ids;

static const struct geo_index
{
    WCHAR  name[4];
    UINT   idx;
} *geo_index;

static unsigned int geo_ids_count;
static unsigned int geo_index_count;

enum charmaps
{
    CHARMAP_FOLDDIGITS,
    CHARMAP_COMPAT,
    CHARMAP_HIRAGANA,
    CHARMAP_KATAKANA,
    CHARMAP_HALFWIDTH,
    CHARMAP_FULLWIDTH,
    CHARMAP_TRADITIONAL,
    CHARMAP_SIMPLIFIED,
    NB_CHARMAPS
};

static const USHORT *charmaps[NB_CHARMAPS];

/* NLS normalization file */
struct norm_table
{
    WCHAR   name[13];      /* 00 file name */
    USHORT  checksum[3];   /* 1a checksum? */
    USHORT  version[4];    /* 20 Unicode version */
    USHORT  form;          /* 28 normalization form */
    USHORT  len_factor;    /* 2a factor for length estimates */
    USHORT  unknown1;      /* 2c */
    USHORT  decomp_size;   /* 2e decomposition hash size */
    USHORT  comp_size;     /* 30 composition hash size */
    USHORT  unknown2;      /* 32 */
    USHORT  classes;       /* 34 combining classes table offset */
    USHORT  props_level1;  /* 36 char properties table level 1 offset */
    USHORT  props_level2;  /* 38 char properties table level 2 offset */
    USHORT  decomp_hash;   /* 3a decomposition hash table offset */
    USHORT  decomp_map;    /* 3c decomposition character map table offset */
    USHORT  decomp_seq;    /* 3e decomposition character sequences offset */
    USHORT  comp_hash;     /* 40 composition hash table offset */
    USHORT  comp_seq;      /* 42 composition character sequences offset */
    /* BYTE[]       combining class values */
    /* BYTE[0x2200] char properties index level 1 */
    /* BYTE[]       char properties index level 2 */
    /* WORD[]       decomposition hash table */
    /* WORD[]       decomposition character map */
    /* WORD[]       decomposition character sequences */
    /* WORD[]       composition hash table */
    /* WORD[]       composition character sequences */
};

static CPTABLEINFO ansi_cpinfo;
static CPTABLEINFO oem_cpinfo;
static UINT unix_cp = CP_UTF8;
static LCID system_lcid;
static LCID user_lcid;
static HKEY intl_key;
static HKEY nls_key;
static HKEY tz_key;
static const NLS_LOCALE_LCID_INDEX *lcids_index;
static const NLS_LOCALE_LCNAME_INDEX *lcnames_index;
static const NLS_LOCALE_HEADER *locale_table;
static const WCHAR *locale_strings;
static const NLS_LOCALE_DATA *system_locale;
static const NLS_LOCALE_DATA *user_locale;

static CPTABLEINFO codepages[128];
static unsigned int nb_codepages;

static struct norm_table *norm_info;

struct sortguid
{
    GUID  id;          /* sort GUID */
    UINT  flags;       /* flags */
    UINT  compr;       /* offset to compression table */
    UINT  except;      /* exception table offset in sortkey table */
    UINT  ling_except; /* exception table offset for linguistic casing */
    UINT  casemap;     /* linguistic casemap table offset */
};

/* flags for sortguid */
#define FLAG_HAS_3_BYTE_WEIGHTS 0x01
#define FLAG_REVERSEDIACRITICS  0x10
#define FLAG_DOUBLECOMPRESSION  0x20
#define FLAG_INVERSECASING      0x40

struct sort_expansion
{
    WCHAR exp[2];
};

struct jamo_sort
{
    BYTE is_old;
    BYTE leading;
    BYTE vowel;
    BYTE trailing;
    BYTE weight;
    BYTE pad[3];
};

struct sort_compression
{
    UINT  offset;
    WCHAR minchar, maxchar;
    WORD  len[8];
};

static inline int compression_size( int len ) { return 2 + len + (len & 1); }

union char_weights
{
    UINT val;
    struct { BYTE primary, script, diacritic, _case; };
};

/* bits for case weights */
#define CASE_FULLWIDTH   0x01  /* full width kana (vs. half width) */
#define CASE_FULLSIZE    0x02  /* full size kana (vs. small) */
#define CASE_SUBSCRIPT   0x08  /* sub/super script */
#define CASE_UPPER       0x10  /* upper case */
#define CASE_KATAKANA    0x20  /* katakana (vs. hiragana) */
#define CASE_COMPR_2     0x40  /* compression exists for >= 2 chars */
#define CASE_COMPR_4     0x80  /* compression exists for >= 4 chars */
#define CASE_COMPR_6     0xc0  /* compression exists for >= 6 chars */

enum sortkey_script
{
    SCRIPT_UNSORTABLE = 0,
    SCRIPT_NONSPACE_MARK = 1,
    SCRIPT_EXPANSION = 2,
    SCRIPT_EASTASIA_SPECIAL = 3,
    SCRIPT_JAMO_SPECIAL = 4,
    SCRIPT_EXTENSION_A = 5,
    SCRIPT_PUNCTUATION = 6,
    SCRIPT_SYMBOL_1 = 7,
    SCRIPT_SYMBOL_2 = 8,
    SCRIPT_SYMBOL_3 = 9,
    SCRIPT_SYMBOL_4 = 10,
    SCRIPT_SYMBOL_5 = 11,
    SCRIPT_SYMBOL_6 = 12,
    SCRIPT_DIGIT = 13,
    SCRIPT_LATIN = 14,
    SCRIPT_GREEK = 15,
    SCRIPT_CYRILLIC = 16,
    SCRIPT_KANA = 34,
    SCRIPT_HEBREW = 40,
    SCRIPT_ARABIC = 41,
    SCRIPT_PUA_FIRST = 169,
    SCRIPT_PUA_LAST = 175,
    SCRIPT_CJK_FIRST = 192,
    SCRIPT_CJK_LAST = 239,
};

static const struct sortguid **locale_sorts;
static const struct sortguid *current_locale_sort;

static struct
{
    UINT                           version;         /* NLS version */
    UINT                           guid_count;      /* number of sort GUIDs */
    UINT                           exp_count;       /* number of character expansions */
    UINT                           compr_count;     /* number of compression tables */
    const UINT                    *keys;            /* sortkey table, indexed by char */
    const USHORT                  *casemap;         /* casemap table, in l_intl.nls format */
    const WORD                    *ctypes;          /* CT_CTYPE1,2,3 values */
    const BYTE                    *ctype_idx;       /* index to map char to ctypes array entry */
    const struct sortguid         *guids;           /* table of sort GUIDs */
    const struct sort_expansion   *expansions;      /* character expansions */
    const struct sort_compression *compressions;    /* character compression tables */
    const WCHAR                   *compr_data;      /* data for individual compressions */
    const struct jamo_sort        *jamo;            /* table for Jamo compositions */
} sort;

static CRITICAL_SECTION locale_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &locale_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": locale_section") }
};
static CRITICAL_SECTION locale_section = { &critsect_debug, -1, 0, 0, 0, 0 };


static void load_locale_nls(void)
{
    struct
    {
        UINT ctypes;
        UINT unknown1;
        UINT unknown2;
        UINT unknown3;
        UINT locales;
        UINT charmaps;
        UINT geoids;
        UINT scripts;
    } *header;
    struct geo_header
    {
        WCHAR signature[4];  /* L"geo" */
        UINT  total_size;
        UINT  ids_offset;
        UINT  ids_count;
        UINT  index_offset;
        UINT  index_count;
    } *geo_header;

    LARGE_INTEGER dummy;
    const USHORT *map_ptr;
    unsigned int i;

    RtlGetLocaleFileMappingAddress( (void **)&header, &system_lcid, &dummy );
    locale_table = (const NLS_LOCALE_HEADER *)((char *)header + header->locales);
    lcids_index = (const NLS_LOCALE_LCID_INDEX *)((char *)locale_table + locale_table->lcids_offset);
    lcnames_index = (const NLS_LOCALE_LCNAME_INDEX *)((char *)locale_table + locale_table->lcnames_offset);
    locale_strings = (const WCHAR *)((char *)locale_table + locale_table->strings_offset);
    geo_header = (struct geo_header *)((char *)header + header->geoids);
    geo_ids = (const struct geo_id *)((char *)geo_header + geo_header->ids_offset);
    geo_index = (const struct geo_index *)((char *)geo_header + geo_header->index_offset);
    geo_ids_count = geo_header->ids_count;
    geo_index_count = geo_header->index_count;
    map_ptr = (const USHORT *)((char *)header + header->charmaps);
    for (i = 0; i < NB_CHARMAPS; i++, map_ptr += *map_ptr) charmaps[i] = map_ptr + 1;
}


static void load_sortdefault_nls(void)
{
    const struct
    {
        UINT sortkeys;
        UINT casemaps;
        UINT ctypes;
        UINT sortids;
    } *header;

    const WORD *ctype;
    const UINT *table;
    UINT i;
    SIZE_T size;
    const struct sort_compression *last_compr;

    NtGetNlsSectionPtr( 9, 0, NULL, (void **)&header, &size );

    sort.keys = (UINT *)((char *)header + header->sortkeys);
    sort.casemap = (USHORT *)((char *)header + header->casemaps);

    ctype = (WORD *)((char *)header + header->ctypes);
    sort.ctypes = ctype + 2;
    sort.ctype_idx = (BYTE *)ctype + ctype[1] + 2;

    table = (UINT *)((char *)header + header->sortids);
    sort.version = table[0];
    sort.guid_count = table[1];
    sort.guids = (struct sortguid *)(table + 2);

    table = (UINT *)(sort.guids + sort.guid_count);
    sort.exp_count = table[0];
    sort.expansions = (struct sort_expansion *)(table + 1);

    table = (UINT *)(sort.expansions + sort.exp_count);
    sort.compr_count = table[0];
    sort.compressions = (struct sort_compression *)(table + 1);
    sort.compr_data = (WCHAR *)(sort.compressions + sort.compr_count);

    last_compr = sort.compressions + sort.compr_count - 1;
    table = (UINT *)(sort.compr_data + last_compr->offset);
    for (i = 0; i < 7; i++) table += last_compr->len[i] * ((i + 5) / 2);
    table += 1 + table[0] / 2;  /* skip multiple weights */
    sort.jamo = (struct jamo_sort *)(table + 1);

    locale_sorts = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    locale_table->nb_lcnames * sizeof(*locale_sorts) );
}


static const struct sortguid *find_sortguid( const GUID *guid )
{
    int pos, ret, min = 0, max = sort.guid_count - 1;

    while (min <= max)
    {
        pos = (min + max) / 2;
        ret = memcmp( guid, &sort.guids[pos].id, sizeof(*guid) );
        if (!ret) return &sort.guids[pos];
        if (ret > 0) min = pos + 1;
        else max = pos - 1;
    }
    ERR( "no sort found for %s\n", debugstr_guid( guid ));
    return NULL;
}


static const NLS_LOCALE_DATA *get_locale_data( UINT idx )
{
    ULONG offset = locale_table->locales_offset + idx * locale_table->locale_size;
    return (const NLS_LOCALE_DATA *)((const char *)locale_table + offset);
}


static const struct calendar *get_calendar_data( const NLS_LOCALE_DATA *locale, UINT id )
{
    if (id == CAL_HIJRI && locale->islamic_cal[0]) id = locale->islamic_cal[0];
    else if (id == CAL_PERSIAN && locale->islamic_cal[1]) id = locale->islamic_cal[1];

    if (!id || id > locale_table->nb_calendars) return NULL;
    return (const struct calendar *)((const char *)locale_table + locale_table->calendars_offset +
                                     (id - 1) * locale_table->calendar_size);
}


static int compare_locale_names( const WCHAR *n1, const WCHAR *n2 )
{
    for (;;)
    {
        WCHAR ch1 = *n1++;
        WCHAR ch2 = *n2++;
        if (ch1 >= 'a' && ch1 <= 'z') ch1 -= 'a' - 'A';
        else if (ch1 == '_') ch1 = '-';
        if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 'a' - 'A';
        else if (ch2 == '_') ch2 = '-';
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
    }
}


static const NLS_LOCALE_LCNAME_INDEX *find_lcname_entry( const WCHAR *name )
{
    int min = 0, max = locale_table->nb_lcnames - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        const WCHAR *str = locale_strings + lcnames_index[pos].name;
        res = compare_locale_names( name, str + 1 );
        if (res < 0) max = pos - 1;
        else if (res > 0) min = pos + 1;
        else return &lcnames_index[pos];
    }
    return NULL;
}


static const NLS_LOCALE_LCID_INDEX *find_lcid_entry( LCID lcid )
{
    int min = 0, max = locale_table->nb_lcids - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (lcid < lcids_index[pos].id) max = pos - 1;
        else if (lcid > lcids_index[pos].id) min = pos + 1;
        else return &lcids_index[pos];
    }
    return NULL;
}


static const struct geo_id *find_geo_id_entry( GEOID id )
{
    int min = 0, max = geo_ids_count - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (id < geo_ids[pos].id) max = pos - 1;
        else if (id > geo_ids[pos].id) min = pos + 1;
        else return &geo_ids[pos];
    }
    return NULL;
}


static const struct geo_id *find_geo_name_entry( const WCHAR *name )
{
    int min = 0, max = geo_index_count - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        res = wcsicmp( name, geo_index[pos].name );
        if (res < 0) max = pos - 1;
        else if (res > 0) min = pos + 1;
        else return &geo_ids[geo_index[pos].idx];
    }
    return NULL;
}


static const NLS_LOCALE_DATA *get_locale_by_name( const WCHAR *name, LCID *lcid )
{
    const NLS_LOCALE_LCNAME_INDEX *entry;

    if (name == LOCALE_NAME_USER_DEFAULT)
    {
        *lcid = user_lcid;
        return user_locale;
    }
    if (name[0] == '!' && !compare_locale_names( name, LOCALE_NAME_SYSTEM_DEFAULT ))
    {
        *lcid = system_lcid;
        return system_locale;
    }
    if (!(entry = find_lcname_entry( name ))) return NULL;
    *lcid = entry->id;
    return get_locale_data( entry->idx );
}


static const struct sortguid *get_language_sort( const WCHAR *name )
{
    const NLS_LOCALE_LCNAME_INDEX *entry;
    const NLS_LOCALE_DATA *locale;
    WCHAR guidstr[39];
    const struct sortguid *ret;
    UNICODE_STRING str;
    LCID lcid;
    GUID guid;
    HKEY key = 0;
    DWORD size, type;

    if (name == LOCALE_NAME_USER_DEFAULT)
    {
        if (current_locale_sort) return current_locale_sort;
        name = locale_strings + user_locale->sname + 1;
    }
    else if (name[0] == '!' && !compare_locale_names( name, LOCALE_NAME_SYSTEM_DEFAULT ))
    {
        name = locale_strings + system_locale->sname + 1;
    }

    if (!(entry = find_lcname_entry( name )))
    {
        WARN( "unknown locale %s\n", debugstr_w(name) );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if ((ret = locale_sorts[entry - lcnames_index])) return ret;

    lcid = entry->id;
    name = locale_strings + entry->name + 1;
    locale = get_locale_data( entry->idx );
    if (!RegOpenKeyExW( nls_key, L"Sorting\\Ids", 0, KEY_READ, &key ))
    {
        for (;;)
        {
            size = sizeof(guidstr);
            if (!RegQueryValueExW( key, name, NULL, &type, (BYTE *)guidstr, &size ) && type == REG_SZ)
            {
                RtlInitUnicodeString( &str, guidstr );
                if (!RtlGUIDFromString( &str, &guid )) ret = find_sortguid( &guid );
                break;
            }
            if (!name[0]) break;
            name = locale_strings + (SORTIDFROMLCID( lcid ) ? locale->sname : locale->sparent) + 1;
            if (!(locale = get_locale_by_name( name, &lcid ))) break;
        }
        RegCloseKey( key );
    }
    if (!ret) ret = &sort.guids[0];
    locale_sorts[entry - lcnames_index] = ret;
    return ret;
}


/******************************************************************************
 *	NlsValidateLocale   (kernelbase.@)
 *
 * Note: it seems to return some internal data on Windows, we simply return the locale.nls data pointer.
 */
const NLS_LOCALE_DATA * WINAPI NlsValidateLocale( LCID *lcid, ULONG flags )
{
    const NLS_LOCALE_LCNAME_INDEX *name_entry;
    const NLS_LOCALE_LCID_INDEX *entry;
    const NLS_LOCALE_DATA *locale;

    switch (*lcid)
    {
    case LOCALE_SYSTEM_DEFAULT:
        *lcid = system_lcid;
        return system_locale;
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
    case LOCALE_CUSTOM_UNSPECIFIED:
    case LOCALE_CUSTOM_UI_DEFAULT:
        *lcid = user_lcid;
        return user_locale;
    default:
        if (!(entry = find_lcid_entry( *lcid ))) return NULL;
        locale = get_locale_data( entry->idx );
        if ((flags & LOCALE_ALLOW_NEUTRAL_NAMES) || locale->inotneutral) return locale;
        if ((name_entry = find_lcname_entry( locale_strings + locale->ssortlocale + 1 )))
            locale = get_locale_data( name_entry->idx );
        return locale;
    }
}


static int locale_return_data( const WCHAR *data, int datalen, LCTYPE type, WCHAR *buffer, int len )
{
    if (type & LOCALE_RETURN_NUMBER)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!len) return datalen;
    if (datalen > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    memcpy( buffer, data, datalen * sizeof(WCHAR) );
    return datalen;
}


static BOOL set_registry_entry( struct registry_entry *entry, const WCHAR *data )
{
    DWORD size = (wcslen(data) + 1) * sizeof(WCHAR);
    LSTATUS ret;

    if (size > sizeof(entry->data))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }
    TRACE( "setting %s to %s\n", debugstr_w(entry->value), debugstr_w(data) );

    RtlEnterCriticalSection( &locale_section );
    if (!(ret = RegSetKeyValueW( intl_key, entry->subkey, entry->value, REG_SZ, (BYTE *)data, size )))
    {
        wcscpy( entry->data, data );
        entry->status = CACHED;
    }
    RtlLeaveCriticalSection( &locale_section );
    if (ret) SetLastError( ret );
    return !ret;
}


static int locale_return_reg_string( struct registry_entry *entry, LCTYPE type, WCHAR *buffer, int len )
{
    DWORD size;
    LRESULT res;
    int ret = -1;

    if (type & LOCALE_NOUSEROVERRIDE) return -1;

    RtlEnterCriticalSection( &locale_section );
    switch (entry->status)
    {
    case NOT_CACHED:
        size = sizeof(entry->data);
        if (entry->subkey)
        {
            HKEY key;
            if (!(res = RegOpenKeyExW( intl_key, entry->subkey, 0, KEY_READ, &key )))
            {
                res = RegQueryValueExW( key, entry->value, NULL, NULL, (BYTE *)entry->data, &size );
                RegCloseKey( key );
            }
        }
        else res = RegQueryValueExW( intl_key, entry->value, NULL, NULL, (BYTE *)entry->data, &size );

        if (res)
        {
            entry->status = MISSING;
            break;
        }
        entry->status = CACHED;
        /* fall through */
    case CACHED:
        ret = locale_return_data( entry->data, wcslen(entry->data) + 1, type, buffer, len );
        break;
    case MISSING:
        break;
    }
    RtlLeaveCriticalSection( &locale_section );
    return ret;
}


static int locale_return_string( DWORD pos, LCTYPE type, WCHAR *buffer, int len )
{
    return locale_return_data( locale_strings + pos + 1, locale_strings[pos] + 1, type, buffer, len );
}


static int locale_return_number( UINT val, LCTYPE type, WCHAR *buffer, int len )
{
    int ret;
    WCHAR tmp[80];

    if (!(type & LOCALE_RETURN_NUMBER))
    {
        switch (LOWORD(type))
        {
        case LOCALE_ILANGUAGE:
        case LOCALE_IDEFAULTLANGUAGE:
            ret = swprintf( tmp, ARRAY_SIZE(tmp), L"%04x", val ) + 1;
            break;
        case LOCALE_IDEFAULTEBCDICCODEPAGE:
            ret = swprintf( tmp, ARRAY_SIZE(tmp), L"%03u", val ) + 1;
            break;
        default:
            ret = swprintf( tmp, ARRAY_SIZE(tmp), L"%u", val ) + 1;
            break;
        }
    }
    else ret = sizeof(UINT) / sizeof(WCHAR);

    if (!len) return ret;
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (type & LOCALE_RETURN_NUMBER) memcpy( buffer, &val, sizeof(val) );
    else wcscpy( buffer, tmp );

    return ret;
}


static int locale_return_reg_number( struct registry_entry *entry, LCTYPE type, WCHAR *buffer, int len )
{
    int ret, val;
    WCHAR *end, tmp[80];

    if (type & LOCALE_RETURN_NUMBER)
    {
        ret = locale_return_reg_string( entry, type & ~LOCALE_RETURN_NUMBER, tmp, ARRAY_SIZE( tmp ));
        if (ret == -1) return ret;
        val = wcstol( tmp, &end, 10 );
        if (*end)  /* invalid number */
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        return locale_return_number( val, type, buffer, len );
    }
    return locale_return_reg_string( entry, type, buffer, len );
}


static int locale_return_grouping( DWORD pos, LCTYPE type, WCHAR *buffer, int len )
{
    WORD i, count = locale_strings[pos];
    const WCHAR *str = locale_strings + pos + 1;
    int ret;

    if (type & LOCALE_RETURN_NUMBER)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    ret = 2 * count;
    if (str[count - 1]) ret += 2;  /* for final zero */

    if (!len) return ret;
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        if (!str[i])  /* explicit null termination */
        {
            buffer[-1] = 0;
            return ret;
        }
        *buffer++ = '0' + str[i];
        *buffer++ = ';';
    }
    *buffer++ = '0';
    *buffer = 0;
    return ret;
}


static int locale_return_strarray( DWORD pos, WORD idx, LCTYPE type, WCHAR *buffer, int len )
{
    const DWORD *array = (const DWORD *)(locale_strings + pos + 1);
    WORD count = locale_strings[pos];

    return locale_return_string( idx < count ? array[idx] : 0, type, buffer, len );
}


static int locale_return_strarray_concat( DWORD pos, LCTYPE type, WCHAR *buffer, int len )
{
    WORD i, count = locale_strings[pos];
    const DWORD *array = (const DWORD *)(locale_strings + pos + 1);
    int ret;

    if (type & LOCALE_RETURN_NUMBER)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    for (i = 0, ret = 1; i < count; i++) ret += locale_strings[array[i]];

    if (!len) return ret;
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        memcpy( buffer, locale_strings + array[i] + 1, locale_strings[array[i]] * sizeof(WCHAR) );
        buffer += locale_strings[array[i]];
    }
    *buffer = 0;
    return ret;
}


static int cal_return_number( UINT val, CALTYPE type, WCHAR *buffer, int len, DWORD *value )
{
    WCHAR tmp[12];
    int ret;

    if (type & CAL_RETURN_NUMBER)
    {
        *value = val;
        return sizeof(UINT) / sizeof(WCHAR);
    }
    ret = swprintf( tmp, ARRAY_SIZE(tmp), L"%u", val );
    return locale_return_data( tmp, ret + 1, 0, buffer, len );
}


/* find the first format char in a format string */
static WCHAR *find_format( WCHAR *str, const WCHAR *accept )
{
    for ( ; *str; str++)
    {
        if (*str == '\'')
        {
            if (!(str = wcschr( str + 1, '\'' ))) return NULL;
        }
        else if (wcschr( accept, *str ))
        {
            /* ignore "ddd" and "dddd" */
            if (str[0] != 'd' || str[1] != 'd' || str[2] != 'd') return str;
            str += 2;
            while (str[1] == 'd') str++;
        }
    }
    return NULL;
}


/* replace the separator in a date/time format string */
static WCHAR *locale_replace_separator( WCHAR *buffer, const WCHAR *sep )
{
    UINT pos = 0;
    WCHAR res[80];
    WCHAR *next, *str = find_format( buffer, L"dMyHhms" );

    if (!str) return buffer;
    pos = str - buffer;
    memcpy( res, buffer, pos * sizeof(WCHAR) );
    for (;;)
    {
        res[pos++] = *str++;
        while (str[0] == str[-1]) res[pos++] = *str++;  /* copy repeated chars */
        if (!(next = find_format( str, L"dMyHhms" ))) break;
        wcscpy( res + pos, sep );
        pos += wcslen(sep);
        str = next;
    }
    wcscpy( res + pos, str );
    return wcscpy( buffer, res );
}


/* FIXME: hardcoded, sortname is apparently not available in locale.nls */
static const WCHAR *get_locale_sortname( LCID lcid )
{
    switch (PRIMARYLANGID( lcid ))
    {
    case LANG_CHINESE:
        switch (SORTIDFROMLCID( lcid ))
        {
        case SORT_CHINESE_PRCP:
            switch (SUBLANGID( lcid ))
            {
            case SUBLANG_CHINESE_TRADITIONAL:
            case SUBLANG_CHINESE_HONGKONG:
            case 0x1f:
                return L"Stroke Count";
            default:
                return L"Pronunciation";
            }
        case SORT_CHINESE_UNICODE: return L"Unicode";
        case SORT_CHINESE_PRC: return L"Stroke Count";
        case SORT_CHINESE_BOPOMOFO: return L"Bopomofo";
        case SORT_CHINESE_RADICALSTROKE: return L"Radical/Stroke";
        case 5: return L"Surname";
        }
        break;

    case LANG_GEORGIAN:
        if (SORTIDFROMLCID( lcid ) == SORT_GEORGIAN_MODERN) return L"Modern";
        return L"Traditional";

    case LANG_GERMAN:
        switch (SUBLANGID( lcid ))
        {
        case SUBLANG_NEUTRAL:
        case SUBLANG_DEFAULT:
            if (SORTIDFROMLCID( lcid ) == SORT_GERMAN_PHONE_BOOK) return L"Phone Book (DIN)";
            return L"Dictionary";
        }
        break;

    case LANG_HUNGARIAN:
        if (SORTIDFROMLCID( lcid ) == SORT_HUNGARIAN_TECHNICAL) return L"Technical";
        break;

    case LANG_INVARIANT:
        if (SORTIDFROMLCID( lcid ) == SORT_INVARIANT_MATH) return L"Default";
        return L"Maths Alphanumerics";

    case LANG_JAPANESE:
        switch (SORTIDFROMLCID( lcid ))
        {
        case SORT_JAPANESE_XJIS: return L"XJIS";
        case SORT_JAPANESE_UNICODE: return L"Unicode";
        case SORT_JAPANESE_RADICALSTROKE: return L"Radical/Stroke";
        }
        break;

    case LANG_KOREAN:
        if (SORTIDFROMLCID( lcid ) == SORT_KOREAN_UNICODE) return L"Unicode";
        return L"Dictionary";

    case LANG_SPANISH:
        switch (SUBLANGID( lcid ))
        {
        case SUBLANG_NEUTRAL:
        case SUBLANG_SPANISH_MODERN:
            return L"International";
        case SUBLANG_DEFAULT:
            return L"Traditional";
        }
        break;
    }
    return L"Default";
}


/* get locale information from the locale.nls file */
static int get_locale_info( const NLS_LOCALE_DATA *locale, LCID lcid, LCTYPE type,
                            WCHAR *buffer, int len )
{
    static const WCHAR spermille[] = { 0x2030, 0 };  /* this one seems hardcoded */
    static const BYTE ipossignposn[]    = { 3, 3, 4, 2, 1, 1, 3, 4, 1, 3, 4, 2, 4, 3, 3, 1 };
    static const BYTE inegsignposn[]    = { 0, 3, 4, 2, 0, 1, 3, 4, 1, 3, 4, 2, 4, 3, 0, 0 };
    static const BYTE inegsymprecedes[] = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, };
    const WCHAR *sort;
    WCHAR *str, *end, tmp[80];
    UINT val;
    int ret;

    if (locale != user_locale) type |= LOCALE_NOUSEROVERRIDE;

    switch (LOWORD(type))
    {
    case LOCALE_ILANGUAGE:
        /* return default language for neutral locales */
        val = locale->inotneutral ? locale->ilanguage : locale->idefaultlanguage;
        return locale_return_number( val, type, buffer, len );

    case LOCALE_SLOCALIZEDDISPLAYNAME:
        /* FIXME: localization */
        return locale_return_string( locale->sengdisplayname, type, buffer, len );

    case LOCALE_SABBREVLANGNAME:
        return locale_return_string( locale->sabbrevlangname, type, buffer, len );

    case LOCALE_SNATIVELANGNAME:
        return locale_return_string( locale->snativelangname, type, buffer, len );

    case LOCALE_ICOUNTRY:
        if ((ret = locale_return_reg_number( &entry_icountry, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->icountry, type, buffer, len );

    case LOCALE_SLOCALIZEDCOUNTRYNAME:
        /* FIXME: localization */
        return locale_return_string( locale->sengcountry, type, buffer, len );

    case LOCALE_SABBREVCTRYNAME:
        return locale_return_string( locale->sabbrevctryname, type, buffer, len );

    case LOCALE_SNATIVECTRYNAME:
        return locale_return_string( locale->snativectryname, type, buffer, len );

    case LOCALE_IDEFAULTLANGUAGE:
        return locale_return_number( locale->idefaultlanguage, type, buffer, len );

    case LOCALE_IDEFAULTCOUNTRY:
        return locale_return_number( locale->icountry, type, buffer, len );

    case LOCALE_IDEFAULTCODEPAGE:
        val = locale->idefaultcodepage == CP_UTF8 ? CP_OEMCP : locale->idefaultcodepage;
        return locale_return_number( val, type, buffer, len );

    case LOCALE_SLIST:
        if ((ret = locale_return_reg_string( &entry_slist, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->slist, type, buffer, len );

    case LOCALE_IMEASURE:
        if ((ret = locale_return_reg_number( &entry_imeasure, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->imeasure, type, buffer, len );

    case LOCALE_SDECIMAL:
        if ((ret = locale_return_reg_string( &entry_sdecimal, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->sdecimal, type, buffer, len );

    case LOCALE_STHOUSAND:
        if ((ret = locale_return_reg_string( &entry_sthousand, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->sthousand, type, buffer, len );

    case LOCALE_SGROUPING:
        if ((ret = locale_return_reg_string( &entry_sgrouping, type, buffer, len )) != -1) return ret;
        return locale_return_grouping( locale->sgrouping, type, buffer, len );

    case LOCALE_IDIGITS:
        if ((ret = locale_return_reg_number( &entry_idigits, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->idigits, type, buffer, len );

    case LOCALE_ILZERO:
        if ((ret = locale_return_reg_number( &entry_ilzero, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->ilzero, type, buffer, len );

    case LOCALE_SNATIVEDIGITS:
        if ((ret = locale_return_reg_string( &entry_snativedigits, type, buffer, len )) != -1) return ret;
        return locale_return_strarray_concat( locale->snativedigits, type, buffer, len );

    case LOCALE_SCURRENCY:
        if ((ret = locale_return_reg_string( &entry_scurrency, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->scurrency, type, buffer, len );

    case LOCALE_SINTLSYMBOL:
        if ((ret = locale_return_reg_string( &entry_sintlsymbol, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->sintlsymbol, type, buffer, len );

    case LOCALE_SMONDECIMALSEP:
        if ((ret = locale_return_reg_string( &entry_smondecimalsep, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->smondecimalsep, type, buffer, len );

    case LOCALE_SMONTHOUSANDSEP:
        if ((ret = locale_return_reg_string( &entry_smonthousandsep, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->smonthousandsep, type, buffer, len );

    case LOCALE_SMONGROUPING:
        if ((ret = locale_return_reg_string( &entry_smongrouping, type, buffer, len )) != -1) return ret;
        return locale_return_grouping( locale->smongrouping, type, buffer, len );

    case LOCALE_ICURRDIGITS:
    case LOCALE_IINTLCURRDIGITS:
        if ((ret = locale_return_reg_number( &entry_icurrdigits, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->icurrdigits, type, buffer, len );

    case LOCALE_ICURRENCY:
        if ((ret = locale_return_reg_number( &entry_icurrency, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->icurrency, type, buffer, len );

    case LOCALE_INEGCURR:
        if ((ret = locale_return_reg_number( &entry_inegcurr, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->inegcurr, type, buffer, len );

    case LOCALE_SDATE:
        if (!get_locale_info( locale, lcid, LOCALE_SSHORTDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"dMy" ))) break;
        while (str[1] == str[0]) str++;  /* skip repeated chars */
        if (!(end = find_format( ++str, L"dMy" ))) break;
        *end++ = 0;
        return locale_return_data( str, end - str, type, buffer, len );

    case LOCALE_STIME:
        if (!get_locale_info( locale, lcid, LOCALE_STIMEFORMAT | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"Hhms" ))) break;
        while (str[1] == str[0]) str++;  /* skip repeated chars */
        if (!(end = find_format( ++str, L"Hhms" ))) break;
        *end++ = 0;
        return locale_return_data( str, end - str, type, buffer, len );

    case LOCALE_SSHORTDATE:
        if ((ret = locale_return_reg_string( &entry_sshortdate, type, buffer, len )) != -1) return ret;
        return locale_return_strarray( locale->sshortdate, 0, type, buffer, len );

    case LOCALE_SLONGDATE:
        if ((ret = locale_return_reg_string( &entry_slongdate, type, buffer, len )) != -1) return ret;
        return locale_return_strarray( locale->slongdate, 0, type, buffer, len );

    case LOCALE_IDATE:
        if (!get_locale_info( locale, lcid, LOCALE_SSHORTDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        /* if both year and day are found before month, the last one takes precedence */
        for (val = 0, str = find_format( tmp, L"dMy" ); str; str = find_format( str + 1, L"dMy" ))
        {
            if (*str == 'M') break;
            val = (*str == 'y' ? 2 : 1);
        }
        return locale_return_number( val, type, buffer, len );

    case LOCALE_ILDATE:
        if (!get_locale_info( locale, lcid, LOCALE_SLONGDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        /* if both year and day are found before month, the last one takes precedence */
        for (val = 0, str = find_format( tmp, L"dMy" ); str; str = find_format( str + 1, L"dMy" ))
        {
            if (*str == 'M') break;
            val = (*str == 'y' ? 2 : 1);
        }
        return locale_return_number( val, type, buffer, len );

    case LOCALE_ITIME:
        if (!get_locale_info( locale, lcid, LOCALE_STIMEFORMAT | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"Hh" ))) break;
        return locale_return_number( *str == 'H', type, buffer, len );

    case LOCALE_ICENTURY:
        if (!get_locale_info( locale, lcid, LOCALE_SSHORTDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"y" ))) break;
        return locale_return_number( !wcsncmp( str, L"yyyy", 4 ), type, buffer, len );

    case LOCALE_ITLZERO:
        if (!get_locale_info( locale, lcid, LOCALE_STIMEFORMAT | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"Hh" ))) break;
        return locale_return_number( str[1] == str[0], type, buffer, len );

    case LOCALE_IDAYLZERO:
        if (!get_locale_info( locale, lcid, LOCALE_SSHORTDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"d" ))) break;
        return locale_return_number( str[1] == 'd', type, buffer, len );

    case LOCALE_IMONLZERO:
        if (!get_locale_info( locale, lcid, LOCALE_SSHORTDATE | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"M" ))) break;
        return locale_return_number( str[1] == 'M', type, buffer, len );

    case LOCALE_S1159:
        if ((ret = locale_return_reg_string( &entry_s1159, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->s1159, type, buffer, len );

    case LOCALE_S2359:
        if ((ret = locale_return_reg_string( &entry_s2359, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->s2359, type, buffer, len );

    case LOCALE_SDAYNAME1:
    case LOCALE_SDAYNAME2:
    case LOCALE_SDAYNAME3:
    case LOCALE_SDAYNAME4:
    case LOCALE_SDAYNAME5:
    case LOCALE_SDAYNAME6:
    case LOCALE_SDAYNAME7:
        return locale_return_strarray( locale->sdayname,
                                       LOWORD(type - LOCALE_SDAYNAME1 + 1) % 7, type, buffer, len );

    case LOCALE_SABBREVDAYNAME1:
    case LOCALE_SABBREVDAYNAME2:
    case LOCALE_SABBREVDAYNAME3:
    case LOCALE_SABBREVDAYNAME4:
    case LOCALE_SABBREVDAYNAME5:
    case LOCALE_SABBREVDAYNAME6:
    case LOCALE_SABBREVDAYNAME7:
        return locale_return_strarray( locale->sabbrevdayname,
                                       LOWORD(type - LOCALE_SABBREVDAYNAME1 + 1) % 7, type, buffer, len );

    case LOCALE_SMONTHNAME1:
    case LOCALE_SMONTHNAME2:
    case LOCALE_SMONTHNAME3:
    case LOCALE_SMONTHNAME4:
    case LOCALE_SMONTHNAME5:
    case LOCALE_SMONTHNAME6:
    case LOCALE_SMONTHNAME7:
    case LOCALE_SMONTHNAME8:
    case LOCALE_SMONTHNAME9:
    case LOCALE_SMONTHNAME10:
    case LOCALE_SMONTHNAME11:
    case LOCALE_SMONTHNAME12:
        return locale_return_strarray( ((type & LOCALE_RETURN_GENITIVE_NAMES) && locale->sgenitivemonth) ?
                                       locale->sgenitivemonth : locale->smonthname,
                                       type - LOCALE_SMONTHNAME1, type, buffer, len );

    case LOCALE_SABBREVMONTHNAME1:
    case LOCALE_SABBREVMONTHNAME2:
    case LOCALE_SABBREVMONTHNAME3:
    case LOCALE_SABBREVMONTHNAME4:
    case LOCALE_SABBREVMONTHNAME5:
    case LOCALE_SABBREVMONTHNAME6:
    case LOCALE_SABBREVMONTHNAME7:
    case LOCALE_SABBREVMONTHNAME8:
    case LOCALE_SABBREVMONTHNAME9:
    case LOCALE_SABBREVMONTHNAME10:
    case LOCALE_SABBREVMONTHNAME11:
    case LOCALE_SABBREVMONTHNAME12:
        return locale_return_strarray( ((type & LOCALE_RETURN_GENITIVE_NAMES) && locale->sabbrevgenitivemonth) ?
                                       locale->sabbrevgenitivemonth : locale->sabbrevmonthname,
                                       type - LOCALE_SABBREVMONTHNAME1, type, buffer, len );

    case LOCALE_SPOSITIVESIGN:
        if ((ret = locale_return_reg_string( &entry_spositivesign, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->spositivesign, type, buffer, len );

    case LOCALE_SNEGATIVESIGN:
        if ((ret = locale_return_reg_string( &entry_snegativesign, type, buffer, len )) != -1) return ret;
        return locale_return_string( locale->snegativesign, type, buffer, len );

    case LOCALE_IPOSSIGNPOSN:
        if (!get_locale_info( locale, lcid,
                              LOCALE_INEGCURR | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( ipossignposn[val], type, buffer, len );

    case LOCALE_INEGSIGNPOSN:
        if (!get_locale_info( locale, lcid,
                              LOCALE_INEGCURR | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( inegsignposn[val], type, buffer, len );

    case LOCALE_IPOSSYMPRECEDES:
        if (!get_locale_info( locale, lcid,
                              LOCALE_ICURRENCY | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( !(val & 1), type, buffer, len );

    case LOCALE_IPOSSEPBYSPACE:
        if (!get_locale_info( locale, lcid,
                              LOCALE_ICURRENCY | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( !!(val & 2), type, buffer, len );

    case LOCALE_INEGSYMPRECEDES:
        if (!get_locale_info( locale, lcid,
                              LOCALE_INEGCURR | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( inegsymprecedes[val], type, buffer, len );

    case LOCALE_INEGSEPBYSPACE:
        if (!get_locale_info( locale, lcid,
                              LOCALE_INEGCURR | LOCALE_RETURN_NUMBER | (type & LOCALE_NOUSEROVERRIDE),
                              (WCHAR *)&val, sizeof(val)/sizeof(WCHAR) )) break;
        return locale_return_number( (val >= 8), type, buffer, len );

    case LOCALE_FONTSIGNATURE:
        return locale_return_data( locale_strings + locale->fontsignature + 1,
                                   locale_strings[locale->fontsignature], type, buffer, len );

    case LOCALE_SISO639LANGNAME:
        return locale_return_string( locale->siso639langname, type, buffer, len );

    case LOCALE_SISO3166CTRYNAME:
        return locale_return_string( locale->siso3166ctryname, type, buffer, len );

    case LOCALE_IGEOID:
        return locale_return_number( locale->igeoid, type, buffer, len );

    case LOCALE_SNAME:
        if (SORTIDFROMLCID(lcid))  /* custom sort locale */
        {
            const NLS_LOCALE_LCID_INDEX *entry = find_lcid_entry( lcid & ~0x80000000 );
            if (entry) return locale_return_string( entry->name, type, buffer, len );
        }
        return locale_return_string( locale->sname, type, buffer, len );

    case LOCALE_SDURATION:
        return locale_return_strarray( locale->sduration, 0, type, buffer, len );

    case LOCALE_SKEYBOARDSTOINSTALL:
        return locale_return_string( locale->skeyboardstoinstall, type, buffer, len );

    case LOCALE_SSHORTESTDAYNAME1:
    case LOCALE_SSHORTESTDAYNAME2:
    case LOCALE_SSHORTESTDAYNAME3:
    case LOCALE_SSHORTESTDAYNAME4:
    case LOCALE_SSHORTESTDAYNAME5:
    case LOCALE_SSHORTESTDAYNAME6:
    case LOCALE_SSHORTESTDAYNAME7:
        return locale_return_strarray( locale->sshortestdayname,
                                       LOWORD(type - LOCALE_SSHORTESTDAYNAME1 + 1) % 7, type, buffer, len );

    case LOCALE_SISO639LANGNAME2:
        return locale_return_string( locale->siso639langname2, type, buffer, len );

    case LOCALE_SISO3166CTRYNAME2:
        return locale_return_string( locale->siso3166ctryname2, type, buffer, len );

    case LOCALE_SNAN:
        return locale_return_string( locale->snan, type, buffer, len );

    case LOCALE_SPOSINFINITY:
        return locale_return_string( locale->sposinfinity, type, buffer, len );

    case LOCALE_SNEGINFINITY:
        return locale_return_string( locale->sneginfinity, type, buffer, len );

    case LOCALE_SSCRIPTS:
        return locale_return_string( locale->sscripts, type, buffer, len );

    case LOCALE_SPARENT:
        return locale_return_string( locale->sparent, type, buffer, len );

    case LOCALE_SCONSOLEFALLBACKNAME:
        return locale_return_string( locale->sconsolefallbackname, type, buffer, len );

    case LOCALE_SLOCALIZEDLANGUAGENAME:
        /* FIXME: localization */
        return locale_return_string( locale->senglanguage, type, buffer, len );

    case LOCALE_IREADINGLAYOUT:
        return locale_return_number( locale->ireadinglayout, type, buffer, len );

    case LOCALE_INEUTRAL:
        return locale_return_number( !locale->inotneutral, type, buffer, len );

    case LOCALE_SENGLISHDISPLAYNAME:
        return locale_return_string( locale->sengdisplayname, type, buffer, len );

    case LOCALE_SNATIVEDISPLAYNAME:
        return locale_return_string( locale->snativedisplayname, type, buffer, len );

    case LOCALE_INEGATIVEPERCENT:
        return locale_return_number( locale->inegativepercent, type, buffer, len );

    case LOCALE_IPOSITIVEPERCENT:
        return locale_return_number( locale->ipositivepercent, type, buffer, len );

    case LOCALE_SPERCENT:
        return locale_return_string( locale->spercent, type, buffer, len );

    case LOCALE_SPERMILLE:
        return locale_return_data( spermille, ARRAY_SIZE(spermille), type, buffer, len );

    case LOCALE_SMONTHDAY:
        return locale_return_strarray( locale->smonthday, 0, type, buffer, len );

    case LOCALE_SSHORTTIME:
        if ((ret = locale_return_reg_string( &entry_sshorttime, type, buffer, len )) != -1) return ret;
        return locale_return_strarray( locale->sshorttime, 0, type, buffer, len );

    case LOCALE_SOPENTYPELANGUAGETAG:
        return locale_return_string( locale->sopentypelanguagetag, type, buffer, len );

    case LOCALE_SSORTLOCALE:
        if (SORTIDFROMLCID(lcid))  /* custom sort locale */
        {
            const NLS_LOCALE_LCID_INDEX *entry = find_lcid_entry( lcid & ~0x80000000 );
            if (entry) return locale_return_string( entry->name, type, buffer, len );
        }
        return locale_return_string( locale->ssortlocale, type, buffer, len );

    case LOCALE_SRELATIVELONGDATE:
        return locale_return_string( locale->srelativelongdate, type, buffer, len );

    case 0x007d: /* undocumented */
        return locale_return_number( 0, type, buffer, len );

    case LOCALE_SSHORTESTAM:
        return locale_return_string( locale->sshortestam, type, buffer, len );

    case LOCALE_SSHORTESTPM:
        return locale_return_string( locale->sshortestpm, type, buffer, len );

    case LOCALE_SENGLANGUAGE:
        return locale_return_string( locale->senglanguage, type, buffer, len );

    case LOCALE_SENGCOUNTRY:
        return locale_return_string( locale->sengcountry, type, buffer, len );

    case LOCALE_STIMEFORMAT:
        if ((ret = locale_return_reg_string( &entry_stimeformat, type, buffer, len )) != -1) return ret;
        return locale_return_strarray( locale->stimeformat, 0, type, buffer, len );

    case LOCALE_IDEFAULTANSICODEPAGE:
        val = locale->idefaultansicodepage == CP_UTF8 ? CP_ACP : locale->idefaultansicodepage;
        return locale_return_number( val, type, buffer, len );

    case LOCALE_ITIMEMARKPOSN:
        if (!get_locale_info( locale, lcid, LOCALE_STIMEFORMAT | (type & LOCALE_NOUSEROVERRIDE),
                              tmp, ARRAY_SIZE( tmp ))) break;
        if (!(str = find_format( tmp, L"Hhmst" ))) break;
        return locale_return_number( *str == 't', type, buffer, len );

    case LOCALE_SYEARMONTH:
        if ((ret = locale_return_reg_string( &entry_syearmonth, type, buffer, len )) != -1) return ret;
        return locale_return_strarray( locale->syearmonth, 0, type, buffer, len );

    case LOCALE_SENGCURRNAME:
        return locale_return_string( locale->sengcurrname, type, buffer, len );

    case LOCALE_SNATIVECURRNAME:
        return locale_return_string( locale->snativecurrname, type, buffer, len );

    case LOCALE_ICALENDARTYPE:
        if ((ret = locale_return_reg_number( &entry_icalendartype, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale_strings[locale->scalendartype + 1], type, buffer, len );

    case LOCALE_IPAPERSIZE:
        if ((ret = locale_return_reg_number( &entry_ipapersize, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->ipapersize, type, buffer, len );

    case LOCALE_IOPTIONALCALENDAR:
        return locale_return_number( locale_strings[locale->scalendartype + 2], type, buffer, len );

    case LOCALE_IFIRSTDAYOFWEEK:
        if ((ret = locale_return_reg_number( &entry_ifirstdayofweek, type, buffer, len )) != -1) return ret;
        return locale_return_number( (locale->ifirstdayofweek + 6) % 7, type, buffer, len );

    case LOCALE_IFIRSTWEEKOFYEAR:
        if ((ret = locale_return_reg_number( &entry_ifirstweekofyear, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->ifirstweekofyear, type, buffer, len );

    case LOCALE_SMONTHNAME13:
        return locale_return_strarray( ((type & LOCALE_RETURN_GENITIVE_NAMES) && locale->sgenitivemonth) ?
                                       locale->sgenitivemonth : locale->smonthname,
                                       12, type, buffer, len );

    case LOCALE_SABBREVMONTHNAME13:
        return locale_return_strarray( ((type & LOCALE_RETURN_GENITIVE_NAMES) && locale->sabbrevgenitivemonth) ?
                                       locale->sabbrevgenitivemonth : locale->sabbrevmonthname,
                                       12, type, buffer, len );

    case LOCALE_INEGNUMBER:
        if ((ret = locale_return_reg_number( &entry_inegnumber, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->inegnumber, type, buffer, len );

    case LOCALE_IDEFAULTMACCODEPAGE:
        val = locale->idefaultmaccodepage == CP_UTF8 ? CP_MACCP : locale->idefaultmaccodepage;
        return locale_return_number( val, type, buffer, len );

    case LOCALE_IDEFAULTEBCDICCODEPAGE:
        return locale_return_number( locale->idefaultebcdiccodepage, type, buffer, len );

    case LOCALE_SSORTNAME:
        sort = get_locale_sortname( lcid );
        return locale_return_data( sort, wcslen(sort) + 1, type, buffer, len );

    case LOCALE_IDIGITSUBSTITUTION:
        if ((ret = locale_return_reg_number( &entry_idigitsubstitution, type, buffer, len )) != -1) return ret;
        return locale_return_number( locale->idigitsubstitution, type, buffer, len );
    }
    SetLastError( ERROR_INVALID_FLAGS );
    return 0;
}


/* get calendar information from the locale.nls file */
static int get_calendar_info( const NLS_LOCALE_DATA *locale, CALID id, CALTYPE type,
                              WCHAR *buffer, int len, DWORD *value )
{
    unsigned int i, val = 0;
    const struct calendar *cal;

    if (type & CAL_RETURN_NUMBER)
    {
        if (buffer || len || !value) goto invalid;
    }
    else if (len < 0 || value) goto invalid;

    if (id != CAL_GREGORIAN && type != CAL_ITWODIGITYEARMAX)
    {
        const USHORT *ids = locale_strings + locale->scalendartype;
        for (i = 0; i < ids[0]; i++) if (ids[1 + i] == id) break;
        if (i == ids[0]) goto invalid;
    }
    if (!(cal = get_calendar_data( locale, id ))) goto invalid;

    switch (LOWORD(type))
    {
    case CAL_ICALINTVALUE:
        return cal_return_number( cal->icalintvalue, type, buffer, len, value );

    case CAL_SCALNAME:
        return locale_return_strarray( locale->calnames, id - 1, type, buffer, len );

    case CAL_IYEAROFFSETRANGE:
        if (cal->iyearoffsetrange)
        {
            const DWORD *array = (const DWORD *)(locale_strings + cal->iyearoffsetrange + 1);
            const short *info = (const short *)locale_strings + array[0];
            val = (info[5] < 0) ? -info[5] : info[5] + 1;  /* year zero */
        }
        return cal_return_number( val, type, buffer, len, value );

    case CAL_SERASTRING:
        if (id == CAL_GREGORIAN) return locale_return_string( locale->serastring, type, buffer, len );
        if (cal->iyearoffsetrange)
        {
            const DWORD *array = (const DWORD *)(locale_strings + cal->iyearoffsetrange + 1);
            const short *info = (const short *)locale_strings + array[0];
            val = info[1] - 1;
        }
        return locale_return_strarray( cal->serastring, val, type, buffer, len );

    case CAL_SSHORTDATE:
        val = (id == CAL_GREGORIAN) ? locale->sshortdate : cal->sshortdate;
        return locale_return_strarray( val, 0, type, buffer, len );

    case CAL_SLONGDATE:
        val = (id == CAL_GREGORIAN) ? locale->slongdate : cal->slongdate;
        return locale_return_strarray( val, 0, type, buffer, len );

    case CAL_SDAYNAME1:
    case CAL_SDAYNAME2:
    case CAL_SDAYNAME3:
    case CAL_SDAYNAME4:
    case CAL_SDAYNAME5:
    case CAL_SDAYNAME6:
    case CAL_SDAYNAME7:
        val = (id == CAL_GREGORIAN) ? locale->sdayname : cal->sdayname;
        return locale_return_strarray( val, (LOWORD(type) - CAL_SDAYNAME1 + 1) % 7, type, buffer, len );

    case CAL_SABBREVDAYNAME1:
    case CAL_SABBREVDAYNAME2:
    case CAL_SABBREVDAYNAME3:
    case CAL_SABBREVDAYNAME4:
    case CAL_SABBREVDAYNAME5:
    case CAL_SABBREVDAYNAME6:
    case CAL_SABBREVDAYNAME7:
        val = (id == CAL_GREGORIAN) ? locale->sabbrevdayname : cal->sabbrevdayname;
        return locale_return_strarray( val, (LOWORD(type) - CAL_SABBREVDAYNAME1 + 1) % 7, type, buffer, len );
    case CAL_SMONTHNAME1:
    case CAL_SMONTHNAME2:
    case CAL_SMONTHNAME3:
    case CAL_SMONTHNAME4:
    case CAL_SMONTHNAME5:
    case CAL_SMONTHNAME6:
    case CAL_SMONTHNAME7:
    case CAL_SMONTHNAME8:
    case CAL_SMONTHNAME9:
    case CAL_SMONTHNAME10:
    case CAL_SMONTHNAME11:
    case CAL_SMONTHNAME12:
    case CAL_SMONTHNAME13:
        if (id != CAL_GREGORIAN) val = cal->smonthname;
        else if ((type & CAL_RETURN_GENITIVE_NAMES) && locale->sgenitivemonth) val = locale->sgenitivemonth;
        else val = locale->smonthname;
        return locale_return_strarray( val, LOWORD(type) - CAL_SMONTHNAME1, type, buffer, len );

    case CAL_SABBREVMONTHNAME1:
    case CAL_SABBREVMONTHNAME2:
    case CAL_SABBREVMONTHNAME3:
    case CAL_SABBREVMONTHNAME4:
    case CAL_SABBREVMONTHNAME5:
    case CAL_SABBREVMONTHNAME6:
    case CAL_SABBREVMONTHNAME7:
    case CAL_SABBREVMONTHNAME8:
    case CAL_SABBREVMONTHNAME9:
    case CAL_SABBREVMONTHNAME10:
    case CAL_SABBREVMONTHNAME11:
    case CAL_SABBREVMONTHNAME12:
    case CAL_SABBREVMONTHNAME13:
        if (id != CAL_GREGORIAN) val = cal->sabbrevmonthname;
        else if ((type & CAL_RETURN_GENITIVE_NAMES) && locale->sabbrevgenitivemonth) val = locale->sabbrevgenitivemonth;
        else val = locale->sabbrevmonthname;
        return locale_return_strarray( val, LOWORD(type) - CAL_SABBREVMONTHNAME1, type, buffer, len );

    case CAL_SYEARMONTH:
        val = (id == CAL_GREGORIAN) ? locale->syearmonth : cal->syearmonth;
        return locale_return_strarray( val, 0, type, buffer, len );

    case CAL_ITWODIGITYEARMAX:
        return cal_return_number( cal->itwodigityearmax, type, buffer, len, value );

    case CAL_SSHORTESTDAYNAME1:
    case CAL_SSHORTESTDAYNAME2:
    case CAL_SSHORTESTDAYNAME3:
    case CAL_SSHORTESTDAYNAME4:
    case CAL_SSHORTESTDAYNAME5:
    case CAL_SSHORTESTDAYNAME6:
    case CAL_SSHORTESTDAYNAME7:
        val = (id == CAL_GREGORIAN) ? locale->sshortestdayname : cal->sshortestdayname;
        return locale_return_strarray( val, (LOWORD(type) - CAL_SSHORTESTDAYNAME1 + 1) % 7, type, buffer, len );

    case CAL_SMONTHDAY:
        val = (id == CAL_GREGORIAN) ? locale->smonthday : cal->smonthday;
        return locale_return_strarray( val, 0, type, buffer, len );

    case CAL_SABBREVERASTRING:
        if (id == CAL_GREGORIAN) return locale_return_string( locale->sabbreverastring, type, buffer, len );
        if (cal->iyearoffsetrange)
        {
            const DWORD *array = (const DWORD *)(locale_strings + cal->iyearoffsetrange + 1);
            const short *info = (const short *)locale_strings + array[0];
            val = info[1] - 1;
        }
        return locale_return_strarray( cal->sabbreverastring, val, type, buffer, len );

    case CAL_SRELATIVELONGDATE:
        val = (id == CAL_GREGORIAN) ? locale->srelativelongdate : cal->srelativelongdate;
        return locale_return_string( val, type, buffer, len );

    case CAL_SENGLISHERANAME:
    case CAL_SENGLISHABBREVERANAME:
        /* not supported on Windows */
        break;
    }
    SetLastError( ERROR_INVALID_FLAGS );
    return 0;

invalid:
    SetLastError( ERROR_INVALID_PARAMETER );
    return 0;
}


/* get geo information from the locale.nls file */
static int get_geo_info( const struct geo_id *geo, enum SYSGEOTYPE type,
                         WCHAR *buffer, int len, LANGID lang )
{
    WCHAR tmp[12];
    const WCHAR *str = tmp;
    int ret;

    switch (type)
    {
    case GEO_NATION:
        if (geo->class != GEOCLASS_NATION) return 0;
        /* fall through */
    case GEO_ID:
        swprintf( tmp, ARRAY_SIZE(tmp), L"%u", geo->id );
        break;
    case GEO_ISO_UN_NUMBER:
        swprintf( tmp, ARRAY_SIZE(tmp), L"%03u", geo->uncode );
        break;
    case GEO_PARENT:
        swprintf( tmp, ARRAY_SIZE(tmp), L"%u", geo->parent );
        break;
    case GEO_DIALINGCODE:
        swprintf( tmp, ARRAY_SIZE(tmp), L"%u", geo->dialcode );
        break;
    case GEO_ISO2:
        str = geo->iso2;
        break;
    case GEO_ISO3:
        str = geo->iso3;
        break;
    case GEO_LATITUDE:
        str = geo->latitude;
        break;
    case GEO_LONGITUDE:
        str = geo->longitude;
        break;
    case GEO_CURRENCYCODE:
        str = geo->currcode;
        break;
    case GEO_CURRENCYSYMBOL:
        str = geo->currsymbol;
        break;
    case GEO_RFC1766:
    case GEO_LCID:
    case GEO_FRIENDLYNAME:
    case GEO_OFFICIALNAME:
    case GEO_TIMEZONES:
    case GEO_OFFICIALLANGUAGES:
    case GEO_NAME:
        FIXME( "type %u is not supported\n", type );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    ret = lstrlenW(str) + 1;
    if (!buffer || !len) return ret;

    memcpy( buffer, str, min( ret, len ) * sizeof(WCHAR) );
    if (len < ret) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return len < ret ? 0 : ret;
}


/* update a registry value based on the current user locale info */
static void update_registry_value( UINT type, const WCHAR *subkey, const WCHAR *value )
{
    WCHAR buffer[80];
    UINT len = get_locale_info( user_locale, user_lcid, type, buffer, ARRAY_SIZE(buffer) );
    if (len) RegSetKeyValueW( intl_key, subkey, value, REG_SZ, (BYTE *)buffer, len * sizeof(WCHAR) );
}


/* update all registry values upon user locale change */
static void update_locale_registry(void)
{
    WCHAR buffer[80];
    UINT len;

    len = swprintf( buffer, ARRAY_SIZE(buffer), L"%08x", GetUserDefaultLCID() );
    RegSetValueExW( intl_key, L"Locale", 0, REG_SZ, (BYTE *)buffer, (len + 1) * sizeof(WCHAR) );

#define UPDATE(val,entry) update_registry_value( LOCALE_NOUSEROVERRIDE | (val), (entry).subkey, (entry).value )
    UPDATE( LOCALE_ICALENDARTYPE, entry_icalendartype );
    UPDATE( LOCALE_ICOUNTRY, entry_icountry );
    UPDATE( LOCALE_ICURRDIGITS, entry_icurrdigits );
    UPDATE( LOCALE_ICURRENCY, entry_icurrency );
    UPDATE( LOCALE_IDIGITS, entry_idigits );
    UPDATE( LOCALE_IDIGITSUBSTITUTION, entry_idigitsubstitution );
    UPDATE( LOCALE_IFIRSTDAYOFWEEK, entry_ifirstdayofweek );
    UPDATE( LOCALE_IFIRSTWEEKOFYEAR, entry_ifirstweekofyear );
    UPDATE( LOCALE_ILZERO, entry_ilzero );
    UPDATE( LOCALE_IMEASURE, entry_imeasure );
    UPDATE( LOCALE_INEGCURR, entry_inegcurr );
    UPDATE( LOCALE_INEGNUMBER, entry_inegnumber );
    UPDATE( LOCALE_IPAPERSIZE, entry_ipapersize );
    UPDATE( LOCALE_S1159, entry_s1159 );
    UPDATE( LOCALE_S2359, entry_s2359 );
    UPDATE( LOCALE_SCURRENCY, entry_scurrency );
    UPDATE( LOCALE_SDECIMAL, entry_sdecimal );
    UPDATE( LOCALE_SGROUPING, entry_sgrouping );
    UPDATE( LOCALE_SINTLSYMBOL, entry_sintlsymbol );
    UPDATE( LOCALE_SLIST, entry_slist );
    UPDATE( LOCALE_SLONGDATE, entry_slongdate );
    UPDATE( LOCALE_SMONDECIMALSEP, entry_smondecimalsep );
    UPDATE( LOCALE_SMONGROUPING, entry_smongrouping );
    UPDATE( LOCALE_SMONTHOUSANDSEP, entry_smonthousandsep );
    UPDATE( LOCALE_SNATIVEDIGITS, entry_snativedigits );
    UPDATE( LOCALE_SNEGATIVESIGN, entry_snegativesign );
    UPDATE( LOCALE_SPOSITIVESIGN, entry_spositivesign );
    UPDATE( LOCALE_SSHORTDATE, entry_sshortdate );
    UPDATE( LOCALE_SSHORTTIME, entry_sshorttime );
    UPDATE( LOCALE_STHOUSAND, entry_sthousand );
    UPDATE( LOCALE_STIMEFORMAT, entry_stimeformat );
    UPDATE( LOCALE_SYEARMONTH, entry_syearmonth );
#undef UPDATE
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_IDATE, NULL, L"iDate" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_ITIME, NULL, L"iTime" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_ITIMEMARKPOSN, NULL, L"iTimePrefix" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_ITLZERO, NULL, L"iTLZero" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_SDATE, NULL, L"sDate" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_STIME, NULL, L"sTime" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_SABBREVLANGNAME, NULL, L"sLanguage" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_SCOUNTRY, NULL, L"sCountry" );
    update_registry_value( LOCALE_NOUSEROVERRIDE | LOCALE_SNAME, NULL, L"LocaleName" );
    SetUserGeoID( user_locale->igeoid );
}


/***********************************************************************
 *		init_locale
 */
void init_locale( HMODULE module )
{
    USHORT utf8[2] = { 0, CP_UTF8 };
    USHORT *ansi_ptr, *oem_ptr;
    WCHAR bufferW[LOCALE_NAME_MAX_LENGTH];
    DYNAMIC_TIME_ZONE_INFORMATION timezone;
    const WCHAR *user_locale_name;
    DWORD count;
    SIZE_T size;
    HKEY hkey;

    kernelbase_handle = module;
    load_locale_nls();
    load_sortdefault_nls();

    if (system_lcid == LOCALE_CUSTOM_UNSPECIFIED) system_lcid = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
    system_locale = NlsValidateLocale( &system_lcid, 0 );

    NtQueryDefaultLocale( TRUE, &user_lcid );
    if (!(user_locale = NlsValidateLocale( &user_lcid, 0 )))
    {
        if (GetEnvironmentVariableW( L"WINEUSERLOCALE", bufferW, ARRAY_SIZE(bufferW) ))
            user_locale = get_locale_by_name( bufferW, &user_lcid );
        if (!user_locale) user_locale = system_locale;
    }
    user_lcid = user_locale->ilanguage;
    if (user_lcid == LOCALE_CUSTOM_UNSPECIFIED) user_lcid = LOCALE_CUSTOM_DEFAULT;

    if (GetEnvironmentVariableW( L"WINEUNIXCP", bufferW, ARRAY_SIZE(bufferW) ))
        unix_cp = wcstoul( bufferW, NULL, 10 );

    NtGetNlsSectionPtr( 12, NormalizationC, NULL, (void **)&norm_info, &size );

    ansi_ptr = NtCurrentTeb()->Peb->AnsiCodePageData ? NtCurrentTeb()->Peb->AnsiCodePageData : utf8;
    oem_ptr = NtCurrentTeb()->Peb->OemCodePageData ? NtCurrentTeb()->Peb->OemCodePageData : utf8;
    RtlInitCodePageTable( ansi_ptr, &ansi_cpinfo );
    RtlInitCodePageTable( oem_ptr, &oem_cpinfo );

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Nls",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nls_key, NULL );
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tz_key, NULL );
    RegCreateKeyExW( HKEY_CURRENT_USER, L"Control Panel\\International",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &intl_key, NULL );

    current_locale_sort = get_language_sort( LOCALE_NAME_USER_DEFAULT );

    if (GetDynamicTimeZoneInformation( &timezone ) != TIME_ZONE_ID_INVALID &&
        !RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\TimeZoneInformation",
                          0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        RegSetValueExW( hkey, L"StandardName", 0, REG_SZ, (BYTE *)timezone.StandardName,
                        (lstrlenW(timezone.StandardName) + 1) * sizeof(WCHAR) );
        RegSetValueExW( hkey, L"TimeZoneKeyName", 0, REG_SZ, (BYTE *)timezone.TimeZoneKeyName,
                        (lstrlenW(timezone.TimeZoneKeyName) + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }

    /* Update registry contents if the user locale has changed.
     * This simulates the action of the Windows control panel. */

    user_locale_name = locale_strings + user_locale->sname + 1;
    count = sizeof(bufferW);
    if (!RegQueryValueExW( intl_key, L"LocaleName", NULL, NULL, (BYTE *)bufferW, &count ))
    {
        if (!wcscmp( bufferW, user_locale_name )) return; /* unchanged */
        TRACE( "updating registry, locale changed %s -> %s\n",
               debugstr_w(bufferW), debugstr_w(user_locale_name) );
    }
    else TRACE( "updating registry, locale changed none -> %s\n", debugstr_w(user_locale_name) );

    update_locale_registry();

    if (!RegCreateKeyExW( nls_key, L"Codepage",
                          0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", GetACP() );
        RegSetValueExW( hkey, L"ACP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", GetOEMCP() );
        RegSetValueExW( hkey, L"OEMCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", system_locale->idefaultmaccodepage );
        RegSetValueExW( hkey, L"MACCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
}


static inline WCHAR casemap( const USHORT *table, WCHAR ch )
{
    return ch + table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}


static inline unsigned int casemap_high( const USHORT *table, WCHAR high, WCHAR low )
{
    unsigned int off = table[table[256 + (high - 0xd800)] + ((low >> 5) & 0x1f)] + 2 * (low & 0x1f);
    return 0x10000 + ((high - 0xd800) << 10) + (low - 0xdc00) + MAKELONG( table[off], table[off+1] );
}


static inline BOOL table_has_high_planes( const USHORT *table )
{
    return table[0] >= 0x500;
}


static inline int put_utf16( WCHAR *dst, int pos, int dstlen, unsigned int ch )
{
    if (ch >= 0x10000)
    {
        if (pos < dstlen - 1)
        {
            ch -= 0x10000;
            dst[pos] = 0xd800 | (ch >> 10);
            dst[pos + 1] = 0xdc00 | (ch & 0x3ff);
        }
        return 2;
    }
    if (pos < dstlen) dst[pos] = ch;
    return 1;
}


static inline WORD get_char_type( DWORD type, WCHAR ch )
{
    const BYTE *ptr = sort.ctype_idx + ((const WORD *)sort.ctype_idx)[ch >> 8];
    ptr = sort.ctype_idx + ((const WORD *)ptr)[(ch >> 4) & 0x0f] + (ch & 0x0f);
    return sort.ctypes[*ptr * 3 + type / 2];
}


static inline void map_byterev( const WCHAR *src, int len, WCHAR *dst )
{
    while (len--) *dst++ = RtlUshortByteSwap( *src++ );
}


static int casemap_string( const USHORT *table, const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    if (table_has_high_planes( table ))
    {
        unsigned int ch;
        int pos = 0;

        while (srclen)
        {
            if (srclen > 1 && IS_SURROGATE_PAIR( src[0], src[1] ))
            {
                ch = casemap_high( table, src[0], src[1] );
                src += 2;
                srclen -= 2;
            }
            else
            {
                ch = casemap( table, *src );
                src++;
                srclen--;
            }
            pos += put_utf16( dst, pos, dstlen, ch );
        }
        return pos;
    }
    else
    {
        int pos, ret = srclen;

        for (pos = 0; pos < dstlen && srclen; pos++, src++, srclen--)
            dst[pos] = casemap( table, *src );
        return ret;
    }
}


static union char_weights get_char_weights( WCHAR c, UINT except )
{
    union char_weights ret;

    ret.val = except ? sort.keys[sort.keys[except + (c >> 8)] + (c & 0xff)] : sort.keys[c];
    return ret;
}


static BYTE rol( BYTE val, BYTE count )
{
    return (val << count) | (val >> (8 - count));
}


static BYTE get_char_props( const struct norm_table *info, unsigned int ch )
{
    const BYTE *level1 = (const BYTE *)((const USHORT *)info + info->props_level1);
    const BYTE *level2 = (const BYTE *)((const USHORT *)info + info->props_level2);
    BYTE off = level1[ch / 128];

    if (!off || off >= 0xfb) return rol( off, 5 );
    return level2[(off - 1) * 128 + ch % 128];
}


static const WCHAR *get_decomposition( WCHAR ch, unsigned int *ret_len )
{
    const struct pair { WCHAR src; USHORT dst; } *pairs;
    const USHORT *hash_table = (const USHORT *)norm_info + norm_info->decomp_hash;
    const WCHAR *ret;
    unsigned int i, pos, end, len, hash;

    *ret_len = 1;
    hash = ch % norm_info->decomp_size;
    pos = hash_table[hash];
    if (pos >> 13)
    {
        if (get_char_props( norm_info, ch ) != 0xbf) return NULL;
        ret = (const USHORT *)norm_info + norm_info->decomp_seq + (pos & 0x1fff);
        len = pos >> 13;
    }
    else
    {
        pairs = (const struct pair *)((const USHORT *)norm_info + norm_info->decomp_map);

        /* find the end of the hash bucket */
        for (i = hash + 1; i < norm_info->decomp_size; i++) if (!(hash_table[i] >> 13)) break;
        if (i < norm_info->decomp_size) end = hash_table[i];
        else for (end = pos; pairs[end].src; end++) ;

        for ( ; pos < end; pos++)
        {
            if (pairs[pos].src != (WCHAR)ch) continue;
            ret = (const USHORT *)norm_info + norm_info->decomp_seq + (pairs[pos].dst & 0x1fff);
            len = pairs[pos].dst >> 13;
            break;
        }
        if (pos >= end) return NULL;
    }

    if (len == 7) while (ret[len]) len++;
    if (!ret[0]) len = 0;  /* ignored char */
    *ret_len = len;
    return ret;
}


static WCHAR compose_chars( WCHAR ch1, WCHAR ch2 )
{
    const USHORT *table = (const USHORT *)norm_info + norm_info->comp_hash;
    const WCHAR *chars = (const USHORT *)norm_info + norm_info->comp_seq;
    unsigned int hash, start, end, i;
    WCHAR ch[3];

    hash = (ch1 + 95 * ch2) % norm_info->comp_size;
    start = table[hash];
    end = table[hash + 1];
    while (start < end)
    {
        for (i = 0; i < 3; i++, start++)
        {
            ch[i] = chars[start];
            if (IS_HIGH_SURROGATE( ch[i] )) start++;
        }
        if (ch[0] == ch1 && ch[1] == ch2) return ch[2];
    }
    return 0;
}


static UINT get_locale_codepage( const NLS_LOCALE_DATA *locale, ULONG flags )
{
    UINT ret = locale->idefaultansicodepage;
    if ((flags & LOCALE_USE_CP_ACP) || ret == CP_UTF8) ret = ansi_cpinfo.CodePage;
    return ret;
}


static UINT get_lcid_codepage( LCID lcid, ULONG flags )
{
    UINT ret = ansi_cpinfo.CodePage;

    if (!(flags & LOCALE_USE_CP_ACP) && lcid != system_lcid)
    {
        const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );
        if (locale) ret = locale->idefaultansicodepage;
    }
    return ret;
}


static const CPTABLEINFO *get_codepage_table( UINT codepage )
{
    static const CPTABLEINFO utf7_cpinfo = { CP_UTF7, 5, '?', 0xfffd, '?', '?' };
    static const CPTABLEINFO utf8_cpinfo = { CP_UTF8, 4, '?', 0xfffd, '?', '?' };
    unsigned int i;
    USHORT *ptr;
    SIZE_T size;

    switch (codepage)
    {
    case CP_ACP:
        return &ansi_cpinfo;
    case CP_OEMCP:
        return &oem_cpinfo;
    case CP_MACCP:
        codepage = system_locale->idefaultmaccodepage;
        break;
    case CP_THREAD_ACP:
        codepage = get_lcid_codepage( NtCurrentTeb()->CurrentLocale, 0 );
        break;
    }
    if (codepage == ansi_cpinfo.CodePage) return &ansi_cpinfo;
    if (codepage == oem_cpinfo.CodePage) return &oem_cpinfo;
    if (codepage == CP_UTF8) return &utf8_cpinfo;
    if (codepage == CP_UTF7) return &utf7_cpinfo;

    RtlEnterCriticalSection( &locale_section );

    for (i = 0; i < nb_codepages; i++) if (codepages[i].CodePage == codepage) goto done;

    if (i == ARRAY_SIZE( codepages ))
    {
        RtlLeaveCriticalSection( &locale_section );
        ERR( "too many codepages\n" );
        return NULL;
    }
    if (NtGetNlsSectionPtr( 11, codepage, NULL, (void **)&ptr, &size ))
    {
        RtlLeaveCriticalSection( &locale_section );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    RtlInitCodePageTable( ptr, &codepages[i] );
    nb_codepages++;
done:
    RtlLeaveCriticalSection( &locale_section );
    return &codepages[i];
}


static const WCHAR *get_ligature( WCHAR wc )
{
    int low = 0, high = ARRAY_SIZE( ligatures ) -1;
    while (low <= high)
    {
        int pos = (low + high) / 2;
        if (ligatures[pos][0] < wc) low = pos + 1;
        else if (ligatures[pos][0] > wc) high = pos - 1;
        else return ligatures[pos] + 1;
    }
    return NULL;
}


static NTSTATUS expand_ligatures( const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    int i, len, pos = 0;
    NTSTATUS ret = STATUS_SUCCESS;
    const WCHAR *expand;

    for (i = 0; i < srclen; i++)
    {
        if (!(expand = get_ligature( src[i] )))
        {
            expand = src + i;
            len = 1;
        }
        else len = lstrlenW( expand );

        if (*dstlen && ret == STATUS_SUCCESS)
        {
            if (pos + len <= *dstlen) memcpy( dst + pos, expand, len * sizeof(WCHAR) );
            else ret = STATUS_BUFFER_TOO_SMALL;
        }
        pos += len;
    }
    *dstlen = pos;
    return ret;
}


static NTSTATUS fold_digits( const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    NTSTATUS ret = STATUS_SUCCESS;
    int len = casemap_string( charmaps[CHARMAP_FOLDDIGITS], src, srclen, dst, *dstlen );

    if (*dstlen && *dstlen < len) ret = STATUS_BUFFER_TOO_SMALL;
    *dstlen = len;
    return ret;
}


static NTSTATUS fold_string( DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    NTSTATUS ret;
    WCHAR *tmp;

    switch (flags)
    {
    case MAP_PRECOMPOSED:
        return RtlNormalizeString( NormalizationC, src, srclen, dst, dstlen );
    case MAP_FOLDCZONE:
    case MAP_PRECOMPOSED | MAP_FOLDCZONE:
        return RtlNormalizeString( NormalizationKC, src, srclen, dst, dstlen );
    case MAP_COMPOSITE:
        return RtlNormalizeString( NormalizationD, src, srclen, dst, dstlen );
    case MAP_COMPOSITE | MAP_FOLDCZONE:
        return RtlNormalizeString( NormalizationKD, src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS:
        return fold_digits( src, srclen, dst, dstlen );
    case MAP_EXPAND_LIGATURES:
    case MAP_EXPAND_LIGATURES | MAP_FOLDCZONE:
        return expand_ligatures( src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_FOLDCZONE:
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationKC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationD, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationKD, tmp, srclen, dst, dstlen );
        break;
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS:
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = expand_ligatures( tmp, srclen, dst, dstlen );
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    RtlFreeHeap( GetProcessHeap(), 0, tmp );
    return ret;
}


static int mbstowcs_cpsymbol( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        unsigned char c = src[i];
        dst[i] = (c < 0x20) ? c : c + 0xf000;
    }
    if (len < srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int mbstowcs_utf7( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    static const signed char base64_decoding_table[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
    };

    const char *source_end = src + srclen;
    int offset = 0, pos = 0;
    DWORD byte_pair = 0;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while(0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            src++;
            if (src >= source_end) break;
            if (*src == '-')
            {
                /* just a plus sign escaped as +- */
                OUTPUT( '+' );
                src++;
                continue;
            }

            do
            {
                signed char sextet = *src;
                if (sextet == '-')
                {
                    /* skip over the dash and end base64 decoding
                     * the current, unfinished byte pair is discarded */
                    src++;
                    offset = 0;
                    break;
                }
                if (sextet < 0)
                {
                    /* the next character of src is < 0 and therefore not part of a base64 sequence
                     * the current, unfinished byte pair is NOT discarded in this case
                     * this is probably a bug in Windows */
                    break;
                }
                sextet = base64_decoding_table[sextet];
                if (sextet == -1)
                {
                    /* -1 means that the next character of src is not part of a base64 sequence
                     * in other words, all sextets in this base64 sequence have been processed
                     * the current, unfinished byte pair is discarded */
                    offset = 0;
                    break;
                }

                byte_pair = (byte_pair << 6) | sextet;
                offset += 6;
                if (offset >= 16)
                {
                    /* this byte pair is done */
                    OUTPUT( byte_pair >> (offset - 16) );
                    offset -= 16;
                }
                src++;
            }
            while (src < source_end);
        }
        else
        {
            OUTPUT( (unsigned char)*src );
            src++;
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
}


static int mbstowcs_utf8( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    DWORD reslen;
    NTSTATUS status;

    if (!dstlen) dst = NULL;
    status = RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    else if (!set_ntstatus( status )) reslen = 0;

    return reslen / sizeof(WCHAR);
}


static inline int is_private_use_area_char( WCHAR code )
{
    return (code >= 0xe000 && code <= 0xf8ff);
}


static int check_invalid_chars( const CPTABLEINFO *info, const unsigned char *src, int srclen )
{
    if (info->DBCSOffsets)
    {
        for ( ; srclen; src++, srclen-- )
        {
            USHORT off = info->DBCSOffsets[*src];
            if (off)
            {
                if (srclen == 1) break;  /* partial char, error */
                if (info->DBCSOffsets[off + src[1]] == info->UniDefaultChar &&
                    ((src[0] << 8) | src[1]) != info->TransUniDefaultChar) break;
                src++;
                srclen--;
                continue;
            }
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransUniDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    else
    {
        for ( ; srclen; src++, srclen-- )
        {
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransUniDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    return !!srclen;

}


static int mbstowcs_decompose( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                               WCHAR *dst, int dstlen )
{
    WCHAR ch;
    USHORT off;
    int len;
    const WCHAR *decomp;
    unsigned int decomp_len;

    if (info->DBCSOffsets)
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++, len += decomp_len)
            {
                if ((off = info->DBCSOffsets[*src]))
                {
                    if (srclen > 1 && src[1])
                    {
                        src++;
                        srclen--;
                        ch = info->DBCSOffsets[off + *src];
                    }
                    else ch = info->UniDefaultChar;
                }
                else ch = info->MultiByteTable[*src];
                get_decomposition( ch, &decomp_len );
            }
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++, dst += decomp_len, len -= decomp_len)
        {
            if ((off = info->DBCSOffsets[*src]))
            {
                if (srclen > 1 && src[1])
                {
                    src++;
                    srclen--;
                    ch = info->DBCSOffsets[off + *src];
                }
                else ch = info->UniDefaultChar;
            }
            else ch = info->MultiByteTable[*src];

            if ((decomp = get_decomposition( ch, &decomp_len )))
            {
                if (len < decomp_len) break;
                memcpy( dst, decomp, decomp_len * sizeof(WCHAR) );
            }
            else *dst = ch;
        }
    }
    else
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++, len += decomp_len)
                get_decomposition( info->MultiByteTable[*src], &decomp_len );
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++, dst += decomp_len, len -= decomp_len)
        {
            ch = info->MultiByteTable[*src];
            if ((decomp = get_decomposition( ch, &decomp_len )))
            {
                if (len < decomp_len) break;
                memcpy( dst, decomp, decomp_len * sizeof(WCHAR) );
            }
            else *dst = ch;
        }
    }

    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - len;
}


static int mbstowcs_sbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    const USHORT *table = info->MultiByteTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)  /* buffer too small: fill it up to dstlen and return error */
    {
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle the remaining characters */
    src += srclen;
    dst += srclen;
    switch (srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int mbstowcs_dbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    USHORT off;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
            if (info->DBCSOffsets[*src] && srclen > 1 && src[1]) { src++; srclen--; }
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++, dst++)
    {
        if ((off = info->DBCSOffsets[*src]))
        {
            if (srclen > 1 && src[1])
            {
                src++;
                srclen--;
                *dst = info->DBCSOffsets[off + *src];
            }
            else *dst = info->UniDefaultChar;
        }
        else *dst = info->MultiByteTable[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int mbstowcs_codepage( const CPTABLEINFO *info, DWORD flags, const char *src, int srclen,
                              WCHAR *dst, int dstlen )
{
    CPTABLEINFO local_info;
    const unsigned char *str = (const unsigned char *)src;

    if ((flags & MB_USEGLYPHCHARS) && info->MultiByteTable[256] == 256)
    {
        local_info = *info;
        local_info.MultiByteTable += 257;
        info = &local_info;
    }
    if ((flags & MB_ERR_INVALID_CHARS) && check_invalid_chars( info, str, srclen ))
    {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return 0;
    }

    if (flags & MB_COMPOSITE) return mbstowcs_decompose( info, str, srclen, dst, dstlen );

    if (info->DBCSOffsets)
        return mbstowcs_dbcs( info, str, srclen, dst, dstlen );
    else
        return mbstowcs_sbcs( info, str, srclen, dst, dstlen );
}


static int wcstombs_cpsymbol( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                              const char *defchar, BOOL *used )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        if (src[i] < 0x20) dst[i] = src[i];
        else if (src[i] >= 0xf020 && src[i] < 0xf100) dst[i] = src[i] - 0xf000;
        else
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    if (srclen > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int wcstombs_utf7( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    static const char directly_encodable[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1f */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7a */
    };
#define ENCODABLE(ch) ((ch) <= 0x7a && directly_encodable[(ch)])

    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const WCHAR *source_end = src + srclen;
    int pos = 0;

    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while (0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            OUTPUT( '+' );
            OUTPUT( '-' );
            src++;
        }
        else if (ENCODABLE(*src))
        {
            OUTPUT( *src );
            src++;
        }
        else
        {
            unsigned int offset = 0, byte_pair = 0;

            OUTPUT( '+' );
            while (src < source_end && !ENCODABLE(*src))
            {
                byte_pair = (byte_pair << 16) | *src;
                offset += 16;
                while (offset >= 6)
                {
                    offset -= 6;
                    OUTPUT( base64_encoding_table[(byte_pair >> offset) & 0x3f] );
                }
                src++;
            }
            if (offset)
            {
                /* Windows won't create a padded base64 character if there's no room for the - sign
                 * as well ; this is probably a bug in Windows */
                if (dstlen > 0 && pos + 1 >= dstlen) goto overflow;
                byte_pair <<= (6 - offset);
                OUTPUT( base64_encoding_table[byte_pair & 0x3f] );
            }
            /* Windows always explicitly terminates the base64 sequence
               even though RFC 2152 (page 3, rule 2) does not require this */
            OUTPUT( '-' );
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
#undef ENCODABLE
}


static int wcstombs_utf8( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    DWORD reslen;
    NTSTATUS status;

    if (used) *used = FALSE;
    if (!dstlen) dst = NULL;
    status = RtlUnicodeToUTF8N( dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & WC_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
        if (used) *used = TRUE;
    }
    else if (!set_ntstatus( status )) reslen = 0;
    return reslen;
}


static int wcstombs_sbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const char *table = info->WideCharTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle remaining characters */
    src += srclen;
    dst += srclen;
    switch(srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int wcstombs_dbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const USHORT *table = info->WideCharTable;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; src++, srclen--, i++) if (table[*src] & 0xff00) i++;
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        if (table[*src] & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = table[*src] >> 8;
        }
        *dst++ = (char)table[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static inline int is_valid_sbcs_mapping( const CPTABLEINFO *info, DWORD flags, unsigned int wch )
{
    const unsigned char *table = info->WideCharTable;

    if (wch >= 0x10000) return 0;
    if ((flags & WC_NO_BEST_FIT_CHARS) || table[wch] == info->DefaultChar)
        return (info->MultiByteTable[table[wch]] == wch);
    return 1;
}


static inline int is_valid_dbcs_mapping( const CPTABLEINFO *info, DWORD flags, unsigned int wch )
{
    const unsigned short *table = info->WideCharTable;
    unsigned short ch;

    if (wch >= 0x10000) return 0;
    ch = table[wch];
    if ((flags & WC_NO_BEST_FIT_CHARS) || ch == info->DefaultChar)
    {
        if (ch >> 8) return info->DBCSOffsets[info->DBCSOffsets[ch >> 8] + (ch & 0xff)] == wch;
        return info->MultiByteTable[ch] == wch;
    }
    return 1;
}


static int wcstombs_sbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const char *table = info->WideCharTable;
    const char def = defchar ? *defchar : (char)info->DefaultChar;
    int i;
    BOOL tmp;
    WCHAR wch;
    unsigned int composed;

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
            {
                /* now check if we can use the composed char */
                if (is_valid_sbcs_mapping( info, flags, composed ))
                {
                    /* we have a good mapping, use it */
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }
            if (!*used) *used = !is_valid_sbcs_mapping( info, flags, wch );
        }
        return i;
    }

    for (i = dstlen; srclen && i; dst++, i--, src++, srclen--)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
        {
            /* now check if we can use the composed char */
            if (is_valid_sbcs_mapping( info, flags, composed ))
            {
                /* we have a good mapping, use it */
                *dst = table[composed];
                src++;
                srclen--;
                continue;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                *dst = def;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                continue;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        *dst = table[wch];
        if (!is_valid_sbcs_mapping( info, flags, wch ))
        {
            *dst = def;
            *used = TRUE;
        }
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_dbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const USHORT *table = info->WideCharTable;
    WCHAR wch, defchar_value;
    unsigned int composed;
    unsigned short res;
    BOOL tmp;
    int i;

    if (!defchar[1]) defchar_value = (unsigned char)defchar[0];
    else defchar_value = ((unsigned char)defchar[0] << 8) | (unsigned char)defchar[1];

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        if (!defchar && !used && !(flags & WC_COMPOSITECHECK))
        {
            for (i = 0; srclen; srclen--, src++, i++) if (table[*src] & 0xff00) i++;
            return i;
        }
        for (i = 0; srclen; srclen--, src++, i++)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
            {
                /* now check if we can use the composed char */
                if (is_valid_dbcs_mapping( info, flags, composed ))
                {
                    /* we have a good mapping for the composed char, use it */
                    res = table[composed];
                    if (res & 0xff00) i++;
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    if (defchar_value & 0xff00) i++;
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }

            res = table[wch];
            if (!is_valid_dbcs_mapping( info, flags, wch ))
            {
                res = defchar_value;
                *used = TRUE;
            }
            if (res & 0xff00) i++;
        }
        return i;
    }


    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
        {
            /* now check if we can use the composed char */
            if (is_valid_dbcs_mapping( info, flags, composed ))
            {
                /* we have a good mapping for the composed char, use it */
                res = table[composed];
                src++;
                srclen--;
                goto output_char;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                res = defchar_value;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                goto output_char;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        res = table[wch];
        if (!is_valid_dbcs_mapping( info, flags, wch ))
        {
            res = defchar_value;
            *used = TRUE;
        }

    output_char:
        if (res & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = res >> 8;
        }
        *dst++ = (char)res;
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_codepage( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, int srclen,
                              char *dst, int dstlen, const char *defchar, BOOL *used )
{
    if (flags || defchar || used)
    {
        if (!defchar) defchar = (const char *)&info->DefaultChar;
        if (info->DBCSOffsets)
            return wcstombs_dbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
        else
            return wcstombs_sbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
    }
    if (info->DBCSOffsets)
        return wcstombs_dbcs( info, src, srclen, dst, dstlen );
    else
        return wcstombs_sbcs( info, src, srclen, dst, dstlen );
}


struct sortkey
{
    BYTE *buf;
    BYTE *new_buf;  /* allocated buf if static buf is not large enough */
    UINT  size;     /* buffer size */
    UINT  max;      /* max possible size */
    UINT  len;      /* current key length */
};

static void append_sortkey( struct sortkey *key, BYTE val )
{
    if (key->len >= key->max) return;
    if (key->len >= key->size)
    {
        key->new_buf = RtlAllocateHeap( GetProcessHeap(), 0, key->max );
        if (key->new_buf) memcpy( key->new_buf, key->buf, key->len );
        else key->max = 0;
        key->buf = key->new_buf;
        key->size = key->max;
    }
    key->buf[key->len++] = val;
}

static void reverse_sortkey( struct sortkey *key )
{
    int i;

    for (i = 0; i < key->len / 2; i++)
    {
        BYTE tmp = key->buf[key->len - i - 1];
        key->buf[key->len - i - 1] = key->buf[i];
        key->buf[i] = tmp;
    }
}

static int compare_sortkeys( const struct sortkey *key1, const struct sortkey *key2, BOOL shorter_wins )
{
    int ret = memcmp( key1->buf, key2->buf, min( key1->len, key2->len ));
    if (!ret) ret = shorter_wins ? key2->len - key1->len : key1->len - key2->len;
    return ret;
}

static void append_normal_weights( const struct sortguid *sortid, struct sortkey *key_primary,
                                   struct sortkey *key_diacritic, struct sortkey *key_case,
                                   union char_weights weights, DWORD flags )
{
    append_sortkey( key_primary, weights.script );
    append_sortkey( key_primary, weights.primary );

    if ((weights.script >= SCRIPT_PUA_FIRST && weights.script <= SCRIPT_PUA_LAST) ||
        ((sortid->flags & FLAG_HAS_3_BYTE_WEIGHTS) &&
         (weights.script >= SCRIPT_CJK_FIRST && weights.script <= SCRIPT_CJK_LAST)))
    {
        append_sortkey( key_primary, weights.diacritic );
        append_sortkey( key_case, weights._case );
        return;
    }
    if (weights.script <= SCRIPT_ARABIC && weights.script != SCRIPT_HEBREW)
    {
        if (flags & LINGUISTIC_IGNOREDIACRITIC) weights.diacritic = 2;
        if (flags & LINGUISTIC_IGNORECASE) weights._case = 2;
    }
    append_sortkey( key_diacritic, weights.diacritic );
    append_sortkey( key_case, weights._case );
}

static void append_nonspace_weights( struct sortkey *key, union char_weights weights, DWORD flags )
{
    if (flags & LINGUISTIC_IGNOREDIACRITIC) weights.diacritic = 2;
    if (key->len) key->buf[key->len - 1] += weights.diacritic;
    else append_sortkey( key, weights.diacritic );
}

static void append_expansion_weights( const struct sortguid *sortid, struct sortkey *key_primary,
                                      struct sortkey *key_diacritic, struct sortkey *key_case,
                                      union char_weights weights, DWORD flags, BOOL is_compare )
{
    /* sortkey and comparison behave differently here */
    if (is_compare)
    {
        if (weights.script == SCRIPT_UNSORTABLE) return;
        if (weights.script == SCRIPT_NONSPACE_MARK)
        {
            append_nonspace_weights( key_diacritic, weights, flags );
            return;
        }
    }
    append_normal_weights( sortid, key_primary, key_diacritic, key_case, weights, flags );
}

static const UINT *find_compression( const WCHAR *src, const WCHAR *table, int count, int len )
{
    int elem_size = compression_size( len ), min = 0, max = count - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        int res = wcsncmp( src, table + pos * elem_size, len );
        if (!res) return (UINT *)(table + (pos + 1) * elem_size) - 1;
        if (res > 0) min = pos + 1;
        else max = pos - 1;
    }
    return NULL;
}

/* find a compression for a char sequence */
/* return the number of extra chars to skip */
static int get_compression_weights( UINT compression, const WCHAR *compr_tables[8],
                                    const WCHAR *src, int srclen, union char_weights *weights )
{
    const struct sort_compression *compr = sort.compressions + compression;
    const UINT *ret;
    BYTE size = weights->_case & CASE_COMPR_6;
    int i, maxlen = 1;

    if (compression >= sort.compr_count) return 0;
    if (size == CASE_COMPR_6) maxlen = 8;
    else if (size == CASE_COMPR_4) maxlen = 5;
    else if (size == CASE_COMPR_2) maxlen = 3;
    maxlen = min( maxlen, srclen );
    for (i = 0; i < maxlen; i++) if (src[i] < compr->minchar || src[i] > compr->maxchar) break;
    maxlen = i;
    if (!compr_tables[0])
    {
        compr_tables[0] = sort.compr_data + compr->offset;
        for (i = 1; i < 8; i++)
            compr_tables[i] = compr_tables[i - 1] + compr->len[i - 1] * compression_size( i + 1 );
    }
    for (i = maxlen - 2; i >= 0; i--)
    {
        if (!(ret = find_compression( src, compr_tables[i], compr->len[i], i + 2 ))) continue;
        weights->val = *ret;
        return i + 1;
    }
    return 0;
}

/* get the zero digit for the digit character range that contains 'ch' */
static WCHAR get_digit_zero_char( WCHAR ch )
{
    static const WCHAR zeroes[] =
    {
        0x0030, 0x0660, 0x06f0, 0x0966, 0x09e6, 0x0a66, 0x0ae6, 0x0b66, 0x0be6, 0x0c66,
        0x0ce6, 0x0d66, 0x0e50, 0x0ed0, 0x0f20, 0x1040, 0x1090, 0x17e0, 0x1810, 0x1946,
        0x1bb0, 0x1c40, 0x1c50, 0xa620, 0xa8d0, 0xa900, 0xaa50, 0xff10
    };
    int min = 0, max = ARRAY_SIZE( zeroes ) - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (zeroes[pos] <= ch && zeroes[pos] + 9 >= ch) return zeroes[pos];
        if (zeroes[pos] < ch) min = pos + 1;
        else max = pos - 1;
    }
    return 0;
}

/* append weights for digits when using SORT_DIGITSASNUMBERS */
/* return the number of extra chars to skip */
static int append_digit_weights( struct sortkey *key, const WCHAR *src, UINT srclen )
{
    UINT i, zero, len, lzero;
    BYTE val, values[19];

    if (!(zero = get_digit_zero_char( *src ))) return -1;

    values[0] = *src - zero;
    for (len = 1; len < ARRAY_SIZE(values) && len < srclen; len++)
    {
        if (src[len] < zero || src[len] > zero + 9) break;
        values[len] = src[len] - zero;
    }
    for (lzero = 0; lzero < len; lzero++) if (values[lzero]) break;

    append_sortkey( key, SCRIPT_DIGIT );
    append_sortkey( key, 2 );
    append_sortkey( key, 2 + len - lzero );
    for (i = lzero, val = 2; i < len; i++)
    {
        if ((len - i) % 2) append_sortkey( key, (val << 4) + values[i] + 2 );
        else val = values[i] + 2;
    }
    append_sortkey( key, 0xfe - lzero );
    return len - 1;
}

/* append the extra weights for kana prolonged sound / repeat marks */
static int append_extra_kana_weights( struct sortkey keys[4], const WCHAR *src, int pos, UINT except,
                                      BYTE case_mask, union char_weights *weights )
{
    BYTE extra1 = 3, case_weight = weights->_case;

    if (weights->primary <= 1)
    {
        while (pos > 0)
        {
            union char_weights prev = get_char_weights( src[--pos], except );
            if (prev.script == SCRIPT_UNSORTABLE || prev.script == SCRIPT_NONSPACE_MARK) continue;
            if (prev.script == SCRIPT_EXPANSION) return 0;
            if (prev.script != SCRIPT_EASTASIA_SPECIAL)
            {
                *weights = prev;
                return 1;
            }
            if (prev.primary <= 1) continue;

            case_weight = prev._case & case_mask;
            if (weights->primary == 1)  /* prolonged sound mark */
            {
                prev.primary &= 0x87;
                case_weight &= ~CASE_FULLWIDTH;
                case_weight |= weights->_case & CASE_FULLWIDTH;
            }
            extra1 = 4 + weights->primary;
            weights->primary = prev.primary;
            goto done;
        }
        return 0;
    }
done:
    append_sortkey( &keys[0], 0xc4 | (case_weight & CASE_FULLSIZE) );
    append_sortkey( &keys[1], extra1 );
    append_sortkey( &keys[2], 0xc4 | (case_weight & CASE_KATAKANA) );
    append_sortkey( &keys[3], 0xc4 | (case_weight & CASE_FULLWIDTH) );
    weights->script = SCRIPT_KANA;
    return 1;
}


#define HANGUL_SBASE  0xac00
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28

static int append_hangul_weights( struct sortkey *key, const WCHAR *src, int srclen, UINT except )
{
    int leading_idx = 0x115f - 0x1100;  /* leading filler */
    int vowel_idx = 0x1160 - 0x1100;  /* vowel filler */
    int trailing_idx = -1;
    BYTE leading_off, vowel_off, trailing_off;
    union char_weights weights;
    WCHAR composed;
    BYTE filler_mask = 0;
    int pos = 0;

    /* leading */
    if (src[pos] >= 0x1100 && src[pos] <= 0x115f) leading_idx = src[pos++] - 0x1100;
    else if (src[pos] >= 0xa960 && src[pos] <= 0xa97c) leading_idx = src[pos++] - (0xa960 - 0x100);

    /* vowel */
    if (srclen > pos)
    {
        if (src[pos] >= 0x1160 && src[pos] <= 0x11a7) vowel_idx = src[pos++] - 0x1100;
        else if (src[pos] >= 0xd7b0 && src[pos] <= 0xd7c6) vowel_idx = src[pos++] - (0xd7b0 - 0x11d);
    }

    /* trailing */
    if (srclen > pos)
    {
        if (src[pos] >= 0x11a8 && src[pos] <= 0x11ff) trailing_idx = src[pos++] - 0x1100;
        else if (src[pos] >= 0xd7cb && src[pos] <= 0xd7fb) trailing_idx = src[pos++] - (0xd7cb - 0x134);
    }

    if (!sort.jamo[leading_idx].is_old && !sort.jamo[vowel_idx].is_old &&
        (trailing_idx == -1 || !sort.jamo[trailing_idx].is_old))
    {
        /* not old Hangul, only use leading char; vowel and trailing will be handled in the next pass */
        pos = 1;
        vowel_idx = 0x1160 - 0x1100;
        trailing_idx = -1;
    }

    leading_off = max( sort.jamo[leading_idx].leading, sort.jamo[vowel_idx].leading );
    vowel_off = max( sort.jamo[leading_idx].vowel, sort.jamo[vowel_idx].vowel );
    trailing_off = max( sort.jamo[leading_idx].trailing, sort.jamo[vowel_idx].trailing );
    if (trailing_idx != -1) trailing_off = max( trailing_off, sort.jamo[trailing_idx].trailing );
    composed = HANGUL_SBASE + (leading_off * HANGUL_VCOUNT + vowel_off) * HANGUL_TCOUNT + trailing_off;

    if (leading_idx == 0x115f - 0x1100 || vowel_idx == 0x1160 - 0x1100)
    {
        filler_mask = 0x80;
        composed--;
    }
    if (composed < HANGUL_SBASE) composed = 0x3260;

    weights = get_char_weights( composed, except );
    append_sortkey( key, weights.script );
    append_sortkey( key, weights.primary );
    append_sortkey( key, 0xff );
    append_sortkey( key, sort.jamo[leading_idx].weight | filler_mask );
    append_sortkey( key, 0xff );
    append_sortkey( key, sort.jamo[vowel_idx].weight );
    append_sortkey( key, 0xff );
    append_sortkey( key, trailing_idx != -1 ? sort.jamo[trailing_idx].weight : 2 );
    return pos - 1;
}

/* put one of the elements of a sortkey into the dst buffer */
static int put_sortkey( BYTE *dst, int dstlen, int pos, const struct sortkey *key, BYTE terminator )
{
    if (dstlen > pos + key->len)
    {
        memcpy( dst + pos, key->buf, key->len );
        dst[pos + key->len] = terminator;
    }
    return pos + key->len + 1;
}


struct sortkey_state
{
    struct sortkey         key_primary;
    struct sortkey         key_diacritic;
    struct sortkey         key_case;
    struct sortkey         key_special;
    struct sortkey         key_extra[4];
    UINT                   primary_pos;
    BYTE                   buffer[3 * 128];
};

static void init_sortkey_state( struct sortkey_state *s, DWORD flags, UINT srclen,
                                BYTE *primary_buf, UINT primary_size )
{
    /* buffer for secondary weights */
    BYTE *secondary_buf = s->buffer;
    UINT secondary_size;

    memset( s, 0, offsetof( struct sortkey_state, buffer ));

    s->key_primary.buf  = primary_buf;
    s->key_primary.size = primary_size;

    if (!(flags & NORM_IGNORENONSPACE))  /* reserve space for diacritics */
    {
        secondary_size = sizeof(s->buffer) / 3;
        s->key_diacritic.buf = secondary_buf;
        s->key_diacritic.size = secondary_size;
        secondary_buf += secondary_size;
    }
    else secondary_size = sizeof(s->buffer) / 2;

    s->key_case.buf = secondary_buf;
    s->key_case.size = secondary_size;
    s->key_special.buf = secondary_buf + secondary_size;
    s->key_special.size = secondary_size;

    s->key_primary.max = srclen * 8;
    s->key_case.max = srclen * 3;
    s->key_special.max = srclen * 4;
    s->key_extra[2].max = s->key_extra[3].max = srclen;
    if (!(flags & NORM_IGNORENONSPACE))
    {
        s->key_diacritic.max = srclen * 3;
        s->key_extra[0].max = s->key_extra[1].max = srclen;
    }
}

static BOOL remove_unneeded_weights( const struct sortguid *sortid, struct sortkey_state *s )
{
    const BYTE ignore[4] = { 0xc4 | CASE_FULLSIZE, 0x03, 0xc4 | CASE_KATAKANA, 0xc4 | CASE_FULLWIDTH };
    int i, j;

    if (sortid->flags & FLAG_REVERSEDIACRITICS) reverse_sortkey( &s->key_diacritic );

    for (i = s->key_diacritic.len; i > 0; i--) if (s->key_diacritic.buf[i - 1] > 2) break;
    s->key_diacritic.len = i;

    for (i = s->key_case.len; i > 0; i--) if (s->key_case.buf[i - 1] > 2) break;
    s->key_case.len = i;

    if (!s->key_extra[2].len) return FALSE;

    for (i = 0; i < 4; i++)
    {
        for (j = s->key_extra[i].len; j > 0; j--) if (s->key_extra[i].buf[j - 1] != ignore[i]) break;
        s->key_extra[i].len = j;
    }
    return TRUE;
}

static void free_sortkey_state( struct sortkey_state *s )
{
    RtlFreeHeap( GetProcessHeap(), 0, s->key_primary.new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_diacritic.new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_case.new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_special.new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_extra[0].new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_extra[1].new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_extra[2].new_buf );
    RtlFreeHeap( GetProcessHeap(), 0, s->key_extra[3].new_buf );
}

static int append_weights( const struct sortguid *sortid, DWORD flags,
                           const WCHAR *src, int srclen, int pos, BYTE case_mask, UINT except,
                           const WCHAR *compr_tables[8], struct sortkey_state *s, BOOL is_compare )
{
    union char_weights weights = get_char_weights( src[pos], except );
    WCHAR idx = (weights.val >> 16) & ~(CASE_COMPR_6 << 8);  /* expansion index */
    int ret = 1;

    if (weights._case & CASE_COMPR_6)
        ret += get_compression_weights( sortid->compr, compr_tables, src + pos, srclen - pos, &weights );

    weights._case &= case_mask;

    switch (weights.script)
    {
    case SCRIPT_UNSORTABLE:
        break;

    case SCRIPT_NONSPACE_MARK:
        append_nonspace_weights( &s->key_diacritic, weights, flags );
        break;

    case SCRIPT_EXPANSION:
        while (weights.script == SCRIPT_EXPANSION)
        {
            weights = get_char_weights( sort.expansions[idx].exp[0], except );
            weights._case &= case_mask;
            append_expansion_weights( sortid, &s->key_primary, &s->key_diacritic,
                                      &s->key_case, weights, flags, is_compare );
            weights = get_char_weights( sort.expansions[idx].exp[1], except );
            idx = weights.val >> 16;
            weights._case &= case_mask;
        }
        append_expansion_weights( sortid, &s->key_primary, &s->key_diacritic,
                                  &s->key_case, weights, flags, is_compare );
        break;

    case SCRIPT_EASTASIA_SPECIAL:
        if (!append_extra_kana_weights( s->key_extra, src, pos, except, case_mask, &weights ))
        {
            append_sortkey( &s->key_primary, 0xff );
            append_sortkey( &s->key_primary, 0xff );
            break;
        }
        weights._case = 2;
        append_normal_weights( sortid, &s->key_primary, &s->key_diacritic, &s->key_case, weights, flags );
        break;

    case SCRIPT_JAMO_SPECIAL:
        ret += append_hangul_weights( &s->key_primary, src + pos, srclen - pos, except );
        append_sortkey( &s->key_diacritic, 2 );
        append_sortkey( &s->key_case, 2 );
        break;

    case SCRIPT_EXTENSION_A:
        append_sortkey( &s->key_primary, 0xfd );
        append_sortkey( &s->key_primary, 0xff );
        append_sortkey( &s->key_primary, weights.primary );
        append_sortkey( &s->key_primary, weights.diacritic );
        append_sortkey( &s->key_diacritic, 2 );
        append_sortkey( &s->key_case, 2 );
        break;

    case SCRIPT_PUNCTUATION:
        if (flags & NORM_IGNORESYMBOLS) break;
        if (!(flags & SORT_STRINGSORT))
        {
            short len = -((s->key_primary.len + s->primary_pos) / 2) - 1;
            if (flags & LINGUISTIC_IGNORECASE) weights._case = 2;
            if (flags & LINGUISTIC_IGNOREDIACRITIC) weights.diacritic = 2;
            append_sortkey( &s->key_special, len >> 8 );
            append_sortkey( &s->key_special, len & 0xff );
            append_sortkey( &s->key_special, weights.primary );
            append_sortkey( &s->key_special, weights._case | (weights.diacritic << 3) );
            break;
        }
        /* fall through */
    case SCRIPT_SYMBOL_1:
    case SCRIPT_SYMBOL_2:
    case SCRIPT_SYMBOL_3:
    case SCRIPT_SYMBOL_4:
    case SCRIPT_SYMBOL_5:
    case SCRIPT_SYMBOL_6:
        if (flags & NORM_IGNORESYMBOLS) break;
        append_sortkey( &s->key_primary, weights.script );
        append_sortkey( &s->key_primary, weights.primary );
        append_sortkey( &s->key_diacritic, weights.diacritic );
        append_sortkey( &s->key_case, weights._case );
        break;

    case SCRIPT_DIGIT:
        if (flags & SORT_DIGITSASNUMBERS)
        {
            int len = append_digit_weights( &s->key_primary, src + pos, srclen - pos );
            if (len >= 0)
            {
                ret += len;
                append_sortkey( &s->key_diacritic, weights.diacritic );
                append_sortkey( &s->key_case, weights._case );
                break;
            }
        }
        /* fall through */
    default:
        append_normal_weights( sortid, &s->key_primary, &s->key_diacritic, &s->key_case, weights, flags );
        break;
    }

    return ret;
}

/* implementation of LCMAP_SORTKEY */
static int get_sortkey( const struct sortguid *sortid, DWORD flags,
                        const WCHAR *src, int srclen, BYTE *dst, int dstlen )
{
    struct sortkey_state s;
    BYTE primary_buf[256];
    int ret = 0, pos = 0;
    BOOL have_extra;
    BYTE case_mask = 0x3f;
    UINT except = sortid->except;
    const WCHAR *compr_tables[8];

    compr_tables[0] = NULL;
    if (flags & NORM_IGNORECASE) case_mask &= ~(CASE_UPPER | CASE_SUBSCRIPT);
    if (flags & NORM_IGNOREWIDTH) case_mask &= ~CASE_FULLWIDTH;
    if (flags & NORM_IGNOREKANATYPE) case_mask &= ~CASE_KATAKANA;
    if ((flags & NORM_LINGUISTIC_CASING) && except && sortid->ling_except) except = sortid->ling_except;

    init_sortkey_state( &s, flags, srclen, primary_buf, sizeof(primary_buf) );

    while (pos < srclen)
        pos += append_weights( sortid, flags, src, srclen, pos, case_mask, except, compr_tables, &s, FALSE );

    have_extra = remove_unneeded_weights( sortid, &s );

    ret = put_sortkey( dst, dstlen, ret, &s.key_primary, 0x01 );
    ret = put_sortkey( dst, dstlen, ret, &s.key_diacritic, 0x01 );
    ret = put_sortkey( dst, dstlen, ret, &s.key_case, 0x01 );

    if (have_extra)
    {
        ret = put_sortkey( dst, dstlen, ret, &s.key_extra[0], 0xff );
        ret = put_sortkey( dst, dstlen, ret, &s.key_extra[1], 0x02 );
        ret = put_sortkey( dst, dstlen, ret, &s.key_extra[2], 0xff );
        ret = put_sortkey( dst, dstlen, ret, &s.key_extra[3], 0xff );
    }
    if (dstlen > ret) dst[ret] = 0x01;
    ret++;

    ret = put_sortkey( dst, dstlen, ret, &s.key_special, 0 );

    free_sortkey_state( &s );

    if (dstlen && dstlen < ret)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    if (flags & LCMAP_BYTEREV)
        map_byterev( (WCHAR *)dst, min( ret, dstlen ) / sizeof(WCHAR), (WCHAR *)dst );
    return ret;
}


/* implementation of CompareStringEx */
static int compare_string( const struct sortguid *sortid, DWORD flags,
                           const WCHAR *src1, int srclen1, const WCHAR *src2, int srclen2 )
{
    struct sortkey_state s1;
    struct sortkey_state s2;
    BYTE primary1[32];
    BYTE primary2[32];
    int i, ret, len, pos1 = 0, pos2 = 0;
    BOOL have_extra1, have_extra2;
    BYTE case_mask = 0x3f;
    UINT except = sortid->except;
    const WCHAR *compr_tables[8];

    compr_tables[0] = NULL;
    if (flags & NORM_IGNORECASE) case_mask &= ~(CASE_UPPER | CASE_SUBSCRIPT);
    if (flags & NORM_IGNOREWIDTH) case_mask &= ~CASE_FULLWIDTH;
    if (flags & NORM_IGNOREKANATYPE) case_mask &= ~CASE_KATAKANA;
    if ((flags & NORM_LINGUISTIC_CASING) && except && sortid->ling_except) except = sortid->ling_except;

    init_sortkey_state( &s1, flags, srclen1, primary1, sizeof(primary1) );
    init_sortkey_state( &s2, flags, srclen2, primary2, sizeof(primary2) );

    while (pos1 < srclen1 || pos2 < srclen2)
    {
        while (pos1 < srclen1 && !s1.key_primary.len)
            pos1 += append_weights( sortid, flags, src1, srclen1, pos1,
                                    case_mask, except, compr_tables, &s1, TRUE );

        while (pos2 < srclen2 && !s2.key_primary.len)
            pos2 += append_weights( sortid, flags, src2, srclen2, pos2,
                                    case_mask, except, compr_tables, &s2, TRUE );

        if (!(len = min( s1.key_primary.len, s2.key_primary.len ))) break;
        if ((ret = memcmp( primary1, primary2, len ))) goto done;
        memmove( primary1, primary1 + len, s1.key_primary.len - len );
        memmove( primary2, primary2 + len, s2.key_primary.len - len );
        s1.key_primary.len -= len;
        s2.key_primary.len -= len;
        s1.primary_pos += len;
        s2.primary_pos += len;
    }

    if ((ret = s1.key_primary.len - s2.key_primary.len)) goto done;

    have_extra1 = remove_unneeded_weights( sortid, &s1 );
    have_extra2 = remove_unneeded_weights( sortid, &s2 );

    if ((ret = compare_sortkeys( &s1.key_diacritic, &s2.key_diacritic, FALSE ))) goto done;
    if ((ret = compare_sortkeys( &s1.key_case, &s2.key_case, FALSE ))) goto done;

    if (have_extra1 && have_extra2)
    {
        for (i = 0; i < 4; i++)
            if ((ret = compare_sortkeys( &s1.key_extra[i], &s2.key_extra[i], i != 1 ))) goto done;
    }
    else if ((ret = have_extra1 - have_extra2)) goto done;

    ret = compare_sortkeys( &s1.key_special, &s2.key_special, FALSE );

done:
    free_sortkey_state( &s1 );
    free_sortkey_state( &s2 );
    return ret;
}


/* implementation of FindNLSStringEx */
static int find_substring( const struct sortguid *sortid, DWORD flags, const WCHAR *src, int srclen,
                           const WCHAR *value, int valuelen, int *reslen )
{
    struct sortkey_state s;
    struct sortkey_state val;
    BYTE primary[32];
    BYTE primary_val[256];
    int i, start, len, found = -1, foundlen = 0, pos = 0;
    BOOL have_extra, have_extra_val;
    BYTE case_mask = 0x3f;
    UINT except = sortid->except;
    const WCHAR *compr_tables[8];

    compr_tables[0] = NULL;
    if (flags & NORM_IGNORECASE) case_mask &= ~(CASE_UPPER | CASE_SUBSCRIPT);
    if (flags & NORM_IGNOREWIDTH) case_mask &= ~CASE_FULLWIDTH;
    if (flags & NORM_IGNOREKANATYPE) case_mask &= ~CASE_KATAKANA;
    if ((flags & NORM_LINGUISTIC_CASING) && except && sortid->ling_except) except = sortid->ling_except;

    init_sortkey_state( &s, flags, srclen, primary, sizeof(primary) );

    /* build the value sortkey just once */
    init_sortkey_state( &val, flags, valuelen, primary_val, sizeof(primary_val) );
    while (pos < valuelen)
        pos += append_weights( sortid, flags, value, valuelen, pos,
                               case_mask, except, compr_tables, &val, TRUE );
    have_extra_val = remove_unneeded_weights( sortid, &val );

    for (start = 0; start < srclen; start++)
    {
        pos = start;
        for (len = start + 1; len <= srclen; len++)
        {
            while (pos < len && s.primary_pos <= val.key_primary.len)
            {
                while (pos < len && !s.key_primary.len)
                    pos += append_weights( sortid, flags, src, srclen, pos,
                                           case_mask, except, compr_tables, &s, TRUE );

                if (s.primary_pos + s.key_primary.len > val.key_primary.len ||
                    memcmp( primary, val.key_primary.buf + s.primary_pos, s.key_primary.len ))
                {
                    len = srclen + 1;
                    goto next;
                }
                s.primary_pos += s.key_primary.len;
                s.key_primary.len = 0;
            }
            if (s.primary_pos < val.key_primary.len) continue;

            have_extra = remove_unneeded_weights( sortid, &s );
            if (compare_sortkeys( &s.key_diacritic, &val.key_diacritic, FALSE )) goto next;
            if (compare_sortkeys( &s.key_case, &val.key_case, FALSE )) goto next;

            if (have_extra && have_extra_val)
            {
                for (i = 0; i < 4; i++)
                    if (compare_sortkeys( &s.key_extra[i], &val.key_extra[i], i != 1 )) goto next;
            }
            else if (have_extra || have_extra_val) goto next;

            if (compare_sortkeys( &s.key_special, &val.key_special, FALSE )) goto next;

            found = start;
            foundlen = pos - start;
            len = srclen;  /* no need to continue checking longer strings */

        next:
            /* reset state */
            s.key_primary.len = s.key_diacritic.len = s.key_case.len = s.key_special.len = 0;
            s.key_extra[0].len = s.key_extra[1].len = s.key_extra[2].len = s.key_extra[3].len = 0;
            s.primary_pos = 0;
            pos = start;
        }
        if (flags & FIND_STARTSWITH) break;
        if (flags & FIND_FROMSTART && found != -1) break;
    }

    if (found != -1)
    {
        if ((flags & FIND_ENDSWITH) && found + foundlen != srclen) found = -1;
        else if (reslen) *reslen = foundlen;
    }
    free_sortkey_state( &s );
    free_sortkey_state( &val );
    return found;
}


/* map buffer to full-width katakana */
static int map_to_fullwidth( const USHORT *table, const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    int pos, len;

    for (pos = 0; srclen; pos++, src += len, srclen -= len)
    {
        unsigned int wch = casemap( charmaps[CHARMAP_FULLWIDTH], *src );

        len = 1;
        if (srclen > 1)
        {
            if (table_has_high_planes( charmaps[CHARMAP_FULLWIDTH] ) && IS_SURROGATE_PAIR( src[0], src[1] ))
            {
                len = 2;
                wch = casemap_high( charmaps[CHARMAP_FULLWIDTH], src[0], src[1] );
                if (wch >= 0x10000)
                {
                    put_utf16( dst, pos, dstlen, wch );
                    pos++;
                    continue;
                }
            }
            else if (src[1] == 0xff9e)  /* dakuten (voiced sound) */
            {
                len = 2;
                if ((*src >= 0xff76 && *src <= 0xff84) ||
                    (*src >= 0xff8a && *src <= 0xff8e) ||
                    *src == 0x30fd)
                    wch++;
                else if (*src == 0xff73)
                    wch = 0x30f4; /* KATAKANA LETTER VU */
                else if (*src == 0xff9c)
                    wch = 0x30f7; /* KATAKANA LETTER VA */
                else if (*src == 0x30f0)
                    wch = 0x30f8; /* KATAKANA LETTER VI */
                else if (*src == 0x30f1)
                    wch = 0x30f9; /* KATAKANA LETTER VE */
                else if (*src == 0xff66)
                    wch = 0x30fa; /* KATAKANA LETTER VO */
                else
                    len = 1;
            }
            else if (src[1] == 0xff9f)  /* handakuten (semi-voiced sound) */
            {
                if (*src >= 0xff8a && *src <= 0xff8e)
                {
                    wch += 2;
                    len = 2;
                }
            }
        }

        if (pos < dstlen) dst[pos] = table ? casemap( table, wch ) : wch;
    }
    return pos;
}


static inline int nonspace_ignored( WCHAR ch )
{
    if (get_char_type( CT_CTYPE2, ch ) != C2_OTHERNEUTRAL) return FALSE;
    return (get_char_type( CT_CTYPE3, ch ) & (C3_NONSPACING | C3_DIACRITIC));
}

/* remove ignored chars for NORM_IGNORENONSPACE/NORM_IGNORESYMBOLS */
static int map_remove_ignored( DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    int pos;

    for (pos = 0; srclen; src++, srclen--)
    {
        if (flags & NORM_IGNORESYMBOLS)
        {
            if (get_char_type( CT_CTYPE1, *src ) & C1_PUNCT) continue;
            if (get_char_type( CT_CTYPE3, *src ) & C3_SYMBOL) continue;
        }
        if (flags & NORM_IGNORENONSPACE)
        {
            WCHAR buffer[8];
            const WCHAR *decomp;
            unsigned int i, j, len;

            if ((decomp = get_decomposition( *src, &len )) && len > 1)
            {
                for (i = j = 0; i < len; i++)
                    if (!nonspace_ignored( decomp[i] )) buffer[j++] = decomp[i];

                if (i > j)  /* something was removed */
                {
                    if (pos + j <= dstlen) memcpy( dst + pos, buffer, j * sizeof(WCHAR) );
                    pos += j;
                    continue;
                }
            }
            else if (nonspace_ignored( *src )) continue;
        }
        if (pos < dstlen) dst[pos] = *src;
        pos++;
    }
    return pos;
}


/* map full-width characters to single or double half-width characters. */
static int map_to_halfwidth( const USHORT *table, const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    static const BYTE katakana_map[] =
    {
                                0x01, 0x00, 0x01, 0x00, /* U+30a8- */
        0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, /* U+30b0- */
        0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, /* U+30b8- */
        0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, /* U+30c0- */
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* U+30c8- */
        0x01, 0x02, 0x00, 0x01, 0x02, 0x00, 0x01, 0x02, /* U+30d0- */
        0x00, 0x01, 0x02, 0x00, 0x01, 0x02, 0x00, 0x00, /* U+30d8- */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* U+30e0- */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* U+30e8- */
        0x00, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x08, /* U+30f0- */
        0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x01        /* U+30f8- */
    };
    int pos;

    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR ch = table ? casemap( table, *src ) : *src;
        USHORT shift = ch - 0x30ac;
        BYTE k;

        if (shift < ARRAY_SIZE(katakana_map) && (k = katakana_map[shift]))
        {
            if (pos < dstlen - 1)
            {
                dst[pos] = casemap( charmaps[CHARMAP_HALFWIDTH], ch - k );
                dst[pos + 1] = (k == 2) ? 0xff9f : 0xff9e;
            }
            pos += 2;
        }
        else
        {
            if (pos < dstlen) dst[pos] = casemap( charmaps[CHARMAP_HALFWIDTH], ch );
            pos++;
        }
    }
    return pos;
}


static int lcmap_string( const struct sortguid *sortid, DWORD flags,
                         const WCHAR *src, int srclen, WCHAR *dst, int dstlen )
{
    const USHORT *case_table = NULL;
    int ret;

    if (flags & (LCMAP_LOWERCASE | LCMAP_UPPERCASE))
    {
        if ((flags & LCMAP_TITLECASE) == LCMAP_TITLECASE)  /* FIXME */
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        case_table = sort.casemap + (flags & LCMAP_LINGUISTIC_CASING ? sortid->casemap : 0);
        case_table = case_table + 2 + (flags & LCMAP_LOWERCASE ? case_table[1] : 0);
    }

    switch (flags & ~(LCMAP_BYTEREV | LCMAP_LOWERCASE | LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING))
    {
    case LCMAP_HIRAGANA:
        ret = casemap_string( charmaps[CHARMAP_HIRAGANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_KATAKANA:
        ret = casemap_string( charmaps[CHARMAP_KATAKANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_HALFWIDTH:
        ret = map_to_halfwidth( NULL, src, srclen, dst, dstlen );
        break;
    case LCMAP_HIRAGANA | LCMAP_HALFWIDTH:
        ret = map_to_halfwidth( charmaps[CHARMAP_HIRAGANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_KATAKANA | LCMAP_HALFWIDTH:
        ret = map_to_halfwidth( charmaps[CHARMAP_KATAKANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_FULLWIDTH:
        ret = map_to_fullwidth( NULL, src, srclen, dst, dstlen );
        break;
    case LCMAP_HIRAGANA | LCMAP_FULLWIDTH:
        ret = map_to_fullwidth( charmaps[CHARMAP_HIRAGANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_KATAKANA | LCMAP_FULLWIDTH:
        ret = map_to_fullwidth( charmaps[CHARMAP_KATAKANA], src, srclen, dst, dstlen );
        break;
    case LCMAP_SIMPLIFIED_CHINESE:
        ret = casemap_string( charmaps[CHARMAP_SIMPLIFIED], src, srclen, dst, dstlen );
        break;
    case LCMAP_TRADITIONAL_CHINESE:
        ret = casemap_string( charmaps[CHARMAP_TRADITIONAL], src, srclen, dst, dstlen );
        break;
    case NORM_IGNORENONSPACE:
    case NORM_IGNORESYMBOLS:
    case NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS:
        if (flags & ~(NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS | LCMAP_BYTEREV))
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = map_remove_ignored( flags, src, srclen, dst, dstlen );
        break;
    case 0:
        if (case_table)
        {
            ret = casemap_string( case_table, src, srclen, dst, dstlen );
            case_table = NULL;
            break;
        }
        if (flags & LCMAP_BYTEREV)
        {
            ret = min( srclen, dstlen );
            memcpy( dst, src, ret * sizeof(WCHAR) );
            break;
        }
        /* fall through */
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (dstlen && case_table) ret = casemap_string( case_table, dst, ret, dst, dstlen );
    if (flags & LCMAP_BYTEREV) map_byterev( dst, min( dstlen, ret ), dst );

    if (dstlen && dstlen < ret)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return ret;
}


static int compare_tzdate( const TIME_FIELDS *tf, const SYSTEMTIME *compare )
{
    static const int month_lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int first, last, limit, dayinsecs;

    if (tf->Month < compare->wMonth) return -1; /* We are in a month before the date limit. */
    if (tf->Month > compare->wMonth) return 1; /* We are in a month after the date limit. */

    /* if year is 0 then date is in day-of-week format, otherwise
     * it's absolute date.
     */
    if (!compare->wYear)
    {
        /* wDay is interpreted as number of the week in the month
         * 5 means: the last week in the month */
        /* calculate the day of the first DayOfWeek in the month */
        first = (6 + compare->wDayOfWeek - tf->Weekday + tf->Day) % 7 + 1;
        /* check needed for the 5th weekday of the month */
        last = month_lengths[tf->Month - 1] +
            (tf->Month == 2 && (!(tf->Year % 4) && (tf->Year % 100 || !(tf->Year % 400))));
        limit = first + 7 * (compare->wDay - 1);
        if (limit > last) limit -= 7;
    }
    else limit = compare->wDay;

    limit = ((limit * 24 + compare->wHour) * 60 + compare->wMinute) * 60;
    dayinsecs = ((tf->Day * 24  + tf->Hour) * 60 + tf->Minute) * 60 + tf->Second;
    return dayinsecs - limit;
}


static DWORD get_timezone_id( const TIME_ZONE_INFORMATION *info, LARGE_INTEGER time, BOOL is_local )
{
    int year;
    BOOL before_standard_date, after_daylight_date;
    LARGE_INTEGER t2;
    TIME_FIELDS tf;

    if (!info->DaylightDate.wMonth) return TIME_ZONE_ID_UNKNOWN;

    /* if year is 0 then date is in day-of-week format, otherwise it's absolute date */
    if (info->StandardDate.wMonth == 0 ||
        (info->StandardDate.wYear == 0 &&
         (info->StandardDate.wDay < 1 || info->StandardDate.wDay > 5 ||
          info->DaylightDate.wDay < 1 || info->DaylightDate.wDay > 5)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return TIME_ZONE_ID_INVALID;
    }

    if (!is_local) time.QuadPart -= info->Bias * (LONGLONG)600000000;
    RtlTimeToTimeFields( &time, &tf );
    year = tf.Year;
    if (!is_local)
    {
        t2.QuadPart = time.QuadPart - info->DaylightBias * (LONGLONG)600000000;
        RtlTimeToTimeFields( &t2, &tf );
    }
    if (tf.Year == year)
        before_standard_date = compare_tzdate( &tf, &info->StandardDate ) < 0;
    else
        before_standard_date = tf.Year < year;

    if (!is_local)
    {
        t2.QuadPart = time.QuadPart - info->StandardBias * (LONGLONG)600000000;
        RtlTimeToTimeFields( &t2, &tf );
    }
    if (tf.Year == year)
        after_daylight_date = compare_tzdate( &tf, &info->DaylightDate ) >= 0;
    else
        after_daylight_date = tf.Year > year;

    if (info->DaylightDate.wMonth < info->StandardDate.wMonth) /* Northern hemisphere */
    {
        if (before_standard_date && after_daylight_date) return TIME_ZONE_ID_DAYLIGHT;
    }
    else /* Down south */
    {
        if (before_standard_date || after_daylight_date) return TIME_ZONE_ID_DAYLIGHT;
    }
    return TIME_ZONE_ID_STANDARD;
}


/* Note: the Internal_ functions are not documented. The number of parameters
 * should be correct, but their exact meaning may not.
 */

/******************************************************************************
 *	Internal_EnumCalendarInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumCalendarInfo( CALINFO_ENUMPROCW proc,
                                                         const NLS_LOCALE_DATA *locale, CALID id,
                                                         CALTYPE type, BOOL unicode, BOOL ex,
                                                         BOOL exex, LPARAM lparam )
{
    const USHORT *calendars;
    USHORT cal = id;
    WCHAR buffer[256];
    INT ret, i, count = 1;

    if (!proc || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (id == ENUM_ALL_CALENDARS)
    {
        count = locale_strings[locale->scalendartype];
        calendars = locale_strings + locale->scalendartype + 1;
    }
    else if (id <= CAL_UMALQURA)
    {
        calendars = &cal;
        count = 1;
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    type &= ~CAL_RETURN_NUMBER;

    for (i = 0; i < count; i++)
    {
        id = calendars[i];
        if (unicode)
        {
            ret = get_calendar_info( locale, id, type, buffer, ARRAY_SIZE(buffer), NULL );
        }
        else
        {
            WCHAR bufW[256];
            ret = get_calendar_info( locale, id, type, bufW, ARRAY_SIZE(bufW), NULL );
            if (ret) WideCharToMultiByte( get_locale_codepage( locale, type ), 0,
                                          bufW, -1, (char *)buffer, sizeof(buffer), NULL, NULL );
        }

        if (ret)
        {
            if (exex) ret = ((CALINFO_ENUMPROCEXEX)proc)( buffer, id, NULL, lparam );
            else if (ex) ret = ((CALINFO_ENUMPROCEXW)proc)( buffer, id );
            else ret = proc( buffer );
        }
        if (!ret) break;
    }
    return TRUE;
}


static BOOL call_enum_date_func( DATEFMT_ENUMPROCW proc, const NLS_LOCALE_DATA *locale, DWORD flags,
                                 DWORD str, WCHAR *buffer, CALID id, BOOL unicode,
                                 BOOL ex, BOOL exex, LPARAM lparam )
{
    char buffA[256];

    if (str) memcpy( buffer, locale_strings + str + 1, (locale_strings[str] + 1) * sizeof(WCHAR) );
    if (exex) return ((DATEFMT_ENUMPROCEXEX)proc)( buffer, id, lparam );
    if (ex) return ((DATEFMT_ENUMPROCEXW)proc)( buffer, id );
    if (unicode) return proc( buffer );
    WideCharToMultiByte( get_locale_codepage( locale, flags ), 0, buffer, -1,
                         buffA, ARRAY_SIZE(buffA), NULL, NULL );
    return proc( (WCHAR *)buffA );
}


/**************************************************************************
 *	Internal_EnumDateFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumDateFormats( DATEFMT_ENUMPROCW proc,
                                                        const NLS_LOCALE_DATA *locale, DWORD flags,
                                                        BOOL unicode, BOOL ex, BOOL exex, LPARAM lparam )
{
    WCHAR buffer[256];
    INT i, j, ret;
    DWORD pos;
    const struct calendar *cal;
    const USHORT *calendars;
    const DWORD *array;

    if (!proc || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    calendars = locale_strings + locale->scalendartype;

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        if (!get_locale_info( locale, 0, LOCALE_SSHORTDATE, buffer, ARRAY_SIZE(buffer) )) return FALSE;
        pos = locale->sshortdate;
        break;
    case DATE_LONGDATE:
        if (!get_locale_info( locale, 0, LOCALE_SLONGDATE, buffer, ARRAY_SIZE(buffer) )) return FALSE;
        pos = locale->slongdate;
        break;
    case DATE_YEARMONTH:
        if (!get_locale_info( locale, 0, LOCALE_SYEARMONTH, buffer, ARRAY_SIZE(buffer) )) return FALSE;
        pos = locale->syearmonth;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* first the user override data */

    ret = call_enum_date_func( proc, locale, flags, 0, buffer, 1, unicode, ex, exex, lparam );

    /* then the remaining locale data */

    array = (const DWORD *)(locale_strings + pos + 1);
    for (i = 1; ret && i < locale_strings[pos]; i++)
        ret = call_enum_date_func( proc, locale, flags, array[i], buffer, 1, unicode, ex, exex, lparam );

    /* then the extra calendars */

    for (i = 0; ret && i < calendars[0]; i++)
    {
        if (calendars[i + 1] == 1) continue;
        if (!(cal = get_calendar_data( locale, calendars[i + 1] ))) continue;
        switch (flags & ~LOCALE_USE_CP_ACP)
        {
        case 0:
        case DATE_SHORTDATE:
            pos = cal->sshortdate;
            break;
        case DATE_LONGDATE:
            pos = cal->slongdate;
            break;
        case DATE_YEARMONTH:
            pos = cal->syearmonth;
            break;
        }
        array = (const DWORD *)(locale_strings + pos + 1);
        for (j = 0; ret && j < locale_strings[pos]; j++)
            ret = call_enum_date_func( proc, locale, flags, array[j], buffer,
                                       calendars[i + 1], unicode, ex, exex, lparam );
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumLanguageGroupLocales   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumLanguageGroupLocales( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10];
    DWORD name_len, value_len, type, index = 0, alt = 0;
    HKEY key, altkey;
    LCID lcid;

    if (!proc || id < LGRPID_WESTERN_EUROPE || id > LGRPID_ARMENIAN)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, L"Alternate Sorts", 0, KEY_READ, &altkey )) altkey = 0;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( alt ? altkey : key, index++, name, &name_len, NULL,
                           &type, (BYTE *)value, &value_len ))
        {
            if (alt++) break;
            index = 0;
            continue;
        }
        if (type != REG_SZ) continue;
        if (id != wcstoul( value, NULL, 16 )) continue;
        lcid = wcstoul( name, NULL, 16 );
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((LANGGROUPLOCALE_ENUMPROCA)proc)( id, lcid, nameA, param )) break;
        }
        else if (!proc( id, lcid, name, param )) break;
    }
    RegCloseKey( altkey );
    RegCloseKey( key );
    return TRUE;
}


/***********************************************************************
 *	Internal_EnumSystemCodePages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemCodePages( CODEPAGE_ENUMPROCW proc, DWORD flags,
                                                            BOOL unicode )
{
    WCHAR name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (RegOpenKeyExW( nls_key, L"Codepage", 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!wcstoul( name, NULL, 10 )) continue;
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((CODEPAGE_ENUMPROCA)proc)( nameA )) break;
        }
        else if (!proc( name )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumSystemLanguageGroups   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemLanguageGroups( LANGUAGEGROUP_ENUMPROCW proc,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10], descr[80];
    DWORD name_len, value_len, type, index = 0;
    HKEY key;
    LGRPID id;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (flags)
    {
    case 0:
        flags = LGRPID_INSTALLED;
        break;
    case LGRPID_INSTALLED:
    case LGRPID_SUPPORTED:
        break;
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, (BYTE *)value, &value_len )) break;
        if (type != REG_SZ) continue;

        id = wcstoul( name, NULL, 16 );

        if (!(flags & LGRPID_SUPPORTED) && !wcstoul( value, NULL, 10 )) continue;
        if (!LoadStringW( kernelbase_handle, id, descr, ARRAY_SIZE(descr) )) descr[0] = 0;
        TRACE( "%p: %lu %s %s %lx %Ix\n", proc, id, debugstr_w(name), debugstr_w(descr), flags, param );
        if (!unicode)
        {
            char nameA[10], descrA[80];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, descr, -1, descrA, sizeof(descrA), NULL, NULL );
            if (!((LANGUAGEGROUP_ENUMPROCA)proc)( id, nameA, descrA, flags, param )) break;
        }
        else if (!proc( id, name, descr, flags, param )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/**************************************************************************
 *	Internal_EnumTimeFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumTimeFormats( TIMEFMT_ENUMPROCW proc,
                                                        const NLS_LOCALE_DATA *locale, DWORD flags,
                                                        BOOL unicode, BOOL ex, LPARAM lparam )
{
    WCHAR buffer[256];
    INT ret = TRUE;
    const DWORD *array;
    DWORD pos, i;

    if (!proc || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
        if (!get_locale_info( locale, 0, LOCALE_STIMEFORMAT, buffer, ARRAY_SIZE(buffer) )) return FALSE;
        pos = locale->stimeformat;
        break;
    case TIME_NOSECONDS:
        if (!get_locale_info( locale, 0, LOCALE_SSHORTTIME, buffer, ARRAY_SIZE(buffer) )) return FALSE;
        pos = locale->sshorttime;
        break;
    default:
        FIXME( "Unknown time format %lx\n", flags );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    array = (const DWORD *)(locale_strings + pos + 1);
    for (i = 0; ret && i < locale_strings[pos]; i++)
    {
        if (i) memcpy( buffer, locale_strings + array[i] + 1,
                       (locale_strings[array[i]] + 1) * sizeof(WCHAR) );

        if (ex) ret = ((TIMEFMT_ENUMPROCEX)proc)( buffer, lparam );
        else if (unicode) ret = proc( buffer );
        else
        {
            char buffA[256];
            WideCharToMultiByte( get_locale_codepage( locale, flags ), 0, buffer, -1,
                                 buffA, ARRAY_SIZE(buffA), NULL, NULL );
            ret = proc( (WCHAR *)buffA );
        }
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumUILanguages( UILANGUAGE_ENUMPROCW proc, DWORD flags,
                                                        LONG_PTR param, BOOL unicode )
{
    WCHAR nameW[LOCALE_NAME_MAX_LENGTH];
    char nameA[LOCALE_NAME_MAX_LENGTH];
    DWORD i;

    if (!proc)
    {
	SetLastError( ERROR_INVALID_PARAMETER );
	return FALSE;
    }
    if (flags & ~(MUI_LANGUAGE_ID | MUI_LANGUAGE_NAME))
    {
	SetLastError( ERROR_INVALID_FLAGS );
	return FALSE;
    }

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (SORTIDFROMLCID( lcnames_index[i].id )) continue;  /* skip alternate sorts */
        if (flags & MUI_LANGUAGE_NAME)
        {
            const WCHAR *str = locale_strings + lcnames_index[i].name;

            if (unicode)
            {
                memcpy( nameW, str + 1, (*str + 1) * sizeof(WCHAR) );
                if (!proc( nameW, param )) break;
            }
            else
            {
                WideCharToMultiByte( CP_ACP, 0, str + 1, -1, nameA, sizeof(nameA), NULL, NULL );
                if (!((UILANGUAGE_ENUMPROCA)proc)( nameA, param )) break;
            }
        }
        else
        {
            if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
            if (unicode)
            {
                swprintf( nameW, ARRAY_SIZE(nameW), L"%04lx", lcnames_index[i].id );
                if (!proc( nameW, param )) break;
            }
            else
            {
                sprintf( nameA, "%04x", lcnames_index[i].id );
                if (!((UILANGUAGE_ENUMPROCA)proc)( nameA, param )) break;
            }
        }
    }
    return TRUE;
}


/******************************************************************************
 *	CompareStringEx   (kernelbase.@)
 */
INT WINAPI CompareStringEx( const WCHAR *locale, DWORD flags, const WCHAR *str1, int len1,
                            const WCHAR *str2, int len2, NLSVERSIONINFO *version,
                            void *reserved, LPARAM handle )
{
    const struct sortguid *sortid;
    const DWORD supported_flags = NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS |
                                  SORT_STRINGSORT | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH |
                                  NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE |
                                  LINGUISTIC_IGNOREDIACRITIC | SORT_DIGITSASNUMBERS |
                                  0x10000000 | LOCALE_USE_CP_ACP;
    /* 0x10000000 is related to diacritics in Arabic, Japanese, and Hebrew */
    int ret;

    if (version) FIXME( "unexpected version parameter\n" );
    if (reserved) FIXME( "unexpected reserved value\n" );
    if (handle) FIXME( "unexpected handle\n" );

    if (flags & ~supported_flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!(sortid = get_language_sort( locale ))) return 0;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (len1 < 0) len1 = lstrlenW(str1);
    if (len2 < 0) len2 = lstrlenW(str2);

    ret = compare_string( sortid, flags, str1, len1, str2, len2 );
    if (ret < 0) return CSTR_LESS_THAN;
    if (ret > 0) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}


/******************************************************************************
 *	CompareStringA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringA( LCID lcid, DWORD flags, const char *str1, int len1,
                                             const char *str2, int len2 )
{
    WCHAR *buf1W = NtCurrentTeb()->StaticUnicodeBuffer;
    WCHAR *buf2W = buf1W + 130;
    LPWSTR str1W, str2W;
    INT len1W = 0, len2W = 0, ret;
    UINT locale_cp = CP_ACP;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (flags & SORT_DIGITSASNUMBERS)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (len1 < 0) len1 = strlen(str1);
    if (len2 < 0) len2 = strlen(str2);

    locale_cp = get_lcid_codepage( lcid, flags );
    if (len1)
    {
        if (len1 <= 130) len1W = MultiByteToWideChar( locale_cp, 0, str1, len1, buf1W, 130 );
        if (len1W) str1W = buf1W;
        else
        {
            len1W = MultiByteToWideChar( locale_cp, 0, str1, len1, NULL, 0 );
            str1W = HeapAlloc( GetProcessHeap(), 0, len1W * sizeof(WCHAR) );
            if (!str1W)
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return 0;
            }
            MultiByteToWideChar( locale_cp, 0, str1, len1, str1W, len1W );
        }
    }
    else
    {
        len1W = 0;
        str1W = buf1W;
    }

    if (len2)
    {
        if (len2 <= 130) len2W = MultiByteToWideChar( locale_cp, 0, str2, len2, buf2W, 130 );
        if (len2W) str2W = buf2W;
        else
        {
            len2W = MultiByteToWideChar( locale_cp, 0, str2, len2, NULL, 0 );
            str2W = HeapAlloc( GetProcessHeap(), 0, len2W * sizeof(WCHAR) );
            if (!str2W)
            {
                if (str1W != buf1W) HeapFree( GetProcessHeap(), 0, str1W );
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return 0;
            }
            MultiByteToWideChar( locale_cp, 0, str2, len2, str2W, len2W );
        }
    }
    else
    {
        len2W = 0;
        str2W = buf2W;
    }

    ret = CompareStringW( lcid, flags, str1W, len1W, str2W, len2W );

    if (str1W != buf1W) HeapFree( GetProcessHeap(), 0, str1W );
    if (str2W != buf2W) HeapFree( GetProcessHeap(), 0, str2W );
    return ret;
}


/******************************************************************************
 *	CompareStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringW( LCID lcid, DWORD flags, const WCHAR *str1, int len1,
                                             const WCHAR *str2, int len2 )
{
    const WCHAR *locale = LOCALE_NAME_USER_DEFAULT;
    const NLS_LOCALE_LCID_INDEX *entry;

    switch (lcid)
    {
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_SYSTEM_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
    case LOCALE_CUSTOM_UNSPECIFIED:
    case LOCALE_CUSTOM_UI_DEFAULT:
        break;
    default:
        if (lcid == user_lcid || lcid == system_lcid) break;
        if (!(entry = find_lcid_entry( lcid )))
        {
            WARN( "unknown locale %04lx\n", lcid );
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        locale = locale_strings + entry->name + 1;
        break;
    }

    return CompareStringEx( locale, flags, str1, len1, str2, len2, NULL, NULL, 0 );
}


/******************************************************************************
 *	CompareStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringOrdinal( const WCHAR *str1, INT len1,
                                                   const WCHAR *str2, INT len2, BOOL ignore_case )
{
    int ret;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len1 < 0) len1 = lstrlenW( str1 );
    if (len2 < 0) len2 = lstrlenW( str2 );

    ret = RtlCompareUnicodeStrings( str1, len1, str2, len2, ignore_case );
    if (ret < 0) return CSTR_LESS_THAN;
    if (ret > 0) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}


/******************************************************************************
 *	ConvertDefaultLocale   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH ConvertDefaultLocale( LCID lcid )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );
    if (locale) lcid = locale->ilanguage;
    return lcid;
}


/******************************************************************************
 *	EnumCalendarInfoW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoW( CALINFO_ENUMPROCW proc, LCID lcid,
                                                 CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( proc, NlsValidateLocale( &lcid, 0 ),
                                      id, type, TRUE, FALSE, FALSE, 0 );
}


/******************************************************************************
 *	EnumCalendarInfoExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExW( CALINFO_ENUMPROCEXW proc, LCID lcid,
                                                   CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                      id, type, TRUE, TRUE, FALSE, 0 );
}

/******************************************************************************
 *	EnumCalendarInfoExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExEx( CALINFO_ENUMPROCEXEX proc, LPCWSTR locale, CALID id,
                                                    LPCWSTR reserved, CALTYPE type, LPARAM lparam )
{
    LCID lcid;
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, get_locale_by_name( locale, &lcid ),
                                      id, type, TRUE, TRUE, TRUE, lparam );
}


/**************************************************************************
 *	EnumDateFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsW( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( proc, NlsValidateLocale( &lcid, 0 ),
                                     flags, TRUE, FALSE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExW( DATEFMT_ENUMPROCEXW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                     flags, TRUE, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExEx( DATEFMT_ENUMPROCEXEX proc, const WCHAR *locale,
                                                   DWORD flags, LPARAM lparam )
{
    LCID lcid;
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, get_locale_by_name( locale, &lcid ),
                                     flags, TRUE, TRUE, TRUE, lparam );
}



/******************************************************************************
 *	EnumDynamicTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH EnumDynamicTimeZoneInformation( DWORD index,
                                                               DYNAMIC_TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION tz;
    LSTATUS ret;
    DWORD size;

    if (!info) return ERROR_INVALID_PARAMETER;

    size = ARRAY_SIZE(tz.TimeZoneKeyName);
    ret = RegEnumKeyExW( tz_key, index, tz.TimeZoneKeyName, &size, NULL, NULL, NULL, NULL );
    if (ret) return ret;

    tz.DynamicDaylightTimeDisabled = TRUE;
    if (!GetTimeZoneInformationForYear( 0, &tz, (TIME_ZONE_INFORMATION *)info )) return GetLastError();

    lstrcpyW( info->TimeZoneKeyName, tz.TimeZoneKeyName );
    info->DynamicDaylightTimeDisabled = FALSE;
    return 0;
}


/******************************************************************************
 *	EnumLanguageGroupLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumLanguageGroupLocalesW( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumLanguageGroupLocales( proc, id, flags, param, TRUE );
}


/******************************************************************************
 *	EnumUILanguagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumUILanguagesW( UILANGUAGE_ENUMPROCW proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumUILanguages( proc, flags, param, TRUE );
}


/***********************************************************************
 *	EnumSystemCodePagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemCodePagesW( CODEPAGE_ENUMPROCW proc, DWORD flags )
{
    return Internal_EnumSystemCodePages( proc, flags, TRUE );
}


/******************************************************************************
 *	EnumSystemGeoID   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemGeoID( GEOCLASS class, GEOID parent, GEO_ENUMPROC proc )
{
    INT i;

    TRACE( "(%ld, %ld, %p)\n", class, parent, proc );

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (class != GEOCLASS_NATION && class != GEOCLASS_REGION && class != GEOCLASS_ALL)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    for (i = 0; i < geo_ids_count; i++)
    {
        if (class != GEOCLASS_ALL && geo_ids[i].class != class) continue;
        if (parent && geo_ids[i].parent != parent) continue;
        if (!proc( geo_ids[i].id )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLanguageGroupsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLanguageGroupsW( LANGUAGEGROUP_ENUMPROCW proc,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumSystemLanguageGroups( proc, flags, param, TRUE );
}


/******************************************************************************
 *	EnumSystemLocalesA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesA( LOCALE_ENUMPROCA proc, DWORD flags )
{
    char name[10];
    DWORD i;

    if (!flags)
        flags = LCID_SUPPORTED;

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (SORTIDFROMLCID( lcnames_index[i].id ) != SORT_DEFAULT && !(flags & LCID_ALTERNATE_SORTS))
            continue; /* skip alternate sorts if not requested */
        if (SORTIDFROMLCID( lcnames_index[i].id ) == SORT_DEFAULT && !(flags & (LCID_INSTALLED | LCID_SUPPORTED)))
            continue;  /* skip default sorts if not requested */
        sprintf( name, "%08x", lcnames_index[i].id );
        if (!proc( name )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesW( LOCALE_ENUMPROCW proc, DWORD flags )
{
    WCHAR name[10];
    DWORD i;

    if (!flags)
        flags = LCID_SUPPORTED;

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (SORTIDFROMLCID( lcnames_index[i].id ) != SORT_DEFAULT && !(flags & LCID_ALTERNATE_SORTS))
            continue; /* skip alternate sorts if not requested */
        if (SORTIDFROMLCID( lcnames_index[i].id ) == SORT_DEFAULT && !(flags & (LCID_INSTALLED | LCID_SUPPORTED)))
            continue;  /* skip default sorts if not requested */
        swprintf( name, ARRAY_SIZE(name), L"%08lx", lcnames_index[i].id );
        if (!proc( name )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesEx( LOCALE_ENUMPROCEX proc, DWORD wanted_flags,
                                                   LPARAM param, void *reserved )
{
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
    DWORD i, flags;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        const NLS_LOCALE_DATA *locale = get_locale_data( lcnames_index[i].idx );
        const WCHAR *str = locale_strings + lcnames_index[i].name;

        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        memcpy( buffer, str + 1, (*str + 1) * sizeof(WCHAR) );
        if (SORTIDFROMLCID( lcnames_index[i].id ) || wcschr( str + 1, '_' ))
            flags = LOCALE_ALTERNATE_SORTS;
        else
            flags = LOCALE_WINDOWS | (locale->inotneutral ? LOCALE_SPECIFICDATA : LOCALE_NEUTRALDATA);
        if (wanted_flags && !(flags & wanted_flags)) continue;
        if (!proc( buffer, flags, param )) break;
    }
    return TRUE;
}


/**************************************************************************
 *	EnumTimeFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsW( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumTimeFormats( proc, NlsValidateLocale( &lcid, 0 ), flags, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumTimeFormatsEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsEx( TIMEFMT_ENUMPROCEX proc, const WCHAR *locale,
                                                 DWORD flags, LPARAM lparam )
{
    LCID lcid;
    return Internal_EnumTimeFormats( (TIMEFMT_ENUMPROCW)proc, get_locale_by_name( locale, &lcid ),
                                     flags, TRUE, TRUE, lparam );
}


/**************************************************************************
 *	FindNLSString   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindNLSString( LCID lcid, DWORD flags, const WCHAR *src,
                                            int srclen, const WCHAR *value, int valuelen, int *found )
{
    const WCHAR *locale = LOCALE_NAME_USER_DEFAULT;
    const NLS_LOCALE_LCID_INDEX *entry;

    switch (lcid)
    {
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_SYSTEM_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
    case LOCALE_CUSTOM_UNSPECIFIED:
    case LOCALE_CUSTOM_UI_DEFAULT:
        break;
    default:
        if (lcid == user_lcid || lcid == system_lcid) break;
        if (!(entry = find_lcid_entry( lcid )))
        {
            WARN( "unknown locale %04lx\n", lcid );
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        locale = locale_strings + entry->name + 1;
        break;
    }

    return FindNLSStringEx( locale, flags, src, srclen, value, valuelen, found, NULL, NULL, 0 );
}


/**************************************************************************
 *	FindNLSStringEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindNLSStringEx( const WCHAR *locale, DWORD flags, const WCHAR *src,
                                              int srclen, const WCHAR *value, int valuelen, int *found,
                                              NLSVERSIONINFO *version, void *reserved, LPARAM handle )
{
    const struct sortguid *sortid;

    TRACE( "%s %lx %s %d %s %d %p %p %p %Id\n", wine_dbgstr_w(locale), flags,
           wine_dbgstr_w(src), srclen, wine_dbgstr_w(value), valuelen, found,
           version, reserved, handle );

    if (version) FIXME( "unexpected version parameter\n" );
    if (reserved) FIXME( "unexpected reserved value\n" );
    if (handle) FIXME( "unexpected handle\n" );

    if (!src || !srclen || srclen < -1 || !value || !valuelen || valuelen < -1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }
    if (!(sortid = get_language_sort( locale ))) return -1;

    if (srclen == -1) srclen = lstrlenW(src);
    if (valuelen == -1) valuelen = lstrlenW(value);

    return find_substring( sortid, flags, src, srclen, value, valuelen, found );
}


/******************************************************************************
 *	FindStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindStringOrdinal( DWORD flag, const WCHAR *src, INT src_size,
                                                const WCHAR *val, INT val_size, BOOL ignore_case )
{
    INT offset, inc, count;

    TRACE( "%#lx %s %d %s %d %d\n", flag, wine_dbgstr_w(src), src_size,
           wine_dbgstr_w(val), val_size, ignore_case );

    if (!src || !val)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (flag != FIND_FROMSTART && flag != FIND_FROMEND && flag != FIND_STARTSWITH && flag != FIND_ENDSWITH)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return -1;
    }

    if (src_size == -1) src_size = lstrlenW( src );
    if (val_size == -1) val_size = lstrlenW( val );

    SetLastError( ERROR_SUCCESS );
    src_size -= val_size;
    if (src_size < 0) return -1;

    count = flag & (FIND_FROMSTART | FIND_FROMEND) ? src_size + 1 : 1;
    offset = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 0 : src_size;
    inc = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 1 : -1;
    while (count--)
    {
        if (CompareStringOrdinal( src + offset, val_size, val, val_size, ignore_case ) == CSTR_EQUAL)
            return offset;
        offset += inc;
    }
    return -1;
}


/******************************************************************************
 *	FoldStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FoldStringW( DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen )
{
    NTSTATUS status;
    WCHAR *buf = dst;
    int len = dstlen;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen == -1) srclen = lstrlenW(src) + 1;

    if (!dstlen && (flags & (MAP_PRECOMPOSED | MAP_FOLDCZONE | MAP_COMPOSITE)))
    {
        len = srclen * 4;
        if (!(buf = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
    }

    for (;;)
    {
        status = fold_string( flags, src, srclen, buf, &len );
        if (buf != dst) RtlFreeHeap( GetProcessHeap(), 0, buf );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        if (!(buf = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
    }
    if (status == STATUS_INVALID_PARAMETER_1)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!set_ntstatus( status )) return 0;

    if (dstlen && dstlen < len) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return len;
}


static const WCHAR *get_message( DWORD flags, const void *src, UINT id, UINT lang,
                                 BOOL ansi, WCHAR **buffer )
{
    DWORD len;

    if (!(flags & FORMAT_MESSAGE_FROM_STRING))
    {
        const MESSAGE_RESOURCE_ENTRY *entry;
        NTSTATUS status = STATUS_INVALID_PARAMETER;

        if (flags & FORMAT_MESSAGE_FROM_HMODULE)
        {
            HMODULE module = (HMODULE)src;
            if (!module) module = GetModuleHandleW( 0 );
            status = RtlFindMessage( module, RT_MESSAGETABLE, lang, id, &entry );
        }
        if (status && (flags & FORMAT_MESSAGE_FROM_SYSTEM))
        {
            /* Fold win32 hresult to its embedded error code. */
            if (HRESULT_SEVERITY(id) == SEVERITY_ERROR && HRESULT_FACILITY(id) == FACILITY_WIN32)
                id = HRESULT_CODE( id );
            status = RtlFindMessage( kernelbase_handle, RT_MESSAGETABLE, lang, id, &entry );
        }
        if (!set_ntstatus( status )) return NULL;

        src = entry->Text;
        ansi = !(entry->Flags & MESSAGE_RESOURCE_UNICODE);
    }

    if (!ansi) return src;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if (!(*buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
    MultiByteToWideChar( CP_ACP, 0, src, -1, *buffer, len );
    return *buffer;
}


/***********************************************************************
 *	FormatMessageA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH FormatMessageA( DWORD flags, const void *source, DWORD msgid, DWORD langid,
                                               char *buffer, DWORD size, va_list *args )
{
    DWORD ret = 0;
    ULONG len, retsize = 0;
    ULONG width = (flags & FORMAT_MESSAGE_MAX_WIDTH_MASK);
    const WCHAR *src;
    WCHAR *result, *message = NULL;
    NTSTATUS status;

    TRACE( "(0x%lx,%p,%#lx,0x%lx,%p,%lu,%p)\n", flags, source, msgid, langid, buffer, size, args );

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        if (!buffer)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        *(char **)buffer = NULL;
    }
    if (size >= 32768)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (width == 0xff) width = ~0u;

    if (!(src = get_message( flags, source, msgid, langid, TRUE, &message ))) return 0;

    if (!(result = HeapAlloc( GetProcessHeap(), 0, 65536 )))
        status = STATUS_NO_MEMORY;
    else
        status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                   TRUE, !!(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY), args,
                                   result, 65536, &retsize );

    HeapFree( GetProcessHeap(), 0, message );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto done;
    }
    if (!set_ntstatus( status )) goto done;

    len = WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (len <= 1)
    {
        SetLastError( ERROR_NO_WORK_DONE );
        goto done;
    }

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        char *buf = LocalAlloc( LMEM_ZEROINIT, max( size, len ));
        if (!buf)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto done;
        }
        *(char **)buffer = buf;
        WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), buf, max( size, len ), NULL, NULL );
    }
    else if (len > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto done;
    }
    else WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), buffer, size, NULL, NULL );

    ret = len - 1;

done:
    HeapFree( GetProcessHeap(), 0, result );
    return ret;
}


/***********************************************************************
 *	FormatMessageW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH FormatMessageW( DWORD flags, const void *source, DWORD msgid, DWORD langid,
                                               WCHAR *buffer, DWORD size, va_list *args )
{
    ULONG retsize = 0;
    ULONG width = (flags & FORMAT_MESSAGE_MAX_WIDTH_MASK);
    const WCHAR *src;
    WCHAR *message = NULL;
    NTSTATUS status;

    TRACE( "(0x%lx,%p,%#lx,0x%lx,%p,%lu,%p)\n", flags, source, msgid, langid, buffer, size, args );

    if (!buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (width == 0xff) width = ~0u;

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) *(LPWSTR *)buffer = NULL;

    if (!(src = get_message( flags, source, msgid, langid, FALSE, &message ))) return 0;

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        WCHAR *result;
        va_list args_copy;
        ULONG alloc = max( size * sizeof(WCHAR), 65536 );

        for (;;)
        {
            if (!(result = HeapAlloc( GetProcessHeap(), 0, alloc )))
            {
                status = STATUS_NO_MEMORY;
                break;
            }
            if (args && !(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY))
            {
                va_copy( args_copy, *args );
                status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                           FALSE, FALSE, &args_copy, result, alloc, &retsize );
                va_end( args_copy );
            }
            else
                status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                           FALSE, TRUE, args, result, alloc, &retsize );

            if (!status)
            {
                if (retsize <= sizeof(WCHAR)) HeapFree( GetProcessHeap(), 0, result );
                else *(WCHAR **)buffer = HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY,
                                                      result, max( retsize, size * sizeof(WCHAR) ));
                break;
            }
            HeapFree( GetProcessHeap(), 0, result );
            if (status != STATUS_BUFFER_OVERFLOW) break;
            alloc *= 2;
        }
    }
    else status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                    FALSE, !!(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY), args,
                                    buffer, size * sizeof(WCHAR), &retsize );

    HeapFree( GetProcessHeap(), 0, message );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        if (size) buffer[size - 1] = 0;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    if (!set_ntstatus( status )) return 0;
    if (retsize <= sizeof(WCHAR)) SetLastError( ERROR_NO_WORK_DONE );
    return retsize / sizeof(WCHAR) - 1;
}


/******************************************************************************
 *	GetACP   (kernelbase.@)
 */
UINT WINAPI GetACP(void)
{
    return ansi_cpinfo.CodePage;
}


/***********************************************************************
 *	GetCPInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCPInfo( UINT codepage, CPINFO *cpinfo )
{
    const CPTABLEINFO *table;

    if (!cpinfo || !(table = get_codepage_table( codepage )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    cpinfo->MaxCharSize = table->MaximumCharacterSize;
    memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
    memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
    return TRUE;
}


/***********************************************************************
 *	GetCPInfoExW   (kernelbase.@)
 */
BOOL WINAPI GetCPInfoExW( UINT codepage, DWORD flags, CPINFOEXW *cpinfo )
{
    const CPTABLEINFO *table;
    int min, max, pos;

    if (!cpinfo || !(table = get_codepage_table( codepage )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    cpinfo->MaxCharSize = table->MaximumCharacterSize;
    memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
    memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
    cpinfo->CodePage = table->CodePage;
    cpinfo->UnicodeDefaultChar = table->UniDefaultChar;

    min = 0;
    max = ARRAY_SIZE(codepage_names) - 1;
    cpinfo->CodePageName[0] = 0;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (codepage_names[pos].cp < cpinfo->CodePage) min = pos + 1;
        else if (codepage_names[pos].cp > cpinfo->CodePage) max = pos - 1;
        else
        {
            wcscpy( cpinfo->CodePageName, codepage_names[pos].name );
            break;
        }
    }
    return TRUE;
}


/***********************************************************************
 *	GetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type,
                                               WCHAR *buffer, INT len, DWORD *value )
{
    const NLS_LOCALE_DATA *locale;

    TRACE( "%04lx %lu 0x%lx %p %d %p\n", lcid, calendar, type, buffer, len, value );

    if (!(locale = NlsValidateLocale( &lcid, 0 )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_calendar_info( locale, calendar, type, buffer, len, value );
}


/***********************************************************************
 *	GetCalendarInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoEx( const WCHAR *name, CALID calendar, const WCHAR *reserved,
                                                CALTYPE type, WCHAR *buffer, INT len, DWORD *value )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    TRACE( "%s %lu 0x%lx %p %d %p\n", debugstr_w(name), calendar, type, buffer, len, value );

    if (!locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_calendar_info( locale, calendar, type, buffer, len, value );
}


static CRITICAL_SECTION tzname_section;
static CRITICAL_SECTION_DEBUG tzname_section_debug =
{
    0, 0, &tzname_section,
    { &tzname_section_debug.ProcessLocksList, &tzname_section_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": tzname_section") }
};
static CRITICAL_SECTION tzname_section = { &tzname_section_debug, -1, 0, 0, 0, 0 };
static struct {
    LCID lcid;
    WCHAR key_name[128];
    WCHAR standard_name[32];
    WCHAR daylight_name[32];
} cached_tzname;

/***********************************************************************
 *	GetDynamicTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDynamicTimeZoneInformation( DYNAMIC_TIME_ZONE_INFORMATION *info )
{
    HKEY key;
    LARGE_INTEGER now;

    if (!set_ntstatus( RtlQueryDynamicTimeZoneInformation( (RTL_DYNAMIC_TIME_ZONE_INFORMATION *)info )))
        return TIME_ZONE_ID_INVALID;

    RtlEnterCriticalSection( &tzname_section );
    if (cached_tzname.lcid == GetThreadLocale() &&
        !wcscmp( info->TimeZoneKeyName, cached_tzname.key_name ))
    {
        wcscpy( info->StandardName, cached_tzname.standard_name );
        wcscpy( info->DaylightName, cached_tzname.daylight_name );
        RtlLeaveCriticalSection( &tzname_section );
    }
    else
    {
        RtlLeaveCriticalSection( &tzname_section );
        if (!RegOpenKeyExW( tz_key, info->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key ))
        {
            RegLoadMUIStringW( key, L"MUI_Std", info->StandardName,
                               sizeof(info->StandardName), NULL, 0, system_dir );
            RegLoadMUIStringW( key, L"MUI_Dlt", info->DaylightName,
                               sizeof(info->DaylightName), NULL, 0, system_dir );
            RegCloseKey( key );
        }
        else return TIME_ZONE_ID_INVALID;

        RtlEnterCriticalSection( &tzname_section );
        cached_tzname.lcid = GetThreadLocale();
        wcscpy( cached_tzname.key_name, info->TimeZoneKeyName );
        wcscpy( cached_tzname.standard_name, info->StandardName );
        wcscpy( cached_tzname.daylight_name, info->DaylightName );
        RtlLeaveCriticalSection( &tzname_section );
    }

    NtQuerySystemTime( &now );
    return get_timezone_id( (TIME_ZONE_INFORMATION *)info, now, FALSE );
}


/******************************************************************************
 *	GetDynamicTimeZoneInformationEffectiveYears   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDynamicTimeZoneInformationEffectiveYears( const DYNAMIC_TIME_ZONE_INFORMATION *info,
                                                                            DWORD *first, DWORD *last )
{
    HKEY key, dst_key = 0;
    DWORD type, count, ret = ERROR_FILE_NOT_FOUND;

    if (RegOpenKeyExW( tz_key, info->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key )) return ret;

    if (RegOpenKeyExW( key, L"Dynamic DST", 0, KEY_ALL_ACCESS, &dst_key )) goto done;
    count = sizeof(DWORD);
    if (RegQueryValueExW( dst_key, L"FirstEntry", NULL, &type, (BYTE *)first, &count )) goto done;
    if (type != REG_DWORD) goto done;
    count = sizeof(DWORD);
    if (RegQueryValueExW( dst_key, L"LastEntry", NULL, &type, (BYTE *)last, &count )) goto done;
    if (type != REG_DWORD) goto done;
    ret = 0;

done:
    RegCloseKey( dst_key );
    RegCloseKey( key );
    return ret;
}


#define MUI_SIGNATURE 0xfecdfecd
struct mui_resource
{
    DWORD signature;
    DWORD size;
    DWORD version;
    DWORD path_type;
    DWORD file_type;
    DWORD system_attributes;
    DWORD fallback_location;
    BYTE service_checksum[16];
    BYTE checksum[16];
    DWORD unk1[2];
    DWORD mui_path_off;
    DWORD mui_path_size;
    DWORD unk2[2];
    DWORD ln_type_name_off;
    DWORD ln_type_name_size;
    DWORD ln_type_id_off;
    DWORD ln_type_id_size;
    DWORD mui_type_name_off;
    DWORD mui_type_name_size;
    DWORD mui_type_id_off;
    DWORD mui_type_id_size;
    DWORD lang_off;
    DWORD lang_size;
    DWORD fallback_lang_off;
    DWORD fallback_lang_size;
};


static BOOL validate_mui_resource(struct mui_resource *mui, DWORD size)
{
    if (size >= sizeof(DWORD) && mui->signature != MUI_SIGNATURE)
    {
        SetLastError(ERROR_MUI_INVALID_RC_CONFIG);
        return FALSE;
    }

    size = min( size, mui->size );
    if (size < sizeof(*mui) ||
        mui->ln_type_name_off >= size || mui->ln_type_name_size > size - mui->ln_type_name_off ||
        mui->ln_type_id_off >= size || mui->ln_type_id_size > size - mui->ln_type_id_off ||
        mui->mui_type_name_off >= size || mui->mui_type_name_size > size - mui->mui_type_name_off ||
        mui->mui_type_id_off >= size || mui->mui_type_id_size > size - mui->mui_type_id_off ||
        mui->lang_off >= size || mui->lang_size > size - mui->lang_off ||
        mui->fallback_lang_off >= size || mui->fallback_lang_size > size - mui->fallback_lang_off)
    {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }
    return TRUE;
}


/******************************************************************************
 *	GetFileMUIInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileMUIInfo( DWORD flags, const WCHAR *path,
                                              FILEMUIINFO *info, DWORD *size )
{
    DWORD off, mui_size, type = MUI_FILETYPE_NOT_LANGUAGE_NEUTRAL;
    struct mui_resource *mui = NULL;
    HMODULE hmod;
    HRSRC hrsrc;

    TRACE( "%lu, %s, %p, %p\n", flags, debugstr_w(path), info, size );

    if (!path || !size || (*size && !info) ||
            (info && (*size < sizeof(*info) || info->dwSize != *size ||
                      info->dwVersion != MUI_FILEINFO_VERSION)))
    {
        if (size) *size = 0;
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!flags) flags = MUI_QUERY_TYPE | MUI_QUERY_CHECKSUM;

    hmod = LoadLibraryExW( path, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE );
    if (!hmod) return FALSE;

    hrsrc = FindResourceW( hmod, MAKEINTRESOURCEW(1), L"MUI" );
    if (hrsrc)
    {
        mui = LockResource( LoadResource(hmod, hrsrc) );
        if (mui) mui_size = SizeofResource( hmod, hrsrc );
        if (!mui || !validate_mui_resource( mui, mui_size ))
        {
            FreeLibrary( hmod );
            return FALSE;
        }
        if (mui->file_type & (MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN >> 1))
            type = MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN;
        else if (mui->file_type & (MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI >> 1))
            type = MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI;
    }
    if (type == MUI_FILETYPE_NOT_LANGUAGE_NEUTRAL)
    {
        FreeLibrary( hmod );

        if (!info)
        {
            *size = sizeof(*info);
            return TRUE;
        }
        if (info->dwSize < sizeof(*info))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        memset( info, 0, sizeof(*info) );
        info->dwSize = *size;
        info->dwVersion = MUI_FILEINFO_VERSION;
        if (flags & MUI_QUERY_TYPE) info->dwFileType = type;
        return TRUE;
    }

    off = offsetof(FILEMUIINFO, abBuffer);
    if (flags & MUI_QUERY_LANGUAGE_NAME)
    {
        off += type == MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN ?
            mui->fallback_lang_size : mui->lang_size;
    }
    if (flags & MUI_QUERY_RESOURCE_TYPES)
    {
        if (type == MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN)
            off += mui->ln_type_name_size + mui->ln_type_id_size;
        off += mui->mui_type_name_size + mui->mui_type_id_size;
    }
    if (off < sizeof(*info)) off = sizeof(*info);

    if (!info || info->dwSize < off)
    {
        FreeLibrary( hmod );
        *size = off;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    off = 0;
    memset( info, 0, sizeof(*info) );
    info->dwSize = *size;
    info->dwVersion = MUI_FILEINFO_VERSION;
    if (flags & MUI_QUERY_TYPE) info->dwFileType = type;
    if (flags & MUI_QUERY_CHECKSUM)
    {
        memcpy( info->pChecksum, mui->checksum, sizeof(info->pChecksum) );
        memcpy( info->pServiceChecksum, mui->service_checksum, sizeof(info->pServiceChecksum) );
    }
    if (flags & MUI_QUERY_LANGUAGE_NAME)
    {
        if (type == MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN && mui->fallback_lang_off)
        {
            info->dwLanguageNameOffset = offsetof(FILEMUIINFO, abBuffer);
            memcpy(info->abBuffer, ((BYTE *)mui) + mui->fallback_lang_off,
                    mui->fallback_lang_size);
            off += mui->fallback_lang_size;
        }
        if (type == MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI && mui->lang_off)
        {
            info->dwLanguageNameOffset = offsetof(FILEMUIINFO, abBuffer);
            memcpy(info->abBuffer, ((BYTE *)mui) + mui->lang_off, mui->lang_size);
            off += mui->lang_size;
        }
    }
    if (flags & MUI_QUERY_RESOURCE_TYPES && type & MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN)
    {
        if (mui->ln_type_id_size && mui->ln_type_id_off)
        {
            info->dwTypeIDMainSize = mui->ln_type_id_size / sizeof(DWORD);
            info->dwTypeIDMainOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->ln_type_id_off, mui->ln_type_id_size);
            off += mui->ln_type_id_size;
        }
        if (mui->ln_type_name_off)
        {
            info->dwTypeNameMainOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->ln_type_name_off, mui->ln_type_name_size);
            off += mui->ln_type_name_size;
        }
        if (mui->mui_type_id_size && mui->mui_type_id_off)
        {
            info->dwTypeIDMUISize = mui->mui_type_id_size / sizeof(DWORD);
            info->dwTypeIDMUIOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->mui_type_id_off, mui->mui_type_id_size);
            off += mui->mui_type_id_size;
        }
        if (mui->mui_type_name_off)
        {
            info->dwTypeNameMUIOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->mui_type_name_off, mui->mui_type_name_size);
            off += mui->mui_type_name_size;
        }
    }
    else if(flags & MUI_QUERY_RESOURCE_TYPES)
    {
        if (mui->ln_type_id_size && mui->ln_type_id_off)
        {
            info->dwTypeIDMUISize = mui->ln_type_id_size / sizeof(DWORD);
            info->dwTypeIDMUIOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->ln_type_id_off, mui->ln_type_id_size);
            off += mui->ln_type_id_size;
        }
        if (mui->ln_type_name_off)
        {
            info->dwTypeNameMUIOffset = offsetof(FILEMUIINFO, abBuffer[off]);
            memcpy(info->abBuffer + off, ((BYTE *)mui) + mui->ln_type_name_off, mui->ln_type_name_size);
            off += mui->ln_type_name_size;
        }
    }

    FreeLibrary( hmod );
    return TRUE;
}


/******************************************************************************
 *	GetFileMUIPath   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ GetFileMUIPath( DWORD flags, const WCHAR *filepath,
                                                    WCHAR *language, ULONG *languagelen,
                                                    WCHAR *muipath, ULONG *muipathlen,
                                                    ULONGLONG *enumerator )
{
    FIXME( "stub: 0x%lx, %s, %s, %p, %p, %p, %p\n", flags, debugstr_w(filepath),
           debugstr_w(language), languagelen, muipath, muipathlen, enumerator );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************************
 *	GetGeoInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetGeoInfoW( GEOID id, GEOTYPE type, WCHAR *data, int count, LANGID lang )
{
    const struct geo_id *ptr = find_geo_id_entry( id );

    TRACE( "%ld %ld %p %d %d\n", id, type, data, count, lang );

    if (!ptr)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_geo_info( ptr, type, data, count, lang );
}


INT WINAPI DECLSPEC_HOTPATCH GetGeoInfoEx( WCHAR *location, GEOTYPE type, WCHAR *data, int data_count )
{
    const struct geo_id *ptr = find_geo_name_entry( location );

    TRACE( "%s %lx %p %d\n", wine_dbgstr_w(location), type, data, data_count );

    if (!ptr)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (type == GEO_LCID || type == GEO_NATION || type == GEO_RFC1766)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    return get_geo_info( ptr, type, data, data_count, 0 );
}


/******************************************************************************
 *	GetLocaleInfoA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoA( LCID lcid, LCTYPE lctype, char *buffer, INT len )
{
    const NLS_LOCALE_DATA *locale;
    WCHAR *bufferW;
    INT lenW, ret;

    TRACE( "lcid=0x%lx lctype=0x%lx %p %d\n", lcid, lctype, buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (LOWORD(lctype) == LOCALE_SSHORTTIME || (lctype & LOCALE_RETURN_GENITIVE_NAMES))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!(locale = NlsValidateLocale( &lcid, 0 )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (LOWORD(lctype) == LOCALE_FONTSIGNATURE || (lctype & LOCALE_RETURN_NUMBER))
    {
        ret = get_locale_info( locale, lcid, lctype, (WCHAR *)buffer, len / sizeof(WCHAR) );
        return ret * sizeof(WCHAR);
    }

    if (!(lenW = get_locale_info( locale, lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = RtlAllocateHeap( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    ret = get_locale_info( locale, lcid, lctype, bufferW, lenW );
    if (ret) ret = WideCharToMultiByte( get_locale_codepage( locale, lctype ), 0,
                                        bufferW, ret, buffer, len, NULL, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, bufferW );
    return ret;
}


/******************************************************************************
 *	GetLocaleInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoW( LCID lcid, LCTYPE lctype, WCHAR *buffer, INT len )
{
    const NLS_LOCALE_DATA *locale;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d)\n", lcid, lctype, buffer, len );

    if (!(locale = NlsValidateLocale( &lcid, 0 )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_locale_info( locale, lcid, lctype, buffer, len );
}


/******************************************************************************
 *	GetLocaleInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoEx( const WCHAR *name, LCTYPE info, WCHAR *buffer, INT len )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    TRACE( "%s 0x%lx %p %d\n", debugstr_w(name), info, buffer, len );

    if (!locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_locale_info( locale, lcid, info, buffer, len );
}


/******************************************************************************
 *	GetNLSVersion   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNLSVersion( NLS_FUNCTION func, LCID lcid, NLSVERSIONINFO *info )
{
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];

    if (info->dwNLSVersionInfoSize < offsetof( NLSVERSIONINFO, dwEffectiveId ))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (!LCIDToLocaleName( lcid, locale, LOCALE_NAME_MAX_LENGTH, LOCALE_ALLOW_NEUTRAL_NAMES ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return GetNLSVersionEx( func, locale, (NLSVERSIONINFOEX *)info );
}


/******************************************************************************
 *	GetNLSVersionEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNLSVersionEx( NLS_FUNCTION func, const WCHAR *locale,
                                               NLSVERSIONINFOEX *info )
{
    const struct sortguid *sortid;

    if (func != COMPARE_STRING)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }
    if (info->dwNLSVersionInfoSize < sizeof(*info) &&
        (info->dwNLSVersionInfoSize != offsetof( NLSVERSIONINFO, dwEffectiveId )))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if (!(sortid = get_language_sort( locale ))) return FALSE;

    info->dwNLSVersion = info->dwDefinedVersion = sort.version;
    if (info->dwNLSVersionInfoSize >= sizeof(*info))
    {
        info->dwEffectiveId = LocaleNameToLCID( locale, 0 );
        info->guidCustomVersion = sortid->id;
    }
    return TRUE;
}


/******************************************************************************
 *	GetOEMCP   (kernelbase.@)
 */
UINT WINAPI GetOEMCP(void)
{
    return oem_cpinfo.CodePage;
}


/***********************************************************************
 *      GetProcessPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessPreferredUILanguages( DWORD flags, ULONG *count,
                                                              WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetProcessPreferredUILanguages( flags, count, buffer, size ));
}


/***********************************************************************
 *	GetStringTypeA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeA( LCID locale, DWORD type, const char *src, int count,
                                              WORD *chartype )
{
    UINT cp;
    INT countW;
    LPWSTR srcW;
    BOOL ret = FALSE;

    if (count == -1) count = strlen(src) + 1;

    cp = get_lcid_codepage( locale, 0 );
    countW = MultiByteToWideChar(cp, 0, src, count, NULL, 0);
    if((srcW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
    {
        MultiByteToWideChar(cp, 0, src, count, srcW, countW);
    /*
     * NOTE: the target buffer has 1 word for each CHARACTER in the source
     * string, with multibyte characters there maybe be more bytes in count
     * than character space in the buffer!
     */
        ret = GetStringTypeW(type, srcW, countW, chartype);
        HeapFree(GetProcessHeap(), 0, srcW);
    }
    return ret;
}


/***********************************************************************
 *	GetStringTypeW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeW( DWORD type, const WCHAR *src, INT count, WORD *chartype )
{
    if (!src)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (type != CT_CTYPE1 && type != CT_CTYPE2 && type != CT_CTYPE3)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (count == -1) count = lstrlenW(src) + 1;

    while (count--) *chartype++ = get_char_type( type, *src++ );

    return TRUE;
}


/***********************************************************************
 *	GetStringTypeExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeExW( LCID locale, DWORD type, const WCHAR *src, int count,
                                                WORD *chartype )
{
    /* locale is ignored for Unicode */
    return GetStringTypeW( type, src, count, chartype );
}


/***********************************************************************
 *	GetSystemDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLCID(void)
{
    return system_lcid;
}


/***********************************************************************
 *	GetSystemDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLangID(void)
{
    return LANGIDFROMLCID( GetSystemDefaultLCID() );
}


/***********************************************************************
 *	GetSystemDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLocaleName( LPWSTR name, INT count )
{
    return get_locale_info( system_locale, system_lcid, LOCALE_SNAME, name, count );
}


/***********************************************************************
 *	GetSystemDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryInstallUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *      GetSystemPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetSystemPreferredUILanguages( DWORD flags, ULONG *count,
                                                             WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetSystemPreferredUILanguages( flags, 0, count, buffer, size ));
}


/***********************************************************************
 *      GetThreadPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadPreferredUILanguages( DWORD flags, ULONG *count,
                                                             WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetThreadPreferredUILanguages( flags, count, buffer, size ));
}


/***********************************************************************
 *	GetTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTimeZoneInformation( TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    DWORD ret = GetDynamicTimeZoneInformation( &tzinfo );

    memcpy( info, &tzinfo, sizeof(*info) );
    return ret;
}


/***********************************************************************
 *	GetTimeZoneInformationForYear   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetTimeZoneInformationForYear( USHORT year,
                                                             DYNAMIC_TIME_ZONE_INFORMATION *dynamic,
                                                             TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION local_info;
    HKEY key = 0, dst_key;
    DWORD count;
    LRESULT ret;
    struct
    {
        LONG bias;
        LONG std_bias;
        LONG dlt_bias;
        SYSTEMTIME std_date;
        SYSTEMTIME dlt_date;
    } data;

    TRACE( "(%u,%p)\n", year, info );

    if (!dynamic)
    {
        if (GetDynamicTimeZoneInformation( &local_info ) == TIME_ZONE_ID_INVALID) return FALSE;
        dynamic = &local_info;
    }

    if ((ret = RegOpenKeyExW( tz_key, dynamic->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key ))) goto done;
    if (RegLoadMUIStringW( key, L"MUI_Std", info->StandardName,
                           sizeof(info->StandardName), NULL, 0, system_dir ))
    {
        count = sizeof(info->StandardName);
        if ((ret = RegQueryValueExW( key, L"Std", NULL, NULL, (BYTE *)info->StandardName, &count )))
            goto done;
    }
    if (RegLoadMUIStringW( key, L"MUI_Dlt", info->DaylightName,
                           sizeof(info->DaylightName), NULL, 0, system_dir ))
    {
        count = sizeof(info->DaylightName);
        if ((ret = RegQueryValueExW( key, L"Dlt", NULL, NULL, (BYTE *)info->DaylightName, &count )))
            goto done;
    }

    ret = ERROR_FILE_NOT_FOUND;
    if (!dynamic->DynamicDaylightTimeDisabled &&
        !RegOpenKeyExW( key, L"Dynamic DST", 0, KEY_ALL_ACCESS, &dst_key ))
    {
        WCHAR yearW[16];
        swprintf( yearW, ARRAY_SIZE(yearW), L"%u", year );
        count = sizeof(data);
        ret = RegQueryValueExW( dst_key, yearW, NULL, NULL, (BYTE *)&data, &count );
        RegCloseKey( dst_key );
    }
    if (ret)
    {
        count = sizeof(data);
        ret = RegQueryValueExW( key, L"TZI", NULL, NULL, (BYTE *)&data, &count );
    }

    if (!ret)
    {
        info->Bias = data.bias;
        info->StandardBias = data.std_bias;
        info->DaylightBias = data.dlt_bias;
        info->StandardDate = data.std_date;
        info->DaylightDate = data.dlt_date;
    }

done:
    RegCloseKey( key );
    if (ret) SetLastError( ret );
    return !ret;
}


/***********************************************************************
 *	GetUserDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLCID(void)
{
    return user_lcid;
}


/***********************************************************************
 *	GetUserDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLangID(void)
{
    return LANGIDFROMLCID( GetUserDefaultLCID() );
}


/***********************************************************************
 *	GetUserDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetUserDefaultLocaleName( LPWSTR name, INT len )
{
    return get_locale_info( user_locale, user_lcid, LOCALE_SNAME, name, len );
}


/***********************************************************************
 *	GetUserDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultUILanguage(void)
{
    return LANGIDFROMLCID( GetUserDefaultLCID() );
}


/******************************************************************************
 *	GetUserGeoID   (kernelbase.@)
 */
GEOID WINAPI DECLSPEC_HOTPATCH GetUserGeoID( GEOCLASS geoclass )
{
    GEOID ret = 39070;
    const WCHAR *name;
    WCHAR bufferW[40];
    HKEY hkey;

    switch (geoclass)
    {
    case GEOCLASS_NATION:
        name = L"Nation";
        break;
    case GEOCLASS_REGION:
        name = L"Region";
        break;
    default:
        WARN("Unknown geoclass %ld\n", geoclass);
        return GEOID_NOT_AVAILABLE;
    }
    if (!RegOpenKeyExW( intl_key, L"Geo", 0, KEY_ALL_ACCESS, &hkey ))
    {
        DWORD count = sizeof(bufferW);
        if (!RegQueryValueExW( hkey, name, NULL, NULL, (BYTE *)bufferW, &count ))
            ret = wcstol( bufferW, NULL, 10 );
        RegCloseKey( hkey );
    }
    return ret;
}


/******************************************************************************
 *      GetUserPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetUserPreferredUILanguages( DWORD flags, ULONG *count,
                                                           WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetUserPreferredUILanguages( flags, 0, count, buffer, size ));
}


/******************************************************************************
 *	IdnToAscii   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToAscii( DWORD flags, const WCHAR *src, INT srclen,
                                         WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToAscii( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IdnToNameprepUnicode   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToNameprepUnicode( DWORD flags, const WCHAR *src, INT srclen,
                                                   WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToNameprepUnicode( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IdnToUnicode   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToUnicode( DWORD flags, const WCHAR *src, INT srclen,
                                           WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToUnicode( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IsCharAlphaA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaA( CHAR c )
{
    WCHAR wc;
    DWORD reslen;
    RtlMultiByteToUnicodeN( &wc, sizeof(WCHAR), &reslen, &c, 1 );
    return reslen && (get_char_type( CT_CTYPE1, wc ) & C1_ALPHA);
}


/******************************************************************************
 *	IsCharAlphaW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_ALPHA);
}


/******************************************************************************
 *	IsCharAlphaNumericA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaNumericA( CHAR c )
{
    WCHAR wc;
    DWORD reslen;
    RtlMultiByteToUnicodeN( &wc, sizeof(WCHAR), &reslen, &c, 1 );
    return reslen && (get_char_type( CT_CTYPE1, wc ) & (C1_ALPHA | C1_DIGIT));
}


/******************************************************************************
 *	IsCharAlphaNumericW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaNumericW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & (C1_ALPHA | C1_DIGIT));
}


/******************************************************************************
 *	IsCharBlankW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharBlankW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_BLANK);
}


/******************************************************************************
 *	IsCharCntrlW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharCntrlW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_CNTRL);
}


/******************************************************************************
 *	IsCharDigitW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharDigitW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_DIGIT);
}


/******************************************************************************
 *	IsCharLowerA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharLowerA( CHAR c )
{
    WCHAR wc;
    DWORD reslen;
    RtlMultiByteToUnicodeN( &wc, sizeof(WCHAR), &reslen, &c, 1 );
    return reslen && (get_char_type( CT_CTYPE1, wc ) & C1_LOWER);
}


/******************************************************************************
 *	IsCharLowerW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharLowerW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_LOWER);
}


/******************************************************************************
 *	IsCharPunctW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharPunctW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_PUNCT);
}


/******************************************************************************
 *	IsCharSpaceA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharSpaceA( CHAR c )
{
    WCHAR wc;
    DWORD reslen;
    RtlMultiByteToUnicodeN( &wc, sizeof(WCHAR), &reslen, &c, 1 );
    return reslen && (get_char_type( CT_CTYPE1, wc ) & C1_SPACE);
}


/******************************************************************************
 *	IsCharSpaceW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharSpaceW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_SPACE);
}


/******************************************************************************
 *	IsCharUpperA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharUpperA( CHAR c )
{
    WCHAR wc;
    DWORD reslen;
    RtlMultiByteToUnicodeN( &wc, sizeof(WCHAR), &reslen, &c, 1 );
    return reslen && (get_char_type( CT_CTYPE1, wc ) & C1_UPPER);
}


/******************************************************************************
 *	IsCharUpperW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharUpperW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_UPPER);
}


/******************************************************************************
 *	IsCharXDigitW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharXDigitW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_XDIGIT);
}


/******************************************************************************
 *	IsDBCSLeadByte   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByte( BYTE testchar )
{
    return ansi_cpinfo.DBCSCodePage && ansi_cpinfo.DBCSOffsets[testchar];
}


/******************************************************************************
 *	IsDBCSLeadByteEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const CPTABLEINFO *table = get_codepage_table( codepage );
    return table && table->DBCSCodePage && table->DBCSOffsets[testchar];
}


/******************************************************************************
 *	IsNormalizedString   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsNormalizedString( NORM_FORM form, const WCHAR *str, INT len )
{
    BOOLEAN res;
    if (!set_ntstatus( RtlIsNormalizedString( form, str, len, &res ))) res = FALSE;
    return res;
}


/******************************************************************************
 *	IsValidCodePage   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidCodePage( UINT codepage )
{
    switch (codepage)
    {
    case CP_ACP:
    case CP_OEMCP:
    case CP_MACCP:
    case CP_THREAD_ACP:
        return FALSE;
    default:
        return get_codepage_table( codepage ) != NULL;
    }
}


/******************************************************************************
 *	IsValidLanguageGroup   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLanguageGroup( LGRPID id, DWORD flags )
{
    WCHAR name[10], value[10];
    DWORD type, value_len = sizeof(value);
    BOOL ret = FALSE;
    HKEY key;

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

    swprintf( name, ARRAY_SIZE(name), L"%x", id );
    if (!RegQueryValueExW( key, name, NULL, &type, (BYTE *)value, &value_len ) && type == REG_SZ)
        ret = (flags & LGRPID_SUPPORTED) || wcstoul( value, NULL, 10 );

    RegCloseKey( key );
    return ret;
}


/******************************************************************************
 *	IsValidLocale   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocale( LCID lcid, DWORD flags )
{
    switch (lcid)
    {
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_SYSTEM_DEFAULT:
        return FALSE;
    default:
        return !!NlsValidateLocale( &lcid, LOCALE_ALLOW_NEUTRAL_NAMES );
    }
}


/******************************************************************************
 *	IsValidLocaleName   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocaleName( const WCHAR *locale )
{
    if (locale == LOCALE_NAME_USER_DEFAULT) return FALSE;
    return !!find_lcname_entry( locale );
}


/******************************************************************************
 *	IsNLSDefinedString   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsNLSDefinedString( NLS_FUNCTION func, DWORD flags, NLSVERSIONINFO *info,
                                                  const WCHAR *str, int len )
{
    int i;

    if (func != COMPARE_STRING)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }
    if (info)
    {
        if (info->dwNLSVersionInfoSize != sizeof(*info) &&
            (info->dwNLSVersionInfoSize != offsetof( NLSVERSIONINFO, dwEffectiveId )))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }

    if (len < 0) len = lstrlenW( str ) + 1;

    for (i = 0; i < len; i++)
    {
        if (is_private_use_area_char( str[i] )) return FALSE;
        if (IS_LOW_SURROGATE( str[i] )) return FALSE;
        if (IS_HIGH_SURROGATE( str[i] ))
        {
            if (++i == len) return FALSE;
            if (!IS_LOW_SURROGATE( str[i] )) return FALSE;
            continue;
        }
        if (!(get_char_type( CT_CTYPE1, str[i] ) & C1_DEFINED)) return FALSE;
    }
    return TRUE;
}


/******************************************************************************
 *	IsValidNLSVersion   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH IsValidNLSVersion( NLS_FUNCTION func, const WCHAR *locale,
                                                  NLSVERSIONINFOEX *info )
{
    static const GUID GUID_NULL;
    NLSVERSIONINFOEX infoex;
    DWORD ret;

    if (func != COMPARE_STRING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (info->dwNLSVersionInfoSize < sizeof(*info) &&
        (info->dwNLSVersionInfoSize != offsetof( NLSVERSIONINFO, dwEffectiveId )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    infoex.dwNLSVersionInfoSize = sizeof(infoex);
    if (!GetNLSVersionEx( func, locale, &infoex )) return FALSE;

    ret = (infoex.dwNLSVersion & ~0xff) == (info->dwNLSVersion & ~0xff);
    if (ret && !IsEqualGUID( &info->guidCustomVersion, &GUID_NULL ))
        ret = find_sortguid( &info->guidCustomVersion ) != NULL;

    if (!ret) SetLastError( ERROR_SUCCESS );
    return ret;
}


/***********************************************************************
 *	LCIDToLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCIDToLocaleName( LCID lcid, WCHAR *name, INT count, DWORD flags )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, flags );

    if (!locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    return get_locale_info( locale, lcid, LOCALE_SNAME, name, count );
}


/***********************************************************************
 *	LCMapStringEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringEx( const WCHAR *locale, DWORD flags, const WCHAR *src, int srclen,
                                            WCHAR *dst, int dstlen, NLSVERSIONINFO *version,
                                            void *reserved, LPARAM handle )
{
    const struct sortguid *sortid = NULL;

    if (version) FIXME( "unsupported version structure %p\n", version );
    if (reserved) FIXME( "unsupported reserved pointer %p\n", reserved );
    if (handle)
    {
        static int once;
        if (!once++) FIXME( "unsupported lparam %Ix\n", handle );
    }

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    TRACE( "(%s,0x%08lx,%s,%d,%p,%d)\n",
           debugstr_w(locale), flags, debugstr_wn(src, srclen), srclen, dst, dstlen );

    flags &= ~LOCALE_USE_CP_ACP;

    if (src == dst && (flags & ~(LCMAP_LOWERCASE | LCMAP_UPPERCASE)))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (flags & (LCMAP_LOWERCASE | LCMAP_UPPERCASE | LCMAP_SORTKEY))
    {
        if (!(sortid = get_language_sort( locale ))) return 0;
    }
    if (flags & LCMAP_HASH)
    {
        FIXME( "LCMAP_HASH %s not supported\n", debugstr_wn( src, srclen ));
        return 0;
    }
    if (flags & LCMAP_SORTHANDLE)
    {
        FIXME( "LCMAP_SORTHANDLE not supported\n" );
        return 0;
    }
    if (flags & LCMAP_SORTKEY) return get_sortkey( sortid, flags, src, srclen, (BYTE *)dst, dstlen );

    return lcmap_string( sortid, flags, src, srclen, dst, dstlen );
}


/***********************************************************************
 *	LCMapStringA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringA( LCID lcid, DWORD flags, const char *src, int srclen,
                                           char *dst, int dstlen )
{
    WCHAR *bufW = NtCurrentTeb()->StaticUnicodeBuffer;
    LPWSTR srcW, dstW;
    INT ret = 0, srclenW, dstlenW;
    UINT locale_cp = CP_ACP;

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    locale_cp = get_lcid_codepage( lcid, flags );

    srclenW = MultiByteToWideChar( locale_cp, 0, src, srclen, bufW, 260 );
    if (srclenW) srcW = bufW;
    else
    {
        srclenW = MultiByteToWideChar( locale_cp, 0, src, srclen, NULL, 0 );
        srcW = HeapAlloc( GetProcessHeap(), 0, srclenW * sizeof(WCHAR) );
        if (!srcW)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        MultiByteToWideChar( locale_cp, 0, src, srclen, srcW, srclenW );
    }

    if (flags & LCMAP_SORTKEY)
    {
        if (src == dst)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            goto done;
        }
        ret = LCMapStringW( lcid, flags, srcW, srclenW, (WCHAR *)dst, dstlen );
        goto done;
    }

    if (flags & SORT_STRINGSORT)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        goto done;
    }

    dstlenW = LCMapStringW( lcid, flags, srcW, srclenW, NULL, 0 );
    if (!dstlenW) goto done;

    dstW = HeapAlloc( GetProcessHeap(), 0, dstlenW * sizeof(WCHAR) );
    if (!dstW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto done;
    }
    LCMapStringW( lcid, flags, srcW, srclenW, dstW, dstlenW );
    ret = WideCharToMultiByte( locale_cp, 0, dstW, dstlenW, dst, dstlen, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, dstW );

done:
    if (srcW != bufW) HeapFree( GetProcessHeap(), 0, srcW );
    return ret;
}


/***********************************************************************
 *	LCMapStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringW( LCID lcid, DWORD flags, const WCHAR *src, int srclen,
                                           WCHAR *dst, int dstlen )
{
    const WCHAR *locale = LOCALE_NAME_USER_DEFAULT;
    const NLS_LOCALE_LCID_INDEX *entry;

    switch (lcid)
    {
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_SYSTEM_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
    case LOCALE_CUSTOM_UNSPECIFIED:
    case LOCALE_CUSTOM_UI_DEFAULT:
        break;
    default:
        if (lcid == user_lcid || lcid == system_lcid) break;
        if (!(entry = find_lcid_entry( lcid )))
        {
            WARN( "unknown locale %04lx\n", lcid );
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        locale = locale_strings + entry->name + 1;
        break;
    }

    return LCMapStringEx( locale, flags, src, srclen, dst, dstlen, NULL, NULL, 0 );
}


/***********************************************************************
 *	LocaleNameToLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH LocaleNameToLCID( const WCHAR *name, DWORD flags )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (!locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(flags & LOCALE_ALLOW_NEUTRAL_NAMES) && !locale->inotneutral)
        lcid = locale->idefaultlanguage;
    return lcid;
}


/******************************************************************************
 *	MultiByteToWideChar   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH MultiByteToWideChar( UINT codepage, DWORD flags, const char *src, INT srclen,
                                                  WCHAR *dst, INT dstlen )
{
    const CPTABLEINFO *info;
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen < 0) srclen = strlen(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = mbstowcs_cpsymbol( flags, src, srclen, dst, dstlen );
        break;
    case CP_UTF7:
        ret = mbstowcs_utf7( flags, src, srclen, dst, dstlen );
        break;
    case CP_UNIXCP:
        codepage = unix_cp;
        /* fall through */
    default:
        if (!(info = get_codepage_table( codepage )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        if (flags & ~(MB_PRECOMPOSED | MB_COMPOSITE | MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        if (info->CodePage == CP_UTF8)
            ret = mbstowcs_utf8( flags, src, srclen, dst, dstlen );
        else
            ret = mbstowcs_codepage( info, flags, src, srclen, dst, dstlen );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_an(src, srclen), debugstr_wn(dst, ret), ret );
    return ret;
}


/******************************************************************************
 *	NormalizeString   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH NormalizeString(NORM_FORM form, const WCHAR *src, INT src_len,
                                             WCHAR *dst, INT dst_len)
{
    NTSTATUS status = RtlNormalizeString( form, src, src_len, dst, &dst_len );

    switch (status)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        status = STATUS_INVALID_PARAMETER;
        break;
    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_NO_UNICODE_TRANSLATION:
        dst_len = -dst_len;
        break;
    }
    SetLastError( RtlNtStatusToDosError( status ));
    return dst_len;
}


/******************************************************************************
 *	ResolveLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH ResolveLocaleName( LPCWSTR name, LPWSTR buffer, INT len )
{
    LCID lcid;
    UINT pos, datalen;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (!locale)
    {
        static const WCHAR valid[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        WCHAR *p, tmp[LOCALE_NAME_MAX_LENGTH];

        if (wcsspn( name, valid ) < wcslen( name ))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        lstrcpynW( tmp, name, LOCALE_NAME_MAX_LENGTH );
        while (!locale)
        {
            for (p = tmp + wcslen(tmp) - 1; p >= tmp; p--) if (*p == '-' || *p == '_') break;
            if (p <= tmp) break;
            *p = 0;
            locale = get_locale_by_name( tmp, &lcid );
        }
    }

    pos = locale ? (locale->inotneutral ? locale->sname : locale->ssortlocale) : 0;
    datalen = locale_strings[pos] + 1;

    if (!len) return datalen;
    lstrcpynW( buffer, locale_strings + pos + 1, len );
    if (datalen > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return datalen;
}


/******************************************************************************
 *	SetLocaleInfoW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetLocaleInfoW( LCID lcid, LCTYPE lctype, const WCHAR *data )
{
    WCHAR *str, tmp[80];

    if (!data)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (LOWORD(lctype))
    {
    case LOCALE_ICALENDARTYPE:      return set_registry_entry( &entry_icalendartype, data );
    case LOCALE_ICURRDIGITS:        return set_registry_entry( &entry_icurrdigits, data );
    case LOCALE_ICURRENCY:          return set_registry_entry( &entry_icurrency, data );
    case LOCALE_IDIGITS:            return set_registry_entry( &entry_idigits, data );
    case LOCALE_IDIGITSUBSTITUTION: return set_registry_entry( &entry_idigitsubstitution, data );
    case LOCALE_IFIRSTDAYOFWEEK:    return set_registry_entry( &entry_ifirstdayofweek, data );
    case LOCALE_IFIRSTWEEKOFYEAR:   return set_registry_entry( &entry_ifirstweekofyear, data );
    case LOCALE_ILZERO:             return set_registry_entry( &entry_ilzero, data );
    case LOCALE_IMEASURE:           return set_registry_entry( &entry_imeasure, data );
    case LOCALE_INEGCURR:           return set_registry_entry( &entry_inegcurr, data );
    case LOCALE_INEGNUMBER:         return set_registry_entry( &entry_inegnumber, data );
    case LOCALE_IPAPERSIZE:         return set_registry_entry( &entry_ipapersize, data );
    case LOCALE_S1159:              return set_registry_entry( &entry_s1159, data );
    case LOCALE_S2359:              return set_registry_entry( &entry_s2359, data );
    case LOCALE_SCURRENCY:          return set_registry_entry( &entry_scurrency, data );
    case LOCALE_SDECIMAL:           return set_registry_entry( &entry_sdecimal, data );
    case LOCALE_SGROUPING:          return set_registry_entry( &entry_sgrouping, data );
    case LOCALE_SLIST:              return set_registry_entry( &entry_slist, data );
    case LOCALE_SLONGDATE:          return set_registry_entry( &entry_slongdate, data );
    case LOCALE_SMONDECIMALSEP:     return set_registry_entry( &entry_smondecimalsep, data );
    case LOCALE_SMONGROUPING:       return set_registry_entry( &entry_smongrouping, data );
    case LOCALE_SMONTHOUSANDSEP:    return set_registry_entry( &entry_smonthousandsep, data );
    case LOCALE_SNATIVEDIGITS:      return set_registry_entry( &entry_snativedigits, data );
    case LOCALE_SNEGATIVESIGN:      return set_registry_entry( &entry_snegativesign, data );
    case LOCALE_SPOSITIVESIGN:      return set_registry_entry( &entry_spositivesign, data );
    case LOCALE_SSHORTTIME:         return set_registry_entry( &entry_sshorttime, data );
    case LOCALE_STHOUSAND:          return set_registry_entry( &entry_sthousand, data );
    case LOCALE_SYEARMONTH:         return set_registry_entry( &entry_syearmonth, data );

    case LOCALE_SDATE:
        if (!get_locale_info( user_locale, user_lcid, LOCALE_SSHORTDATE, tmp, ARRAY_SIZE(tmp) )) break;
        data = locale_replace_separator( tmp, data );
        /* fall through */
    case LOCALE_SSHORTDATE:
        if (!set_registry_entry( &entry_sshortdate, data )) return FALSE;
        update_registry_value( LOCALE_IDATE, NULL, L"iDate" );
        update_registry_value( LOCALE_SDATE, NULL, L"sDate" );
        return TRUE;

    case LOCALE_STIME:
        if (!get_locale_info( user_locale, user_lcid, LOCALE_STIMEFORMAT, tmp, ARRAY_SIZE(tmp) )) break;
        data = locale_replace_separator( tmp, data );
        /* fall through */
    case LOCALE_STIMEFORMAT:
        if (!set_registry_entry( &entry_stimeformat, data )) return FALSE;
        update_registry_value( LOCALE_ITIME, NULL, L"iTime" );
        update_registry_value( LOCALE_ITIMEMARKPOSN, NULL, L"iTimePrefix" );
        update_registry_value( LOCALE_ITLZERO, NULL, L"iTLZero" );
        update_registry_value( LOCALE_STIME, NULL, L"sTime" );
        return TRUE;

    case LOCALE_ITIME:
        if (!get_locale_info( user_locale, user_lcid, LOCALE_STIMEFORMAT, tmp, ARRAY_SIZE(tmp) )) break;
        if (!(str = find_format( tmp, L"Hh" ))) break;
        while (*str == 'h' || *str == 'H') *str++ = (*data == '0' ? 'h' : 'H');
        if (!set_registry_entry( &entry_stimeformat, tmp )) break;
        update_registry_value( LOCALE_ITIME, NULL, L"iTime" );
        return TRUE;

    case LOCALE_SINTLSYMBOL:
        if (!set_registry_entry( &entry_sintlsymbol, data )) return FALSE;
        /* if restoring the original value, restore the original LOCALE_SCURRENCY as well */
        if (!wcsicmp( data, locale_strings + user_locale->sintlsymbol + 1 ))
            data = locale_strings + user_locale->scurrency + 1;
        set_registry_entry( &entry_scurrency, data );
        return TRUE;
    }
    SetLastError( ERROR_INVALID_FLAGS );
    return FALSE;
}


/***********************************************************************
 *	SetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI /* DECLSPEC_HOTPATCH */ SetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type, const WCHAR *data )
{
    FIXME( "(%08lx,%08lx,%08lx,%s): stub\n", lcid, calendar, type, debugstr_w(data) );
    return 0;
}


/***********************************************************************
 *      SetProcessPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    return set_ntstatus( RtlSetProcessPreferredUILanguages( flags, buffer, count ));
}


/***********************************************************************
 *      SetThreadPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    return set_ntstatus( RtlSetThreadPreferredUILanguages( flags, buffer, count ));
}


/***********************************************************************
 *	SetTimeZoneInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetTimeZoneInformation( const TIME_ZONE_INFORMATION *info )
{
    return set_ntstatus( RtlSetTimeZoneInformation( (const RTL_TIME_ZONE_INFORMATION *)info ));
}


/******************************************************************************
 *	SetUserGeoID   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetUserGeoID( GEOID id )
{
    const struct geo_id *geo = find_geo_id_entry( id );
    WCHAR bufferW[10];
    HKEY hkey;

    if (!geo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!RegCreateKeyExW( intl_key, L"Geo", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        const WCHAR *name = geo->class == GEOCLASS_NATION ? L"Nation" : L"Region";
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"%u", geo->id );
        RegSetValueExW( hkey, name, 0, REG_SZ, (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );

        if (geo->class == GEOCLASS_NATION || wcscmp( geo->iso2, L"XX" ))
            lstrcpyW( bufferW, geo->iso2 );
        else
            swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03u", geo->uncode );
        RegSetValueExW( hkey, L"Name", 0, REG_SZ, (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
    return TRUE;
}


/***********************************************************************
 *	SystemTimeToTzSpecificLocalTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SystemTimeToTzSpecificLocalTime( const TIME_ZONE_INFORMATION *info,
                                                               const SYSTEMTIME *system,
                                                               SYSTEMTIME *local )
{
    TIME_ZONE_INFORMATION tzinfo;
    LARGE_INTEGER ft;

    if (!info)
    {
        RtlQueryTimeZoneInformation( (RTL_TIME_ZONE_INFORMATION *)&tzinfo );
        info = &tzinfo;
    }

    if (!SystemTimeToFileTime( system, (FILETIME *)&ft )) return FALSE;
    switch (get_timezone_id( info, ft, FALSE ))
    {
    case TIME_ZONE_ID_UNKNOWN:
        ft.QuadPart -= info->Bias * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_STANDARD:
        ft.QuadPart -= (info->Bias + info->StandardBias) * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        ft.QuadPart -= (info->Bias + info->DaylightBias) * (LONGLONG)600000000;
        break;
    default:
        return FALSE;
    }
    return FileTimeToSystemTime( (FILETIME *)&ft, local );
}


/***********************************************************************
 *	TzSpecificLocalTimeToSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TzSpecificLocalTimeToSystemTime( const TIME_ZONE_INFORMATION *info,
                                                               const SYSTEMTIME *local,
                                                               SYSTEMTIME *system )
{
    TIME_ZONE_INFORMATION tzinfo;
    LARGE_INTEGER ft;

    if (!info)
    {
        RtlQueryTimeZoneInformation( (RTL_TIME_ZONE_INFORMATION *)&tzinfo );
        info = &tzinfo;
    }

    if (!SystemTimeToFileTime( local, (FILETIME *)&ft )) return FALSE;
    switch (get_timezone_id( info, ft, TRUE ))
    {
    case TIME_ZONE_ID_UNKNOWN:
        ft.QuadPart += info->Bias * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_STANDARD:
        ft.QuadPart += (info->Bias + info->StandardBias) * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        ft.QuadPart += (info->Bias + info->DaylightBias) * (LONGLONG)600000000;
        break;
    default:
        return FALSE;
    }
    return FileTimeToSystemTime( (FILETIME *)&ft, system );
}


/***********************************************************************
 *	VerLanguageNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameA( DWORD lang, LPSTR buffer, DWORD size )
{
    return GetLocaleInfoA( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}


/***********************************************************************
 *	VerLanguageNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameW( DWORD lang, LPWSTR buffer, DWORD size )
{
    return GetLocaleInfoW( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}


/***********************************************************************
 *	WideCharToMultiByte   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH WideCharToMultiByte( UINT codepage, DWORD flags, LPCWSTR src, INT srclen,
                                                  LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    const CPTABLEINFO *info;
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = wcstombs_cpsymbol( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UTF7:
        ret = wcstombs_utf7( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UNIXCP:
        codepage = unix_cp;
        /* fall through */
    default:
        if (!(info = get_codepage_table( codepage )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        if (flags & ~(WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR | WC_ERR_INVALID_CHARS |
                      WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS))
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        if (info->CodePage == CP_UTF8)
            ret = wcstombs_utf8( flags, src, srclen, dst, dstlen, defchar, used );
        else
            ret = wcstombs_codepage( info, flags, src, srclen, dst, dstlen, defchar, used );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_wn(src, srclen), debugstr_an(dst, ret), ret );
    return ret;
}


/***********************************************************************
 *	GetUserDefaultGeoName  (kernelbase.@)
 */
INT WINAPI GetUserDefaultGeoName(LPWSTR geo_name, int count)
{
    WCHAR buffer[32];
    LSTATUS status;
    DWORD size;
    HKEY key;

    TRACE( "geo_name %p, count %d.\n", geo_name, count );

    if (count && !geo_name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(status = RegOpenKeyExW( intl_key, L"Geo", 0, KEY_ALL_ACCESS, &key )))
    {
        size = sizeof(buffer);
        status = RegQueryValueExW( key, L"Name", NULL, NULL, (BYTE *)buffer, &size );
        RegCloseKey( key );
    }
    if (status)
    {
        const struct geo_id *geo = find_geo_id_entry( GetUserGeoID( GEOCLASS_NATION ));
        if (geo && geo->id != 39070)
            lstrcpyW( buffer, geo->iso2 );
        else
            lstrcpyW( buffer, L"001" );
    }
    size = lstrlenW( buffer ) + 1;
    if (count < size)
    {
        if (!count)
            return size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    lstrcpyW( geo_name, buffer );
    return size;
}


/***********************************************************************
 *	SetUserDefaultGeoName  (kernelbase.@)
 */
BOOL WINAPI SetUserGeoName(PWSTR geo_name)
{
    const struct geo_id *geo;

    TRACE( "geo_name %s.\n", debugstr_w( geo_name ));

    if (!geo_name || !(geo = find_geo_name_entry( geo_name )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return SetUserGeoID( geo->id );
}


static void grouping_to_string( UINT grouping, WCHAR *buffer )
{
    UINT last_digit = grouping % 10;
    WCHAR tmp[10], *p = tmp;

    /* The string is confusingly different when it comes to repetitions (trailing zeros). For a string,
     * a 0 signals that the format needs to be repeated, which is the opposite of the grouping integer. */
    if (last_digit == 0)
    {
        grouping /= 10;

        /* Special case: two or more trailing zeros result in zero-sided groupings, with no repeats */
        if (grouping % 10 == 0)
            last_digit = ~0;
    }

    while (grouping)
    {
        *p++ = '0' + grouping % 10;
        grouping /= 10;
    }
    while (p > tmp)
    {
        *buffer++ = *(--p);
        if (p > tmp) *buffer++ = ';';
    }
    if (last_digit != 0)
    {
        *buffer++ = ';';
        *buffer++ = '0';
        if (last_digit == ~0)
        {
            /* Add another trailing zero due to the weird way trailing zeros work in grouping string */
            *buffer++ = ';';
            *buffer++ = '0';
        }
    }
    *buffer = 0;
}


static WCHAR *prepend_str( WCHAR *end, const WCHAR *str )
{
    UINT len = wcslen( str );
    return memcpy( end - len, str, len * sizeof(WCHAR) );
}


/* format a positive number with decimal part; helper for get_number_format */
static WCHAR *format_number( WCHAR *end, const WCHAR *value, const WCHAR *decimal_sep,
                             const WCHAR *thousand_sep, const WCHAR *grouping, UINT digits, BOOL lzero )
{
    BOOL round = FALSE, repeat = FALSE;
    UINT i, len = 0, prev = ~0;
    const WCHAR *frac = NULL;

    *(--end) = 0;

    for (i = 0; value[i]; i++)
    {
        if (value[i] >= '0' && value[i] <= '9') continue;
        if (value[i] != '.') return NULL;
        if (frac) return NULL;
        frac = value + i + 1;
    }

    /* format fractional part */

    len = frac ? wcslen( frac ) : 0;

    if (len > digits)
    {
        round = frac[digits] >= '5';
        len = digits;
    }
    while (digits > len)
    {
        (*--end) = '0';
        digits--;
    }
    while (len)
    {
        WCHAR ch = frac[--len];
        if (round)
        {
            if (ch != '9')
            {
                ch++;
                round = FALSE;
            }
            else ch = '0';
        }
        *(--end) = ch;
    }
    if (*end) end = prepend_str( end, decimal_sep );

    /* format integer part */

    len = frac ? frac - value - 1 : wcslen( value );

    while (len && *value == '0')
    {
        value++;
        len--;
    }
    if (len) lzero = FALSE;

    /* leading 0s are ignored */
    while (grouping[0] == '0' && grouping[1] == ';')
        grouping += 2;

    while (len)
    {
        UINT limit = prev;

        if (!repeat)
        {
            limit = *grouping - '0';
            if (grouping[1] == ';')
            {
                grouping += 2;
                if (limit)
                    prev = limit;
                else
                {
                    /* Trailing 0;0 is a special case */
                    prev = ~0;
                    if (grouping[0] == '0' && grouping[1] != ';')
                    {
                        repeat = TRUE;
                        limit = prev;
                    }
                }
            }
            else
            {
                repeat = TRUE;
                if (!limit)
                    limit = prev;
                else
                    prev = ~0;
            }
        }

        while (len && limit--)
        {
            WCHAR ch = value[--len];
            if (round)
            {
                if (ch != '9')
                {
                    ch++;
                    round = FALSE;
                }
                else ch = '0';
            }
            *(--end) = ch;
        }
        if (len) end = prepend_str( end, thousand_sep );
    }
    if (round) *(--end) = '1';
    else if (lzero) *(--end) = '0';
    return end;
}


static int get_number_format( const NLS_LOCALE_DATA *locale, DWORD flags, const WCHAR *value,
                              const NUMBERFMTW *format, WCHAR *buffer, int len )
{
    WCHAR *num, fmt_decimal[4], fmt_thousand[4], fmt_neg[5], grouping[24], output[256];
    const WCHAR *decimal_sep = fmt_decimal, *thousand_sep = fmt_thousand;
    DWORD digits, lzero, order;
    int ret = 0;
    BOOL negative = (*value == '-');

    flags &= LOCALE_NOUSEROVERRIDE;

    if (!format)
    {
        get_locale_info( locale, 0, LOCALE_SGROUPING | flags, grouping, ARRAY_SIZE(grouping) );
        get_locale_info( locale, 0, LOCALE_SDECIMAL | flags, fmt_decimal, ARRAY_SIZE(fmt_decimal) );
        get_locale_info( locale, 0, LOCALE_STHOUSAND | flags, fmt_thousand, ARRAY_SIZE(fmt_thousand) );
        get_locale_info( locale, 0, LOCALE_IDIGITS | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&digits, sizeof(DWORD)/sizeof(WCHAR) );
        get_locale_info( locale, 0, LOCALE_ILZERO | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&lzero, sizeof(DWORD)/sizeof(WCHAR) );
        get_locale_info( locale, 0, LOCALE_INEGNUMBER | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&order, sizeof(DWORD)/sizeof(WCHAR) );
    }
    else
    {
        if (flags)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        decimal_sep = format->lpDecimalSep;
        thousand_sep = format->lpThousandSep;
        grouping_to_string( format->Grouping, grouping );
        digits = format->NumDigits;
        lzero = format->LeadingZero;
        order = format->NegativeOrder;
        if (!decimal_sep || !thousand_sep)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    if (negative)
    {
        value++;
        get_locale_info( locale, 0, LOCALE_SNEGATIVESIGN | flags, fmt_neg, ARRAY_SIZE(fmt_neg) );
    }

    if (!(num = format_number( output + ARRAY_SIZE(output) - 6, value,
                               decimal_sep, thousand_sep, grouping, digits, lzero )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (negative)
    {
        switch (order)
        {
        case 0:  /* (1.1) */
            num = prepend_str( num, L"(" );
            wcscat( num, L")" );
            break;
        case 2:  /* - 1.1 */
            num = prepend_str( num, L" " );
            /* fall through */
        case 1:  /* -1.1 */
            num = prepend_str( num, fmt_neg );
            break;
        case 4:  /* 1.1 - */
            wcscat( num, L" " );
            /* fall through */
        case 3:  /* 1.1- */
            wcscat( num, fmt_neg );
            break;
        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    ret = wcslen( num ) + 1;
    if (!len) return ret;
    lstrcpynW( buffer, num, len );
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return ret;
}


static int get_currency_format( const NLS_LOCALE_DATA *locale, DWORD flags, const WCHAR *value,
                                const CURRENCYFMTW *format, WCHAR *buffer, int len )
{
    WCHAR *num, fmt_decimal[4], fmt_thousand[4], fmt_symbol[13], fmt_neg[5], grouping[20], output[256];
    const WCHAR *decimal_sep = fmt_decimal, *thousand_sep = fmt_thousand, *symbol = fmt_symbol;
    DWORD digits, lzero, pos_order, neg_order;
    int ret = 0;
    BOOL negative = (*value == '-');

    flags &= LOCALE_NOUSEROVERRIDE;

    if (!format)
    {
        get_locale_info( locale, 0, LOCALE_SCURRENCY | flags, fmt_symbol, ARRAY_SIZE(fmt_symbol) );
        get_locale_info( locale, 0, LOCALE_SMONGROUPING | flags, grouping, ARRAY_SIZE(grouping) );
        get_locale_info( locale, 0, LOCALE_SMONDECIMALSEP | flags, fmt_decimal, ARRAY_SIZE(fmt_decimal) );
        get_locale_info( locale, 0, LOCALE_SMONTHOUSANDSEP | flags, fmt_thousand, ARRAY_SIZE(fmt_thousand) );
        get_locale_info( locale, 0, LOCALE_ICURRDIGITS | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&digits, sizeof(DWORD)/sizeof(WCHAR) );
        get_locale_info( locale, 0, LOCALE_ILZERO | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&lzero, sizeof(DWORD)/sizeof(WCHAR) );
        get_locale_info( locale, 0, LOCALE_ICURRENCY | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&pos_order, sizeof(DWORD)/sizeof(WCHAR) );
        get_locale_info( locale, 0, LOCALE_INEGCURR | LOCALE_RETURN_NUMBER | flags,
                         (WCHAR *)&neg_order, sizeof(DWORD)/sizeof(WCHAR) );
    }
    else
    {
        if (flags)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        decimal_sep = format->lpDecimalSep;
        thousand_sep = format->lpThousandSep;
        symbol = format->lpCurrencySymbol;
        grouping_to_string( format->Grouping, grouping );
        digits = format->NumDigits;
        lzero = format->LeadingZero;
        pos_order = format->PositiveOrder;
        neg_order = format->NegativeOrder;
        if (!decimal_sep || !thousand_sep || !symbol)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    if (negative)
    {
        value++;
        get_locale_info( locale, 0, LOCALE_SNEGATIVESIGN | flags, fmt_neg, ARRAY_SIZE(fmt_neg) );
    }

    if (!(num = format_number( output + ARRAY_SIZE(output) - 20, value,
                               decimal_sep, thousand_sep, grouping, digits, lzero )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (negative)
    {
        switch (neg_order)
        {
        case 14:  /* ($ 1.1) */
            num = prepend_str( num, L" " );
            /* fall through */
        case 0:  /* ($1.1) */
            num = prepend_str( num, symbol );
            num = prepend_str( num, L"(" );
            wcscat( num, L")" );
            break;
        case 9:  /* -$ 1.1 */
            num = prepend_str( num, L" " );
            /* fall through */
        case 1:  /* -$1.1 */
            num = prepend_str( num, symbol );
            num = prepend_str( num, fmt_neg );
            break;
        case 2:  /* $-1.1 */
            num = prepend_str( num, fmt_neg );
            num = prepend_str( num, symbol );
            break;
        case 11:  /* $ 1.1- */
            num = prepend_str( num, L" " );
            /* fall through */
        case 3:  /* $1.1- */
            num = prepend_str( num, symbol );
            wcscat( num, fmt_neg );
            break;
        case 15:  /* (1.1 $) */
            wcscat( num, L" " );
            /* fall through */
        case 4:  /* (1.1$) */
            wcscat( num, symbol );
            num = prepend_str( num, L"(" );
            wcscat( num, L")" );
            break;
        case 8:  /* -1.1 $ */
            wcscat( num, L" " );
            /* fall through */
        case 5:  /* -1.1$ */
            num = prepend_str( num, fmt_neg );
            wcscat( num, symbol );
            break;
        case 6:  /* 1.1-$ */
            wcscat( num, fmt_neg );
            wcscat( num, symbol );
            break;
        case 10:  /* 1.1 $- */
            wcscat( num, L" " );
            /* fall through */
        case 7:  /* 1.1$- */
            wcscat( num, symbol );
            wcscat( num, fmt_neg );
            break;
        case 12:  /* $ -1.1 */
            num = prepend_str( num, fmt_neg );
            num = prepend_str( num, L" " );
            num = prepend_str( num, symbol );
            break;
        case 13:  /* 1.1- $ */
            wcscat( num, fmt_neg );
            wcscat( num, L" " );
            wcscat( num, symbol );
            break;
        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else
    {
        switch (pos_order)
        {
        case 2: /* $ 1.1 */
            num = prepend_str( num, L" " );
            /* fall through */
        case 0: /* $1.1 */
            num = prepend_str( num, symbol );
            break;
        case 3: /* 1.1 $ */
            wcscat( num, L" " );
            /* fall through */
        case 1: /* 1.1$ */
            wcscat( num, symbol );
            break;
        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }

    ret = wcslen( num ) + 1;
    if (!len) return ret;
    lstrcpynW( buffer, num, len );
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return ret;
}


/* get the length of a date/time formatting pattern */
static int get_pattern_len( const WCHAR *pattern, const WCHAR *accept )
{
    int i;

    if (*pattern == '\'')
    {
        for (i = 1; pattern[i]; i++)
        {
            if (pattern[i] != '\'') continue;
            if (pattern[++i] != '\'') return i;
        }
        return i;
    }
    if (!wcschr( accept, *pattern )) return 1;
    for (i = 1; pattern[i]; i++) if (pattern[i] != pattern[0]) break;
    return i;
}


static int get_date_format( const NLS_LOCALE_DATA *locale, DWORD flags, const SYSTEMTIME *systime,
                            const WCHAR *format, WCHAR *buffer, int len )
{
    DWORD override = flags & LOCALE_NOUSEROVERRIDE;
    DWORD genitive = 0;
    WCHAR *p, fmt[80], output[256];
    SYSTEMTIME time;
    int ret, val, count, i;

    if (!format)
    {
        if (flags & DATE_USE_ALT_CALENDAR) FIXME( "alt calendar not supported\n" );
        switch (flags & (DATE_SHORTDATE | DATE_LONGDATE | DATE_YEARMONTH | DATE_MONTHDAY))
        {
        case 0:
        case DATE_SHORTDATE:
            get_locale_info( locale, 0, LOCALE_SSHORTDATE | override, fmt, ARRAY_SIZE(fmt) );
            break;
        case DATE_LONGDATE:
            get_locale_info( locale, 0, LOCALE_SLONGDATE | override, fmt, ARRAY_SIZE(fmt) );
            break;
        case DATE_YEARMONTH:
            get_locale_info( locale, 0, LOCALE_SYEARMONTH | override, fmt, ARRAY_SIZE(fmt) );
            break;
        case DATE_MONTHDAY:
            get_locale_info( locale, 0, LOCALE_SMONTHDAY | override, fmt, ARRAY_SIZE(fmt) );
            break;
        default:
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        format = fmt;
    }
    else if (override || (flags & (DATE_SHORTDATE | DATE_LONGDATE | DATE_YEARMONTH | DATE_MONTHDAY)))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (systime)
    {
        FILETIME ft;

        time = *systime;
        time.wHour = time.wMinute = time.wSecond = time.wMilliseconds = 0;
        if (!SystemTimeToFileTime( &time, &ft ) || !FileTimeToSystemTime( &ft, &time )) return 0;
    }
    else GetLocalTime( &time );

    for (p = output; *format; format += count)
    {
        count = get_pattern_len( format, L"yMd" );

        switch (*format)
        {
        case '\'':
            for (i = 1; i < count; i++)
            {
                if (format[i] == '\'') i++;
                if (i < count) *p++ = format[i];
            }
            break;

        case 'y':
            p += swprintf( p, output + ARRAY_SIZE(output) - p, L"%02u",
                           (count <= 2) ? time.wYear % 100 : time.wYear );
            break;

        case 'M':
            if (count <= 2)
            {
                p += swprintf( p, output + ARRAY_SIZE(output) - p, L"%.*u", count, time.wMonth );
                break;
            }
            val = (count == 3 ? LOCALE_SABBREVMONTHNAME1 : LOCALE_SMONTHNAME1) + time.wMonth - 1;
            if (!genitive)
            {
                for (i = count; format[i]; i += get_pattern_len( format + i, L"yMd" ))
                {
                    if (format[i] != 'd') continue;
                    if (format[i + 1] != 'd' || format[i + 2] != 'd')
                        genitive = LOCALE_RETURN_GENITIVE_NAMES;
                    break;
                }
            }
            p += get_locale_info( locale, 0, val | override | genitive,
                                  p, output + ARRAY_SIZE(output) - p ) - 1;
            break;

        case 'd':
            if (count <= 2)
            {
                genitive = LOCALE_RETURN_GENITIVE_NAMES;
                p += swprintf( p, output + ARRAY_SIZE(output) - p, L"%.*u", count, time.wDay );
                break;
            }
            genitive = 0;
            val = (count == 3 ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1) + (time.wDayOfWeek + 6) % 7;
            p += get_locale_info( locale, 0, val | override, p, output + ARRAY_SIZE(output) - p ) - 1;
            break;

        case 'g':
            p += locale_return_string( count >= 2 ? locale->serastring : locale->sabbreverastring,
                                       override, p, output + ARRAY_SIZE(output) - p ) - 1;
            break;

        default:
            *p++ = *format;
            break;
        }
    }
    *p++ = 0;
    ret = p - output;

    if (!len) return ret;
    lstrcpynW( buffer, output, len );
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return ret;
}


static int get_time_format( const NLS_LOCALE_DATA *locale, DWORD flags, const SYSTEMTIME *systime,
                            const WCHAR *format, WCHAR *buffer, int len )
{
    DWORD override = flags & LOCALE_NOUSEROVERRIDE;
    WCHAR *p, *last, fmt[80], output[256];
    SYSTEMTIME time;
    int i, ret, val, count;
    BOOL skip = FALSE;

    if (!format)
    {
        get_locale_info( locale, 0, LOCALE_STIMEFORMAT | override, fmt, ARRAY_SIZE(fmt) );
        format = fmt;
    }
    else if (override)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (systime)
    {
        time = *systime;
        if (time.wMilliseconds > 999 || time.wSecond > 59 || time.wMinute > 59 || time.wHour > 23)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else GetLocalTime( &time );

    for (p = last = output; *format; format += count)
    {
        count = get_pattern_len( format, L"Hhmst" );

        switch (*format)
        {
        case '\'':
            for (i = 1; i < count; i++)
            {
                if (format[i] == '\'') i++;
                if (!skip && i < count) *p++ = format[i];
            }
            continue;

        case 'H':
            val = time.wHour;
            break;

        case 'h':
            val = time.wHour;
            if (!(flags & TIME_FORCE24HOURFORMAT))
            {
                val %= 12;
                if (!val) val = 12;
            }
            break;

        case 'm':
            if (flags & TIME_NOMINUTESORSECONDS)
            {
                p = last;
                skip = TRUE;
                continue;
            }
            val = time.wMinute;
            break;

        case 's':
            if (flags & (TIME_NOMINUTESORSECONDS | TIME_NOSECONDS))
            {
                p = last;
                skip = TRUE;
                continue;
            }
            val = time.wSecond;
            break;

        case 't':
            if (flags & TIME_NOTIMEMARKER)
            {
                p = last;
                skip = TRUE;
                continue;
            }
            val = time.wHour < 12 ? LOCALE_S1159 : LOCALE_S2359;
            ret = get_locale_info( locale, 0, val | override, p, output + ARRAY_SIZE(output) - p );
            p += (count > 1) ? ret - 1 : 1;
            skip = FALSE;
            continue;

        default:
            if (!skip || *format == ' ') *p++ = *format;
            continue;
        }

        p += swprintf( p, output + ARRAY_SIZE(output) - p, L"%.*u", min( 2, count ), val );
        last = p;
        skip = FALSE;
    }
    *p++ = 0;
    ret = p - output;

    if (!len) return ret;
    lstrcpynW( buffer, output, len );
    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return ret;
}


/**************************************************************************
 *	GetNumberFormatW  (kernelbase.@)
 */
int WINAPI GetNumberFormatW( LCID lcid, DWORD flags, const WCHAR *value,
                             const NUMBERFMTW *format, WCHAR *buffer, int len )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );

    if (len < 0 || (len && !buffer) || !value || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%04lx,%lx,%s,%p,%p,%d)\n", lcid, flags, debugstr_w(value), format, buffer, len );
    return get_number_format( locale, flags, value, format, buffer, len );
}


/**************************************************************************
 *	GetNumberFormatEx  (kernelbase.@)
 */
int WINAPI GetNumberFormatEx( const WCHAR *name, DWORD flags, const WCHAR *value,
                              const NUMBERFMTW *format, WCHAR *buffer, int len )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (len < 0 || (len && !buffer) || !value || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%s,%lx,%s,%p,%p,%d)\n", debugstr_w(name), flags, debugstr_w(value), format, buffer, len );
    return get_number_format( locale, flags, value, format, buffer, len );
}


/***********************************************************************
 *	GetCurrencyFormatW  (kernelbase.@)
 */
int WINAPI GetCurrencyFormatW( LCID lcid, DWORD flags, const WCHAR *value,
                               const CURRENCYFMTW *format, WCHAR *buffer, int len )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );

    if (len < 0 || (len && !buffer) || !value || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%04lx,%lx,%s,%p,%p,%d)\n", lcid, flags, debugstr_w(value), format, buffer, len );
    return get_currency_format( locale, flags, value, format, buffer, len );
}


/***********************************************************************
 *	GetCurrencyFormatEx  (kernelbase.@)
 */
int WINAPI GetCurrencyFormatEx( const WCHAR *name, DWORD flags, const WCHAR *value,
                                const CURRENCYFMTW *format, WCHAR *buffer, int len )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (len < 0 || (len && !buffer) || !value || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%s,%lx,%s,%p,%p,%d)\n", debugstr_w(name), flags, debugstr_w(value), format, buffer, len );
    return get_currency_format( locale, flags, value, format, buffer, len );
}


/******************************************************************************
 *           GetDateFormatA (KERNEL32.@)
 */
int WINAPI GetDateFormatA( LCID lcid, DWORD flags, const SYSTEMTIME *time,
                           const char *format, char *buffer, int len )
{
    UINT cp = get_lcid_codepage( lcid, flags );
    WCHAR formatW[128], output[128];
    int ret;

    TRACE( "(0x%04lx,0x%08lx,%p,%s,%p,%d)\n", lcid, flags, time, debugstr_a(format), buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (format)
    {
        MultiByteToWideChar( cp, 0, format, -1, formatW, ARRAY_SIZE(formatW) );
        ret = GetDateFormatW( lcid, flags, time, formatW, output, ARRAY_SIZE(output) );
    }
    else ret = GetDateFormatW( lcid, flags, time, NULL, output, ARRAY_SIZE(output) );

    if (ret) ret = WideCharToMultiByte( cp, 0, output, -1, buffer, len, 0, 0 );
    return ret;
}


/***********************************************************************
 *	GetDateFormatW  (kernelbase.@)
 */
int WINAPI GetDateFormatW( LCID lcid, DWORD flags, const SYSTEMTIME *systime,
                           const WCHAR *format, WCHAR *buffer, int len )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );

    if (len < 0 || (len && !buffer) || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%04lx,%lx,%p,%s,%p,%d)\n", lcid, flags, systime, debugstr_w(format), buffer, len );
    return get_date_format( locale, flags, systime, format, buffer, len );
}


/***********************************************************************
 *	GetDateFormatEx  (kernelbase.@)
 */
int WINAPI GetDateFormatEx( const WCHAR *name, DWORD flags, const SYSTEMTIME *systime,
                            const WCHAR *format, WCHAR *buffer, int len, const WCHAR *calendar )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (len < 0 || (len && !buffer) || !locale || calendar)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%s,%lx,%p,%s,%p,%d)\n", debugstr_w(name), flags, systime, debugstr_w(format), buffer, len );
    return get_date_format( locale, flags, systime, format, buffer, len );
}


/******************************************************************************
 *	GetTimeFormatA  (kernelbase.@)
 */
int WINAPI GetTimeFormatA( LCID lcid, DWORD flags, const SYSTEMTIME *time,
                           const char *format, char *buffer, int len )
{
    UINT cp = get_lcid_codepage( lcid, flags );
    WCHAR formatW[128], output[128];
    int ret;

    TRACE( "(0x%04lx,0x%08lx,%p,%s,%p,%d)\n", lcid, flags, time, debugstr_a(format), buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (format)
    {
        MultiByteToWideChar( cp, 0, format, -1, formatW, ARRAY_SIZE(formatW) );
        ret = GetTimeFormatW( lcid, flags, time, formatW, output, ARRAY_SIZE(output) );
    }
    else ret = GetTimeFormatW( lcid, flags, time, NULL, output, ARRAY_SIZE(output) );

    if (ret) ret = WideCharToMultiByte( cp, 0, output, -1, buffer, len, 0, 0 );
    return ret;
}


/***********************************************************************
 *	GetTimeFormatW  (kernelbase.@)
 */
int WINAPI GetTimeFormatW( LCID lcid, DWORD flags, const SYSTEMTIME *systime,
                            const WCHAR *format, WCHAR *buffer, int len )
{
    const NLS_LOCALE_DATA *locale = NlsValidateLocale( &lcid, 0 );

    if (len < 0 || (len && !buffer) || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%04lx,%lx,%p,%s,%p,%d)\n", lcid, flags, systime, debugstr_w(format), buffer, len );
    return get_time_format( locale, flags, systime, format, buffer, len );
}


/***********************************************************************
 *	GetTimeFormatEx  (kernelbase.@)
 */
int WINAPI GetTimeFormatEx( const WCHAR *name, DWORD flags, const SYSTEMTIME *systime,
                            const WCHAR *format, WCHAR *buffer, int len )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (len < 0 || (len && !buffer) || !locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    TRACE( "(%s,%lx,%p,%s,%p,%d)\n", debugstr_w(name), flags, systime, debugstr_w(format), buffer, len );
    return get_time_format( locale, flags, systime, format, buffer, len );
}
