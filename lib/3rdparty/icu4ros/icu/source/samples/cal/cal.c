/*
**********************************************************************
* Copyright (C) 1998-2004, International Business Machines Corporation
* and others.  All Rights Reserved.
**********************************************************************
*
* File date.c
*
* Modification History:
*
*   Date        Name        Description
*   06/16/99    stephen     Creation.
*******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unicode/uloc.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/ustring.h"
#include "unicode/uclean.h"

#include "uprint.h"

#if UCONFIG_NO_FORMATTING

int main(int argc, char **argv)
{
  printf("%s: Sorry, UCONFIG_NO_FORMATTING was turned on (see uconfig.h). No formatting can be done. \n", argv[0]);
  return 0;
}
#else


/* Protos */
static void usage(void);

static void version(void);

static void cal(int32_t month, int32_t year,
                UBool useLongNames, UErrorCode *status);

static void get_symbols(const UDateFormat *fmt,
                        UDateFormatSymbolType type,
                        UChar *array[],
                        int32_t arrayLength,
                        int32_t lowestIndex,
                        int32_t firstIndex,
                        UErrorCode *status);

static void free_symbols(UChar *array[],
                         int32_t arrayLength);

static void get_days(const UDateFormat *fmt,
                     UChar *days [], UBool useLongNames, 
                     int32_t fdow, UErrorCode *status);

static void free_days(UChar *days[]);

static void get_months(const UDateFormat *fmt,
                       UChar *months [], UBool useLongNames,
                       UErrorCode *status);

static void free_months(UChar *months[]);

static void indent(int32_t count, FILE *f);

static void print_days(UChar *days [], FILE *f, UErrorCode *status);

static void  print_month(UCalendar *c, 
                         UChar *days [], 
                         UBool useLongNames, int32_t fdow, 
                         UErrorCode *status);

static void  print_year(UCalendar *c, 
                        UChar *days [], UChar *months [],
                        UBool useLongNames, int32_t fdow, 
                        UErrorCode *status);

/* The version of cal */
#define CAL_VERSION "1.0"

/* Number of days in a week */
#define DAY_COUNT 7

/* Number of months in a year (yes, 13) */
#define MONTH_COUNT 13

/* Separation between months in year view */
#define MARGIN_WIDTH 4

/* Size of stack buffers */
#define BUF_SIZE 64

/* Patterm string - "MMM yyyy" */
static const UChar sShortPat [] = { 0x004D, 0x004D, 0x004D, 0x0020, 
0x0079, 0x0079, 0x0079, 0x0079 };
/* Pattern string - "MMMM yyyy" */
static const UChar sLongPat [] = { 0x004D, 0x004D, 0x004D, 0x004D, 0x0020, 
0x0079, 0x0079, 0x0079, 0x0079 };


int
main(int argc,
     char **argv)
{
    int printUsage = 0;
    int printVersion = 0;
    UBool useLongNames = 0;
    int optind = 1;
    char *arg;
    int32_t month = -1, year = -1;
    UErrorCode status = U_ZERO_ERROR;
    
    
    /* parse the options */
    for(optind = 1; optind < argc; ++optind) {
        arg = argv[optind];
        
        /* version info */
        if(strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            printVersion = 1;
        }
        /* usage info */
        else if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            printUsage = 1;
        }
        /* use long day names */
        else if(strcmp(arg, "-l") == 0 || strcmp(arg, "--long") == 0) {
            useLongNames = 1;
        }
        /* POSIX.1 says all arguments after -- are not options */
        else if(strcmp(arg, "--") == 0) {
            /* skip the -- */
            ++optind;
            break;
        }
        /* unrecognized option */
        else if(strncmp(arg, "-", strlen("-")) == 0) {
            printf("cal: invalid option -- %s\n", arg+1);
            printUsage = 1;
        }
        /* done with options, display cal */
        else {
            break;
        }
    }
    
    /* Get the month and year to display, if specified */
    if(optind != argc) {
        
        /* Month and year specified */
        if(argc - optind == 2) {
            sscanf(argv[optind], "%d", (int*)&month);
            sscanf(argv[optind + 1], "%d", (int*)&year);
            
            /* Make sure the month value is legal */
            if(month < 0 || month > 12) {
                printf("icucal: Bad value for month -- %d\n", (int)month);
                
                /* Display usage */
                printUsage = 1;
            }
            
            /* Adjust because months are 0-based */
            --month;
        }
        /* Only year specified */
        else {
            sscanf(argv[optind], "%d", (int*)&year);
        }
    }
    
    /* print usage info */
    if(printUsage) {
        usage();
        return 0;
    }
    
    /* print version info */
    if(printVersion) {
        version();
        return 0;
    }
    
    /* print the cal */
    cal(month, year, useLongNames, &status);
    
    /* ICU cleanup.  Deallocate any memory ICU may be holding.  */
    u_cleanup();

    return (U_FAILURE(status) ? 1 : 0);
}

