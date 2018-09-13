/*	File: D:\WACKER\tdll\sf.h (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

#if !defined(INCL_SF)
#define INCL_SF

#if !defined(SF_HANDLE)
#define SF_HANDLE int
#endif

/*
 * Function prototypes
 */

SF_HANDLE CreateSysFileHdl(void);

int sfOpenSessionFile(const SF_HANDLE, const TCHAR *);

int sfCloseSessionFile(const SF_HANDLE);

int sfFlushSessionFile(const SF_HANDLE);

int sfReleaseSessionFile(const SF_HANDLE);

int sfGetSessionFileName(const SF_HANDLE, const int, TCHAR *);

int sfSetSessionFileName(const SF_HANDLE, const TCHAR *);

int sfGetSessionItem(const SF_HANDLE,
					 const unsigned int,
					 unsigned long *,
					 void *);

int sfPutSessionItem(const SF_HANDLE,
					 const unsigned int,
					 const unsigned long,
					 const void *);

/*
 * Error codes
 */

#define SF_OK					 0
#define SF_ERR_FILE_FORMAT		-1
#define SF_ERR_MEMORY_ERROR		-2
#define SF_ERR_BAD_PARAMETER	-3
#define SF_ERR_FILE_TOO_LARGE	-4
#define SF_ERR_FILE_ACCESS		-5

#endif
