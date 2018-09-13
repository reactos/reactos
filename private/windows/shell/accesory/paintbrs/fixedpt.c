/****************************Module*Header******************************\
* Module Name: fixedpt.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include   <windows.h>
#include "port1632.h"

//#define    NOEXTERN
#include   "pbrush.h"
#include   "fixedpt.h"

extern int uAttrUnit;

extern HWND pbrushWnd[];
TCHAR   cDecimal;
TCHAR   cThousand;
BOOL    iLzero;
int     iDigits;
TCHAR   szIntl[] = TEXT("intl");
TCHAR    sDecimal[] = TEXT("sDecimal");
TCHAR   szLzero[] = TEXT("iLzero");
TCHAR   szDigits[] = TEXT("iDigits");
TCHAR   sThousand[] = TEXT("sThousand");
TCHAR   szMeasure[] = TEXT("iMeasure");
int     iMeasure = 1;   /* default system of measurement is English, 0 for metric */

void FAR InitDecimal(LPTSTR szkey)
{
    TCHAR szVal[3];

    if (!szkey || !lstrcmpi(szkey, szIntl))
    {
        iMeasure = GetProfileInt(szIntl, szMeasure, 1);
        if (uAttrUnit != IDPELS)
            uAttrUnit = iMeasure ? IDIN : IDCM;
        iLzero = !!GetProfileInt(szIntl, szLzero, 1);
#ifdef JAPAN                // added by Hiraisi  19 May. 1992
        iDigits = 2;
#else
        iDigits = GetProfileInt(szIntl, szDigits, 2);
#endif
        GetProfileString(szIntl, sDecimal, TEXT("."), szVal, CharSizeOf(szVal)); cDecimal = *szVal;
        GetProfileString(szIntl, sThousand, TEXT(","), szVal, CharSizeOf(szVal)); cThousand = *szVal;
    }
}

LONG NumToPels(LONG lNum, BOOL bHoriz, BOOL bInches)
{
   int nLogPixels;
   HDC   hDC;

   hDC = GetDisplayDC(pbrushWnd[PARENTid]);
   nLogPixels = GetDeviceCaps(hDC, bHoriz ? LOGPIXELSX : LOGPIXELSY);
   ReleaseDC(pbrushWnd[PARENTid], hDC);

   if (!bInches)
      lNum = CMToInches(lNum);

   return lNum * nLogPixels;
}

LONG PelsToNum(LONG lNum, BOOL bHoriz, BOOL bInches)
{
   int nLogPixels;
   HDC   hDC;
   LONG  lResult;

   hDC = GetDisplayDC(pbrushWnd[PARENTid]);
   nLogPixels = GetDeviceCaps(hDC, bHoriz ? LOGPIXELSX : LOGPIXELSY);
   ReleaseDC(pbrushWnd[PARENTid], hDC);

   lResult = (lNum + (nLogPixels >> 1)) / nLogPixels;
   if (!bInches)
      lResult = InchesToCM(lResult);

   return lResult;
}

BOOL StrToNum(LPTSTR num, LONG FAR *lpNum)
{
   LPTSTR s;
   LONG  lNum = 0;
   BOOL  fSign;

   // Note that the fixed-pt scheme assumes a factor of 100, so don't heed
   // the Intl iDigits value here!

   /* assume we have an invalid number */
   *lpNum = -1;


   /* find the decimal point or EOS */
   for (s = num; *s && *s != cDecimal; ++s)
      ;

   /* add enough zeros on end of string */
   lstrcat(num, TEXT("00"));

   /* delete decimal point from string, and any excessive digits */
   s[3] = TEXT('\0');
   if (*s == cDecimal)
      lstrcpy(s, s + 1);


   /* find beginning of number */
   for (s = num; *s == TEXT(' ') || *s == TEXT('\t'); ++s)
      ;

   /* save sign */
   if (*s == TEXT('-')) {
       fSign = TRUE;
       ++s;
   } else
       fSign = FALSE;

   /* convert the number to a long; note that it must skip over cThousand */
   while (*s) {
      if (*s >= TEXT('0') && *s <= TEXT('9'))
         lNum = lNum * 10 + *s - TEXT('0');
      else if (*s != cThousand)
         return FALSE;
      s++;
   }

   /* negate result if we saw negative sign */
   if (fSign)
       lNum = -lNum;

   *lpNum = lNum;
   return TRUE;
}

BOOL GetDlgItemNum(HWND hDlg, int nItemID, LONG FAR * lpNum)
{
   TCHAR num[20];

   /* get the edit text */
   if (!GetDlgItemText(hDlg, nItemID, num, 20))
      return FALSE;

   return StrToNum(num, lpNum);
}

BOOL NumToStr(LPTSTR num, LONG lNum, BOOL bDecimal)
{
   TCHAR szFormat[10];
   short sFloor = FLOOR(lNum);
   *num = TEXT('\0');

   // safe assumption: FLOOR(lNum) won't be in the millions...

   if (bDecimal)
   {
        if (iLzero || sFloor != 0)
        {
                if (sFloor >= 1000)
                        wsprintf(num, TEXT("%d%c%03d"), sFloor / 1000, cThousand, sFloor % 1000);
                else
                        wsprintf(num, TEXT("%d"), sFloor);
        }

        wsprintf(szFormat, TEXT("%d"), NumRemToShort(lNum));
        szFormat[iDigits] = TEXT('\0');
        wsprintf(num+lstrlen(num), TEXT("%c%s"), cDecimal, szFormat);
   }
   else
   {
        sFloor = ROUND(lNum);

        if (sFloor > 1000)
                wsprintf(num, TEXT("%d%c%03d"), sFloor / 1000, cThousand, sFloor % 1000);
        else
                wsprintf(num, TEXT("%d"), sFloor);
   }

   return TRUE;
}

BOOL SetDlgItemNum(HWND hDlg, int nItemID, LONG lNum, BOOL bDecimal)
{
   TCHAR  num[20];

   NumToStr(num, lNum, bDecimal);
   SetDlgItemText(hDlg, nItemID, num);

   return TRUE;
}
