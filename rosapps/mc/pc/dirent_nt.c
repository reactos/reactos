
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <errno.h>
#include "dirent.h"

DIR *opendir (const char * a_dir)
{
	int err;
	WIN32_FIND_DATA wfd;
	DIR* dd_dir = (DIR*) malloc (sizeof(DIR));

	char *c_dir = malloc (strlen(a_dir) + 4);
	strcpy (c_dir, a_dir);
	strcat (c_dir, "\\*");
			
	dd_dir->d_handle = FindFirstFile (c_dir, &wfd);
	if (dd_dir->d_handle == INVALID_HANDLE_VALUE)	{
		err = GetLastError();
		switch (err) {
			case ERROR_NO_MORE_FILES:
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				errno = ENOENT;
				break;
        		case ERROR_NOT_ENOUGH_MEMORY:
				errno = ENOMEM;
				break;
        		default:
				errno = EINVAL;
				break;
		}
		free(dd_dir);
		return NULL;
	}
	dd_dir->d_attr = (wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
                   		? 0 : wfd.dwFileAttributes;

	dd_dir->d_time = dd_dir->d_date = 10;
	dd_dir->d_size = wfd.nFileSizeLow;
	strcpy (dd_dir->d_name, wfd.cFileName);
	dd_dir->d_first = 1;
	
	free (c_dir);
	return dd_dir;
}

DIR *readdir( DIR * dd_dir)
{
	int err;
	WIN32_FIND_DATA wfd;
	
	if (dd_dir->d_first) {
		dd_dir->d_first = 0;
		return dd_dir;
	}
			
	if(!FindNextFile (dd_dir->d_handle, &wfd)) {
		err = GetLastError();
		switch (err) {
			case ERROR_NO_MORE_FILES:
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				errno = ENOENT;
				break;
        		case ERROR_NOT_ENOUGH_MEMORY:
				errno = ENOMEM;
				break;
        		default:
				errno = EINVAL;
				break;
		}
		return NULL;
	}
	dd_dir->d_attr = (wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
                   		? 0 : wfd.dwFileAttributes;

	dd_dir->d_time = dd_dir->d_date = 10;
	dd_dir->d_size         = wfd.nFileSizeLow;
	strcpy (dd_dir->d_name, wfd.cFileName);
	return dd_dir;
}		  

int closedir (DIR *dd_dir)
{
	FindClose(dd_dir->d_handle);
	free (dd_dir);
	return 1;
}

