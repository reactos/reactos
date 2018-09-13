/*	File: D:\WACKER\tdll\file_msc.hh (Created: 26-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

/*
 * This is where the hidden stuff for the files and directorys code gets
 * defined.  You shouldn't be looking at this.
 */

struct stFilesAndDirectorys
	{
	HSESSION hSession;

	LPTSTR pszInternalSendDirectory;	/* Used if not set by user */
	LPTSTR pszTransferSendDirectory;

	LPTSTR pszInternalRecvDirectory;	/* Used if not set by user */
	LPTSTR pszTransferRecvDirectory;
	};

typedef struct stFilesAndDirectorys FD_DATA;

