#include <io.h>
#include <windows.h>

#define F_OK	0x01
#define R_OK	0x02
#define W_OK	0x04
#define X_OK	0x08
#define D_OK	0x10

int access(const char *_path, int _amode)
{
	return _access(_path,_amode);
}

int _access( const char *_path, int _amode )
{
	DWORD Attributes = GetFileAttributesA(_path);

	if ( Attributes == -1 )
		return -1;

	if ( _amode & W_OK == W_OK ) {
		if ( (Attributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY )
			return -1;
	}
	if ( _amode & D_OK == D_OK ) {
		if ( (Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY )
			return 0;
	}

	return 0;
		
}
