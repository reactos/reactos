#include <crtdll/stdlib.h>
#include <crtdll/sys/utime.h>
#include <crtdll/io.h>
#include <crtdll/internal/file.h>

int     _futime (int nHandle, struct _utimbuf *pTimes)
{
	FILETIME  LastAccessTime;
	FILETIME  LastWriteTime;
	UnixTimeToFileTime(pTimes->actime,&LastAccessTime,0);
	UnixTimeToFileTime(pTimes->modtime,&LastWriteTime,0);
	if ( !SetFileTime(_get_osfhandle(nHandle),NULL, &LastAccessTime, &LastWriteTime) )
		return -1;

	return 0;
}