/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
*         descr:  text synthesis for outline and shadow                 *
*         date:    02/19/87 @ 16:50                                     *
\***********************************************************************/

#include <windows.h>
#include <port1632.h>
#include <shellapi.h>

#include "oleglue.h"
#include "pbrush.h"

#ifdef PENWIN
#include <penwin.h>
extern int (FAR PASCAL *lpfnProcessWriting)(HWND, LPRC);
BOOL PUBLIC TapInText( POINT tapPt );
extern (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL);
extern BOOL (FAR PASCAL *lpfnCorrectWriting)(HWND, LPTSTR, int, LPRC, DWORD, DWORD);
extern BOOL (FAR PASCAL *lpfnIsPenEvent)( WORD, LONG);
#endif


int RedrawText(HWND hWnd);

extern LOGFONT theFont;
#ifdef PENWIN
extern BOOL TerminateKill;
extern BOOL fIPExists;
#endif


// defined in ntdef.h but cannot include the file
#ifdef UNICODE
#define TUCHAR  WCHAR
#else
#define TUCHAR  unsigned char
#endif


#ifdef WIN32
#define IsDBCSLeadByte(a) 0
#else
BOOL FAR PASCAL IsDBCSLeadByte( BYTE );
#endif

extern BOOL bExchanged;
extern RECT rDirty;


#define REDRAW 0
#define DELCHAR 1
#define ADDCHAR 2

static HDC paintDC = NULL, charDC = NULL;
static POINT startPt,endPt;
static int yExtra;
static int xBitmap, yBitmap;
static TEXTMETRIC tm;
static BOOL inText = FALSE;
static int charHgt;
static HBRUSH hbForeg, hbBackg;
static TUCHAR textBuff[TEXTBUFFsize+1], *linePtr;
static TUCHAR tempBuff[TEXTBUFFsize+1], *ptempBuff, *ptextBuff;
static int buffIdx, lineLen;
static HDC hMonoDC;
static HBITMAP hMonoBM;
static HFONT oldFont, newFont;
static RECT rect1;              /* rect changed by this output */


BOOL
PasteText(HGLOBAL hText)
{
    LPTSTR lpText;
    int i, j, cText;
    HWND hWnd = pbrushWnd[PAINTid];


    if((lpText = GlobalLock(hText)) == NULL)
        return FALSE;

    cText = lstrlen(lpText);
    if ((cText + lstrlen(textBuff)) >= TEXTBUFFsize)
    {
        GlobalUnlock(hText);
        CloseClipboard();
        return FALSE;
    }

    for (i = 0, j = 0; j < cText; j++)
    {
        if (lpText[j] != LF)    /* skip LF */
        {
            textBuff[buffIdx+i] = lpText[j];
            i++;
        }
    }

    textBuff[buffIdx+i] = 0;
    buffIdx = lstrlen(textBuff);

    GlobalUnlock(hText);
    SendMessage(hWnd, WM_CHANGEFONT, 0, 0L);

    return TRUE;
}

BOOL
PasteTextFromOle(void)
{
    HGLOBAL hText = NULL;
    CLIPFORMAT cf = CF_UNICODETEXT;

    if(ghGlobalToPaste != NULL)
    {
        return PasteText(ghGlobalToPaste);
    }
    else if(GetTypedHGlobalFromOleClipboard(cf, &hText))
    {
        PasteText(hText);
        goto ExitAndFree;
    }
    else
    {
        cf = CF_TEXT;
        if(GetTypedHGlobalFromOleClipboard(cf, &hText))
        {
            PasteText(hText);
            goto ExitAndFree;
        }
    }
    return FALSE;

ExitAndFree:
    GlobalFree(hText);
    return TRUE;
}



