/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(nls);

#include "lcformat_private.h"
#ifdef __REACTOS__
    #include "japanese.h"
    #define strcasecmp _stricmp
#endif

INT WINAPI CompareStringEx(LPCWSTR locale, DWORD flags, LPCWSTR str1, INT len1,
                           LPCWSTR str2, INT len2, LPNLSVERSIONINFO version, LPVOID reserved, LPARAM lParam);

#undef WINVER
#define WINVER DLL_EXPORT_VERSION

/* From winnls.h */
#define LOCALE_NAME_USER_DEFAULT    NULL

#define REG_SZ 1
extern int wine_fold_string(int flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen);
extern int wine_get_sortkey(int flags, const WCHAR *src, int srclen, char *dst, int dstlen);
extern int wine_compare_string(int flags, const WCHAR *str1, int len1, const WCHAR *str2, int len2);
#ifdef __REACTOS__
extern UINT GetLocalisedText(IN UINT uID, IN LPWSTR lpszDest, IN UINT cchDest, IN LANGID lang);
#else
extern UINT GetLocalisedText(IN UINT uID, IN LPWSTR lpszDest, IN UINT cchDest);
#endif
#define NLSRC_OFFSET 5000 /* FIXME */

extern HMODULE kernel32_handle;

#define LOCALE_LOCALEINFOFLAGSMASK (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP|\
                                    LOCALE_RETURN_NUMBER|LOCALE_RETURN_GENITIVE_NAMES)
#define MB_FLAGSMASK (MB_PRECOMPOSED|MB_COMPOSITE|MB_USEGLYPHCHARS|MB_ERR_INVALID_CHARS)
#define WC_FLAGSMASK (WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR|WC_ERR_INVALID_CHARS|\
                      WC_COMPOSITECHECK|WC_NO_BEST_FIT_CHARS)

/* current code pages */
static const union cptable *ansi_cptable;
static const union cptable *oem_cptable;
static const union cptable *mac_cptable;
static const union cptable *unix_cptable;  /* NULL if UTF8 */

static const WCHAR szLocaleKeyName[] = {
    '\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
    'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
    'C','o','n','t','r','o','l','\\','N','l','s','\\','L','o','c','a','l','e',0
};

static const WCHAR szLangGroupsKeyName[] = {
    '\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
    'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
    'C','o','n','t','r','o','l','\\','N','l','s','\\',
    'L','a','n','g','u','a','g','e',' ','G','r','o','u','p','s',0
};

#if (WINVER >= 0x0600)
/* Charset to codepage map, sorted by name. */
static const struct charset_entry
{
    const char *charset_name;
    UINT        codepage;
} charset_names[] =
{
    { "BIG5", 950 },
    { "CP1250", 1250 },
    { "CP1251", 1251 },
    { "CP1252", 1252 },
    { "CP1253", 1253 },
    { "CP1254", 1254 },
    { "CP1255", 1255 },
    { "CP1256", 1256 },
    { "CP1257", 1257 },
    { "CP1258", 1258 },
    { "CP932", 932 },
    { "CP936", 936 },
    { "CP949", 949 },
    { "CP950", 950 },
    { "EUCJP", 20932 },
    { "GB2312", 936 },
    { "IBM037", 37 },
    { "IBM1026", 1026 },
    { "IBM424", 424 },
    { "IBM437", 437 },
    { "IBM500", 500 },
    { "IBM850", 850 },
    { "IBM852", 852 },
    { "IBM855", 855 },
    { "IBM857", 857 },
    { "IBM860", 860 },
    { "IBM861", 861 },
    { "IBM862", 862 },
    { "IBM863", 863 },
    { "IBM864", 864 },
    { "IBM865", 865 },
    { "IBM866", 866 },
    { "IBM869", 869 },
    { "IBM874", 874 },
    { "IBM875", 875 },
    { "ISO88591", 28591 },
    { "ISO885910", 28600 },
    { "ISO885913", 28603 },
    { "ISO885914", 28604 },
    { "ISO885915", 28605 },
    { "ISO885916", 28606 },
    { "ISO88592", 28592 },
    { "ISO88593", 28593 },
    { "ISO88594", 28594 },
    { "ISO88595", 28595 },
    { "ISO88596", 28596 },
    { "ISO88597", 28597 },
    { "ISO88598", 28598 },
    { "ISO88599", 28599 },
    { "KOI8R", 20866 },
    { "KOI8U", 21866 },
    { "UTF8", CP_UTF8 }
};
#endif


struct locale_name
{
    WCHAR  win_name[128];   /* Windows name ("en-US") */
    WCHAR  lang[128];       /* language ("en") (note: buffer contains the other strings too) */
    WCHAR *country;         /* country ("US") */
    WCHAR *charset;         /* charset ("UTF-8") for Unix format only */
    WCHAR *script;          /* script ("Latn") for Windows format only */
    WCHAR *modifier;        /* modifier or sort order */
    LCID   lcid;            /* corresponding LCID */
    int    matches;         /* number of elements matching LCID (0..4) */
    UINT   codepage;        /* codepage corresponding to charset */
};

/* locale ids corresponding to the various Unix locale parameters */
static LCID lcid_LC_COLLATE;
static LCID lcid_LC_CTYPE;
static LCID lcid_LC_MESSAGES;
static LCID lcid_LC_MONETARY;
static LCID lcid_LC_NUMERIC;
static LCID lcid_LC_TIME;
static LCID lcid_LC_PAPER;
static LCID lcid_LC_MEASUREMENT;
static LCID lcid_LC_TELEPHONE;

static const WCHAR iCalendarTypeW[] = {'i','C','a','l','e','n','d','a','r','T','y','p','e',0};
static const WCHAR iCountryW[] = {'i','C','o','u','n','t','r','y',0};
static const WCHAR iCurrDigitsW[] = {'i','C','u','r','r','D','i','g','i','t','s',0};
static const WCHAR iCurrencyW[] = {'i','C','u','r','r','e','n','c','y',0};
static const WCHAR iDateW[] = {'i','D','a','t','e',0};
static const WCHAR iDigitsW[] = {'i','D','i','g','i','t','s',0};
static const WCHAR iFirstDayOfWeekW[] = {'i','F','i','r','s','t','D','a','y','O','f','W','e','e','k',0};
static const WCHAR iFirstWeekOfYearW[] = {'i','F','i','r','s','t','W','e','e','k','O','f','Y','e','a','r',0};
static const WCHAR iLDateW[] = {'i','L','D','a','t','e',0};
static const WCHAR iLZeroW[] = {'i','L','Z','e','r','o',0};
static const WCHAR iMeasureW[] = {'i','M','e','a','s','u','r','e',0};
static const WCHAR iNegCurrW[] = {'i','N','e','g','C','u','r','r',0};
static const WCHAR iNegNumberW[] = {'i','N','e','g','N','u','m','b','e','r',0};
static const WCHAR iPaperSizeW[] = {'i','P','a','p','e','r','S','i','z','e',0};
static const WCHAR iTLZeroW[] = {'i','T','L','Z','e','r','o',0};
static const WCHAR iTimePrefixW[] = {'i','T','i','m','e','P','r','e','f','i','x',0};
static const WCHAR iTimeW[] = {'i','T','i','m','e',0};
static const WCHAR s1159W[] = {'s','1','1','5','9',0};
static const WCHAR s2359W[] = {'s','2','3','5','9',0};
static const WCHAR sCountryW[] = {'s','C','o','u','n','t','r','y',0};
static const WCHAR sCurrencyW[] = {'s','C','u','r','r','e','n','c','y',0};
static const WCHAR sDateW[] = {'s','D','a','t','e',0};
static const WCHAR sDecimalW[] = {'s','D','e','c','i','m','a','l',0};
static const WCHAR sGroupingW[] = {'s','G','r','o','u','p','i','n','g',0};
static const WCHAR sLanguageW[] = {'s','L','a','n','g','u','a','g','e',0};
static const WCHAR sListW[] = {'s','L','i','s','t',0};
static const WCHAR sLongDateW[] = {'s','L','o','n','g','D','a','t','e',0};
static const WCHAR sMonDecimalSepW[] = {'s','M','o','n','D','e','c','i','m','a','l','S','e','p',0};
static const WCHAR sMonGroupingW[] = {'s','M','o','n','G','r','o','u','p','i','n','g',0};
static const WCHAR sMonThousandSepW[] = {'s','M','o','n','T','h','o','u','s','a','n','d','S','e','p',0};
static const WCHAR sNativeDigitsW[] = {'s','N','a','t','i','v','e','D','i','g','i','t','s',0};
static const WCHAR sNegativeSignW[] = {'s','N','e','g','a','t','i','v','e','S','i','g','n',0};
static const WCHAR sPositiveSignW[] = {'s','P','o','s','i','t','i','v','e','S','i','g','n',0};
static const WCHAR sShortDateW[] = {'s','S','h','o','r','t','D','a','t','e',0};
static const WCHAR sThousandW[] = {'s','T','h','o','u','s','a','n','d',0};
static const WCHAR sTimeFormatW[] = {'s','T','i','m','e','F','o','r','m','a','t',0};
static const WCHAR sTimeW[] = {'s','T','i','m','e',0};
static const WCHAR sYearMonthW[] = {'s','Y','e','a','r','M','o','n','t','h',0};
static const WCHAR NumShapeW[] = {'N','u','m','s','h','a','p','e',0};

static struct registry_value
{
    DWORD           lctype;
    const WCHAR    *name;
    WCHAR          *cached_value;
} registry_values[] =
{
    { LOCALE_ICALENDARTYPE, iCalendarTypeW },
    { LOCALE_ICURRDIGITS, iCurrDigitsW },
    { LOCALE_ICURRENCY, iCurrencyW },
    { LOCALE_IDIGITS, iDigitsW },
    { LOCALE_IFIRSTDAYOFWEEK, iFirstDayOfWeekW },
    { LOCALE_IFIRSTWEEKOFYEAR, iFirstWeekOfYearW },
    { LOCALE_ILZERO, iLZeroW },
    { LOCALE_IMEASURE, iMeasureW },
    { LOCALE_INEGCURR, iNegCurrW },
    { LOCALE_INEGNUMBER, iNegNumberW },
    { LOCALE_IPAPERSIZE, iPaperSizeW },
    { LOCALE_ITIME, iTimeW },
    { LOCALE_S1159, s1159W },
    { LOCALE_S2359, s2359W },
    { LOCALE_SCURRENCY, sCurrencyW },
    { LOCALE_SDATE, sDateW },
    { LOCALE_SDECIMAL, sDecimalW },
    { LOCALE_SGROUPING, sGroupingW },
    { LOCALE_SLIST, sListW },
    { LOCALE_SLONGDATE, sLongDateW },
    { LOCALE_SMONDECIMALSEP, sMonDecimalSepW },
    { LOCALE_SMONGROUPING, sMonGroupingW },
    { LOCALE_SMONTHOUSANDSEP, sMonThousandSepW },
    { LOCALE_SNEGATIVESIGN, sNegativeSignW },
    { LOCALE_SPOSITIVESIGN, sPositiveSignW },
    { LOCALE_SSHORTDATE, sShortDateW },
    { LOCALE_STHOUSAND, sThousandW },
    { LOCALE_STIME, sTimeW },
    { LOCALE_STIMEFORMAT, sTimeFormatW },
    { LOCALE_SYEARMONTH, sYearMonthW },
    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    { LOCALE_ICOUNTRY, iCountryW },
    { LOCALE_IDATE, iDateW },
    { LOCALE_ILDATE, iLDateW },
    { LOCALE_ITLZERO, iTLZeroW },
    { LOCALE_SCOUNTRY, sCountryW },
    { LOCALE_SABBREVLANGNAME, sLanguageW },
    /* The following are used in XP and later */
    { LOCALE_IDIGITSUBSTITUTION, NumShapeW },
    { LOCALE_SNATIVEDIGITS, sNativeDigitsW },
    { LOCALE_ITIMEMARKPOSN, iTimePrefixW }
};

static RTL_CRITICAL_SECTION cache_section = { NULL, -1, 0, 0, 0, 0 };

#ifndef __REACTOS__
/* Copy Ascii string to Unicode without using codepages */
static inline void strcpynAtoW( WCHAR *dst, const char *src, size_t n )
{
    while (n > 1 && *src)
    {
        *dst++ = (unsigned char)*src++;
        n--;
    }
    if (n) *dst = 0;
}

static inline unsigned short get_table_entry( const unsigned short *table, WCHAR ch )
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}
#endif // !__REACTOS__

/***********************************************************************
 *		get_lcid_codepage
 *
 * Retrieve the ANSI codepage for a given locale.
 */
static inline UINT get_lcid_codepage( LCID lcid )
{
    UINT ret;
    if (!GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (WCHAR *)&ret,
                         sizeof(ret)/sizeof(WCHAR) )) ret = 0;
    return ret;
}

#ifndef __REACTOS__
/***********************************************************************
 *		get_codepage_table
 *
 * Find the table for a given codepage, handling CP_ACP etc. pseudo-codepages
 */
static const union cptable *get_codepage_table( unsigned int codepage )
{
    const union cptable *ret = NULL;

    assert( ansi_cptable );  /* init must have been done already */

    switch(codepage)
    {
    case CP_ACP:
        return ansi_cptable;
    case CP_OEMCP:
        return oem_cptable;
    case CP_MACCP:
        return mac_cptable;
    case CP_UTF7:
    case CP_UTF8:
        break;
    case CP_THREAD_ACP:
        if (NtCurrentTeb()->CurrentLocale == GetUserDefaultLCID()) return ansi_cptable;
        codepage = get_lcid_codepage( NtCurrentTeb()->CurrentLocale );
        if (!codepage) return ansi_cptable;
        /* fall through */
    default:
        if (codepage == ansi_cptable->info.codepage) return ansi_cptable;
        if (codepage == oem_cptable->info.codepage) return oem_cptable;
        if (codepage == mac_cptable->info.codepage) return mac_cptable;
        ret = wine_cp_get_table( codepage );
        break;
    }
    return ret;
}
#endif // !__REACTOS__

#if (WINVER >= 0x0600)
/***********************************************************************
 *              charset_cmp (internal)
 */
static int charset_cmp( const void *name, const void *entry )
{
    const struct charset_entry *charset = entry;
    return strcasecmp( name, charset->charset_name );
}

/***********************************************************************
 *		find_charset
 */
static UINT find_charset( const WCHAR *name )
{
    const struct charset_entry *entry;
    char charset_name[16];
    size_t i, j;

    /* remove punctuation characters from charset name */
    for (i = j = 0; name[i] && j < sizeof(charset_name)-1; i++)
        if (isalnum((unsigned char)name[i])) charset_name[j++] = name[i];
    charset_name[j] = 0;

    entry = bsearch( charset_name, charset_names,
                     sizeof(charset_names)/sizeof(charset_names[0]),
                     sizeof(charset_names[0]), charset_cmp );
    if (entry) return entry->codepage;
    return 0;
}
#endif // (WINVER >= 0x0600)

static LANGID get_default_sublang( LANGID lang )
{
    switch (lang)
    {
    case MAKELANGID( LANG_SPANISH, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH_MODERN );
    case MAKELANGID( LANG_CHINESE, SUBLANG_NEUTRAL ):
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
    case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE ):
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
    case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL ):
    case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_MACAU ):
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG );
    }
    if (SUBLANGID( lang ) == SUBLANG_NEUTRAL) lang = MAKELANGID( PRIMARYLANGID(lang), SUBLANG_DEFAULT );
    return lang;
}

#if (WINVER >= 0x0600)
/***********************************************************************
 *           find_locale_id_callback
 */
static BOOL CALLBACK find_locale_id_callback( HMODULE hModule, LPCWSTR type,
                                              LPCWSTR name, LANGID lang, LPARAM lParam )
{
    struct locale_name *data = (struct locale_name *)lParam;
    WCHAR buffer[128];
    int matches = 0;
    LCID lcid = MAKELCID( lang, SORT_DEFAULT );  /* FIXME: handle sort order */

    if (PRIMARYLANGID(lang) == LANG_NEUTRAL) return TRUE; /* continue search */

    /* first check exact name */
    if (data->win_name[0] &&
        GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE,
                        buffer, sizeof(buffer)/sizeof(WCHAR) ))
    {
        if (!strcmpiW( data->win_name, buffer ))
        {
            matches = 4;  /* everything matches */
            goto done;
        }
    }

    if (!GetLocaleInfoW( lcid, LOCALE_SISO639LANGNAME | LOCALE_NOUSEROVERRIDE,
                         buffer, sizeof(buffer)/sizeof(WCHAR) ))
        return TRUE;
    if (strcmpiW( buffer, data->lang )) return TRUE;
    matches++;  /* language name matched */

    if (data->script)
    {
        if (GetLocaleInfoW( lcid, LOCALE_SSCRIPTS | LOCALE_NOUSEROVERRIDE,
                            buffer, sizeof(buffer)/sizeof(WCHAR) ))
        {
            const WCHAR *p = buffer;
            unsigned int len = strlenW( data->script );
            while (*p)
            {
                if (!strncmpiW( p, data->script, len ) && (!p[len] || p[len] == ';')) break;
                if (!(p = strchrW( p, ';'))) goto done;
                p++;
            }
            if (!*p) goto done;
            matches++;  /* script matched */
        }
    }

    if (data->country)
    {
        if (GetLocaleInfoW( lcid, LOCALE_SISO3166CTRYNAME|LOCALE_NOUSEROVERRIDE,
                            buffer, sizeof(buffer)/sizeof(WCHAR) ))
        {
            if (strcmpiW( buffer, data->country )) goto done;
            matches++;  /* country name matched */
        }
    }
    else  /* match default language */
    {
        LANGID def_lang = data->script ? lang : MAKELANGID( PRIMARYLANGID(lang), LANG_NEUTRAL );
        if (lang == get_default_sublang( def_lang )) matches++;
    }

    if (data->codepage)
    {
        UINT unix_cp;
        if (GetLocaleInfoW( lcid, LOCALE_IDEFAULTUNIXCODEPAGE | LOCALE_RETURN_NUMBER,
                            (LPWSTR)&unix_cp, sizeof(unix_cp)/sizeof(WCHAR) ))
        {
            if (unix_cp == data->codepage) matches++;
        }
    }

    /* FIXME: check sort order */

done:
    if (matches > data->matches)
    {
        data->lcid = lcid;
        data->matches = matches;
    }
    return (data->matches < 4);  /* no need to continue for perfect match */
}


/***********************************************************************
 *		parse_locale_name
 *
 * Parse a locale name into a struct locale_name, handling both Windows and Unix formats.
 * Unix format is: lang[_country][.charset][@modifier]
 * Windows format is: lang[-script][-country][_modifier]
 */
static void parse_locale_name( const WCHAR *str, struct locale_name *name )
{
    static const WCHAR sepW[] = {'-','_','.','@',0};
    static const WCHAR winsepW[] = {'-','_',0};
    static const WCHAR posixW[] = {'P','O','S','I','X',0};
    static const WCHAR cW[] = {'C',0};
    static const WCHAR latinW[] = {'l','a','t','i','n',0};
    static const WCHAR latnW[] = {'-','L','a','t','n',0};
    WCHAR *p;

    TRACE("%s\n", debugstr_w(str));

    name->country = name->charset = name->script = name->modifier = NULL;
    name->lcid = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );
    name->matches = 0;
    name->codepage = 0;
    name->win_name[0] = 0;
    lstrcpynW( name->lang, str, sizeof(name->lang)/sizeof(WCHAR) );

    if (!*name->lang)
    {
        name->lcid = LOCALE_INVARIANT;
        name->matches = 4;
        return;
    }

    if (!(p = strpbrkW( name->lang, sepW )))
    {
        if (!strcmpW( name->lang, posixW ) || !strcmpW( name->lang, cW ))
        {
            name->matches = 4;  /* perfect match for default English lcid */
            return;
        }
        strcpyW( name->win_name, name->lang );
    }
    else if (*p == '-')  /* Windows format */
    {
        strcpyW( name->win_name, name->lang );
        *p++ = 0;
        name->country = p;
        if ((p = strpbrkW( p, winsepW )) && *p == '-')
        {
            *p++ = 0;
            name->script = name->country;
            name->country = p;
            p = strpbrkW( p, winsepW );
        }
        if (p)
        {
            *p++ = 0;
            name->modifier = p;
        }
        /* second value can be script or country, check length to resolve the ambiguity */
        if (!name->script && strlenW( name->country ) == 4)
        {
            name->script = name->country;
            name->country = NULL;
        }
    }
    else  /* Unix format */
    {
        if (*p == '_')
        {
            *p++ = 0;
            name->country = p;
            p = strpbrkW( p, sepW + 2 );
        }
        if (p && *p == '.')
        {
            *p++ = 0;
            name->charset = p;
            p = strchrW( p, '@' );
        }
        if (p)
        {
            *p++ = 0;
            name->modifier = p;
        }

        if (name->charset)
            name->codepage = find_charset( name->charset );

        /* rebuild a Windows name if possible */

        if (name->charset) goto done;  /* can't specify charset in Windows format */
        if (name->modifier && strcmpW( name->modifier, latinW ))
            goto done;  /* only Latn script supported for now */
        strcpyW( name->win_name, name->lang );
        if (name->modifier) strcatW( name->win_name, latnW );
        if (name->country)
        {
            p = name->win_name + strlenW(name->win_name);
            *p++ = '-';
            strcpyW( p, name->country );
        }
    }
done:
    EnumResourceLanguagesW( kernel32_handle, (LPCWSTR)RT_STRING, (LPCWSTR)LOCALE_ILANGUAGE,
                            find_locale_id_callback, (LPARAM)name );
}
#endif


/***********************************************************************
 *           convert_default_lcid
 *
 * Get the default LCID to use for a given lctype in GetLocaleInfo.
 */
static LCID convert_default_lcid( LCID lcid, LCTYPE lctype )
{
    if (lcid == LOCALE_SYSTEM_DEFAULT ||
        lcid == LOCALE_USER_DEFAULT ||
        lcid == LOCALE_NEUTRAL)
    {
        LCID default_id = 0;

        switch(lctype & 0xffff)
        {
        case LOCALE_SSORTNAME:
            default_id = lcid_LC_COLLATE;
            break;

        case LOCALE_FONTSIGNATURE:
        case LOCALE_IDEFAULTANSICODEPAGE:
        case LOCALE_IDEFAULTCODEPAGE:
        case LOCALE_IDEFAULTEBCDICCODEPAGE:
        case LOCALE_IDEFAULTMACCODEPAGE:
        case LOCALE_IDEFAULTUNIXCODEPAGE:
            default_id = lcid_LC_CTYPE;
            break;

        case LOCALE_ICURRDIGITS:
        case LOCALE_ICURRENCY:
        case LOCALE_IINTLCURRDIGITS:
        case LOCALE_INEGCURR:
        case LOCALE_INEGSEPBYSPACE:
        case LOCALE_INEGSIGNPOSN:
        case LOCALE_INEGSYMPRECEDES:
        case LOCALE_IPOSSEPBYSPACE:
        case LOCALE_IPOSSIGNPOSN:
        case LOCALE_IPOSSYMPRECEDES:
        case LOCALE_SCURRENCY:
        case LOCALE_SINTLSYMBOL:
        case LOCALE_SMONDECIMALSEP:
        case LOCALE_SMONGROUPING:
        case LOCALE_SMONTHOUSANDSEP:
        case LOCALE_SNATIVECURRNAME:
            default_id = lcid_LC_MONETARY;
            break;

        case LOCALE_IDIGITS:
        case LOCALE_IDIGITSUBSTITUTION:
        case LOCALE_ILZERO:
        case LOCALE_INEGNUMBER:
        case LOCALE_SDECIMAL:
        case LOCALE_SGROUPING:
        //case LOCALE_SNAN:
        case LOCALE_SNATIVEDIGITS:
        case LOCALE_SNEGATIVESIGN:
        //case LOCALE_SNEGINFINITY:
        //case LOCALE_SPOSINFINITY:
        case LOCALE_SPOSITIVESIGN:
        case LOCALE_STHOUSAND:
            default_id = lcid_LC_NUMERIC;
            break;

        case LOCALE_ICALENDARTYPE:
        case LOCALE_ICENTURY:
        case LOCALE_IDATE:
        case LOCALE_IDAYLZERO:
        case LOCALE_IFIRSTDAYOFWEEK:
        case LOCALE_IFIRSTWEEKOFYEAR:
        case LOCALE_ILDATE:
        case LOCALE_IMONLZERO:
        case LOCALE_IOPTIONALCALENDAR:
        case LOCALE_ITIME:
        case LOCALE_ITIMEMARKPOSN:
        case LOCALE_ITLZERO:
        case LOCALE_S1159:
        case LOCALE_S2359:
        case LOCALE_SABBREVDAYNAME1:
        case LOCALE_SABBREVDAYNAME2:
        case LOCALE_SABBREVDAYNAME3:
        case LOCALE_SABBREVDAYNAME4:
        case LOCALE_SABBREVDAYNAME5:
        case LOCALE_SABBREVDAYNAME6:
        case LOCALE_SABBREVDAYNAME7:
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
        case LOCALE_SABBREVMONTHNAME13:
        case LOCALE_SDATE:
        case LOCALE_SDAYNAME1:
        case LOCALE_SDAYNAME2:
        case LOCALE_SDAYNAME3:
        case LOCALE_SDAYNAME4:
        case LOCALE_SDAYNAME5:
        case LOCALE_SDAYNAME6:
        case LOCALE_SDAYNAME7:
        //case LOCALE_SDURATION:
        case LOCALE_SLONGDATE:
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
        case LOCALE_SMONTHNAME13:
        case LOCALE_SSHORTDATE:
        //case LOCALE_SSHORTESTDAYNAME1:
        //case LOCALE_SSHORTESTDAYNAME2:
        //case LOCALE_SSHORTESTDAYNAME3:
        //case LOCALE_SSHORTESTDAYNAME4:
        //case LOCALE_SSHORTESTDAYNAME5:
        //case LOCALE_SSHORTESTDAYNAME6:
        //case LOCALE_SSHORTESTDAYNAME7:
        case LOCALE_STIME:
        case LOCALE_STIMEFORMAT:
        case LOCALE_SYEARMONTH:
            default_id = lcid_LC_TIME;
            break;

        case LOCALE_IPAPERSIZE:
            default_id = lcid_LC_PAPER;
            break;

        case LOCALE_IMEASURE:
            default_id = lcid_LC_MEASUREMENT;
            break;

        case LOCALE_ICOUNTRY:
            default_id = lcid_LC_TELEPHONE;
            break;
        }
        if (default_id) lcid = default_id;
    }
    return ConvertDefaultLocale( lcid );
}

