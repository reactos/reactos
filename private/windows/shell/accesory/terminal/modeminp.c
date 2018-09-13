/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "video.h"
#include "printfil.h"

static BOOL decPrivate;                      /* True if ? in set mode string */
static BOOL heathPrivate;                    /* True if > in set mode string */

#define MAXANSIARGS 10
static INT  argNdx;
static INT  argCount;
static INT  ansiArg;
static INT  argList[MAXANSIARGS+1];

static INT  vidFG;

static INT p1;
static INT p2;

static INT saveCol = 0;
static INT saveLin = 0;


/*---------------------------------------------------------------------------*/
/* valIndex() - Return index into buffer given current line & column.  [scf] */
/*---------------------------------------------------------------------------*/

INT valIndex ()
{
   return ((curLin + savTopLine) * (maxChars + 2) + curCol);
}


/*---------------------------------------------------------------------------*/
/* cleanRect() - Erase nLines starting at line                               */
/*---------------------------------------------------------------------------*/

VOID cleanRect(INT  line, INT nLines)
{
   RECT  eraseRect;
   INT   eraseLine;

   eraseRect = hTE.viewRect;
   eraseRect.top = (line + (savTopLine - curTopLine)) * chrHeight;
   if((eraseLine = eraseRect.top + (nLines * chrHeight)) < eraseRect.bottom)
      eraseRect.bottom = eraseLine;

   if(eraseRect.top < eraseRect.bottom)
   {
      eraseColorRect((HDC) getPort(), (LPRECT) &eraseRect, ANORMAL);
      releasePort();
   }
}


/*---------------------------------------------------------------------------*/
/* scrollBuffer() -                                                   [scf]  */
/*---------------------------------------------------------------------------*/

proc scrollBuffer ()
{
   register INT line; // sdj: was unref- count;
   INT buffLines;
   RECT scrollRect, notScrollRect;

   buffLines = savTopLine - curTopLine;
   buffLines = min (buffLines, visScreenLine + 1);
   if (buffLines > 0)
   {
      scrollRect = notScrollRect = hTE.viewRect;
      if (lineFeeds < buffLines)
      {
         scrollRect.bottom = buffLines * chrHeight;
         ScrollWindow (hTermWnd, 0, -(lineFeeds * chrHeight), 
                 (LPRECT) &scrollRect, (LPRECT) &scrollRect);
      }
      lineFeeds = min (buffLines, lineFeeds);

      line = -lineFeeds;
      if(curTopLine == 0)
         line += (visScreenLine+1) - savTopLine;
      reDrawTermScreen(0, lineFeeds, line);

      notScrollRect.top = scrollRect.bottom;             /* Only validate our */
      ValidateRect (hTermWnd, (LPRECT) &notScrollRect);  /* stuff that we     */
   }                                                     /* repaint           */
   lineFeeds = 0;
}


/*---------------------------------------------------------------------------*/
/* doScroll() -                                                              */
/*---------------------------------------------------------------------------*/

proc doScroll ()
{
   INT top, bottom, line, nLines; //sdj: was unref- buffLines;
   RECT scrollRect, uncoveredRect;

   top = savTopLine - curTopLine + scrollBegin;
   bottom = top + (scrollEnd + 1 - scrollBegin);
   top = max (top, 0);
   bottom = min (bottom, visScreenLine + 1);
   scrollRect.top = top * chrHeight;
   scrollRect.bottom = bottom * chrHeight;
   scrollRect.right = hTE.viewRect.right;
   scrollRect.left = hTE.viewRect.left;
   getPort ();
   clipRect (&hTE.viewRect);
   if (scrollRect.bottom > scrollRect.top) 
   {
      hideTermCursor ();
      if (abs(nScroll) < bottom - top)
      {
         ScrollWindow (hTermWnd, 0, -(nScroll * chrHeight), 
                    (LPRECT) &scrollRect, (LPRECT) &scrollRect);
         uncoveredRect.right = hTE.viewRect.right;
         uncoveredRect.left  = hTE.viewRect.left;
         if (nScroll < 0)  /* Inserting lines */
         {
            uncoveredRect.top = scrollRect.top;
            uncoveredRect.bottom = scrollRect.top + ((-nScroll) * chrHeight);
         } else            /* Line feeds or deleting lines */
         {
            uncoveredRect.top = scrollRect.bottom - nScroll * chrHeight;
            uncoveredRect.bottom = scrollRect.bottom;
         }
         ValidateRect (hTermWnd, (LPRECT) &uncoveredRect);
      }
      nLines = min (abs(nScroll), bottom - top);
      if (nScroll > 0)
         top = bottom - nLines + curTopLine - savTopLine;
      else
         top = top + curTopLine - savTopLine;
      for (line = top; line < top + nLines; line++)
         attrib[line][LFLAGS] = (LDIRTY | LCLEAR);
      showTermCursor ();
   }
   releasePort ();
   nScroll = 0;
}

/*---------------------------------------------------------------------------*/
/* termCleanUp() -                                                           */
/*---------------------------------------------------------------------------*/

VOID termCleanUp ()
{
   register INT line, lin2;
   BYTE lattr;
   // sdj: was unref local - BYTE *attr;

   checkSelect();
   if(nScroll != 0)
      doScroll();
   if(lineFeeds != 0)
      scrollBuffer();

   for(line = 0; line <= maxScreenLine+1; )
   {
      lattr = attrib[line][LFLAGS];
      if(lattr & LDIRTY)
      {
         updateLine(line);
         line++;
      }
      else if(lattr & LCLEAR)
      {
         attrib[line][LFLAGS] = 0;
         for(lin2 = line+1; lin2 <= maxScreenLine; lin2++)
         {
            if (lattr != attrib[lin2][LFLAGS])
               break;
            attrib[lin2][LFLAGS] = 0;
         }
         cleanRect (line, lin2 - line);
         line = lin2;
      }
      else
         line++;
   }
   termDirty = FALSE;                        /* mbbx: termLine -> termDirty */
}

/*---------------------------------------------------------------------------*/
/* putChar() - Put received characters into buffer                     [scf] */
/*---------------------------------------------------------------------------*/

proc putChar(BYTE ch)
{
   INT  ndx;
   LPBYTE txtPtr;
   LPBYTE txtSrc;
   LPBYTE txtDst;
   INT  theLine;
   INT  len;
   BYTE *attr;

   txtPtr = GlobalLock(hTE.hText);
   if(!statusLine)
   {
      ndx = (curLin + savTopLine) * (maxChars + 2) + curCol;
      theLine = curLin;
   }
   else
   {
      ndx     = curCol;
      txtPtr  = (LPBYTE) line25;
      theLine = maxScreenLine + 1;
   }
   attr = attrib[theLine];
//   if (attr[curCol] & ADIRTY)
//      termCleanUp();
   if (chInsMode)
   {
      len = maxChars - curCol - 2;
      if (len > 0)
      {
         txtSrc = txtPtr + ndx;
         txtDst = txtSrc + 1;
         blockMove (txtSrc, txtDst, len);
         txtSrc = (LPBYTE) attrib + theLine * ATTRROWLEN + curCol;
         txtDst = txtSrc + 1;
         blockMove (txtSrc, txtDst, len);
         attr[LFLAGS] = LCLEAR;
      }
   }

   if(!(termData.flags & TF_DIM))
      txtPtr[ndx] = ch;

   GlobalUnlock (hTE.hText);

   attr[curCol]  = curAttrib | ADIRTY;
   attr[LFLAGS] |= LDIRTY;
   
   termDirty     = TRUE;
   termCleanUp();
}

