/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    httptime.h

Abstract:

    This file contains the numerical defines for the date/parsing routines located
    in the httptime.cxx file.

Author:

    Arthur Bierer (arthurbi) 12-Dec-1997

Revision History:

--*/


#ifndef _HTTPTIME_H_
#define _HTTPTIME_H_

#define BASE_DEC 10 // base 10

//
// Date indicies used to figure out what each entry is.
//


#define DATE_INDEX_DAY_OF_WEEK     0

#define DATE_1123_INDEX_DAY        1
#define DATE_1123_INDEX_MONTH      2
#define DATE_1123_INDEX_YEAR       3
#define DATE_1123_INDEX_HRS        4
#define DATE_1123_INDEX_MINS       5
#define DATE_1123_INDEX_SECS       6

#define DATE_ANSI_INDEX_MONTH      1
#define DATE_ANSI_INDEX_DAY        2
#define DATE_ANSI_INDEX_HRS        3
#define DATE_ANSI_INDEX_MINS       4
#define DATE_ANSI_INDEX_SECS       5
#define DATE_ANSI_INDEX_YEAR       6

#define DATE_INDEX_TZ              7

#define DATE_INDEX_LAST            DATE_INDEX_TZ
#define MAX_DATE_ENTRIES           (DATE_INDEX_LAST+1)




//
// DATE_TOKEN's DWORD values used to determine what day/month we're on
//

#define DATE_TOKEN_JANUARY      1
#define DATE_TOKEN_FEBRUARY     2
#define DATE_TOKEN_MARCH        3
#define DATE_TOKEN_APRIL        4
#define DATE_TOKEN_MAY          5
#define DATE_TOKEN_JUNE         6
#define DATE_TOKEN_JULY         7
#define DATE_TOKEN_AUGUST       8
#define DATE_TOKEN_SEPTEMBER    9
#define DATE_TOKEN_OCTOBER      10
#define DATE_TOKEN_NOVEMBER     11
#define DATE_TOKEN_DECEMBER     12       

#define DATE_TOKEN_LAST_MONTH   (DATE_TOKEN_DECEMBER+1)

#define DATE_TOKEN_SUNDAY       0
#define DATE_TOKEN_MONDAY       1
#define DATE_TOKEN_TUESDAY      2                  
#define DATE_TOKEN_WEDNESDAY    3
#define DATE_TOKEN_THURSDAY     4
#define DATE_TOKEN_FRIDAY       5
#define DATE_TOKEN_SATURDAY     6

#define DATE_TOKEN_LAST_DAY     (DATE_TOKEN_SATURDAY+1)
 
#define DATE_TOKEN_GMT          0xFFFFFFFD

#define DATE_TOKEN_LAST         DATE_TOKEN_GMT

#define DATE_TOKEN_ERROR        (DATE_TOKEN_LAST+1)

                            
#endif  // _HTTPTIME_H_