/***********************************************************************
 *           is_genitive_name_supported
 *
 * Determine could LCTYPE basically support genitive name form or not.
 */
static BOOL is_genitive_name_supported( LCTYPE lctype )
{
    switch(lctype & 0xffff)
    {
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
    case LOCALE_SMONTHNAME13:
         return TRUE;
    default:
         return FALSE;
    }
}

/***********************************************************************
 *		create_registry_key
 *
 * Create the Control Panel\\International registry key.
 */
static inline HANDLE create_registry_key(void)
{
    static const WCHAR cplW[] = {'C','o','n','t','r','o','l',' ','P','a','n','e','l',0};
    static const WCHAR intlW[] = {'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE cpl_key, hkey = 0;

    if (RtlOpenCurrentUser( KEY_ALL_ACCESS, &hkey ) != STATUS_SUCCESS) return 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, cplW );

    if (!NtCreateKey( &cpl_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
    {
        NtClose( attr.RootDirectory );
        attr.RootDirectory = cpl_key;
        RtlInitUnicodeString( &nameW, intlW );
        if (NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) hkey = 0;
    }
    NtClose( attr.RootDirectory );
    return hkey;
}


#ifndef __REACTOS__
/* update the registry settings for a given locale parameter */
/* return TRUE if an update was needed */
static BOOL locale_update_registry( HKEY hkey, const WCHAR *name, LCID lcid,
                                    const LCTYPE *values, UINT nb_values )
{
    static const WCHAR formatW[] = { '%','0','8','x',0 };
    WCHAR bufferW[40];
    UNICODE_STRING nameW;
    DWORD count, i;

    RtlInitUnicodeString( &nameW, name );
    count = sizeof(bufferW);
    if (!NtQueryValueKey(hkey, &nameW, KeyValuePartialInformation, bufferW, count, &count))
    {
        const KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)bufferW;
        LPCWSTR text = (LPCWSTR)info->Data;

        if (strtoulW( text, NULL, 16 ) == lcid) return FALSE; /* already set correctly */
        TRACE( "updating registry, locale %s changed %s -> %08x\n",
               debugstr_w(name), debugstr_w(text), lcid );
    }
    else TRACE( "updating registry, locale %s changed none -> %08x\n", debugstr_w(name), lcid );
    sprintfW( bufferW, formatW, lcid );
    NtSetValueKey( hkey, &nameW, 0, REG_SZ, bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR) );

    for (i = 0; i < nb_values; i++)
    {
        GetLocaleInfoW( lcid, values[i] | LOCALE_NOUSEROVERRIDE, bufferW,
                        sizeof(bufferW)/sizeof(WCHAR) );
        SetLocaleInfoW( lcid, values[i], bufferW );
    }
    return TRUE;
}


/***********************************************************************
 *		LOCALE_InitRegistry
 *
 * Update registry contents on startup if the user locale has changed.
 * This simulates the action of the Windows control panel.
 */
void LOCALE_InitRegistry(void)
{
    static const WCHAR acpW[] = {'A','C','P',0};
    static const WCHAR oemcpW[] = {'O','E','M','C','P',0};
    static const WCHAR maccpW[] = {'M','A','C','C','P',0};
    static const WCHAR localeW[] = {'L','o','c','a','l','e',0};
    static const WCHAR lc_ctypeW[] = { 'L','C','_','C','T','Y','P','E',0 };
    static const WCHAR lc_monetaryW[] = { 'L','C','_','M','O','N','E','T','A','R','Y',0 };
    static const WCHAR lc_numericW[] = { 'L','C','_','N','U','M','E','R','I','C',0 };
    static const WCHAR lc_timeW[] = { 'L','C','_','T','I','M','E',0 };
    static const WCHAR lc_measurementW[] = { 'L','C','_','M','E','A','S','U','R','E','M','E','N','T',0 };
    static const WCHAR lc_telephoneW[] = { 'L','C','_','T','E','L','E','P','H','O','N','E',0 };
    static const WCHAR lc_paperW[] = { 'L','C','_','P','A','P','E','R',0};
    static const struct
    {
        LPCWSTR name;
        USHORT value;
    } update_cp_values[] = {
        { acpW, LOCALE_IDEFAULTANSICODEPAGE },
        { oemcpW, LOCALE_IDEFAULTCODEPAGE },
        { maccpW, LOCALE_IDEFAULTMACCODEPAGE }
    };
    static const LCTYPE lc_messages_values[] = {
      LOCALE_SABBREVLANGNAME,
      LOCALE_SCOUNTRY,
      LOCALE_SLIST };
    static const LCTYPE lc_monetary_values[] = {
      LOCALE_SCURRENCY,
      LOCALE_ICURRENCY,
      LOCALE_INEGCURR,
      LOCALE_ICURRDIGITS,
      LOCALE_ILZERO,
      LOCALE_SMONDECIMALSEP,
      LOCALE_SMONGROUPING,
      LOCALE_SMONTHOUSANDSEP };
    static const LCTYPE lc_numeric_values[] = {
      LOCALE_SDECIMAL,
      LOCALE_STHOUSAND,
      LOCALE_IDIGITS,
      LOCALE_IDIGITSUBSTITUTION,
      LOCALE_SNATIVEDIGITS,
      LOCALE_INEGNUMBER,
      LOCALE_SNEGATIVESIGN,
      LOCALE_SPOSITIVESIGN,
      LOCALE_SGROUPING };
    static const LCTYPE lc_time_values[] = {
      LOCALE_S1159,
      LOCALE_S2359,
      LOCALE_STIME,
      LOCALE_ITIME,
      LOCALE_ITLZERO,
      LOCALE_SSHORTDATE,
      LOCALE_SLONGDATE,
      LOCALE_SDATE,
      LOCALE_ITIMEMARKPOSN,
      LOCALE_ICALENDARTYPE,
      LOCALE_IFIRSTDAYOFWEEK,
      LOCALE_IFIRSTWEEKOFYEAR,
      LOCALE_STIMEFORMAT,
      LOCALE_SYEARMONTH,
      LOCALE_IDATE };
    static const LCTYPE lc_measurement_values[] = { LOCALE_IMEASURE };
    static const LCTYPE lc_telephone_values[] = { LOCALE_ICOUNTRY };
    static const LCTYPE lc_paper_values[] = { LOCALE_IPAPERSIZE };

    UNICODE_STRING nameW;
    WCHAR bufferW[80];
    DWORD count, i;
    HANDLE hkey;
    LCID lcid = GetUserDefaultLCID();

    if (!(hkey = create_registry_key()))
        return;  /* don't do anything if we can't create the registry key */

    locale_update_registry( hkey, localeW, lcid_LC_MESSAGES, lc_messages_values,
                            sizeof(lc_messages_values)/sizeof(lc_messages_values[0]) );
    locale_update_registry( hkey, lc_monetaryW, lcid_LC_MONETARY, lc_monetary_values,
                            sizeof(lc_monetary_values)/sizeof(lc_monetary_values[0]) );
    locale_update_registry( hkey, lc_numericW, lcid_LC_NUMERIC, lc_numeric_values,
                            sizeof(lc_numeric_values)/sizeof(lc_numeric_values[0]) );
    locale_update_registry( hkey, lc_timeW, lcid_LC_TIME, lc_time_values,
                            sizeof(lc_time_values)/sizeof(lc_time_values[0]) );
    locale_update_registry( hkey, lc_measurementW, lcid_LC_MEASUREMENT, lc_measurement_values,
                            sizeof(lc_measurement_values)/sizeof(lc_measurement_values[0]) );
    locale_update_registry( hkey, lc_telephoneW, lcid_LC_TELEPHONE, lc_telephone_values,
                            sizeof(lc_telephone_values)/sizeof(lc_telephone_values[0]) );
    locale_update_registry( hkey, lc_paperW, lcid_LC_PAPER, lc_paper_values,
                            sizeof(lc_paper_values)/sizeof(lc_paper_values[0]) );

    if (locale_update_registry( hkey, lc_ctypeW, lcid_LC_CTYPE, NULL, 0 ))
    {
        static const WCHAR codepageW[] =
            {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
             'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
             'C','o','n','t','r','o','l','\\','N','l','s','\\','C','o','d','e','p','a','g','e',0};

        OBJECT_ATTRIBUTES attr;
        HANDLE nls_key;
        DWORD len = 14;

        RtlInitUnicodeString( &nameW, codepageW );
        InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
        while (codepageW[len])
        {
            nameW.Length = len * sizeof(WCHAR);
            if (NtCreateKey( &nls_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) break;
            NtClose( nls_key );
            len++;
            while (codepageW[len] && codepageW[len] != '\\') len++;
        }
        nameW.Length = len * sizeof(WCHAR);
        if (!NtCreateKey( &nls_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
        {
            for (i = 0; i < sizeof(update_cp_values)/sizeof(update_cp_values[0]); i++)
            {
                count = GetLocaleInfoW( lcid, update_cp_values[i].value | LOCALE_NOUSEROVERRIDE,
                                        bufferW, sizeof(bufferW)/sizeof(WCHAR) );
                RtlInitUnicodeString( &nameW, update_cp_values[i].name );
                NtSetValueKey( nls_key, &nameW, 0, REG_SZ, bufferW, count * sizeof(WCHAR) );
            }
            NtClose( nls_key );
        }
    }

    NtClose( hkey );
}


#ifdef __APPLE__
/***********************************************************************
 *           get_mac_locale
 *
 * Return a locale identifier string reflecting the Mac locale, in a form
 * that parse_locale_name() will understand.  So, strip out unusual
 * things like script, variant, etc.  Or, rather, just construct it as
 * <lang>[_<country>].UTF-8.
 */
static const char* get_mac_locale(void)
{
    static char mac_locale[50];

    if (!mac_locale[0])
    {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef lang = CFLocaleGetValue( locale, kCFLocaleLanguageCode );
        CFStringRef country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
        CFStringRef locale_string;

        if (country)
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@_%@"), lang, country);
        else
            locale_string = CFStringCreateCopy(NULL, lang);

        CFStringGetCString(locale_string, mac_locale, sizeof(mac_locale), kCFStringEncodingUTF8);
        strcat(mac_locale, ".UTF-8");

        CFRelease(locale);
        CFRelease(locale_string);
    }

    return mac_locale;
}


/***********************************************************************
 *           has_env
 */
static BOOL has_env(const char* name)
{
    const char* value = getenv( name );
    return value && value[0];
}
#endif


/***********************************************************************
 *           get_locale
 *
 * Get the locale identifier for a given category.  On most platforms,
 * this is just a thin wrapper around setlocale().  On OS X, though, it
 * is common for the Mac locale settings to not be supported by the C
 * library.  So, we sometimes override the result with the Mac locale.
 */
static const char* get_locale(int category, const char* category_name)
{
    const char* ret = setlocale(category, NULL);

#ifdef __ANDROID__
    if (!strcmp(ret, "C"))
    {
        ret = getenv( category_name );
        if (!ret || !ret[0]) ret = getenv( "LC_ALL" );
        if (!ret || !ret[0]) ret = "C";
    }
#endif

#ifdef __APPLE__
    /* If LC_ALL is set, respect it as a user override.
       If LC_* is set, respect it as a user override, except if it's LC_CTYPE
       and equal to UTF-8.  That's because, when the Mac locale isn't supported
       by the C library, Terminal.app sets LC_CTYPE=UTF-8 and doesn't set LANG.
       parse_locale_name() doesn't handle that properly, so we override that
       with the Mac locale (which uses UTF-8 for the charset, anyway).
       Otherwise:
       For LC_MESSAGES, we override the C library because the user language
       setting is separate from the locale setting on which LANG was based.
       If the C library didn't get anything better from LANG than C or POSIX,
       override that.  That probably means the Mac locale isn't supported by
       the C library. */
    if (!has_env( "LC_ALL" ) &&
        ((category == LC_CTYPE && !strcmp( ret, "UTF-8" )) ||
         (!has_env( category_name ) &&
          (category == LC_MESSAGES || !strcmp( ret, "C" ) || !strcmp( ret, "POSIX" )))))
    {
        const char* override = get_mac_locale();

        if (category == LC_MESSAGES)
        {
            /* Retrieve the preferred language as chosen in System Preferences. */
            static char messages_locale[50];

            if (!messages_locale[0])
            {
                CFArrayRef preferred_langs = CFLocaleCopyPreferredLanguages();
                if (preferred_langs && CFArrayGetCount( preferred_langs ))
                {
                    CFStringRef preferred_lang = CFArrayGetValueAtIndex( preferred_langs, 0 );
                    CFDictionaryRef components = CFLocaleCreateComponentsFromLocaleIdentifier( NULL, preferred_lang );
                    if (components)
                    {
                        CFStringRef lang = CFDictionaryGetValue( components, kCFLocaleLanguageCode );
                        CFStringRef country = CFDictionaryGetValue( components, kCFLocaleCountryCode );
                        CFLocaleRef locale = NULL;
                        CFStringRef locale_string;

                        if (!country)
                        {
                            locale = CFLocaleCopyCurrent();
                            country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
                        }

                        if (country)
                            locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@_%@"), lang, country );
                        else
                            locale_string = CFStringCreateCopy( NULL, lang );
                        CFStringGetCString( locale_string, messages_locale, sizeof(messages_locale), kCFStringEncodingUTF8 );
                        strcat( messages_locale, ".UTF-8" );

                        CFRelease( locale_string );
                        if (locale) CFRelease( locale );
                        CFRelease( components );
                    }
                }
                if (preferred_langs)
                    CFRelease( preferred_langs );
            }

            if (messages_locale[0])
                override = messages_locale;
        }

        TRACE( "%s is %s; overriding with %s\n", category_name, debugstr_a(ret), debugstr_a(override) );
        ret = override;
    }
#endif

    return ret;
}


/***********************************************************************
 *           setup_unix_locales
 */
static UINT setup_unix_locales(void)
{
    struct locale_name locale_name;
    WCHAR buffer[128], ctype_buff[128];
    const char *locale;
    UINT unix_cp = 0;

    if ((locale = get_locale( LC_CTYPE, "LC_CTYPE" )))
    {
        strcpynAtoW( ctype_buff, locale, sizeof(ctype_buff)/sizeof(WCHAR) );
        parse_locale_name( ctype_buff, &locale_name );
        lcid_LC_CTYPE = locale_name.lcid;
        unix_cp = locale_name.codepage;
    }
    if (!lcid_LC_CTYPE)  /* this one needs a default value */
        lcid_LC_CTYPE = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );

    TRACE( "got lcid %04x (%d matches) for LC_CTYPE=%s\n",
           locale_name.lcid, locale_name.matches, debugstr_a(locale) );

#define GET_UNIX_LOCALE(cat) do \
    if ((locale = get_locale( cat, #cat ))) \
    { \
        strcpynAtoW( buffer, locale, sizeof(buffer)/sizeof(WCHAR) ); \
        if (!strcmpW( buffer, ctype_buff )) lcid_##cat = lcid_LC_CTYPE; \
        else { \
            parse_locale_name( buffer, &locale_name );  \
            lcid_##cat = locale_name.lcid; \
            TRACE( "got lcid %04x (%d matches) for " #cat "=%s\n",        \
                   locale_name.lcid, locale_name.matches, debugstr_a(locale) ); \
        } \
    } while (0)

    GET_UNIX_LOCALE( LC_COLLATE );
    GET_UNIX_LOCALE( LC_MESSAGES );
    GET_UNIX_LOCALE( LC_MONETARY );
    GET_UNIX_LOCALE( LC_NUMERIC );
    GET_UNIX_LOCALE( LC_TIME );
#ifdef LC_PAPER
    GET_UNIX_LOCALE( LC_PAPER );
#endif
#ifdef LC_MEASUREMENT
    GET_UNIX_LOCALE( LC_MEASUREMENT );
#endif
#ifdef LC_TELEPHONE
    GET_UNIX_LOCALE( LC_TELEPHONE );
#endif

#undef GET_UNIX_LOCALE

    return unix_cp;
}
#endif // !__REACTOS__


/***********************************************************************
 *		GetUserDefaultLangID (KERNEL32.@)
 *
 * Get the default language Id for the current user.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LANGID of the default language for the current user.
 */
LANGID WINAPI GetUserDefaultLangID(void)
{
    return LANGIDFROMLCID(GetUserDefaultLCID());
}


/***********************************************************************
 *		GetSystemDefaultLangID (KERNEL32.@)
 *
 * Get the default language Id for the system.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LANGID of the default language for the system.
 */
LANGID WINAPI GetSystemDefaultLangID(void)
{
    return LANGIDFROMLCID(GetSystemDefaultLCID());
}


/***********************************************************************
 *		GetUserDefaultLCID (KERNEL32.@)
 *
 * Get the default locale Id for the current user.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LCID of the default locale for the current user.
 */
LCID WINAPI GetUserDefaultLCID(void)
{
    LCID lcid;
    NtQueryDefaultLocale( TRUE, &lcid );
    return lcid;
}


/***********************************************************************
 *		GetSystemDefaultLCID (KERNEL32.@)
 *
 * Get the default locale Id for the system.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LCID of the default locale for the system.
 */
LCID WINAPI GetSystemDefaultLCID(void)
{
    LCID lcid;
    NtQueryDefaultLocale( FALSE, &lcid );
    return lcid;
}

#ifndef __REACTOS__
/***********************************************************************
 *		GetSystemDefaultLocaleName (KERNEL32.@)
 */
INT WINAPI GetSystemDefaultLocaleName(LPWSTR localename, INT len)
{
    LCID lcid = GetSystemDefaultLCID();
    return LCIDToLocaleName(lcid, localename, len, 0);
}

static BOOL get_dummy_preferred_ui_language( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    LCTYPE type;
    int lsize;

    FIXME("(0x%x %p %p %p) returning a dummy value (current locale)\n", flags, count, buffer, size);

    if (flags & MUI_LANGUAGE_ID)
        type = LOCALE_ILANGUAGE;
    else
        type = LOCALE_SNAME;

    lsize = GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, type, NULL, 0);
    if (!lsize)
    {
        /* keep last error from callee */
        return FALSE;
    }
    lsize++;
    if (!*size)
    {
        *size = lsize;
        *count = 1;
        return TRUE;
    }

    if (lsize > *size)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, type, buffer, *size))
    {
        /* keep last error from callee */
        return FALSE;
    }

    buffer[lsize-1] = 0;
    *size = lsize;
    *count = 1;
    TRACE("returned variable content: %d, \"%s\", %d\n", *count, debugstr_w(buffer), *size);
    return TRUE;

}

/***********************************************************************
 *             GetSystemPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetSystemPreferredUILanguages(DWORD flags, ULONG* count, WCHAR* buffer, ULONG* size)
{
    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID | MUI_MACHINE_LANGUAGE_SETTINGS))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (*size && !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return get_dummy_preferred_ui_language( flags, count, buffer, size );
}

/***********************************************************************
 *              SetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI SetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, PULONG count )
{
    FIXME( "%u, %p, %p\n", flags, buffer, count );
    return TRUE;
}

/***********************************************************************
 *              GetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetThreadPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buf, ULONG *size )
{
    FIXME( "%08x, %p, %p %p\n", flags, count, buf, size );
    return get_dummy_preferred_ui_language( flags, count, buf, size );
}

#if (WINVER >= 0x0600)
/******************************************************************************
 *             GetUserPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetUserPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    TRACE( "%u %p %p %p\n", flags, count, buffer, size );

    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (*size && !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return get_dummy_preferred_ui_language( flags, count, buffer, size );
}
#endif // (WINVER >= 0x0600)
#endif // !__REACTOS__

/***********************************************************************
 *		GetUserDefaultUILanguage (KERNEL32.@)
 *
 * Get the default user interface language Id for the current user.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LANGID of the default UI language for the current user.
 */
LANGID WINAPI GetUserDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryDefaultUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *		GetSystemDefaultUILanguage (KERNEL32.@)
 *
 * Get the default user interface language Id for the system.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current LANGID of the default UI language for the system. This is
 *  typically the same language used during the installation process.
 */
LANGID WINAPI GetSystemDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryInstallUILanguage( &lang );
    return lang;
}

#if (WINVER >= 0x0600)
/***********************************************************************
 *           LocaleNameToLCID  (KERNEL32.@)
 */
LCID WINAPI LocaleNameToLCID( LPCWSTR name, DWORD flags )
{
    struct locale_name locale_name;

    if (flags) FIXME( "unsupported flags %x\n", flags );

    if (name == LOCALE_NAME_USER_DEFAULT)
        return GetUserDefaultLCID();

    /* string parsing */
    parse_locale_name( name, &locale_name );

    TRACE( "found lcid %x for %s, matches %d\n",
           locale_name.lcid, debugstr_w(name), locale_name.matches );

    if (!locale_name.matches)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (locale_name.matches == 1)
        WARN( "locale %s not recognized, defaulting to %s\n",
              debugstr_w(name), debugstr_w(locale_name.lang) );

    return locale_name.lcid;
}


/***********************************************************************
 *           LCIDToLocaleName  (KERNEL32.@)
 */
INT WINAPI LCIDToLocaleName( LCID lcid, LPWSTR name, INT count, DWORD flags )
{
    if (flags) FIXME( "unsupported flags %x\n", flags );

    return GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, name, count );
}
#endif


/******************************************************************************
 *		get_locale_registry_value
 *
 * Gets the registry value name and cache for a given lctype.
 */
static struct registry_value *get_locale_registry_value( DWORD lctype )
{
    int i;
    for (i=0; i < sizeof(registry_values)/sizeof(registry_values[0]); i++)
        if (registry_values[i].lctype == lctype)
            return &registry_values[i];
    return NULL;
}


/******************************************************************************
 *		get_registry_locale_info
 *
 * Retrieve user-modified locale info from the registry.
 * Return length, 0 on error, -1 if not found.
 */
static INT get_registry_locale_info( struct registry_value *registry_value, LPWSTR buffer, INT len )
{
    DWORD size;
    INT ret;
    HANDLE hkey;
    NTSTATUS status;
    UNICODE_STRING nameW;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    static const int info_size = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    RtlEnterCriticalSection( &cache_section );

    if (!registry_value->cached_value)
    {
        if (!(hkey = create_registry_key()))
        {
            RtlLeaveCriticalSection( &cache_section );
            return -1;
        }

        RtlInitUnicodeString( &nameW, registry_value->name );
        size = info_size + len * sizeof(WCHAR);

        if (!(info = HeapAlloc( GetProcessHeap(), 0, size )))
        {
            NtClose( hkey );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            RtlLeaveCriticalSection( &cache_section );
            return 0;
        }

        status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, info, size, &size );

        /* try again with a bigger buffer when we have to return the correct size */
        if (status == STATUS_BUFFER_OVERFLOW && !buffer && size > info_size)
        {
            KEY_VALUE_PARTIAL_INFORMATION *new_info;
            if ((new_info = HeapReAlloc( GetProcessHeap(), 0, info, size )))
            {
                info = new_info;
                status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, info, size, &size );
            }
        }

        NtClose( hkey );

        if (!status)
        {
            INT length = (size - info_size) / sizeof(WCHAR);
            LPWSTR cached_value;

            if (!length || ((WCHAR *)&info->Data)[length-1])
                length++;

            cached_value = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) );

            if (!cached_value)
            {
                HeapFree( GetProcessHeap(), 0, info );
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                RtlLeaveCriticalSection( &cache_section );
                return 0;
            }

            memcpy( cached_value, info->Data, (length-1) * sizeof(WCHAR) );
            cached_value[length-1] = 0;
            HeapFree( GetProcessHeap(), 0, info );
            registry_value->cached_value = cached_value;
        }
        else
        {
            if (status == STATUS_BUFFER_OVERFLOW && !buffer)
            {
                ret = (size - info_size) / sizeof(WCHAR);
            }
            else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                ret = -1;
            }
            else
            {
                SetLastError( RtlNtStatusToDosError(status) );
                ret = 0;
            }
            HeapFree( GetProcessHeap(), 0, info );
            RtlLeaveCriticalSection( &cache_section );
            return ret;
        }
    }

    ret = lstrlenW( registry_value->cached_value ) + 1;

    if (buffer)
    {
        if (ret > len)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            ret = 0;
        }
        else
        {
            lstrcpyW( buffer, registry_value->cached_value );
        }
    }

    RtlLeaveCriticalSection( &cache_section );

    return ret;
}


/******************************************************************************
 *		GetLocaleInfoA (KERNEL32.@)
 *
 * Get information about an aspect of a locale.
 *
 * PARAMS
 *  lcid   [I] LCID of the locale
 *  lctype [I] LCTYPE_ flags from "winnls.h"
 *  buffer [O] Destination for the information
 *  len    [I] Length of buffer in characters
 *
 * RETURNS
 *  Success: The size of the data requested. If buffer is non-NULL, it is filled
 *           with the information.
 *  Failure: 0. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  - LOCALE_NEUTRAL is equal to LOCALE_SYSTEM_DEFAULT
 *  - The string returned is NUL terminated, except for LOCALE_FONTSIGNATURE,
 *    which is a bit string.
 */
