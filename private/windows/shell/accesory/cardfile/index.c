#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/


int iTopScreenCard;
int iFirstCard = 0;
int cScreenHeads;       /* the number of headers that are partially visible (>=1) */
int cFSHeads;           /* the number of fully visible headers */
int cScreenCards;       /* the number of cards that are partially visible (>=1) */
int xFirstCard;         /* the x */
int yFirstCard;         /* and y coordinates of the front card */
int ySpacing;           /* the y offset of each card (CharFixHeight + 1) */
int iTopCard;

HBRUSH hbrCard;     /* brush for drawing cards (COLOR_WINDOW) */
HBRUSH hbrBorder;   /* brush for drawing card outline (COLOR_WINDOWFRAME) */

HWND hCardWnd;      /* client area of main less space for status stuff */
HWND hEditWnd;      /* sub classed edit window for editing cur card */
HWND hLeftWnd;      /* displays viewing mode */
HWND hRightWnd;     /* display # of cards */
HWND hScrollWnd;    /* for scrolling of cards */
HWND hListWnd;      /* for PHONEBOOK mode (list view) */

BOOL fInSaveAsDlg;  /* Are we in the middle of a SaveAs Dlg */

int CardPhone = CCARDFILE;

HANDLE hArrowCurs;
HANDLE hWaitCurs;
HFONT   hFont= NULL;                  // current font
LOGFONT FontStruct;                   // font structure of current font
INT     iPointSize=INITPOINTSIZE;     // current point size of FontStruct

HANDLE hIndexInstance;
HWND hIndexWnd;

TCHAR CurIFile[PATHMAX];
TCHAR szSearch[40];
BOOL fCase = FALSE;
BOOL fReverse = FALSE;
BOOL fRepeatSearch = FALSE;

TCHAR szHelpFile[30];
TCHAR szUntitled[30];
TCHAR szCardView[25];
TCHAR szListView[25];

BOOL fFileDirty = FALSE;
BOOL fHorzSBOn = FALSE;
BOOL fFullSize;

int EditMode = I_TEXT;

INT xCardWnd;       /* size of the Card window */
INT yCardWnd;

int CardWidth;      /* size of a card (Edit window + some stuff) */
int CardHeight;

INT EditWidth;      /* size of Edit window */
INT EditHeight;

INT CharFixWidth;   /* from text metrics */
INT CharFixHeight;
INT ExtLeading;

INT cxHScrollBar;   /* scroll bar dimensions */
INT cyHScrollBar;


INT cCards;         /* the current number of cards */
HANDLE hCards;      /* the handle of the header buffers */
CARDHEADER CurCardHead;    /* the working card header */
CARD CurCard;       /* the working card data */
DWORD   idObjectMax = 0;

FARPROC lpDlgProc;
FARPROC lpEditWndProc;
FARPROC lpfnAbortProc;
FARPROC lpfnAbortDlgProc;
FARPROC lpfnPageDlgProc;
FARPROC lpfnDial;
FARPROC lpfnLinksDlg;
FARPROC lpfnInvalidLink;

TCHAR NotEnoughMem[160];
TCHAR szWarning[30];
TCHAR szNote[30];

#ifdef DBCS_IME
                //KKBUGFIX  #3082: 02/02/1993: Disabling IME while Picture mode
BOOL    fNowFocus = FALSE;
//IMEPRO  imepCurrent;
//IMEPRO  imepNull;
#if 0
void
SaveIMEStatus(void)
{
    IMPGetIME (NULL, &imepCurrent);
    imepNull.szName[0] = '\0';
}
#endif
void
EnableIME(BOOL fEnable)
{
    WINNLSEnableIME ((HWND)NULL, fEnable);
#if 0
    if (fEnable)
        IMPSetIME (NULL, &imepCurrent);
    else
        IMPSetIME (NULL, &imepNull);
#endif
}
#endif


/*
 * Window Proc for the main window (Index)
 */
