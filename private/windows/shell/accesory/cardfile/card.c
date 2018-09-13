#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

#if !defined(UNICODE) && defined(JAPAN)
/* Edit Control tune up routine */
WORD NEAR PASCAL EatOneCharacter(HWND);
/*
 * routine to retrieve WM_CHAR from the message queue associated with hwnd.
 * this is called by EatString.
 */
WORD NEAR PASCAL EatOneCharacter(hwnd)
register HWND hwnd;
{
    MSG msg;
    register int i = 10;

    while (!PeekMessage (&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE))
    {
        if (--i == 0)
            return -1;
        Yield ();
    }
    return (msg.wParam & 0x00FF);
}

BOOL FAR PASCAL EatString(HWND,LPSTR,WORD);
/*
 * This routine is called when the Edit Control receives WM_IME_REPORT
 * with IR_STRINGSTART message. The purpose of this function is to eat
 * all WM_CHARs between IR_STRINGSTART and IR_STRINGEND and to build a
 * string block.
 */
BOOL FAR PASCAL EatString(hwnd, lpSp, cchLen)
register HWND hwnd;
LPSTR lpSp;
WORD cchLen;
{
    MSG msg;
    int i = 10; /* loop counter for avoid infinite loop */
    int w;

    *lpSp = '\0';
    if (cchLen < 4)
        return NULL;    /* not enough */
    cchLen -= 2;

    while (i--)
    {
        while (PeekMessage (&msg, hwnd, NULL, NULL, PM_REMOVE))
        {
            i = 10;
            switch (msg.message)
            {
                case WM_CHAR:
                    *lpSp++ = (BYTE)msg.wParam;
                    cchLen--;
                    if (IsDBCSLeadByte((BYTE)msg.wParam))
                    {
                        if ((w = EatOneCharacter(hwnd)) == -1)
                        {
                            /* Bad DBCS sequence - abort */
                            lpSp--;
                            goto WillBeDone;
                        }
                        *lpSp++ = (BYTE)w;
                        cchLen--;
                    }
                    if (cchLen <= 0)
                        goto WillBeDone;   /* buffer exhausted */
                    break;

                case WM_IME_REPORT:
                    if (msg.wParam == IR_STRINGEND)
                    {
                        if (cchLen <= 0)
                            goto WillBeDone; /* no more room to stuff */
                        if ((w = EatOneCharacter(hwnd)) == -1)
                            goto WillBeDone;
                        *lpSp++ = (BYTE)w;
                        if (IsDBCSLeadByte((BYTE)w))
                        {
                            if ((w = EatOneCharacter(hwnd)) == -1)
                            {
                                /* Bad DBCS sequence - abort */
                                lpSp--;
                                goto WillBeDone;
                            }
                            *lpSp++ = (BYTE)w;
                        }
                        goto WillBeDone;
                    }

                    /* Fall through */

                default:
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
            }
        }
    }
    /* We don't get WM_IME_REPORT + IR_STRINGEND
     * But received string will be OK
     */

WillBeDone:

    *lpSp = '\0';

    return TRUE;
}
#endif


#ifdef WIN32

/* These are C equivalents to functions that were in INDOS2.ASM */

VOID RepMov(
    LPBYTE lpDest,
    LPBYTE lpSrc,
    UINT n)
{
   while (n--)
   {
      *lpDest++ = *lpSrc++;
   }
}


VOID RepMovDown(
    LPBYTE lpDest,
    LPBYTE lpSrc,
    UINT n)
{
   lpDest += n;
   lpSrc  += n;

   lpDest--;
   lpSrc--;

   while (n--)
   {
      *lpDest-- = *lpSrc--;
   }
}

#endif

NOEXPORT BOOL NEAR CardKey( INT wParam);

/*
 * Hook Proc for the multi line Edit control.
 * In picture mode,
 *      WM_SETCURSOR - sets the arrow cursor,
 *      mouse msgs - responds to left button down, up, dbl click and mouse move
 *      WM_KEYDOWN - responds to VK_UP/DOWN/LEFT/RIGHT/INSERT/DELETE
 * In text mode,
 *      implements hot key to move to a card(e.g. using Ctrl+A, goto first/last/
 *      next/prev card), by responding to WM_CHAR and WM_KEYDOWN.
 */
