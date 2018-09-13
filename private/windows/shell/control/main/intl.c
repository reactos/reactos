/** FILE: intl.c *********** Module Header ********************************
 *
 *  Control panel applet for International configuration.  This file holds
 *  everything to do with the "International" dialog box in the Control
 *  Panel.
 *
 *  International Scheme --
 *     This module allows the modification of the International section of
 *     the WIN.INI file, it allows either to select one among the predefined
 *     countries or to build a new section. By magic stuff it reacts
 *     automatically to any number of countries declared in the resource
 *     file, no compilation is needed to add a new country.
 *
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

// Setup api
#include "prsinf.h"


//==========================================================================
//                            Local Definitions
//==========================================================================

//  Linked-list structure used at init time
typedef struct _nlslcid
{
    struct _nlslcid *pNext;
    LCID   lcid;                    //  NLS LCID value from registry
    int    iCountry;                //  NLS Country Code for this LCID
    BOOL   bMulti;                  //  Multiple Languages per Country?
} NLSLCID;


#if 1
#define STATIC
#else
#define STATIC static
#endif

#define KEYBOARDHARDWARECHANGABLE 1

/*
 *  Default settings for integer values of INTLSTRUCT.
 */
typedef struct _intl_istrings
{
    TCHAR  iCountry[6];     /* Country code (phone ID) */
    TCHAR  iDate[2];        /* Date mode (0:MDY, 1:DMY, 2:YMD) */
    TCHAR  iTime[2];        /* Time mode (0: 12 hour clock, 1: 24 ) */
    TCHAR  iTLZero[2];      /* Leading zeros for hour (0: no, 1: yes) */
    TCHAR  iCurFmt[2];      /* Currency mode(0: prefix, no separation
                                          1: suffix, no separation
                                          2: prefix, 1 char separation
                                          3: suffix, 1 char separation) */

    TCHAR  iCurDec[2];      /* Currency Decimal Place */
    TCHAR  iNegCur[2];      /* Negative currency pattern:
                              ($1.23), -$1.23, $-1.23, $1.23-, etc. */
    TCHAR  iLzero[2];       /* Leading zeros of decimal (0: no, 1: yes) */
    TCHAR  iDigits[2];      /* Significant decimal digits */
    TCHAR  iMeasure[2];     /* Metric 0; British 1 */
    TCHAR  lcid[10];        /* NT NLS Language/Locale Identifier */
    TCHAR  iTimeMarker[2];  /* Time marker position (0: suffix, 1: prefix) */
    TCHAR  iNegNumber[2];   /* Negative number pattern:
                              (1.1), -1.1, - 1.1, 1.1-, 1.1 -   */
} INTL_ISTRINGS;
typedef INTL_ISTRINGS *LPINTL_ISTRINGS;

//==========================================================================
//                            External Declarations
//==========================================================================

//==========================================================================
//                            Local Data Declarations
//==========================================================================

LPTSTR pszCountry;       /* pointer to this string used by ReadNext () */
LPTSTR pLocales;         // Ptr to Locales option info from SETUP api call
LPTSTR pLayouts;         // Ptr to Locales option info from SETUP api call

TCHAR    szPct02D[]  = TEXT("%02d");
TCHAR    szPctD[]    = TEXT("%d");
TCHAR    szQuote[]   = TEXT("\"");
TCHAR    szColon[]   = TEXT(":");
TCHAR    szDotDLL[]  = TEXT(".DLL");

TCHAR    szInstalledLocales[] = TEXT("System\\CurrentControlSet\\Control\\NLS\\Language");

TCHAR    szInstalledLayouts[] = TEXT("System\\CurrentControlSet\\Control\\Keyboard Layout");

TCHAR    szRegCPIntl[]   = TEXT("Control Panel\\International");
TCHAR    szRegLayerDef[] = TEXT("Keyboard Layout");
TCHAR    szRegLocale[]   = TEXT("Locale");
TCHAR    szRegDefault[]  = TEXT("Default");
TCHAR    szRegActive[]   = TEXT("Active");


NLSLCID *pFirstNlsLcid = NULL;   /* ptr to linked list for country listbox info */


#ifdef JAPAN
// On the safe side..   v-hirot
INTLSTRUCT Current    = { TEXT(""),
                          81,
                          2,
                          1,
                          0,
                          0,
                          0,
                          1,
                          1,
                          0,
                          0,
                          TEXT(""),
                          TEXT(""),
                          TEXT("\\"),
                          TEXT(","),
                          TEXT("."),
                          TEXT("/"),
                          TEXT(":"),
                          TEXT(","),
                          TEXT("yyyy”N MŒŽ d“ú WW"),
                          TEXT("yy/MM/dd"),
                          TEXT("JPN"),
                          1,
                          1,
                          1,
                          2,
                          0x0411,
                          TEXT("hh:mm:ss tt"),
                          0,
                          1,
                          TEXT(","),
                          TEXT(".")
                        };

/* default used if no WIN.INI or no [intl] section inside WIN.INI */
INTLSTRUCT IntlDef    = { TEXT("Other Country"),
                          81,
                          2,
                          1,
                          0,
                          0,
                          0,
                          1,
                          1,
                          0,
                          0,
                          TEXT(""),
                          TEXT(""),
                          TEXT("\\"),
                          TEXT(","),
                          TEXT("."),
                          TEXT("/"),
                          TEXT(":"),
                          TEXT(","),
                          TEXT("yyyy”N MŒŽ d“ú WW"),
                          TEXT("yy/MM/dd"),
                          TEXT("JPN"),
                          1,
                          1,
                          1,
                          2,
                          0x0411,
                          TEXT("hh:mm:ss tt"),
                          0,
                          1,
                          TEXT(","),
                          TEXT(".")
                        };