LONG IndexWndProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PAINTSTRUCT ps;
    int i;
    static int xClient, yClient;

    switch (message)
    {
        case WM_DROPFILES:
            DoDragDrop(hwnd, (HANDLE)wParam, FALSE);
            break;

        case WM_CREATE:
            hwndError = hIndexWnd = hwnd;
            IndexWinIniChange();
            SetCaption();

            LoadString(hIndexInstance, ICARDVIEW, szCardView, CharSizeOf(szCardView));
            LoadString(hIndexInstance, ILISTVIEW, szListView, CharSizeOf(szListView));

            hListWnd = CreateWindow(TEXT("listbox"), NULL,
                WS_CHILD | LBS_NOTIFY | WS_VSCROLL,
                0, 0, 0, 0,
                hwnd, (HMENU)LISTWINDOW, hIndexInstance, NULL);

            /* Enable this line for a Unicode font. */
            SendMessage(hListWnd, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));

            hCardWnd = CreateWindow(szCardClass, NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
                0, 0, 0, 0,
                hwnd, (HMENU)CARDWINDOW, hIndexInstance, NULL);

            hLeftWnd = CreateWindow(TEXT("static"), szCardView,
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0,
                hwnd, (HMENU)LEFTWINDOW, hIndexInstance, NULL);

            hRightWnd = CreateWindow(TEXT("static"), NULL,
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0,
                hwnd, (HMENU)RIGHTWINDOW, hIndexInstance, NULL);

            hScrollWnd = CreateWindow(TEXT("scrollbar"), NULL,
                WS_CHILD | WS_VISIBLE | SBS_HORZ,
                0, 0, 0, 0,
                hwnd, (HMENU)SCROLLWINDOW, hIndexInstance, NULL);


            break;

        case WM_SYSCOLORCHANGE:
            DeleteObject(hbrCard);
            DeleteObject(hbrBorder);
            hbrCard   = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            hbrBorder = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
            break;

        case WM_ENDSESSION:
            if (wParam)
                Fdelete(TempFile);
            break;

        case WM_QUERYENDSESSION:
            if (CheckForBusyObjects())
                return FALSE;
            if (fInSaveAsDlg)
            {
                TCHAR szMsg[100];

                MessageBeep(0);
                MessageBeep(0);
                LoadString(hIndexInstance, IDS_CANNOTQUIT, szMsg, CharSizeOf(szMsg));
                MessageBox(hIndexWnd, szMsg, szCardfile, MB_OK | MB_SYSTEMMODAL);
            }
            else if (MaybeSaveFile(TRUE))
            {
                SetCurCard(iFirstCard);
                return(TRUE);
            }
            return(FALSE);

        case WM_CLOSE:
            if (CheckForBusyObjects())
                return FALSE;
            if (MaybeSaveFile(FALSE))
                DestroyWindow(hwnd);
            return(TRUE);

        case WM_DESTROY:
            Fdelete(TempFile);
            /* Delete hbrCard and hbrBorder which is per instance data */
            DeleteObject(hbrCard);
            DeleteObject(hbrBorder);
            WinHelp(hIndexWnd, szHelpFile, HELP_QUIT, (DWORD)0);

            if (fOLE)
                ReleaseClientDoc();
            if (fhMain != INVALID_HANDLE_VALUE)
                MyCloseFile(fhMain);
            PostQuitMessage(0);
            return(TRUE);

        case WM_INITMENU:
            UpdateMenu();
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case LISTWINDOW:
#if defined( WIN32 )
                    switch (HIWORD(wParam))
#else
                    switch ((int) HIWORD(lParam))
#endif
                    {
                        case LBN_ERRSPACE:
                            IndexOkError(EINSMEMORY);
                            break;

                        case LBN_DBLCLK:
                        case LBN_SELCHANGE:
                            /* this is necessary for a win 2.1 list box bug */
                            if ((i = (WORD)SendMessage(hListWnd, LB_GETCURSEL, 0, 0L)) == LB_ERR)
                                break;
                            iFirstCard = i;
#if defined(WIN32)
                            if (HIWORD(wParam) == LBN_DBLCLK)
#else
                            if (HIWORD(lParam) == LBN_DBLCLK)
#endif
                                IndexInput(hListWnd, HEADER);
                            break;
                    }
                    break;

                case LEFTWINDOW:
                    break;

                default:
                    return IndexInput(hwnd, LOWORD(wParam));
            }
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            SetNumOfCards();
#if defined(WIN32)
            MoveToEx(ps.hdc, 0, cyHScrollBar, NULL);
#else
            MoveTo(ps.hdc, 0, cyHScrollBar);
