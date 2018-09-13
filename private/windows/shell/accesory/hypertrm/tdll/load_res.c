/*	File: D:\WACKER\tdll\load_res.c (Created: 16-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "session.h"
#include "assert.h"

#include "tdll.h"
#include "tchar.h"
#include "load_res.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	resLoadDataBlock
 *
 * DESCRIPTION:
 *	This function is used to get a block of data stored in the resource file
 *	as an RCDATA item.	Note that in WIN32, it is not necessary to free the
 *	resource after it has been locked.
 *
 * PARAMETERS:
 *	hSession	-- the session handle
 *	pszName 	-- the id for the data block
 *	ppData		-- where to put the pointer to the data block
 *	pSize		-- addres of integer for size value
 *
 * RETURNS: 0 if successful, otherwise a defined error value.
 *
 *	The size of the resource that has been loaded (in bytes).
 *	NOTE:  The return value may be (and often is) larger than the actual
 *	size of the resource as it is defined in the rc file.  For resources
 *	of type RCDATA, the resource definition itself should include either a
 *	delimiter, or a count of the number of items included in that resource.
 *	See also RCDATA_TYPE in stdtype.h
 */
int resLoadDataBlock(const HINSTANCE hInst,
						const int id,
						const void **ppData,
						int *pSize)
	{
	HGLOBAL hG;
	HRSRC hR;
	LPVOID pV;
	int nSize;

	hR = FindResource(hInst, MAKEINTRESOURCE(id), (LPCTSTR)RT_RCDATA);
	if (hR == (HRSRC)0)
		return LDR_BAD_ID;

	hG = LoadResource(hInst, hR);
	if (hG == 0)
		{
		assert(FALSE);
		return LDR_NO_RES;
		}

	nSize = SizeofResource(hInst, hR);
	if (nSize == 0)
		{
		assert(FALSE);
		return LDR_NO_RES;
		}

	if(pSize)
		*pSize = nSize;

	pV = LockResource(hG);
	if (pV == 0)
		{
		assert(FALSE);
		return LDR_NO_RES;
		}

	*ppData = pV;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	resFreeDataBlock
 *
 * DESCRIPTION:
 *	This function is not necessary for WIN32.
 *
 * PARAMETERS:
 *	hSession      -- the session handle
 *	pData         -- pointer to the data block
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
int resFreeDataBlock(const HSESSION hSession,
					 const void *pData)
	{
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	resLoadFileMask
 *
 * DESCRIPTION:
 *	This function is used to get around a problem that exists in loading
 *	strings into the common file dialogs.  The file name masks are two strings
 *	that are NULL separated.  This internal NULL is not treated with any
 *	respect by the resource functions, so we split them up and do a two part
 *	load to rebuild the string.
 *
 * PARAMETERS:
 *	hInst     -- the instance handle to use
 *	uId       -- the ID of the first resource to load
 *	nCount    -- the number of string PAIRS to load, starting a uId
 *	pszBuffer -- where to put the strings
 *	nSize     -- the size of the buffer in characters
 *
 * RETURNS:
 *	Zero if everything is OK, otherwise (-1)
 */
int resLoadFileMask(HINSTANCE hInst,
					UINT uId,
					int nCount,
					LPTSTR pszBuffer,
					int nSize)
	{
	int i;
	LPTSTR pszEnd;
	LPTSTR pszPtr;

	if (pszBuffer == 0 || nSize == 0)
		{
		assert(0);
		return -1;
		}

	TCHAR_Fill(pszBuffer, TEXT('\0'), nSize);

	pszPtr = pszBuffer;
	pszEnd = pszBuffer + nSize;

	for (nCount *= 2 ; nCount > 0 ; --nCount)
		{
		i = LoadString(hInst, uId++, pszPtr, (int)(pszEnd - pszPtr - 1));
		pszPtr += (unsigned)i + 1;

		if (pszPtr >= pszEnd)
			{
			assert(0);
			return -1;
			}
		}

	return 0;
	}