/*---------------------------------------------------------------------------*/
/* checkSelect() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

VOID checkSelect()
{
   LONG savTopNdx;
   if (activSelect)
   {
      noSelect = FALSE;
      savTopNdx = savTopLine*(maxChars + 2);
      if (hTE.selEnd > savTopNdx)
         if (savTopNdx <= hTE.selStart)
         {
            termSetSelect (MAXINTL, MAXINTL);
            activSelect = FALSE;
         }
         else
            termSetSelect (hTE.selStart, savTopNdx);
   }
   else
      if(!noSelect)
      {
         termSetSelect (MAXINTL, MAXINTL);
         noSelect = TRUE;
      }
}

/*---------------------------------------------------------------------------*/
/* clrAttrib() -                                                       [scf] */
/*---------------------------------------------------------------------------*/

VOID clrAttrib (INT startLin, INT endLin, INT startCol, INT endCol)
{
   INT lin, col;

   for (lin = startLin; lin <= endLin; lin++)
      for (col = startCol; col <= endCol; col++)
         attrib[lin][col] = 0;
}


/*---------------------------------------------------------------------------*/
/* clrLines() -                                                              */
/*---------------------------------------------------------------------------*/

proc clrLines (INT lin1, INT lin2)
{
   LPBYTE txt, attr;
   register INT line;

   txt = (LPBYTE)GlobalLock (hTE.hText) + (savTopLine + lin1) * (maxChars + 2);
   if(lin1 == 24)
      txt = (LPBYTE) line25; 
   for(line = lin1; line <= lin2; line++) 
   {
      if(!protectMode)                       /* mbbx: protected fields */
      {
         attr = attrib[line];
         if(attr[LFLAGS])
            termCleanUp();
         lsetmem(txt, CHFILL, maxChars);
         lsetmem(attr, 0, maxChars);
         attr[LFLAGS] = LCLEAR;
         attr[LATTRIB] = LNORMAL;            /* mbbx: mac version */
         txt += maxChars + 2;
         attr += ATTRROWLEN;
      }
      else
         clrChars(line, 0, maxChars-1);
   }
   GlobalUnlock (hTE.hText);
   termDirty = TRUE;                         /* mbbx: set dirty flag */
}


/*---------------------------------------------------------------------------*/
/* clrChars() -                                                              */
/*---------------------------------------------------------------------------*/

proc clrChars (INT line, INT col1, INT col2)
{
   LPBYTE txt, attr;
   register INT col;

   txt = (LPBYTE)GlobalLock(hTE.hText) + (savTopLine + line) * (maxChars + 2) + col1;
   if(line == 24)
      txt = (LPBYTE) line25 + col1; 
   attr = attrib[line] + col1;
   for(col = col1; col <= col2; col++)
   {
      if(*attr & ADIRTY)
         termCleanUp();
      if(!protectMode || !(*attr & APROTECT))   /* mbbx: protected fields */
      {
         *txt = CHFILL;
         *attr |= ADIRTY;
      }
      txt += 1;
      attr += 1;
   }
   attrib[line][LFLAGS] = LDIRTY;
   GlobalUnlock (hTE.hText);
   termDirty = TRUE;                         /* mbbx: set dirty flag */
}


/*---------------------------------------------------------------------------*/
/* getUnprot() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

VOID getUnprot(INT   begLin, INT   begCol, INT   *lin, INT   *col)
{
   INT   iLin;
   INT   iCol;

   *lin = 0;
   *col = 0;
   for(iLin = begLin; iLin < maxScreenLine; iLin++)      /* MBBX: TEST LIMITS!!! */
      for(iCol = begCol; iCol < maxChars; iCol++)
         if(!(attrib[iLin][iCol] & APROTECT))
         {
            *lin = iLin;
            *col = iCol;
            break;
         }
}


/*---------------------------------------------------------------------------*/
/* getProtCol() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

INT getProtCol()
{
   INT   col;

   for(col = curCol; col+1 < maxChars; col++)
      if(attrib[curLin][col+1] & APROTECT)
         break;

   return(col);
}


/*---------------------------------------------------------------------------*/
/* pCursToggle() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

proc pCursToggle ()
{
   if (cursorOn == 1)
      offCursor ();
   else if (cursorOn == 0)
      onCursor ();
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursOn() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc pCursOn ()
{
   onCursor ();
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursOff() -                                                        [scf] */
/*---------------------------------------------------------------------------*/

