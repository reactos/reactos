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
/* $Id: messagebox.c,v 1.23 2004/01/23 23:38:26 ekohl Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/messagebox.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      2003/07/28  Added some NT features
 *      2003/07/27  Code ported from wine
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <messages.h>
#include <user32.h>
#include <string.h>
#include <ntos/rtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>
#include "resource.h"

typedef UINT *LPUINT;
#include <mmsystem.h>

/* DEFINES *******************************************************************/

#define MSGBOX_IDICON   (1088)
#define MSGBOX_IDTEXT   (100)

#define IDI_HANDA          MAKEINTRESOURCEA(32513)
#define IDI_HANDW          MAKEINTRESOURCEW(32513)
#define IDI_QUESTIONA      MAKEINTRESOURCEA(32514)
#define IDI_QUESTIONW      MAKEINTRESOURCEW(32514)
#define IDI_EXCLAMATIONA   MAKEINTRESOURCEA(32515)
#define IDI_EXCLAMATIONW   MAKEINTRESOURCEW(32515)
#define IDI_ASTERISKA      MAKEINTRESOURCEA(32516)
#define IDI_ASTERISKW      MAKEINTRESOURCEW(32516)
#define IDI_WINLOGOA       MAKEINTRESOURCEA(32517)
#define IDI_WINLOGOW       MAKEINTRESOURCEW(32517)

#ifndef MB_TYPEMASK
#define MB_TYPEMASK             0x0000000F
#endif
#ifndef MB_ICONMASK
#define MB_ICONMASK             0x000000F0
#endif
#ifndef MB_DEFMASK
#define MB_DEFMASK              0x00000F00
#endif

#define BTN_CX (75)
#define BTN_CY (23)

#define MSGBOXEX_SPACING    (16)
#define MSGBOXEX_BUTTONSPACING  (6)
#define MSGBOXEX_MARGIN (12)
#define MSGBOXEX_MAXBTNSTR  (32)
#define MSGBOXEX_MAXBTNS    (4)

typedef struct _MSGBOXINFO {
  HICON Icon;
  HFONT Font;
  DWORD ContextHelpId;
  MSGBOXCALLBACK Callback;
  DWORD Style;
  int DefBtn;
  int nButtons;
  LONG *Btns;
  UINT Timeout;
} MSGBOXINFO, *PMSGBOXINFO;

