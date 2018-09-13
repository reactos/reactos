
#include "precomp.h"
#pragma hdrstop

BOOL bBkgdOrText;
BOOL bCaptured;
BOOL bHotText;
BOOL bChanged;
short nChanged;

extern TCHAR g_szClose[];

#define CXYDESKPATTERN 8
#define NONE_LENGTH 16

WORD patbits[CXYDESKPATTERN];                  /* bits for Background pattern */

HWND g_hWndListSrc = NULL;  // yucko, but it worked right from win31 with this

void FAR PASCAL ReadPattern(LPTSTR lpStr, WORD FAR *patbits);

HBRUSH NEAR PASCAL CreateBkPatBrush(HWND hDlg)
{
  HBITMAP hbmDesktop, hbmMem;
  HDC hdcScreen, hdcMemSrc, hdcMemDest;
  HBRUSH hbrPat = NULL;

  if (hbmDesktop=CreateBitmap(CXYDESKPATTERN, CXYDESKPATTERN, 1, 1, patbits))
    {
      hdcScreen = GetDC(hDlg);

      if (hdcMemSrc=CreateCompatibleDC(hdcScreen))
	{
	  SelectObject(hdcMemSrc, hbmDesktop);

	  if (hbmMem=CreateCompatibleBitmap(hdcScreen,
		CXYDESKPATTERN, CXYDESKPATTERN))
	    {
	      if (hdcMemDest=CreateCompatibleDC(hdcScreen))
		{
		  SelectObject(hdcMemDest, hbmMem);
		  SetTextColor(hdcMemDest, GetSysColor(COLOR_BACKGROUND));
		  SetBkColor(hdcMemDest, GetSysColor(COLOR_WINDOWTEXT));
		  BitBlt(hdcMemDest, 0, 0, CXYDESKPATTERN, CXYDESKPATTERN,
			hdcMemSrc, 0, 0, SRCCOPY);

		  hbrPat = CreatePatternBrush(hbmMem);

		  /* Clean up */
		  DeleteDC(hdcMemDest);
		}
	      DeleteObject(hbmMem);
	    }
	  DeleteDC(hdcMemSrc);
	}

      ReleaseDC(hDlg, hdcScreen);

      DeleteObject(hbmDesktop);
    }

  return(hbrPat);
}

void PatternPaint(HWND hDlg, HDC hDC, LPRECT lprUpdate)
{
  short x, y;
  RECT rBox, PatRect;
  HBRUSH hbrBkgd, hbrText;

  GetWindowRect(GetDlgItem(hDlg, IDD_PATTERN), (LPRECT)&PatRect);
  PatRect.top++, PatRect.left++, PatRect.bottom--, PatRect.right--;
  // Use MapWindowPoints instead of ScreenToClient 
  // because it works on mirrored windows and on non mirrored windows.
  MapWindowPoints(NULL, hDlg, (LPPOINT) &PatRect, 2);
  if (IntersectRect((LPRECT)&rBox, (LPRECT)lprUpdate, (LPRECT)&PatRect))
    {
      if (hbrBkgd=CreateSolidBrush(GetNearestColor(hDC,
	    GetSysColor(COLOR_BACKGROUND))))
	{
	  if (hbrText=CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT)))
	    {
	      rBox.right = PatRect.left;
	      for (x = 0; x < CXYDESKPATTERN; x++)
		{
		  rBox.left = rBox.right;
		  rBox.right = PatRect.left+((PatRect.right-PatRect.left)*(x+1))/CXYDESKPATTERN;
		  rBox.bottom = PatRect.top;
		  for (y = 0; y < CXYDESKPATTERN; y++)
		    {
		      rBox.top = rBox.bottom;
		      rBox.bottom = PatRect.top+((PatRect.bottom-PatRect.top)*(y+1))/CXYDESKPATTERN;
		      FillRect(hDC, (LPRECT) &rBox,
                            (patbits[y] & (0x01 << (7-x))) ? hbrText : hbrBkgd);
		    }
		}
	      DeleteObject(hbrText);
	    }
	  DeleteObject(hbrBkgd);
	}
    }
  GetWindowRect(GetDlgItem(hDlg, IDD_PATSAMPLE), (LPRECT)&PatRect);
  PatRect.top++, PatRect.left++, PatRect.bottom--, PatRect.right--;
  // Use MapWindowPoints instead of ScreenToClient 
  // because it works on mirrored windows and on non mirrored windows.
  MapWindowPoints(NULL, hDlg, (LPPOINT) &PatRect, 2);
  if (IntersectRect((LPRECT)&rBox, (LPRECT)lprUpdate, (LPRECT)&PatRect))
    {
      if (hbrBkgd=CreateBkPatBrush(hDlg))
	{
	  FillRect(hDC, (LPRECT) &rBox, hbrBkgd);
	  DeleteObject(hbrBkgd);
	}
    }
}

