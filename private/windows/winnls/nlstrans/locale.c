/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    locale.c

Abstract:

    This file contains functions necessary to parse and write the locale
    specific tables to a data file.

    External Routines in this file:
      ParseWriteLocale

Revision History:

    12-10-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstrans.h"




//
//  Constant Declarations.
//

#define OPT_CAL_FLAG    1
#define ERA_RANGE_FLAG  2




//
//  Forward Declarations.
//

int
ParseLocaleInfo(
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PLOCALE_HEADER pLocCnt,
    PSZ pszKeyWord);

int
ParseLine(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    BOOL fConvert);

int
ParseMultiLine(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize);

int ParseMultiLineSize(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    WORD *pNum,
    int Flag);

int
GetLocaleInfoSize(
    int *pSize);

int
ScanBuffer(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    BYTE *pszKey,
    BOOL fConvert);

int
WriteLocaleInit(
    FILE **ppOutputFile,
    int NumLoc,
    int OffLoc);

int
WriteLocaleInfo(
    DWORD Locale,
    int *pOffHdr,
    int *pOffLoc,
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PLOCALE_HEADER pLocCnt,
    FILE *pOutputFile);

int
WriteVariableLength(
    PLOCALE_HEADER pLocCnt,
    PLOCALE_VARIABLE pLocVar,
    int *pTotalSize,
    FILE *pOutputFile);

int
ParseWriteCalendar(
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PSZ pszKeyWord,
    FILE *pOutputFile,
    int OffHdr);

int
ParseCalendarInfo(
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PCALENDAR_HEADER pCalCnt,
    PSZ pszKeyWord);

int
WriteCalendarInit(
    FILE *pOutputFile,
    int NumCal,
    int OffCalHdr);

int
WriteCalendarInfo(
    DWORD Calendar,
    int *pOffHdr,
    int *pOffCal,
    int OffCalBegin,
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PCALENDAR_HEADER pCalCnt,
    FILE *pOutputFile);

int
WriteCalendarVariableLength(
    PCALENDAR_HEADER pCalCnt,
    PCALENDAR_VARIABLE pCalVar,
    int *pTotalSize,
    FILE *pOutputFile);

int
ConvertUnicodeToWord(
    WORD *pString,
    WORD *pValue);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseWriteLocale
