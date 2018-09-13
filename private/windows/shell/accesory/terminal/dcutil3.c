/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "video.h"


/*---------------------------------------------------------------------------*/
/* offCursor() - Turn off terminal emulation cursor.                  [scf]  */
/*---------------------------------------------------------------------------*/

VOID offCursor ()
{
   if (activTerm)
   {
      cursorOn--;
      if (cursorOn == 0)
      {
         if (cursBlinkOn)
            toggleCursor(&cursorRect);
      }
      else
         if(cursorOn < 0)
            cursorOn = 0;
   }
}


/*---------------------------------------------------------------------------*/
/* onCursor() - Turn on the terminal emulation cursor.                [scf]  */
/*---------------------------------------------------------------------------*/

VOID onCursor ()
{
   if (activTerm)
   {
      cursorOn++;
      if (cursorOn == 1)
      {
         cursorTick = 1;
         rectCursor (&cursorRect);
         cursBlinkOn = FALSE;
         toggleCursor (&cursorRect);
      }
      else
         if (cursorOn > 1)
            cursorOn = 1;
   }
}


/*---------------------------------------------------------------------------*/
/* rectCursor() - Set the terminal cursor rectangle.                  [scf]  */
/*---------------------------------------------------------------------------*/

VOID rectCursor(RECT *theRect)
{
   if(activCursor > 0)
   {
      if(statusLine)
         theRect->top = statusRect.top + (STATUSRECTBORDER / 2);
      else
         theRect->top = (savTopLine - curTopLine + curLin) * chrHeight;

      theRect->bottom = theRect->top + chrHeight;
      if(trmParams.termCursor != ITMBLKCURSOR)
         theRect->top += ((3 * chrHeight) / 4);
      else
         theRect->top++; /* jtf terminal changed to make cursor smaller */

      if(statusLine || (attrib[curLin][LATTRIB] == LNORMAL))
      {
         theRect->left  = (curCol - curLeftCol) * chrWidth;
         theRect->right = theRect->left + chrWidth;
      }
      else
      {
         theRect->left  = curCol * chrWidth * 2 - curLeftCol * chrWidth;
         theRect->right = theRect->left + chrWidth * 2;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* toggleCursor() - Toggle the terminal emulation cursor visible or not      */
/*---------------------------------------------------------------------------*/

VOID toggleCursor(RECT *theRect)
{
   RECT  clientRect;

   if(activCursor > 0)
   {
      getPort();

      cursBlinkOn = !cursBlinkOn;
                                             /* mbbx 1.04: for lack of a better method... */
      if(theRect->top == statusRect.top + (STATUSRECTBORDER / 2))
         clipRect(&statusRect);
      else
         clipRect(&hTE.viewRect);
      InvertRect(thePort, (LPRECT) theRect);

      releasePort();
   }
}


/*---------------------------------------------------------------------------*/
/* hideTermCursor                                                            */
/*---------------------------------------------------------------------------*/

VOID hideTermCursor ()
{
   if (activCursor == 1)
      offCursor ();
   activCursor--;
}

/*---------------------------------------------------------------------------*/
/* showTermCursor                                                            */
/*---------------------------------------------------------------------------*/

VOID showTermCursor ()
{
   activCursor++;
   if (activCursor == 1)
      onCursor ();
}


/*---------------------------------------------------------------------------*/
/* stripBlanks() - Purge unecessary CHFILL's for text file.            [scf] */
/*---------------------------------------------------------------------------*/

VOID stripBlanks (LPBYTE ptr, DWORD *len)
{
   register INT src;
   register INT dst;
   INT deltaLen;

   ptr[*len]     = 0;                     /* Terminate for 'C'            */
   for (src = (INT) *len; src > 0; src = dst - 1)
   {
      while ( (src > 0) && (ptr[src] != CR) ) /* jtf 3.11 fixed segmentation error */
         src--;
      dst = src - 1;
      while ((dst > 0) && (ptr[dst] == CHFILL) ) /* jtf 3.11 fixed segmentation error */
         dst--;
      dst++;
      deltaLen = src - dst;
      lmovmem (&ptr[src], &ptr[dst], (DWORD) (*len - src));
      *len -= (DWORD) deltaLen;
   }
   ptr[*len] = 0;
}


/*---------------------------------------------------------------------------*/
/* loadTermFont() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID loadTermFont(LOGFONT *logFont)
{
   HFONT       hOldFont;
   TEXTMETRIC  fontMetrics;
   INT         ndx;

   hOldFont = hTE.hFont;

   if((hTE.hFont = CreateFontIndirect((LPLOGFONT) logFont)) != hOldFont)
   {
      getPort();
      SelectObject(thePort, hTE.hFont);
      GetTextMetrics(thePort, (TEXTMETRIC FAR *) &fontMetrics);
      releasePort();

      stdChrHeight = chrHeight = fontMetrics.tmHeight;
      stdChrWidth  = chrWidth  = fontMetrics.tmMaxCharWidth; 

      for(ndx = 0; ndx < 256; ndx += 1)
         vidCharWidths[ndx] = chrWidth;

      if(hOldFont != NULL)
      {
         DeleteObject(hOldFont);
         clearFontCache();
         resetEmul();
         if(!IsIconic(hItWnd))   /* rjs bugs 015 */
            sizeTerm(0L);
      }
   }
}