/* INTERNAL FUNCTIONS ********************************************************/

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static INT_PTR CALLBACK MessageBoxProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  int i;
  PMSGBOXINFO mbi;
  HELPINFO hi;
  HWND owner;
  
  switch(message) {
    case WM_INITDIALOG:
      mbi = (PMSGBOXINFO)lParam;
      if(!GetPropW(hwnd, L"ROS_MSGBOX"))
      {
        SetPropW(hwnd, L"ROS_MSGBOX", (HANDLE)lParam);
        if(mbi->Icon)
          SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM)mbi->Icon, 0);
        SetWindowContextHelpId(hwnd, mbi->ContextHelpId);
        
        /* set control fonts */
        SendDlgItemMessageW(hwnd, MSGBOX_IDTEXT, WM_SETFONT, (WPARAM)mbi->Font, 0);
        for(i = 0; i < mbi->nButtons; i++)
        {
          SendDlgItemMessageW(hwnd, mbi->Btns[i], WM_SETFONT, (WPARAM)mbi->Font, 0);
        }
        switch(mbi->Style & MB_TYPEMASK)
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
    
    case WM_HELP:
      mbi = (PMSGBOXINFO)GetPropW(hwnd, L"ROS_MSGBOX");
      if(!mbi)
        return 0;
      memcpy(&hi, (void *)lParam, sizeof(hi));
      hi.dwContextId = GetWindowContextHelpId(hwnd);

      if (mbi->Callback)
        mbi->Callback(&hi);
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
      switch(mbi->Style & MB_TYPEMASK)
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
  CONST LPMSGBOXPARAMS lpMsgBoxParams, UINT Timeout)
{
    DLGTEMPLATE *tpl;
    DLGITEMTEMPLATE *iico, *itxt;
    NONCLIENTMETRICSW nclm;
    WCHAR capbuf[32];
    HMODULE hUser32;
    LPVOID buf;
    BYTE *dest;
    LPWSTR caption, text;
    HFONT hFont;
    HICON Icon;
    HDC hDC;
    int bufsize, ret, caplen, textlen, btnlen, i, btnleft, btntop, lmargin, nButtons = 0;
    LONG Buttons[MSGBOXEX_MAXBTNS];
    WCHAR ButtonText[MSGBOXEX_MAXBTNS][MSGBOXEX_MAXBTNSTR];
    DLGITEMTEMPLATE *ibtn[MSGBOXEX_MAXBTNS];
    RECT btnrect, txtrect, rc;
    SIZE btnsize;
    MSGBOXINFO mbi;
    BOOL defbtn = FALSE;
    DWORD units = GetDialogBaseUnits();
    
    hUser32 = GetModuleHandleW(L"USER32");
    
    if(!lpMsgBoxParams->lpszCaption || !HIWORD((LPWSTR)lpMsgBoxParams->lpszCaption))
    {
      LoadStringW(hUser32, IDS_ERROR, &capbuf[0], 32);
      caption = &capbuf[0];
    }
    else
      caption = (LPWSTR)lpMsgBoxParams->lpszCaption;
      
    if(!lpMsgBoxParams->lpszText || !HIWORD((LPWSTR)lpMsgBoxParams->lpszText))
      text = L"";
    else
      text = (LPWSTR)lpMsgBoxParams->lpszText;
    
    caplen = strlenW(caption);
    textlen = strlenW(text);
    
    /* Create selected buttons */
    switch(lpMsgBoxParams->dwStyle & MB_TYPEMASK)
    {
        case MB_OKCANCEL:
            Buttons[0] = IDOK;
            Buttons[1] = IDCANCEL;
            nButtons = 2;
            break;
        case MB_CANCELTRYCONTINUE:
            Buttons[0] = IDCANCEL;
            Buttons[1] = IDTRYAGAIN;
            Buttons[2] = IDCONTINUE;
            nButtons = 3;
            break;
        case MB_ABORTRETRYIGNORE:
            Buttons[0] = IDABORT;
            Buttons[1] = IDRETRY;
            Buttons[2] = IDIGNORE;
            nButtons = 3;
            break;
        case MB_YESNO:
            Buttons[0] = IDYES;
            Buttons[1] = IDNO;
            nButtons = 2;
            break;
        case MB_YESNOCANCEL:
            Buttons[0] = IDYES;
            Buttons[1] = IDNO;
            Buttons[2] = IDCANCEL;
            nButtons = 3;
            break;
        case MB_RETRYCANCEL:
            Buttons[0] = IDRETRY;
            Buttons[1] = IDCANCEL;
            nButtons = 2;
            break;
        case MB_OK:
            /* fall through */
        default:
            Buttons[0] = IDOK;
            nButtons = 1;
            break;
    }
    /* Create Help button */
    if(lpMsgBoxParams->dwStyle & MB_HELP)
      Buttons[nButtons++] = IDHELP;
    
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
        Icon = LoadIconW(lpMsgBoxParams->hInstance, (LPCWSTR)lpMsgBoxParams->lpszIcon);
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
    
    bufsize = 0;
    
    for(i = 0; i < nButtons; i++)
    {
      switch(Buttons[i])
      {
        case IDOK:
          LoadStringW(hUser32, IDS_OK, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDCANCEL:
          LoadStringW(hUser32, IDS_CANCEL, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDYES:
          LoadStringW(hUser32, IDS_YES, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDNO:
          LoadStringW(hUser32, IDS_NO, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDTRYAGAIN:
          LoadStringW(hUser32, IDS_TRYAGAIN, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDCONTINUE:
          LoadStringW(hUser32, IDS_CONTINUE, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDABORT:
          LoadStringW(hUser32, IDS_ABORT, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDRETRY:
          LoadStringW(hUser32, IDS_RETRY, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDIGNORE:
          LoadStringW(hUser32, IDS_IGNORE, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        case IDHELP:
          LoadStringW(hUser32, IDS_HELP, ButtonText[i], MSGBOXEX_MAXBTNSTR - 1);
          break;
        default:
          ButtonText[i][0] = (WCHAR)0;
          break;
      }
      if(ButtonText[i][0])
      {
        bufsize += ((strlenW(ButtonText[i]) + 1) * sizeof(WCHAR));
      }
    }
    
    if(Icon)
    {
      bufsize += sizeof(DLGITEMTEMPLATE)+3 + (5 * sizeof(WORD)); /* icon */
    }
    
    bufsize += sizeof(DLGTEMPLATE) + 
               (2 * sizeof(WORD)) +                         /* menu and class */
               ((caplen + 1) * sizeof(WCHAR)) +             /* title */
               ((nButtons + 1) * sizeof(DLGITEMTEMPLATE)) + /* text and controls */
               ((textlen + 1) * sizeof(WCHAR));             /* text */
    
    bufsize += 1024; /* FIXME */
    buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, bufsize);
    if(!buf)
    {
      return 0;
    }
    iico = itxt = NULL;
    
    hDC = CreateCompatibleDC(0);
    
    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof(nclm), &nclm, 0);
    hFont = CreateFontIndirectW (&nclm.lfMessageFont);
    
    tpl = (DLGTEMPLATE *)buf;
    
    tpl->style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU | DS_CENTER | DS_MODALFRAME | DS_NOIDLEMSG;
    tpl->dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
    if(lpMsgBoxParams->dwStyle & MB_TOPMOST)
      tpl->dwExtendedStyle |= WS_EX_TOPMOST;
    if(lpMsgBoxParams->dwStyle & MB_RIGHT)
      tpl->dwExtendedStyle |= WS_EX_RIGHT;
    tpl->x = 100;
    tpl->y = 100;
    tpl->cdit = nButtons + (Icon != (HICON)0) + 1;
    
    dest = (BYTE *)(tpl + 1);
    
    *(WORD*)dest = 0; /* no menu */
    *(((WORD*)dest) + 1) = 0; /* use default window class */
    dest += 2 * sizeof(WORD);
    memcpy(dest, caption, caplen * sizeof(WCHAR));
    dest += caplen * sizeof(WCHAR);
    *(WORD*)dest = 0;
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
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
    }
    
    /* create static for text */
    dest = (BYTE*)(((DWORD)dest + 3) & ~3);
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
    *(WORD*)dest = 0;
    dest += sizeof(WORD);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);
    
    /* create buttons */
    btnsize.cx = BTN_CX;
    btnsize.cy = BTN_CY;
    btnrect.left = btnrect.top = 0;
    for(i = 0; i < nButtons; i++)
    {
      dest = (BYTE*)(((DWORD)dest + 3) & ~3);
      ibtn[i] = (DLGITEMTEMPLATE *)dest;
      ibtn[i]->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
      if(!defbtn && (i == ((lpMsgBoxParams->dwStyle & MB_DEFMASK) >> 8)))
      {
        ibtn[i]->style |= BS_DEFPUSHBUTTON;
        mbi.DefBtn = Buttons[i];
        defbtn = TRUE;
      }
      else
        ibtn[i]->style |= BS_PUSHBUTTON;
      ibtn[i]->dwExtendedStyle = 0;
      ibtn[i]->id = Buttons[i];
      dest += sizeof(DLGITEMTEMPLATE);
      *(WORD*)dest = 0xFFFF;
      dest += sizeof(WORD);
      *(WORD*)dest = 0x0080; /* button control */
      dest += sizeof(WORD);
      btnlen = strlenW(ButtonText[i]);
      memcpy(dest, ButtonText[i], btnlen * sizeof(WCHAR));
      dest += btnlen * sizeof(WCHAR);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
      *(WORD*)dest = 0;
      dest += sizeof(WORD);
      SelectObject(hDC, hFont);
      DrawTextW(hDC, ButtonText[i], btnlen, &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
      btnsize.cx = max(btnsize.cx, btnrect.right);
      btnsize.cy = max(btnsize.cy, btnrect.bottom);
    }

	if ( (ULONG_PTR)dest > ((ULONG_PTR)buf + (ULONG_PTR)bufsize) ) 
    { 
      DbgPrint ( "buffer overrun in messagebox.c ( bufsize = %lu, but used %lu )\n", bufsize, (ULONG_PTR)dest-(ULONG_PTR)buf );
    }
    
    /* make first button the default button if no other is */
    if(!defbtn)
    {
      ibtn[0]->style &= ~BS_PUSHBUTTON;
      ibtn[0]->style |= BS_DEFPUSHBUTTON;
      mbi.DefBtn = Buttons[0];
    }
    
    /* calculate position and size of controls */
    txtrect.right = GetSystemMetrics(SM_CXSCREEN) / 5 * 4;
    if(Icon)
      txtrect.right -= GetSystemMetrics(SM_CXICON) + MSGBOXEX_SPACING;
    txtrect.top = txtrect.left = txtrect.bottom = 0;
    SelectObject(hDC, hFont);
    DrawTextW(hDC, text, textlen, &txtrect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
    txtrect.right++;
    
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
      btnleft = (nButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
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
      iico->x = (rc.left * 4) / LOWORD(units);
      iico->y = (rc.top * 8) / HIWORD(units);
      iico->cx = (rc.right * 4) / LOWORD(units);
      iico->cy = (rc.bottom * 8) / HIWORD(units);
      btntop = rc.top + rc.bottom + MSGBOXEX_SPACING;
      rc.left += rc.right + MSGBOXEX_SPACING;
    }
    else
    {
      btnleft = (nButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
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
    for(i = 0; i < nButtons; i++)
    {
      ibtn[i]->x = (btnleft * 4) / LOWORD(units);
      ibtn[i]->y = (btntop * 8) / HIWORD(units);
      ibtn[i]->cx = (btnsize.cx * 4) / LOWORD(units);
      ibtn[i]->cy = (btnsize.cy * 8) / HIWORD(units);
      btnleft += btnsize.cx + MSGBOXEX_BUTTONSPACING;
    }
    /* calculate size and position of the messagebox window */
    btnleft = max(btnleft - MSGBOXEX_BUTTONSPACING, rc.left + txtrect.right); 
    btnleft += MSGBOXEX_MARGIN;
    btntop +=  btnsize.cy + MSGBOXEX_MARGIN;
    /* set size and position of the message static */
    itxt->x = (rc.left * 4) / LOWORD(units);
    itxt->y = (rc.top * 8) / HIWORD(units);
    itxt->cx = (((btnleft - rc.left - MSGBOXEX_MARGIN) * 4) / LOWORD(units));
    itxt->cy = ((txtrect.bottom * 8) / HIWORD(units));
    /* set size of the window */
    tpl->cx = (btnleft * 4) / LOWORD(units);
    tpl->cy = (btntop * 8) / HIWORD(units);
    
    /* finally show the messagebox */
    mbi.Icon = Icon;
    mbi.Font = hFont;
    mbi.ContextHelpId = lpMsgBoxParams->dwContextHelpId;
    mbi.Callback = lpMsgBoxParams->lpfnMsgBoxCallback;
    mbi.Style = lpMsgBoxParams->dwStyle;
    mbi.nButtons = nButtons;
    mbi.Btns = &Buttons[0];
    mbi.Timeout = Timeout;
    
    if(hDC)
      DeleteDC(hDC);
    
    ret =  DialogBoxIndirectParamW(lpMsgBoxParams->hInstance, tpl, lpMsgBoxParams->hwndOwner,
                                   MessageBoxProc, (LPARAM)&mbi);

    if(hFont)
      DeleteObject(hFont);
    
    RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
    return ret;
}

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
int
STDCALL
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
STDCALL
MessageBoxExA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
    MSGBOXPARAMS msgbox;

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
STDCALL
MessageBoxExW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
    MSGBOXPARAMS msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = (LPCSTR)lpText;
    msgbox.lpszCaption =(LPCSTR) lpCaption;
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
STDCALL
MessageBoxIndirectA(
  CONST LPMSGBOXPARAMS lpMsgBoxParams)
{
    MSGBOXPARAMS msgboxW;
    UNICODE_STRING textW, captionW, iconW;
    int ret;

    if (HIWORD((UINT)lpMsgBoxParams->lpszText))
    {
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpMsgBoxParams->lpszText);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        textW.Buffer[textW.Length] = L'\0';
    }
    else
        textW.Buffer = (LPWSTR)lpMsgBoxParams->lpszText;

    if (HIWORD((UINT)lpMsgBoxParams->lpszCaption))
    {
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpMsgBoxParams->lpszCaption);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        captionW.Buffer[captionW.Length] = L'\0';
    }
    else
        captionW.Buffer = (LPWSTR)lpMsgBoxParams->lpszCaption;

    if (HIWORD((UINT)lpMsgBoxParams->lpszIcon))
    {
        RtlCreateUnicodeStringFromAsciiz(&iconW, (PCSZ)lpMsgBoxParams->lpszIcon);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        iconW.Buffer[iconW.Length] = L'\0';
    }
    else
        iconW.Buffer = (LPWSTR)lpMsgBoxParams->lpszIcon;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = lpMsgBoxParams->hwndOwner;
    msgboxW.hInstance = lpMsgBoxParams->hInstance;
    msgboxW.lpszText = (LPCSTR)textW.Buffer;
    msgboxW.lpszCaption = (LPCSTR)captionW.Buffer;
    msgboxW.dwStyle = lpMsgBoxParams->dwStyle;
    msgboxW.lpszIcon = (LPCSTR)iconW.Buffer;
    msgboxW.dwContextHelpId = lpMsgBoxParams->dwContextHelpId;
    msgboxW.lpfnMsgBoxCallback = lpMsgBoxParams->lpfnMsgBoxCallback;
    msgboxW.dwLanguageId = lpMsgBoxParams->dwLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)-1);

    if (HIWORD((UINT)lpMsgBoxParams->lpszText))
        RtlFreeUnicodeString(&textW);

    if (HIWORD((UINT)lpMsgBoxParams->lpszCaption))
        RtlFreeUnicodeString(&captionW);

    if (HIWORD((UINT)lpMsgBoxParams->lpszIcon))
        RtlFreeUnicodeString(&iconW);

    return ret;
}


/*
 * @implemented
 */
int
STDCALL
MessageBoxIndirectW(
  CONST LPMSGBOXPARAMS lpMsgBoxParams)
{
    return MessageBoxTimeoutIndirectW(lpMsgBoxParams, (UINT)-1);
}


/*
 * @implemented
 */
int
STDCALL
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
STDCALL
MessageBoxTimeoutA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType,
  WORD wLanguageId,
  DWORD dwTime)
{
    MSGBOXPARAMS msgboxW;
    UNICODE_STRING textW, captionW;
    int ret;

    if (HIWORD((UINT)lpText))
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpText);
    else
        textW.Buffer = (LPWSTR)lpText;

    if (HIWORD((UINT)lpCaption))
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpCaption);
    else
        captionW.Buffer = (LPWSTR)lpCaption;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = hWnd;
    msgboxW.hInstance = 0;
    msgboxW.lpszText = (LPCSTR)textW.Buffer;
    msgboxW.lpszCaption = (LPCSTR)captionW.Buffer;
    msgboxW.dwStyle = uType;
    msgboxW.lpszIcon = NULL;
    msgboxW.dwContextHelpId = 0;
    msgboxW.lpfnMsgBoxCallback = NULL;
    msgboxW.dwLanguageId = wLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)dwTime);

    if (HIWORD(textW.Buffer))
        RtlFreeUnicodeString(&textW);

    if (HIWORD(captionW.Buffer))
        RtlFreeUnicodeString(&captionW);

    return ret;
}