/* Usage information */
static void
usage()
{  
    puts("Usage: icucal [OPTIONS] [[MONTH] YEAR]");
    puts("");
    puts("Options:");
    puts("  -h, --help        Print this message and exit.");
    puts("  -v, --version     Print the version number of cal and exit.");
    puts("  -l, --long        Use long names.");
    puts("");
    puts("Arguments (optional):");
    puts("  MONTH             An integer (1-12) indicating the month to display");
    puts("  YEAR              An integer indicating the year to display");
    puts("");
    puts("For an interesting calendar, look at October 1582");
}

/* Version information */
static void
version()
{
    printf("icucal version %s (ICU version %s), created by Stephen F. Booth.\n", 
        CAL_VERSION, U_ICU_VERSION); 
    puts(U_COPYRIGHT_STRING);
}

static void
cal(int32_t month,
    int32_t year,
    UBool useLongNames,
    UErrorCode *status)
{
    UCalendar *c;
    UChar *days [DAY_COUNT];
    UChar *months [MONTH_COUNT];
    int32_t fdow;
    
    if(U_FAILURE(*status)) return;
    
    /* Create a new calendar */
    c = ucal_open(0, -1, uloc_getDefault(), UCAL_TRADITIONAL, status);
    
    /* Determine if we are printing a calendar for one month or for a year */
    
    /* Print an entire year */
    if(month == -1 && year != -1) {
        
        /* Set the year */
        ucal_set(c, UCAL_YEAR, year);
        
        /* Determine the first day of the week */
        fdow = ucal_getAttribute(c, UCAL_FIRST_DAY_OF_WEEK);
        
        /* Print the calendar for the year */
        print_year(c, days, months, useLongNames, fdow, status);
    }
    
    /* Print only one month */
    else {
        
        /* Set the month and the year, if specified */
        if(month != -1)
            ucal_set(c, UCAL_MONTH, month);
        if(year != -1)
            ucal_set(c, UCAL_YEAR, year);
        
        /* Determine the first day of the week */
        fdow = ucal_getAttribute(c, UCAL_FIRST_DAY_OF_WEEK);
        
        /* Print the calendar for the month */
        print_month(c, days, useLongNames, fdow, status);
    }
    
    /* Clean up */
    ucal_close(c);
}
/*
 * Get a set of DateFormat symbols of a given type.
 *
 * lowestIndex is the index of the first symbol to fetch.
 * (e.g. it will be one to fetch day names, since Sunday is
 *  day 1 *not* day 0.)
 *
 * firstIndex is the index of the symbol to place first in
 * the output array. This is used when fetching day names
 * in locales where the week doesn't start on Sunday.
 */
static void get_symbols(const UDateFormat *fmt,
                        UDateFormatSymbolType type,
                        UChar *array[],
                        int32_t arrayLength,
                        int32_t lowestIndex,
                        int32_t firstIndex,
                        UErrorCode *status)
{
    int32_t count, i;
    
    if (U_FAILURE(*status)) {
        return;
    }

    count = udat_countSymbols(fmt, type);

    if(count != arrayLength + lowestIndex) {
        return;
    }

    for(i = 0; i < arrayLength; i++) {
        int32_t index = (i + firstIndex) % arrayLength;
        int32_t size = 1 + udat_getSymbols(fmt, type, index + lowestIndex, NULL, 0, status);
        
        array[index] = (UChar *) malloc(sizeof(UChar) * size);

        *status = U_ZERO_ERROR;
        udat_getSymbols(fmt, type, index + lowestIndex, array[index], size, status);
    }
}

/* Free the symbols allocated by get_symbols(). */
static void free_symbols(UChar *array[],
                         int32_t arrayLength)
{
    int32_t i;

    for(i = 0; i < arrayLength; i++) {
        free(array[i]);
    }
}

