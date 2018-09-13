/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/


/*
 *****
 ***** calprint.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS

#include "cal.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

SHORT TranslateString(CHAR *);
CHAR * APIENTRY PFileInPath(CHAR *sz);
VOID   APIENTRY DestroyAbortWnd(VOID);

/* We'll dynamically allocate this */
HANDLE   hHeadFoot=NULL;
LPSTR    szHeadFoot;
SHORT    xCharPage, dyHeadFoot;
SHORT    dyTop,dyBottom,dxLeft ,dxRight;
SHORT    CharPrintWidth, CharPrintHeight;
HDC      vhDCPrint;
BOOL     vfAbortPrint;
HWND     vhwndAbortPrint;

INT      vcyPrintLineToLine;
INT      xPrintRes, yPrintRes, xPixInch, yPixInch;
INT      hChars;
INT      vclnPage;
INT      nSpace;
INT      vclnPrinted;
INT      vclnDate;
INT      vlnFooter;         /* footer line */
INT      vlnBottom;         /* last line of calendar text */
INT      vlnTop;            /* first line of calendar text */
BOOL     vfPrint;
CHAR     szCurDateBuf[9];   /* buffer containing date string for header/footer */
CHAR     *szcurDptr = szCurDateBuf;
INT      iCurDateLength ;
DOSDATE  CurDD;

CHAR     *vpchPrintBuf;

#define CLNHEADING 1
#define CLNAFTERHEADING 1
#define CLNAFTERAPPOINTMENTS 1
#define CLNBETWEENDATES 2

/* 1 - blank or asterisk for alarm
   1 - blank
   CCHTIMESZ - appointment time (includes 0 terminator, which is used
    to hold a blank here)
   CCHQDMAX - room for a maximum length appointment description
   1 - room for the 0 terminator

   The print buffer is also used for outputting the heading, so it
   must be long enough for that too.  To make sure this is the case,
   add in CCHDATEDISP.
*/

#define CCHPRINTBUF (1 + 1 + CCHTIMESZ + CCHQDMAX + 1 + CCHDATEDISP)



/* Format of a printed date:
   if not at top of page, CLNBETWEENDATES blank lines
   Heading (e.g., Thursday, July 11, 1985)
   CLNAFTERHEADING blank lines
   Appointments (e.g., * 10:00 Call Tandy to report progress (asterisk
    indicates alarm set)
   CLNAFTERAPPOINTMENTS blank lines
   notes
 */




/**** FnPrint ****/

BOOL APIENTRY FnPrint (
     HWND hwnd,
     WORD message,
     WPARAM wParam,
     LONG lParam)
     {
     CHAR szFromDate [CCHDASHDATE];

     switch (message)
	  {
	  case WM_INITDIALOG:
	       /* Remember the window handle of the dialog for AlertBox. */
	       vhwndDialog = hwnd;

               GetDashDateSel (szFromDate);
	       SetDlgItemText (hwnd, IDCN_FROMDATE, (LPSTR)szFromDate);
	       return (TRUE);

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		    case IDOK:
			 GetRangeOfDates (hwnd);
			 /* line added to fix keyboard hanging when Calendar
			    is run under ver3.0 rel 1.11 */
			 CalSetFocus (GetDlgItem(hwnd, IDCN_FROMDATE));
			 break;

		    case IDCANCEL:
			 EndDialog (hwnd, FALSE);
			 break;
		    }

	       return (TRUE);
	  }

     /* Tell Windows we did not process the message. */
     return (FALSE);
     }




/**** Print */

