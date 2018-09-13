/* Copyright (c) 1991, Microsoft Corporation, all rights reserved

 ipaddr.c - TCP/IP Address custom control

 November 9, 1992    Greg Strange
 */

#include "ctlspriv.h"


// The character that is displayed between address fields.
#define FILLER          TEXT('.')
#define SZFILLER        TEXT(".")
#define SPACE           TEXT(' ')
#define BACK_SPACE      8

/* Min, max values */
#define NUM_FIELDS      4
#define CHARS_PER_FIELD 3
#define HEAD_ROOM       1       // space at top of control
#define LEAD_ROOM       3       // space at front of control
#define MIN_FIELD_VALUE 0       // default minimum allowable field value
#define MAX_FIELD_VALUE 255     // default maximum allowable field value


// All the information unique to one control is stuffed in one of these
// structures in global memory and the handle to the memory is stored in the
// Windows extra space.

typedef struct tagFIELD {
    HANDLE      hWnd;
    WNDPROC     lpfnWndProc;
    BYTE        byLow;  // lowest allowed value for this field.
    BYTE        byHigh; // Highest allowed value for this field.
} FIELD;

typedef struct tagIPADDR {
    HWND        hwndParent;
    HWND        hwnd;
    UINT        uiFieldWidth;
    UINT        uiFillerWidth;
    BOOL        fEnabled : 1;
    BOOL        fPainted : 1;
    BOOL        bControlInFocus : 1;        // TRUE if the control is already in focus, dont't send another focus command
    BOOL        bCancelParentNotify : 1;    // Don't allow the edit controls to notify parent if TRUE
    BOOL        fInMessageBox : 1;  // Set when a message box is displayed so that
    BOOL        fFontCreated :1;
    HFONT       hfont;
    // we don't send a EN_KILLFOCUS message when
    // we receive the EN_KILLFOCUS message for the
    // current field.
    FIELD       Children[NUM_FIELDS];
} IPADDR;


// The following macros extract and store the CONTROL structure for a control.
#define    IPADDRESS_EXTRA            sizeof(DWORD)

#define GET_IPADDR_HANDLE(hWnd)        ((HGLOBAL)(GetWindowLongPtr((hWnd), GWLP_USERDATA)))
#define SAVE_IPADDR_HANDLE(hWnd,x)     (SetWindowLongPtr((hWnd), GWLP_USERDATA, (LONG_PTR)(x)))


/* internal IPAddress function prototypes */
LRESULT IPAddressWndFn( HWND, UINT, WPARAM, LPARAM );
LRESULT IPAddressFieldProc(HWND, UINT, WPARAM, LPARAM);
BOOL SwitchFields(IPADDR FAR *, int, int, WORD, WORD);
void EnterField(FIELD FAR *, WORD, WORD);
BOOL ExitField(IPADDR FAR *, int iField);
int GetFieldValue(FIELD FAR *);
void SetFieldValue(IPADDR *pipa, int iField, int iValue);
BOOL IsDBCS();





/*
 IPAddrInit() - IPAddress custom control initialization
 call
 hInstance = library or application instance
 return
 TRUE on success, FALSE on failure.

 This function does all the one time initialization of IPAddress custom
 controls.  Specifically it creates the IPAddress window class.
 */
int InitIPAddr(HANDLE hInstance)
{
    WNDCLASS        wc;

    if (!GetClassInfo(hInstance, WC_IPADDRESS, &wc)) {
        /* define class attributes */
        wc.lpszClassName = WC_IPADDRESS;
        wc.hCursor =       LoadCursor(NULL,IDC_IBEAM);
        wc.hIcon           = NULL;
        wc.lpszMenuName =  (LPCTSTR)NULL;
        wc.style =         CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS|CS_GLOBALCLASS;
        wc.lpfnWndProc =   IPAddressWndFn;
        wc.hInstance =     hInstance;
        wc.hIcon =         NULL;
        wc.cbWndExtra =    IPADDRESS_EXTRA;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );
        wc.cbClsExtra      = 0;

        /* register IPAddress window class */
        return RegisterClass(&wc);
    }
    return TRUE;
}


