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

#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbctype.h>
#include <wctype.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "msvcrt.h"
#include "mtdll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256
_locale_t MSVCRT_locale = NULL;
unsigned short *MSVCRT__pctype = NULL;
unsigned int MSVCRT___lc_codepage = 0;
int MSVCRT___lc_collate_cp = 0;
LCID MSVCRT___lc_handle[LC_MAX - LC_MIN + 1] = { 0 };
int MSVCRT___mb_cur_max = 1;
BOOL initial_locale = TRUE;

#define MSVCRT_LEADBYTE  0x8000
#define MSVCRT_C1_DEFINED 0x200

#if _MSVCR_VER >= 110
#define LCID_CONVERSION_FLAGS LOCALE_ALLOW_NEUTRAL_NAMES
#else
#define LCID_CONVERSION_FLAGS 0
#endif

__lc_time_data cloc_time_data =
{
    {{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
      "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
      "January", "February", "March", "April", "May", "June", "July",
      "August", "September", "October", "November", "December",
      "AM", "PM", "MM/dd/yy", "dddd, MMMM dd, yyyy", "HH:mm:ss"}},
#if _MSVCR_VER < 110
    MAKELCID(LANG_ENGLISH, SORT_DEFAULT),
#endif
    1, -1,
#if _MSVCR_VER == 0 || _MSVCR_VER >= 100
    {{L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat",
      L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday",
      L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec",
      L"January", L"February", L"March", L"April", L"May", L"June", L"July",
      L"August", L"September", L"October", L"November", L"December",
      L"AM", L"PM", L"MM/dd/yy", L"dddd, MMMM dd, yyyy", L"HH:mm:ss"}},
#endif
#if _MSVCR_VER >= 110
    L"en-US",
#endif
};

static const unsigned char cloc_clmap[256] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static const unsigned char cloc_cumap[256] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static char empty[] = "";
static char cloc_dec_point[] = ".";
#if _MSVCR_VER >= 100
static wchar_t emptyW[] = L"";
static wchar_t cloc_dec_pointW[] = L".";
#endif
static struct lconv cloc_lconv =
{
    cloc_dec_point, empty, empty, empty, empty, empty, empty, empty, empty, empty,
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
#if _MSVCR_VER >= 100
    cloc_dec_pointW, emptyW, emptyW, emptyW, emptyW, emptyW, emptyW, emptyW
#endif
};

/* Friendly country strings & language names abbreviations. */
static const char * const _country_synonyms[] =
{
    "american", "en",
    "american english", "en-US",
    "american-english", "en-US",
    "english-american", "en-US",
    "english-us", "en-US",
    "english-usa", "en-US",
    "us", "en-US",
    "usa", "en-US",
    "australian", "en-AU",
    "english-aus", "en-AU",
    "belgian", "nl-BE",
    "french-belgian", "fr-BE",
    "canadian", "en-CA",
    "english-can", "en-CA",
    "french-canadian", "fr-CA",
#if _MSVCR_VER >= 110
    "chinese", "zh",
    "chinese-simplified", "zh",
    "chinese-traditional", "zh-HK",
    "chs", "zh",
    "cht", "zh-HK",
#else
    "chinese", "zh-CN",
    "chinese-simplified", "zh-CN",
    "chinese-traditional", "zh-TW",
    "chs", "zh-CN",
    "cht", "zh-TW",
#endif
    "dutch-belgian", "nl-BE",
    "english-nz", "en-NZ",
    "uk", "en-GB",
    "english-uk", "en-GB",
    "french-swiss", "fr-CH",
    "swiss", "de-CH",
    "german-swiss", "de-CH",
    "italian-swiss", "it-CH",
    "german-austrian", "de-AT",
    "portuguese", "pt-BR",
    "portuguese-brazil", "pt-BR",
    "spanish-mexican", "es-MX",
    "norwegian-bokmal", "nb",
    "norwegian-nynorsk", "nn-NO",
    "spanish-modern", "es-ES"
};


/* INTERNAL: Map a synonym to an ISO code */
static BOOL remap_synonym(char *name)
{
  unsigned int i;
  for (i = 0; i < ARRAY_SIZE(_country_synonyms); i += 2)
  {
    if (!_stricmp(_country_synonyms[i],name))
    {
      TRACE(":Mapping synonym %s to %s\n",name,_country_synonyms[i+1]);
      strcpy(name, _country_synonyms[i+1]);
      return TRUE;
    }
  }

  return FALSE;
}

/* Note: Flags are weighted in order of matching importance */
#define FOUND_SNAME            0x4
#define FOUND_LANGUAGE         0x2
#define FOUND_COUNTRY          0x1

typedef struct {
  WCHAR search_language[MAX_ELEM_LEN];
  WCHAR search_country[MAX_ELEM_LEN];
  WCHAR found_lang_sname[LOCALE_NAME_MAX_LENGTH];
  unsigned int match_flags;
  BOOL allow_sname;
} locale_search_t;

#define CONTINUE_LOOKING TRUE
#define STOP_LOOKING     FALSE

/* INTERNAL: Get and compare locale info with a given string */
static int compare_info(WCHAR *name, DWORD flags, WCHAR *buff, const WCHAR *cmp, BOOL exact)
{
  int len;

  if(!cmp[0])
    return 0;

  buff[0] = 0;
  GetLocaleInfoEx(name, flags|LOCALE_NOUSEROVERRIDE, buff, MAX_ELEM_LEN);
  if (!buff[0])
    return 0;

  /* Partial matches are only allowed on language/country names */
  len = wcslen(cmp);

  if(exact || len<=3)
    return !_wcsicmp(cmp, buff);
  else
    return !_wcsnicmp(cmp, buff, len);
}

static BOOL CALLBACK
find_best_locale_proc( WCHAR *name, DWORD locale_flags, LPARAM lParam )
{
  locale_search_t *res = (locale_search_t *)lParam;
  WCHAR buff[MAX_ELEM_LEN];
  unsigned int flags = 0;

  if (res->allow_sname && compare_info(name,LOCALE_SNAME,buff,res->search_language, TRUE))
  {
    TRACE(":Found locale: %s->%s\n", wine_dbgstr_w(res->search_language), wine_dbgstr_w(buff));
    res->match_flags = FOUND_SNAME;
    wcscpy(res->found_lang_sname, name);
    return STOP_LOOKING;
  }

  /* Check Language */
  if (compare_info(name,LOCALE_SISO639LANGNAME,buff,res->search_language, TRUE) ||
      compare_info(name,LOCALE_SABBREVLANGNAME,buff,res->search_language, TRUE) ||
      compare_info(name,LOCALE_SENGLANGUAGE,buff,res->search_language, FALSE))
  {
    TRACE(":Found language: %s->%s\n", wine_dbgstr_w(res->search_language), wine_dbgstr_w(buff));
    flags |= FOUND_LANGUAGE;
  }
  else if (res->match_flags & FOUND_LANGUAGE)
  {
    return CONTINUE_LOOKING;
  }

  /* Check Country */
  if (compare_info(name,LOCALE_SISO3166CTRYNAME,buff,res->search_country, TRUE) ||
      compare_info(name,LOCALE_SABBREVCTRYNAME,buff,res->search_country, TRUE) ||
      compare_info(name,LOCALE_SENGCOUNTRY,buff,res->search_country, FALSE))
  {
    TRACE("Found country:%s->%s\n", wine_dbgstr_w(res->search_country), wine_dbgstr_w(buff));
    flags |= FOUND_COUNTRY;
  }
  else if (!flags && (res->match_flags & FOUND_COUNTRY))
  {
    return CONTINUE_LOOKING;
  }

  if (flags > res->match_flags)
  {
    /* Found a better match than previously */
    res->match_flags = flags;
    wcscpy(res->found_lang_sname, name);
  }
  if ((flags & (FOUND_LANGUAGE | FOUND_COUNTRY)) ==
        (FOUND_LANGUAGE | FOUND_COUNTRY))
  {
    TRACE(":found exact locale match\n");
    return STOP_LOOKING;
  }
  return CONTINUE_LOOKING;
}

/* Internal: Find the sname for a locale specification.
 * sname must be at least LOCALE_NAME_MAX_LENGTH characters long
 */