VOID FAR APIENTRY Print ()
     {
     TEXTMETRIC     Metrics;
     WORD  idrFree;
     INT  itdd;

     DD   *pdd;
     DT   dt;
     DL   dl;
     WORD  idr;
     CHAR rgchPrintBuf [CCHPRINTBUF];
     INT iDateLen;
     INT        iHeight;
     INT        iWidth;

     /* Note - there is no need to force edits to be recorded here.
	We got here after the Print dialog, which took over the focus
	and therefore has recorded the edits.  We are not going to
	modify the current DR, so we do not need to set the focus
	to NULL (as we sometimes do to prevent recording data into
	the wrong DR).
     */


     if (BeginPrint () < 0)
	  {
	  /* Unable to get print DC - display error and give up. */
	  CalPrintAlert(SP_ERROR);
	  return;
	  }

     /* Determine the number of lines per page. */
     GetTextMetrics (vhDCPrint, (LPTEXTMETRIC)(&Metrics));
     CharPrintHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
     CharPrintWidth = (Metrics.tmAveCharWidth + Metrics.tmMaxCharWidth)/2;             /* character width */

     xPrintRes = GetDeviceCaps(vhDCPrint, HORZRES);
     yPrintRes = GetDeviceCaps(vhDCPrint, VERTRES);
     xPixInch  = GetDeviceCaps(vhDCPrint, LOGPIXELSX);
     yPixInch  = GetDeviceCaps(vhDCPrint, LOGPIXELSY);

     dyHeadFoot = yPixInch / 2;                  /* 1/2 an inch */

     dyTop      = atopix(chPageText[4], yPixInch);
     dyBottom   = atopix(chPageText[5], yPixInch);
     dxLeft     = atopix(chPageText[2], xPixInch);
     dxRight    = atopix(chPageText[3], xPixInch);

/* There's some recalculating here entirely unneeded.  The call to
   GetDeviceCaps for hChars isn't needed since xPrintRes already has
   that value.  CharPrintHeight already has the value desired for
   vcyPrintLineToLine.  (I'd pull one of them out entirely, but that
   requres a lot of code review without the time.)  The call to
   GetDeviceCaps for vlnFooter isn't needed since yPrintRes already
   has that value.                           21 Sept 1989  Clark Cyr */
#if 0
     hChars=GetDeviceCaps(vhDCPrint, HORZRES)/Metrics.tmAveCharWidth;
     vcyPrintLineToLine = Metrics.tmHeight + Metrics.tmExternalLeading;
     vlnFooter = GetDeviceCaps(vhDCPrint, VERTRES) / vcyPrintLineToLine - 1;
#endif
     hChars = xPrintRes / Metrics.tmAveCharWidth;
     vlnFooter = yPrintRes / (vcyPrintLineToLine = CharPrintHeight) - 1;

     MGetTextExtent(vhDCPrint, " ", 1, &iHeight, &iWidth);
     nSpace = iWidth;

     viLeftMarginLen =dxLeft/nSpace;
     viRightMarginLen=dxRight/nSpace;
     viTopMarginLen  =dyTop/vcyPrintLineToLine;
     viBotMarginLen  =dyBottom/vcyPrintLineToLine;

     /* Number of characters between margins */
     xCharPage=(xPrintRes/CharPrintWidth)-viLeftMarginLen-viRightMarginLen;


     /* Allocate memory for the header.footer string.  Will allow any size
      * of paper and still have enough for the string.
      */
     hHeadFoot=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)xCharPage+2);

     if (!hHeadFoot)
         {
         /* Tell user that there's not memory to do this... */
         CalPrintAlert(SP_OUTOFMEMORY);
         return;
         }



/* Change 2 to 3.  Leave room for footer itself.  21 Sept 1989 Clark Cyr */
     vclnPage  = vlnFooter - 3;
     vlnTop    = viTopMarginLen;
     vlnBottom = vclnPage - viBotMarginLen;

     CurDD.dayofweek = 0xff;
     CurDD.year =  vd3Cur.wYear + 1980;
     CurDD.month = vd3Cur.wMonth + 1;
     CurDD.day = vd3Cur.wDay + 1;
     iDateLen = GetDateString(&CurDD, szCurDateBuf, GDS_LONG|GDS_DAYOFWEEK);
     *(szcurDptr + iDateLen) = '\0';

     /* Find a free DR in case we need to read from the disk. */
     idrFree = IdrFree ();

     /* Say we are at the top of the page. */
     /* vclnPrinted = 0; */
     /* print out header string */

     viCurrentPage = 1;
     vpchPrintBuf = rgchPrintBuf;
     PrintHeaderFooter(TRUE);

     vclnPrinted = vlnTop;

     for (itdd = vitddFirst; !vfAbortPrint && itdd < vitddMax; itdd++)
	  {
	  pdd = TddLock () + itdd;
	  dt = pdd -> dt;
	  dl = pdd -> dl;
	  idr = pdd -> idr;
	  TddUnlock ();

	  if (idr == IDRNIL)
	       {
	       /* The date is not in memory, see if it's on disk. */
	       if (dl == DLNIL)
		    {
		    /*
		       Not on disk either - this is an empty DD.  Skip
		       over this date.
		    */
		    continue;
		    }

	       /* Read the date from disk into the free DR. */
	       ReadTempDr (idr = idrFree, dl);
	       }

	  /* Calculate how many lines are needed to print this date. */
	  if (!PrintDate (idr, dt, FALSE))
	      return;
	  if (vclnDate == 0)
	       {
	       /* There's nothing to print in this date (must just
		  be special times without alarms or text and no notes).
	       */
	       continue;
	       }

/* Change 0 to vlnTop + 2.   21 Sept 1989   Clark Cyr */
	  if (vclnPrinted > vlnTop + 2)
	       {
	       /* Not at top of page - see if this entire date will fit
		  on the remainder of this page.  If it won't fit, start
		  a new page.
	       */
	       /* print out the footer if bottom of page is reached */

	       if (vclnPrinted + vclnDate > vlnBottom ){

		    PrintHeaderFooter( FALSE );
		    if (!NewPage ())
			return;
		    viCurrentPage++;

		    /* print out the header on top of new page */
		    PrintHeaderFooter( TRUE );
		    vclnPrinted = vlnTop;
	       }
	       }
	  /* Print the schedule for the date. */
	  if (!PrintDate (idr, dt, TRUE))
	      return;
	  }
     /* print out the footer if bottom of page is reached */

    if (vclnPrinted < vlnFooter - 3)
	PrintHeaderFooter(FALSE);

     if (NewPage ())
	 EndPrint ();


     GlobalFree(hHeadFoot);
     hHeadFoot=NULL;
     }