/*
 IPAddressWndFn() - Main window function for an IPAddress control.

 call
 hWnd    handle to IPAddress window
 wMsg    message number
 wParam  word parameter
 lParam  long parameter
 */

void FormatIPAddress(LPTSTR pszString, DWORD* dwValue)
{
    int nField, nPos;
    BOOL fFinish = FALSE;

    dwValue[0] = 0; dwValue[1] = 0; dwValue[2] = 0; dwValue[3] = 0;

    if (pszString[0] == 0)
        return;

    for( nField = 0, nPos = 0; !fFinish; nPos++)
    {
        if (( pszString[nPos]<TEXT('0')) || (pszString[nPos]>TEXT('9')))
        {
            // not a number
            nField++;
            fFinish = (nField == 4);
        }
        else
        {
            dwValue[nField] *= 10;
            dwValue[nField] += (pszString[nPos]-TEXT('0'));
        }
    }
}

void IP_OnSetFont(IPADDR* pipa, HFONT hfont, BOOL fRedraw)
{
    int i;
    RECT rect;
    HFONT OldFont;
    BOOL fNewFont = FALSE;
    UINT uiFieldStart;
    HDC hdc;
    
    if (hfont) {
        fNewFont = TRUE;
    } else {
        hfont = (HFONT)SendMessage(pipa->hwnd, WM_GETFONT, 0, 0);
    }
    
    hdc = GetDC(pipa->hwnd);
    OldFont = SelectObject(hdc, hfont);
    GetCharWidth(hdc, FILLER, FILLER,
                 (int *)(&pipa->uiFillerWidth));
    SelectObject(hdc, OldFont);
    ReleaseDC(pipa->hwnd, hdc);
    
    GetClientRect(pipa->hwnd, &rect);
    pipa->hfont = hfont;
    pipa->uiFieldWidth = (RECTWIDTH(rect)
                          - LEAD_ROOM
                          - pipa->uiFillerWidth
                          *(NUM_FIELDS-1))
        / NUM_FIELDS;


    uiFieldStart = LEAD_ROOM;

    for (i = 0; i < NUM_FIELDS; i++) {

        HWND hwnd = pipa->Children[i].hWnd;
        
        if (fNewFont)
            SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)fRedraw);
        
        SetWindowPos(hwnd, NULL,
                     uiFieldStart,
                     HEAD_ROOM,
                     pipa->uiFieldWidth,
                     (rect.bottom-rect.top),
                     SWP_NOACTIVATE);

        uiFieldStart += pipa->uiFieldWidth
            + pipa->uiFillerWidth;

    }
    
}

