/*	File: D:\WACKER\tdll\backscrl.hh (Created: 10-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#if !defined(INCL_HHBACKSCRL)
#define INCL_HHBACKSCRL

typedef struct stBackscrlPage * HBKPAGE;
typedef struct stBackscrl	  * HHBACKSCRL;

struct stBackscrl
	{
	HBKPAGE 	*hBkPages;			// where the pages live (array of ptrs).
	int 		 iPages,			// number of backscroll pages.
				 iCurrPage, 		// current page
				 iOffset,			// offset into current page.
				 iLines,			// total number of lines in all pages
				 iChanged,			// backscrl has changed since last
									// backscrlResetChangeFlag() call

				 iUserLines,		// User set number of backscroll lines to save
				 iUserLinesSave;	// Starting value

	int 		 fShowBackscrl; 	// Turns backscroll display on/off
	HSESSION	 hSession;
	};

/* --- Macros --- */

#define BACKSCRL_PAGESIZE (1024)
#define BACKSCRL_MAXPAGES (INT_MAX / (int)sizeof(HBKPAGE))

struct stBackscrlPage
	{
	int 	 iLines;					// number of lines stored in this page
	ECHAR	 *pachPage;					// text buffer for page.
	};

#endif
