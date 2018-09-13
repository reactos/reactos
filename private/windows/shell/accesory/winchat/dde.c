/*---------------------------------------------------------------------------*\
| DDE MODULE
|   This module contains the routines necessary for maintaining dde
|   conversations.
|
|   FUNCTIONS
|   ---------
|   CreateCharData
|   CreatePasteData
|   SendFontToPartner
|
|
| Copyright (c) Microsoft Corp., 1990-1993
|
| created: 01-Nov-91
| history: 01-Nov-91 <clausgi>  created.
|          29-Dec-92 <chriswil> port to NT, cleanup.
|          19-Oct-93 <chriswil> unicode enhancements from a-dianeo.
|
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <ddeml.h>
#include <commdlg.h>
#include <commctrl.h>
#include "winchat.h"
#include "globals.h"

// CP963
BOOL TranslateMultiBytePosToWideCharPos( HWND hWnd, DWORD dwStart, DWORD dwEnd, LPDWORD lpdwStart, LPDWORD lpdwEnd )
{

    INT     nLine=0, wChars = 0, i, j, delta;   // Just to make compiler happy, initialize wChars here.
    DWORD   mCnt=0, mChars, offset, p_offset=0;
    HANDLE  hText;
    PTCHAR  pStartText;
    CHAR   szBuff[800];
    LONG   wStart, wEnd;


    *lpdwStart = 0;
    *lpdwEnd   = 0;
    wStart     = -1;
    wEnd       = -1;

    hText = (HANDLE)SendMessage( hWnd, EM_GETHANDLE, 0, 0);
    if( !( hText ) )
        return( FALSE );

    pStartText = LocalLock( hText);
    if( !( pStartText ) )
    {
        LocalUnlock( hText );
        return( FALSE );
    }

    while(1)
    {
        INT flag = 0;

        offset = (DWORD)SendMessage( hWnd, EM_LINEINDEX, nLine++, 0 );

        if( offset > 0 ) {                           //0D0A
            delta = offset - (p_offset+wChars);
            if( delta ) mCnt += delta;
            p_offset = offset;
        } else if ( offset == 0) {
            *lpdwStart = dwStart;
            *lpdwEnd   = dwEnd;
            LocalUnlock( hText );
            return( TRUE );
        } else {
            LocalUnlock( hText );
            return( FALSE );
        }

        wChars = (WORD)SendMessage( hWnd, EM_LINELENGTH, offset, 0 );

        mChars = WideCharToMultiByte( CP_ACP, 0, pStartText+offset,
            wChars, NULL, 0, NULL, NULL );

        if( (dwStart>=mCnt) && (dwStart<=mCnt+mChars)) flag |= 1;
        if((dwEnd>=mCnt) && (dwEnd<=mCnt+mChars))      flag |= 2;
        if( flag )
        {
            WideCharToMultiByte( CP_ACP, 0, pStartText+offset, wChars,
                szBuff, mChars, NULL, NULL );

            for(i=0,j=0; ; i++,j++ )
            {
                if( (flag&1) && (wStart==-1) )
                {
                    if(dwStart <= (mCnt+i) )
                    {
                        wStart = offset+j;
                        if( flag == 1 ) break;
                    }
                }
                if( (flag&2) && (wEnd==-1) )
                {
                    if(dwEnd <= (mCnt+i) )
                    {
                        wEnd = offset+j;
                        if( flag == 2 ) break;
                    }
                }
                if( (flag==3) && (wStart>-1) && (wEnd>-1) ) break;
                if( IsDBCSLeadByte(szBuff[i]) ) i++;
            }
        }
        if( (wStart>-1) && (wEnd>-1) )  break;
        mCnt += mChars;
    }

    *lpdwStart = (DWORD)wStart;
    *lpdwEnd   = (DWORD)wEnd;

   LocalUnlock( hText );
   return( TRUE );
}

BOOL
TranslateWideCharPosToMultiBytePos( HWND hWnd, DWORD dwStart, DWORD dwEnd, LPDWORD lpdwStart, LPDWORD lpdwEnd )
{
    INT     nLine=0, wChars = 0, i, j,delta;    // Just to make compiler happy, initialize wChars here.
    DWORD   mChars, mCnt=0, offset, p_offset=0;
    HANDLE  hText;
    PTCHAR  pStartText;
    CHAR   szBuff[800];
    LONG   mStart, mEnd;


    *lpdwStart = 0;
    *lpdwEnd = 0;
    mStart = -1;
    mEnd = -1;

    hText = (HANDLE)SendMessage( hWnd, EM_GETHANDLE, 0, 0);
    if( !( hText ) )
        return( FALSE );

    pStartText = LocalLock( hText);
    if( !( pStartText ) )
    {
        LocalUnlock( hText );
        return( FALSE );
    }

    while(1)
    {
        INT flag = 0;

        offset = (DWORD)SendMessage( hWnd, EM_LINEINDEX, nLine++, 0 );

        if( offset > 0 ) {                           //ODOA
            delta = offset - (p_offset+wChars);
            if( delta ) mCnt += delta;
            p_offset = offset;
        } else if ( offset == 0) {
            *lpdwStart = dwStart;
            *lpdwEnd   = dwEnd;
            LocalUnlock( hText );
            return( TRUE );
        } else {
            LocalUnlock( hText );
            return( FALSE );
        }

        wChars = (WORD)SendMessage( hWnd, EM_LINELENGTH, offset, 0 );

        mChars = WideCharToMultiByte( CP_ACP, 0, pStartText+offset,
            wChars, NULL, 0, NULL, NULL );

        if( (dwStart>=offset) && (dwStart<=offset+wChars)) flag |= 1;
        if( (dwEnd>=offset) && (dwEnd<=offset+wChars) ) flag |= 2;
        if( flag )
        {
            WideCharToMultiByte( CP_ACP, 0, pStartText+offset, wChars,
                szBuff, mChars, NULL, NULL );

            for(i=0,j=0; ; i++, j++ )
            {
                if( (flag&1) && (mStart==-1) )
                {
                    if(dwStart == (offset+i) )
                    {
                        mStart = mCnt+j;
                        if( flag==1 ) break;
                    }
                }
                if( (flag&2) && (mEnd==-1) )
                {
                    if(dwEnd == (offset+i) )
                    {
                        mEnd = mCnt+j;
                        if( flag == 2 ) break;
                    }
                }
                if( (flag==3) && (mStart>-1) && (mEnd>-1) ) break;
                if( IsDBCSLeadByte(szBuff[j]) ) j++;
            }
        }
        if( (mStart>-1) && (mEnd>-1) )  break;
        mCnt += mChars;
    }
    *lpdwStart = (DWORD)mStart;
    *lpdwEnd   = (DWORD)mEnd;

   LocalUnlock( hText );
   return( TRUE );
}


/*---------------------------------------------------------------------------*\
| DDE CALLBACK PROCEDURE
|   This routine handles the events sent by DDEML.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HDDEDATA CALLBACK DdeCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hszTopic, HSZ hszItem, HDDEDATA hData, DWORD lData1, DWORD lData2)
{
    HDC      hdc;
    HDDEDATA hRet;
    WPARAM   wParam;
    LPARAM   lParam;
    DWORD    dwTemp1,dwTemp2;

    hRet = (HDDEDATA)0;
    switch(wType)
    {
        case XTYP_REGISTER:
        case XTYP_UNREGISTER:
            break;


            case XTYP_XACT_COMPLETE:
                    if(lData1 == XactID)
            {
                            if(hData != (HDDEDATA)0)
                {
                                    ChatState.fServerVerified = TRUE;
                            }
                else
                {
                                    SetStatusWindowText(szNoConnect);
                                    ChatState.fConnectPending = FALSE;
                                    UpdateButtonStates();
                            }
            }
            break;


            case XTYP_ADVDATA:
            case XTYP_POKE:
            if(ChatState.fConnected && (wFmt == cf_chatdata))
            {
                DdeGetData(hData,(LPBYTE)&ChatDataRcv,sizeof(ChatDataRcv),0L);

// This is failing in some cases.  Eventually, this should be in.
//
#ifndef DDEMLBUG
                if(DdeGetLastError(idInst) == DMLERR_NO_ERROR)
#endif
                {
                    switch(ChatDataRcv.type)
                    {
                        // FE specific:
                        // We have a DBCS string selection.
                        //
                        case CHT_DBCS_STRING:
                        {
                            HANDLE  hStrBuf;
                            LPSTR   lpStrBuf;

                            hStrBuf = GlobalAlloc(GMEM_FIXED,ChatDataRcv.uval.cd_dbcs.size+1);
                            if (hStrBuf)
                            {
                                lpStrBuf = GlobalLock(hStrBuf);
                                if (lpStrBuf)
                                {
                                    DdeGetData(hData,(BYTE *)lpStrBuf,ChatDataRcv.uval.cd_dbcs.size+1,XCHATSIZEA);

#ifndef DDEMLBUG
                                    if (DdeGetLastError(idInst) == DMLERR_NO_ERROR)
#endif
                                    {
                                        SendMessage(hwndRcv,EM_SETREADONLY,(WPARAM)FALSE,0L);
                                    #ifdef UNICODE
                                    {
                                        LPWSTR lpStrUnicode;
                                        ULONG  cChars = strlen(lpStrBuf) + 1;

                                        //
                                        // Get text output position from DDE packet, and set it to EditControl.
                                        //
                                        // !!! BUG BUG BUG !!!
                                        //
                                        //  If the sender is not Unicode Edit control. the position data might be
                                        // stored for MBCS string context.
                                        // in that case, we might draw the text at incorrect position.
                                        //
                                        // We have to convert to fit Unicode string.

//                                        wParam = SET_EM_SETSEL_WPARAM(LOWORD(
//ChatDataRcv.uval.cd_dbcs.SelPos),HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos));
//                                        lParam = SET_EM_SETSEL_LPARAM(LOWORD(
//ChatDataRcv.uval.cd_dbcs.SelPos),HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos));


                                        TranslateMultiBytePosToWideCharPos(
                                            hwndRcv,
                                            (DWORD)LOWORD(ChatDataRcv.uval.cd_dbcs.SelPos),
                                            (DWORD)HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos),
                                            &dwTemp1,
                                            &dwTemp2 );
                                        // sign extend them
                                        wParam=(WPARAM)(INT_PTR)dwTemp1;
                                        lParam=(LPARAM)(INT_PTR)dwTemp2;
                                        SendMessage(hwndRcv, EM_SETSEL, wParam, lParam);

                                        //
                                        // Allocate temporary buffer for Nls conversion.
                                        //

                                        if((lpStrUnicode = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,
                                                                      cChars * 2)) == NULL )
                                        {
                                           break;
                                        }

                                        //
                                        //  Convert MBCS to Unicode. because DDE packet contains MBCS string any time
                                        // for downlevel connectivity, But if we are compiled with -DUNICODE flag,
                                        // EditControl can only handled Unicode, just convert it.
                                        //

                                        MultiByteToWideChar(CP_ACP,0,
                                                            lpStrBuf,cChars,
                                                            lpStrUnicode,cChars * sizeof(WCHAR)
                                                           );

                                        // Set string to EditControl.

                                        SendMessage(hwndRcv,EM_REPLACESEL,0,(LPARAM)lpStrUnicode);
                                        LocalFree(lpStrUnicode);
                                    }
                                    #else // !UNICODE
                                        wParam = SET_EM_SETSEL_WPARAM(LOWORD(ChatDataRcv.uval.cd_dbcs.SelPos),HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos));
                                        lParam = SET_EM_SETSEL_LPARAM(LOWORD(ChatDataRcv.uval.cd_dbcs.SelPos),HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos));
                                        SendMessage(hwndRcv,EM_SETSEL,wParam,lParam);
                                        SendMessage(hwndRcv,EM_REPLACESEL,0,(LPARAM)lpStrBuf);
                                    #endif // UNICODE
                                        SendMessage(hwndRcv,EM_SETREADONLY,(WPARAM)TRUE,0L);
                                        hRet = (HDDEDATA)TRUE;
                                    }

                                    GlobalUnlock(hStrBuf);
                                }

                                GlobalFree(hStrBuf);
                            }
                        }

                        break;


                        // This is a Unicode conversation, so mark the flag.
                        //
                        case CHT_UNICODE:
                            ChatState.fUnicode = TRUE;
                            hRet               = (HDDEDATA)TRUE;
                            break;


                        // We got a character...stuff it into the control.
                        //
                        case CHT_CHAR:

                            // In case user is tracking, so WM_CHAR is not tossed (thanks Dave)
                            //
                            SendMessage(hwndRcv,WM_LBUTTONUP,0,0L);
                            SendMessage(hwndRcv,EM_SETREADONLY,(WPARAM)FALSE,0L);

                            if (gfDbcsEnabled) {
                                TranslateMultiBytePosToWideCharPos( hwndRcv,
                                  (DWORD)HIWORD(ChatDataRcv.uval.cd_dbcs.SelPos),
                                  (DWORD)LOWORD(ChatDataRcv.uval.cd_dbcs.SelPos),
                                  &dwTemp1, &dwTemp2 );
                                // sign extend them
                                wParam=(WPARAM)(INT_PTR)dwTemp1;
                                lParam=(LPARAM)(INT_PTR)dwTemp2;
                            } else {

                                wParam = SET_EM_SETSEL_WPARAM(LOWORD(ChatDataRcv.uval.cd_char.SelPos),HIWORD(ChatDataRcv.uval.cd_char.SelPos));
                                lParam = SET_EM_SETSEL_LPARAM(LOWORD(ChatDataRcv.uval.cd_char.SelPos),HIWORD(ChatDataRcv.uval.cd_char.SelPos));
                            }
                            SendMessage(hwndRcv,EM_SETSEL,wParam,lParam);
                            SendMessage(hwndRcv,WM_CHAR,ChatDataRcv.uval.cd_char.Char,0L);
                            SendMessage(hwndRcv,EM_SETREADONLY,TRUE,0L);
                            hRet = (HDDEDATA)TRUE;
                            break;



                        // We have a paste selection.
                        //
                        case CHT_PASTEA:
                        case CHT_PASTEW:
                            {
                                HANDLE hPasteBuf,hAnsiBuf;
                                LPSTR  lpPasteBuf,lpAnsiBuf;
                                DWORD  BufSize;


                                BufSize = (ChatDataRcv.type == CHT_PASTEA ? ((ChatDataRcv.uval.cd_paste.size + 1) * sizeof(TCHAR)) : (ChatDataRcv.uval.cd_paste.size + sizeof(WCHAR)));

                                hPasteBuf = GlobalAlloc(GMEM_FIXED,BufSize);
                                if(hPasteBuf)
                                {
                                    lpPasteBuf = GlobalLock(hPasteBuf);
                                    if(lpPasteBuf)
                                    {
                                        if(ChatDataRcv.type == CHT_PASTEA)
                                        {
                                            hAnsiBuf = GlobalAlloc(GMEM_FIXED,ChatDataRcv.uval.cd_paste.size+sizeof(WCHAR));
                                            if(hAnsiBuf)
                                            {
                                                lpAnsiBuf = GlobalLock(hAnsiBuf);
                                                if(lpAnsiBuf)
                                                {
                                                    DdeGetData(hData,(BYTE *)lpAnsiBuf,ChatDataRcv.uval.cd_paste.size+sizeof(WCHAR),XCHATSIZEA);
                                                    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpAnsiBuf,ChatDataRcv.uval.cd_paste.size+sizeof(WCHAR),
                                                                        (LPWSTR)lpPasteBuf,ChatDataRcv.uval.cd_paste.size+1);

                                                    GlobalUnlock(hAnsiBuf);
                                                }

                                                GlobalFree(hAnsiBuf);
                                            }
                                        }
                                        else
                                            DdeGetData(hData,(BYTE *)lpPasteBuf,ChatDataRcv.uval.cd_paste.size+sizeof(WCHAR),XCHATSIZEW);


#ifndef DDEMLBUG
                                        if(DdeGetLastError(idInst) == DMLERR_NO_ERROR)
#endif
                                        {
                                            SendMessage(hwndRcv,EM_SETREADONLY,(WPARAM)FALSE,0L);

                                            wParam = SET_EM_SETSEL_WPARAM(LOWORD(ChatDataRcv.uval.cd_char.SelPos),HIWORD(ChatDataRcv.uval.cd_char.SelPos));
                                            lParam = SET_EM_SETSEL_LPARAM(LOWORD(ChatDataRcv.uval.cd_char.SelPos),HIWORD(ChatDataRcv.uval.cd_char.SelPos));
                                            SendMessage(hwndRcv,EM_SETSEL,wParam,lParam);
                                            SendMessage(hwndRcv,EM_REPLACESEL,0,(LPARAM)lpPasteBuf);
                                            SendMessage(hwndRcv,EM_SETREADONLY,(WPARAM)TRUE,0L);
                                            hRet = (HDDEDATA)TRUE;
                                        }

                                        GlobalUnlock(hPasteBuf);
                                    }

                                    GlobalFree(hPasteBuf);
                                }
                            }
                            break;


                        // We got a font change.  Create and stuff.
                        //
                                case CHT_FONTA:
                                case CHT_FONTW:
                            if(ChatDataRcv.type == CHT_FONTA)
                            {
                               CHATDATAA ChatDataA;

                               memcpy(ChatDataA.uval.cd_win.lf.lfFaceName,ChatDataRcv.uval.cd_win.lf.lfFaceName,LF_XPACKFACESIZE + (sizeof(COLORREF) * 2));
                               ChatDataRcv.uval.cd_win.cref  = ChatDataA.uval.cd_win.cref;
                               ChatDataRcv.uval.cd_win.brush = ChatDataA.uval.cd_win.brush;

                               MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,ChatDataA.uval.cd_win.lf.lfFaceName,LF_XPACKFACESIZE,ChatDataRcv.uval.cd_win.lf.lfFaceName,LF_XPACKFACESIZE);
                            }

                            UnpackFont(&lfRcv,&ChatDataRcv.uval.cd_win.lf);

                            hdc = GetDC(hwndApp);
                            if(hdc)
                            {
                                RcvBrushColor = PartBrushColor = GetNearestColor(hdc,ChatDataRcv.uval.cd_win.brush);
                                RcvColorref   = GetNearestColor(hdc,ChatDataRcv.uval.cd_win.cref);
                                ReleaseDC(hwndApp,hdc);
                            }

                            if(!ChatState.fUseOwnFont)
                            {
                                if(hEditRcvFont)
                                    DeleteObject(hEditRcvFont);
                                hEditRcvFont = CreateFontIndirect(&lfRcv);

                                DeleteObject(hEditRcvBrush);
                                hEditRcvBrush = CreateSolidBrush(RcvBrushColor);

                                if(hEditRcvFont)
                                {
                                    SendMessage(hwndRcv,WM_SETFONT,(WPARAM)hEditRcvFont,1L);
                                    InvalidateRect(hwndRcv,NULL,TRUE);
                                }
                            }
                            hRet = (HDDEDATA)TRUE;
                            break;

#ifdef PROTOCOL_NEGOTIATE
                        case CHT_PROTOCOL:
                            // Determine characteristics we have in common.
                            //
                            FlagIntersection(ChatDataRcv.uval.cd_protocol.pckt);

                            // Return the flavor, if not already done.
                            //
                            if(!ChatState.fProtocolSent)
                                AnnounceSupport();
                            hRet = (HDDEDATA)TRUE;
                            break;
#endif

                        default:
                            break;
                    }
                }
            }
            break;


        case XTYP_CONNECT:
            if(!ChatState.fConnected && !ChatState.fConnectPending && !ChatState.fInProcessOfDialing)
            {
                // allow connect only on the chat topic.
                //
                if(!DdeCmpStringHandles(hszTopic,hszChatTopic))
                    hRet = (HDDEDATA)TRUE;
            }
            break;


        case XTYP_CONNECT_CONFIRM:
            ChatState.fConnectPending = TRUE;
            ChatState.fAllowAnswer    = FALSE;
            ChatState.fIsServer       = TRUE;
            ghConv                    = hConv;
            nConnectAttempt           = 0;
            UpdateButtonStates();
            break;


        case XTYP_DISCONNECT:
            if(ChatState.fConnectPending || ChatState.fConnected)
            {
                if(ChatState.fConnected)
                    wsprintf(szBuf,szHasTerminated,(LPTSTR)szConvPartner);
                else
                if(ChatState.fServerVerified)
                    wsprintf(szBuf,szNoConnectionTo,(LPTSTR)szConvPartner);
                else
                    lstrcpy(szBuf,szNoConnect);

                SetStatusWindowText(szBuf);
                ChatState.fConnectPending = FALSE;
                ChatState.fConnected      = FALSE;
                ChatState.fIsServer       = FALSE;
                ChatState.fUnicode        = FALSE;

#ifdef PROTOCOL_NEGOTIATE
                ChatState.fProtocolSent   = FALSE;
#endif

                // suspend text entry
                //
                UpdateButtonStates();
                SendMessage(hwndSnd,EM_SETREADONLY,TRUE,0L);
                SetWindowText(hwndApp,szAppName);


                // stop the ringing immediately
                //
                if(ChatState.fMMSound)
                    sndPlaySound(NULL,SND_ASYNC);


                // cut the animation short
                //
                if(cAnimate)
                   cAnimate = 1;
            }
            break;


        case XTYP_REQUEST:
            break;


        case XTYP_ADVREQ:
            if(ChatState.fIsServer && ChatState.fConnected)
            {
                switch(ChatData.type)
                {
                    case CHT_DBCS_STRING:
                        hRet = CreateDbcsStringData();
                        break;

                    case CHT_CHAR:
                    case CHT_FONTA:
                    case CHT_FONTW:
                    case CHT_UNICODE:
                        hRet = CreateCharData();
                        break;

                    case CHT_PASTEA:
                    case CHT_PASTEW:
                        hRet = CreatePasteData();
                        break;

#ifdef PROTOCOL_NEGOTIATE
                    case CHT_PROTOCOL:
                        hRet = CreateProtocolData();
                        break;
#endif
                    default:
                        break;
                }
            }
            break;


        case XTYP_ADVSTART:
            if(ChatState.fConnectPending)
            {
                // is this the connect confirm attempt?
                //
                if(!DdeCmpStringHandles(hszItem,hszConnectTest))
                    return((HDDEDATA)TRUE);


                DdeQueryString(idInst,hszItem,szConvPartner,32L,0);
                wsprintf(szBuf,szIsCalling,(LPTSTR)szConvPartner);
                SetStatusWindowText(szBuf);


                // set window text on initial connect attempt
                //
                if(nConnectAttempt == 0)
                {
                    wsprintf(szBuf,TEXT("%s - [%s]"),(LPTSTR)szAppName,(LPTSTR)szConvPartner);
                    SetWindowText(hwndApp,szBuf);
                }


                if(ChatState.fAllowAnswer)
                {
                    ChatState.fConnected      = TRUE;
                    ChatState.fConnectPending = FALSE;
                    UpdateButtonStates();
                    ClearEditControls();

                    SendMessage(hwndSnd,EM_SETREADONLY,FALSE,0L);
                    wsprintf(szBuf,szConnectedTo,(LPTSTR)szConvPartner);
                    SetStatusWindowText(szBuf);

                    if(hszConvPartner)
                        DdeFreeStringHandle(idInst,hszConvPartner);

                    hszConvPartner = DdeCreateStringHandle(idInst,szConvPartner,CP_WINUNICODE);


                    // Indicate that it is a Unicode conversation.
                    //
                    PostMessage(hwndApp,WM_COMMAND,IDX_UNICODECONV,0L);


                    // SendFontToPartner(); -- would like to do this - won't work
                    // so we workaround it by posting the app window a message
                    // to perform this function...
                    //
                    PostMessage(hwndApp,WM_COMMAND,IDX_DEFERFONTCHANGE,0L);

#ifdef PROTOCOL_NEGOTIATE
                    PostMessage(hwndApp,WM_COMMAND,IDX_DEFERPROTOCOL,0L);
#endif
                    hRet = (HDDEDATA)TRUE;
                }
                else
                if(!(nConnectAttempt++ % 6))
                {
                    // Number of animation cycles == 24: ring remote.
                    //
                    cAnimate = 24;
                    idTimer  = SetTimer(hwndApp,(UINT_PTR)1,(UINT_PTR)55,NULL);
                    FlashWindow(hwndApp,TRUE);
                    DoRing(szWcRingIn);
                }
            }
            break;

        default:
            break;
    }

    return(hRet);
}


/*---------------------------------------------------------------------------*\
| FE specific:
| CREATE DBCS STRING TRANSACTION DATA
|   This routine creates a DDE object representing the DBCS string information.
|
| created: 07-Jul-93
|
\*---------------------------------------------------------------------------*/
HDDEDATA CreateDbcsStringData(VOID)
{
    HDDEDATA hTmp = (HDDEDATA)0;
    LPSTR    lpDbcsMem;
    DWORD    cbDbcs;


    hTmp = (HDDEDATA)0;
    lpDbcsMem = GlobalLock(ChatData.uval.cd_dbcs.hString);
    if(lpDbcsMem)
    {
        cbDbcs                     = (DWORD)GlobalSize(ChatData.uval.cd_dbcs.hString);
        ChatData.uval.cd_dbcs.size = (DWORD)cbDbcs;

        hTmp = DdeCreateDataHandle(idInst,NULL,sizeof(ChatData)+cbDbcs,0L,hszTextItem,cf_chatdata,0);
        if(hTmp)
        {
            DdeAddData(hTmp,(LPBYTE)&ChatData,sizeof(ChatData),0L);
            DdeAddData(hTmp,(BYTE *)lpDbcsMem,cbDbcs,XCHATSIZEA);
        }

        GlobalUnlock(ChatData.uval.cd_dbcs.hString);
    }

    GlobalFree(ChatData.uval.cd_dbcs.hString);

    return(hTmp);
}



