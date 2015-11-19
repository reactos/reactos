/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/messagebox.c
 * PURPOSE:         Input
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      2003/07/28  Added some NT features
 *      2003/07/27  Code ported from wine
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* DEFINES *******************************************************************/

#define MSGBOX_IDICON   (1088)
#define MSGBOX_IDTEXT   (100)

#define IDI_HANDW          MAKEINTRESOURCEW(32513)
#define IDI_QUESTIONW      MAKEINTRESOURCEW(32514)
#define IDI_EXCLAMATIONW   MAKEINTRESOURCEW(32515)
#define IDI_ASTERISKW      MAKEINTRESOURCEW(32516)
#define IDI_WINLOGOW       MAKEINTRESOURCEW(32517)


/* MessageBox metrics */

#define BTN_CX (75)
#define BTN_CY (23)

#define MSGBOXEX_SPACING        (16)
#define MSGBOXEX_BUTTONSPACING  (6)
#define MSGBOXEX_MARGIN         (12)
#define MSGBOXEX_MAXBTNSTR      (32)
#define MSGBOXEX_MAXBTNS        (4)

/* Rescale logical coordinates */
#define RESCALE_X(_x, _unit)    (((_x) * 4 + LOWORD(_unit) - 1) / LOWORD(_unit))
#define RESCALE_Y(_y, _unit)    (((_y) * 8 + HIWORD(_unit) - 1) / HIWORD(_unit))


/* MessageBox button helpers */

