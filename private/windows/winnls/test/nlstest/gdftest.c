/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gdftest.c

Abstract:

    Test module for NLS API GetDateFormat.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    04-30-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE                50          // buffer size in wide chars
#define  GDF_INVALID_FLAGS      ((DWORD)(~(LOCALE_NOUSEROVERRIDE |        \
                                           DATE_SHORTDATE |               \
                                           DATE_LONGDATE |                \
                                           DATE_YEARMONTH |               \
                                           DATE_USE_ALT_CALENDAR)))

#define  ENGLISH_US             L"5/1/1993"
#define  L_ENGLISH_US           L"Saturday, May 01, 1993"
#define  CZECH                  L"1.5. 1993"

#define  YEAR                   L"1993"
#define  YEAR_2                 L"93"

#define  US_DAYOFWEEK           L"Saturday"
#define  US_MONTH               L"May"
#define  US_ABBREVDAY           L"Sat"
#define  US_ABBREVMONTH         L"May"

#define  SPANISH_DAYOFWEEK      L"sábado"
#define  SPANISH_MONTH          L"mayo"
#define  SPANISH_ABBREVDAY      L"sáb"
#define  SPANISH_ABBREVMONTH    L"may"

#define  L_RUSSIAN_2            L"\x041c\x0430\x0439"
#define  L_RUSSIAN_3            L"\x043c\x0430\x044f 1"
#define  L_RUSSIAN_4            L"1 93 \x041c\x0430\x0439"

#define  L_POLISH_2             L"maj"
#define  L_POLISH_3             L"maja 1"
#define  L_POLISH_4             L"1 93 maj"

#define  JAPAN_ALT              L"\x5e73\x6210 5/5/1"
#define  CHINA_ALT              L"82/5/1"
#define  KOREA_ALT              L"\xb2e8\xae30 4326-05-01"

#define  L_JAPAN_ALT            L"\x5e73\x6210 5\x5e74\x0035\x6708\x0031\x65e5"
#define  L_CHINA_ALT            L"\x4e2d\x83ef\x6c11\x570b\x0038\x0032\x5e74\x0035\x6708\x0031\x65e5"
#define  L_KOREA_ALT            L"\xb2e8\xae30 4326\xb144 5\xc6d4 1\xc77c \xd1a0\xc694\xc77c"

#define  L_JAPAN_ALT_2          L"\x5927\x6b63 2\x5e74\x0035\x6708\x0031\x65e5"
#define  L_CHINA_ALT_2          L"1832\x5e74\x0035\x6708\x0031\x65e5"
#define  L_KOREA_ALT_2          L"\xb2e8\xae30 4165\xb144 5\xc6d4 1\xc77c \xd654\xc694\xc77c"

#define  L_JAPAN_ALT_3          L" 32\x5e74\x0035\x6708\x0031\x65e5"




//
//  Global Variables.
//

LCID Locale;

SYSTEMTIME SysDate;

WCHAR lpDateStr[BUFSIZE];

WCHAR pSShortDate[BUFSIZE];


//
//  Date format buffers must be in line with the pAllLocales global
//  buffer.
//
LPWSTR pShortDate[] =
{
    L"01.5.1993 \x0433.",                  //  0x0402
    L"1993/5/1",                           //  0x0404
    L"1993-5-1",                           //  0x0804
    L"1/5/1993",                           //  0x0c04
    L"1/5/1993",                           //  0x1004
    L"1.5. 1993",                          //  0x0405
    L"01-05-1993",                         //  0x0406
    L"01.05.1993",                         //  0x0407
    L"01.05.1993",                         //  0x0807
    L"01.05.1993",                         //  0x0c07
    L"1/5/1993",                           //  0x0408
    L"5/1/1993",                           //  0x0409
    L"01/05/1993",                         //  0x0809
    L"1/05/1993",                          //  0x0c09
    L"01/05/1993",                         //  0x1009
    L"1/05/1993",                          //  0x1409
    L"01/05/1993",                         //  0x1809
    L"01/05/1993",                         //  0x040a
    L"01/05/1993",                         //  0x080a
    L"01/05/1993",                         //  0x0c0a
    L"1.5.1993",                           //  0x040b
    L"01/05/1993",                         //  0x040c
    L"1/05/1993",                          //  0x080c
    L"1993-05-01",                         //  0x0c0c
    L"01.05.1993",                         //  0x100c
    L"1993. 05. 01.",                      //  0x040e
    L"1.5.1993",                           //  0x040f
    L"01/05/1993",                         //  0x0410
    L"01.05.1993",                         //  0x0810
    L"1993/05/01",                         //  0x0411
    L"1993-05-01",                         //  0x0412
    L"1-5-1993",                           //  0x0413
    L"1/05/1993",                          //  0x0813
    L"01.05.1993",                         //  0x0414
    L"01.05.1993",                         //  0x0814
    L"1993-05-01",                         //  0x0415
    L"1/5/1993",                           //  0x0416
    L"01-05-1993",                         //  0x0816
    L"01.05.1993",                         //  0x0418
    L"01.05.1993",                         //  0x0419
    L"1.5.1993",                           //  0x041a
    L"1. 5. 1993",                         //  0x041b
    L"1993-05-01",                         //  0x041d
    L"01.05.1993",                         //  0x041f
    L"1.5.1993"                            //  0x0424
};