proc pCursOff ()
{
   offCursor ();
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursRC() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc pCursRC ()
{
   INT maxWide;

   if (escLin == -1)
   {
      escLin = ch - ' ';
      if ((escLin < 0) || (escLin > maxScreenLine))
         termState = NULL;
   }
   else 
   {
      escCol = ch - ' ';
      if ((escCol >= 0) && (escCol < maxChars))
      {
         curLin = escLin;
         curCol = escCol;
         maxWide = maxChars / 2 - 1;
         if (attrib[curLin][LATTRIB] != LNORMAL)
            if (curCol > maxWide)
               curCol = maxWide;
      }
      termState = NULL;
   }
   if (termState == NULL)
      escLin = -1;
}


/*---------------------------------------------------------------------------*/
/* pSetStatusLine() -                                                        */
/*---------------------------------------------------------------------------*/

proc pSetStatusLine()                        /* mbbx 1.03: TV925 ... */
{
   curLin = 24;
   curCol = 0;
   statusLine = TRUE;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursHome() - Home the terminal emulation cursor                    [scf] */
/*---------------------------------------------------------------------------*/

proc pCursHome ()
{
   curLin = 0;
   curCol = 0;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pVideo() -                                                          [scf] */
/*---------------------------------------------------------------------------*/

proc pVideo(BYTE attr)
{
   if(attr == 0)
      curAttrib = 0;
   else
      curAttrib |= attr;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursRelative() - Move cursor relative to current position          [scf] */
/*---------------------------------------------------------------------------*/

proc pCursRelative (INT  dh, INT  dv)
{
   INT  maxWide;

   curLin += dv;
   curCol += dh;
   if (curLin < 0)
      curLin = 0;
   else if (curLin > maxScreenLine)
      curLin = maxScreenLine;
   if (curCol < 0)
      curCol = 0;
   else if (curCol > (maxChars - 1))
      curCol = maxChars - 1;
   if (attrib[curLin][LATTRIB] != LNORMAL)
   {
      maxWide = maxChars / 2 - 1;
      if (curCol > maxWide)
         curCol = maxWide;
   }
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCursUp() - Move terminal emulation cursor up one relative to curr  [scf] */
/*---------------------------------------------------------------------------*/

proc pCursUp ()
{
   pCursRelative (0, -1);
}


/*---------------------------------------------------------------------------*/
/* pCursDn() - Move terminal emulation cursor down one relative to curr [scf]*/
/*---------------------------------------------------------------------------*/

proc pCursDn ()
{
   pCursRelative (0, 1);
}


/*---------------------------------------------------------------------------*/
/* pCursRt() - Move terminal emulation cursor right one relative to curr[scf]*/
/*---------------------------------------------------------------------------*/

proc pCursRt ()
{
   pCursRelative (1, 0);
}


/*---------------------------------------------------------------------------*/
/* pCursLt() - Move terminal emulation cursor left one relative to curr [scf]*/
/*---------------------------------------------------------------------------*/

proc pCursLt ()
{
   pCursRelative (-1, 0);
}


/*---------------------------------------------------------------------------*/
/* pVPosAbsolute() -                                                   [scf] */
/*---------------------------------------------------------------------------*/

proc pVPosAbsolute ()
{
   INT vPos;

   vPos = chAscii & 0x1f;
   curLin = vPos % (maxScreenLine + 1);
   escSeq = FALSE;
   escExtend = EXNONE;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pHPosAbsolute() --                                                  [scf] */
/*---------------------------------------------------------------------------*/

proc pHPosAbsolute ()
{
   INT hPos;

   hPos = ((chAscii & 0x70) / 16) * 10;   /* to BCD! */
   hPos += chAscii & 0x0f;
   curCol = hPos % 80;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClrScr() - Clear the screen including attributes.                        */
/*---------------------------------------------------------------------------*/

VOID NEAR pClrScr()
{
   pCursHome();
   clrLines(0, maxScreenLine);
   termState = NULL;
}


VOID FAR fpClrScr()                          /* mbbx: called from aFAR ... */
{
   pClrScr();
}


/*---------------------------------------------------------------------------*/
/* pClrBol() - Clear terminal emulation video from current to beg. of line.  */
/*---------------------------------------------------------------------------*/

proc pClrBol ()
{
   if (curCol == maxChars-1)
      clrLines (curLin, curLin);
   else
      clrChars (curLin, 0, curCol);
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClrBop() - Clear terminal emulation video from curr to beg. of page.     */
/*---------------------------------------------------------------------------*/

proc pClrBop ()
{
   pClrBol ();
   if (curLin > 0)
      clrLines (0, curLin-1);
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClrEol() - Clear logical IT video from present to end of line.     [scf] */
/*---------------------------------------------------------------------------*/

proc pClrEol ()
{
   if (curCol == 0)
      clrLines (curLin, curLin);
   else
      clrChars (curLin, curCol, maxChars-1);
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClrEop() - Clear the logical IT video from present to end of page. [scf] */
/*---------------------------------------------------------------------------*/

proc pClrEop ()
{
   pClrEol ();
   if (curLin < maxScreenLine)
      clrLines (curLin + 1, maxScreenLine);
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClrLine () Clear logical IT video from beginning to end of line.   [scf] */
/*---------------------------------------------------------------------------*/

proc pClrLine ()
{
   clrLines (curLin, curLin);
}


/*---------------------------------------------------------------------------*/
/* scrollAttrib () -                                                   [scf] */
/*---------------------------------------------------------------------------*/

proc scrollAttrib (INT  startLin, INT  endLin, INT  nLines, BOOL up)
{
   LPBYTE  txtSrc;
   LPBYTE  txtDst;
   INT     len;
   // sdj: was unref local - INT	lin;
   // sdj: was unref local - INT	col;

   if ((endLin - startLin) >= nLines)
   {
      txtSrc = (LPBYTE) attrib[0] + startLin*ATTRROWLEN;
      txtDst = txtSrc + nLines*ATTRROWLEN;
      len    = (endLin - startLin + 1 - nLines)*ATTRROWLEN;
      if (up)
         blockMove(txtDst, txtSrc, len);
      else
         blockMove(txtSrc, txtDst, len);
   }
}

/*---------------------------------------------------------------------------*/
/* pLF() -                                                                   */
/*---------------------------------------------------------------------------*/

proc pLF()
{
   LPBYTE txtDst, txtSrc;
   // sdj: was unref local - LONG	newStart, newEnd;
   INT	  len; // sdj: was unref local - maxLin, savLin, bottomLine;
   // sdj: was unref local - INT	width, height;

   checkSelect ();
   UpdateWindow(hTermWnd); /* jtf 3.30 */
   if(curLin == scrRgnEnd)
   {
      if(savTopLine == (maxLines - (maxScreenLine + 2))) /* no buffer left */
      {
         if(savTopLine > 0)                  /* mbbx 1.01: (maxLines == 25) */
         {
            txtDst = GlobalLock (hTE.hText);
            txtSrc = txtDst + maxChars + 2;
            len = (savTopLine - 1) * (maxChars + 2);
            blockMove(txtSrc, txtDst, len);
            txtSrc = txtDst + (savTopLine + scrRgnBeg) * (maxChars + 2);
            txtDst = txtDst + len;
            blockMove(txtSrc, txtDst, (DWORD) maxChars);
            GlobalUnlock(hTE.hText);
            if(curTopLine + visScreenLine < savTopLine)
            {
               if (curTopLine > 0)
                  curTopLine--;
               else
                  lineFeeds++;
            }
            else
               lineFeeds++;
         }
         pDelLin(scrRgnBeg, scrRgnEnd, 1);
      }
      else  /* plenty of buffer */
      {
         if (scrRgnBeg != 0 || scrRgnEnd != maxScreenLine)
         {
            txtSrc = (LPBYTE)GlobalLock (hTE.hText) + savTopLine * (maxChars + 2);
            txtDst = txtSrc + maxChars + 2;
            blockMove (txtSrc, txtDst,  
                        (DWORD) (maxScreenLine + 1) * (maxChars + 2));
            txtDst = txtSrc;
            txtSrc += (scrRgnBeg + 1) * (maxChars + 2);
            blockMove (txtSrc, txtDst, (DWORD) maxChars);
            GlobalUnlock (hTE.hText);
            if (curTopLine + visScreenLine >= savTopLine)
            {
               curTopLine++;
               lineFeeds++;
            }
            savTopLine++;
            pDelLin (scrRgnBeg, scrRgnEnd, 1);
         }
         else 
         {
            if(nScroll > 0)
            {
               if(((scrollBegin == scrRgnBeg) && (scrollEnd == scrRgnEnd)) && 
                  ((nScroll + 1) < ((scrRgnEnd + 1 - scrRgnBeg) >> 2)))
               {
                  nScroll++;
               }
               else
                  termCleanUp();
            }
            else if(nScroll < 0)
               termCleanUp ();

            if(nScroll == 0)
            {
               scrollBegin = scrRgnBeg;
               scrollEnd = scrRgnEnd;
               nScroll = 1;
            }
            if (curTopLine + visScreenLine >= savTopLine)
            {
               curTopLine++;
               lineFeeds++;
            }
            savTopLine++;
            scrollAttrib (scrRgnBeg, scrRgnEnd, 1, TRUE);
            attrib[scrRgnEnd][LFLAGS] = 0;
            clrLines (scrRgnEnd, scrRgnEnd);
         }
      }

      nScrollPos.y = curTopLine;
      updateTermScrollBars(FALSE);

      termDirty = TRUE;
   }
   else
   {
      pCursRelative(0, 1);
      if((curLin - savTopLine) - nScrollPos.y == visScreenLine+1)  /* mbbx: TEST */
         scrollDown(SB_VERT, SB_LINEDOWN, 1);
   }
}

/*---------------------------------------------------------------------------*/
/* pInsLin() - Insert a line into the logical video screen.            [scf] */
/*---------------------------------------------------------------------------*/

proc pInsLin (INT maxLin, INT nLines)
{
   INT line, lin1;
   INT  len;
   LPBYTE txtSrc;
   LPBYTE txtDst;

   if (statusLine)
     clrLines (24,24);
   else
   {
      if(nScroll < 0)
      {
         if(((scrollBegin == curLin) && (scrollEnd == maxLin)) && 
            ((abs(nScroll) + nLines) <= ((maxLin + 1 - curLin) >> 1)))
         {
            nScroll -= nLines;
         }
         else
            termCleanUp();
      }
      else if(nScroll > 0)
         termCleanUp();

      if(nScroll == 0)
      {
         scrollBegin = curLin;
         scrollEnd = maxLin;
         nScroll = -nLines;
      }
      for(line = maxLin - nLines + 1; line <= maxLin; line++)
         if(attrib[line][LFLAGS])
         {
            termCleanUp();
            break;
         }
      scrollAttrib (curLin, maxLin, nLines, FALSE);
      txtSrc = (LPBYTE)GlobalLock (hTE.hText) + 
            (curLin + savTopLine) * (maxChars + 2);
      txtDst = txtSrc + nLines * (maxChars + 2);
      len = (maxLin + 1 - curLin - nLines) * (maxChars + 2);
 
      if (len > 0)
         blockMove (txtSrc, txtDst, (DWORD) len);
      line = curLin + nLines - 1;
      for (lin1 = curLin; lin1 <= line; lin1++)
         attrib[lin1][LFLAGS] = 0;
      clrLines (curLin, line);
      GlobalUnlock (hTE.hText);
   }
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pDelLin() - Delete a line                                                 */
/*---------------------------------------------------------------------------*/

proc pDelLin (INT curLin, INT maxLin, INT nLines)
{
   INT line, lin1;
   INT  len;
   LPBYTE txtSrc;
   LPBYTE txtDst;

   if(statusLine)
      clrLines(24, 24);
   else
   {
      if(nScroll > 0)
      {
         if(((scrollBegin == curLin) && (scrollEnd == maxLin)) && 
            ((nScroll + nLines) <= ((maxLin + 1 - curLin) >> 2)))
         {
            nScroll += nLines;
         }
         else
            termCleanUp();
      }
      else if(nScroll < 0)
         termCleanUp();

      if(nScroll == 0)
      {
         scrollBegin = curLin;
         scrollEnd = maxLin;
         nScroll = nLines;
      }
      for(line = curLin; line < curLin + nLines; line++)
         if(attrib[line][LFLAGS])
         {
            termCleanUp();
            break;
         }
      scrollAttrib (curLin, maxLin, nLines, TRUE);
      txtDst = (LPBYTE)GlobalLock (hTE.hText) + 
            (curLin + savTopLine) * (maxChars + 2);
      txtSrc = txtDst + nLines * (maxChars + 2);
      len = (maxLin + 1 - curLin - nLines) * (maxChars + 2);
      if (len > 0)
         blockMove (txtSrc, txtDst, (DWORD) len);
      line = maxLin - nLines + 1;
      for (lin1 = line; lin1 <= maxLin; lin1++)
         attrib[lin1][LFLAGS] = 0;
      clrLines (line, maxLin);
      GlobalUnlock (hTE.hText);
   }
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pDelChar() - Delete a char from the terminal emulation video.       [scf] */
/*---------------------------------------------------------------------------*/

proc pDelChar(INT nChars)
{
   INT  ndx;
   // sdj: was unref local - INT	savCol;
   INT  theLine;

   DWORD len;

   LPBYTE txtSrc;
   LPBYTE txtDst;
   // sdj: was unref local - LPBYTE txtPtr;
   LPBYTE globalHeap;

   globalHeap = GlobalLock(hTE.hText);
   if(!protectMode)                          /* mbbx: protected fields */
      len = maxChars - nChars - curCol;
   else
      len = getProtCol() - nChars - (curCol+1);
   if(statusLine)
   {
      theLine = maxScreenLine + 1;
      txtDst  = (LPBYTE) line25 + curCol;
   }
   else
   {
      theLine = curLin;
      txtDst  = globalHeap + valIndex ();
   }
   if (len > 0)
   {
      txtSrc  = txtDst + nChars;
      blockMove (txtSrc, txtDst, len);
   }
   else
   {
      nChars = maxChars - curCol;
      len = 0;
   }
   txtSrc = txtDst + len;
   for (ndx = 0; ndx <= (nChars - 1); ndx++)
      txtSrc[ndx] = CHFILL;
   GlobalUnlock (hTE.hText);
   attrib[theLine][LFLAGS] = LCLEAR | LDIRTY;
}


/*---------------------------------------------------------------------------*/
/* begGraphics() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

VOID begGraphics ()   /* macx */
{
   termCleanUp ();
   offCursor ();
}


/*---------------------------------------------------------------------------*/
/* endGraphics() -                                                     [scf] */
/*---------------------------------------------------------------------------*/

VOID endGraphics ()   /* macx */
{
   onCursor ();
}

/*---------------------------------------------------------------------------*/
/* pGrChar () -                                                              */
/*---------------------------------------------------------------------------*/

static HDC graphicsPort;

#define GRCOLBEG  48
#define GRLINBEG  0
#define MAXGRCOL  512

proc pGrSemi ()
{
/* jtfterm */
}


/*---------------------------------------------------------------------------*/
/* pGrDoIt() -                                                               */
/*---------------------------------------------------------------------------*/

proc pGrDoIt (INT    len, HBRUSH p)
{
/* jtfterm */
}


/*---------------------------------------------------------------------------*/
/* pGrFill() -                                                        [scf]  */
/*---------------------------------------------------------------------------*/

proc pGrFill ()
{
   ch = theChar;
   if (ch == 7)
   {
      sysBeep ();
      vidGraphics = GRNONE;
      termState = NULL;
      chrWidth    = stdChrWidth;
      chrHeight   = stdChrHeight;
      endGraphics ();
   }
   else if (ch != CR && ch != LF)
   {
      if (vidBG == -1)
         vidBG = (ch - ' ')*chrWidth;
      else
      {
         vidFG = (ch - ' ')*chrWidth;
         if (vidBG > 0)
            pGrDoIt(vidBG, blackBrush);
         if (vidFG > 0)
            pGrDoIt(vidFG, theBrush);
         vidBG = -1;
      }
   }
   else
      termState = NULL;
} 


/*---------------------------------------------------------------------------*/
/* pGrChar()                                                                 */
/*---------------------------------------------------------------------------*/

proc pGrChar ()
{
   HRGN  hClipRgn;
   RECT  clpRect;
 
   getPort ();
   graphicsPort = thePort;

   GetClientRect (hTermWnd, (LPRECT) &clpRect);
   hClipRgn = CreateRectRgnIndirect ((LPRECT) &clpRect);
   SelectClipRgn(thePort, hClipRgn);
   DeleteObject (hClipRgn);

   ch = the8Char;
   if (vidGraphics == GRSEMI)
      if ((the8Char & 0x80) != 0)
         pGrSemi ();
      else
         termState = NULL;
   else
      pGrFill ();
   releasePort ();
}


/*---------------------------------------------------------------------------*/
/* pSetGrMode() -                                                      [scf] */
/*---------------------------------------------------------------------------*/

proc pSetGrMode()
{
/* jtfterm */
}


/*---------------------------------------------------------------------------*/
/* pSetMode() -                                                        [scf] */
/*---------------------------------------------------------------------------*/

proc pSetMode ()
{
   BYTE chSet;

   termState = NULL;
   if((trmParams.emulate >= ITMVT52) && (trmParams.emulate <= ITMVT220)) /* mbbx: why VT52??? */
      switch (ch)
      {
         case 0x0e:
         case 0x0f:
            if (ch == 0x0e)
               shiftCharSet = 1;
            else
               shiftCharSet = 0;
            chSet = charSet[shiftCharSet];
            if ((chSet == '0') || (chSet == '2'))
               curAttrib |= AGRAPHICS;
            else
               curAttrib &= ANOTGRAPHICS;
            break;
         case 'x': /* VT52/Heath*/
            escExtend = EXSETMODE;
            termState = pSetMode;
            break;
         case 'y': /* VT52/Heath */
            escExtend = EXRESETMODE;
            termState = pSetMode;
            break;
         case '=': 
            keyPadAppMode = TRUE;
            break;
         case '>':
            keyPadAppMode = FALSE;
            break;
         case '<':
            trmParams.emulate = ITMVT100;
            resetEmul ();
            break;
         case '5': /* VT52/Heath */
            if (escExtend == EXSETMODE)
              pCursOff ();
            else
              pCursOn ();
            break;
         case '7': /* VT52/Heath */
            keyPadAppMode = (escExtend == EXSETMODE);
            break;
         case '@':
            chInsMode = TRUE;
            break;
         case 'O':
            chInsMode = FALSE;
            break;
      }
}

/*---------------------------------------------------------------------------*/
/* pDecScs() - VT-100 character set selection.                         [scf] */
/*---------------------------------------------------------------------------*/

proc pDecScs ()
{
   BYTE chSet;

   termState = NULL;
   switch (ch)
   {
      case '(':
         decScs = 0;
         termState = pDecScs;
         break;
      case ')':
         decScs = 1;
         termState = pDecScs;
         break;
      case 'A':
      case 'B':
      case '0':
      case '1':
      case '2':
         charSet[decScs] = ch;
         chSet = charSet[shiftCharSet];
         if ((chSet == '0') || (chSet == '2'))
            curAttrib |= AGRAPHICS;
         else
            curAttrib &= ANOTGRAPHICS;
         break;
   }
}

/*---------------------------------------------------------------------------*/
/* getArg() -                                                                */
/*---------------------------------------------------------------------------*/

#define getArg    ((argNdx < argCount) ? argList[argNdx++] : 0)

/*---------------------------------------------------------------------------*/
/* getParms ()                                                               */
/*---------------------------------------------------------------------------*/

proc getParms ()
{
   p1 = getArg;
   p2 = getArg;
}


/*---------------------------------------------------------------------------*/
/* pInquire() -                                                        [scf] */
/*---------------------------------------------------------------------------*/

proc pInquire()                              /* mbbx 1.04: VT220... */
{
   STRING respStr[32];

   *respStr = 0;

   switch(trmParams.emulate)
   {
   case ITMVT100:
      getParms();
      if(p1 == 0)
         memcpy(respStr, "\7\033[?1;2c", 9);
      break;

   case ITMVT52:
      memcpy(respStr, "\3\033/Z", 5);
      break;
   }

   if(*respStr)
      termStr(respStr, 0, FALSE);

   termState = NULL;
}



/*---------------------------------------------------------------------------*/
/* pTab() -                                                            [scf] */
/*---------------------------------------------------------------------------*/

proc pTab()
{
   INT  ndx;
   INT  savCol;
   INT  lin, col;

   if(!protectMode)
   {
      savCol = curCol;
      ndx    = curCol + 1;
      while (ndx <= 131)
         if(tabs[ndx] == 1)
         {
            curCol = ndx;
            ndx    = 133;
         }
         else
            ndx++;
   }
   else                                      /* mbbx: tab for protected mode */
   {
      lin = 0;
      col = 0;
      repeat
         getUnprot(lin, col, &lin, &col);
      until((lin >= curLin) && (col >= curCol));
      ndx = 0;
      curLin = lin;
      curCol = col;
   }

   if((ndx == 132) || (curCol >= maxChars))
      curCol = maxChars - 1;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClearAllTabs() -                                                   [scf] */
/*---------------------------------------------------------------------------*/

proc pClearAllTabs ()
{
   INT  ndx;

   for (ndx = 0; ndx <= 131; ndx++)
      tabs[ndx] = 0;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pSetTab() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc pSetTab ()
{
   tabs[curCol] = 1;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pClearTab() -                                                       [scf] */
/*---------------------------------------------------------------------------*/

proc pClearTab ()
{
   tabs[curCol] = 0;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pCmpSrvResponse() -                                                 [scf] */
/*---------------------------------------------------------------------------*/

proc pCmpSrvResponse ()
{
/* jtfterm */
}


/*---------------------------------------------------------------------------*/
/* pSndCursor() -                                                      [scf] */
/*---------------------------------------------------------------------------*/

proc pSndCursor()
{
   LONG  finalTicks;

   termState = NULL;

   switch(trmParams.emulate)
   {
   case ITMVT52:
      modemWr (0x1b); delay (5, &finalTicks);
      modemWr ('Y');  delay (5, &finalTicks);
      modemWr ((BYTE) curLin + ' '); delay (5, &finalTicks);
      modemWr ((BYTE) curCol + ' '); delay (5, &finalTicks);
      break;
   }
}


/*---------------------------------------------------------------------------*/
/* pIndex() -                                                                */
/*---------------------------------------------------------------------------*/

proc pIndex ()
{

   if (curLin == scrRgnEnd)
      pDelLin (scrRgnBeg, scrRgnEnd, 1);
   else
      pCursRelative (0, 1);
}

/*---------------------------------------------------------------------------*/
/* pRevIndex() -                                                       [scf] */
/*---------------------------------------------------------------------------*/

proc pRevIndex ()
{
   if (curLin == scrRgnBeg)
      pInsLin (scrRgnEnd, 1);
   else
      pCursRelative (0, -1);
}


/*---------------------------------------------------------------------------*/
/* pSetLineAttrib() -                                                  [scf] */
/*---------------------------------------------------------------------------*/

proc pSetLineAttrib ()
{
   BYTE lAttr;
   INT  theLin;
   INT  savCol;
   INT  ndx;
   INT  half;
   LPBYTE txtPtr;
   LPBYTE globalHeap;

   lAttr = -1;
   switch (ch)
   {
      case '3':
         lAttr = LHIGHTOP;
         break;
      case '4':
         lAttr = LHIGHBOTTOM;
         break;
      case '5':
         lAttr = LNORMAL;
         break;
      case '6':
         lAttr = LWIDE;
         break;
   }
   if (statusLine)
      theLin = 24;
   else
      theLin = curLin;
   if (lAttr != -1)
      if (lAttr != attrib[theLin][LATTRIB])
      {
         savCol = curCol;
         curCol = 0;
         half = maxChars;
         if (lAttr != LNORMAL)
         {
            half = maxChars / 2;
            globalHeap = GlobalLock (hTE.hText);
            txtPtr = globalHeap + valIndex ();
            for (ndx = half; ndx <= (maxChars - 1); ndx++)
            {
               txtPtr[ndx] = ' ';
               attrib[theLin][ndx] = 0;
            }
            GlobalUnlock (hTE.hText);
         }
         attrib[theLin][LATTRIB] = lAttr;
         attrib[theLin][LFLAGS] = LCLEAR | LDIRTY;      /* mbbx: mac version */
         reDrawTermLine (theLin, 0, half);
         curCol = savCol;
      }

   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pInsChar() -                                                        [scf] */
/*---------------------------------------------------------------------------*/

proc pInsChar ()
{
   BOOL savMode;

   if(!protectMode)
   {
      savMode = chInsMode;
      chInsMode = TRUE;
      putChar (' ');
      chInsMode = savMode;
      termState = NULL;
   }
   else
      pDelChar(1);
}


/*---------------------------------------------------------------------------*/
/* pSaveCursorPos() -                                                        */
/*---------------------------------------------------------------------------*/

proc pSaveCursorPos ()
{
   saveCol = curCol;
   saveLin = curLin;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pRestoreCursorPos() -                                                     */
/*---------------------------------------------------------------------------*/

proc pRestoreCursorPos ()
{
   curCol = saveCol;
   curLin = saveLin;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pEscSkip                                                                  */
/*---------------------------------------------------------------------------*/

proc pEscSkip ()
{
   escSkip--;
   if (escSkip <= 0)
      termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* Some change of state procedures                                           */
/*---------------------------------------------------------------------------*/

proc pNullState ()
{
   termState = NULL;
}

proc pCursorState ()
{
   termState = pCursRC;
}	

proc pVPosState ()
{
   termState = pVPosAbsolute;
}

proc pHPosState ()
{
   termState = pHPosAbsolute;
}

proc pLAttrState ()
{
   termState = pSetLineAttrib;
}

/*---------------------------------------------------------------------------*/
/* pAnsi() -                                                           [scf] */
/*---------------------------------------------------------------------------*/

proc pAnsi()
{
   if((chAscii >= '0') && (chAscii <= '?'))
      (*ansiParseTable[chAscii-'0'])();
   else
   {
      if(ansiArg)
         argList[argCount++] = ansiArg;

      (*aEscTable[chAscii &= 0x7F])();       /* mbbx 1.06: modemInp() not masking 8th bit */

      if(termState != aEscTable[chAscii])    /* mbbx 1.10: VT220 8BIT patch !!! */
         termState = NULL;
   }
}

proc pAnsiState ()
{
   termState = pAnsi;
   argNdx = 0;
   argCount = 0;
   ansiArg = 0;
   decPrivate = FALSE;
   heathPrivate = FALSE;
}

proc pGrState ()
{
   termState = pSetGrMode;
}

proc pSkipState ()
{
   escSkip = emulInfo[chAscii] - 128;
   termState = pEscSkip;
}

proc pReverseOff()                           /* mbbx: pProtOff -> pReverseOff */
{
   pVideo(0);
}

proc pReverseOn()                            /* mbbx: pProtOn -> pReverseOn */
{
   pVideo(ABOLD);                               /* mbbx per jtfx */
}

proc pProtOff()                              /* mbbx */
{
   protectMode = FALSE;
   termState = NULL;
}

proc pProtOn()                               /* mbbx */
{
   protectMode = TRUE;
   termState = NULL;
}

proc pBegProtect()                           /* mbbx 1.03: TV925... */
{
   pVideo(APROTECT);
   pVideo(ABOLD);
}

proc pEndProtect()                           /* mbbx 1.03: TV925... */
{
   pVideo(ANORMAL);
}

proc pBegGraphics()                          /* mbbx 1.03: TV925... */
{
   curAttrib |= AGRAPHICS;
   termState = NULL;
}

proc pEndGraphics()                          /* mbbx 1.03: TV925... */
{
   curAttrib &= ANOTGRAPHICS;
   termState = NULL;
}

/*---------------------------------------------------------------------------*/
/* Some patches                                                              */
/*---------------------------------------------------------------------------*/

proc pLinDel ()
{
   pDelLin(curLin, maxScreenLine, 1);        /* mbbx per slcx */
}

proc pCharDel ()
{
   pDelChar(1);
}

proc pLinIns ()
{
   pInsLin(maxScreenLine, 1);
}

proc pNextLine ()
{
   pLF ();
   curCol = 0;
}


proc pClrAll()
{
   protectMode = FALSE;
   pClrScr();
}


proc pPrintOn()
{
   PrintFileComm(TRUE);         /* mbbx 1.03: prAction(!prtFlag); */
   termState = NULL;
}


proc pPrintOff()
{
   // sdj: was unref local - BYTE	work[STR255];

   PrintFileComm(FALSE);         /* mbbx 1.03: prAction(!prtFlag); */
   termState = NULL;
}

/*---------------------------------------------------------------------------*/

#define TRANS_PRINT_BUFFER       4

STRING   transPrtBuf[TRANS_PRINT_BUFFER+1];

proc pTransPrint()                           /* mbbx 2.01.32 ... */
{
   if(trmParams.emulate == ITMVT100)
   {
      getParms();
      switch(p1)
      {
      case 5:
         transPrintFlag = TRUE;
         *transPrtBuf = 0;                /* slc gold 017 */
         break;

      case 4:
         PrintFileComm(FALSE);
         transPrintFlag = FALSE;
         break;

      case 1:
         /* print cursor line */
         break;

      case 0:
         /* print screen */
         break;
      }
   }
   else
      transPrintFlag = TRUE;

   if(transPrintFlag)
      PrintFileComm(TRUE);

   termState = NULL;
}

/*---------------------------------------------------------------------------*/


proc pVideoAttrib()                          /* mbbx 1.03: added breaks !!! */
{
   switch(trmParams.emulate)
   {
/* jtfterm */
   }
   termState = NULL;
}


proc pVideoAttribState()
{
   termState = pVideoAttrib;
}


proc pCursorOnOff()
{
   if(chAscii != '0')
      pCursOn();
   else
      pCursOff();
}


proc pCursorOnOffState()
{
   termState = pCursorOnOff;
}


/*---------------------------------------------------------------------------*/
/* pAnswerBack() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

proc pAnswerBack()
{
#ifdef ORGCODE
   INT     i;
#else
   DWORD     i;
#endif

   BYTE    saveEcho;
   LONG    finalTicks;
   STRING  temp[DCS_ANSWERBACKSZ+1];                  /* rkhx 2.00 */

   saveEcho = trmParams.localEcho;
   trmParams.localEcho = FALSE;              /* mbbx 1.10: CUA */

   strcpy(temp+1, trmParams.answerBack);     /* rkhx 2.00 */
   *temp = strlen(temp+1);
   stripControl(temp);
   for(i = 1; i <= *temp; i++)
   {
      modemWr(temp[i]);
      delay(2, &finalTicks);
   }

   trmParams.localEcho = saveEcho;
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pEchoOff() -                                                        [mbb] */
/*---------------------------------------------------------------------------*/

proc pEchoOff()
{
   trmParams.localEcho = FALSE;              /* mbbx 1.10: CUA */
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pEchoOn() -                                                         [mbb] */
/*---------------------------------------------------------------------------*/

proc pEchoOn()
{
   trmParams.localEcho = TRUE;               /* mbbx 1.10: CUA */
   termState = NULL;
}


/* mbbx: added to make the TABLE handling consistent 1/6/88 */

proc pCR()
{
   if((trmParams.emulate == ITMDELTA) && (curCol > 0))
   {
      while(curCol < maxChars)
      {
         putChar(SP);
         curCol++;
      }
   }
   curCol = 0;

   if(statusLine)                            /* mbbx 1.03: TV925 per slcx... */
   {
      curLin = 0;
      statusLine = FALSE;
   }
}


proc pBackSpace()
{
   if(curCol > 0) /* jtf 3.14 */
      curCol--;
/* jtf 3.33   putChar(SP); */  /* desctructive backspace 3.17 */ 

}


proc pBeep()
{
   if(trmParams.sound)                       /* mbbx 1.04: synch */
      sysBeep();
}


/*---------------------------------------------------------------------------*/
/* pEscSequence                                                              */
/*---------------------------------------------------------------------------*/

proc pEscSequence()
{
   (*pEscTable[chAscii & 0x7F])();           /* mbbx 2.00: modemInp() not masking 8th bit */
}


/*------------------  >>> pAnsi() Global Declarations <<<  ------------------*/

/* Procedure pAnsi () */

/*---------------------------------------------------------------------------*/
/* aSetCompLevel() - set VT220 Compatibility Level                 [mbb/rkh] */
/*---------------------------------------------------------------------------*/

VOID NEAR aSetCompLevel()                    /* mbbx 1.10: VT220 8BIT... */
{
   BYTE  newEmul;

   termState = NULL;

   switch(ch)
   {
   case '"':
      termState = aSetCompLevel;
      break;

   case 'p':
      switch(getArg)
      {
      case 61:
         newEmul = ITMVT100;
         break;
      case 62:
         newEmul = ITMVT220;
         trmParams.fCtrlBits = !(getArg == 1);
         break;
      default:
         return;
      }

      if(newEmul != trmParams.emulate)
      {
         trmParams.emulate = newEmul;
         resetEmul();
      }
      break;
   }
}


VOID NEAR pSetCtrlBits()                     /* mbbx 2.00: VT220 8BIT... */
{
   termState = NULL;

   switch(ch)
   {
   case ' ':
      termState = pSetCtrlBits;
      break;
   case 'F':
      trmParams.fCtrlBits = FALSE;
      break;
   case 'G':
      trmParams.fCtrlBits = TRUE;
      break;
   }
}


/*---------------------------------------------------------------------------*/
/* aCursor()                                                           [scf] */
/*---------------------------------------------------------------------------*/

proc aCursor()
{
   register INT lin, col;

   getParms ();
   if (p1 == 0)
      p1 = 1;
   if (p2 == 0)
      p2 = 1;
   lin = p1 - 1;
   col = p2 - 1;
   if (originMode)
   {
      lin += scrRgnBeg;
      if (lin < scrRgnEnd)
         curLin = lin;
      else
         curLin = scrRgnEnd;
   }
   else
      if (lin <= maxScreenLine)
      {
         if (statusLine)
         {
            statusLine = FALSE;
         }
         curLin = lin;
      }
      else
      {
         if (!statusLine)
         {
            statusLine = TRUE;
         }
         curLin = maxScreenLine;
      }
   if (col < maxChars)
      curCol = col;
}


/*---------------------------------------------------------------------------*/
/* aClrEol() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aClrEol ()
{
   getParms ();
   if (p1 == 0)
      pClrEol ();
   else if (p1 == 1)
      pClrBol ();
   else if (p1 == 2)
   {
      pClrBol ();
      pClrEol ();
   }
}


/*---------------------------------------------------------------------------*/
/* aClrEop() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aClrEop ()
{
   getParms ();
   if (p1 == 0)
      pClrEop ();
   else if (p1 == 2)
      pClrScr ();
   else if (p1 == 1)
      pClrBop ();
}


/*---------------------------------------------------------------------------*/
/* aCursUp() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aCursUp ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   pCursRelative (0, (INT) -p1);
}


/*---------------------------------------------------------------------------*/
/* aCursDn() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aCursDn ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   pCursRelative (0, (INT) p1);
}


/*---------------------------------------------------------------------------*/
/* aCursRt() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aCursRt ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   pCursRelative ((INT) p1, 0);
}


/*---------------------------------------------------------------------------*/
/* aCursLt() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aCursLt ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   pCursRelative ((INT) -p1, 0);
}


/*---------------------------------------------------------------------------*/
/* aClearTabs() -                                                      [scf] */
/*---------------------------------------------------------------------------*/

proc aClearTabs ()
{
   getParms ();
   if (p1 == 3)
      pClearAllTabs ();
   else if (p1 == 0)
      pClearTab ();
}


/*---------------------------------------------------------------------------*/
/* aVideo() -                                                          [scf] */
/*---------------------------------------------------------------------------*/

proc aVideo ()
{
   repeat
   {
      switch ((WORD) getArg)
      {
         case 0:
            curAttrib &= AGRAPHICS;
            break;
         case 1:
            curAttrib |= ABOLD;
            break;
         case 4:
            curAttrib |= AUNDERLINE;
            break;
         case 5:
            curAttrib |= ABLINK;
            break;
         case 7:
            curAttrib |= AREVERSE;
            break;
      }
   }
   until (argNdx == argCount);
}


/*---------------------------------------------------------------------------*/
/* aSetMode() -                                                        [mbb] */
/*---------------------------------------------------------------------------*/

proc aSetMode()                              /* mbbx 2.00: cleanup... */
{
   while(argNdx < argCount)
   {
      switch(ch)
      {
      case 'h':
         if(decPrivate)                      /* SET DEC private modes */
         {
            switch(getArg)
            {
            case 1:
               cursorKeyMode = TRUE;
               break;
            case 6:
               originMode = TRUE;
               curLin     = scrRgnBeg;
               curCol     = 0;
               break;
            case 7:
               trmParams.lineWrap = TRUE;    /* mbbx 1.10: CUA... */
               break;
            }
         }
         else if(heathPrivate)               /* RESET heath private modes */
         {
            switch(getArg)
            {
            case 5:
               pCursOn();
               break;
            case 7:
               keyPadAppMode = FALSE;
               break;
            }
         }
         else
         {
            switch(getArg)
            {
            case 4:
               chInsMode = TRUE;
               break;
            }
         }
         break;

      case 'l':
         if(decPrivate)                      /* RESET DEC private modes */
         {
            switch(getArg)
            {
            case 1:
               cursorKeyMode = FALSE;
               break;
            case 2:
               trmParams.emulate = ITMVT52;
               resetEmul ();
               break;
            case 6:
               originMode = FALSE;
               pCursHome();
               break;
            case 7:
               trmParams.lineWrap = FALSE;   /* mbbx 1.10: CUA... */
               break;
            }
         }
         else if(heathPrivate)
         {
            switch(getArg)                   /* SET HEATH private modes */
            {
            case 5:
               pCursOff();
               break;
            case 7:
               keyPadAppMode = TRUE;
               break;
            }
         }
         else
         {
            switch(getArg)
            {
            case 4:
               chInsMode = FALSE;
               break;
            }
         }
         break;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* aReport() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aReport()                               /* mbbx 1.04: cleanup... */
{
   STRING   respStr[16];

   getParms();
   switch(p1)
   {
   case 6:
      sprintf(respStr+1, "\033[%d;%dR", curLin+1, curCol+1);
      *respStr = strlen(respStr+1);
      break;

   case 5:
      memcpy(respStr, "\4\033[0n", 5);
      break;
   }

   if(*respStr != 0)
      termStr(respStr, 0, FALSE);
}



/*---------------------------------------------------------------------------*/
/* aSetScrRgn() -                                                      [scf] */
/*---------------------------------------------------------------------------*/

proc aSetScrRgn ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   if (p2 == 0)
      p2 = 1;
   if ((p2 > p1) && (p2 <= (maxScreenLine + 1)))
   {
      scrRgnBeg = p1 - 1;
      scrRgnEnd = p2 - 1;
   }
   else if (p2 == 1)
   {
      scrRgnBeg = 0;
      scrRgnEnd = maxScreenLine;
   }
   if (originMode)
      curLin = scrRgnBeg;
   else
      curLin = 0;
   curCol = 0;
}


/*---------------------------------------------------------------------------*/
/* aDelLin() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aDelLin ()
{
   INT  len;

   getParms ();
   if (p1 == 0)
      p1 = 1;
   if ((curLin >= scrRgnBeg) && (curLin <= scrRgnEnd))
   {
      len = scrRgnEnd - curLin + 1;
      if (p1 > len)
         p1 = len;
      pDelLin(curLin, scrRgnEnd, (INT) p1);  /* mbbx per slcx */
      curCol = 0;
   }
}


/*---------------------------------------------------------------------------*/
/* aInsLin() -                                                         [scf] */
/*---------------------------------------------------------------------------*/

proc aInsLin ()
{
   INT  len;

   getParms ();
   if (p1 == 0)
      p1 = 1;
   if ((curLin >= scrRgnBeg) && (curLin <= scrRgnEnd))
   {
      len = scrRgnEnd - curLin + 1;
      if (p1 > len)
         p1 = len;
      pInsLin (scrRgnEnd, (INT) p1);
      curCol = 0;
   }
}


/*---------------------------------------------------------------------------*/
/* aDelChar() -                                                        [scf] */
/*---------------------------------------------------------------------------*/

proc aDelChar ()
{
   getParms ();
   if (p1 == 0)
      p1 = 1;
   pDelChar ((INT) p1);
}


/*---------------------------------------------------------------------------*/
/* VT-100 exceptions (same terminator for ANSI and non-Ansi sequence)        */
/*---------------------------------------------------------------------------*/

proc pVT100H ()
{
   if (termState == pAnsi)
      aCursor ();
   else
      pSetTab ();
   termState = NULL;
}

proc pVT100D ()
{
   if (termState == pAnsi)
      aCursLt ();
   else
      pLF ();
   termState = NULL;
}

proc pVT100M ()
{
   if (termState == pAnsi)
      aDelLin ();
   else
      pRevIndex ();
   termState = NULL;
}

proc pVT100c()
{
   if(termState == pAnsi)
      pInquire();                            /* mbbx 1.10: aReport() was wrong! */
   else
   {
      resetEmul();
      pClrScr();
   }
   termState = NULL;
}


proc pVT100P()                               /* mbbx: new routine */
{
   if(termState == pAnsi)
      aDelChar();
   else
      termState = pDCS;                      /* mbbx: yterm */
}


/*---------------------------------------------------------------------------*/
/* pDCS() -                                                                  */
/*---------------------------------------------------------------------------*/

proc pDCS()
{
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* pDCSTerminate() -                                                         */
/*---------------------------------------------------------------------------*/

proc pDCSTerminate()
{
   termState = NULL;
}


/*---------------------------------------------------------------------------*/
/* Ansi Parser Routines                                                      */
/*---------------------------------------------------------------------------*/

proc ansiArgument ()   /* chAscii IN ['0' .. '9'] */
{
   ansiArg = (ansiArg*10) + chAscii - '0';
}

proc ansiDelimiter ()  /* chAscii == ';' */
{
   if (argCount < MAXANSIARGS)
      argList[argCount++] = ansiArg;
   ansiArg = 0;
}

proc ansiHeathPrivate ()  /* chAscii == '>' */
{
   heathPrivate = TRUE;
}

proc ansiDecPrivate ()   /* chAscii == '?' */
{
   decPrivate = TRUE;
}


/*---------------------------------------------------------------------------*/
/* putRcvChar() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR writeRcvChar(BYTE  theChar)
{
LPSTR lpBuffer;                                                         /* rjs bugs 016 */
   lpBuffer = GlobalLock(xferBufferHandle);                             /* rjs bugs 016 */

   if((!xferSaveCtlChr) && ((theChar == BS) || (theChar == DEL)))	/* rjs bugs 016 */
      {
      // sdj: if the very first character in the 1k pkt happens to
      // sdj: be BS or DEL, then --xferBufferCount is going to be
      // sdj: 64K! causing the access violation.
      // sdj: lets not write the BS,DEL into the text file in these
      // sdj: cases because what needs to be changed is the last byte
      // sdj: of the last pkt which was written to the file! forget
      // sdj: that for the time being and avoid the access violation.
      if (xferBufferCount)
      lpBuffer[--xferBufferCount] = 0x00;				/* rjs bugs 016 */
      }
   else                                                                 /* rjs bugs 016 */
      lpBuffer[xferBufferCount++] = theChar;                            /* rjs bugs 016 */

   if (xferBufferCount == 1024)                                         /* rjs bugs 016 */
     if (_lwrite((INT)xferRefNo, (LPSTR)lpBuffer,(INT) 1024) != 1024)   /* rjs bugs 016 */
   {
      GlobalUnlock(xferBufferHandle);                                   /* rjs bugs 016 */
      xferEnd();

      TF_ErrProc(STREWRERR, MB_OK | MB_ICONHAND, 999);
      return(FALSE);
   }

      GlobalUnlock(xferBufferHandle);                                   /* rjs bugs 016 */
      if (xferBufferCount==1024) xferBufferCount = 0;                   /* rjs bugs 016 */

   if(( (xferLength += 1) % 0x0400 ) == 0L )                            /* rjs bugs 016 */
      showBBytes(xferLength, FALSE);

   return(TRUE);
}


VOID NEAR putRcvChar(BYTE  theChar)
{
   BYTE  the7Char = theChar & 0x7F;

   if(xferTableSave)
   {
      switch(the7Char)
      {
      case CR:
         tblPos = TBLBEGINLINE;
         break;

      case TAB:
         tblPos = TBLSPACES;
         return;

      case SP:
         if(tblPos < TBLSPACES)
            tblPos += 1;
         return;

      default:
         if(xferSaveCtlChr || (the7Char == FF) || (the7Char == LF) || 
            ((the7Char >= 0x20) && (the7Char <= 0x7E)))
         {
            switch(tblPos)
            {
            case TBLONESPACE:
               if(!writeRcvChar(SP))
                  return;
               break;
            case TBLSPACES:
               if(!writeRcvChar(TAB))
                  return;
               break;
            }

            tblPos = TBLNONSPACE;
         }
         break;
      }
   }

   if(xferSaveCtlChr || (the7Char == TAB) || (the7Char == FF) || (the7Char == LF) || 
      (the7Char == CR) || (the7Char == BS) || (the7Char == DEL) ||   /* rjs bugs 016 -> add BS and DEL */
      ((the7Char >= 0x20) && (the7Char <= 0x7E)))
   {
      writeRcvChar(theChar);
   }
}

/*---------------------------------------------------------------------------*/
/* checkTransPrint() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

#define TRANS_PRINT_BUFFER       4

STRING   transPrtBuf[TRANS_PRINT_BUFFER+1];

VOID NEAR checkTransPrint(BYTE  theChar)
{
   switch(theChar)
   {
   case 0x14:
      if((trmParams.emulate == ITMADDS) || (trmParams.emulate == ITMTVI925))
         transPrintFlag |= 0x8000;
      break;

   case '4':
      if((trmParams.emulate == ITMADDS) && (*transPrtBuf == 1))
         transPrintFlag = FALSE;
      break;

   case 'a':
      if((trmParams.emulate == ITMTVI925) && (*transPrtBuf == 1))
         transPrintFlag = FALSE;
      break;

   case 'i':
      if(((trmParams.emulate == ITMVT220) || (trmParams.emulate == ITMVT100)) && 
         (*transPrtBuf == 3) && (transPrtBuf[2] == '[') && (transPrtBuf[3] == '4'))
      {
         if(transPrintFlag)
         {
            transPrintFlag = FALSE;
            PrintFileComm(FALSE);
         }
      }
      break;
   }

   if(transPrintFlag || (prtFlag && !prtPause))
   {
      if(((theChar & 0x7F) != ESC) && !(transPrintFlag & 0x8000))
         transPrtBuf[++(*transPrtBuf)] = theChar;

      if(((theChar & 0x7F) == ESC) || (transPrintFlag & 0x8000) || (*transPrtBuf == 1) || (*transPrtBuf >= TRANS_PRINT_BUFFER))
      {
         if(*transPrtBuf > 0)
         {
            PrintFileString((LPSTR) transPrtBuf+1, (LONG) *transPrtBuf, FALSE);
            *transPrtBuf = 0;
         }

         if((theChar & 0x7F) == ESC)
         {
            transPrtBuf[++(*transPrtBuf)] = ESC;
            if((theChar == 0x9B) && (trmParams.emulate == ITMVT220))
               transPrtBuf[++(*transPrtBuf)] = '[';
         }

         if(transPrintFlag & 0x8000)
            transPrintFlag = FALSE;
      }
   }

   if(transPrintFlag && !prtFlag)
      PrintFileComm(TRUE);
}


/*---------------------------------------------------------------------------*/
/* modemInp () -                                                       [mbb] */
/*---------------------------------------------------------------------------*/

VOID modemInp(INT   theChar, BOOL  bRcvComm)
{
   cursorValid = FALSE;

   the8Char = (BYTE) theChar;

   if(bRcvComm)      /* mbbx 2.00: not restricted... */
   {
      if((xferFlag == XFRRCV) && !xferPaused)
         putRcvChar(the8Char);
   }

   if(transPrintFlag || (prtFlag && !prtPause))
      checkTransPrint(the8Char);

   chAscii = the8Char;                       /* ... mbbx 2.00: mask done in getMdmChar() */
   ch = chAscii;                             /* mbbx: patch */

   if((chAscii & 0x7F) == ESC)               /* mbbx 2.00: not masking 8th bit... */
   {
      if((the8Char & 0x80) && (trmParams.emulate == ITMVT220))    /* mbbx 1.10: CSI = ESC + '[' */
         pAnsiState();
      else
         termState = escHandler;
   }
   else if(termState)                        /* mbbx 2.00: do this AFTER checking for ESC... */
      (*termState)();
   else if((chAscii >= 0x20) && (chAscii != 0x7F))    /* mbbx 1.04: ics... */
   {
      putChar(chAscii);
      if(curCol < maxChars-1)
         curCol += 1;
      else if(trmParams.lineWrap)            /* mbbx 1.10: CUA... */
      {
         curCol = 0;
         pLF();
      }
   }
   else
      (*escHandler)();

}