#endif
            LineTo(ps.hdc, xClient, cyHScrollBar);
            EndPaint(hwnd, &ps);
            break;

        case WM_SIZE:
            if (wParam != SIZEICONIC)
            {
                xClient = LOWORD(lParam);
                yClient = HIWORD(lParam);

                MoveWindow(hLeftWnd, 0, 0,
                    xClient/2-cxHScrollBar, cyHScrollBar, TRUE);

                MoveWindow(hRightWnd, xClient/2+cxHScrollBar, 0,
                    xClient/2-cxHScrollBar+1, cyHScrollBar, TRUE);

                MoveWindow(hScrollWnd, xClient/2-cxHScrollBar, 0,
                    cxHScrollBar*2, cyHScrollBar, TRUE);

                MoveWindow(hListWnd, 0, cyHScrollBar+1,
                    xClient, yClient-cyHScrollBar, CardPhone == PHONEBOOK);

                MoveWindow(hCardWnd, 0, cyHScrollBar+1,
                    xClient+1, yClient-cyHScrollBar, CardPhone == CCARDFILE);

                fFullSize = (wParam == SIZEFULLSCREEN);
                SizeListWindow();
            }
            break;

        case WM_HSCROLL:
            if (CardPhone == CCARDFILE)
#if defined(WIN32)
                ScrollCards(hCardWnd, LOWORD(wParam), HIWORD(wParam));
#else
                ScrollCards(hCardWnd, wParam, LOWORD(lParam));
#endif
            else if (wParam == SB_LINEDOWN)
            {
                /* scroll as if an arrow key was pressed */
                iFirstCard++;
                if (iFirstCard == cCards)
                    iFirstCard = 0;
                SetFocus(NULL);
                SendMessage(hListWnd, LB_SETCURSEL, iFirstCard, 0L);
                SetFocus(hListWnd);
            }
            else if (wParam == SB_LINEUP)
            {
                iFirstCard--;
                if (iFirstCard < 0)
                    iFirstCard = cCards - 1;
                SetFocus(NULL);
                SendMessage(hListWnd, LB_SETCURSEL, iFirstCard, 0L);
                SetFocus(hListWnd);
            }
            break;

        case WM_WININICHANGE:
            IndexWinIniChange();
            break;

        case WM_SETFOCUS:
            SetFocus(CardPhone == CCARDFILE ? hCardWnd : hListWnd);
            break;

        default:
            if (message == wFRMsg)
            {
                LPFINDREPLACE lpfr;
                DWORD dwFlags;

                lpfr = (LPFINDREPLACE)lParam;
                dwFlags = lpfr->Flags;
                fReverse = (dwFlags & FR_DOWN       ? FALSE : TRUE);
                fCase     = (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);

                if (lpfr->Flags & FR_FINDNEXT)
                {
                    /* wParam contains handle of the Find dialog */
                    /* Has the user typed in a new string? */
                    if ((fCase && lstrcmp(szSearch, lpfr->lpstrFindWhat)) ||
                        (!fCase && lstrcmpi(szSearch, lpfr->lpstrFindWhat)))
                    {
                        fRepeatSearch = FALSE;
                        lstrcpy(szSearch, lpfr->lpstrFindWhat);
                    }
                    else    /* No */
                        fRepeatSearch = TRUE;
                    if (fReverse)
                        ReverseSearch();
                    else
                        ForwardSearch();
                }
                else if (lpfr->Flags & FR_DIALOGTERM)
                {
                    GlobalUnlock(hFind);
                    hDlgFind = NULL;
                }
                break;
            }
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

