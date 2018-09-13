#define USECOMM
#include "precomp.h"


// Nota Bene:  modems aren't unicode aware
//             so only send them ascii.

#define HANGUP_CMD    "ATH0\r"

#define MAXDIALSIZE    80   /* max size of dial string */

#if defined(WIN32)
#define GetCommError( cid, lp ) ClearCommError( cid, NULL, lp )
#endif

/*
 * general form of modem string
 *
 * MODEM=COMn,T|P,0|1|2|3|4...
 *
 * COMn == com # (1, 2, 3, 4)
 * TorP == Tone or Pulse
 * 0|1|2|.. == speed number (0==120, 1==300, 2==1200, 3==2400...
 *
 */

#define PRE_LEN 6
TCHAR PrefixDefault[] = TEXT("9-");
TCHAR Prefix[PRE_LEN];

TCHAR ModemInitDefault[] = TEXT("COM1,T,2");    /* com1, tone dialing, 1200 baud */
TCHAR ModemInit[15];

TCHAR *pcComNum    = ModemInit + 3;    /* ptrs to the stuff we want */
TCHAR *pcTonePulse = ModemInit + 5;
TCHAR *pcSpeedNum  = ModemInit + 7;    // baud rate encoded


NOEXPORT void NEAR GetPhoneNumber(
    LPTSTR pchBuf,
    int cchMax);

NOEXPORT BOOL NEAR ParseNumber(
    LPTSTR lpSrc,
    TCHAR *pchBuf,
    int cchMax);

NOEXPORT void NEAR SetPortState(
    HFILE  cid);

NOEXPORT int NEAR MakeDialCmd(
    TCHAR *pBuf,
    int  cchMax,
    LPTSTR pchNumber);

BOOL fnDial(
    HWND hDB,
    UINT message,
    WPARAM wParam,
    LONG lParam)
{
    TCHAR lpBuf[45];
    TCHAR *pResultBuf;
    int len;
    TCHAR PhoneNumber[256];
    TCHAR buf[40];
    TCHAR cmdBuf[160];
    RECT rect, rect1;

    static BOOL fWriteWinIni = FALSE;
    static BOOL fReadWinIni = TRUE;
    static BOOL fUsePrefix = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hDB, ID_SETUP), TRUE);    /* make sure he is on */

            /* resize myself to hold just the number and buttons */
            GetWindowRect(GetDlgItem(hDB, ID_BOX1), &rect);
            GetWindowRect(hDB, &rect1);
            MoveWindow(hDB, rect1.left, rect1.top,
                rect1.right-rect1.left, rect.bottom-rect1.top, FALSE);

            /* do first time initilazation? */
            if (fReadWinIni)
            {
                GetProfileString(szWindows, TEXT("Modem"), ModemInitDefault, ModemInit, CharSizeOf(ModemInit));
                GetProfileString(szWindows, TEXT("Prefix"), PrefixDefault, Prefix, CharSizeOf(Prefix));
                fUsePrefix = GetProfileInt(szWindows, TEXT("UsePrefix"), fUsePrefix);

                if (*pcSpeedNum == TEXT('F')) {    /* for backwards compatability */
                    *pcSpeedNum = TEXT('2');
                    fWriteWinIni = TRUE;
                }

                fReadWinIni = FALSE;
            }

            /* set phone number text and select it */
            GetPhoneNumber(PhoneNumber, 256);
            SetDlgItemText(hDB, ID_NUM, PhoneNumber);
            SendMessage(GetDlgItem(hDB, ID_NUM), EM_SETSEL, 0, (long)lstrlen(PhoneNumber));

            /* set the prefix text */
            SetDlgItemText(hDB, ID_PREFIX, Prefix);

            /* check the use prefix box */
            SendMessage(GetDlgItem(hDB, ID_USEPREFIX), BM_SETCHECK, fUsePrefix, 0L);

            /* check the rest of the buttons... */
            CheckRadioButton(hDB, RB_TONE, RB_PULSE, (*pcTonePulse == TEXT('T')) ? RB_TONE : RB_PULSE);
            CheckRadioButton(hDB, RB_COM1, RB_COM4, RB_COM1 + (*pcComNum - TEXT('1')));
            CheckRadioButton(hDB, RB_110,  RB_19200, RB_110 + (*pcSpeedNum - TEXT('0')));
