#include <windows.h>
#include <msvcrt/io.h>

/*
 * @implemented
 */
int _eof( int _fd )
{
	int cur_pos = _lseek(_fd, 0, SEEK_CUR);
	int end_pos = _filelength( _fd );
	if ( cur_pos == -1 || end_pos == -1)
		return -1;

	if ( cur_pos == end_pos )
		return 1;

	return 0;
}
