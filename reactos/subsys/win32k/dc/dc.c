/*
 * DC.C - Device context functions
 * 
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>

//  ---------------------------------------------------------  File Statics

/*  FIXME: these should probably be placed in an object in the future  */
HANDLE  gDriverHandle = NULL;
DRVENABLEDATA  gDED;
typedef struct _DC
{
  DHPDEV  PDev;  
  DEVMODEW  DMW;
  HSURF  FillPatternSurfaces[HS_DDI_MAX];
  GDIINFO  GDIInfo;
  DEVINFO  DevInfo;
  HSURF  Surface = NULL;
} DC, *PDC;

DC gSurfaceDC;

//  -----------------------------------------------------  Public Functions

BOOL  W32kCancelDC(HDC  DC)
{
  UNIMPLEMENTED;
}

NTSTATUS W32kCreateDC(HDC  *DC,
                      LPCWSTR  Driver,
                      LPCWSTR  Device,
                      CONST PDEVMODE InitData)
{
  NTSTATUS  Status;
  UNICODE_STRING  DeviceName, ErrorMsg;
  OBJECT_ATTRIBUTES  ObjectAttributes;

  /*  Is this a request for a printer DC?  */
  if (wcsicmp(Driver, L"DISPLAY"))
    {
      UNIMPLEMENTED;
    }
  
  /*  Initialize driver pair here on first call for display DC.  */
  if (gDriverHandle == NULL)
    {
      /* FIXME: should get the device name from the registry  */
      RtlInitUnicodeString(&DeviceName, L"\\Device\\DISPLAY");
      InitializeObjectAttributes(&ObjectAttributes,
                                 &DeviceName,
                                 0,
                                 NULL,
                                 NULL);
      Status = ZwOpenFile(&gDriverHandle, 
                          FILE_ALL_ACCESS, 
                          &ObjectAttributes, 
                          NULL, 
                          0, 
                          FILE_SYNCHRONOUS_IO_ALERT);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("Failed to open display device\n");
          DbgGetErrorText(Status, &ErrorMsg, 0xf);
          DbgPrint("%W\n", &ErrorMsg);
          RtlFreeUnicodeString(&ErrorMsg);
          
          return  Status;
        }

      /*  Call DrvEnableDriver  */
      RtlZeroMemory(&gDED, sizeof(gDED));
      if (!DrvEnableDriver(DDI_DRIVER_VERSION, sizeof(gDED), &gDED))
        {
          DbgPrint("DrvEnableDriver failed\n");
          
          return  STATUS_UNSUCCESSFUL;
        }
    }

  /*  Allocate a phyical device handle from the driver  */
  if (gSurfaceDC.PDev == NULL)
    {
      RtlZeroMemory(&gSurfaceDC, sizeof(gSurfaceDC));
      /* FIXME: get mode selection information from somewhere  */
      if (Device != NULL)
        {
          wcsncpy(gDMW.dmDeviceName, Device, DMMAXDEVICENAME);
        }
      gSurfaceDC.DMW.dmSize = sizeof(gSurfaceDC.DMW);
      gSurfaceDC.DMW.dmFields = 0x000fc000;
      gSurfaceDC.DMW.dmLogPixels = 96;
      gSurfaceDC.DMW.BitsPerPel = 8;
      gSurfaceDC.DMW.dmPelsWidth = 640;
      gSurfaceDC.DMW.dmPelsHeight = 480;
      gSurfaceDC.DMW.dmDisplayFlags = 0;
      gSurfaceDC.DMW.dmDisplayFrequency = 0;
      gSurfaceDC.PDev = DrvEnablePDEV(&gSurfaceDC.DMW, 
                            L"",
                            HS_DDI_MAX,
                            gSurfaceDC.FillPatternSurfaces,
                            sizeof(gSurfaceDC.GDIInfo),
                            &gSurfaceDC.GDIInfo,
                            sizeof(gSurfaceDC.DevInfo),
                            &gSurfaceDC.DevInfo,
                            NULL,
                            L"",
                            gDriverHandle);
      if (gSurfaceDC.PDev == NULL)
        {
          DbgPrint("DrvEnablePDEV failed\n");
          
          return  STATUS_UNSUCCESSFUL;
        }
      DrvCompletePDEV(gSurfaceDC.PDev, gSurfaceDC);
      gSurfaceDC.Surface = DrvEnableSurface(gSurfaceDC.PDev);
    }
  *DC = &SurfaceDC;
  
  return  STATUS_SUCCESS;
}

NTSTATUS  W32kDeleteDC(HDC  DC)
{
  UNIMPLEMENTED;
}