LPWSTR pLongDate[] =
{
    L"01 \x041c\x0430\x0439 1993 \x0433.",                                                    //  0x0402
    L"1993\x5e74\x0035\x6708\x0031\x65e5",                                                    //  0x0404
    L"1993\x5e74\x0035\x6708\x0031\x65e5",                                                    //  0x0804
    L"Saturday, 1 May, 1993",                                                                 //  0x0c04
    L"\x661f\x671f\x516d, 1 \x4e94\x6708, 1993",                                              //  0x1004
    L"1. kv\x011btna 1993",                                                                   //  0x0405
    L"1. maj 1993",                                                                           //  0x0406
    L"Samstag, 1. Mai 1993",                                                                  //  0x0407
    L"Samstag, 1. Mai 1993",                                                                  //  0x0807
    L"Samstag, 01. Mai 1993",                                                                 //  0x0c07
    L"\x03a3\x03ac\x03b2\x03b2\x03b1\x03c4\x03bf, 1 \x039c\x03b1\x0390\x03bf\x03c5 1993",     //  0x0408
    L"Saturday, May 01, 1993",                                                                //  0x0409
    L"01 May 1993",                                                                           //  0x0809
    L"Saturday, 1 May 1993",                                                                  //  0x0c09
    L"May 1, 1993",                                                                           //  0x1009
    L"Saturday, 1 May 1993",                                                                  //  0x1409
    L"01 May 1993",                                                                           //  0x1809
    L"sábado, 01 de mayo de 1993",                                                            //  0x040a
    L"Sábado, 01 de Mayo de 1993",                                                            //  0x080a
    L"sábado, 01 de mayo de 1993",                                                            //  0x0c0a
    L"1. toukokuuta 1993",                                                                    //  0x040b
    L"samedi 1 mai 1993",                                                                     //  0x040c
    L"samedi 1 mai 1993",                                                                     //  0x080c
    L"1 mai, 1993",                                                                           //  0x0c0c
    L"samedi, 1. mai 1993",                                                                   //  0x100c
    L"1993. május 1.",                                                                        //  0x040e
    L"1. maí 1993",                                                                           //  0x040f
    L"sabato 1 maggio 1993",                                                                  //  0x0410
    L"sabato, 1. maggio 1993",                                                                //  0x0810
    L"1993\x5e74\x0035\x6708\x0031\x65e5",                                                    //  0x0411
    L"1993\xb144 5\xc6d4 1\xc77c \xd1a0\xc694\xc77c",                                         //  0x0412
    L"zaterdag 1 mei 1993",                                                                   //  0x0413
    L"zaterdag 1 mei 1993",                                                                   //  0x0813
    L"1. mai 1993",                                                                           //  0x0414
    L"1. mai 1993",                                                                           //  0x0814
    L"1 maja 1993",                                                                           //  0x0415
    L"sábado, 1 de maio de 1993",                                                             //  0x0416
    L"sábado, 1 de Maio de 1993",                                                             //  0x0816
    L"1 mai 1993",                                                                            //  0x0418
    L"1 \x043c\x0430\x044f 1993 \x0433.",                                                     //  0x0419
    L"1. svibanj 1993",                                                                       //  0x041a
    L"1. m\x00e1ja 1993",                                                                     //  0x041b
    L"den 1 maj 1993",                                                                        //  0x041d
    L"01 May\x0131s 1993 Cumartesi",                                                          //  0x041f
    L"1. maj 1993"                                                                            //  0x0424
};

LPWSTR pYearMonth[] =
{
    L"\x041c\x0430\x0439 1993 \x0433.",                                                       //  0x0402
    L"1993\x5e74\x0035\x6708",                                                                //  0x0404
    L"1993\x5e74\x0035\x6708",                                                                //  0x0804
    L"May, 1993",                                                                             //  0x0c04
    L"\x4e94\x6708, 1993",                                                                    //  0x1004
    L"kv\x011bten 1993",                                                                      //  0x0405
    L"maj 1993",                                                                              //  0x0406
    L"Mai 1993",                                                                              //  0x0407
    L"Mai 1993",                                                                              //  0x0807
    L"Mai 1993",                                                                              //  0x0c07
    L"\x039c\x03ac\x03b9\x03bf\x03c2 1993",                                                   //  0x0408
    L"May, 1993",                                                                             //  0x0409
    L"May 1993",                                                                              //  0x0809
    L"May 1993",                                                                              //  0x0c09
    L"May, 1993",                                                                             //  0x1009
    L"May 1993",                                                                              //  0x1409
    L"May 1993",                                                                              //  0x1809
    L"mayo de 1993",                                                                          //  0x040a
    L"Mayo de 1993",                                                                          //  0x080a
    L"mayo de 1993",                                                                          //  0x0c0a
    L"toukokuu 1993",                                                                       //  0x040b
    L"mai 1993",                                                                              //  0x040c
    L"mai 1993",                                                                              //  0x080c
    L"mai, 1993",                                                                             //  0x0c0c
    L"mai 1993",                                                                              //  0x100c
    L"1993. május",                                                                           //  0x040e
    L"maí 1993",                                                                              //  0x040f
    L"maggio 1993",                                                                           //  0x0410
    L"maggio 1993",                                                                           //  0x0810
    L"1993\x5e74\x0035\x6708",                                                                //  0x0411
    L"1993\xb144 5\xc6d4",                                                                    //  0x0412
    L"mei 1993",                                                                              //  0x0413
    L"mei 1993",                                                                              //  0x0813
    L"mai 1993",                                                                              //  0x0414
    L"mai 1993",                                                                              //  0x0814
    L"maj 1993",                                                                              //  0x0415
    L"maio de 1993",                                                                          //  0x0416
    L"Maio de 1993",                                                                          //  0x0816
    L"mai 1993",                                                                              //  0x0418
    L"\x041c\x0430\x0439 1993 \x0433.",                                                       //  0x0419
    L"svibanj, 1993",                                                                         //  0x041a
    L"máj 1993",                                                                              //  0x041b
    L"maj 1993",                                                                              //  0x041d
    L"May\x0131s 1993",                                                                       //  0x041f
    L"maj 1993"                                                                               //  0x0424
};