void PatternUpdate(HWND hDlg)
{
  RECT rBox;
  HDC hDC = GetDC(hDlg);

  GetWindowRect(GetDlgItem(hDlg, IDD_PATTERN), (LPRECT)&rBox);
  // Use MapWindowPoints instead of ScreenToClient 
  // because it works on mirrored windows and on non mirrored windows.
  MapWindowPoints(NULL, hDlg, (LPPOINT) &rBox, 2);
  PatternPaint(hDlg, hDC, (LPRECT)&rBox);
  GetWindowRect(GetDlgItem(hDlg, IDD_PATSAMPLE), (LPRECT)&rBox);
  // Use MapWindowPoints instead of ScreenToClient 
  // because it works on mirrored windows and on non mirrored windows.
  MapWindowPoints(NULL, hDlg, (LPPOINT) &rBox, 2);
  PatternPaint(hDlg, hDC, (LPRECT)&rBox);
  ReleaseDC(hDlg, hDC);
  return;
}

BOOL RemoveMsgBox( HWND hWnd, LPTSTR lpStr1 )
{
  TCHAR lpS[CCH_MAX_STRING];
  TCHAR removemsg[CCH_MAX_STRING];
  TCHAR caption[CCH_MAX_STRING];

  LoadString(hInstance, IDS_REMOVEPATCAPTION, caption, ARRAYSIZE(caption));
  LoadString(hInstance, IDS_PAT_REMOVE, removemsg, ARRAYSIZE(removemsg));
  wsprintf( lpS, removemsg, lpStr1);
  MessageBeep( MB_ICONQUESTION );
  return MessageBox(hWnd, lpS, caption, MB_YESNO | MB_ICONQUESTION) == IDYES;
}

int ChangeMsgBox( HWND hWnd, LPTSTR lpStr1, BOOL fNew )
{
  TCHAR lpS[CCH_MAX_STRING];
  TCHAR changemsg[CCH_MAX_STRING];
  TCHAR caption[CCH_MAX_STRING];

  LoadString(hInstance, IDS_CHANGEPATCAPTION, caption, ARRAYSIZE(caption));
  LoadString(hInstance, ( fNew? IDS_PAT_CREATE : IDS_PAT_CHANGE ),
    changemsg, ARRAYSIZE(changemsg));
  wsprintf( lpS, changemsg, lpStr1);
  MessageBeep( MB_ICONQUESTION );
  return MessageBox( hWnd, lpS, caption, MB_YESNOCANCEL | MB_ICONQUESTION );
}