INTL_ISTRINGS DefIStr = { TEXT("81"),
                          TEXT("2"),
                          TEXT("1"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("1"),
                          TEXT("1"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("0x0411"),
                          TEXT("0"),
                          TEXT("1")
                        };

#else
INTLSTRUCT Current    = { TEXT(""),
                          1,
                          0,
                          0,
                          0,
                          0,
                          2,
                          0,
                          1,
                          2,
                          1,
                          TEXT("AM"),
                          TEXT("PM"),
                          TEXT("$"),
                          TEXT(","),
                          TEXT("."),
                          TEXT("/"),
                          TEXT(":"),
                          TEXT(","),
                          TEXT("dddd, MMMM dd, yyyy"),
                          TEXT("M/d/yy"),
                          TEXT("USA"),
                          1,
                          0,
                          1,
                          0,
                          0x0409,
                          TEXT("hh:mm:ss tt"),
                          0,
                          1,
                          TEXT(","),
                          TEXT(".")
                        };

/* default used if no WIN.INI or no [intl] section inside WIN.INI */
INTLSTRUCT IntlDef    = { TEXT("Other Country"),
                          1,
                          0,
                          0,
                          0,
                          0,
                          2,
                          0,
                          1,
                          2,
                          1,
                          TEXT("AM"),
                          TEXT("PM"),
                          TEXT("$"),
                          TEXT(","),
                          TEXT("."),
                          TEXT("/"),
                          TEXT(":"),
                          TEXT(","),
                          TEXT("dddd, MMMM dd, yyyy"),
                          TEXT("M/d/yy"),
                          TEXT("USA"),
                          1,
                          0,
                          1,
                          0,
                          0x0409,
                          TEXT("hh:mm:ss tt"),
                          0,
                          1,
                          TEXT(","),
                          TEXT(".")
                        };

INTL_ISTRINGS DefIStr = { TEXT("1"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("0"),
                          TEXT("2"),
                          TEXT("0"),
                          TEXT("1"),
                          TEXT("2"),
                          TEXT("1"),
                          TEXT("0x0409"),
                          TEXT("0"),
                          TEXT("1")
                        };
#endif


int    nCurCountry;         /* current Country CB selection */
int    nCurLang;            /* current Language CB selection */
int    nCurKbd;             /* current Keyboard CB selection */


TCHAR *pszNegNumPat[NUM_NEGNUM_PAT] = { TEXT("(%s)"),  // the positions of these can't change
                                        TEXT("-%s"),
                                        TEXT("- %s"),
                                        TEXT("%s-"),
                                        TEXT("%s -")
                                      };

TCHAR *pszCurPat[NUM_CUR_PAT]       = { TEXT("%s%s"),
                                        TEXT("%s %s")
                                      };

TCHAR *pszNegCurPat[NUM_NEG_PAT]    = { TEXT("(%s%s)"),  // the positions of these can't change
                                        TEXT("-%s%s"),
                                        TEXT("%s-%s"),
                                        TEXT("%s%s-"),
                                        TEXT("(%s%s)"),
                                        TEXT("-%s%s"),
                                        TEXT("%s-%s"),
                                        TEXT("%s%s-"),
                                        TEXT("-%s %s"),
                                        TEXT("-%s %s"),
                                        TEXT("%s %s-"),
                                        TEXT("%s %s-"),
                                        TEXT("%s -%s"),
                                        TEXT("%s- %s"),
                                        TEXT("(%s %s)"),
                                        TEXT("(%s %s)")
                                      };

TCHAR *pszSymPlacement[]            = { TEXT("%s1"),
                                        TEXT("1%s"),
                                        TEXT("%s 1"),
                                        TEXT("1 %s")
                                      };

WORD wFontHeight;

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
// for new emperor's era
TCHAR NewEra[8];
WORD NewYear = 9999;
WORD NewMonth = 0;
WORD NewDay = 0;
#endif

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

BOOL
InvokeSetup(
    HWND hDlg,
    LPTSTR pszInfFile,
    LPTSTR pszOption);

BOOL
CheckOptionInstall(
    LPTSTR pszRegKey,
    LPTSTR pszOption);


//==========================================================================
//                                Functions
//==========================================================================

// *** private functions ***

/*
 *  This helps parse a date string.  It returns the number of characters
 *  in the current token or'ed with a token identifier.
 */
STATIC WORD GetLDFToken(
    LPTSTR pString,
    LPTSTR pBuf)
{
    TCHAR  cTest;
    WORD   retval, wLength;
    LPTSTR pBase = pString, pBufBase;

    /* check the first letter of the token;
     * by default this will be a separator
     */
    switch (cTest = *pString)
    {
      case TEXT('d'):
        retval = LDF_DAY;
        break;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
      case TEXT('W'):
        retval = LDF_JaDAY;
        break;
#endif

      case TEXT('M'):
        retval = LDF_MONTH;
        break;

      case TEXT('y'):
        retval = LDF_YEAR;
        break;

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
      case TEXT('G'):
      case TEXT('n'):
        retval = LDF_JaYEAR;
        for (wLength = 0; (*pString == TEXT('G') || *pString == TEXT('n')) && wLength < 4;
                wLength++, pBuf++, pString++)
            *pBuf = *pString;
        goto OutOfHere;
        break;
#endif

      case TEXT('\0'):
        return (0);
        break;

      case TEXT('\''):
        ++pString;

      /* Fall through */

      default:
        retval = LDF_SEP;
        /* This is a separator; copy to the next unescaped "'" if we started
         * with a "'", otherwise to the next unescaped token marker
         */
        pBufBase = pBuf;
        while (1)
        {
            switch (*pString)
            {
              case TEXT('d'):
              case TEXT('M'):
              case TEXT('y'):
                if (cTest == TEXT('\''))
                    goto DoDefault;

              case TEXT('\0'):
#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG DATE
              case TEXT('n'):
              case TEXT('G'):
              case TEXT('W'):
#endif
                goto EndOfSep;

              case TEXT('\''):
                ++pString;
                if (cTest == TEXT('\'') && *pString != TEXT('\''))
#ifdef JAPAN
                    goto OutOfHere;
#else
                    goto EndOfSep;
#endif

                /* Fall through */

              default:
DoDefault:
                 *pBuf++ = *pString++;
            }
        }
EndOfSep:
        if (*(pBuf = CharPrev (pBufBase, pBuf)) != TEXT(' '))
            ++pBuf;
        goto OutOfHere;
    }

    /* This is not a separator; see how long the thing is (up to
     * 4 characters)
     */
    wLength = 0;
    do
    {
        ++wLength;
        *pBuf++ = *pString++;
    } while (*pString == cTest && wLength < 4) ;

OutOfHere:
    *pBuf = TEXT('\0');
    return (retval + (WORD) (pString - pBase));
}


/*
 *  This fills the INTL structure with date information.
 */
STATIC void TranslateShortDate(
    PINTL pI)
{
    LDF   LongDF;
    WORD  wElement;
    short i;

    ParseLDF (pI->sShortDate, &LongDF);
    pI->iDate = (int) ((LongDF.Order[0] & 0xF0) - LDF_MONTH) >> 4;

    for (i = 0; i < 3; i++)
    {
        wElement = LongDF.Order[i];
        switch (wElement & 0xF0)
        {
          case LDF_MONTH:
            pI->iMonLzero = (short) ((wElement & 0x0F) - 1);
            break;

          case LDF_DAY:
            pI->iDayLzero = (short) ((wElement & 0x0F) - 1);
            break;

          case LDF_YEAR:
            pI->iCentury = (short) ((wElement & 0x0F) - 2);
            break;

          default:
            break;
        }
    }
}


/*
 *  This routine sets the time sample into the appropriate
 *  static text item of the dialog.
 */
STATIC void SetTimeSamples(
    HWND hDlg,
    PINTL pI,
    BOOL fUseDefault)
{
    LPTSTR pszAMPM;
    TCHAR  szSample[30];


    if (fUseDefault)
    {
        GetTimeFormat( pI->lcid,
                       LOCALE_NOUSEROVERRIDE,
                       NULL,
                       NULL,
                       szSample,
                       30 );
    }
    else
    {
        /*
         *  Must do this by hand since pszAMPM could be set to anything.
         */
        GetTime ();

        pszAMPM = (wDateTime[HOUR] < 12) ? pI->s1159 : pI->s2359;
        if (!pI->iTime)
            if (!(wDateTime[HOUR] %= 12))
                wDateTime[HOUR] = 12;
        if (pI->iTimeMarker == 1)
        {
            wsprintf ( szSample,
                       pI->iTLZero ? TEXT("%s %02d%s%02d%s%02d")
                                   : TEXT("%s %d%s%02d%s%02d"),
                       pszAMPM,
                       wDateTime[HOUR],
                       pI->sTime,
                       wDateTime[MINUTE],
                       pI->sTime,
                       wDateTime[SECOND] );
        }
        else
        {
            wsprintf ( szSample,
                       pI->iTLZero ? TEXT("%02d%s%02d%s%02d %s")
                                   : TEXT("%d%s%02d%s%02d %s"),
                       wDateTime[HOUR],
                       pI->sTime,
                       wDateTime[MINUTE],
                       pI->sTime,
                       wDateTime[SECOND],
                       pszAMPM );
        }
    }

    SetDlgItemText (hDlg, INTL_TIMESAMPLE, szSample);
}


/*
 *  This routine sets the number sample into the appropriate
 *  static text item of the dialog.
 */
STATIC void SetNumSamples(
    HWND hDlg,
    PINTL pI,
    BOOL fUseDefault)
{
    TCHAR  szPosSample[30];
    TCHAR  szNegSample[30];
    TCHAR  *psz;
    TCHAR  ch;
    int    i;

    TCHAR  szValue[] = TEXT("-1234.000000");
    NUMBERFMT NumFmt;


    /*
     *  Change the decimal digit values to the number of digits.
     *  The maximum value is MAX_DEC_DIGITS.
     */
    psz = szValue + 6;

    i = (pI->iDigits < MAX_DEC_DIGITS) ? pI->iDigits : MAX_DEC_DIGITS;
    ch = (TCHAR) (TEXT('0') + i);
    for ( ; i; i--)
    {
        *psz++ = ch;
    }
    *psz = TEXT('\0');

    if (fUseDefault)
    {
        /*
         *  Get negative number sample.
         */
        GetNumberFormat( pI->lcid,
                         LOCALE_NOUSEROVERRIDE,
                         szValue,
                         NULL,
                         szNegSample,
                         30 );

        /*
         *  Get positive number sample.
         */
        psz = szValue + 1;
        GetNumberFormat( pI->lcid,
                         LOCALE_NOUSEROVERRIDE,
                         psz,
                         NULL,
                         szPosSample,
                         30 );
    }
    else
    {
        NumFmt.NumDigits = (pI->iDigits < MAX_DEC_DIGITS) ? pI->iDigits : MAX_DEC_DIGITS;
        NumFmt.LeadingZero = pI->iLzero;
        NumFmt.Grouping = 3;
        NumFmt.lpDecimalSep = pI->sDecimal;
        NumFmt.lpThousandSep = pI->sThousand;
        NumFmt.NegativeOrder = pI->iNegNumber;

        /*
         *  Get negative number sample.
         */
        GetNumberFormat( pI->lcid,
                         0,
                         szValue,
                         &NumFmt,
                         szNegSample,
                         30 );

        /*
         *  Get positive number sample.
         */
        psz = szValue + 1;
        GetNumberFormat( pI->lcid,
                         0,
                         psz,
                         &NumFmt,
                         szPosSample,
                         30 );
    }

    SetDlgItemText (hDlg, INTL_NUMSAMPLE, szPosSample);
    SetDlgItemText (hDlg, INTL_NEGNUMSAMPLE, szNegSample);
}


#ifdef JAPAN    /* V-KeijiY  June.30.1992 */

// LONG_DATE_FORMAT
WORD ConvertStringToInteger(
    LPTSTR far *p)
{
    TCHAR *pt = *p;
    int result = 0;

    if (*pt == TEXT(','))
        pt++;
    while (*pt == TEXT(' '))
        pt++;
    while (*pt >= TEXT('0') && *pt <= TEXT('9'))
        result = result * 10 + (*pt++ - TEXT('0'));
    *p = pt;

    return (result);
}

DWORD ConvertEraToJapaneseEra(
    WORD Year,
    WORD Month,
    WORD Day)
{
  WORD eranameindex = 0;
  WORD ryear = 1;

  if (Year >= NewYear)
  {
      if (Year == NewYear && Month <= NewMonth && Day < NewDay)
          goto Heisei;
      eranameindex = 5;
      ryear = Year - (NewYear - 1);
  }
  else if (Year >= 1989)
  {
Heisei:
      if (Year == 1989 && Month == 1 && Day < 8)
          goto Syouwa;
      eranameindex = 4;
      ryear = Year - 1988;
  }
  else if (Year >= 1926)
  {
Syouwa:
      eranameindex = 3;
      ryear = Year - 1925;
  }
  else if (Year >= 1912)
  {
      eranameindex = 2;
      ryear = Year - 1911;
  }
  else if (Year >= 1868)
  {
      eranameindex = 1;
      ryear = Year - 1867;
  }

  return ( MAKELONG(ryear, eranameindex) );
}

#endif


/*
 *  Format the date strings and put them in the dialog.
 */
STATIC void SetDateSamples(
    HWND hDlg,
    PINTL pI)
{
    TCHAR    szDate[80];
#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
// Additional space is required in order to support Jpn Emperor's Era
    TCHAR    szDay[3], szMonth[16], szYear[8], szDayName[16], szDayNameTrail[16];
#else
    TCHAR    szDay[3], szMonth[16], szYear[5], szDayName[16];
#endif

    TCHAR   *first, *second, *third;
    LDF      LongDF;
    short    i, j;
    WORD     wKey, wOffset;
    WORD     wTemp, wString;

    /* Get the three date strings: month, day, year
     */
    GetDate ();
    i = wDateTime[YEAR];
    if (!(pI->iCentury))
        i %= 100;
    MyItoa (i, szYear, 10);
    wsprintf (szMonth, pI->iMonLzero ? szPct02D : szPctD, wDateTime[MONTH]);
    wsprintf (szDay, pI->iDayLzero ? szPct02D : szPctD, wDateTime[DAY]);

    /* Determine the order of the strings,
     * and print them to szDate
     */
    if (pI->iDate == 2)
    {
        first = szYear;
        second = szMonth;
        third = szDay;
    }
    else
    {
        third = szYear;
        if (pI->iDate == 1)
        {
            first = szDay;
            second = szMonth;
        }
        else
        {
            first = szMonth;
            second = szDay;
        }
    }
    wsprintf (szDate, TEXT("%s%s%s%s%s"), first, pI->sDateSep,
              second, pI->sDateSep, third);
    SetDlgItemText (hDlg, INTL_DATESAMPLE, szDate);

    /* Parse the long date string
     * and get the day of week string
     */
    ParseLDF (pI->sLongDate, &LongDF);

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
    szDayNameTrail[0] = TEXT('\0');
    if (LongDF.Leadin)
    {
        if ((LongDF.Leadin & 0xF0) == LDF_JaDAY)
            LoadString(hModule,
                JaDAYSOFWK + wDateTime[WEEKDAY] + ((LongDF.Leadin & 0x0F) - 4 ?
                7 : 0 ),
                szDayName, CharSizeOf(szDayName));
        else
            LoadString(hModule,
                DAYSOFWK + wDateTime[WEEKDAY] + ((LongDF.Leadin & 0x0F) - 4 ?
                7 : 0 ),
                szDayName, CharSizeOf(szDayName));
        if (LongDF.Trailin)
        {
            szDayNameTrail[0] = TEXT(' ');
            szDayNameTrail[1] = TEXT('\0');
            lstrcat(szDayNameTrail, szDayName);
            szDayName[0] = TEXT('\0');
        }
        else
            szDayNameTrail[0] = TEXT('\0');
    }
#else
    if (LongDF.Leadin)
    {
        wTemp   = (WORD) ((LongDF.Leadin & 0x0F) - 4) ? 7 : 0;
        wString = (WORD) (DAYSOFWK + wDateTime[WEEKDAY] + wTemp),

        LoadString (hModule, wString, szDayName, CharSizeOf(szDayName));
    }
#endif

    else
        szDayName[0] = TEXT('\0');

    /* Get the month, day, year strings
     */
    for (i = 2; i >= 0; i--)
    {
        wKey = LongDF.Order[i];
        switch (wKey & 0xF0)
        {
          case LDF_DAY:
            wsprintf (szDay, (wKey & 0x0f) == 2 ? szPct02D : szPctD, wDateTime[DAY]);
            break;

          case LDF_MONTH:
            wOffset = 0;
            switch (wKey & 0x0F)
            {
              case 3:
                wOffset = 12;
              case 4:
                LoadString (hModule, (WORD) (MON_OF_YR - 1 + wDateTime[MONTH] + wOffset),
                            szMonth, CharSizeOf(szMonth));
                break;

              default:
                wsprintf (szMonth, (wKey & 0x0f) == 2 ? szPct02D : szPctD,
                          wDateTime[MONTH]);
                break;
            }
            break;

          case LDF_YEAR:
            j = wDateTime[YEAR];
            if ((wKey & 0x0F) == 2)
                j %= 100;
            MyItoa (j, szYear, 10);
            break;

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
          case LDF_JaYEAR:
          {
            DWORD dd = ConvertEraToJapaneseEra (wDateTime[YEAR], wDateTime[MONTH],
                                                wDateTime[DAY]);
            if ((wKey & 0x0F) > 2)
            {
                // set era string
                if (HIWORD(dd) >= 5)    // supporse new emperor's era
                    lstrcpy(szYear, NewEra);
                else
                    LoadString (hModule, JaEMPERORYEAR + HIWORD(dd), szYear,
                                CharSizeOf(szYear));
            }
            else
                szYear[0] = TEXT('\0');
            switch (wKey & 0x0F)
            {
                case 1:
                case 3:
                    wsprintf (szYear + lstrlen(szYear), TEXT("%d"), LOWORD(dd));
                    break;
                case 2:
                case 4:
                default:
                    wsprintf (szYear + lstrlen(szYear), TEXT("%02.02d"),
                              LOWORD(dd));
                    break;
            }
            break;
          }
#endif

        }
    }

    /* Determine the order of the date strings
     * program management has decided that only these three
     * formats will be allowed
     */
    switch (wKey & 0xF0)
    {
      case LDF_DAY:
        first = szDay;
        second = szMonth;
        third = szYear;
        break;

      case LDF_YEAR:
#ifdef JAPAN // LONG_DATE_FORMAT
      case LDF_JaYEAR:
#endif
        first = szYear;
        second = szMonth;
        third = szDay;
        break;

      case LDF_MONTH:
      default:
        first = szMonth;
        second = szDay;
        third = szYear;
        break;
    }

    /* Put the string into the dialog
     */

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
    wsprintf (szDate, TEXT("%s%s %s%s %s%s %s%s%s"), szDayName,
              LongDF.LeadinSep, first, LongDF.Sep[0],
              second, LongDF.Sep[1], third, LongDF.Sep[2],
              szDayNameTrail);
#else
    wsprintf (szDate, TEXT("%s%s %s%s %s%s %s"), szDayName,
              LongDF.LeadinSep, first, LongDF.Sep[0],
              second, LongDF.Sep[1], third);
#endif

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
    // Japanese date format will overflow in the rectangle, truncate required.
    {
        LPTSTR lpPos;
        RECT rcRect;
        WORD wWidth;
        HDC hDc;
        SIZE TxtSize;

        GetWindowRect(GetDlgItem(hDlg, INTL_DATESAMPLE2), (LPRECT) &rcRect);
        // width of long date sample window
        wWidth = rcRect.right - rcRect.left - 4;
        lpPos = szDate + lstrlen(szDate);
        if (hDc = GetDC(hDlg))
        {
            GetTextExtentPoint (hDc, szDate, lstrlen(szDate), &TxtSize);
            if (TxtSize.cx > wWidth)
            {
                // doesn't fit in the window - truncate & try again
                do {
                    lpPos = CharPrev(szDate,lpPos);
                    *lpPos = TEXT('\0');
                    lstrcat(szDate, TEXT("..."));
                    GetTextExtentPoint(hDc, szDate, lstrlen(szDate), &TxtSize);
                } while ( TxtSize.cx > wWidth );
            }
            ReleaseDC (hDlg, hDc);
        }
    }
#endif

    SetDlgItemText (hDlg, INTL_DATESAMPLE2, szDate);
}


/*
 *  This puts the currency samples into the dialog box.
 */
STATIC void SetCurSamples(
    HWND hDlg,
    PINTL pI,
    BOOL fUseDefault)
{
    TCHAR  szPosSample[30];
    TCHAR  szNegSample[30];
    TCHAR  *psz;
    TCHAR  ch;
    int    i;

    TCHAR  szValue[] = TEXT("-1.000000000");
    CURRENCYFMT CurFmt;


    /*
     *  Change the decimal digit values to the number of digits.
     *  The maximum value is MAX_DEC_DIGITS.
     */
    psz = szValue + 3;

    i = (pI->iCurDec < MAX_DEC_DIGITS) ? pI->iCurDec : MAX_DEC_DIGITS;
    ch = (TCHAR) (TEXT('0') + i);
    for ( ; i; i--)
    {
        *psz++ = ch;
    }
    *psz = TEXT('\0');

    if (fUseDefault)
    {
        /*
         *  Get negative currency sample.
         */
        GetCurrencyFormat( pI->lcid,
                           LOCALE_NOUSEROVERRIDE,
                           szValue,
                           NULL,
                           szNegSample,
                           30 );

        /*
         *  Get positive currency sample.
         */
        psz = szValue + 1;
        GetCurrencyFormat( pI->lcid,
                           LOCALE_NOUSEROVERRIDE,
                           psz,
                           NULL,
                           szPosSample,
                           30 );
    }
    else
    {
        CurFmt.NumDigits = (pI->iCurDec < MAX_DEC_DIGITS) ? pI->iCurDec : MAX_DEC_DIGITS;
        CurFmt.LeadingZero = pI->iLzero;
        CurFmt.Grouping = 3;
        CurFmt.lpDecimalSep = pI->sMonDecimal;
        CurFmt.lpThousandSep = pI->sMonThousand;
        CurFmt.NegativeOrder = pI->iNegCur;
        CurFmt.PositiveOrder = pI->iCurFmt;
        CurFmt.lpCurrencySymbol = pI->sCurrency;

        /*
         *  Get negative currency sample.
         */
        GetCurrencyFormat( pI->lcid,
                           0,
                           szValue,
                           &CurFmt,
                           szNegSample,
                           30 );

        /*
         *  Get positive currency sample.
         */
        psz = szValue + 1;
        GetCurrencyFormat( pI->lcid,
                           0,
                           psz,
                           &CurFmt,
                           szPosSample,
                           30 );
    }

    SetDlgItemText (hDlg, INTL_CURSAMPLE, szPosSample);
    SetDlgItemText (hDlg, INTL_NEGSAMPLE, szNegSample);
}


/*
 *  This routine loads the International section of WIN.INI
 *  into the structure "Current". If items are missing U.S. values
 *  are used.
 */
STATIC void LoadIntlWin (void)
{
    TCHAR   szwork[128];


    /*
     *  Get language and country information.
     */
    GetLocaleValue (0,
                    LOCALE_SABBREVLANGNAME,
                    Current.sLanguage,
                    CharSizeOf(Current.sLanguage),
                    IntlDef.sLanguage);
    GetLocaleValue (0,
                    LOCALE_SCOUNTRY,
                    Current.sCountry,
                    CharSizeOf(Current.sCountry),
                    IntlDef.sCountry);
    Current.iCountry = GetLocaleValue (0,
                                       LOCALE_ICOUNTRY,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iCountry);

    /*
     *  Get list separator and measurement information.
     */
    GetLocaleValue (0,
                    LOCALE_SLIST,
                    Current.sList,
                    CharSizeOf(Current.sList),
                    IntlDef.sList);
    if ((Current.iMeasure = GetLocaleValue (0,
                                            LOCALE_IMEASURE,
                                            szwork,
                                            CharSizeOf(szwork),
                                            DefIStr.iMeasure)) > 1)
    {
        Current.iMeasure = 0;
    }

    /*
     *  Get number format information.
     */
    GetLocaleValue (0,
                    LOCALE_STHOUSAND,
                    Current.sThousand,
                    CharSizeOf(Current.sThousand),
                    IntlDef.sThousand);
    GetLocaleValue (0,
                    LOCALE_SDECIMAL,
                    Current.sDecimal,
                    CharSizeOf(Current.sDecimal),
                    IntlDef.sDecimal);
    Current.iDigits  = GetLocaleValue (0,
                                       LOCALE_IDIGITS,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iDigits);
    Current.iLzero   = GetLocaleValue (0,
                                       LOCALE_ILZERO,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iLzero);
    Current.iNegNumber  = GetLocaleValue (0,
                                         LOCALE_INEGNUMBER,
                                         szwork,
                                         CharSizeOf(szwork),
                                         DefIStr.iNegNumber);

    /*
     *  Get currency format information.
     */
    GetLocaleValue (0,
                    LOCALE_SCURRENCY,
                    Current.sCurrency,
                    CharSizeOf(Current.sCurrency),
                    IntlDef.sCurrency);
    GetLocaleValue (0,
                    LOCALE_SMONTHOUSANDSEP,
                    Current.sMonThousand,
                    CharSizeOf(Current.sMonThousand),
                    IntlDef.sMonThousand);
    GetLocaleValue (0,
                    LOCALE_SMONDECIMALSEP,
                    Current.sMonDecimal,
                    CharSizeOf(Current.sMonDecimal),
                    IntlDef.sMonDecimal);
    Current.iCurDec  = GetLocaleValue (0,
                                       LOCALE_ICURRDIGITS,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iCurDec);
    Current.iCurFmt  = GetLocaleValue (0,
                                       LOCALE_ICURRENCY,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iCurFmt);
    Current.iNegCur  = GetLocaleValue (0,
                                       LOCALE_INEGCURR,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iNegCur);

    /*
     *  Get time format information.
     */
    GetLocaleValue (0,
                    LOCALE_STIMEFORMAT,
                    Current.sTimeFormat,
                    CharSizeOf(Current.sTimeFormat),
                    IntlDef.sTimeFormat);
    GetLocaleValue (0,
                    LOCALE_STIME,
                    Current.sTime,
                    CharSizeOf(Current.sTime),
                    IntlDef.sTime);
    Current.iTime    = GetLocaleValue (0,
                                       LOCALE_ITIME,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iTime);
    Current.iTLZero  = GetLocaleValue (0,
                                       LOCALE_ITLZERO,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iTLZero);
    Current.iTimeMarker = GetLocaleValue (0,
                                         LOCALE_ITIMEMARKPOSN,
                                         szwork,
                                         CharSizeOf(szwork),
                                         DefIStr.iTimeMarker);
    GetLocaleValue (0,
                    LOCALE_S1159,
                    Current.s1159,
                    CharSizeOf(Current.s1159),
                    IntlDef.s1159);
    GetLocaleValue (0,
                    LOCALE_S2359,
                    Current.s2359,
                    CharSizeOf(Current.s2359),
                    IntlDef.s2359);

    /*
     *  Get date format information.
     */
    GetLocaleValue (0,
                    LOCALE_SSHORTDATE,
                    Current.sShortDate,
                    CharSizeOf(Current.sShortDate),
                    IntlDef.sShortDate);
    GetLocaleValue (0,
                    LOCALE_SDATE,
                    Current.sDateSep,
                    CharSizeOf(Current.sDateSep),
                    IntlDef.sDateSep);
    Current.iDate    = GetLocaleValue (0,
                                       LOCALE_IDATE,
                                       szwork,
                                       CharSizeOf(szwork),
                                       DefIStr.iDate);
    GetLocaleValue (0,
                    LOCALE_SLONGDATE,
                    Current.sLongDate,
                    CharSizeOf(Current.sLongDate),
                    IntlDef.sLongDate);


    TranslateShortDate (&Current);

#ifdef JAPAN // LONG_DATE_FORMAT
    {
      TCHAR szBuf[40];
      LPTSTR lp, lpt;

      if (GetProfileString (szIntl, TEXT("NewEra"), TEXT(""), szBuf, CharSizeOf(szBuf)))
      {
          lp = szBuf;
          lpt = NewEra;
          while (*lp != TEXT(','))
              *lpt++ = *lp++;
          *lpt = TEXT('\0');
          NewYear = ConvertStringToInteger(&lp);
          NewMonth = ConvertStringToInteger(&lp);
          NewDay = ConvertStringToInteger(&lp);
          if (NewYear < 1991)
              NewYear = 0;      // wrong conversion data
      }
  }
#endif

}


/*
 *  This does a total reset of all the samples in the dialog.
 */
STATIC void FillIntlDlg(
    HWND hDlg,
    PINTL pIntl,
    BOOL fUseDefault)
{
    SetDlgItemText (hDlg, INTL_LISTSEP, pIntl->sList);
    SendDlgItemMessage (hDlg, INTL_MEASUREMENT, CB_SETCURSEL, pIntl->iMeasure, 0L);
    SetDateSamples (hDlg, pIntl);
    SetTimeSamples (hDlg, pIntl, fUseDefault);
    SetNumSamples (hDlg, pIntl, fUseDefault);
    SetCurSamples (hDlg, pIntl, fUseDefault);
}


/*
 *  Callback function for EnumSystemLocales.
 */
BOOL CALLBACK EnumSysLocaleFunc(
    LPTSTR pLocale)
{
    NLSLCID *pTemp;          /* temp pointer to linked list */
    TCHAR   pData[128];      /* 


    /*
     *  Make the new nlslcid node and add it to the linked list.
     */
    if (pFirstNlsLcid)
    {
        pTemp = (NLSLCID *) AllocMem (sizeof(NLSLCID));
        pTemp->pNext = pFirstNlsLcid;
        pFirstNlsLcid = pTemp;
    }
    else        // First time thru
    {
        pFirstNlsLcid = (NLSLCID *) AllocMem (sizeof(NLSLCID));
        pFirstNlsLcid->pNext = NULL;
    }

    /*
     *  Initialize bMulti flag to false.
     */
    pFirstNlsLcid->bMulti = FALSE;

    /*
     *  Save LCID value and get "Country Code" value for the LCID.
     */
    pFirstNlsLcid->lcid = _tcstoul (pLocale, NULL, 16);
    pFirstNlsLcid->iCountry =
        GetLocaleValue (pFirstNlsLcid->lcid,
                        LOCALE_ICOUNTRY | LOCALE_NOUSEROVERRIDE,
                        pData,
                        CharSizeOf(pData),
                        NULL);

    /*
     *  Return success.
     */
    return (TRUE);
}


/*
 *  This initializes all of the international dialog controls.
 */
STATIC BOOL InitIntlDlg(
    HWND hDlg)
{
    int     nLen, nDefault;
    int     count;            /* iterative */
    HANDLE  hLBName, hkey;
    TCHAR   szLBEntry[128];
    int     nCountry;         /* number of countries available in setup.inf */
    LPTSTR  pszTemp;
    LPSTR   pszAnsi;          /* ptr to ANSI Locales option info from SETUP api call */
    TCHAR   szLocale[40];
    TCHAR   szLayout[40];
    DWORD   dwSize, dwType;
    int     rc;
    NLSLCID *pNlsLcid;
    LPTSTR  pszFileName, pszOption, pszOptionText;
    LPTSTR  pszCurrent, pszDefault;
    int     nPlace;


    pLocales = NULL;
    pLayouts = NULL;

    if ((pszTemp = BackslashTerm (szSharedDir)) - szSharedDir > 3)
        *(pszTemp - 1) = TEXT('\0');

    SetCurrentDirectory(szSharedDir);

    LoadString (hModule, PRN, szSetupDir, CharSizeOf(szSetupDir));
    LoadString (hModule, PRN, szDirOfSrc, CharSizeOf(szDirOfSrc));

    /* load Window International section into structure "Current"
     */
    LoadIntlWin ();

    hLBName = GetDlgItem (hDlg, INTL_COUNTRY);

    /////////////////////////////////////////////////////////////////
    // Enumerate all supported locales for the Country listbox.
    /////////////////////////////////////////////////////////////////

    nCountry = 0;

    /*
     *  First: Make a linked list of all LCIDs in Registry.
     *
     *  This uses the EnumSysLocaleFunc callback function to
     *  build the linked list.
     */
    pFirstNlsLcid = (NLSLCID *) NULL;
    EnumSystemLocales (EnumSysLocaleFunc, LCID_SUPPORTED);

    /*
     *  Now traverse the list, and determine duplicate country codes
     *
     *  Outer loop ensures that we check every LCID entry from registry
     *  If statement determines if a check is even necessary
     *          (i.e. if the field is already TRUE, we already have an
     *           answer for this LCID, so  just move on to the next one).
     *  Inner loop tests all remaining LCIDs against current LCID for a
     *  matching Country code and sets bMulti to TRUE.
     */
    pNlsLcid = pFirstNlsLcid;
    while (pNlsLcid)
    {
        if (!pNlsLcid->bMulti)
        {
            NLSLCID *pTemp;

            pTemp = pNlsLcid->pNext;

            while (pTemp)
            {
                if (pNlsLcid->iCountry == pTemp->iCountry)
                {
                    pNlsLcid->bMulti = TRUE;
                    pTemp->bMulti = TRUE;
                }
                pTemp = pTemp->pNext;
            }
        }

        pNlsLcid = pNlsLcid->pNext;
    }

    /*
     *  Traverse the list again, putting values in ListBox based on
     *  the findings above.
     */
    pNlsLcid = pFirstNlsLcid;
    while (pNlsLcid)
    {
        TCHAR szLocName[256];
        TCHAR szLang[128];

        if (pNlsLcid->bMulti)
        {
            rc = GetLocaleValue (pNlsLcid->lcid,
                                 LOCALE_SCOUNTRY | LOCALE_NOUSEROVERRIDE,
                                 szLBEntry,
                                 CharSizeOf(szLBEntry),
                                 NULL);

            // In case of error - do not put this entry in Combobox
            if (rc == -1)
                goto SkipBadEntry;

            rc = GetLocaleValue (pNlsLcid->lcid,
                                 LOCALE_SLANGUAGE | LOCALE_NOUSEROVERRIDE,
                                 szLang,
                                 CharSizeOf(szLang),
                                 NULL);

            if (rc == -1)
                goto SkipBadEntry;

            wsprintf (szLocName, TEXT("%s (%s)"), szLBEntry, szLang);
        }
        else
        {
            rc = GetLocaleValue (pNlsLcid->lcid,
                                 LOCALE_SCOUNTRY | LOCALE_NOUSEROVERRIDE,
                                 szLocName,
                                 CharSizeOf(szLocName),
                                 NULL);

            if (rc == -1)
                goto SkipBadEntry;
        }

        count = SendMessage (hLBName, CB_ADDSTRING, 0, (LPARAM) szLocName);
        nCountry++;
        SendMessage (hLBName, CB_SETITEMDATA, count, (LPARAM) pNlsLcid->lcid);

SkipBadEntry:
        pNlsLcid = pNlsLcid->pNext;
    }

    /*
     *  Finally traverse the list, freeing list memory.
     */
    pNlsLcid = pFirstNlsLcid;
    while (pNlsLcid)
    {
        pFirstNlsLcid  = pNlsLcid;
        pNlsLcid = pNlsLcid->pNext;

        FreeMem ((LPVOID) pFirstNlsLcid, sizeof(NLSLCID));
    }

    /*
     *  If there are no entries in the listbox, then error.
     */
    if (!nCountry)
        goto Error1;

    /*
     *  Find the currently selected country by searching for
     *  Current.sCountry in the listbox.
     */
    count = -1;
    nLen = lstrlen (Current.sCountry);
    while (1)
    {
        if ((nCurCountry = SendMessage (hLBName, CB_FINDSTRING, count,
                                        (LPARAM) Current.sCountry)) <= count)
        {
            nCurCountry = nCountry - 1;
            break;
        }
        if (SendMessage (hLBName, CB_GETLBTEXTLEN, nCurCountry, 0L) == nLen)
        {
            /*
             *  We have an exact match on country name alone, no further
             *  checks are necessary, just use it.
             */
            break;
        }
        else
        {
            LCID lcid;

            /*
             *  Since we can have "country" entries like
             *         "Canada (Canadian English)"
             *         "Canada (Canadian French)"
             *         "Switzerland (Swiss French)"
             *         "Switzerland (Swiss German)"
             *         "Switzerland (Swiss Italian)"
             *
             *  the above CB_FINDSTRING will match only the first entry
             *  initially.  We need to do a further check against
             *  LOCALE_SABREVLANGNAME to find the exact match.
             */
            lcid = (LCID)SendMessage (hLBName, CB_GETITEMDATA, nCurCountry, 0L);

            GetLocaleValue (lcid,
                            LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                            szLBEntry,
                            CharSizeOf(szLBEntry),
                            NULL);

            if (!lstrcmpi (szLBEntry, Current.sLanguage))
                break;

            /*
             *  else this entry didn't match, so go check next closest
             *  Combobox entry.
             */
        }
        count = nCurCountry;
    }

    Current.lcid = (LCID)SendMessage (hLBName, CB_GETITEMDATA, nCurCountry, 0L);
    SendMessage (hLBName, CB_SETCURSEL, nCurCountry, 0L);

    //////////////////////////////////////////////////////////////////////////
    //  Get user's default selection for Locale/Language and Keyboard
    //  Layout from Registry and select it in ComboBox.
    //////////////////////////////////////////////////////////////////////////

    /*
     *  Assume that no Locale and Layout config choice exists.
     */
    szLocale[0] = TEXT('\0');
    szLayout[0] = TEXT('\0');

    /*
     *  Get the system default Locale.
     */
    if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szInstalledLocales,
                       0L, KEY_READ, (PHKEY) &hkey))
    {
        dwSize = sizeof(szLocale);

        rc = RegQueryValueEx (hkey, szRegDefault, NULL, &dwType,
                              (LPBYTE) szLocale, &dwSize);
        RegCloseKey (hkey);

        if (rc != 0)
        {
            goto DefaultLocaleFail;
        }
    }
    else
    {
DefaultLocaleFail:
        /*
         *  Error condition - System DEFAULT locale not found.
         *
         *  Put up a message box error stating that the USER does
         *  not have any USER or SYSTEM default locale info
         *  configured in the registry and just continue for now.
         */
        MyMessageBox (hDlg, INTL+9, INITS+1, MB_OK | MB_ICONINFORMATION);
    }


    //  Try to get the USER's Keyboard Layout from the registry

    if (!RegOpenKeyEx (HKEY_CURRENT_USER, szRegLayerDef, 0L, KEY_READ, (PHKEY) &hkey))
    {
        dwSize = sizeof(szLayout);
        rc = RegQueryValueEx (hkey, szRegActive, NULL, &dwType,
                              (LPBYTE) szLayout, &dwSize);
        RegCloseKey (hkey);

        //  Error condition - USER Layout not found,
        //  Try to find a system default Layout

        if (rc != 0)
            goto FindDefaultLayout;
    }
    else
    {
        //  Error reading registry for USER
        //  Try to find a system default Layout
FindDefaultLayout:
        if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szInstalledLayouts,
                           0L, KEY_READ, (PHKEY) &hkey))
        {
            dwSize = sizeof(szLayout);

            rc = RegQueryValueEx (hkey, szRegDefault, NULL, &dwType,
                                  (LPBYTE) szLayout, &dwSize);
            RegCloseKey (hkey);

            if (rc != 0)
                goto DefaultLayoutFail;
        }
        else
        {
DefaultLayoutFail:
            //  Error condition - System DEFAULT locale not found

            //  Put up a message box error stating that the USER does
            //  not have any USER or SYSTEM default Keyboard Layout info
            //  configured in the registry and just continue for now.

            MyMessageBox (hDlg, INTL+10, INITS+1, MB_OK | MB_ICONINFORMATION);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //  Get option lists from .INF files, select USER defaults
    //////////////////////////////////////////////////////////////////////////

    //  Get the "LANGUAGE" options

    hLBName = GetDlgItem (hDlg, INTL_LANGUAGE);

    LoadString (hModule, KBD + 1, szLBEntry, CharSizeOf(szLBEntry));
    nDefault = -1;
    nCurLang = -1;

    pszCurrent = NULL;
    pszDefault = NULL;

    pszAnsi = GetAllOptionsText ("LANGUAGE", 0);

    pLocales = pszTemp = (LPTSTR) pszAnsi;

    //  Check for api errors and NoOptions

    if ((pszAnsi == NULL) || ((*pszAnsi == '\0') && (*(pszAnsi+1) == '\0')))
        goto Error1;

#ifdef UNICODE
    if (pszAnsi)
    {
        // Get size of buffer pointed to by pszAnsi
        for (nLen = 0; ;pszAnsi++)
        {
            nLen++;
            if (*pszAnsi == '\0' && *(pszAnsi+1) == '\0')
                break;
        }

        // Take into account double-null termination of buffer
        nLen += 2;
        pszAnsi = (LPSTR) pszTemp;

        pLocales = pszTemp = (LPTSTR) LocalAlloc (LPTR, nLen*2);
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, pszAnsi, nLen, pLocales, nLen);
        LocalFree ((HLOCAL) pszAnsi);
    }
#endif

    //  Continue until we reach end of buffer, marked by Double '\0'

    while (*pszTemp != TEXT('\0'))
    {
        //  Get a ptr to each of item in the triplet strings returned
        pszOption     = pszTemp;
        pszOptionText = pszOption + lstrlen (pszOption) + 1;
        pszFileName   = pszOptionText + lstrlen (pszOptionText) + 1;

        nPlace = SendMessage (hLBName, CB_ADDSTRING, (DWORD)-1, (LPARAM)pszOptionText);

        //  Save ptr to Filename and Option strings for later use with SETUP

        SendMessage (hLBName, CB_SETITEMDATA, nPlace, (LPARAM)pszOption);

        //  Find OPTION that matches User's selection
        //
        //  Since this is a sorted Combo-box, we must save a ptr to the
        //  OptionText for both current and default selections.  Later we
        //  will perform a search to get the actual cbox index of the
        //  matching selections.

        // Special Note:  The pszOption values point to 8-char strings
        //                like "00000409" BUT szLocale is a 4-char Language
        //                number string value like "0409"; hence, we will
        //                do the comparison starting at pszOption+4.
        //     Use this when we go to UniCode (pszOption + 4*sizeof(TCHAR)).

        if (!lstrcmpi (pszOption + 4, szLocale))
            pszCurrent = pszOptionText;

        //  Find selection matching our "Default", for possible use below

        if (!lstrcmpi (pszOption, szLBEntry))
            pszDefault = pszOptionText;

        //  Point to next triplet
        pszTemp = pszFileName + lstrlen (pszFileName) + 1;
    }

    // Find the currently selected Language by searching for it in the listbox

    if (pszCurrent)
    {
        count = -1;
        nLen = lstrlen (pszCurrent);

        while (1)
        {
            if ((nCurLang = SendMessage (hLBName, CB_FINDSTRING, count,
                                          (LPARAM)pszCurrent)) <= count)
            {
                //  If we can't find a match set to invalid value
                nCurLang = -1;
                break;
            }
            if (SendMessage (hLBName, CB_GETLBTEXTLEN, nCurLang, 0L) == nLen)
                break;
            count = nCurLang;
        }
    }

    // Find the default Language by searching for it in the listbox

    if (pszDefault)
    {
        count = -1;
        nLen = lstrlen (pszDefault);

        while (1)
        {
            if ((nDefault = SendMessage (hLBName, CB_FINDSTRING, count,
                                          (LPARAM) pszDefault)) <= count)
            {
                //  If we can't find a match set to invalid value
                nDefault = -1;
                break;
            }
            if (SendMessage (hLBName, CB_GETLBTEXTLEN, nDefault, 0L) == nLen)
                break;
            count = nDefault;
        }
    }

    if (nCurLang == -1)
    {
// LangDLLFailure:
        //  Set ultimate default selection if nothing found
        nCurLang = (nDefault == -1) ? 0 : nDefault;
    }

    SendMessage (hLBName, CB_SETCURSEL, nCurLang, 0L);

    //////////////////////////////////////////////////////////////////////////
    //  Get option lists from .INF files, select USER defaults for Layout
    //////////////////////////////////////////////////////////////////////////

#if KEYBOARDHARDWARECHANGABLE
    /* Get the keyboard section
     */
    hLBName = GetDlgItem (hDlg, INTL_KEYBOARD);

    LoadString (hModule, KBD + 2, szLBEntry, CharSizeOf(szLBEntry));
    nDefault = -1;
    nCurKbd = -1;

    pszCurrent = NULL;
    pszDefault = NULL;

//    pszAnsi = GetAllOptionsText ("LAYOUT", GetUserDefaultLangID());
//    pszAnsi = GetAllOptionsText ("LAYOUT", LANG_ENGLISH);
    pszAnsi = GetAllOptionsText ("LAYOUT", 0);

    pLayouts = pszTemp = (LPTSTR) pszAnsi;

    //  Check for api errors and NoOptions

    if ((pszAnsi == NULL) || ((*pszAnsi == '\0') && (*(pszAnsi+1) == '\0')))
        goto Error1;

#ifdef UNICODE
    if (pszAnsi)
    {
        // Get size of buffer pointed to by pszAnsi
        for (nLen = 0; ;pszAnsi++)
        {
            nLen++;
            if (*pszAnsi == '\0' && *(pszAnsi+1) == '\0')
                break;
        }

        nLen += 2;
        pszAnsi = (LPSTR) pszTemp;

        pLayouts = pszTemp = (LPTSTR) LocalAlloc (LPTR, nLen*2);
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, pszAnsi, nLen, pLayouts, nLen);
        LocalFree ((HLOCAL) pszAnsi);
    }