//
//  Forward Declarations.
//

BOOL
InitGetDateFormat();

int
GDF_BadParamCheck();

int
GDF_NormalCase();

int
GDF_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetDateFormat
//
//  Test routine for GetDateFormatW API.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetDateFormat()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetDateFormatW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetDateFormat())
    {
        printf("\nABORTED TestGetDateFormat: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GDF_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GDF_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GDF_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetDateFormatW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetDateFormat
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetDateFormat()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Initialize the system date.
    //
    SysDate.wYear = 1993;
    SysDate.wMonth = 5;
    SysDate.wDayOfWeek = 6;
    SysDate.wDay = 1;
    SysDate.wHour = 15;
    SysDate.wMinute = 45;
    SysDate.wSecond = 25;
    SysDate.wMilliseconds = 13;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GDF_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GDF_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyDate;            // structure to hold custom date


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetDateFormatW( (LCID)333,
                         0,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpDateStr = NULL
    rc = GetDateFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         NULL,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpDateStr NULL",
                         &NumErrors );


    //
    //  Bad Count.
    //

    //  Variation 1  -  cchDate < 0
    rc = GetDateFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         lpDateStr,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchDate < 0",
                         &NumErrors );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  flags invalid
    rc = GetDateFormatW( Locale,
                         GDF_INVALID_FLAGS,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Flag invalid",
                         &NumErrors );

    //  Variation 2  -  short date AND long date
    rc = GetDateFormatW( Locale,
                         DATE_SHORTDATE | DATE_LONGDATE,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Flag invalid (shortdate and longdate)",
                         &NumErrors );

    //  Variation 3  -  pFormat not null AND short date
    rc = GetDateFormatW( Locale,
                         DATE_SHORTDATE,
                         NULL,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "pFormat not null and short date",
                         &NumErrors );

    //  Variation 4  -  pFormat not null AND long date
    rc = GetDateFormatW( Locale,
                         DATE_LONGDATE,
                         NULL,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "pFormat not null and long date",
                         &NumErrors );

    //  Variation 5  -  pFormat not null AND no user override
    rc = GetDateFormatW( Locale,
                         LOCALE_NOUSEROVERRIDE,
                         NULL,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "pFormat not null and no user override",
                         &NumErrors );

    //  Variation 6  -  Use CP ACP, pFormat not null AND no user override
    rc = GetDateFormatW( Locale,
                         LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE,
                         NULL,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, pFormat not null and no user override",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchDate = too small
    rc = GetDateFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         lpDateStr,
                         2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchDate too small",
                         &NumErrors );


    //
    //  Bad date passed in.
    //

    //  Variation 1  -  bad wMonth (0)
    MyDate.wYear = 1993;
    MyDate.wMonth = 0;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wMonth (0)",
                         &NumErrors );

    //  Variation 2  -  bad wMonth (13)
    MyDate.wYear = 1993;
    MyDate.wMonth = 13;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wMonth (13)",
                         &NumErrors );

    //  Variation 3  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 32;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (May 32)",
                         &NumErrors );

    //  Variation 4  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 4;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 31;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (April 31)",
                         &NumErrors );

    //  Variation 5  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 6;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 31;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (June 31)",
                         &NumErrors );

    //  Variation 6  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 9;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 31;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (Sept 31)",
                         &NumErrors );

    //  Variation 7  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 11;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 31;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (Nov 31)",
                         &NumErrors );

    //  Variation 8  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 2;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 29;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (Feb 29, 1993)",
                         &NumErrors );

    //  Variation 9  -  bad wDay
    MyDate.wYear = 1993;
    MyDate.wMonth = 2;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 30;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (Feb 30, 1993)",
                         &NumErrors );

    //  Variation 10  -  bad wDay
    MyDate.wYear = 2100;
    MyDate.wMonth = 2;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 29;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wDay (Feb 29, 2100)",
                         &NumErrors );

    //  Variation 11  -  bad wDayOfWeek
    MyDate.wYear = 1993;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 7;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "bad wDayOfWeek (7)",
                       &NumErrors );

    //  Variation 12  -  bad wDayOfWeek
    MyDate.wYear = 1993;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 4;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( Locale,
                         0,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "bad wDayOfWeek (Thursday, May 1, 1993)",
                       &NumErrors );



    //
    //  DATE_LTRREADING and DATE_RTLREADING flags.
    //

    SetLastError(0);
    rc = GetDateFormatW( Locale,
                         DATE_LTRREADING | DATE_RTLREADING,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "LTR and RTL flags",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GDF_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GDF_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyDate;            // structure to hold custom date
    int ctr;                      // loop counter


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  Locales.
    //

    //  Variation 1  -  System Default Locale
    rc = GetDateFormatW( LOCALE_SYSTEM_DEFAULT,
                         0,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "system default locale",
                      &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = GetDateFormatW( LOCALE_USER_DEFAULT,
                         0,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "current user locale",
                      &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  neutral
    rc = GetDateFormatW( 0x0000,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "neutral locale",
                       &NumErrors );

    //  Variation 2  -  sys default
    rc = GetDateFormatW( 0x0400,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "sys default locale",
                       &NumErrors );

    //  Variation 3  -  user default
    rc = GetDateFormatW( 0x0800,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "user default locale",
                       &NumErrors );

    //  Variation 4  -  sub lang neutral US
    rc = GetDateFormatW( 0x0009,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "sub lang neutral US",
                       &NumErrors );

    //  Variation 5  -  sub lang neutral Czech
    rc = GetDateFormatW( 0x0005,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       CZECH,
                       "sub lang neutral Czech",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = GetDateFormatW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_USE_CP_ACP,
                         NULL,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "Use CP ACP, system default locale",
                      &NumErrors );


    //
    //  cchDate.
    //

    //  Variation 1  -  cchDate = size of lpDateStr buffer
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "cchDate = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchDate = 0
    lpDateStr[0] = 0x0000;
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       ENGLISH_US,
                       "cchDate zero",
                       &NumErrors );

    //  Variation 3  -  cchDate = 0, lpDateStr = NULL
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         NULL,
                         NULL,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       ENGLISH_US,
                       "cchDate (NULL ptr)",
                       &NumErrors );


    //
    //  lpFormat.
    //

    //  Variation 1  -  Year
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"yyyy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       YEAR,
                       "lpFormat year (yyyy)",
                       &NumErrors );

    //  Variation 2  -  Year
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"yyy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       YEAR,
                       "lpFormat year (yyy)",
                       &NumErrors );

    //  Variation 3  -  Year
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"yyyyy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       YEAR,
                       "lpFormat year (yyyyy)",
                       &NumErrors );

    //  Variation 4  -  Year
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"yy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       YEAR_2,
                       "lpFormat year (yy)",
                       &NumErrors );

    //  Variation 5  -  Year
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"y",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       YEAR_2,
                       "lpFormat year (y)",
                       &NumErrors );

    //  Variation 6  -  US day of week
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_DAYOFWEEK,
                       "US day of week",
                       &NumErrors );

    //  Variation 7  -  US day of week
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"ddddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_DAYOFWEEK,
                       "US day of week (ddddd)",
                       &NumErrors );

    //  Variation 8  -  US abbrev day of week
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"ddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_ABBREVDAY,
                       "US abbrev day of week",
                       &NumErrors );

    //  Variation 9  -  US Month
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_MONTH,
                       "US Month",
                       &NumErrors );

    //  Variation 10  -  US Month
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"MMMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_MONTH,
                       "US Month (MMMMM)",
                       &NumErrors );

    //  Variation 11  -  US Abbrev Month
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"MMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       US_ABBREVMONTH,
                       "US Abbrev Month",
                       &NumErrors );

    //  Variation 12  -  SPANISH day of week
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"dddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_DAYOFWEEK,
                       "SPANISH day of week",
                       &NumErrors );

    //  Variation 13  -  SPANISH day of week
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"ddddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_DAYOFWEEK,
                       "SPANISH day of week (ddddd)",
                       &NumErrors );

    //  Variation 14  -  SPANISH abbrev day of week
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"ddd",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_ABBREVDAY,
                       "SPANISH abbrev day of week",
                       &NumErrors );

    //  Variation 15  -  SPANISH Month
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_MONTH,
                       "SPANISH Month",
                       &NumErrors );

    //  Variation 16  -  SPANISH Month
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"MMMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_MONTH,
                       "SPANISH Month (MMMMM)",
                       &NumErrors );

    //  Variation 17  -  SPANISH Abbrev Month
    rc = GetDateFormatW( 0x040a,
                         0,
                         &SysDate,
                         L"MMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       SPANISH_ABBREVMONTH,
                       "SPANISH Abbrev Month",
                       &NumErrors );


    //
    //  Single quote usage.
    //

    //  Variation 1  -  US single quote
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"dddd, MMMM dd, '''yy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"Saturday, May 01, 'yy",
                       "US single quote",
                       &NumErrors );

    //  Variation 2  -  US single quote 2
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L"dddd, MMMM dd, ''''yy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"Saturday, May 01, '93",
                       "US single quote 2",
                       &NumErrors );

    //  Variation 3  -  US single quote 3
    rc = GetDateFormatW( Locale,
                         0,
                         &SysDate,
                         L" 'year: '''yy",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L" year: '93",
                       "US single quote 3",
                       &NumErrors );

    //  Variation 4  -  SHORTDATE single quote
    rc = GetLocaleInfoW( 0x0409,
                         LOCALE_SSHORTDATE,
                         pSShortDate,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GetLocaleInfoW SSHORTDATE",
                      &NumErrors );

    rc = SetLocaleInfoW( 0x0409,
                         LOCALE_SSHORTDATE,
                         L"MM''''dd''''yy" );
    CheckReturnValidW( rc,
                       TRUE,
                       NULL,
                       NULL,
                       "SetLocaleInfoW SSHORTDATE",
                       &NumErrors );

    rc = GetDateFormatW( 0x0409,
                         DATE_SHORTDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"05'01'93",
                       "ShortDate single quote 1",
                       &NumErrors );

    rc = SetLocaleInfoW( 0x0409,
                         LOCALE_SSHORTDATE,
                         L"'Date: 'MMM ''''yy" );
    CheckReturnValidW( rc,
                       TRUE,
                       NULL,
                       NULL,
                       "SetLocaleInfoW SSHORTDATE",
                       &NumErrors );

    rc = GetDateFormatW( 0x0409,
                         DATE_SHORTDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"Date: May '93",
                       "ShortDate single quote 2",
                       &NumErrors );

    rc = SetLocaleInfoW( 0x0409,
                         LOCALE_SSHORTDATE,
                         pSShortDate );
    CheckReturnValidW( rc,
                       TRUE,
                       NULL,
                       NULL,
                       "SetLocaleInfoW SSHORTDATE Final",
                       &NumErrors );


    //
    //  Test all locales - Short Date flag value.
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetDateFormatW( pAllLocales[ctr],
                             DATE_SHORTDATE,
                             &SysDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpDateStr,
                               pShortDate[ctr],
                               "ShortDate",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  Test all locales - Long Date flag value.
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetDateFormatW( pAllLocales[ctr],
                             DATE_LONGDATE,
                             &SysDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpDateStr,
                               pLongDate[ctr],
                               "LongDate",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  Test all locales - Year Month flag value.
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetDateFormatW( pAllLocales[ctr],
                             DATE_YEARMONTH,
                             &SysDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpDateStr,
                               pYearMonth[ctr],
                               "YearMonth",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  NO User Override flag value.
    //

    //  Variation 1  -  NOUSEROVERRIDE
    rc = GetDateFormatW( Locale,
                         LOCALE_NOUSEROVERRIDE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "NoUserOverride",
                       &NumErrors );


    //
    //  Test Russian date formats.
    //
    //  Variation 1 -  LONGDATE Russian
    rc = GetDateFormatW( 0x0419,
                         0,
                         &SysDate,
                         L"MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_RUSSIAN_2,
                       "LongDate Russian 2",
                       &NumErrors );

    //  Variation 2 -  LONGDATE Russian
    rc = GetDateFormatW( 0x0419,
                         0,
                         &SysDate,
                         L"MMMM d",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_RUSSIAN_3,
                       "LongDate Russian 3",
                       &NumErrors );

    //  Variation 3 -  LONGDATE Russian
    rc = GetDateFormatW( 0x0419,
                         0,
                         &SysDate,
                         L"d yy MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_RUSSIAN_4,
                       "LongDate Russian 4",
                       &NumErrors );


    //
    //  Test Polish date formats.
    //
    //  Variation 1 -  LONGDATE Polish
    rc = GetDateFormatW( 0x0415,
                         0,
                         &SysDate,
                         L"MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_POLISH_2,
                       "LongDate Polish 2",
                       &NumErrors );

    //  Variation 2 -  LONGDATE Polish
    rc = GetDateFormatW( 0x0415,
                         0,
                         &SysDate,
                         L"MMMM d",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_POLISH_3,
                       "LongDate Polish 3",
                       &NumErrors );

    //  Variation 3 -  LONGDATE Polish
    rc = GetDateFormatW( 0x0415,
                         0,
                         &SysDate,
                         L"d yy MMMM",
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_POLISH_4,
                       "LongDate Polish 4",
                       &NumErrors );


    //
    //  Test various calendars.
    //

    //  Variation 1 -  English
    rc = GetDateFormatW( 0x0409,
                         DATE_USE_ALT_CALENDAR,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       ENGLISH_US,
                       "Alt Calendar English (ShortDate)",
                       &NumErrors );

    //  Variation 2 -  English
    rc = GetDateFormatW( 0x0409,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_ENGLISH_US,
                       "Alt Calendar English (LongDate)",
                       &NumErrors );

    //  Variation 3 -  Japan
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       JAPAN_ALT,
                       "Alt Calendar Japan (ShortDate)",
                       &NumErrors );

    //  Variation 4 -  Japan
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_JAPAN_ALT,
                       "Alt Calendar Japan (LongDate)",
                       &NumErrors );

#if 0
    //  Variation 5 -  China
    rc = GetDateFormatW( 0x0404,
                         DATE_USE_ALT_CALENDAR,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       CHINA_ALT,
                       "Alt Calendar China (ShortDate)",
                       &NumErrors );

    //  Variation 6 -  China
    rc = GetDateFormatW( 0x0404,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_CHINA_ALT,
                       "Alt Calendar China (LongDate)",
                       &NumErrors );
#endif

    //  Variation 7 -  Korea
    rc = GetDateFormatW( 0x0412,
                         DATE_USE_ALT_CALENDAR,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       KOREA_ALT,
                       "Alt Calendar Korea (ShortDate)",
                       &NumErrors );

    //  Variation 8 -  Korea
    rc = GetDateFormatW( 0x0412,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_KOREA_ALT,
                       "Alt Calendar Korea (LongDate)",
                       &NumErrors );



    //
    //  Different dates for calendars.
    //

    //  Variation 1 -  Japan
    MyDate.wYear = 1913;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_JAPAN_ALT_2,
                       "Alt Calendar Japan (long) 2",
                       &NumErrors );