/*---------------------------------------------------------------------------*/
/* nextFontSize() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

INT  APIENTRY  nextFontSize(LPLOGFONT lpLogFont, LPTEXTMETRIC lpTextMetrics, 
                            DWORD FontType, LPSTR lpData)
{
   INT   ndx, ndx2;

   if(*lpData != 0)
   {
      if (FontType & RASTER_FONTTYPE) 
      {
         for(ndx = 1; ndx <= 5; ndx += 1)
         {
            if((ndx2 = lpLogFont->lfHeight * ndx) > *lpData)
               break;
            lpData[ndx2] = lpLogFont->lfHeight;

            if(lpLogFont->lfQuality == PROOF_QUALITY)    /* no scaling... */
               break;
         }
      }
      else                                   /* vector font... */
         lsetmem(lpData+1, lpLogFont->lfHeight, *lpData);
   }
   else if(lpLogFont->lfHeight == lpData[1])
   {
      lmovmem((LPSTR) lpLogFont, lpData, sizeof(LOGFONT));

      return(FALSE);
   }

   return(TRUE);                             /* continue... */
}


/*---------------------------------------------------------------------------*/
/* listFontSizes() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID listFontSizes(BYTE *faceName, BYTE *sizeList, INT maxSize)
{
   FARPROC  lpFontProc;

   memset(sizeList+1, 0, *sizeList = maxSize);

   getPort();
   lpFontProc = MakeProcInstance((FARPROC) nextFontSize, hInst);
   EnumFonts(thePort, (LPSTR) faceName, lpFontProc, (LPARAM) sizeList);
   FreeProcInstance(lpFontProc);
   releasePort();
}


/*---------------------------------------------------------------------------*/
/* buildTermFont() -                                                   [mbb] */
/*---------------------------------------------------------------------------*/

VOID buildTermFont()                         /* mbbx 2.00: font selection... */
{
   BYTE     sizeList[64];
   LOGFONT  logFont;
   FARPROC  lpFontProc;

   if((hTE.hFont != NULL) && (trmParams.fontSize > 0))
   {
      strcpy(sizeList, trmParams.fontFace);
      AnsiUpper((LPSTR)sizeList);
      if(!strcmp(sizeList, "SYSTEM"))
         LoadString(hInst, STR_INI_FONTFACE, (LPSTR) trmParams.fontFace, LF_FACESIZE);

      listFontSizes(trmParams.fontFace, sizeList, 64-1);

      if((logFont.lfHeight = (sizeList[trmParams.fontSize] * 256)) != 0)
      {
         getPort();
         lpFontProc = MakeProcInstance((FARPROC) nextFontSize, hInst);
         EnumFonts(thePort, (LPSTR) trmParams.fontFace, lpFontProc, (LPARAM) &logFont);
         FreeProcInstance(lpFontProc);
         releasePort();
      }
   }
   else
      logFont.lfHeight = 0;

   if(LOBYTE(logFont.lfHeight) == 0)
   {
      logFont.lfEscapement     = 0;
      logFont.lfOrientation    = 0;
      logFont.lfWeight         = 0; /* rjs bugs 017 -> 0 to 200 */
      logFont.lfItalic         = FALSE;
      logFont.lfUnderline      = FALSE;
      logFont.lfStrikeOut      = FALSE;
      logFont.lfCharSet        = ANSI_CHARSET;
      logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
      logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
      logFont.lfQuality        = DEFAULT_QUALITY;
      logFont.lfPitchAndFamily = FF_DONTCARE;
      strcpy(logFont.lfFaceName, trmParams.fontFace);
   }

   logFont.lfHeight = trmParams.fontSize;
   logFont.lfWidth = 0;

   loadTermFont(&logFont);
}


