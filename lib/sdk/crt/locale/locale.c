/*
 * Some stuff takem from wine msvcrt\locale.c
 *
 * Copyright 2000 Jon Griffiths
 */

#include <precomp.h>
#include <locale.h>
#include <internal/tls.h>

#define NDEBUG
#include <internal/debug.h>

unsigned int __setlc_active;
unsigned int __unguarded_readlc_active;
int _current_category;	/* used by setlocale */
const char *_current_locale;

int parse_locale(const char *locale, char *lang, char *country, char *code_page);

/*
 * @unimplemented
 */
char *setlocale(int category, const char *locale)
{
	char lang[100];
	char country[100];
	char code_page[100];
	if (NULL != locale) {
		parse_locale(locale,lang,country,code_page);
	}

	//printf("%s %s %s %s\n",locale,lang,country,code_page);


	switch ( category )
  	{
  		case LC_COLLATE:
  		break;
		case LC_CTYPE:
		break;
		case LC_MONETARY:
		break;
		case LC_NUMERIC:
		break;
		case LC_TIME:
		break;
		case LC_ALL:
		break;
		default:
		break;
	}

	return "C";

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
	{"usa" "united states"}
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
 *
 * @unimplemented
 */
void _setmbcp(int cp)
{
DPRINT1("_setmbcp - stub\n");
return;
}


/*********************************************************************
 *		__lc_collate_cp (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_collate_cp(int cp)
{
DPRINT1("__lc_collate_cp - stub\n");
return;
}


/*********************************************************************
 *		__lc_handle (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_handle(void)
{
DPRINT1("__lc_handle - stub\n");
return;
}


/*********************************************************************
 *		__lc_codepage (MSVCRT.@)
 *
 * @unimplemented
 */
void __lc_codepage(void)
{
DPRINT1("__lc_codepage - stub\n");
return;
}


/*********************************************************************
 *    _Gettnames (MSVCRT.@)
 */
void *_Gettnames(void)
{
  DPRINT1("(void), stub!\n");
  return NULL;
}

/*********************************************************************
 *    __lconv_init (MSVCRT.@)
 */
void __lconv_init(void)
{
  DPRINT1(" stub\n");
}


/*********************************************************************
 *    _Strftime (MSVCRT.@)
 */
const char* _Strftime(char *out, unsigned int len, const char *fmt,
                                     const void *tm, void *foo)
{
  /* FIXME: */
  DPRINT1("(%p %d %s %p %p) stub\n", out, len, fmt, tm, foo);
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
  DPRINT1("(void) semi-stub\n");
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
  DPRINT1("(void) semi-stub\n");
  return MSVCRT_months;
}

/*********************************************************************
 *    __crtLCMapStringA (MSVCRT.@)
 */
int __crtLCMapStringA(
  LCID lcid, DWORD mapflags, const char* src, int srclen, char* dst,
  int dstlen, unsigned int codepage, int xflag
) {
  DPRINT1("(lcid %lx, flags %lx, %s(%d), %p(%d), %x, %d), partial stub!\n",
        lcid,mapflags,src,srclen,dst,dstlen,codepage,xflag);
  /* FIXME: A bit incorrect. But msvcrt itself just converts its
   * arguments to wide strings and then calls LCMapStringW
   */
  return LCMapStringA(lcid,mapflags,src,srclen,dst,dstlen);
}
