#define NOMINMAX
#define WIN31
#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/
typedef  VOID (FAR PASCAL *PENAPPPROC)(WORD, BOOL);
int NEAR PASCAL LogHimetricToPels(HDC hDC,BOOL bHoriz, int mmHimetric);

extern TCHAR szCardfileSect[];

int   fEnglish = 1; // default system of measurement is English, 0 for metric

int fNeedToUpdateObject = FALSE;
DWORD   CdHt    =   0;
DWORD   CdWd    =   0;
BOOL    fOLE;
HDC     hDisplayDC;


// from picture.c
extern BOOL fDropOperationFailure ;

/* use this buffer for reading in text from a card or as a scratch buffer */
TCHAR szText[CARDTEXTSIZE];

/* Let Cardfile save font values to the registry.*/
/* name of section to save into -- never internationalize */
#define OURKEYNAME TEXT("Software\\Microsoft\\Cardfile")

// RegWriteInt - write an integer to the registry

VOID RegWriteInt( HKEY hKey, PTCHAR pszKey, INT iValue )
{
    RegSetValueEx( hKey, pszKey, 0, REG_DWORD, (BYTE*)&iValue, sizeof(INT) );
}

// RegWriteString - write a string to the registry

VOID RegWriteString( HKEY hKey, PTCHAR pszKey, PTCHAR pszValue )
{
    INT len;     // length of string with null in bytes

    len= (lstrlen( pszValue )+1) * sizeof(TCHAR);
    RegSetValueEx( hKey, pszKey, 0, REG_SZ, (BYTE*)pszValue, len );
}

// RegGetInt - Get integer from registry

DWORD RegGetInt( HKEY hKey, PTCHAR pszKey, DWORD dwDefault )
{
    DWORD dwResult= !ERROR_SUCCESS;
    LONG  lStatus;
    DWORD dwSize= sizeof(DWORD);
    DWORD dwType;

    if( hKey )
    {
        lStatus= RegQueryValueEx( hKey,
                                  pszKey,
                                  NULL,
                                  &dwType,
                          (BYTE*) &dwResult,
                                  &dwSize );
    }

    if( lStatus != ERROR_SUCCESS || dwType != REG_DWORD )
    {
        dwResult= dwDefault;
    }
    return( dwResult );
}

// RegGetString - get string from registry

VOID RegGetString( HKEY hKey, PTCHAR pszKey, PTCHAR pszDefault, PTCHAR pszResult, INT iCharLen )
{
    LONG  lStatus= !ERROR_SUCCESS;
    DWORD dwSize;      // size of buffer
    DWORD dwType;

    dwSize= iCharLen * sizeof(TCHAR);

    if( hKey )
    {
        lStatus= RegQueryValueEx( hKey,
                                  pszKey,
                                  NULL,
                                  &dwType,
                          (BYTE*) pszResult,
                                  &dwSize );
    }

    if( lStatus != ERROR_SUCCESS || dwType != REG_SZ )
    {
        CopyMemory( pszResult, pszDefault, iCharLen*sizeof(TCHAR) );
    }
}



BOOL fUnicodeFont=FALSE;     // true if unicode font found

