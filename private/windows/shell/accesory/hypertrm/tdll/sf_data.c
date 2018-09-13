/*	File: D:\WACKER\tdll\sf_data.c (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "session.h"
#include "sf.h"
#include "sf_data.h"
#include "mc.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	sfdGetDataBlock
 *
 * DESCRIPTION:
 *	This function is called to load a "standard" data block from the session
 *	file.  A "standard" data block is any data structure with an int as the
 *	first element that contains the size of the block.
 *
 * PARAMETERS:
 *	hSession      -- a session handle
 *	nId           -- the item id
 *	ppData        -- where to put the data pointer
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int sfdGetDataBlock(const HSESSION hSession,
					const int nId,
					const void **ppData)
	{
	SF_DATA *pSFD;
	SF_HANDLE hSF;
	int sRet;
	unsigned long ulSize;

	/* Do a little error checking */
	hSF = sessQuerySysFileHdl(hSession);
	if (hSF == (SF_HANDLE)0)
		return SFD_BAD_POINTER;

	pSFD = (SF_DATA *)0;

	/* Check the size of the block in the file */
	ulSize = 0;
	sRet = sfGetSessionItem(hSF, nId, &ulSize, (void *)0);
	if (sRet != SF_OK)
		{
		return sRet;
		}

	/* If > 0, allocate and load the block */
	if (ulSize > 0)
		{
		pSFD = (SF_DATA *)malloc(ulSize);
		if (pSFD == (SF_DATA *)0)
			{
			return SFD_NO_MEMORY;
			}
		else
			{
			sRet = sfGetSessionItem(hSF, nId, &ulSize, (void *)pSFD);
			if (sRet != SF_OK)
				{
				free(pSFD);
				return sRet;
				}
			}
		}

	*ppData = (void *)pSFD;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	sfdPutDataBlock
 *
 * DESCRIPTION:
 *	This function is called to save a "standard" data block to the session
 *	file.  A "standard" data block is any data structure with an int as the
 *	first element that contains the size of the block.
 *
 * PARAMETERS:
 *	hSession       -- a session handle
 *	nId            -- the item id
 *	pData          -- a pointer to the data block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int sfdPutDataBlock(const HSESSION hSession,
					const int nId,
					const void *pData)
	{
	SF_DATA *pSFD;
	SF_HANDLE hSF;
	int sRet;

	pSFD = (SF_DATA *)pData;

	/* Just for fun, do a little error checking */
	if (pSFD == (SF_DATA *)0)
		return SFD_BAD_POINTER;
	if ((pSFD->nSize < 0) || (pSFD->nSize > SFD_MAX))
		return SFD_SIZE_ERROR;

	hSF = sessQuerySysFileHdl(hSession);
	if (hSF == (SF_HANDLE)0)
		return SFD_BAD_POINTER;

	sRet = sfPutSessionItem(hSF, nId, pSFD->nSize, pData);

	return sRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
