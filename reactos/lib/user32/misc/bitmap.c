#include <windows.h>

HBITMAP BITMAP_LoadBitmapW(HINSTANCE instance,LPCWSTR name,  UINT loadflags);



HBITMAP 
STDCALL 
LoadBitmapW(HINSTANCE  hInstance, LPCWSTR  lpBitmapName)
{
	return BITMAP_LoadBitmapW(hInstance, lpBitmapName, 0);
}

HBITMAP 
STDCALL
LoadBitmapA(HINSTANCE  hInstance, LPCSTR  lpBitmapName)
{
	return CreateBitmap(GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CXSMICON),
		1,1, NULL );
	return NULL;
//	return BITMAP_LoadBitmap32W(hInstance, lpBitmapName, 0);
}

HBITMAP BITMAP_LoadBitmapW(HINSTANCE instance,LPCWSTR name, 
			       UINT loadflags)
{
		return CreateBitmap(GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CXSMICON),
		1,1, NULL );
#if 0
    HBITMAP hbitmap = 0;
    HDC hdc;
    HRSRC hRsrc;
    HGLOBAL handle;
    char *ptr = NULL;
    BITMAPINFO *info, *fix_info=NULL;
    HGLOBAL hFix;
    int size;

    if (!(loadflags & LR_LOADFROMFILE)) {
      if (!instance)  /* OEM bitmap */
      {
          HDC hdc;
	  DC *dc;

	  if (HIWORD((int)name)) return 0;
	  hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
	  dc = DC_GetDCPtr( hdc );
	  if(dc->funcs->pLoadOEMResource)
	      hbitmap = dc->funcs->pLoadOEMResource( LOWORD((int)name), 
						     OEM_BITMAP );
	  GDI_HEAP_UNLOCK( hdc );
	  DeleteDC( hdc );
	  return hbitmap;
      }

      if (!(hRsrc = FindResourceW( instance, name, RT_BITMAPW ))) return 0;
      if (!(handle = LoadResource( instance, hRsrc ))) return 0;

      if ((info = (BITMAPINFO *)LockResource( handle )) == NULL) return 0;
    }
    else
    {
        if (!(ptr = (char *)VIRTUAL_MapFileW( name ))) return 0;
        info = (BITMAPINFO *)(ptr + sizeof(BITMAPFILEHEADER));
    }
    size = DIB_BitmapInfoSize(info, DIB_RGB_COLORS);
    if ((hFix = GlobalAlloc(0, size))) fix_info=GlobalLock(hFix);
    if (fix_info) {
      BYTE pix;

      memcpy(fix_info, info, size);
      pix = *((LPBYTE)info+DIB_BitmapInfoSize(info, DIB_RGB_COLORS));
      DIB_FixColorsToLoadflags(fix_info, loadflags, pix);
      if ((hdc = GetDC(0)) != 0) {
	if (loadflags & LR_CREATEDIBSECTION)
	  hbitmap = CreateDIBSection(hdc, fix_info, DIB_RGB_COLORS, NULL, 0, 0);
        else {
          char *bits = (char *)info + size;;
          hbitmap = CreateDIBitmap( hdc, &fix_info->bmiHeader, CBM_INIT,
                                      bits, fix_info, DIB_RGB_COLORS );
	}
        ReleaseDC( 0, hdc );
      }
      GlobalUnlock(hFix);
      GlobalFree(hFix);
    }
    if (loadflags & LR_LOADFROMFILE) UnmapViewOfFile( ptr );
    return hbitmap;
#endif
}