#endif

    //  Continue until we reach end of buffer, marked by Double '\0'

    while (*pszTemp != TEXT('\0'))
    {
        //  Get a ptr to each of item in the triplet strings returned
        pszOption     = pszTemp;
        pszOptionText = pszOption + lstrlen (pszOption) + 1;
        pszFileName   = pszOptionText + lstrlen (pszOptionText) + 1;

        nPlace = SendMessage (hLBName, CB_ADDSTRING, (DWORD) -1, (LPARAM)pszOptionText);

        //  Save ptr to Filename and Option strings for later use with SETUP

        SendMessage (hLBName, CB_SETITEMDATA, nPlace, (LPARAM)pszOption);

        //  Find OPTION that matches User's selection
        //
        //  Since this is a sorted Combo-box, we must save a ptr to the
        //  OptionText for both current and default selections.  Later we
        //  will perform a search to get the actual cbox index of the
        //  matching selections.

        if (!lstrcmpi (pszOption, szLayout))
            pszCurrent = pszOptionText;

        //  Find selection matching our "Default", for possible use below

        if (!lstrcmpi (pszOption, szLBEntry))
            pszDefault = pszOptionText;

        //  Point to next triplet
        pszTemp = pszFileName + lstrlen (pszFileName) + 1;
    }

    // Find the currently selected Layout by searching for it in the listbox

    if (pszCurrent)
    {
        count = -1;
        nLen = lstrlen (pszCurrent);

        while (1)
        {
            if ((nCurKbd = SendMessage (hLBName, CB_FINDSTRING, count,
                                          (LPARAM)pszCurrent)) <= count)
            {
                //  If we can't find a match set to invalid value
                nCurKbd = -1;
                break;
            }
            if (SendMessage (hLBName, CB_GETLBTEXTLEN, nCurKbd, 0L) == nLen)
                break;
            count = nCurKbd;
        }
    }

    // Find the default Layout by searching for it in the listbox

    if (pszDefault)
    {
        count = -1;
        nLen = lstrlen (pszDefault);

        while (1)
        {
            if ((nDefault = SendMessage (hLBName, CB_FINDSTRING, count,
                                          (LPARAM)pszDefault)) <= count)
            {
                //  If we can't find a match set to invalid value
                nDefault = -1;
                break;
            }
            if (SendMessage (hLBName, CB_GETLBTEXTLEN, nDefault, 0L) == nLen)
                break;
            count = nDefault;
        }
    }

    if (nCurKbd == -1)
    {
// CannotFindKbdDll:
        //  Set ultimate default selection if nothing found
        nCurKbd = (nDefault == -1) ? 0 : nDefault;
    }

    SendMessage (hLBName, CB_SETCURSEL, nCurKbd, 0L);