VOID VerifyUnicodeFont()
{
    TCHAR   szBuf[200],szOutbuf[200+LF_FACESIZE]; // message buffers

    // give user a warning if we can't create  the unicode font or
    // the font gets mapped to a non-unicode font.
    // actually, we just want to know if it is a unicode font, but
    // there is no easy way to know this.
    if( !fUnicodeFont )
    {
       LoadString( hIndexInstance, E_NOUNICODEFONT, szBuf, CharSizeOf(szBuf) );
       wsprintf( szOutbuf, szBuf, UNICODE_FIXED_FONT_NAME );
       MessageBox( NULL, szOutbuf, szWarning, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    }
}

// Save current global font data to the registry.
VOID SaveGlobals(VOID) {
    HKEY hKey; // key to our registry root
    LONG lStatus; // status from RegCreateKey

    lStatus= RegCreateKey( HKEY_CURRENT_USER, OURKEYNAME, &hKey );
    if( lStatus != ERROR_SUCCESS ) {
        return; // too bad
    }

    RegWriteInt( hKey, TEXT("lfEscapement"),     FontStruct.lfEscapement);
    RegWriteInt( hKey, TEXT("lfOrientation"),    FontStruct.lfOrientation);
    RegWriteInt( hKey, TEXT("lfWeight"),         FontStruct.lfWeight);
    RegWriteInt( hKey, TEXT("lfItalic"),         FontStruct.lfItalic);
    RegWriteInt( hKey, TEXT("lfUnderline"),      FontStruct.lfUnderline);
    RegWriteInt( hKey, TEXT("lfStrikeOut"),      FontStruct.lfStrikeOut);
    RegWriteInt( hKey, TEXT("lfCharSet"),        FontStruct.lfCharSet);
    RegWriteInt( hKey, TEXT("lfOutPrecision"),   FontStruct.lfOutPrecision);
    RegWriteInt( hKey, TEXT("lfClipPrecision"),  FontStruct.lfClipPrecision);
    RegWriteInt( hKey, TEXT("lfQuality"),        FontStruct.lfQuality);
    RegWriteInt( hKey, TEXT("lfPitchAndFamily"), FontStruct.lfPitchAndFamily);
    RegWriteInt( hKey, TEXT("iPointSize"),       iPointSize);

    RegWriteString( hKey, TEXT("lfFaceName"), FontStruct.lfFaceName);

}

// Read global font data from the registry. If the registry doesn't have the data,
// fall back to the unicode font. If that isn't installed, use a regular fixed
//font.
VOID GetGlobals (VOID) {
    LOGFONT lfDef;     // default logical font, hopefully UNICODE
    HFONT   hFont;     // standard font to use
    LONG    lStatus;   // Status from RegCreateKey
    HKEY    hKey;      // key into registry
    TCHAR   szFaceName[LF_FACESIZE+1];
   /* initialize the Unicode font */
    GetObject( GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lfDef );
	 lfDef.lfHeight= -( INITPOINTSIZE * GetDeviceCaps(hDisplayDC,LOGPIXELSY) ) / 720;
    lfDef.lfCharSet= ANSI_CHARSET;
    lfDef.lfWidth= 0;              // use native width blah blah blah
    lstrcpy( lfDef.lfFaceName, UNICODE_FIXED_FONT_NAME );
    hFont= CreateFontIndirect( &lfDef );

    // get name of actual font that will be used on screen
    if( hFont )
    {
        HFONT hPrevFont;          // previously selected font from screen dc
        hPrevFont= (HFONT)SelectObject( hDisplayDC, hFont );
        GetTextFace( hDisplayDC, CharSizeOf(szFaceName), szFaceName );
        SelectObject( hDisplayDC, hPrevFont );
    }

    // give user a warning if we can't create or mapped font isn't unicode
    // actually, we just want to know if it is a unicode font, but
    // there is no easy way to know this.
    fUnicodeFont= TRUE;
    if (hFont == NULL || lstrcmp(szFaceName, lfDef.lfFaceName) )
    {
       fUnicodeFont= FALSE;
       if( hFont )          // delete font if wrong one
       {
           DeleteObject( hFont );
       }

       // Use the system font if we can't get the unicode font
       // The system font better have a iPointSize of INITPOINTSIZE

       hFont = (HFONT) GetStockObject (SYSTEM_FIXED_FONT);
       GetObject( hFont, sizeof(LOGFONT), &lfDef );
    }

    // Now read the registry keys, backed up by lfDef defaults

    lStatus= RegCreateKey( HKEY_CURRENT_USER, OURKEYNAME, &hKey );
    if( lStatus != ERROR_SUCCESS )
    {
        hKey= NULL;   // later calls to RegGet... will return defaults
    }
    FontStruct.lfWidth= lfDef.lfWidth;

    FontStruct.lfEscapement=     (LONG)RegGetInt( hKey, TEXT("lfEscapement"),     lfDef.lfEscapement);
    FontStruct.lfOrientation=    (LONG)RegGetInt( hKey, TEXT("lfOrientation"),    lfDef.lfOrientation);
    FontStruct.lfWeight=         (LONG)RegGetInt( hKey, TEXT("lfWeight"),         lfDef.lfWeight);
    FontStruct.lfItalic=         (BYTE)RegGetInt( hKey, TEXT("lfItalic"),         lfDef.lfItalic);
    FontStruct.lfUnderline=      (BYTE)RegGetInt( hKey, TEXT("lfUnderline"),      lfDef.lfUnderline);
    FontStruct.lfStrikeOut=      (BYTE)RegGetInt( hKey, TEXT("lfStrikeOut"),      lfDef.lfStrikeOut);
    FontStruct.lfCharSet=        (BYTE)RegGetInt( hKey, TEXT("lfCharSet"),        lfDef.lfCharSet);
    FontStruct.lfOutPrecision=   (BYTE)RegGetInt( hKey, TEXT("lfOutPrecision"),   lfDef.lfOutPrecision);
    FontStruct.lfClipPrecision=  (BYTE)RegGetInt( hKey, TEXT("lfClipPrecision"),  lfDef.lfClipPrecision);
    FontStruct.lfQuality=        (BYTE)RegGetInt( hKey, TEXT("lfQuality"),        lfDef.lfQuality);
    FontStruct.lfPitchAndFamily= (BYTE)RegGetInt( hKey, TEXT("lfPitchAndFamily"), lfDef.lfPitchAndFamily);

    RegGetString( hKey, TEXT("lfFaceName"), lfDef.lfFaceName, FontStruct.lfFaceName, LF_FACESIZE);

    iPointSize= RegGetInt( hKey, TEXT("iPointSize"), INITPOINTSIZE);
    FontStruct.lfHeight= -( iPointSize * GetDeviceCaps(hDisplayDC,LOGPIXELSY) ) / 720;
    hFont = CreateFontIndirect (&FontStruct);
    SetGlobalFont (hFont, iPointSize);
	
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpAnsiCmdLine,
    int cmdShow)
{
    MSG msg;
    HDC hDC;
    VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;

    LPTSTR  lpCmdLine = GetCommandLine ();
    TCHAR   szBuf[12];

#if DBG
    GdiSetBatchLimit( 1 );   // disable batching
#endif

    hDisplayDC= CreateDC( TEXT("DISPLAY"), NULL, NULL, NULL );

#if !defined( WIN32 )
    fOLE = GetWinFlags() & WF_PMODE;    /* Are we in real mode today? */
#else
    fOLE = TRUE;    // No real mode on NT
#endif
    Hourglass(TRUE);

    hIndexInstance = hInstance;

    fEnglish= GetProfileInt( TEXT("intl"), TEXT("iMeasure"), 1 );

    //GetUnicodeFont();
    GetGlobals();
    if (!hPrevInstance)
    {
        if (!IndexInit())
        {
            Hourglass(FALSE);
            goto InitError;
        }
    }
    else
        GetOldData(hPrevInstance);

    /* Create a seperate hbrCard and hbrBorder for each instance */
    if(!(hbrCard   = CreateSolidBrush(GetSysColor(COLOR_WINDOW))) ||
        !(hbrBorder = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME))))
        goto InitError;

    if (InitInstance(hInstance, SkipProgramName (lpCmdLine), cmdShow))
    {
        Hourglass(FALSE);
        VerifyUnicodeFont();
        if (lpfnRegisterPenApp = (PENAPPPROC)GetProcAddress((HINSTANCE) GetSystemMetrics (SM_PENWINDOWS),
                                                            "RegisterPenApp"))
            (*lpfnRegisterPenApp)(1, TRUE);

        while(TRUE)
        {
            if (IsWindow(hEditWnd) && !PeekMessage(&msg, hEditWnd, WM_KEYFIRST, WM_KEYLAST, FALSE))
            {
                if (fNeedToUpdateObject)
                {
                    hDC = GetDC(hEditWnd);
                    SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
                    SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
                    CardPaint(hDC);
                    ReleaseDC(hEditWnd, hDC);
                    fNeedToUpdateObject = FALSE;
                }
            }
            if (!ProcessMessage(hIndexWnd, hAccel))
                break;
        }
        if (lpfnRegisterPenApp)
            (*lpfnRegisterPenApp)(1, FALSE);

        Hourglass(TRUE);
    }
    else
    {
InitError:
        MessageBox(NULL, NotEnoughMem, NULL, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
    }
    /* Free any global memory used by Printer Setup. */
    if(PD.hDevMode)
        GlobalFree(PD.hDevMode);
    if(PD.hDevNames)
        GlobalFree(PD.hDevNames);

    wsprintf(szBuf, TEXT("%d"),  fValidate);
    WriteProfileString (szCardfileSect, szValidateFileWrite, szBuf);
    Hourglass(FALSE);
    return(0);
}