#ifdef JAPAN //KKBUGFIX //#4311: 2/27/93: made buttons which are disapeared disable
            EnableWindow(GetDlgItem(hDB, RB_TONE), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_PULSE), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_COM1), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_COM2), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_COM3), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_COM4), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_110), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_300), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_1200), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_2400), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_4800), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_9600), FALSE);
            EnableWindow(GetDlgItem(hDB, RB_19200), FALSE);
#endif

            return(TRUE);

        case WM_COMMAND:
            pResultBuf = NULL;

            switch (LOWORD(wParam))
            {
                case IDOK:              /* DIAL it! */
                    if (fWriteWinIni)
                    {
                        WriteProfileString(szWindows, TEXT("Modem"), ModemInit);
                        WriteProfileString(szWindows, TEXT("Prefix"), Prefix);
                        if (fUsePrefix)
                            WriteProfileString(szWindows, TEXT("UsePrefix"), TEXT("1"));
                        else
                            WriteProfileString(szWindows, TEXT("UsePrefix"), TEXT("0"));
                        fWriteWinIni = FALSE;
                    }

                    /* check for valid number. */

                    /* Get the length.  If 0, do nothing.*/
                    len=GetDlgItemText(hDB, ID_NUM, lpBuf, 40);
                    if (len!=0)
                    {
                        /* Check for a valid number, give error if not valid */
                        if (!ParseNumber(lpBuf, buf, 40))
                        {
                            LoadString(hIndexInstance, ECANTDIAL, cmdBuf, CharSizeOf(cmdBuf));
                            MessageBox(hDB, cmdBuf, szCardfile, MB_OK | MB_ICONEXCLAMATION);
                        }
                        else
                        {
                            /* note: this Alloc gets freeded by the caller */
                            if (pResultBuf = (TCHAR *)LocalAlloc(LPTR, ByteCountOf(40)))
                            {
                                if (fUsePrefix)
                                    len = GetDlgItemText(hDB, ID_PREFIX,
                                                     pResultBuf, PRE_LEN-1);
                                else
                                    len = 0;

                                GetDlgItemText(hDB, ID_NUM, pResultBuf + len, 30);
                            }
                        }
                    }
                /* fall through... */

                case IDCANCEL:
                    EndDialog(hDB, (int)pResultBuf);
                    break;

                case ID_SETUP:
#ifdef JAPAN //KKBUGFIX //#4311: 2/27/93: made buttons which are disapeared disable
                    EnableWindow(GetDlgItem(hDB, RB_TONE), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_PULSE), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_COM1), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_COM2), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_COM3), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_COM4), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_110), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_300), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_1200), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_2400), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_4800), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_9600), TRUE);
                    EnableWindow(GetDlgItem(hDB, RB_19200), TRUE);
#endif
                    /* resize myself to fit in the setup controls */
                    SetFocus(GetDlgItem(hDB, ((*pcTonePulse == TEXT('T')) ? RB_TONE : RB_PULSE)));
                    EnableWindow(GetDlgItem(hDB, ID_SETUP), FALSE);
                    GetWindowRect(GetDlgItem(hDB, ID_BOX2), &rect);
                    GetWindowRect(hDB, &rect1);
                    MoveWindow(hDB, rect1.left, rect1.top,
                        rect1.right-rect1.left, rect.bottom-rect1.top, TRUE);
                    break;

                case ID_PREFIX:
#if defined(WIN32)
                    if (HIWORD(lParam) == EN_CHANGE) {
#else
                    if (HIWORD(lParam) == EN_CHANGE) {
#endif
                        GetDlgItemText(hDB, ID_PREFIX, Prefix, PRE_LEN-1);
                        fWriteWinIni = TRUE;
                    }
                    break;

                case ID_USEPREFIX:
                    fUsePrefix = !fUsePrefix;
                    SendMessage(GetDlgItem(hDB, ID_USEPREFIX), BM_SETCHECK, fUsePrefix, 0L);
                    break;

                case RB_TONE:
                case RB_PULSE:
                    CheckRadioButton(hDB, RB_TONE, RB_PULSE, LOWORD(wParam));
                    *pcTonePulse = ((wParam == RB_TONE) ? TEXT('T') : TEXT('P'));
                    fWriteWinIni = TRUE;
                    break;

                case RB_COM1:
                case RB_COM2:
                case RB_COM3:
                case RB_COM4:
                    CheckRadioButton(hDB, RB_COM1, RB_COM4, LOWORD(wParam));
                    *pcComNum= (TCHAR) ((wParam - RB_COM1) + TEXT('1'));
                    fWriteWinIni = TRUE;
                    break;

                case RB_110:
                case RB_300:
                case RB_1200:
                case RB_2400:
                case RB_4800:
                case RB_9600:
                case RB_19200:
                    CheckRadioButton(hDB, RB_110, RB_19200, LOWORD(wParam));
                    *pcSpeedNum= (TCHAR) ((wParam - RB_110) + TEXT('0'));
                    fWriteWinIni = TRUE;
                    break;

                default:
                    return(FALSE);
            }
            return(TRUE);

        default:
            return(FALSE);
    }
}

/***************************************************************************
 * GetPhoneNumber(pchBuf, cchMax)
 *
 * purpose:
 *    look for phone numbers in:
 *    1) the header line of the card
 *    2) the text of the card itself, starting first with the selection
 *
 * params:
 *    IN    cchMax    limit pchBuf to this # chars
 *    OUT    pchBuf    gets the dialing string if
 *
 * used by:
 *    this is called by the dailing dialog function
 *
 * uses:
 *    ParseNumber() to find phone numbers
 *
 * returns:
 *    nothing of interest
 *
 ***************************************************************************/
NOEXPORT void NEAR GetPhoneNumber(
    LPTSTR pchBuf,
    int cchMax)
{
    LPCARDHEADER lpCard;
    int fFound = FALSE;
    unsigned long lSelection;

    lSelection = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);

    /* first look in card header */
    if (HIWORD(lSelection) == LOWORD(lSelection))
    {
        /* no selection search card */
        lpCard = (LPCARDHEADER) GlobalLock(hCards) + iFirstCard;
        fFound = ParseNumber(lpCard->line, pchBuf, cchMax);
        GlobalUnlock(hCards);
    }

    /* now look in the text of the card, first at the selection */
    if (!fFound)
    {
        GetWindowText(hEditWnd, szText, CARDTEXTSIZE);
        if (HIWORD(lSelection) != LOWORD(lSelection))
        {
            lstrcpy(szText, szText+LOWORD(lSelection));
            *(szText + (HIWORD(lSelection) - LOWORD(lSelection))) = (TCHAR) 0;
        }
        fFound = ParseNumber(szText, pchBuf, cchMax);
    }

    if (!fFound)
        *pchBuf = (TCHAR) 0;
}

/***************************************************************************
 * BOOL ParseNumber(lpSrc, pchBuf, cchMax)
 *
 * purpose:
 *    look for phone number strings in lpSrc (return the first one found)
 *
 * params:
 *     IN    lpSrc    string to search for numbers
 *    OUT    pchBuf    place to put result
 *    IN    cchMax    limit # chars in pchBuf
 *
 * used by:
 *      GetPhoneNumber(), and from Autodial Dialog box.
 *
 * uses:
 *
 * returns:
 *    BOOL    TRUE if something found, FALSE otherwise
 *
 * restrictions:
 *    as of now this is a very stupid function. a valid number has to
 *    be 4 chars min for example.  a number matching scheme should be
 *    implemented that looks for things like ###-#### (###) ###-#### or
 *    ####.  Also this function returns the first one found.  It should
 *    probally enumerate all that are found.
 *
 ***************************************************************************/
