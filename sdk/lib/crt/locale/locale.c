/*
 * msvcrt.dll locale functions
 *
 * Copyright 2000 Jon Griffiths
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

#include <precomp.h>
#include <locale.h>

#include "mbctype.h"
#include <internal/wine/msvcrt.h>

#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256

#ifdef _pctype
#error _pctype should not be defined
#endif

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
unsigned int __lc_codepage = 0;
int MSVCRT___lc_collate_cp = 0;
LCID MSVCRT___lc_handle[LC_MAX - LC_MIN + 1] = { 0 };
int __mb_cur_max = 1;
static unsigned char charmax = CHAR_MAX;

unsigned char _mbctype[257] = { 0 };

/* MT */
#define LOCK_LOCALE   _mlock(_SETLOCALE_LOCK);
#define UNLOCK_LOCALE _munlock(_SETLOCALE_LOCK);

#define MSVCRT_LEADBYTE  0x8000
#define MSVCRT_C1_DEFINED 0x200

/* Friendly country strings & language names abbreviations. */
static const char * const _country_synonyms[] =
{
    "american", "enu",
    "american english", "enu",
    "american-english", "enu",
    "english-american", "enu",
    "english-us", "enu",
    "english-usa", "enu",
    "us", "enu",
    "usa", "enu",
    "australian", "ena",
    "english-aus", "ena",
    "belgian", "nlb",
    "french-belgian", "frb",
    "canadian", "enc",
    "english-can", "enc",
    "french-canadian", "frc",
    "chinese", "chs",
    "chinese-simplified", "chs",
    "chinese-traditional", "cht",
    "dutch-belgian", "nlb",
    "english-nz", "enz",
    "uk", "eng",
    "english-uk", "eng",
    "french-swiss", "frs",
    "swiss", "des",
    "german-swiss", "des",
    "italian-swiss", "its",
    "german-austrian", "dea",
    "portuguese", "ptb",
    "portuguese-brazil", "ptb",
    "spanish-mexican", "esm",
    "norwegian-bokmal", "nor",
    "norwegian-nynorsk", "non",
    "spanish-modern", "esn"
};

/* INTERNAL: Map a synonym to an ISO code */
static void remap_synonym(char *name)
{
  unsigned int i;
  for (i = 0; i < sizeof(_country_synonyms)/sizeof(char*); i += 2 )
  {
    if (!strcasecmp(_country_synonyms[i],name))
    {
      TRACE(":Mapping synonym %s to %s\n",name,_country_synonyms[i+1]);
      strcpy(name, _country_synonyms[i+1]);
      return;
    }
  }
}

/* Note: Flags are weighted in order of matching importance */
#define FOUND_LANGUAGE         0x4
#define FOUND_COUNTRY          0x2
#define FOUND_CODEPAGE         0x1

typedef struct {
  char search_language[MAX_ELEM_LEN];
  char search_country[MAX_ELEM_LEN];
  char search_codepage[MAX_ELEM_LEN];
  char found_codepage[MAX_ELEM_LEN];
  unsigned int match_flags;
  LANGID found_lang_id;
} locale_search_t;

#define CONTINUE_LOOKING TRUE
#define STOP_LOOKING     FALSE

/* INTERNAL: Get and compare locale info with a given string */
static int compare_info(LCID lcid, DWORD flags, char* buff, const char* cmp, BOOL exact)
{
  int len;

  if(!cmp[0])
      return 0;

  buff[0] = 0;
  GetLocaleInfoA(lcid, flags|LOCALE_NOUSEROVERRIDE, buff, MAX_ELEM_LEN);
  if (!buff[0])
    return 0;

  /* Partial matches are only allowed on language/country names */
  len = strlen(cmp);
  if(exact || len<=3)
    return !strcasecmp(cmp, buff);
  else
    return !strncasecmp(cmp, buff, len);
}

static BOOL CALLBACK
find_best_locale_proc(HMODULE hModule, LPCSTR type, LPCSTR name, WORD LangID, LONG_PTR lParam)
{
  locale_search_t *res = (locale_search_t *)lParam;
  const LCID lcid = MAKELCID(LangID, SORT_DEFAULT);
  char buff[MAX_ELEM_LEN];
  unsigned int flags = 0;

  if(PRIMARYLANGID(LangID) == LANG_NEUTRAL)
    return CONTINUE_LOOKING;

  /* Check Language */
  if (compare_info(lcid,LOCALE_SISO639LANGNAME,buff,res->search_language, TRUE) ||
      compare_info(lcid,LOCALE_SABBREVLANGNAME,buff,res->search_language, TRUE) ||
      compare_info(lcid,LOCALE_SENGLANGUAGE,buff,res->search_language, FALSE))
  {
    TRACE(":Found language: %s->%s\n", res->search_language, buff);
    flags |= FOUND_LANGUAGE;
  }
  else if (res->match_flags & FOUND_LANGUAGE)
  {
    return CONTINUE_LOOKING;
  }

  /* Check Country */
  if (compare_info(lcid,LOCALE_SISO3166CTRYNAME,buff,res->search_country, TRUE) ||
      compare_info(lcid,LOCALE_SABBREVCTRYNAME,buff,res->search_country, TRUE) ||
      compare_info(lcid,LOCALE_SENGCOUNTRY,buff,res->search_country, FALSE))
  {
    TRACE("Found country:%s->%s\n", res->search_country, buff);
    flags |= FOUND_COUNTRY;
  }
  else if (!flags && (res->match_flags & FOUND_COUNTRY))
  {
    return CONTINUE_LOOKING;
  }

  /* Check codepage */
  if (compare_info(lcid,LOCALE_IDEFAULTCODEPAGE,buff,res->search_codepage, TRUE) ||
      (compare_info(lcid,LOCALE_IDEFAULTANSICODEPAGE,buff,res->search_codepage, TRUE)))
  {
    TRACE("Found codepage:%s->%s\n", res->search_codepage, buff);
    flags |= FOUND_CODEPAGE;
    memcpy(res->found_codepage,res->search_codepage,MAX_ELEM_LEN);
  }
  else if (!flags && (res->match_flags & FOUND_CODEPAGE))
  {
    return CONTINUE_LOOKING;
  }

  if (flags > res->match_flags)
  {
    /* Found a better match than previously */
    res->match_flags = flags;
    res->found_lang_id = LangID;
  }
  if ((flags & (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE)) ==
        (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE))
  {
    TRACE(":found exact locale match\n");
    return STOP_LOOKING;
  }
  return CONTINUE_LOOKING;
}