#endif  //  KEYBOARDHARDWARECHANGABLE

    /* Set up measurement systems (English and Metric)
     */
    for (count = 0; count < MEASUREMENTSYSTEMS; count++)
    {
        LoadString (hModule, (WORD) (count + MEASUREMENTSYS), szLBEntry, CharSizeOf(szLBEntry));
        SendDlgItemMessage (hDlg, INTL_MEASUREMENT, CB_ADDSTRING, 0L,
                            (LPARAM)szLBEntry);
    }

    /* now fill the dialog box */
    FillIntlDlg (hDlg, &Current, FALSE);
    SendDlgItemMessage (hDlg, INTL_LISTSEP, EM_LIMITTEXT, 1, 0L);
    return (TRUE);          /* made it past error exits */

Error1:
    //  Free memory from SETUP api calls
    if (pLocales != NULL)
        LocalFree ((HLOCAL) pLocales);

    if (pLayouts != NULL)
        LocalFree ((HLOCAL) pLayouts);

    MyMessageBox (hDlg, INTL + 8, INITS + 1, MB_OK | MB_ICONINFORMATION);
    return (FALSE);
}


/*
 *  Use GetLocaleInfo NLS api to get the value for the given lctype.
 *  If no user override is to be used (want system default), then
 *  the caller must pass in the LOCALE_NOUSEROVERRIDE flag as part
 *  of the lctype.  It returns the Integer value of the returned
 *  string (if appropriate).
 */