BOOL locale_to_sname(const char *locale, unsigned short *codepage, BOOL *sname_match, WCHAR *sname)
{
    thread_data_t *data = msvcrt_get_thread_data();
    const char *cp, *region;
    BOOL is_sname = FALSE;
    DWORD locale_cp;

    if (!strcmp(locale, data->cached_locale)) {
        if (codepage)
            *codepage = data->cached_cp;
        if (sname_match)
            *sname_match = data->cached_sname_match;
        wcscpy(sname, data->cached_sname);
        return TRUE;
    }

    cp = strchr(locale, '.');
    region = strchr(locale, '_');

    if(!locale[0] || (cp == locale && !region)) {
        GetUserDefaultLocaleName(sname, LOCALE_NAME_MAX_LENGTH);
    } else {
        char search_language_buf[MAX_ELEM_LEN] = { 0 }, search_country_buf[MAX_ELEM_LEN] = { 0 };
        locale_search_t search;
        BOOL remapped = FALSE;

        memset(&search, 0, sizeof(locale_search_t));
        lstrcpynA(search_language_buf, locale, MAX_ELEM_LEN);
        if(region) {
            lstrcpynA(search_country_buf, region+1, MAX_ELEM_LEN);
            if(region-locale < MAX_ELEM_LEN)
                search_language_buf[region-locale] = '\0';
        } else
            search_country_buf[0] = '\0';

        if(cp) {
            if(region && cp-region-1<MAX_ELEM_LEN)
                search_country_buf[cp-region-1] = '\0';
            if(cp-locale < MAX_ELEM_LEN)
                search_language_buf[cp-locale] = '\0';
        }

        if ((remapped = remap_synonym(search_language_buf)))
        {
            search.allow_sname = TRUE;
        }

#if _MSVCR_VER >= 110
        if(!cp && !region)
        {
            search.allow_sname = TRUE;
        }
#endif

        MultiByteToWideChar(CP_ACP, 0, search_language_buf, -1, search.search_language, MAX_ELEM_LEN);
        if (search.allow_sname && IsValidLocaleName(search.search_language))
        {
            search.match_flags = FOUND_SNAME;
            wcscpy(sname, search.search_language);
        }
        else
        {
            MultiByteToWideChar(CP_ACP, 0, search_country_buf, -1, search.search_country, MAX_ELEM_LEN);
            EnumSystemLocalesEx( find_best_locale_proc, 0, (LPARAM)&search, NULL);

            if (!search.match_flags)
                return FALSE;

            /* If we were given something that didn't match, fail */
            if (search.search_language[0] && !(search.match_flags & (FOUND_SNAME | FOUND_LANGUAGE)))
                return FALSE;
            if (search.search_country[0] && !(search.match_flags & FOUND_COUNTRY))
                return FALSE;

            wcscpy(sname, search.found_lang_sname);
        }

        is_sname = !remapped && (search.match_flags & FOUND_SNAME) != 0;
    }

    /* Obtain code page */
    if (!cp || !cp[1] || !_strnicmp(cp, ".ACP", 4)) {
        GetLocaleInfoEx(sname, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                (WCHAR *)&locale_cp, sizeof(DWORD)/sizeof(WCHAR));
        if (!locale_cp)
            locale_cp = GetACP();
    } else if (!_strnicmp(cp, ".OCP", 4)) {
        GetLocaleInfoEx(sname, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                (WCHAR *)&locale_cp, sizeof(DWORD)/sizeof(WCHAR));
#if _MSVCR_VER >= 140
    } else if (!_strnicmp(cp, ".UTF-8", 6)
            || !_strnicmp(cp, ".UTF8", 5)) {
        locale_cp = CP_UTF8;
#endif
    } else {
        locale_cp = atoi(cp + 1);
    }
    if (!IsValidCodePage(locale_cp))
        return FALSE;

    if (!locale_cp)
        return FALSE;

    if (codepage)
        *codepage = locale_cp;
    if (sname_match)
        *sname_match = is_sname;

    if (strlen(locale) < sizeof(data->cached_locale)) {
        strcpy(data->cached_locale, locale);
        data->cached_cp = locale_cp;
        data->cached_sname_match = is_sname;
        wcscpy(data->cached_sname, sname);
    }

    return TRUE;
}

static void copy_threadlocinfo_category(pthreadlocinfo locinfo,
        const threadlocinfo *old_locinfo, int category)
{
    locinfo->lc_handle[category] = old_locinfo->lc_handle[category];
    locinfo->lc_id[category] = old_locinfo->lc_id[category];
    if(!locinfo->lc_category[category].locale) {
        locinfo->lc_category[category].locale = old_locinfo->lc_category[category].locale;
        locinfo->lc_category[category].refcount = old_locinfo->lc_category[category].refcount;
        InterlockedIncrement((LONG *)locinfo->lc_category[category].refcount);
    }
#if _MSVCR_VER >= 110
    locinfo->lc_name[category] = old_locinfo->lc_name[category];
    locinfo->lc_category[category].wrefcount = old_locinfo->lc_category[category].wrefcount;
    if(locinfo->lc_category[category].wrefcount)
        InterlockedIncrement((LONG *)locinfo->lc_category[category].wrefcount);
#endif
}

static BOOL init_category_name(const char *name, int len,
        pthreadlocinfo locinfo, int category)
{
    locinfo->lc_category[category].locale = malloc(len+1);
    locinfo->lc_category[category].refcount = malloc(sizeof(int));
    if(!locinfo->lc_category[category].locale
            || !locinfo->lc_category[category].refcount) {
        free(locinfo->lc_category[category].locale);
        free(locinfo->lc_category[category].refcount);
        locinfo->lc_category[category].locale = NULL;
        locinfo->lc_category[category].refcount = NULL;
        return FALSE;
    }

    memcpy(locinfo->lc_category[category].locale, name, len);
    locinfo->lc_category[category].locale[len] = 0;
    *locinfo->lc_category[category].refcount = 1;
    return TRUE;
}

#if _MSVCR_VER >= 110
static inline BOOL set_lc_locale_name(pthreadlocinfo locinfo, int cat, WCHAR *sname)
{
    locinfo->lc_category[cat].wrefcount = malloc(sizeof(int));
    if(!locinfo->lc_category[cat].wrefcount)
        return FALSE;
    *locinfo->lc_category[cat].wrefcount = 1;

    if(!(locinfo->lc_name[cat] = wcsdup(sname)))
        return FALSE;

    return TRUE;
}
#else
static inline BOOL set_lc_locale_name(pthreadlocinfo locinfo, int cat, WCHAR *sname)
{
    return TRUE;
}
#endif

/* INTERNAL: Set lc_handle, lc_id and lc_category in threadlocinfo struct */
static BOOL update_threadlocinfo_category(WCHAR *sname, unsigned short cp,
        pthreadlocinfo locinfo, int category)
{
    WCHAR wbuf[256], *p;

    if(GetLocaleInfoEx(sname, LOCALE_ILANGUAGE|LOCALE_NOUSEROVERRIDE, wbuf, ARRAY_SIZE(wbuf))) {
        p = wbuf;

        locinfo->lc_id[category].wLanguage = 0;
        while(*p) {
            locinfo->lc_id[category].wLanguage *= 16;

            if(*p <= '9')
                locinfo->lc_id[category].wLanguage += *p-'0';
            else
                locinfo->lc_id[category].wLanguage += *p-'a'+10;

            p++;
        }

        locinfo->lc_id[category].wCountry =
            locinfo->lc_id[category].wLanguage;
    }

    locinfo->lc_id[category].wCodePage = cp;

    locinfo->lc_handle[category] = LocaleNameToLCID(sname, LCID_CONVERSION_FLAGS);

    set_lc_locale_name(locinfo, category, sname);

    if(!locinfo->lc_category[category].locale) {
        char buf[256];
        int len = 0;

#if _MSVCR_VER < 110
        if (LANGIDFROMLCID(locinfo->lc_handle[category]) == MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK))
        {
            /* locale.nls contains "Norwegian Nynorsk" instead for LOCALE_SENGLANGUAGE */
            wcscpy( wbuf, L"Norwegian-Nynorsk" );
            len = wcslen( wbuf ) + 1;
        }
        else
#endif
            len += GetLocaleInfoEx(sname, LOCALE_SENGLANGUAGE|LOCALE_NOUSEROVERRIDE, wbuf, ARRAY_SIZE(wbuf));
        wbuf[len-1] = '_';
        len += GetLocaleInfoEx(sname, LOCALE_SENGCOUNTRY
                |LOCALE_NOUSEROVERRIDE, &wbuf[len], ARRAY_SIZE(wbuf) - len);
        wbuf[len-1] = '.';
        swprintf(wbuf+len, ARRAY_SIZE(wbuf) - len,L"%d", cp);
        len += wcslen(wbuf+len);

        WideCharToMultiByte(cp, 0, wbuf, -1, buf, ARRAY_SIZE(buf), NULL, NULL);

        return init_category_name(buf, len, locinfo, category);
    }

    return TRUE;
}

/*********************************************************************
 *      _lock_locales (UCRTBASE.@)
 */
void CDECL _lock_locales(void)
{
    _lock(_SETLOCALE_LOCK);
}

/*********************************************************************
 *      _unlock_locales (UCRTBASE.@)
 */
void CDECL _unlock_locales(void)
{
    _unlock(_SETLOCALE_LOCK);
}

