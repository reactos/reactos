/*
FUNCTION
<<setlocale>>, <<localeconv>>---select or query locale

INDEX
	setlocale
INDEX
	localeconv
INDEX
	_setlocale_r
INDEX
	_localeconv_r

ANSI_SYNOPSIS
	#include <crtdll/locale.h>
	char *setlocale(int <[category]>, const char *<[locale]>);
	lconv *localeconv(void);

	char *_setlocale_r(void *<[reent]>,
                        int <[category]>, const char *<[locale]>);
	lconv *_localeconv_r(void *<[reent]>);

TRAD_SYNOPSIS
	#include <crtdll/locale.h>
	char *setlocale(<[category]>, <[locale]>)
	int <[category]>;
	char *<[locale]>;

	lconv *localeconv();

	char *_setlocale_r(<[reent]>, <[category]>, <[locale]>)
	char *<[reent]>;
	int <[category]>;
	char *<[locale]>;

	lconv *_localeconv_r(<[reent]>);
	char *<[reent]>;

DESCRIPTION
<<setlocale>> is the facility defined by ANSI C to condition the
execution environment for international collating and formatting
information; <<localeconv>> reports on the settings of the current
locale.

This is a minimal implementation, supporting only the required <<``C''>>
value for <[locale]>; strings representing other locales are not
honored unless MB_CAPABLE is defined in which case three new
extensions are allowed for LC_CTYPE only: <<''C-JIS''>>, <<''C-EUCJP''>>,
and <<''C-SJIS''>>.  (<<``''>> is also accepted; it represents the default locale
for an implementation, here equivalent to <<``C''>>.)

If you use <<NULL>> as the <[locale]> argument, <<setlocale>> returns
a pointer to the string representing the current locale (always
<<``C''>> in this implementation).  The acceptable values for
<[category]> are defined in `<<locale.h>>' as macros beginning with
<<"LC_">>, but this implementation does not check the values you pass
in the <[category]> argument.

<<localeconv>> returns a pointer to a structure (also defined in
`<<locale.h>>') describing the locale-specific conventions currently
in effect.  

<<_localeconv_r>> and <<_setlocale_r>> are reentrant versions of
<<localeconv>> and <<setlocale>> respectively.  The extra argument
<[reent]> is a pointer to a reentrancy structure.

RETURNS
<<setlocale>> returns either a pointer to a string naming the locale
currently in effect (always <<``C''>> for this implementation, or, if
the locale request cannot be honored, <<NULL>>.

<<localeconv>> returns a pointer to a structure of type <<lconv>>,
which describes the formatting and collating conventions in effect (in
this implementation, always those of the C locale).

PORTABILITY
ANSI C requires <<setlocale>>, but the only locale required across all
implementations is the C locale.

No supporting OS subroutines are required.
*/

/*
 * setlocale, localeconv : internationalize your locale.
 *                         (Only "C" or null supported).
 */

#include <crtdll/locale.h>
#include <crtdll/string.h>
#include <limits.h>


int _current_category;	/* used by setlocale */
const char *_current_locale;
int __mb_cur_max_dll = 1;

static const struct lconv lconv = 
{
  ".", "", "", "", "", "", "", "", "", "",
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
};


char * setlocale(int category ,const char *locale)
{
  static char lc_ctype[8] = "C";
  static char last_lc_ctype[8] = "C";

#ifndef MB_CAPABLE
  if (locale)
    { 
      if (strcmp (locale, "C") && strcmp (locale, ""))
        return 0;
      _current_category = category;  
      _current_locale = locale;
    }
  return "C";
#else
  if (locale)
    {
      if (category != LC_CTYPE) 
        { 
          if (strcmp (locale, "C") && strcmp (locale, ""))
            return 0;
          if (category == LC_ALL)
            {
              strcpy (last_lc_ctype, lc_ctype);
              strcpy (lc_ctype, locale);
              __mb_cur_max_dll = 1;
            }
        }
      else
        { 
          if (strcmp (locale, "C") && strcmp (locale, "") &&
              strcmp (locale, "C") && strcmp (locale, "C-JIS") && 
              strcmp (locale, "C-EUCJP") && strcmp (locale, "C-SJIS"))
            return 0;

          strcpy (last_lc_ctype, lc_ctype);
          strcpy (lc_ctype, locale);

          if (!strcmp (locale, "C-JIS"))
            __mb_cur_max_dll = 8;
          else if (strlen (locale) > 1)
            __mb_cur_max_dll = 2;
          else
            __mb_cur_max_dll = 1; 
        }
      _current_category = category;  
      _current_locale = locale;

      if (category == LC_CTYPE)
        return last_lc_ctype;
    }
  else
    {
      if (category == LC_CTYPE)
        return lc_ctype;
    }
 
  return "C";
#endif
  
}


struct lconv *localeconv(void)
{
  return (struct lconv *) &lconv;
}

