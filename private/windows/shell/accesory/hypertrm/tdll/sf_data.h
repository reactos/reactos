/*	File: D:\WACKER\tdll\sf_data.h (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Error codes */
#define	SFD_ERR_BASE		0x200
#define	SFD_NO_MEMORY		SFD_ERR_BASE+1
#define	SFD_BAD_POINTER		SFD_ERR_BASE+2
#define	SFD_SIZE_ERROR		SFD_ERR_BASE+3

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Generic data structure */

struct stDataPointer
	{
	int nSize;
	};

typedef struct stDataPointer SF_DATA;

#define	SFD_MAX		(32*1024)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

extern int sfdGetDataBlock(const HSESSION hSession,
							const int nId,
							const void **ppData);

extern int sfdPutDataBlock(const HSESSION hSession,
							const int nId,
							const void *pData);