#define DECLARE_MB_1(_btn0) \
    { 1, { ID##_btn0, 0, 0 }, { IDS_##_btn0, 0, 0 } }

#define DECLARE_MB_2(_btn0, _btn1) \
    { 2, { ID##_btn0, ID##_btn1, 0 }, { IDS_##_btn0, IDS_##_btn1, 0 } }

#define DECLARE_MB_3(_btn0, _btn1, _btn2) \
    { 3, { ID##_btn0, ID##_btn1, ID##_btn2 }, { IDS_##_btn0, IDS_##_btn1, IDS_##_btn2 } }

typedef struct _MSGBTNINFO
{
    LONG btnCnt;
    LONG btnIdx[MSGBOXEX_MAXBTNS];
    UINT btnIds[MSGBOXEX_MAXBTNS];
} MSGBTNINFO, *PMSGBTNINFO;

/* Default MessageBox buttons */
static const MSGBTNINFO MsgBtnInfo[] =
{
    /* MB_OK (0) */
    DECLARE_MB_1(OK),
    /* MB_OKCANCEL (1) */
    DECLARE_MB_2(OK, CANCEL),
    /* MB_ABORTRETRYIGNORE (2) */
    DECLARE_MB_3(ABORT, RETRY, IGNORE),
    /* MB_YESNOCANCEL (3) */
    DECLARE_MB_3(YES, NO, CANCEL),
    /* MB_YESNO (4) */
    DECLARE_MB_2(YES, NO),
    /* MB_RETRYCANCEL (5) */
    DECLARE_MB_2(RETRY, CANCEL),
    /* MB_CANCELTRYCONTINUE (6) */
    DECLARE_MB_3(CANCEL, TRYAGAIN, CONTINUE)
};


typedef struct _MSGBOXINFO
{
  MSGBOXPARAMSW; // Wine passes this too.
  // ReactOS
  HICON Icon;
  HFONT Font;
  int DefBtn;
  int nButtons;
  LONG *Btns;
  UINT Timeout;
} MSGBOXINFO, *PMSGBOXINFO;

/* INTERNAL FUNCTIONS ********************************************************/

static VOID MessageBoxTextToClipboard(HWND DialogWindow)
{
    HWND hwndText;
    PMSGBOXINFO mbi;
    int cchTotal, cchTitle, cchText, cchButton, i, n, cchBuffer;
    LPWSTR pszBuffer, pszBufferPos, pMessageBoxText, pszTitle, pszText, pszButton;
    WCHAR szButton[MSGBOXEX_MAXBTNSTR];
    HGLOBAL hGlobal;

    static const WCHAR szLine[] = L"---------------------------\r\n";

    mbi = (PMSGBOXINFO)GetPropW(DialogWindow, L"ROS_MSGBOX");
    hwndText = GetDlgItem(DialogWindow, MSGBOX_IDTEXT);
    cchTitle = GetWindowTextLengthW(DialogWindow) + 1;
    cchText = GetWindowTextLengthW(hwndText) + 1;

    if (!mbi)
        return;

    pMessageBoxText = (LPWSTR)RtlAllocateHeap(GetProcessHeap(), 0, (cchTitle + cchText) * sizeof(WCHAR));

    if (pMessageBoxText == NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        return;
    }

    pszTitle = pMessageBoxText;
    pszText = pMessageBoxText + cchTitle;

    if (GetWindowTextW(DialogWindow, pszTitle, cchTitle) == 0 ||
        GetWindowTextW(hwndText, pszText, cchText) == 0)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        return;
    }

    /*
     * Calculate the total buffer size.
     */
    cchTotal = 6 + cchTitle + cchText + (lstrlenW(szLine) * 4) + (mbi->nButtons * MSGBOXEX_MAXBTNSTR + 3);

    hGlobal = GlobalAlloc(GHND, cchTotal * sizeof(WCHAR));

    pszBuffer = (LPWSTR)GlobalLock(hGlobal);

    if (pszBuffer == NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        GlobalFree(hGlobal);
        return;
    }

    /*
     * First format title and text.
     * ------------------
     * Title
     * ------------------
     * Text
     * ------------------
     */
    cchBuffer = wsprintfW(pszBuffer, L"%s%s\r\n%s%s\r\n%s", szLine, pszTitle, szLine, pszText, szLine);
    pszBufferPos = pszBuffer + cchBuffer;

    for (i = 0; i < mbi->nButtons; i++)
    {
        GetDlgItemTextW(DialogWindow, mbi->Btns[i], szButton, MSGBOXEX_MAXBTNSTR);

        cchButton = strlenW(szButton);
        pszButton = szButton;

        /* Skip '&' character. */
        if (szButton[0] == '&')
        {
            pszButton = pszButton + 1;
            cchButton = cchButton - 1;
        }

        for (n = 0; n < cchButton; n++)
            *(pszBufferPos++) = pszButton[n];

        /* Add spaces. */
        *(pszBufferPos++) = L' ';
        *(pszBufferPos++) = L' ';
        *(pszBufferPos++) = L' ';
    }

    wsprintfW(pszBufferPos, L"\r\n%s", szLine);

    GlobalUnlock(hGlobal);

    if (OpenClipboard(DialogWindow))
    {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hGlobal);
        CloseClipboard();
    }
    else
    {
        GlobalFree(hGlobal);
    }
    RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
}

static INT_PTR CALLBACK MessageBoxProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  int i, Alert;
  PMSGBOXINFO mbi;
  HELPINFO hi;
  HWND owner;

  switch(message) {
    case WM_INITDIALOG:
      mbi = (PMSGBOXINFO)lParam;

      SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)mbi);
      NtUserxSetMessageBox(hwnd);

      if(!GetPropW(hwnd, L"ROS_MSGBOX"))
      {
        SetPropW(hwnd, L"ROS_MSGBOX", (HANDLE)lParam);

        if (mbi->dwContextHelpId)
          SetWindowContextHelpId(hwnd, mbi->dwContextHelpId);

        if (mbi->Icon)
        {
          SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM)mbi->Icon, 0);
          Alert = ALERT_SYSTEM_WARNING;
        }
        else // Setup the rest of the alerts.
        {
          switch(mbi->dwStyle & MB_ICONMASK)
          {
             case MB_ICONWARNING:
                Alert = ALERT_SYSTEM_WARNING;
             break;
             case MB_ICONERROR:
                Alert = ALERT_SYSTEM_ERROR;
             break;
             case MB_ICONQUESTION:
                Alert = ALERT_SYSTEM_QUERY;
             break;
             default:
                Alert = ALERT_SYSTEM_INFORMATIONAL;
             /* fall through */
          }
        }
        /* Send out the alert notifications. */
        NotifyWinEvent(EVENT_SYSTEM_ALERT, hwnd, OBJID_ALERT, Alert);

        /* set control fonts */
        SendDlgItemMessageW(hwnd, MSGBOX_IDTEXT, WM_SETFONT, (WPARAM)mbi->Font, 0);
        for(i = 0; i < mbi->nButtons; i++)
        {
          SendDlgItemMessageW(hwnd, mbi->Btns[i], WM_SETFONT, (WPARAM)mbi->Font, 0);
        }
        switch(mbi->dwStyle & MB_TYPEMASK)
        {
          case MB_ABORTRETRYIGNORE:
          case MB_YESNO:
            RemoveMenu(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND);
            break;
        }
        SetFocus(GetDlgItem(hwnd, mbi->DefBtn));
        if(mbi->Timeout && (mbi->Timeout != (UINT)-1))
          SetTimer(hwnd, 0, mbi->Timeout, NULL);
      }
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
        case IDCANCEL:
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
        case IDYES:
        case IDNO:
        case IDTRYAGAIN:
        case IDCONTINUE:
          EndDialog(hwnd, wParam);
          return 0;
        case IDHELP:
          /* send WM_HELP message to messagebox window */
          hi.cbSize = sizeof(HELPINFO);
          hi.iContextType = HELPINFO_WINDOW;
          hi.iCtrlId = LOWORD(wParam);
          hi.hItemHandle = (HANDLE)lParam;
          hi.dwContextId = 0;
          GetCursorPos(&hi.MousePos);
          SendMessageW(hwnd, WM_HELP, 0, (LPARAM)&hi);
          return 0;
      }
      return 0;

    case WM_COPY:
        MessageBoxTextToClipboard(hwnd);
        return 0;

    case WM_HELP:
      mbi = (PMSGBOXINFO)GetPropW(hwnd, L"ROS_MSGBOX");
      if(!mbi)
        return 0;
      memcpy(&hi, (void *)lParam, sizeof(hi));
      hi.dwContextId = GetWindowContextHelpId(hwnd);

      if (mbi->lpfnMsgBoxCallback)
        mbi->lpfnMsgBoxCallback(&hi);
      else
      {
        owner = GetWindow(hwnd, GW_OWNER);
        if(owner)
          SendMessageW(GetWindow(hwnd, GW_OWNER), WM_HELP, 0, (LPARAM)&hi);
      }
      return 0;

    case WM_CLOSE:
      mbi = (PMSGBOXINFO)GetPropW(hwnd, L"ROS_MSGBOX");
      if(!mbi)
        return 0;
      switch(mbi->dwStyle & MB_TYPEMASK)
      {
        case MB_ABORTRETRYIGNORE:
        case MB_YESNO:
          return 1;
      }
      EndDialog(hwnd, IDCANCEL);
      return 1;

    case WM_TIMER:
      if(wParam == 0)
      {
        EndDialog(hwnd, 32000);
      }
      return 0;
  }
  return 0;
}

