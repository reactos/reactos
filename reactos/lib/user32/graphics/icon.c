
#include <windows.h>



HICON
STDCALL
CreateIcon(
	   HINSTANCE hInstance,
	   int nWidth,
	   int nHeight,
	   BYTE cPlanes,
	   BYTE cBitsPixel,
	   CONST BYTE *lpbANDbits,
	   CONST BYTE *lpbXORbits)
{
#if 0
	
    ICONINFO IconInfo;
    IconInfo.fIcon = TRUE;
    IconInfo.hbmMask = NULL;
    IconInfo.hbmColor = NULL;
    return CreateIconIndirect( &IconInfo );
#endif
}

HICON
STDCALL
CreateIconIndirect(
		   PICONINFO piconinfo)
{
}

 
HICON
STDCALL
CopyIcon(
	 HICON hIcon)
{
}

 
WINBOOL
STDCALL
GetIconInfo(
	    HICON hIcon,
	    PICONINFO piconinfo)
{
}




HICON LoadIconA(HINSTANCE hInstance,LPCSTR  lpIconName )
{
	return CreateIcon(hInstance,	GetSystemMetrics(SM_CXSMICON),
	GetSystemMetrics(SM_CYSMICON),
 	0,0,NULL,NULL);
}

HICON LoadIconW(HINSTANCE hInstance,LPCWSTR  lpIconName )
{
	return CreateIcon(hInstance,	GetSystemMetrics(SM_CXSMICON),
	GetSystemMetrics(SM_CYSMICON),
 	0,0,NULL,NULL);
}