/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "video.h"


/*---------------------------------------------------------------------------*/
/* eraseColorRect() - use current attribute to erase area              [jtf] */
/*---------------------------------------------------------------------------*/

VOID eraseColorRect(HDC hDC, LPRECT rect, BYTE cAttr)
{
   HBRUSH   hTmpBrush;

   hTmpBrush = CreateSolidBrush(RGB(vidAttr[cAttr & AMASK].bkgd[VID_RED], 
                                    vidAttr[cAttr & AMASK].bkgd[VID_GREEN], 
                                    vidAttr[cAttr & AMASK].bkgd[VID_BLUE]));
   FillRect(hDC, (LPRECT) rect, (HBRUSH) hTmpBrush);
   DeleteObject((HBRUSH) hTmpBrush);
}


/*---------------------------------------------------------------------------*/
/* reDrawTermScreen() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

VOID reDrawTermScreen(INT nStart, INT nCount, INT nOffset)
{
   while(nStart < nCount)
      reDrawTermLine((nStart++) + nOffset, 0, maxChars);
}


/*---------------------------------------------------------------------------*/
/* reDrawTermLine() - Reslap some lines back up on the terminal.  [scf]      */
/*---------------------------------------------------------------------------*/

VOID reDrawTermLine(INT lin, INT col, INT len)
{
   LPBYTE   lpText;

   if(lin < 0)
   {
      getPort();
      hideTermCursor();

      if((lpText = GlobalLock(hTE.hText)) != NULL)
      {
         drawTermLine(lpText + ((savTopLine + lin) * (maxChars + 2)) + col,
                      len, lin, col + len, LNORMAL, 0);
         GlobalUnlock(hTE.hText);
      }
      showTermCursor();
      releasePort();
   }
   else if(lin <= (maxScreenLine+1))         /* mbbx 2.00.06: jtf disp2 */
   {
      attrib[lin][LFLAGS] = (LCLEAR | LDIRTY);
      updateLine(lin);
   }
}


/*---------------------------------------------------------------------------*/
/* updateLine() -                                                            */
/*---------------------------------------------------------------------------*/

VOID updateLine(INT line)
{
   BOOL     bStatusLine;                     /* mbbx 2.00.06: jtf disp2 */
   LPBYTE   txt;
   BYTE     *attr;
   INT	    col1, col2, len, txtLen; // sdj: unref variable was- bottom;
   BYTE     cAttr;
   BOOL     anyway;

                                             /* mbbx 2.00.06: dont overwrite xferCtrls... */
   if((bStatusLine = (line == maxScreenLine+1)) && (xferFlag > XFRNONE))
      return;

   getPort();
   hideTermCursor();

   if(!bStatusLine)
   {
      clipRect(&hTE.viewRect); /* jtf 3.Final */
      txt = (LPBYTE)GlobalLock(hTE.hText) + (savTopLine + line) * (maxChars + 2);
   }
   else
   {
      clipRect(&statusRect);                 /* rjs bugs 019 */
      txt = (LPBYTE) line25;
   }

   attr     = attrib[line];
   anyway   = (attr[LFLAGS] == (LCLEAR | LDIRTY));

   for(col1 = 0; col1 < maxChars; col1 = col2)
   {
      cAttr = attr[col1];
      for (col2 = col1+1; col2 < maxChars; col2++)
      {
         attr[col2-1] &= ~ADIRTY;
         if (cAttr != attr[col2])
            break;
      }
      len    = col2 - col1;
      txtLen = len;
      if((cAttr & ADIRTY) || anyway)
         drawTermLine(txt, txtLen, line, col1 + txtLen, attr[LATTRIB], cAttr);

      txt   += len;
   }
   attr[col2-1] &= ~ADIRTY;
   attr[LFLAGS]  =  0;

   if(!bStatusLine)
      GlobalUnlock(hTE.hText);

   showTermCursor();
   releasePort();
}


/*---------------------------------------------------------------------------*/
/* drawTermLine() -                                                          */
/*---------------------------------------------------------------------------*/

