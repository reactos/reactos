#include <msvcrt/string.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/crttypes.h>

/*
 * @implemented
 */
char* _strnset (char* szToFill, int szFill, size_t sizeMaxFill)
{
	char *t = szToFill;
	int i = 0;
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
