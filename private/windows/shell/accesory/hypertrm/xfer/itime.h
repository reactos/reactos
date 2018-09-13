/* itime.h -- functions to handle time in our program
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

extern void itimeSetFileTime(LPCTSTR pszName, unsigned long ulTime);

extern unsigned long itimeGetFileTime(LPCTSTR pszName);

extern unsigned long itimeGetBasetime(void);

