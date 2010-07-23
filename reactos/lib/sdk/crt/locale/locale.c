/*
 * Some stuff takem from wine msvcrt\locale.c
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
#include <internal/wine/msvcrt.h>
#include "mbctype.h"
#include "mbstring.h"

// mtdll.h
#define _SETLOCALE_LOCK 19

// msvcrt.h
#define MSVCRT_LC_ALL          0
#define MSVCRT_LC_COLLATE      1
#define MSVCRT_LC_CTYPE        2
#define MSVCRT_LC_MONETARY     3
#define MSVCRT_LC_NUMERIC      4
#define MSVCRT_LC_TIME         5
#define MSVCRT_LC_MIN          MSVCRT_LC_ALL
#define MSVCRT_LC_MAX          MSVCRT_LC_TIME

/* FIXME: Need to hold locale for each LC_* type and aggregate
 * string to produce lc_all.
 */
#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256
_locale_t MSVCRT_locale = NULL;
unsigned char _mbctype[257] = { 0 };
int g_mbcp_is_multibyte = 0;
char MSVCRT_current_lc_all[MAX_LOCALE_LENGTH] = { 0 };
LCID MSVCRT_current_lc_all_lcid = 0;
int MSVCRT___lc_codepage = 0;
int MSVCRT___lc_collate_cp = 0;
HANDLE MSVCRT___lc_handle[MSVCRT_LC_MAX - MSVCRT_LC_MIN + 1] = { 0 };
unsigned char charmax = CHAR_MAX;

/* MT */
#define LOCK_LOCALE   _mlock(_SETLOCALE_LOCK);
#define UNLOCK_LOCALE _munlock(_SETLOCALE_LOCK);

#define MSVCRT_LEADBYTE  0x8000
#define MSVCRT_C1_DEFINED 0x200

unsigned int __setlc_active;
unsigned int __unguarded_readlc_active;
int _current_category;	/* used by setlocale */
const char *_current_locale;
/* Friendly country strings & iso codes for synonym support.
 * Based on MS documentation for setlocale().
 */
static const char * const _country_synonyms[] =
{
  "Hong Kong","HK",
  "Hong-Kong","HK",
  "New Zealand","NZ",
  "New-Zealand","NZ",
  "PR China","CN",
  "PR-China","CN",
  "United Kingdom","GB",
  "United-Kingdom","GB",
  "Britain","GB",
  "England","GB",
  "Great Britain","GB",
  "United States","US",
  "United-States","US",
  "America","US"
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
      name[0] = _country_synonyms[i+1][0];
      name[1] = _country_synonyms[i+1][1];
      name[2] = '\0';
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
  char found_language[MAX_ELEM_LEN];
  char found_country[MAX_ELEM_LEN];
  char found_codepage[MAX_ELEM_LEN];
  unsigned int match_flags;
  LANGID found_lang_id;
} locale_search_t;

#define CONTINUE_LOOKING TRUE
#define STOP_LOOKING     FALSE

