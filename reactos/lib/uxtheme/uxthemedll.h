#ifndef UXTHEMEDLL_H_INCLUDED__
#define UXTHEMEDLL_H_INCLUDED__

#include <windows.h>

#define _UXTHEME_
#include <uxtheme.h>
#include <tmschema.h>

#ifdef __cplusplus
extern "C"
{
#endif 

struct UXTHEME_DATA_;
struct UXTHEME_VTABLE_;

struct UXTHEME_VTABLE_
{
 HRESULT (STDAPICALLTYPE * p_DrawBackground)
 (
  IN OUT struct UXTHEME_DATA_ * pData,
  IN HDC hdc,
  IN int iPartId,
  IN int iStateId,
  IN const RECT * pRect,
  IN const RECT * pClipRect
 );

 HRESULT (STDAPICALLTYPE * p_DrawText)
 (
  IN OUT struct UXTHEME_DATA_ * pData,
  IN HDC hdc,
  IN int iPartId,
  IN int iStateId,
  IN LPCWSTR pszText,
  IN int iCharCount,
  IN DWORD dwTextFlags,
  IN DWORD dwTextFlags2,
  IN const RECT * pRect
 );

 HRESULT (STDAPICALLTYPE * p_GetBackgroundContentRect)
 (
  IN OUT struct UXTHEME_DATA_ * pData,
  IN HDC hdc,
  IN int iPartId,
  IN int iStateId,
  IN const RECT * pBoundingRect,
  OUT RECT * pContentRect
 );
};

typedef struct UXTHEME_VTABLE_ UXTHEME_VTABLE, * PUXTHEME_VTABLE;
typedef const struct UXTHEME_VTABLE_ * PCUXTHEME_VTABLE;

typedef struct UXTHEME_DATA_
{
 PCUXTHEME_VTABLE pvt;
 PVOID pData;                                  
}
UXTHEME_DATA, * PUXTHEME_DATA;

STDAPI UxTheme_ClipDc(HDC hdc, const RECT * pClipRect, HRGN * phrgnSave);
STDAPI UxTheme_UnclipDc(HDC hdc, HRGN hrgnSave);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