BOOL PRIVATE
Text2DPInit(HWND hWnd)
{
    HFONT hOldFont;

#ifdef JAPAN              //  added 01 Sep. 1992  by Hiraisi
    GetClientRect( hWnd, &cR );
#endif

    if(!(paintDC = GetDisplayDC(hWnd)))
        goto Error1;

    if(!(newFont = CreateFontIndirect(&theFont)))
        goto Error2;

    if(!(hMonoBM = CreateBitmap(imageWid, imageHgt,
            1, 1, NULL)))
    {
        goto Error3;
    }

    if(!(hMonoDC = CreateCompatibleDC(hdcWork)))
        goto Error4;

    SelectObject(hMonoDC, hMonoBM);
    hOldFont = SelectObject(hMonoDC, newFont);

    SetTextColor(hMonoDC, RGB(255, 255, 255));
    SetBkColor(hMonoDC, RGB(0, 0, 0));

    GetTextMetrics(hMonoDC, &tm);

#ifdef JAPAN      // added 14 Apr. 1992 by Hiraisi
    bTrueType = tm.tmPitchAndFamily & TMPF_TRUETYPE;
    yExtra = shadow ? 4 : (outline ? 2 : 0);
    charHgt = tm.tmHeight + tm.tmExternalLeading + yExtra;

//    if( bVertical && (bTrueType == TMPF_TRUETYPE) ) {
    if( bVertical ){
        startPt.x -= tm.tmDescent;  /* move start pt left */

        xBitmap = tm.tmHeight + yExtra;
        yBitmap = 2*(tm.tmMaxCharWidth + yExtra + tm.tmOverhang);

        xCaret = xBitmap;
        yCaret = 1;
        flagVert  = TRUE;
    }
    else{
        startPt.y -= tm.tmAscent;  /* move start pt up */

        xBitmap = 2*(tm.tmMaxCharWidth + yExtra + tm.tmOverhang);
        yBitmap = tm.tmHeight + yExtra;

        xCaret = 1;
        yCaret = yBitmap;
        flagVert   = FALSE;
    }
#else
    startPt.y -= tm.tmAscent;  /* move start pt up */

    yExtra = shadow ? 4 : (outline ? 2 : 0);
    charHgt = tm.tmHeight + tm.tmExternalLeading + yExtra;

    xBitmap = 2*(tm.tmMaxCharWidth + yExtra + tm.tmOverhang);
    yBitmap = tm.tmHeight + yExtra;
#endif

    if(!(hbForeg = CreateSolidBrush(rgbColor[theForeg])))
        goto Error9;
    if(!(hbBackg = CreateSolidBrush(rgbColor[theBackg])))
        goto Error10;

    return(TRUE);

Error10:
    DeleteObject(hbForeg);
Error9:
    DeleteDC(hMonoDC);
Error4:
    DeleteObject(hMonoBM);
Error3:
    DeleteObject(newFont);
Error2:
    ReleaseDC(hWnd, paintDC);
Error1:
    inText = FALSE;
    return(FALSE);
}

#if defined(JAPAN) || defined (KOREA)  //  added by Hiraisi
int pbGetTextExtent( HDC hDC, LPTSTR pString, int iLen )
{
    SIZE sz;
    WORD width;

    GetTextExtentPoint(hDC, pString, iLen, &sz);
    if( bVertical && (bTrueType != TMPF_TRUETYPE) )
        width = sz.cy;
    else
        width = sz.cx;
    return width;
}
#endif


//
// For True Type fonts, takes into account negative C widths to determine
// the correct TextExtent
//
int
GetBitmapTextWidth(HDC hDC, LPTSTR Line, int Len)
{
    ABC abc;
    SIZE sz;
#ifdef JAPAN     //  added 16 Apr. 1992  by Hiraisi
    int width;
#endif

#ifdef JAPAN  //  added 16 Apr. 1992  by Hiraisi
    width = pbGetTextExtent(hDC, Line, Len);
#else
    GetTextExtentPoint(hDC, Line, Len,&sz);
#endif

    if (GetCharABCWidths(hDC, Line[Len-1], Line[Len-1], &abc))
    {
        if (abc.abcC < 0)
#ifdef JAPAN
            width += -abc.abcC;
#else
            sz.cx += -abc.abcC;
#endif
    }
// Should add the A width of the first char in the line if A width is negative?
//        if (abc.abcA < 0)
//            width += -abc.abcA;     or    sz.cx += -abc.abcA;
#ifdef JAPAN
    return width;
#else
    return sz.cx;
#endif
}