/* INTERNAL: Get and compare locale info with a given string */
static int compare_info(LCID lcid, DWORD flags, char* buff, const char* cmp)
{
  buff[0] = 0;
  GetLocaleInfoA(lcid, flags|LOCALE_NOUSEROVERRIDE,buff, MAX_ELEM_LEN);
  if (!buff[0] || !cmp[0])
    return 0;
  /* Partial matches are allowed, e.g. "Germ" matches "Germany" */
  return !strncasecmp(cmp, buff, strlen(cmp));
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
  if (compare_info(lcid,LOCALE_SISO639LANGNAME,buff,res->search_language) ||
      compare_info(lcid,LOCALE_SABBREVLANGNAME,buff,res->search_language) ||
      compare_info(lcid,LOCALE_SENGLANGUAGE,buff,res->search_language))
  {
    TRACE(":Found language: %s->%s\n", res->search_language, buff);
    flags |= FOUND_LANGUAGE;
    memcpy(res->found_language,res->search_language,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_LANGUAGE)
  {
    return CONTINUE_LOOKING;
  }

  /* Check Country */
  if (compare_info(lcid,LOCALE_SISO3166CTRYNAME,buff,res->search_country) ||
      compare_info(lcid,LOCALE_SABBREVCTRYNAME,buff,res->search_country) ||
      compare_info(lcid,LOCALE_SENGCOUNTRY,buff,res->search_country))
  {
    TRACE("Found country:%s->%s\n", res->search_country, buff);
    flags |= FOUND_COUNTRY;
    memcpy(res->found_country,res->search_country,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_COUNTRY)
  {
    return CONTINUE_LOOKING;
  }

  /* Check codepage */
  if (compare_info(lcid,LOCALE_IDEFAULTCODEPAGE,buff,res->search_codepage) ||
      (compare_info(lcid,LOCALE_IDEFAULTANSICODEPAGE,buff,res->search_codepage)))
  {
    TRACE("Found codepage:%s->%s\n", res->search_codepage, buff);
    flags |= FOUND_CODEPAGE;
    memcpy(res->found_codepage,res->search_codepage,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_CODEPAGE)
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
static LCID MSVCRT_locale_to_LCID(const char *locale)
{
    LCID lcid;
    locale_search_t search;
    char *cp, *region;

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
        if(cp-region-1 < MAX_ELEM_LEN)
          search.search_country[cp-region-1] = '\0';
        if(cp-locale < MAX_ELEM_LEN)
            search.search_language[cp-locale] = '\0';
    } else
        search.search_codepage[0] = '\0';

    /* FIXME:  MSVCRT_locale_to_LCID is not finding remaped values */
    remap_synonym(search.search_country);

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
    if (!search.found_codepage[0]) {
        /* Even if a codepage is not enumerated for a locale
         * it can be set if valid */
        if (search.search_codepage[0]) {
            if (IsValidCodePage(atoi(search.search_codepage)))
                memcpy(search.found_codepage,search.search_codepage,MAX_ELEM_LEN);
            else {
                /* Special codepage values: OEM & ANSI */
                if (strcasecmp(search.search_codepage,"OCP")) {
                    GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                            search.found_codepage, MAX_ELEM_LEN);
                } else if (strcasecmp(search.search_codepage,"ACP")) {
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

    GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE|LOCALE_NOUSEROVERRIDE,
            search.found_language, MAX_ELEM_LEN);
    GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY|LOCALE_NOUSEROVERRIDE,
            search.found_country, MAX_ELEM_LEN);
    return lcid;
}

/* INTERNAL: Set lc_handle, lc_id and lc_category in threadlocinfo struct */
static BOOL update_threadlocinfo_category(LCID lcid, _locale_t loc, int category)
{
    char buf[256], *p;
    int len;

    if(GetLocaleInfoA(lcid, LOCALE_ILANGUAGE, buf, 256)) {
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

    if(GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, buf, 256))
        loc->locinfo->lc_id[category].wCodePage = atoi(buf);

    loc->locinfo->lc_handle[category] = lcid;

    len = 0;
    len += GetLocaleInfoA(lcid, LOCALE_SLANGUAGE, buf, 256);
    buf[len-1] = '_';
    len += GetLocaleInfoA(lcid, LOCALE_SCOUNTRY, &buf[len], 256-len);
    buf[len-1] = '.';
    len += GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, &buf[len], 256-len);

    loc->locinfo->lc_category[category].locale = malloc(sizeof(char[len]));
    loc->locinfo->lc_category[category].refcount = malloc(sizeof(int));
    if(!loc->locinfo->lc_category[category].locale
            || !loc->locinfo->lc_category[category].refcount) {
        free(loc->locinfo->lc_category[category].locale);
        free(loc->locinfo->lc_category[category].refcount);
        loc->locinfo->lc_category[category].locale = NULL;
        loc->locinfo->lc_category[category].refcount = NULL;
        return TRUE;
    }
    memcpy(loc->locinfo->lc_category[category].locale, buf, sizeof(char[len]));
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

/* INTERNAL: returns _locale_t struct for current locale */
_locale_t get_locale(void) {
    MSVCRT_thread_data *data = msvcrt_get_thread_data();

    if(!data || !data->locale)
        return MSVCRT_locale;

    return data->locale;
}

/* INTERNAL: constructs string returned by setlocale */
static inline char* construct_lc_all(_locale_t cur) {
    static char current_lc_all[MAX_LOCALE_LENGTH];

    int i;

    for(i=MSVCRT_LC_MIN+1; i<MSVCRT_LC_MAX; i++) {
        if(strcmp(cur->locinfo->lc_category[i].locale,
                    cur->locinfo->lc_category[i+1].locale))
            break;
    }

    if(i==MSVCRT_LC_MAX)
        return cur->locinfo->lc_category[MSVCRT_LC_COLLATE].locale;

    sprintf(current_lc_all,
            "LC_COLLATE=%s;LC_CTYPE=%s;LC_MONETARY=%s;LC_NUMERIC=%s;LC_TIME=%s",
            cur->locinfo->lc_category[MSVCRT_LC_COLLATE].locale,
            cur->locinfo->lc_category[MSVCRT_LC_CTYPE].locale,
            cur->locinfo->lc_category[MSVCRT_LC_MONETARY].locale,
            cur->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale,
            cur->locinfo->lc_category[MSVCRT_LC_TIME].locale);

    return current_lc_all;
}


/*********************************************************************
 *		wsetlocale (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wsetlocale(int category, const MSVCRT_wchar_t* locale)
{
  static MSVCRT_wchar_t fake[] = {
    'E','n','g','l','i','s','h','_','U','n','i','t','e','d',' ',
    'S','t','a','t','e','s','.','1','2','5','2',0 };

  FIXME("%d %s\n", category, debugstr_w(locale));

  return fake;
}

/*********************************************************************
 *		_Getdays (MSVCRT.@)
 */
const char* CDECL _Getdays(void)
{
  static const char MSVCRT_days[] = ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:"
                            "Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
  /* FIXME: Use locale */
  TRACE("(void) semi-stub\n");
  return MSVCRT_days;
}

/*********************************************************************
 *		_Getmonths (MSVCRT.@)
 */
const char* CDECL _Getmonths(void)
{
  static const char MSVCRT_months[] = ":Jan:January:Feb:February:Mar:March:Apr:"
                "April:May:May:Jun:June:Jul:July:Aug:August:Sep:September:Oct:"
                "October:Nov:November:Dec:December";
  /* FIXME: Use locale */
  TRACE("(void) semi-stub\n");
  return MSVCRT_months;
}

/*********************************************************************
 *		_Gettnames (MSVCRT.@)
 */
const char* CDECL _Gettnames(void)
{
  /* FIXME: */
  TRACE("(void) stub\n");
  return "";
}

/*********************************************************************
 *		_Strftime (MSVCRT.@)
 */
const char* CDECL _Strftime(char *out, unsigned int len, const char *fmt,
                            const void *tm, void *foo)
{
  /* FIXME: */
  TRACE("(%p %d %s %p %p) stub\n", out, len, fmt, tm, foo);
  return "";
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
int CDECL __crtCompareStringW( LCID lcid, DWORD flags, const MSVCRT_wchar_t *src1, int len1,
                               const MSVCRT_wchar_t *src2, int len2 )
{
    FIXME("(lcid %x, flags %x, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_w(src1), len1, debugstr_w(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringW( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtGetLocaleInfoW (MSVCRT.@)
 */
int CDECL __crtGetLocaleInfoW( LCID lcid, LCTYPE type, MSVCRT_wchar_t *buffer, int len )
{
    FIXME("(lcid %x, type %x, %p(%d), partial stub\n", lcid, type, buffer, len );
    /* FIXME: probably not entirely right */
    return GetLocaleInfoW( lcid, type, buffer, len );
}

/*********************************************************************
 *              btowc(MSVCRT.@)
 */
wint_t CDECL btowc(int c)
{
    _locale_t locale = get_locale();
    unsigned char letter = c;
    wchar_t ret;

    if(!MultiByteToWideChar(locale->locinfo->lc_handle[MSVCRT_LC_CTYPE],
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
        MSVCRT_wchar_t *buffer, int len, WORD *out)
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
    return get_locale()->locinfo->lconv;
}

/*********************************************************************
 *		__lconv_init (MSVCRT.@)
 */
void CDECL __lconv_init(void)
{
    /* this is used to make chars unsigned */
    charmax = 255;
}

/*********************************************************************
 *      ___lc_handle_func (MSVCRT.@)
 */
HANDLE * CDECL ___lc_handle_func(void)
{
    return MSVCRT___lc_handle;
}

/*********************************************************************
 *      ___lc_codepage_func (MSVCRT.@)
 */
int CDECL ___lc_codepage_func(void)
{
    return MSVCRT___lc_codepage;
}

/*********************************************************************
 *      ___lc_collate_cp_func (MSVCRT.@)
 */
int CDECL ___lc_collate_cp_func(void)
{
    return get_locale()->locinfo->lc_collate_cp;
}

/* _free_locale - not exported in native msvcrt */
void CDECL _free_locale(_locale_t locale)
{
    int i;

    for(i=MSVCRT_LC_MIN+1; i<=MSVCRT_LC_MAX; i++) {
        free(locale->locinfo->lc_category[i].locale);
        free(locale->locinfo->lc_category[i].refcount);
    }

    if(locale->locinfo->lconv) {
        free(locale->locinfo->lconv->decimal_point);
        free(locale->locinfo->lconv->thousands_sep);
        free(locale->locinfo->lconv->grouping);
        free(locale->locinfo->lconv->int_curr_symbol);
        free(locale->locinfo->lconv->currency_symbol);
        free(locale->locinfo->lconv->mon_decimal_point);
        free(locale->locinfo->lconv->mon_thousands_sep);
        free(locale->locinfo->lconv->mon_grouping);
        free(locale->locinfo->lconv->positive_sign);
        free(locale->locinfo->lconv->negative_sign);
    }
    free(locale->locinfo->lconv_intl_refcount);
    free(locale->locinfo->lconv_num_refcount);
    free(locale->locinfo->lconv_mon_refcount);
    free(locale->locinfo->lconv);

    free(locale->locinfo->ctype1_refcount);
    free(locale->locinfo->ctype1);

    free((void*)locale->locinfo->pclmap);
    free((void*)locale->locinfo->pcumap);

    free(locale->locinfo);
    free(locale->mbcinfo);
    free(locale);
}

/* _create_locale - not exported in native msvcrt */
_locale_t _create_locale(int category, const char *locale)
{
    static const char collate[] = "COLLATE=";
    static const char ctype[] = "CTYPE=";
    static const char monetary[] = "MONETARY=";
    static const char numeric[] = "NUMERIC=";
    static const char time[] = "TIME=";

    _locale_t loc;
    LCID lcid[6] = { 0 };
    char buf[256];
    int i;

    TRACE("(%d %s)\n", category, locale);

    if(category<MSVCRT_LC_MIN || category>MSVCRT_LC_MAX || !locale)
        return NULL;

    if(locale[0]=='C' && !locale[1])
        lcid[0] = CP_ACP;
    else if(!locale[0])
        lcid[0] = GetSystemDefaultLCID();
    else if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_') {
        char *p;

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
            if(locale[0]=='C' && (locale[1]==';' || locale[1]=='\0'))
                lcid[i] = 0;
            else if(p) {
                memcpy(buf, locale, p-locale);
                lcid[i] = MSVCRT_locale_to_LCID(buf);
            } else
                lcid[i] = MSVCRT_locale_to_LCID(locale);

            if(lcid[i] == -1)
                return NULL;

            if(!p || *(p+1)!='L' || *(p+2)!='C' || *(p+3)!='_')
                break;

            locale = p+1;
        }
    } else {
        lcid[0] = MSVCRT_locale_to_LCID(locale);
        if(lcid[0] == -1)
            return NULL;
    }

    for(i=1; i<6; i++) {
        if(!lcid[i])
            lcid[i] = lcid[0];
    }

    loc = malloc(sizeof(_locale_tstruct));
    if(!loc)
        return NULL;

    loc->locinfo = malloc(sizeof(threadlocinfo));
    if(!loc->locinfo) {
        free(loc);
        return NULL;
    }

    loc->mbcinfo = malloc(sizeof(threadmbcinfo));
    if(!loc->mbcinfo) {
        free(loc->locinfo);
        free(loc);
        return NULL;
    }

    memset(loc->locinfo, 0, sizeof(threadlocinfo));
    memset(loc->mbcinfo, 0, sizeof(threadmbcinfo));

    loc->locinfo->lconv = malloc(sizeof(struct lconv));
    if(!loc->locinfo->lconv) {
        _free_locale(loc);
        return NULL;
    }
    memset(loc->locinfo->lconv, 0, sizeof(struct lconv));

    loc->locinfo->pclmap = malloc(sizeof(char[256]));
    loc->locinfo->pcumap = malloc(sizeof(char[256]));
    if(!loc->locinfo->pclmap || !loc->locinfo->pcumap) {
        _free_locale(loc);
        return NULL;
    }

    loc->locinfo->refcount = 1;

    if(lcid[MSVCRT_LC_COLLATE] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_COLLATE)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_COLLATE], loc, MSVCRT_LC_COLLATE)) {
            _free_locale(loc);
            return NULL;
        }
    } else
        loc->locinfo->lc_category[MSVCRT_LC_COLLATE].locale = _strdup("C");

    if(lcid[MSVCRT_LC_CTYPE] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_CTYPE)) {
        CPINFO cp;

        if(update_threadlocinfo_category(lcid[MSVCRT_LC_CTYPE], loc, MSVCRT_LC_CTYPE)) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lc_codepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;
        loc->locinfo->lc_collate_cp = loc->locinfo->lc_codepage;
        loc->locinfo->lc_clike = 1;
        if(!GetCPInfo(loc->locinfo->lc_codepage, &cp)) {
            _free_locale(loc);
            return NULL;
        }
        loc->locinfo->mb_cur_max = cp.MaxCharSize;

        loc->locinfo->ctype1_refcount = malloc(sizeof(int));
        loc->locinfo->ctype1 = malloc(sizeof(short[257]));
        if(!loc->locinfo->ctype1_refcount || !loc->locinfo->ctype1) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->ctype1_refcount = 1;
        loc->locinfo->ctype1[0] = 0;
        loc->locinfo->pctype = loc->locinfo->ctype1+1;

        buf[1] = buf[2] = '\0';
        for(i=1; i<257; i++) {
            buf[0] = i-1;

            GetStringTypeA(lcid[MSVCRT_LC_CTYPE], CT_CTYPE1, buf,
                    1, loc->locinfo->ctype1+i);
            loc->locinfo->ctype1[i] |= 0x200;
        }
    } else {
        loc->locinfo->lc_clike = 1;
        loc->locinfo->mb_cur_max = 1;
        loc->locinfo->pctype = (unsigned short*)ctype+1;
        loc->locinfo->lc_category[MSVCRT_LC_CTYPE].locale = _strdup("C");
    }

    for(i=0; i<256; i++)
        buf[i] = i;

    LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_LOWERCASE, buf, 256,
            (char*)loc->locinfo->pclmap, 256);
    LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_UPPERCASE, buf, 256,
            (char*)loc->locinfo->pcumap, 256);

    loc->mbcinfo->refcount = 1;
    loc->mbcinfo->mbcodepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;

    for(i=0; i<256; i++) {
        if(loc->locinfo->pclmap[i] != i) {
            loc->mbcinfo->mbctype[i+1] |= 0x10;
            loc->mbcinfo->mbcasemap[i] = loc->locinfo->pclmap[i];
        } else if(loc->locinfo->pcumap[i] != i) {
            loc->mbcinfo->mbctype[i+1] |= 0x20;
            loc->mbcinfo->mbcasemap[i] = loc->locinfo->pcumap[i];
        }
    }

    if(lcid[MSVCRT_LC_MONETARY] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_MONETARY)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_MONETARY], loc, MSVCRT_LC_MONETARY)) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv_intl_refcount = malloc(sizeof(int));
        loc->locinfo->lconv_mon_refcount = malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_mon_refcount) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_mon_refcount = 1;

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SINTLSYMBOL, buf, 256);
        if(i && (loc->locinfo->lconv->int_curr_symbol = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->int_curr_symbol, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SCURRENCY, buf, 256);
        if(i && (loc->locinfo->lconv->currency_symbol = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->currency_symbol, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONDECIMALSEP, buf, 256);
        if(i && (loc->locinfo->lconv->mon_decimal_point = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->mon_decimal_point, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONTHOUSANDSEP, buf, 256);
        if(i && (loc->locinfo->lconv->mon_thousands_sep = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->mon_thousands_sep, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONGROUPING, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->mon_grouping = malloc(sizeof(char[i])))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->mon_grouping[i/2+1] = 127;
        } else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SPOSITIVESIGN, buf, 256);
        if(i && (loc->locinfo->lconv->positive_sign = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->positive_sign, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SNEGATIVESIGN, buf, 256);
        if(i && (loc->locinfo->lconv->negative_sign = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->negative_sign, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IINTLCURRDIGITS, buf, 256))
            loc->locinfo->lconv->int_frac_digits = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_ICURRDIGITS, buf, 256))
            loc->locinfo->lconv->frac_digits = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSYMPRECEDES, buf, 256))
            loc->locinfo->lconv->p_cs_precedes = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSEPBYSPACE, buf, 256))
            loc->locinfo->lconv->p_sep_by_space = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSYMPRECEDES, buf, 256))
            loc->locinfo->lconv->n_cs_precedes = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSEPBYSPACE, buf, 256))
            loc->locinfo->lconv->n_sep_by_space = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSIGNPOSN, buf, 256))
            loc->locinfo->lconv->p_sign_posn = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSIGNPOSN, buf, 256))
            loc->locinfo->lconv->n_sign_posn = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->int_curr_symbol = malloc(sizeof(char));
        loc->locinfo->lconv->currency_symbol = malloc(sizeof(char));
        loc->locinfo->lconv->mon_decimal_point = malloc(sizeof(char));
        loc->locinfo->lconv->mon_thousands_sep = malloc(sizeof(char));
        loc->locinfo->lconv->mon_grouping = malloc(sizeof(char));
        loc->locinfo->lconv->positive_sign = malloc(sizeof(char));
        loc->locinfo->lconv->negative_sign = malloc(sizeof(char));

        if(!loc->locinfo->lconv->int_curr_symbol || !loc->locinfo->lconv->currency_symbol
                || !loc->locinfo->lconv->mon_decimal_point || !loc->locinfo->lconv->mon_thousands_sep
                || !loc->locinfo->lconv->mon_grouping || !loc->locinfo->lconv->positive_sign
                || !loc->locinfo->lconv->negative_sign) {
            _free_locale(loc);
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

        loc->locinfo->lc_category[MSVCRT_LC_MONETARY].locale = _strdup("C");
    }

    if(lcid[MSVCRT_LC_NUMERIC] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_NUMERIC)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_NUMERIC], loc, MSVCRT_LC_NUMERIC)) {
            _free_locale(loc);
            return NULL;
        }

        if(!loc->locinfo->lconv_intl_refcount)
            loc->locinfo->lconv_intl_refcount = malloc(sizeof(int));
        loc->locinfo->lconv_num_refcount = malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_num_refcount) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_num_refcount = 1;

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SDECIMAL, buf, 256);
        if(i && (loc->locinfo->lconv->decimal_point = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->decimal_point, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_STHOUSAND, buf, 256);
        if(i && (loc->locinfo->lconv->thousands_sep = malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->thousands_sep, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SGROUPING, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->grouping = malloc(sizeof(char[i])))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->grouping[i/2+1] = 127;
        } else {
            _free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->decimal_point = malloc(sizeof(char[2]));
        loc->locinfo->lconv->thousands_sep = malloc(sizeof(char));
        loc->locinfo->lconv->grouping = malloc(sizeof(char));
        if(!loc->locinfo->lconv->decimal_point || !loc->locinfo->lconv->thousands_sep
                || !loc->locinfo->lconv->grouping) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv->decimal_point[0] = '.';
        loc->locinfo->lconv->decimal_point[1] = '\0';
        loc->locinfo->lconv->thousands_sep[0] = '\0';
        loc->locinfo->lconv->grouping[0] = '\0';

        loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale = _strdup("C");
    }

    if(lcid[MSVCRT_LC_TIME] && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_TIME)) {
        if(update_threadlocinfo_category(lcid[MSVCRT_LC_TIME], loc, MSVCRT_LC_TIME)) {
            _free_locale(loc);
            return NULL;
        }
    } else
        loc->locinfo->lc_category[MSVCRT_LC_TIME].locale = _strdup("C");

    return loc;
}

