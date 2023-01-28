// #ifdef HAVE_CONFIG_H
// #endif
//#include <ldns/config.h>

// #ifdef HAVE_TIME_H
// #endif
#include <sys/time.h>

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	/* no thread safety. */
	*result = *localtime(timep);
	return result;
}