/*
 * convert floating point strings (like 2.75 1.5 2) into number of pixels
 * given the number of pixels per inch
 */

INT atopix(CHAR *ptr, INT pix_per_in)
    {
    CHAR *dot_ptr;
    CHAR sz[20];
    INT decimal;

    lstrcpy(sz, ptr);

    dot_ptr = strchr(sz, szDec[0]);

    if (dot_ptr)
        {
    	*dot_ptr++ = 0;		/* terminate the inches */
        if (*(dot_ptr + 1) == 0)
            {
            *(dot_ptr + 1) = '0';   /* convert decimal part to hundredths */
            *(dot_ptr + 2) = 0;
            }
	decimal = ((INT)atol(dot_ptr) * pix_per_in) / 100;	/* first part */
        }
    else
    	decimal = 0;		/* there is not fraction part */

    return ((INT)atol(sz) * pix_per_in) + decimal;     /* second part */
    }




/**** PrintDate - Print the specified date.  If fPrint == FALSE then
      don't actually print - just set up vclnDate with the number of
      lines required to print the date.
*/

BOOL APIENTRY PrintDate (
     INT      idr,
     DT       dt,
     BOOL     fPrint)
     {
     CHAR     rgchPrintBuf [CCHPRINTBUF];
     CHAR     szTemp[CCHPRINTBUF];
     DR       *pdr;
     register PQR pqr;
     PQR	pqrMax;
     CHAR     *pchTemp;
     register CHAR *pchSrc;
     CHAR     *pchDest;
     CHAR     c;
     BOOL     fSameLine;
     INT      cchTime, i, nx;
     BOOL     fFirstLine;
     BOOL     fFirstPM;

     /* Set up global print flag for routines to be called. */
     vfPrint = fPrint;

     /* Set up a global pointer to the print buffer. */
     vpchPrintBuf = rgchPrintBuf;

     /* Initialize the count of lines required to print this date. */
     vclnDate = 0;

     pdr = PdrLock (idr);
     fFirstLine = TRUE;
     fFirstPM = TRUE;
     for (pqrMax = (PQR)((BYTE UNALIGNED*)(pqr = (PQR)PbTqrFromPdr(pdr)) +
	pdr->cbTqr); pqr<pqrMax; pqr = (PQR)((BYTE UNALIGNED*)pqr + pqr->cb))
	  {
	  /* Don't print special times that don't have an alarm
	     or an appointment description.
	  */
	  if (pqr -> fAlarm || pqr -> cb != CBQRHEAD)
	       {
	       if (!PrintHeading (dt))
		  goto error0;

	       FillBuf (vpchPrintBuf, CCHPRINTBUF, ' ');
	       if (pqr -> fAlarm)
		    *(vpchPrintBuf+viLeftMarginLen) = '*';

	       cchTime = GetTimeSz (pqr -> tm, vpchPrintBuf + viLeftMarginLen + 2);

	       /* Print am/pm strings only for first line printed and noon */
	       if (!(fFirstLine || ((fFirstPM && pqr->tm >= TMNOON))))
		   FillBuf (vpchPrintBuf+viLeftMarginLen+5+2, cchTime-5, ' ');

	       FillBuf (vpchPrintBuf, viLeftMarginLen, ' '); /* pad margin space
							      with blanks */
	       pchTemp = vpchPrintBuf + viLeftMarginLen + cchTime+2;
	       *pchTemp++ = ' ';
	       *pchTemp = '\0';
	       if (pqr -> cb > CBQRHEAD)
		    lstrcpy (pchTemp, (CHAR *)((BYTE *)pqr + CBQRHEAD));


CheckMarg:
               if (lstrlen(vpchPrintBuf) > (hChars-viRightMarginLen))
                   {
                   lstrcpy(szTemp, vpchPrintBuf+hChars-viRightMarginLen);
                   vpchPrintBuf[hChars-viRightMarginLen]=0;

                   if (!PrintLine ())
                       goto error0;

                   /* CHeck for one space at the beginning and strip if needed */
                   nx=0;
                   if (szTemp[0]==' ' && szTemp[1]!=' ')
                       nx=1;

                   /* Note that 11 = strlen[* + Time + AM/PM + Space] */

                   FillBuf (vpchPrintBuf, viLeftMarginLen+11-nx, ' ');
                   vpchPrintBuf[viLeftMarginLen+11-nx]=0;
                   lstrcat(vpchPrintBuf, szTemp);
                   goto CheckMarg;
                   }

	       if (!PrintLine ())
		    goto error0;
	       fFirstLine = FALSE;
	       if (pqr->tm >= TMNOON)
		   fFirstPM = FALSE;
	       }
	  }

     if (pdr -> cbNotes != 0)
	  {
          if (vclnDate != 0)
              {
              if (!PrintBlankLn (CLNAFTERAPPOINTMENTS))
                   goto error0;
              }
          else
              if (!PrintHeading (dt))
                 goto error0;

	  /* The notes are split into lines as follows:
	     '\0' terminates a line and terminates the notes.
	     <CR,LF> is a hard line break (user typed Enter key).
	     <CR,CR,LF> is a soft line break (caused by word wrap).
	     In order to do something reasonable no matter what is seen,
	     this code skips an abitrary number of CRs followed by an
	     arbitrary number of LFs.
	  */
	  pchSrc = (CHAR *)((BYTE *)pdr + CBDRHEAD);
	  while (*pchSrc != '\0')
               {
	       pchDest = rgchPrintBuf;
	       fSameLine = TRUE;

	       i=0;		  /* fill margin space with blanks */
               while (i<viLeftMarginLen)
                  {
		  *pchDest++ = ' ';
		  i++;
                  }

	       while (fSameLine)
                    {
                    c=*pchSrc;
#ifdef DBCS
                    if( IsDBCSLeadByte(*pchSrc) )
                        *pchDest++ = *pchSrc++;
                    *pchDest++ = *pchSrc++;
#else
                    *pchDest++ = *pchSrc++;
#endif

                    if (c=='\r')
                        {
                        /* Eat multiple CRs if present. */
                        while (*pchSrc == '\r')
                             pchSrc++;

                        /* Eat line feeds following carriage return if
                           there are any.
                        */
                        while (*pchSrc == '\n')
                             *pchSrc++;
                        /* Terminate the line. */
                        *(pchDest - 1) = 0;

                        fSameLine = FALSE;
                        }
                    else
                        {
                        if (c=='\0')
                            {
                            /* Backup to point to the 0 so the outer
                               loop terminates. */
                            pchSrc--;
                            fSameLine = FALSE;
                            }
                        }

                    i++;
                    if ( i >= (hChars - viRightMarginLen))
                        fSameLine = FALSE;
		    }

               while (*(vpchPrintBuf+viLeftMarginLen)==' ')
                    vpchPrintBuf++;

	       if (!PrintLine ())
                    goto error0;

               vpchPrintBuf=rgchPrintBuf;
	       }
	  }
     DrUnlock (idr);
     return TRUE;
error0:
     DrUnlock(idr);
     return FALSE;
     }