INT WINAPI GetLocaleInfoA( LCID lcid, LCTYPE lctype, LPSTR buffer, INT len )
{
    WCHAR *bufferW;
    INT lenW, ret;

    TRACE( "(lcid=0x%x,lctype=0x%x,%p,%d)\n", lcid, lctype, buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (((lctype & ~LOCALE_LOCALEINFOFLAGSMASK) == LOCALE_SSHORTTIME) ||
         (lctype & LOCALE_RETURN_GENITIVE_NAMES))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!len) buffer = NULL;

    if (!(lenW = GetLocaleInfoW( lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((ret = GetLocaleInfoW( lcid, lctype, bufferW, lenW )))
    {
        if ((lctype & LOCALE_RETURN_NUMBER) ||
            ((lctype & ~LOCALE_LOCALEINFOFLAGSMASK) == LOCALE_FONTSIGNATURE))
        {
            /* it's not an ASCII string, just bytes */
            ret *= sizeof(WCHAR);
            if (buffer)
            {
                if (ret <= len) memcpy( buffer, bufferW, ret );
                else
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    ret = 0;
                }
            }
        }
        else
        {
            UINT codepage = CP_ACP;
            if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );
            ret = WideCharToMultiByte( codepage, 0, bufferW, ret, buffer, len, NULL, NULL );
        }
    }
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}

static int get_value_base_by_lctype( LCTYPE lctype )
{
    return lctype == LOCALE_ILANGUAGE || lctype == LOCALE_IDEFAULTLANGUAGE ? 16 : 10;
}

/******************************************************************************
 *		GetLocaleInfoW (KERNEL32.@)
 *
 * See GetLocaleInfoA.
 */
INT WINAPI GetLocaleInfoW( LCID lcid, LCTYPE lctype, LPWSTR buffer, INT len )
{
    LANGID lang_id;
    HRSRC hrsrc;
    HGLOBAL hmem;
    INT ret;
    UINT lcflags;
    const WCHAR *p;
    unsigned int i;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (lctype & LOCALE_RETURN_GENITIVE_NAMES &&
       !is_genitive_name_supported( lctype ))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!len) buffer = NULL;

    lcid = convert_default_lcid( lcid, lctype );

    lcflags = lctype & LOCALE_LOCALEINFOFLAGSMASK;
    lctype &= 0xffff;

    TRACE( "(lcid=0x%x,lctype=0x%x,%p,%d)\n", lcid, lctype, buffer, len );

    /* first check for overrides in the registry */

    if (!(lcflags & LOCALE_NOUSEROVERRIDE) &&
        lcid == convert_default_lcid( LOCALE_USER_DEFAULT, lctype ))
    {
        struct registry_value *value = get_locale_registry_value(lctype);

        if (value)
        {
            if (lcflags & LOCALE_RETURN_NUMBER)
            {
                WCHAR tmp[16];
                ret = get_registry_locale_info( value, tmp, sizeof(tmp)/sizeof(WCHAR) );
                if (ret > 0)
                {
                    WCHAR *end;
                    UINT number = strtolW( tmp, &end, get_value_base_by_lctype( lctype ) );
                    if (*end)  /* invalid number */
                    {
                        SetLastError( ERROR_INVALID_FLAGS );
                        return 0;
                    }
                    ret = sizeof(UINT)/sizeof(WCHAR);
                    if (!buffer) return ret;
                    if (ret > len)
                    {
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        return 0;
                    }
                    memcpy( buffer, &number, sizeof(number) );
                }
            }
            else ret = get_registry_locale_info( value, buffer, len );

            if (ret != -1) return ret;
        }
    }

    /* now load it from kernel resources */

    lang_id = LANGIDFROMLCID( lcid );

    /* replace SUBLANG_NEUTRAL by SUBLANG_DEFAULT */
    if (SUBLANGID(lang_id) == SUBLANG_NEUTRAL) lang_id = get_default_sublang( lang_id );

    if (!(hrsrc = FindResourceExW( kernel32_handle, (LPWSTR)RT_STRING,
                                   ULongToPtr((lctype >> 4) + 1), lang_id )))
    {
        SetLastError( ERROR_INVALID_FLAGS );  /* no such lctype */
        return 0;
    }
    if (!(hmem = LoadResource( kernel32_handle, hrsrc )))
        return 0;

    p = LockResource( hmem );
    for (i = 0; i < (lctype & 0x0f); i++) p += *p + 1;

    if (lcflags & LOCALE_RETURN_NUMBER) ret = sizeof(UINT)/sizeof(WCHAR);
    else if (is_genitive_name_supported( lctype ) && *p)
    {
        /* genitive form's stored after a null separator from a nominative */
        for (i = 1; i <= *p; i++) if (!p[i]) break;

        if (i <= *p && (lcflags & LOCALE_RETURN_GENITIVE_NAMES))
        {
            ret = *p - i + 1;
            p += i;
        }
        else ret = i;
    }
    else
        ret = (lctype == LOCALE_FONTSIGNATURE) ? *p : *p + 1;

    if (!buffer) return ret;

    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (lcflags & LOCALE_RETURN_NUMBER)
    {
        UINT number;
        WCHAR *end, *tmp = HeapAlloc( GetProcessHeap(), 0, (*p + 1) * sizeof(WCHAR) );
        if (!tmp) return 0;
        memcpy( tmp, p + 1, *p * sizeof(WCHAR) );
        tmp[*p] = 0;
        number = strtolW( tmp, &end, get_value_base_by_lctype( lctype ) );
        if (!*end)
            memcpy( buffer, &number, sizeof(number) );
        else  /* invalid number */
        {
            SetLastError( ERROR_INVALID_FLAGS );
            ret = 0;
        }
        HeapFree( GetProcessHeap(), 0, tmp );

        TRACE( "(lcid=0x%x,lctype=0x%x,%p,%d) returning number %d\n",
               lcid, lctype, buffer, len, number );
    }
    else
    {
        memcpy( buffer, p + 1, ret * sizeof(WCHAR) );
        if (lctype != LOCALE_FONTSIGNATURE) buffer[ret-1] = 0;

        TRACE( "(lcid=0x%x,lctype=0x%x,%p,%d) returning %d %s\n",
               lcid, lctype, buffer, len, ret, debugstr_w(buffer) );
    }
    return ret;
}

#if (WINVER >= 0x0600)
/******************************************************************************
 *           GetLocaleInfoEx (KERNEL32.@)
 */
INT WINAPI GetLocaleInfoEx(LPCWSTR locale, LCTYPE info, LPWSTR buffer, INT len)
{
    LCID lcid = LocaleNameToLCID(locale, 0);

    TRACE("%s, lcid=0x%x, 0x%x\n", debugstr_w(locale), lcid, info);

    if (!lcid) return 0;

    /* special handling for neutral locale names */
    if (locale && strlenW(locale) == 2)
    {
        switch (info)
        {
        case LOCALE_SNAME:
            if (len && len < 3)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            if (len) strcpyW(buffer, locale);
            return 3;
        case LOCALE_SPARENT:
            if (len) buffer[0] = 0;
            return 1;
        }
    }

    return GetLocaleInfoW(lcid, info, buffer, len);
}

BOOL
WINAPI
IsValidLocaleName(
  LPCWSTR lpLocaleName
)
{
    TRACE( "IsValidLocaleName not implemented (lpLocaleName=%s)\n", debugstr_w(lpLocaleName));
    return TRUE;
}

INT
WINAPI
GetUserDefaultLocaleName(
  LPWSTR lpLocaleName,
  INT    cchLocaleName
)
{
    TRACE( "GetUserDefaultLocaleName not implemented (lpLocaleName=%s, cchLocaleName=%d)\n", debugstr_w(lpLocaleName), cchLocaleName);
    return 0;
}
#endif

/******************************************************************************
 *		SetLocaleInfoA	[KERNEL32.@]
 *
 * Set information about an aspect of a locale.
 *
 * PARAMS
 *  lcid   [I] LCID of the locale
 *  lctype [I] LCTYPE_ flags from "winnls.h"
 *  data   [I] Information to set
 *
 * RETURNS
 *  Success: TRUE. The information given will be returned by GetLocaleInfoA()
 *           whenever it is called without LOCALE_NOUSEROVERRIDE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  - Values are only be set for the current user locale; the system locale
 *  settings cannot be changed.
 *  - Any settings changed by this call are lost when the locale is changed by
 *  the control panel (in Wine, this happens every time you change LANG).
 *  - The native implementation of this function does not check that lcid matches
 *  the current user locale, and simply sets the new values. Wine warns you in
 *  this case, but behaves the same.
 */
BOOL WINAPI SetLocaleInfoA(LCID lcid, LCTYPE lctype, LPCSTR data)
{
    UINT codepage = CP_ACP;
    WCHAR *strW;
    DWORD len;
    BOOL ret;

    if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );

    if (!data)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    len = MultiByteToWideChar( codepage, 0, data, -1, NULL, 0 );
    if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    MultiByteToWideChar( codepage, 0, data, -1, strW, len );
    ret = SetLocaleInfoW( lcid, lctype, strW );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 *		SetLocaleInfoW	(KERNEL32.@)
 *
 * See SetLocaleInfoA.
 */
BOOL WINAPI SetLocaleInfoW( LCID lcid, LCTYPE lctype, LPCWSTR data )
{
    struct registry_value *value;
    static const WCHAR intlW[] = {'i','n','t','l',0 };
    UNICODE_STRING valueW;
    NTSTATUS status;
    HANDLE hkey;

    lctype &= 0xffff;
    value = get_locale_registry_value( lctype );

    if (!data || !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lctype == LOCALE_IDATE || lctype == LOCALE_ILDATE)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    TRACE("setting %x (%s) to %s\n", lctype, debugstr_w(value->name), debugstr_w(data) );

    /* FIXME: should check that data to set is sane */

    /* FIXME: profile functions should map to registry */
    WriteProfileStringW( intlW, value->name, data );

    if (!(hkey = create_registry_key())) return FALSE;
    RtlInitUnicodeString( &valueW, value->name );
    status = NtSetValueKey( hkey, &valueW, 0, REG_SZ, (PVOID)data, (strlenW(data)+1)*sizeof(WCHAR) );

    RtlEnterCriticalSection( &cache_section );
    HeapFree( GetProcessHeap(), 0, value->cached_value );
    value->cached_value = NULL;
    RtlLeaveCriticalSection( &cache_section );

    if (lctype == LOCALE_SSHORTDATE || lctype == LOCALE_SLONGDATE)
    {
      /* Set I-value from S value */
      WCHAR *lpD, *lpM, *lpY;
      WCHAR szBuff[2];

      lpD = strrchrW(data, 'd');
      lpM = strrchrW(data, 'M');
      lpY = strrchrW(data, 'y');

      if (lpD <= lpM)
      {
        szBuff[0] = '1'; /* D-M-Y */
      }
      else
      {
        if (lpY <= lpM)
          szBuff[0] = '2'; /* Y-M-D */
        else
          szBuff[0] = '0'; /* M-D-Y */
      }

      szBuff[1] = '\0';

      if (lctype == LOCALE_SSHORTDATE)
        lctype = LOCALE_IDATE;
      else
        lctype = LOCALE_ILDATE;

      value = get_locale_registry_value( lctype );

      WriteProfileStringW( intlW, value->name, szBuff );

      RtlInitUnicodeString( &valueW, value->name );
      status = NtSetValueKey( hkey, &valueW, 0, REG_SZ, szBuff, sizeof(szBuff) );

      RtlEnterCriticalSection( &cache_section );
      HeapFree( GetProcessHeap(), 0, value->cached_value );
      value->cached_value = NULL;
      RtlLeaveCriticalSection( &cache_section );
    }

    NtClose( hkey );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


#ifndef __REACTOS__
/******************************************************************************
 *              GetACP   (KERNEL32.@)
 *
 * Get the current Ansi code page Id for the system.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *    The current Ansi code page identifier for the system.
 */
UINT WINAPI GetACP(void)
{
    assert( ansi_cptable );
    return ansi_cptable->info.codepage;
}


/******************************************************************************
 *              SetCPGlobal   (KERNEL32.@)
 *
 * Set the current Ansi code page Id for the system.
 *
 * PARAMS
 *    acp [I] code page ID to be the new ACP.
 *
 * RETURNS
 *    The previous ACP.
 */
UINT WINAPI SetCPGlobal( UINT acp )
{
    UINT ret = GetACP();
    const union cptable *new_cptable = wine_cp_get_table( acp );

    if (new_cptable) ansi_cptable = new_cptable;
    return ret;
}


/***********************************************************************
 *              GetOEMCP   (KERNEL32.@)
 *
 * Get the current OEM code page Id for the system.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *    The current OEM code page identifier for the system.
 */
UINT WINAPI GetOEMCP(void)
{
    assert( oem_cptable );
    return oem_cptable->info.codepage;
}


/***********************************************************************
 *           IsValidCodePage   (KERNEL32.@)
 *
 * Determine if a given code page identifier is valid.
 *
 * PARAMS
 *  codepage [I] Code page Id to verify.
 *
 * RETURNS
 *  TRUE, If codepage is valid and available on the system,
 *  FALSE otherwise.
 */
BOOL WINAPI IsValidCodePage( UINT codepage )
{
    switch(codepage) {
    case CP_UTF7:
    case CP_UTF8:
        return TRUE;
    default:
        return wine_cp_get_table( codepage ) != NULL;
    }
}


/***********************************************************************
 *           IsDBCSLeadByteEx   (KERNEL32.@)
 *
 * Determine if a character is a lead byte in a given code page.
 *
 * PARAMS
 *  codepage [I] Code page for the test.
 *  testchar [I] Character to test
 *
 * RETURNS
 *  TRUE, if testchar is a lead byte in codepage,
 *  FALSE otherwise.
 */
BOOL WINAPI IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const union cptable *table = get_codepage_table( codepage );
    return table && wine_is_dbcs_leadbyte( table, testchar );
}


/***********************************************************************
 *           IsDBCSLeadByte   (KERNEL32.@)
 *           IsDBCSLeadByte   (KERNEL.207)
 *
 * Determine if a character is a lead byte.
 *
 * PARAMS
 *  testchar [I] Character to test
 *
 * RETURNS
 *  TRUE, if testchar is a lead byte in the ANSI code page,
 *  FALSE otherwise.
 */
BOOL WINAPI IsDBCSLeadByte( BYTE testchar )
{
    if (!ansi_cptable) return FALSE;
    return wine_is_dbcs_leadbyte( ansi_cptable, testchar );
}


/***********************************************************************
 *           GetCPInfo   (KERNEL32.@)
 *
 * Get information about a code page.
 *
 * PARAMS
 *  codepage [I] Code page number
 *  cpinfo   [O] Destination for code page information
 *
 * RETURNS
 *  Success: TRUE. cpinfo is updated with the information about codepage.
 *  Failure: FALSE, if codepage is invalid or cpinfo is NULL.
 */
BOOL WINAPI GetCPInfo( UINT codepage, LPCPINFO cpinfo )
{
    const union cptable *table;

    if (!cpinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(table = get_codepage_table( codepage )))
    {
        switch(codepage)
        {
            case CP_UTF7:
            case CP_UTF8:
                cpinfo->DefaultChar[0] = 0x3f;
                cpinfo->DefaultChar[1] = 0;
                cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
                cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
                return TRUE;
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (table->info.def_char & 0xff00)
    {
        cpinfo->DefaultChar[0] = (table->info.def_char & 0xff00) >> 8;
        cpinfo->DefaultChar[1] = table->info.def_char & 0x00ff;
    }
    else
    {
        cpinfo->DefaultChar[0] = table->info.def_char & 0xff;
        cpinfo->DefaultChar[1] = 0;
    }
    if ((cpinfo->MaxCharSize = table->info.char_size) == 2)
        memcpy( cpinfo->LeadByte, table->dbcs.lead_bytes, sizeof(cpinfo->LeadByte) );
    else
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;

    return TRUE;
}

/***********************************************************************
 *           GetCPInfoExA   (KERNEL32.@)
 *
 * Get extended information about a code page.
 *
 * PARAMS
 *  codepage [I] Code page number
 *  dwFlags  [I] Reserved, must to 0.
 *  cpinfo   [O] Destination for code page information
 *
 * RETURNS
 *  Success: TRUE. cpinfo is updated with the information about codepage.
 *  Failure: FALSE, if codepage is invalid or cpinfo is NULL.
 */
BOOL WINAPI GetCPInfoExA( UINT codepage, DWORD dwFlags, LPCPINFOEXA cpinfo )
{
    CPINFOEXW cpinfoW;

    if (!GetCPInfoExW( codepage, dwFlags, &cpinfoW ))
      return FALSE;

    /* the layout is the same except for CodePageName */
    memcpy(cpinfo, &cpinfoW, sizeof(CPINFOEXA));
    WideCharToMultiByte(CP_ACP, 0, cpinfoW.CodePageName, -1, cpinfo->CodePageName, sizeof(cpinfo->CodePageName), NULL, NULL);
    return TRUE;
}

/***********************************************************************
 *           GetCPInfoExW   (KERNEL32.@)
 *
 * Unicode version of GetCPInfoExA.
 */
BOOL WINAPI GetCPInfoExW( UINT codepage, DWORD dwFlags, LPCPINFOEXW cpinfo )
{
    if (!GetCPInfo( codepage, (LPCPINFO)cpinfo ))
      return FALSE;

    switch(codepage)
    {
        case CP_UTF7:
        {
            static const WCHAR utf7[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','7',')',0};

            cpinfo->CodePage = CP_UTF7;
            cpinfo->UnicodeDefaultChar = 0x3f;
            strcpyW(cpinfo->CodePageName, utf7);
            break;
        }

        case CP_UTF8:
        {
            static const WCHAR utf8[] = {'U','n','i','c','o','d','e',' ','(','U','T','F','-','8',')',0};

            cpinfo->CodePage = CP_UTF8;
            cpinfo->UnicodeDefaultChar = 0x3f;
            strcpyW(cpinfo->CodePageName, utf8);
            break;
        }

        default:
        {
            const union cptable *table = get_codepage_table( codepage );

            cpinfo->CodePage = table->info.codepage;
            cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
            MultiByteToWideChar( CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
                                 sizeof(cpinfo->CodePageName)/sizeof(WCHAR));
            break;
        }
    }
    return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePagesA   (KERNEL32.@)
 *
 * Call a user defined function for every code page installed on the system.
 *
 * PARAMS
 *   lpfnCodePageEnum [I] User CODEPAGE_ENUMPROC to call with each found code page
 *   flags            [I] Reserved, set to 0.
 *
 * RETURNS
 *  TRUE, If all code pages have been enumerated, or
 *  FALSE if lpfnCodePageEnum returned FALSE to stop the enumeration.
 */
BOOL WINAPI EnumSystemCodePagesA( CODEPAGE_ENUMPROCA lpfnCodePageEnum, DWORD flags )
{
    const union cptable *table;
    char buffer[10];
    int index = 0;

    for (;;)
    {
        if (!(table = wine_cp_enum_table( index++ ))) break;
        sprintf( buffer, "%d", table->info.codepage );
        if (!lpfnCodePageEnum( buffer )) break;
    }
    return TRUE;
}


/***********************************************************************
 *              EnumSystemCodePagesW   (KERNEL32.@)
 *
 * See EnumSystemCodePagesA.
 */
BOOL WINAPI EnumSystemCodePagesW( CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags )
{
    const union cptable *table;
    WCHAR buffer[10], *p;
    int page, index = 0;

    for (;;)
    {
        if (!(table = wine_cp_enum_table( index++ ))) break;
        p = buffer + sizeof(buffer)/sizeof(WCHAR);
        *--p = 0;
        page = table->info.codepage;
        do
        {
            *--p = '0' + (page % 10);
            page /= 10;
        } while( page );
        if (!lpfnCodePageEnum( p )) break;
    }
    return TRUE;
}


/***********************************************************************
 *              utf7_write_w
 *
 * Helper for utf7_mbstowcs
 *
 * RETURNS
 *   TRUE on success, FALSE on error
 */
static inline BOOL utf7_write_w(WCHAR *dst, int dstlen, int *index, WCHAR character)
{
    if (dstlen > 0)
    {
        if (*index >= dstlen)
            return FALSE;

        dst[*index] = character;
    }

    (*index)++;

    return TRUE;
}

/***********************************************************************
 *              utf7_mbstowcs
 *
 * UTF-7 to UTF-16 string conversion, helper for MultiByteToWideChar
 *
 * RETURNS
 *   On success, the number of characters written
 *   On dst buffer overflow, -1
 */
static int utf7_mbstowcs(const char *src, int srclen, WCHAR *dst, int dstlen)
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
    int dest_index = 0;

    DWORD byte_pair = 0;
    short offset = 0;

    while (src < source_end)
    {
        if (*src == '+')
        {
            src++;
            if (src >= source_end)
                break;

            if (*src == '-')
            {
                /* just a plus sign escaped as +- */
                if (!utf7_write_w(dst, dstlen, &dest_index, '+'))
                    return -1;
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
                    if (!utf7_write_w(dst, dstlen, &dest_index, (byte_pair >> (offset - 16)) & 0xFFFF))
                        return -1;
                    offset -= 16;
                }

                src++;
            }
            while (src < source_end);
        }
        else
        {
            /* we have to convert to unsigned char in case *src < 0 */
            if (!utf7_write_w(dst, dstlen, &dest_index, (unsigned char)*src))
                return -1;
            src++;
        }
    }

    return dest_index;
}

/***********************************************************************
 *              MultiByteToWideChar   (KERNEL32.@)
 *
 * Convert a multibyte character string into a Unicode string.
 *
 * PARAMS
 *   page   [I] Codepage character set to convert from
 *   flags  [I] Character mapping flags
 *   src    [I] Source string buffer
 *   srclen [I] Length of src (in bytes), or -1 if src is NUL terminated
 *   dst    [O] Destination buffer
 *   dstlen [I] Length of dst (in WCHARs), or 0 to compute the required length
 *
 * RETURNS
 *   Success: If dstlen > 0, the number of characters written to dst.
 *            If dstlen == 0, the number of characters needed to perform the
 *            conversion. In both cases the count includes the terminating NUL.
 *   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
 *            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
 *            and dstlen != 0; ERROR_INVALID_PARAMETER,  if an invalid parameter
 *            is passed, and ERROR_NO_UNICODE_TRANSLATION if no translation is
 *            possible for src.
 */
INT WINAPI MultiByteToWideChar( UINT page, DWORD flags, LPCSTR src, INT srclen,
                                LPWSTR dst, INT dstlen )
{
    const union cptable *table;
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlen(src) + 1;

    switch(page)
    {
    case CP_SYMBOL:
        if (flags)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = wine_cpsymbol_mbstowcs( src, srclen, dst, dstlen );
        break;
    case CP_UTF7:
        if (flags)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = utf7_mbstowcs( src, srclen, dst, dstlen );
        break;
    case CP_UNIXCP:
        if (unix_cptable)
        {
            ret = wine_cp_mbstowcs( unix_cptable, flags, src, srclen, dst, dstlen );
            break;
        }
#ifdef __APPLE__
        flags |= MB_COMPOSITE;  /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
#endif
        /* fall through */
    case CP_UTF8:
        if (flags & ~MB_FLAGSMASK)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = wine_utf8_mbstowcs( flags, src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        if (flags & ~MB_FLAGSMASK)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = wine_cp_mbstowcs( table, flags, src, srclen, dst, dstlen );
        break;
    }

    if (ret < 0)
    {
        switch(ret)
        {
        case -1: SetLastError( ERROR_INSUFFICIENT_BUFFER ); break;
        case -2: SetLastError( ERROR_NO_UNICODE_TRANSLATION ); break;
        }
        ret = 0;
    }
    TRACE("cp %d %s -> %s, ret = %d\n",
          page, debugstr_an(src, srclen), debugstr_wn(dst, ret), ret);
    return ret;
}


/***********************************************************************
 *              utf7_can_directly_encode
 *
 * Helper for utf7_wcstombs
 */
static inline BOOL utf7_can_directly_encode(WCHAR codepoint)
{
    static const BOOL directly_encodable_table[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0F */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1F */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7A */
    };

    return codepoint <= 0x7A ? directly_encodable_table[codepoint] : FALSE;
}

/***********************************************************************
 *              utf7_write_c
 *
 * Helper for utf7_wcstombs
 *
 * RETURNS
 *   TRUE on success, FALSE on error
 */
static inline BOOL utf7_write_c(char *dst, int dstlen, int *index, char character)
{
    if (dstlen > 0)
    {
        if (*index >= dstlen)
            return FALSE;

        dst[*index] = character;
    }

    (*index)++;

    return TRUE;
}

/***********************************************************************
 *              utf7_wcstombs
 *
 * UTF-16 to UTF-7 string conversion, helper for WideCharToMultiByte
 *
 * RETURNS
 *   On success, the number of characters written
 *   On dst buffer overflow, -1
 */
static int utf7_wcstombs(const WCHAR *src, int srclen, char *dst, int dstlen)
{
    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const WCHAR *source_end = src + srclen;
    int dest_index = 0;

    while (src < source_end)
    {
        if (*src == '+')
        {
            if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
                return -1;
            if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
                return -1;
            src++;
        }
        else if (utf7_can_directly_encode(*src))
        {
            if (!utf7_write_c(dst, dstlen, &dest_index, *src))
                return -1;
            src++;
        }
        else
        {
            unsigned int offset = 0;
            DWORD byte_pair = 0;

            if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
                return -1;

            while (src < source_end && !utf7_can_directly_encode(*src))
            {
                byte_pair = (byte_pair << 16) | *src;
                offset += 16;
                while (offset >= 6)
                {
                    if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[(byte_pair >> (offset - 6)) & 0x3F]))
                        return -1;
                    offset -= 6;
                }
                src++;
            }

            if (offset)
            {
                /* Windows won't create a padded base64 character if there's no room for the - sign
                 * as well ; this is probably a bug in Windows */
                if (dstlen > 0 && dest_index + 1 >= dstlen)
                    return -1;

                byte_pair <<= (6 - offset);
                if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[byte_pair & 0x3F]))
                    return -1;
            }

            /* Windows always explicitly terminates the base64 sequence
               even though RFC 2152 (page 3, rule 2) does not require this */
            if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
                return -1;
        }
    }

    return dest_index;
}

