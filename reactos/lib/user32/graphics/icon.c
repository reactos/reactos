
#include <windows.h>

//#include <user32/static.h>

HICON LoadStandardIcon(UINT IconId);

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
		   PICONINFO lpIconInfo)
{
    BITMAP bmpXor,bmpAnd;
    HICON hObj;
    int	sizeXor,sizeAnd;

    GetObject(lpIconInfo->hbmColor,sizeof(BITMAP),&bmpXor);
    GetObject(lpIconInfo->hbmMask,sizeof(BITMAP),&bmpAnd);
   
   

    sizeXor = bmpXor.bmHeight * bmpXor.bmWidthBytes;
    sizeAnd = bmpAnd.bmHeight * bmpAnd.bmWidthBytes;

    hObj = GlobalAlloc( GMEM_MOVEABLE, 
		     sizeof(ICONINFO) + sizeXor + sizeAnd );
    if (hObj)
    {
	ICONINFO *info;

	info = (ICONINFO *)( hObj );
	info->xHotspot   = lpIconInfo->xHotspot;
	info->yHotspot   = lpIconInfo->yHotspot;
	//info->nWidth        = bmpXor.bmWidth;
	//info->nHeight       = bmpXor.bmHeight;
	//info->nWidthBytes   = bmpXor.bmWidthBytes;
	//info->bPlanes       = bmpXor.bmPlanes;
	//info->bBitsPerPixel = bmpXor.bmBitsPixel;

	/* Transfer the bitmap bits to the CURSORICONINFO structure */

	GetBitmapBits( lpIconInfo->hbmMask ,sizeAnd,(char*)(info + 1) );
	GetBitmapBits( lpIconInfo->hbmColor,sizeXor,(char*)(info + 1) +sizeAnd);
	
    }
    return hObj;
}

WINBOOL
STDCALL
DestroyIcon(
	    HICON hIcon)
{
	return FALSE;
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
	
	return TRUE;
}




HICON
STDCALL
LoadIconA(HINSTANCE hInstance,LPCSTR  lpIconName )
{
	HRSRC hrsrc;
	ICONINFO *IconInfo;
	
	if ( hInstance == NULL ) {
		return LoadStandardIcon((UINT)lpIconName);
	}
//RT_GROUP_ICON
	hrsrc = FindResourceExA(hInstance,RT_GROUP_ICON, lpIconName, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

	if ( hrsrc == NULL )
		return NULL;

	IconInfo = (ICONINFO *)LoadResource(hInstance, hrsrc);
	if ( IconInfo != NULL || IconInfo->fIcon == FALSE )
		return NULL;	

    	return CreateIconIndirect(IconInfo);
}

HICON 
STDCALL
LoadIconW(HINSTANCE hInstance,LPCWSTR  lpIconName )
{
	HRSRC hrsrc;
	ICONINFO *IconInfo;

	if ( hInstance == NULL ) {
		return LoadStandardIcon((UINT)lpIconName);
	}

	hrsrc = FindResourceW(hInstance,lpIconName,RT_GROUP_ICON);
	if ( hrsrc == NULL )
		return NULL;

	IconInfo = (ICONINFO *)LoadResource(hInstance, hrsrc);
	if ( IconInfo != NULL || IconInfo->fIcon == FALSE )
		return NULL;	

    	return CreateIconIndirect(IconInfo);
}

HICON LoadStandardIcon(UINT IconId)
{
	HMODULE hModule = LoadLibraryA("user32.dll");
	switch (IconId )
	{	
		case IDI_APPLICATION:
			IconId = 100;
			return LoadIconW(hModule,(LPWSTR)IconId);
			break;
		case IDI_ASTERISK:
		//
			IconId = 103;
			return LoadIconW(hModule,(LPWSTR)IconId);
			break;
		case IDI_EXCLAMATION:
			IconId = 101;
			return LoadIconW(hModule,(LPWSTR)IconId);
			break;
		case IDI_HAND:
		// 
			return LoadIconW(hModule,(LPWSTR)MAKEINTRESOURCE(104));
			break;
		case IDI_QUESTION:
			IconId = 102;
			return LoadIconW(hModule,(LPWSTR)IconId);
			break;
		case IDI_WINLOGO:
			IconId = 105;
			return LoadIconW(hModule,(LPWSTR)IconId);
			break;
		default:
			return NULL;
			break;
	
	}
	return NULL;
}

WINBOOL STDCALL DrawIcon(HDC  hDC, int  xLeft, int  yTop, HICON  hIcon )
{
	
	return DrawIconEx( hDC, xLeft, yTop,hIcon, -1, -1,0,NULL, DI_DEFAULTSIZE);
}

WINBOOL
STDCALL
DrawIconEx(HDC hdc, int xLeft, int yTop,
	   HICON hIcon, int cxWidth, int cyWidth,
	   UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
	//ICONINFO IconInfo;
	//SIZE Size;;
	//GetIconInfo(hIcon,&IconInfo);
	//GetBitmapDimensionEx(IconInfo.hbmMask,&Size);
	return FALSE;
}

WINBOOL
STDCALL
DrawFocusRect(
	      HDC hDC,
	      CONST RECT * lprc)
{
	return FALSE;
}