int NEAR PASCAL CheckSaveChanges( HWND hDlg )
{
    int result = IDYES;
    BOOL fNew = IsWindowEnabled( GetDlgItem( hDlg, IDD_ADDPATTERN ) );

    if( fNew || IsWindowEnabled( GetDlgItem( hDlg, IDD_CHANGEPATTERN ) ) )
    {
        TCHAR name[ 81 ];

        SendMessage( GetDlgItem( hDlg, IDD_PATTERNCOMBO ), WM_GETTEXT, ARRAYSIZE(name) - 1,
            (LPARAM)name );

        if( ( result = ChangeMsgBox( hDlg, name, fNew ) ) == IDYES )
            SendMessage( hDlg, WM_APP, fNew, 0L );
    }

    return result;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PatternDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

const static DWORD /* _based(__segname("_CODE")) */ aPatternHelpIDs[] = {  // Context Help IDs
  IDC_NO_HELP_1,       NO_HELP,
  IDC_NO_HELP_2,       NO_HELP,
  IDD_PATTERNCOMBO,    IDH_PATTERN_EDIT_NAME,
  IDD_ADDPATTERN,      IDH_PATTERN_EDIT_ADD,
  IDD_CHANGEPATTERN,   IDH_PATTERN_EDIT_CHANGE,
  IDD_DELPATTERN,      IDH_PATTERN_EDIT_REMOVE,
  IDD_PATSAMPLE_TXT,   IDH_PATTERN_EDIT_SAMPLE,
  IDD_PATSAMPLE,       IDH_PATTERN_EDIT_SAMPLE,
  IDD_PATTERN,         IDH_PATTERN_EDIT_PIXEL_SCREEN,
  IDD_PATTERN_TXT,     IDH_PATTERN_EDIT_PIXEL_SCREEN,
  IDCANCEL,            IDH_PATTERN_EDIT_EXIT,

  0, 0
};

BOOL CALLBACK PatternDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  HWND hWndCombo;
  short nPats;
  int x, y;
  TCHAR szPats[81];
  TCHAR szLHS[81], szRHS[81];
  short nOldBkMode;
  RECT rBox;
  PAINTSTRUCT ps;
  POINT ptParam;

  switch (wMsg)
    {
      case WM_INITDIALOG:
        bChanged = bCaptured = bHotText = FALSE;

        GetWindowText(hDlg, szPats, ARRAYSIZE(szPats));

        hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);
        g_hWndListSrc = (HWND)lParam;
        for (nPats = (WORD)SendMessage(g_hWndListSrc, LB_GETCOUNT, 0, 0L) - 1;
                                                          nPats > 0; nPats--)
          {
            SendMessage(g_hWndListSrc, LB_GETTEXT,nPats,(LPARAM)szPats);
            SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)szPats);
          }

/* This is kind of cute.  Since the NULL selection is index 0 in the parent,
   the indexes are 1 less than in the parent, and the user won't be editing
   a NULL bitmap, selecting (nPats - 1) highlights the appropriate item, or
   no item at all if the index was 0.  */

        nChanged = (WORD)SendMessage(g_hWndListSrc, LB_GETCURSEL, (WPARAM)NULL, 0L),
        SendMessage(hWndCombo, CB_SETCURSEL, --nChanged, 0L);
        if (nChanged >= 0)
          {
            SendMessage(hWndCombo, CB_GETLBTEXT, nChanged, (LPARAM)szPats);
            GetPrivateProfileString(g_szPatterns, szPats, g_szNULL, szPats, 81, g_szControlIni);
            ReadPattern(szPats,patbits);
          }
        else
          {
            patbits[0] = patbits[1] = patbits[2] = patbits[3] =
            patbits[4] = patbits[5] = patbits[6] = patbits[7] = 0;
          }