LONG EditWndProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LONG   lParam)
{
    PAINTSTRUCT ps;
#if !defined(UNICODE) && defined(JAPAN)
    LPSTR lpP;
    HANDLE hMem;
    HANDLE hClipSave;
#endif

    switch (message) {

#if !defined(UNICODE) && defined(JAPAN)
/*
 *
 *
 *
 */
        case  WM_IME_REPORT:
            if (EditMode != I_TEXT)
                break;
            switch (wParam)
            {
                case IR_STRING:
                    /*OutputDebugString("IR_STRING\r\n");*/
                    if (lpP = (LPSTR) GlobalLock((HANDLE)lParam))
                    {
                        CallWindowProc(lpEditWndProc, hwnd, EM_REPLACESEL, 0, (DWORD)lpP);
                        GlobalUnlock((HANDLE)lParam);
                        return 1L; /* processed */
                    }
                    break;

                case IR_STRINGSTART:
                    /* OutputDebugString("IR_STRINGSTART\r\n"); */
                    if ((hMem = GlobalAlloc(GMEM_MOVEABLE, 512)) == NULL)
                    {
                        /* OutputDebugString("Ga failed\r\n");*/
                        goto PassMessageOn;
                    }
                    if ((lpP = (LPSTR) GlobalLock(hMem)) == NULL)
                    {
                        /* OutputDebugString("Lock failed\r\n"); */
                        GlobalFree(hMem);
                        goto PassMessageOn;
                    }
                    if (EatString(hwnd, lpP, 512))
                    {
                        /* OutputDebugString("Eat ok\r\n"); */
                        CallWindowProc(lpEditWndProc, hwnd, EM_REPLACESEL, 0, (DWORD)lpP);
                        GlobalUnlock(hMem);
                        GlobalFree(hMem);
                        break;
                    }
                    GlobalUnlock(hMem);
                    GlobalFree(hMem);
            }
            break;
#endif

        case WM_DROPFILES:
            DoDragDrop(hwnd, (HANDLE)wParam, TRUE);
            break;
        case WM_SETCURSOR:
            /* use arrow cursor when in picture mode */
            if (EditMode == I_OBJECT)
                SetCursor(hArrowCurs);
            else
                goto PassMessageOn;
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
            if (EditMode == I_OBJECT)
                BMMouse(hwnd, message, wParam, MYMAKEPOINT(lParam));
            else    /* In text mode, pass it to edit control */
                goto PassMessageOn;
            break;

        case WM_CHAR:
            /* In text mode, Ctrl+'char' acts as a hot key to move to a
             * specific card. Respond to Ctrl+'A' etc */
            if (!CardChar(wParam) && (EditMode == I_TEXT))
                /* If it doesn't move our object, pass it to edit control */
                goto PassMessageOn;
            break;

        case WM_KEYDOWN:
            /* In text mode, respond to Ctrl+HOME, Ctrl+END(move to first/last card),
             * VK_NEXT/VK_PRIOR(goto next and prev cards).
             * In picture mode, respond to VK_UP/DOWN/LEFT/RIGHT/VK_INSERT/VK_DELETE
             * keys */
            if (!CardKey(wParam) && (EditMode == I_TEXT))
                /* If it doesn't affect our object, ... */
                goto PassMessageOn;
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            CallWindowProc((WNDPROC)lpEditWndProc, hEditWnd, message, (LONG)ps.hdc, 0L);
            CardPaint(ps.hdc);
            EndPaint(hwnd, &ps);
            break;

        default:
PassMessageOn:
            return CallWindowProc((WNDPROC)lpEditWndProc, hEditWnd, message, wParam, lParam);
    }
    return(0L);
}

/*
 * Responds to WM_KEYDOWN message.
 * In text mode,
 *      Ctrl+HOME/END - goto first/last card
 *      VK_PRIOR/NEXT - goto previous/next card
 *
 * In picture mode,
 *      VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_INSERT, VK_DELETE handled
 *      in BMKey
 */