LONG CardWndProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PAINTSTRUCT ps;
    HFONT    hOldFont;

    switch (message)
    {
        case WM_CREATE:
            hEditWnd = CreateWindow(TEXT("edit"), NULL,
                        WS_CHILD | ES_MULTILINE | ES_NOHIDESEL | WS_VISIBLE,
                        0, 0, 0, 0,
                        hwnd, (HMENU) EDITWINDOW, hIndexInstance, NULL);
            DragAcceptFiles(hEditWnd, TRUE);

            /* Use a Unicode font */
            SendMessage(hEditWnd,WM_SETFONT,(WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hEditWnd, EM_LIMITTEXT, CARDTEXTSIZE, 0L);

            /* sub class the edit window to support object drawing */
            lpEditWndProc = (FARPROC)GetWindowLong(hEditWnd, GWL_WNDPROC);
            SetWindowLong(hEditWnd, GWL_WNDPROC,
                (LONG)MakeProcInstance((FARPROC)EditWndProc, hIndexInstance));
            break;

        case WM_HSCROLL:
            ScrollIndexHorz(hwnd, wParam, LOWORD(lParam));
            break;

        case WM_VSCROLL:
            ScrollIndexVert(hwnd, wParam, LOWORD(lParam));
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            IndexMouse(hwnd, message, wParam, MYMAKEPOINT(lParam));
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case EDITWINDOW:    /* message from hEditWnd */
#if defined(WIN32)
                    switch(HIWORD(wParam))
#else
                    switch(HIWORD(lParam))
#endif
                    {
                        case EN_MAXTEXT:
                            IndexOkError(E_PASTEDTEXTTOOLONG);
                            break;

                        case EN_ERRSPACE:
                            IndexOkError(EINSMEMORY);
                            break;

                        case EN_CHANGE:
                           //fNeedToUpdateObject = TRUE;
                           break;
#if 0
#ifdef JAPAN    //KKBUGFIX     // #3082: 02/02/1993: Disabling IME while Picture mode
                        case EN_SETFOCUS:
                            SaveIMEStatus ();
                            EnableIME (EditMode == I_TEXT);
                            fNowFocus = TRUE;
                            break;

                        case EN_KILLFOCUS:
                            EnableIME (TRUE);
                            fNowFocus = FALSE;
                            break;
#endif
#endif
#ifdef DBCS_IME // 24-Mar-93, by v-kenich
                        {
                            static  BOOL bPrvIMEStat;

                            case EN_SETFOCUS:
                                bPrvIMEStat = WINNLSEnableIME ((HWND)NULL,
                                                               EditMode == I_TEXT);
                                fNowFocus = TRUE;
                                break;

                            case EN_KILLFOCUS:
                                WINNLSEnableIME ((HWND)NULL, bPrvIMEStat);
                                fNowFocus = FALSE;
                                break;
                        }
#endif // DBCS_IME
                    }
                    break;
            }
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            SetNumOfCards();
            hOldFont = SelectObject(ps.hdc, hFont);

            IndexPaint(ps.hdc);
            if (hOldFont)
                SelectObject(ps.hdc, hOldFont);
            EndPaint(hwnd, &ps);
            break;

        case WM_SIZE:
            if (wParam == SIZEICONIC)
                break;

            xCardWnd = LOWORD(lParam);
            yCardWnd = HIWORD(lParam);

            /* make sure at least one header line shows */
            yFirstCard = max(TOPMARGIN, yCardWnd - BOTTOMMARGIN - CardHeight);
            xFirstCard = LEFTMARGIN;

            /*
             * much strangness is involved in getting the scroll ranges
             * right.  we throw up the scroll bars when the main window
             * (Index) is smaller than the card.  To see the range the
             * basic rule is overlap the index window with the card in
             * the two extreme cases.  we will only allow the card space
             * to be scrolled so that the opposite edge has just become
             * visible (ie you can not scroll the card out of view).
             * draw some pictures and the numbers below will make sense.
             */

            if (fHorzSBOn = ((xFirstCard + CardWidth) > xCardWnd))
            {
                SetScrollRange(hwnd, SB_HORZ,
                    -CardWidth + xCardWnd - 1, LEFTMARGIN, FALSE);
                SetScrollPos(hwnd, SB_HORZ, -CardWidth + xCardWnd - 1, TRUE);
            }
            else
                ShowScrollBar(hwnd, SB_HORZ, FALSE);

            if ((yFirstCard + CardHeight) > yCardWnd)
            {
                SetScrollRange(hwnd, SB_VERT,
                    -CardHeight + yCardWnd - 1, TOPMARGIN, FALSE);
                SetScrollPos(hwnd, SB_VERT, -CardHeight + yCardWnd - 1, TRUE);
            }
            else
                ShowScrollBar(hwnd, SB_VERT, FALSE);

            MoveWindow(hEditWnd, xFirstCard+1, yFirstCard + 1 + CharFixHeight + 1 + 2,
            EditWidth, EditHeight, TRUE);
            break;

        case WM_CHAR:
            CardChar(wParam);
            break;

        case WM_SETFOCUS:
            SetFocus(hEditWnd);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/*
 * this will resize the main window to make sure that the list window height
 * will be multiples of the char height
 *
 * this gets called for on WM_SIZE of the main window
 * and when we change view mode.
 */
void SizeListWindow(
    void)
{
    int dy;
    RECT rect;
    RECT rect1;
    static BOOL fFirstSize = TRUE;

    if (CardPhone == CCARDFILE || fFullSize)
        return;            /* ignore all of these cases */

    if (fFirstSize)
    {
        GetClientRect(hListWnd, &rect);
        GetClientRect(hCardWnd, &rect1);

        if (fHorzSBOn)
            dy =  (rect1.bottom + cyHScrollBar) - rect.bottom;
        else
            dy =  rect1.bottom - rect.bottom;

        GetWindowRect(hIndexWnd, &rect);

        /* resize the main window */

        fFirstSize = FALSE;    /* avoid infinte recursion on WM_SIZE */

        MoveWindow(hIndexWnd, rect.left, rect.top,
                rect.right-rect.left, rect.bottom - rect.top - dy, TRUE);

        fFirstSize = TRUE;    /* allow next call to enter */
    }
}

/* ReleaseClientDoc() - Release the OLE client document.
 *
 * We wait for all OLE operations to complete before revoking the document.
 */
void ReleaseClientDoc(
    void)
{
    if (cOleWait > 0)   /* Do the safe test */
    {
        while (cOleWait)
            ProcessMessage(hIndexWnd, hAccel);
    }

    if (lhcdoc && OleRevokeClientDoc(lhcdoc) == OLE_WAIT_FOR_RELEASE)
    {
        cOleWait++;
        while (cOleWait)
            ProcessMessage(hIndexWnd, hAccel);
    }
    lhcdoc = 0;
}

void IndexMouse(
    HWND hWindow,
    UINT message,
    WPARAM wParam,
    MYPOINT pt)
{
    int iCard;
    MSG msg;

    if (CardPhone != CCARDFILE ||           /* not in card mode */
        (iCard = MapPtToCard(pt)) <= -1)    /* see if click on a card or background */
        return;

    if (iCard != iFirstCard)        /* if on another card */
    {                               /* bring it to front */
        if (!CheckForBusyObjects()) /* switch only if no busy objects around */
            ScrollCards(hCardWnd, SB_THUMBPOSITION, iCard);
    }
    else if (message == WM_LBUTTONDBLCLK)   /* if double click on first */
    {                                       /* bring up index box */
        SetCapture(hWindow);
        while(GetKeyState(VK_LBUTTON) < 0)
        {
            PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, TRUE);
            PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, TRUE);
        }
        ReleaseCapture();
        IndexInput(hWindow, HEADER);
    }
}