extern int atoi(const char *);

/* Internal: Find the LCID for a locale specification */
LCID MSVCRT_locale_to_LCID(const char *locale, unsigned short *codepage)
{
    LCID lcid;
    locale_search_t search;
    const char *cp, *region;

    memset(&search, 0, sizeof(locale_search_t));

    cp = strchr(locale, '.');
    region = strchr(locale, '_');

    lstrcpynA(search.search_language, locale, MAX_ELEM_LEN);
    if(region) {
        lstrcpynA(search.search_country, region+1, MAX_ELEM_LEN);
        if(region-locale < MAX_ELEM_LEN)
            search.search_language[region-locale] = '\0';
    } else
        search.search_country[0] = '\0';

    if(cp) {
        lstrcpynA(search.search_codepage, cp+1, MAX_ELEM_LEN);
        if(region && cp-region-1<MAX_ELEM_LEN)
          search.search_country[cp-region-1] = '\0';
        if(cp-locale < MAX_ELEM_LEN)
            search.search_language[cp-locale] = '\0';
    } else
        search.search_codepage[0] = '\0';

    if(!search.search_country[0] && !search.search_codepage[0])
        remap_synonym(search.search_language);

    EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), (LPSTR)RT_STRING,
            (LPCSTR)LOCALE_ILANGUAGE,find_best_locale_proc,
            (LONG_PTR)&search);

    if (!search.match_flags)
        return -1;

    /* If we were given something that didn't match, fail */
    if (search.search_country[0] && !(search.match_flags & FOUND_COUNTRY))
        return -1;

    lcid =  MAKELCID(search.found_lang_id, SORT_DEFAULT);

    /* Populate partial locale, translating LCID to locale string elements */
    if (!(search.match_flags & FOUND_CODEPAGE)) {
        /* Even if a codepage is not enumerated for a locale
         * it can be set if valid */
        if (search.search_codepage[0]) {
            if (IsValidCodePage(atoi(search.search_codepage)))
                memcpy(search.found_codepage,search.search_codepage,MAX_ELEM_LEN);
            else {
                /* Special codepage values: OEM & ANSI */
                if (!strcasecmp(search.search_codepage,"OCP")) {
                    GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                            search.found_codepage, MAX_ELEM_LEN);
                } else if (!strcasecmp(search.search_codepage,"ACP")) {
                    GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                            search.found_codepage, MAX_ELEM_LEN);
                } else
                    return -1;

                if (!atoi(search.found_codepage))
                    return -1;
            }
        } else {
            /* Prefer ANSI codepages if present */
            GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                    search.found_codepage, MAX_ELEM_LEN);
            if (!search.found_codepage[0] || !atoi(search.found_codepage))
                GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                        search.found_codepage, MAX_ELEM_LEN);
        }
    }
    if (codepage)
        *codepage = atoi(search.found_codepage);

    return lcid;
}

/* INTERNAL: Set lc_handle, lc_id and lc_category in threadlocinfo struct */
static BOOL update_threadlocinfo_category(LCID lcid, unsigned short cp,
        MSVCRT__locale_t loc, int category)
{
    char buf[256], *p;
    int len;

    if(GetLocaleInfoA(lcid, LOCALE_ILANGUAGE|LOCALE_NOUSEROVERRIDE, buf, 256)) {
        p = buf;

        loc->locinfo->lc_id[category].wLanguage = 0;
        while(*p) {
            loc->locinfo->lc_id[category].wLanguage *= 16;

            if(*p <= '9')
                loc->locinfo->lc_id[category].wLanguage += *p-'0';
            else
                loc->locinfo->lc_id[category].wLanguage += *p-'a'+10;

            p++;
        }

        loc->locinfo->lc_id[category].wCountry =
            loc->locinfo->lc_id[category].wLanguage;
    }

    loc->locinfo->lc_id[category].wCodePage = cp;

    loc->locinfo->lc_handle[category] = lcid;

    len = 0;
    len += GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE
            |LOCALE_NOUSEROVERRIDE, buf, 256);
    buf[len-1] = '_';
    len += GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY
            |LOCALE_NOUSEROVERRIDE, &buf[len], 256-len);
    buf[len-1] = '.';
    sprintf(buf+len, "%u", cp);
    len += strlen(buf+len)+1;

    loc->locinfo->lc_category[category].locale = MSVCRT_malloc(len);
    loc->locinfo->lc_category[category].refcount = MSVCRT_malloc(sizeof(int));
    if(!loc->locinfo->lc_category[category].locale
            || !loc->locinfo->lc_category[category].refcount) {
        MSVCRT_free(loc->locinfo->lc_category[category].locale);
        MSVCRT_free(loc->locinfo->lc_category[category].refcount);
        loc->locinfo->lc_category[category].locale = NULL;
        loc->locinfo->lc_category[category].refcount = NULL;
        return TRUE;
    }
    memcpy(loc->locinfo->lc_category[category].locale, buf, len);
    *loc->locinfo->lc_category[category].refcount = 1;

    return FALSE;
}

/* INTERNAL: swap pointers values */
static inline void swap_pointers(void **p1, void **p2) {
    void *hlp;

    hlp = *p1;
    *p1 = *p2;
    *p2 = hlp;
}

/* INTERNAL: returns pthreadlocinfo struct */
MSVCRT_pthreadlocinfo get_locinfo(void) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data || !data->have_locale)
        return MSVCRT_locale->locinfo;

    return data->locinfo;
}

/* INTERNAL: returns pthreadlocinfo struct */
MSVCRT_pthreadmbcinfo get_mbcinfo(void) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data || !data->have_locale)
        return MSVCRT_locale->mbcinfo;

    return data->mbcinfo;
}

