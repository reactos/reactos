#include <crtdll/sys/time.h>

unsigned int _getsystime(struct tm *tp)
{
	printf("getsystime\n");
	return 0;
}


unsigned int _setsystime(struct tm *tp, unsigned int ms)
{
	printf("setsystime\n");
	return 0;
}