static void grab_locinfo(pthreadlocinfo locinfo)
{
    int i;

    InterlockedIncrement((LONG *)&locinfo->refcount);
    for(i=LC_MIN+1; i<=LC_MAX; i++)
    {
        InterlockedIncrement((LONG *)locinfo->lc_category[i].refcount);
        if(locinfo->lc_category[i].wrefcount)
            InterlockedIncrement((LONG *)locinfo->lc_category[i].wrefcount);
    }
    if(locinfo->lconv_intl_refcount)
        InterlockedIncrement((LONG *)locinfo->lconv_intl_refcount);
    if(locinfo->lconv_num_refcount)
        InterlockedIncrement((LONG *)locinfo->lconv_num_refcount);
    if(locinfo->lconv_mon_refcount)
        InterlockedIncrement((LONG *)locinfo->lconv_mon_refcount);
    if(locinfo->ctype1_refcount)
        InterlockedIncrement((LONG *)locinfo->ctype1_refcount);
    InterlockedIncrement(&locinfo->lc_time_curr->refcount);
}

static void update_thread_locale(thread_data_t *data)
{
    if((data->locale_flags & LOCALE_FREE) && ((data->locale_flags & LOCALE_THREAD) ||
                (data->locinfo == MSVCRT_locale->locinfo && data->mbcinfo == MSVCRT_locale->mbcinfo)))
        return;

    if(data->locale_flags & LOCALE_FREE)
    {
        free_locinfo(data->locinfo);
        free_mbcinfo(data->mbcinfo);
    }

    _lock_locales();
    data->locinfo = MSVCRT_locale->locinfo;
    grab_locinfo(data->locinfo);
    _unlock_locales();

    _lock(_MB_CP_LOCK);
    data->mbcinfo = MSVCRT_locale->mbcinfo;
    InterlockedIncrement(&data->mbcinfo->refcount);
    _unlock(_MB_CP_LOCK);

    data->locale_flags |= LOCALE_FREE;
}

/* INTERNAL: returns threadlocinfo struct */
pthreadlocinfo CDECL get_locinfo(void) {
    thread_data_t *data = msvcrt_get_thread_data();
    update_thread_locale(data);
    return data->locinfo;
}

/* INTERNAL: returns pthreadmbcinfo struct */
pthreadmbcinfo CDECL get_mbcinfo(void) {
    thread_data_t *data = msvcrt_get_thread_data();
    update_thread_locale(data);
    return data->mbcinfo;
}

/* INTERNAL: constructs string returned by setlocale */
static inline char* construct_lc_all(pthreadlocinfo locinfo) {
    static char current_lc_all[MAX_LOCALE_LENGTH];

    int i;

    for(i=LC_MIN+1; i<LC_MAX; i++) {
        if(strcmp(locinfo->lc_category[i].locale,
                    locinfo->lc_category[i+1].locale))
            break;
    }

    if(i==LC_MAX)
        return locinfo->lc_category[LC_COLLATE].locale;

    sprintf(current_lc_all,
            "LC_COLLATE=%s;LC_CTYPE=%s;LC_MONETARY=%s;LC_NUMERIC=%s;LC_TIME=%s",
            locinfo->lc_category[LC_COLLATE].locale,
            locinfo->lc_category[LC_CTYPE].locale,
            locinfo->lc_category[LC_MONETARY].locale,
            locinfo->lc_category[LC_NUMERIC].locale,
            locinfo->lc_category[LC_TIME].locale);

    return current_lc_all;
}


/*********************************************************************
 *		_Getdays (MSVCRT.@)
 */
char* CDECL _Getdays(void)
{
    __lc_time_data *cur = get_locinfo()->lc_time_curr;
    int i, len, size = 0;
    char *out;

    TRACE("\n");

    for(i=0; i<7; i++) {
        size += strlen(cur->str.names.short_wday[i]) + 1;
        size += strlen(cur->str.names.wday[i]) + 1;
    }
    out = malloc(size+1);
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

#if _MSVCR_VER >= 110
/*********************************************************************
 *		_W_Getdays (MSVCR110.@)
 */
wchar_t* CDECL _W_Getdays(void)
{
    __lc_time_data *cur = get_locinfo()->lc_time_curr;
    wchar_t *out;
    int i, len, size = 0;

    TRACE("\n");

    for(i=0; i<7; i++) {
        size += wcslen(cur->wstr.names.short_wday[i]) + 1;
        size += wcslen(cur->wstr.names.wday[i]) + 1;
    }
    out = malloc((size+1)*sizeof(*out));
    if(!out)
        return NULL;

    size = 0;
    for(i=0; i<7; i++) {
        out[size++] = ':';
        len = wcslen(cur->wstr.names.short_wday[i]);
        memcpy(&out[size], cur->wstr.names.short_wday[i], len*sizeof(*out));
        size += len;

        out[size++] = ':';
        len = wcslen(cur->wstr.names.wday[i]);
        memcpy(&out[size], cur->wstr.names.wday[i], len*sizeof(*out));
        size += len;
    }
    out[size] = '\0';

    return out;
}
#endif

/*********************************************************************
 *		_Getmonths (MSVCRT.@)
 */
char* CDECL _Getmonths(void)
{
    __lc_time_data *cur = get_locinfo()->lc_time_curr;
    int i, len, size = 0;
    char *out;

    TRACE("\n");

    for(i=0; i<12; i++) {
        size += strlen(cur->str.names.short_mon[i]) + 1;
        size += strlen(cur->str.names.mon[i]) + 1;
    }
    out = malloc(size+1);
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

#if _MSVCR_VER >= 110
/*********************************************************************
 *		_W_Getmonths (MSVCR110.@)
 */
wchar_t* CDECL _W_Getmonths(void)
{
    __lc_time_data *cur = get_locinfo()->lc_time_curr;
    wchar_t *out;
    int i, len, size = 0;

    TRACE("\n");

    for(i=0; i<12; i++) {
        size += wcslen(cur->wstr.names.short_mon[i]) + 1;
        size += wcslen(cur->wstr.names.mon[i]) + 1;
    }
    out = malloc((size+1)*sizeof(*out));
    if(!out)
        return NULL;

    size = 0;
    for(i=0; i<12; i++) {
        out[size++] = ':';
        len = wcslen(cur->wstr.names.short_mon[i]);
        memcpy(&out[size], cur->wstr.names.short_mon[i], len*sizeof(*out));
        size += len;

        out[size++] = ':';
        len = wcslen(cur->wstr.names.mon[i]);
        memcpy(&out[size], cur->wstr.names.mon[i], len*sizeof(*out));
        size += len;
    }
    out[size] = '\0';

    return out;
}
#endif

/*********************************************************************
 *		_Gettnames (MSVCRT.@)
 */
void* CDECL _Gettnames(void)
{
    __lc_time_data *ret, *cur = get_locinfo()->lc_time_curr;
    unsigned int i, len, size = sizeof(__lc_time_data);

    TRACE("\n");

    for(i=0; i<ARRAY_SIZE(cur->str.str); i++)
        size += strlen(cur->str.str[i])+1;
#if _MSVCR_VER >= 110
    for(i=0; i<ARRAY_SIZE(cur->wstr.wstr); i++)
        size += (wcslen(cur->wstr.wstr[i]) + 1) * sizeof(wchar_t);
#endif

    ret = malloc(size);
    if(!ret)
        return NULL;
    memcpy(ret, cur, sizeof(*ret));

    size = 0;
    for(i=0; i<ARRAY_SIZE(cur->str.str); i++) {
        len = strlen(cur->str.str[i])+1;
        memcpy(&ret->data[size], cur->str.str[i], len);
        ret->str.str[i] = &ret->data[size];
        size += len;
    }
#if _MSVCR_VER >= 110
    for(i=0; i<ARRAY_SIZE(cur->wstr.wstr); i++) {
        len = (wcslen(cur->wstr.wstr[i]) + 1) * sizeof(wchar_t);
        memcpy(&ret->data[size], cur->wstr.wstr[i], len);
        ret->wstr.wstr[i] = (wchar_t*)&ret->data[size];
        size += len;
    }
#endif

    return ret;
}

#if _MSVCR_VER >= 110
/*********************************************************************
 *              _W_Gettnames (MSVCR110.@)
 */
void* CDECL _W_Gettnames(void)
{
    return _Gettnames();
}
#endif

/*********************************************************************
 *		__crtLCMapStringA (MSVCRT.@)
 */
int CDECL __crtLCMapStringA(
  LCID lcid, DWORD mapflags, const char* src, int srclen, char* dst,
  int dstlen, unsigned int codepage, int xflag
) {
    WCHAR buf_in[32], *in = buf_in;
    WCHAR buf_out[32], *out = buf_out;
    int in_len, out_len, r;

    TRACE("(lcid %lx, flags %lx, %s(%d), %p(%d), %x, %d), partial stub!\n",
            lcid, mapflags, src, srclen, dst, dstlen, codepage, xflag);

    in_len = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, src, srclen, NULL, 0);
    if (!in_len) return 0;
    if (in_len > ARRAY_SIZE(buf_in))
    {
        in = malloc(in_len * sizeof(WCHAR));
        if (!in) return 0;
    }

    r = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, src, srclen, in, in_len);
    if (!r) goto done;

    if (mapflags & LCMAP_SORTKEY)
    {
        r = LCMapStringW(lcid, mapflags, in, in_len, (WCHAR*)dst, dstlen);
        goto done;
    }

    r = LCMapStringW(lcid, mapflags, in, in_len, NULL, 0);
    if (!r) goto done;
    out_len = r;
    if (r > ARRAY_SIZE(buf_out))
    {
        out = malloc(r * sizeof(WCHAR));
        if (!out)
        {
            r = 0;
            goto done;
        }
    }

    r = LCMapStringW(lcid, mapflags, in, in_len, out, out_len);
    if (!r) goto done;

    r = WideCharToMultiByte(codepage, 0, out, out_len, dst, dstlen, NULL, NULL);

done:
    if (in != buf_in) free(in);
    if (out != buf_out) free(out);
    return r;
}