/***********************************************************************
 *              WideCharToMultiByte   (KERNEL32.@)
 *
 * Convert a Unicode character string into a multibyte string.
 *
 * PARAMS
 *   page    [I] Code page character set to convert to
 *   flags   [I] Mapping Flags (MB_ constants from "winnls.h").
 *   src     [I] Source string buffer
 *   srclen  [I] Length of src (in WCHARs), or -1 if src is NUL terminated
 *   dst     [O] Destination buffer
 *   dstlen  [I] Length of dst (in bytes), or 0 to compute the required length
 *   defchar [I] Default character to use for conversion if no exact
 *		    conversion can be made
 *   used    [O] Set if default character was used in the conversion
 *
 * RETURNS
 *   Success: If dstlen > 0, the number of characters written to dst.
 *            If dstlen == 0, number of characters needed to perform the
 *            conversion. In both cases the count includes the terminating NUL.
 *   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
 *            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
 *            and dstlen != 0, and ERROR_INVALID_PARAMETER, if an invalid
 *            parameter was given.
 */
INT WINAPI WideCharToMultiByte( UINT page, DWORD flags, LPCWSTR src, INT srclen,
                                LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    const union cptable *table;
    int ret, used_tmp;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlenW(src) + 1;

    switch(page)
    {
    case CP_SYMBOL:
        /* when using CP_SYMBOL, ERROR_INVALID_FLAGS takes precedence */
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
        ret = wine_cpsymbol_wcstombs( src, srclen, dst, dstlen );
        break;
    case CP_UTF7:
        /* when using CP_UTF7, ERROR_INVALID_PARAMETER takes precedence */
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
        ret = utf7_wcstombs( src, srclen, dst, dstlen );
        break;
    case CP_UNIXCP:
        if (unix_cptable)
        {
            ret = wine_cp_wcstombs( unix_cptable, flags, src, srclen, dst, dstlen,
                                    defchar, used ? &used_tmp : NULL );
            break;
        }
        /* fall through */
    case CP_UTF8:
        if (defchar || used)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        if (flags & ~WC_FLAGSMASK)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = wine_utf8_wcstombs( flags, src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        if (flags & ~WC_FLAGSMASK)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        ret = wine_cp_wcstombs( table, flags, src, srclen, dst, dstlen,
                                defchar, used ? &used_tmp : NULL );
        if (used) *used = used_tmp;
        break;
    }

    if (ret < 0)
    {
        switch(ret)
        {
        case -1: SetLastError( ERROR_INSUFFICIENT_BUFFER ); break;
        case -2: SetLastError( ERROR_NO_UNICODE_TRANSLATION ); break;
        }
        ret = 0;
    }
    TRACE("cp %d %s -> %s, ret = %d\n",
          page, debugstr_wn(src, srclen), debugstr_an(dst, ret), ret);
    return ret;
}
#endif // !__REACTOS__


/***********************************************************************
 *           GetThreadLocale    (KERNEL32.@)
 *
 * Get the current threads locale.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The LCID currently associated with the calling thread.
 */
LCID WINAPI GetThreadLocale(void)
{
    LCID ret = NtCurrentTeb()->CurrentLocale;
    if (!ret) NtCurrentTeb()->CurrentLocale = ret = GetUserDefaultLCID();
    return ret;
}

/**********************************************************************
 *           SetThreadLocale    (KERNEL32.@)
 *
 * Set the current threads locale.
 *
 * PARAMS
 *  lcid [I] LCID of the locale to set
 *
 * RETURNS
 *  Success: TRUE. The threads locale is set to lcid.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI SetThreadLocale( LCID lcid )
{
    TRACE("(0x%04X)\n", lcid);

    lcid = ConvertDefaultLocale(lcid);

    if (lcid != GetThreadLocale())
    {
        if (!IsValidLocale(lcid, LCID_SUPPORTED))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        NtCurrentTeb()->CurrentLocale = lcid;
    }
    return TRUE;
}

#ifndef __REACTOS__
/**********************************************************************
 *           SetThreadUILanguage    (KERNEL32.@)
 *
 * Set the current threads UI language.
 *
 * PARAMS
 *  langid [I] LANGID of the language to set, or 0 to use
 *             the available language which is best supported
 *             for console applications
 *
 * RETURNS
 *  Success: The return value is the same as the input value.
 *  Failure: The return value differs from the input value.
 *           Use GetLastError() to determine the cause.
 */
LANGID WINAPI SetThreadUILanguage( LANGID langid )
{
    TRACE("(0x%04x) stub - returning success\n", langid);
    return langid;
}
#endif // !__REACTOS__

/******************************************************************************
 *		ConvertDefaultLocale (KERNEL32.@)
 *
 * Convert a default locale identifier into a real identifier.
 *
 * PARAMS
 *  lcid [I] LCID identifier of the locale to convert
 *
 * RETURNS
 *  lcid unchanged, if not a default locale or its sublanguage is
 *   not SUBLANG_NEUTRAL.
 *  GetSystemDefaultLCID(), if lcid == LOCALE_SYSTEM_DEFAULT.
 *  GetUserDefaultLCID(), if lcid == LOCALE_USER_DEFAULT or LOCALE_NEUTRAL.
 *  Otherwise, lcid with sublanguage changed to SUBLANG_DEFAULT.
 */
LCID WINAPI ConvertDefaultLocale( LCID lcid )
{
    LANGID langid;

    switch (lcid)
    {
    case LOCALE_INVARIANT:
        /* keep as-is */
        break;
    case LOCALE_SYSTEM_DEFAULT:
        lcid = GetSystemDefaultLCID();
        break;
    case LOCALE_USER_DEFAULT:
    case LOCALE_NEUTRAL:
        lcid = GetUserDefaultLCID();
        break;
    default:
        /* Replace SUBLANG_NEUTRAL with SUBLANG_DEFAULT */
        langid = LANGIDFROMLCID(lcid);
        if (SUBLANGID(langid) == SUBLANG_NEUTRAL)
        {
          langid = get_default_sublang( langid );
          lcid = MAKELCID(langid, SORTIDFROMLCID(lcid));
        }
    }
    return lcid;
}


/******************************************************************************
 *           IsValidLocale   (KERNEL32.@)
 *
 * Determine if a locale is valid.
 *
 * PARAMS
 *  lcid  [I] LCID of the locale to check
 *  flags [I] LCID_SUPPORTED = Valid, LCID_INSTALLED = Valid and installed on the system
 *
 * RETURNS
 *  TRUE,  if lcid is valid,
 *  FALSE, otherwise.
 *
 * NOTES
 *  Wine does not currently make the distinction between supported and installed. All
 *  languages supported are installed by default.
 */
BOOL WINAPI IsValidLocale( LCID lcid, DWORD flags )
{
    /* check if language is registered in the kernel32 resources */
    return FindResourceExW( kernel32_handle, (LPWSTR)RT_STRING,
                            (LPCWSTR)LOCALE_ILANGUAGE, LANGIDFROMLCID(lcid)) != 0;
}

#ifndef __REACTOS__
/******************************************************************************
 *           IsValidLocaleName   (KERNEL32.@)
 */
BOOL WINAPI IsValidLocaleName( LPCWSTR locale )
{
    struct locale_name locale_name;

    if (!locale)
        return FALSE;

    /* string parsing */
    parse_locale_name( locale, &locale_name );

    TRACE( "found lcid %x for %s, matches %d\n",
           locale_name.lcid, debugstr_w(locale), locale_name.matches );

    return locale_name.matches > 0;
}
#endif // !__REACTOS__

static BOOL CALLBACK enum_lang_proc_a( HMODULE hModule, LPCSTR type,
                                       LPCSTR name, WORD LangID, LONG_PTR lParam )
{
    LOCALE_ENUMPROCA lpfnLocaleEnum = (LOCALE_ENUMPROCA)lParam;
    char buf[20];

    sprintf(buf, "%08x", (UINT)LangID);
    return lpfnLocaleEnum( buf );
}

static BOOL CALLBACK enum_lang_proc_w( HMODULE hModule, LPCWSTR type,
                                       LPCWSTR name, WORD LangID, LONG_PTR lParam )
{
    static const WCHAR formatW[] = {'%','0','8','x',0};
    LOCALE_ENUMPROCW lpfnLocaleEnum = (LOCALE_ENUMPROCW)lParam;
    WCHAR buf[20];
    sprintfW( buf, formatW, (UINT)LangID );
    return lpfnLocaleEnum( buf );
}

/******************************************************************************
 *           EnumSystemLocalesA  (KERNEL32.@)
 *
 * Call a users function for each locale available on the system.
 *
 * PARAMS
 *  lpfnLocaleEnum [I] Callback function to call for each locale
 *  dwFlags        [I] LOCALE_SUPPORTED=All supported, LOCALE_INSTALLED=Installed only
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI EnumSystemLocalesA( LOCALE_ENUMPROCA lpfnLocaleEnum, DWORD dwFlags )
{
    TRACE("(%p,%08x)\n", lpfnLocaleEnum, dwFlags);
    EnumResourceLanguagesA( kernel32_handle, (LPSTR)RT_STRING,
                            (LPCSTR)LOCALE_ILANGUAGE, enum_lang_proc_a,
                            (LONG_PTR)lpfnLocaleEnum);
    return TRUE;
}


/******************************************************************************
 *           EnumSystemLocalesW  (KERNEL32.@)
 *
 * See EnumSystemLocalesA.
 */
BOOL WINAPI EnumSystemLocalesW( LOCALE_ENUMPROCW lpfnLocaleEnum, DWORD dwFlags )
{
    TRACE("(%p,%08x)\n", lpfnLocaleEnum, dwFlags);
    EnumResourceLanguagesW( kernel32_handle, (LPWSTR)RT_STRING,
                            (LPCWSTR)LOCALE_ILANGUAGE, enum_lang_proc_w,
                            (LONG_PTR)lpfnLocaleEnum);
    return TRUE;
}


struct enum_locale_ex_data
{
    LOCALE_ENUMPROCEX proc;
    DWORD             flags;
    LPARAM            lparam;
};

static BOOL CALLBACK enum_locale_ex_proc( HMODULE module, LPCWSTR type,
                                          LPCWSTR name, WORD lang, LONG_PTR lparam )
{
    struct enum_locale_ex_data *data = (struct enum_locale_ex_data *)lparam;
    WCHAR buffer[256];
    DWORD neutral;
    unsigned int flags;

    GetLocaleInfoW( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SNAME | LOCALE_NOUSEROVERRIDE,
                    buffer, sizeof(buffer) / sizeof(WCHAR) );
    if (!GetLocaleInfoW( MAKELCID( lang, SORT_DEFAULT ),
                         LOCALE_INEUTRAL | LOCALE_NOUSEROVERRIDE | LOCALE_RETURN_NUMBER,
                         (LPWSTR)&neutral, sizeof(neutral) / sizeof(WCHAR) ))
        neutral = 0;
    flags = LOCALE_WINDOWS;
    flags |= neutral ? LOCALE_NEUTRALDATA : LOCALE_SPECIFICDATA;
    if (data->flags && !(data->flags & flags)) return TRUE;
    return data->proc( buffer, flags, data->lparam );
}

/******************************************************************************
 *           EnumSystemLocalesEx  (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLocalesEx( LOCALE_ENUMPROCEX proc, DWORD flags, LPARAM lparam, LPVOID reserved )
{
    struct enum_locale_ex_data data;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    data.proc   = proc;
    data.flags  = flags;
    data.lparam = lparam;
    EnumResourceLanguagesW( kernel32_handle, (LPCWSTR)RT_STRING,
                            (LPCWSTR)MAKEINTRESOURCE((LOCALE_SNAME >> 4) + 1),
                            enum_locale_ex_proc, (LONG_PTR)&data );
    return TRUE;
}


/***********************************************************************
 *           VerLanguageNameA  (KERNEL32.@)
 *
 * Get the name of a language.
 *
 * PARAMS
 *  wLang  [I] LANGID of the language
 *  szLang [O] Destination for the language name
 *
 * RETURNS
 *  Success: The size of the language name. If szLang is non-NULL, it is filled
 *           with the name.
 *  Failure: 0. Use GetLastError() to determine the cause.
 *
 */
DWORD WINAPI VerLanguageNameA( DWORD wLang, LPSTR szLang, DWORD nSize )
{
    return GetLocaleInfoA( MAKELCID(wLang, SORT_DEFAULT), LOCALE_SENGLANGUAGE, szLang, nSize );
}


/***********************************************************************
 *           VerLanguageNameW  (KERNEL32.@)
 *
 * See VerLanguageNameA.
 */
DWORD WINAPI VerLanguageNameW( DWORD wLang, LPWSTR szLang, DWORD nSize )
{
    return GetLocaleInfoW( MAKELCID(wLang, SORT_DEFAULT), LOCALE_SENGLANGUAGE, szLang, nSize );
}


/******************************************************************************
 *           GetStringTypeW    (KERNEL32.@)
 *
 * See GetStringTypeA.
 */
BOOL WINAPI GetStringTypeW( DWORD type, LPCWSTR src, INT count, LPWORD chartype )
{
    static const unsigned char type2_map[16] =
    {
        C2_NOTAPPLICABLE,      /* unassigned */
        C2_LEFTTORIGHT,        /* L */
        C2_RIGHTTOLEFT,        /* R */
        C2_EUROPENUMBER,       /* EN */
        C2_EUROPESEPARATOR,    /* ES */
        C2_EUROPETERMINATOR,   /* ET */
        C2_ARABICNUMBER,       /* AN */
        C2_COMMONSEPARATOR,    /* CS */
        C2_BLOCKSEPARATOR,     /* B */
        C2_SEGMENTSEPARATOR,   /* S */
        C2_WHITESPACE,         /* WS */
        C2_OTHERNEUTRAL,       /* ON */
        C2_RIGHTTOLEFT,        /* AL */
        C2_NOTAPPLICABLE,      /* NSM */
        C2_NOTAPPLICABLE,      /* BN */
        C2_OTHERNEUTRAL        /* LRE, LRO, RLE, RLO, PDF */
    };

    if (!src)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (count == -1) count = strlenW(src) + 1;
    switch(type)
    {
    case CT_CTYPE1:
        while (count--) *chartype++ = get_char_typeW( *src++ ) & 0xfff;
        break;
    case CT_CTYPE2:
        while (count--) *chartype++ = type2_map[get_char_typeW( *src++ ) >> 12];
        break;
    case CT_CTYPE3:
    {
        WARN("CT_CTYPE3: semi-stub.\n");
        while (count--)
        {
            int c = *src;
            WORD type1, type3 = 0; /* C3_NOTAPPLICABLE */

            type1 = get_char_typeW( *src++ ) & 0xfff;
            /* try to construct type3 from type1 */
            if(type1 & C1_SPACE) type3 |= C3_SYMBOL;
            if(type1 & C1_ALPHA) type3 |= C3_ALPHA;
            if ((c>=0x30A0)&&(c<=0x30FF)) type3 |= C3_KATAKANA;
            if ((c>=0x3040)&&(c<=0x309F)) type3 |= C3_HIRAGANA;
            if ((c>=0x4E00)&&(c<=0x9FAF)) type3 |= C3_IDEOGRAPH;
            if (c == 0x0640) type3 |= C3_KASHIDA;
            if ((c>=0x3000)&&(c<=0x303F)) type3 |= C3_SYMBOL;

            if ((c>=0xD800)&&(c<=0xDBFF)) type3 |= C3_HIGHSURROGATE;
            if ((c>=0xDC00)&&(c<=0xDFFF)) type3 |= C3_LOWSURROGATE;

            if ((c>=0xFF00)&&(c<=0xFF60)) type3 |= C3_FULLWIDTH;
            if ((c>=0xFF00)&&(c<=0xFF20)) type3 |= C3_SYMBOL;
            if ((c>=0xFF3B)&&(c<=0xFF40)) type3 |= C3_SYMBOL;
            if ((c>=0xFF5B)&&(c<=0xFF60)) type3 |= C3_SYMBOL;
            if ((c>=0xFF21)&&(c<=0xFF3A)) type3 |= C3_ALPHA;
            if ((c>=0xFF41)&&(c<=0xFF5A)) type3 |= C3_ALPHA;
            if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_FULLWIDTH;
            if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_SYMBOL;

            if ((c>=0xFF61)&&(c<=0xFFDC)) type3 |= C3_HALFWIDTH;
            if ((c>=0xFF61)&&(c<=0xFF64)) type3 |= C3_SYMBOL;
            if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_KATAKANA;
            if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_ALPHA;
            if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_HALFWIDTH;
            if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_SYMBOL;
            *chartype++ = type3;
        }
        break;
    }
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/******************************************************************************
 *           GetStringTypeExW    (KERNEL32.@)
 *
 * See GetStringTypeExA.
 */
BOOL WINAPI GetStringTypeExW( LCID locale, DWORD type, LPCWSTR src, INT count, LPWORD chartype )
{
    /* locale is ignored for Unicode */
    return GetStringTypeW( type, src, count, chartype );
}


/******************************************************************************
 *           GetStringTypeA    (KERNEL32.@)
 *
 * Get characteristics of the characters making up a string.
 *
 * PARAMS
 *  locale   [I] Locale Id for the string
 *  type     [I] CT_CTYPE1 = classification, CT_CTYPE2 = directionality, CT_CTYPE3 = typographic info
 *  src      [I] String to analyse
 *  count    [I] Length of src in chars, or -1 if src is NUL terminated
 *  chartype [O] Destination for the calculated characteristics
 *
 * RETURNS
 *  Success: TRUE. chartype is filled with the requested characteristics of each char
 *           in src.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI GetStringTypeA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    UINT cp;
    INT countW;
    LPWSTR srcW;
    BOOL ret = FALSE;

    if(count == -1) count = strlen(src) + 1;

    if (!(cp = get_lcid_codepage( locale )))
    {
        FIXME("For locale %04x using current ANSI code page\n", locale);
        cp = GetACP();
    }

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

/******************************************************************************
 *           GetStringTypeExA    (KERNEL32.@)
 *
 * Get characteristics of the characters making up a string.
 *
 * PARAMS
 *  locale   [I] Locale Id for the string
 *  type     [I] CT_CTYPE1 = classification, CT_CTYPE2 = directionality, CT_CTYPE3 = typographic info
 *  src      [I] String to analyse
 *  count    [I] Length of src in chars, or -1 if src is NUL terminated
 *  chartype [O] Destination for the calculated characteristics
 *
 * RETURNS
 *  Success: TRUE. chartype is filled with the requested characteristics of each char
 *           in src.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI GetStringTypeExA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    return GetStringTypeA(locale, type, src, count, chartype);
}

#ifdef __REACTOS__
static inline void map_byterev(const WCHAR *src, int len, WCHAR *dst)
{
    while (len--)
        *dst++ = RtlUshortByteSwap(*src++);
}

static int map_to_hiragana(const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--, pos++)
    {
        /*
         * U+30A1 ... U+30F3: Katakana
         * U+30F4: Katakana Letter VU
         * U+30F5: Katakana Letter Small KA
         * U+30FD: Katakana Iteration Mark
         * U+30FE: Katakana Voiced Iteration Mark
         */
        WCHAR wch = *src;
        if ((0x30A1 <= wch && wch <= 0x30F3) ||
            wch == 0x30F4 || wch == 0x30F5 || wch == 0x30FD || wch == 0x30FE)
        {
            wch -= 0x60; /* Katakana to Hiragana */
        }
        if (pos < dstlen)
            dst[pos] = wch;
    }
    return pos;
}

static int map_to_katakana(const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--, pos++)
    {
        /*
         * U+3041 ... U+3093: Hiragana
         * U+3094: Hiragana Letter VU
         * U+3095: Hiragana Letter Small KA
         * U+309D: Hiragana Iteration Mark
         * U+309E: Hiragana Voiced Iteration Mark
         */
        WCHAR wch = *src;
        if ((0x3041 <= wch && wch <= 0x3093) ||
            wch == 3094 || wch == 0x3095 || wch == 0x309D || wch == 0x309E)
        {
            wch += 0x60; /* Hiragana to Katakana */
        }
        if (pos < dstlen)
            dst[pos] = wch;
    }
    return pos;
}

/* The table that contains fullwidth characters and halfwidth characters */
typedef WCHAR FULL2HALF_ENTRY[3];
static const FULL2HALF_ENTRY full2half_table[] =
{
#define DEFINE_FULL2HALF(full, half1, half2) { full, half1, half2 },
#include "full2half.h"
#undef DEFINE_FULL2HALF
};
#define GET_FULL(table, index)  ((table)[index][0])
#define GET_HALF1(table, index) ((table)[index][1])
#define GET_HALF2(table, index) ((table)[index][2])

/* The table that contains dakuten entries */
typedef WCHAR DAKUTEN_ENTRY[3];
static const DAKUTEN_ENTRY dakuten_table[] =
{
#define DEFINE_DAKUTEN(voiced, single1, single2, half1, half2) { voiced, single1, single2 },
#include "dakuten.h"
#undef DEFINE_DAKUTEN
};
#define GET_VOICED(table, index) ((table)[index][0])
#define GET_SINGLE1(table, index) ((table)[index][1])
#define GET_SINGLE2(table, index) ((table)[index][2])

static int map_to_halfwidth(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos, i;
    const int count1 = (int)ARRAY_SIZE(full2half_table);
    const FULL2HALF_ENTRY *table1 = full2half_table;

    for (pos = 0; srclen; src++, srclen--, pos++)
    {
        WCHAR ch = *src;

        if (flags & LCMAP_KATAKANA)
            map_to_katakana(&ch, 1, &ch, 1);
        else if (flags & LCMAP_HIRAGANA)
            map_to_hiragana(&ch, 1, &ch, 1);

        if (ch < 0x3000) /* Quick judgment */
        {
            if (pos < dstlen)
                dst[pos] = ch;
            continue;
        }

        if (0xFF01 <= ch && ch <= 0xFF5E) /* U+FF01 ... U+FF5E */
        {
            if (pos < dstlen)
                dst[pos] = ch - 0xFEE0; /* Fullwidth ASCII to halfwidth ASCII */
            continue;
        }

        /* Search in table1 (full/half) */
        for (i = count1 - 1; i >= 0; --i) /* In reverse order */
        {
            if (GET_FULL(table1, i) != ch)
                continue;

            if (GET_HALF2(table1, i) == 0)
            {
                if (pos < dstlen)
                    dst[pos] = GET_HALF1(table1, i);
            }
            else if (!dstlen)
            {
                pos++;
            }
            else if (pos + 1 < dstlen)
            {
                dst[pos++] = GET_HALF1(table1, i);
                dst[pos  ] = GET_HALF2(table1, i);
            }
            else
            {
                dst[pos] = ch;
            }
            break;
        }

        if (i >= 0)
            continue;

        if (pos < dstlen)
            dst[pos] = ch;
    }

    return pos;
}

static int map_to_fullwidth(const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos, i;
    const FULL2HALF_ENTRY *table1 = full2half_table;
    const DAKUTEN_ENTRY *table2 = dakuten_table;
    const int count1 = (int)ARRAY_SIZE(full2half_table);
    const int count2 = (int)ARRAY_SIZE(dakuten_table);

    for (pos = 0; srclen; src++, srclen--, pos++)
    {
        WCHAR ch = *src;

        if (ch == 0x20) /* U+0020: Space */
        {
            if (pos < dstlen)
                dst[pos] = 0x3000; /* U+3000: Ideographic Space */
            continue;
        }

        if (0x21 <= ch && ch <= 0x7E) /* Mappable halfwidth ASCII */
        {
            if (pos < dstlen)
                dst[pos] = ch + 0xFEE0; /* U+FF01 ... U+FF5E */
            continue;
        }

        if (ch < 0xFF00) /* Quick judgment */
        {
            if (pos < dstlen)
                dst[pos] = ch;
            continue;
        }

        /* Search in table1 (full/half) */
        for (i = count1 - 1; i >= 0; --i) /* In reverse order */
        {
            if (GET_HALF1(table1, i) != ch)
                continue; /* Mismatched */

            if (GET_HALF2(table1, i) == 0)
            {
                if (pos < dstlen)
                    dst[pos] = GET_FULL(table1, i);
                break;
            }

            if (srclen <= 1 || GET_HALF2(table1, i) != src[1])
                continue; /* Mismatched */

            --srclen;
            ++src;

            if (pos < dstlen)
                dst[pos] = GET_FULL(table1, i);
            break;
        }

        if (i >= 0)
            continue;

        /* Search in table2 (dakuten) */
        for (i = count2 - 1; i >= 0; --i) /* In reverse order */
        {
            if (GET_SINGLE1(table2, i) != ch)
                continue; /* Mismatched */

            if (srclen <= 1 || GET_SINGLE2(table2, i) != src[1])
                continue; /* Mismatched */

            --srclen;
            ++src;

            if (pos < dstlen)
                dst[pos] = GET_VOICED(table2, i);
            break;
        }

        if (i >= 0)
            continue;

        if (pos < dstlen)
            dst[pos] = ch;
    }

    return pos;
}

static int map_to_lowercase(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR wch = *src;
        if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
            continue;
        if (pos < dstlen)
            dst[pos] = tolowerW(wch);
        pos++;
    }
    return pos;
}

static int map_to_uppercase(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR wch = *src;
        if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
            continue;
        if (pos < dstlen)
            dst[pos] = toupperW(wch);
        pos++;
    }
    return pos;
}

typedef struct tagWCHAR_PAIR
{
    WCHAR from, to;
} WCHAR_PAIR, *PWCHAR_PAIR;

/* The table to convert Simplified Chinese to Traditional Chinese */
static const WCHAR_PAIR s_sim2tra[] =
{
#define DEFINE_SIM2TRA(from, to) { from, to },
#include "sim2tra.h"
#undef DEFINE_SIM2TRA
};

/* The table to convert Traditional Chinese to Simplified Chinese */
static const WCHAR_PAIR s_tra2sim[] =
{
#define DEFINE_TRA2SIM(from, to) { from, to },
#include "tra2sim.h"
#undef DEFINE_TRA2SIM
};

/* The comparison function to do bsearch */
static int compare_wchar_pair(const void *x, const void *y)
{
    const WCHAR_PAIR *a = x;
    const WCHAR_PAIR *b = y;
    if (a->from < b->from)
        return -1;
    if (a->from > b->from)
        return +1;
    return 0;
}

static WCHAR find_wchar_pair(const WCHAR_PAIR *pairs, size_t count, WCHAR ch)
{
    PWCHAR_PAIR found = bsearch(&ch, pairs, count, sizeof(WCHAR_PAIR), compare_wchar_pair);
    if (found)
        return found->to;
    return ch;
}

