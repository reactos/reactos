#include <windows.h>
#include <io.h>

int _access( const char *_path, int _amode )
{
	DWORD Attributes = GetFileAttributesA(_path);

	if ( Attributes == -1 )
		return -1;

	if ( (_amode & W_OK) == W_OK ) {
		if ( (Attributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY )
			return -1;
	}
	if ( (_amode & D_OK) != D_OK ) {
		if ( (Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY )
			return -1;
	}

	return 0;
		
}