/*********************************************************************
 *              __crtLCMapStringW (MSVCRT.@)
 */
int CDECL __crtLCMapStringW(LCID lcid, DWORD mapflags, const wchar_t *src,
        int srclen, wchar_t *dst, int dstlen, unsigned int codepage, int xflag)
{
    FIXME("(lcid %lx, flags %lx, %s(%d), %p(%d), %x, %d), partial stub!\n",
            lcid, mapflags, debugstr_w(src), srclen, dst, dstlen, codepage, xflag);

    return LCMapStringW(lcid, mapflags, src, srclen, dst, dstlen);
}

/*********************************************************************
 *		__crtCompareStringA (MSVCRT.@)
 */
int CDECL __crtCompareStringA( LCID lcid, DWORD flags, const char *src1, int len1,
                               const char *src2, int len2 )
{
    FIXME("(lcid %lx, flags %lx, %s(%d), %s(%d), partial stub\n",
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
    FIXME("(lcid %lx, flags %lx, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_w(src1), len1, debugstr_w(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringW( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtGetLocaleInfoW (MSVCRT.@)
 */
int CDECL __crtGetLocaleInfoW( LCID lcid, LCTYPE type, wchar_t *buffer, int len )
{
    FIXME("(lcid %lx, type %lx, %p(%d), partial stub\n", lcid, type, buffer, len );
    /* FIXME: probably not entirely right */
    return GetLocaleInfoW( lcid, type, buffer, len );
}

#if _MSVCR_VER >= 110
/*********************************************************************
 *		__crtGetLocaleInfoEx (MSVC110.@)
 */
int CDECL __crtGetLocaleInfoEx( const WCHAR *locale, LCTYPE type, wchar_t *buffer, int len )
{
    TRACE("(%s, %lx, %p, %d)\n", debugstr_w(locale), type, buffer, len);
    return GetLocaleInfoEx(locale, type, buffer, len);
}
#endif

/*********************************************************************
 *              __crtGetStringTypeW(MSVCRT.@)
 *
 * This function was accepting different number of arguments in older
 * versions of msvcrt.
 */
BOOL CDECL __crtGetStringTypeW(DWORD unk, DWORD type,
        wchar_t *buffer, int len, WORD *out)
{
    FIXME("(unk %lx, type %lx, wstr %p(%d), %p) partial stub\n",
            unk, type, buffer, len, out);

    return GetStringTypeW(type, buffer, len, out);
}

/*********************************************************************
 *		localeconv (MSVCRT.@)
 */
struct lconv* CDECL localeconv(void)
{
    return get_locinfo()->lconv;
}

/*********************************************************************
 *		__lconv_init (MSVCRT.@)
 */
int CDECL __lconv_init(void)
{
    /* this is used to make chars unsigned */
    cloc_lconv.int_frac_digits = (char)UCHAR_MAX;
    cloc_lconv.frac_digits = (char)UCHAR_MAX;
    cloc_lconv.p_cs_precedes = (char)UCHAR_MAX;
    cloc_lconv.p_sep_by_space = (char)UCHAR_MAX;
    cloc_lconv.n_cs_precedes = (char)UCHAR_MAX;
    cloc_lconv.n_sep_by_space = (char)UCHAR_MAX;
    cloc_lconv.p_sign_posn = (char)UCHAR_MAX;
    cloc_lconv.n_sign_posn = (char)UCHAR_MAX;
    return 0;
}

/*********************************************************************
 *      ___lc_handle_func (MSVCRT.@)
 */
LCID* CDECL ___lc_handle_func(void)
{
    return (LCID *)get_locinfo()->lc_handle;
}

#if _MSVCR_VER >= 110
/*********************************************************************
 *      ___lc_locale_name_func (MSVCR110.@)
 */
wchar_t** CDECL ___lc_locale_name_func(void)
{
    return get_locinfo()->lc_name;
}
#endif

/*********************************************************************
 *      ___lc_codepage_func (MSVCRT.@)
 */
unsigned int CDECL ___lc_codepage_func(void)
{
    return get_locinfo()->lc_codepage;
}

/*********************************************************************
 *      ___lc_collate_cp_func (MSVCRT.@)
 */
int CDECL ___lc_collate_cp_func(void)
{
    return get_locinfo()->lc_collate_cp;
}

/* INTERNAL: frees pthreadlocinfo struct */
void free_locinfo(pthreadlocinfo locinfo)
{
    int i;

    if(!locinfo)
        return;

    for(i=LC_MIN+1; i<=LC_MAX; i++) {
        if(!locinfo->lc_category[i].refcount
                || !InterlockedDecrement((LONG *)locinfo->lc_category[i].refcount)) {
            free(locinfo->lc_category[i].locale);
            free(locinfo->lc_category[i].refcount);
        }
        if(!locinfo->lc_category[i].wrefcount
                || !InterlockedDecrement((LONG *)locinfo->lc_category[i].wrefcount)) {
#if _MSVCR_VER >= 110
            free(locinfo->lc_name[i]);
#endif
            free(locinfo->lc_category[i].wrefcount);
        }
    }

    if(locinfo->lconv_num_refcount
            && !InterlockedDecrement((LONG *)locinfo->lconv_num_refcount)) {
        free(locinfo->lconv->decimal_point);
        free(locinfo->lconv->thousands_sep);
        free(locinfo->lconv->grouping);
#if _MSVCR_VER >= 100
        free(locinfo->lconv->_W_decimal_point);
        free(locinfo->lconv->_W_thousands_sep);
#endif
        free(locinfo->lconv_num_refcount);
    }
    if(locinfo->lconv_mon_refcount
            && !InterlockedDecrement((LONG *)locinfo->lconv_mon_refcount)) {
        free(locinfo->lconv->int_curr_symbol);
        free(locinfo->lconv->currency_symbol);
        free(locinfo->lconv->mon_decimal_point);
        free(locinfo->lconv->mon_thousands_sep);
        free(locinfo->lconv->mon_grouping);
        free(locinfo->lconv->positive_sign);
        free(locinfo->lconv->negative_sign);
#if _MSVCR_VER >= 100
        free(locinfo->lconv->_W_int_curr_symbol);
        free(locinfo->lconv->_W_currency_symbol);
        free(locinfo->lconv->_W_mon_decimal_point);
        free(locinfo->lconv->_W_mon_thousands_sep);
        free(locinfo->lconv->_W_positive_sign);
        free(locinfo->lconv->_W_negative_sign);
#endif
        free(locinfo->lconv_mon_refcount);
    }
    if(locinfo->lconv_intl_refcount
            && !InterlockedDecrement((LONG *)locinfo->lconv_intl_refcount)) {
        free(locinfo->lconv_intl_refcount);
        free(locinfo->lconv);
    }

    if(locinfo->ctype1_refcount
            && !InterlockedDecrement((LONG *)locinfo->ctype1_refcount)) {
        free(locinfo->ctype1_refcount);
        free(locinfo->ctype1);
        free((void*)locinfo->pclmap);
        free((void*)locinfo->pcumap);
    }

    if(locinfo->lc_time_curr && !InterlockedDecrement(&locinfo->lc_time_curr->refcount)
            && locinfo->lc_time_curr != &cloc_time_data)
        free(locinfo->lc_time_curr);

    if(InterlockedDecrement((LONG *)&locinfo->refcount))
        return;

    free(locinfo);
}

/* INTERNAL: frees pthreadmbcinfo struct */
void free_mbcinfo(pthreadmbcinfo mbcinfo)
{
    if(!mbcinfo)
        return;

    if(InterlockedDecrement(&mbcinfo->refcount))
        return;

    free(mbcinfo);
}

_locale_t CDECL get_current_locale_noalloc(_locale_t locale)
{
    thread_data_t *data = msvcrt_get_thread_data();

    update_thread_locale(data);
    locale->locinfo = data->locinfo;
    locale->mbcinfo = data->mbcinfo;

    grab_locinfo(locale->locinfo);
    InterlockedIncrement(&locale->mbcinfo->refcount);
    return locale;
}

void CDECL free_locale_noalloc(_locale_t locale)
{
    free_locinfo(locale->locinfo);
    free_mbcinfo(locale->mbcinfo);
}

/*********************************************************************
 *      _get_current_locale (MSVCRT.@)
 */
_locale_t CDECL _get_current_locale(void)
{
    _locale_t loc = malloc(sizeof(_locale_tstruct));
    if(!loc)
        return NULL;

    return get_current_locale_noalloc(loc);
}

/*********************************************************************
 *      _free_locale (MSVCRT.@)
 */
void CDECL _free_locale(_locale_t locale)
{
    if (!locale)
        return;

    free_locale_noalloc(locale);
    free(locale);
}

static inline BOOL category_needs_update(int cat,
        const threadlocinfo *locinfo, WCHAR *sname, unsigned short cp)
{
#if _MSVCR_VER < 110
    LCID lcid;
#endif
    if(!locinfo) return TRUE;
#if _MSVCR_VER >= 110
    if(!locinfo->lc_name[cat] || !sname) return TRUE;
    return wcscmp(sname, locinfo->lc_name[cat]) != 0 || cp!=locinfo->lc_id[cat].wCodePage;
#else
    lcid = sname ? LocaleNameToLCID(sname, 0) : 0;
    return lcid!=locinfo->lc_handle[cat] || cp!=locinfo->lc_id[cat].wCodePage;
#endif
}

static __lc_time_data* create_time_data(WCHAR *sname)
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

    __lc_time_data *cur;
    int i, ret, size;
    LCID lcid = LocaleNameToLCID(sname, LCID_CONVERSION_FLAGS);

    size = 0;
    for(i=0; i<ARRAY_SIZE(time_data); i++) {
        ret = GetLocaleInfoA(lcid, time_data[i], NULL, 0);
        if(!ret)
            return NULL;
        size += ret;

#if _MSVCR_VER == 0 || _MSVCR_VER >= 100
        ret = GetLocaleInfoEx(sname, time_data[i], NULL, 0);
        if(!ret)
            return NULL;
        size += ret*sizeof(wchar_t);
#endif
    }
#if _MSVCR_VER >= 110
    size += (wcslen(sname) + 1) * sizeof(wchar_t);
#endif

    cur = malloc(FIELD_OFFSET(__lc_time_data, data[size]));
    if(!cur)
        return NULL;

    ret = 0;
    for(i=0; i<ARRAY_SIZE(time_data); i++) {
        cur->str.str[i] = &cur->data[ret];
        ret += GetLocaleInfoA(lcid, time_data[i], &cur->data[ret], size-ret);
    }
#if _MSVCR_VER == 0 || _MSVCR_VER >= 100
    for(i=0; i<ARRAY_SIZE(time_data); i++) {
        cur->wstr.wstr[i] = (wchar_t*)&cur->data[ret];
        ret += GetLocaleInfoEx(sname, time_data[i], (wchar_t*)&cur->data[ret],
                (size - ret) / sizeof(wchar_t)) * sizeof(wchar_t);
    }
#endif
#if _MSVCR_VER >= 110
    cur->locname = (wchar_t*)&cur->data[ret];
    wcscpy((wchar_t*)&cur->data[ret], sname);
#else
    cur->lcid = lcid;
#endif
    cur->unk = 1;
    cur->refcount = 1;

    return cur;
}

static pthreadlocinfo create_locinfo(int category,
        const char *locale, const threadlocinfo *old_locinfo)
{
    static const char collate[] = "COLLATE=";
    static const char ctype[] = "CTYPE=";
    static const char monetary[] = "MONETARY=";
    static const char numeric[] = "NUMERIC=";
    static const char time[] = "TIME=";

    pthreadlocinfo locinfo = NULL;
    unsigned short cp[6] = { 0 };
    const char *locale_name[6] = { 0 };
    WCHAR *locale_sname[6] = { 0 };
    int val, locale_len[6] = { 0 };
    char buf[256];
    BOOL sname_match;
    wchar_t wbuf[256], map_buf[256];
    int i;

    TRACE("(%d %s)\n", category, locale);

    if(category<LC_MIN || category>LC_MAX || !locale)
        return NULL;

    if(locale[0]=='C' && !locale[1]) {
        locale_sname[0] = NULL;
        cp[0] = CP_ACP;
    } else if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_') {
        const char *p;

        while(1) {
            locale += 3; /* LC_ */
            if(!memcmp(locale, collate, sizeof(collate)-1)) {
                i = LC_COLLATE;
                locale += sizeof(collate)-1;
            } else if(!memcmp(locale, ctype, sizeof(ctype)-1)) {
                i = LC_CTYPE;
                locale += sizeof(ctype)-1;
            } else if(!memcmp(locale, monetary, sizeof(monetary)-1)) {
                i = LC_MONETARY;
                locale += sizeof(monetary)-1;
            } else if(!memcmp(locale, numeric, sizeof(numeric)-1)) {
                i = LC_NUMERIC;
                locale += sizeof(numeric)-1;
            } else if(!memcmp(locale, time, sizeof(time)-1)) {
                i = LC_TIME;
                locale += sizeof(time)-1;
            } else
                goto fail;

            p = strchr(locale, ';');
            if(locale[0]=='C' && (locale[1]==';' || locale[1]=='\0')) {
                locale_sname[i] = NULL;
                cp[i] = CP_ACP;
            } else {
                BOOL locale_found = FALSE;

                if(p) {
                    memcpy(buf, locale, p-locale);
                    buf[p-locale] = '\0';
                    locale_found = locale_to_sname(buf, &cp[i], &sname_match, wbuf);
                } else {
                    locale_found = locale_to_sname(locale, &cp[i], &sname_match, wbuf);
                }

                if(!locale_found || !(locale_sname[i] = wcsdup(wbuf)))
                    goto fail;
                if(sname_match) {
                    locale_name[i] = locale;
                    locale_len[i] = p ? p-locale : strlen(locale);
                }
            }

            if(!p || *(p+1)!='L' || *(p+2)!='C' || *(p+3)!='_')
                break;

            locale = p+1;
        }
    } else {
        BOOL locale_found = locale_to_sname(locale, &cp[0], &sname_match, wbuf);

        if(!locale_found)
            return NULL;

        locale_sname[0] = wcsdup(wbuf);
        if(!locale_sname[0])
            return NULL;

        if(sname_match) {
            locale_name[0] = locale;
            locale_len[0] = strlen(locale);
        }

        for(i=1; i<6; i++) {
            locale_sname[i] = wcsdup(locale_sname[0]);
            if(!locale_sname[i])
                goto fail;

            cp[i] = cp[0];
            locale_name[i] = locale_name[0];
            locale_len[i] = locale_len[0];
        }
    }

    for(i=1; i<6; i++) {
#if _MSVCR_VER < 140
        if(i==LC_CTYPE && cp[i]==CP_UTF8) {
#if _MSVCR_VER >= 110
            if(old_locinfo) {
                locale_sname[i] = wcsdup(old_locinfo->lc_name[i]);
                if (old_locinfo->lc_name[i] && !locale_sname[i])
                    goto fail;
            }
#else
            int sname_size;
            if(old_locinfo && old_locinfo->lc_handle[i]) {
                sname_size = LCIDToLocaleName(old_locinfo->lc_handle[i], NULL, 0, 0);
                locale_sname[i] = malloc(sname_size * sizeof(WCHAR));
                if(!locale_sname[i])
                    goto fail;
                LCIDToLocaleName(old_locinfo->lc_handle[i], locale_sname[i], sname_size, 0);
            } else {
                locale_sname[i] = NULL;
            }
#endif

            locale_name[i] = NULL;
            locale_len[i] = 0;
            cp[i] = old_locinfo ? old_locinfo->lc_id[i].wCodePage : 0;
        }
#endif
        if(category!=LC_ALL && category!=i) {
            if(old_locinfo) {
#if _MSVCR_VER >= 110
                locale_sname[i] = wcsdup(old_locinfo->lc_name[i]);
                if(old_locinfo->lc_name[i] && !locale_sname[i])
                    goto fail;
#else
                int sname_size;
                if(old_locinfo->lc_handle[i]) {
                    sname_size = LCIDToLocaleName(old_locinfo->lc_handle[i], NULL, 0, 0);
                    locale_sname[i] = malloc(sname_size * sizeof(WCHAR));
                    if(!locale_sname[i])
                        goto fail;
                    LCIDToLocaleName(old_locinfo->lc_handle[i], locale_sname[i], sname_size, 0);
                } else {
                    locale_sname[i] = NULL;
                }
#endif
                cp[i] = old_locinfo->lc_id[i].wCodePage;
            } else {
                locale_sname[i] = NULL;
                cp[i] = 0;
            }
        }
    }

    locinfo = malloc(sizeof(threadlocinfo));
    if(!locinfo)
        goto fail;

    memset(locinfo, 0, sizeof(threadlocinfo));
    locinfo->refcount = 1;

    if(locale_name[LC_COLLATE] &&
            !init_category_name(locale_name[LC_COLLATE],
                locale_len[LC_COLLATE], locinfo, LC_COLLATE)) {
        goto fail;
    }

    if(!category_needs_update(LC_COLLATE, old_locinfo,
                locale_sname[LC_COLLATE], cp[LC_COLLATE])) {
        copy_threadlocinfo_category(locinfo, old_locinfo, LC_COLLATE);
        locinfo->lc_collate_cp = old_locinfo->lc_collate_cp;
    } else if(locale_sname[LC_COLLATE]) {
        if(!update_threadlocinfo_category(locale_sname[LC_COLLATE],
                    cp[LC_COLLATE], locinfo, LC_COLLATE)) {
            goto fail;
        }

        locinfo->lc_collate_cp = locinfo->lc_id[LC_COLLATE].wCodePage;
    } else {
        if(!init_category_name("C", 1, locinfo, LC_COLLATE)) {
            goto fail;
        }
    }

    if(locale_name[LC_CTYPE] &&
            !init_category_name(locale_name[LC_CTYPE],
                locale_len[LC_CTYPE], locinfo, LC_CTYPE)) {
        goto fail;
    }

    if(!category_needs_update(LC_CTYPE, old_locinfo,
                locale_sname[LC_CTYPE], cp[LC_CTYPE])) {
        copy_threadlocinfo_category(locinfo, old_locinfo, LC_CTYPE);
        locinfo->lc_codepage = old_locinfo->lc_codepage;
        locinfo->lc_clike = old_locinfo->lc_clike;
        locinfo->mb_cur_max = old_locinfo->mb_cur_max;
        locinfo->ctype1 = old_locinfo->ctype1;
        locinfo->ctype1_refcount = old_locinfo->ctype1_refcount;
        locinfo->pctype = old_locinfo->pctype;
        locinfo->pclmap = old_locinfo->pclmap;
        locinfo->pcumap = old_locinfo->pcumap;
        if(locinfo->ctype1_refcount)
            InterlockedIncrement((LONG *)locinfo->ctype1_refcount);
    } else if(locale_sname[LC_CTYPE]) {
        CPINFO cp_info;
        int j;

        if(!update_threadlocinfo_category(locale_sname[LC_CTYPE],
                    cp[LC_CTYPE], locinfo, LC_CTYPE)) {
            goto fail;
        }

        locinfo->lc_codepage = locinfo->lc_id[LC_CTYPE].wCodePage;
        locinfo->lc_clike = 1;
        if(!GetCPInfo(locinfo->lc_codepage, &cp_info)) {
            goto fail;
        }
        locinfo->mb_cur_max = cp_info.MaxCharSize;

        locinfo->ctype1_refcount = malloc(sizeof(int));
        if(!locinfo->ctype1_refcount) {
            goto fail;
        }
        *locinfo->ctype1_refcount = 1;

        locinfo->ctype1 = malloc(257 * sizeof(*locinfo->ctype1));
        locinfo->pclmap = malloc(256 * sizeof(*locinfo->pclmap));
        locinfo->pcumap = malloc(256 * sizeof(*locinfo->pcumap));
        if(!locinfo->ctype1 || !locinfo->pclmap || !locinfo->pcumap) {
            goto fail;
        }

        locinfo->ctype1[0] = 0;
        locinfo->pctype = locinfo->ctype1+1;

        buf[1] = buf[2] = '\0';
        for(i=1; i<257; i++) {
            buf[0] = i-1;

            MultiByteToWideChar(locinfo->lc_codepage, 0, buf, 1, wbuf, 1);
            /* builtin GetStringType doesn't set output to 0 on invalid input */
            locinfo->ctype1[i] = 0;
            GetStringTypeW(CT_CTYPE1, wbuf, 1, &locinfo->ctype1[i]);
        }

        for(i=0; cp_info.LeadByte[i+1]!=0; i+=2)
            for(j=cp_info.LeadByte[i]; j<=cp_info.LeadByte[i+1]; j++)
                locinfo->ctype1[j+1] |= _LEADBYTE;

        for(i=0; i<256; i++) {
            if(locinfo->pctype[i] & _LEADBYTE)
                buf[i] = ' ';
            else
                buf[i] = i;
        }

        MultiByteToWideChar(locinfo->lc_codepage, 0, buf, 256, wbuf, 256);
        LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, wbuf, 256, map_buf, 256);
        WideCharToMultiByte(locinfo->lc_codepage, 0, map_buf, 256, (char *)locinfo->pclmap, 256, NULL, NULL);
        LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, wbuf, 256, map_buf, 256);
        WideCharToMultiByte(locinfo->lc_codepage, 0, map_buf, 256, (char *)locinfo->pcumap, 256, NULL, NULL);
    } else {
        locinfo->lc_clike = 1;
        locinfo->mb_cur_max = 1;
        locinfo->pctype = MSVCRT__ctype+1;
        locinfo->pclmap = cloc_clmap;
        locinfo->pcumap = cloc_cumap;
        if(!init_category_name("C", 1, locinfo, LC_CTYPE)) {
            goto fail;
        }
    }

    if(!category_needs_update(LC_MONETARY, old_locinfo,
                locale_sname[LC_MONETARY], cp[LC_MONETARY]) &&
            !category_needs_update(LC_NUMERIC, old_locinfo,
                locale_sname[LC_NUMERIC], cp[LC_NUMERIC])) {
        locinfo->lconv = old_locinfo->lconv;
        locinfo->lconv_intl_refcount = old_locinfo->lconv_intl_refcount;
        if(locinfo->lconv_intl_refcount)
            InterlockedIncrement((LONG *)locinfo->lconv_intl_refcount);
    } else if(locale_sname[LC_MONETARY] || locale_sname[LC_NUMERIC]) {
        locinfo->lconv = malloc(sizeof(struct lconv));
        locinfo->lconv_intl_refcount = malloc(sizeof(int));
        if(!locinfo->lconv || !locinfo->lconv_intl_refcount) {
            free(locinfo->lconv);
            free(locinfo->lconv_intl_refcount);
            locinfo->lconv = NULL;
            locinfo->lconv_intl_refcount = NULL;
            goto fail;
        }
        memset(locinfo->lconv, 0, sizeof(struct lconv));
        *locinfo->lconv_intl_refcount = 1;
    } else {
        locinfo->lconv = &cloc_lconv;
    }

    if(locale_name[LC_MONETARY] &&
            !init_category_name(locale_name[LC_MONETARY],
                locale_len[LC_MONETARY], locinfo, LC_MONETARY)) {
        goto fail;
    }

    if(!category_needs_update(LC_MONETARY, old_locinfo,
                locale_sname[LC_MONETARY], cp[LC_MONETARY])) {
        copy_threadlocinfo_category(locinfo, old_locinfo, LC_MONETARY);
        locinfo->lconv_mon_refcount = old_locinfo->lconv_mon_refcount;
        if(locinfo->lconv_mon_refcount)
            InterlockedIncrement((LONG *)locinfo->lconv_mon_refcount);
        if(locinfo->lconv != &cloc_lconv && locinfo->lconv != old_locinfo->lconv) {
            locinfo->lconv->int_curr_symbol = old_locinfo->lconv->int_curr_symbol;
            locinfo->lconv->currency_symbol = old_locinfo->lconv->currency_symbol;
            locinfo->lconv->mon_decimal_point = old_locinfo->lconv->mon_decimal_point;
            locinfo->lconv->mon_thousands_sep = old_locinfo->lconv->mon_thousands_sep;
            locinfo->lconv->mon_grouping = old_locinfo->lconv->mon_grouping;
            locinfo->lconv->positive_sign = old_locinfo->lconv->positive_sign;
            locinfo->lconv->negative_sign = old_locinfo->lconv->negative_sign;
            locinfo->lconv->int_frac_digits = old_locinfo->lconv->int_frac_digits;
            locinfo->lconv->frac_digits = old_locinfo->lconv->frac_digits;
            locinfo->lconv->p_cs_precedes = old_locinfo->lconv->p_cs_precedes;
            locinfo->lconv->p_sep_by_space = old_locinfo->lconv->p_sep_by_space;
            locinfo->lconv->n_cs_precedes = old_locinfo->lconv->n_cs_precedes;
            locinfo->lconv->n_sep_by_space = old_locinfo->lconv->n_sep_by_space;
            locinfo->lconv->p_sign_posn = old_locinfo->lconv->p_sign_posn;
            locinfo->lconv->n_sign_posn = old_locinfo->lconv->n_sign_posn;
#if _MSVCR_VER >= 100
            locinfo->lconv->_W_int_curr_symbol = old_locinfo->lconv->_W_int_curr_symbol;
            locinfo->lconv->_W_currency_symbol = old_locinfo->lconv->_W_currency_symbol;
            locinfo->lconv->_W_mon_decimal_point = old_locinfo->lconv->_W_mon_decimal_point;
            locinfo->lconv->_W_mon_thousands_sep = old_locinfo->lconv->_W_mon_thousands_sep;
            locinfo->lconv->_W_positive_sign = old_locinfo->lconv->_W_positive_sign;
            locinfo->lconv->_W_negative_sign = old_locinfo->lconv->_W_negative_sign;
#endif
        }
    } else if(locale_sname[LC_MONETARY]) {
        if(!update_threadlocinfo_category(locale_sname[LC_MONETARY],
                    cp[LC_MONETARY], locinfo, LC_MONETARY)) {
            goto fail;
        }

        locinfo->lconv_mon_refcount = malloc(sizeof(int));
        if(!locinfo->lconv_mon_refcount) {
            goto fail;
        }

        *locinfo->lconv_mon_refcount = 1;

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SINTLSYMBOL
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->int_curr_symbol = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->int_curr_symbol, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SCURRENCY
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->currency_symbol = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->currency_symbol, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SMONDECIMALSEP
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->mon_decimal_point = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->mon_decimal_point, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SMONTHOUSANDSEP
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->mon_thousands_sep = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->mon_thousands_sep, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SMONGROUPING
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf, 256, NULL, NULL);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (locinfo->lconv->mon_grouping = malloc(i))) {
            for(i=0; buf[i+1]==';'; i+=2)
                locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                locinfo->lconv->mon_grouping[i/2+1] = 127;
        } else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SPOSITIVESIGN
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->positive_sign = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->positive_sign, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SNEGATIVESIGN
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->negative_sign = malloc(i)))
            WideCharToMultiByte(cp[LC_MONETARY], 0, wbuf, -1, locinfo->lconv->negative_sign, i, NULL, NULL);
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_IINTLCURRDIGITS
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->int_frac_digits = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_ICURRDIGITS
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->frac_digits = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_IPOSSYMPRECEDES
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->p_cs_precedes = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_IPOSSEPBYSPACE
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->p_sep_by_space = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_INEGSYMPRECEDES
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->n_cs_precedes = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_INEGSEPBYSPACE
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->n_sep_by_space = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_IPOSSIGNPOSN
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->p_sign_posn = val;
        else {
            goto fail;
        }

        if(GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_INEGSIGNPOSN
                          |LOCALE_NOUSEROVERRIDE|LOCALE_RETURN_NUMBER, (WCHAR *)&val, 2))
            locinfo->lconv->n_sign_posn = val;
        else {
            goto fail;
        }