/*
 * paint the cards and their titles top to bottom, right to left
 *
 */

void IndexPaint( HDC hDC )
{
    LPCARDHEADER Cards;
    LPCARDHEADER lpTCards;
    RECT rect;
    int idCard;
    int    x, y;
    int dx, dy;
    int i;
    DWORD rgbOld;
    DWORD rgbTextOld;


    /* calc how many headers will fit in the current space */

    /* fully visible headers */
    cFSHeads = min(yFirstCard / ySpacing + 1, cCards);

    cScreenHeads = min(cFSHeads + 1, cCards);
    cScreenCards = cCards;

    dx = 2 * CharFixWidth;
    dy = ySpacing;

    y = yFirstCard - (cScreenCards - 1) * dy;
    x = xFirstCard + (cScreenCards - 1) * dx;

    idCard = (iFirstCard + cScreenCards-1) % cCards;    /* index of card */

    Cards = (LPCARDHEADER) GlobalLock(hCards);/* hold these guys down */
    lpTCards = &Cards[idCard];        /* ptr to card header */

    /* display the headers and draw card parts */

    rgbOld = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
    rgbTextOld = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    for (i = 0; i < cScreenCards; i++)
    {
        SetRect(&rect,
                x + CardWidth - dx, y + dy,
                x + CardWidth,      y + CardHeight);

        /* is part of this possibly visible? */
        if (rect.bottom > 0)
        {
            /* draw two background parts of card */
            FillRect(hDC, &rect, hbrCard);

            SetRect(&rect,
                    x,             y,
                    x + CardWidth, y + dy);
            FillRect(hDC, &rect, hbrCard);
            /* draw outline */
            SetRect(&rect,
                    x,             y,
                    x + CardWidth, y + CardHeight);
            FrameRect(hDC, &rect, hbrBorder);
        }

        x -= dx;    /* move right to left */
        y += dy;    /* move top to bottom */

        lpTCards--;    /* bump pointer to card headers */
        idCard--;    /* and the count */

        if (idCard < 0)     /* did this wrap around to end? */
        {
            idCard = cCards - 1;
            lpTCards = &Cards[idCard];
        }
    }
    SetBkColor(hDC, rgbOld);
    SetTextColor(hDC, rgbTextOld);

    PaintNewHeaders( hDC );

    /* draw double line under top card header */
    SetRect(&rect,
            xFirstCard,             yFirstCard + 1 + CharFixHeight,
            xFirstCard + CardWidth, yFirstCard + 4 + CharFixHeight);

    FillRect(hDC, &rect, hbrCard);
    FrameRect(hDC, &rect, hbrBorder);

    InvalidateRect(hEditWnd, NULL, TRUE);

    GlobalUnlock(hCards);
}
