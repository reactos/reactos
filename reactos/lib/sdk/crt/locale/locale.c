/*
 * Some stuff takem from wine msvcrt\locale.c
 *
 * Copyright 2000 Jon Griffiths
 */

#include <precomp.h>
#include <locale.h>

#include "mbctype.h"

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

unsigned char MSVCRT_mbctype[257] = { 0 };
static int g_mbcp_is_multibyte = 0;

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
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};


char MSVCRT_current_lc_all[MAX_LOCALE_LENGTH] = { 0 };
LCID MSVCRT_current_lc_all_lcid = 0;
int MSVCRT___lc_codepage = 0;
int MSVCRT___lc_collate_cp = 0;
HANDLE MSVCRT___lc_handle[MSVCRT_LC_MAX - MSVCRT_LC_MIN + 1] = { 0 };

/* MT */
#define LOCK_LOCALE   _mlock(_SETLOCALE_LOCK);
#define UNLOCK_LOCALE _munlock(_SETLOCALE_LOCK);

#define MSVCRT_LEADBYTE  0x8000

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

unsigned int __setlc_active;
unsigned int __unguarded_readlc_active;
int _current_category;	/* used by setlocale */
const char *_current_locale;


int parse_locale(const char *locale, char *lang, char *country, char *code_page);

#define _C_ _CONTROL
#define _S_ _SPACE
#define _P_ _PUNCT
#define _D_ _DIGIT
#define _H_ _HEX
#define _U_ _UPPER
#define _L_ _LOWER

WORD MSVCRT__ctype [257] = {
  0, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_C_, _S_|_C_,
  _S_|_C_, _S_|_C_, _S_|_C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_,
  _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_BLANK,
  _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_,
  _P_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_,
  _D_|_H_, _D_|_H_, _D_|_H_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _U_|_H_,
  _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_, _U_, _U_, _U_, _U_,
  _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_,
  _U_, _P_, _P_, _P_, _P_, _P_, _P_, _L_|_H_, _L_|_H_, _L_|_H_, _L_|_H_,
  _L_|_H_, _L_|_H_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_,
  _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _P_, _P_, _P_, _P_,
  _C_, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Internal: Current ctype table for locale */
WORD MSVCRT_current_ctype[257];

/* pctype is used by macros in the Win32 headers. It must point
 * To a table of flags exactly like ctype. To allow locale
 * changes to affect ctypes (i.e. isleadbyte), we use a second table
 * and update its flags whenever the current locale changes.
 */
WORD* MSVCRT__pctype = MSVCRT_current_ctype + 1;

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

/* Note: Flags are weighted in order of matching importance */
#define FOUND_LANGUAGE         0x4
#define FOUND_COUNTRY          0x2
#define FOUND_CODEPAGE         0x1

/* INTERNAL: Map a synonym to an ISO code */
static void remap_synonym(char *name)
{
  size_t i;
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
  if (flags & (FOUND_LANGUAGE & FOUND_COUNTRY & FOUND_CODEPAGE))
  {
    TRACE(":found exact locale match\n");
    return STOP_LOOKING;
  }
  return CONTINUE_LOOKING;
}

/* Internal: Find the LCID for a locale specification */
static LCID MSVCRT_locale_to_LCID(locale_search_t* locale)
{
  LCID lcid;
  EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), (LPSTR)RT_STRING,
			 (LPCSTR)LOCALE_ILANGUAGE,find_best_locale_proc,
			 (LONG_PTR)locale);

  if (!locale->match_flags)
    return 0;

  /* If we were given something that didn't match, fail */
  if (locale->search_country[0] && !(locale->match_flags & FOUND_COUNTRY))
    return 0;

  lcid =  MAKELCID(locale->found_lang_id, SORT_DEFAULT);

  /* Populate partial locale, translating LCID to locale string elements */
  if (!locale->found_codepage[0])
  {
    /* Even if a codepage is not enumerated for a locale
     * it can be set if valid */
    if (locale->search_codepage[0])
    {
      if (IsValidCodePage(atoi(locale->search_codepage)))
        memcpy(locale->found_codepage,locale->search_codepage,MAX_ELEM_LEN);
      else
      {
        /* Special codepage values: OEM & ANSI */
        if (strcasecmp(locale->search_codepage,"OCP"))
        {
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
        }
        if (strcasecmp(locale->search_codepage,"ACP"))
        {
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
        }
        else
          return 0;

        if (!atoi(locale->found_codepage))
           return 0;
      }
    }
    else
    {
      /* Prefer ANSI codepages if present */
      GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                     locale->found_codepage, MAX_ELEM_LEN);
      if (!locale->found_codepage[0] || !atoi(locale->found_codepage))
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
    }
  }
  GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE|LOCALE_NOUSEROVERRIDE,
                 locale->found_language, MAX_ELEM_LEN);
  GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY|LOCALE_NOUSEROVERRIDE,
                 locale->found_country, MAX_ELEM_LEN);
  return lcid;
}