/* Get the day names for the specified locale, in either long or short
form.  Also, reorder the days so that they are in the proper order
for the locale (not all locales begin weeks on Sunday; in France,
weeks start on Monday) */
static void
get_days(const UDateFormat *fmt,
         UChar *days [],
         UBool useLongNames,
         int32_t fdow,
         UErrorCode *status)
{
    UDateFormatSymbolType dayType = (useLongNames ? UDAT_WEEKDAYS : UDAT_SHORT_WEEKDAYS);
    
    if(U_FAILURE(*status))
        return;
    
    /* fdow is 1-based */
    --fdow;

    get_symbols(fmt, dayType, days, DAY_COUNT, 1, fdow, status);
}

static void free_days(UChar *days[])
{
    free_symbols(days, DAY_COUNT);
}

/* Get the month names for the specified locale, in either long or
short form. */
static void
get_months(const UDateFormat *fmt,
           UChar *months [],
           UBool useLongNames,
           UErrorCode *status)
{
    UDateFormatSymbolType monthType = (useLongNames ? UDAT_MONTHS : UDAT_SHORT_MONTHS);
    
    if(U_FAILURE(*status))
        return;
    
    get_symbols(fmt, monthType, months, MONTH_COUNT - 1, 0, 0, status); /* some locales have 13 months, no idea why */
}

static void free_months(UChar *months[])
{
    free_symbols(months, MONTH_COUNT - 1);
}

/* Indent a certain number of spaces */
static void
indent(int32_t count,
       FILE *f)
{
    char c [BUF_SIZE];

    if(count <= 0)
    {
        return;
    }
    
    if(count < BUF_SIZE) {
        memset(c, (int)' ', count);
        fwrite(c, sizeof(char), count, f);
    }
    else {
        int32_t i;
        for(i = 0; i < count; ++i)
            putc(' ', f);
    }
}

/* Print the days */
static void
print_days(UChar *days [],
           FILE *f,
           UErrorCode *status)
{
    int32_t i;
    
    if(U_FAILURE(*status)) return;
    
    /* Print the day names */
    for(i = 0; i < DAY_COUNT; ++i) {
        uprint(days[i], f, status);
        putc(' ', f);
    }
}

/* Print out a calendar for c's current month */
static void
print_month(UCalendar *c, 
            UChar *days [], 
            UBool useLongNames,
            int32_t fdow,
            UErrorCode *status)
{
    int32_t width, pad, i, day;
    int32_t lens [DAY_COUNT];
    int32_t firstday, current;
    UNumberFormat *nfmt;
    UDateFormat *dfmt;
    UChar s [BUF_SIZE];
    const UChar *pat = (useLongNames ? sLongPat : sShortPat);
    int32_t len = (useLongNames ? 9 : 8);
    
    if(U_FAILURE(*status)) return;
    
    
    /* ========== Generate the header containing the month and year */
    
    /* Open a formatter with a month and year only pattern */
    dfmt = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL,NULL,0,pat, len,status);
    
    /* Format the date */
    udat_format(dfmt, ucal_getMillis(c, status), s, BUF_SIZE, 0, status);
    
    
    /* ========== Get the day names */
    get_days(dfmt, days, useLongNames, fdow, status);

    /* ========== Print the header */
    
    /* Calculate widths for justification */
    width = 6; /* 6 spaces, 1 between each day name */
    for(i = 0; i < DAY_COUNT; ++i) {
        lens[i] = u_strlen(days[i]);
        width += lens[i];
    }
    
    /* Print the header, centered among the day names */
    pad = width - u_strlen(s);
    indent(pad / 2, stdout);
    uprint(s, stdout, status);
    putc('\n', stdout);
    
    
    /* ========== Print the day names */
    
    print_days(days, stdout, status);
    putc('\n', stdout);
    
    
    /* ========== Print the calendar */
    
    /* Get the first of the month */
    ucal_set(c, UCAL_DATE, 1);
    firstday = ucal_get(c, UCAL_DAY_OF_WEEK, status);
    
    /* The day of the week for the first day of the month is based on
    1-based days of the week, which were also reordered when placed
    in the days array.  Account for this here by offsetting by the
    first day of the week for the locale, which is also 1-based. */
    firstday -= fdow;
    
    /* Open the formatter */
    nfmt = unum_open(UNUM_DECIMAL, NULL,0,NULL,NULL, status);
    
    /* Indent the correct number of spaces for the first week */
    current = firstday;
    if(current < 0)
    {
       current += 7;
    }
    for(i = 0; i < current; ++i)
        indent(lens[i] + 1, stdout);
    
    /* Finally, print out the days */
    day = ucal_get(c, UCAL_DATE, status);
    do {
        
        /* Format the current day string */
        unum_format(nfmt, day, s, BUF_SIZE, 0, status);
        
        /* Calculate the justification and indent */
        pad = lens[current] - u_strlen(s);
        indent(pad, stdout);
        
        /* Print the day number out, followed by a space */
        uprint(s, stdout, status);
        putc(' ', stdout);
        
        /* Update the current day */
        ++current;
        current %= DAY_COUNT;
        
        /* If we're at day 0 (first day of the week), insert a newline */
        if(current == 0) {
            putc('\n', stdout);
        }
        
        /* Go to the next day */
        ucal_add(c, UCAL_DATE, 1, status);
        day = ucal_get(c, UCAL_DATE, status);
        
    } while(day != 1);
    
    /* Output a trailing newline */
    putc('\n', stdout);
    
    /* Clean up */
    free_days(days);
    unum_close(nfmt);
    udat_close(dfmt);
}