int PRIVATE
MakeText(HDC paintDC, int x, int y,
        LPTSTR buff, int len, HDC charDC,
        BOOL bShadow, BOOL bOutline, int Action)
{
    int width,height,xx,yy;
    HBRUSH oldBrush;
    BOOL b;
    int i,j;

    if(len <= 0)
        return(0);

    /* yes, I do mean yExtra below, and the +1 is to work with script */
    width = GetBitmapTextWidth(hMonoDC, buff, len) + yExtra + 1;
    height = yBitmap;
    xx = x;
    yy = y;
    if (bShadow)
    {
        xx -= 4;
        width += 5;
        yy += 4;
        height += 5;
    }
    else if (bOutline)
    {
        xx -= 1;
        width += 2;
        yy -= 1;
        height += 2;
    }

    if(Action == DELCHAR)
    {
        --len;
        b=BitBlt(hdcWork, xx, yy, width, height,
               hdcImage, xx, yy, SRCCOPY);
    }

    PatBlt(hMonoDC, xx, yy, width, height, BLACKNESS);
    b=TextOut(hMonoDC,x,y,buff,len);
    oldBrush = SelectObject(hdcWork, hbBackg);
    if(bShadow)
    {
        for(i=0; i<5; ++i)
            BitBlt(hdcWork, x-i, y+i, width, yBitmap,
                    hMonoDC, x, y, ROP_DSPDxax);
    }
    else if(bOutline)
    {
        for(i=-1; i<2; ++i)
        {
            for(j=-1; j<2; ++j)
            {
                BitBlt(hdcWork, x+i, y+j, width, yBitmap,
                        hMonoDC, x, y, ROP_DSPDxax);
            }
        }
    }
    if (oldBrush)
        SelectObject(hdcWork, oldBrush);

    oldBrush = SelectObject(hdcWork, hbForeg);
    b = BitBlt(hdcWork,x,y,
           width,yBitmap,
           hMonoDC,x,y,ROP_DSPDxax);
    if (oldBrush)
        SelectObject(hdcWork, oldBrush);

    BitBlt(paintDC,xx - imageView.left,yy - imageView.top,width,height,
           hdcWork,xx,yy,SRCCOPY);

    if (rect1.left > xx)
        rect1.left = xx;
    if (rect1.right < xx + width)
        rect1.right = xx + width;
    if (rect1.top > yy)
        rect1.top = yy;
    if (rect1.bottom < yy + height)
        rect1.bottom = yy + height;

    UnionWithRect(&rDirty, &rect1);
}