static int map_to_simplified_chinese(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR wch = *src;
        if (pos < dstlen)
            dst[pos] = find_wchar_pair(s_tra2sim, ARRAY_SIZE(s_tra2sim), wch);
        pos++;
    }
    return pos;
}

static int map_to_traditional_chinese(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR wch = *src;
        if (pos < dstlen)
            dst[pos] = find_wchar_pair(s_sim2tra, ARRAY_SIZE(s_sim2tra), wch);
        pos++;
    }
    return pos;
}

static int map_remove_ignored(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int pos;
    WORD wC1, wC2, wC3;
    for (pos = 0; srclen; src++, srclen--)
    {
        WCHAR wch = *src;
        GetStringTypeW(CT_CTYPE1, &wch, 1, &wC1);
        GetStringTypeW(CT_CTYPE2, &wch, 1, &wC2);
        GetStringTypeW(CT_CTYPE3, &wch, 1, &wC3);
        if (flags & NORM_IGNORESYMBOLS)
        {
            if ((wC1 & C1_PUNCT) || (wC3 & C3_SYMBOL))
                continue;
        }
        if (flags & NORM_IGNORENONSPACE)
        {
            if ((wC2 & C2_OTHERNEUTRAL) && (wC3 & (C3_NONSPACING | C3_DIACRITIC)))
                continue;
        }
        if (pos < dstlen)
            dst[pos] = wch;
        pos++;
    }
    return pos;
}

static int lcmap_string(DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int dstlen)
{
    int ret = 0;

    if ((flags & (LCMAP_LOWERCASE | LCMAP_UPPERCASE)) == (LCMAP_LOWERCASE | LCMAP_UPPERCASE))
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    switch (flags & ~(LCMAP_BYTEREV | LCMAP_LOWERCASE | LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING))
    {
    case LCMAP_HIRAGANA:
        ret = map_to_hiragana(src, srclen, dst, dstlen);
        break;
    case LCMAP_KATAKANA:
        ret = map_to_katakana(src, srclen, dst, dstlen);
        break;
    case LCMAP_HALFWIDTH:
        ret = map_to_halfwidth(flags, src, srclen, dst, dstlen);
        break;
    case LCMAP_HIRAGANA | LCMAP_HALFWIDTH:
        ret = map_to_halfwidth(flags, src, srclen, dst, dstlen);
        break;
    case LCMAP_KATAKANA | LCMAP_HALFWIDTH:
        ret = map_to_halfwidth(flags, src, srclen, dst, dstlen);
        break;
    case LCMAP_FULLWIDTH:
        ret = map_to_fullwidth(src, srclen, dst, dstlen);
        break;
    case LCMAP_HIRAGANA | LCMAP_FULLWIDTH:
        ret = map_to_fullwidth(src, srclen, dst, dstlen);
        if (dstlen && ret)
            map_to_hiragana(dst, ret, dst, dstlen);
        break;
    case LCMAP_KATAKANA | LCMAP_FULLWIDTH:
        ret = map_to_fullwidth(src, srclen, dst, dstlen);
        if (dstlen && ret)
            map_to_katakana(dst, ret, dst, dstlen);
        break;
    case LCMAP_SIMPLIFIED_CHINESE:
        ret = map_to_simplified_chinese(flags, src, srclen, dst, dstlen);
        break;
    case LCMAP_TRADITIONAL_CHINESE:
        ret = map_to_traditional_chinese(flags, src, srclen, dst, dstlen);
        break;
    case NORM_IGNORENONSPACE:
    case NORM_IGNORESYMBOLS:
    case NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS:
        if (flags & ~(NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS | LCMAP_BYTEREV))
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return 0;
        }
        ret = map_remove_ignored(flags, src, srclen, dst, dstlen);
        break;
    case 0:
        if (flags & LCMAP_LOWERCASE)
        {
            ret = map_to_lowercase(flags, src, srclen, dst, dstlen);
            flags &= ~LCMAP_LOWERCASE;
            break;
        }
        if (flags & LCMAP_UPPERCASE)
        {
            ret = map_to_uppercase(flags, src, srclen, dst, dstlen);
            flags &= ~LCMAP_UPPERCASE;
            break;
        }
        if (flags & LCMAP_BYTEREV)
        {
            if (dstlen == 0)
            {
                ret = srclen;
                break;
            }
            ret = min(srclen, dstlen);
            RtlCopyMemory(dst, src, ret * sizeof(WCHAR));
            break;
        }
        /* fall through */
    default:
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (dstlen)
    {
        if (flags & LCMAP_LOWERCASE)
            map_to_lowercase(flags, dst, ret, dst, dstlen);
        if (flags & LCMAP_UPPERCASE)
            map_to_uppercase(flags, dst, ret, dst, dstlen);
        if (flags & LCMAP_BYTEREV)
            map_byterev(dst, min(ret, dstlen), dst);

        if (dstlen < ret)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
    }

    return ret;
}
#endif // __REACTOS__

/*************************************************************************
 *           LCMapStringEx   (KERNEL32.@)
 *
 * Map characters in a locale sensitive string.
 *
 * PARAMS
 *  locale   [I] Locale name for the conversion.
 *  flags    [I] Flags controlling the mapping (LCMAP_ constants from "winnls.h")
 *  src      [I] String to map
 *  srclen   [I] Length of src in chars, or -1 if src is NUL terminated
 *  dst      [O] Destination for mapped string
 *  dstlen   [I] Length of dst in characters
 *  version  [I] reserved, must be NULL
 *  reserved [I] reserved, must be NULL
 *  lparam   [I] reserved, must be 0
 *
 * RETURNS
 *  Success: The length of the mapped string in dst, including the NUL terminator.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI LCMapStringEx(LPCWSTR locale, DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen,
                         LPNLSVERSIONINFO version, LPVOID reserved, LPARAM handle)
{
    if (version) FIXME("unsupported version structure %p\n", version);
    if (reserved) FIXME("unsupported reserved pointer %p\n", reserved);
    if (handle)
    {
        static int once;
        if (!once++) FIXME("unsupported lparam %Ix\n", handle);
    }

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    TRACE( "(%s,0x%08lx,%s,%d,%p,%d)\n",
           debugstr_w(locale), flags, debugstr_wn(src, srclen), srclen, dst, dstlen );

    flags &= ~LOCALE_USE_CP_ACP;

    if (src == dst && (flags & ~(LCMAP_LOWERCASE | LCMAP_UPPERCASE)))
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (!dstlen) dst = NULL;

    if (flags & LCMAP_SORTKEY)
    {
        INT ret;

        if (srclen < 0)
            srclen = strlenW(src);

        ret = wine_get_sortkey(flags, src, srclen, (char *)dst, dstlen);
        if (ret == 0)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        else
            ret++;
        return ret;
    }

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    if (flags & SORT_STRINGSORT)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    return lcmap_string(flags, src, srclen, dst, dstlen);
}

/*************************************************************************
 *           LCMapStringW    (KERNEL32.@)
 *
 * See LCMapStringA.
 */
INT WINAPI LCMapStringW(LCID lcid, DWORD flags, LPCWSTR src, INT srclen,
                        LPWSTR dst, INT dstlen)
{
    TRACE("(0x%04x,0x%08x,%s,%d,%p,%d)\n",
          lcid, flags, debugstr_wn(src, srclen), srclen, dst, dstlen);

    return LCMapStringEx(NULL, flags, src, srclen, dst, dstlen, NULL, NULL, 0);
}

/*************************************************************************
 *           LCMapStringA    (KERNEL32.@)
 *
 * Map characters in a locale sensitive string.
 *
 * PARAMS
 *  lcid   [I] LCID for the conversion.
 *  flags  [I] Flags controlling the mapping (LCMAP_ constants from "winnls.h").
 *  src    [I] String to map
 *  srclen [I] Length of src in chars, or -1 if src is NUL terminated
 *  dst    [O] Destination for mapped string
 *  dstlen [I] Length of dst in characters
 *
 * RETURNS
 *  Success: The length of the mapped string in dst, including the NUL terminator.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI LCMapStringA(LCID lcid, DWORD flags, LPCSTR src, INT srclen,
                        LPSTR dst, INT dstlen)
{
    WCHAR *bufW = NtCurrentTeb()->StaticUnicodeBuffer;
    LPWSTR srcW, dstW;
    INT ret = 0, srclenW, dstlenW;
    UINT locale_cp = CP_ACP;

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(flags & LOCALE_USE_CP_ACP)) locale_cp = get_lcid_codepage( lcid );

    srclenW = MultiByteToWideChar(locale_cp, 0, src, srclen, bufW, 260);
    if (srclenW)
        srcW = bufW;
    else
    {
        srclenW = MultiByteToWideChar(locale_cp, 0, src, srclen, NULL, 0);
        srcW = HeapAlloc(GetProcessHeap(), 0, srclenW * sizeof(WCHAR));
        if (!srcW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        MultiByteToWideChar(locale_cp, 0, src, srclen, srcW, srclenW);
    }

    if (flags & LCMAP_SORTKEY)
    {
        if (src == dst)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            goto map_string_exit;
        }
        ret = wine_get_sortkey(flags, srcW, srclenW, dst, dstlen);
        if (ret == 0)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        else
            ret++;
        goto map_string_exit;
    }

    if (flags & SORT_STRINGSORT)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        goto map_string_exit;
    }

    dstlenW = LCMapStringEx(NULL, flags, srcW, srclenW, NULL, 0, NULL, NULL, 0);
    if (!dstlenW)
        goto map_string_exit;

    dstW = HeapAlloc(GetProcessHeap(), 0, dstlenW * sizeof(WCHAR));
    if (!dstW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto map_string_exit;
    }

    LCMapStringEx(NULL, flags, srcW, srclenW, dstW, dstlenW, NULL, NULL, 0);
    ret = WideCharToMultiByte(locale_cp, 0, dstW, dstlenW, dst, dstlen, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, dstW);

map_string_exit:
    if (srcW != bufW) HeapFree(GetProcessHeap(), 0, srcW);
    return ret;
}

/*************************************************************************
 *           FoldStringA    (KERNEL32.@)
 *
 * Map characters in a string.
 *
 * PARAMS
 *  dwFlags [I] Flags controlling chars to map (MAP_ constants from "winnls.h")
 *  src     [I] String to map
 *  srclen  [I] Length of src, or -1 if src is NUL terminated
 *  dst     [O] Destination for mapped string
 *  dstlen  [I] Length of dst, or 0 to find the required length for the mapped string
 *
 * RETURNS
 *  Success: The length of the string written to dst, including the terminating NUL. If
 *           dstlen is 0, the value returned is the same, but nothing is written to dst,
 *           and dst may be NULL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI FoldStringA(DWORD dwFlags, LPCSTR src, INT srclen,
                       LPSTR dst, INT dstlen)
{
    INT ret = 0, srclenW = 0;
    WCHAR *srcW = NULL, *dstW = NULL;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    srclenW = MultiByteToWideChar(CP_ACP, dwFlags & MAP_COMPOSITE ? MB_COMPOSITE : 0,
                                  src, srclen, NULL, 0);
    srcW = HeapAlloc(GetProcessHeap(), 0, srclenW * sizeof(WCHAR));

    if (!srcW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto FoldStringA_exit;
    }

    MultiByteToWideChar(CP_ACP, dwFlags & MAP_COMPOSITE ? MB_COMPOSITE : 0,
                        src, srclen, srcW, srclenW);

    dwFlags = (dwFlags & ~MAP_PRECOMPOSED) | MAP_FOLDCZONE;

    ret = FoldStringW(dwFlags, srcW, srclenW, NULL, 0);
    if (ret && dstlen)
    {
        dstW = HeapAlloc(GetProcessHeap(), 0, ret * sizeof(WCHAR));

        if (!dstW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto FoldStringA_exit;
        }

        ret = FoldStringW(dwFlags, srcW, srclenW, dstW, ret);
        if (!WideCharToMultiByte(CP_ACP, 0, dstW, ret, dst, dstlen, NULL, NULL))
        {
            ret = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    HeapFree(GetProcessHeap(), 0, dstW);

FoldStringA_exit:
    HeapFree(GetProcessHeap(), 0, srcW);
    return ret;
}

/*************************************************************************
 *           FoldStringW    (KERNEL32.@)
 *
 * See FoldStringA.
 */
INT WINAPI FoldStringW(DWORD dwFlags, LPCWSTR src, INT srclen,
                       LPWSTR dst, INT dstlen)
{
    int ret;

    switch (dwFlags & (MAP_COMPOSITE|MAP_PRECOMPOSED|MAP_EXPAND_LIGATURES))
    {
    case 0:
        if (dwFlags)
          break;
        /* Fall through for dwFlags == 0 */
    case MAP_PRECOMPOSED|MAP_COMPOSITE:
    case MAP_PRECOMPOSED|MAP_EXPAND_LIGATURES:
    case MAP_COMPOSITE|MAP_EXPAND_LIGATURES:
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ret = wine_fold_string(dwFlags, src, srclen, dst, dstlen);
    if (!ret)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return ret;
}

/******************************************************************************
 *           CompareStringW    (KERNEL32.@)
 *
 * See CompareStringA.
 */
INT WINAPI CompareStringW(LCID lcid, DWORD flags,
                          LPCWSTR str1, INT len1, LPCWSTR str2, INT len2)
{
    return CompareStringEx(NULL, flags, str1, len1, str2, len2, NULL, NULL, 0);
}

/******************************************************************************
 *           CompareStringEx    (KERNEL32.@)
 */
INT WINAPI CompareStringEx(LPCWSTR locale, DWORD flags, LPCWSTR str1, INT len1,
                           LPCWSTR str2, INT len2, LPNLSVERSIONINFO version, LPVOID reserved, LPARAM lParam)
{
    DWORD supported_flags = NORM_IGNORECASE|NORM_IGNORENONSPACE|NORM_IGNORESYMBOLS|SORT_STRINGSORT
                           |NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH|LOCALE_USE_CP_ACP;
    DWORD semistub_flags = NORM_LINGUISTIC_CASING|LINGUISTIC_IGNORECASE|0x10000000;
    /* 0x10000000 is related to diacritics in Arabic, Japanese, and Hebrew */
    INT ret;
    static int once;

    if (version) FIXME("unexpected version parameter\n");
    if (reserved) FIXME("unexpected reserved value\n");
    if (lParam) FIXME("unexpected lParam\n");

    if (!str1 || !str2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (flags & ~(supported_flags|semistub_flags))
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (flags & semistub_flags)
    {
        if (!once++)
            FIXME("semi-stub behavior for flag(s) 0x%x\n", flags & semistub_flags);
    }

    if (len1 < 0) len1 = strlenW(str1);
    if (len2 < 0) len2 = strlenW(str2);

    ret = wine_compare_string(flags, str1, len1, str2, len2);

    if (ret) /* need to translate result */
        return (ret < 0) ? CSTR_LESS_THAN : CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}

/******************************************************************************
 *           CompareStringA    (KERNEL32.@)
 *
 * Compare two locale sensitive strings.
 *
 * PARAMS
 *  lcid  [I] LCID for the comparison
 *  flags [I] Flags for the comparison (NORM_ constants from "winnls.h").
 *  str1  [I] First string to compare
 *  len1  [I] Length of str1, or -1 if str1 is NUL terminated
 *  str2  [I] Second string to compare
 *  len2  [I] Length of str2, or -1 if str2 is NUL terminated
 *
 * RETURNS
 *  Success: CSTR_LESS_THAN, CSTR_EQUAL or CSTR_GREATER_THAN depending on whether
 *           str1 is less than, equal to or greater than str2 respectively.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
INT WINAPI CompareStringA(LCID lcid, DWORD flags,
                          LPCSTR str1, INT len1, LPCSTR str2, INT len2)
{
    WCHAR *buf1W = NtCurrentTeb()->StaticUnicodeBuffer;
    WCHAR *buf2W = buf1W + 130;
    LPWSTR str1W, str2W;
    INT len1W = 0, len2W = 0, ret;
    UINT locale_cp = CP_ACP;

    if (!str1 || !str2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (len1 < 0) len1 = strlen(str1);
    if (len2 < 0) len2 = strlen(str2);

    if (!(flags & LOCALE_USE_CP_ACP)) locale_cp = get_lcid_codepage( lcid );

    if (len1)
    {
        if (len1 <= 130) len1W = MultiByteToWideChar(locale_cp, 0, str1, len1, buf1W, 130);
        if (len1W)
            str1W = buf1W;
        else
        {
            len1W = MultiByteToWideChar(locale_cp, 0, str1, len1, NULL, 0);
            str1W = HeapAlloc(GetProcessHeap(), 0, len1W * sizeof(WCHAR));
            if (!str1W)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
            MultiByteToWideChar(locale_cp, 0, str1, len1, str1W, len1W);
        }
    }
    else
    {
        len1W = 0;
        str1W = buf1W;
    }

    if (len2)
    {
        if (len2 <= 130) len2W = MultiByteToWideChar(locale_cp, 0, str2, len2, buf2W, 130);
        if (len2W)
            str2W = buf2W;
        else
        {
            len2W = MultiByteToWideChar(locale_cp, 0, str2, len2, NULL, 0);
            str2W = HeapAlloc(GetProcessHeap(), 0, len2W * sizeof(WCHAR));
            if (!str2W)
            {
                if (str1W != buf1W) HeapFree(GetProcessHeap(), 0, str1W);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
            MultiByteToWideChar(locale_cp, 0, str2, len2, str2W, len2W);
        }
    }
    else
    {
        len2W = 0;
        str2W = buf2W;
    }

    ret = CompareStringEx(NULL, flags, str1W, len1W, str2W, len2W, NULL, NULL, 0);

    if (str1W != buf1W) HeapFree(GetProcessHeap(), 0, str1W);
    if (str2W != buf2W) HeapFree(GetProcessHeap(), 0, str2W);
    return ret;
}

#if (WINVER >= 0x0600)
/******************************************************************************
 *           CompareStringOrdinal    (KERNEL32.@)
 */
INT WINAPI CompareStringOrdinal(const WCHAR *str1, INT len1, const WCHAR *str2, INT len2, BOOL ignore_case)
{
    int ret;

    if (!str1 || !str2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (len1 < 0) len1 = strlenW(str1);
    if (len2 < 0) len2 = strlenW(str2);

    ret = RtlCompareUnicodeStrings( str1, len1, str2, len2, ignore_case );
    if (ret < 0) return CSTR_LESS_THAN;
    if (ret > 0) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}
#endif // (WINVER >= 0x0600)

#ifndef __REACTOS__
/*************************************************************************
 *           lstrcmp     (KERNEL32.@)
 *           lstrcmpA    (KERNEL32.@)
 *
 * Compare two strings using the current thread locale.
 *
 * PARAMS
 *  str1  [I] First string to compare
 *  str2  [I] Second string to compare
 *
 * RETURNS
 *  Success: A number less than, equal to or greater than 0 depending on whether
 *           str1 is less than, equal to or greater than str2 respectively.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
int WINAPI lstrcmpA(LPCSTR str1, LPCSTR str2)
{
    int ret;

    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    ret = CompareStringA(GetThreadLocale(), LOCALE_USE_CP_ACP, str1, -1, str2, -1);
    if (ret) ret -= 2;

    return ret;
}

/*************************************************************************
 *           lstrcmpi     (KERNEL32.@)
 *           lstrcmpiA    (KERNEL32.@)
 *
 * Compare two strings using the current thread locale, ignoring case.
 *
 * PARAMS
 *  str1  [I] First string to compare
 *  str2  [I] Second string to compare
 *
 * RETURNS
 *  Success: A number less than, equal to or greater than 0 depending on whether
 *           str2 is less than, equal to or greater than str1 respectively.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
int WINAPI lstrcmpiA(LPCSTR str1, LPCSTR str2)
{
    int ret;

    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    ret = CompareStringA(GetThreadLocale(), NORM_IGNORECASE|LOCALE_USE_CP_ACP, str1, -1, str2, -1);
    if (ret) ret -= 2;

    return ret;
}

/*************************************************************************
 *           lstrcmpW    (KERNEL32.@)
 *
 * See lstrcmpA.
 */
int WINAPI lstrcmpW(LPCWSTR str1, LPCWSTR str2)
{
    int ret;

    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    ret = CompareStringW(GetThreadLocale(), 0, str1, -1, str2, -1);
    if (ret) ret -= 2;

    return ret;
}

/*************************************************************************
 *           lstrcmpiW    (KERNEL32.@)
 *
 * See lstrcmpiA.
 */
int WINAPI lstrcmpiW(LPCWSTR str1, LPCWSTR str2)
{
    int ret;

    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    ret = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str1, -1, str2, -1);
    if (ret) ret -= 2;

    return ret;
}

/******************************************************************************
 *		LOCALE_Init
 */
void LOCALE_Init(void)
{
    extern void CDECL __wine_init_codepages( const union cptable *ansi_cp, const union cptable *oem_cp,
                                             const union cptable *unix_cp );

    UINT ansi_cp = 1252, oem_cp = 437, mac_cp = 10000, unix_cp;

    setlocale( LC_ALL, "" );

#ifdef __APPLE__
    /* MacOS doesn't set the locale environment variables so we have to do it ourselves */
    if (!has_env("LANG"))
    {
        const char* mac_locale = get_mac_locale();

        setenv( "LANG", mac_locale, 1 );
        if (setlocale( LC_ALL, "" ))
            TRACE( "setting LANG to '%s'\n", mac_locale );
        else
        {
            /* no C library locale matching Mac locale; don't pass garbage to children */
            unsetenv("LANG");
            TRACE( "Mac locale %s is not supported by the C library\n", debugstr_a(mac_locale) );
        }
    }
#endif /* __APPLE__ */

    unix_cp = setup_unix_locales();
    if (!lcid_LC_MESSAGES) lcid_LC_MESSAGES = lcid_LC_CTYPE;

#ifdef __APPLE__
    if (!unix_cp)
        unix_cp = CP_UTF8;  /* default to utf-8 even if we don't get a valid locale */
#endif

    NtSetDefaultUILanguage( LANGIDFROMLCID(lcid_LC_MESSAGES) );
    NtSetDefaultLocale( TRUE, lcid_LC_MESSAGES );
    NtSetDefaultLocale( FALSE, lcid_LC_CTYPE );

    ansi_cp = get_lcid_codepage( LOCALE_USER_DEFAULT );
    GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&mac_cp, sizeof(mac_cp)/sizeof(WCHAR) );
    GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&oem_cp, sizeof(oem_cp)/sizeof(WCHAR) );
    if (!unix_cp)
        GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IDEFAULTUNIXCODEPAGE | LOCALE_RETURN_NUMBER,
                        (LPWSTR)&unix_cp, sizeof(unix_cp)/sizeof(WCHAR) );

    if (!(ansi_cptable = wine_cp_get_table( ansi_cp )))
        ansi_cptable = wine_cp_get_table( 1252 );
    if (!(oem_cptable = wine_cp_get_table( oem_cp )))
        oem_cptable  = wine_cp_get_table( 437 );
    if (!(mac_cptable = wine_cp_get_table( mac_cp )))
        mac_cptable  = wine_cp_get_table( 10000 );
    if (unix_cp != CP_UTF8)
    {
        if (!(unix_cptable = wine_cp_get_table( unix_cp )))
            unix_cptable  = wine_cp_get_table( 28591 );
    }

    __wine_init_codepages( ansi_cptable, oem_cptable, unix_cptable );

    TRACE( "ansi=%03d oem=%03d mac=%03d unix=%03d\n",
           ansi_cptable->info.codepage, oem_cptable->info.codepage,
           mac_cptable->info.codepage, unix_cp );

    setlocale(LC_NUMERIC, "C");  /* FIXME: oleaut32 depends on this */
}

#endif // !__REACTOS__

#ifdef __REACTOS__
HANDLE NLS_RegOpenKey(HANDLE hRootKey, LPCWSTR szKeyName)
#else
static HANDLE NLS_RegOpenKey(HANDLE hRootKey, LPCWSTR szKeyName)
#endif
{
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES attr;
    HANDLE hkey;

    RtlInitUnicodeString( &keyName, szKeyName );
#ifdef __REACTOS__
    InitializeObjectAttributes(&attr, &keyName, OBJ_CASE_INSENSITIVE, hRootKey, NULL);
#else
    InitializeObjectAttributes(&attr, &keyName, 0, hRootKey, NULL);
#endif

    if (NtOpenKey( &hkey, KEY_READ, &attr ) != STATUS_SUCCESS)
        hkey = 0;

    return hkey;
}

#ifdef __REACTOS__
BOOL NLS_RegEnumValue(HANDLE hKey, UINT ulIndex,
                      LPWSTR szValueName, ULONG valueNameSize,
                      LPWSTR szValueData, ULONG valueDataSize)
#else
static BOOL NLS_RegEnumValue(HANDLE hKey, UINT ulIndex,
                             LPWSTR szValueName, ULONG valueNameSize,
                             LPWSTR szValueData, ULONG valueDataSize)
#endif
{
    BYTE buffer[80];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    DWORD dwLen;

    if (NtEnumerateValueKey( hKey, ulIndex, KeyValueFullInformation,
        buffer, sizeof(buffer), &dwLen ) != STATUS_SUCCESS ||
        info->NameLength > valueNameSize ||
        info->DataLength > valueDataSize)
    {
        return FALSE;
    }

    TRACE("info->Name %s info->DataLength %d\n", debugstr_w(info->Name), info->DataLength);

    memcpy( szValueName, info->Name, info->NameLength);
    szValueName[info->NameLength / sizeof(WCHAR)] = '\0';
    memcpy( szValueData, buffer + info->DataOffset, info->DataLength );
    szValueData[info->DataLength / sizeof(WCHAR)] = '\0';

    TRACE("returning %s %s\n", debugstr_w(szValueName), debugstr_w(szValueData));
    return TRUE;
}