LRESULT IPAddressWndFn( hWnd, wMsg, wParam, lParam )
    HWND            hWnd;
    UINT            wMsg;
    WPARAM            wParam;
    LPARAM            lParam;
{
    LRESULT lResult;
    IPADDR *pipa;
    int i;

    pipa = (IPADDR *)GET_IPADDR_HANDLE(hWnd);
    lResult = TRUE;

    switch( wMsg )
    {

        // use empty string (not NULL) to set to blank
        case WM_SETTEXT:
        {
            TCHAR szBuf[CHARS_PER_FIELD+1];
            DWORD dwValue[4];
#ifdef UNICODE_WIN9x
            WCHAR szTemp[80];
            LPTSTR pszString = szTemp;
            ConvertAToWN(CP_ACP, szTemp, ARRAYSIZE(szTemp), (LPSTR)lParam, -1);
#else
            LPTSTR pszString = (LPTSTR)lParam;
#endif

            FormatIPAddress(pszString, &dwValue[0]);
            pipa->bCancelParentNotify = TRUE;

            for (i = 0; i < NUM_FIELDS; ++i)
            {
                if (pszString[0] == 0)
                {
                    szBuf[0] = 0;
                }
                else
                {
                    wsprintf(szBuf, TEXT("%d"), dwValue[i]);
                }
                SendMessage(pipa->Children[i].hWnd, WM_SETTEXT,
                            0, (LPARAM) (LPSTR) szBuf);
            }

            pipa->bCancelParentNotify = FALSE;

            SendMessage(pipa->hwndParent, WM_COMMAND,
                        MAKEWPARAM(GetDlgCtrlID(hWnd), EN_CHANGE), (LPARAM)hWnd);
        }
        break;

    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
    {
        int iFieldValue;
        DWORD dwValue[4];
#ifdef UNICODE_WIN9x
        char pszResult[30];
        char *pszDest = (char *)lParam;
#else
        TCHAR pszResult[30];
        TCHAR *pszDest = (TCHAR *)lParam;
#endif

        lResult = 0;
        dwValue[0] = 0;
        dwValue[1] = 0;
        dwValue[2] = 0;
        dwValue[3] = 0;
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            iFieldValue = GetFieldValue(&(pipa->Children[i]));
            if (iFieldValue == -1)
                iFieldValue = 0;
            else
                ++lResult;
            dwValue[i] = iFieldValue;
        }
#ifdef UNICODE_WIN9x
        wsprintfA( pszResult, "%d.%d.%d.%d", dwValue[0], dwValue[1], dwValue[2], dwValue[3] );
#else
        wsprintf( pszResult, TEXT("%d.%d.%d.%d"), dwValue[0], dwValue[1], dwValue[2], dwValue[3] );
#endif
        if (wMsg == WM_GETTEXT) {
#ifdef UNICODE_WIN9x
            lstrcpynA(pszDest, pszResult, (int) wParam);
            lResult = lstrlenA( pszDest );
#else
            StrCpyN(pszDest, pszResult, (int) wParam);
            lResult = lstrlen( pszDest );
#endif

        } else {
#ifdef UNICODE_WIN9x
            lResult = lstrlenA( pszResult );
#else
            lResult = lstrlen( pszResult );
#endif
        }
    }
        break;

    case WM_GETDLGCODE :
        lResult = DLGC_WANTCHARS;
        break;

    case WM_NCCREATE:
        SetWindowBits(hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        lResult = TRUE;
        break;

    case WM_CREATE : /* create pallette window */
    {
        LONG id;

        pipa = (IPADDR*)LocalAlloc(LPTR, sizeof(IPADDR));

        if (pipa)
        {

#define LPCS    ((CREATESTRUCT *)lParam)

            pipa->fEnabled = TRUE;
            pipa->hwndParent = LPCS->hwndParent;
            pipa->hwnd = hWnd;

            id = GetDlgCtrlID(hWnd);
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                pipa->Children[i].byLow = MIN_FIELD_VALUE;
                pipa->Children[i].byHigh = MAX_FIELD_VALUE;

                pipa->Children[i].hWnd = CreateWindowEx(0,
                                                        TEXT("Edit"),
                                                        NULL,
                                                        WS_CHILD |
                                                        ES_CENTER, 
                                                        0, 10, 100, 100,
                                                        hWnd,
                                                        (HMENU)(LONG_PTR)id,
                                                        LPCS->hInstance,
                                                        (LPVOID)NULL);

                SAVE_IPADDR_HANDLE(pipa->Children[i].hWnd, i);
                SendMessage(pipa->Children[i].hWnd, EM_LIMITTEXT,
                            CHARS_PER_FIELD, 0L);

                pipa->Children[i].lpfnWndProc =
                    (WNDPROC) GetWindowLongPtr(pipa->Children[i].hWnd,
                                               GWLP_WNDPROC);

                SetWindowLongPtr(pipa->Children[i].hWnd,
                                 GWLP_WNDPROC, (LONG_PTR)IPAddressFieldProc);

            }

            SAVE_IPADDR_HANDLE(hWnd, pipa);
            
            IP_OnSetFont(pipa, NULL, FALSE);
            for (i = 0; i < NUM_FIELDS; ++i)
                ShowWindow(pipa->Children[i].hWnd, SW_SHOW);


#undef LPCS
        }
        else
            DestroyWindow(hWnd);
    }
        lResult = 0;
        break;

    case WM_PAINT: /* paint IPADDR window */
    {
        PAINTSTRUCT Ps;
        RECT rect;
        COLORREF TextColor;
        COLORREF cRef;
        HFONT OldFont;

        BeginPaint(hWnd, (LPPAINTSTRUCT)&Ps);
        OldFont = SelectObject( Ps.hdc, pipa->hfont);
        GetClientRect(hWnd, &rect);
        if (pipa->fEnabled)
        {
            TextColor = GetSysColor(COLOR_WINDOWTEXT);
            cRef = GetSysColor(COLOR_WINDOW);
        }
        else
        {
            TextColor = GetSysColor(COLOR_GRAYTEXT);
            cRef = GetSysColor(COLOR_3DFACE);
        }

        FillRectClr(Ps.hdc, &rect, cRef);
        SetRect(&rect, 0, HEAD_ROOM, pipa->uiFillerWidth, (rect.bottom-rect.top));


        SetBkColor(Ps.hdc, cRef);
        SetTextColor(Ps.hdc, TextColor);

        for (i = 0; i < NUM_FIELDS-1; ++i)
        {
            rect.left += pipa->uiFieldWidth + pipa->uiFillerWidth;
            rect.right += rect.left + pipa->uiFillerWidth;
            ExtTextOut(Ps.hdc, rect.left, HEAD_ROOM, ETO_OPAQUE, &rect, SZFILLER, 1, NULL);
        }

        pipa->fPainted = TRUE;

        SelectObject(Ps.hdc, OldFont);
        EndPaint(hWnd, &Ps);
    }
        break;

    case WM_SETFOCUS : /* get focus - display caret */
        EnterField(&(pipa->Children[0]), 0, CHARS_PER_FIELD);
        break;
        
        HANDLE_MSG(pipa, WM_SETFONT, IP_OnSetFont);

    case WM_LBUTTONDOWN : /* left button depressed - fall through */
        SetFocus(hWnd);
        break;

    case WM_ENABLE:
    {
        pipa->fEnabled = (BOOL)wParam;
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            EnableWindow(pipa->Children[i].hWnd, (BOOL)wParam);
        }
        if (pipa->fPainted)    
            InvalidateRect(hWnd, NULL, FALSE);
    }
        break;

    case WM_DESTROY :
        // Restore all the child window procedures before we delete our memory block.
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            SendMessage(pipa->Children[i].hWnd, WM_DESTROY, 0, 0);
            SetWindowLongPtr(pipa->Children[i].hWnd, GWLP_WNDPROC,
                             (LONG_PTR)pipa->Children[i].lpfnWndProc);
        }

        LocalFree(pipa);
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
            // One of the fields lost the focus, see if it lost the focus to another field
            // of if we've lost the focus altogether.  If its lost altogether, we must send
            // an EN_KILLFOCUS notification on up the ladder.
            case EN_KILLFOCUS:
            {
                HWND hFocus;

                if (!pipa->fInMessageBox)
                {
                    hFocus = GetFocus();
                    for (i = 0; i < NUM_FIELDS; ++i)
                        if (pipa->Children[i].hWnd == hFocus)
                            break;

                    if (i >= NUM_FIELDS)
                    {
                        SendMessage(pipa->hwndParent, WM_COMMAND,
                                    MAKEWPARAM(GetDlgCtrlID(hWnd),
                                               EN_KILLFOCUS), (LPARAM)hWnd);
                        pipa->bControlInFocus = FALSE;
                    }
                }
            }
            break;

        case EN_SETFOCUS:
        {
            HWND hFocus;

            if (!pipa->fInMessageBox)
            {
                hFocus = (HWND)lParam;

                for (i = 0; i < NUM_FIELDS; ++i)
                    if (pipa->Children[i].hWnd == hFocus)
                        break;

                // send a focus message when the
                if (i < NUM_FIELDS && pipa->bControlInFocus == FALSE)
                {
                    SendMessage(pipa->hwndParent, WM_COMMAND,
                                MAKEWPARAM(GetDlgCtrlID(hWnd),
                                           EN_SETFOCUS), (LPARAM)hWnd);

                    pipa->bControlInFocus = TRUE; // only set the focus once
                }
            }
        }
            break;

        case EN_CHANGE:
            if (pipa->bCancelParentNotify == FALSE)
            {
                SendMessage(pipa->hwndParent, WM_COMMAND,
                            MAKEWPARAM(GetDlgCtrlID(hWnd), EN_CHANGE), (LPARAM)hWnd);

            }
            break;
        }
        break;

        // Get the value of the IP Address.  The address is placed in the DWORD pointed
        // to by lParam and the number of non-blank fields is returned.
        case IPM_GETADDRESS:
        {
            int iFieldValue;
            DWORD dwValue;

            lResult = 0;
            dwValue = 0;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                iFieldValue = GetFieldValue(&(pipa->Children[i]));
                if (iFieldValue == -1)
                    iFieldValue = 0;
                else
                    ++lResult;
                dwValue = (dwValue << 8) + iFieldValue;
            }
            *((DWORD *)lParam) = dwValue;
        }
        break;

        // Clear all fields to blanks.
        case IPM_CLEARADDRESS:
        {
            pipa->bCancelParentNotify = TRUE;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                SendMessage(pipa->Children[i].hWnd, WM_SETTEXT,
                            0, (LPARAM) (LPSTR) TEXT(""));
            }
            pipa->bCancelParentNotify = FALSE;
            SendMessage(pipa->hwndParent, WM_COMMAND,
                        MAKEWPARAM(GetDlgCtrlID(hWnd), EN_CHANGE), (LPARAM)hWnd);
        }
        break;

        // Set the value of the IP Address.  The address is in the lParam with the
        // first address byte being the high byte, the second being the second byte,
        // and so on.  A lParam value of -1 removes the address.
        case IPM_SETADDRESS:
        {
            pipa->bCancelParentNotify = TRUE;

            for (i = 0; i < NUM_FIELDS; ++i)
            {
                BYTE bVal = HIBYTE(HIWORD(lParam));
                if (pipa->Children[i].byLow <= bVal &&
                    bVal <= pipa->Children[i].byHigh) {
                    SetFieldValue(pipa, i, bVal);

                } else {
                    lResult = FALSE;
                }

                lParam <<= 8;
            }

            pipa->bCancelParentNotify = FALSE;

            SendMessage(pipa->hwndParent, WM_COMMAND,
                        MAKEWPARAM(GetDlgCtrlID(hWnd), EN_CHANGE), (LPARAM)hWnd);
        }
        break;

    case IPM_SETRANGE:
        if (wParam < NUM_FIELDS && LOBYTE(LOWORD(lParam)) <= HIBYTE(LOWORD(lParam)))
        {
            lResult = MAKEIPRANGE(pipa->Children[wParam].byLow, pipa->Children[wParam].byHigh);
            pipa->Children[wParam].byLow = LOBYTE(LOWORD(lParam));
            pipa->Children[wParam].byHigh = HIBYTE(LOWORD(lParam));
            break;
        }
        lResult = 0;
        break;

        // Set the focus to this IPADDR.
        // wParam = the field number to set focus to, or -1 to set the focus to the
        // first non-blank field.
    case IPM_SETFOCUS:

        if (wParam >= NUM_FIELDS)
        {
            for (wParam = 0; wParam < NUM_FIELDS; ++wParam)
                if (GetFieldValue(&(pipa->Children[wParam])) == -1)   break;
            if (wParam >= NUM_FIELDS)    wParam = 0;
        }
        EnterField(&(pipa->Children[wParam]), 0, CHARS_PER_FIELD);
        break;

        // Determine whether all four subfields are blank
    case IPM_ISBLANK:

        lResult = TRUE;
        for (i = 0; i < NUM_FIELDS; ++i)
        {
            if (GetFieldValue(&(pipa->Children[i])) != -1)
            {
                lResult = FALSE;
                break;
            }
        }
        break;

    default:
        lResult = DefWindowProc( hWnd, wMsg, wParam, lParam );
        break;
    }
    return( lResult );
}