/**** PrintHeading - print the heading if it hasn't been printed yet. */

BOOL APIENTRY PrintHeading (DT dt)
     {
     D3   d3;
     if (vclnDate == 0)
	  {
	  /* The heading has not yet been printed - print it out
	     now since we now know that the date is not empty.
	  */

	  /* First put out the lines between dates if this date is
	     not being printed at the top of a page.
	  */
	  vclnPrinted++;
	  /* Convert the date into an ASCII string. */
	  GetD3FromDt (dt, &d3);

	  FillBuf (vpchPrintBuf, viLeftMarginLen, ' '); /* pad margin space
							   with blanks */
	  GetDateDisp (&d3, vpchPrintBuf + viLeftMarginLen);

	  if (!PrintLine ())
		return FALSE;
	  if (!PrintBlankLn (CLNAFTERHEADING))
		return FALSE;
	  }

     return TRUE;
     }




/**** PrintBlankLn */

INT  APIENTRY PrintBlankLn (INT cln)
     {
     *vpchPrintBuf = '\0';
     while (cln--)
         if (!PrintLine ())
             return FALSE;

     return TRUE;
     }




/**** PrintLine */

BOOL APIENTRY PrintLine ()
     {
     if (vfPrint)
	  {
	  /* print footer if bottom of page is reached */

          if (vclnPrinted >= vclnPage)
                {
		PrintHeaderFooter(FALSE);
		if (!NewPage ())
		    return FALSE;
		viCurrentPage++;
		PrintHeaderFooter(TRUE);     /* print header */
		vclnPrinted = vlnTop;
              }

	  TextOut (vhDCPrint, 0, vclnPrinted * vcyPrintLineToLine,
		   (LPSTR)vpchPrintBuf,  lstrlen ((LPSTR)vpchPrintBuf));
	  vclnPrinted++;
	  }

     vclnDate++;
     return TRUE;

     }

