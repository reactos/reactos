/****************************************************************************/
/*                                                                          */
/*  Windows Reversi -                                                       */
/*                                                                          */
/*      Originally written by Chris Peters                                  */
/*                                                                          */
/****************************************************************************/

#include "windows.h"
#include <port1632.h>
#include <process.h>
#include <stdlib.h>
#include "reversi.h"

/* Exported procedures called from other modules */
LRESULT APIENTRY ReversiWndProc(HWND, UINT, WPARAM, LPARAM);
VOID APIENTRY InverseMessage(HWND, UINT, UINT_PTR, DWORD);
INT_PTR APIENTRY AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

PSTR    pDisplayMessage;
HBRUSH  hbrBlack;
HBRUSH  hbrPat;
HBRUSH  hbrWhite;
HBRUSH  hbrRed;
HBRUSH  hbrGreen;
HBRUSH  hbrBlue;
HBRUSH  hbrHuman;
HBRUSH  hbrComputer;


HANDLE  hInst;
HANDLE  curIllegal;
HANDLE  curLegal;
HANDLE  curThink;
HANDLE  curBlank;
BOOL    fThinking = FALSE;
BOOL    fCheated = FALSE;
INT     direc[9] = {9, 10, 11, 1, -1, -9, -10, -11, 0};
WORD     prevCheck;
BYTE    board[max_depth+2][BoardSize];
INT     fPass;
INT     flashtimes;
INT     count;
INT     MessageOn;
INT     charheight;
INT     charwidth;
INT     xscr;
CHAR    strBuf[80];
BOOL    bMouseDownInReversi = FALSE;
INT     xExt;
INT     yExt;
INT     Bx;
INT     By;
INT     ASPECT;
INT     COLOR;
INT     TXMIN;
INT     TYMIN = 45;
INT     dimension;
BOOL    ffirstmove;

CHAR    szReversi[20];
CHAR    szReversiPractice[40];
CHAR    szPass[30];
CHAR    szMustPass[30];
CHAR    szTie[30];
CHAR    szLoss[30];
CHAR    szWon[30];
CHAR    szWonPost[30];
CHAR    szLossPost[30];
CHAR    szAbout[20];
CHAR    szIllegal[70];
CHAR    szNoPass[70];
CHAR    szHelpFile[15];

HANDLE  hAccel;

POINT   MousePos;

INT     depth;
INT     BestMove[max_depth+2];
HDC     hDisp;
HWND    hWin;
INT     moves[61] = {11,18,81,88, 13,31,16,61,
                     38,83,68,86, 14,41,15,51,
                     48,84,58,85, 33,36,63,66,
                     34,35,43,46, 53,56,64,65,
                     24,25,42,47, 52,57,74,75,
                     23,26,32,37, 62,67,73,76,
                     12,17,21,28, 71,78,82,87,
                     22,27,72,77,
              0};


INT NEAR PASCAL minmax(BYTE b[max_depth + 2][100], INT move, INT friendly,
    INT enemy, INT ply, INT vmin, INT vmax);
VOID NEAR PASCAL makemove(BYTE b[], INT move, INT friendly, INT enemy);
INT NEAR PASCAL legalcheck(BYTE b[], INT move, INT friendly, INT enemy);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  UpdateCursor() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* To use UpdateCursor,  set the global var MousePos.x and MousePos.y and make
 * the call.  The cursor will appear at the new position
 */