/*
 IPAddressFieldProc() - Edit field window procedure

 This function sub-classes each edit field.
 */
LRESULT IPAddressFieldProc(HWND hWnd,
                                   UINT wMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    IPADDR *pipa;
    FIELD *pField;
    HWND hIPADDRWindow;
    WORD wChildID;
    LRESULT lresult;

    if (!(hIPADDRWindow = GetParent(hWnd)))
        return 0;

    pipa = (IPADDR *)GET_IPADDR_HANDLE(hIPADDRWindow);
    if (!pipa)
        return 0;
    
    wChildID = (WORD)GET_IPADDR_HANDLE(hWnd);
    pField = &(pipa->Children[wChildID]);

    if (pField->hWnd != hWnd)    
        return 0;

    switch (wMsg)
    {
    case WM_DESTROY:
        DeleteObject((HGDIOBJ)SendMessage(hWnd, WM_GETFONT, 0, 0));
        return 0;

    case WM_CHAR:

        // Typing in the last digit in a field, skips to the next field.
        if (wParam >= TEXT('0') && wParam <= TEXT('9'))
        {
            LRESULT lResult;

            lResult = CallWindowProc(pipa->Children[wChildID].lpfnWndProc,
                                      hWnd, wMsg, wParam, lParam);
            lResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);

            if (lResult == MAKELPARAM(CHARS_PER_FIELD, CHARS_PER_FIELD)
                && ExitField(pipa, wChildID)
                && wChildID < NUM_FIELDS-1)
            {
                EnterField(&(pipa->Children[wChildID+1]),
                           0, CHARS_PER_FIELD);
            }
            return lResult;
        }

        // spaces and periods fills out the current field and then if possible,
        // goes to the next field.
        else if (wParam == FILLER || wParam == SPACE )
        {
            LRESULT lResult;
            lResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
            if (lResult != 0L && HIWORD(lResult) == LOWORD(lResult)
                && ExitField(pipa, wChildID))
            {
                if (wChildID >= NUM_FIELDS-1)
                    MessageBeep((UINT)-1);
                else
                {
                    EnterField(&(pipa->Children[wChildID+1]),
                               0, CHARS_PER_FIELD);
                }
            }
            return 0;
        }

        // Backspaces go to the previous field if at the beginning of the current field.
        // Also, if the focus shifts to the previous field, the backspace must be
        // processed by that field.
        else if (wParam == BACK_SPACE)
        {
            if (wChildID > 0 && SendMessage(hWnd, EM_GETSEL, 0, 0L) == 0L)
            {
                if (SwitchFields(pipa, wChildID, wChildID-1,
                                 CHARS_PER_FIELD, CHARS_PER_FIELD)
                    && SendMessage(pipa->Children[wChildID-1].hWnd,
                                   EM_LINELENGTH, 0, 0L) != 0L)
                {
                    SendMessage(pipa->Children[wChildID-1].hWnd,
                                wMsg, wParam, lParam);
                }
                return 0;
            }
        }

        // Any other printable characters are not allowed.
        else if (wParam > SPACE)
        {
            MessageBeep((UINT)-1);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {

            // Arrow keys move between fields when the end of a field is reached.
            case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (GetKeyState(VK_CONTROL) < 0)
            {
                if ((wParam == VK_LEFT || wParam == VK_UP) && wChildID > 0)
                {
                    SwitchFields(pipa, wChildID, wChildID-1,
                                 0, CHARS_PER_FIELD);
                    return 0;
                }
                else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                         && wChildID < NUM_FIELDS-1)
                {
                    SwitchFields(pipa, wChildID, wChildID+1,
                                 0, CHARS_PER_FIELD);
                    return 0;
                }
            }
            else
            {
                DWORD dwResult;
                WORD wStart, wEnd;

                dwResult = (DWORD)SendMessage(hWnd, EM_GETSEL, 0, 0L);
                wStart = LOWORD(dwResult);
                wEnd = HIWORD(dwResult);
                if (wStart == wEnd)
                {
                    if ((wParam == VK_LEFT || wParam == VK_UP)
                        && wStart == 0
                        && wChildID > 0)
                    {
                        SwitchFields(pipa, wChildID, wChildID-1,
                                     CHARS_PER_FIELD, CHARS_PER_FIELD);
                        return 0;
                    }
                    else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                             && wChildID < NUM_FIELDS-1)
                    {
                        dwResult = (DWORD)SendMessage(hWnd, EM_LINELENGTH, 0, 0L);
                        if (wStart >= dwResult)
                        {
                            SwitchFields(pipa, wChildID, wChildID+1, 0, 0);
                            return 0;
                        }
                    }
                }
            }
            break;

            // Home jumps back to the beginning of the first field.
            case VK_HOME:
                if (wChildID > 0)
                {
                    SwitchFields(pipa, wChildID, 0, 0, 0);
                    return 0;
                }
            break;

            // End scoots to the end of the last field.
            case VK_END:
                if (wChildID < NUM_FIELDS-1)
                {
                    SwitchFields(pipa, wChildID, NUM_FIELDS-1,
                                 CHARS_PER_FIELD, CHARS_PER_FIELD);
                    return 0;
                }
            break;


        } // switch (wParam)

        break;

    case WM_KILLFOCUS:
        if ( !ExitField( pipa, wChildID ))
        {
            return 0;
        }

    } // switch (wMsg)

    lresult = CallWindowProc( pipa->Children[wChildID].lpfnWndProc,
                             hWnd, wMsg, wParam, lParam);
    return lresult;
}