/* INTERNAL: constructs string returned by setlocale */
static inline char* construct_lc_all(MSVCRT_pthreadlocinfo locinfo) {
    static char current_lc_all[MAX_LOCALE_LENGTH];

    int i;

    for(i=MSVCRT_LC_MIN+1; i<MSVCRT_LC_MAX; i++) {
        if(strcmp(locinfo->lc_category[i].locale,
                    locinfo->lc_category[i+1].locale))
            break;
    }

    if(i==MSVCRT_LC_MAX)
        return locinfo->lc_category[MSVCRT_LC_COLLATE].locale;

    sprintf(current_lc_all,
            "LC_COLLATE=%s;LC_CTYPE=%s;LC_MONETARY=%s;LC_NUMERIC=%s;LC_TIME=%s",
            locinfo->lc_category[MSVCRT_LC_COLLATE].locale,
            locinfo->lc_category[MSVCRT_LC_CTYPE].locale,
            locinfo->lc_category[MSVCRT_LC_MONETARY].locale,
            locinfo->lc_category[MSVCRT_LC_NUMERIC].locale,
            locinfo->lc_category[MSVCRT_LC_TIME].locale);

    return current_lc_all;
}


/*********************************************************************
 *		wsetlocale (MSVCRT.@)
 */
wchar_t* CDECL _wsetlocale(int category, const wchar_t* locale)
{
  static wchar_t fake[] = {
    'E','n','g','l','i','s','h','_','U','n','i','t','e','d',' ',
    'S','t','a','t','e','s','.','1','2','5','2',0 };

  FIXME("%d %s\n", category, debugstr_w(locale));

  return fake;
}

/*********************************************************************
 *		_Getdays (MSVCRT.@)
 */
char* CDECL _Getdays(void)
{
    MSVCRT___lc_time_data *cur = get_locinfo()->lc_time_curr;
    int i, len, size;
    char *out;

    TRACE("\n");

    size = cur->str.names.short_mon[0]-cur->str.names.short_wday[0];
    out = MSVCRT_malloc(size+1);
    if(!out)
        return NULL;

    size = 0;
    for(i=0; i<7; i++) {
        out[size++] = ':';
        len = strlen(cur->str.names.short_wday[i]);
        memcpy(&out[size], cur->str.names.short_wday[i], len);
        size += len;

        out[size++] = ':';
        len = strlen(cur->str.names.wday[i]);
        memcpy(&out[size], cur->str.names.wday[i], len);
        size += len;
    }
    out[size] = '\0';

    return out;
}

/*********************************************************************
 *		_Getmonths (MSVCRT.@)
 */
char* CDECL _Getmonths(void)
{
    MSVCRT___lc_time_data *cur = get_locinfo()->lc_time_curr;
    int i, len, size;
    char *out;

    TRACE("\n");

    size = cur->str.names.am-cur->str.names.short_mon[0];
    out = MSVCRT_malloc(size+1);
    if(!out)
        return NULL;

    size = 0;
    for(i=0; i<12; i++) {
        out[size++] = ':';
        len = strlen(cur->str.names.short_mon[i]);
        memcpy(&out[size], cur->str.names.short_mon[i], len);
        size += len;

        out[size++] = ':';
        len = strlen(cur->str.names.mon[i]);
        memcpy(&out[size], cur->str.names.mon[i], len);
        size += len;
    }
    out[size] = '\0';

    return out;
}

/*********************************************************************
 *		_Gettnames (MSVCRT.@)
 */
void* CDECL _Gettnames(void)
{
    MSVCRT___lc_time_data *ret, *cur = get_locinfo()->lc_time_curr;
    unsigned int i, size = sizeof(MSVCRT___lc_time_data);

    TRACE("\n");

    for(i=0; i<sizeof(cur->str.str)/sizeof(cur->str.str[0]); i++)
        size += strlen(cur->str.str[i])+1;

    ret = MSVCRT_malloc(size);
    if(!ret)
        return NULL;
    memcpy(ret, cur, size);

    size = 0;
    for(i=0; i<sizeof(cur->str.str)/sizeof(cur->str.str[0]); i++) {
        ret->str.str[i] = &ret->data[size];
        size += strlen(&ret->data[size])+1;
    }

    return ret;
}

/*********************************************************************
 *		__crtLCMapStringA (MSVCRT.@)
 */
int CDECL __crtLCMapStringA(
  LCID lcid, DWORD mapflags, const char* src, int srclen, char* dst,
  int dstlen, unsigned int codepage, int xflag
) {
  FIXME("(lcid %x, flags %x, %s(%d), %p(%d), %x, %d), partial stub!\n",
        lcid,mapflags,src,srclen,dst,dstlen,codepage,xflag);
  /* FIXME: A bit incorrect. But msvcrt itself just converts its
   * arguments to wide strings and then calls LCMapStringW
   */
  return LCMapStringA(lcid,mapflags,src,srclen,dst,dstlen);
}

/*********************************************************************
 *              __crtLCMapStringW (MSVCRT.@)
 */
int CDECL __crtLCMapStringW(LCID lcid, DWORD mapflags, const wchar_t *src,
        int srclen, wchar_t *dst, int dstlen, unsigned int codepage, int xflag)
{
    FIXME("(lcid %x, flags %x, %s(%d), %p(%d), %x, %d), partial stub!\n",
            lcid, mapflags, debugstr_w(src), srclen, dst, dstlen, codepage, xflag);

    return LCMapStringW(lcid, mapflags, src, srclen, dst, dstlen);
}

/*********************************************************************
 *		__crtCompareStringA (MSVCRT.@)
 */
