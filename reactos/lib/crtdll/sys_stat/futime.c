#include <crtdll/alloc.h>
#include <crtdll/stdlib.h>
#include <crtdll/sys/utime.h>
#include <crtdll/io.h>
#include <crtdll/time.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

int     _futime (int nHandle, struct _utimbuf *pTimes)
{
        FILETIME  LastAccessTime;
        FILETIME  LastWriteTime;
        
        // check for stdin / stdout  handles ??
        if ( nHandle == -1 ) {
        	__set_errno(EBADF);
		return -1;
	}  
        
        if ( pTimes == NULL ) {
  		pTimes = alloca(sizeof(struct _utimbuf));
  		time(&pTimes->actime);
  		time(&pTimes->modtime);
  	}
  	
  	if ( pTimes->actime < pTimes->modtime  ) {
        	__set_errno(EINVAL);
        	return -1;
        }
               
        UnixTimeToFileTime(pTimes->actime,&LastAccessTime,0);
        UnixTimeToFileTime(pTimes->modtime,&LastWriteTime,0);
        if ( !SetFileTime(_get_osfhandle(nHandle),NULL, &LastAccessTime, &LastWriteTime) ) {
        	__set_errno(EBADF);
                return -1;
        }

        return 0;
}