static BOOL NLS_RegGetDword(HANDLE hKey, LPCWSTR szValueName, DWORD *lpVal)
{
    BYTE buffer[128];
    const KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD dwSize = sizeof(buffer);
    UNICODE_STRING valueName;

    RtlInitUnicodeString( &valueName, szValueName );

    TRACE("%p, %s\n", hKey, debugstr_w(szValueName));
    if (NtQueryValueKey( hKey, &valueName, KeyValuePartialInformation,
                         buffer, dwSize, &dwSize ) == STATUS_SUCCESS &&
        info->DataLength == sizeof(DWORD))
    {
        memcpy(lpVal, info->Data, sizeof(DWORD));
        return TRUE;
    }

    return FALSE;
}

static BOOL NLS_GetLanguageGroupName(LGRPID lgrpid, LPWSTR szName, ULONG nameSize)
{
    LANGID  langId;
    LPCWSTR szResourceName = MAKEINTRESOURCEW(((lgrpid + 0x2000) >> 4) + 1);
    HRSRC   hResource;
    BOOL    bRet = FALSE;

    /* FIXME: Is it correct to use the system default langid? */
    langId = GetSystemDefaultLangID();

    if (SUBLANGID(langId) == SUBLANG_NEUTRAL) langId = get_default_sublang( langId );

    hResource = FindResourceExW( kernel32_handle, (LPWSTR)RT_STRING, szResourceName, langId );

    if (hResource)
    {
        HGLOBAL hResDir = LoadResource( kernel32_handle, hResource );

        if (hResDir)
        {
            ULONG   iResourceIndex = lgrpid & 0xf;
            LPCWSTR lpResEntry = LockResource( hResDir );
            ULONG   i;

            for (i = 0; i < iResourceIndex; i++)
                lpResEntry += *lpResEntry + 1;

            if (*lpResEntry < nameSize)
            {
                memcpy( szName, lpResEntry + 1, *lpResEntry * sizeof(WCHAR) );
                szName[*lpResEntry] = '\0';
                bRet = TRUE;
            }

        }
        FreeResource( hResource );
    }
    return bRet;
}

/* Callback function ptrs for EnumSystemLanguageGroupsA/W */
typedef struct
{
  LANGUAGEGROUP_ENUMPROCA procA;
  LANGUAGEGROUP_ENUMPROCW procW;
  DWORD    dwFlags;
  LONG_PTR lParam;
} ENUMLANGUAGEGROUP_CALLBACKS;

/* Internal implementation of EnumSystemLanguageGroupsA/W */
static BOOL NLS_EnumSystemLanguageGroups(ENUMLANGUAGEGROUP_CALLBACKS *lpProcs)
{
    WCHAR szNumber[10], szValue[4];
    HANDLE hKey;
    BOOL bContinue = TRUE;
    ULONG ulIndex = 0;

    if (!lpProcs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (lpProcs->dwFlags)
    {
    case 0:
        /* Default to LGRPID_INSTALLED */
        lpProcs->dwFlags = LGRPID_INSTALLED;
        /* Fall through... */
    case LGRPID_INSTALLED:
    case LGRPID_SUPPORTED:
        break;
    default:
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    hKey = NLS_RegOpenKey( 0, szLangGroupsKeyName );

    if (!hKey)
        FIXME("NLS registry key not found. Please apply the default registry file 'wine.inf'\n");

    while (bContinue)
    {
        if (NLS_RegEnumValue( hKey, ulIndex, szNumber, sizeof(szNumber),
                              szValue, sizeof(szValue) ))
        {
            BOOL bInstalled = szValue[0] == '1';
            LGRPID lgrpid = strtoulW( szNumber, NULL, 16 );

            TRACE("grpid %s (%sinstalled)\n", debugstr_w(szNumber),
                   bInstalled ? "" : "not ");

            if (lpProcs->dwFlags == LGRPID_SUPPORTED || bInstalled)
            {
                WCHAR szGrpName[48];

                if (!NLS_GetLanguageGroupName( lgrpid, szGrpName, sizeof(szGrpName) / sizeof(WCHAR) ))
                    szGrpName[0] = '\0';

                if (lpProcs->procW)
                    bContinue = lpProcs->procW( lgrpid, szNumber, szGrpName, lpProcs->dwFlags,
                                                lpProcs->lParam );
                else
                {
                    char szNumberA[sizeof(szNumber)/sizeof(WCHAR)];
                    char szGrpNameA[48];

                    /* FIXME: MSDN doesn't say which code page the W->A translation uses,
                     *        or whether the language names are ever localised. Assume CP_ACP.
                     */

                    WideCharToMultiByte(CP_ACP, 0, szNumber, -1, szNumberA, sizeof(szNumberA), 0, 0);
                    WideCharToMultiByte(CP_ACP, 0, szGrpName, -1, szGrpNameA, sizeof(szGrpNameA), 0, 0);

                    bContinue = lpProcs->procA( lgrpid, szNumberA, szGrpNameA, lpProcs->dwFlags,
                                                lpProcs->lParam );
                }
            }

            ulIndex++;
        }
        else
            bContinue = FALSE;

        if (!bContinue)
            break;
    }

    if (hKey)
        NtClose( hKey );

    return TRUE;
}

/******************************************************************************
 *           EnumSystemLanguageGroupsA    (KERNEL32.@)
 *
 * Call a users function for each language group available on the system.
 *
 * PARAMS
 *  pLangGrpEnumProc [I] Callback function to call for each language group
 *  dwFlags          [I] LGRPID_SUPPORTED=All Supported, LGRPID_INSTALLED=Installed only
 *  lParam           [I] User parameter to pass to pLangGrpEnumProc
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI EnumSystemLanguageGroupsA(LANGUAGEGROUP_ENUMPROCA pLangGrpEnumProc,
                                      DWORD dwFlags, LONG_PTR lParam)
{
    ENUMLANGUAGEGROUP_CALLBACKS procs;

    TRACE("(%p,0x%08X,0x%08lX)\n", pLangGrpEnumProc, dwFlags, lParam);

    procs.procA = pLangGrpEnumProc;
    procs.procW = NULL;
    procs.dwFlags = dwFlags;
    procs.lParam = lParam;

    return NLS_EnumSystemLanguageGroups( pLangGrpEnumProc ? &procs : NULL);
}

/******************************************************************************
 *           EnumSystemLanguageGroupsW    (KERNEL32.@)
 *
 * See EnumSystemLanguageGroupsA.
 */
BOOL WINAPI EnumSystemLanguageGroupsW(LANGUAGEGROUP_ENUMPROCW pLangGrpEnumProc,
                                      DWORD dwFlags, LONG_PTR lParam)
{
    ENUMLANGUAGEGROUP_CALLBACKS procs;

    TRACE("(%p,0x%08X,0x%08lX)\n", pLangGrpEnumProc, dwFlags, lParam);

    procs.procA = NULL;
    procs.procW = pLangGrpEnumProc;
    procs.dwFlags = dwFlags;
    procs.lParam = lParam;

    return NLS_EnumSystemLanguageGroups( pLangGrpEnumProc ? &procs : NULL);
}

/******************************************************************************
 *           IsValidLanguageGroup    (KERNEL32.@)
 *
 * Determine if a language group is supported and/or installed.
 *
 * PARAMS
 *  lgrpid  [I] Language Group Id (LGRPID_ values from "winnls.h")
 *  dwFlags [I] LGRPID_SUPPORTED=Supported, LGRPID_INSTALLED=Installed
 *
 * RETURNS
 *  TRUE, if lgrpid is supported and/or installed, according to dwFlags.
 *  FALSE otherwise.
 */
BOOL WINAPI IsValidLanguageGroup(LGRPID lgrpid, DWORD dwFlags)
{
    static const WCHAR szFormat[] = { '%','x','\0' };
    WCHAR szValueName[16], szValue[2];
    BOOL bSupported = FALSE, bInstalled = FALSE;
    HANDLE hKey;


    switch (dwFlags)
    {
    case LGRPID_INSTALLED:
    case LGRPID_SUPPORTED:

        hKey = NLS_RegOpenKey( 0, szLangGroupsKeyName );

        sprintfW( szValueName, szFormat, lgrpid );

        if (NLS_RegGetDword( hKey, szValueName, (LPDWORD)szValue ))
        {
            bSupported = TRUE;

            if (szValue[0] == '1')
                bInstalled = TRUE;
        }

        if (hKey)
            NtClose( hKey );

        break;
    }

    if ((dwFlags == LGRPID_SUPPORTED && bSupported) ||
        (dwFlags == LGRPID_INSTALLED && bInstalled))
        return TRUE;

    return FALSE;
}

/* Callback function ptrs for EnumLanguageGrouplocalesA/W */
typedef struct
{
  LANGGROUPLOCALE_ENUMPROCA procA;
  LANGGROUPLOCALE_ENUMPROCW procW;
  DWORD    dwFlags;
  LGRPID   lgrpid;
  LONG_PTR lParam;
} ENUMLANGUAGEGROUPLOCALE_CALLBACKS;

/* Internal implementation of EnumLanguageGrouplocalesA/W */
static BOOL NLS_EnumLanguageGroupLocales(ENUMLANGUAGEGROUPLOCALE_CALLBACKS *lpProcs)
{
    static const WCHAR szAlternateSortsKeyName[] = {
      'A','l','t','e','r','n','a','t','e',' ','S','o','r','t','s','\0'
    };
    WCHAR szNumber[10], szValue[4];
    HANDLE hKey;
    BOOL bContinue = TRUE, bAlternate = FALSE;
    LGRPID lgrpid;
    ULONG ulIndex = 1;  /* Ignore default entry of 1st key */

    if (!lpProcs || !lpProcs->lgrpid || lpProcs->lgrpid > LGRPID_ARMENIAN)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpProcs->dwFlags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    hKey = NLS_RegOpenKey( 0, szLocaleKeyName );

    if (!hKey)
        WARN("NLS registry key not found. Please apply the default registry file 'wine.inf'\n");

    while (bContinue)
    {
        if (NLS_RegEnumValue( hKey, ulIndex, szNumber, sizeof(szNumber),
                              szValue, sizeof(szValue) ))
        {
            lgrpid = strtoulW( szValue, NULL, 16 );

            TRACE("lcid %s, grpid %d (%smatched)\n", debugstr_w(szNumber),
                   lgrpid, lgrpid == lpProcs->lgrpid ? "" : "not ");

            if (lgrpid == lpProcs->lgrpid)
            {
                LCID lcid;

                lcid = strtoulW( szNumber, NULL, 16 );

                /* FIXME: native returns extra text for a few (17/150) locales, e.g:
                 * '00000437          ;Georgian'
                 * At present we only pass the LCID string.
                 */

                if (lpProcs->procW)
                    bContinue = lpProcs->procW( lgrpid, lcid, szNumber, lpProcs->lParam );
                else
                {
                    char szNumberA[sizeof(szNumber)/sizeof(WCHAR)];

                    WideCharToMultiByte(CP_ACP, 0, szNumber, -1, szNumberA, sizeof(szNumberA), 0, 0);

                    bContinue = lpProcs->procA( lgrpid, lcid, szNumberA, lpProcs->lParam );
                }
            }

            ulIndex++;
        }
        else
        {
            /* Finished enumerating this key */
            if (!bAlternate)
            {
                /* Enumerate alternate sorts also */
                hKey = NLS_RegOpenKey( hKey, szAlternateSortsKeyName );
                bAlternate = TRUE;
                ulIndex = 0;
            }
            else
                bContinue = FALSE; /* Finished both keys */
        }

        if (!bContinue)
            break;
    }

    if (hKey)
        NtClose( hKey );

    return TRUE;
}

/******************************************************************************
 *           EnumLanguageGroupLocalesA    (KERNEL32.@)
 *
 * Call a users function for every locale in a language group available on the system.
 *
 * PARAMS
 *  pLangGrpLcEnumProc [I] Callback function to call for each locale
 *  lgrpid             [I] Language group (LGRPID_ values from "winnls.h")
 *  dwFlags            [I] Reserved, set to 0
 *  lParam             [I] User parameter to pass to pLangGrpLcEnumProc
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI EnumLanguageGroupLocalesA(LANGGROUPLOCALE_ENUMPROCA pLangGrpLcEnumProc,
                                      LGRPID lgrpid, DWORD dwFlags, LONG_PTR lParam)
{
    ENUMLANGUAGEGROUPLOCALE_CALLBACKS callbacks;

    TRACE("(%p,0x%08X,0x%08X,0x%08lX)\n", pLangGrpLcEnumProc, lgrpid, dwFlags, lParam);

    callbacks.procA   = pLangGrpLcEnumProc;
    callbacks.procW   = NULL;
    callbacks.dwFlags = dwFlags;
    callbacks.lgrpid  = lgrpid;
    callbacks.lParam  = lParam;

    return NLS_EnumLanguageGroupLocales( pLangGrpLcEnumProc ? &callbacks : NULL );
}

/******************************************************************************
 *           EnumLanguageGroupLocalesW    (KERNEL32.@)
 *
 * See EnumLanguageGroupLocalesA.
 */
BOOL WINAPI EnumLanguageGroupLocalesW(LANGGROUPLOCALE_ENUMPROCW pLangGrpLcEnumProc,
                                      LGRPID lgrpid, DWORD dwFlags, LONG_PTR lParam)
{
    ENUMLANGUAGEGROUPLOCALE_CALLBACKS callbacks;

    TRACE("(%p,0x%08X,0x%08X,0x%08lX)\n", pLangGrpLcEnumProc, lgrpid, dwFlags, lParam);

    callbacks.procA   = NULL;
    callbacks.procW   = pLangGrpLcEnumProc;
    callbacks.dwFlags = dwFlags;
    callbacks.lgrpid  = lgrpid;
    callbacks.lParam  = lParam;

    return NLS_EnumLanguageGroupLocales( pLangGrpLcEnumProc ? &callbacks : NULL );
}

/******************************************************************************
 *           InvalidateNLSCache           (KERNEL32.@)
 *
 * Invalidate the cache of NLS values.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI InvalidateNLSCache(void)
{
#ifdef __REACTOS__
    JapaneseEra_ClearCache();
    return TRUE;
#else
  FIXME("() stub\n");
  return FALSE;
#endif
}

/******************************************************************************
 *           GetUserGeoID (KERNEL32.@)
 */
GEOID WINAPI GetUserGeoID( GEOCLASS GeoClass )
{
    GEOID ret = GEOID_NOT_AVAILABLE;
    static const WCHAR geoW[] = {'G','e','o',0};
    static const WCHAR nationW[] = {'N','a','t','i','o','n',0};
    WCHAR bufferW[40], *end;
    DWORD count;
    HANDLE hkey, hSubkey = 0;
    UNICODE_STRING keyW;
    const KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)bufferW;
    RtlInitUnicodeString( &keyW, nationW );
    count = sizeof(bufferW);

    if(!(hkey = create_registry_key())) return ret;

    switch( GeoClass ){
    case GEOCLASS_NATION:
        if ((hSubkey = NLS_RegOpenKey(hkey, geoW)))
        {
            if((NtQueryValueKey(hSubkey, &keyW, KeyValuePartialInformation,
                                bufferW, count, &count) == STATUS_SUCCESS ) && info->DataLength)
                ret = strtolW((LPCWSTR)info->Data, &end, 10);
        }
        break;
    case GEOCLASS_REGION:
        FIXME("GEOCLASS_REGION not handled yet\n");
        break;
    }

    NtClose(hkey);
    if (hSubkey) NtClose(hSubkey);
    return ret;
}

/******************************************************************************
 *           SetUserGeoID (KERNEL32.@)
 */
BOOL WINAPI SetUserGeoID( GEOID GeoID )
{
    static const WCHAR geoW[] = {'G','e','o',0};
    static const WCHAR nationW[] = {'N','a','t','i','o','n',0};
    static const WCHAR formatW[] = {'%','i',0};
    UNICODE_STRING nameW,keyW;
    WCHAR bufferW[10];
    OBJECT_ATTRIBUTES attr;
    HANDLE hkey;

    if(!(hkey = create_registry_key())) return FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, geoW );
    RtlInitUnicodeString( &keyW, nationW );

    if (NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)

    {
        NtClose(attr.RootDirectory);
        return FALSE;
    }

    sprintfW(bufferW, formatW, GeoID);
    NtSetValueKey(hkey, &keyW, 0, REG_SZ, bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR));
    NtClose(attr.RootDirectory);
    NtClose(hkey);
    return TRUE;
}

typedef struct
{
    union
    {
        UILANGUAGE_ENUMPROCA procA;
        UILANGUAGE_ENUMPROCW procW;
    } u;
    DWORD flags;
    LONG_PTR param;
} ENUM_UILANG_CALLBACK;

static BOOL CALLBACK enum_uilang_proc_a( HMODULE hModule, LPCSTR type,
                                         LPCSTR name, WORD LangID, LONG_PTR lParam )
{
    ENUM_UILANG_CALLBACK *enum_uilang = (ENUM_UILANG_CALLBACK *)lParam;
    char buf[20];

    sprintf(buf, "%08x", (UINT)LangID);
    return enum_uilang->u.procA( buf, enum_uilang->param );
}

static BOOL CALLBACK enum_uilang_proc_w( HMODULE hModule, LPCWSTR type,
                                         LPCWSTR name, WORD LangID, LONG_PTR lParam )
{
    static const WCHAR formatW[] = {'%','0','8','x',0};
    ENUM_UILANG_CALLBACK *enum_uilang = (ENUM_UILANG_CALLBACK *)lParam;
    WCHAR buf[20];

    sprintfW( buf, formatW, (UINT)LangID );
    return enum_uilang->u.procW( buf, enum_uilang->param );
}

/******************************************************************************
 *           EnumUILanguagesA (KERNEL32.@)
 */