/*---------------------------------------------------------------------------*\
| CREATE CHARACTER TRANSACTION DATA
|   This routine creates a DDE object representing the charater information.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HDDEDATA CreateCharData(VOID)
{
    HANDLE     hData;
    LPCHATDATA lpData;
    HDDEDATA   hTmp;
    BOOL       fDefCharUsed;


    hTmp = (HDDEDATA)0;
    hData = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE,sizeof(ChatData));
    if(hData)
    {
        lpData = (LPCHATDATA)GlobalLock(hData);
        if(lpData)
        {
            *lpData = ChatData;

            if(ChatData.type == CHT_FONTA)
            {
                lpData->uval.cd_win.cref  = ((LPCHATDATAA)lpData)->uval.cd_win.cref;
                lpData->uval.cd_win.brush = ((LPCHATDATAA)lpData)->uval.cd_win.brush;
            }

            hTmp = DdeCreateDataHandle(idInst,(LPBYTE)lpData,sizeof(ChatData),0L,hszTextItem,cf_chatdata,0);

            GlobalUnlock(hData);
        }

        GlobalFree(hData);
    }

    return(hTmp);
    UNREFERENCED_PARAMETER(fDefCharUsed);
}


/*---------------------------------------------------------------------------*\
| CREATE PASTE TRANSACTION DATA
|   This routine creates a DDE object representing the paste information.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HDDEDATA CreatePasteData(VOID)
{
    HDDEDATA hTmp,hRet;
    HANDLE   hClipb;
    LPTSTR   lpClipMem;
    DWORD    cbClip;
    LPSTR    lpBuf;
    DWORD    dwBytes;


    hRet = (HDDEDATA)0;
    if(OpenClipboard(hwndSnd))
    {
        hClipb = GetClipboardData(CF_UNICODETEXT);
        if(hClipb)
        {
            lpClipMem = GlobalLock(hClipb);
            if(lpClipMem)
            {
                cbClip                      = (DWORD)GlobalSize(hClipb);
                                ChatData.uval.cd_paste.size = cbClip;
                hTmp = DdeCreateDataHandle(idInst,NULL,(sizeof(ChatData)+cbClip),
                                              0,hszTextItem,cf_chatdata,0);
                if(hTmp)
                {
                    DdeAddData(hTmp,(LPBYTE)&ChatData,sizeof(ChatData),0L);

                    if(ChatData.type == CHT_PASTEA)
                    {
                        dwBytes                     = WideCharToMultiByte(CP_ACP,0,lpClipMem,-1,NULL,0,NULL,NULL);
                        ChatData.uval.cd_paste.size = dwBytes;

                        lpBuf = LocalAlloc(LPTR,dwBytes);
                        if(lpBuf)
                        {
                            WideCharToMultiByte(CP_ACP,0,lpClipMem,-1,lpBuf,dwBytes,NULL,NULL);

                            DdeAddData(hTmp,(LPBYTE)lpBuf,dwBytes,XCHATSIZEA);

                            hRet = hTmp;

                            LocalFree(lpBuf);
                        }
                    }
                    else
                    {
                        DdeAddData(hTmp,(LPBYTE)lpClipMem,cbClip,XCHATSIZEW);

                        hRet = hTmp;
                    }
                }

                GlobalUnlock(hClipb);
                        }
        }

                CloseClipboard();
    }

    return(hRet);
}


#ifdef PROTOCOL_NEGOTIATE
/*---------------------------------------------------------------------------*\
| CREATE PROTOCOL TRANSACTION DATA
|   This routine creates a DDE object representing the protocol information.
|
| created: 11-Nov-91
| history: 07-Apr-93 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
HDDEDATA CreateProtocolData(VOID)
{
    HANDLE     hData;
    LPCHATDATA lpData;
    HDDEDATA   hTmp;


    hTmp = (HDDEDATA)0;
    hData = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE,sizeof(ChatData));
    if(hData)
    {
        lpData = (LPCHATDATA)GlobalLock(hData);
        if(lpData)
        {
            ChatData.type                   = CHT_PROTOCOL;
            ChatData.uval.cd_protocol.dwVer = CHT_VER;
            ChatData.uval.cd_protocol.pckt  = GetCurrentPckt();

            *lpData = ChatData;
            hTmp    = DdeCreateDataHandle(idInst,(LPBYTE)lpData,sizeof(ChatData),0L,hszTextItem,cf_chatdata,0);

            GlobalUnlock(hData);
        }

        GlobalFree(hData);
    }

    return(hTmp);
}
#endif


#ifdef PROTOCOL_NEGOTIATE
/*---------------------------------------------------------------------------*\
| GET CURRENT PACKET
|   This routine returns the current packet capabilities of the system.
|
| created: 11-Nov-91
| history: 07-Apr-93 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
PCKT GetCurrentPckt(VOID)
{
    PCKT pckt;


    pckt = PCKT_TEXT;

    return(pckt);
}
#endif


#ifdef PROTOCOL_NEGOTIATE
/*---------------------------------------------------------------------------*\
| FLAG INTERSECTION
|   This routine determines which packet types are supporte and flags the
|   appropriate ones.
|
| created: 11-Nov-91
| history: 07-Apr-93 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FlagIntersection(PCKT pcktPartner)
{
    PCKT pcktNet;


    pcktNet = GetCurrentPckt() & pcktPartner;

    return;
}
#endif


/*---------------------------------------------------------------------------*\
| SEND FONT TO PARTNER
|   This routine sends the font-information to the partner in this
|   conversation.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID SendFontToPartner(VOID)
{
        HDDEDATA   hDdeData;
    PCHATDATAA pAChat;


    ChatData.type = (WORD)((ChatState.fUnicode ? CHT_FONTW : CHT_FONTA));

    PackFont(&ChatData.uval.cd_win.lf,&lfSnd);

    if(ChatData.type == CHT_FONTA)
    {
        pAChat                    = (PCHATDATAA)&ChatData;
        pAChat->uval.cd_win.cref  = SndColorref;
        pAChat->uval.cd_win.brush = SndBrushColor;
    }
    else
    {
        ChatData.uval.cd_win.cref  = SndColorref;
        ChatData.uval.cd_win.brush = SndBrushColor;
    }

    if(!ChatState.fIsServer)
    {
        hDdeData = DdeCreateDataHandle(idInst,(LPBYTE) &ChatData,sizeof(ChatData),0L,hszTextItem,cf_chatdata,0);
        if(hDdeData)
            DdeClientTransaction((LPBYTE)hDdeData,(DWORD)-1,ghConv,hszTextItem,cf_chatdata,XTYP_POKE,(DWORD)TIMEOUT_ASYNC,(LPDWORD)&StrXactID);
        }
    else
        DdePostAdvise(idInst,hszChatTopic,hszConvPartner);

    return;
}


/*---------------------------------------------------------------------------*\
| UNPACK FONT
|   This routine unpacks the font stored in the packed transaction.
|
| created: 04-Feb-93
| history: 04-Feb-93 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
VOID UnpackFont(LPLOGFONT lf, LPXPACKFONT lfPacked)
{
    lf->lfHeight         = (LONG)(short)lfPacked->lfHeight;
    lf->lfWidth          = (LONG)(short)lfPacked->lfWidth;
    lf->lfEscapement     = (LONG)(short)lfPacked->lfEscapement;
    lf->lfOrientation    = (LONG)(short)lfPacked->lfOrientation;
    lf->lfWeight         = (LONG)(short)lfPacked->lfWeight;
    lf->lfItalic         = (BYTE)lfPacked->lfItalic;
    lf->lfUnderline      = (BYTE)lfPacked->lfUnderline;
    lf->lfStrikeOut      = (BYTE)lfPacked->lfStrikeOut;
    lf->lfCharSet        = (BYTE)lfPacked->lfCharSet;
    lf->lfOutPrecision   = (BYTE)lfPacked->lfOutPrecision;
    lf->lfClipPrecision  = (BYTE)lfPacked->lfClipPrecision;
    lf->lfQuality        = (BYTE)lfPacked->lfQuality;
    lf->lfPitchAndFamily = (BYTE)lfPacked->lfPitchAndFamily;

    lstrcpy(lf->lfFaceName,lfPacked->lfFaceName);

    return;
}


/*---------------------------------------------------------------------------*\
| PACK FONT
|   This routine packs the font for transaction.
|
| created: 04-Feb-93
| history: 04-Feb-93 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
VOID PackFont(LPXPACKFONT lfPacked, LPLOGFONT lf)
{
    BOOL fDefCharUsed;

    lfPacked->lfHeight          = (WORD)lf->lfHeight;
    lfPacked->lfWidth           = (WORD)lf->lfWidth;
    lfPacked->lfEscapement      = (WORD)lf->lfEscapement;
    lfPacked->lfOrientation     = (WORD)lf->lfOrientation;
    lfPacked->lfWeight          = (WORD)lf->lfWeight;
    lfPacked->lfItalic          = (BYTE)lf->lfItalic;
    lfPacked->lfUnderline       = (BYTE)lf->lfUnderline;
    lfPacked->lfStrikeOut       = (BYTE)lf->lfStrikeOut;
    lfPacked->lfCharSet         = (BYTE)lf->lfCharSet;
    lfPacked->lfOutPrecision    = (BYTE)lf->lfOutPrecision;
    lfPacked->lfClipPrecision   = (BYTE)lf->lfClipPrecision;
    lfPacked->lfQuality         = (BYTE)lf->lfQuality;
    lfPacked->lfPitchAndFamily  = (BYTE)lf->lfPitchAndFamily;

    if(ChatData.type == CHT_FONTA)
        WideCharToMultiByte(CP_OEMCP,0,lf->lfFaceName,LF_XPACKFACESIZE,(LPSTR)(lfPacked->lfFaceName),LF_XPACKFACESIZE,NULL,&fDefCharUsed);
    else
        lstrcpy(lfPacked->lfFaceName,lf->lfFaceName);

    return;
}