//
//  This routine parses the input file for the locale specific tables, and
//  then writes the data to the output file.  This routine is only entered
//  when the LOCALE keyword is found.  The parsing continues until the
//  ENDLOCALE keyword is found.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseWriteLocale(
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PSZ pszKeyWord)
{
    int Num;                      // number of locales
    int OffHdr;                   // file offset to header info
    int OffLoc;                   // file offset to locale info
    DWORD LocId;                  // locale id
    FILE *pOutputFile;            // ptr to output file
    LOCALE_HEADER LocCnt;         // locale character counts
    CALENDAR_HEADER CalHdr;       // calendar header structure
    CALENDAR_VARIABLE CalVar;     // calendar variable structure


    //
    //  Get size parameter.
    //
    if (GetSize(&Num))
        return (1);

    //
    //  Set up initial file pointer offsets.
    //
    //      OffHdr = header size
    //      OffLoc = header size + (Number of locales * header entry size)
    //
    OffHdr = LOC_CAL_HDR_WORDS;
    OffLoc = LOC_CAL_HDR_WORDS + (Num * LOCALE_HDR_WORDS);

    //
    //  Initialize the output file and write the number of locales to
    //  the file.  Also, in order to allow for the seek, write zeros
    //  in the file up to the first locale field.
    //
    if (WriteLocaleInit( &pOutputFile,
                         Num,
                         OffLoc ))
    {
        return (1);
    }

    //
    //  Parse all of the locales one by one.  Write each one to the file
    //  separately to conserve memory.
    //
    for (; Num > 0; Num--)
    {
        //
        //  Initialize all Locale structures each time.
        //
        memset(pLocHdr, 0, sizeof(LOCALE_HEADER));
        memset(pLocStat, 0, sizeof(LOCALE_STATIC));
        memset(pLocVar, 0, sizeof(LOCALE_VARIABLE));
        memset(&LocCnt, 0, sizeof(LOCALE_HEADER));

        //
        //  Get the BEGINLOCALE keyword and locale id.
        //
        if (fscanf( pInputFile,
                    "%s %lx ;%*[^\n]",
                    pszKeyWord,
                    &LocId ) == 2)
        {
            if (_strcmpi(pszKeyWord, "BEGINLOCALE") == 0)
            {
                if (Verbose)
                    printf("\n\nFound BEGINLOCALE keyword, LocaleID = %x\n\n",
                           LocId);
            }
            else
            {
                printf("Parse Error: Error reading BEGINLOCALE and Locale ID.\n");
                fclose(pOutputFile);
                return (1);
            }
        }
        else
        {
            printf("Parse Error: Invalid Instruction '%s'.\n", pszKeyWord);
            printf("             Expecting BEGINLOCALE keyword and Locale ID.\n");
            fclose(pOutputFile);
            return (1);
        }

        //
        //  Parse the locale information.
        //
        if (ParseLocaleInfo( pLocHdr,
                             pLocStat,
                             pLocVar,
                             &LocCnt,
                             pszKeyWord ))
        {
            printf("Parse Error: Language == %ws.\n", pLocStat->szILanguage);
            fclose(pOutputFile);
            return (1);
        }

        //
        //  Write the locale id, offset, and locale information to
        //  the output file.
        //
        if (WriteLocaleInfo( LocId,
                             &OffHdr,
                             &OffLoc,
                             pLocHdr,
                             pLocStat,
                             pLocVar,
                             &LocCnt,
                             pOutputFile ))
        {
            printf("Write Error: Language == %ws.\n", pLocStat->szILanguage);
            fclose(pOutputFile);
            return (1);
        }
    }

    //
    //  Look for ENDLOCALE keyword.
    //
    if (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "ENDLOCALE") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDLOCALE keyword.\n");
        }
        else
        {
            //
            //  The ENDLOCALE keyword was not found.  Return failure.
            //
            printf("Parse Error: Expecting ENDLOCALE keyword.\n");
            fclose(pOutputFile);
            return (1);
        }
    }
    else
    {
        //
        //  The ENDLOCALE keyword was not found.  Return failure.
        //
        printf("Parse Error: Expecting ENDLOCALE keyword.\n");
        fclose(pOutputFile);
        return (1);
    }


    //
    //  Look for CALENDAR keyword.
    //
    if (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "CALENDAR") == 0)
        {
            if (Verbose)
                printf("\n\nFound CALENDAR keyword.\n");
        }
        else
        {
            //
            //  The CALENDAR keyword was not found.  Return failure.
            //
            printf("Parse Error: Expecting CALENDAR keyword.\n");
            fclose(pOutputFile);
            return (1);
        }
    }
    else
    {
        //
        //  The CALENDAR keyword was not found.  Return failure.
        //
        printf("Parse Error: Expecting CALENDAR keyword.\n");
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Get the valid keywords for CALENDAR.
    //  Write the CALENDAR information to an output file.
    //
    if (ParseWriteCalendar( &CalHdr,
                            &CalVar,
                            pszKeyWord,
                            pOutputFile,
                            OffLoc ))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", LOCALE_FILE);
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseLocaleInfo
//
//  This routine parses the locale information from the input file.  If an
//  error is encountered, a 1 is returned.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseLocaleInfo(
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PLOCALE_HEADER pLocCnt,
    PSZ pszKeyWord)
{
    int Count;               // number of characters written
    WORD Tmp;                // tmp place holder


    //
    //  Read in the information associated with the language of a locale
    //  and store it in the locale structure.
    //
    if (!ParseLine( pszKeyWord,
                    pLocStat->szILanguage,
                    5,
                    TRUE ))
    {
        return (1);
    }

    pLocHdr->SLanguage = (sizeof(LOCALE_HEADER) + sizeof(LOCALE_STATIC)) /
                         sizeof(WORD);
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSLanguage,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SLanguage = Count;

    pLocHdr->SAbbrevLang = pLocHdr->SLanguage + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevLang,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevLang = Count;

    pLocHdr->SAbbrevLangISO = pLocHdr->SAbbrevLang + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevLangISO,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevLangISO = Count;

    pLocHdr->SNativeLang = pLocHdr->SAbbrevLangISO + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSNativeLang,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SNativeLang = Count;

    //
    //  Read in the information associated with the country of a locale
    //  and store it in the locale structure.
    //
    if (!ParseLine( pszKeyWord,
                    pLocStat->szICountry,
                    6,
                    TRUE ))
    {
        return (1);
    }

    pLocHdr->SCountry = pLocHdr->SNativeLang + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSCountry,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SCountry = Count;

    pLocHdr->SAbbrevCtry = pLocHdr->SCountry + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevCtry,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevCtry = Count;

    pLocHdr->SAbbrevCtryISO = pLocHdr->SAbbrevCtry + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevCtryISO,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevCtryISO = Count;

    pLocHdr->SNativeCtry = pLocHdr->SAbbrevCtryISO + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSNativeCtry,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SNativeCtry = Count;

    //
    //  Read in the default language, country, and code pages of a locale
    //  and store it in the locale structure.
    //
    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultLang,
                    5,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultCtry,
                    6,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultACP,
                    6,
                    TRUE ))
    {
        return (1);
    }
    if (!ConvertUnicodeToWord( pLocStat->szIDefaultACP,
                               &(pLocStat->DefaultACP) ))
    {
        printf("Parse Error: Invalid IDEFAULTACP value  %s\n",
               pLocStat->szIDefaultACP);
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultOCP,
                    6,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultMACCP,
                    6,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDefaultEBCDICCP,
                    6,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the list separator, measurement info, and default paper size
    //  of a locale and store it in the locale structure.
    //
    pLocHdr->SList = pLocHdr->SNativeCtry + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSList,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SList = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIMeasure,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIPaperSize,
                    2,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the digits information of a locale and store it in the
    //  locale structure.
    //
    pLocHdr->SDecimal = pLocHdr->SList + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDecimal,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDecimal = Count;

    pLocHdr->SThousand = pLocHdr->SDecimal + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSThousand,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SThousand = Count;

    pLocHdr->SGrouping = pLocHdr->SThousand + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSGrouping,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SGrouping = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDigits,
                    3,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szILZero,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szINegNumber,
                    2,
                    TRUE ))
    {
        return (1);
    }

    pLocHdr->SNativeDigits = pLocHdr->SGrouping + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSNativeDigits,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SNativeDigits = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDigitSubstitution,
                    2,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the monetary information of a locale and store it in the
    //  locale structure.
    //
    pLocHdr->SCurrency = pLocHdr->SNativeDigits + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSCurrency,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SCurrency = Count;

    pLocHdr->SIntlSymbol = pLocHdr->SCurrency + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSIntlSymbol,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SIntlSymbol = Count;

    pLocHdr->SEngCurrName = pLocHdr->SIntlSymbol + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSEngCurrName,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SEngCurrName = Count;

    pLocHdr->SNativeCurrName = pLocHdr->SEngCurrName + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSNativeCurrName,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SNativeCurrName = Count;

    pLocHdr->SMonDecSep = pLocHdr->SNativeCurrName + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonDecSep,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonDecSep = Count;

    pLocHdr->SMonThousSep = pLocHdr->SMonDecSep + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonThousSep,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonThousSep = Count;

    pLocHdr->SMonGrouping = pLocHdr->SMonThousSep + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonGrouping,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonGrouping = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szICurrDigits,
                    3,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIIntlCurrDigits,
                    3,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szICurrency,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szINegCurr,
                    3,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the positive and negative sign information of a locale
    //  and store it in the locale structure.
    //
    pLocHdr->SPositiveSign = pLocHdr->SMonGrouping + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSPositiveSign,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SPositiveSign = Count;

    pLocHdr->SNegativeSign = pLocHdr->SPositiveSign + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSNegativeSign,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SNegativeSign = Count;

    //
    //  Read in the time information of a locale and store it
    //  in the locale structure.
    //
    pLocHdr->STimeFormat = pLocHdr->SNegativeSign + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pLocVar->szSTimeFormat,
                                  MAX )))
    {
        return (1);
    }
    pLocCnt->STimeFormat = Count;

    pLocHdr->STime = pLocHdr->STimeFormat + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSTime,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->STime = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szITime,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szITLZero,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szITimeMarkPosn,
                    2,
                    TRUE ))
    {
        return (1);
    }

    pLocHdr->S1159 = pLocHdr->STime + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szS1159,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->S1159 = Count;

    pLocHdr->S2359 = pLocHdr->S1159 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szS2359,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->S2359 = Count;

    //
    //  Read in the short date information of a locale and store it
    //  in the locale structure.
    //
    pLocHdr->SShortDate = pLocHdr->S2359 + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pLocVar->szSShortDate,
                                  MAX )))
    {
        return (1);
    }
    pLocCnt->SShortDate = Count;

    pLocHdr->SDate = pLocHdr->SShortDate + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDate,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDate = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDate,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szICentury,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIDayLZero,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIMonLZero,
                    2,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the long date information of a locale and store it
    //  in the locale structure.
    //
    pLocHdr->SYearMonth = pLocHdr->SDate + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pLocVar->szSYearMonth,
                                  MAX )))
    {
        return (1);
    }
    pLocCnt->SYearMonth = Count;

    pLocHdr->SLongDate = pLocHdr->SYearMonth + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pLocVar->szSLongDate,
                                  MAX )))
    {
        return (1);
    }
    pLocCnt->SLongDate = Count;

    if (!ParseLine( pszKeyWord,
                    pLocStat->szILDate,
                    2,
                    TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szICalendarType,
                    2,
                    TRUE ))
    {
        return (1);
    }

    pLocHdr->IOptionalCalendar = pLocHdr->SLongDate + Count;
    if (!(Count = ParseMultiLineSize( pszKeyWord,
                                      pLocVar->szIOptionalCalendar,
                                      MAX,
                                      &Tmp,
                                      OPT_CAL_FLAG )))
    {
        return (1);
    }
    pLocCnt->IOptionalCalendar = Count;

    if (!ParseLine(pszKeyWord,
                   pLocStat->szIFirstDayOfWeek,
                   2,
                   TRUE ))
    {
        return (1);
    }

    if (!ParseLine( pszKeyWord,
                    pLocStat->szIFirstWeekOfYear,
                    2,
                    TRUE ))
    {
        return (1);
    }

    //
    //  Read in the day information of a locale and store it in the
    //  locale structure.
    //
    pLocHdr->SDayName1 = pLocHdr->IOptionalCalendar + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName1 = Count;

    pLocHdr->SDayName2 = pLocHdr->SDayName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName2 = Count;

    pLocHdr->SDayName3 = pLocHdr->SDayName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName3 = Count;

    pLocHdr->SDayName4 = pLocHdr->SDayName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName4 = Count;

    pLocHdr->SDayName5 = pLocHdr->SDayName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName5 = Count;

    pLocHdr->SDayName6 = pLocHdr->SDayName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName6 = Count;

    pLocHdr->SDayName7 = pLocHdr->SDayName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSDayName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SDayName7 = Count;

    pLocHdr->SAbbrevDayName1 = pLocHdr->SDayName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName1 = Count;

    pLocHdr->SAbbrevDayName2 = pLocHdr->SAbbrevDayName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName2 = Count;

    pLocHdr->SAbbrevDayName3 = pLocHdr->SAbbrevDayName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName3 = Count;

    pLocHdr->SAbbrevDayName4 = pLocHdr->SAbbrevDayName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName4 = Count;

    pLocHdr->SAbbrevDayName5 = pLocHdr->SAbbrevDayName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName5 = Count;

    pLocHdr->SAbbrevDayName6 = pLocHdr->SAbbrevDayName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName6 = Count;

    pLocHdr->SAbbrevDayName7 = pLocHdr->SAbbrevDayName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevDayName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevDayName7 = Count;

    //
    //  Read in the month information of a locale and store it in the
    //  locale structure.
    //
    pLocHdr->SMonthName1 = pLocHdr->SAbbrevDayName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName1 = Count;

    pLocHdr->SMonthName2 = pLocHdr->SMonthName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName2 = Count;

    pLocHdr->SMonthName3 = pLocHdr->SMonthName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName3 = Count;

    pLocHdr->SMonthName4 = pLocHdr->SMonthName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName4 = Count;

    pLocHdr->SMonthName5 = pLocHdr->SMonthName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName5 = Count;

    pLocHdr->SMonthName6 = pLocHdr->SMonthName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName6 = Count;

    pLocHdr->SMonthName7 = pLocHdr->SMonthName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName7 = Count;

    pLocHdr->SMonthName8 = pLocHdr->SMonthName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName8,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName8 = Count;

    pLocHdr->SMonthName9 = pLocHdr->SMonthName8 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName9,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName9 = Count;

    pLocHdr->SMonthName10 = pLocHdr->SMonthName9 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName10,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName10 = Count;

    pLocHdr->SMonthName11 = pLocHdr->SMonthName10 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName11,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName11 = Count;

    pLocHdr->SMonthName12 = pLocHdr->SMonthName11 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName12,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName12 = Count;

    pLocHdr->SMonthName13 = pLocHdr->SMonthName12 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSMonthName13,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SMonthName13 = Count;

    pLocHdr->SAbbrevMonthName1 = pLocHdr->SMonthName13 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName1 = Count;

    pLocHdr->SAbbrevMonthName2 = pLocHdr->SAbbrevMonthName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName2 = Count;

    pLocHdr->SAbbrevMonthName3 = pLocHdr->SAbbrevMonthName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName3 = Count;

    pLocHdr->SAbbrevMonthName4 = pLocHdr->SAbbrevMonthName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName4 = Count;

    pLocHdr->SAbbrevMonthName5 = pLocHdr->SAbbrevMonthName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName5 = Count;

    pLocHdr->SAbbrevMonthName6 = pLocHdr->SAbbrevMonthName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName6 = Count;

    pLocHdr->SAbbrevMonthName7 = pLocHdr->SAbbrevMonthName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName7 = Count;

    pLocHdr->SAbbrevMonthName8 = pLocHdr->SAbbrevMonthName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName8,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName8 = Count;

    pLocHdr->SAbbrevMonthName9 = pLocHdr->SAbbrevMonthName8 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName9,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName9 = Count;

    pLocHdr->SAbbrevMonthName10 = pLocHdr->SAbbrevMonthName9 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName10,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName10 = Count;

    pLocHdr->SAbbrevMonthName11 = pLocHdr->SAbbrevMonthName10 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName11,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName11 = Count;

    pLocHdr->SAbbrevMonthName12 = pLocHdr->SAbbrevMonthName11 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName12,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName12 = Count;

    pLocHdr->SAbbrevMonthName13 = pLocHdr->SAbbrevMonthName12 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pLocVar->szSAbbrevMonthName13,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pLocCnt->SAbbrevMonthName13 = Count;

    pLocHdr->SEndOfLocale = pLocHdr->SAbbrevMonthName13 + Count;


    //
    //  Read in the font signature information of a locale and store it in
    //  the locale structure.
    //
    //  NOTE: Don't want the null terminator on this string, so tell
    //        the parse routine that there is one more space in the buffer.
    //        This works because the buffer is filled with 0 initially, so
    //        no null terminator is added onto the end of the string.
    //        Instead, it will simply fill in the buffer with the
    //        MAX_FONTSIGNATURE amount of values.
    //
    if (!ParseLine( pszKeyWord,
                    pLocStat->szFontSignature,
                    MAX_FONTSIGNATURE + 1,         // don't want null term
                    FALSE ))
    {
        return (1);
    }



    //
    //  Get szIPosSymPrecedes and szIPosSepBySpace from the szICurrency
    //  value.
    //
    //  NOTE:  All buffers initialized to 0, so no need to zero terminate.
    //
    switch (*(pLocStat->szICurrency))
    {
        case ( '0' ) :
        {
            *(pLocStat->szIPosSymPrecedes) = L'1';
            *(pLocStat->szIPosSepBySpace) = L'0';
            break;
        }

        case ( '1' ) :
        {
            *(pLocStat->szIPosSymPrecedes) = L'0';
            *(pLocStat->szIPosSepBySpace) = L'0';
            break;
        }

        case ( '2' ) :
        {
            *(pLocStat->szIPosSymPrecedes) = L'1';
            *(pLocStat->szIPosSepBySpace) = L'1';
            break;
        }

        case ( '3' ) :
        {
            *(pLocStat->szIPosSymPrecedes) = L'0';
            *(pLocStat->szIPosSepBySpace) = L'1';
            break;
        }

        default :
        {
            printf("Parse Error: Invalid ICURRENCY value.\n");
            return (1);
        }
    }

    //
    //  Get szIPosSignPosn, szINegSignPosn, szINegSymPrecedes, and
    //  szINegSepBySpace from the szINegCurr value.
    //
    //  NOTE:  All buffers initialized to 0, so no need to zero terminate.
    //
    switch (*(pLocStat->szINegCurr))
    {
        case ( '0' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'3';
            *(pLocStat->szINegSignPosn) = L'0';
            *(pLocStat->szINegSymPrecedes) = L'1';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '1' ) :
        {
            switch (*((pLocStat->szINegCurr) + 1))
            {
                case ( 0 ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'3';
                    *(pLocStat->szINegSignPosn) = L'3';
                    *(pLocStat->szINegSymPrecedes) = L'1';
                    *(pLocStat->szINegSepBySpace) = L'0';
                    break;
                }
                case ( '0' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'4';
                    *(pLocStat->szINegSignPosn) = L'4';
                    *(pLocStat->szINegSymPrecedes) = L'0';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                case ( '1' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'2';
                    *(pLocStat->szINegSignPosn) = L'2';
                    *(pLocStat->szINegSymPrecedes) = L'1';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                case ( '2' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'4';
                    *(pLocStat->szINegSignPosn) = L'4';
                    *(pLocStat->szINegSymPrecedes) = L'1';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                case ( '3' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'3';
                    *(pLocStat->szINegSignPosn) = L'3';
                    *(pLocStat->szINegSymPrecedes) = L'0';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                case ( '4' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'3';
                    *(pLocStat->szINegSignPosn) = L'0';
                    *(pLocStat->szINegSymPrecedes) = L'1';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                case ( '5' ) :
                {
                    *(pLocStat->szIPosSignPosn) = L'1';
                    *(pLocStat->szINegSignPosn) = L'0';
                    *(pLocStat->szINegSymPrecedes) = L'0';
                    *(pLocStat->szINegSepBySpace) = L'1';
                    break;
                }
                default :
                {
                    printf("Parse Error: Invalid INEGCURR value.\n");
                    return (1);
                }
            }

            break;
        }

        case ( '2' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'4';
            *(pLocStat->szINegSignPosn) = L'4';
            *(pLocStat->szINegSymPrecedes) = L'1';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '3' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'2';
            *(pLocStat->szINegSignPosn) = L'2';
            *(pLocStat->szINegSymPrecedes) = L'1';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '4' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'1';
            *(pLocStat->szINegSignPosn) = L'0';
            *(pLocStat->szINegSymPrecedes) = L'0';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '5' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'1';
            *(pLocStat->szINegSignPosn) = L'1';
            *(pLocStat->szINegSymPrecedes) = L'0';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '6' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'3';
            *(pLocStat->szINegSignPosn) = L'3';
            *(pLocStat->szINegSymPrecedes) = L'0';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '7' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'4';
            *(pLocStat->szINegSignPosn) = L'4';
            *(pLocStat->szINegSymPrecedes) = L'0';
            *(pLocStat->szINegSepBySpace) = L'0';
            break;
        }

        case ( '8' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'1';
            *(pLocStat->szINegSignPosn) = L'1';
            *(pLocStat->szINegSymPrecedes) = L'0';
            *(pLocStat->szINegSepBySpace) = L'1';
            break;
        }

        case ( '9' ) :
        {
            *(pLocStat->szIPosSignPosn) = L'3';
            *(pLocStat->szINegSignPosn) = L'3';
            *(pLocStat->szINegSymPrecedes) = L'1';
            *(pLocStat->szINegSepBySpace) = L'1';
            break;
        }

        default :
        {
            printf("Parse Error: Invalid INEGCURR value.\n");
            return (1);
        }
    }


    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseLine
//
//  This routine parses one line of the input file.  This routine is only
//  called to parse a line within the LOCALE keyword section.
//  Returns the number of characters written to the buffer (0 means error).
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseLine(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    BOOL fConvert)
{
    int Num = 0;                  // number of strings read in
    BYTE pszKey[MAX];             // keyword - ignored


    //
    //  Get to next line of information.
    //  If no more strings could be read in, return an error.
    //
    if (fscanf(pInputFile, "%s", pszKey) == 0)
    {
        printf("Parse Error: Incomplete LOCALE information.\n");
        return (0);
    }

    //
    //  Read in the rest of the line.
    //
    if (fgets(pszLine, MAX, pInputFile) == NULL)
    {
        *pInfo = 0;
        return (1);
    }

    //
    //  Return the count of characters put in the buffer.
    //
    return (ScanBuffer( pszLine,
                        pInfo,
                        BufSize,
                        pszKey,
                        fConvert ));
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMultiLine
//
//  This routine parses multiple lines of the input file.  This routine is only
//  called to parse a set of lines within the LOCALE keyword section.
//  Returns the number of characters written to the buffer (0 means error).
//  This should only be called to parse multiple lines for ONE locale item.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseMultiLine(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize)
{
    int Num = 0;                  // number of strings read in
    BYTE pszKey[MAX];             // keyword - ignored
    int Count = 0;                // count of characters
    int TmpCt;                    // ScanBuffer return count
    WORD *pInfoPtr;               // tmp ptr to pInfo buffer


    //
    //  Get to next line of information.
    //  If no more strings could be read in, return an error.
    //
    if (fscanf(pInputFile, "%s", pszKey) == 0)
    {
        printf("Parse Error: Incomplete LOCALE information.\n");
        return (0);
    }

    //
    //  Get size parameter.
    //
    if (GetLocaleInfoSize(&Num))
    {
        return (0);
    }

    //
    //  Check for num == 0.
    //
    if (Num == 0)
    {
        *pInfo = 0;
        return (1);
    }

    //
    //  Read in the appropriate number of lines.
    //
    pInfoPtr = pInfo;
    while (Num > 0)
    {
        //
        //  Read in the rest of the line.  If nothing on the current line,
        //  go to the next line and try again.
        //
        if (fgets(pszLine, MAX, pInputFile) != NULL)
        {
            TmpCt = ScanBuffer( pszLine,
                                pInfoPtr,
                                BufSize - Count,
                                pszKey,
                                TRUE );
            Num--;
            pInfoPtr += TmpCt;
            Count += TmpCt;
        }
    }

    //
    //  Return the count of characters put in the buffer.
    //
    return (Count);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMultiLineSize
//
//  This routine parses the IOPTIONALCALENDAR line and the SERARANGES line
//  of the calendar input file.  It stores the value as both a WORD and a
//  string.  It also stores the size of the information read in, including
//  the 2 words for the value and the size.
//
//  Returns the number of characters written to the buffer (0 means error).
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseMultiLineSize(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    WORD *pNum,
    int Flag)
{
    int Num = 0;                  // number of strings read in
    BYTE pszKey[MAX];             // keyword - ignored
    int Count = 0;                // count of characters
    int TmpCt;                    // ScanBuffer return count
    int Value;                    // value for sscanf
    WORD *pInfoPtr;               // tmp ptr to pInfo buffer
    int Incr;                     // increment amount for buffer


    //
    //  Get to next line of information.
    //  If no more strings could be read in, return an error.
    //
    if (fscanf(pInputFile, "%s", pszKey) == 0)
    {
        printf("Parse Error: Incomplete LOCALE information.\n");
        return (0);
    }

    //
    //  Get size parameter.
    //
    if (GetLocaleInfoSize(&Num))
    {
        return (0);
    }

    //
    //  Save the number of ranges.
    //
    *pNum = Num;

    //
    //  Check for num == 0.
    //
    if (Num == 0)
    {
        *pInfo = 0;
        return (1);
    }

    //
    //  Set the increment amount based on the Flag parameter.
    //
    Incr = (Flag == ERA_RANGE_FLAG) ? 4 : 2;

    //
    //  Read in the appropriate number of lines.
    //
    pInfoPtr = pInfo;
    while (Num > 0)
    {
        //
        //  If we're getting the era ranges, then we need to read in the
        //  month and day of the era before we read in the year.
        //
        //  Ordering in Buffer:
        //      Month, Day, Year, Offset, Year String, Era Name String
        //
        if (Flag == ERA_RANGE_FLAG)
        {
            //
            //  Get the Month.
            //
            if (GetLocaleInfoSize(&Value))
            {
                return (0);
            }
            pInfoPtr[0] = (WORD)Value;

            //
            //  Get the Day.
            //
            if (GetLocaleInfoSize(&Value))
            {
                return (0);
            }
            pInfoPtr[1] = (WORD)Value;
        }

        //
        //  Read in the rest of the line.  If nothing on the current line,
        //  go to the next line and try again.
        //
        if (fgets(pszLine, MAX, pInputFile) != NULL)
        {
            TmpCt = ScanBuffer( pszLine,
                                pInfoPtr + Incr,
                                BufSize - Count - Incr,
                                pszKey,
                                TRUE );

            TmpCt += Incr;

            swscanf(pInfoPtr + Incr, L"%d", &Value);
            pInfoPtr[Incr - 2] = (WORD)Value;
            pInfoPtr[Incr - 1] = (WORD)TmpCt;

            Num--;
            pInfoPtr += TmpCt;
            Count += TmpCt;
        }
    }

    //
    //  Return the count of characters put in the buffer.
    //
    return (Count);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocaleInfoSize
//
//  This routine gets the size of the table from the input file.  If the
//  size is not there, then an error is returned.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetLocaleInfoSize(
    int *pSize)
{
    int NumItems;                 // number of items returned from fscanf


    //
    //  Read the size from the input file.
    //
    NumItems = fscanf(pInputFile, "%d", pSize);
    if (NumItems != 1)
    {
        printf("Parse Error: Error reading size value.\n");
        return (1);
    }

    if (*pSize < 0)
    {
        printf("Parse Error: Invalid size value  %d\n", *pSize);
        return (1);
    }

    if (Verbose)
        printf("  SIZE = %d\n", *pSize);

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ScanBuffer
//
//  This routine converts the given ansi buffer to a wide character buffer,
//  removes leading and trailing white space, and then scans it for escape
//  characters.  The final buffer and the number of characters written to
//  the buffer (0 means error) are returned.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ScanBuffer(
    PSZ pszLine,
    WORD *pInfo,
    int BufSize,
    BYTE *pszKey,
    BOOL fConvert)
{
    int Num = 0;                  // number of strings read in
    WORD pwszTemp[MAX];           // first string of information
    WORD *pwszInfoPtr;            // ptr to string of information to store
    int Count = 0;                // count of characters


    //
    //  Convert the ansi buffer to a wide char buffer and skip over any
    //  leading white space.
    //
    if (sscanf(pszLine, "%*[\t ]%255w[^\n]", pwszTemp) == 0)
    {
        //
        //  This should only happen if there is only white space on the line.
        //
        *pInfo = 0;
        return (1);
    }

    //
    //  Remove trailing spaces.
    //
    //  NOTE: Subtract 1 from the end of the string to skip over the
    //  null terminator.  The line feed was already filtered out above.
    //
    pwszInfoPtr = pwszTemp + wcslen(pwszTemp) - 1;
    while ((pwszInfoPtr != pwszTemp) &&
           ((*pwszInfoPtr == L' ') || (*pwszInfoPtr == L'\t')))
    {
        pwszInfoPtr--;
    }
    *(pwszInfoPtr + 1) = 0;

    if (Verbose)
        printf("  %s\t%ws\n", pszKey, pwszTemp);

    //
    //  Buffer should be initialized to zero, so no need to
    //  zero terminate the string.
    //
    pwszInfoPtr = pwszTemp;
    while ((*pwszInfoPtr != L'\n') && (*pwszInfoPtr != 0))
    {
        //
        //  Check output buffer size.
        //
        if (Count >= (BufSize - 1))
        {
            printf("WARNING: String is too long - truncating  %s\n",
                   pszKey);
            break;
        }

        //
        //  Check for escape sequence.
        //
        if (*pwszInfoPtr == L'\\')
        {
            pwszInfoPtr++;

            if ((*pwszInfoPtr == L'x') || (*pwszInfoPtr == L'X'))
            {
                //
                //  Read in hex value.
                //
                //  NOTE:  All hex values MUST be 4 digits long -
                //         character may be ignored or hex values may be
                //         incorrect.
                //
                if (swscanf(pwszInfoPtr + 1, L"%4x", &Num) != 1)
                {
                    printf("Parse Error: No number following \\x for %s.\n", pszKey);
                    return (0);
                }

                //
                //  Check for special character - 0xffff.  Change it to a
                //  null terminator.
                //  This means that there is more than one string for one
                //  LCTYPE information.
                //
                if ((fConvert) && (Num == 0xffff))
                {
                    *pInfo = (WORD)0;
                }
                else
                {
                    *pInfo = (WORD)Num;
                }
                pInfo++;
                pwszInfoPtr += 5;
                Count++;
            }
            else if (*pwszInfoPtr == L'\\')
            {
                //
                //  Want to print out backslash, so do it.
                //
                *pInfo = *pwszInfoPtr;
                pInfo++;
                pwszInfoPtr++;
                Count++;
            }
            else
            {
                //
                //  Escape character not followed by valid character.
                //
                printf("Parse Error: Invalid escape sequence for %s.\n",
                       pszKey);
                return (0);
            }
        }
        else
        {
            //
            //  Simply copy character.  No special casing required.
            //
            *pInfo = *pwszInfoPtr;
            pInfo++;
            pwszInfoPtr++;
            Count++;
        }
    }

    //
    //  Return the count of characters put in the buffer.
    //
    return (Count + 1);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteLocaleInit
//
//  This routine opens the output file for writing and writes the number
//  of locales as the first piece of data to the file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLocaleInit(
    FILE **ppOutputFile,
    int NumLoc,
    int OffLoc)
{
    WORD pDummy[MAX];             // dummy storage
    DWORD dwValue;                // temp storage value


    //
    //  Make sure output file can be opened for writing.
    //
    if ((*ppOutputFile = fopen(LOCALE_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", LOCALE_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", LOCALE_FILE);

    //
    //  Write the number of locales to the file.
    //
    dwValue = (DWORD)NumLoc;
    if (FileWrite( *ppOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Number of Locales" ))
    {
        fclose(*ppOutputFile);
        return (1);
    }

    //
    //  Write zeros in the file to allow the seek to work.
    //
    memset(pDummy, 0, MAX * sizeof(WORD));
    if (FileWrite( *ppOutputFile,
                   pDummy,
                   sizeof(WORD),
                   OffLoc,
                   "Locale File Header" ))
    {
        fclose(*ppOutputFile);
        return (1);
    }

    //
    //  Seek back to the beginning of the locale header.
    //
    if (fseek( *ppOutputFile,
               LOC_CAL_HDR_WORDS * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }


    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteLocaleInfo
//
//  This routine writes the locale id, offset, and locale information to
//  the output file.  It needs to seek ahead to the correct position for the
//  locale information, and then seeks back to the header position.  The
//  file positions are updated to reflect the next offsets.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLocaleInfo(
    DWORD Locale,
    int *pOffHdr,
    int *pOffLoc,
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PLOCALE_HEADER pLocCnt,
    FILE *pOutputFile)
{
    int Size;                     // size of locale information
    int TotalSize = 0;            // total size of the locale information
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting Locale Information for %x...\n", Locale);

    //
    //  Write the locale id and offset to the locale information in
    //  the header area of the output file.
    //
    dwValue = (DWORD)(*pOffLoc);
    if (FileWrite( pOutputFile,
                   &Locale,
                   sizeof(DWORD),
                   1,
                   "Locale ID" ) ||
        FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Locale Info Offset" ))
    {
        return (1);
    }

    //
    //  Seek forward to locale info offset.
    //
    if (fseek( pOutputFile,
               (*pOffLoc) * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Write the locale information to the output file.
    //        Header Info
    //        Static Length Info
    //        Variable Length Info
    //
    TotalSize = Size = sizeof(LOCALE_HEADER) / sizeof(WORD);
    if (FileWrite( pOutputFile,
                   pLocHdr,
                   sizeof(WORD),
                   Size,
                   "Locale Header" ))
    {
        return (1);
    }

    TotalSize += (Size = sizeof(LOCALE_STATIC) / sizeof(WORD));
    if (FileWrite( pOutputFile,
                   pLocStat,
                   sizeof(WORD),
                   Size,
                   "Locale Static Info" ))
    {
        return (1);
    }

    if (WriteVariableLength( pLocCnt,
                             pLocVar,
                             &TotalSize,
                             pOutputFile ))
    {
        return (1);
    }

    //
    //  Set the offsets to their new values.
    //
    Size = *pOffLoc;
    (*pOffHdr) += LOCALE_HDR_WORDS;
    (*pOffLoc) += TotalSize;

    //
    //  Make sure the size is not wrapping - can't be greater than
    //  a DWORD.
    //
    if (Size > *pOffLoc)
    {
        printf("Size Error: Offset is greater than a DWORD for locale %x.\n", Locale);
        return (1);
    }

    //
    //  Seek back to the header offset.
    //
    if (fseek( pOutputFile,
               (*pOffHdr) * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteVariableLength
//
//  This routine writes the variable length locale information to the output
//  file.  It adds on to the total size of the locale information as it adds
//  the variable length information.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteVariableLength(
    PLOCALE_HEADER pLocCnt,
    PLOCALE_VARIABLE pLocVar,
    int *pTotalSize,
    FILE *pOutputFile)
{
    int Size;                     // size of string


    *pTotalSize += (Size = pLocCnt->SLanguage);
    if (FileWrite( pOutputFile,
                   pLocVar->szSLanguage,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevLang);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevLang,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevLangISO);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevLangISO,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SNativeLang);
    if (FileWrite( pOutputFile,
                   pLocVar->szSNativeLang,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SCountry);
    if (FileWrite( pOutputFile,
                   pLocVar->szSCountry,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevCtry);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevCtry,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevCtryISO);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevCtryISO,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SNativeCtry);
    if (FileWrite( pOutputFile,
                   pLocVar->szSNativeCtry,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SList);
    if (FileWrite( pOutputFile,
                   pLocVar->szSList,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDecimal);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDecimal,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SThousand);
    if (FileWrite( pOutputFile,
                   pLocVar->szSThousand,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SGrouping);
    if (FileWrite( pOutputFile,
                   pLocVar->szSGrouping,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SNativeDigits);
    if (FileWrite( pOutputFile,
                   pLocVar->szSNativeDigits,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SCurrency);
    if (FileWrite( pOutputFile,
                   pLocVar->szSCurrency,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SIntlSymbol);
    if (FileWrite( pOutputFile,
                   pLocVar->szSIntlSymbol,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SEngCurrName);
    if (FileWrite( pOutputFile,
                   pLocVar->szSEngCurrName,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SNativeCurrName);
    if (FileWrite( pOutputFile,
                   pLocVar->szSNativeCurrName,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonDecSep);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonDecSep,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonThousSep);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonThousSep,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonGrouping);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonGrouping,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SPositiveSign);
    if (FileWrite( pOutputFile,
                   pLocVar->szSPositiveSign,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SNegativeSign);
    if (FileWrite( pOutputFile,
                   pLocVar->szSNegativeSign,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->STimeFormat);
    if (FileWrite( pOutputFile,
                   pLocVar->szSTimeFormat,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->STime);
    if (FileWrite( pOutputFile,
                   pLocVar->szSTime,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->S1159);
    if (FileWrite( pOutputFile,
                   pLocVar->szS1159,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->S2359);
    if (FileWrite( pOutputFile,
                   pLocVar->szS2359,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SShortDate);
    if (FileWrite( pOutputFile,
                   pLocVar->szSShortDate,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDate);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDate,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SYearMonth);
    if (FileWrite( pOutputFile,
                   pLocVar->szSYearMonth,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SLongDate);
    if (FileWrite( pOutputFile,
                   pLocVar->szSLongDate,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->IOptionalCalendar);
    if (FileWrite( pOutputFile,
                   pLocVar->szIOptionalCalendar,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName1);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName1,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName2);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName2,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName3);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName3,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName4);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName4,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName5);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName5,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName6);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName6,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SDayName7);
    if (FileWrite( pOutputFile,
                   pLocVar->szSDayName7,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName1);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName1,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName2);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName2,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName3);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName3,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName4);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName4,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName5);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName5,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName6);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName6,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevDayName7);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevDayName7,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName1);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName1,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName2);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName2,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName3);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName3,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName4);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName4,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName5);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName5,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName6);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName6,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName7);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName7,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName8);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName8,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName9);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName9,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName10);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName10,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName11);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName11,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName12);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName12,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SMonthName13);
    if (FileWrite( pOutputFile,
                   pLocVar->szSMonthName13,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName1);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName1,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName2);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName2,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName3);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName3,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName4);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName4,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName5);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName5,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName6);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName6,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName7);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName7,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName8);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName8,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName9);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName9,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName10);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName10,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName11);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName11,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName12);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName12,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pLocCnt->SAbbrevMonthName13);
    if (FileWrite( pOutputFile,
                   pLocVar->szSAbbrevMonthName13,
                   sizeof(WORD),
                   Size,
                   "Locale Variable Info" ))
    {
        return (1);
    }


    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseWriteCalendar
//
//  This routine parses the input file for the calendar specific tables, and
//  then writes the data to the output file.  This routine is only entered
//  when the CALENDAR keyword is found.  The parsing continues until the
//  ENDCALENDAR keyword is found.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseWriteCalendar(
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PSZ pszKeyWord,
    FILE *pOutputFile,
    int OffHdr)
{
    int Num;                      // number of calendars
    int OffCal;                   // file offset to calendar info
    int OffCalBegin;              // file offset to beginning of calendar info
    DWORD CalId;                  // calendar id
    CALENDAR_HEADER CalCnt;       // calendar character counts


    //
    //  Get size parameter.
    //
    if (GetSize(&Num))
        return (1);

    //
    //  Set up initial file pointer offsets.
    //
    //      OffCal = (Number of calendars * header entry size)
    //
    OffCalBegin = OffHdr;
    OffCal = Num * CALENDAR_HDR_WORDS;

    //
    //  Initialize the output file and write the number of calendars to
    //  the file.  Also, in order to allow for the seek, write zeros
    //  in the file up to the first calendar field.
    //
    if (WriteCalendarInit( pOutputFile,
                           Num,
                           OffHdr ))
    {
        return (1);
    }

    //
    //  Parse all of the calendars one by one.  Write each one to the file
    //  separately to conserve memory.
    //
    for (; Num > 0; Num--)
    {
        //
        //  Initialize all Calendar structures each time.
        //
        memset(pCalHdr, 0, sizeof(CALENDAR_HEADER));
        memset(pCalVar, 0, sizeof(CALENDAR_VARIABLE));
        memset(&CalCnt, 0, sizeof(CALENDAR_HEADER));

        //
        //  Get the BEGINCALENDAR keyword and calendar id.
        //
        if (fscanf( pInputFile,
                    "%s %ld ;%*[^\n]",
                    pszKeyWord,
                    &CalId ) == 2)
        {
            if (_strcmpi(pszKeyWord, "BEGINCALENDAR") == 0)
            {
                if (Verbose)
                    printf("\n\nFound BEGINCALENDAR keyword, CalendarID = %d\n\n",
                           CalId);
            }
            else
            {
                printf("Parse Error: Error reading BEGINCALENDAR and Calendar ID.\n");
                return (1);
            }
        }
        else
        {
            printf("Parse Error: Invalid Instruction '%s'.\n", pszKeyWord);
            printf("             Expecting BEGINCALENDAR keyword and Calendar ID.\n");
            return (1);
        }

        //
        //  Parse the calendar information.
        //
        if (ParseCalendarInfo( pCalHdr,
                               pCalVar,
                               &CalCnt,
                               pszKeyWord ))
        {
            printf("Parse Error: Calendar == %d.\n", CalId);
            return (1);
        }

        //
        //  Write the calendar id, offset, and calendar information to
        //  the output file.
        //
        if (WriteCalendarInfo( CalId,
                               &OffHdr,
                               &OffCal,
                               OffCalBegin,
                               pCalHdr,
                               pCalVar,
                               &CalCnt,
                               pOutputFile ))
        {
            printf("Write Error: Calendar == %d.\n", CalId);
            return (1);
        }
    }

    //
    //  Look for ENDCALENDAR keyword.
    //
    if (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "ENDCALENDAR") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDCALENDAR keyword.\n");

            //
            //  Return success.
            //
            return (0);
        }
    }

    //
    //  If this point is reached, then the ENDCALENDAR keyword was not
    //  found.  Return failure.
    //
    printf("Parse Error: Expecting ENDCALENDAR keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseCalendarInfo
//
//  This routine parses the calendar information from the input file.  If an
//  error is encountered, a 1 is returned.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseCalendarInfo(
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PCALENDAR_HEADER pCalCnt,
    PSZ pszKeyWord)
{
    int Count;               // number of characters written
    int Value;               // hex value returned


    //
    //  Read in the calendar id and store it in the calendar structure.
    //
    pCalHdr->SCalendar = sizeof(CALENDAR_HEADER) / sizeof(WORD);
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSCalendar,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SCalendar = Count;


    //
    //  Read in the two digit year max and store it in the calendar
    //  structure.
    //
    pCalHdr->STwoDigitYearMax = pCalHdr->SCalendar + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSTwoDigitYearMax,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->STwoDigitYearMax = Count;


    //
    //  Read in the range offsets and store them in the calendar structure.
    //
    pCalHdr->SEraRanges = pCalHdr->STwoDigitYearMax + Count;
    if (!(Count = ParseMultiLineSize( pszKeyWord,
                                      pCalVar->szSEraRanges,
                                      MAX,
                                      &(pCalHdr->NumRanges),
                                      ERA_RANGE_FLAG )))
    {
        return (1);
    }
    pCalCnt->SEraRanges = Count;
    pCalCnt->NumRanges = pCalHdr->NumRanges;


    //
    //  Read in the short date information and store it in the
    //  calendar structure.
    //
    pCalHdr->SShortDate = pCalHdr->SEraRanges + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pCalVar->szSShortDate,
                                  MAX )))
    {
        return (1);
    }
    pCalCnt->SShortDate = Count;


    //
    //  Read in the year month information and store it in the
    //  calendar structure.
    //
    pCalHdr->SYearMonth = pCalHdr->SShortDate + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pCalVar->szSYearMonth,
                                  MAX )))
    {
        return (1);
    }
    pCalCnt->SYearMonth = Count;


    //
    //  Read in the long date information and store it in the
    //  calendar structure.
    //
    pCalHdr->SLongDate = pCalHdr->SYearMonth + Count;
    if (!(Count = ParseMultiLine( pszKeyWord,
                                  pCalVar->szSLongDate,
                                  MAX )))
    {
        return (1);
    }
    pCalCnt->SLongDate = Count;

    pCalHdr->SDayName1 = pCalHdr->SLongDate + Count;


    //
    //  See if any day or month names exist.
    //
    if (fscanf(pInputFile, "%s", pszKeyWord) != 1)
    {
        printf("Parse Error: Error reading ERA keyword.\n");
        return (1);
    }
    if (GetLocaleInfoSize(&Value))
        return (1);

    pCalHdr->IfNames = (WORD)Value;
    pCalCnt->IfNames = (WORD)Value;
    if (!Value)
    {
        pCalHdr->SCalendar         -=  CAL_NAME_HDR_SIZE;
        pCalHdr->STwoDigitYearMax  -=  CAL_NAME_HDR_SIZE;
        pCalHdr->SEraRanges        -=  CAL_NAME_HDR_SIZE;
        pCalHdr->SShortDate        -=  CAL_NAME_HDR_SIZE;
        pCalHdr->SYearMonth        -=  CAL_NAME_HDR_SIZE;
        pCalHdr->SLongDate         -=  CAL_NAME_HDR_SIZE;
        pCalHdr->SDayName1         -=  CAL_NAME_HDR_SIZE;

        //
        //  Return success.  Don't read any more values for this
        //  calendar.
        //
        return (0);
    }


    //
    //  Read in the day information and store it in the
    //  calendar structure.
    //
    pCalHdr->SDayName1 = pCalHdr->SLongDate + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName1 = Count;

    pCalHdr->SDayName2 = pCalHdr->SDayName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName2 = Count;

    pCalHdr->SDayName3 = pCalHdr->SDayName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName3 = Count;

    pCalHdr->SDayName4 = pCalHdr->SDayName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName4 = Count;

    pCalHdr->SDayName5 = pCalHdr->SDayName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName5 = Count;

    pCalHdr->SDayName6 = pCalHdr->SDayName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName6 = Count;

    pCalHdr->SDayName7 = pCalHdr->SDayName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSDayName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SDayName7 = Count;

    pCalHdr->SAbbrevDayName1 = pCalHdr->SDayName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName1 = Count;

    pCalHdr->SAbbrevDayName2 = pCalHdr->SAbbrevDayName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName2 = Count;

    pCalHdr->SAbbrevDayName3 = pCalHdr->SAbbrevDayName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName3 = Count;

    pCalHdr->SAbbrevDayName4 = pCalHdr->SAbbrevDayName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName4 = Count;

    pCalHdr->SAbbrevDayName5 = pCalHdr->SAbbrevDayName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName5 = Count;

    pCalHdr->SAbbrevDayName6 = pCalHdr->SAbbrevDayName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName6 = Count;

    pCalHdr->SAbbrevDayName7 = pCalHdr->SAbbrevDayName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevDayName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevDayName7 = Count;


    //
    //  Read in the month information of a locale and store it in the
    //  locale structure.
    //
    pCalHdr->SMonthName1 = pCalHdr->SAbbrevDayName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName1 = Count;

    pCalHdr->SMonthName2 = pCalHdr->SMonthName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName2 = Count;

    pCalHdr->SMonthName3 = pCalHdr->SMonthName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName3 = Count;

    pCalHdr->SMonthName4 = pCalHdr->SMonthName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName4 = Count;

    pCalHdr->SMonthName5 = pCalHdr->SMonthName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName5 = Count;

    pCalHdr->SMonthName6 = pCalHdr->SMonthName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName6 = Count;

    pCalHdr->SMonthName7 = pCalHdr->SMonthName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName7 = Count;

    pCalHdr->SMonthName8 = pCalHdr->SMonthName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName8,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName8 = Count;

    pCalHdr->SMonthName9 = pCalHdr->SMonthName8 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName9,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName9 = Count;

    pCalHdr->SMonthName10 = pCalHdr->SMonthName9 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName10,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName10 = Count;

    pCalHdr->SMonthName11 = pCalHdr->SMonthName10 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName11,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName11 = Count;

    pCalHdr->SMonthName12 = pCalHdr->SMonthName11 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName12,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName12 = Count;

    pCalHdr->SMonthName13 = pCalHdr->SMonthName12 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSMonthName13,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SMonthName13 = Count;

    pCalHdr->SAbbrevMonthName1 = pCalHdr->SMonthName13 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName1,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName1 = Count;

    pCalHdr->SAbbrevMonthName2 = pCalHdr->SAbbrevMonthName1 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName2,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName2 = Count;

    pCalHdr->SAbbrevMonthName3 = pCalHdr->SAbbrevMonthName2 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName3,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName3 = Count;

    pCalHdr->SAbbrevMonthName4 = pCalHdr->SAbbrevMonthName3 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName4,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName4 = Count;

    pCalHdr->SAbbrevMonthName5 = pCalHdr->SAbbrevMonthName4 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName5,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName5 = Count;

    pCalHdr->SAbbrevMonthName6 = pCalHdr->SAbbrevMonthName5 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName6,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName6 = Count;

    pCalHdr->SAbbrevMonthName7 = pCalHdr->SAbbrevMonthName6 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName7,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName7 = Count;

    pCalHdr->SAbbrevMonthName8 = pCalHdr->SAbbrevMonthName7 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName8,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName8 = Count;

    pCalHdr->SAbbrevMonthName9 = pCalHdr->SAbbrevMonthName8 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName9,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName9 = Count;

    pCalHdr->SAbbrevMonthName10 = pCalHdr->SAbbrevMonthName9 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName10,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName10 = Count;

    pCalHdr->SAbbrevMonthName11 = pCalHdr->SAbbrevMonthName10 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName11,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName11 = Count;

    pCalHdr->SAbbrevMonthName12 = pCalHdr->SAbbrevMonthName11 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName12,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName12 = Count;

    pCalHdr->SAbbrevMonthName13 = pCalHdr->SAbbrevMonthName12 + Count;
    if (!(Count = ParseLine( pszKeyWord,
                             pCalVar->szSAbbrevMonthName13,
                             MAX,
                             TRUE )))
    {
        return (1);
    }
    pCalCnt->SAbbrevMonthName13 = Count;

    pCalHdr->SEndOfCalendar = pCalHdr->SAbbrevMonthName13 + Count;


    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCalendarInit
//
//  This routine writes the number of calendars and the offset to the
//  calendar information in the data file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCalendarInit(
    FILE *pOutputFile,
    int NumCal,
    int OffCalHdr)
{
    WORD pDummy[MAX];             // dummy storage
    DWORD dwValue;                // temp storage value


    //
    //  Write the number of calendars to the file.
    //
    //
    //  Seek to NumCalendar offset.
    //
    if (fseek( pOutputFile,
               LOC_NUM_CAL_WORDS * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Write the number of calendars to the file.
    //
    dwValue = (DWORD)NumCal;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Number of Calendars" ))
    {
        return (1);
    }

    //
    //  Write the offset of calendar info to the file.
    //
    dwValue = (DWORD)OffCalHdr;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Offset of Calendar Info" ))
    {
        return (1);
    }

    //
    //  Seek to calendar header.
    //
    if (fseek( pOutputFile,
               OffCalHdr * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Write zeros in the file to allow the seek to work.
    //
    memset(pDummy, 0, MAX * sizeof(WORD));
    if (FileWrite( pOutputFile,
                   pDummy,
                   sizeof(WORD),
                   NumCal * CALENDAR_HDR_WORDS,
                   "Calendar Header" ))
    {
        return (1);
    }

    //
    //  Seek back to the beginning of the calendar header.
    //
    if (fseek( pOutputFile,
               OffCalHdr * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCalendarInfo
//
//  This routine writes the calendar id, offset, and calendar information to
//  the output file.  It needs to seek ahead to the correct position for the
//  calendar information, and then seeks back to the header position.  The
//  file positions are updated to reflect the next offsets.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCalendarInfo(
    DWORD Calendar,
    int *pOffHdr,
    int *pOffCal,
    int OffCalBegin,
    PCALENDAR_HEADER pCalHdr,
    PCALENDAR_VARIABLE pCalVar,
    PCALENDAR_HEADER pCalCnt,
    FILE *pOutputFile)
{
    int Size;                     // size of locale information
    int TotalSize = 0;            // total size of the locale information
    int NameSize;                 // size of name space to subtract
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Calendar Information for %x...\n", Calendar);

    //
    //  Write the calendar id and offset to the calendar information in
    //  the calendar header area of the output file.
    //
    wValue = (WORD)Calendar;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Calendar ID" ))
    {
        return (1);
    }

    wValue = (WORD)(*pOffCal);
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Calendar Info Offset" ))
    {
        return (1);
    }

    //
    //  Seek forward to calendar info offset.
    //
    if (fseek( pOutputFile,
               (OffCalBegin + (*pOffCal)) * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Write the calendar information to the output file.
    //        Header Info
    //        Variable Length Info
    //
    NameSize = (pCalHdr->IfNames) ? 0 : CAL_NAME_HDR_SIZE;
    TotalSize = Size = (sizeof(CALENDAR_HEADER) / sizeof(WORD)) - NameSize;
    if (FileWrite( pOutputFile,
                   pCalHdr,
                   sizeof(WORD),
                   Size,
                   "Calendar Header" ))
    {
        return (1);
    }

    if (WriteCalendarVariableLength( pCalCnt,
                                     pCalVar,
                                     &TotalSize,
                                     pOutputFile ))
    {
        return (1);
    }

    //
    //  Set the offsets to their new values.
    //
    (*pOffHdr) += CALENDAR_HDR_WORDS;
    (*pOffCal) += TotalSize;

    //
    //  Make sure the size is not wrapping - can't be greater than
    //  a DWORD.
    //
    if (*pOffCal > 0xffff)
    {
        printf("Size Error: Offset is greater than a WORD for calendar %d.\n", Calendar);
        return (1);
    }


    //
    //  Seek back to the calendar header offset.
    //
    if (fseek( pOutputFile,
               (*pOffHdr) * sizeof(WORD),
               0 ))
    {
        printf("Seek Error: Can't seek in file %s.\n", LOCALE_FILE);
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCalendarVariableLength
//
//  This routine writes the variable length calendar information to the output
//  file.  It adds on to the total size of the calendar information as it adds
//  the variable length information.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCalendarVariableLength(
    PCALENDAR_HEADER pCalCnt,
    PCALENDAR_VARIABLE pCalVar,
    int *pTotalSize,
    FILE *pOutputFile)
{
    int Size;                     // size of string


    *pTotalSize += (Size = pCalCnt->SCalendar);
    if (FileWrite( pOutputFile,
                   pCalVar->szSCalendar,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->STwoDigitYearMax);
    if (FileWrite( pOutputFile,
                   pCalVar->szSTwoDigitYearMax,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SEraRanges);
    if (FileWrite( pOutputFile,
                   pCalVar->szSEraRanges,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SShortDate);
    if (FileWrite( pOutputFile,
                   pCalVar->szSShortDate,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SYearMonth);
    if (FileWrite( pOutputFile,
                   pCalVar->szSYearMonth,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SLongDate);
    if (FileWrite( pOutputFile,
                   pCalVar->szSLongDate,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }


    //
    //  See if any of the day or month names exist.
    //
    if (pCalCnt->IfNames == 0)
    {
        //
        //  Return success.  Don't write any of the day or month names.
        //
        return (0);
    }


    *pTotalSize += (Size = pCalCnt->SDayName1);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName1,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName2);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName2,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName3);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName3,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName4);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName4,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName5);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName5,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName6);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName6,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SDayName7);
    if (FileWrite( pOutputFile,
                   pCalVar->szSDayName7,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName1);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName1,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName2);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName2,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName3);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName3,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName4);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName4,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName5);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName5,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName6);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName6,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevDayName7);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevDayName7,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName1);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName1,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName2);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName2,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName3);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName3,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName4);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName4,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName5);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName5,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName6);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName6,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName7);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName7,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName8);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName8,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName9);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName9,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName10);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName10,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName11);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName11,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName12);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName12,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SMonthName13);
    if (FileWrite( pOutputFile,
                   pCalVar->szSMonthName13,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName1);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName1,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName2);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName2,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName3);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName3,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName4);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName4,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName5);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName5,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName6);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName6,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName7);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName7,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName8);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName8,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName9);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName9,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName10);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName10,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName11);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName11,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName12);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName12,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }

    *pTotalSize += (Size = pCalCnt->SAbbrevMonthName13);
    if (FileWrite( pOutputFile,
                   pCalVar->szSAbbrevMonthName13,
                   sizeof(WORD),
                   Size,
                   "Calendar Variable Info" ))
    {
        return (1);
    }


    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ConvertUnicodeToWord
//
//  This routine converts a Unicode string to a WORD value.
//
//  08-21-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define ISDIGIT(c)      ((c) >= L'0' && (c) <= L'9')

int ConvertUnicodeToWord(
    WORD *pString,
    WORD *pValue)
{
    UINT Val = 0;

    while (*pString)
    {
        if (ISDIGIT(*pString))
        {
            Val *= 10;
            Val += *pString - L'0';
            pString++;
        }
        else
        {
            //
            //  Error.
            //
            return (0);
        }
    }

    if (Val > 0xffff)
    {
        //
        //  Code page cannot be greater than a WORD value.
        //
        return (0);
    }

    *pValue = (WORD)Val;

    return (1);
}