/****************************************************************************
 *
 *  BOOL PASCAL PrintHeaderFooter ( hdr )
 *
 *  function : generates and formats the header/footer strings and copies
 *	       them to the print buffer
 *
 *  params   : IN hdr : boolean indicating if header(TRUE) or footer(FALSE)
 *			is to be generated
 *
 *  called by: Print(), PrintLine()
 *
 *  returns  : none
 *
 ***************************************************************************/

BOOL APIENTRY PrintHeaderFooter(BOOL bHeader)
    {
    CHAR    buf[80];
    SHORT   len;

    /* 1-bHeader gives 0 for header, 1 for footer. */
    lstrcpy(buf, chPageText[1-bHeader]);

    szHeadFoot=GlobalLock(hHeadFoot);
    len=TranslateString(buf);

    if (*szHeadFoot)
        {
        if (bHeader)
            TextOut(vhDCPrint, dxLeft, dyHeadFoot - CharPrintHeight, szHeadFoot, len);
        else
            TextOut(vhDCPrint, dxLeft, yPrintRes-CharPrintHeight-dyHeadFoot, szHeadFoot, len);
        }
    GlobalUnlock(hHeadFoot);
    return TRUE;
    }


/***************************************************************************
 * short TranslateString(char *src)
 *
 * purpose:
 *	translate a header/footer strings
 *
 * 	supports the following:
 *
 *	&&	insert a & char
 *	&f	current file name or (untitiled)
 *	&d	date in Day Month Year
 *	&t	time
 *	&p	page number
 *	&p+num	set first page number to num
 *
 * params:
 *	IN/OUT	src	this is the string to translate, gets filled with
 *			translate string.  limited by len chars
 *	IN	len	# chars src pts to
 *
 * used by:
 *	Header Footer stuff
 *
 * uses:
 *	lots of c lib stuff
 *
 * restrictions:
 * 	this function uses the following global data
 *
 *	iPageNum
 *	text from main window caption
 *
 ***************************************************************************/

