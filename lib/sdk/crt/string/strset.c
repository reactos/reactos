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
char* _strnset(char* szToFill, int szFill, size_t sizeMaxFill)
{
	char *t = szToFill;
	int i = 0;
	while (*szToFill != 0 && i < (int) sizeMaxFill)
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
#ifndef _MSC_VER
char* _strset(char* szToFill, int szFill)
{
	char *t = szToFill;
	while (*szToFill != 0)
	{
		*szToFill = szFill;
		szToFill++;

	}
	return t;
}
#endif