int GetLocaleValue(
    LCID lcid,
    LCTYPE lcType,
    TCHAR *pszStr,
    int size,
    LPTSTR pszDefault )
{
    /*
     *  Initialize the output buffer.
     */
    *pszStr = (TCHAR) 0;

    /*
     *  Get the locale information.
     */
    if (!GetLocaleInfo ( lcid,
                         lcType,
                         pszStr,
                         size ))
    {
        /*
         *  Couldn't get info from GetLocaleInfo.
         */
        if (pszDefault)
        {
            /*
             *  Return the default info.
             */
            lstrcpy (pszStr, pszDefault);
        }
        else
        {
            /*
             *  Return error.
             */
            return (-1);
        }
    }

    /*
     *  Convert the string to an integer and return the result.
     *  This will only be used by the caller of this routine when
     *  appropriate.
     */
    return ( MyAtoi (pszStr) );
}


/*
 *  This routine reads the pszCountry string, which contains
 *  the current country string read from the inf file,
 *  and builds an INTLSTRUCT pointed by pIntl.
 */
STATIC void DecodeCountryString(
    PINTL pIntl)
{
    TCHAR   szwork[128];
    LCID    lcid = pIntl->lcid;


    /*
     *  Get language and country information.
     */
    GetLocaleValue (lcid,
                    LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                    pIntl->sLanguage,
                    CharSizeOf(pIntl->sLanguage),
                    IntlDef.sLanguage);
    GetLocaleValue (lcid,
                    LOCALE_SCOUNTRY | LOCALE_NOUSEROVERRIDE,
                    pIntl->sCountry,
                    CharSizeOf(pIntl->sCountry),
                    DefIStr.lcid);
    pIntl->iCountry = GetLocaleValue (lcid,
                                      LOCALE_ICOUNTRY | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iCountry);

    /*
     *  Get list separator and measurement information.
     */
    GetLocaleValue (lcid,
                    LOCALE_SLIST | LOCALE_NOUSEROVERRIDE,
                    pIntl->sList,
                    CharSizeOf(pIntl->sList),
                    IntlDef.sList);
    pIntl->iMeasure = GetLocaleValue (lcid,
                                      LOCALE_IMEASURE | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iMeasure);

    /*
     *  Get number format information.
     */
    GetLocaleValue (lcid,
                    LOCALE_STHOUSAND | LOCALE_NOUSEROVERRIDE,
                    pIntl->sThousand,
                    CharSizeOf(pIntl->sThousand),
                    IntlDef.sThousand);
    GetLocaleValue (lcid,
                    LOCALE_SDECIMAL | LOCALE_NOUSEROVERRIDE,
                    pIntl->sDecimal,
                    CharSizeOf(pIntl->sDecimal),
                    IntlDef.sDecimal);
    pIntl->iDigits  = GetLocaleValue (lcid,
                                      LOCALE_IDIGITS | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iDigits);
    pIntl->iLzero   = GetLocaleValue (lcid,
                                      LOCALE_ILZERO | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iLzero);
    pIntl->iNegNumber  = GetLocaleValue (lcid,
                                         LOCALE_INEGNUMBER | LOCALE_NOUSEROVERRIDE,
                                         szwork,
                                         CharSizeOf(szwork),
                                         DefIStr.iNegNumber);

    /*
     *  Get currency format information.
     */
    GetLocaleValue (lcid,
                    LOCALE_SCURRENCY | LOCALE_NOUSEROVERRIDE,
                    pIntl->sCurrency,
                    CharSizeOf(pIntl->sCurrency),
                    IntlDef.sCurrency);
    GetLocaleValue (lcid,
                    LOCALE_SMONTHOUSANDSEP | LOCALE_NOUSEROVERRIDE,
                    pIntl->sMonThousand,
                    CharSizeOf(pIntl->sMonThousand),
                    IntlDef.sMonThousand);
    GetLocaleValue (lcid,
                    LOCALE_SMONDECIMALSEP | LOCALE_NOUSEROVERRIDE,
                    pIntl->sMonDecimal,
                    CharSizeOf(pIntl->sMonDecimal),
                    IntlDef.sMonDecimal);
    pIntl->iCurDec  = GetLocaleValue (lcid,
                                      LOCALE_ICURRDIGITS | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iCurDec);
    pIntl->iCurFmt  = GetLocaleValue (lcid,
                                      LOCALE_ICURRENCY | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iCurFmt);
    pIntl->iNegCur  = GetLocaleValue (lcid,
                                      LOCALE_INEGCURR | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iNegCur);

    /*
     *  Get time format information.
     */
    GetLocaleValue (lcid,
                    LOCALE_STIMEFORMAT | LOCALE_NOUSEROVERRIDE,
                    pIntl->sTimeFormat,
                    CharSizeOf(pIntl->sTimeFormat),
                    IntlDef.sTimeFormat);
    GetLocaleValue (lcid,
                    LOCALE_STIME | LOCALE_NOUSEROVERRIDE,
                    pIntl->sTime,
                    CharSizeOf(pIntl->sTime),
                    IntlDef.sTime);
    pIntl->iTime    = GetLocaleValue (lcid,
                                      LOCALE_ITIME | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iTime);
    pIntl->iTLZero  = GetLocaleValue (lcid,
                                      LOCALE_ITLZERO | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iTLZero);
    pIntl->iTimeMarker = GetLocaleValue (lcid,
                                         LOCALE_ITIMEMARKPOSN | LOCALE_NOUSEROVERRIDE,
                                         szwork,
                                         CharSizeOf(szwork),
                                         DefIStr.iTimeMarker);
    GetLocaleValue (lcid,
                    LOCALE_S1159 | LOCALE_NOUSEROVERRIDE,
                    pIntl->s1159,
                    CharSizeOf(pIntl->s1159),
                    IntlDef.s1159);
    GetLocaleValue (lcid,
                    LOCALE_S2359 | LOCALE_NOUSEROVERRIDE,
                    pIntl->s2359,
                    CharSizeOf(pIntl->s2359),
                    IntlDef.s2359);

    /*
     *  Get date format information.
     */
    GetLocaleValue (lcid,
                    LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE,
                    pIntl->sShortDate,
                    CharSizeOf(pIntl->sShortDate),
                    IntlDef.sShortDate);
    GetLocaleValue (lcid,
                    LOCALE_SDATE | LOCALE_NOUSEROVERRIDE,
                    pIntl->sDateSep,
                    CharSizeOf(pIntl->sDateSep),
                    IntlDef.sDateSep);
    pIntl->iDate    = GetLocaleValue (lcid,
                                      LOCALE_IDATE | LOCALE_NOUSEROVERRIDE,
                                      szwork,
                                      CharSizeOf(szwork),
                                      DefIStr.iDate);
    GetLocaleValue (lcid,
                    LOCALE_SLONGDATE | LOCALE_NOUSEROVERRIDE,
                    pIntl->sLongDate,
                    CharSizeOf(pIntl->sLongDate),
                    IntlDef.sLongDate);
}


