#define UNICODE
#include <windows.h>
#include <user32/win.h>
#include <user32/resource.h>


#define MB_TYPEMASK		0x0000000F
#define MB_ICONMASK		0x000000F0
#define MB_DEFMASK		0x00000F00

LRESULT CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam );


// FIXME ?????????
    /* Static text */
#define stc1        0x0440
/**************************************************************************
 *           MessageBoxA   (USER.391)
 *
 * NOTES
 *   The WARN is here to help debug erroneous MessageBoxes
 *   Use: -debugmsg warn+dialog,+relay
 */
INT STDCALL MessageBoxA(HWND hWnd, LPCSTR text, LPCSTR title, UINT type)
{

    return MessageBoxExA(hWnd,text,title,type,0);
 
}


/**************************************************************************
 *           MessageBoxW   (USER.396)
 */
INT STDCALL MessageBoxW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                            UINT type )
{
   return MessageBoxExW(hWnd,text,title,type,0);
}


/**************************************************************************
 *           MessageBoxExA   (USER.392)
 */
INT STDCALL MessageBoxExA( HWND hWnd, LPCSTR text, LPCSTR title,
                              UINT type, WORD langid )
{
    MSGBOXPARAMS mbox;
   
    if (title == NULL)
      	title="Error";
    if (text == NULL)
      	text=" ";

    mbox.lpszCaption = title;
    mbox.lpszText  = text;
    mbox.dwStyle  = type;
    return DialogBoxIndirectParamA( WIN_GetWindowInstance(hWnd),
                                      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      hWnd, MSGBOX_DlgProc, (LPARAM)&mbox );
}

/**************************************************************************
 *           MessageBoxExW   (USER.393)
 */
INT STDCALL MessageBoxExW( HWND hWnd, LPCWSTR text, LPCWSTR title,
                              UINT type, WORD langid )
{
    MSGBOXPARAMS mbox;
   
    if (title == NULL)
      	title=L"Error";
    if (text == NULL)
      	text=L" ";

    mbox.lpszCaption = title;
    mbox.lpszText  = text;
    mbox.dwStyle  = type;
    return DialogBoxIndirectParamW( WIN_GetWindowInstance(hWnd),
                                      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      hWnd, MSGBOX_DlgProc, (LPARAM)&mbox );
}



/**************************************************************************
 *           MessageBoxIndirectA   (USER.394)
 */
INT STDCALL MessageBoxIndirectA( LPMSGBOXPARAMS msgbox )
{
     
    return DialogBoxIndirectParamA( msgbox->hInstance,
   				      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      msgbox->hwndOwner, MSGBOX_DlgProc,
				      (LPARAM)msgbox );
}

/**************************************************************************
 *           MessageBoxIndirectW   (USER.395)
 */
INT STDCALL MessageBoxIndirectW( LPMSGBOXPARAMS msgbox )
{

    return DialogBoxIndirectParamW( msgbox->hInstance,
   				      SYSRES_GetResPtr( SYSRES_DIALOG_MSGBOX ),
                                      msgbox->hwndOwner, MSGBOX_DlgProc,
				      (LPARAM)msgbox );
}

/**************************************************************************
 *           MSGBOX_DlgProc
 *
 * Dialog procedure for message boxes.
 */