#if _MSVCR_VER >= 100
        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SINTLSYMBOL
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_int_curr_symbol = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_int_curr_symbol, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SCURRENCY
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_currency_symbol = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_currency_symbol, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SMONDECIMALSEP
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_mon_decimal_point = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_mon_decimal_point, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SMONTHOUSANDSEP
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_mon_thousands_sep = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_mon_thousands_sep, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SPOSITIVESIGN
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_positive_sign = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_positive_sign, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_MONETARY], LOCALE_SNEGATIVESIGN
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_negative_sign = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_negative_sign, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }
#endif
    } else {
        if (locinfo->lconv != &cloc_lconv) {
            locinfo->lconv->int_curr_symbol = cloc_lconv.int_curr_symbol;
            locinfo->lconv->currency_symbol = cloc_lconv.currency_symbol;
            locinfo->lconv->mon_decimal_point = cloc_lconv.mon_decimal_point;
            locinfo->lconv->mon_thousands_sep = cloc_lconv.mon_thousands_sep;
            locinfo->lconv->mon_grouping = cloc_lconv.mon_grouping;
            locinfo->lconv->positive_sign = cloc_lconv.positive_sign;
            locinfo->lconv->negative_sign = cloc_lconv.negative_sign;
            locinfo->lconv->int_frac_digits = cloc_lconv.int_frac_digits;
            locinfo->lconv->frac_digits = cloc_lconv.frac_digits;
            locinfo->lconv->p_cs_precedes = cloc_lconv.p_cs_precedes;
            locinfo->lconv->p_sep_by_space = cloc_lconv.p_sep_by_space;
            locinfo->lconv->n_cs_precedes = cloc_lconv.n_cs_precedes;
            locinfo->lconv->n_sep_by_space = cloc_lconv.n_sep_by_space;
            locinfo->lconv->p_sign_posn = cloc_lconv.p_sign_posn;
            locinfo->lconv->n_sign_posn = cloc_lconv.n_sign_posn;

#if _MSVCR_VER >= 100
            locinfo->lconv->_W_int_curr_symbol = cloc_lconv._W_int_curr_symbol;
            locinfo->lconv->_W_currency_symbol = cloc_lconv._W_currency_symbol;
            locinfo->lconv->_W_mon_decimal_point = cloc_lconv._W_mon_decimal_point;
            locinfo->lconv->_W_mon_thousands_sep = cloc_lconv._W_mon_thousands_sep;
            locinfo->lconv->_W_positive_sign = cloc_lconv._W_positive_sign;
            locinfo->lconv->_W_negative_sign = cloc_lconv._W_negative_sign;
#endif
        }

        if(!init_category_name("C", 1, locinfo, LC_MONETARY)) {
            goto fail;
        }
    }

    if(locale_name[LC_NUMERIC] &&
            !init_category_name(locale_name[LC_NUMERIC],
                locale_len[LC_NUMERIC], locinfo, LC_NUMERIC)) {
        goto fail;
    }

    if(!category_needs_update(LC_NUMERIC, old_locinfo,
                locale_sname[LC_NUMERIC], cp[LC_NUMERIC])) {
        copy_threadlocinfo_category(locinfo, old_locinfo, LC_NUMERIC);
        locinfo->lconv_num_refcount = old_locinfo->lconv_num_refcount;
        if(locinfo->lconv_num_refcount)
            InterlockedIncrement((LONG *)locinfo->lconv_num_refcount);
        if(locinfo->lconv != &cloc_lconv && locinfo->lconv != old_locinfo->lconv) {
            locinfo->lconv->decimal_point = old_locinfo->lconv->decimal_point;
            locinfo->lconv->thousands_sep = old_locinfo->lconv->thousands_sep;
            locinfo->lconv->grouping = old_locinfo->lconv->grouping;
#if _MSVCR_VER >= 100
            locinfo->lconv->_W_decimal_point = old_locinfo->lconv->_W_decimal_point;
            locinfo->lconv->_W_thousands_sep = old_locinfo->lconv->_W_thousands_sep;
#endif
        }
    } else if(locale_sname[LC_NUMERIC]) {
        if(!update_threadlocinfo_category(locale_sname[LC_NUMERIC],
                    cp[LC_NUMERIC], locinfo, LC_NUMERIC)) {
            goto fail;
        }

        locinfo->lconv_num_refcount = malloc(sizeof(int));
        if(!locinfo->lconv_num_refcount) {
            goto fail;
        }

        *locinfo->lconv_num_refcount = 1;

        i = GetLocaleInfoEx(locale_sname[LC_NUMERIC], LOCALE_SDECIMAL
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_NUMERIC], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->decimal_point = malloc(i)))
            WideCharToMultiByte(cp[LC_NUMERIC], 0, wbuf, -1, locinfo->lconv->decimal_point, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_NUMERIC], LOCALE_STHOUSAND
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        i = WideCharToMultiByte(cp[LC_NUMERIC], 0, wbuf, -1, NULL, 0, NULL, NULL);
        if(i && (locinfo->lconv->thousands_sep = malloc(i)))
            WideCharToMultiByte(cp[LC_NUMERIC], 0, wbuf, -1, locinfo->lconv->thousands_sep, i, NULL, NULL);
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_NUMERIC], LOCALE_SGROUPING
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        WideCharToMultiByte(cp[LC_NUMERIC], 0, wbuf, -1, buf, 256, NULL, NULL);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (locinfo->lconv->grouping = malloc(i))) {
            for(i=0; buf[i+1]==';'; i+=2)
                locinfo->lconv->grouping[i/2] = buf[i]-'0';
            locinfo->lconv->grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                locinfo->lconv->grouping[i/2+1] = 127;
        } else {
            goto fail;
        }

