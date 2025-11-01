#ifndef __country_w32_h__
#define __country_w32_h__

struct country {
    int co_date;         /* date format */
    char co_curr[ 5 ];   /* currency symbol */
    char co_thsep[ 2 ];  /* thousands separator */
    char co_desep[ 2 ];  /* decimal separator */
    char co_dtsep[ 2 ];  /* date separator */
    char co_tmsep[ 2 ];  /* time separator */
    char co_currstyle;   /* currency style */
    char co_digits;      /* significant digits in currency */
    char co_time;        /* time format */
    long co_case;        /* case map */
    char co_dasep[ 2 ];  /* data separator */
    char co_fill[ 10 ];  /* filler */
};
#define COUNTRY country

struct COUNTRY *country( int xcode, struct COUNTRY *ct );

#endif