/* Both ADD and CHANGE are always disabled, since no new scheme has been
   entered and no change to an old scheme has been made.  DELETE is
   enabled only if an old scheme is selected. */
        EnableWindow(GetDlgItem(hDlg, IDD_DELPATTERN), nChanged >= 0);
        EnableWindow(GetDlgItem(hDlg, IDD_ADDPATTERN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_CHANGEPATTERN), FALSE);
        break;

      case WM_PAINT:
        BeginPaint(hDlg, &ps);
        nOldBkMode = SetBkMode(ps.hdc, TRANSPARENT);
        PatternPaint(hDlg,  ps.hdc, &ps.rcPaint);
        SetBkMode(ps.hdc, nOldBkMode);
        EndPaint(hDlg, &ps);
        return FALSE;		// let defdlg proc see this
        break;

      case WM_MOUSEMOVE:
        if (!bCaptured)
            return(FALSE);     /* Let it fall through to the DefDlgProc */

      case WM_LBUTTONDOWN:
        GetWindowRect(GetDlgItem(hDlg, IDD_PATTERN), (LPRECT)&rBox);
        // Use MapWindowPoints instead of ScreenToClient 
        // because it works on mirrored windows and on non mirrored windows.
        MapWindowPoints(NULL, hDlg, (LPPOINT) &rBox, 2);
        LPARAM2POINT(lParam, &ptParam);
        if (PtInRect((LPRECT) &rBox, ptParam))
          {
            x = ((ptParam.x - rBox.left) * CXYDESKPATTERN)
                                                   / (rBox.right - rBox.left);
            y = ((ptParam.y - rBox.top) * CXYDESKPATTERN)
                                                   / (rBox.bottom - rBox.top);

            nPats = patbits[y];     /* Save old value */
            if (wMsg == WM_LBUTTONDOWN)
              {
                SetCapture(hDlg);
                EnableWindow(GetDlgItem(hDlg, IDD_DELPATTERN), FALSE);
                bChanged = bCaptured = TRUE;
                bBkgdOrText = patbits[y] & (0x01 << (7-x));

                if( nChanged >= 0 )
                  EnableWindow(GetDlgItem(hDlg,IDD_CHANGEPATTERN),TRUE);
              }
            if (bBkgdOrText)
                patbits[y] &= (~(0x01 << (7-x)));
            else
                patbits[y] |= (0x01 << (7-x));
            if (nPats != (short) patbits[y])
                PatternUpdate(hDlg);
          }
        return(FALSE);     /* Let it fall through to the DefDlgProc */
        break;

      case WM_LBUTTONUP:
        if (bCaptured)
          {
            ReleaseCapture();
            bCaptured = FALSE;
          }
        return(FALSE);     /* Let it fall through to the DefDlgProc */
        break;

      case WM_APP:  // we send this to ourselves to save the current item
        hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);
        SendMessage(hWndCombo, WM_GETTEXT, ARRAYSIZE(szLHS)-1, (LPARAM)szLHS);
        if (wParam) {
            nChanged = (short) SendMessage(hWndCombo, CB_ADDSTRING, 0, 
                                            (LPARAM)szLHS);
            SendMessage (hWndCombo,CB_SETCURSEL,nChanged,0L);
        }

        wsprintf(szRHS, TEXT("%d %d %d %d %d %d %d %d"),
                (int)patbits[0], (int)patbits[1], (int)patbits[2], (int)patbits[3],
                (int)patbits[4], (int)patbits[5], (int)patbits[6], (int)patbits[7] );

        WritePrivateProfileString(g_szPatterns, szLHS,
                                        szRHS, g_szControlIni);
        bChanged = FALSE;

        // Make sure we don't disable a button that has the current focus
        {
            HWND hwndFoc = GetFocus();
            if (hwndFoc == GetDlgItem(hDlg, IDD_ADDPATTERN) || hwndFoc == GetDlgItem(hDlg, IDD_CHANGEPATTERN)) {
                SendMessage( hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDD_PATTERNCOMBO), TRUE );
            }
        }

        EnableWindow( GetDlgItem (hDlg, IDD_ADDPATTERN), FALSE);
        EnableWindow( GetDlgItem (hDlg, IDD_CHANGEPATTERN), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDD_DELPATTERN),TRUE);
        break;

      case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
          HELP_WM_HELP, (DWORD)aPatternHelpIDs);
        break;

      case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
          (DWORD)(LPVOID) aPatternHelpIDs);
        break;

      case WM_COMMAND:
        switch (LOWORD(wParam))
          {
            case IDD_PATTERNCOMBO:
                hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);
                switch( HIWORD(wParam) )
                {
                    case CBN_SELCHANGE:
                        nChanged=x=(short)SendMessage(hWndCombo, CB_GETCURSEL, 0, 0L);
                        SendMessage(hWndCombo, CB_GETLBTEXT, x, (LPARAM)szLHS);
                        GetPrivateProfileString(g_szPatterns, szLHS, g_szNULL, szPats, ARRAYSIZE(szPats), g_szControlIni);
                        ReadPattern(szPats,patbits);
                        PatternUpdate(hDlg);
                        EnableWindow(GetDlgItem(hDlg, IDD_ADDPATTERN),bChanged=FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDD_CHANGEPATTERN), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDD_DELPATTERN), TRUE);
                        bHotText = FALSE;
                        break;

                    case CBN_EDITCHANGE:
                        x = (short)SendMessage( hWndCombo, WM_GETTEXTLENGTH, 0, 0L );

                        if( x <= 0 )
                        {
                            EnableWindow(GetDlgItem(hDlg, IDD_DELPATTERN), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDD_CHANGEPATTERN), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDD_ADDPATTERN), FALSE);
                        }
                        else
                        {
                            SendMessage(hWndCombo, WM_GETTEXT, ARRAYSIZE(szLHS)-1, (LPARAM)szLHS);
                            y = (int)SendMessage(hWndCombo, CB_FINDSTRINGEXACT,
                                                (WPARAM)-1, (LPARAM)szLHS);
                            bHotText = ( y >= 0 );

                            if( bHotText && ( nChanged != y ) )
                            {
                                nChanged = y;
                                bChanged = TRUE;
                            }

                            EnableWindow( GetDlgItem( hDlg, IDD_DELPATTERN ),
                                ( y >= 0 ) );
                            EnableWindow( GetDlgItem( hDlg, IDD_CHANGEPATTERN ),
                                ( y >= 0 ) );
                            EnableWindow( GetDlgItem( hDlg, IDD_ADDPATTERN ),
                                ( y < 0 ) );
                        }
                        break;

                    case CBN_SETFOCUS:
                        bHotText = FALSE;
                        break;

                    case CBN_KILLFOCUS:
                        if( bHotText )
                        {
                            // update here so we don't disrupt the typing
                            SendMessage( GetDlgItem( hDlg, IDD_PATTERNCOMBO ),
                                CB_SETCURSEL, nChanged, 0L );
                            bHotText = FALSE;
                        }
                        break;
                }
                break;

            case IDD_ADDPATTERN:
            case IDD_CHANGEPATTERN:
              SendMessage( hDlg, WM_APP, (LOWORD(wParam) == IDD_ADDPATTERN), 0L );
              break;

            case IDD_DELPATTERN:
              hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);
              if ((x = (short)SendMessage(hWndCombo, CB_GETCURSEL, 0, 0L)) >= 0)
                {
                  SendMessage(hWndCombo, WM_GETTEXT, ARRAYSIZE(szLHS)-1, (LPARAM)szLHS);
                  if (RemoveMsgBox(hDlg, szLHS)) {
                      SendMessage(hWndCombo, CB_DELETESTRING, x, 0L);
                      WritePrivateProfileString (g_szPatterns,szLHS,NULL,
                                                 g_szControlIni);

                      SendMessage( hDlg, WM_NEXTDLGCTL, (WPARAM)hWndCombo,
                        TRUE );
                      EnableWindow (GetDlgItem (hDlg,IDD_DELPATTERN),FALSE);
                   }
                }
              break;

            case IDCANCEL:
            {
              int action = CheckSaveChanges( hDlg );

              if( action == IDCANCEL )
                break;

              hWndCombo = GetDlgItem(hDlg, IDD_PATTERNCOMBO);

              /* Save current selection in parent combobox */

              nChanged = (int)SendMessage (g_hWndListSrc,LB_GETCURSEL,0,0L);
              SendMessage (g_hWndListSrc,LB_GETTEXT,nChanged,
                           (LPARAM) szLHS);

              /* Now rebuild parent combobox */

              SendMessage(g_hWndListSrc, LB_RESETCONTENT, 0, 0L);
              for (nPats = (WORD) SendMessage (hWndCombo,CB_GETCOUNT,
                                               0,0L) - 1;nPats >= 0; 
                   nPats--) {
                 SendMessage(hWndCombo,CB_GETLBTEXT,nPats,
                             (LPARAM)szPats);
                 SendMessage(g_hWndListSrc, LB_ADDSTRING,0,
                             (LPARAM)szPats);
              }
              SendMessage(g_hWndListSrc, LB_INSERTSTRING,0,
                          (LPARAM)g_szNone);

              if (action == IDYES)
              {
                 nPats = (WORD)SendMessage(hWndCombo, CB_GETCURSEL, (WPARAM)NULL, 0L);
                 SendMessage(g_hWndListSrc, LB_SETCURSEL, ++nPats, 0L);
              }
              else
              {
                 nChanged = (int)SendMessage (g_hWndListSrc,LB_FINDSTRING,
                                         (WPARAM)-1, (LPARAM) szLHS);
                 if (nChanged > 0)
                    SendMessage(g_hWndListSrc, LB_SETCURSEL,nChanged, 0L);
                 else
                    SendMessage(g_hWndListSrc, LB_SETCURSEL,0, 0L);
              }

              EndDialog( hDlg, ( action == IDYES ) );
              break;
            }

            default:
              return FALSE;
          }
        break;

      default:
        return FALSE;
    }
  return(TRUE);
}