LRESULT CALLBACK MSGBOX_DlgProc( HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam )
{
  static HFONT hFont = 0;
  LPMSGBOXPARAMS lpmb;

  RECT rect, textrect;
  HWND hItem;
  HDC hdc;
  LRESULT lRet;
  int i, buttons, bwidth, bheight, theight, wwidth, bpos;
  int borheight, iheight, tiheight;

  NONCLIENTMETRICS nclm;
  
  switch(message) {
   case WM_INITDIALOG:
    	lpmb = (LPMSGBOXPARAMS)lParam;
 
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	hFont = CreateFontIndirect(&nclm.lfMessageFont);
        /* set button font */
	for (i=1; i < 8; i++)
	    SendDlgItemMessageW (hwnd, i, WM_SETFONT, (WPARAM)hFont, 0);
        /* set text font */
	SendDlgItemMessageW (hwnd, 100, WM_SETFONT, (WPARAM)hFont, 0);
 
    	if (lpmb->lpszCaption) 
		SetWindowTextW(hwnd, lpmb->lpszCaption);
    	SetWindowTextW(GetDlgItem(hwnd, 100), lpmb->lpszText);
    /* Hide not selected buttons */
    	switch(lpmb->dwStyle & MB_TYPEMASK) {
     case MB_OK:
     	 ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_OKCANCEL:
      	ShowWindow(GetDlgItem(hwnd, 3), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 4), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 5), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 6), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 7), SW_HIDE);
      break;
     case MB_ABORTRETRYIGNORE:
      	ShowWindow(GetDlgItem(hwnd, 1), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 6), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 7), SW_HIDE);
      break;
     case MB_YESNO:
      	ShowWindow(GetDlgItem(hwnd, 2), SW_HIDE);
      /* fall through */
     case MB_YESNOCANCEL:
      	ShowWindow(GetDlgItem(hwnd, 1), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 3), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 4), SW_HIDE);
      	ShowWindow(GetDlgItem(hwnd, 5), SW_HIDE);
      break;
    }
    /* Set the icon */
    switch(lpmb->dwStyle & MB_ICONMASK) {
     case MB_ICONEXCLAMATION:
      	SendDlgItemMessage(hwnd, stc1, STM_SETICON,
                           (WPARAM)LoadIcon(0, IDI_EXCLAMATION), 0);
      break;
     case MB_ICONQUESTION:
      	SendDlgItemMessage(hwnd, stc1, STM_SETICON,
                           (WPARAM)LoadIcon(0, IDI_QUESTION), 0);
      break;
     case MB_ICONASTERISK:
      	SendDlgItemMessage(hwnd, stc1, STM_SETICON,
                           (WPARAM)LoadIcon(0, IDI_ASTERISK), 0);
      break;
     case MB_ICONHAND:
     default:
      	SendDlgItemMessage(hwnd, stc1, STM_SETICON,
                           (WPARAM)LoadIcon(0, IDI_HAND), 0);
      break;
    }
    
    /* Position everything */
    GetWindowRect(hwnd, &rect);
    borheight = rect.bottom - rect.top;
    wwidth = rect.right - rect.left;
    GetClientRect(hwnd, &rect);
    borheight -= rect.bottom - rect.top;

    /* Get the icon height */
    GetWindowRect(GetDlgItem(hwnd, 1088), &rect);
    iheight = rect.bottom - rect.top;
    
    /* Get the number of visible buttons and their width */
    GetWindowRect(GetDlgItem(hwnd, 2), &rect);
    bheight = rect.bottom - rect.top;
    bwidth = rect.left;
    GetWindowRect(GetDlgItem(hwnd, 1), &rect);
    bwidth -= rect.left;
    for (buttons = 0, i = 1; i < 8; i++)
    {
      hItem = GetDlgItem(hwnd, i);
      if (GetWindowLongW(hItem, GWL_STYLE) & WS_VISIBLE)
	 buttons++;
    }
    
    /* Get the text size */
    hItem = GetDlgItem(hwnd, 100);
    GetWindowRect(hItem, &textrect);
    MapWindowPoints(0, hwnd, (LPPOINT)&textrect, 2);
    
    GetClientRect(hItem, &rect);
    hdc = GetDC(hItem);
    lRet = DrawTextW( hdc, lpmb->lpszText, -1, &rect,
                        DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_CALCRECT);
    theight = rect.bottom  - rect.top;
    tiheight = 16 + max(iheight, theight);
    ReleaseDC(hItem, hdc);
    
    /* Position the text */
    SetWindowPos(hItem, 0, textrect.left, (tiheight - theight) / 2, 
		   rect.right - rect.left, theight,
		   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the icon */
    hItem = GetDlgItem(hwnd, 1088);
    GetWindowRect(hItem, &rect);
    MapWindowPoints(0, hwnd, (LPPOINT)&rect, 2);
    SetWindowPos(hItem, 0, rect.left, (tiheight - iheight) / 2, 0, 0,
		   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Resize the window */
    SetWindowPos(hwnd, 0, 0, 0, wwidth, 8 + tiheight + bheight + borheight,
		   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    
    /* Position the buttons */
    bpos = (wwidth - bwidth * buttons) / 2;
    GetWindowRect(GetDlgItem(hwnd, 1), &rect);
    for (buttons = i = 0; i < 7; i++) {
      /* some arithmetic to get the right order for YesNoCancel windows */
      hItem = GetDlgItem(hwnd, (i + 5) % 7 + 1);
      if (GetWindowLongW(hItem, GWL_STYLE) & WS_VISIBLE) {
	if (buttons++ == ((lpmb->dwStyle & MB_DEFMASK) >> 8)) {
	  SetFocus(hItem);
	  SendMessageW( hItem, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
	}
	SetWindowPos(hItem, 0, bpos, tiheight, 0, 0,
		       SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
	bpos += bwidth;
      }
    }
    return 0;
    break;
    
   case WM_COMMAND:
    switch (wParam)
    {
     case IDOK:
     case IDCANCEL:
     case IDABORT:
     case IDRETRY:
     case IDIGNORE:
     case IDYES:
     case IDNO:
      if ( hFont)
        DeleteObject (hFont);
      EndDialog(hwnd, wParam);
      break;
    }

   default:
    break;
  }
  return 0;
}




