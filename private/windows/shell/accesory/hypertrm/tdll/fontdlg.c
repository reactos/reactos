/*	File: D:\WACKER\tdll\fontdlg.c (Created: 14-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "globals.h"
#include "print.hh"
#include "session.h"
#include "misc.h"
#include "term.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DisplayFontDialog
 *
 * DESCRIPTION:
 *	Invokes the common dialog box for font selection.
 *
 * ARGUMENTS:
 *	HWND	hwnd	- handle to parent dialog window.
 *
 * RETURNS:
 *	void
 *
 */
void DisplayFontDialog(const HSESSION hSession, BOOL fPrinterFont )
	{
	LOGFONT 	lf, lfOld;

	CHOOSEFONT 	chf;
	BOOL 		fRet;

	const HWND 	hwnd     = sessQueryHwnd(hSession);
    HHPRINT     hhPrint  = (HHPRINT) sessQueryPrintHdl(hSession);

    //
    // setup font structure
    //

	chf.lStructSize = sizeof(CHOOSEFONT);
	chf.hwndOwner   = hwnd;
	chf.lpLogFont   = &lf;
	chf.rgbColors   = RGB(0, 0, 0);
	chf.lCustData   = 0;
	chf.hInstance   = glblQueryHinst();
	chf.lpszStyle   = (LPTSTR)0;
	chf.nFontType   = SCREEN_FONTTYPE;
	chf.nSizeMin    = 0;
	chf.nSizeMax    = 0;

    //
    // set up for terminal font selection
    //

    if ( !fPrinterFont )
        {
	    SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_GETLOGFONT, 0,
                   (LPARAM)&lf);

	    chf.hDC    = GetDC(hwnd);
        chf.Flags  = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS |
				     CF_INITTOLOGFONTSTRUCT;

    	lfOld = lf;
	    fRet = ChooseFont(&chf);
    	ReleaseDC(hwnd, chf.hDC);
        }
    
    //
    // set up for printer font selection
    //

    else
        {
        hhPrint->hDC = printCtrlCreateDC((HPRINT)hhPrint);
        
        lf = hhPrint->lf;

        chf.hDC    = hhPrint->hDC;
	    chf.Flags  = CF_EFFECTS | CF_PRINTERFONTS | CF_NOVERTFONTS | 
                     CF_INITTOLOGFONTSTRUCT;

    	lfOld = lf;
	    fRet = ChooseFont(&chf);
        }

    // 
    // Save any changes that were made
    //

	if (fRet && memcmp(&lf, &lfOld, sizeof(LOGFONT)) != 0)
		{
		const HWND hwndTerm = sessQueryHwndTerminal(hSession);

        if ( !fPrinterFont )
	        {
        	SendMessage(hwndTerm, WM_TERM_SETLOGFONT, 0, (LPARAM)&lf);
    		RefreshTermWindow(hwndTerm);
            }
        else
            {
            //
            // save the dialog returned log font in the print handle and also 
            // save the selected point size.  This is done since the font point
            // size returned by the dialog is is not correct when used for 
            // printing.  However the dialog settings must be saved for the 
            // next time the dialog is displayed.  The correct font is calculated
            // based on the save point size and face name before printing by the 
            // printCreatePointFont function.
            //

            hhPrint->iFontPointSize = chf.iPointSize;
            hhPrint->lf = lf; 
  
            lf.lfHeight = chf.iPointSize;

            //
            // if char set is ansi change to oem to get line draw characters
            //
        
            if ( lf.lfCharSet == ANSI_CHARSET )
                {
                lf.lfCharSet = OEM_CHARSET;
                }
            }

        //
        // get rid of the printer device context if one was created
        //

        if ( fPrinterFont )
            {
            printCtrlDeleteDC((HPRINT)hhPrint);
            }
		}
	
    return;
	}
