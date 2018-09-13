/*	File: D:\WACKER\tdll\tchar.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 7/23/99 12:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <string.h>

#include "stdtyp.h"
#include "tdll.h"
#include "assert.h"
#include "tchar.h"
#include <tdll\mc.h>
#include <tdll\features.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Fill
 *
 * DESCRIPTION:
 *	Fills a TCHAR string with the specified TCHAR.
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of TCHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
TCHAR *TCHAR_Fill(TCHAR *dest, TCHAR c, size_t count)
	{
#if defined(UNICODE)
	int i;

	for (i = 0 ; i < count ; ++i)
		dest[i] = c;

	return dest;

#else

	return (TCHAR *)memset(dest, c, count);

#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Trim
 *
 * DESCRIPTION:
 *	This function is called to clean up user input.  It strips all white
 *	space from the front and rear of a string.  Sometimes nothing is left.
 *
 *	NOTE: This won't work on strings > 512 bytes
 *
 * ARGUEMENTS:
 *	pszStr     -- the string to trim
 *
 * RETURNS:
 *	pointer to the string
 */
TCHAR *TCHAR_Trim(TCHAR *pszStr)
	{
	int nExit;
	TCHAR *pszPtr;
	TCHAR *pszLast;
	TCHAR acBuf[512];

	/* Skip the leading white space */
	for (nExit = FALSE, pszPtr = pszStr; nExit == FALSE; )
		{
		switch (*pszPtr)
			{
			/* Anything here is considered white space */
			case 0x20:
			case 0x9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
				pszPtr += 1;		/* Skip the white space */
				break;
			default:
				nExit = TRUE;
				break;
			}
		}

	if ((unsigned int)lstrlen(pszPtr) > sizeof(acBuf))
		{
		return NULL;
		}

	lstrcpy(acBuf, pszPtr);

	/* Find the last non white space character */
	pszPtr = pszLast = acBuf;
	while (*pszPtr != TEXT('\0'))
		{
		switch (*pszPtr)
			{
			/* Anything here is considered white space */
			case 0x20:
			case 0x9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
				break;
			default:
				pszLast = pszPtr;
				break;
			}
		pszPtr += 1;
		}
	pszLast += 1;
	*pszLast = TEXT('\0');

	lstrcpy(pszStr, acBuf);

	return pszStr;
	}

#if 0 // Thought I needed this but I didn't.  May be useful someday however.
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Trunc
 *
 * DESCRIPTION:
 *	Removes trailing space from a character array.	Does not assume
 *
 * ARGUMENTS:
 *	psz - string of characters (null terminated).
 *
 * RETURNS:
 *	Length of truncated string
 *
 */
