#include <stdlib.h>
#include <string.h>

/*
 * @implemented
 */
void _splitpath( const char *path, char *drive, char *dir, char *fname, char *ext )
{
	char *tmp_drive;
	char *tmp_dir;
	char *tmp_ext;

	tmp_drive = (char *)strchr(path,':');
	if ( tmp_drive != (char *)NULL ) {
		strncpy(drive,tmp_drive-1,1);
		*(drive+1) = 0;
	}
	else {
		*drive = 0; 
		tmp_drive = (char *)path;
	}

	tmp_dir = (char *)strrchr(path,'\\');
	if( tmp_dir != NULL && tmp_dir != tmp_drive + 1 ) {
		strncpy(dir,tmp_drive+1,tmp_dir - tmp_drive);
		*(dir + (tmp_dir - tmp_drive)) = 0;
	}
	else 	
		*dir =0;

	tmp_ext = ( char *)strrchr(path,'.');
	if ( tmp_ext != NULL ) {
		strcpy(ext,tmp_ext);
	}
	else
		*ext = 0; 
    if ( tmp_dir != NULL ) {
		strncpy(fname,tmp_dir+1,tmp_ext - tmp_dir - 1);
		*(fname + (tmp_ext - tmp_dir -1)) = 0;
	}
	else
		strncpy(fname,path,tmp_ext - path);

}