VOID drawTermLine(LPBYTE txtPtr, INT len, INT lin, INT col, BYTE lAttr, BYTE cAttr)
{
   BOOL     bStatusLine;
   HDC      savePort;
   HBITMAP  hBitMap;
   RECT     srcRect, dstRect, tmpRect;
   INT      ndx;
   BYTE     grSave[132], grChar[132];        
   HPEN     newPen, oldPen;
   INT	    grLeft, grWidth2, grHeight2;
   BOOL     bhBitMapValid;

   bhBitMapValid = FALSE;  //sdj: there is a chance that hbitmap maynot
			   //sdj: be valid and still deleteobject(hbitmap)
			   //sdj: is called, and this may delete some random
			   //sdj: object, who knows, so lets delete it only
			   //sdj: if you create one

   if(!(bStatusLine = (lin == maxScreenLine+1)))
   {
      if(((lin += (savTopLine - curTopLine)) < 0) || (lin > visScreenLine))
         return;
   }

   if(lAttr != LNORMAL)
   {
      thePort = CreateCompatibleDC(savePort = thePort);
      hBitMap = CreateCompatibleBitmap(thePort, chrWidth * maxChars, chrHeight);
      bhBitMapValid = TRUE;
      SelectObject(thePort, hBitMap);
      SelectObject(thePort, hTE.hFont);

      /* srcRect : rect to draw chars in (memory DC) */
      srcRect.left   = 0;
      srcRect.right  = len * chrWidth;
      srcRect.top    = 0;
      srcRect.bottom = chrHeight;

      /* dstRect : rect to copy bits to (stretchBlt) */
      dstRect.right = col*chrWidth*2 - curLeftCol*chrWidth;
      dstRect.left  = dstRect.right - len*chrWidth*2;
      dstRect.top   = lin * chrHeight;

      if(lAttr == LHIGHBOTTOM)
         dstRect.top -= chrHeight;

      if((dstRect.bottom = dstRect.top + chrHeight) >= statusRect.top)
         dstRect.bottom = statusRect.top-1;  /* mbbx 2.00.06: wont this look FUNKY ??? */
      if(lAttr != LWIDE)
         dstRect.bottom += chrHeight;

      tmpRect = dstRect;
      eraseColorRect((HDC) thePort, (LPRECT) &tmpRect, (BYTE) cAttr);
   }
   else
   {
      srcRect.right = (col - curLeftCol) * chrWidth;
      srcRect.left = srcRect.right - (len * chrWidth);

                                             /* mbbx 2.00.06: jtf disp2... */
      srcRect.top = !bStatusLine ? (lin * chrHeight) : statusRect.top + (STATUSRECTBORDER / 2);
      srcRect.bottom = srcRect.top + chrHeight;
   }

   if(cAttr & AGRAPHICS)
   {
      lmovmem(txtPtr, (LPSTR) grSave, len);  /* mbbx 2.00.06: save actual buffer data */

      for(ndx = 0; ndx < len; ndx += 1)
      {
         if((txtPtr[ndx] >= 0x40) && (txtPtr[ndx] <= 0x7F))
         {
            grChar[ndx] = vidGraphChars[txtPtr[ndx] - 0x40].display;
            txtPtr[ndx] = vidGraphChars[txtPtr[ndx] - 0x40].buffer;
         }
         else
            grChar[ndx] = 0;
      }
   }

   ndx = 0;
   if(!(vidAttr[cAttr & AMASK].flags & VID_UNDERLINE) && !bStatusLine)  /* mbbx 2.00.06: jtf disp2... */
   {
      while((txtPtr[ndx] == ' ') && (ndx < len))
         ndx += 1;
   }

   if(ndx > 0)
   {
      CopyRect((LPRECT) &tmpRect, (LPRECT) &srcRect);
      tmpRect.right = tmpRect.left + (ndx * chrWidth);
      if(tmpRect.bottom >= statusRect.top)   /* mbbx 2.00.06: jtf disp2 - dont overwrite statusRect... */
         tmpRect.bottom = statusRect.top-1;
      eraseColorRect((HDC) thePort, (LPRECT) &tmpRect, (BYTE) cAttr);
   }

   if((len - ndx) > 0)
   {
      setAttrib(cAttr);
      ExtTextOut(thePort, srcRect.left + (ndx * chrWidth), srcRect.top, ETO_CLIPPED, (LPRECT) &srcRect, 
                 (LPSTR) &txtPtr[ndx], len-ndx, (LPINT) vidCharWidths);
   }

   if(cAttr & AGRAPHICS)
   {
      newPen = CreatePen(0, 1, RGB(vidAttr[cAttr & AMASK].text[VID_RED], 
                         vidAttr[cAttr & AMASK].text[VID_GREEN], 
                         vidAttr[cAttr & AMASK].text[VID_BLUE]));
      oldPen = SelectObject(thePort, (HPEN) newPen);

      grLeft = srcRect.left;
      grWidth2 = chrWidth / 2;
      grHeight2 = chrHeight / 2;

      for(ndx = 0; ndx < len; ndx += 1)
      {
         if(grChar[ndx] & VID_DRAW_LEFT)
         {
            MMoveTo(thePort, grLeft, srcRect.top + grHeight2);
            LineTo(thePort, grLeft + grWidth2, srcRect.top + grHeight2);
         }

         if(grChar[ndx] & VID_DRAW_RIGHT)
         {
            MMoveTo(thePort, grLeft + grWidth2, srcRect.top + grHeight2);
            LineTo(thePort, grLeft + chrWidth, srcRect.top + grHeight2);
         }

         if(grChar[ndx] & VID_DRAW_TOP)
         {
            MMoveTo(thePort, grLeft + grWidth2, srcRect.top);
            LineTo(thePort, grLeft + grWidth2, srcRect.top + grHeight2);
         }

         if(grChar[ndx] & VID_DRAW_BOTTOM)
         {
            MMoveTo(thePort, grLeft + grWidth2, srcRect.top + grHeight2);
            LineTo(thePort, grLeft + grWidth2, srcRect.top + chrHeight);
         }

         if(grChar[ndx] & VID_DRAW_SCAN1)
         {
            MMoveTo(thePort, grLeft, srcRect.top);
            LineTo(thePort, grLeft + chrWidth, srcRect.top);
         }

         if(grChar[ndx] & VID_DRAW_SCAN3)
         {
            MMoveTo(thePort, grLeft, srcRect.top + (chrHeight / 4));
            LineTo(thePort, grLeft + chrWidth, srcRect.top + (chrHeight/4));
         }

         if(grChar[ndx] & VID_DRAW_SCAN7)
         {
            MMoveTo(thePort, grLeft, srcRect.top + (3 * (chrHeight / 4)));
            LineTo(thePort, grLeft + chrWidth, srcRect.top + (3 * (chrHeight/4)));
         }

         if(grChar[ndx] & VID_DRAW_SCAN9)
         {
            MMoveTo(thePort, grLeft, srcRect.top + (chrHeight-1));
            LineTo(thePort, grLeft + chrWidth, srcRect.top + (chrHeight-1));
         }

         grLeft += chrWidth;
      }

      SelectObject(thePort, (HPEN) oldPen);
      DeleteObject((HPEN) newPen);

      lmovmem((LPSTR) grSave, txtPtr, len);  /* mbbx 2.00.06: restore actual buffer data */
   }

   if(vidAttr[cAttr & AMASK].flags & VID_REVERSE)
      InvertRect(thePort, (LPRECT) &srcRect);

   if(lAttr != LNORMAL)
   {
                                             /* mbbx 2.00: font selection... */
      StretchBlt(savePort, dstRect.left, dstRect.top, 
                 dstRect.right - dstRect.left, dstRect.bottom - dstRect.top, 
                 thePort, srcRect.left, srcRect.top, 
                 srcRect.right - srcRect.left, chrHeight, SRCCOPY);

      DeleteDC(thePort);
      if (bhBitMapValid)	  //sdj: check if valid, only then call delete
	  DeleteObject(hBitMap);
      thePort = savePort;
   }

   if(vidAttr[cAttr & AMASK].flags & (VID_BOLD | VID_ITALIC | VID_UNDERLINE | VID_STRIKEOUT))
      SelectObject(thePort, hTE.hFont);
}


