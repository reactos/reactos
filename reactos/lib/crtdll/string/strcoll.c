#include <windows.h>
#include <msvcrt/string.h>

/* Compare S1 and S2, returning less than, equal to or
   greater than zero if the collated form of S1 is lexicographically
   less than, equal to or greater than the collated form of S2.  */

#if 1
int strcoll (const char* s1, const char* s2)
{
	return strcmp(s1,s2);
}

int _stricoll (const char* s1, const char* s2)
{
	return _stricmp(s1,s2);
}

#else
int strcoll (const char *s1,const char *s2)
{
   int ret;
   ret = CompareStringA(LOCALE_USER_DEFAULT,0,s1,strlen(s1),s2,strlen(s2));
   if (ret == 0)
     return 0;
   else 
     return ret - 2;
   return 0;
}
#endif