static int
MessageBoxTimeoutIndirectW(
  CONST MSGBOXPARAMSW *lpMsgBoxParams, UINT Timeout)
{
    DLGTEMPLATE *tpl;
    DLGITEMTEMPLATE *iico, *itxt;
    NONCLIENTMETRICSW nclm;
    LPVOID buf;
    BYTE *dest;
    LPCWSTR caption, text;
    HFONT hFont;
    HICON Icon;
    HDC hDC;
    int bufsize, ret, caplen, textlen, i, btnleft, btntop, lmargin;
    MSGBTNINFO Buttons;
    LPCWSTR ButtonText[MSGBOXEX_MAXBTNS];
    int ButtonLen[MSGBOXEX_MAXBTNS];
    DLGITEMTEMPLATE *ibtn[MSGBOXEX_MAXBTNS];
    RECT btnrect, txtrect, rc;
    SIZE btnsize;
    MSGBOXINFO mbi;
    BOOL defbtn = FALSE;
    DWORD units = GetDialogBaseUnits();

    if (!lpMsgBoxParams->lpszCaption)
    {
        /* No caption, use the default one */
        caplen = LoadStringW(User32Instance, IDS_ERROR, (LPWSTR)&caption, 0);
    }
    else if (IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
    {
        /* User-defined resource string */
        caplen = LoadStringW(lpMsgBoxParams->hInstance, (UINT)lpMsgBoxParams->lpszCaption, (LPWSTR)&caption, 0);
    }
    else
    {
        /* UNICODE string pointer */
        caption = lpMsgBoxParams->lpszCaption;
        caplen = strlenW(caption);
    }

    if (!lpMsgBoxParams->lpszText)
    {
        /* No text, use blank */
        text = L"";
        textlen = 0;
    }
    else if (IS_INTRESOURCE(lpMsgBoxParams->lpszText))
    {
        /* User-defined resource string */
        textlen = LoadStringW(lpMsgBoxParams->hInstance, (UINT)lpMsgBoxParams->lpszText, (LPWSTR)&text, 0);
    }
    else
    {
        /* UNICODE string pointer */
        text = lpMsgBoxParams->lpszText;
        textlen = strlenW(text);
    }

    /* Create the selected buttons; unknown types will fall back to MB_OK */
    i = (lpMsgBoxParams->dwStyle & MB_TYPEMASK);
    if (i >= ARRAYSIZE(MsgBtnInfo))
        i = MB_OK;

    /* Get buttons IDs */
    Buttons = MsgBtnInfo[i];

    /* Add the Help button */
    if (lpMsgBoxParams->dwStyle & MB_HELP)
    {
        Buttons.btnIdx[Buttons.btnCnt] = IDHELP;
        Buttons.btnIds[Buttons.btnCnt] = IDS_HELP;
        Buttons.btnCnt++;
    }

    switch(lpMsgBoxParams->dwStyle & MB_ICONMASK)
    {
      case MB_ICONEXCLAMATION:
        Icon = LoadIconW(0, IDI_EXCLAMATIONW);
        MessageBeep(MB_ICONEXCLAMATION);
        break;
      case MB_ICONQUESTION:
        Icon = LoadIconW(0, IDI_QUESTIONW);
        MessageBeep(MB_ICONQUESTION);
        break;
      case MB_ICONASTERISK:
        Icon = LoadIconW(0, IDI_ASTERISKW);
        MessageBeep(MB_ICONASTERISK);
        break;
      case MB_ICONHAND:
        Icon = LoadIconW(0, IDI_HANDW);
        MessageBeep(MB_ICONHAND);
        break;
      case MB_USERICON:
        Icon = LoadIconW(lpMsgBoxParams->hInstance, lpMsgBoxParams->lpszIcon);
        MessageBeep(MB_OK);
        break;
      default:
        /* By default, Windows 95/98/NT does not associate an icon to message boxes.
         * So ReactOS should do the same.
         */
        Icon = (HICON)0;
        MessageBeep(MB_OK);
        break;
    }

    /* Basic space */
    bufsize = sizeof(DLGTEMPLATE) +
              2 * sizeof(WORD) +                         /* menu and class */
              (caplen + 1) * sizeof(WCHAR);              /* title */

    /* Space for icon */
    if (NULL != Icon)
    {
      bufsize = (bufsize + 3) & ~3;
      bufsize += sizeof(DLGITEMTEMPLATE) +
                 4 * sizeof(WORD) +
                 sizeof(WCHAR);
    }

    /* Space for text */
    bufsize = (bufsize + 3) & ~3;
    bufsize += sizeof(DLGITEMTEMPLATE) +
               3 * sizeof(WORD) +
               (textlen + 1) * sizeof(WCHAR);

    for (i = 0; i < Buttons.btnCnt; i++)
    {
        /* Get the default text of the buttons */
        if (Buttons.btnIds[i])
        {
            ButtonLen[i] = LoadStringW(User32Instance, Buttons.btnIds[i], (LPWSTR)&ButtonText[i], 0);
        }
        else
        {
            ButtonText[i] = L"";
            ButtonLen[i]  = 0;
        }

      /* Space for buttons */
      bufsize = (bufsize + 3) & ~3;
      bufsize += sizeof(DLGITEMTEMPLATE) +
                 3 * sizeof(WORD) +
                 (ButtonLen[i] + 1) * sizeof(WCHAR);
    }

    buf = RtlAllocateHeap(GetProcessHeap(), 0, bufsize);
    if(!buf)
    {
      return 0;
    }
    iico = itxt = NULL;

    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof(nclm), &nclm, 0);
    hFont = CreateFontIndirectW(&nclm.lfMessageFont);

    tpl = (DLGTEMPLATE *)buf;

    tpl->style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | DS_CENTER | DS_MODALFRAME | DS_NOIDLEMSG;
    tpl->dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
    if(lpMsgBoxParams->dwStyle & MB_TOPMOST)
      tpl->dwExtendedStyle |= WS_EX_TOPMOST;
    if(lpMsgBoxParams->dwStyle & MB_RIGHT)
      tpl->dwExtendedStyle |= WS_EX_RIGHT;
    tpl->x = 100;
    tpl->y = 100;
    tpl->cdit = Buttons.btnCnt + ((Icon != (HICON)0) ? 1 : 0) + 1;

    dest = (BYTE *)(tpl + 1);

    *(WORD*)dest = 0; /* no menu */
    *(((WORD*)dest) + 1) = 0; /* use default window class */
    dest += 2 * sizeof(WORD);
    memcpy(dest, caption, caplen * sizeof(WCHAR));
    dest += caplen * sizeof(WCHAR);
    *(WCHAR*)dest = L'\0';
    dest += sizeof(WCHAR);

    /* Create icon */
    if(Icon)
    {
      dest = (BYTE*)(((ULONG_PTR)dest + 3) & ~3);
      iico = (DLGITEMTEMPLATE *)dest;
      iico->style = WS_CHILD | WS_VISIBLE | SS_ICON;
      iico->dwExtendedStyle = 0;
      iico->id = MSGBOX_IDICON;

      dest += sizeof(DLGITEMTEMPLATE);
      *(WORD*)dest = 0xFFFF;
      dest += sizeof(WORD);
      *(WORD*)dest = 0x0082; /* static control */
      dest += sizeof(WORD);
      *(WORD*)dest = 0xFFFF;
      dest += sizeof(WORD);
      *(WCHAR*)dest = 0;
      dest += sizeof(WCHAR);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
    }

    /* create static for text */
    dest = (BYTE*)(((UINT_PTR)dest + 3) & ~3);
    itxt = (DLGITEMTEMPLATE *)dest;
    itxt->style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
    if(lpMsgBoxParams->dwStyle & MB_RIGHT)
      itxt->style |= SS_RIGHT;
    else
      itxt->style |= SS_LEFT;
    itxt->dwExtendedStyle = 0;
    itxt->id = MSGBOX_IDTEXT;
    dest += sizeof(DLGITEMTEMPLATE);
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0082; /* static control */
    dest += sizeof(WORD);
    memcpy(dest, text, textlen * sizeof(WCHAR));
    dest += textlen * sizeof(WCHAR);
    *(WCHAR*)dest = 0;
    dest += sizeof(WCHAR);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);

    hDC = CreateCompatibleDC(0);
    SelectObject(hDC, hFont);

    /* create buttons */
    btnsize.cx = BTN_CX;
    btnsize.cy = BTN_CY;
    btnrect.left = btnrect.top = 0;

    for(i = 0; i < Buttons.btnCnt; i++)
    {
      dest = (BYTE*)(((UINT_PTR)dest + 3) & ~3);
      ibtn[i] = (DLGITEMTEMPLATE *)dest;
      ibtn[i]->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
      if(!defbtn && (i == ((lpMsgBoxParams->dwStyle & MB_DEFMASK) >> 8)))
      {
        ibtn[i]->style |= BS_DEFPUSHBUTTON;
        mbi.DefBtn = Buttons.btnIdx[i];
        defbtn = TRUE;
      }
      else
        ibtn[i]->style |= BS_PUSHBUTTON;
      ibtn[i]->dwExtendedStyle = 0;
      ibtn[i]->id = Buttons.btnIdx[i];
      dest += sizeof(DLGITEMTEMPLATE);
      *(WORD*)dest = 0xFFFF;
      dest += sizeof(WORD);
      *(WORD*)dest = 0x0080; /* button control */
      dest += sizeof(WORD);
      memcpy(dest, ButtonText[i], ButtonLen[i] * sizeof(WCHAR));
      dest += ButtonLen[i] * sizeof(WCHAR);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);

      // btnrect.right = btnrect.bottom = 0; // FIXME: Is it needed??
      DrawTextW(hDC, ButtonText[i], ButtonLen[i], &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
      btnsize.cx = max(btnsize.cx, btnrect.right);
      btnsize.cy = max(btnsize.cy, btnrect.bottom);
    }

    /* make first button the default button if no other is */
    if(!defbtn)
    {
      ibtn[0]->style &= ~BS_PUSHBUTTON;
      ibtn[0]->style |= BS_DEFPUSHBUTTON;
      mbi.DefBtn = Buttons.btnIdx[0];
    }

    /* calculate position and size of controls */
    txtrect.right = GetSystemMetrics(SM_CXSCREEN) / 5 * 4;
    if(Icon)
      txtrect.right -= GetSystemMetrics(SM_CXICON) + MSGBOXEX_SPACING;
    txtrect.top = txtrect.left = txtrect.bottom = 0;
    if (textlen != 0)
    {
      DrawTextW(hDC, text, textlen, &txtrect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
    }
    else
    {
      txtrect.right = txtrect.left + 1;
      txtrect.bottom = txtrect.top + 1;
    }
    txtrect.right++;

    if(hDC)
      DeleteDC(hDC);

    /* calculate position and size of the icon */
    rc.left = rc.bottom = rc.right = 0;
    btntop = 0;

    if(iico)
    {
      rc.right = GetSystemMetrics(SM_CXICON);
      rc.bottom = GetSystemMetrics(SM_CYICON);
#ifdef MSGBOX_ICONVCENTER
      rc.top = MSGBOXEX_MARGIN + (max(txtrect.bottom, rc.bottom) / 2) - (GetSystemMetrics(SM_CYICON) / 2);
      rc.top = max(MSGBOXEX_SPACING, rc.top);
#else
      rc.top = MSGBOXEX_MARGIN;
#endif
      btnleft = (Buttons.btnCnt * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
      if(btnleft > txtrect.right + rc.right + MSGBOXEX_SPACING)
      {
#ifdef MSGBOX_TEXTHCENTER
        lmargin = MSGBOXEX_MARGIN + ((btnleft - txtrect.right - rc.right - MSGBOXEX_SPACING) / 2);
#else
        lmargin = MSGBOXEX_MARGIN;
#endif
        btnleft = MSGBOXEX_MARGIN;
      }
      else
      {
        lmargin = MSGBOXEX_MARGIN;
        btnleft = MSGBOXEX_MARGIN + ((txtrect.right + rc.right + MSGBOXEX_SPACING) / 2) - (btnleft / 2);
      }
      rc.left = lmargin;
      iico->x = RESCALE_X(rc.left, units);
      iico->y = RESCALE_Y(rc.top, units);
      iico->cx = RESCALE_X(rc.right, units);
      iico->cy = RESCALE_Y(rc.bottom, units);
      btntop = rc.top + rc.bottom + MSGBOXEX_SPACING;
      rc.left += rc.right + MSGBOXEX_SPACING;
    }
    else
    {
      btnleft = (Buttons.btnCnt * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
      if(btnleft > txtrect.right)
      {
#ifdef MSGBOX_TEXTHCENTER
        lmargin = MSGBOXEX_MARGIN + ((btnleft - txtrect.right) / 2);
#else
        lmargin = MSGBOXEX_MARGIN;
#endif
        btnleft = MSGBOXEX_MARGIN;
      }
      else
      {
        lmargin = MSGBOXEX_MARGIN;
        btnleft = MSGBOXEX_MARGIN + (txtrect.right / 2) - (btnleft / 2);
      }
      rc.left = lmargin;
    }
    /* calculate position of the text */
    rc.top = MSGBOXEX_MARGIN + (rc.bottom / 2) - (txtrect.bottom / 2);
    rc.top = max(rc.top, MSGBOXEX_MARGIN);
    /* calculate position of the buttons */
    btntop = max(rc.top + txtrect.bottom + MSGBOXEX_SPACING, btntop);
    for(i = 0; i < Buttons.btnCnt; i++)
    {
      ibtn[i]->x = RESCALE_X(btnleft, units);
      ibtn[i]->y = RESCALE_Y(btntop, units);
      ibtn[i]->cx = RESCALE_X(btnsize.cx, units);
      ibtn[i]->cy = RESCALE_Y(btnsize.cy, units);
      btnleft += btnsize.cx + MSGBOXEX_BUTTONSPACING;
    }
    /* calculate size and position of the messagebox window */
    btnleft = max(btnleft - MSGBOXEX_BUTTONSPACING, rc.left + txtrect.right);
    btnleft += MSGBOXEX_MARGIN;
    btntop +=  btnsize.cy + MSGBOXEX_MARGIN;
    /* set size and position of the message static */
    itxt->x = RESCALE_X(rc.left, units);
    itxt->y = RESCALE_Y(rc.top, units);
    itxt->cx = RESCALE_X(btnleft - rc.left - MSGBOXEX_MARGIN, units);
    itxt->cy = RESCALE_Y(txtrect.bottom, units);
    /* set size of the window */
    tpl->cx = RESCALE_X(btnleft, units);
    tpl->cy = RESCALE_Y(btntop, units);

    /* finally show the messagebox */
    mbi.Icon = Icon;
    mbi.Font = hFont;
    mbi.dwContextHelpId = lpMsgBoxParams->dwContextHelpId;
    mbi.lpfnMsgBoxCallback = lpMsgBoxParams->lpfnMsgBoxCallback;
    mbi.dwStyle = lpMsgBoxParams->dwStyle;
    mbi.nButtons = Buttons.btnCnt;
    mbi.Btns = Buttons.btnIdx;
    mbi.Timeout = Timeout;

    /* Pass on to Justin Case so he can peek the message? */
    mbi.cbSize       = lpMsgBoxParams->cbSize;
    mbi.hwndOwner    = lpMsgBoxParams->hwndOwner;
    mbi.hInstance    = lpMsgBoxParams->hInstance;
    mbi.lpszText     = lpMsgBoxParams->lpszText;
    mbi.lpszCaption  = lpMsgBoxParams->lpszCaption;
    mbi.lpszIcon     = lpMsgBoxParams->lpszIcon;
    mbi.dwLanguageId = lpMsgBoxParams->dwLanguageId;

    ret = DialogBoxIndirectParamW(lpMsgBoxParams->hInstance, tpl, lpMsgBoxParams->hwndOwner,
                                  MessageBoxProc, (LPARAM)&mbi);

    if(hFont)
      DeleteObject(hFont);

    RtlFreeHeap(GetProcessHeap(), 0, buf);
    return ret;
}

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
int
WINAPI
MessageBoxA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType)
{
    return MessageBoxExA(hWnd, lpText, lpCaption, uType, LANG_NEUTRAL);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxExA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
    MSGBOXPARAMSA msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxIndirectA(&msgbox);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxExW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
    MSGBOXPARAMSW msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxTimeoutIndirectW(&msgbox, (UINT)-1);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxIndirectA(
  CONST MSGBOXPARAMSA *lpMsgBoxParams)
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW, iconW;
    int ret;

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszText))
    {
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpMsgBoxParams->lpszText);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        textW.Buffer[textW.Length / sizeof(WCHAR)] = L'\0';
    }
    else
        textW.Buffer = (LPWSTR)lpMsgBoxParams->lpszText;

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
    {
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpMsgBoxParams->lpszCaption);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        captionW.Buffer[captionW.Length / sizeof(WCHAR)] = L'\0';
    }
    else
        captionW.Buffer = (LPWSTR)lpMsgBoxParams->lpszCaption;

    if(lpMsgBoxParams->dwStyle & MB_USERICON)
    {
        if (!IS_INTRESOURCE(lpMsgBoxParams->lpszIcon))
        {
            RtlCreateUnicodeStringFromAsciiz(&iconW, (PCSZ)lpMsgBoxParams->lpszIcon);
            /*
             * UNICODE_STRING objects are always allocated with an extra byte so you
             * can null-term if you want
             */
            iconW.Buffer[iconW.Length / sizeof(WCHAR)] = L'\0';
        }
        else
            iconW.Buffer = (LPWSTR)lpMsgBoxParams->lpszIcon;
    }
    else
        iconW.Buffer = NULL;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = lpMsgBoxParams->hwndOwner;
    msgboxW.hInstance = lpMsgBoxParams->hInstance;
    msgboxW.lpszText = textW.Buffer;
    msgboxW.lpszCaption = captionW.Buffer;
    msgboxW.dwStyle = lpMsgBoxParams->dwStyle;
    msgboxW.lpszIcon = iconW.Buffer;
    msgboxW.dwContextHelpId = lpMsgBoxParams->dwContextHelpId;
    msgboxW.lpfnMsgBoxCallback = lpMsgBoxParams->lpfnMsgBoxCallback;
    msgboxW.dwLanguageId = lpMsgBoxParams->dwLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)-1);

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszText))
        RtlFreeUnicodeString(&textW);

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
        RtlFreeUnicodeString(&captionW);

    if ((lpMsgBoxParams->dwStyle & MB_USERICON) && !IS_INTRESOURCE(iconW.Buffer))
        RtlFreeUnicodeString(&iconW);

    return ret;
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxIndirectW(
  CONST MSGBOXPARAMSW *lpMsgBoxParams)
{
    return MessageBoxTimeoutIndirectW(lpMsgBoxParams, (UINT)-1);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType)
{
    return MessageBoxExW(hWnd, lpText, lpCaption, uType, LANG_NEUTRAL);
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxTimeoutA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType,
  WORD wLanguageId,
  DWORD dwTime)
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW;
    int ret;

    if (!IS_INTRESOURCE(lpText))
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpText);
    else
        textW.Buffer = (LPWSTR)lpText;

    if (!IS_INTRESOURCE(lpCaption))
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpCaption);
    else
        captionW.Buffer = (LPWSTR)lpCaption;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = hWnd;
    msgboxW.hInstance = 0;
    msgboxW.lpszText = textW.Buffer;
    msgboxW.lpszCaption = captionW.Buffer;
    msgboxW.dwStyle = uType;
    msgboxW.lpszIcon = NULL;
    msgboxW.dwContextHelpId = 0;
    msgboxW.lpfnMsgBoxCallback = NULL;
    msgboxW.dwLanguageId = wLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)dwTime);

    if (!IS_INTRESOURCE(textW.Buffer))
        RtlFreeUnicodeString(&textW);

    if (!IS_INTRESOURCE(captionW.Buffer))
        RtlFreeUnicodeString(&captionW);

    return ret;
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxTimeoutW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType,
  WORD wLanguageId,
  DWORD dwTime)
{
    MSGBOXPARAMSW msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxTimeoutIndirectW(&msgbox, (UINT)dwTime);
}


/*
 * @unimplemented
 */
DWORD
WINAPI
SoftModalMessageBox(DWORD Unknown0)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
MessageBeep(UINT uType)
{
    return NtUserxMessageBeep(uType);
}


/*
 * @implemented
 *
 * See: https://msdn.microsoft.com/en-us/library/windows/desktop/dn910915(v=vs.85).aspx
 * and: http://undoc.airesoft.co.uk/user32.dll/MB_GetString.php
 * for more information.
 */
LPCWSTR WINAPI MB_GetString(UINT wBtn)
{
    LPCWSTR btnStr = NULL;

    /* The allowable IDs are between IDOK (0) and IDCONTINUE (11) inclusive */
    if (wBtn >= IDCONTINUE)
        return NULL;

    LoadStringW(User32Instance, wBtn, (LPWSTR)&btnStr, 0);
    return btnStr;
}

/* EOF */
