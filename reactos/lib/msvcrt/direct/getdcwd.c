#include <msvcrti.h>


char* _getdcwd (int nDrive, char* caBuffer, int nBufLen)
{
	int i =0;
	int dr = _getdrive();
	
	if ( nDrive < 1 || nDrive > 26 )
		return NULL;
	
	if ( dr != nDrive )
		_chdrive(nDrive);
	
	i = GetCurrentDirectoryA(nBufLen,caBuffer);
	if ( i  == nBufLen )
		return NULL;
	
	if ( dr != nDrive )
		_chdrive(dr);
	
	return caBuffer;
}

wchar_t* _wgetdcwd (int nDrive, wchar_t* caBuffer, int nBufLen)
{
	int i =0;
	int dr = _getdrive();
	
	if ( nDrive < 1 || nDrive > 26 )
		return NULL;
	
	if ( dr != nDrive )
		_chdrive(nDrive);
	
	i = GetCurrentDirectoryW(nBufLen,caBuffer);
	if ( i  == nBufLen )
		return NULL;
	
	if ( dr != nDrive )
		_chdrive(dr);
	
	return caBuffer;
}