SHORT TranslateString(CHAR *src)
    {
    CHAR         letters[15];
    CHAR         chBuff[3][80], buf[80];
    CHAR         *ptr, *dst=buf, *save_src=src;
    INT          page;
    SHORT        nAlign=1, foo, nx,
                 nIndex[3];
    struct tm    *newtime;
    time_t       long_time;

    nIndex[0]=0;
    nIndex[1]=0;
    nIndex[2]=0;

    /* Get the time we need in case we use &t. */
    time(&long_time);
    newtime=localtime(&long_time);

    LoadString(vhInstance, IDS_LETTERS, letters, 15);

    while (*src)   /* look at all of source */
        {
        while (*src && *src != '&')
            {
            chBuff[nAlign][nIndex[nAlign]]=*src++;
            nIndex[nAlign] += 1;
            }

        if (*src == '&')   /* is it the escape char? */
            {
            src++;

            if (*src == letters[0] || *src == letters[1])
                {                      /* &f file name (no path) */

                    /* a bit of sleez... get the caption from
                     * the main window.  search for the '-' and
                     * look two chars beyond, there is the
                     * file name or (untitiled) (cute hu?)
                     */

                    GetWindowText(vhwnd0, buf, 80);
                    ptr=strchr(buf, '-') + 2;

                    /* Copy to the currently aligned string. */
                    lstrcpy((LPSTR)(chBuff[nAlign]+nIndex[nAlign]), (LPSTR)ptr);

                    /* Update insertion position. */
                    nIndex[nAlign] += lstrlen((LPSTR)ptr);
                }
            else

            if (*src == letters[2] || *src == letters[3])
                {                      /* &P or &P+num page */
                    src++;
                    page = 0;
                    if (*src == '+')       /* &p+num case */
                        {
                        src++;
                        while (isdigit(*src))
                            {
                            /* Convert to int on-the-fly*/
                            page = (10*page)+(UCHAR)(*src)-48;
                            src++;
                            }

                        }

                    _itoa(viCurrentPage+page, buf, 10);
                    lstrcpy((LPSTR)(chBuff[nAlign]+nIndex[nAlign]), (LPSTR)buf);
                    nIndex[nAlign] += lstrlen((LPSTR)buf);
                    src--;
                }
            else

            if (*src == letters[4] || *src == letters[5])
                {                      /* &t time */

                    ptr = asctime(newtime);

                    /* extract time */
                    strncpy(chBuff[nAlign]+nIndex[nAlign], ptr+11, 8);
                    nIndex[nAlign] += 8;

                }
            else

            if (*src == letters[6] || *src == letters[7])
                {                      /* &d date */

                    ptr = asctime(newtime);

                    /* extract day month day */
                    strncpy(chBuff[nAlign]+nIndex[nAlign], ptr, 11);
                    nIndex[nAlign] += 11;

                    /* extract year */
                    strncpy(chBuff[nAlign]+nIndex[nAlign], ptr+20, 4);
                    nIndex[nAlign] += 4;

                }
            else

            if (*src == '&')
                {               /* quote a single & */

                    chBuff[nAlign][nIndex[nAlign]]='&';
                    nIndex[nAlign] += 1;

                }
            else

                /* Set the alignment for whichever has last occured. */

            if (*src == letters[8] || *src == letters[9])
                                       /* &c center */

                    nAlign=1;

            else

            if (*src == letters[10] || *src == letters[11])
                                       /* &r right */
                    nAlign=2;

            else

            if (*src == letters[12] || *src == letters[13])
                                       /* &d date */
                    nAlign=0;


            src++;


            }
        }
        /* Make sure all strings are null-terminated. */
        for (nAlign=0; nAlign<3; nAlign++)
            chBuff[nAlign][nIndex[nAlign]]=0;

        /* Initialize Header/Footer string */
        for (nx=0; nx<xCharPage; nx++)
            *(szHeadFoot+nx)=32;

        /* Copy Left aligned text. */
        for (nx=0; nx < nIndex[0]; nx++)
            *(szHeadFoot+nx)=chBuff[0][nx];

        /* Calculate where the centered text should go. */
        foo=(xCharPage-nIndex[1])/2;
        for (nx=0; nx<nIndex[1]; nx++)
            *(szHeadFoot+foo+nx)=(CHAR)chBuff[1][nx];

        /* Calculate where the right aligned text should go. */
        foo=xCharPage-nIndex[2];
        for (nx=0; nx<nIndex[2]; nx++)
            *(szHeadFoot+foo+nx)=(CHAR)chBuff[2][nx];


        return lstrlen(szHeadFoot);
    }




/**** NewPage */
BOOL APIENTRY NewPage ()

     {
     INT    iErr;

     if ((iErr = Escape(vhDCPrint, NEWFRAME, 0, (LPSTR)NULL, (LPSTR)0)) < 0) {
	 EndPrint();
	 CalPrintAlert(iErr);
	 return FALSE;
     }
     vclnPrinted = 0;
     return TRUE;
     }


/**** BeginPrint - code taken from Cardfile - inprint.c - modified
      as necessary. Returns 0 if successful, SP_errorcode (which is
      < 0 for error.
*/

INT APIENTRY BeginPrint ()

     {

     CHAR rgchWindowText [CCHSZWINDOWTEXTMAX];
     INT	iErr;

     /* If can't create print DC, return FALSE to indicate can't print. */
     if (((INT)(vhDCPrint = (HANDLE)GetPrinterDC())) < 0)
	  return (SP_ERROR);

     /* Show the hour glass cursor. */
     HourGlassOn ();

     vfAbortPrint = FALSE;
     Escape(vhDCPrint, SETABORTPROC, 0,
      (LPSTR)(LONG)MakeProcInstance (FnProcAbortPrint,vhInstance), (LPSTR)0);
     GetWindowText (vhwnd0, (LPSTR)rgchWindowText, CCHSZWINDOWTEXTMAX);

    /* Gotta disable the window before doing the start doc so that the user
     * can't quickly do multiple prints.
     */
     EnableWindow (vhwnd0, FALSE);

     if ((iErr = Escape(vhDCPrint, STARTDOC, lstrlen((LPSTR)rgchWindowText),
			(LPSTR)rgchWindowText, (LPSTR)0)) < 0) {
     	     EnableWindow (vhwnd0, TRUE);
	     DeleteDC(vhDCPrint);
	     HourGlassOff();
	     return iErr;
     }

     vhwndAbortPrint = CreateDialog(vhInstance, MAKEINTRESOURCE(IDD_ABORTPRINT),
	  vhwnd0,(DLGPROC)MakeProcInstance(FnDlgAbortPrint,(DWORD)vhInstance));

     return (0);

     }


