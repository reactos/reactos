/*	File: D:\WACKER\tdll\update.h (Created: 09-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

#if !defined(INCL_HUPDATE)
#define INCL_HUPDATE

// Function prototypes...

HUPDATE updateCreate		(const HSESSION);
void	updateDestroy		(const HUPDATE hUpdate);

void	updateScroll		(const HUPDATE hUpdate,
							 const int yBeg,
							 const int yEnd,
							 const int iScrlInc,
							 const int iTopRow,
							 const BOOL fSave
							);

void	updateLine			(const HUPDATE hUpdate,
							 const int yBeg,
							 const int yEnd
							);

void	updateChar			(const HUPDATE hUpdate,
							 const int yPos,
							 const int xBegPos,
							 const int xEndPos
							);

void	updateCursorPos 	(const HUPDATE hUpdate,
							 const int sRow,
							 const int sCol
							);

int  updateSetReallocFlag(const HUPDATE hUpdate, const BOOL fState);
BOOL updateIsLocked 	 (const HUPDATE hUpdate);
int  updateSetLock		 (const HUPDATE hUpdate, const BOOL fState);
int  updateSetScrlMax	 (const HUPDATE hUpdate, const int iScrlMax);

void updateBackscroll	 (const HUPDATE hUpdate, const int iLines);

#endif