/* INTERNAL: Set ctype behaviour for a codepage */
static void msvcrt_set_ctype(unsigned int codepage, LCID lcid)
{
  CPINFO cp;

  memset(&cp, 0, sizeof(CPINFO));

  if (GetCPInfo(codepage, &cp))
  {
    int i;
    char str[3];
    unsigned char *traverse = (unsigned char *)cp.LeadByte;

    memset(MSVCRT_current_ctype, 0, sizeof(MSVCRT__ctype));
    MSVCRT___lc_codepage = codepage;
    MSVCRT___lc_collate_cp = codepage;

    /* Switch ctype macros to MBCS if needed */
    __mb_cur_max = cp.MaxCharSize;

    /* Set remaining ctype flags: FIXME: faster way to do this? */
    str[1] = str[2] = 0;
    for (i = 0; i < 256; i++)
    {
      if (!(MSVCRT__pctype[i] & MSVCRT_LEADBYTE))
      {
        str[0] = i;
        GetStringTypeA(lcid, CT_CTYPE1, str, 1, MSVCRT__pctype + i);
      }
    }

    /* Set leadbyte flags */
    while (traverse[0] || traverse[1])
    {
      for( i = traverse[0]; i <= traverse[1]; i++ )
        MSVCRT_current_ctype[i+1] |= MSVCRT_LEADBYTE;
      traverse += 2;
    };
  }
}


/*
 * @implemented
 */
