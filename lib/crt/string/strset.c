/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
char* _strnset (char* szToFill, int szFill, size_t sizeMaxFill)
{
	char *t = szToFill;
	size_t i = 0;
	while( *szToFill != 0 && i < sizeMaxFill)
	{
		*szToFill = szFill;
		szToFill++;
		i++;

	}
	return t;
}

/*
 * @implemented
 */
char* _strset (char* szToFill, int szFill)
{
	char *t = szToFill;
	while( *szToFill != 0 )
	{
		*szToFill = szFill;
		szToFill++;

	}
	return t;
}
