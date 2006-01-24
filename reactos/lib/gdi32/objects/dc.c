#include "precomp.h"

#define NDEBUG
#include <debug.h>


/*
 * @implemented
 */
HDC
STDCALL
CreateDCA (
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODEA	* lpInitData
	)
{
        ANSI_STRING DriverA, DeviceA, OutputA;
        UNICODE_STRING DriverU, DeviceU, OutputU;
	HDC	hDC;
	DEVMODEW *lpInitDataW;

	/*
	 * If needed, convert to Unicode
	 * any string parameter.
	 */

	if (NULL != lpszDriver)
	{
		RtlInitAnsiString(&DriverA, (LPSTR)lpszDriver);
		RtlAnsiStringToUnicodeString(&DriverU, &DriverA, TRUE);
	} else
		DriverU.Buffer = NULL;
	if (NULL != lpszDevice)
	{
		RtlInitAnsiString(&DeviceA, (LPSTR)lpszDevice);
		RtlAnsiStringToUnicodeString(&DeviceU, &DeviceA, TRUE);
	} else
		DeviceU.Buffer = NULL;
	if (NULL != lpszOutput)
	{
		RtlInitAnsiString(&OutputA, (LPSTR)lpszOutput);
		RtlAnsiStringToUnicodeString(&OutputU, &OutputA, TRUE);
	} else
		OutputU.Buffer = NULL;

	if (NULL != lpInitData)
	{
//		lpInitDataW = HeapAllocMem(
	} else
		lpInitDataW = NULL;

	/*
	 * Call the Unicode version
	 * of CreateDC.
	 */

	hDC = CreateDCW (
		DriverU.Buffer,
		DeviceU.Buffer,
		OutputU.Buffer,
		NULL);
//		lpInitDataW);
	/*
	 * Free Unicode parameters.
	 */
	RtlFreeUnicodeString(&DriverU);
	RtlFreeUnicodeString(&DeviceU);
	RtlFreeUnicodeString(&OutputU);

	/*
	 * Return the possible DC handle.
	 */

	return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCW (
	LPCWSTR		lpwszDriver,
	LPCWSTR		lpwszDevice,
	LPCWSTR		lpwszOutput,
	CONST DEVMODEW	* lpInitData
	)
{
	UNICODE_STRING Driver, Device, Output;

	if(lpwszDriver)
		RtlInitUnicodeString(&Driver, lpwszDriver);
	if(lpwszDevice)
		RtlInitUnicodeString(&Driver, lpwszDevice);
	if(lpwszOutput)
		RtlInitUnicodeString(&Driver, lpwszOutput);

	return NtGdiCreateDC((lpwszDriver ? &Driver : NULL),
						 (lpwszDevice ? &Device : NULL),
						 (lpwszOutput ? &Output : NULL),
						 (PDEVMODEW)lpInitData);
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICW(
	LPCWSTR			lpszDriver,
	LPCWSTR			lpszDevice,
	LPCWSTR			lpszOutput,
	CONST DEVMODEW *	lpdvmInit
	)
{
  UNICODE_STRING Driver, Device, Output;
  
  if(lpszDriver)
    RtlInitUnicodeString(&Driver, lpszDriver);
  if(lpszDevice)
    RtlInitUnicodeString(&Device, lpszDevice);
  if(lpszOutput)
    RtlInitUnicodeString(&Output, lpszOutput);
  return NtGdiCreateIC ((lpszDriver ? &Driver : NULL),
		      (lpszDevice ? &Device : NULL),
		      (lpszOutput ? &Output : NULL),
		      (CONST PDEVMODEW)lpdvmInit );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICA(
	LPCSTR			lpszDriver,
	LPCSTR			lpszDevice,
	LPCSTR			lpszOutput,
	CONST DEVMODEA *	lpdvmInit
	)
{
  NTSTATUS Status;
  LPWSTR lpszDriverW, lpszDeviceW, lpszOutputW;
  UNICODE_STRING Driver, Device, Output;
  LPDEVMODEW dvmInitW = NULL;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( &lpszDriverW, lpszDriver );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    Status = HEAP_strdupA2W ( &lpszDeviceW, lpszDevice );
    if (!NT_SUCCESS (Status))
      SetLastError (RtlNtStatusToDosError(Status));
    else
      {
	Status = HEAP_strdupA2W ( &lpszOutputW, lpszOutput );
	if (!NT_SUCCESS (Status))
	  SetLastError (RtlNtStatusToDosError(Status));
	else
	  {
	    if ( lpdvmInit )
          dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);
        
        RtlInitUnicodeString(&Driver, lpszDriverW);
        RtlInitUnicodeString(&Device, lpszDeviceW);
        RtlInitUnicodeString(&Output, lpszOutputW);
	    rc = NtGdiCreateIC ( &Driver,
				&Device,
				&Output,
				lpdvmInit ? dvmInitW : NULL );
        HEAP_free (dvmInitW);
	    HEAP_free ( lpszOutputW );
	  }
	HEAP_free ( lpszDeviceW );
      }
    HEAP_free ( lpszDriverW );
  }
  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteObject(HGDIOBJ hObject)
{
  /* From Wine: DeleteObject does not SetLastError() on a null object */
  if(!hObject) return FALSE;
  
  if (0 != ((DWORD) hObject & GDI_HANDLE_STOCK_MASK))
    {
      DPRINT1("Trying to delete system object 0x%x\n", hObject);
      return TRUE;
    }

  /* deleting a handle that doesn't belong to the caller should be rather rarely
     so for the sake of speed just try to delete it without checking validity */
  return NtGdiDeleteObject(hObject);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetRelAbs(
         HDC  hdc,
         DWORD dwIgnore
           )
{
  return NtGdiGetRelAbs(hdc);
}

/*
 * @implemented
*/
LONG
STDCALL
GetDCOrg(
    HDC hdc
    )
{
  // Officially obsolete by Microsoft
  POINT Pt;
  if (!NtGdiGetDCOrgEx(hdc, &Pt))
    return 0;
  return(MAKELONG(Pt.x, Pt.y));
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectA(HGDIOBJ Handle, int Size, LPVOID Buffer)
{
  LOGFONTW LogFontW;
  DWORD Type;
  int Result;

  Type = NtGdiGetObjectType(Handle);
  if (0 == Type)
    {
      /* From Wine: GetObject does not SetLastError() on a null object */
      SetLastError(0);
      return 0;
    }

  if (OBJ_FONT == Type)
    {
      if (Size < (int)sizeof(LOGFONTA))
        {
          SetLastError(ERROR_BUFFER_OVERFLOW);
          return 0;
        }
      Result = NtGdiGetObject(Handle, sizeof(LOGFONTW), &LogFontW);
      if (0 == Result)
        {
          return 0;
        }
      LogFontW2A((LPLOGFONTA) Buffer, &LogFontW);
      Result = sizeof(LOGFONTA);
    }
  else
    {
      Result = NtGdiGetObject(Handle, Size, Buffer);
    }

  return Result;
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectW(HGDIOBJ Handle, int Size, LPVOID Buffer)
{
  return NtGdiGetObject(Handle, Size, Buffer);
}


/*
 * @implemented
 */
COLORREF 
STDCALL
GetDCBrushColor(
	HDC hdc
)
{
  return NtUserGetDCBrushColor(hdc);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
GetDCPenColor(
	HDC hdc
)
{
  return NtUserGetDCPenColor(hdc);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
SetDCBrushColor(
	HDC hdc,
	COLORREF crColor
)
{
  return NtUserSetDCBrushColor(hdc, crColor);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
SetDCPenColor(
	HDC hdc,
	COLORREF crColor
)
{
  return NtUserSetDCPenColor(hdc, crColor);
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCW(
	HDC		hdc,
	CONST DEVMODEW	*lpInitData
	)
{
  NtGdiResetDC ( hdc, (PDEVMODEW)lpInitData, NULL, NULL, NULL);
  return hdc;
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCA(
	HDC		hdc,
	CONST DEVMODEA	*lpInitData
	)
{
  LPDEVMODEW InitDataW;

  InitDataW = GdiConvertToDevmodeW((LPDEVMODEA)lpInitData);

  NtGdiResetDC ( hdc, InitDataW, NULL, NULL, NULL);
  HEAP_free(InitDataW);
  return hdc;
}


/*
 * @implemented
 */
int 
STDCALL 
StartDocW(
	HDC		hdc,
	CONST DOCINFOW	*a1
	)
{
	return NtGdiStartDoc ( hdc, (DOCINFOW *)a1, NULL, 0);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetObjectType(
	HGDIOBJ h
	)
{
  DWORD Ret = 0;
  
  if(GdiIsHandleValid(h))
  {
    LONG Type = GDI_HANDLE_GET_TYPE(h);
    switch(Type)
    {
      case GDI_OBJECT_TYPE_PEN:
        Ret = OBJ_PEN;
        break;
      case GDI_OBJECT_TYPE_BRUSH:
        Ret = OBJ_BRUSH;
        break;
      case GDI_OBJECT_TYPE_BITMAP:
        Ret = OBJ_BITMAP;
        break;
      case GDI_OBJECT_TYPE_FONT:
        Ret = OBJ_FONT;
        break;
      case GDI_OBJECT_TYPE_PALETTE:
        Ret = OBJ_PAL;
        break;
      case GDI_OBJECT_TYPE_REGION:
        Ret = OBJ_REGION;
        break;
      case GDI_OBJECT_TYPE_DC:
        Ret = OBJ_DC;
        break;
      case GDI_OBJECT_TYPE_METADC:
        Ret = OBJ_METADC;
        break;
      case GDI_OBJECT_TYPE_METAFILE:
        Ret = OBJ_METAFILE;
        break;
      case GDI_OBJECT_TYPE_ENHMETAFILE:
        Ret = OBJ_ENHMETAFILE;
        break;
      case GDI_OBJECT_TYPE_ENHMETADC:
        Ret = OBJ_ENHMETADC;
        break;
      case GDI_OBJECT_TYPE_EXTPEN:
        Ret = OBJ_EXTPEN;
        break;
      case GDI_OBJECT_TYPE_MEMDC:
        Ret = OBJ_MEMDC;
        break;

      default:
        DPRINT1("GetObjectType: Magic 0x%08x not implemented\n", Type);
        break;
    }
  }
  else
    /* From Wine: GetObjectType does SetLastError() on a null object */
    SetLastError(ERROR_INVALID_HANDLE);
  return Ret;
}
