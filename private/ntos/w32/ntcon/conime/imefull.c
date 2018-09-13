// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   imefull.c
//
//  PURPOSE:   Console IME control.
//
//  PLATFORMS: Windows NT-J 3.51
//
//  FUNCTIONS:
//    ImeOpenClose() - calls initialization functions, processes message loop
//
//  History:
//
//  27.Jul.1995 v-HirShi (Hirotoshi Shimizu)    created
//
//  COMMENTS:
//

#include "precomp.h"
#pragma hdrstop

//**********************************************************************
//
// IMEOpenClose()
//
// This routines calls IMM API to open or close IME.
//
//**********************************************************************

VOID ImeOpenClose( HWND hWnd, BOOL fFlag )
{
    HIMC            hIMC;

    //
    // If fFlag is true then open IME; otherwise close it.
    //

    if ( !( hIMC = ImmGetContext( hWnd ) ) )
        return;

    ImmSetOpenStatus( hIMC, fFlag );

    ImmReleaseContext( hWnd, hIMC );

}

#ifdef DEBUG_MODE
/************************************************************************
*
*   VirtualKeyHandler - WM_KEYDOWN handler
*
*
*   INPUT:  HWND - handle to the window for repainting output.
*           UINT - virtual key code.
*
************************************************************************/

VOID VirtualKeyHandler( HWND hWnd, UINT wParam, UINT lParam )
{
    PCONSOLE_TABLE ConTbl;
    int i;
    static int delta ;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    if ( ConTbl->fInCandidate ||
         ( ConTbl->fInComposition && !MoveCaret( hWnd ) )
       )
        return;

    switch( wParam )
    {
    case VK_HOME:   // beginning of line
        xPos = FIRSTCOL;
        break;

    case VK_END:    // end of line
        xPos = xPosLast ;
        break;

    case VK_RIGHT:
        if ( IsUnicodeFullWidth( ConvertLine[xPos] ) ){
            if (xPos > xPosLast - 2 ) break;  //last character don't move
            xPos += 2;                     //skip 2 for DB Character
        }
        else
            xPos = min( xPos+1, xPosLast );
        break;

    case VK_LEFT:

        xPos = max( xPos-1, FIRSTCOL );

        if ( IsUnicodeFullWidth( ConvertLine[xPos] ) )
            xPos--;
        break;

    case VK_BACK:   // backspace

        if ( xPos > FIRSTCOL ) {
            delta = 1 ;

            //
            // DB Character so backup one more to allign on boundary
            //
            if ( IsUnicodeFullWidth( ConvertLine[xPos] ) )
                delta = 2 ;
            //
            // Fall Through to VK_DELETE to adjust row
            //
            xPos -= delta ;
            for ( i = xPos ; i < xPosLast+2 ; i++) {
                ConvertLine[i] = ConvertLine[i+delta] ;
                ConvertLineAtr[i] = ConvertLineAtr[i+delta] ;
            }
            xPosLast -= delta ;
        }
        else     //FIRST COLUMN  don't backup -- this would change for wrapping
           break;
        goto Repaint ;
        break;
    case VK_DELETE:
        if ( !IsUnicodeFullWidth( ConvertLine[xPos] ) ) {

            //
            // Move rest of line left by one, then blank out last character
            //

            for ( i = xPos; i < xPosLast; i++ ) {
                ConvertLine[i] = ConvertLine[i+1];
                ConvertLineAtr[i] = ConvertLineAtr[i+1];
            }
            xPosLast-- ;

        } else {

            //
            // Move line left by two bytes, blank out last two bytes
            //

            for ( i = xPos; i < xPosLast; i++ ) {
                ConvertLine[i] = ConvertLine[i+2];
                ConvertLineAtr[i] = ConvertLineAtr[i+2];
            }
            xPosLast -= 2 ;
        }

        goto Repaint ;
        break;

    case VK_TAB:    // tab  -- tabs are column allignment not character
        {
         int xTabMax = xPos + TABSTOP;
         int xPosPrev;

         do {
             xPosPrev = xPos;
            if ( IsUnicodeFullWidth( ConvertLine[xPos] ) ){
                if (xPos > xPosLast - 2 ) break;  //last character don't move
                xPos += 2;                     //skip 2 for DB Character
            }
            else
                xPos = min( xPos+1, xPosLast );

         } while ( (xPos % TABSTOP) &&
                   (xPos < xTabMax) &&
                   (xPos != xPosPrev));

        }
        goto Repaint ;
        break;

    case VK_RETURN: // linefeed
        for (i = FIRSTCOL ; i < MAXCOL ; i++) {
            ConvertLine[i] = ' ' ;
            ConvertLineAtr[i] = 0 ;
        }
        xPos = FIRSTCOL;
        xPosLast = FIRSTCOL;
Repaint:
        {
        //
        // Repaint the entire line
        //
        HDC hdc;

        hdc = GetDC( hWnd );
        HideCaret( hWnd );
        DisplayConvInformation( hWnd ) ;
        ReleaseDC( hWnd, hdc );
        }
        break;
    }
    ResetCaret( hWnd );
}
#endif