/*
 * Set card count on the right top corner
 */
void SetNumOfCards(
    void)
{
    TCHAR szString[20];
    TCHAR szWndText[50];

    if (cCards == 1)
        LoadString(hIndexInstance, IONECARD, szWndText, 10);
    else
    {
        LoadString(hIndexInstance, ICARDS, szString, 10);
        wsprintf(szWndText, TEXT("%d %s"), cCards, szString);
    }
    SetWindowText(hRightWnd, szWndText);
}

/* Why is it here?  Because the DWORD mul/div routines are in _TEXT */
void FixBounds(
    LPRECT lprc)
{
    HDC hDC;
    DWORD xDiff;
    DWORD yDiff;

    if (!CdHt)
    {
        CdHt = (CARDLINES * CharFixHeight);
        CdWd = (LINELENGTH * CharFixWidth);
    }
    /* First map from HIMETRIC back to screen coordinates */
    hDC = GetDC(NULL);
    lprc->right  = LogHimetricToPels(hDC, TRUE,  lprc->right);
    lprc->bottom = LogHimetricToPels(hDC, FALSE, -lprc->bottom);

    ReleaseDC(NULL, hDC);

    /* Preserve the Aspect Ratio of the picture */
    xDiff = (DWORD) (lprc->right - lprc->left + 1);
    yDiff = (DWORD) (lprc->bottom - lprc->top + 1);

    /* Don't use *= here because of integer arithmetic... */
    if ((xDiff > CdWd) || (yDiff > CdHt))
    {
        if ((xDiff * CdHt) > (yDiff * CdWd))
        {
            yDiff = ((yDiff * CdWd) / xDiff);
            xDiff = CdWd;
        }
        else
        {
            xDiff = ((xDiff * CdHt) / yDiff);
            yDiff = CdHt;
        }
    }
    SetRect(lprc, 0, 0, (int)xDiff - 1, (int)yDiff - 1);
}