/*
 Switch the focus from one field to another.
 call
 pipa = Pointer to the IPADDR structure.
 iOld = Field we're leaving.
 iNew = Field we're entering.
 hNew = Window of field to goto
 wStart = First character selected
 wEnd = Last character selected + 1
 returns
 TRUE on success, FALSE on failure.

 Only switches fields if the current field can be validated.
 */
BOOL SwitchFields(IPADDR *pipa, int iOld, int iNew, WORD wStart, WORD wEnd)
{
    if (!ExitField(pipa, iOld))    return FALSE;
    EnterField(&(pipa->Children[iNew]), wStart, wEnd);
    return TRUE;
}



/*
 Set the focus to a specific field's window.
 call
 pField = pointer to field structure for the field.
 wStart = First character selected
 wEnd = Last character selected + 1
 */
void EnterField(FIELD *pField, WORD wStart, WORD wEnd)
{
    SetFocus(pField->hWnd);
    SendMessage(pField->hWnd, EM_SETSEL, wStart, wEnd);
}

void SetFieldValue(IPADDR *pipa, int iField, int iValue)
{
    TCHAR szBuf[CHARS_PER_FIELD+1];
    FIELD* pField = &(pipa->Children[iField]);

    wsprintf(szBuf, TEXT("%d"), iValue);
    SendMessage(pField->hWnd, WM_SETTEXT, 0, (LPARAM) (LPSTR) szBuf);
}

