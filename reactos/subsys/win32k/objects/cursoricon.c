#undef WIN32_LEAN_AND_MEAN
#include <win32k/win32k.h>
#include <windows.h>
#include <stdlib.h>
#include <win32k/cursoricon.h>
#include <win32k/bitmaps.h>
#include <include/winsta.h>
#include <include/error.h>

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


/*
 * @implemented
 */
HICON 
STDCALL 
NtGdiCreateIcon(BOOL fIcon,
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
		DPRINT("NtGdiCreateIcon: ICONCURSOROBJ_AllocIconCursor(hIcon == 0x%x) returned 0\n", hIcon);
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


/*
 * @implemented
 */
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
	DPRINT1("NtUserGetIconInfo: ICONCURSOROBJ_LockIconCursor(hIcon == 0x%x) returned 0\n", hIcon);
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


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetIconSize(
  HICON hIcon,
  BOOL *fIcon,
  LONG *Width,
  LONG *Height)
{
  PICONCURSOROBJ icon;
  
  if (!hIcon || !Width || !Width)
    return FALSE;

  icon = ICONCURSOROBJ_LockIconCursor(hIcon);
  
  if (!icon)
  {
	DPRINT1("NtUserGetIconInfo: ICONCURSOROBJ_LockIconCursor() returned 0\n");
	return FALSE;
  }
  
  if(fIcon) *fIcon = icon->fIcon;
  *Width = icon->ANDBitmap.bmWidth;
  *Width = icon->ANDBitmap.bmHeight;
  
  ICONCURSOROBJ_UnlockIconCursor(hIcon);
    
  return TRUE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserGetCursorInfo(
  PCURSORINFO pci)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserClipCursor(
  RECT *lpRect)
{
  /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;

  NTSTATUS Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastWin32Error(Status);
    return FALSE;
  }
  if(lpRect)
  {
    WinStaObject->SystemCursor.CursorClipInfo.IsClipped = TRUE;
    WinStaObject->SystemCursor.CursorClipInfo.Left = lpRect->left;
    WinStaObject->SystemCursor.CursorClipInfo.Top = lpRect->top;
    WinStaObject->SystemCursor.CursorClipInfo.Right = lpRect->right;
    WinStaObject->SystemCursor.CursorClipInfo.Bottom = lpRect->bottom;
    
    /* FIXME - update cursor position in case the cursor is not within the rectangle */
  }
  else
    WinStaObject->SystemCursor.CursorClipInfo.IsClipped = FALSE;
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserDestroyCursor(
  HCURSOR hCursor,
  DWORD Unknown)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetClipCursor(
  RECT *lpRect)
{
  /* FIXME - check if process has WINSTA_READATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;
  
  if(!lpRect)
    return FALSE;

  NTSTATUS Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastWin32Error(Status);
    return FALSE;
  }
  
  if(WinStaObject->SystemCursor.CursorClipInfo.IsClipped)
  {
    lpRect->left = WinStaObject->SystemCursor.CursorClipInfo.Left;
    lpRect->top = WinStaObject->SystemCursor.CursorClipInfo.Top;
    lpRect->right = WinStaObject->SystemCursor.CursorClipInfo.Right;
    lpRect->bottom = WinStaObject->SystemCursor.CursorClipInfo.Bottom;
  }
  else
  {
    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = NtUserGetSystemMetrics(SM_CXSCREEN);
    lpRect->bottom = NtUserGetSystemMetrics(SM_CYSCREEN);
  }
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @unimplemented
 */
HCURSOR
STDCALL
NtUserSetCursor(
  HCURSOR hCursor)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetCursorContents(
  HCURSOR hCursor,
  DWORD Unknown)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetCursorIconData(
  HICON hIcon,
  PBOOL fIcon,
  PDWORD xHotspot,
  PDWORD yHotspot)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
  UNIMPLEMENTED

  return 0;
}
