#include <crtdll/string.h>
#include <crtdll/stdlib.h>

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