VOID NEAR PASCAL UpdateCursor(
HWND    hwnd)
{
  POINT curpoint;

  curpoint.x = MousePos.x;
  curpoint.y = MousePos.y;
  ClientToScreen(hwnd, (LPPOINT)&curpoint);
  SetCursorPos(curpoint.x, curpoint.y);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  checkdepth() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL checkdepth(
HWND hWindow,
WORD  d)
{
  HMENU hMenu;

  hMenu = GetMenu(hWindow);
  CheckMenuItem(hMenu, prevCheck, MF_UNCHECKED);
  CheckMenuItem(hMenu, d, MF_CHECKED);
  prevCheck = d;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  clearboard() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL clearboard(
BYTE b[max_depth+2][BoardSize])
{
  register INT  i,j;
  INT           k;

  for (i=0; i<=max_depth ; i++)
      for (j=0 ; j<=99 ; j++)
          b[i][j] = edge;

    for (i=0 ; i<=max_depth ; i++)
      {
        for (j=11 ; j<=81 ; j=j+10)
            for (k=j ; k<j+8 ; k++)
                b[i][k] = empty;

        b[i][45]=computer;
        b[i][54]=computer;
        b[i][44]=human;
        b[i][55]=human;
      }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevCreate() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Called on WM_CREATE messages. */

VOID NEAR PASCAL RevCreate(
register HWND   hWindow)

{
  register HDC  hDC;
  TEXTMETRIC    charsize;           /* characteristics of the characters */

  MessageOn   = FALSE;
  hDC = GetDC(hWindow);
  GetTextMetrics(hDC, (LPTEXTMETRIC)&charsize);

  charheight = charsize.tmHeight;
  charwidth = charsize.tmAveCharWidth;

  ReleaseDC(hWindow, hDC);

  if (COLOR == TRUE)
    {
      hbrComputer = hbrBlue;
      hbrHuman = hbrRed;
    }
  else
    {
      hbrComputer = hbrBlack;
      hbrHuman = hbrWhite;
    }

  TXMIN = 45 * ASPECT;

  clearboard(board);

  /* Okay to pass on first move */
  fPass = PASS;
  depth = 1;
  prevCheck = EASY;
  ffirstmove = TRUE;
  checkdepth(hWindow, prevCheck);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  printboard() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL printboard(
BYTE b[max_depth+2][BoardSize])

{
  register INT  i,j;
  INT sq;

  for (i=0; i < 8; i++)
    {
      for (j=0; j < 8; j++)
        {
          if ((sq = (INT)b[0][i*10+j+11]) != empty)
            {
              if (sq == computer)
                  SelectObject(hDisp, hbrComputer);
              else
                  SelectObject(hDisp, hbrHuman);

              Ellipse(hDisp,
                      Bx+2*ASPECT+i*xExt,
                      By+2+j*yExt,
                      Bx+2*ASPECT + i*xExt + xExt-4*ASPECT,
                      By+2+j*yExt + yExt-4);
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ClearMessageTop() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL ClearMessageTop(
HDC     hDC)

{
  if (MessageOn == TRUE)
    {
      flashtimes = count + 1;
      SelectObject(hDC, (COLOR) ? hbrGreen : hbrWhite);
      PatBlt(hDC, 0, 0, xscr, charheight, PATCOPY);
      MessageOn = FALSE;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ShowMessageTop() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL ShowMessageTop(
HDC     hDC,
PSTR    string)

{
  INT   dx;

  pDisplayMessage = string;
  ClearMessageTop(hDC);
  SelectObject(hDC, hbrWhite);
  PatBlt(hDC, 0, 0, xscr, charheight, PATCOPY);
  SetBkMode(hDC, TRANSPARENT);
  MGetTextExtent(hDC, (LPSTR)string, lstrlen(string), &dx, NULL);
  TextOut(hDC, (INT)(xscr - dx)/2, 0, (LPSTR)string, lstrlen(string));
  MessageOn = TRUE;
  SetBkMode(hDC, OPAQUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  drawboard() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL drawboard(
BYTE b[max_depth+2][BoardSize])
{
  register INT  i;
  INT           lcx,lcy;
  register INT  xdimension;
  INT           xLineExt,yLineExt;

  yLineExt = 8 * yExt;
  xLineExt = 8 * xExt;
  xdimension = dimension * ASPECT;

  SelectObject(hDisp, hbrBlack);
  PatBlt(hDisp, Bx+2*xdimension, By+2*dimension, xLineExt, yLineExt, PATCOPY);
  MUnrealizeObject(hbrPat);
  SelectObject(hDisp, hbrPat);
  PatBlt(hDisp, Bx, By, xLineExt, yLineExt, PATCOPY);

  SelectObject(hDisp, hbrBlack);
  for (i=Bx; i <= Bx + xLineExt; i += xExt)
      PatBlt(hDisp, i, By, ASPECT, yLineExt, PATCOPY);

  for (i=By; i <= By + yLineExt; i += yExt)
      PatBlt(hDisp, Bx, i, xLineExt, 1, PATCOPY);

  lcx = Bx+xLineExt;
  lcy = By+yLineExt;

  MUnrealizeObject(hbrPat);
  SelectObject(hDisp, hbrPat);

  for (i=1; i < xdimension; ++i)
      PatBlt(hDisp, lcx+i, By+i/ASPECT, 1, yLineExt, PATCOPY);

  /* Fill in bottom edge of puzzle. */
  for (i=1; i < dimension; ++i)
      PatBlt(hDisp, Bx+i*ASPECT, lcy+i, xLineExt, 1, PATCOPY);

  SelectObject(hDisp, hbrBlack);

  MMoveTo(hDisp, lcx, By);
  LineTo(hDisp, lcx+xdimension, By+dimension);
  LineTo(hDisp, lcx+xdimension, lcy+dimension);
  LineTo(hDisp, Bx+xdimension, lcy+dimension);
  LineTo(hDisp, Bx, lcy);
  MMoveTo(hDisp, lcx+xdimension, lcy+dimension);
  LineTo(hDisp, lcx, lcy);

  printboard(b);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevPaint() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Called on WM_PAINT messages. */

VOID NEAR PASCAL RevPaint(
HWND    hWindow,
HDC     hDC)

{
  register INT  Tx, Ty;
  INT           xLineExt, yLineExt;
  RECT          lpSize;

  /* Since it is easy to resize we'll do it on every repaint */
  hDisp = hDC;
  hWin  = hWindow;
  SetBkMode(hDisp, OPAQUE);
  GetClientRect(hWindow, (LPRECT)&lpSize);
  xscr = Tx = lpSize.right - lpSize.left;
  Ty = lpSize.bottom - lpSize.top;

  /* Dont go below minimum size */
  if (Tx < Ty*ASPECT)
    {
      if (Tx < TXMIN)
          Tx = TXMIN;
      xExt = Tx / (9 + 1);
      yExt = xExt / ASPECT;
    }
  else
    {
      if (Ty < TYMIN)
          Ty = TYMIN;
      yExt = Ty / (9 + 1);
      xExt = yExt * ASPECT;
    }
  yLineExt = 8 * yExt;
  xLineExt = 8 * xExt;
  dimension = yLineExt/30;

  Bx = (Tx > xLineExt) ? (Tx - xLineExt) / 2 : 0;
  By = (Ty > yLineExt) ? (Ty - yLineExt) / 2 : 0;

  drawboard(board);

  if (MessageOn)
    {
      ShowMessageTop(hDisp, pDisplayMessage);
      PatBlt(hDC, 0, 0, xscr, charheight, DSTINVERT);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FlashMessageTop() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL FlashMessageTop(
HWND    hWindow)
{
  flashtimes = 0;
  count = 4;
  SetTimer(hWindow, 666, 200, InverseMessage);    /* Timer ID is 666 */
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevMessage() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL RevMessage(
HWND            hWindow,
HDC             hDC,
register CHAR   *pS,
INT             n,
CHAR            *pchPostStr)
{
  register CHAR *pch;

  pch = strBuf;
  while (*pS)
      *pch++ = *pS++;

  if (n)
    {
      if (n / 10)
          *pch++ = (CHAR)(n / 10 + '0');
      *pch++ = (CHAR)(n % 10 + '0');
    }

  if (pchPostStr)
    {
      while (*pchPostStr)
          *pch++ = *pchPostStr++;
    }
  *pch = TEXT('\0');

  ShowMessageTop(hDC, strBuf);
  FlashMessageTop(hWindow);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  flashsqr() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL flashsqr(
register HDC    hDC,
INT             x1,
INT             y1,
INT             Ex,
INT             Ey,
INT             color,
BOOL            fBlankSquare,
INT             n)

{
  register INT  i;

  if (fBlankSquare)
      SelectObject(hDC, GetStockObject(NULL_PEN));

  SetCursor(curBlank);

  for (i=0; i < n; ++i)
    {
      if (color == 1)
          color = 2;
      else
          color = 1;

      if (color == 1)
          SelectObject(hDC,hbrComputer);
      else
          SelectObject(hDC, hbrHuman);

      SetBkMode(hDC, OPAQUE);
      Ellipse(hDC, x1, y1, x1+Ex, y1+Ey);
    }

  if (fBlankSquare)
    {
      MUnrealizeObject(hbrPat);
      SelectObject(hDC, hbrPat);
      Ellipse(hDC, x1, y1, x1+Ex, y1+Ey);
    }
  else
      SetCursor(curThink);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevMouseMove() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL RevMouseMove(
POINT   point)

{
  INT     move;
  INT     Si, Sj;
  INT     yLineExt = 8 * yExt;
  INT     xLineExt = 8 * xExt;
  HANDLE  cur;

  MousePos.x = point.x;
  MousePos.y = point.y;

  if(xExt ==0 || yExt == 0)
      return;

  cur = curIllegal;

  if ((point.x > Bx) && (point.x < (Bx+xLineExt)) && (point.y > By) && (point.y < (By+yLineExt)))
    {
      Si = (point.x - Bx) / xExt;
      Sj = (point.y - By) / yExt;
      move = Si * 10 + Sj + 11;
      if (legalcheck(board[0], move, human, computer))
          cur = curLegal;
    }
  SetCursor(cur);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ShowBestMove() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL ShowBestMove(
HWND hwnd)

{
  HDC           hdc;
  INT           sq;
  register INT  x, y;
  INT           *pMoves;
  BOOL          bDone;

  if (fPass == PASS && !ffirstmove)
      return;

  if (!fCheated)
      SetWindowText(hwnd, (LPSTR)szReversiPractice);

  fCheated = TRUE;
  SetCursor(curThink);
  fThinking = TRUE;

  if (ffirstmove)
    {
      /* HACK: Hardcode the first move hint. */
      x = 4;
      y = 2;
    }
  else
    {
      if (depth == 1)
        {
          bDone = FALSE;
          pMoves = moves;
          sq = *pMoves;
          while (!bDone)
            {
              sq = *pMoves;
              if (legalcheck(board[0], sq, human, computer))
                  bDone = TRUE;
              else
                  pMoves++;
            }
          y = (sq - 11) % 10;
          x = (sq - 11) / 10;
        }
      else
        {
          minmax(board, BestMove[0],  computer, human, 1, -infin, infin);
          y = (BestMove[1] - 11) % 10;
          x = (BestMove[1] - 11) / 10;
        }
    }

  MousePos.x = (x * xExt) + Bx + xExt/2;
  MousePos.y = (y * yExt) + By + yExt/2;

  UpdateCursor(hwnd);

  hdc = GetDC(hwnd);

  x = x * xExt + Bx + 2 * ASPECT;
  y = y * yExt + By + 2;

  flashsqr(hdc, x, y, xExt - 4 * ASPECT, yExt - 4, computer, TRUE, 3);

  fThinking = FALSE;

  ReleaseDC(hwnd, hdc);

  RevMouseMove(MousePos);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  gameover() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Find a human reply to the computers move.
 * As a side effect set flag fPass if the human
 * has a legal move.
 */

VOID NEAR PASCAL gameover(
register HWND   hWindow,
HDC             hDC,
BYTE            b[max_depth + 2][BoardSize],
INT             r)

{
  register INT  i;
  INT           cc;
  INT           hc;
  INT           sq;
  INT           reply2;
  INT           *pMoves;

  pMoves = moves;
  fPass = PASS;
  reply2 = PASS;
  while ((sq = *pMoves++) != 0)
    {
      if (legalcheck(b[0], sq, human, computer))
          fPass = sq;
      else if (legalcheck(b[0], sq, computer, human))
          reply2 = sq;
    }

  if (fPass == PASS)
    {
      if ((r == PASS) || (reply2 == PASS))
        {
          hc = 0;
          cc = 0;
          for (i=11; i <= 88; i++)
            {
              if (b[0][i] == human)
                  hc++;
              else if (b[0][i] == computer)
                  cc++;
            }

          if (hc > cc)
              RevMessage(hWindow, hDC, szWon, hc-cc, szWonPost);
          else if (hc < cc)
              RevMessage(hWindow, hDC, szLoss, cc-hc, szLossPost);
          else
              RevMessage(hWindow, hDC, szTie, 0, NULL);
        }
      else
        {
          RevMessage(hWindow, hDC, szMustPass, 0, NULL);
        }
    }
  else if (r == PASS)
    {
      RevMessage(hWindow, hDC, szPass, 0, NULL);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  paintmove() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Make a move and show the results. */

VOID NEAR PASCAL paintmove(
BYTE    b[BoardSize],
INT     move,
BYTE    friendly,
BYTE    enemy)
{
  INT           d;
  INT           sq;
  INT           *p;
  register INT  i,j;
  INT           color;

  if (move != PASS)
    {
      if (friendly == computer)
        {
          SelectObject(hDisp, hbrComputer);
          color = 1;
        }
      else
        {
          SelectObject(hDisp, hbrHuman);
          color = 2;
        }

      i = ((move - 11) / 10) * xExt + Bx + 2 * ASPECT;
      j = ((move - 11) % 10) * yExt + By + 2;
      Ellipse(hDisp, i, j, i + xExt - 4 * ASPECT, j + yExt - 4);
      flashsqr(hDisp, i, j, xExt - 4 * ASPECT, yExt - 4, color, FALSE, 4);

      p = direc;
      while ((d = *p++) != 0)
        {
          sq=move;
          if (b[sq += d] == enemy)
           {
             while(b[sq += d] == enemy)
                ;
             if (b[sq] == (BYTE)friendly)
               {
                 while(b[sq -= d] == enemy)
                   {
                     board[0][sq] = b[sq] = friendly;
                     i = ((sq - 11)/10)*xExt+Bx+2*ASPECT;
                     j = ((sq - 11)%10)*yExt+By+2;
                     Ellipse(hDisp, i, j, i + xExt-4*ASPECT, j + yExt-4);
                   }
               }
           }
        }
      b[move]=friendly;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevMenu() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Called on WM_COMMAND messages. */

VOID NEAR PASCAL RevMenu(
register HWND   hWindow,
INT             idval)

{
  HDC           hDC;
  register INT  cmd;

  if (fThinking)
      return;

  hWin = hWindow;

  switch (idval)
    {
      case EXIT:
          PostMessage(hWindow, WM_CLOSE, 0, 0L);
          break;

      case MN_HELP_ABOUT:
          DialogBox(hInst, MAKEINTRESOURCE(3), hWindow, AboutDlgProc);
          break;

      case MN_HELP_INDEX:
          //TEMPFIX WinHelp(hWindow, (LPSTR)szHelpFile, HELP_INDEX, 0L);
          break;

      case MN_HELP_USINGHELP:
          //TEMPFIX WinHelp(hWindow, (LPSTR)NULL, HELP_HELPONHELP, 0L);
          break;

      case MN_HELP_KEYBOARD:
          cmd = 0x1e;
          goto HelpCommon;

      case MN_HELP_COMMANDS:
          cmd = 0x20;
          goto HelpCommon;

      case MN_HELP_PLAYING:
          cmd = 0x21;
          goto HelpCommon;

      case MN_HELP_RULES:
          cmd = 0x22;
HelpCommon:
          //TEMPFIX WinHelp(hWindow, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)cmd);
          break;

      case HINT:
          ShowBestMove(hWindow);
          return;
          break;

      case NEW:
          SetWindowText(hWindow , (LPSTR)szReversi);
          ffirstmove = TRUE;
          hDisp = hDC = GetDC(hWindow);
          fCheated = FALSE;
          SetBkMode(hDisp, OPAQUE);
          ClearMessageTop(hDC);
          fPass = PASS;
          clearboard(board);
          drawboard(board);
          ReleaseDC(hWindow, hDC);
          hDisp = 0;
          break;

      case EASY:
          depth = 1;                      /* MUST BE AT LEAST 1.  */
          checkdepth(hWindow, EASY);      /* KEEP HANDS OFF!      */
          break;

      case MEDIUM:
          depth = 2;
          checkdepth(hWindow, MEDIUM);
          break;

      case HARD:
          depth = 4;
          checkdepth(hWindow, HARD);
          break;

      case VHARD:
          depth = 6;
          checkdepth(hWindow, VHARD);
          break;

      case PASS:
          if (fPass == PASS)
            {
              hDisp = hDC = GetDC(hWindow);
              SetBkMode(hDisp, OPAQUE);
              fThinking = TRUE;
              ClearMessageTop(hDC);
              SetCursor(curThink);
              ReleaseDC(hWindow, hDC);
              hDisp = 0;
              minmax(board, PASS, human, computer, 0, -infin, infin);
              hDisp = hDC = GetDC(hWindow);
              paintmove(board[0], BestMove[0], (BYTE)computer, (BYTE)human);
              gameover(hWindow, hDC, board, BestMove[0]);
              SetCursor(curIllegal);
              fThinking = FALSE;
              ReleaseDC(hWindow, hDC);
              hDisp = 0;
            }
          else
              MessageBox(hWindow, (LPSTR)szNoPass, (LPSTR)szReversi, MB_OK | MB_ICONASTERISK);
          break;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  msgcheck() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Called by ASM routine to allow other tasks to run. */

BOOL NEAR PASCAL msgcheck()
{
  MSG msg;

  if (PeekMessage((LPMSG)&msg, NULL, 0, 0, TRUE))
    {
      if (msg.message == WM_QUIT)
        exit(0);

      if (TranslateAccelerator(msg.hwnd, hAccel, (LPMSG)&msg) == 0)
        {
          TranslateMessage((LPMSG)&msg);
          DispatchMessage((LPMSG)&msg);
        }

      return(TRUE);
    }
  return(FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevInit() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL NEAR PASCAL RevInit(
HANDLE hInstance)

{
  register PWNDCLASS    pRevClass;
  HANDLE                hbmdel;
  HDC                   hdc;
  static INT rgpat[] = { 170, 85, 170, 85, 170, 85, 170, 85 };

  hbrWhite = GetStockObject(WHITE_BRUSH);
  hbrBlack = GetStockObject(BLACK_BRUSH);
  hbmdel   = CreateBitmap(8, 8, 1, 1, (LPSTR)rgpat);
  hbrPat   = CreatePatternBrush(hbmdel);

  if (!hbrPat)
      return(FALSE);
  DeleteObject(hbmdel);
  ffirstmove = TRUE;
  hdc = GetDC((HWND)NULL);

  COLOR = GetDeviceCaps(hdc, NUMCOLORS) > 2;

  if (GetDeviceCaps(hdc, VERTRES) == 200)
      ASPECT = 2;
  else
      ASPECT = 1;
  ReleaseDC((HWND)NULL, hdc);

  hbrRed        = (HBRUSH)CreateSolidBrush(RGB(0xFF,0,0));
  hbrGreen      = (HBRUSH)CreateSolidBrush(RGB(0,0xFF,0));
  hbrBlue       = (HBRUSH)CreateSolidBrush(RGB(0,0,0xFF));

  if (!hbrRed || !hbrGreen || !hbrBlue)
      return(FALSE);

  LoadString(hInstance, 3,  (LPSTR)szReversi, 20);
  LoadString(hInstance, 4,  (LPSTR)szReversiPractice, 40);
  LoadString(hInstance, 5,  (LPSTR)szPass, 30);
  LoadString(hInstance, 6,  (LPSTR)szMustPass, 30);
  LoadString(hInstance, 7,  (LPSTR)szTie, 30);
  LoadString(hInstance, 8,  (LPSTR)szLoss, 30);
  LoadString(hInstance, 9,  (LPSTR)szWon, 30);
  LoadString(hInstance, 10, (LPSTR)szAbout, 20);
  LoadString(hInstance, 11, (LPSTR)szLossPost, 30);
  LoadString(hInstance, 12, (LPSTR)szWonPost, 30);
  LoadString(hInstance, 13, (LPSTR)szIllegal, 70);
  LoadString(hInstance, 14, (LPSTR)szNoPass, 70);
  LoadString(hInstance, 15, (LPSTR)szHelpFile, 15);

  hAccel = LoadAccelerators(hInstance, (LPSTR)"MAINACC");
  pRevClass = (PWNDCLASS)((CHAR *)LocalAlloc(LMEM_ZEROINIT, sizeof(WNDCLASS)));
  if (!pRevClass)
      return(FALSE);

  curLegal   = LoadCursor(NULL, IDC_CROSS);
  curIllegal = LoadCursor(NULL, IDC_ARROW);
  curThink   = LoadCursor(NULL, IDC_WAIT);
  curBlank   = LoadCursor(hInstance, MAKEINTRESOURCE(1));
  if (!curLegal || !curIllegal || !curThink || !curBlank)
      return(FALSE);
  pRevClass->hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(3));

  pRevClass->lpszClassName = (LPSTR)"Reversi";
  pRevClass->hbrBackground = ((COLOR) ? hbrGreen : hbrWhite);
  pRevClass->lpfnWndProc   = ReversiWndProc;
  pRevClass->lpszMenuName  = MAKEINTRESOURCE(1);
  pRevClass->hInstance    = hInstance;
  pRevClass->style         = CS_VREDRAW | CS_HREDRAW | CS_BYTEALIGNCLIENT;

  if (!RegisterClass((LPWNDCLASS)pRevClass))
    {
      LocalFree((HANDLE)pRevClass);
      return(FALSE);
    }
  LocalFree((HANDLE)pRevClass);

  return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevMouseClick() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL RevMouseClick(
HWND  hWnd,
POINT point)
{
  INT     move;
  INT     Si, Sj;
  INT     yLineExt = 8 * yExt;
  INT     xLineExt = 8 * xExt;
  HDC     hDC;

  MousePos.x = point.x;
  MousePos.y = point.y;

  if (xExt == 0 || yExt == 0)
      return;

  if ((point.x > Bx) && (point.x < (Bx+xLineExt)) && (point.y > By) && (point.y < (By+yLineExt)))
    {
      Si = (point.x - Bx) / xExt;
      Sj = (point.y - By) / yExt;
      move = Si * 10 + Sj + 11;
      if (legalcheck(board[0], move, human, computer))
        {
          board[0][move] = human;
          ffirstmove = FALSE;
          fThinking = TRUE;
          SetCursor(curThink);

          hDisp = hDC = GetDC(hWnd);
          ClearMessageTop(hDC);

          minmax(board, move, human, computer, 0, -infin, infin);
          makemove(board[0], move, human, computer);

          hDisp = hDC;

          paintmove(board[0], BestMove[0], computer, human);
          gameover(hWnd, hDC, board, BestMove[0]);

          ReleaseDC(hWnd, hDC);
          hDisp = 0;

          SetCursor(curIllegal);
          fThinking = FALSE;
        }
      else
          MessageBox(hWnd, (LPSTR)szIllegal, (LPSTR)szReversi, MB_OK | MB_ICONASTERISK);
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Next() -                                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL Next(
register INT *px,
register INT *py)

{
  (*px)++;
  if (*px > 7)
    {
      *px = 0;
      (*py)++;
      if (*py > 7)
          *py = 0;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Previous() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL Previous(
register INT *px,
register INT *py)
{
  (*px)--;
  if (*px < 0)
    {
      *px = 7;
      (*py)--;
      if (*py < 0)
          *py = 7;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ShowNextMove() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL ShowNextMove(
HWND    hwnd,
BOOL    fforward)
{
  INT       x, y;
  INT       potentialmove;
  BOOL      done;

  /* What out for infinite loops. */
  if (fPass == PASS && !ffirstmove)
      return;

  x = (MousePos.x - Bx) / xExt;
  y = (MousePos.y - By) / yExt;

  done = FALSE;
  while (!done)
    {
      do
        {
          if (fforward)
              Next(&x, &y);
          else
              Previous(&x, &y);
        }
      while ((board[0][potentialmove = (x * 10 + y + 11)]) != empty);

      fThinking = TRUE;
      if (legalcheck(board[0], potentialmove, human, computer))
          done = TRUE;

      fThinking = FALSE;
    }

  MousePos.x = x * xExt + Bx + xExt / 2;
  MousePos.y = y * yExt + By + yExt / 2;

  UpdateCursor(hwnd);
  RevMouseMove(MousePos);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RevChar() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL RevChar(
HWND            hwnd,
register WORD   code)
{
  INT   a;
  POINT curpoint;

  curpoint.x = curpoint.y = 1;
  switch (code)
    {
      case 0x27:
          MousePos.x += xExt;
          break;

      case 0x28:
          MousePos.y += yExt;
          break;

      case 0x25:
          curpoint.x = (MousePos.x - Bx)/xExt;
          MousePos.x -= xExt;
          break;

      case 0x26:
          curpoint.y = (MousePos.y - By)/yExt;
          MousePos.y -= yExt;
          break;

      case 0x24:
          curpoint.y = (MousePos.y - By)/yExt;
          curpoint.x = (MousePos.x - Bx)/xExt;
          MousePos.y -= yExt;
          MousePos.x -= xExt;
          break;

      case 0x21:
          curpoint.y = (MousePos.y - By)/yExt;
          MousePos.y -= yExt;
          MousePos.x += xExt;
          break;

      case 0x23:
          curpoint.x = (MousePos.x - Bx)/xExt;
          MousePos.y += yExt;
          MousePos.x -= xExt;
          break;

      case 0x22:
          MousePos.y += yExt;
          MousePos.x += xExt;
          break;

      case 0x0020:
      case 0x000D:
          if (!fThinking)
              RevMouseClick(hwnd, MousePos);
          return;

      case 0x0009:
          if (fThinking)
              break;
          if (GetKeyState(VK_SHIFT) < 0)
              ShowNextMove(hwnd, FALSE);    /* look backwards */
          else
              ShowNextMove(hwnd, TRUE);     /* look forwards */
          return;

      default:
          return;
    }

  if (((a = ((MousePos.x - Bx) / xExt)) >7) || a <= 0)
      MousePos.x = Bx + xExt / 2;             /* wrap around horizontally */

  if (a > 8 || (curpoint.x == 0 && a == 0))
      MousePos.x = (7*xExt) + Bx + xExt / 2 ;

  if ( ((a = ((MousePos.y - By) / yExt)) >7) || a <= 0)
      MousePos.y = By + yExt / 2;

  if ( a > 8 || (curpoint.y == 0 && a == 0))
      MousePos.y = (7*yExt) + By + yExt / 2;

  MousePos.x = ((MousePos.x - Bx) / xExt) * xExt + Bx + xExt / 2;
  MousePos.y = ((MousePos.y - By) / yExt) * yExt + By + yExt / 2;
  UpdateCursor(hwnd);
  RevMouseMove(MousePos);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InverseMessage() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID APIENTRY InverseMessage(
register HWND   hWindow,
UINT            message,
UINT_PTR        wParam,
DWORD           lParam)
{
  HDC   hDC;

  message;
  wParam;
  lParam;

  if (flashtimes <= count)
    {
      hDC = GetDC(hWindow);
      PatBlt(hDC, 0, 0, xscr, charheight, DSTINVERT);
      flashtimes++;
      ReleaseDC(hWindow, hDC);
    }
  else
      KillTimer(hWindow, 666);

  return;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReversiWndProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT APIENTRY ReversiWndProc(
HWND            hWnd,
register UINT   message,
WPARAM          wParam,
LPARAM          lParam)
{
  HMENU         hm;
  PAINTSTRUCT   ps;
  POINT         curpoint;

  switch (message)
    {
      case WM_COMMAND:
          RevMenu(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
          break;

      case WM_INITMENU:                 /* disable the menu if thinking */
          hm = GetMenu(hWnd);
          if (fThinking)
            {
              EnableMenuItem(hm, 0, MF_DISABLED | MF_BYPOSITION);
              EnableMenuItem(hm, 1, MF_DISABLED | MF_BYPOSITION);
            }
          else
            {
              EnableMenuItem(hm, 0, MF_ENABLED | MF_BYPOSITION);
              EnableMenuItem(hm, 1, MF_ENABLED | MF_BYPOSITION);
            }
          break;

      case WM_CREATE:
          RevCreate(hWnd);
          hWin = hWnd;
          break;

      case WM_CLOSE:
          if (hDisp)
              ReleaseDC(hWnd, hDisp);
          return(DefWindowProc(hWnd, message, wParam, lParam));

      case WM_DESTROY:
          if (MGetModuleUsage(hInst) == 1)
            {
              DeleteObject(hbrGreen);
              DeleteObject(hbrPat);
              DeleteObject(hbrRed);
              DeleteObject(hbrBlue);
            }

          /* In case WinHelp keys off hWindow, we need to do the HELP_QUIT
           * here instead of when there is just one instance of help...
           */
          //TEMPFIX WinHelp(hWnd, (LPSTR)szHelpFile, HELP_QUIT, 0L);

          PostQuitMessage(0);
          break;

      case WM_KEYDOWN:
          if (IsIconic(hWnd))
              return 0L;
          RevChar(hWnd, (WORD)wParam);
          break;

      case WM_ACTIVATE:
          if (!GetSystemMetrics(SM_MOUSEPRESENT))
            {
              if (GET_WM_ACTIVATE_STATE(wParam, lParam))
                {
                  if (GET_WM_ACTIVATE_HWND(wParam, lParam) != hWnd)
                    {
                      curpoint.x = MousePos.x;
                      curpoint.y = MousePos.y;
                      ClientToScreen(hWnd, (LPPOINT)&curpoint);
                      SetCursorPos(curpoint.x, curpoint.y);
                      RevMouseMove(MousePos);
                      ShowCursor(GET_WM_ACTIVATE_STATE(wParam, lParam));
                    }
                }
              else
                  ShowCursor((BOOL) wParam);
            }
          if (wParam && (!HIWORD(lParam)))
              SetFocus(hWnd);
          break;

      case WM_PAINT:
          BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
          RevPaint(hWnd, ps.hdc);
          EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
          break;

      case WM_MOUSEMOVE:
		  {
		  POINT pt;

		  LONG2POINT(lParam, pt);		/* convert LONG lParam to POINT structure*/
          if (!fThinking)
#ifdef ORGCODE		
              RevMouseMove(MAKEPOINT(lParam));
#else
              RevMouseMove(pt);
#endif
          else
              SetCursor(curThink);
          break;
		  }
      case WM_LBUTTONDOWN:
          SetCapture(hWnd);
          bMouseDownInReversi = TRUE;
          break;

      case WM_LBUTTONUP:
		  {
		  POINT pt;

		  LONG2POINT(lParam, pt);		/* convert LONG lParam to POINT structure*/

          ReleaseCapture();
          if (!fThinking && bMouseDownInReversi)
#ifdef ORGCODE
              RevMouseClick(hWnd, MAKEMPOINT(lParam));
#else
              RevMouseClick(hWnd, pt);
#endif
          bMouseDownInReversi = FALSE;
          break;
		  }
      case WM_TIMER:
          /* This should never be called. */
          break;

      case WM_VSCROLL:
      case WM_HSCROLL:
              break;

      default:
          return(DefWindowProc(hWnd, message, wParam, lParam));
          break;
      }
  return(0L);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AboutDlgProc()                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR APIENTRY AboutDlgProc(
HWND          hDlg,
UINT          message,
WPARAM        wParam,
LPARAM        lParam)

{
  if (message == WM_COMMAND)
    {
      EndDialog(hDlg, TRUE);
      return(TRUE);
    }
  if (message == WM_INITDIALOG)
      return(TRUE);
  else
      return(FALSE);
  UNREFERENCED_PARAMETER(wParam);	
  UNREFERENCED_PARAMETER(lParam);	
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WinMain() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

MMain(hInstance, hPrev, lpszCmdLine, cmdShow) /* { */
  HWND hWnd;
  MSG   msg;

  _argc;
  _argv;

  hInst = hInstance;
  if (!hPrev)
    {
      if (!RevInit(hInstance))
          return(FALSE);
    }
  else
    {
      if (fThinking)
          return FALSE;
#ifdef WIN16
      GetInstanceData(hPrev, (PSTR)&hbrBlack,           sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrPat,             sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrWhite,           sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrRed,             sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrBlue,            sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrGreen,           sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrComputer,        sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&hbrHuman,           sizeof(HBRUSH));
      GetInstanceData(hPrev, (PSTR)&curIllegal,         sizeof(HCURSOR));
      GetInstanceData(hPrev, (PSTR)&curLegal,           sizeof(HCURSOR));
      GetInstanceData(hPrev, (PSTR)&curThink,           sizeof(HCURSOR));
      GetInstanceData(hPrev, (PSTR)&curBlank,           sizeof(HCURSOR));
      GetInstanceData(hPrev, (PSTR)&prevCheck,          sizeof(prevCheck));
      GetInstanceData(hPrev, (PSTR)&depth,              sizeof(depth));
      GetInstanceData(hPrev, (PSTR)direc,               sizeof(direc));
      GetInstanceData(hPrev, (PSTR)moves,               sizeof(moves));
      GetInstanceData(hPrev, (PSTR)szReversi,           20);
      GetInstanceData(hPrev, (PSTR)szReversiPractice,   40);
      GetInstanceData(hPrev, (PSTR)szPass,              10);
      GetInstanceData(hPrev, (PSTR)szMustPass,          20);
      GetInstanceData(hPrev, (PSTR)szTie,               15);
      GetInstanceData(hPrev, (PSTR)szLoss,              15);
      GetInstanceData(hPrev, (PSTR)szWon,               15);
      GetInstanceData(hPrev, (PSTR)szAbout,             20);
      GetInstanceData(hPrev, (PSTR)&COLOR,              sizeof(INT));
      GetInstanceData(hPrev, (PSTR)&ASPECT,             sizeof(INT));
      GetInstanceData(hPrev, (PSTR)&hAccel,             2);
      GetInstanceData(hPrev, (PSTR)szIllegal,           70);
      GetInstanceData(hPrev, (PSTR)szNoPass,            70);
      GetInstanceData(hPrev, (PSTR)szHelpFile,          15);
#endif /* WIN16 */
    }

  TYMIN = 45;
  fThinking = FALSE;

  hWnd = CreateWindow((LPSTR) "Reversi",
                fCheated ? (LPSTR)szReversiPractice : (LPSTR)szReversi,
                WS_TILEDWINDOW,
                CW_USEDEFAULT,
                0,
                (GetSystemMetrics(SM_CXSCREEN) >> 1),
                (GetSystemMetrics(SM_CYSCREEN) * 4 / 5),
                (HANDLE)NULL,
                (HANDLE)NULL,
                (HANDLE)hInstance,
                NULL);

  if (!hWnd)
      return(FALSE);

  ShowWindow(hWnd, cmdShow);
  UpdateWindow(hWnd);

  /* Messaging Loop. */
  while (GetMessage((LPMSG)&msg, NULL, 0, 0))
    {
      if (!TranslateAccelerator(msg.hwnd, hAccel, (LPMSG)&msg))
        {
          TranslateMessage((LPMSG)&msg);
          DispatchMessage((LPMSG)&msg);
        }
    }
  return(0);
}