BOOL WINAPI EnumUILanguagesA(UILANGUAGE_ENUMPROCA pUILangEnumProc, DWORD dwFlags, LONG_PTR lParam)
{
    ENUM_UILANG_CALLBACK enum_uilang;

    TRACE("%p, %x, %lx\n", pUILangEnumProc, dwFlags, lParam);

    if(!pUILangEnumProc) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(dwFlags) {
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    enum_uilang.u.procA = pUILangEnumProc;
    enum_uilang.flags = dwFlags;
    enum_uilang.param = lParam;

    EnumResourceLanguagesA( kernel32_handle, (LPCSTR)RT_STRING,
                            (LPCSTR)LOCALE_ILANGUAGE, enum_uilang_proc_a,
                            (LONG_PTR)&enum_uilang);
    return TRUE;
}

/******************************************************************************
 *           EnumUILanguagesW (KERNEL32.@)
 */
BOOL WINAPI EnumUILanguagesW(UILANGUAGE_ENUMPROCW pUILangEnumProc, DWORD dwFlags, LONG_PTR lParam)
{
    ENUM_UILANG_CALLBACK enum_uilang;

    TRACE("%p, %x, %lx\n", pUILangEnumProc, dwFlags, lParam);


    if(!pUILangEnumProc) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(dwFlags) {
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    enum_uilang.u.procW = pUILangEnumProc;
    enum_uilang.flags = dwFlags;
    enum_uilang.param = lParam;

    EnumResourceLanguagesW( kernel32_handle, (LPCWSTR)RT_STRING,
                            (LPCWSTR)LOCALE_ILANGUAGE, enum_uilang_proc_w,
                            (LONG_PTR)&enum_uilang);
    return TRUE;
}

enum locationkind {
    LOCATION_NATION = 0,
    LOCATION_REGION,
    LOCATION_BOTH
};

struct geoinfo_t {
    GEOID id;
    WCHAR iso2W[3];
    WCHAR iso3W[4];
    GEOID parent;
    INT   uncode;
    enum locationkind kind;
};

static const struct geoinfo_t geoinfodata[] = {
    { 2, {'A','G',0}, {'A','T','G',0}, 10039880,  28 }, /* Antigua and Barbuda */
    { 3, {'A','F',0}, {'A','F','G',0}, 47614,   4 }, /* Afghanistan */
    { 4, {'D','Z',0}, {'D','Z','A',0}, 42487,  12 }, /* Algeria */
    { 5, {'A','Z',0}, {'A','Z','E',0}, 47611,  31 }, /* Azerbaijan */
    { 6, {'A','L',0}, {'A','L','B',0}, 47610,   8 }, /* Albania */
    { 7, {'A','M',0}, {'A','R','M',0}, 47611,  51 }, /* Armenia */
    { 8, {'A','D',0}, {'A','N','D',0}, 47610,  20 }, /* Andorra */
    { 9, {'A','O',0}, {'A','G','O',0}, 42484,  24 }, /* Angola */
    { 10, {'A','S',0}, {'A','S','M',0}, 26286,  16 }, /* American Samoa */
    { 11, {'A','R',0}, {'A','R','G',0}, 31396,  32 }, /* Argentina */
    { 12, {'A','U',0}, {'A','U','S',0}, 10210825,  36 }, /* Australia */
    { 14, {'A','T',0}, {'A','U','T',0}, 10210824,  40 }, /* Austria */
    { 17, {'B','H',0}, {'B','H','R',0}, 47611,  48 }, /* Bahrain */
    { 18, {'B','B',0}, {'B','R','B',0}, 10039880,  52 }, /* Barbados */
    { 19, {'B','W',0}, {'B','W','A',0}, 10039883,  72 }, /* Botswana */
    { 20, {'B','M',0}, {'B','M','U',0}, 23581,  60 }, /* Bermuda */
    { 21, {'B','E',0}, {'B','E','L',0}, 10210824,  56 }, /* Belgium */
    { 22, {'B','S',0}, {'B','H','S',0}, 10039880,  44 }, /* Bahamas, The */
    { 23, {'B','D',0}, {'B','G','D',0}, 47614,  50 }, /* Bangladesh */
    { 24, {'B','Z',0}, {'B','L','Z',0}, 27082,  84 }, /* Belize */
    { 25, {'B','A',0}, {'B','I','H',0}, 47610,  70 }, /* Bosnia and Herzegovina */
    { 26, {'B','O',0}, {'B','O','L',0}, 31396,  68 }, /* Bolivia */
    { 27, {'M','M',0}, {'M','M','R',0}, 47599, 104 }, /* Myanmar */
    { 28, {'B','J',0}, {'B','E','N',0}, 42483, 204 }, /* Benin */
    { 29, {'B','Y',0}, {'B','L','R',0}, 47609, 112 }, /* Belarus */
    { 30, {'S','B',0}, {'S','L','B',0}, 20900,  90 }, /* Solomon Islands */
    { 32, {'B','R',0}, {'B','R','A',0}, 31396,  76 }, /* Brazil */
    { 34, {'B','T',0}, {'B','T','N',0}, 47614,  64 }, /* Bhutan */
    { 35, {'B','G',0}, {'B','G','R',0}, 47609, 100 }, /* Bulgaria */
    { 37, {'B','N',0}, {'B','R','N',0}, 47599,  96 }, /* Brunei */
    { 38, {'B','I',0}, {'B','D','I',0}, 47603, 108 }, /* Burundi */
    { 39, {'C','A',0}, {'C','A','N',0}, 23581, 124 }, /* Canada */
    { 40, {'K','H',0}, {'K','H','M',0}, 47599, 116 }, /* Cambodia */
    { 41, {'T','D',0}, {'T','C','D',0}, 42484, 148 }, /* Chad */
    { 42, {'L','K',0}, {'L','K','A',0}, 47614, 144 }, /* Sri Lanka */
    { 43, {'C','G',0}, {'C','O','G',0}, 42484, 178 }, /* Congo */
    { 44, {'C','D',0}, {'C','O','D',0}, 42484, 180 }, /* Congo (DRC) */
    { 45, {'C','N',0}, {'C','H','N',0}, 47600, 156 }, /* China */
    { 46, {'C','L',0}, {'C','H','L',0}, 31396, 152 }, /* Chile */
    { 49, {'C','M',0}, {'C','M','R',0}, 42484, 120 }, /* Cameroon */
    { 50, {'K','M',0}, {'C','O','M',0}, 47603, 174 }, /* Comoros */
    { 51, {'C','O',0}, {'C','O','L',0}, 31396, 170 }, /* Colombia */
    { 54, {'C','R',0}, {'C','R','I',0}, 27082, 188 }, /* Costa Rica */
    { 55, {'C','F',0}, {'C','A','F',0}, 42484, 140 }, /* Central African Republic */
    { 56, {'C','U',0}, {'C','U','B',0}, 10039880, 192 }, /* Cuba */
    { 57, {'C','V',0}, {'C','P','V',0}, 42483, 132 }, /* Cape Verde */
    { 59, {'C','Y',0}, {'C','Y','P',0}, 47611, 196 }, /* Cyprus */
    { 61, {'D','K',0}, {'D','N','K',0}, 10039882, 208 }, /* Denmark */
    { 62, {'D','J',0}, {'D','J','I',0}, 47603, 262 }, /* Djibouti */
    { 63, {'D','M',0}, {'D','M','A',0}, 10039880, 212 }, /* Dominica */
    { 65, {'D','O',0}, {'D','O','M',0}, 10039880, 214 }, /* Dominican Republic */
    { 66, {'E','C',0}, {'E','C','U',0}, 31396, 218 }, /* Ecuador */
    { 67, {'E','G',0}, {'E','G','Y',0}, 42487, 818 }, /* Egypt */
    { 68, {'I','E',0}, {'I','R','L',0}, 10039882, 372 }, /* Ireland */
    { 69, {'G','Q',0}, {'G','N','Q',0}, 42484, 226 }, /* Equatorial Guinea */
    { 70, {'E','E',0}, {'E','S','T',0}, 10039882, 233 }, /* Estonia */
    { 71, {'E','R',0}, {'E','R','I',0}, 47603, 232 }, /* Eritrea */
    { 72, {'S','V',0}, {'S','L','V',0}, 27082, 222 }, /* El Salvador */
    { 73, {'E','T',0}, {'E','T','H',0}, 47603, 231 }, /* Ethiopia */
    { 75, {'C','Z',0}, {'C','Z','E',0}, 47609, 203 }, /* Czech Republic */
    { 77, {'F','I',0}, {'F','I','N',0}, 10039882, 246 }, /* Finland */
    { 78, {'F','J',0}, {'F','J','I',0}, 20900, 242 }, /* Fiji Islands */
    { 80, {'F','M',0}, {'F','S','M',0}, 21206, 583 }, /* Micronesia */
    { 81, {'F','O',0}, {'F','R','O',0}, 10039882, 234 }, /* Faroe Islands */
    { 84, {'F','R',0}, {'F','R','A',0}, 10210824, 250 }, /* France */
    { 86, {'G','M',0}, {'G','M','B',0}, 42483, 270 }, /* Gambia, The */
    { 87, {'G','A',0}, {'G','A','B',0}, 42484, 266 }, /* Gabon */
    { 88, {'G','E',0}, {'G','E','O',0}, 47611, 268 }, /* Georgia */
    { 89, {'G','H',0}, {'G','H','A',0}, 42483, 288 }, /* Ghana */
    { 90, {'G','I',0}, {'G','I','B',0}, 47610, 292 }, /* Gibraltar */
    { 91, {'G','D',0}, {'G','R','D',0}, 10039880, 308 }, /* Grenada */
    { 93, {'G','L',0}, {'G','R','L',0}, 23581, 304 }, /* Greenland */
    { 94, {'D','E',0}, {'D','E','U',0}, 10210824, 276 }, /* Germany */
    { 98, {'G','R',0}, {'G','R','C',0}, 47610, 300 }, /* Greece */
    { 99, {'G','T',0}, {'G','T','M',0}, 27082, 320 }, /* Guatemala */
    { 100, {'G','N',0}, {'G','I','N',0}, 42483, 324 }, /* Guinea */
    { 101, {'G','Y',0}, {'G','U','Y',0}, 31396, 328 }, /* Guyana */
    { 103, {'H','T',0}, {'H','T','I',0}, 10039880, 332 }, /* Haiti */
    { 104, {'H','K',0}, {'H','K','G',0}, 47600, 344 }, /* Hong Kong S.A.R. */
    { 106, {'H','N',0}, {'H','N','D',0}, 27082, 340 }, /* Honduras */
    { 108, {'H','R',0}, {'H','R','V',0}, 47610, 191 }, /* Croatia */
    { 109, {'H','U',0}, {'H','U','N',0}, 47609, 348 }, /* Hungary */
    { 110, {'I','S',0}, {'I','S','L',0}, 10039882, 352 }, /* Iceland */
    { 111, {'I','D',0}, {'I','D','N',0}, 47599, 360 }, /* Indonesia */
    { 113, {'I','N',0}, {'I','N','D',0}, 47614, 356 }, /* India */
    { 114, {'I','O',0}, {'I','O','T',0}, 39070,  86 }, /* British Indian Ocean Territory */
    { 116, {'I','R',0}, {'I','R','N',0}, 47614, 364 }, /* Iran */
    { 117, {'I','L',0}, {'I','S','R',0}, 47611, 376 }, /* Israel */
    { 118, {'I','T',0}, {'I','T','A',0}, 47610, 380 }, /* Italy */
    { 119, {'C','I',0}, {'C','I','V',0}, 42483, 384 }, /* Côte d'Ivoire */
    { 121, {'I','Q',0}, {'I','R','Q',0}, 47611, 368 }, /* Iraq */
    { 122, {'J','P',0}, {'J','P','N',0}, 47600, 392 }, /* Japan */
    { 124, {'J','M',0}, {'J','A','M',0}, 10039880, 388 }, /* Jamaica */
    { 125, {'S','J',0}, {'S','J','M',0}, 10039882, 744 }, /* Jan Mayen */
    { 126, {'J','O',0}, {'J','O','R',0}, 47611, 400 }, /* Jordan */
    { 127, {'X','X',0}, {'X','X',0}, 161832256 }, /* Johnston Atoll */
    { 129, {'K','E',0}, {'K','E','N',0}, 47603, 404 }, /* Kenya */
    { 130, {'K','G',0}, {'K','G','Z',0}, 47590, 417 }, /* Kyrgyzstan */
    { 131, {'K','P',0}, {'P','R','K',0}, 47600, 408 }, /* North Korea */
    { 133, {'K','I',0}, {'K','I','R',0}, 21206, 296 }, /* Kiribati */
    { 134, {'K','R',0}, {'K','O','R',0}, 47600, 410 }, /* Korea */
    { 136, {'K','W',0}, {'K','W','T',0}, 47611, 414 }, /* Kuwait */
    { 137, {'K','Z',0}, {'K','A','Z',0}, 47590, 398 }, /* Kazakhstan */
    { 138, {'L','A',0}, {'L','A','O',0}, 47599, 418 }, /* Laos */
    { 139, {'L','B',0}, {'L','B','N',0}, 47611, 422 }, /* Lebanon */
    { 140, {'L','V',0}, {'L','V','A',0}, 10039882, 428 }, /* Latvia */
    { 141, {'L','T',0}, {'L','T','U',0}, 10039882, 440 }, /* Lithuania */
    { 142, {'L','R',0}, {'L','B','R',0}, 42483, 430 }, /* Liberia */
    { 143, {'S','K',0}, {'S','V','K',0}, 47609, 703 }, /* Slovakia */
    { 145, {'L','I',0}, {'L','I','E',0}, 10210824, 438 }, /* Liechtenstein */
    { 146, {'L','S',0}, {'L','S','O',0}, 10039883, 426 }, /* Lesotho */
    { 147, {'L','U',0}, {'L','U','X',0}, 10210824, 442 }, /* Luxembourg */
    { 148, {'L','Y',0}, {'L','B','Y',0}, 42487, 434 }, /* Libya */
    { 149, {'M','G',0}, {'M','D','G',0}, 47603, 450 }, /* Madagascar */
    { 151, {'M','O',0}, {'M','A','C',0}, 47600, 446 }, /* Macao S.A.R. */
    { 152, {'M','D',0}, {'M','D','A',0}, 47609, 498 }, /* Moldova */
    { 154, {'M','N',0}, {'M','N','G',0}, 47600, 496 }, /* Mongolia */
    { 156, {'M','W',0}, {'M','W','I',0}, 47603, 454 }, /* Malawi */
    { 157, {'M','L',0}, {'M','L','I',0}, 42483, 466 }, /* Mali */
    { 158, {'M','C',0}, {'M','C','O',0}, 10210824, 492 }, /* Monaco */
    { 159, {'M','A',0}, {'M','A','R',0}, 42487, 504 }, /* Morocco */
    { 160, {'M','U',0}, {'M','U','S',0}, 47603, 480 }, /* Mauritius */
    { 162, {'M','R',0}, {'M','R','T',0}, 42483, 478 }, /* Mauritania */
    { 163, {'M','T',0}, {'M','L','T',0}, 47610, 470 }, /* Malta */
    { 164, {'O','M',0}, {'O','M','N',0}, 47611, 512 }, /* Oman */
    { 165, {'M','V',0}, {'M','D','V',0}, 47614, 462 }, /* Maldives */
    { 166, {'M','X',0}, {'M','E','X',0}, 27082, 484 }, /* Mexico */
    { 167, {'M','Y',0}, {'M','Y','S',0}, 47599, 458 }, /* Malaysia */
    { 168, {'M','Z',0}, {'M','O','Z',0}, 47603, 508 }, /* Mozambique */
    { 173, {'N','E',0}, {'N','E','R',0}, 42483, 562 }, /* Niger */
    { 174, {'V','U',0}, {'V','U','T',0}, 20900, 548 }, /* Vanuatu */
    { 175, {'N','G',0}, {'N','G','A',0}, 42483, 566 }, /* Nigeria */
    { 176, {'N','L',0}, {'N','L','D',0}, 10210824, 528 }, /* Netherlands */
    { 177, {'N','O',0}, {'N','O','R',0}, 10039882, 578 }, /* Norway */
    { 178, {'N','P',0}, {'N','P','L',0}, 47614, 524 }, /* Nepal */
    { 180, {'N','R',0}, {'N','R','U',0}, 21206, 520 }, /* Nauru */
    { 181, {'S','R',0}, {'S','U','R',0}, 31396, 740 }, /* Suriname */
    { 182, {'N','I',0}, {'N','I','C',0}, 27082, 558 }, /* Nicaragua */
    { 183, {'N','Z',0}, {'N','Z','L',0}, 10210825, 554 }, /* New Zealand */
    { 184, {'P','S',0}, {'P','S','E',0}, 47611, 275 }, /* Palestinian Authority */
    { 185, {'P','Y',0}, {'P','R','Y',0}, 31396, 600 }, /* Paraguay */
    { 187, {'P','E',0}, {'P','E','R',0}, 31396, 604 }, /* Peru */
    { 190, {'P','K',0}, {'P','A','K',0}, 47614, 586 }, /* Pakistan */
    { 191, {'P','L',0}, {'P','O','L',0}, 47609, 616 }, /* Poland */
    { 192, {'P','A',0}, {'P','A','N',0}, 27082, 591 }, /* Panama */
    { 193, {'P','T',0}, {'P','R','T',0}, 47610, 620 }, /* Portugal */
    { 194, {'P','G',0}, {'P','N','G',0}, 20900, 598 }, /* Papua New Guinea */
    { 195, {'P','W',0}, {'P','L','W',0}, 21206, 585 }, /* Palau */
    { 196, {'G','W',0}, {'G','N','B',0}, 42483, 624 }, /* Guinea-Bissau */
    { 197, {'Q','A',0}, {'Q','A','T',0}, 47611, 634 }, /* Qatar */
    { 198, {'R','E',0}, {'R','E','U',0}, 47603, 638 }, /* Reunion */
    { 199, {'M','H',0}, {'M','H','L',0}, 21206, 584 }, /* Marshall Islands */
    { 200, {'R','O',0}, {'R','O','U',0}, 47609, 642 }, /* Romania */
    { 201, {'P','H',0}, {'P','H','L',0}, 47599, 608 }, /* Philippines */
    { 202, {'P','R',0}, {'P','R','I',0}, 10039880, 630 }, /* Puerto Rico */
    { 203, {'R','U',0}, {'R','U','S',0}, 47609, 643 }, /* Russia */
    { 204, {'R','W',0}, {'R','W','A',0}, 47603, 646 }, /* Rwanda */
    { 205, {'S','A',0}, {'S','A','U',0}, 47611, 682 }, /* Saudi Arabia */
    { 206, {'P','M',0}, {'S','P','M',0}, 23581, 666 }, /* St. Pierre and Miquelon */
    { 207, {'K','N',0}, {'K','N','A',0}, 10039880, 659 }, /* St. Kitts and Nevis */
    { 208, {'S','C',0}, {'S','Y','C',0}, 47603, 690 }, /* Seychelles */
    { 209, {'Z','A',0}, {'Z','A','F',0}, 10039883, 710 }, /* South Africa */
    { 210, {'S','N',0}, {'S','E','N',0}, 42483, 686 }, /* Senegal */
    { 212, {'S','I',0}, {'S','V','N',0}, 47610, 705 }, /* Slovenia */
    { 213, {'S','L',0}, {'S','L','E',0}, 42483, 694 }, /* Sierra Leone */
    { 214, {'S','M',0}, {'S','M','R',0}, 47610, 674 }, /* San Marino */
    { 215, {'S','G',0}, {'S','G','P',0}, 47599, 702 }, /* Singapore */
    { 216, {'S','O',0}, {'S','O','M',0}, 47603, 706 }, /* Somalia */
    { 217, {'E','S',0}, {'E','S','P',0}, 47610, 724 }, /* Spain */
    { 218, {'L','C',0}, {'L','C','A',0}, 10039880, 662 }, /* St. Lucia */
    { 219, {'S','D',0}, {'S','D','N',0}, 42487, 736 }, /* Sudan */
    { 220, {'S','J',0}, {'S','J','M',0}, 10039882, 744 }, /* Svalbard */
    { 221, {'S','E',0}, {'S','W','E',0}, 10039882, 752 }, /* Sweden */
    { 222, {'S','Y',0}, {'S','Y','R',0}, 47611, 760 }, /* Syria */
    { 223, {'C','H',0}, {'C','H','E',0}, 10210824, 756 }, /* Switzerland */
    { 224, {'A','E',0}, {'A','R','E',0}, 47611, 784 }, /* United Arab Emirates */
    { 225, {'T','T',0}, {'T','T','O',0}, 10039880, 780 }, /* Trinidad and Tobago */
    { 227, {'T','H',0}, {'T','H','A',0}, 47599, 764 }, /* Thailand */
    { 228, {'T','J',0}, {'T','J','K',0}, 47590, 762 }, /* Tajikistan */
    { 231, {'T','O',0}, {'T','O','N',0}, 26286, 776 }, /* Tonga */
    { 232, {'T','G',0}, {'T','G','O',0}, 42483, 768 }, /* Togo */
    { 233, {'S','T',0}, {'S','T','P',0}, 42484, 678 }, /* São Tomé and Príncipe */
    { 234, {'T','N',0}, {'T','U','N',0}, 42487, 788 }, /* Tunisia */
    { 235, {'T','R',0}, {'T','U','R',0}, 47611, 792 }, /* Turkey */
    { 236, {'T','V',0}, {'T','U','V',0}, 26286, 798 }, /* Tuvalu */
    { 237, {'T','W',0}, {'T','W','N',0}, 47600, 158 }, /* Taiwan */
    { 238, {'T','M',0}, {'T','K','M',0}, 47590, 795 }, /* Turkmenistan */
    { 239, {'T','Z',0}, {'T','Z','A',0}, 47603, 834 }, /* Tanzania */
    { 240, {'U','G',0}, {'U','G','A',0}, 47603, 800 }, /* Uganda */
    { 241, {'U','A',0}, {'U','K','R',0}, 47609, 804 }, /* Ukraine */
    { 242, {'G','B',0}, {'G','B','R',0}, 10039882, 826 }, /* United Kingdom */
    { 244, {'U','S',0}, {'U','S','A',0}, 23581, 840 }, /* United States */
    { 245, {'B','F',0}, {'B','F','A',0}, 42483, 854 }, /* Burkina Faso */
    { 246, {'U','Y',0}, {'U','R','Y',0}, 31396, 858 }, /* Uruguay */
    { 247, {'U','Z',0}, {'U','Z','B',0}, 47590, 860 }, /* Uzbekistan */
    { 248, {'V','C',0}, {'V','C','T',0}, 10039880, 670 }, /* St. Vincent and the Grenadines */
    { 249, {'V','E',0}, {'V','E','N',0}, 31396, 862 }, /* Bolivarian Republic of Venezuela */
    { 251, {'V','N',0}, {'V','N','M',0}, 47599, 704 }, /* Vietnam */
    { 252, {'V','I',0}, {'V','I','R',0}, 10039880, 850 }, /* Virgin Islands */
    { 253, {'V','A',0}, {'V','A','T',0}, 47610, 336 }, /* Vatican City */
    { 254, {'N','A',0}, {'N','A','M',0}, 10039883, 516 }, /* Namibia */
    { 257, {'E','H',0}, {'E','S','H',0}, 42487, 732 }, /* Western Sahara (disputed) */
    { 258, {'X','X',0}, {'X','X',0}, 161832256 }, /* Wake Island */
    { 259, {'W','S',0}, {'W','S','M',0}, 26286, 882 }, /* Samoa */
    { 260, {'S','Z',0}, {'S','W','Z',0}, 10039883, 748 }, /* Swaziland */
    { 261, {'Y','E',0}, {'Y','E','M',0}, 47611, 887 }, /* Yemen */
    { 263, {'Z','M',0}, {'Z','M','B',0}, 47603, 894 }, /* Zambia */
    { 264, {'Z','W',0}, {'Z','W','E',0}, 47603, 716 }, /* Zimbabwe */
    { 269, {'C','S',0}, {'S','C','G',0}, 47610, 891 }, /* Serbia and Montenegro (Former) */
    { 270, {'M','E',0}, {'M','N','E',0}, 47610, 499 }, /* Montenegro */
    { 271, {'R','S',0}, {'S','R','B',0}, 47610, 688 }, /* Serbia */
    { 273, {'C','W',0}, {'C','U','W',0}, 10039880, 531 }, /* Curaçao */
    { 276, {'S','S',0}, {'S','S','D',0}, 42487, 728 }, /* South Sudan */
    { 300, {'A','I',0}, {'A','I','A',0}, 10039880, 660 }, /* Anguilla */
    { 301, {'A','Q',0}, {'A','T','A',0}, 39070,  10 }, /* Antarctica */
    { 302, {'A','W',0}, {'A','B','W',0}, 10039880, 533 }, /* Aruba */
    { 303, {'X','X',0}, {'X','X',0}, 39070 }, /* Ascension Island */
    { 304, {'X','X',0}, {'X','X',0}, 10210825 }, /* Ashmore and Cartier Islands */
    { 305, {'X','X',0}, {'X','X',0}, 161832256 }, /* Baker Island */
    { 306, {'B','V',0}, {'B','V','T',0}, 39070,  74 }, /* Bouvet Island */
    { 307, {'K','Y',0}, {'C','Y','M',0}, 10039880, 136 }, /* Cayman Islands */
    { 308, {'X','X',0}, {'X','X',0}, 10210824, 0, LOCATION_BOTH }, /* Channel Islands */
    { 309, {'C','X',0}, {'C','X','R',0}, 12, 162 }, /* Christmas Island */
    { 310, {'X','X',0}, {'X','X',0}, 27114 }, /* Clipperton Island */
    { 311, {'C','C',0}, {'C','C','K',0}, 10210825, 166 }, /* Cocos (Keeling) Islands */
    { 312, {'C','K',0}, {'C','O','K',0}, 26286, 184 }, /* Cook Islands */
    { 313, {'X','X',0}, {'X','X',0}, 10210825 }, /* Coral Sea Islands */
    { 314, {'X','X',0}, {'X','X',0}, 114 }, /* Diego Garcia */
    { 315, {'F','K',0}, {'F','L','K',0}, 31396, 238 }, /* Falkland Islands (Islas Malvinas) */
    { 317, {'G','F',0}, {'G','U','F',0}, 31396, 254 }, /* French Guiana */
    { 318, {'P','F',0}, {'P','Y','F',0}, 26286, 258 }, /* French Polynesia */
    { 319, {'T','F',0}, {'A','T','F',0}, 39070, 260 }, /* French Southern and Antarctic Lands */
    { 321, {'G','P',0}, {'G','L','P',0}, 10039880, 312 }, /* Guadeloupe */
    { 322, {'G','U',0}, {'G','U','M',0}, 21206, 316 }, /* Guam */
    { 323, {'X','X',0}, {'X','X',0}, 39070 }, /* Guantanamo Bay */
    { 324, {'G','G',0}, {'G','G','Y',0}, 308, 831 }, /* Guernsey */
    { 325, {'H','M',0}, {'H','M','D',0}, 39070, 334 }, /* Heard Island and McDonald Islands */
    { 326, {'X','X',0}, {'X','X',0}, 161832256 }, /* Howland Island */
    { 327, {'X','X',0}, {'X','X',0}, 161832256 }, /* Jarvis Island */
    { 328, {'J','E',0}, {'J','E','Y',0}, 308, 832 }, /* Jersey */
    { 329, {'X','X',0}, {'X','X',0}, 161832256 }, /* Kingman Reef */
    { 330, {'M','Q',0}, {'M','T','Q',0}, 10039880, 474 }, /* Martinique */
    { 331, {'Y','T',0}, {'M','Y','T',0}, 47603, 175 }, /* Mayotte */
    { 332, {'M','S',0}, {'M','S','R',0}, 10039880, 500 }, /* Montserrat */
    { 333, {'A','N',0}, {'A','N','T',0}, 10039880, 530, LOCATION_BOTH }, /* Netherlands Antilles (Former) */
    { 334, {'N','C',0}, {'N','C','L',0}, 20900, 540 }, /* New Caledonia */
    { 335, {'N','U',0}, {'N','I','U',0}, 26286, 570 }, /* Niue */
    { 336, {'N','F',0}, {'N','F','K',0}, 10210825, 574 }, /* Norfolk Island */
    { 337, {'M','P',0}, {'M','N','P',0}, 21206, 580 }, /* Northern Mariana Islands */
    { 338, {'X','X',0}, {'X','X',0}, 161832256 }, /* Palmyra Atoll */
    { 339, {'P','N',0}, {'P','C','N',0}, 26286, 612 }, /* Pitcairn Islands */
    { 340, {'X','X',0}, {'X','X',0}, 337 }, /* Rota Island */
    { 341, {'X','X',0}, {'X','X',0}, 337 }, /* Saipan */
    { 342, {'G','S',0}, {'S','G','S',0}, 39070, 239 }, /* South Georgia and the South Sandwich Islands */
    { 343, {'S','H',0}, {'S','H','N',0}, 42483, 654 }, /* St. Helena */
    { 346, {'X','X',0}, {'X','X',0}, 337 }, /* Tinian Island */
    { 347, {'T','K',0}, {'T','K','L',0}, 26286, 772 }, /* Tokelau */
    { 348, {'X','X',0}, {'X','X',0}, 39070 }, /* Tristan da Cunha */
    { 349, {'T','C',0}, {'T','C','A',0}, 10039880, 796 }, /* Turks and Caicos Islands */
    { 351, {'V','G',0}, {'V','G','B',0}, 10039880,  92 }, /* Virgin Islands, British */
    { 352, {'W','F',0}, {'W','L','F',0}, 26286, 876 }, /* Wallis and Futuna */
    { 742, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* Africa */
    { 2129, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* Asia */
    { 10541, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* Europe */
    { 15126, {'I','M',0}, {'I','M','N',0}, 10039882, 833 }, /* Man, Isle of */
    { 19618, {'M','K',0}, {'M','K','D',0}, 47610, 807 }, /* Macedonia, Former Yugoslav Republic of */
    { 20900, {'X','X',0}, {'X','X',0}, 27114, 0, LOCATION_REGION }, /* Melanesia */
    { 21206, {'X','X',0}, {'X','X',0}, 27114, 0, LOCATION_REGION }, /* Micronesia */
    { 21242, {'X','X',0}, {'X','X',0}, 161832256 }, /* Midway Islands */
    { 23581, {'X','X',0}, {'X','X',0}, 10026358, 0, LOCATION_REGION }, /* Northern America */
    { 26286, {'X','X',0}, {'X','X',0}, 27114, 0, LOCATION_REGION }, /* Polynesia */
    { 27082, {'X','X',0}, {'X','X',0}, 161832257, 0, LOCATION_REGION }, /* Central America */
    { 27114, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* Oceania */
    { 30967, {'S','X',0}, {'S','X','M',0}, 10039880, 534 }, /* Sint Maarten (Dutch part) */
    { 31396, {'X','X',0}, {'X','X',0}, 161832257, 0, LOCATION_REGION }, /* South America */
    { 31706, {'M','F',0}, {'M','A','F',0}, 10039880, 663 }, /* Saint Martin (French part) */
    { 39070, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* World */
    { 42483, {'X','X',0}, {'X','X',0}, 742, 0, LOCATION_REGION }, /* Western Africa */
    { 42484, {'X','X',0}, {'X','X',0}, 742, 0, LOCATION_REGION }, /* Middle Africa */
    { 42487, {'X','X',0}, {'X','X',0}, 742, 0, LOCATION_REGION }, /* Northern Africa */
    { 47590, {'X','X',0}, {'X','X',0}, 2129, 0, LOCATION_REGION }, /* Central Asia */
    { 47599, {'X','X',0}, {'X','X',0}, 2129, 0, LOCATION_REGION }, /* South-Eastern Asia */
    { 47600, {'X','X',0}, {'X','X',0}, 2129, 0, LOCATION_REGION }, /* Eastern Asia */
    { 47603, {'X','X',0}, {'X','X',0}, 742, 0, LOCATION_REGION }, /* Eastern Africa */
    { 47609, {'X','X',0}, {'X','X',0}, 10541, 0, LOCATION_REGION }, /* Eastern Europe */
    { 47610, {'X','X',0}, {'X','X',0}, 10541, 0, LOCATION_REGION }, /* Southern Europe */
    { 47611, {'X','X',0}, {'X','X',0}, 2129, 0, LOCATION_REGION }, /* Middle East */
    { 47614, {'X','X',0}, {'X','X',0}, 2129, 0, LOCATION_REGION }, /* Southern Asia */
    { 7299303, {'T','L',0}, {'T','L','S',0}, 47599, 626 }, /* Democratic Republic of Timor-Leste */
    { 10026358, {'X','X',0}, {'X','X',0}, 39070, 0, LOCATION_REGION }, /* Americas */
    { 10028789, {'A','X',0}, {'A','L','A',0}, 10039882, 248 }, /* Åland Islands */
    { 10039880, {'X','X',0}, {'X','X',0}, 161832257, 0, LOCATION_REGION }, /* Caribbean */
    { 10039882, {'X','X',0}, {'X','X',0}, 10541, 0, LOCATION_REGION }, /* Northern Europe */
    { 10039883, {'X','X',0}, {'X','X',0}, 742, 0, LOCATION_REGION }, /* Southern Africa */
    { 10210824, {'X','X',0}, {'X','X',0}, 10541, 0, LOCATION_REGION }, /* Western Europe */
    { 10210825, {'X','X',0}, {'X','X',0}, 27114, 0, LOCATION_REGION }, /* Australia and New Zealand */
    { 161832015, {'B','L',0}, {'B','L','M',0}, 10039880, 652 }, /* Saint Barthélemy */
    { 161832256, {'U','M',0}, {'U','M','I',0}, 27114, 581 }, /* U.S. Minor Outlying Islands */
    { 161832257, {'X','X',0}, {'X','X',0}, 10026358, 0, LOCATION_REGION }, /* Latin America and the Caribbean */
};

#ifdef __REACTOS__
/* Callback function ptrs for EnumSystemCodePagesA/W */
typedef struct
{
  CODEPAGE_ENUMPROCA procA;
  CODEPAGE_ENUMPROCW procW;
  DWORD    dwFlags;
} ENUMSYSTEMCODEPAGES_CALLBACKS;

/* Internal implementation of EnumSystemCodePagesA/W */
static BOOL NLS_EnumSystemCodePages(ENUMSYSTEMCODEPAGES_CALLBACKS *lpProcs)
{
    WCHAR szNumber[5 + 1], szValue[MAX_PATH];
    HANDLE hKey;
    BOOL bContinue = TRUE;
    ULONG ulIndex = 0;

    if (!lpProcs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (lpProcs->dwFlags)
    {
        case CP_INSTALLED:
        case CP_SUPPORTED:
            break;
        default:
            SetLastError(ERROR_INVALID_FLAGS);
            return FALSE;
    }

    hKey = NLS_RegOpenKey(0, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage");
    if (!hKey)
    {
        WARN("NLS_RegOpenKey() failed\n");
        return FALSE;
    }

    while (bContinue)
    {
        if (NLS_RegEnumValue(hKey, ulIndex, szNumber, sizeof(szNumber),
                             szValue, sizeof(szValue)))
        {
            if ((lpProcs->dwFlags == CP_SUPPORTED)||
                ((lpProcs->dwFlags == CP_INSTALLED)&&(wcslen(szValue) > 2)))
            {
                if (lpProcs->procW)
                {
                    bContinue = lpProcs->procW(szNumber);
                }
                else
                {
                    char szNumberA[sizeof(szNumber)/sizeof(WCHAR)];

                    WideCharToMultiByte(CP_ACP, 0, szNumber, -1, szNumberA, sizeof(szNumberA), 0, 0);
                    bContinue = lpProcs->procA(szNumberA);
                }
            }

            ulIndex++;

        } else bContinue = FALSE;

        if (!bContinue)
            break;
    }

    if (hKey)
        NtClose(hKey);

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
EnumSystemCodePagesW (
    CODEPAGE_ENUMPROCW  lpCodePageEnumProc,
    DWORD               dwFlags
    )
{
    ENUMSYSTEMCODEPAGES_CALLBACKS procs;

    TRACE("(%p,0x%08X)\n", lpCodePageEnumProc, dwFlags);

    procs.procA = NULL;
    procs.procW = lpCodePageEnumProc;
    procs.dwFlags = dwFlags;

    return NLS_EnumSystemCodePages(lpCodePageEnumProc ? &procs : NULL);
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumSystemCodePagesA (
    CODEPAGE_ENUMPROCA lpCodePageEnumProc,
    DWORD              dwFlags
    )
{
    ENUMSYSTEMCODEPAGES_CALLBACKS procs;

    TRACE("(%p,0x%08X)\n", lpCodePageEnumProc, dwFlags);

    procs.procA = lpCodePageEnumProc;
    procs.procW = NULL;
    procs.dwFlags = dwFlags;

    return NLS_EnumSystemCodePages(lpCodePageEnumProc ? &procs : NULL);
}


static int
#ifdef __REACTOS__
NLS_GetGeoFriendlyName(GEOID Location, LPWSTR szFriendlyName, int cchData, LANGID lang)
#else
NLS_GetGeoFriendlyName(GEOID Location, LPWSTR szFriendlyName, int cchData)
#endif
{
    /* FIXME: move *.nls resources out of kernel32 into locale.nls */
    Location += NLSRC_OFFSET;
    Location &= 0xFFFF;

    if (cchData == 0)
#ifdef __REACTOS__
        return GetLocalisedText(Location, NULL, 0, lang);
#else
        return GetLocalisedText(Location, NULL, 0);
#endif

#ifdef __REACTOS__
    if (GetLocalisedText(Location, szFriendlyName, (UINT)cchData, lang))
#else
    if (GetLocalisedText(Location, szFriendlyName, (UINT)cchData))
#endif
        return strlenW(szFriendlyName) + 1;

    return 0;
}
#endif // __REACTOS__

static const struct geoinfo_t *get_geoinfo_dataptr(GEOID geoid)
{
    int min, max;

    min = 0;
    max = sizeof(geoinfodata)/sizeof(struct geoinfo_t)-1;

    while (min <= max) {
        const struct geoinfo_t *ptr;
        int n = (min+max)/2;

        ptr = &geoinfodata[n];
        if (geoid == ptr->id)
            /* we don't need empty entries */
            return *ptr->iso2W ? ptr : NULL;

        if (ptr->id > geoid)
            max = n-1;
        else
            min = n+1;
    }

    return NULL;
}

/******************************************************************************
 *           GetGeoInfoW (KERNEL32.@)
 */
INT WINAPI GetGeoInfoW(GEOID geoid, GEOTYPE geotype, LPWSTR data, int data_len, LANGID lang)
{
    const struct geoinfo_t *ptr;
    const WCHAR *str = NULL;
    WCHAR buffW[12];
    LONG val = 0;
    INT len;

    TRACE("%d %d %p %d %d\n", geoid, geotype, data, data_len, lang);

    if (!(ptr = get_geoinfo_dataptr(geoid))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    switch (geotype) {
    case GEO_FRIENDLYNAME:
    {
#ifdef __REACTOS__
        return NLS_GetGeoFriendlyName(geoid, data, data_len, lang);
#else
        return NLS_GetGeoFriendlyName(geoid, data, data_len);
#endif
    }
    case GEO_NATION:
        val = geoid;
        break;
    case GEO_ISO_UN_NUMBER:
        val = ptr->uncode;
        break;
    case GEO_PARENT:
        val = ptr->parent;
        break;
    case GEO_ISO2:
    case GEO_ISO3:
    {
        str = geotype == GEO_ISO2 ? ptr->iso2W : ptr->iso3W;
        break;
    }
    case GEO_RFC1766:
    case GEO_LCID:
    case GEO_OFFICIALNAME:
    case GEO_TIMEZONES:
    case GEO_OFFICIALLANGUAGES:
    case GEO_LATITUDE:
    case GEO_LONGITUDE:
        FIXME("type %d is not supported\n", geotype);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
    default:
        WARN("unrecognized type %d\n", geotype);
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (val) {
        static const WCHAR fmtW[] = {'%','d',0};
        sprintfW(buffW, fmtW, val);
        str = buffW;
    }

    len = strlenW(str) + 1;
    if (!data || !data_len)
        return len;

    memcpy(data, str, min(len, data_len)*sizeof(WCHAR));
    if (data_len < len)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return data_len < len ? 0 : len;
}

/******************************************************************************
 *           GetGeoInfoA (KERNEL32.@)
 */
INT WINAPI GetGeoInfoA(GEOID geoid, GEOTYPE geotype, LPSTR data, int data_len, LANGID lang)
{
    WCHAR *buffW;
    INT len;

    TRACE("%d %d %p %d %d\n", geoid, geotype, data, data_len, lang);

    len = GetGeoInfoW(geoid, geotype, NULL, 0, lang);
    if (!len)
        return 0;

    buffW = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    if (!buffW)
        return 0;

    GetGeoInfoW(geoid, geotype, buffW, len, lang);
    len = WideCharToMultiByte(CP_ACP, 0, buffW, -1, NULL, 0, NULL, NULL);
    if (!data || !data_len) {
        HeapFree(GetProcessHeap(), 0, buffW);
        return len;
    }

    len = WideCharToMultiByte(CP_ACP, 0, buffW, -1, data, data_len, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, buffW);

    if (data_len < len)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return data_len < len ? 0 : len;
}

/******************************************************************************
 *           EnumSystemGeoID    (KERNEL32.@)
 *
 * Call a users function for every location available on the system.
 *
 * PARAMS
 *  geoclass   [I] Type of information desired (SYSGEOTYPE enum from "winnls.h")
 *  parent     [I] GEOID for the parent
 *  enumproc   [I] Callback function to call for each location
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI EnumSystemGeoID(GEOCLASS geoclass, GEOID parent, GEO_ENUMPROC enumproc)
{
    INT i;

    TRACE("(%d, %d, %p)\n", geoclass, parent, enumproc);

    if (!enumproc) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (geoclass != GEOCLASS_NATION && geoclass != GEOCLASS_REGION) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    for (i = 0; i < sizeof(geoinfodata)/sizeof(struct geoinfo_t); i++) {
        const struct geoinfo_t *ptr = &geoinfodata[i];

        if (geoclass == GEOCLASS_NATION && (ptr->kind == LOCATION_REGION))
            continue;

        if (geoclass == GEOCLASS_REGION && (ptr->kind == LOCATION_NATION))
            continue;

        if (parent && ptr->parent != parent)
            continue;

        if (!enumproc(ptr->id))
            return TRUE;
    }

    return TRUE;
}

#ifndef __REACTOS__
INT WINAPI GetUserDefaultLocaleName(LPWSTR localename, int buffersize)
{
    LCID userlcid;

    TRACE("%p, %d\n", localename,  buffersize);

    userlcid = GetUserDefaultLCID();
    return LCIDToLocaleName(userlcid, localename, buffersize, 0);
}

/******************************************************************************
 *           NormalizeString (KERNEL32.@)
 */
INT WINAPI NormalizeString(NORM_FORM NormForm, LPCWSTR lpSrcString, INT cwSrcLength,
                           LPWSTR lpDstString, INT cwDstLength)
{
    FIXME("%x %p %d %p %d\n", NormForm, lpSrcString, cwSrcLength, lpDstString, cwDstLength);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************************
 *           IsNormalizedString (KERNEL32.@)
 */
BOOL WINAPI IsNormalizedString(NORM_FORM NormForm, LPCWSTR lpString, INT cwLength)
{
    FIXME("%x %p %d\n", NormForm, lpString, cwLength);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

enum {
    BASE = 36,
    TMIN = 1,
    TMAX = 26,
    SKEW = 38,
    DAMP = 700,
    INIT_BIAS = 72,
    INIT_N = 128
};

static inline INT adapt(INT delta, INT numpoints, BOOL firsttime)
{
    INT k;

    delta /= (firsttime ? DAMP : 2);
    delta += delta/numpoints;

    for(k=0; delta>((BASE-TMIN)*TMAX)/2; k+=BASE)
        delta /= BASE-TMIN;
    return k+((BASE-TMIN+1)*delta)/(delta+SKEW);
}

/******************************************************************************
 *           IdnToAscii (KERNEL32.@)
 * Implementation of Punycode based on RFC 3492.
 */
INT WINAPI IdnToAscii(DWORD dwFlags, LPCWSTR lpUnicodeCharStr, INT cchUnicodeChar,
                      LPWSTR lpASCIICharStr, INT cchASCIIChar)
{
    static const WCHAR prefixW[] = {'x','n','-','-'};

    WCHAR *norm_str;
    INT i, label_start, label_end, norm_len, out_label, out = 0;

    TRACE("%x %p %d %p %d\n", dwFlags, lpUnicodeCharStr, cchUnicodeChar,
        lpASCIICharStr, cchASCIIChar);

    norm_len = IdnToNameprepUnicode(dwFlags, lpUnicodeCharStr, cchUnicodeChar, NULL, 0);
    if(!norm_len)
        return 0;
    norm_str = HeapAlloc(GetProcessHeap(), 0, norm_len*sizeof(WCHAR));
    if(!norm_str) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    norm_len = IdnToNameprepUnicode(dwFlags, lpUnicodeCharStr,
            cchUnicodeChar, norm_str, norm_len);
    if(!norm_len) {
        HeapFree(GetProcessHeap(), 0, norm_str);
        return 0;
    }

    for(label_start=0; label_start<norm_len;) {
        INT n = INIT_N, bias = INIT_BIAS;
        INT delta = 0, b = 0, h;

        out_label = out;
        for(i=label_start; i<norm_len && norm_str[i]!='.' &&
                norm_str[i]!=0x3002 && norm_str[i]!='\0'; i++)
            if(norm_str[i] < 0x80)
                b++;
        label_end = i;

        if(b == label_end-label_start) {
            if(label_end < norm_len)
                b++;
            if(!lpASCIICharStr) {
                out += b;
            }else if(out+b <= cchASCIIChar) {
                memcpy(lpASCIICharStr+out, norm_str+label_start, b*sizeof(WCHAR));
                out += b;
            }else {
                HeapFree(GetProcessHeap(), 0, norm_str);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            label_start = label_end+1;
            continue;
        }

        if(!lpASCIICharStr) {
            out += 5+b; /* strlen(xn--...-) */
        }else if(out+5+b <= cchASCIIChar) {
            memcpy(lpASCIICharStr+out, prefixW, sizeof(prefixW));
            out += 4;
            for(i=label_start; i<label_end; i++)
                if(norm_str[i] < 0x80)
                    lpASCIICharStr[out++] = norm_str[i];
            lpASCIICharStr[out++] = '-';
        }else {
            HeapFree(GetProcessHeap(), 0, norm_str);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        if(!b)
            out--;

        for(h=b; h<label_end-label_start;) {
            INT m = 0xffff, q, k;

            for(i=label_start; i<label_end; i++) {
                if(norm_str[i]>=n && m>norm_str[i])
                    m = norm_str[i];
            }
            delta += (m-n)*(h+1);
            n = m;

            for(i=label_start; i<label_end; i++) {
                if(norm_str[i] < n) {
                    delta++;
                }else if(norm_str[i] == n) {
                    for(q=delta, k=BASE; ; k+=BASE) {
                        INT t = k<=bias ? TMIN : k>=bias+TMAX ? TMAX : k-bias;
                        INT disp = q<t ? q : t+(q-t)%(BASE-t);
                        if(!lpASCIICharStr) {
                            out++;
                        }else if(out+1 <= cchASCIIChar) {
                            lpASCIICharStr[out++] = disp<='z'-'a' ?
                                'a'+disp : '0'+disp-'z'+'a'-1;
                        }else {
                            HeapFree(GetProcessHeap(), 0, norm_str);
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return 0;
                        }
                        if(q < t)
                            break;
                        q = (q-t)/(BASE-t);
                    }
                    bias = adapt(delta, h+1, h==b);
                    delta = 0;
                    h++;
                }
            }
            delta++;
            n++;
        }

        if(out-out_label > 63) {
            HeapFree(GetProcessHeap(), 0, norm_str);
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < norm_len) {
            if(!lpASCIICharStr) {
                out++;
            }else if(out+1 <= cchASCIIChar) {
                lpASCIICharStr[out++] = norm_str[label_end] ? '.' : 0;
            }else {
                HeapFree(GetProcessHeap(), 0, norm_str);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
        }
        label_start = label_end+1;
    }

    HeapFree(GetProcessHeap(), 0, norm_str);
    return out;
}

/******************************************************************************
 *           IdnToNameprepUnicode (KERNEL32.@)
 */
INT WINAPI IdnToNameprepUnicode(DWORD dwFlags, LPCWSTR lpUnicodeCharStr, INT cchUnicodeChar,
                                LPWSTR lpNameprepCharStr, INT cchNameprepChar)
{
    enum {
        UNASSIGNED = 0x1,
        PROHIBITED = 0x2,
        BIDI_RAL   = 0x4,
        BIDI_L     = 0x8
    };

    extern const unsigned short nameprep_char_type[] DECLSPEC_HIDDEN;
    extern const WCHAR nameprep_mapping[] DECLSPEC_HIDDEN;
    const WCHAR *ptr;
    WORD flags;
    WCHAR buf[64], *map_str, norm_str[64], ch;
    DWORD i, map_len, norm_len, mask, label_start, label_end, out = 0;
    BOOL have_bidi_ral, prohibit_bidi_ral, ascii_only;

    TRACE("%x %p %d %p %d\n", dwFlags, lpUnicodeCharStr, cchUnicodeChar,
        lpNameprepCharStr, cchNameprepChar);

    if(dwFlags & ~(IDN_ALLOW_UNASSIGNED|IDN_USE_STD3_ASCII_RULES)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if(!lpUnicodeCharStr || cchUnicodeChar<-1) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if(cchUnicodeChar == -1)
        cchUnicodeChar = strlenW(lpUnicodeCharStr)+1;
    if(!cchUnicodeChar || (cchUnicodeChar==1 && lpUnicodeCharStr[0]==0)) {
        SetLastError(ERROR_INVALID_NAME);
        return 0;
    }

    for(label_start=0; label_start<cchUnicodeChar;) {
        ascii_only = TRUE;
        for(i=label_start; i<cchUnicodeChar; i++) {
            ch = lpUnicodeCharStr[i];

            if(i!=cchUnicodeChar-1 && !ch) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            /* check if ch is one of label separators defined in RFC3490 */
            if(!ch || ch=='.' || ch==0x3002 || ch==0xff0e || ch==0xff61)
                break;

            if(ch > 0x7f) {
                ascii_only = FALSE;
                continue;
            }

            if((dwFlags&IDN_USE_STD3_ASCII_RULES) == 0)
                continue;
            if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
                    || (ch>='0' && ch<='9') || ch=='-')
                continue;

            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        label_end = i;
        /* last label may be empty */
        if(label_start==label_end && ch) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if((dwFlags&IDN_USE_STD3_ASCII_RULES) && (lpUnicodeCharStr[label_start]=='-' ||
                    lpUnicodeCharStr[label_end-1]=='-')) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(ascii_only) {
            /* maximal label length is 63 characters */
            if(label_end-label_start > 63) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            if(label_end < cchUnicodeChar)
                label_end++;

            if(!lpNameprepCharStr) {
                out += label_end-label_start;
            }else if(out+label_end-label_start <= cchNameprepChar) {
                memcpy(lpNameprepCharStr+out, lpUnicodeCharStr+label_start,
                        (label_end-label_start)*sizeof(WCHAR));
                if(lpUnicodeCharStr[label_end-1] > 0x7f)
                    lpNameprepCharStr[out+label_end-label_start-1] = '.';
                out += label_end-label_start;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            label_start = label_end;
            continue;
        }

        map_len = 0;
        for(i=label_start; i<label_end; i++) {
            ch = lpUnicodeCharStr[i];
            ptr = nameprep_mapping + nameprep_mapping[ch>>8];
            ptr = nameprep_mapping + ptr[(ch>>4)&0x0f] + 3*(ch&0x0f);

            if(!ptr[0]) map_len++;
            else if(!ptr[1]) map_len++;
            else if(!ptr[2]) map_len += 2;
            else if(ptr[0]!=0xffff || ptr[1]!=0xffff || ptr[2]!=0xffff) map_len += 3;
        }
        if(map_len*sizeof(WCHAR) > sizeof(buf)) {
            map_str = HeapAlloc(GetProcessHeap(), 0, map_len*sizeof(WCHAR));
            if(!map_str) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
        }else {
            map_str = buf;
        }
        map_len = 0;
        for(i=label_start; i<label_end; i++) {
            ch = lpUnicodeCharStr[i];
            ptr = nameprep_mapping + nameprep_mapping[ch>>8];
            ptr = nameprep_mapping + ptr[(ch>>4)&0x0f] + 3*(ch&0x0f);

            if(!ptr[0]) {
                map_str[map_len++] = ch;
            }else if(!ptr[1]) {
                map_str[map_len++] = ptr[0];
            }else if(!ptr[2]) {
                map_str[map_len++] = ptr[0];
                map_str[map_len++] = ptr[1];
            }else if(ptr[0]!=0xffff || ptr[1]!=0xffff || ptr[2]!=0xffff) {
                map_str[map_len++] = ptr[0];
                map_str[map_len++] = ptr[1];
                map_str[map_len++] = ptr[2];
            }
        }

        norm_len = FoldStringW(MAP_FOLDCZONE, map_str, map_len,
                norm_str, sizeof(norm_str)/sizeof(WCHAR)-1);
        if(map_str != buf)
            HeapFree(GetProcessHeap(), 0, map_str);
        if(!norm_len) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < cchUnicodeChar) {
            norm_str[norm_len++] = lpUnicodeCharStr[label_end] ? '.' : 0;
            label_end++;
        }

        if(!lpNameprepCharStr) {
            out += norm_len;
        }else if(out+norm_len <= cchNameprepChar) {
            memcpy(lpNameprepCharStr+out, norm_str, norm_len*sizeof(WCHAR));
            out += norm_len;
        }else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

        have_bidi_ral = prohibit_bidi_ral = FALSE;
        mask = PROHIBITED;
        if((dwFlags&IDN_ALLOW_UNASSIGNED) == 0)
            mask |= UNASSIGNED;
        for(i=0; i<norm_len; i++) {
            ch = norm_str[i];
            flags = get_table_entry( nameprep_char_type, ch );

            if(flags & mask) {
                SetLastError((flags & PROHIBITED) ? ERROR_INVALID_NAME
                        : ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }

            if(flags & BIDI_RAL)
                have_bidi_ral = TRUE;
            if(flags & BIDI_L)
                prohibit_bidi_ral = TRUE;
        }

        if(have_bidi_ral) {
            ch = norm_str[0];
            flags = get_table_entry( nameprep_char_type, ch );
            if((flags & BIDI_RAL) == 0)
                prohibit_bidi_ral = TRUE;

            ch = norm_str[norm_len-1];
            flags = get_table_entry( nameprep_char_type, ch );
            if((flags & BIDI_RAL) == 0)
                prohibit_bidi_ral = TRUE;
        }

        if(have_bidi_ral && prohibit_bidi_ral) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        label_start = label_end;
    }

    return out;
}

/******************************************************************************
 *           IdnToUnicode (KERNEL32.@)
 */
INT WINAPI IdnToUnicode(DWORD dwFlags, LPCWSTR lpASCIICharStr, INT cchASCIIChar,
                        LPWSTR lpUnicodeCharStr, INT cchUnicodeChar)
{
    extern const unsigned short nameprep_char_type[];

    INT i, label_start, label_end, out_label, out = 0;
    WCHAR ch;

    TRACE("%x %p %d %p %d\n", dwFlags, lpASCIICharStr, cchASCIIChar,
        lpUnicodeCharStr, cchUnicodeChar);

    for(label_start=0; label_start<cchASCIIChar;) {
        INT n = INIT_N, pos = 0, old_pos, w, k, bias = INIT_BIAS, delim=0, digit, t;

        out_label = out;
        for(i=label_start; i<cchASCIIChar; i++) {
            ch = lpASCIICharStr[i];

            if(ch>0x7f || (i!=cchASCIIChar-1 && !ch)) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }

            if(!ch || ch=='.')
                break;
            if(ch == '-')
                delim = i;

            if((dwFlags&IDN_USE_STD3_ASCII_RULES) == 0)
                continue;
            if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
                    || (ch>='0' && ch<='9') || ch=='-')
                continue;

            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        label_end = i;
        /* last label may be empty */
        if(label_start==label_end && ch) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if((dwFlags&IDN_USE_STD3_ASCII_RULES) && (lpASCIICharStr[label_start]=='-' ||
                    lpASCIICharStr[label_end-1]=='-')) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        if(label_end-label_start > 63) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end-label_start<4 ||
                tolowerW(lpASCIICharStr[label_start])!='x' ||
                tolowerW(lpASCIICharStr[label_start+1])!='n' ||
                lpASCIICharStr[label_start+2]!='-' || lpASCIICharStr[label_start+3]!='-') {
            if(label_end < cchASCIIChar)
                label_end++;

            if(!lpUnicodeCharStr) {
                out += label_end-label_start;
            }else if(out+label_end-label_start <= cchUnicodeChar) {
                memcpy(lpUnicodeCharStr+out, lpASCIICharStr+label_start,
                        (label_end-label_start)*sizeof(WCHAR));
                out += label_end-label_start;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            label_start = label_end;
            continue;
        }

        if(delim == label_start+3)
            delim++;
        if(!lpUnicodeCharStr) {
            out += delim-label_start-4;
        }else if(out+delim-label_start-4 <= cchUnicodeChar) {
            memcpy(lpUnicodeCharStr+out, lpASCIICharStr+label_start+4,
                    (delim-label_start-4)*sizeof(WCHAR));
            out += delim-label_start-4;
        }else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        if(out != out_label)
            delim++;

        for(i=delim; i<label_end;) {
            old_pos = pos;
            w = 1;
            for(k=BASE; ; k+=BASE) {
                ch = i<label_end ? tolowerW(lpASCIICharStr[i++]) : 0;
                if((ch<'a' || ch>'z') && (ch<'0' || ch>'9')) {
                    SetLastError(ERROR_INVALID_NAME);
                    return 0;
                }
                digit = ch<='9' ? ch-'0'+'z'-'a'+1 : ch-'a';
                pos += digit*w;
                t = k<=bias ? TMIN : k>=bias+TMAX ? TMAX : k-bias;
                if(digit < t)
                    break;
                w *= BASE-t;
            }
            bias = adapt(pos-old_pos, out-out_label+1, old_pos==0);
            n += pos/(out-out_label+1);
            pos %= out-out_label+1;

            if((dwFlags&IDN_ALLOW_UNASSIGNED)==0 &&
                    get_table_entry(nameprep_char_type, n)==1/*UNASSIGNED*/) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            if(!lpUnicodeCharStr) {
                out++;
            }else if(out+1 <= cchASCIIChar) {
                memmove(lpUnicodeCharStr+out_label+pos+1,
                        lpUnicodeCharStr+out_label+pos,
                        (out-out_label-pos)*sizeof(WCHAR));
                lpUnicodeCharStr[out_label+pos] = n;
                out++;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            pos++;
        }

        if(out-out_label > 63) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < cchASCIIChar) {
            if(!lpUnicodeCharStr) {
                out++;
            }else if(out+1 <= cchUnicodeChar) {
                lpUnicodeCharStr[out++] = lpASCIICharStr[label_end];
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
        }
        label_start = label_end+1;
    }

    return out;
}


/******************************************************************************
 *           GetFileMUIPath (KERNEL32.@)
 */

BOOL WINAPI GetFileMUIPath(DWORD flags, PCWSTR filepath, PWSTR language, PULONG languagelen,
                           PWSTR muipath, PULONG muipathlen, PULONGLONG enumerator)
{
    FIXME("stub: 0x%x, %s, %s, %p, %p, %p, %p\n", flags, debugstr_w(filepath),
           debugstr_w(language), languagelen, muipath, muipathlen, enumerator);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/******************************************************************************
 *           GetFileMUIInfo (KERNEL32.@)
 */

BOOL WINAPI GetFileMUIInfo(DWORD flags, PCWSTR path, FILEMUIINFO *info, DWORD *size)
{
    FIXME("stub: %u, %s, %p, %p\n", flags, debugstr_w(path), info, size);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *           ResolveLocaleName (KERNEL32.@)
 */

INT WINAPI ResolveLocaleName(LPCWSTR name, LPWSTR localename, INT len)
{
    FIXME("stub: %s, %p, %d\n", wine_dbgstr_w(name), localename, len);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
#endif // !__REACTOS__
