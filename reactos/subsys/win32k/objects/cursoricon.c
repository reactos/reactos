#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/cursoricon.h>
#include <win32k/bitmaps.h>

#define NDEBUG
#include <win32k/debug1.h>

BOOL FASTCALL IconCursor_InternalDelete( PICONCURSOROBJ pIconCursor )
{
	ASSERT( pIconCursor );
	if( pIconCursor->ANDBitmap.bmBits )
		ExFreePool(pIconCursor->ANDBitmap.bmBits);
	if( pIconCursor->XORBitmap.bmBits )
		ExFreePool(pIconCursor->XORBitmap.bmBits);	
	return TRUE;
}

HICON STDCALL NtGdiCreateIcon(BOOL fIcon,
                             INT  Width,
                             INT  Height,
                             UINT  Planes,
                             UINT  BitsPerPel,
                             DWORD xHotspot,
                             DWORD yHotspot,
                             CONST VOID *ANDBits,
                             CONST VOID *XORBits)
{
	PICONCURSOROBJ icon;
	HICON hIcon;
	
	Planes = (BYTE) Planes;
	BitsPerPel = (BYTE) BitsPerPel;	

	/* Check parameters */
	if (!Height || !Width)
	{
		return 0;
	}
	if (Planes != 1)
	{
		UNIMPLEMENTED;
		return  0;
	}
	
	/* Create the ICONCURSOROBJ object*/
	hIcon = ICONCURSOROBJ_AllocIconCursor ();
	if (!hIcon)
	{
		DPRINT("NtGdiCreateIcon: ICONCURSOROBJ_AllocIconCursor() returned 0\n");
		return 0;
	}
	
	icon = ICONCURSOROBJ_LockIconCursor(hIcon);
	
	/* Set up the basic icon stuff */
	icon->fIcon = TRUE;
	icon->xHotspot = xHotspot;
	icon->yHotspot = yHotspot;
	
	/* Setup the icon mask and icon color bitmaps */
	icon->ANDBitmap.bmType = 0;
	icon->ANDBitmap.bmWidth = Width;
	icon->ANDBitmap.bmHeight = Height;
	icon->ANDBitmap.bmPlanes = 1;
	icon->ANDBitmap.bmBitsPixel = 1;
	icon->ANDBitmap.bmWidthBytes = BITMAPOBJ_GetWidthBytes (Width, 1);
	icon->ANDBitmap.bmBits = NULL;

	icon->XORBitmap.bmType = 0;
	icon->XORBitmap.bmWidth = Width;
	icon->XORBitmap.bmHeight = Height;
	icon->XORBitmap.bmPlanes = Planes;
	icon->XORBitmap.bmBitsPixel = BitsPerPel;
	icon->XORBitmap.bmWidthBytes = BITMAPOBJ_GetWidthBytes (Width, BitsPerPel);
	icon->XORBitmap.bmBits = NULL;	
	
	/* allocate memory for the icon mask and icon color bitmaps,  
	   this will be freed in IconCursor_InternalDelete */
    icon->ANDBitmap.bmBits = ExAllocatePool(PagedPool, Height * icon->ANDBitmap.bmWidthBytes);
    icon->XORBitmap.bmBits = ExAllocatePool(PagedPool, Height * icon->XORBitmap.bmWidthBytes);

	/* set the bits of the mask and color bitmaps */
	if (ANDBits)
	{
		memcpy(icon->ANDBitmap.bmBits, (PVOID)ANDBits, Height * icon->ANDBitmap.bmWidthBytes);
	}

	if (XORBits)
	{
		memcpy(icon->XORBitmap.bmBits, (PVOID)XORBits, Height * icon->XORBitmap.bmWidthBytes);		
	}
	
	ICONCURSOROBJ_UnlockIconCursor( hIcon );

	return hIcon;
}

DWORD
STDCALL
NtUserGetIconInfo(
  HICON hIcon,
  PBOOL fIcon,
  PDWORD xHotspot,
  PDWORD yHotspot,
  HBITMAP *hbmMask,
  HBITMAP *hbmColor)
{
  PICONCURSOROBJ icon;

  icon = ICONCURSOROBJ_LockIconCursor(hIcon);
  
  if (!icon)
  {
	DPRINT1("NtUserGetIconInfo: ICONCURSOROBJ_LockIconCursor() returned 0\n");
	return FALSE;
  }

  *fIcon = icon->fIcon ;
  *xHotspot = icon->xHotspot;
  *yHotspot = icon->yHotspot;

  *hbmMask = NtGdiCreateBitmap(icon->ANDBitmap.bmWidth,
                              icon->ANDBitmap.bmHeight,
                              icon->ANDBitmap.bmPlanes,
                              icon->ANDBitmap.bmBitsPixel,
                              icon->ANDBitmap.bmBits);

  *hbmColor = NtGdiCreateBitmap(icon->XORBitmap.bmWidth,
                               icon->XORBitmap.bmHeight,
                               icon->XORBitmap.bmPlanes,
                               icon->XORBitmap.bmBitsPixel,
                               icon->XORBitmap.bmBits);

  ICONCURSOROBJ_UnlockIconCursor(hIcon);

  if (!*hbmMask || !*hbmColor)
    return FALSE;
    
  return TRUE;
}

DWORD
STDCALL
NtUserGetIconSize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCursorInfo(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserClipCursor(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDestroyCursor(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserFindExistingCursorIcon(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipCursor(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetCursor(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}
DWORD
STDCALL
NtUserSetCursorContents(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetCursorIconData(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetSystemCursor(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}