/* _configthreadlocale - not exported in native msvcrt */
int CDECL _configthreadlocale(int type)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    int ret;

    if(!data)
        return -1;

    ret = (data->locale ? _ENABLE_PER_THREAD_LOCALE : _DISABLE_PER_THREAD_LOCALE);

    if(type == _ENABLE_PER_THREAD_LOCALE) {
        if(!data->locale) {
            /* Copy current global locale */
            data->locale = _create_locale(MSVCRT_LC_ALL, setlocale(MSVCRT_LC_ALL, NULL));
            if(!data->locale)
                return -1;
        }

        return ret;
    }

    if(type == _DISABLE_PER_THREAD_LOCALE) {
        if(data->locale) {
            _free_locale(data->locale);
            data->locale = NULL;
        }

        return ret;
    }

    if(!type)
        return ret;

    return -1;
}

/*********************************************************************
 *             setlocale (MSVCRT.@)
 */
char* CDECL setlocale(int category, const char* locale)
{
    _locale_t loc, cur;

    cur = get_locale();

    if(category<MSVCRT_LC_MIN || category>MSVCRT_LC_MAX)
        return NULL;

    if(!locale) {
        if(category == MSVCRT_LC_ALL)
            return construct_lc_all(cur);

        return cur->locinfo->lc_category[category].locale;
    }

    loc = _create_locale(category, locale);
    if(!loc) {
        WARN("%d %s failed\n", category, locale);
        return NULL;
    }

    LOCK_LOCALE;

    switch(category) {
        case MSVCRT_LC_ALL:
            if(!cur)
                break;
        case MSVCRT_LC_COLLATE:
            cur->locinfo->lc_handle[MSVCRT_LC_COLLATE] =
                loc->locinfo->lc_handle[MSVCRT_LC_COLLATE];
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_COLLATE].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_COLLATE].locale);
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_COLLATE].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_COLLATE].refcount);

            if(category != MSVCRT_LC_ALL)
                break;
        case MSVCRT_LC_CTYPE:
            cur->locinfo->lc_handle[MSVCRT_LC_CTYPE] =
                loc->locinfo->lc_handle[MSVCRT_LC_CTYPE];
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_CTYPE].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_CTYPE].locale);
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_CTYPE].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_CTYPE].refcount);

            cur->locinfo->lc_codepage = loc->locinfo->lc_codepage;
            cur->locinfo->lc_collate_cp = loc->locinfo->lc_collate_cp;
            cur->locinfo->lc_clike = loc->locinfo->lc_clike;
            cur->locinfo->mb_cur_max = loc->locinfo->mb_cur_max;

            swap_pointers((void**)&cur->locinfo->ctype1_refcount,
                    (void**)&loc->locinfo->ctype1_refcount);
            swap_pointers((void**)&cur->locinfo->ctype1, (void**)&loc->locinfo->ctype1);
            swap_pointers((void**)&cur->locinfo->pctype, (void**)&loc->locinfo->pctype);
            swap_pointers((void**)&cur->locinfo->pclmap, (void**)&loc->locinfo->pclmap);
            swap_pointers((void**)&cur->locinfo->pcumap, (void**)&loc->locinfo->pcumap);

            memcpy(cur->mbcinfo, loc->mbcinfo, sizeof(threadmbcinfo));

            if(category != MSVCRT_LC_ALL)
                break;
        case MSVCRT_LC_MONETARY:
            cur->locinfo->lc_handle[MSVCRT_LC_MONETARY] =
                loc->locinfo->lc_handle[MSVCRT_LC_MONETARY];
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_MONETARY].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_MONETARY].locale);
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_MONETARY].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_MONETARY].refcount);

            swap_pointers((void**)&cur->locinfo->lconv->int_curr_symbol,
                    (void**)&loc->locinfo->lconv->int_curr_symbol);
            swap_pointers((void**)&cur->locinfo->lconv->currency_symbol,
                    (void**)&loc->locinfo->lconv->currency_symbol);
            swap_pointers((void**)&cur->locinfo->lconv->mon_decimal_point,
                    (void**)&loc->locinfo->lconv->mon_decimal_point);
            swap_pointers((void**)&cur->locinfo->lconv->mon_thousands_sep,
                    (void**)&loc->locinfo->lconv->mon_thousands_sep);
            swap_pointers((void**)&cur->locinfo->lconv->mon_grouping,
                    (void**)&loc->locinfo->lconv->mon_grouping);
            swap_pointers((void**)&cur->locinfo->lconv->positive_sign,
                    (void**)&loc->locinfo->lconv->positive_sign);
            swap_pointers((void**)&cur->locinfo->lconv->negative_sign,
                    (void**)&loc->locinfo->lconv->negative_sign);
            cur->locinfo->lconv->int_frac_digits = loc->locinfo->lconv->int_frac_digits;
            cur->locinfo->lconv->frac_digits = loc->locinfo->lconv->frac_digits;
            cur->locinfo->lconv->p_cs_precedes = loc->locinfo->lconv->p_cs_precedes;
            cur->locinfo->lconv->p_sep_by_space = loc->locinfo->lconv->p_sep_by_space;
            cur->locinfo->lconv->n_cs_precedes = loc->locinfo->lconv->n_cs_precedes;
            cur->locinfo->lconv->n_sep_by_space = loc->locinfo->lconv->n_sep_by_space;
            cur->locinfo->lconv->p_sign_posn = loc->locinfo->lconv->p_sign_posn;
            cur->locinfo->lconv->n_sign_posn = loc->locinfo->lconv->n_sign_posn;

            if(category != MSVCRT_LC_ALL)
                break;
        case MSVCRT_LC_NUMERIC:
            cur->locinfo->lc_handle[MSVCRT_LC_NUMERIC] =
                loc->locinfo->lc_handle[MSVCRT_LC_NUMERIC];
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale);
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_NUMERIC].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].refcount);

            swap_pointers((void**)&cur->locinfo->lconv->decimal_point,
                    (void**)&loc->locinfo->lconv->decimal_point);
            swap_pointers((void**)&cur->locinfo->lconv->thousands_sep,
                    (void**)&loc->locinfo->lconv->thousands_sep);
            swap_pointers((void**)&cur->locinfo->lconv->grouping,
                    (void**)&loc->locinfo->lconv->grouping);

            if(category != MSVCRT_LC_ALL)
                break;
        case MSVCRT_LC_TIME:
            cur->locinfo->lc_handle[MSVCRT_LC_TIME] =
                loc->locinfo->lc_handle[MSVCRT_LC_TIME];
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_TIME].locale,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_TIME].locale);
            swap_pointers((void**)&cur->locinfo->lc_category[MSVCRT_LC_TIME].refcount,
                    (void**)&loc->locinfo->lc_category[MSVCRT_LC_TIME].refcount);

            if(category != MSVCRT_LC_ALL)
                break;
    }

    if(!cur)
        MSVCRT_locale = cur = loc;
    else
        _free_locale(loc);

    UNLOCK_LOCALE;

    if(cur == MSVCRT_locale) {
        MSVCRT___lc_codepage = cur->locinfo->lc_codepage;
        MSVCRT___lc_collate_cp = cur->locinfo->lc_collate_cp;
        __mb_cur_max = cur->locinfo->mb_cur_max;
        _pctype = cur->locinfo->pctype;
    }

    if(category == MSVCRT_LC_ALL)
        return construct_lc_all(cur);

    return cur->locinfo->lc_category[category].locale;
}