#if _MSVCR_VER >= 100
        i = GetLocaleInfoEx(locale_sname[LC_NUMERIC], LOCALE_SDECIMAL
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_decimal_point = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_decimal_point, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }

        i = GetLocaleInfoEx(locale_sname[LC_NUMERIC], LOCALE_STHOUSAND
                |LOCALE_NOUSEROVERRIDE, wbuf, 256);
        if(i && (locinfo->lconv->_W_thousands_sep = malloc(i * sizeof(wchar_t))))
            memcpy(locinfo->lconv->_W_thousands_sep, wbuf, i * sizeof(wchar_t));
        else {
            goto fail;
        }
#endif
    } else {
        if (locinfo->lconv != &cloc_lconv) {
            locinfo->lconv->decimal_point = cloc_lconv.decimal_point;
            locinfo->lconv->thousands_sep = cloc_lconv.thousands_sep;
            locinfo->lconv->grouping = cloc_lconv.grouping;

#if _MSVCR_VER >= 100
            locinfo->lconv->_W_decimal_point = cloc_lconv._W_decimal_point;
            locinfo->lconv->_W_thousands_sep = cloc_lconv._W_thousands_sep;
#endif
        }

        if (!init_category_name("C", 1, locinfo, LC_NUMERIC)) {
            goto fail;
        }
    }

    if(locale_name[LC_TIME] &&
            !init_category_name(locale_name[LC_TIME],
                locale_len[LC_TIME], locinfo, LC_TIME)) {
        goto fail;
    }

    if(!category_needs_update(LC_TIME, old_locinfo,
                locale_sname[LC_TIME], cp[LC_TIME])) {
        copy_threadlocinfo_category(locinfo, old_locinfo, LC_TIME);
        locinfo->lc_time_curr = old_locinfo->lc_time_curr;
        InterlockedIncrement(&locinfo->lc_time_curr->refcount);
    } else if(locale_sname[LC_TIME]) {
        if(!update_threadlocinfo_category(locale_sname[LC_TIME],
                    cp[LC_TIME], locinfo, LC_TIME)) {
            goto fail;
        }

        locinfo->lc_time_curr = create_time_data(locale_sname[LC_TIME]);
        if(!locinfo->lc_time_curr) {
            goto fail;
        }
    } else {
        if(!init_category_name("C", 1, locinfo, LC_TIME)) {
            goto fail;
        }
        locinfo->lc_time_curr = &cloc_time_data;
        InterlockedIncrement(&locinfo->lc_time_curr->refcount);
    }

    for (i = 0; i < LC_MAX; i++)
        free(locale_sname[i]);

    return locinfo;

