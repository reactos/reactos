#include <direct.h>

char*	_getdcwd (int nDrive, char* caBuffer, int nBufLen)
{
	int i =0;
	int dr = getdrive();
	
	if ( nDrive < 1 || nDrive > 26 )
		return NULL;
	
	if ( dr != nDrive )
		chdrive(nDrive);
	
	i = GetCurrentDirectory(nBufLen,caBuffer);
	if ( i  == nBufLen )
		return NULL;
	
	if ( dr != nDrive )
		chdrive(dr);
	
	return caBuffer;
}