void
Text2DP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
    POINT newPt;
    TUCHAR c;
    SIZE sz;


    LONG2POINT(lParam, newPt);
    switch (code)
    {
    case WM_LBUTTONDOWN:

#ifdef PENWIN
//
// NOTE: there is a matching ifdef for the closing brace
//
      ShowCursor(FALSE);    // Hide cursor

      if( !lpfnRegisterPenApp || (*lpfnProcessWriting)(hWnd, NULL) < 0)
      {
        if(lpfnIsPenEvent && ((*lpfnIsPenEvent)( code, GetMessageExtraInfo())))
          {
          if(fIPExists)
            {
            if(TapInText(newPt))
              {
              ShowCursor(TRUE);    // Unhide cursor
              break;
              }
            }
          }
#endif // PENWIN

           /* initialize text input */
           if(bExchanged)
           {
              PasteDownRect(rDirty.left, rDirty.top,
                 rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);
           }

           SendMessage(hWnd, WM_TERMINATE, 0, 0L);
           inText = TRUE;

#ifdef PENWIN
           // Clear the textBuff
           while (buffIdx--)
              textBuff[buffIdx] = (TUCHAR) 0;
#endif

           lineLen = buffIdx = 0;
           linePtr = textBuff;
           *linePtr = TEXT('\0');

           startPt = newPt;

           if(!Text2DPInit(hWnd))
           {

#ifdef JAPAN    //KKBUGFIX     // added by Hiraisi  (BUG#2219/WIN31 in Japan)
               ShowCursor(TRUE);
#endif
               break;
           }
           endPt = startPt;

           rect1.left = rect1.right = startPt.x + imageView.left;
           rect1.top  = rect1.bottom= startPt.y + imageView.top;

#if defined(JAPAN) || defined(KOREA) //  added 10 Apr. 1992  by Hiraisi
           CreateCaret(hWnd, (HBITMAP)NULL, xCaret, yCaret);
#else
           CreateCaret(hWnd, (HBITMAP)NULL, 1, yBitmap);
#endif

#ifdef PENWIN
           fIPExists = TRUE;
#endif

           SetCaretPos(endPt.x, endPt.y);
           ShowCaret(hWnd);

#ifdef PENWIN
        }
      ShowCursor(TRUE);    // Hide cursor
#endif // PENWIN
        break;

    /* display a typed character */
    case WM_CHAR:
        if(!inText)
            break;

        HideCaret(NULL);
        c = (TUCHAR) wParam;

        switch(c)
        {
        case BS:
            if (buffIdx <= 0) break;
            --buffIdx;
            if(lineLen == 0)
            {
                endPt.y -= charHgt;
                for(--linePtr;
                        *(linePtr-1)!=CR && linePtr>textBuff;
                        --linePtr, ++lineLen)
                {
                    /* do nothing */ ;
                }
                if (lineLen > 0)
                {
                    GetTextExtentPoint(hMonoDC, linePtr, lineLen,&sz);
                    endPt.x = startPt.x + yExtra - tm.tmOverhang + sz.cx;
                } else
                    endPt.x = startPt.x;
            }
            else
            {
                MakeText(paintDC, startPt.x + imageView.left,
                                  endPt.y + imageView.top,
                        linePtr, lineLen, charDC, shadow, outline, DELCHAR);
                --lineLen;
            }
#if defined(PENWIN) || defined(JAPAN) || defined(KOREA)
            textBuff[buffIdx] = (TUCHAR) 0;
#endif
            if (lineLen > 0)
            {
                GetTextExtentPoint(hMonoDC, linePtr, lineLen,&sz);
                endPt.x = startPt.x + yExtra - tm.tmOverhang + sz.cx;
            }
            else
                endPt.x = startPt.x;
            SetCaretPos(endPt.x, endPt.y);
            break;
        case CR:
            if( buffIdx < TEXTBUFFsize)
            {
                textBuff[buffIdx++] = c;
                linePtr = &textBuff[buffIdx];
                lineLen = 0;

                endPt.x = startPt.x;
                endPt.y += charHgt;
                SetCaretPos(endPt.x, endPt.y);
            }
            break;
        default:
            if(buffIdx < TEXTBUFFsize)
            {
                textBuff[buffIdx++] = c;
                ++lineLen;
                MakeText(paintDC, startPt.x + imageView.left,
                         endPt.y + imageView.top,
                         linePtr, lineLen, charDC, shadow, outline, ADDCHAR);

                GetTextExtentPoint(hMonoDC, linePtr, lineLen,&sz);
#ifdef JAPAN
                endPt.x = startPt.x + yExtra - tm.tmOverhang +sz.cx;
#else
                endPt.x = startPt.x - tm.tmOverhang +sz.cx;
#endif
                SetCaretPos(endPt.x, endPt.y);
            }

        }

        ShowCaret(hWnd);
        break;

    case WM_CHANGEFONT:
        if(!inText)
            break;

        HideCaret(NULL);

        BitBlt(hdcWork,rect1.left, rect1.top,
                       rect1.right - rect1.left, rect1.bottom - rect1.top,
                       hdcImage, rect1.left, rect1.top, SRCCOPY);

        BitBlt(paintDC,rect1.left - imageView.left,
                       rect1.top - imageView.top,
                       rect1.right - rect1.left, rect1.bottom - rect1.top,
                       hdcWork, rect1.left,rect1.top, SRCCOPY);

#if defined(JAPAN) || defined(KOREA)   //  added by Hiraisi
        /*
         * The reason is as follows.
         *    Parts of bk color of texts become color of the parent window
         *   when text style is changed, example from non-bold to bold.
         *    This occurs when color of the parent window is not white and
         *   an IME is open.
        */
        SendMessage(hWnd, WM_PAINT, 0, 0L);
#endif

        SendMessage(hWnd, WM_TERMINATE, 0, 0L);
        inText = TRUE;

        /* Actually, we just want an undo here, and later we will redo */
//      UndoRect(0, 0, 0, 0);

#if defined(JAPAN) || defined(KOREA)     //  added 15 Apr. 1992  by Hiraisi
        if( flagVert )
            startPt.x += tm.tmDescent;  /* move start point back to center */
        else
            startPt.y += tm.tmAscent;  /* move start point back to center */
#else
        startPt.y += tm.tmAscent;  /* move start point back to center */
#endif

        if(!Text2DPInit(hWnd))
            break;

        endPt = startPt;

#if defined(JAPAN) || defined(KOREA)  //  added 15 Apr. 1992  by Hiraisi
        if( bVertical )
            RedrawTextVert(hWnd);
        else
            RedrawText(hWnd);
#else
        RedrawText(hWnd);
#endif

        ShowCaret(NULL);
        break;

    case WM_PASTE:
        if (inText)
        {
            /* In text mode, we have already established a startPt
             * Paste clipbrd text at the end of the current text buffer */
            if (PasteTextFromOle())
                gfDirty = TRUE;
        }
        else
        {
            /* Tell the user to identify the startPt for the Paste and then
             * try again */
            SimpleMessage(IDSTextPasteMsg, TEXT(""), MB_OK);
        }
        break;

    case WM_TERMINATE:

        if(!inText)
            break;
        inText = FALSE;

        DestroyCaret();
#ifdef PENWIN
        fIPExists = FALSE;
#endif
        DeleteObject(hbForeg);
        DeleteObject(hbBackg);
        DeleteDC(hMonoDC);
        DeleteObject(hMonoBM);
        DeleteObject(newFont);
        ReleaseDC(hWnd, paintDC);
        break;

#ifdef PENWIN
        case WM_CORRECTTEXT:
        if(!inText)
            break;

        // Set TerminateKill so that the WM_TERMINATE is not sent
        TerminateKill = FALSE;
        HideCaret(hWnd);
        if((*lpfnCorrectWriting)( hWnd, textBuff, TEXTBUFFsize, NULL,
                      CWR_STRIPLF|CWR_STRIPTAB, NULL))
           {
#ifdef STRIPLINEFEEDS
           ptempBuff = tempBuff;
           ptextBuff = textBuff;

           // Also need to strip tabs for now.
           while( *ptextBuff )
           {
              if( *ptextBuff != 0x000a && *ptextBuff != 0x0009 )
                 *ptempBuff++ = *ptextBuff;
              ptextBuff++;
           }
           *ptempBuff = (TCHAR) 0;

           lstrcpy(textBuff, tempBuff);
#endif // STRIPLINEFEEDS
           buffIdx = lstrlen(textBuff);
           }
        TerminateKill = TRUE;
        UndoRect(0, 0, 0, 0);
        endPt = startPt;
#if defined(JAPAN) || defined(KOREA)  //  added 15 Apr. 1992  by Hiraisi
        if( bVertical )
            RedrawTextVert(hWnd);
        else
            RedrawText(hWnd);
#else
        RedrawText(hWnd);
#endif
        ShowCaret(hWnd);
        break;
#endif // PENWIN

#ifdef JAPAN //KKBUGFIX       // added by Hiraisi  04 Sep. 1992 (in Japan)
    case WM_RESETCARET:
        CreateCaret(hWnd, (HBITMAP)NULL, xCaret, yCaret);
        SetCaretPos(endPt.x, endPt.y);
        ShowCaret(hWnd);
        break;
#endif  //JAPAN //KKBUGFIX

    default:
        break;
    }
    return;
}