int CDECL __crtCompareStringA( LCID lcid, DWORD flags, const char *src1, int len1,
                               const char *src2, int len2 )
{
    FIXME("(lcid %x, flags %x, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_a(src1), len1, debugstr_a(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringA( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtCompareStringW (MSVCRT.@)
 */
int CDECL __crtCompareStringW( LCID lcid, DWORD flags, const wchar_t *src1, int len1,
                               const wchar_t *src2, int len2 )
{
    FIXME("(lcid %x, flags %x, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_w(src1), len1, debugstr_w(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringW( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtGetLocaleInfoW (MSVCRT.@)
 */
int CDECL __crtGetLocaleInfoW( LCID lcid, LCTYPE type, wchar_t *buffer, int len )
{
    FIXME("(lcid %x, type %x, %p(%d), partial stub\n", lcid, type, buffer, len );
    /* FIXME: probably not entirely right */
    return GetLocaleInfoW( lcid, type, buffer, len );
}

/*********************************************************************
 *              btowc(MSVCRT.@)
 */
wint_t CDECL MSVCRT_btowc(int c)
{
    unsigned char letter = c;
    wchar_t ret;

    if(!MultiByteToWideChar(get_locinfo()->lc_handle[LC_CTYPE],
                0, (LPCSTR)&letter, 1, &ret, 1))
        return 0;

    return ret;
}

/*********************************************************************
 *              __crtGetStringTypeW(MSVCRT.@)
 *
 * This function was accepting different number of arguments in older
 * versions of msvcrt.
 */
BOOL CDECL __crtGetStringTypeW(DWORD unk, DWORD type,
        wchar_t *buffer, int len, WORD *out)
{
    FIXME("(unk %x, type %x, wstr %p(%d), %p) partial stub\n",
            unk, type, buffer, len, out);

    return GetStringTypeW(type, buffer, len, out);
}

/*********************************************************************
 *		localeconv (MSVCRT.@)
 */
struct lconv * CDECL localeconv(void)
{
    return (struct lconv*)get_locinfo()->lconv;
}

/*********************************************************************
 *		__lconv_init (MSVCRT.@)
 */
int CDECL __lconv_init(void)
{
    /* this is used to make chars unsigned */
    charmax = 255;
    return 0;
}

/*********************************************************************
 *      ___lc_handle_func (MSVCRT.@)
 */
LCID* CDECL ___lc_handle_func(void)
{
    return MSVCRT___lc_handle;
}

/*********************************************************************
 *      ___lc_codepage_func (MSVCRT.@)
 */
unsigned int CDECL ___lc_codepage_func(void)
{
    return __lc_codepage;
}

/*********************************************************************
 *      ___lc_collate_cp_func (MSVCRT.@)
 */
unsigned int CDECL ___lc_collate_cp_func(void)
{
    return get_locinfo()->lc_collate_cp;
}

/* INTERNAL: frees MSVCRT_pthreadlocinfo struct */
void free_locinfo(MSVCRT_pthreadlocinfo locinfo)
{
    int i;

    if(!locinfo)
        return;

    if(InterlockedDecrement(&locinfo->refcount))
        return;

    for(i=MSVCRT_LC_MIN+1; i<=MSVCRT_LC_MAX; i++) {
        MSVCRT_free(locinfo->lc_category[i].locale);
        MSVCRT_free(locinfo->lc_category[i].refcount);
    }

    if(locinfo->lconv) {
        MSVCRT_free(locinfo->lconv->decimal_point);
        MSVCRT_free(locinfo->lconv->thousands_sep);
        MSVCRT_free(locinfo->lconv->grouping);
        MSVCRT_free(locinfo->lconv->int_curr_symbol);
        MSVCRT_free(locinfo->lconv->currency_symbol);
        MSVCRT_free(locinfo->lconv->mon_decimal_point);
        MSVCRT_free(locinfo->lconv->mon_thousands_sep);
        MSVCRT_free(locinfo->lconv->mon_grouping);
        MSVCRT_free(locinfo->lconv->positive_sign);
        MSVCRT_free(locinfo->lconv->negative_sign);
    }
    MSVCRT_free(locinfo->lconv_intl_refcount);
    MSVCRT_free(locinfo->lconv_num_refcount);
    MSVCRT_free(locinfo->lconv_mon_refcount);
    MSVCRT_free(locinfo->lconv);

    MSVCRT_free(locinfo->ctype1_refcount);
    MSVCRT_free(locinfo->ctype1);

    MSVCRT_free(locinfo->pclmap);
    MSVCRT_free(locinfo->pcumap);

    MSVCRT_free(locinfo->lc_time_curr);

    MSVCRT_free(locinfo);
}

/* INTERNAL: frees MSVCRT_pthreadmbcinfo struct */
void free_mbcinfo(MSVCRT_pthreadmbcinfo mbcinfo)
{
    if(!mbcinfo)
        return;

    if(InterlockedDecrement(&mbcinfo->refcount))
        return;

    MSVCRT_free(mbcinfo);
}

/* _get_current_locale - not exported in native msvcrt */
MSVCRT__locale_t CDECL MSVCRT__get_current_locale(void)
{
    MSVCRT__locale_t loc = MSVCRT_malloc(sizeof(MSVCRT__locale_tstruct));
    if(!loc)
        return NULL;

    loc->locinfo = get_locinfo();
    loc->mbcinfo = get_mbcinfo();
    InterlockedIncrement(&loc->locinfo->refcount);
    InterlockedIncrement(&loc->mbcinfo->refcount);
    return loc;
}

/* _free_locale - not exported in native msvcrt */
void CDECL MSVCRT__free_locale(MSVCRT__locale_t locale)
{
    if (!locale)
        return;

    free_locinfo(locale->locinfo);
    free_mbcinfo(locale->mbcinfo);
    MSVCRT_free(locale);
}

/* _create_locale - not exported in native msvcrt */
MSVCRT__locale_t CDECL MSVCRT__create_locale(int category, const char *locale)
{
    static const DWORD time_data[] = {
        LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
        LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
        LOCALE_SABBREVDAYNAME6,
        LOCALE_SDAYNAME7, LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
        LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6,
        LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
        LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
        LOCALE_S1159, LOCALE_S2359,
        LOCALE_SSHORTDATE, LOCALE_SLONGDATE,
        LOCALE_STIMEFORMAT
    };
    static const char collate[] = "COLLATE=";
    static const char ctype[] = "CTYPE=";
    static const char monetary[] = "MONETARY=";
    static const char numeric[] = "NUMERIC=";
    static const char time[] = "TIME=";
    static const char cloc_short_date[] = "MM/dd/yy";
    static const wchar_t cloc_short_dateW[] = {'M','M','/','d','d','/','y','y',0};
    static const char cloc_long_date[] = "dddd, MMMM dd, yyyy";
    static const wchar_t cloc_long_dateW[] = {'d','d','d','d',',',' ','M','M','M','M',' ','d','d',',',' ','y','y','y','y',0};
    static const char cloc_time[] = "HH:mm:ss";
    static const wchar_t cloc_timeW[] = {'H','H',':','m','m',':','s','s',0};

    MSVCRT__locale_t loc;
    LCID lcid[6] = { 0 }, lcid_tmp;
    unsigned short cp[6] = { 0 };
    char buf[256];
    int i, ret, size;

    TRACE("(%d %s)\n", category, locale);

    if(category<MSVCRT_LC_MIN || category>MSVCRT_LC_MAX || !locale)
        return NULL;

    if(locale[0]=='C' && !locale[1]) {
        lcid[0] = 0;
        cp[0] = CP_ACP;
    } else if(!locale[0]) {
        lcid[0] = GetSystemDefaultLCID();
        GetLocaleInfoA(lcid[0], LOCALE_IDEFAULTANSICODEPAGE
                |LOCALE_NOUSEROVERRIDE, buf, sizeof(buf));
        cp[0] = atoi(buf);

        for(i=1; i<6; i++) {
            lcid[i] = lcid[0];
            cp[i] = cp[0];
        }
    } else if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_') {
        const char *p;

        while(1) {
            locale += 3; /* LC_ */
            if(!memcmp(locale, collate, sizeof(collate)-1)) {
                i = MSVCRT_LC_COLLATE;
                locale += sizeof(collate)-1;
            } else if(!memcmp(locale, ctype, sizeof(ctype)-1)) {
                i = MSVCRT_LC_CTYPE;
                locale += sizeof(ctype)-1;
            } else if(!memcmp(locale, monetary, sizeof(monetary)-1)) {
                i = MSVCRT_LC_MONETARY;
                locale += sizeof(monetary)-1;
            } else if(!memcmp(locale, numeric, sizeof(numeric)-1)) {
                i = MSVCRT_LC_NUMERIC;
                locale += sizeof(numeric)-1;
            } else if(!memcmp(locale, time, sizeof(time)-1)) {
                i = MSVCRT_LC_TIME;
                locale += sizeof(time)-1;
            } else
                return NULL;

            p = strchr(locale, ';');
            if(locale[0]=='C' && (locale[1]==';' || locale[1]=='\0')) {
                lcid[i] = 0;
                cp[i] = CP_ACP;
            } else if(p) {
                memcpy(buf, locale, p-locale);
                buf[p-locale] = '\0';
                lcid[i] = MSVCRT_locale_to_LCID(buf, &cp[i]);
            } else
                lcid[i] = MSVCRT_locale_to_LCID(locale, &cp[i]);

            if(lcid[i] == -1)
                return NULL;

            if(!p || *(p+1)!='L' || *(p+2)!='C' || *(p+3)!='_')
                break;

            locale = p+1;
        }
    } else {
        lcid[0] = MSVCRT_locale_to_LCID(locale, &cp[0]);
        if(lcid[0] == -1)
            return NULL;

        for(i=1; i<6; i++) {
            lcid[i] = lcid[0];
            cp[i] = cp[0];
        }
    }

    loc = MSVCRT_malloc(sizeof(MSVCRT__locale_tstruct));
    if(!loc)
        return NULL;

    loc->locinfo = MSVCRT_malloc(sizeof(MSVCRT_threadlocinfo));
    if(!loc->locinfo) {
        MSVCRT_free(loc);
        return NULL;
    }

    loc->mbcinfo = MSVCRT_malloc(sizeof(MSVCRT_threadmbcinfo));
    if(!loc->mbcinfo) {
        MSVCRT_free(loc->locinfo);
        MSVCRT_free(loc);
        return NULL;
    }

    memset(loc->locinfo, 0, sizeof(MSVCRT_threadlocinfo));
    loc->locinfo->refcount = 1;
    loc->mbcinfo->refcount = 1;

    loc->locinfo->lconv = MSVCRT_malloc(sizeof(struct MSVCRT_lconv));
    if(!loc->locinfo->lconv) {
        MSVCRT__free_locale(loc);
        return NULL;
    }
    memset(loc->locinfo->lconv, 0, sizeof(struct MSVCRT_lconv));

    loc->locinfo->pclmap = MSVCRT_malloc(sizeof(char[256]));
    loc->locinfo->pcumap = MSVCRT_malloc(sizeof(char[256]));
    if(!loc->locinfo->pclmap || !loc->locinfo->pcumap) {
        MSVCRT__free_locale(loc);
        return NULL;
    }

    if(lcid[MSVCRT_LC_COLLATE] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_COLLATE)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_COLLATE], cp[MSVCRT_LC_COLLATE], loc, MSVCRT_LC_COLLATE)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        loc->locinfo->lc_collate_cp = loc->locinfo->lc_id[MSVCRT_LC_COLLATE].wCodePage;
    } else
        loc->locinfo->lc_category[LC_COLLATE].locale = _strdup("C");

    if(lcid[MSVCRT_LC_CTYPE] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_CTYPE)) {
        CPINFO cp_info;
        int j;

        if(update_threadlocinfo_category(lcid[MSVCRT_LC_CTYPE], cp[MSVCRT_LC_CTYPE], loc, MSVCRT_LC_CTYPE)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        loc->locinfo->lc_codepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;
        loc->locinfo->lc_clike = 1;
        if(!GetCPInfo(loc->locinfo->lc_codepage, &cp_info)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }
        loc->locinfo->mb_cur_max = cp_info.MaxCharSize;

        loc->locinfo->ctype1_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->ctype1 = MSVCRT_malloc(sizeof(short[257]));
        if(!loc->locinfo->ctype1_refcount || !loc->locinfo->ctype1) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        *loc->locinfo->ctype1_refcount = 1;
        loc->locinfo->ctype1[0] = 0;
        loc->locinfo->pctype = loc->locinfo->ctype1+1;

        buf[1] = buf[2] = '\0';
        for(i=1; i<257; i++) {
            buf[0] = i-1;

            /* builtin GetStringTypeA doesn't set output to 0 on invalid input */
            loc->locinfo->ctype1[i] = 0;

            GetStringTypeA(lcid[MSVCRT_LC_CTYPE], CT_CTYPE1, buf,
                    1, loc->locinfo->ctype1+i);
        }

        for(i=0; cp_info.LeadByte[i+1]!=0; i+=2)
            for(j=cp_info.LeadByte[i]; j<=cp_info.LeadByte[i+1]; j++)
                loc->locinfo->ctype1[j+1] |= _LEADBYTE;
    } else {
        loc->locinfo->lc_clike = 1;
        loc->locinfo->mb_cur_max = 1;
        loc->locinfo->pctype = _ctype+1;
        loc->locinfo->lc_category[LC_CTYPE].locale = _strdup("C");
    }

    for(i=0; i<256; i++) {
        if(loc->locinfo->pctype[i] & _LEADBYTE)
            buf[i] = ' ';
        else
            buf[i] = i;

    }

    if(lcid[MSVCRT_LC_CTYPE]) {
        LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_LOWERCASE, buf, 256,
                (char*)loc->locinfo->pclmap, 256);
        LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_UPPERCASE, buf, 256,
                (char*)loc->locinfo->pcumap, 256);
    } else {
        for(i=0; i<256; i++) {
            loc->locinfo->pclmap[i] = (i>='A' && i<='Z' ? i-'A'+'a' : i);
            loc->locinfo->pcumap[i] = (i>='a' && i<='z' ? i-'a'+'A' : i);
        }
    }

    _setmbcp_l(loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage, lcid[MSVCRT_LC_CTYPE], loc->mbcinfo);

    if(lcid[MSVCRT_LC_MONETARY] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_MONETARY)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_MONETARY], cp[MSVCRT_LC_MONETARY], loc, MSVCRT_LC_MONETARY)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv_intl_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->lconv_mon_refcount = MSVCRT_malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_mon_refcount) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_mon_refcount = 1;

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SINTLSYMBOL
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->int_curr_symbol = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->int_curr_symbol, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SCURRENCY
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->currency_symbol = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->currency_symbol, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONDECIMALSEP
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->mon_decimal_point = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->mon_decimal_point, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONTHOUSANDSEP
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->mon_thousands_sep = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->mon_thousands_sep, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONGROUPING
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->mon_grouping = MSVCRT_malloc(i))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->mon_grouping[i/2+1] = 127;
        } else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SPOSITIVESIGN
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->positive_sign = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->positive_sign, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SNEGATIVESIGN
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->negative_sign = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->negative_sign, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IINTLCURRDIGITS
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->int_frac_digits = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_ICURRDIGITS
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->frac_digits = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSYMPRECEDES
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->p_cs_precedes = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSEPBYSPACE
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->p_sep_by_space = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSYMPRECEDES
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->n_cs_precedes = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSEPBYSPACE
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->n_sep_by_space = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSIGNPOSN
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->p_sign_posn = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSIGNPOSN
                    |LOCALE_NOUSEROVERRIDE, buf, 256))
            loc->locinfo->lconv->n_sign_posn = atoi(buf);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->int_curr_symbol = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->currency_symbol = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_decimal_point = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_thousands_sep = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_grouping = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->positive_sign = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->negative_sign = MSVCRT_malloc(sizeof(char));

        if(!loc->locinfo->lconv->int_curr_symbol || !loc->locinfo->lconv->currency_symbol
                || !loc->locinfo->lconv->mon_decimal_point || !loc->locinfo->lconv->mon_thousands_sep
                || !loc->locinfo->lconv->mon_grouping || !loc->locinfo->lconv->positive_sign
                || !loc->locinfo->lconv->negative_sign) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv->int_curr_symbol[0] = '\0';
        loc->locinfo->lconv->currency_symbol[0] = '\0';
        loc->locinfo->lconv->mon_decimal_point[0] = '\0';
        loc->locinfo->lconv->mon_thousands_sep[0] = '\0';
        loc->locinfo->lconv->mon_grouping[0] = '\0';
        loc->locinfo->lconv->positive_sign[0] = '\0';
        loc->locinfo->lconv->negative_sign[0] = '\0';
        loc->locinfo->lconv->int_frac_digits = 127;
        loc->locinfo->lconv->frac_digits = 127;
        loc->locinfo->lconv->p_cs_precedes = 127;
        loc->locinfo->lconv->p_sep_by_space = 127;
        loc->locinfo->lconv->n_cs_precedes = 127;
        loc->locinfo->lconv->n_sep_by_space = 127;
        loc->locinfo->lconv->p_sign_posn = 127;
        loc->locinfo->lconv->n_sign_posn = 127;

        loc->locinfo->lc_category[LC_MONETARY].locale = _strdup("C");
    }

    if(lcid[MSVCRT_LC_NUMERIC] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_NUMERIC)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_NUMERIC], cp[MSVCRT_LC_NUMERIC], loc, MSVCRT_LC_NUMERIC)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        if(!loc->locinfo->lconv_intl_refcount)
            loc->locinfo->lconv_intl_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->lconv_num_refcount = MSVCRT_malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_num_refcount) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_num_refcount = 1;

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SDECIMAL
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->decimal_point = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->decimal_point, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_STHOUSAND
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i && (loc->locinfo->lconv->thousands_sep = MSVCRT_malloc(i)))
            memcpy(loc->locinfo->lconv->thousands_sep, buf, i);
        else {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SGROUPING
                |LOCALE_NOUSEROVERRIDE, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->grouping = MSVCRT_malloc(i))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->grouping[i/2+1] = 127;
        } else {
            MSVCRT__free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->decimal_point = MSVCRT_malloc(sizeof(char[2]));
        loc->locinfo->lconv->thousands_sep = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->grouping = MSVCRT_malloc(sizeof(char));
        if(!loc->locinfo->lconv->decimal_point || !loc->locinfo->lconv->thousands_sep
                || !loc->locinfo->lconv->grouping) {
            MSVCRT__free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv->decimal_point[0] = '.';
        loc->locinfo->lconv->decimal_point[1] = '\0';
        loc->locinfo->lconv->thousands_sep[0] = '\0';
        loc->locinfo->lconv->grouping[0] = '\0';

        loc->locinfo->lc_category[LC_NUMERIC].locale = _strdup("C");
    }

    if(lcid[MSVCRT_LC_TIME] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_TIME)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_TIME], cp[MSVCRT_LC_TIME], loc, MSVCRT_LC_TIME)) {
            MSVCRT__free_locale(loc);
            return NULL;
        }
    } else
        loc->locinfo->lc_category[LC_TIME].locale = _strdup("C");

    size = sizeof(MSVCRT___lc_time_data);
    lcid_tmp = lcid[MSVCRT_LC_TIME] ? lcid[MSVCRT_LC_TIME] : MAKELCID(LANG_ENGLISH, SORT_DEFAULT);
    for(i=0; i<sizeof(time_data)/sizeof(time_data[0]); i++) {
        if(time_data[i]==LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
            size += sizeof(cloc_short_date) + sizeof(cloc_short_dateW);
        }else if(time_data[i]==LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
            size += sizeof(cloc_long_date) + sizeof(cloc_long_dateW);
        }else {
            ret = GetLocaleInfoA(lcid_tmp, time_data[i]
                    |LOCALE_NOUSEROVERRIDE, NULL, 0);
            if(!ret) {
                MSVCRT__free_locale(loc);
                return NULL;
            }
            size += ret;

            ret = GetLocaleInfoW(lcid_tmp, time_data[i]
                    |LOCALE_NOUSEROVERRIDE, NULL, 0);
            if(!ret) {
                MSVCRT__free_locale(loc);
                return NULL;
            }
            size += ret*sizeof(wchar_t);
        }
    }

    loc->locinfo->lc_time_curr = MSVCRT_malloc(size);
    if(!loc->locinfo->lc_time_curr) {
        MSVCRT__free_locale(loc);
        return NULL;
    }

    ret = 0;
    for(i=0; i<sizeof(time_data)/sizeof(time_data[0]); i++) {
        loc->locinfo->lc_time_curr->str.str[i] = &loc->locinfo->lc_time_curr->data[ret];
        if(time_data[i]==LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_short_date, sizeof(cloc_short_date));
            ret += sizeof(cloc_short_date);
        }else if(time_data[i]==LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_long_date, sizeof(cloc_long_date));
            ret += sizeof(cloc_long_date);
        }else if(time_data[i]==LOCALE_STIMEFORMAT && !lcid[MSVCRT_LC_TIME]) {
            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_time, sizeof(cloc_time));
            ret += sizeof(cloc_time);
        }else {
            ret += GetLocaleInfoA(lcid_tmp, time_data[i]|LOCALE_NOUSEROVERRIDE,
                    &loc->locinfo->lc_time_curr->data[ret], size-ret);
        }
    }
    for(i=0; i<sizeof(time_data)/sizeof(time_data[0]); i++) {
        loc->locinfo->lc_time_curr->wstr[i] = (wchar_t*)&loc->locinfo->lc_time_curr->data[ret];
        if(time_data[i]==LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_short_dateW, sizeof(cloc_short_dateW));
            ret += sizeof(cloc_short_dateW);
        }else if(time_data[i]==LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_long_dateW, sizeof(cloc_long_dateW));
            ret += sizeof(cloc_long_dateW);
        }else if(time_data[i]==LOCALE_STIMEFORMAT && !lcid[MSVCRT_LC_TIME]) {            memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_timeW, sizeof(cloc_timeW));
            ret += sizeof(cloc_timeW);
        }else {
            ret += GetLocaleInfoW(lcid_tmp, time_data[i]|LOCALE_NOUSEROVERRIDE,
                    (wchar_t*)&loc->locinfo->lc_time_curr->data[ret], size-ret)*sizeof(wchar_t);
        }
    }
    loc->locinfo->lc_time_curr->lcid = lcid[MSVCRT_LC_TIME];

    return loc;
}