NOEXPORT BOOL NEAR CardKey( INT wParam)
{
    switch(wParam)
    {
        case VK_HOME:
            /* Home         Beginning of line (don't process)
             * Shft+Home    Extend selection to beg. of line (don't process)
             * Ctrl+Home    Go to first card (process here)
             */
            if (GetKeyState(VK_CONTROL) >= 0)
                return(FALSE);
            ScrollCards(hCardWnd, SB_THUMBPOSITION, 0);
            break;

        case VK_END:
            /* End          End of line (don't process)
             * Shft+End     Extend selection to end of line (don't process)
             * Ctrl+End     Go to last card (process here)
             */
            if (GetKeyState(VK_CONTROL) >= 0)
                return(FALSE);
            ScrollCards(hCardWnd, SB_THUMBPOSITION, cCards-1);
            break;

        case VK_PRIOR:
            ScrollCards(hCardWnd, SB_LINEUP, 0);
            break;

        case VK_NEXT:
            ScrollCards(hCardWnd, SB_LINEDOWN, 0);
            break;

        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_INSERT:
        case VK_DELETE:
            if (EditMode == I_TEXT) /* should be handled by the edit control */
                return(FALSE);
            else
                BMKey((WORD) wParam);      /* we will handle this */
            return(TRUE);

        default:
            return(FALSE);
    }

    ScrollCards(hCardWnd, SB_ENDSCROLL, 0);
    return(TRUE);
}

/*
 * Checks if the user is trying to move to a specific card using a hot key
 * e.g. Ctrl+A will move it to a card starting with A
 */
BOOL CardChar(
    int ch)
{
    int fControl;
    LPCARDHEADER Cards;
    int i;
    int iCard;

    fControl = (GetKeyState(VK_CONTROL) < 0) && (GetKeyState(VK_SHIFT) < 0);
    if (!fControl || ch >= TEXT(' '))
        return(FALSE);

    /* convert control char, i.e. from Ctrl+A get 'A' */
    ch += TEXT('A') - 1;

    Cards = (LPCARDHEADER) GlobalLock(hCards);
    for (i = 0, iCard = iFirstCard+1; i < cCards; ++i, iCard++) {
        if (iCard == cCards)
            iCard = 0;
        if ((TCHAR)(DWORD)CharUpper((LPTSTR)(DWORD)(BYTE)*(Cards[iCard].line)) == ch)
            break;
    }
    GlobalUnlock(hCards);

    if (i < cCards)     /* make sure a card was found */
        ScrollCards(hCardWnd, SB_THUMBPOSITION, iCard);

    return(TRUE);
}

/*
 * this paints the bitmap in the edit window.
 */
void CardPaint(
    HDC hDC)
{
    RECT rc;

    if (CardPhone == PHONEBOOK ||   /* list mode */
        !CurCard.lpObject ||        /* no object to draw */
        fInsertComplete == FALSE)   /* InsertObject in progress */
    {
        return;
    }

    Hourglass(TRUE);

    /* If RECT is null, reget scaled object size */
    /* This will never happen with a plain static BITMAP */
    if (!CurCard.rcObject.right)
    {
        if (OleQueryBounds(CurCard.lpObject, &rc) != OLE_OK)
        {
            Hourglass(FALSE);
            ErrorMessage(E_BOUNDS_QUERY_FAILED);
            return;
        }
        FixBounds(&rc);
        SetRect(&(CurCard.rcObject),
            CurCard.rcObject.left, CurCard.rcObject.top,
            CurCard.rcObject.left + (rc.right - rc.left),
            CurCard.rcObject.top + (rc.bottom - rc.top));
    }

    /* Draw the object */
    PicDraw(&CurCard, hDC, FALSE);
    Hourglass(FALSE);
}

/*
 * Delete the ith card by removing its header
 */
void DeleteCard(
    int iCard)
{
    LPCARDHEADER Cards;

    cCards--;
    Cards = (LPCARDHEADER) GlobalLock(hCards);
    RepMov((LPBYTE)&Cards[iCard], (LPBYTE)&Cards[iCard+1], (cCards-iCard)*sizeof(CARDHEADER));
    GlobalUnlock(hCards);

    InitPhoneList(hListWnd, iFirstCard);
}

/*
 * Add a card in order in the card array.
 */