/* ProcessMessage() - Spin in a message dispatch loop.
 */
BOOL ProcessMessage(
    HWND hwndFrame,
    HANDLE hAccTable)
{
    BOOL    fReturn;
    MSG     msg;

    if (fReturn = GetMessage((LPMSG)&msg, NULL, 0, 0))
    {
        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg))
        {
            if (!TranslateAccelerator(hIndexWnd, hAccel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    return fReturn;
}

INT Scale(
    INT coord,
    INT s1,
    INT s2)
{
    return ((INT) (((LONG)coord * (LONG)s1) / (LONG)s2));
}

BOOL IndexOkError(
    int strid)
{
    TCHAR  buf[300];
    LPTSTR lpbuf;

    if (strid == EINSMEMORY)
        lpbuf = NotEnoughMem;
    else if(!LoadString (hIndexInstance, strid, buf, 300))
        lpbuf = NotEnoughMem;
    else if (strid == E_FLOPPY_WITH_SOURCE_REMOVED)
      {
        WORD wLen = lstrlen(buf);

        if(!LoadString(hIndexInstance, strid + 1, buf + wLen, 300 - wLen))
            lpbuf = NotEnoughMem;
        else
            lpbuf = buf;
      }
    else
        lpbuf = buf;
    MessageBox(hIndexWnd, lpbuf, szWarning, MB_OK | MB_ICONEXCLAMATION);
    return FALSE; /* caller can return this to indicate failure */
}

/* Scan lpSrc for merge spec.
 * If found, insert string lpMerge at that point.
 * Then append rest of lpSrc.
 * NOTE! Merge spec guaranteed to be two chars.
 *     returns TRUE if it does a merge, false otherwise.
 */
BOOL MergeStrings(
    LPTSTR lpSrc,
    LPTSTR lpMerge,
    LPTSTR lpDst)
{
    LPTSTR lpMatch;
    int nChars;

    /* Find merge spec */
    if (!(lpMatch = _tcsstr (lpSrc, szMerge)))
        return FALSE;       /* none found */

    /* copy from src to dest, upto the merge spec char */
    nChars = lpMatch - lpSrc;
    _tcsncpy(lpDst, lpSrc, nChars);
    lpDst += nChars;

    /* If merge spec found, insert string to be merged. */
    if (lpMerge)
    {
        lstrcpy(lpDst, lpMerge);
        lpDst += lstrlen(lpMerge);
    }

    /* copy rest of the source after the merge spec */
    lstrcpy(lpDst, lpMatch+2);
    return TRUE;
}

void MakeBlankCard(
    void)
{
    CurCardHead.line[0] = (TCHAR) 0;
    SavedIndexLine[0] = (TCHAR) 0;
    szText[0] = (TCHAR) 0;
    CurCard.lpObject = NULL;
    SetEditText (TEXT(""));
    CurCardHead.flags = FNEW;
}

void SetCaption(
    void)
{
    TCHAR buf[100];

    BuildCaption(buf, CharSizeOf(buf));
    SetWindowText(hIndexWnd, buf);
}

/*
 * build a string to display in the caption bar
 *
 * note: the caption is used in the printing options.  check there
 * before you change this.
 *
 */
void BuildCaption( TCHAR *pBuf, WORD wLen )
{
    LPTSTR pFile;

    pFile = (*CurIFile) ? FileFromPath(CurIFile) : szUntitled;
    wsprintf(pBuf, TEXT("%s - "), szCardfile);
    _tcsncpy (pBuf + lstrlen(pBuf), pFile, wLen - 1 - lstrlen(pBuf));
    pBuf[wLen - 1] = TEXT('\0');
}

/* Get country info from win.ini file, and initialize global vars
      that determine format of date/time string. */
void FAR InitLocale (void)
{
  LCID   lcid;
  int    i, id;
  TCHAR  szBuf[3];

    extern TIME Time;
    extern DATE Date;

    lcid = GetUserDefaultLCID ();

    /* Get short date format */
    GetLocaleInfoW(lcid, LOCALE_SSHORTDATE, (LPWSTR) Date.szFormat, MAX_FORMAT);

    /* Get time related info */
    GetLocaleInfoW (lcid, LOCALE_ITIME, (LPWSTR) szBuf, 3);
    Time.iTime = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_ITLZERO, (LPWSTR) szBuf, 3);
    Time.iTLZero = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_S1159, (LPWSTR) Time.sz1159, 6);
    GetLocaleInfoW (lcid, LOCALE_S2359, (LPWSTR) Time.sz2359, 6);
    GetLocaleInfoW (lcid, LOCALE_STIME, (LPWSTR) Time.szSep, 2);

    /* Get system of measurement */
    fEnglish= GetProfileInt( TEXT("intl"), TEXT("iMeasure"), 1 );

    /* Get decimal character */
    szBuf[0] = szDec[0];
    GetLocaleInfoW (lcid, LOCALE_SDECIMAL, (LPWSTR) szDec, 2);
    /* Scan for . and replace with intl decimal */
    for (id = 2; id < 6; id++)
    {
        for (i = 0; i < lstrlen (chPageText[id]); i++)
            if (chPageText[id][i] == szBuf[0])
                chPageText[id][i] = szDec[0];
    }
}

