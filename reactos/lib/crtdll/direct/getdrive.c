#include <direct.h>
#include <windows.h>

extern int cur_drive;



int _getdrive( void )
{
	char Buffer[MAX_PATH];

	if ( cur_drive == 0 ) {
		GetCurrentDirectory(MAX_PATH,Buffer);
		cur_drive = toupper(Buffer[0] - '@');
	}
	
	return cur_drive;
}
