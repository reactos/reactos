/*
* created:	Boudewijn Dekker
* org. source:	WINE
* date:		june 1998
* todo:		check the _lopen for correctness
*/

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <wstring.h>
#include <fcntl.h>



long _hread(
    HFILE  hFile,	
    LPVOID  lpBuffer,	
    long  lBytes 	
   )
{
	DWORD  NumberOfBytesRead;
	if ( ReadFile((HANDLE)hFile,(LPCVOID)lpBuffer,(DWORD)lBytes,&NumberOfBytesRead, NULL) == FALSE )
		return -1;
	else
		return NumberOfBytesRead;

}

UINT STDCALL _lread(HFILE fd,LPVOID buffer,UINT count)
{
    return _hread(fd,buffer, count);
}


long _hwrite(
    HFILE  hFile,	
    LPCSTR  lpBuffer,	
    long  lBytes 	
   )
{

	DWORD  NumberOfBytesWritten;
	if ( lBytes == 0 ) {
		if ( SetEndOfFile((HANDLE) hFile ) == FALSE )
			return -1;
		else
			return 0;
	}
	if ( WriteFile((HANDLE)hFile,(LPCVOID)lpBuffer,(DWORD)lBytes, &NumberOfBytesWritten,NULL) == FALSE )
		return -1;
	else
		return NumberOfBytesWritten;

}

UINT
STDCALL
_lwrite(
	HFILE hFile,
	LPCSTR lpBuffer,
	UINT uBytes
	)
{
	return _hwrite(hFile,lpBuffer,uBytes);
}

#define OF_OPENMASK	(OF_READ|OF_READWRITE|OF_WRITE|OF_CREATE)
#define OF_FILEMASK	(OF_DELETE|OF_PARSE)
#define OF_MASK		(OF_OPENMASK|OF_FILEMASK)

HFILE _open( LPCSTR  lpPathName, int  iReadWrite 	 )
{


	
	HFILE fd;
	int nFunction;
	


   	
	/* Don't assume a 1:1 relationship between OF_* modes and O_* modes */
	/* Here we translate the read/write permission bits (which had better */
	/* be the low 2 bits.  If not, we're in trouble.  Other bits are  */
	/* passed through unchanged */
	
	nFunction = wFunction & 3;
   	
	switch (wFunction & 3) {
		case OF_READ:
			nFunction |= O_RDONLY;
   			break;
   		case OF_READWRITE:
   			nFunction |= O_RDWR;
   			break;
   		case OF_WRITE:
   			nFunction |= O_WRONLY;
   			break;
   		default:
   			//ERRSTR((LF_ERROR, "_lopen: bad file open mode %x\n", wFunction));
   			return HFILE_ERROR;
   	}
	SetLastError(0);
	fd = CreateFileA( filename,nFunction,OPEN_EXISTING,
			NULL,OPEN_EXISTING,NULL,NULL);	
	if (fd == INVALID_HANDLE_VALUE )
		 return HFILE_ERROR;
	return fd;
}

int _creat(const char *filename, int pmode)
{
	SetLastError(0);
	return CreateFileA( filename,GENERIC_READ & GENERIC_WRITE,FILE_SHARE_WRITE,
			NULL,CREATE_ALWAYS,pmode & 0x00003FB7,NULL);	
}


int _lclose(
    HFILE  hFile 	
   )
{
	if ( CloseHandle((HANDLE)hFile) )
		return 0;
	else
		return -1; 
}

LONG _llseek(
    HFILE  hFile,
    LONG  lOffset, 
    int  iOrigin 
   )
{
	return  SetFilePointer((HANDLE)  hFile,  lOffset, NULL,(DWORD)iOrigin );
}


