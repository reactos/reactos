#include <msvcrt/wchar.h>


/*
 * @implemented
 *
 * this function is now forwarded to NTDLL._wtoi to reduce code duplication
 */
#if 0
int _wtoi(const wchar_t* str)
{
	return (int)wcstol(str, 0, 10);
}
#endif
  
/*
 * @implemented
 *
 * this function is now forwarded to NTDLL._wtol to reduce code duplication
 */
#if 0
long _wtol(const wchar_t* str)
{
	return (int)wcstol(str, 0, 10);
}
#endif
