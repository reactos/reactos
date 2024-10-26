
#include <alias.inc>

    // These exports only exist in msvcrt.dll, not in ucrtbase.dll
    //  Do not remove indentation, this would break ARM build!
    CREATE_ALIAS2 daylight, _daylight
    CREATE_ALIAS snprintf, _snprintf
    CREATE_ALIAS strcmpi, _strcmpi
    CREATE_ALIAS2 timezone, _timezone
    CREATE_ALIAS2 tzname, _tzname

    // C99 functions (already properly exported in ucrtbase.dll))
    CREATE_ALIAS cabs, _cabs
    CREATE_ALIAS hypot, _hypot
    CREATE_ALIAS logb, _logb
    CREATE_ALIAS nextafter, _nextafter

    END