/**** EndPrint - code taken from Cardfile - inprint.c - modified
      as necessary.
*/

VOID APIENTRY EndPrint ()

     {


     if (!vfAbortPrint)
	Escape(vhDCPrint, ENDDOC, 0, (LPSTR)0, (LPSTR)0);

     /* The previous Escape could have changed the value of vfAbortPrint;
      * So, this has to be tested again;
      * Fix for Bug #6029 --SANKAR-- 11-9-89
      */
     if(!vfAbortPrint)
	DestroyAbortWnd();

     DeleteDC (vhDCPrint);

     /* The waiting is over. */
     HourGlassOff ();

     }


/**** FnProcAbortPrint - code taken from Cardfile - inprint.c - modified
      as necessary.
*/

INT  APIENTRY FnProcAbortPrint (
    HDC  hDC,
    INT  iReserved)
     {

     MSG msg;

     while (!vfAbortPrint && PeekMessage((LPMSG)&msg, NULL, 0L, 0L, TRUE))
	  if (vhwndAbortPrint == NULL || !IsDialogMessage(vhwndAbortPrint, (LPMSG)&msg))
	       {
	       TranslateMessage ((LPMSG)&msg);
	       DispatchMessage ((LPMSG)&msg);
	       }
     return (!vfAbortPrint);

     }


/**** FnDlgAbortPrint - code taken from Cardfile - inprint.c - modified
      as necessary.
*/

INT APIENTRY FnDlgAbortPrint (
    HWND hwnd,
    WORD msg,
    WPARAM wParam,
    LONG lParam)
    {

    static HMENU hMenuSys;

     switch (msg)
	  {
	  case WM_COMMAND:
	       vfAbortPrint = TRUE;
	       DestroyAbortWnd();
	       vhwndAbortPrint = NULL;
	       return (TRUE);

	  case WM_INITDIALOG:
	       hMenuSys = GetSystemMenu (hwnd, FALSE);
	       SetDlgItemText (hwnd, IDCN_PATH,
		(LPSTR)PFileInPath (vszFileSpec));
	       CalSetFocus (hwnd);
	       return (TRUE);

	  case WM_INITMENU:
	       EnableMenuItem (hMenuSys, SC_CLOSE, MF_GRAYED);
	       return(TRUE);
	  }

     return(FALSE);

     }

/**** Enable Tiled window, THEN destroy dialog window. */
VOID DestroyAbortWnd()
{
    EnableWindow (vhwnd0, TRUE);
    DestroyWindow (vhwndAbortPrint);
    vhwndAbortPrint = NULL;
}

