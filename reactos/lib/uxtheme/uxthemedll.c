#include "uxthemedll.h"
#include "nostyle/button.h"

#include <stdlib.h>

#define HTHEME_TO_UXTHEME_(H__) ((PUXTHEME_DATA)(H__))

STDAPI UxTheme_ClipDc(HDC hdc, const RECT * pClipRect, HRGN * phrgnSave)
{
 *phrgnSave = CreateRectRgn(0, 0, 0, 0);

 switch(GetClipRgn(hdc, *phrgnSave))
 {
  /* no user-defined clipping region */
  case 0:
  {
   HRGN hrgnClip;

   /* no region to restore */
   DeleteObject(*phrgnSave);
   *phrgnSave = NULL;

   /* create the clipping rectangle */
   if((hrgnClip = CreateRectRgnIndirect(pClipRect)) != NULL)
   {
    /* select the clipping rectangle */
    if(SelectClipRgn(hdc, hrgnClip) != ERROR)
    {
     /* success */
     DeleteObject(hrgnClip);
     return S_OK;
    }

    /* failure */
    DeleteObject(hrgnClip);
   }

   break;
  }

  /* user-defined clipping region */
  case 1:
  {
   HRGN hrgnClip;
   
   /* create the clipping rectangle */
   if((hrgnClip = CreateRectRgnIndirect(pClipRect)) != NULL)
   {
    /* select the clipping rectangle */
    if(ExtSelectClipRgn(hdc, hrgnClip, RGN_AND) != ERROR)
    {
     /* success */
     DeleteObject(hrgnClip);
     return S_OK;
    }

    /* failure */
    DeleteObject(hrgnClip);
   }

   break;
  }

  /* error */
  case -1:
  {
   /* no region to restore */
   DeleteObject(*phrgnSave);
   *phrgnSave = NULL;

   break;
  }
 }

 /* failure */
 return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI UxTheme_UnclipDc(HDC hdc, HRGN hrgnSave)
{
 HRESULT hres;
 
 if(hrgnSave == NULL) return S_FALSE;

 if(SelectClipRgn(hdc, hrgnSave) != ERROR) hres = S_OK;
 else hres = HRESULT_FROM_WIN32(GetLastError());

 DeleteObject(hrgnSave);
 return hres;
}

THEMEAPI DrawThemeBackground
(
 HTHEME hTheme,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pRect,
 const RECT * pClipRect
)
{
 PUXTHEME_DATA pUxTheme = HTHEME_TO_UXTHEME_(hTheme);

 return pUxTheme->pvt->p_DrawBackground
 (
  pUxTheme,
  hdc,
  iPartId,
  iStateId,
  pRect,
  pClipRect
 );
}

THEMEAPI DrawThemeEdge
(
 HTHEME hTheme,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pDestRect,
 UINT uEdge,
 UINT uFlags,
 RECT * pContentRect
)
{
 return E_FAIL;
}

THEMEAPI DrawThemeIcon
(
 HTHEME hTheme,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pRect,
 HIMAGELIST himl,
 int iImageIndex
)
{
 return E_FAIL;
}

THEMEAPI DrawThemeParentBackground
(
 HWND hwnd,
 HDC hdc,
 RECT * prc
)
{
 return E_FAIL;
}

THEMEAPI_(BOOL) IsThemeBackgroundPartiallyTransparent
(
 HTHEME hTheme,
 int iPartId,
 int iStateId
)
{
 return FALSE;
}

THEMEAPI_(HTHEME) OpenThemeData
(
 HWND hwnd,
 LPCWSTR pszClassList
)
{
 PUXTHEME_DATA pUxTheme;
 PCUXTHEME_VTABLE pvt;

 if(_wcsicmp(pszClassList, L"Button") == 0) pvt = &Button_Vt;
 else return NULL;

 pUxTheme = (PUXTHEME_DATA)malloc(sizeof(*pUxTheme));

 if(pUxTheme == NULL) return NULL;

 pUxTheme->pvt = pvt;

 return (HTHEME)pUxTheme;
}

THEMEAPI CloseThemeData
(
 HTHEME hTheme
)
{
 return E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved)
{
 (void)lpvReserved;

 if(fwdreason == DLL_PROCESS_ATTACH)
  DisableThreadLibraryCalls(hinstDLL);

 return TRUE;
}

/* EOF */


