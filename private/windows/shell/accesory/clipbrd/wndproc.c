#include <windows.h>
#include <shellapi.h>
#include "clipbrd.h"



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitOwnerScrollInfo() -                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/
void InitOwnerScrollInfo(
void)
{
OwnVerPos = OwnHorPos = OwnVerMin = OwnHorMin = 0;
OwnVerMax = VPOSLAST;
OwnHorMax = HPOSLAST;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ClipbrdWndProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LONG FAR PASCAL ClipbrdWndProc(register HWND hwnd,
                                UINT message,register WPARAM wParam,LONG lParam)

{
  int           i;
  HDC           hdc;
  UINT          wNewFormat;
  UINT          wOldFormat;
  HPALETTE      hpal;
  HPALETTE      hpalT;

  static INT UpdateCount = 0;

  switch (message)
    {
      case WM_RENDERALLFORMATS:
          /* When clipboard is cleared by user using EDIT-CLEAR command
           * we (clipboard viewer) become the owner and this results in
           * WM_RENDERALLFORMATS message when Clipbrd viewer is closed.
           * So, we should check if we have anything to render before
           * processing this message.  Sankar
           */
          if (!CountClipboardFormats())
              break;

          /* Check if the clipbrd viewer has done any File I/O before.
           * If it has not, then it has nothing to render!  Sankar
           */
          if (!fAnythingToRender)
              break;

          /* Empty the clipboard */
          if (!MyOpenClipboard(hwnd))
                  break;
          EmptyClipboard();
          /*** FALL THRU ***/

      case WM_RENDERFORMAT:
        {
          INT           fh;
          DWORD         HeaderPos;
          FILEHEADER    FileHeader;
          FORMATHEADER  FormatHeader;
          WORD          w;

          fh = (INT)CreateFile((LPCTSTR)szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
          FileHeader.FormatCount = 0; /* If _lread fails, we can detect */
          _lread(fh, (LPBYTE)&FileHeader, sizeof(FILEHEADER));

          HeaderPos = sizeof(FILEHEADER);

          for (w=0; w < FileHeader.FormatCount; w++)
            {
              _llseek(fh, HeaderPos, 0);
              if (fNTReadFileFormat) {
                  if(_lread(fh, (LPBYTE)&(FormatHeader.FormatID), sizeof(FormatHeader.FormatID)) < sizeof(FormatHeader.FormatID))
                      break;
              } else {
                  FormatHeader.FormatID = 0;  /* initialize the hight WORD */
                  if(_lread(fh, (LPBYTE)&(FormatHeader.FormatID), sizeof(WORD)) < sizeof(WORD))
                      break;
              }
              if(_lread(fh, (LPBYTE)&(FormatHeader.DataLen), sizeof(FormatHeader.DataLen)) < sizeof(FormatHeader.DataLen))
                  break;
              if(_lread(fh, (LPBYTE)&(FormatHeader.DataOffset), sizeof(FormatHeader.DataOffset)) < sizeof(FormatHeader.DataOffset))
                  break;
              if(_lread(fh, (LPBYTE)&(FormatHeader.Name), sizeof(FormatHeader.Name)) < sizeof(FormatHeader.Name))
                  break;
              if (fNTReadFileFormat)
                  HeaderPos += (sizeof(UINT) + 2*sizeof(DWORD) + CCHFMTNAMEMAX*sizeof(TCHAR));
              else
                  HeaderPos += (sizeof(WORD) + 2*sizeof(DWORD) + CCHFMTNAMEMAX*sizeof(TCHAR));

              if (PRIVATE_FORMAT(FormatHeader.FormatID))
                  FormatHeader.FormatID = (UINT)RegisterClipboardFormat(FormatHeader.Name);

              if ((message == WM_RENDERALLFORMATS) || (FormatHeader.FormatID == (UINT)wParam))
                  RenderFormat(&FormatHeader, fh);
            }

          if (message == WM_RENDERALLFORMATS)
              CloseClipboard();

          _lclose(fh);
          break;
        }

      case WM_DESTROYCLIPBOARD:
          /* Prevent unnecessary file I/O when getting a WM_RENDERALLFORMATS */
          fAnythingToRender = FALSE;
          break;

      case WM_CREATE:
          hMainMenu = GetMenu(hwnd);
            /* Get the handle to the Display popup menu */
          hDispMenu = GetSubMenu(hMainMenu, 2);
          UpdateCBMenu(hwnd);
          break;

      case WM_COMMAND:
          switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
              case CBM_CLEAR:
                  ClearClipboard(hwnd);
                  break;

              case CBM_EXIT:
                  SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                  break;

              case CBM_ABOUT:
                  if(ShellAbout(hwnd, szCaptionName, (LPTSTR)TEXT(""),
                             LoadIcon(hInst, MAKEINTRESOURCE(CBICON))) == -1)
                        MemErrorMessage();
                  break;

              case CBM_OPEN:
                  OpenClipboardFile(hwnd);
                  break;

              case CBM_SAVEAS:
                  SaveClipboardToFile(hwnd);
                  break;

              case CBM_AUTO:
              case CF_PALETTE:
              case CF_TEXT:
              case CF_BITMAP:
              case CF_METAFILEPICT:
              case CF_SYLK:
              case CF_DIF:
              case CF_TIFF:
              case CF_OEMTEXT:
              case CF_DIB:
              case CF_OWNERDISPLAY:
              case CF_DSPTEXT:
              case CF_DSPBITMAP:
              case CF_DSPMETAFILEPICT:
              case CF_PENDATA:
              case CF_UNICODETEXT:
              case CF_RIFF:
              case CF_WAVE:
              case CF_ENHMETAFILE:
              case CF_DSPENHMETAFILE:
                  if (CurSelFormat == GET_WM_COMMAND_ID(wParam, lParam))
                      break;

                  CheckMenuItem(hDispMenu, CurSelFormat, MF_BYCOMMAND | MF_UNCHECKED);
                  CheckMenuItem(hDispMenu, GET_WM_COMMAND_ID(wParam, lParam), MF_BYCOMMAND | MF_CHECKED);
                  DrawMenuBar(hwnd);

                  wOldFormat = GetBestFormat(CurSelFormat);
                  wNewFormat = GetBestFormat(GET_WM_COMMAND_ID(wParam, lParam));
                  if (wOldFormat == wNewFormat)
                    {
                       /* An equivalent format is selected; No change */
                       CurSelFormat = GET_WM_COMMAND_ID(wParam, lParam);
                       break;
                    }

                  /* A different format is selected; So, refresh... */

                  /* Change the character sizes based on new format. */
                  ChangeCharDimensions(hwnd, wOldFormat, wNewFormat);

                  fDisplayFormatChanged = TRUE;

                  CurSelFormat = GET_WM_COMMAND_ID(wParam, lParam);
                  if (wOldFormat == CF_OWNERDISPLAY)
                    {
                      /* Save the owner Display Scroll info */
                      SaveOwnerScrollInfo(hwnd);
                      goto InvalidateScroll1;
                    }

                  if (wNewFormat == CF_OWNERDISPLAY)
                    {
                      /* Restore the owner display scroll info */
                      RestoreOwnerScrollInfo(hwnd);
                      InvalidateRect(hwnd,NULL,TRUE);
                    }
                  else
                      goto InvalidateScroll1;
                  break;

              case CBM_USEHELP:
                  if(!WinHelp(hwnd, (LPTSTR)NULL, HELP_HELPONHELP, 0L))
                        MemErrorMessage();
                  break;

              case CBM_HELP:
                  if(!WinHelp(hwnd, (LPTSTR)szHelpFileName, HELP_INDEX, 0L))
                        MemErrorMessage();
                  break;

              case CBM_SEARCH:
                  if(!WinHelp(hwnd, (LPTSTR)szHelpFileName, HELP_PARTIALKEY, (DWORD)(LPTSTR)TEXT("")))                /*        Anas May 92 should I?        */
                               MemErrorMessage();
                  break;

              default:
                  return(DefWindowProc(hwnd, message, wParam, lParam));
            }
          break;

      case WM_CHANGECBCHAIN:
          /* Some window is being removed from the clipboard viewer chain. */

          if (hwndNextViewer == NULL)
              return(FALSE);

          if ((HWND)wParam == hwndNextViewer)
            {
              /* Chain link being removed is our descendant */
              hwndNextViewer = GET_WM_CHANGECBCHAIN_HWNDNEXT(wParam, lParam);
              return(TRUE);
            }
          return(SendMessage(hwndNextViewer, WM_CHANGECBCHAIN, wParam, lParam));

      case WM_KEYDOWN:
        {
          WPARAM                sb;

          switch (wParam)
            {
              case VK_UP:
                  sb = SB_LINEUP;
                  goto VertScroll;

              case VK_DOWN:
                  sb = SB_LINEDOWN;
                  goto VertScroll;

              case VK_PRIOR:
                  sb = SB_PAGEUP;
                  goto VertScroll;

              case VK_NEXT:
                  sb = SB_PAGEDOWN;
VertScroll:
                  SendMessage(hwnd, WM_VSCROLL, sb, 0L);
                  break;

              case VK_LEFT:
                  sb = SB_LINEUP;
                  goto HorzScroll;

              case VK_RIGHT:
                  sb = SB_LINEDOWN;
                  goto HorzScroll;

              case VK_TAB:
                  sb = (GetKeyState( VK_SHIFT ) < 0) ? SB_PAGEUP : SB_PAGEDOWN;
HorzScroll:
                  SendMessage( hwnd, WM_HSCROLL, sb, 0L);
                  break;

              default:
                  goto DefaultProc;
            }
          break;
        }

      case WM_SIZE:
          fDisplayFormatChanged = TRUE;
          if (wParam == SIZEICONIC)
            {
              if (fOwnerDisplay)
                  SendOwnerSizeMessage(hwnd, 0, 0, 0, 0);
            }
          else
            {
              /* Invalidate scroll offsets since they are dependent on
               * window size UNLESS it is a Owner display item.  Also the
               * "object size" of CF_TEXT changes when the window width
               * changes.
               */
              if (fOwnerDisplay)
                  SendOwnerSizeMessage(hwnd, 0, 0, LOWORD(lParam), HIWORD(lParam));
              else
                  goto InvalidateScroll;
            }
          break;

      case WM_DESTROY:
          /* Take us out of the viewer chain */
          ChangeClipboardChain(hwnd, hwndNextViewer);

          if (fOwnerDisplay)
              SendOwnerSizeMessage(hwnd, 0, 0, 0, 0);

          DeleteObject(hbrBackground);

          WinHelp(hwnd, (LPTSTR)szHelpFileName, HELP_QUIT, 0L);

          PostQuitMessage(0);
          break;

      case WM_DRAWCLIPBOARD:
          fDisplayFormatChanged = TRUE;

          /* Pass the message on to the next clipboard viewer in the chain. */
          if (hwndNextViewer != NULL)
              SendMessage(hwndNextViewer, WM_DRAWCLIPBOARD, wParam, lParam);

          wOldFormat = GetBestFormat(CurSelFormat);

          /* Update the popup menu entries. */
          UpdateCBMenu(hwnd);
          wNewFormat = GetBestFormat(CurSelFormat);

          /* Change the character dimensions based on the format. */
          ChangeCharDimensions(hwnd, wOldFormat, wNewFormat);

          /* Initialize the owner display scroll info, because the
           * contents have changed.
           */
          InitOwnerScrollInfo();

InvalidateScroll1:
          /* Force a total repaint. fOwnerDisplay gets updated during
           * a total repaint.
           */
          InvalidateRect(hwnd,NULL,TRUE);

InvalidateScroll:
          /* Invalidate object info; reset scroll position to 0. */
          cyScrollLast = cxScrollLast = -1;

          /* Range is set in case CF_OWNERDISPLAY owner changed it. */
          SetScrollRange(hwnd, SB_VERT, 0, VPOSLAST, FALSE);
          SetScrollPos(hwnd, SB_VERT, (INT)(cyScrollNow = 0), TRUE);
          SetScrollRange(hwnd, SB_HORZ, 0, HPOSLAST, FALSE);
          SetScrollPos(hwnd, SB_HORZ, cxScrollNow = 0, TRUE);
          break;

      case WM_QUERYNEWPALETTE:
          /* If palette realization caused a palette change, do a full redraw. */
          if (!MyOpenClipboard(hwnd))
              return(FALSE);

          if (IsClipboardFormatAvailable(CF_PALETTE))
              hpal = GetClipboardData(CF_PALETTE);
          else
              hpal = NULL;

          CloseClipboard();

          if (hpal)
            {
              hdc = GetDC(hwnd);
              hpalT = SelectPalette(hdc, hpal, FALSE);

              i = RealizePalette(hdc);

              SelectPalette (hdc, hpalT, FALSE);
              ReleaseDC(hwnd, hdc);

              if (i)
                {
                  InvalidateRect(hwnd, NULL, TRUE);
                  UpdateCount = 0;
                  return TRUE;
                }
            }
          return(FALSE);

      case WM_PALETTECHANGED:
          if ((HWND)wParam != hwnd && IsClipboardFormatAvailable(CF_PALETTE))
            {
              if (!MyOpenClipboard(hwnd))
                  return(FALSE);

              if (IsClipboardFormatAvailable(CF_PALETTE))
                  hpal = GetClipboardData(CF_PALETTE);
              else
                  hpal = NULL;

              CloseClipboard();

              if (hpal)
                {
                  hdc = GetDC(hwnd);
                  hpalT = SelectPalette (hdc, hpal, FALSE);

                  if (RealizePalette(hdc))
                    {
#ifdef ORG_CODE
                      UpdateColors(hdc);
                      UpdateCount++;
#else
                      InvalidateRect(hwnd, NULL, TRUE);
                      UpdateCount = 0;
#endif
                    }

                  SelectPalette (hdc, hpalT, 0);
                  ReleaseDC(hwnd, hdc);
                }
            }
          break;

      case WM_PAINT:
        {
          PAINTSTRUCT   ps;

          /* If we have updated more than once, the rest of our
           * window is not in some level of degradation worse than
           * our redraw...  We need to redraw the whole area.
           */
          if (UpdateCount > 1)
            {
              UpdateCount = 0;
              InvalidateRect(hwnd, NULL, TRUE);
            }

          BeginPaint(hwnd, &ps);
          hdc = ps.hdc;
          if (MyOpenClipboard(hwnd))
            {
              SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
              SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

              /* Check if a palette is also available in the clipboard.
               * If so, select it before drawing data.
               */
              if (IsClipboardFormatAvailable(CF_PALETTE)) {
                if ((hpal = GetClipboardData(CF_PALETTE)) != NULL)
                {
                  hpalT = SelectPalette(hdc, hpal, FALSE);
                  RealizePalette(hdc);
                }
              } else
                hpal = NULL;

              DrawStuff(hwnd, &ps);

              if (hpal)
                  SelectPalette(hdc, hpalT, FALSE);

              CloseClipboard();
            }
          EndPaint(hwnd, &ps);
          break;
        }

      case WM_VSCROLL:
          if (GET_WM_VSCROLL_CODE(wParam, lParam) != SB_THUMBTRACK)
            {
              if (fOwnerDisplay)
                  SendOwnerMessage(WM_VSCROLLCLIPBOARD, (WPARAM)hwnd,
                           (LPARAM)MAKELONG(GET_WM_VSCROLL_CODE(wParam, lParam),
                                    GET_WM_VSCROLL_POS(wParam, lParam)));
              else
                  ClipbrdVScroll(hwnd, GET_WM_VSCROLL_CODE(wParam, lParam),
                                 GET_WM_VSCROLL_POS(wParam, lParam));
            }
          break;

      case WM_HSCROLL:
          if (GET_WM_HSCROLL_CODE(wParam, lParam) != SB_THUMBTRACK)
            {
              if (fOwnerDisplay)
                  SendOwnerMessage(WM_HSCROLLCLIPBOARD, (WPARAM)hwnd,
                           (LPARAM)MAKELONG(GET_WM_HSCROLL_CODE(wParam, lParam),
                                    GET_WM_HSCROLL_POS(wParam, lParam)));
              else
                  ClipbrdHScroll(hwnd, GET_WM_HSCROLL_CODE(wParam, lParam),
                                 GET_WM_HSCROLL_POS(wParam, lParam));
            }
          break;

      case WM_SYSCOLORCHANGE:
          /* Update pen and brush to reflect new color */
          DeleteObject(hbrBackground);
          hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
          break;

      default:
DefaultProc:
          return(DefWindowProc(hwnd, message, wParam, lParam));
    }
  return(0L);
}
