/*	File: D:\WACKER\tdll\load_res.h (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/* Error codes */
#define	LDR_ERR_BASE		0x300
#define	LDR_BAD_ID			LDR_ERR_BASE+1
#define	LDR_NO_RES			LDR_ERR_BASE+2
#define	LDR_BAD_PTR			LDR_ERR_BASE+3

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

extern int resLoadDataBlock(const HINSTANCE hInst,
							const int id,
							const void **ppData,
							int *pSize);

extern int resFreeDataBlock(const HSESSION hSession,
							const void *pData);

extern int resLoadFileMask(HINSTANCE hInst,
							UINT uId,
							int nCount,
							LPTSTR pszBuffer,
							int nSize);
