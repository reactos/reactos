/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/time/strtime.c
 * PURPOSE:     Fills a buffer with a formatted date representation
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrt/time.h>
#include <msvcrt/stdio.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


char *_strdate( const char *datestr )
{
	time_t t;
	struct tm *d;
	char *dt = (char *)datestr;

	if ( datestr == NULL ){
		__set_errno(EINVAL);
		return NULL;
	}
	t =  time(NULL);
	d = localtime(&t);
	sprintf(dt,"%d/%d/%d",d->tm_mday,d->tm_mon+1,d->tm_year);
	return dt;
}