int AddCurCard(
    void)
{
    LPCARDHEADER Cards;
    int i;

    Cards = (LPCARDHEADER) GlobalLock(hCards);
    for (i = 0; i < cCards; i++)
    {
        if (lstrcmp(CurCardHead.line, Cards[i].line) <= 0)
            break;
    }

    if (i != cCards)
        RepMovDown((LPBYTE)&Cards[i+1], (LPBYTE)&Cards[i], (cCards - i) * sizeof(CARDHEADER));

    Cards[i] = CurCardHead;
    GlobalUnlock(hCards);
    cCards++;
    /* Set highlight to the new card */
    InitPhoneList(hListWnd, i);

    return(i);
}

/*
 *  save CurCardHead and assorted things??
 */
BOOL SaveCurrentCard(
    int iCard)
{
    LPCARDHEADER Cards;

    /* save the card if it's dirty */
    /* dirty if bitmap has changed or edittext has changed */
    if (CurCardHead.flags & (FDIRTY+FNEW) || SendMessage(hEditWnd, EM_GETMODIFY, 0, 0L))
    {
        /* get modified text from edit window */
        GetWindowText(hEditWnd, szText, CARDTEXTSIZE);
        if (WriteCurCard(&CurCardHead, &CurCard, szText))
        {
            if (CurCardHead.flags & FDIRTY || SendMessage(hEditWnd, EM_GETMODIFY, 0, 0L))
                fFileDirty = TRUE;

            SendMessage(hEditWnd, EM_SETMODIFY, FALSE, 0L);
            CurCardHead.flags &= (!FNEW);
            CurCardHead.flags &= (!FDIRTY);
            CurCardHead.flags |= FTMPFILE;
            Cards = (LPCARDHEADER) GlobalLock(hCards);
            Cards[iCard] = CurCardHead;
            GlobalUnlock(hCards);
        }
        else
            return(FALSE);
    }
    if (CurCard.lpObject)
        PicDelete(&CurCard);
    return(TRUE);
}

/*
 * make card # iCard the current (editable and displayed) card
 *
 * copy the header into CurCardHead    (from global memory)
 * copy the bitmap into CurCard        (from the file)
 * copy the text into the Edit window    (from the file)
 *
 */
void SetCurCard(
    int iCard)
{
    LPCARDHEADER Cards;

    /* Setting new card, remove undo possibility... */
    DeleteUndoObject();
    /* copy the header info */
    Cards = (LPCARDHEADER) GlobalLock(hCards);
    CurCardHead = Cards[iCard];
    GlobalUnlock(hCards);

    /* read in the bitmap and text stuff from the file */
    if (ReadCurCardData(&CurCardHead, &CurCard, szText))
    {
        SetEditText(szText);
        DoSetHostNames(CurCard.lpObject, CurCard.otObject);
    }
}

/*
 * Set edit windows text
 */
void SetEditText(
    TCHAR *pText)
{
    fNeedToUpdateObject = TRUE;
    SendMessage(hEditWnd, WM_SETTEXT, 0, (LONG)pText);
}

int iCardStartScroll;
int fScrolling = FALSE;

/*
 * scroll through the stack of cards
 *
 * this gets called when using the scroll bar control
 * or when a card is selected by clicking on on or
 * when someone presses movement keys
 *
 * the user can hold down a key or a mouse button and cause repetitive
 * scroll messages to be sent.  this action will update the headers but
 * the edit window should only be updated when the key or mouse is
 * released.  saving of data also follows this.
 * see the fScrolling stuff below.
 *
 * also note: some routines send SB_THUMBPOS messages here (we won't
 * get a SB_ENDSCROLL).  This case works because !fScrolling and
 * iCardStartScroll gets set correctly.
 */