/*
 * check for printer availablilty
 */
void IndexWinIniChange(
    void)
{
    HANDLE hMenu;
    TCHAR  ch;
    TCHAR msgbuf[120];
    int fEnabled = MF_GRAYED;

    static bszDecRead=FALSE;

    /* Set decimal to scan for */
    if (bszDecRead)
        ch=szDec[0]; /* If we already changed it. */
    else
        ch=TEXT('.');  /* First time. */

    bszDecRead = TRUE;

    hMenu = GetMenu(hIndexWnd);

 /* Bug 8017:  If we're setup for the default printer, throw away whatever
 * we've got as the settings may have changed.  We'll grab the new default
 * immediately before we print.     Clark Cyr    5 December 1991
 */
    if (PD.hDevNames)
    {
        BOOL bIsDefault;
        LPDEVNAMES lpDevNames  = (LPDEVNAMES)GlobalLock(PD.hDevNames);

        bIsDefault = lpDevNames->wDefault & DN_DEFAULTPRN;
        GlobalUnlock(PD.hDevNames);
        if (bIsDefault)
            FreePrintHandles();  /* bPrinterSetupDone is set to NULL */
    }

    if (bPrinterSetupDone ||
        GetProfileString(szWindows, szDevice, DefaultNullStr, msgbuf, 120))
    {
        fCanPrint = TRUE;
    }
    else
        fCanPrint = FALSE;

    InitLocale ();
}