/*
 Exit a field.
 call
 pipa = pointer to IPADDR structure.
 iField = field number being exited.
 returns
 TRUE if the user may exit the field.
 FALSE if he may not.
 */
BOOL ExitField(IPADDR  *pipa, int iField)
{
    FIELD *pField;
    int i;
    NMIPADDRESS nm;
    int iOldValue;

    pField = &(pipa->Children[iField]);
    i = GetFieldValue(pField);
    iOldValue = i;
    
    nm.iField = iField;
    nm.iValue = i;
    
    SendNotifyEx(pipa->hwndParent, pipa->hwnd, IPN_FIELDCHANGED, &nm.hdr, FALSE);
    i = nm.iValue;
    
    if (i != -1) {

        if (i < (int)(UINT)pField->byLow || i > (int)(UINT)pField->byHigh)
        {
            
            if ( i < (int)(UINT) pField->byLow )
            {
                /* too small */
                i = (int)(UINT)pField->byLow;
            }
            else
            {
                /* must be bigger */
                i = (int)(UINT)pField->byHigh;
            }
            SetFieldValue(pipa, iField, i);
            // CHEEBUGBUG: send notify up
            return FALSE;
        }
    } 

    if (iOldValue != i) {
        SetFieldValue(pipa, iField, i);
    }
    return TRUE;
}


/*
 Get the value stored in a field.
 call
 pField = pointer to the FIELD structure for the field.
 returns
 The value (0..255) or -1 if the field has not value.
 */
int GetFieldValue(FIELD *pField)
{
    WORD wLength;
    TCHAR szBuf[CHARS_PER_FIELD+1];
    INT i;

    *(WORD *)szBuf = (sizeof(szBuf)/sizeof(TCHAR)) - 1;
    wLength = (WORD)SendMessage(pField->hWnd,EM_GETLINE,0,(LPARAM)(LPSTR)szBuf);
    if (wLength != 0)
    {
        szBuf[wLength] = TEXT('\0');
        i = StrToInt(szBuf);
        return i;
    }
    else
        return -1;
}



BOOL IsDBCS()
{
    LANGID langid;
    langid = PRIMARYLANGID(GetThreadLocale());
    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
    {
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
