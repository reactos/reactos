#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

/* Page margins values are always stored in inches.
 * chPageText[ID_LEFT .. ID_BOTTOM] contain the strings used to display the
 * margin values in the dlg box.
 * When the system of measurement is metric, margins are displayed in CMs,
 * validated in CMs, but are converted to inches before being stored in
 * chPageText[].
 * For precision, arithmetic with margins values is done after multiplying
 * by BASE and them dividing by 100.
 */
NOEXPORT int NEAR CheckMarginNums(
    HWND hWnd);

NOEXPORT BOOL NEAR SetDlgItemNum(
    HWND hDlg,
    int nItemID,
    LONG lNum,
    BOOL bDecimal);

NOEXPORT BOOL NEAR StrToNum(
    LPTSTR lpNumStr,
    LONG FAR *lpNum);

NOEXPORT BOOL NEAR NumToStr(
    LPTSTR lpNumStr,
    LONG lNum,
    BOOL bDecimal);

/* define a type for NUM and the base */
typedef long NUM;
#define BASE 100L

/* converting in/out of fixed point */
#define  NumToShort(x,s)   (LOWORD(((x) + (s)) / BASE))
#define  NumRemToShort(x)  (LOWORD((x) % BASE))

/* rounding options for NumToShort */
#define  NUMFLOOR      0
#define  NUMROUND      (BASE/2)
#define  NUMCEILING    (BASE-1)

#define  ROUND(x)  NumToShort(x,NUMROUND)
#define  FLOOR(x)  NumToShort(x,NUMFLOOR)

/* Unit conversion */
#define  InchesToCM(x)  (((x) * 254L + 50) / 100)
#define  CMToInches(x)  (((x) * 100L + 127) / 254)


/*
 * convert floating point strings (like 2.75 1.5 2) into number of pixels
 * given the number of pixels per CM
 */
long CMToPixels(
    TCHAR *ptr,
    int pix_per_CM)
{
    TCHAR *dot_ptr;
    TCHAR sz[20];
    int decimal;

    lstrcpy(sz, ptr);
    dot_ptr = _tcschr(sz, szDec[0]);
    if (dot_ptr)
    {
        *dot_ptr++ = 0;        /* terminate the inches */
        if (*(dot_ptr + 1) == 0)
        {
            *(dot_ptr + 1) = TEXT('0');   /* convert decimal part to hundredths */
            *(dot_ptr + 2) = 0;
        }
        decimal = ((int)MyAtol(dot_ptr) * pix_per_CM) / 100;    /* first part */
    }
    else
        decimal = 0;        /* there is not fraction part */

    return (MyAtol(sz) * pix_per_CM) + decimal;     /* second part */
}


/*
 * dialog procedure for page setup
 *
 * this guy sets the global variables that define how printing is too be done
 * (ie margins, headers, footers)
 */
int PageSetupDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LONG lParam)
{
    int id;         /* ID of dialog edit controls */
    long Num;
    TCHAR szSpaceText[32];
    /* store the value of system of measurement currently in use. Shouldn't
     * change when the dlg is up */

    switch (msg)
    {
        case WM_INITDIALOG:

            for (id = ID_HEADER; id <= ID_FOOTER; id++)
            {
                SendDlgItemMessage(hwnd, id, EM_LIMITTEXT, PT_LEN-1, 0L);
                SetDlgItemText(hwnd, id, chPageText[id - ID_HEADER]);
            }
            for (id = ID_LEFT; id <= ID_BOTTOM; id++)
            {
                SendDlgItemMessage(hwnd, id, EM_LIMITTEXT, 4, 0L);
                if (!fEnglish)
                {
                    StrToNum(chPageText[id-ID_HEADER], &Num);
                    Num = InchesToCM(Num);
                    SetDlgItemNum(hwnd, id, Num, TRUE);
                }
                else
                    SetDlgItemText(hwnd, id, chPageText[id-ID_HEADER]);
            }
            LoadString((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                       (fEnglish ? IDS_SPACEISINCH : IDS_SPACEISCENTI),
                       szSpaceText, CharSizeOf(szSpaceText) );
            SetDlgItemText(hwnd, ID_SPACE, szSpaceText);
            SendDlgItemMessage(hwnd, ID_HEADER, EM_SETSEL, 0, MAKELONG(0, PT_LEN-1));
            return(TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    /* Check if margin values are valid. */
                    id = CheckMarginNums(hwnd);
                    if ( id <= 0)   /* invalid */
                    {
                        MessageBox(hwnd, szMarginError, szCardfile, MB_OK | MB_ICONEXCLAMATION);
                        if (id == 0)    /* can't guess which margin is invalid */
                            return TRUE;    /* continue the dialog */
                        else if (id < 0)    /* -id is the ID of the child with invalid value */
                        {
                            SetFocus(GetDlgItem(hwnd, -id));
                            return FALSE;
                        }
                    }
                    /* store the changes made only if valid. */
                    for (id = ID_HEADER; id <= ID_FOOTER; id++)
                        GetDlgItemText(hwnd, id, chPageText[id-ID_HEADER], PT_LEN);
                    for (id = ID_LEFT; id <= ID_BOTTOM; id++)
                    {
                        GetDlgItemText(hwnd, id, chPageText[id-ID_HEADER], PT_LEN);
                        if (!fEnglish)  /* metric system, convert to inches before storing */
                            if (StrToNum(chPageText[id-ID_HEADER], &Num))
                                NumToStr(chPageText[id-ID_HEADER], CMToInches(Num), TRUE);
                    }

                    /* fall through... */

                case IDCANCEL:
                    EndDialog(hwnd, 0); // lhb tracks
                    break;
                }
            return(TRUE);
    }

    return(FALSE);
}

/* Check validity of margin values specified.
 *  return TRUE if margins are valid.
 *
 *  returns  -ID_LEFT if Left margin is invalid,
 *           -ID_RIGHT if Right margin is invalid
 *           -ID_TOP   if Top margin is invalid
 *           -ID_BOTTOM if Bottom margin is invalid
 *           FALSE if it cannot guess the invalid margin
 */
NOEXPORT int NEAR CheckMarginNums(
    HWND hWnd)
{
    short   n;
    TCHAR    *pStr;
    TCHAR    szStr[PT_LEN];
    long    Left, Right, Top, Bottom;
    HANDLE  hPrintDC;
    int     xPixelsPerInch, yPixelsPerInch, xPrintRes, yPrintRes;
    int xPixelsPerCM;    /* Horz Pixels per CM */
    int yPixelsPerCM;    /* Vert Pixels per CM */
    int xSizeMM;        /* Horz size in millimetres */
    int ySizeMM;        /* Vert size in millimeters */

    for (n = ID_HEADER+2; n <= ID_BOTTOM; n++)
    {
        GetDlgItemText(hWnd, n, szStr, PT_LEN);
        pStr = szStr;

        while (*pStr)
        {
            if (_istdigit(*pStr) || *pStr == szDec[0])
                pStr = CharNext(pStr);
            else
                return (-n);
        }
    }

    if (!(hPrintDC = GetPrinterDC()))
        return TRUE;    /* can't do any range check, assume OK */

    xPrintRes = GetDeviceCaps(hPrintDC, HORZRES);
    yPrintRes = GetDeviceCaps(hPrintDC, VERTRES);
    if (fEnglish == 0)
    {
        xSizeMM = GetDeviceCaps(hPrintDC, HORZSIZE);
        ySizeMM = GetDeviceCaps(hPrintDC, VERTSIZE);
        xPixelsPerCM = (xPrintRes/xSizeMM)/10;
        yPixelsPerCM = (yPrintRes/ySizeMM)/10;
    }
    else
    {
        xPixelsPerInch  = GetDeviceCaps(hPrintDC, LOGPIXELSX);
        yPixelsPerInch  = GetDeviceCaps(hPrintDC, LOGPIXELSY);
    }

    DeleteDC(hPrintDC);

    /* margin values have int/float values. Do range check */
    GetDlgItemText(hWnd, ID_LEFT, szStr, PT_LEN);
    Left     = (fEnglish == 0) ? CMToPixels(szStr, xPixelsPerCM):
                                atopix(szStr,xPixelsPerInch);

    GetDlgItemText(hWnd, ID_RIGHT, szStr, PT_LEN);
    Right    = (fEnglish == 0) ? CMToPixels(szStr, xPixelsPerCM):
                                atopix(szStr, xPixelsPerInch);

    GetDlgItemText(hWnd, ID_TOP, szStr, PT_LEN);
    Top      = (fEnglish == 0) ? CMToPixels(szStr, yPixelsPerCM):
                                atopix(szStr, yPixelsPerInch);

    GetDlgItemText(hWnd, ID_BOTTOM, szStr, PT_LEN);
    Bottom   = (fEnglish == 0) ? CMToPixels(szStr, yPixelsPerCM):
                                atopix(szStr, yPixelsPerInch);

    /* try to guess the invalid margin */
    if (Left >= xPrintRes)
        return -ID_LEFT;            /* Left margin is invalid */
    else if (Right >= xPrintRes)
        return -ID_RIGHT;           /* Right margin is invalid */
    else if (Top >= yPrintRes)
        return -ID_TOP;             /* Top margin is invalid */
    else if (Bottom >= yPrintRes)
        return -ID_BOTTOM;          /* Bottom margin is invalid */
    else if (Left >= (xPrintRes-Right))
        return FALSE;                   /* can't guess, return FALSE */
    else if (Top >= (yPrintRes-Bottom))
        return FALSE;                   /* can't guess, return FALSE */

    return TRUE;
}


NOEXPORT BOOL NEAR StrToNum(LPTSTR lpNumStr, LONG FAR *lpNum)
{
   LPTSTR s;
   TCHAR szNum[10];
   LONG  lNum = 0;
   BOOL  fSign;

   lstrcpy(szNum, lpNumStr);
   /* assume we have an invalid number */
   *lpNum = -1;

   /* find the decimal point or EOS */
   for (s = szNum; *s && *s != szDec[0]; ++s)
      ;

   /* add two zeros on end of string */
   lstrcat(szNum, TEXT("00"));

   /* move decimal point right two places */
   s[3] = TEXT('\0');
   if (*s == szDec[0])
      lstrcpy(s, s + 1);

   /* find beginning of number */
   for (s = szNum; *s == TEXT(' ') || *s == TEXT('\t'); ++s)
      ;

   /* save sign */
   if (*s == TEXT('-')) {
       fSign = TRUE;
       ++s;
   } else
       fSign = FALSE;

   /* convert the number to a long */
   while (*s) {
      if (*s < TEXT('0') || *s > TEXT('9'))
         return FALSE;

      lNum = lNum * 10 + *s++ - TEXT('0');
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

NOEXPORT BOOL NEAR NumToStr(LPTSTR lpNumStr, LONG lNum, BOOL bDecimal)
{
   if (bDecimal)
      wsprintf(lpNumStr, TEXT("%d%c%02d"), FLOOR(lNum), szDec[0], NumRemToShort(lNum));
   else
      wsprintf(lpNumStr, TEXT("%d"), ROUND(lNum));

   return TRUE;
}

NOEXPORT BOOL NEAR SetDlgItemNum(
    HWND hDlg,
    int nItemID,
    LONG lNum,
    BOOL bDecimal)
{
   TCHAR  num[20];

   NumToStr(num, lNum, bDecimal);
   SetDlgItemText(hDlg, nItemID, num);

   return TRUE;
}