/*
 * @implemented
 */
int
STDCALL
MessageBoxTimeoutW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType,
  WORD wLanguageId,
  DWORD dwTime)
{
    MSGBOXPARAMS msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = (LPCSTR)lpText;
    msgbox.lpszCaption =(LPCSTR) lpCaption;
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
STDCALL
SoftModalMessageBox(DWORD Unknown0)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
MessageBeep(UINT uType)
{
#if 0
  LPWSTR EventName;

  switch(uType)
  {
    case 0xFFFFFFFF:
      if(waveOutGetNumDevs() == 0)
        return Beep(500, 100);    // Beep through speaker
      /* fall through */
    case MB_OK: 
      EventName = L"SystemDefault";
      break;
    case MB_ICONASTERISK:
      EventName = L"SystemAsterisk";
      break;
    case MB_ICONEXCLAMATION:
      EventName = L"SystemExclamation";
      break;
    case MB_ICONHAND:
      EventName = L"SystemHand";
      break;
    case MB_ICONQUESTION:
      EventName = L"SystemQuestion";
      break;
  }

  return PlaySoundW((LPCWSTR)EventName, NULL, SND_ALIAS | SND_NOWAIT | SND_NOSTOP | SND_ASYNC);
#else
  return Beep(500, 100);    // Beep through speaker
#endif
}

/* EOF */