/************************************************************************
*
*   CharHandler - WM_CHAR handler
*
************************************************************************/

VOID CharHandlerFromConsole( HWND hWnd, UINT Message, ULONG wParam, ULONG lParam)
{
    UINT TmpMessage ;
    DWORD dwImmRet ;
    UINT uVKey ;
    UINT wParamSave ;

    if (HIWORD(wParam) == 0){
        wParamSave = wParam ;
    }
    else {
        if (Message == WM_KEYDOWN   +CONIME_KEYDATA || Message == WM_KEYUP   +CONIME_KEYDATA ||
            Message == WM_SYSKEYDOWN+CONIME_KEYDATA || Message == WM_SYSKEYUP+CONIME_KEYDATA){
            wParamSave = 0 ;
        }
        else if(HIWORD(wParam) > 0x00ff){
            WCHAR WideChar ;
            UCHAR MultiChar ;
            WideChar = HIWORD(wParam) ;
            WideCharToMultiByte(CP_OEMCP, 0, &WideChar, 1, &MultiChar, 1, NULL, NULL) ;
            wParamSave = MultiChar ;
        }
        else {
            wParamSave = HIWORD(wParam) ;
        }
    }

    if (HIWORD(lParam) & KF_UP) // KEY_TRANSITION_UP
        TmpMessage = WM_KEYUP ;
    else
        TmpMessage = WM_KEYDOWN ;


    // Return Value of ClientImmProcessKeyConsoleIME
    // IPHK_HOTKEY          1   - the vkey is IME hotkey
    // IPHK_PROCESSBYIME    2   - the vkey is the one that the IME is waiting for
    // IPHK_CHECKCTRL       4   - not used by NT IME
    dwImmRet = ImmCallImeConsoleIME(hWnd, TmpMessage, wParam, lParam, &uVKey) ;

    if ( dwImmRet & IPHK_HOTKEY ) {
    //
    // if this vkey is the IME hotkey, we won't pass
    // it to application or hook procedure.
    // This is what Win95 does. [takaok]
    //
       return ;
    }
    else if (dwImmRet & IPHK_PROCESSBYIME) {
        BOOL Status ;

//3.51
//      uVKey = (wParamSave<<8) | uVKey ;
//      Status = ClientImmTranslateMessageMain( hWnd,uVKey,lParam);

        Status = ImmTranslateMessage(hWnd, TmpMessage, wParam, lParam);


    }
    else if (dwImmRet & IPHK_CHECKCTRL) {
        CharHandlerToConsole( hWnd, Message-CONIME_KEYDATA, wParamSave, lParam);
    }
    else
    {
        if ((Message == WM_CHAR   +CONIME_KEYDATA)||
            (Message == WM_SYSCHAR+CONIME_KEYDATA)) {
            CharHandlerToConsole( hWnd, Message-CONIME_KEYDATA, wParamSave, lParam);
        }
        else
            CharHandlerToConsole( hWnd, Message-CONIME_KEYDATA, wParam, lParam);
    }

}