/*
 *  This gets the new country information and updates the dialog.
 */
STATIC void ChangeCountry(
    HWND hDlg)
{
    int  nNation;
    LCID lcid;

    /* First check if this is the current country, and if we have enough room
     * in the buffer for the string (all reasonable strings will be OK)
     * Notice that LB_ERR >= any WORD
     */
    if ((nNation = SendDlgItemMessage (hDlg, INTL_COUNTRY, CB_GETCURSEL, 0, 0L))
                 == nCurCountry)
        return;

    lcid = (LCID) SendDlgItemMessage (hDlg, INTL_COUNTRY, CB_GETITEMDATA,
                                      nCurCountry = nNation, 0L);

    if (lcid == Current.lcid)
        return;
    else
        Current.lcid = lcid;

    DecodeCountryString (&Current);

    TranslateShortDate (&Current);
    FillIntlDlg (hDlg, &Current, TRUE);
}


/*
 *  This changes the system and ini files to reflect the new choice
 *  for language or keyboard driver.
 */
STATIC BOOL ChangeDriver(
    HWND hDlg,
    short nID)
{
    short nOriginal, nCurrent;
    HWND hCB;
    LPTSTR pszInfFile, pszOption, pszOptionText;
    HKEY hkey;

    /*
     *  Determine what we are talking about, and get the two windows.
     */
    nOriginal = (nID == INTL_LANGUAGE) ? nCurLang : nCurKbd;

    hCB = GetDlgItem(hDlg, nID);

    /*
     *  Get the current data string.
     */
    if ((nCurrent = (short)SendMessage(hCB, CB_GETCURSEL, 0, 0L)) == nOriginal)
        return (TRUE);

    //
    //  Check for invalid selection or error return from User api
    //

    if ((nCurrent == 0xFFFF) || (nCurrent == (short)CB_ERR) ||
        (nCurrent == (short)CB_ERRSPACE))
        return (TRUE);

    pszOption     = (LPTSTR) SendMessage (hCB, CB_GETITEMDATA, nCurrent, 0L);
    pszOptionText = pszOption + lstrlen (pszOption) + 1;
    pszInfFile    = pszOptionText + lstrlen (pszOptionText) + 1;

    if (nID == INTL_LANGUAGE)
    {
        /*
         *  Verify that we can install this option - by checking for
         *  write permission to registry key.
         */
        if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szInstalledLocales,
                           0L, KEY_WRITE, &hkey))
        {
            /*
             *  We can write to the HKEY_LOCAL_MACHINE key, close
             *  and call setup.
             */
            RegCloseKey (hkey);
        }
        else
            goto CannotInstallOption;
    }
    else
    {
        //  See if this LAYOUT option is already installed on system before
        //  calling off to SETUP to install it

        if (CheckOptionInstall (szInstalledLayouts, pszOption))
        {
            //  LAYOUT installed.  Set it for current user

            if (LoadKeyboardLayout (pszOption,
                                    KLF_ACTIVATE | KLF_UNLOADPREVIOUS) != 0)
            {
                return (TRUE);
            }
            else
            {
                //  Load failure, Failsafe is to try to install it again
                goto InstallKeybd;
            }
        }
        else
        {
InstallKeybd:
            if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szInstalledLayouts,
                               0L, KEY_WRITE, &hkey))
            {
                //  We can write to the HKEY_LOCAL_MACHINE key, close
                //  and call setup
                RegCloseKey (hkey);
            }
            else
                goto CannotInstallOption;
        }
    }

    if (!InvokeSetup (hDlg, pszInfFile, pszOption))
        goto FailureExit;

    //
    //  REMOVE THIS FOR PRODUCT11    [julieb  6/23/93]
    //
    //  Set the Locale value in the user's control panel international
    //  section of the registry so that GetThreadLocale will return the
    //  correct result.  This should be removed when locales are on
    //  a per user basis.
    //
    if (nID == INTL_LANGUAGE)
    {
        if (!RegOpenKeyEx (HKEY_CURRENT_USER, szRegCPIntl, 0L, KEY_WRITE, &hkey))
        {
            RegSetValueEx (hkey, szRegLocale, 0L, REG_SZ,
                           (LPBYTE)pszOption, ByteCountOf(lstrlen(pszOption) + 1));
            RegCloseKey (hkey);
        }
    }

    return (TRUE);

