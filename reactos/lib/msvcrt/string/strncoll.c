#include <windows.h>
#include <msvcrt/string.h>

/* Compare S1 and S2, returning less than, equal to or
   greater than zero if the collated form of S1 is lexicographically
   less than, equal to or greater than the collated form of S2.  */


/*
 * @unimplemented
 */
int _strncoll(const char* s1, const char* s2, size_t c)
{
    return strncmp(s1, s2, c);
}

/*
 * @unimplemented
 */
int _strnicoll(const char* s1, const char* s2, size_t c)
{
    return _strnicmp(s1, s2, c);
}