BOOL ScrollCards(
    HWND hWindow,
    int cmd,
    int pos)
{
    int OldFirst = iFirstCard;    /* note where we started */

    if (cCards < 2)
        return TRUE;        /* only one card, can't scroll */

    /* two states:
        fScrolling  - we are receiving scroll messages (mouse is not released)
        !fScrolling - enter scrolling state, save current position
     */

    if (!fScrolling)
    {
        iCardStartScroll = iFirstCard;
        fScrolling = TRUE;
    }

    switch (cmd)
    {
        /* these cases always change the card and leave us in
         * Scrolling state */
        case SB_LINEUP:
            iFirstCard--;        /* next card */
            if (iFirstCard < 0)
                iFirstCard = cCards-1;
            break;

        case SB_LINEDOWN:
            iFirstCard++;        /* prev card */
            if (iFirstCard == cCards)
                iFirstCard = 0;
            break;

        case SB_PAGEUP:
            if (cFSHeads == cCards)    /* already at top? */
                break;
            iFirstCard -= cFSHeads;
            if (iFirstCard < 0)
                iFirstCard += cCards; /* a negative number */
            break;

        case SB_PAGEDOWN:
            if (cFSHeads == cCards)    /* already at bottom? */
                break;
            iFirstCard += cFSHeads;
            if (iFirstCard >= cCards)
                iFirstCard -= cCards;
            break;

            /* these cases may change the current card and
             * always leave Scrolling state */
        case SB_THUMBPOSITION:
            iFirstCard = pos;        /* for single SB_THUMB calls */
            /* fall through... */

        case SB_ENDSCROLL:
            fScrolling = FALSE;        /* leave scrolling mode */

            if (iFirstCard != iCardStartScroll)
            {
                if (SaveCurrentCard(iCardStartScroll))
                {
                    SetCurCard(iFirstCard);
                }
                else
                {
                    iFirstCard = iCardStartScroll;
                    return FALSE;        /* save failed */
                }
            }
            break;
    }

    if (iFirstCard != OldFirst)        /* did the above change anything? */
    {
        HDC hDC;
        hDC= GetDC( hWindow );
        PaintNewHeaders( hDC );    /* yes, so redraw headers */
        ReleaseDC( hWindow, hDC );
    }

    return TRUE;            /* sucessful scroll */
}

void DoCutCopy(int event)
{
    if (EditMode == I_TEXT)
        SendMessage(hEditWnd, event == CUT ? WM_CUT : WM_COPY, 0, 0L);
    else if (CurCard.lpObject)
        PicCutCopy(&CurCard, (event == CUT));
}

void DoPaste(int event)
{
    if (EditMode == I_TEXT) {
        if (!SendMessage(hEditWnd, WM_PASTE, 0, 0L))
            IndexOkError(ECLIPEMPTYTEXT);
    } else
        PicPaste(&CurCard, (event == PASTE), 0);
}

/*
 * update the text in the card headers
 */
void PaintNewHeaders( HDC hDC )
{
    int idCard;
    LPCARDHEADER Cards;
    LPCARDHEADER lpTCards;
    int xCur;
    int yCur;
    int i;
    RECT rect;
    DWORD rgbOld;
    DWORD rgbTextOld;
    HFONT  hOldFont;


    yCur = yFirstCard - (cScreenHeads - 1) * ySpacing;
    xCur = xFirstCard + (cScreenHeads - 1) * (2 * CharFixWidth);
    idCard = (iFirstCard + cScreenHeads-1) % cCards;

    Cards = (LPCARDHEADER) GlobalLock(hCards);
    lpTCards = &Cards[idCard];

    hOldFont= SelectObject(hDC, hFont);  // use our selected font

    /* for all cards with headers showing */

    rgbOld = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
    rgbTextOld = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    for (i = 0; i < cScreenHeads; i++)  /* cScreenHeads */
    {
        SetRect(&rect, xCur+1, yCur+1, xCur+CardWidth-1, yCur+ySpacing);
        ExtTextOut(hDC,
                   xCur+1, yCur+1+(ExtLeading/2),
                   ETO_OPAQUE|ETO_CLIPPED,    // use background color as fill
                   &rect,                     // clipping rect
                   lpTCards->line,            // title
                   lstrlen(lpTCards->line),   // length of title
                   NULL);                     // interchar spacing

        xCur -= (2*CharFixWidth);
        yCur += ySpacing;
        lpTCards--;
        idCard--;
        if (idCard < 0)
        {
            idCard = cCards - 1;
            lpTCards = &Cards[idCard];
        }
    }
    SetBkColor(hDC, rgbOld);
    SetTextColor(hDC, rgbTextOld);
    SelectObject(hDC, hOldFont);
    GlobalUnlock(hCards);

}
