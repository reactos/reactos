#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "dirent.h"

DIR *opendir (const char * a_dir)
{
        APIRET          rc;
        FILEFINDBUF3    FindBuffer = {0};
        ULONG           FileCount  = 1;
	DIR             *dd_dir = (DIR*) malloc (sizeof(DIR));
	char            *c_dir = (char*) malloc (strlen(a_dir) + 5);

	strcpy (c_dir, a_dir);
	strcat (c_dir, "\\*.*");
        dd_dir->d_handle = (unsigned long*) HDIR_CREATE;
        
        rc = DosFindFirst(c_dir, 
            (PHDIR) &dd_dir->d_handle,
            FILE_SYSTEM | FILE_HIDDEN | FILE_DIRECTORY,
            (PVOID) &FindBuffer,
            sizeof(FILEFINDBUF3),
            &FileCount,
            FIL_STANDARD);

        if (rc) {
	    switch (rc) {
	    case ERROR_NO_MORE_FILES:
	    case ERROR_FILE_NOT_FOUND:
	    case ERROR_PATH_NOT_FOUND:
		errno = ENOENT;
		break;
	    case ERROR_BUFFER_OVERFLOW:
		errno = ENOMEM;
		break;
	    default: 
		errno = EINVAL;
		break;
	    }
	    free(dd_dir);
	    return NULL;
        }
	dd_dir->d_attr = FindBuffer.attrFile;
	dd_dir->d_time = dd_dir->d_date = 10; 
	dd_dir->d_size = FindBuffer.cbFile;
	strcpy (dd_dir->d_name, FindBuffer.achName);
	dd_dir->d_first = 1;
	
	free (c_dir);
	return dd_dir;
}

DIR *readdir( DIR * dd_dir)
{
        APIRET          rc;
        FILEFINDBUF3    FindBuffer = {0};
        ULONG           FileCount  = 1;
	DIR             *ret_dir = (DIR*) malloc (sizeof(DIR));
	
	if (dd_dir->d_first) {
		dd_dir->d_first = 0;
		return dd_dir;
	}

        rc = DosFindNext((HDIR) dd_dir->d_handle, 			
                        (PVOID) &FindBuffer,
                        sizeof(FILEFINDBUF3),
                        &FileCount);

        if (rc) {
	    switch (rc) {
	    case ERROR_NO_MORE_FILES:
	    case ERROR_FILE_NOT_FOUND:
	    case ERROR_PATH_NOT_FOUND:
		errno = ENOENT;
		break;
	    case ERROR_BUFFER_OVERFLOW:
		errno = ENOMEM;
		break;
	    default: 
		errno = EINVAL;
		break;
	    }
	    return NULL;
        }

	ret_dir->d_attr = FindBuffer.attrFile; 
	ret_dir->d_time = ret_dir->d_date = 10;
	ret_dir->d_size = FindBuffer.cbFile;
	strcpy (ret_dir->d_name, FindBuffer.achName);
	return ret_dir;
}		  

int closedir (DIR *dd_dir)
{
   if (dd_dir->d_handle != (unsigned long*) HDIR_CREATE) {
	DosFindClose((HDIR) dd_dir->d_handle);
   }
   free (dd_dir);
   return 1;
}