int TCHAR_Trunc(const LPTSTR psz)
	{
	int i;

	for (i = lstrlen(psz) - 1 ; i > 0 ; --i)
		{
		switch (psz[i])
			{
		/* Whitespace characters */
		case TEXT(' '):
		case TEXT('\t'):
		case TEXT('\n'):
		case TEXT('\v'):
		case TEXT('\f'):
		case TEXT('\r'):
			break;

		default:
			psz[i+1] = TEXT('\0');
			return i;
			}
		}

	return i;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharNext
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharNext(LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		/* Could be done with 'IsDBCSLeadByte' etc. */
		pszRet = CharNextExA(0, pszStr, 0);
#else
		pszRet = (LPTSTR)pszStr + 1;
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharPrev
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharPrev(LPCTSTR pszStart, LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if ((pszStart != (LPTSTR)NULL) && (pszStr != (LPTSTR)NULL))
		{
#if defined(CHAR_MIXED)
		pszRet = CharPrev(pszStart, pszStr);
#else
		if (pszStr > pszStart)
			pszRet = (LPTSTR)pszStr - 1;
		else
			pszRet = (LPTSTR)pszStart;
#endif
		}

	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharLast
 *
 * DESCRIPTION:
 *	Returns a pointer to the last character in a string
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharLast(LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			pszRet = (LPTSTR)pszStr;
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		/* It might be possible to use 'strlen' here. Then again... */
		// pszRet = pszStr + StrCharGetByteCount(pszStr) - 1;
		pszRet = (LPTSTR)pszStr + lstrlen(pszStr) - 1;
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharEnd
 *
 * DESCRIPTION:
 *	Returns a pointer to the NULL terminating a string
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharEnd(LPCTSTR pszStr)
	{

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			pszStr = StrCharNext(pszStr);
			pszStr += 1;
			}
#else
		pszStr = pszStr + lstrlen(pszStr);
#endif
		}
	return (LPTSTR)pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharFindFirst
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharFindFirst(LPCTSTR pszStr, int nChar)
	{
#if defined(CHAR_MIXED)
	WORD *pszW;
#endif

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			/*
			 * NOTE: this may not work for UNICODE
			 */
			if (nChar > 0xFF)
				{
				/* Two byte character */
				if (IsDBCSLeadByte(*pszStr))
					{
					pszW = (WORD *)pszStr;
					if (*pszW == (WORD)nChar)
						return (LPTSTR)pszStr;
					}
				}
			else
				{
				/* Single byte character */
				if (*pszStr == (TCHAR)nChar)
					return (LPTSTR)pszStr;
				}
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		while (pszStr && (*pszStr != TEXT('\0')) )
			{
			if (*pszStr == (TCHAR)nChar)
				return (LPTSTR)pszStr;

			pszStr = StrCharNext(pszStr);
			}
#endif
		}
	return (LPTSTR)NULL;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharFindLast
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharFindLast(LPCTSTR pszStr, int nChar)
	{
	LPTSTR pszRet = (LPTSTR)NULL;
#if defined(CHAR_MIXED)
	WORD *pszW;
#else
	LPTSTR pszEnd;
#endif

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			/*
			 * NOTE: this may not work for UNICODE
			 */
			if (nChar > 0xFF)
				{
				/* Two byte character */
				if (IsDBCSLeadByte(*pszStr))
					{
					pszW = (WORD *)pszStr;
					if (*pszW == (WORD)nChar)
						pszRet = (LPTSTR)pszStr;
					}
				}
			else
				{
				/* Single byte character */
				if (*pszStr == (TCHAR)nChar)
					pszRet = (LPTSTR)pszStr;
				}
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		pszEnd = StrCharLast(pszStr);
		while (pszEnd && (pszEnd > pszStr) )
			{
			if (*pszEnd == (TCHAR)nChar)
				{
				pszRet = (LPTSTR)pszEnd;
				break;
				}
			pszEnd = StrCharPrev(pszStr, pszEnd);
			}
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetStrLength
 *
 * DESCRIPTION:
 *	This function returns the number of characters in a string.  A two byte
 *	character counts as one.
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetStrLength(LPCTSTR pszStr)
	{
	int nRet = 0;

#if defined(CHAR_MIXED)
	if (pszStr != (LPTSTR)NULL)
		{
		while (*pszStr != TEXT('\0'))
			{
			nRet += 1;
			pszStr = CharNextExA(0, pszStr, 0);
			}
		}
#else
	if (pszStr != (LPTSTR)NULL)
		{
		nRet = lstrlen(pszStr);
		}
#endif
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetByteCount
 *
 * DESCRIPTION:
 *	This function returns the number of bytes in a string.  A two byte char
 *	counts as two.
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetByteCount(LPCTSTR pszStr)
	{
	int nRet = 0;
#if defined(CHAR_MIXED)
	LPCTSTR pszFoo;

	if (pszStr != (LPTSTR)NULL)
		{
		pszFoo = pszStr;
		while (*pszFoo != TEXT('\0'))
			{
			pszFoo = CharNextExA(0, pszFoo, 0);
			}
		nRet = (int)(pszFoo - pszStr);
		}
#else
	if (pszStr != (LPTSTR)NULL)
		{
		nRet = lstrlen(pszStr);
		}
#endif
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCopy
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharCopy(LPTSTR pszDst, LPCTSTR pszSrc)
	{
	return lstrcpy(pszDst, pszSrc);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCat
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharCat(LPTSTR pszDst, LPCTSTR pszSrc)
	{
	return lstrcat(pszDst, pszSrc);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCmp
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharCmp(LPCTSTR pszA, LPCTSTR pszB)
	{
	return lstrcmp(pszA, pszB);
	}
int StrCharCmpi(LPCTSTR pszA, LPCTSTR pszB)
	{
	return lstrcmpi(pszA, pszB);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharStrStr
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharStrStr(LPCTSTR pszA, LPCTSTR pszB)
	{
	LPTSTR pszRet = (LPTSTR)NULL;
	int nSize;
	int nRemaining;
	LPTSTR pszPtr;

	/*
	 * We need to write a version of 'strstr' that will work.
	 * Do we really know what the problems are ?
	 */
	nSize = StrCharGetByteCount(pszB);

	pszPtr = (LPTSTR)pszA;
	while ((nRemaining = StrCharGetByteCount(pszPtr)) >= nSize)
		{
		if (memcmp(pszPtr, pszB, (size_t)nSize) == 0)
			return pszPtr;
		pszPtr = StrCharNext(pszPtr);
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	StrCharCopyN
 *
 * DESCRIPTION:
 *  Basically do a lstrcpy of n bytes, with one exception, we make sure that
 *	the copied string does not end in a lead-byte of a double-byte character.
 *
 * ARGUMENTS:
 *  pszDst - pointer to the copy target string.
 *	pszSrc - pointer to the copy source string.
 *	iLen   - the maximum number of BYTES to copy.  Like strcpyn, the
			 string may not be null terminated if the buffer is exceeded.
 *
 * RETURNS:
 *	0=error, else pszDst
 *
 */
LPTSTR StrCharCopyN(LPTSTR pszDst, LPTSTR pszSrc, int iLen)
	{
	int i;
	LPTSTR psz = pszSrc;

	if (pszDst == 0 || pszSrc == 0 || iLen == 0)
		return 0;

	while (1)
		{
		i = (int)(StrCharNext(psz) - psz);
		iLen -= i;

		if (iLen <= 0)
			break;

		if (*psz == TEXT('\0'))
			{
			psz += i;	// still need to increment
			break;
			}

		psz += i;
		}

	MemCopy(pszDst, pszSrc, (size_t)(psz - pszSrc));
	return pszDst;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CnvrtMBCStoECHAR
 *
 * DESCRIPTION:
 *	Converts a DBCS (mixed byte) string into an ECHAR (double byte) string.
 *
 * PARAMETERS:
 *	tchrSource   - Source String
 *  ulDestSize   - Length of Destination String in Bytes
 *  echrDest     - Destination String
 *  ulSourceSize - Length of Destination String in Bytes
 *
 * RETURNS:
 *	0 - Success
 *  1 - Error
 */
int CnvrtMBCStoECHAR(ECHAR * echrDest, const unsigned long ulDestSize, const TCHAR * const tchrSource, const unsigned long ulSourceSize)
	{
	ULONG ulLoop      = 0;
	ULONG ulDestCount = 0;
	ULONG ulDestEChars = ulDestSize / sizeof(ECHAR);
	BOOL fLeadByteFound = FALSE;

	if ((echrDest == NULL) || (tchrSource == NULL))
		{
		assert(FALSE);
		return TRUE;					
		}

	// Make sure that the destination string is big enough to handle to source string
	if (ulDestEChars < ulSourceSize)
		{
		assert(FALSE);
		return 1;
		}


#if defined(CHAR_MIXED)
	// because we do a strcpy in the NARROW version of this function,
	// and we want the behavior to be the save between the two.  We
	// clear out the string, just like strcpy does
    memset(echrDest, 0, ulDestSize);

	for (ulLoop = 0; ulLoop < ulSourceSize; ulLoop++)
		{
		if ((IsDBCSLeadByte(tchrSource[ulLoop])) && (!fLeadByteFound))
			// If we found a lead byte, and the last one was not a lead
			// byte.  We load the byte into the top half of the ECHAR
			{
			echrDest[ulDestCount] = (tchrSource[ulLoop] & 0x00FF);
			echrDest[ulDestCount] = (ECHAR)(echrDest[ulDestCount] << 8);
			fLeadByteFound = TRUE;
			}
		else if (fLeadByteFound)
			{
			// If the last byte was a lead byte, we or it into the
			// bottom half of the ECHAR
			echrDest[ulDestCount] |= (tchrSource[ulLoop] & 0x00FF);
			fLeadByteFound = FALSE;
			ulDestCount++;
			}
		else
			{
			// Otherwise we load the byte into the bottom half of the
			// ECHAR and clear the top half.
			echrDest[ulDestCount] = (tchrSource[ulLoop] & 0x00FF);
			ulDestCount++;
			}
		}
#else
	// ECHAR is only a byte, so do a straight string copy.
    if (ulSourceSize)
        MemCopy(echrDest, tchrSource, ulSourceSize);
#endif
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CnvrtECHARtoMBCS
 *
 * DESCRIPTION:
 *	Converts an ECHAR (double byte) string into a DBCS (mixed byte) string.
 *
 * PARAMETERS:
 *	echrSource   - Source String
 *  ulDestSize   - Length of Destination String in Bytes
 *  tchrDest     - Destination String
 *
 * RETURNS:
 *	Number of bytes in the converted string
 *  1 - Error
 */
int CnvrtECHARtoMBCS(TCHAR * tchrDest, const unsigned long ulDestSize, const ECHAR * const echrSource, const unsigned long ulSourceSize)
	{
	ULONG ulLoop      = 0;
	ULONG ulDestCount = 0;
	ULONG ulSourceEChars = ulSourceSize / sizeof(ECHAR);

	if ((tchrDest == NULL) || (echrSource == NULL))
		{
		assert(FALSE);
		return TRUE;
		}

#if defined(CHAR_MIXED)
	// because we do a strcpy in the NARROW version of this function,
	// and we want the behavior to be the save between the two.  We
	// clear out the string, just like strcpy does
    memset(tchrDest, 0, ulDestSize);

	// We can't do a strlen of an ECHAR string, so we loop
	// until we hit NULL or we are over the size of the destination.
	while ((ulLoop < ulSourceEChars) && (ulDestCount <= ulDestSize))
		{
		if (echrSource[ulLoop] & 0xFF00)
			// Lead byte in this character, load the lead byte into one
			// TCHAR and the lower byte into a second TCHAR.
			{
			tchrDest[ulDestCount] = (TCHAR)((echrSource[ulLoop] & 0xFF00) >> 8);
			ulDestCount++;
			tchrDest[ulDestCount] = (TCHAR)(echrSource[ulLoop] & 0x00FF);
			}
		else
			// No lead byte in this ECHAR, just load the lower half into
			// the TCHAR.
			{
			tchrDest[ulDestCount] = (TCHAR)(echrSource[ulLoop] & 0x00FF);
			}
		ulDestCount++;
		ulLoop++;
		if(ulDestCount > ulDestSize)
			assert(FALSE);
		}
	return ulDestCount;
#else
	// ECHAR is only a byte, so do a straight string copy.
    if (ulSourceSize)
	    MemCopy(tchrDest, echrSource, ulSourceSize);
	return ulSourceSize;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetEcharLen
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetEcharLen(const ECHAR * const pszA)
	{
#if defined(CHAR_MIXED)
	int nLoop = 0;
#endif
	if (pszA == NULL)
		{
		assert(FALSE);
		return 0;
		}

#if defined(CHAR_MIXED)
	while (pszA[nLoop] != 0)
		{
		nLoop++;
		}

	return nLoop;
#else
	return (int)strlen(pszA);
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetEcharByteCount
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetEcharByteCount(const ECHAR * const pszA)
	{
#if defined(CHAR_MIXED)
	int nLoop = 0;
#endif
	if (pszA == NULL)
		{
		assert(FALSE);
		return 0;
		}

#if defined(CHAR_MIXED)
	while (pszA[nLoop] != 0)
		{
		nLoop++;
		}

	nLoop *= (int)sizeof(ECHAR);
	return nLoop;
#else
	return (int)strlen(pszA);
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCmpEtoT
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharCmpEtoT(const ECHAR * const pszA, const TCHAR * const pszB)
	{

#if defined(CHAR_MIXED)

	TCHAR *tpszA = NULL;
	int    nLenA = StrCharGetEcharLen(pszA);

	tpszA = (TCHAR *)malloc((unsigned int)nLenA * sizeof(ECHAR));
	if (tpszA == NULL)
		{
		assert(FALSE);
		return 0;
		}

	CnvrtECHARtoMBCS(tpszA, (unsigned long)(nLenA * (int)sizeof(ECHAR)), pszA, StrCharGetEcharByteCount(pszA));

	return StrCharCmp(tpszA, pszB);
#else
	return strcmp(pszA, pszB);
#endif
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ECHAR_Fill
 *
 * DESCRIPTION:
 *	Fills a ECHAR string with the specified ECHAR.
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of ECHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
ECHAR *ECHAR_Fill(ECHAR *dest, ECHAR c, size_t count)
	{
#if defined(CHAR_NARROW)

	return (TCHAR *)memset(dest, c, count);

#else
	unsigned int i;

	if (dest == NULL)
		{
		assert(FALSE);
		return 0;
		}

	for (i = 0 ; i < count ; ++i)
		dest[i] = c;

	return dest;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ECHAR_Fill
 *
 * DESCRIPTION:
 *	
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of TCHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
int CnvrtECHARtoTCHAR(TCHAR *dest, size_t TCHARSize, ECHAR eChar)
	{
#if defined(CHAR_NARROW)
	dest[0] = eChar;
	dest[1] = ETEXT('\0');
	
#else
	memset(dest, 0, sizeof(TCHARSize));
	// This is the only place where we convert a single ECHAR to TCHAR's
	// so as of right now we will not make this into a function.
	if (eChar & 0xFF00)
		// Lead byte in this character, load the lead byte into one
		// TCHAR and the lower byte into a second TCHAR.
		{
		if (TCHARSize >= 2)
			{
			dest[0] = (TCHAR)((eChar & 0xFF00) >> 8);
			dest[1] = (TCHAR)(eChar & 0x00FF);
			}
		else
			{
			return 1;
			}
		}
	else
		// No lead byte in this ECHAR, just load the lower half into
		// the TCHAR.
		{
		dest[0] = (TCHAR)(eChar & 0x00FF);
		}
#endif
	return 0;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	isDBCSChar
 *
 * DESCRIPTION:
 *	Determines if the Character is Double Byte or not
 *	
 *
 * ARGUMENTS:
 *	c		- character to test.
 *
 * RETURNS:
 *	int 	TRUE  - if DBCS
 *			FALSE - if SBCS
 *
 */
int isDBCSChar(unsigned int Char)
	{
	int rtn = 0;
#if defined(CHAR_NARROW)
	rtn = 0;

#else
	ECHAR ech = 0;
	char ch;

	if (Char == 0)
		{
		// assert(FALSE);
		return FALSE;
		}

	ech = ETEXT(Char);

	if (ech & 0xFF00)
		{
		ch = (char)(ech >> 8);
		if (IsDBCSLeadByte(ch))
			{
			rtn = 1;
			}

		}
#endif
	return rtn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	StrCharStripDBCSString
 *
 * DESCRIPTION:
 *	Strips out Left/Right pairs of wide characters and leaves a single wide character
 *	in it's place
 *	
 *
 * ARGUMENTS:
 *	aech		- String to be stripped
 *
 * RETURNS:
 *	int - number of characters striped out of string
 *
 */
int StrCharStripDBCSString(ECHAR *aechDest, const long lDestSize,
    ECHAR *aechSource)
	{
	int nCount = 0;
#if !defined(CHAR_NARROW)
	ECHAR *pechTmpS;
	ECHAR *pechTmpD;
	long j;
	long lDLen = lDestSize / sizeof(ECHAR);;

	if ((aechSource == NULL) || (aechDest == NULL))
		{
		assert(FALSE);
		return nCount;
		}

        pechTmpS = aechSource;
        pechTmpD = aechDest;

	for (j = 0; (*pechTmpS != '\0') && (j < lDLen); j++)
		{
		*pechTmpD = *pechTmpS;

		if ((isDBCSChar(*pechTmpS)) && (*(pechTmpS + 1) != '\0'))
			{
			if (*pechTmpS == *(pechTmpS + 1))
				{
				pechTmpS++;
				nCount++;
				}
			}
		pechTmpS++;
                pechTmpD++;
		}

	*pechTmpD = ETEXT('\0');
#endif
	return nCount;
	}