char *setlocale(int category, const char *locale)
{
  LCID lcid = 0;
  locale_search_t lc;
  int haveLang, haveCountry, haveCP;
  char* next;
  int lc_all = 0;

  TRACE("(%d %s)\n",category,locale);

  if (category < MSVCRT_LC_MIN || category > MSVCRT_LC_MAX)
    return NULL;

  if (locale == NULL)
  {
    /* Report the current Locale */
    return MSVCRT_current_lc_all;
  }

  LOCK_LOCALE;

  if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_')
  {
    WARN(":restore previous locale not implemented!\n");
    /* FIXME: Easiest way to do this is parse the string and
     * call this function recursively with its elements,
     * Where they differ for each lc_ type.
     */
    UNLOCK_LOCALE;
    return MSVCRT_current_lc_all;
  }

  /* Default Locale: Special case handling */
  if (!strlen(locale) || ((toupper(locale[0]) == 'C') && !locale[1]))
  {
    MSVCRT_current_lc_all[0] = 'C';
    MSVCRT_current_lc_all[1] = '\0';
    MSVCRT___lc_codepage = 1252;
    MSVCRT___lc_collate_cp = 1252;

    switch (category) {
    case MSVCRT_LC_ALL:
      lc_all = 1; /* Fall through all cases ... */
    case MSVCRT_LC_COLLATE:
      if (!lc_all) break;
    case MSVCRT_LC_CTYPE:
      /* Restore C locale ctype info */
      __mb_cur_max = 1;
      memcpy(MSVCRT_current_ctype, MSVCRT__ctype, sizeof(MSVCRT__ctype));
      if (!lc_all) break;
    case MSVCRT_LC_MONETARY:
      if (!lc_all) break;
    case MSVCRT_LC_NUMERIC:
      if (!lc_all) break;
    case MSVCRT_LC_TIME:
      break;
    }
    UNLOCK_LOCALE;
    return MSVCRT_current_lc_all;
  }

  /* Get locale elements */
  haveLang = haveCountry = haveCP = 0;
  memset(&lc,0,sizeof(lc));

  next = strchr(locale,'_');
  if (next && next != locale)
  {
    haveLang = 1;
    memcpy(lc.search_language,locale,next-locale);
    locale += next-locale+1;
  }

  next = strchr(locale,'.');
  if (next)
  {
    haveCP = 1;
    if (next == locale)
    {
      locale++;
      lstrcpynA(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
    else
    {
      if (haveLang)
      {
        haveCountry = 1;
        memcpy(lc.search_country,locale,next-locale);
        locale += next-locale+1;
      }
      else
      {
        haveLang = 1;
        memcpy(lc.search_language,locale,next-locale);
        locale += next-locale+1;
      }
      lstrcpynA(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
  }
  else
  {
    if (haveLang)
    {
      haveCountry = 1;
      lstrcpynA(lc.search_country, locale, MAX_ELEM_LEN);
    }
    else
    {
      haveLang = 1;
      lstrcpynA(lc.search_language, locale, MAX_ELEM_LEN);
    }
  }

  if (haveCountry)
    remap_synonym(lc.search_country);

  if (haveCP && !haveCountry && !haveLang)
  {
    ERR(":Codepage only locale not implemented\n");
    /* FIXME: Use default lang/country and skip locale_to_LCID()
     * call below...
     */
    UNLOCK_LOCALE;
    return NULL;
  }

  lcid = MSVCRT_locale_to_LCID(&lc);

  TRACE(":found LCID %d\n",lcid);

  if (lcid == 0)
  {
    UNLOCK_LOCALE;
    return NULL;
  }

  MSVCRT_current_lc_all_lcid = lcid;

  _snprintf(MSVCRT_current_lc_all,MAX_LOCALE_LENGTH,"%s_%s.%s",
	   lc.found_language,lc.found_country,lc.found_codepage);

  switch (category) {
  case MSVCRT_LC_ALL:
    lc_all = 1; /* Fall through all cases ... */
  case MSVCRT_LC_COLLATE:
    if (!lc_all) break;
  case MSVCRT_LC_CTYPE:
    msvcrt_set_ctype(atoi(lc.found_codepage),lcid);
    if (!lc_all) break;
  case MSVCRT_LC_MONETARY:
    if (!lc_all) break;
  case MSVCRT_LC_NUMERIC:
    if (!lc_all) break;
  case MSVCRT_LC_TIME:
    break;
  }
  UNLOCK_LOCALE;
  return MSVCRT_current_lc_all;
}

/*
 * @unimplemented
 */
wchar_t* _wsetlocale(int category, const wchar_t* locale)
{
  static wchar_t fake[] = {
    'E','n','g','l','i','s','h','_','U','n','i','t','e','d',' ',
    'S','t','a','t','e','s','.','1','2','5','2',0 };

  TRACE("%d %S\n", category, locale);

  return fake;
}

/*

locale  "lang[_country[.code_page]]"
            | ".code_page"
            | ""
            | NULL

*/
int parse_locale(const char *locale, char *lang, char *country, char *code_page)
{
	while ( *locale != 0 && *locale != '.' && *locale != '_' )
	{
		*lang = *locale;
		lang++;
		locale++;
	}
	*lang = 0;
	if ( *locale == '_' ) {
		locale++;
		while ( *locale != 0 && *locale != '.' )
		{
			*country = *locale;
			country++;
			locale++;
		}
	}
	*country = 0;


	if ( *locale == '.' ) {
		locale++;
		while ( *locale != 0 && *locale != '.' )
		{
			*code_page = *locale;
			code_page++;
			locale++;
		}
	}

	*code_page = 0;
	return 0;
}

const struct map_lcid2str {
        short            langid;
        const char      *langname;
	const char	*country;
} languages[]={
        {0x0409,"English", "United States"},
        {0x0809,"English", "United Kingdom"},
        {0x0000,"Unknown", "Unknown"}

};

const struct map_cntr {
	const char *abrev;
	const char  *country;
} abrev[] = {
	{"britain", "united kingdom"},
	{"england", "united kingdom"},
	{"gbr", "united kingdom"},
	{"great britain", "united kingdom"},
	{"uk", "united kingdom"},
	{"united kingdom", "united kingdom"},
	{"united-kingdom", "united kingdom"},
	{"america", "united states" },
	{"united states", "united states"},
	{"united-states", "united states"},
	{"us", "united states"},
	{"usa", "united states"}
};


struct lconv _lconv = {
".",   // decimal_point
",",   // thousands_sep
"",    // grouping;
"DOL", // int_curr_symbol
"$",   // currency_symbol
".",   // mon_decimal_point
",",   // mon_thousands_sep
"",    // mon_grouping;
"+",   // positive_sign
"-",   // negative_sign
127,     // int_frac_digits
127,     // frac_digits
127,     // p_cs_precedes
127,     // p_sep_by_space
127,     // n_cs_precedes
127,     // n_sep_by_space
127,     // p_sign_posn;
127      // n_sign_posn;
};

/*
 * @implemented
 */
struct lconv *localeconv(void)
{
  return (struct lconv *) &_lconv;
}

/*********************************************************************
 *		_setmbcp (MSVCRT.@)
 * @implemented
 */
int CDECL _setmbcp(int cp)
{
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

  TRACE("_setmbcp %d\n",cp);
  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = MSVCRT___lc_codepage;
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
    ERR("Codepage %d not found\n", newcp);
    __set_errno(EINVAL);
    return -1;
  }

  /* setup the _mbctype */
  memset(MSVCRT_mbctype, 0, sizeof(MSVCRT_mbctype));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      MSVCRT_mbctype[i + 1] |= _M1;
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
          ERR("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);

        bytes = cpextra->TrailBytes;
        while (bytes[0] || bytes[1])
        {
          for (i = bytes[0]; i <= bytes[1]; i++)
            MSVCRT_mbctype[i + 1] |= _M2;
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
    if (!(MSVCRT_mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

  curr_type = chartypes;
  for (i = 0; i < 256; i++)
    if (!(MSVCRT_mbctype[i + 1] & _M1))
    {
	if ((*curr_type) & C1_UPPER)
	    MSVCRT_mbctype[i + 1] |= _SBUP;
	if ((*curr_type) & C1_LOWER)
	    MSVCRT_mbctype[i + 1] |= _SBLOW;
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
      MSVCRT_mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      MSVCRT_mbctype[i + 1] |= _MS;
  }

  MSVCRT___lc_collate_cp = MSVCRT___lc_codepage = newcp;
  TRACE("(%d) -> %d\n", cp, MSVCRT___lc_codepage);
  return 0;
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
 *		__lc_handle (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_handle(void)
{
FIXME("__lc_handle - stub\n");
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
 *    _Gettnames (MSVCRT.@)
 */
void *_Gettnames(void)
{
  FIXME("(void), stub!\n");
  return NULL;
}

/*********************************************************************
 *    __lconv_init (MSVCRT.@)
 */
void __lconv_init(void)
{
  char Char = (char) UCHAR_MAX;

  TRACE("__lconv_init()\n");

  _lconv.int_frac_digits = Char;
  _lconv.frac_digits = Char;
  _lconv.p_sep_by_space = _lconv.n_sep_by_space = Char;
  _lconv.p_cs_precedes = _lconv.n_cs_precedes = Char;
  _lconv.p_sign_posn = _lconv.n_sign_posn = Char;
}


/*********************************************************************
 *    _Strftime (MSVCRT.@)
 */
const char* _Strftime(char *out, unsigned int len, const char *fmt,
                                     const void *tm, void *foo)
{
  /* FIXME: */
  FIXME("(%p %d %s %p %p) stub\n", out, len, fmt, tm, foo);
  return "";
}


/*********************************************************************
 *    _Getdays (MSVCRT.@)
 */
const char* _Getdays(void)
{
  static const char *MSVCRT_days = ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:"
                            "Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
  /* FIXME: Use locale */
  FIXME("(void) semi-stub\n");
  return MSVCRT_days;
}

/*********************************************************************
 *    _Getmonths (MSVCRT.@)
 */
const char* _Getmonths(void)
{
  static const char *MSVCRT_months = ":Jan:January:Feb:February:Mar:March:Apr:"
                "April:May:May:Jun:June:Jul:July:Aug:August:Sep:September:Oct:"
                "October:Nov:November:Dec:December";
  /* FIXME: Use locale */
  FIXME("(void) semi-stub\n");
  return MSVCRT_months;
}

/*********************************************************************
 *    __crtLCMapStringA (MSVCRT.@)
 */
int __crtLCMapStringA(
  LCID lcid, DWORD mapflags, const char* src, int srclen, char* dst,
  int dstlen, unsigned int codepage, int xflag
) {
  TRACE("(lcid %lx, flags %lx, %s(%d), %p(%d), %x, %d), partial stub!\n",
        lcid,mapflags,src,srclen,dst,dstlen,codepage,xflag);
  /* FIXME: A bit incorrect. But msvcrt itself just converts its
   * arguments to wide strings and then calls LCMapStringW
   */
  return LCMapStringA(lcid,mapflags,src,srclen,dst,dstlen);
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

int CDECL _getmbcp(void)
{
    return MSVCRT___lc_codepage;
}

/*********************************************************************
 *		___unguarded_readlc_active_add_func (MSVCRT.@)
 */
unsigned int * CDECL ___unguarded_readlc_active_add_func(void)
{
  return &__unguarded_readlc_active;
}

/*********************************************************************
 *		___setlc_active_func (MSVCRT.@)
 */
unsigned int CDECL ___setlc_active_func(void)
{
  return __setlc_active;
}
