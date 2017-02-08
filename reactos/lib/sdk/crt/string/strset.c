/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/string/strset.c
 * PURPOSE:     Implementation of _strnset and _strset
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#if defined(__GNUC__) && !defined(__clang__)
#define __int64 long long
#elif defined(_MSC_VER)
#pragma warning(disable: 4164)
#pragma function(_strset)
#endif

#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif

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