#if 0
    //  Variation 2 -  China
    MyDate.wYear = 1832;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0404,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_CHINA_ALT_2,
                       "Alt Calendar China (long) 2",
                       &NumErrors );
#endif

    //  Variation 3 -  Korea
    MyDate.wYear = 1832;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0412,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_KOREA_ALT_2,
                       "Alt Calendar Korea (long) 2",
                       &NumErrors );

    //  Variation 4 -  Japan
    MyDate.wYear = 1832;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L_JAPAN_ALT_3,
                       "Alt Calendar Japan (long) 3",
                       &NumErrors );


    //  Variation 5 -  Japan
    MyDate.wYear = 1989;
    MyDate.wMonth = 1;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 8;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x5e73\x6210 1/1/8",
                       "Alt Calendar Japan (short) 1",
                       &NumErrors );

    //  Variation 6 -  Japan
    MyDate.wYear = 1989;
    MyDate.wMonth = 1;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 7;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x662d\x548c 64/1/7",
                       "Alt Calendar Japan (short) 2",
                       &NumErrors );

    //  Variation 7 -  Japan
    MyDate.wYear = 1989;
    MyDate.wMonth = 2;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 3;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x5e73\x6210 1/2/3",
                       "Alt Calendar Japan (short) 3",
                       &NumErrors );

    //  Variation 8 -  Japan
    MyDate.wYear = 1926;
    MyDate.wMonth = 2;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 3;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x5927\x6b63 15/2/3",
                       "Alt Calendar Japan (short) 4",
                       &NumErrors );

    //  Variation 9 -  Japan
    MyDate.wYear = 1989;
    MyDate.wMonth = 1;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 8;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x5e73\x6210 1\x5e74\x0031\x6708\x0038\x65e5",
                       "Alt Calendar Japan (long) 4",
                       &NumErrors );

    //  Variation 10 -  Japan
    MyDate.wYear = 1989;
    MyDate.wMonth = 1;
    MyDate.wDayOfWeek = 1;
    MyDate.wDay = 7;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;
    rc = GetDateFormatW( 0x0411,
                         DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x662d\x548c 64\x5e74\x0031\x6708\x0037\x65e5",
                       "Alt Calendar Japan (long) 5",
                       &NumErrors );


    //
    //  DATE_LTRREADING and DATE_RTLREADING flags.
    //

    rc = GetDateFormatW( Locale,
                         DATE_LTRREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200e\x0035/\x200e\x0031/\x200e\x0031\x0039\x0039\x0033",
                       "LTR flag (shortdate) - US",
                       &NumErrors );

    rc = GetDateFormatW( Locale,
                         DATE_RTLREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200f\x0035/\x200f\x0031/\x200f\x0031\x0039\x0039\x0033",
                       "RTL flag (shortdate) - US",
                       &NumErrors );

    rc = GetDateFormatW( Locale,
                         DATE_SHORTDATE | DATE_LTRREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200e\x0035/\x200e\x0031/\x200e\x0031\x0039\x0039\x0033",
                       "LTR flag (shortdate) - US 2",
                       &NumErrors );

    rc = GetDateFormatW( Locale,
                         DATE_SHORTDATE | DATE_RTLREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200f\x0035/\x200f\x0031/\x200f\x0031\x0039\x0039\x0033",
                       "RTL flag (shortdate) - US 2",
                       &NumErrors );

    rc = GetDateFormatW( Locale,
                         DATE_LONGDATE | DATE_LTRREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200eSaturday, \x200eMay \x200e\x0030\x0031, \x200e\x0031\x0039\x0039\x0033",
                       "LTR flag (longdate) - US",
                       &NumErrors );

    rc = GetDateFormatW( Locale,
                         DATE_LONGDATE | DATE_RTLREADING,
                         &SysDate,
                         NULL,
                         lpDateStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpDateStr,
                       L"\x200fSaturday, \x200fMay \x200f\x0030\x0031, \x200f\x0031\x0039\x0039\x0033",
                       "RTL flag (longdate) - US",
                       &NumErrors );

    // Iran - Farsi
    if (IsValidLocale(0x0429, LCID_INSTALLED))
    {
        rc = GetDateFormatW( 0x0429,
                             DATE_LTRREADING,
                             &SysDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"\x200e\x0031\x0039\x0039\x0033/\x200e\x0030\x0035/\x200e\x0030\x0031",
                           "LTR - Iran Farsi",
                           &NumErrors );

        rc = GetDateFormatW( 0x0429,
                             DATE_RTLREADING,
                             &SysDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"\x200f\x0031\x0039\x0039\x0033/\x200f\x0030\x0035/\x200f\x0030\x0031",
                           "RTL - Iran Farsi",
                           &NumErrors );
    }



    //
    //  Hijri Calendar.
    //

    if (IsValidLocale(0x0401, LCID_INSTALLED))
    {
        //  Variation 1  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x0401,
                             DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"07/12/64",
                           "Hijri (short) 1",
                           &NumErrors );

        //  Variation 2  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x0401,
                             DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"07/\x0630\x0648\x00a0\x0627\x0644\x062d\x062c\x0629/1364",   // year 1364
                           "Hijri (long) 1",
                           &NumErrors );

        //  Variation 3  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x0401,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"07/12/64",     // year 1364
                           "Alt Calendar Hijri (short) 1",
                           &NumErrors );

        //  Variation 4  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x0401,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"07/\x0630\x0648\x00a0\x0627\x0644\x062d\x062c\x0629/1364",   // year 1364
                           "Alt Calendar Hijri (long) 1",
                           &NumErrors );
    }



    //
    //  Hebrew Calendar.
    //

    if (IsValidLocale(0x040d, LCID_INSTALLED))
    {
        //  Variation 1  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"12/11/1945",
                           "Hebrew (short) 1",
                           &NumErrors );

        //  Variation 2  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,
                           -1,
                           lpDateStr,
                           L"\x05d9\x05d5\x05dd\x00a0\x05e9\x05e0\x05d9 12 \x05e0\x05d5\x05d1\x05de\x05d1\x05e8 1945",
                           "Hebrew (long) 1",
                           &NumErrors );

        //  Variation 3  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,                        // Kislev 7, 5706
                           -1,
                           lpDateStr,
                           L"\x05d6'/\x05db\x05e1\x05dc\x05d5/\x05ea\x05e9\"\x05d5",
                           "Alt Calendar Hebrew (short) 1",
                           &NumErrors );

        //  Variation 4  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,                        // Kislev 7, 5706
                           -1,
                           lpDateStr,
                           L"\x05d6' \x05db\x05e1\x05dc\x05d5 \x05ea\x05e9\"\x05d5",
                           "Alt Calendar Hebrew (long) 1",
                           &NumErrors );

        //  Variation 5  -  Hebrew
        MyDate.wYear = 1984;
        MyDate.wMonth = 9;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 27;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,                        // Tishri 1, 5745
                           -1,
                           lpDateStr,
                           L"\x05d0'/\x05ea\x05e9\x05e8\x05d9/\x05ea\x05e9\x05de\"\x05d4",
                           "Alt Calendar Hebrew (short) 2",
                           &NumErrors );


        //  Variation 6  -  Hebrew
        MyDate.wYear = 1984;
        MyDate.wMonth = 9;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 27;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatW( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             lpDateStr,
                             BUFSIZE );
        CheckReturnValidW( rc,                        // Tishri 1, 5745
                           -1,
                           lpDateStr,
                           L"\x05d0' \x05ea\x05e9\x05e8\x05d9 \x05ea\x05e9\x05de\"\x05d4",
                           "Alt Calendar Hebrew (long) 2",
                           &NumErrors );
    }


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GDF_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GDF_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyDate;            // structure to hold custom date
    BYTE pDateStrA[BUFSIZE];      // ptr to date string


    MyDate.wYear = 1993;
    MyDate.wMonth = 5;
    MyDate.wDayOfWeek = 6;
    MyDate.wDay = 1;
    MyDate.wHour = 15;
    MyDate.wMinute = 45;
    MyDate.wSecond = 25;
    MyDate.wMilliseconds = 13;


    //
    //  GetDateFormatA.
    //

    //  Variation 1  -  US single quote
    rc = GetDateFormatA( Locale,
                         0,
                         &MyDate,
                         "dddd, MMMM dd, ''''yy",
                         pDateStrA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pDateStrA,
                       "Saturday, May 01, '93",
                       NULL,
                       "A version US single quote",
                       &NumErrors );

    //  Variation 2  -  US single quote (no dest)
    rc = GetDateFormatA( Locale,
                         0,
                         &MyDate,
                         "dddd, MMMM dd, ''''yy",
                         NULL,
                         0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "Saturday, May 01, '93",
                       NULL,
                       "A version US single quote, no Dest",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, US single quote
    rc = GetDateFormatA( Locale,
                         LOCALE_USE_CP_ACP,
                         &MyDate,
                         "dddd, MMMM dd, ''''yy",
                         pDateStrA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pDateStrA,
                       "Saturday, May 01, '93",
                       NULL,
                       "A version Use CP ACP, US single quote",
                       &NumErrors );


    //
    //  Make sure the A and W versions set the same error value.
    //

    SetLastError(0);
    rc = GetDateFormatA( Locale,
                         DATE_SHORTDATE,
                         &MyDate,
                         NULL,
                         pDateStrA,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W same bad param - A version",
                         &NumErrors );

    SetLastError(0);
    rc = GetDateFormatW( Locale,
                         DATE_SHORTDATE,
                         &MyDate,
                         NULL,
                         lpDateStr,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W same bad param - W version",
                         &NumErrors );


    //
    //  DATE_LTRREADING and DATE_RTLREADING flags.
    //

    SetLastError(0);
    rc = GetDateFormatA( Locale,
                         DATE_LTRREADING | DATE_RTLREADING,
                         &MyDate,
                         NULL,
                         pDateStrA,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "A version - LTR and RTL flags",
                         &NumErrors );

    SetLastError(0);
    rc = GetDateFormatA( Locale,
                         DATE_LTRREADING,
                         &MyDate,
                         NULL,
                         pDateStrA,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "A version - LTR flag",
                         &NumErrors );

    SetLastError(0);
    rc = GetDateFormatA( Locale,
                         DATE_RTLREADING,
                         &MyDate,
                         NULL,
                         pDateStrA,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "A version - RTL flag",
                         &NumErrors );

    // Iran - Farsi
    if (IsValidLocale(0x0429, LCID_INSTALLED))
    {
        rc = GetDateFormatA( 0x0429,
                             DATE_LTRREADING,
                             &SysDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "\xfd\x31\x39\x39\x33/\xfd\x30\x35/\xfd\x30\x31",
                           NULL,
                           "A version - LTR - Iran Farsi",
                           &NumErrors );

        rc = GetDateFormatA( 0x0429,
                             DATE_RTLREADING,
                             &SysDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "\xfe\x31\x39\x39\x33/\xfe\x30\x35/\xfe\x30\x31",
                           NULL,
                           "A version - RTL - Iran Farsi",
                           &NumErrors );
    }



    //
    //  Hijri Calendar.
    //

    if (IsValidLocale(0x0401, LCID_INSTALLED))
    {
        //  Variation 1  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x0401,
                             DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "07/12/64",
                           NULL,
                           "A version Hijri (short) 1",
                           &NumErrors );

        //  Variation 2  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x0401,
                             DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "07/\xd0\xe6\xa0\xc7\xe1\xcd\xcc\xc9/1364",   // year 1364
                           NULL,
                           "A version Hijri (long) 1",
                           &NumErrors );

        //  Variation 3  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x0401,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "07/12/64",     // year 1364
                           NULL,
                           "A version Alt Calendar Hijri (short) 1",
                           &NumErrors );

        //  Variation 4  -  Hijri
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x0401,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "07/\xd0\xe6\xa0\xc7\xe1\xcd\xcc\xc9/1364",   // year 1364
                           NULL,
                           "A version Alt Calendar Hijri (long) 1",
                           &NumErrors );
    }



    //
    //  Hebrew Calendar.
    //

    if (IsValidLocale(0x040d, LCID_INSTALLED))
    {
        //  Variation 1  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "12/11/1945",
                           NULL,
                           "A version Hebrew (short) 1",
                           &NumErrors );

        //  Variation 2  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,
                           -1,
                           pDateStrA,
                           "\xe9\xe5\xed\xa0\xf9\xf0\xe9 12 \xf0\xe5\xe1\xee\xe1\xf8 1945",
                           NULL,
                           "A version Hebrew (long) 1",
                           &NumErrors );

        //  Variation 3  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,                        // Kislev 7, 5706
                           -1,
                           pDateStrA,
                           "\xe6'/\xeb\xf1\xec\xe5/\xfa\xf9\"\xe5",
                           NULL,
                           "A version Alt Calendar Hebrew (short) 1",
                           &NumErrors );

        //  Variation 4  -  Hebrew
        MyDate.wYear = 1945;
        MyDate.wMonth = 11;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 12;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,                        // Kislev 7, 5706
                           -1,
                           pDateStrA,
                           "\xe6' \xeb\xf1\xec\xe5 \xfa\xf9\"\xe5",
                           NULL,
                           "A version Alt Calendar Hebrew (long) 1",
                           &NumErrors );

        //  Variation 5  -  Hebrew
        MyDate.wYear = 1984;
        MyDate.wMonth = 9;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 27;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_SHORTDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,                        // Tishri 1, 5745
                           -1,
                           pDateStrA,
                           "\xe0'/\xfa\xf9\xf8\xe9/\xfa\xf9\xee\"\xe4",
                           NULL,
                           "A version Alt Calendar Hebrew (short) 2",
                           &NumErrors );


        //  Variation 6  -  Hebrew
        MyDate.wYear = 1984;
        MyDate.wMonth = 9;
        MyDate.wDayOfWeek = 1;
        MyDate.wDay = 27;
        MyDate.wHour = 15;
        MyDate.wMinute = 45;
        MyDate.wSecond = 25;
        MyDate.wMilliseconds = 13;
        rc = GetDateFormatA( 0x040d,
                             DATE_USE_ALT_CALENDAR | DATE_LONGDATE,
                             &MyDate,
                             NULL,
                             pDateStrA,
                             BUFSIZE );
        CheckReturnValidA( rc,                        // Tishri 1, 5745
                           -1,
                           pDateStrA,
                           "\xe0' \xfa\xf9\xf8\xe9 \xfa\xf9\xee\"\xe4",
                           NULL,
                           "A version Alt Calendar Hebrew (long) 2",
                           &NumErrors );
    }


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