/*********************************************************************
 *             setlocale (MSVCRT.@)
 */
char* CDECL setlocale(int category, const char* locale)
{
    MSVCRT__locale_t loc;
    MSVCRT_pthreadlocinfo locinfo = get_locinfo();

    if(category<MSVCRT_LC_MIN || category>MSVCRT_LC_MAX)
        return NULL;

    if(!locale) {
        if(category == MSVCRT_LC_ALL)
            return construct_lc_all(locinfo);

        return locinfo->lc_category[category].locale;
    }

    loc = MSVCRT__create_locale(category, locale);
    if(!loc) {
        WARN("%d %s failed\n", category, locale);
        return NULL;
    }

    LOCK_LOCALE;

    switch(category) {
        case MSVCRT_LC_ALL:
        case MSVCRT_LC_COLLATE:
            locinfo->lc_collate_cp = loc->locinfo->lc_collate_cp;
            locinfo->lc_handle[MSVCRT_LC_COLLATE] =
                loc->locinfo->lc_handle[MSVCRT_LC_COLLATE];
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_COLLATE].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_COLLATE].locale);
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_COLLATE].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_COLLATE].refcount);

            if(category != MSVCRT_LC_ALL)
                break;
            /* fall through */
        case MSVCRT_LC_CTYPE:
            locinfo->lc_handle[MSVCRT_LC_CTYPE] =
                loc->locinfo->lc_handle[MSVCRT_LC_CTYPE];
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_CTYPE].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_CTYPE].locale);
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_CTYPE].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_CTYPE].refcount);

            locinfo->lc_codepage = loc->locinfo->lc_codepage;
            locinfo->lc_clike = loc->locinfo->lc_clike;
            locinfo->mb_cur_max = loc->locinfo->mb_cur_max;

            swap_pointers((void**)&locinfo->ctype1_refcount,
                    (void**)&loc->locinfo->ctype1_refcount);
            swap_pointers((void**)&locinfo->ctype1, (void**)&loc->locinfo->ctype1);
            swap_pointers((void**)&locinfo->pctype, (void**)&loc->locinfo->pctype);
            swap_pointers((void**)&locinfo->pclmap, (void**)&loc->locinfo->pclmap);
            swap_pointers((void**)&locinfo->pcumap, (void**)&loc->locinfo->pcumap);

            if(category != MSVCRT_LC_ALL)
                break;
            /* fall through */
        case MSVCRT_LC_MONETARY:
            locinfo->lc_handle[MSVCRT_LC_MONETARY] =
                loc->locinfo->lc_handle[MSVCRT_LC_MONETARY];
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_MONETARY].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_MONETARY].locale);
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_MONETARY].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_MONETARY].refcount);

            swap_pointers((void**)&locinfo->lconv->int_curr_symbol,
                    (void**)&loc->locinfo->lconv->int_curr_symbol);
            swap_pointers((void**)&locinfo->lconv->currency_symbol,
                    (void**)&loc->locinfo->lconv->currency_symbol);
            swap_pointers((void**)&locinfo->lconv->mon_decimal_point,
                    (void**)&loc->locinfo->lconv->mon_decimal_point);
            swap_pointers((void**)&locinfo->lconv->mon_thousands_sep,
                    (void**)&loc->locinfo->lconv->mon_thousands_sep);
            swap_pointers((void**)&locinfo->lconv->mon_grouping,
                    (void**)&loc->locinfo->lconv->mon_grouping);
            swap_pointers((void**)&locinfo->lconv->positive_sign,
                    (void**)&loc->locinfo->lconv->positive_sign);
            swap_pointers((void**)&locinfo->lconv->negative_sign,
                    (void**)&loc->locinfo->lconv->negative_sign);
            locinfo->lconv->int_frac_digits = loc->locinfo->lconv->int_frac_digits;
            locinfo->lconv->frac_digits = loc->locinfo->lconv->frac_digits;
            locinfo->lconv->p_cs_precedes = loc->locinfo->lconv->p_cs_precedes;
            locinfo->lconv->p_sep_by_space = loc->locinfo->lconv->p_sep_by_space;
            locinfo->lconv->n_cs_precedes = loc->locinfo->lconv->n_cs_precedes;
            locinfo->lconv->n_sep_by_space = loc->locinfo->lconv->n_sep_by_space;
            locinfo->lconv->p_sign_posn = loc->locinfo->lconv->p_sign_posn;
            locinfo->lconv->n_sign_posn = loc->locinfo->lconv->n_sign_posn;

            if(category != MSVCRT_LC_ALL)
                break;
            /* fall through */
        case MSVCRT_LC_NUMERIC:
            locinfo->lc_handle[MSVCRT_LC_NUMERIC] =
                loc->locinfo->lc_handle[MSVCRT_LC_NUMERIC];
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_NUMERIC].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale);
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_NUMERIC].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].refcount);

            swap_pointers((void**)&locinfo->lconv->decimal_point,
                    (void**)&loc->locinfo->lconv->decimal_point);
            swap_pointers((void**)&locinfo->lconv->thousands_sep,
                    (void**)&loc->locinfo->lconv->thousands_sep);
            swap_pointers((void**)&locinfo->lconv->grouping,
                    (void**)&loc->locinfo->lconv->grouping);

            if(category != MSVCRT_LC_ALL)
                break;
            /* fall through */
        case MSVCRT_LC_TIME:
            locinfo->lc_handle[MSVCRT_LC_TIME] =
                loc->locinfo->lc_handle[MSVCRT_LC_TIME];
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_TIME].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_TIME].locale);
            swap_pointers((void**)&locinfo->lc_category[MSVCRT_LC_TIME].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_TIME].refcount);
            swap_pointers((void**)&locinfo->lc_time_curr,
                    (void**)&loc->locinfo->lc_time_curr);

            if(category != MSVCRT_LC_ALL)
                break;
    }

    MSVCRT__free_locale(loc);
    UNLOCK_LOCALE;

    if(locinfo == MSVCRT_locale->locinfo) {
        int i;

        __lc_codepage = locinfo->lc_codepage;
        MSVCRT___lc_collate_cp = locinfo->lc_collate_cp;
        __mb_cur_max = locinfo->mb_cur_max;
        _pctype = locinfo->pctype;
        for(i=LC_MIN; i<=LC_MAX; i++)
            MSVCRT___lc_handle[i] = MSVCRT_locale->locinfo->lc_handle[i];
    }

    if(category == MSVCRT_LC_ALL)
        return construct_lc_all(locinfo);

    _Analysis_assume_(category <= 5);
    return locinfo->lc_category[category].locale;
}