//from mbcs.c

/* It seems that the data about valid trail bytes is not available from kernel32
 * so we have to store is here. The format is the same as for lead bytes in CPINFO */
struct cp_extra_info_t
{
    int cp;
    BYTE TrailBytes[MAX_LEADBYTES];
};

static struct cp_extra_info_t g_cpextrainfo[] =
{
    {932, {0x40, 0x7e, 0x80, 0xfc, 0, 0}},
    {936, {0x40, 0xfe, 0, 0}},
    {949, {0x41, 0xfe, 0, 0}},
    {950, {0x40, 0x7e, 0xa1, 0xfe, 0, 0}},
    {1361, {0x31, 0x7e, 0x81, 0xfe, 0, 0}},
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};

/*********************************************************************
 *		___setlc_active_func (MSVCRT.@)
 */
unsigned int CDECL ___setlc_active_func(void)
{
  return __setlc_active;
}

/*********************************************************************
 *              ___mb_cur_max_func(MSVCRT.@)
 */
int* CDECL MSVCRT___mb_cur_max_func()
{
  return &get_locale()->locinfo->mb_cur_max;
}

/*********************************************************************
 *		___unguarded_readlc_active_add_func (MSVCRT.@)
 */
unsigned int * CDECL ___unguarded_readlc_active_add_func(void)
{
    return &__unguarded_readlc_active;
}