NOEXPORT BOOL NEAR ParseNumber(
    LPTSTR lpSrc,
    TCHAR *pchBuf,
    int cchMax)
{
    LPTSTR lpchTmp;
    LPTSTR lpchEnd;
    LPTSTR pchTmp;
    int fValid = FALSE;
    TCHAR ch;

    for (lpchTmp = lpSrc; *lpchTmp; lpchTmp++)
    {
        pchTmp = pchBuf;
        lpchEnd = lpchTmp;
        while ((pchTmp - pchBuf) < cchMax)
        {
            ch = *lpchEnd++;
            if (ch == TEXT('-'))
            {
                *pchTmp++ = ch;
            }
            else
            {
                FoldStringW( MAP_FOLDDIGITS, &ch, 1, &ch, 1 );  // handle odd number characters!

                if ((ch >= TEXT('0') && ch <= TEXT('9')) || ch == TEXT('@') ||
                     ch == TEXT(',') || ch == TEXT('(')  || ch == TEXT(')') ||
                     ch == TEXT('*') || ch == TEXT('#'))
                {
                    if (ch >= TEXT('0') && ch <= TEXT('9'))
                       fValid = TRUE;
                    *pchTmp++ = ch;
                }
                /* Allow a space after an area code in parens. */
                else if (!(ch ==TEXT(' ') && *(pchTmp - 1) == TEXT(')') ))
                {
                    *pchTmp = (TCHAR) 0;
                    break;
                }
            }
        }

        if (fValid && ((pchTmp - pchBuf) > 3))    /* allow 4 digit numbers */
            return(TRUE);
    }
    return(FALSE);
}

/*
 * dial the phone with the phone number in pchNumber
 */
void DoDial(
    LPTSTR pchNumber)
{
    HFILE cid;
    TCHAR szComm[5];
    TCHAR cmdBuf[MAXDIALSIZE*2];
    char  aCmdBuf[MAXDIALSIZE];      // ascii version of cmdBuf
    int cch;                         // length of unicode to write
    int acch;                        // length of ascii to write
    COMSTAT ComStatInfo;
    long oldtime;
    HCURSOR hPrevCursor;

    lstrcpy(szComm, TEXT("COMx"));

    szComm[3] = *pcComNum;

    cid = CreateFile( szComm,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL ) ;

    if (cid != (HFILE)-1)
    {
        SetPortState(cid);
        GetCommError(cid, &ComStatInfo);

        /* Dial the number */
        cch = MakeDialCmd(cmdBuf, MAXDIALSIZE, pchNumber);
        acch= WideCharToMultiByte(
            CP_ACP,       // use ascii code page
            0,            // no flags
            cmdBuf,       // input buffer
            cch,          // buffer length
            aCmdBuf,      // output buffer
            MAXDIALSIZE,  // output buffer size
            NULL,         // use system default char
            NULL);        // and don't tell me if you used it
        while (!WriteFile(cid, aCmdBuf, acch, &acch, NULL))
        {
            GetCommError(cid, &ComStatInfo);
            FlushFileBuffers(cid) ;
        }

        /* wait for dialing to complete */
        /* Set the HourGlass cursor while waiting */
        /* Fix for Bug #6402 --SANKAR-- 12-1-89 */
        hPrevCursor = SetCursor(hWaitCurs);

        oldtime = GetCurrentTime();
        for(;;)
        {
            GetCommError(cid, &ComStatInfo);
            if (GetCurrentTime() - oldtime > 3000)    /* 30 seconds */
            {
                /* Restore the original cursor shape */
                SetCursor(hPrevCursor);
                IndexOkError(ENOMODEM);
                goto DoneDialing;
            }
            if (!ComStatInfo.cbOutQue)
               break;
        }
        /* Restore the original cursor shape */
        SetCursor(hPrevCursor);

        FlushFileBuffers(cid) ;
        LoadString(hIndexInstance, IPICKUPPHONE, cmdBuf, CharSizeOf(cmdBuf));
        MessageBox(hIndexWnd, cmdBuf, szCardfile, MB_OK);

        while(!WriteFile(cid, HANGUP_CMD, sizeof(HANGUP_CMD)-1, &cch, NULL))
        {
            GetCommError(cid, &ComStatInfo);
            FlushFileBuffers(cid) ;
        }
        while(TRUE)
        {
            GetCommError(cid, &ComStatInfo);
            if (!ComStatInfo.cbOutQue)
                break;
        }

DoneDialing:
      CloseHandle(cid) ;
    }
    else
    {
        LoadString(hIndexInstance, ECANTDIAL, cmdBuf, CharSizeOf(cmdBuf));
        MessageBox(hIndexWnd, cmdBuf, szCardfile, MB_OK | MB_ICONEXCLAMATION);
    }
}