fail:
    free_locinfo(locinfo);

    for (i = 0; i < LC_MAX; i++)
        free(locale_sname[i]);

    return NULL;
}

/*********************************************************************
 *      _create_locale (MSVCRT.@)
 */
_locale_t CDECL _create_locale(int category, const char *locale)
{
    _locale_t loc;

    loc = malloc(sizeof(_locale_tstruct));
    if(!loc)
        return NULL;

    loc->locinfo = create_locinfo(category, locale, NULL);
    if(!loc->locinfo) {
        free(loc);
        return NULL;
    }

    loc->mbcinfo = create_mbcinfo(loc->locinfo->lc_id[LC_CTYPE].wCodePage,
            loc->locinfo->lc_handle[LC_CTYPE], NULL);
    if(!loc->mbcinfo) {
        free_locinfo(loc->locinfo);
        free(loc);
        return NULL;
    }
    return loc;
}

#if _MSVCR_VER >= 110
/*********************************************************************
 *      _wcreate_locale (MSVCR110.@)
 */
_locale_t CDECL _wcreate_locale(int category, const wchar_t *locale)
{
    _locale_t loc;
    size_t len;
    char *str;

    if(category<LC_MIN || category>LC_MAX || !locale)
        return NULL;

    len = wcstombs(NULL, locale, 0);
    if(len == -1)
        return NULL;
    if(!(str = malloc(++len)))
        return NULL;
    wcstombs(str, locale, len);

    loc = _create_locale(category, str);

    free(str);
    return loc;
}
#endif