/*********************************************************************
 *		__lc_collate_cp (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_collate_cp(int cp)
{
    FIXME("__lc_collate_cp - stub\n");
    return;
}

/*********************************************************************
 *		__lc_codepage (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_codepage(void)
{
    FIXME("__lc_codepage - stub\n");
    return;
}

/*********************************************************************
 *		__lc_handle (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_handle(void)
{
    FIXME("__lc_handle - stub\n");
    return;
}

int CDECL _getmbcp(void)
{
    return MSVCRT___lc_codepage;
}

/*********************************************************************
 *		_setmbcp (MSVCRT.@)
 */
int CDECL _setmbcp(int cp)
{
  _locale_t locale = get_locale();
  int newcp;
  CPINFO cpi;
  BYTE *bytes;
  WORD chartypes[256];
  WORD *curr_type;
  char bufA[256];
  WCHAR bufW[256];
  int charcount;
  int ret;
  int i;

  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = locale->locinfo->lc_codepage;
      break;
    case _MB_CP_SBCS:
      newcp = 20127;   /* ASCII */
      break;
    default:
      newcp = cp;
      break;
  }

  if (!GetCPInfo(newcp, &cpi))
  {
    WARN("Codepage %d not found\n", newcp);
    *_errno() = EINVAL;
    return -1;
  }

  /* setup the _mbctype */
  memset(_mbctype, 0, sizeof(_mbctype));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      _mbctype[i + 1] |= _M1;
    bytes += 2;
  }

  if (cpi.MaxCharSize > 1)
  {
    /* trail bytes not available through kernel32 but stored in a structure in msvcrt */
    struct cp_extra_info_t *cpextra = g_cpextrainfo;

    g_mbcp_is_multibyte = 1;
    while (TRUE)
    {
      if (cpextra->cp == 0 || cpextra->cp == newcp)
      {
        if (cpextra->cp == 0)
          FIXME("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);

        bytes = cpextra->TrailBytes;
        while (bytes[0] || bytes[1])
        {
          for (i = bytes[0]; i <= bytes[1]; i++)
            _mbctype[i + 1] |= _M2;
          bytes += 2;
        }
        break;
      }
      cpextra++;
    }
  }
  else
    g_mbcp_is_multibyte = 0;

  /* we can't use GetStringTypeA directly because we don't have a locale - only a code page
   */
  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(_mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

  curr_type = chartypes;
  for (i = 0; i < 256; i++)
    if (!(_mbctype[i + 1] & _M1))
    {
	if ((*curr_type) & C1_UPPER)
	    _mbctype[i + 1] |= _SBUP;
	if ((*curr_type) & C1_LOWER)
	    _mbctype[i + 1] |= _SBLOW;
	curr_type++;
    }

  if (newcp == 932)   /* CP932 only - set _MP and _MS */
  {
    /* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
     * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
     * it hard. As this is set only for codepage 932 we hardcode it what gives
     * also faster execution.
     */
    for (i = 161; i <= 165; i++)
      _mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      _mbctype[i + 1] |= _MS;
  }

  locale->locinfo->lc_collate_cp = newcp;
  locale->locinfo->lc_codepage = newcp;
  TRACE("(%d) -> %d\n", cp, locale->locinfo->lc_codepage);
  return 0;
}


/*********************************************************************
 *    __crtLCMapStringW (MSVCRT.@)
 */
int __crtLCMapStringW(
  LCID lcid, DWORD mapflags, LPCWSTR src, int srclen, LPWSTR dst,
  int dstlen, unsigned int codepage, int xflag
) {
  TRACE("(lcid %lx, flags %lx, %s(%d), %p(%d), %x, %d), partial stub!\n",
        lcid,mapflags,src,srclen,dst,dstlen,codepage,xflag);

  return LCMapStringW(lcid,mapflags,src,srclen,dst,dstlen);
}


