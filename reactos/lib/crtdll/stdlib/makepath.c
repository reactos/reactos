#include <crtdll/stdlib.h>
#include <crtdll/string.h>

void _makepath( char *path, const char *drive, const char *dir, const char *fname, const char *ext )
{
	int dir_len;
	if ( drive != NULL ) {
		strcpy(path,drive);
		strcat(path,":");
	}
        else
            (*path)=0;

	if ( dir != NULL ) {
		strcat(path,dir);
		if ( *dir != '\\' )
			strcat(path,"\\");
		dir_len = strlen(dir);
		if ( dir_len && *(dir + dir_len - 1) != '\\' ) 
			strcat(path,"\\"); 
	}
	if ( fname != NULL ) {
		strcat(path,fname);
		if ( ext != NULL ) {
			if ( *ext != '.')
				strcat(path,".");
			strcat(path,ext);
		}
	}
	
		
}