BOOL BuildAndDisplayMsg(
    int idError,
    TCHAR szString[])
{
    TCHAR szError[300];
    TCHAR szMsg[600];

    /* load specified error msg */
    LoadString(hIndexInstance, idError, szError, CharSizeOf(szError));
    /* Merge in the given string into the error msg */
    MergeStrings(szError, szString, szMsg);
    MessageBox(hIndexWnd, szMsg, szNote, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
}

OLESTATUS OleStatusCallBack = OLE_OK;

int CallBack(
    LPOLECLIENT lpclient,
    OLE_NOTIFICATION flags,
    LPOLEOBJECT lpObject)
{
    OLE_RELEASE_METHOD method;
    WORD fOleErrMsg;
    RECT rc;

    switch(flags)
    {
        case OLE_CLOSED:
            /* After an InsertObject, the server was closed without updating the
             * embedded object */
            if (fInsertComplete == FALSE)
                PicDelete(&CurCard);

            fInsertComplete = TRUE;
            break;

        case OLE_SAVED:
        case OLE_CHANGED:
            /* in case we did an InsertObject, the object is updated now. */
            fInsertComplete = TRUE;
            /*
            * The OLE libraries make sure that we only receive
            * update messages according to the Auto/Manual flags.
            */
            CurCardHead.flags |= FDIRTY;
            InvalidateRect(hEditWnd, NULL, TRUE);

            /* recalc size */
            if (CurCard.lpObject)
            {
                if (OleQueryBounds(CurCard.lpObject, &rc) != OLE_OK)
                {
                    Hourglass(FALSE);
                    ErrorMessage(E_BOUNDS_QUERY_FAILED);
                    return( 0 ); //bugbug Is this right?
                }
                FixBounds(&rc);
                SetRect(&(CurCard.rcObject),
                    CurCard.rcObject.left, CurCard.rcObject.top,
                    CurCard.rcObject.left + (rc.right - rc.left),
                    CurCard.rcObject.top + (rc.bottom - rc.top));
            }
            break;

        case OLE_RELEASE:
            --cOleWait;

            method = OleQueryReleaseMethod(lpObject);
            if (method == OLE_LOADFROMSTREAM)
            {
                oleloadstat = OleQueryReleaseError(lpObject);
                break;
            }
            else if (method == OLE_SETDATA ||
                    method == OLE_UPDATE)
            {
                OleStatusCallBack = OleQueryReleaseError(lpObject);
                break;
            }

            switch (fOleErrMsg = OleError(OleQueryReleaseError(lpObject)))
            {
                case FOLEERROR_NOTGIVEN:
                case FOLEERROR_GIVEN:
                    switch (OleQueryReleaseMethod(lpObject))
                    {
                        case OLE_CREATEFROMFILE:
                            PicDelete(&CurCard);
                            InvalidateRect(hEditWnd, NULL, TRUE);
                            if (fOleErrMsg == FOLEERROR_NOTGIVEN)
                                ErrorMessage(E_DRAG_DROP_FAILED);
                            break;

                        default:
                            break;
                    }
                    break;

                case FOLEERROR_OK:
                    switch (OleQueryReleaseMethod(lpObject))
                    {
                        case OLE_SETUPDATEOPTIONS:
                            if (hwndLinkWait)
                                PostMessage(hwndLinkWait, WM_COMMAND, IDD_LINKDONE, 0L);
                            default:
                                break;
                    }

                default:
                    break;
           }
           break;

        case OLE_QUERY_RETRY:
        case OLE_QUERY_PAINT:
            return TRUE;    /* Always continue painting, for now */
            break;

        default:
            break;
    }
    return 0;
}


int NEAR PASCAL LogHimetricToPels(HDC hDC,BOOL bHoriz, int mmHimetric)
{
    #define nHMPerInch 2540
    #define nPelsPerLogInch GetDeviceCaps(hDC, bHoriz ? LOGPIXELSX : LOGPIXELSY)

    return MulDiv(mmHimetric,nPelsPerLogInch,nHMPerInch);
}

