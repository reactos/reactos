#include <msvcrt/string.h>

/*
 * This is a CRTDLL internal function to return the lasttoken
 * bit of data used by strtok. The reason for it's existence is
 * so that CRTDLL can use MSVCRT's implementation of strtok.
 */
char** _lasttoken()
{
	static char *lasttoken = NULL;
	return &lasttoken;
}