NOEXPORT void NEAR SetPortState(
    HFILE  cid)
{
    DCB dcb;
    TCHAR szPortInfo[30];
    TCHAR *pch;
    TCHAR szPort[6];

    if (GetCommState(cid, &dcb)!=-1)
    {
        switch (*pcSpeedNum)
        {
            case TEXT('0'):
                dcb.BaudRate = 110;
                break;

            case TEXT('1'):
                dcb.BaudRate = 300;
                break;

            case TEXT('2'):
                dcb.BaudRate = 1200;
                break;

            case TEXT('3'):
                dcb.BaudRate = 2400;
                break;

            case TEXT('4'):
                dcb.BaudRate = 4800;
                break;

            case TEXT('5'):
                dcb.BaudRate = 9600;
                break;

            case TEXT('6'):
                dcb.BaudRate = 19200;
                break;
        }

        lstrcpy(szPort, TEXT("COMx:"));
#if defined(WIN32)
        szPort[3] = *pcComNum ;
#else
        szPort[3] = TEXT('1') + cid;
#endif

        GetProfileString(TEXT("Ports"), szPort, TEXT("300,n,8,1"), szPortInfo, CharSizeOf(szPortInfo));
        for (pch = szPortInfo; *pch && *pch != TEXT(','); ++pch)
            ;
        while(*pch == TEXT(',') || *pch == TEXT(' '))
            pch++;
        dcb.Parity = *pch == TEXT('n') ? NOPARITY : (*pch == TEXT('o') ? ODDPARITY : EVENPARITY);
        if (*pch)
            pch++;
        while(*pch == TEXT(',') || *pch == TEXT(' '))
            pch++;
        dcb.ByteSize = *pch == TEXT('8') ? 8 : 7;
        if (*pch)
            pch++;
        while (*pch == TEXT(',') || *pch == TEXT(' '))
            pch++;
        dcb.StopBits = *pch == TEXT('2') ? 2 : 0;

#if !defined(WIN32)
        dcb.fDtrDisable = FALSE;    /* use DTR for hangup */
        SetCommState(&dcb);
#else
        dcb.fDtrControl = FALSE;    /* use DTR for hangup */
        SetCommState(cid, &dcb);
#endif

    }
}

/*
 * Create a string for dialing Hayes compatable modems.
 *
 * in:
 *    cchMax - max number of chars to stuff into pBuf
 *    pchNumber - number string to build dialing stuff out of
 *
 * out:
 *    pBuf - output buffer
 *
 * returns:
 *    the length of the command built
 */
NOEXPORT int NEAR MakeDialCmd(
    TCHAR *pBuf,
    int  cchMax,
    LPTSTR pchNumber)
{
    LPTSTR pch1;
    LPTSTR pch2;
    int  cb;
    TCHAR szCmd[MAXDIALSIZE];        /* build it here */
    TCHAR ch;

    lstrcpy(szCmd, TEXT("ATDx"));    /* dialing prefix */
    szCmd[3] = *pcTonePulse;    /* pulse or tone dialing */

    pch2 = szCmd + 4;        /* use this to fill the string */

    for (pch1 = pchNumber; ch = *pch1++; )
    {
        /* copy only these characters */
        if ((ch >= TEXT('0') && ch <= TEXT('9')) || (ch == TEXT(',')) || (ch == TEXT('#')) || (ch == TEXT('*')))
            *pch2++ = ch;
        else if (ch == TEXT('@'))    /* delay */
        {
            *pch2++ = TEXT(',');
            *pch2++ = TEXT(',');
            *pch2++ = TEXT(',');
        }
        else if (ch == TEXT('P') || ch == TEXT('T')) /* manual switch to Tone or Pulse */
        {
            *pch2++ = TEXT('D');
            *pch2++ = ch;
        }
    }

    *pch2++ = TEXT(';');        /* terminate the string */
    *pch2++ = 0x0d;
    *pch2 = 0;

    cb = lstrlen(szCmd);
    if (cchMax < pch2 - szCmd)
    {
        szCmd[cchMax] = (TCHAR) 0;
        cb = cchMax;
    }

    lstrcpy(pBuf, szCmd);
    return cb;
}