VOID CharHandlerToConsole( HWND hWnd, UINT Message, ULONG wParam, ULONG lParam)
{
    PCONSOLE_TABLE ConTbl;
    WORD  ch ;
    int   NumByte = 0 ;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    if (HIWORD(lParam) & KF_UP ) {
        PostMessage( ConTbl->hWndCon,
                     Message+CONIME_KEYDATA,
                     wParam,
                     lParam) ;
        return ;
    }

    ch = LOWORD(wParam) ;
    if ((ch < UNICODE_SPACE) ||
        ((ch >= UNICODE_SPACE) &&
        ((Message == WM_KEYDOWN) || (Message == WM_SYSKEYDOWN) ))) {
#ifdef DEBUG_MODE
        VirtualKeyHandler( hWnd, wParam ,lParam) ;
#endif
        PostMessage( ConTbl->hWndCon,
                     Message+CONIME_KEYDATA,
                     wParam,
                     lParam) ;
        return ;
    }

#ifdef DEBUG_MODE
    StoreChar( hWnd, ch, 0);
#endif

    PostMessage( ConTbl->hWndCon,
                 Message+CONIME_KEYDATA,
                 wParam,          //*Dest,
                 lParam) ;
}

#ifdef DEBUG_MODE
//**********************************************************************
//
// void ImeUIMove()
//
// Handler routine of WM_MOVE message.
//
//*********************************************************************

VOID ImeUIMoveCandWin( HWND hwnd )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    if ( ConTbl->fInCandidate )
    {
        POINT           point;          // Storage for caret position.
        int             i;              // loop counter.
        int             NumCandWin;     // Storage for num of cand win.
        RECT            rect;           // Storage for client rect.

        //
        // If current IME state is in chosing candidate, here we
        // move all candidate windows, if any, to the appropriate
        // position based on the parent window's position.
        //

        NumCandWin = 0;

        GetCaretPos( (LPPOINT)&point );
        ClientToScreen( hwnd, (LPPOINT)&point );

        for ( i = 0; i < MAX_LISTCAND ; i++ )
        {
            if ( ConTbl->hListCand[ i ] )
            {
                GetClientRect( ConTbl->hListCand[ i ], &rect );

                MoveWindow( ConTbl->hListCand[ i ],
                            point.x + X_INDENT * NumCandWin,
                            point.y + Y_INDENT * NumCandWin + cyMetrics,
                            ( rect.right - rect.left + 1 ),
                            ( rect.bottom - rect.top + 1 ), TRUE );

                NumCandWin++;
            }
        }
    }
}
#endif

#ifdef DEBUG_MODE
/************************************************************************
*
*   ResetCaret - Reset caret shape to match input mode (overtype/insert)
*
************************************************************************/

VOID ResetCaret( HWND hWnd )
{

    HideCaret( hWnd );
    DestroyCaret();
    CreateCaret( hWnd,
         NULL,
         IsUnicodeFullWidth( ConvertLine[xPos] ) ?
           CaretWidth*2 : CaretWidth,
         cyMetrics );
    SetCaretPos( xPos * cxMetrics, 0 );
    ShowCaret( hWnd );

}

//**********************************************************************
//
// BOOL MoveCaret()
//
//**********************************************************************

BOOL MoveCaret( HWND hwnd )
{
    HIMC        hIMC;
    BOOL        retVal = TRUE;

    if ( !( hIMC = ImmGetContext( hwnd ) ) )
    return retVal;

    if ( ImmGetCompositionString( hIMC, GCS_CURSORPOS,
                  (void FAR *)NULL, 0 ) )
    retVal = FALSE;

    ImmReleaseContext( hwnd, hIMC );

    return retVal;
}
#endif

#ifdef DEBUG_MODE
/************************************************************************
*
*   StoreChar - Stores one character into text buffer and advances
*               cursor
*
************************************************************************/

VOID StoreChar( HWND hWnd, WORD ch, UCHAR atr )
{
    HDC hdc;

    if ( xPos >= CVMAX-3 )
        return;

    //
    // Store input character at current caret position
    //
    ConvertLine[xPos] = ch;
    ConvertLineAtr[xPos] = atr;
    xPos++ ;
    xPosLast = max(xPosLast,xPos) ;

    //
    // Repaint the entire line
    //
    hdc = GetDC( hWnd );
    HideCaret( hWnd );
    DisplayConvInformation( hWnd ) ;
    ResetCaret( hWnd );
    ReleaseDC( hWnd, hdc );

}
#endif