/*--------------------------------------------------------------------------*/
/* setAttrib() -                                                      [jtf] */
/*--------------------------------------------------------------------------*/

VOID setAttrib(BYTE cAttr)
{
   INT      ndx;
   LOGFONT  logFont;
   HFONT    hFont;

   cAttr &= AMASK;

   SetTextColor(thePort, RGB(vidAttr[cAttr].text[VID_RED], 
                vidAttr[cAttr].text[VID_GREEN], vidAttr[cAttr].text[VID_BLUE]));

   SetBkColor(thePort, RGB(vidAttr[cAttr].bkgd[VID_RED], 
              vidAttr[cAttr].bkgd[VID_GREEN], vidAttr[cAttr].bkgd[VID_BLUE]));

   if(vidAttr[cAttr].flags & (VID_BOLD | VID_ITALIC | VID_UNDERLINE | VID_STRIKEOUT))
   {
      for(ndx = 0; ndx < VID_MAXFONTCACHE; ndx += 1)
      {
         if(vidFontCache[ndx].hFont == NULL)
            break;

         if(vidFontCache[ndx].flags == (vidAttr[cAttr].flags & VID_MASK))
         {
            SelectObject(thePort, vidFontCache[ndx].hFont);
            return;
         }
      }

      GetObject(hTE.hFont, sizeof(LOGFONT), (LPSTR) &logFont);

      if(vidAttr[cAttr].flags & VID_BOLD)
         logFont.lfWeight = 700;

      logFont.lfUnderline = (vidAttr[cAttr].flags & VID_UNDERLINE) ? TRUE : FALSE;
      logFont.lfStrikeOut = (vidAttr[cAttr].flags & VID_STRIKEOUT) ? TRUE : FALSE;
      logFont.lfItalic = (vidAttr[cAttr].flags & VID_ITALIC) ? TRUE : FALSE;

      hFont = CreateFontIndirect((LPLOGFONT) &logFont);
      if(ndx == VID_MAXFONTCACHE)
      {
         DeleteObject(vidFontCache[0].hFont);
         for(ndx = 0; ndx < VID_MAXFONTCACHE-1; ndx += 1)
         {
            vidFontCache[ndx].hFont = vidFontCache[ndx+1].hFont;
            vidFontCache[ndx].flags = vidFontCache[ndx+1].flags;
         }
      }

      vidFontCache[ndx].hFont = hFont;
      vidFontCache[ndx].flags = (vidAttr[cAttr].flags & VID_MASK);
      SelectObject(thePort, hFont);
   }
}
