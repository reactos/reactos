#include <crtdll/time.h>
#include <crtdll/stdio.h>

// copy date according to mm/dd/yy format
char *_strdate( const char *datestr )
{

	time_t t;
	struct tm *d;
	char *dt = (char *)datestr;

	if ( datestr == NULL )
		return NULL;
	t =  time(NULL);
	d = localtime(&t);
	sprintf(dt,"%d\%d\%d",d->tm_mday,d->tm_mon+1,d->tm_year);
	return dt;
}