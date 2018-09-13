/*	File: D:\WACKER\tdll\update.hh (Created: 09-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:36p $
 */

#if !defined(INCL_HHUPDATE)
#define INCL_HHUPDATE

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

	Update record formats:

	The server hands display information to the client via update records.
	These update records describe changes since the last time the client
	requested display information.	To optimize this information exchange,
	the update record has several "personalities".	Scroll mode, sends
	information on a line basis.  Line mode sends information about a
	single line.  The server decides which to send.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define UPD_SCROLL		1			// scroll mode
#define UPD_LINE		2			// line mode

#define UPD_MAXLINES  	MAX_EMUROWS * 2		// This value must be twice the size
											// of the maximum number of rows for any
											// emulator in any mode!

struct stScrlMode
	{
	int   yBeg, yEnd,
		  iScrlInc, 				// iScrlInc for full screen scrolls.
		  iRgnScrlInc,				// iRgnScrlInc for region scrolls.
		  iBksScrlInc;				// amount backscrl has scrolled.
	BYTE  auchLines[UPD_MAXLINES];	// array used to record line changes.
	int   iFirstLine;				// points to first line in auchLines[].
	BOOL  fSave;					// if true, save changes to backscroll.
	};


struct stLineMode
	{
	int iLine, xBeg, xEnd;		  // range of columns within a line.
	};

struct stUpdate
	{
	BYTE		bUpdateType;
	int 		iTopline;			// always need to know the top line.
	int 		iLines; 			// number of lines in backscroll buffer
	int 		iRow;				// cursor row (offset from 0)
	int 		iCol;				// cursor col (offset from 0)
	int 		iCType; 			// cursor type
	int 		iScrlMax;			// maximum allowable amount to scroll.
	BOOL		fUpdateLock;		// update record full.
	BOOL		fRealloc;			// emulator resized its image buffers.

	struct stScrlMode	stScrl;
	struct stLineMode	stLine;

	HSESSION	hSession;
	};

typedef struct stUpdate *HHUPDATE;

/* --- Function Prototypes ---*/

void updateReset(const HHUPDATE hUpd);

#endif
