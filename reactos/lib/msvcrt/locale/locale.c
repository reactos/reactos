#include <msvcrt/stdio.h>
#include <msvcrt/locale.h>
#include <msvcrt/string.h>
#include <limits.h>


int _current_category;	/* used by setlocale */
const char *_current_locale;

int parse_locale(char *locale, char *lang, char *country, char *code_page);

char *setlocale(int category, const char *locale)
{
	char lang[100];
	char country[100];
	char code_page[100];
	parse_locale((char *)locale,lang,country,code_page);	

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
int parse_locale(char *locale, char *lang, char *country, char *code_page)
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

struct lconv *localeconv(void)
{
  return (struct lconv *) &_lconv;
}