/* Print out a calendar for c's current year */
static void
print_year(UCalendar *c, 
           UChar *days [], 
           UChar *months [],
           UBool useLongNames, 
           int32_t fdow, 
           UErrorCode *status)
{
    int32_t width, pad, i, j;
    int32_t lens [DAY_COUNT];
    UNumberFormat *nfmt;
    UDateFormat *dfmt;
    UChar s [BUF_SIZE];
    const UChar pat [] = { 0x0079, 0x0079, 0x0079, 0x0079 };
    int32_t len = 4;
    UCalendar  *left_cal, *right_cal;
    int32_t left_day, right_day;
    int32_t left_firstday, right_firstday, left_current, right_current;
    int32_t left_month, right_month;
    
    if(U_FAILURE(*status)) return;
    
    /* Alias */
    left_cal = c;
    
    /* ========== Generate the header containing the year (only) */
    
    /* Open a formatter with a month and year only pattern */
    dfmt = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL,NULL,0,pat, len, status);
    
    /* Format the date */
    udat_format(dfmt, ucal_getMillis(left_cal, status), s, BUF_SIZE, 0, status);
    
    /* ========== Get the month and day names */
    get_days(dfmt, days, useLongNames, fdow, status);
    get_months(dfmt, months, useLongNames, status);

    /* ========== Print the header, centered */
    
    /* Calculate widths for justification */
    width = 6; /* 6 spaces, 1 between each day name */
    for(i = 0; i < DAY_COUNT; ++i) {
        lens[i] = u_strlen(days[i]);
        width += lens[i];
    }
    
    /* width is the width for 1 calendar; we are displaying in 2 cols
    with MARGIN_WIDTH spaces between months */
    
    /* Print the header, centered among the day names */
    pad = 2 * width + MARGIN_WIDTH - u_strlen(s);
    indent(pad / 2, stdout);
    uprint(s, stdout, status);
    putc('\n', stdout);
    putc('\n', stdout);
    
    /* Generate a copy of the calendar to use */
    right_cal = ucal_open(0, -1, uloc_getDefault(), UCAL_TRADITIONAL, status);
    ucal_setMillis(right_cal, ucal_getMillis(left_cal, status), status);
    
    /* Open the formatter */
    nfmt = unum_open(UNUM_DECIMAL,NULL, 0,NULL,NULL, status);
    
    /* ========== Calculate and display the months, two at a time */
    for(i = 0; i < MONTH_COUNT - 1; i += 2) {
        
        /* Print the month names for the two current months */
        pad = width - u_strlen(months[i]);
        indent(pad / 2, stdout);
        uprint(months[i], stdout, status);
        indent(pad / 2 + MARGIN_WIDTH, stdout);
        pad = width - u_strlen(months[i + 1]);
        indent(pad / 2, stdout);
        uprint(months[i + 1], stdout, status);
        putc('\n', stdout);
        
        /* Print the day names, twice  */
        print_days(days, stdout, status);
        indent(MARGIN_WIDTH, stdout);
        print_days(days, stdout, status);
        putc('\n', stdout);
        
        /* Setup the two calendars */
        ucal_set(left_cal, UCAL_MONTH, i);
        ucal_set(left_cal, UCAL_DATE, 1);
        ucal_set(right_cal, UCAL_MONTH, i + 1);
        ucal_set(right_cal, UCAL_DATE, 1);
        
        left_firstday = ucal_get(left_cal, UCAL_DAY_OF_WEEK, status);
        right_firstday = ucal_get(right_cal, UCAL_DAY_OF_WEEK, status);
        
        /* The day of the week for the first day of the month is based on
        1-based days of the week.  However, the days were reordered
        when placed in the days array.  Account for this here by
        offsetting by the first day of the week for the locale, which
        is also 1-based. */
        
        /* We need to mod by DAY_COUNT since fdow can be > firstday.  IE,
        if fdow = 2 = Monday (like in France) and the first day of the
        month is a 1 = Sunday, we want firstday to be 6, not -1 */
        left_firstday += (DAY_COUNT - fdow);
        left_firstday %= DAY_COUNT;
        
        right_firstday += (DAY_COUNT - fdow);
        right_firstday %= DAY_COUNT;
        
        left_current = left_firstday;
        right_current = right_firstday;
        
        left_day = ucal_get(left_cal, UCAL_DATE, status);
        right_day = ucal_get(right_cal, UCAL_DATE, status);
        
        left_month = ucal_get(left_cal, UCAL_MONTH, status);
        right_month = ucal_get(right_cal, UCAL_MONTH, status);
        
        /* Finally, print out the days */
        while(left_month == i || right_month == i + 1) {
            
        /* If the left month is finished printing, but the right month
        still has days to be printed, indent the width of the days
            strings and reset the left calendar's current day to 0 */
            if(left_month != i && right_month == i + 1) {
                indent(width + 1, stdout);
                left_current = 0;
            }
            
            while(left_month == i) {
                
            /* If the day is the first, indent the correct number of
                spaces for the first week */
                if(left_day == 1) {
                    for(j = 0; j < left_current; ++j)
                        indent(lens[j] + 1, stdout);
                }
                
                /* Format the current day string */
                unum_format(nfmt, left_day, s, BUF_SIZE, 0, status);
                
                /* Calculate the justification and indent */
                pad = lens[left_current] - u_strlen(s);
                indent(pad, stdout);
                
                /* Print the day number out, followed by a space */
                uprint(s, stdout, status);
                putc(' ', stdout);
                
                /* Update the current day */
                ++left_current;
                left_current %= DAY_COUNT;
                
                /* Go to the next day */
                ucal_add(left_cal, UCAL_DATE, 1, status);
                left_day = ucal_get(left_cal, UCAL_DATE, status);
                
                /* Determine the month */
                left_month = ucal_get(left_cal, UCAL_MONTH, status);
                
                /* If we're at day 0 (first day of the week), break and go to
                the next month */
                if(left_current == 0) {
                    break;
                }
            };
            
            /* If the current day isn't 0, indent to make up for missing
            days at the end of the month */
            if(left_current != 0) {
                for(j = left_current; j < DAY_COUNT; ++j)
                    indent(lens[j] + 1, stdout);
            }
            
            /* Indent between the two months */
            indent(MARGIN_WIDTH, stdout);
            
            while(right_month == i + 1) {
                
            /* If the day is the first, indent the correct number of
                spaces for the first week */
                if(right_day == 1) {
                    for(j = 0; j < right_current; ++j)
                        indent(lens[j] + 1, stdout);
                }
                
                /* Format the current day string */
                unum_format(nfmt, right_day, s, BUF_SIZE, 0, status);
                
                /* Calculate the justification and indent */
                pad = lens[right_current] - u_strlen(s);
                indent(pad, stdout);
                
                /* Print the day number out, followed by a space */
                uprint(s, stdout, status);
                putc(' ', stdout);
                
                /* Update the current day */
                ++right_current;
                right_current %= DAY_COUNT;
                
                /* Go to the next day */
                ucal_add(right_cal, UCAL_DATE, 1, status);
                right_day = ucal_get(right_cal, UCAL_DATE, status);
                
                /* Determine the month */
                right_month = ucal_get(right_cal, UCAL_MONTH, status);
                
                /* If we're at day 0 (first day of the week), break out */
                if(right_current == 0) {
                    break;
                }
                
            };
            
            /* Output a newline */
            putc('\n', stdout);
        }
        
        /* Output a trailing newline */
        putc('\n', stdout);
  }
  
  /* Clean up */
  free_months(months);
  free_days(days);
  udat_close(dfmt);
  unum_close(nfmt);
  ucal_close(right_cal);
}

#endif