/**** Post printing error message box */
VOID APIENTRY CalPrintAlert(INT iErr)
    {
    INT     iszErr;

    /* Map error code to string index */
    if (iErr == SP_OUTOFDISK)
	iszErr = IDS_NEDSTP;
    else if (iErr == SP_OUTOFMEMORY)
	iszErr = IDS_NEMTP;
    else iszErr = IDS_CANNOTPRINT;

    AlertBox (vrgsz[iszErr], PFileInPath (vszFileSpec),
	      MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }



/* ** GetPrinterDc()
        Get Dc for current device on current output port according to
        info in win.ini.
            returns
                DC > 0 if success
                error code < 0 if failure

            error codes (to be extended):
                -1 error reading user profile
                -2 CreateDC failed.
*/
INT APIENTRY GetPrinterDC()
    {
    extern BOOL bPrinterSetupDone;
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;

    if(!bPrinterSetupDone){ /* Retrieve default printer if none selected. */
    	vPD.Flags = PD_RETURNDEFAULT|PD_PRINTSETUP;
	vPD.hDevNames = NULL;
	vPD.hDevMode  = NULL;
	PrintDlg(&vPD);
    }

    if(!vPD.hDevNames)
    	return -2;

    lpDevNames  = (LPDEVNAMES)GlobalLock(vPD.hDevNames);

    if(vPD.hDevMode)
    	lpDevMode = (LPDEVMODE)GlobalLock(vPD.hDevMode);
    else
    	lpDevMode = NULL;


    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *	will be NULL after GlobalLock()
     */

    vPD.hDC = CreateDC((LPSTR)lpDevNames+lpDevNames->wDriverOffset,
                           (LPSTR)lpDevNames+lpDevNames->wDeviceOffset,
			   (LPSTR)lpDevNames+lpDevNames->wOutputOffset,
			   lpDevMode);

    GlobalUnlock(vPD.hDevNames);

    if (vPD.hDevMode)
    	GlobalUnlock(vPD.hDevMode);

    if(!vPD.hDC)
    	return -2;
    else	
	return (INT)vPD.hDC;
    }

/****************************************************************************
**  IsDefaultPrinterStillValid()
**	The user might setup the app for a particular printer and a port
**	using the Printer Setup available in the application command menu;
**	But later, he might go to control panel and delete that printer
**	driver altogether or he might connect it to a different port;
**  So, the application must process the WININICHANGE message and at that
**  time, it must check whether the printer setup is still valid or not!
**  This function is used to make that check;
**     The input parameter is a string containing the printer name, driver,
**  port selected by the printer setup; This function checks if the printer
**  name is still present under the [Devices] section of Win.INI and if so,
**  it will check if the port selected is listed among the ports to which
**  this printer is connected; If not, it will automatically select the
**  first port listed as the default port and modify the input "lpszPrinter"
**  string accordingly;
**    If the default printer is not listed in WIN.INI at all, then this
**    function return FALSE;
**  Fix for Bug #5607 -- SANKAR -- 10-30-89
**
****************************************************************************/
BOOL FAR APIENTRY  IsDefaultPrinterStillValid(LPSTR lpszPrinter)
{
    CHAR  PrinterBuff[128];
    CHAR  DeviceBuff[128];
    LPSTR lpPort;	/* Default port name */
    LPSTR lpFirstPort = NULL;
    LPSTR lpch;
    LPSTR lpListedPorts;

    /* lpszPrinter contains "PrinterName,DriverName,Port" */
    lstrcpy(PrinterBuff, lpszPrinter);  /* Make a local copy of the default printer name */

    /* Search for the  end of printer name */
    for(lpch = PrinterBuff; (*lpch)&&(*lpch != ','); lpch = AnsiNext(lpch))
        ;
    if(*lpch)
        *lpch++ = '\0';
    /* Skip the Driver name; We do not need it! */
    while(*lpch && *lpch <= ' ')  /* Skip the blanks preceeding the driver name */
        lpch = AnsiNext(lpch);
    while(*lpch && *lpch != ',' && *lpch > ' ') /* Search for ',' following driver name */
        lpch = AnsiNext(lpch);
    while (*lpch && (*lpch <= ' ' || *lpch == ',')) /* Search for begining of port name */
        lpch = AnsiNext(lpch);
    lpPort = lpch;	/* Default port name */

    /* Search for the printer name among the [devices] section */
    if (!GetProfileString("devices", PrinterBuff, "", DeviceBuff, 128))
        return(FALSE);  /* Default printer no longer exists */

    lpch = DeviceBuff;

    /* Skip the Driver filename */
    while(*lpch && *lpch != ',')
        lpch = AnsiNext(lpch);

    while(*lpch)
      {
        /* Skip all blanks */
	while(*lpch && (*lpch <= ' ' || *lpch == ','))
	    lpch = AnsiNext(lpch);
	lpListedPorts = lpch;
	if(!lpFirstPort)
	    lpFirstPort = lpch; /* Save the first port in the list */
        /* Search for the end of the Port name */
	while(*lpch && *lpch != ',' && *lpch > ' ')
	    lpch = AnsiNext(lpch);
	if(*lpch)
	    *lpch++ = '\0';

	/* Check if the port names are the same */
	if(lstrcmp(lpPort, lpListedPorts) == 0)
	    return(TRUE); /* Default port exists among the listed ones */
      }

     /* The default port does not exist among the listed ports; So, change
      * the default port to the first port in the list;
      */
     if(*lpFirstPort)
       {
         /* Search for the end of the printer name */
	 for (lpch = szPrinter; (*lpch)&&(*lpch != ','); lpch = AnsiNext(lpch))
	     ;
	 /* Skip the driver file name */
	 if(*lpch)
	     lpch++;
	 while(*lpch && *lpch != ',')
	     lpch = AnsiNext(lpch);
	 if(*lpch)
	     lpch++;
	 lstrcpy(lpch, lpFirstPort);
	 return(TRUE);
       }
     return(FALSE);
}