CannotInstallOption:
    //  Put up a message box error stating that the USER does
    //  not have correct privilege to install a new option on
    //  the system - contact System Administrator

    MyMessageBox (hDlg, INTL+11, INITS+1, MB_OK | MB_ICONINFORMATION);

    // fall thru...

FailureExit:
    //  If SETUP fails or if User does not have enough permission to
    //  change this option, reset original selection in ComboBox
    SendMessage(hCB, CB_SETCURSEL, nOriginal, 0L);

    return (FALSE);
}


/*
 *  InvokeSetup
 *
 *  Call the SETUP.EXE program to install an option listed in an .INF file.
 *  The SETUP program will make the correct registry entries for this option
 *  under both HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER.  It will set the
 *  new default value for the USER (i.e. a new locale or keyboard layout).
 *
 *
 *  FIX FIX FIX  [stevecat]  Need to make a better choice for SRCDIR and
 *                           STF_LANGUAGE based on some user preference.
 */
BOOL InvokeSetup(
    HWND hDlg,
    LPTSTR pszInfFile,
    LPTSTR pszOption)
{
    TCHAR szCmdSetup[1024];
    TCHAR *pszSetup = TEXT("setup -f -i %s /t STF_LANGUAGE = ENG /t OPTION = %s \
                            /s A:\\  /t ADDCOPY = YES /t DOCOPY = YES  \
                            /c ExternalInstallOption /t DOCONFIG = YES");
    MSG         Msg;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL b;
    DWORD dwExitCode;

    //  Create comamnd line to invoke SETUP program

    wsprintf (szCmdSetup, pszSetup, pszInfFile, pszOption);

//    WinExec (szCmdSetup, SW_SHOW);

    // Create setup process
    memset (&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;

    b = CreateProcess ( NULL,
                        szCmdSetup,         // CommandLine
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        pszSysDir,          // szCurrentDirectory
                        &StartupInfo,
                        &ProcessInformation
                        );
    // If process creation successful, wait for it to
    // complete before continuing

    if ( b )
    {
        EnableWindow (hDlg, FALSE);
        while (MsgWaitForMultipleObjects (
                            1,
                            &ProcessInformation.hProcess,
                            FALSE,
                            (DWORD) -1,
                            QS_ALLEVENTS | QS_SENDMESSAGE) != 0)
        {
        // This message loop is a duplicate of main
        // message loop with the exception of using
        // PeekMessage instead of waiting inside of
        // GetMessage.  Process wait will actually
        // be done in MsgWaitForMultipleObjects api.
        //
            while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }

        }

        if (b = GetExitCodeProcess (ProcessInformation.hProcess, &dwExitCode))
            b = (dwExitCode == 0);
//            b = (dwExitCode == SETUP_ERROR_SUCCESS);

        // Close handles and re-enable window

        CloseHandle (ProcessInformation.hProcess);
        CloseHandle (ProcessInformation.hThread);

        EnableWindow (hDlg, TRUE);

        // Bring Intl window to foreground, always

        SetForegroundWindow (hDlg);
    }

    return (b);
}


BOOL CheckOptionInstall(
    LPTSTR pszRegKey,
    LPTSTR pszOption)
{
    HKEY  hkeyOptions;
    TCHAR szValueName[256];
    TCHAR szValue[256];
    DWORD dwIndex, dwBufz, dwBufv;
    BOOL  b = FALSE;

    if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, pszRegKey, 0L, KEY_READ, &hkeyOptions))
    {
        dwBufz = CharSizeOf(szValueName);
        dwBufv = sizeof(szValue);
        dwIndex = 0;

        while (!RegEnumValue (hkeyOptions, dwIndex++, szValueName, &dwBufz,
                              NULL, NULL, (LPBYTE) szValue, &dwBufv))
        {
            if (!lstrcmpi (pszOption, szValueName))
            {
                //  Found a matching option - now verify that a string
                //  value exists for this "pszOption" valuename
                //  This is done by simply looking for any data.

                if (dwBufv > 1)
                {
                    b = TRUE;
                    break;
                }
            }

            dwBufz = CharSizeOf(szValueName);
            dwBufv = sizeof(szValue);
        }
        RegCloseKey (hkeyOptions);
    }

    return (b);
}


/*
 *  This routine updates the international section of WIN.INI.  If
 *  there are any problems with WriteProfileString (), an error
 *  message is given to the user.
 */