/* _configthreadlocale - not exported in native msvcrt */
int CDECL _configthreadlocale(int type)
{
    thread_data_t *data = msvcrt_get_thread_data();
    MSVCRT__locale_t locale;
    int ret;

    if(!data)
        return -1;

    ret = (data->have_locale ? _ENABLE_PER_THREAD_LOCALE : _DISABLE_PER_THREAD_LOCALE);

    if(type == _ENABLE_PER_THREAD_LOCALE) {
        if(!data->have_locale) {
            /* Copy current global locale */
            locale = MSVCRT__create_locale(LC_ALL, setlocale(LC_ALL, NULL));
            if(!locale)
                return -1;

            data->locinfo = locale->locinfo;
            data->mbcinfo = locale->mbcinfo;
            data->have_locale = TRUE;
            MSVCRT_free(locale);
        }

        return ret;
    }

    if(type == _DISABLE_PER_THREAD_LOCALE) {
        if(data->have_locale) {
            free_locinfo(data->locinfo);
            free_mbcinfo(data->mbcinfo);
            data->locinfo = MSVCRT_locale->locinfo;
            data->mbcinfo = MSVCRT_locale->mbcinfo;
            data->have_locale = FALSE;
        }

        return ret;
    }

    if(!type)
        return ret;

    return -1;
}