int
PasteTextFromClipbrd(void)
{
    HANDLE hText;
    LPTSTR lpText;
    int i, j, TextLen;
    HWND hWnd = pbrushWnd[PAINTid];


    /* quit if we cannot open the clipbrd */
    if (!OpenClipboard(hWnd))
    {
        PbrushOkError(IDSNoClipboard, MB_ICONEXCLAMATION);
        return FALSE;
    }

#ifdef UNICODE
    if (!(hText = GetClipboardData(CF_UNICODETEXT)) || !(lpText = GlobalLock(hText)))
#else
    if (!(hText = GetClipboardData(CF_TEXT)) || !(lpText = GlobalLock(hText)))
#endif
    {
        CloseClipboard();
        return FALSE;
    }

    TextLen = lstrlen(lpText);
    if ((TextLen + lstrlen(textBuff)) >= TEXTBUFFsize)
    {
        GlobalUnlock(hText);
        CloseClipboard();
        return FALSE;
    }

    for (i = 0, j = 0; j < TextLen; j++)
    {
        if (lpText[j] != LF)    /* skip LF */
        {
            textBuff[buffIdx+i] = lpText[j];
            i++;
        }
    }

    textBuff[buffIdx+i] = 0;
    buffIdx = lstrlen(textBuff);

    GlobalUnlock(hText);
    CloseClipboard();
    SendMessage(hWnd, WM_CHANGEFONT, 0, 0L);

    return TRUE;
}

int
RedrawText(HWND hWnd)
{
    int i;
    SIZE sz;

    for(i=lineLen=0, linePtr=textBuff; i < buffIdx; ++i)
    {
        if (textBuff[i] == CR)
        {
            MakeText(paintDC, startPt.x + imageView.left,
                     endPt.y + imageView.top,
                     linePtr, lineLen, charDC, shadow, outline, REDRAW);
            linePtr += lineLen + 1;
            lineLen = 0;
            endPt.y += charHgt;
        }
        else
            ++lineLen;
    }
    MakeText(paintDC, startPt.x + imageView.left,
            endPt.y + imageView.top,
            linePtr, lineLen, charDC, shadow, outline, REDRAW);

    if (lineLen > 0)
    {
        GetTextExtentPoint(hMonoDC, linePtr, lineLen,&sz);
        endPt.x = startPt.x + yExtra - tm.tmOverhang + sz.cx;
    }
    else
        endPt.x = startPt.x;

#if defined(JAPAN) || defined(KOREA)  //  added 10 Apr. 1992  by Hiraisi
    CreateCaret(hWnd, (HBITMAP)NULL, xCaret, yCaret);
#else
    CreateCaret(hWnd, (HBITMAP)NULL, 1, yBitmap);
#endif

#ifdef PENWIN
        fIPExists = TRUE;
#endif
    SetCaretPos(endPt.x, endPt.y);
    ShowCaret(hWnd);

    return TRUE;
}