/*---------------------------------------------------------------------------*/
/* clearFontCache() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID clearFontCache()                        /* mbbx 1.10 ... */
{
   INT   ndx;

   for(ndx = 0; ndx < VID_MAXFONTCACHE; ndx += 1)
   {
      if(vidFontCache[ndx].hFont != NULL)
      {
         DeleteObject(vidFontCache[ndx].hFont);
         vidFontCache[ndx].hFont = NULL;
         vidFontCache[ndx].flags = 0;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* calcDefaultAttrib() -                                               [jtf] */
/*---------------------------------------------------------------------------*/

VOID calcDefaultAttrib()                     /* mbbx 1.03 ... */
{
   INT   ndx;

   for(ndx = 3; ndx < 32; ndx += 1)          /* skip NORMAL, BOLD, REVERSE */
   {
      if((ndx == ABLINK) || (ndx == AUNDERLINE) || (ndx == APROTECT))
         continue;

      if(vidAttr[ndx].flags & VID_CALCTEXT)
      {
         vidAttr[ndx].text[VID_RED]   = 0;
         vidAttr[ndx].text[VID_GREEN] = 0;
         vidAttr[ndx].text[VID_BLUE]  = 0;
      }

      if(vidAttr[ndx].flags & VID_CALCBKGD)
      {
         vidAttr[ndx].bkgd[VID_RED]   = 0;
         vidAttr[ndx].bkgd[VID_GREEN] = 0;
         vidAttr[ndx].bkgd[VID_BLUE]  = 0;
      }

      if(vidAttr[ndx].flags & VID_CALCATTR)
         vidAttr[ndx].flags &= (VID_CALCTEXT | VID_CALCBKGD | VID_CALCATTR);

      if(ndx & ABOLD)
      {
         if(vidAttr[ndx].flags & VID_CALCTEXT)
         {
            vidAttr[ndx].text[VID_RED]   = min(vidAttr[ndx].text[VID_RED]   + vidAttr[ABOLD].text[VID_RED], 255);
            vidAttr[ndx].text[VID_GREEN] = min(vidAttr[ndx].text[VID_GREEN] + vidAttr[ABOLD].text[VID_GREEN], 255);
            vidAttr[ndx].text[VID_BLUE]  = min(vidAttr[ndx].text[VID_BLUE]  + vidAttr[ABOLD].text[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCBKGD)
         {
            vidAttr[ndx].bkgd[VID_RED]   = min(vidAttr[ndx].bkgd[VID_RED]   + vidAttr[ABOLD].bkgd[VID_RED], 255);
            vidAttr[ndx].bkgd[VID_GREEN] = min(vidAttr[ndx].bkgd[VID_GREEN] + vidAttr[ABOLD].bkgd[VID_GREEN], 255);
            vidAttr[ndx].bkgd[VID_BLUE]  = min(vidAttr[ndx].bkgd[VID_BLUE]  + vidAttr[ABOLD].bkgd[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCATTR)
            vidAttr[ndx].flags |= (vidAttr[ABOLD].flags & VID_MASK);
      }

      if(ndx & AREVERSE)
      {
         if(vidAttr[ndx].flags & VID_CALCTEXT)
         {
            vidAttr[ndx].text[VID_RED]   = min(vidAttr[ndx].text[VID_RED]   + vidAttr[AREVERSE].text[VID_RED], 255);
            vidAttr[ndx].text[VID_GREEN] = min(vidAttr[ndx].text[VID_GREEN] + vidAttr[AREVERSE].text[VID_GREEN], 255);
            vidAttr[ndx].text[VID_BLUE]  = min(vidAttr[ndx].text[VID_BLUE]  + vidAttr[AREVERSE].text[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCBKGD)
         {
            vidAttr[ndx].bkgd[VID_RED]   = min(vidAttr[ndx].bkgd[VID_RED]   + vidAttr[AREVERSE].bkgd[VID_RED], 255);
            vidAttr[ndx].bkgd[VID_GREEN] = min(vidAttr[ndx].bkgd[VID_GREEN] + vidAttr[AREVERSE].bkgd[VID_GREEN], 255);
            vidAttr[ndx].bkgd[VID_BLUE]  = min(vidAttr[ndx].bkgd[VID_BLUE]  + vidAttr[AREVERSE].bkgd[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCATTR)
            vidAttr[ndx].flags |= (vidAttr[AREVERSE].flags & VID_MASK);
      }

      if(ndx & ABLINK)
      {
         if(vidAttr[ndx].flags & VID_CALCTEXT)
         {
            vidAttr[ndx].text[VID_RED]   = min(vidAttr[ndx].text[VID_RED]   + vidAttr[ABLINK].text[VID_RED], 255);
            vidAttr[ndx].text[VID_GREEN] = min(vidAttr[ndx].text[VID_GREEN] + vidAttr[ABLINK].text[VID_GREEN], 255);
            vidAttr[ndx].text[VID_BLUE]  = min(vidAttr[ndx].text[VID_BLUE]  + vidAttr[ABLINK].text[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCBKGD)
         {
            vidAttr[ndx].bkgd[VID_RED]   = min(vidAttr[ndx].bkgd[VID_RED]   + vidAttr[ABLINK].bkgd[VID_RED], 255);
            vidAttr[ndx].bkgd[VID_GREEN] = min(vidAttr[ndx].bkgd[VID_GREEN] + vidAttr[ABLINK].bkgd[VID_GREEN], 255);
            vidAttr[ndx].bkgd[VID_BLUE]  = min(vidAttr[ndx].bkgd[VID_BLUE]  + vidAttr[ABLINK].bkgd[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCATTR)
            vidAttr[ndx].flags |= (vidAttr[ABLINK].flags & VID_MASK);
      }

      if(ndx & AUNDERLINE)
      {
         if(vidAttr[ndx].flags & VID_CALCTEXT)
         {
            vidAttr[ndx].text[VID_RED]   = min(vidAttr[ndx].text[VID_RED]   + vidAttr[AUNDERLINE].text[VID_RED], 255);
            vidAttr[ndx].text[VID_GREEN] = min(vidAttr[ndx].text[VID_GREEN] + vidAttr[AUNDERLINE].text[VID_GREEN], 255);
            vidAttr[ndx].text[VID_BLUE]  = min(vidAttr[ndx].text[VID_BLUE]  + vidAttr[AUNDERLINE].text[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCBKGD)
         {
            vidAttr[ndx].bkgd[VID_RED]   = min(vidAttr[ndx].bkgd[VID_RED]   + vidAttr[AUNDERLINE].bkgd[VID_RED], 255);
            vidAttr[ndx].bkgd[VID_GREEN] = min(vidAttr[ndx].bkgd[VID_GREEN] + vidAttr[AUNDERLINE].bkgd[VID_GREEN], 255);
            vidAttr[ndx].bkgd[VID_BLUE]  = min(vidAttr[ndx].bkgd[VID_BLUE]  + vidAttr[AUNDERLINE].bkgd[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCATTR)
            vidAttr[ndx].flags |= (vidAttr[AUNDERLINE].flags & VID_MASK);
      }

      if(ndx & APROTECT)
      {
         if(vidAttr[ndx].flags & VID_CALCTEXT)
         {
            vidAttr[ndx].text[VID_RED]   = min(vidAttr[ndx].text[VID_RED]   + vidAttr[APROTECT].text[VID_RED], 255);
            vidAttr[ndx].text[VID_GREEN] = min(vidAttr[ndx].text[VID_GREEN] + vidAttr[APROTECT].text[VID_GREEN], 255);
            vidAttr[ndx].text[VID_BLUE]  = min(vidAttr[ndx].text[VID_BLUE]  + vidAttr[APROTECT].text[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCBKGD)
         {
            vidAttr[ndx].bkgd[VID_RED]   = min(vidAttr[ndx].bkgd[VID_RED]   + vidAttr[APROTECT].bkgd[VID_RED], 255);
            vidAttr[ndx].bkgd[VID_GREEN] = min(vidAttr[ndx].bkgd[VID_GREEN] + vidAttr[APROTECT].bkgd[VID_GREEN], 255);
            vidAttr[ndx].bkgd[VID_BLUE]  = min(vidAttr[ndx].bkgd[VID_BLUE]  + vidAttr[APROTECT].bkgd[VID_BLUE], 255);
         }

         if(vidAttr[ndx].flags & VID_CALCATTR)
            vidAttr[ndx].flags |= (vidAttr[APROTECT].flags & VID_MASK);
      }
   }
}


/*---------------------------------------------------------------------------*/
/* setDefaultAttrib() -                                                [jtf] */
/*---------------------------------------------------------------------------*/

VOID setDefaultAttrib(BOOL bLoad)
{
   BYTE  szSection[MINRESSTR], szKeyWord[MINRESSTR], szDefault[MINRESSTR];
   BYTE  work[16];
#ifdef ORGCODE
   INT   text[3], bkgd[3];
#else
   DWORD text[3], bkgd[3];         // -sdj 10/8/91 bug#3279
#endif
   INT   ndx;

   clearFontCache();


   LoadString(hInst, STR_INI_COLORS, (LPSTR) szSection, MINRESSTR);
   LoadString(hInst, STR_INI_WINDOWTEXT, (LPSTR) szKeyWord, MINRESSTR);
   LoadString(hInst, STR_INI_TEXTCOLOR, (LPSTR) szDefault, MINRESSTR);
   GetProfileString((LPSTR) szSection, (LPSTR) szKeyWord, (LPSTR) szDefault, (LPSTR) work, 16-1);

   sscanf(work, "%d %d %d", text+0, text+1, text+2);

   LoadString(hInst, STR_INI_WINDOW, (LPSTR) szKeyWord, MINRESSTR);
   LoadString(hInst, STR_INI_BKGDCOLOR, (LPSTR) szDefault, MINRESSTR);
   GetProfileString((LPSTR) szSection, (LPSTR) szKeyWord, (LPSTR) szDefault, (LPSTR) work, 16-1);

   sscanf(work, "%d %d %d", bkgd+0, bkgd+1, bkgd+2);

   for(ndx = 0; ndx < 32; ndx += 1)

   {
// -sdj 10/8/91 bug#3279 remove the hardcoded color stuff, new crt libs are in

      vidAttr[ndx].text[VID_RED]   = (BYTE)text[VID_RED];
      vidAttr[ndx].text[VID_GREEN] = (BYTE)text[VID_GREEN];
      vidAttr[ndx].text[VID_BLUE]  = (BYTE)text[VID_BLUE];

      vidAttr[ndx].bkgd[VID_RED]   = (BYTE)bkgd[VID_RED];
      vidAttr[ndx].bkgd[VID_GREEN] = (BYTE)bkgd[VID_GREEN];
      vidAttr[ndx].bkgd[VID_BLUE]  = (BYTE)bkgd[VID_BLUE];
      vidAttr[ndx].flags = VID_CALCATTR | VID_CALCBKGD | VID_CALCTEXT;
   }

   vidAttr[ANORMAL].flags    = 0;
   vidAttr[ABOLD].flags      = VID_BOLD;
   vidAttr[AREVERSE].flags   = VID_REVERSE;
   vidAttr[ABLINK].flags     = VID_BOLD;
   vidAttr[AUNDERLINE].flags = VID_UNDERLINE;
   vidAttr[APROTECT].flags   = VID_REVERSE;

   calcDefaultAttrib();
}