/*********************************************************************
 *         _getmbcp (MSVCRT.@)
 */
int CDECL _getmbcp(void)
{
  return get_mbcinfo()->mbcodepage;
}

extern unsigned int __setlc_active;
/*********************************************************************
 *         ___setlc_active_func (MSVCRT.@)
 */
unsigned int CDECL ___setlc_active_func(void)
{
  return __setlc_active;
}

extern unsigned int __unguarded_readlc_active;
/*********************************************************************
 *         ___unguarded_readlc_active_add_func (MSVCRT.@)
 */
unsigned int * CDECL ___unguarded_readlc_active_add_func(void)
{
  return &__unguarded_readlc_active;
}

MSVCRT__locale_t global_locale = NULL;
void __init_global_locale()
{
    unsigned i;

    LOCK_LOCALE;
    /* Someone created it before us */
    if(global_locale)
        return;
    global_locale = MSVCRT__create_locale(0, "C");

    __lc_codepage = MSVCRT_locale->locinfo->lc_codepage;
    MSVCRT___lc_collate_cp = MSVCRT_locale->locinfo->lc_collate_cp;
    __mb_cur_max = MSVCRT_locale->locinfo->mb_cur_max;
    for(i=LC_MIN; i<=LC_MAX; i++)
        MSVCRT___lc_handle[i] = MSVCRT_locale->locinfo->lc_handle[i];
    _setmbcp(_MB_CP_ANSI);
    UNLOCK_LOCALE;
}

/*
 * @implemented
 */
const unsigned short **__p__pctype(void)
{
   return &get_locinfo()->pctype;
}

const unsigned short* __cdecl __pctype_func(void)
{
   return get_locinfo()->pctype;
}

