#include <direct.h>
#include <stdlib.h>
#include <windows.h>

int cur_drive = 0;



int _chdrive( int drive )
{
	char d[3];
	if (!( drive >= 1 && drive <= 26 )) 
		return -1;

	if ( cur_drive != drive ) {
		cur_drive = drive;
		d[0] = toupper(cur_drive + '@');
		d[1] = ':';
		d[2] = 0;
		SetCurrentDirectory(d);
	}


	return 0;
}