/*********************************************************************
 *             setlocale (MSVCRT.@)
 */
char* CDECL setlocale(int category, const char* locale)
{
    thread_data_t *data = msvcrt_get_thread_data();
    pthreadlocinfo locinfo = get_locinfo(), newlocinfo;
    int locale_flags;

    if(category<LC_MIN || category>LC_MAX)
        return NULL;

    if(!locale) {
        if(category == LC_ALL)
            return construct_lc_all(locinfo);

        return locinfo->lc_category[category].locale;
    }

    /* Make sure that locinfo is not updated by e.g. stricmp function */
    locale_flags = data->locale_flags;
    data->locale_flags |= LOCALE_THREAD;
    newlocinfo = create_locinfo(category, locale, locinfo);
    data->locale_flags = locale_flags;
    if(!newlocinfo) {
        WARN("%d %s failed\n", category, locale);
        return NULL;
    }

    if(locale[0] != 'C' || locale[1] != '\0')
        initial_locale = FALSE;

    if(data->locale_flags & LOCALE_THREAD)
    {
        if(data->locale_flags & LOCALE_FREE)
            free_locinfo(data->locinfo);
        data->locinfo = newlocinfo;
    }
    else
    {
        int i;

        _lock_locales();
        free_locinfo(MSVCRT_locale->locinfo);
        MSVCRT_locale->locinfo = newlocinfo;

        MSVCRT___lc_codepage = newlocinfo->lc_codepage;
        MSVCRT___lc_collate_cp = newlocinfo->lc_collate_cp;
        MSVCRT___mb_cur_max = newlocinfo->mb_cur_max;
        MSVCRT__pctype = newlocinfo->pctype;
        for(i=LC_MIN; i<=LC_MAX; i++)
            MSVCRT___lc_handle[i] = MSVCRT_locale->locinfo->lc_handle[i];
        _unlock_locales();
        update_thread_locale(data);
    }

    if(category == LC_ALL)
        return construct_lc_all(data->locinfo);

    return data->locinfo->lc_category[category].locale;
}

/*********************************************************************
 *		_wsetlocale (MSVCRT.@)
 */
wchar_t* CDECL _wsetlocale(int category, const wchar_t* wlocale)
{
    static wchar_t current_lc_all[MAX_LOCALE_LENGTH];

    char *locale = NULL;
    const char *ret;
    size_t len;

    if(wlocale) {
        len = wcstombs(NULL, wlocale, 0);
        if(len == -1)
            return NULL;

        locale = malloc(++len);
        if(!locale)
            return NULL;

        wcstombs(locale, wlocale, len);
    }

    _lock_locales();
    ret = setlocale(category, locale);
    free(locale);

    if(ret && mbstowcs(current_lc_all, ret, MAX_LOCALE_LENGTH)==-1)
        ret = NULL;

    _unlock_locales();
    return ret ? current_lc_all : NULL;
}

#if _MSVCR_VER >= 80
/*********************************************************************
 *		_configthreadlocale (MSVCR80.@)
 */
int CDECL _configthreadlocale(int type)
{
    thread_data_t *data = msvcrt_get_thread_data();
    int ret;

    ret = (data->locale_flags & LOCALE_THREAD ? _ENABLE_PER_THREAD_LOCALE :
            _DISABLE_PER_THREAD_LOCALE);

    if(type == _ENABLE_PER_THREAD_LOCALE)
        data->locale_flags |= LOCALE_THREAD;
    else if(type == _DISABLE_PER_THREAD_LOCALE)
        data->locale_flags &= ~LOCALE_THREAD;
    else if(type)
        ret = -1;

    return ret;
}
#endif

BOOL msvcrt_init_locale(void)
{
    int i;

    _lock_locales();
    MSVCRT_locale = _create_locale(0, "C");
    _unlock_locales();
    if(!MSVCRT_locale)
        return FALSE;

    MSVCRT___lc_codepage = MSVCRT_locale->locinfo->lc_codepage;
    MSVCRT___lc_collate_cp = MSVCRT_locale->locinfo->lc_collate_cp;
    MSVCRT___mb_cur_max = MSVCRT_locale->locinfo->mb_cur_max;
    MSVCRT__pctype = MSVCRT_locale->locinfo->pctype;
    for(i=LC_MIN; i<=LC_MAX; i++)
        MSVCRT___lc_handle[i] = MSVCRT_locale->locinfo->lc_handle[i];
    _setmbcp(_MB_CP_ANSI);
    return TRUE;
}

#if _MSVCR_VER >= 120
/*********************************************************************
 *      wctrans (MSVCR120.@)
 */
wctrans_t CDECL wctrans(const char *property)
{
    static const char str_tolower[] = "tolower";
    static const char str_toupper[] = "toupper";

    if(!strcmp(property, str_tolower))
        return 2;
    if(!strcmp(property, str_toupper))
        return 1;
    return 0;
}
#endif