STATIC BOOL WriteIntlWin(
    HWND hDlg)
{
    int      item;               /* iterative */
    TCHAR    szCountry[128];     /* needed for strings storage */
    TCHAR    sziCountry[6];
    TCHAR    sziCurFmt[2];
    TCHAR    sziCurDec[3];
    TCHAR    sziNegCur[3];
    TCHAR    sziLzero[2];
    TCHAR    sziDigits[2];
    TCHAR    sziMeasure[2];
    TCHAR    sziTimeMarker[2];
    TCHAR    sziNegNumber[2];


    /*
     *  Get country code.
     */
    item = (short)SendDlgItemMessage (hDlg, INTL_COUNTRY, CB_GETCURSEL, 0, 0L);
    if (item != CB_ERR)
    {
        SendDlgItemMessage (hDlg, INTL_COUNTRY, CB_GETLBTEXT, item,
                            (LPARAM) szCountry);
    }
    else
        lstrcpy (szCountry, IntlDef.sCountry);

    /*
     *  Change the language and keyboard drivers.
     */
    if (!ChangeDriver (hDlg, INTL_LANGUAGE) || !ChangeDriver (hDlg, INTL_KEYBOARD))
        return (FALSE);

    /*
     *  Show the hour glass.
     */
    HourGlass (TRUE);

    /*
     *  Convert the integers to strings so that they can be written
     *  to the registry.
     */
    MyItoa (Current.iCountry, sziCountry, 10);
    MyItoa (Current.iCurFmt, sziCurFmt, 10);
    MyItoa (Current.iCurDec, sziCurDec, 10);
    MyItoa (Current.iNegCur, sziNegCur, 10);
    MyItoa (Current.iLzero, sziLzero, 10);
    MyItoa (Current.iDigits, sziDigits, 10);
    item = (short)SendDlgItemMessage (hDlg, INTL_MEASUREMENT, CB_GETCURSEL, 0, 0L);
    MyItoa (item, sziMeasure, 10);
    MyItoa (Current.iTimeMarker, sziTimeMarker, 10);
    MyItoa (Current.iNegNumber, sziNegNumber, 10);

    /*
     *  Set language and country information.
     */
    WriteProfileString (szIntl, TEXT("sLanguage"), Current.sLanguage);
    WriteProfileString (szIntl, TEXT("sCountry"), Current.sCountry);
    WriteProfileString (szIntl, TEXT("iCountry"), sziCountry);

    /*
     *  Set list separator and measurement information.
     */
    GetDlgItemText (hDlg, INTL_LISTSEP, Current.sList, CharSizeOf(Current.sList));
    SetLocaleInfo (0, LOCALE_SLIST, Current.sList);
    SetLocaleInfo (0, LOCALE_IMEASURE, sziMeasure);

    /*
     *  Set number format information.
     */
    SetLocaleInfo (0, LOCALE_STHOUSAND, Current.sThousand);
    SetLocaleInfo (0, LOCALE_SDECIMAL, Current.sDecimal);
    SetLocaleInfo (0, LOCALE_IDIGITS, sziDigits);
    SetLocaleInfo (0, LOCALE_ILZERO, sziLzero);
    SetLocaleInfo (0, LOCALE_INEGNUMBER, sziNegNumber);

    /*
     *  Set currency format information.
     */
    SetLocaleInfo (0, LOCALE_SCURRENCY, Current.sCurrency);
    SetLocaleInfo (0, LOCALE_SMONTHOUSANDSEP, Current.sMonThousand);
    SetLocaleInfo (0, LOCALE_SMONDECIMALSEP, Current.sMonDecimal);
    SetLocaleInfo (0, LOCALE_ICURRDIGITS, sziCurDec);
    SetLocaleInfo (0, LOCALE_ICURRENCY, sziCurFmt);
    SetLocaleInfo (0, LOCALE_INEGCURR, sziNegCur);

    /*
     *  Set time format information.
     *
     *  Setting the STIMEFORMAT value also sets the values for
     *  STIME, ITIME, and ITLZERO.
     */
    SetLocaleInfo (0, LOCALE_STIMEFORMAT, Current.sTimeFormat);
    WriteProfileString(szIntl, TEXT("iTimePrefix"), sziTimeMarker);
    SetLocaleInfo (0, LOCALE_S1159, Current.s1159);
    SetLocaleInfo (0, LOCALE_S2359, Current.s2359);

    /*
     *  Set date format information.
     *
     *  Setting the SSHORTDATE value also sets the values for
     *  SDATE and IDATE.
     */
    SetLocaleInfo (0, LOCALE_SSHORTDATE, Current.sShortDate);
    SetLocaleInfo (0, LOCALE_SLONGDATE, Current.sLongDate);

    /*
     *  Stop showing the hour glass.
     */
    HourGlass (FALSE);

    /*
     *  Broadcast the winini change message.
     */
    SendWinIniChange (szIntl);

    /*
     *  Return success.
     */
    return (TRUE);
}


/*
 *  Handle any button pushing or listbox changes in the intl dialog.
 */
STATIC void IntlDlgCommand(
    HWND hDlg,
    int id,
    HWND hWndCtl,
    int codeCtl)
{
    BOOL  bOK;

    switch (id)
    {
      case IDOK:
        HourGlass (TRUE);
        bOK = WriteIntlWin (hDlg);      /* update WIN.INI */
        HourGlass (FALSE);

        if (!bOK)
            break;
/* fall through */
      case IDCANCEL:
        wFontHeight = 0;                /* Clear out global variable */

        //  Free memory from SETUP api calls
        if (pLocales != NULL)
            LocalFree ((HLOCAL) pLocales);

        if (pLayouts != NULL)
            LocalFree ((HLOCAL) pLayouts);

        EndDialog (hDlg, 0L);           /* end this party (was OK) */
        break;

      case INTL_COUNTRY:
        if (codeCtl == CBN_SELCHANGE)
            ChangeCountry (hDlg);
        break;

      case INTL_DATECHANGE:
      case INTL_DATECHANGE2:
      case INTL_DATEACCEL:
        DoDialogBoxParam (DLG_INTLDATE, hDlg, (DLGPROC) DateIntlDlg,
                          IDH_DLG_INTLDATE, 0L);
        SetDateSamples (hDlg, &Current);
        break;

      case INTL_TIMECHANGE:
      case INTL_TIMECHANGE2:
      case INTL_TIMEACCEL:
        DoDialogBoxParam (DLG_INTLTIME, hDlg, (DLGPROC) TimeIntlDlg,
                          IDH_DLG_INTLTIME, 0L);
        SetTimeSamples (hDlg, &Current, FALSE);
        break;

      case INTL_NUMCHANGE:
      case INTL_NUMCHANGE2:
      case INTL_NUMACCEL:
        DoDialogBoxParam (DLG_INTLNUM, hDlg, (DLGPROC) NumIntlDlg,
                          IDH_DLG_INTLNUM, 0L);
        SetNumSamples (hDlg, &Current, FALSE);
        break;

      case INTL_CURCHANGE:
      case INTL_CURCHANGE2:
      case INTL_CURACCEL:
        DoDialogBoxParam (DLG_INTLCUR, hDlg, (DLGPROC) CurIntlDlg,
                          IDH_DLG_INTLCUR, 0L);
        SetCurSamples (hDlg, &Current, FALSE);
        break;

      case INTL_LANGUAGE:
#if KEYBOARDHARDWARECHANGABLE
      case INTL_KEYBOARD:
#endif
      default:
        break;
    }

    UNREFERENCED_PARAMETER(hWndCtl);
}


/*****************************************************/
/******************** public functions ***************/
/*****************************************************/

/*
 *  This routine parses the date format string into the pLDF structure.
 */
void ParseLDF(
    LPTSTR pszLDate,
    PLDF pLDF)
{
    short   i;
    WORD    wLen;
    TCHAR   szBuf[128];
    int     nLen;

    /* Check for a leading day and separator
     * (like "Mon, 26 June")
     */
    wLen = GetLDFToken (pszLDate, szBuf);
#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
    if (((wLen & 0xF0) == LDF_DAY && (wLen & 0x0F) > 2) ||
        ((wLen & 0xF0) == LDF_JaDAY && (wLen & 0x0F) && (wLen & 0x0F) < 3))
#else
    if (((wLen & 0xF0) == LDF_DAY) && ((wLen & 0x0F) > 2))
#endif
    {
        pLDF->Leadin = wLen;
        wLen = GetLDFToken (pszLDate += (wLen & 0x0F), szBuf);
    }
    else
        pLDF->Leadin = 0;

    if ((wLen & 0xF0) >= LDF_SEP)
    {
        nLen = CharSizeOf(pLDF->LeadinSep) - 1;
        _tcsncpy (pLDF->LeadinSep, szBuf, nLen);

        //  Properly NULL terminate string
        pLDF->LeadinSep[nLen] = TEXT('\0');

        wLen = GetLDFToken (pszLDate += (wLen - LDF_SEP), szBuf);
    }
    else
        pLDF->LeadinSep[0] = TEXT('\0');

    /* Get the order of the date parts, the lengths,
     * and the separators
     */
    for (i = 0; i < 3 && wLen; i++)
    {
        switch (wLen & 0xF0)
        {
          case LDF_DAY:
          case LDF_MONTH:
          case LDF_YEAR:
#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
          case LDF_JaYEAR:
#endif
            pLDF->Order[i] = wLen;
            break;

          default:
            pLDF->Order[i] = (WORD) (((MONTH + i) << 4) & 1);
            break;
        }

        /* Get the separator if another field to go;
         * set to null string if no separator
         */
#ifndef JAPAN
        if (i < 2)
#endif
        {
            wLen = GetLDFToken (pszLDate += (wLen & 0x0F), szBuf);
            if ((wLen & 0xF0) >= LDF_SEP)
            {
                nLen = CharSizeOf(pLDF->Sep[0]) - 1;

                _tcsncpy (pLDF->Sep[i], szBuf, nLen);

                //  Properly NULL terminate string
                pLDF->Sep[i][nLen] = TEXT('\0');

                wLen = GetLDFToken (pszLDate += (wLen - LDF_SEP), szBuf);
            }
            else
                pLDF->Sep[i][0] = TEXT('\0');
        }
    }

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
        // LONG_DATE_FORMAT
    // see trailing 'day of the week' for Japanese
    pLDF->Trailin = 0;
    if (!pLDF->Leadin)
    {
        wLen = GetLDFToken (pszLDate, szBuf);
        if (((wLen & 0xF0) == LDF_DAY && (wLen & 0x0F) > 2) ||
            ((wLen & 0xF0) == LDF_JaDAY && (wLen & 0x0F) && (wLen & 0x0F) < 3))
        {
            pLDF->Leadin = wLen;
            pLDF->Trailin = 1;
        }
    }
    if (wLen = pLDF->Leadin)
    {
        if ((wLen & 0x0F) <= 2)
            pLDF->Leadin += 2;  // offset japanese day format
    }
#endif

}

/*
 *  This is the dialog procedure for the international dialog.
 */
BOOL APIENTRY IntlDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
        // Set initial font height is required since dialog box doesn't
        // contains font information.
        if (!wFontHeight)
        {
            TEXTMETRIC TM;
            HDC hDC = GetDC(hDlg);

            GetTextMetrics(hDC, &TM);
            ReleaseDC(hDlg, hDC);
            wFontHeight = (WORD) TM.tmHeight;
        }
#endif
        {
            BOOL bSuccess;

            HourGlass (TRUE);
            bSuccess = InitIntlDlg (hDlg);
            HourGlass (FALSE);

            if (!bSuccess)
                EndDialog (hDlg, OUT_OF_MEM);
            break;
        }

    case WM_COMMAND:
        if (LOWORD (wParam) == IDD_HELP)
            goto DoHelp;
        else
            IntlDlgCommand (hDlg, LOWORD(wParam), (HWND) lParam, (LONG) HIWORD(wParam));
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
        }
        else
            return (FALSE);
    }
    return (TRUE);
}

