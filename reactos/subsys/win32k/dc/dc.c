/*
 * DC.C - Device context functions
 * 
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/driver.h>

//  ---------------------------------------------------------  File Statics

/*  FIXME: these should probably be placed in an object in the future  */
DC  gSurfaceDC;

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
  PGD_ENABLEDRIVER  GDEnableDriver;
  UNICODE_STRING  DeviceName, ErrorMsg;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  
  /*  Get the DDI driver's entry point  */
  if ((GDEnableDriver = DRIVER_FindDriver(Driver)) == NULL)
    {
      return  STATUS_UNSUCCESSFUL;
    }
  
/* FIXME: left off here...  */
/* FIXME: allocate a DC object  */
/* FIXME: recode the rest of the function to use the allocated DC */
  
  /* FIXME: should get the device name from the DRIVER funcs  */
  RtlInitUnicodeString(&DeviceName, L"\\Device\\DISPLAY");
  InitializeObjectAttributes(&ObjectAttributes,
                             &DeviceName,
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&NewDC->DeviceDriver,
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
  /* FIXME: we should only call this once, so the data should be stored in
   * DRIVER funcs  */
  RtlZeroMemory(&gDED, sizeof(gDED));
  if (!GDEnableDriver(DDI_DRIVER_VERSION, sizeof(gDED), &gDED))
    {
      DbgPrint("DrvEnableDriver failed\n");
      
      return  STATUS_UNSUCCESSFUL;
    }

  /*  Allocate a phyical device handle from the driver  */
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
  /* FIXME: get the function pointer from the DED struct  */
  gSurfaceDC.PDev = GDEnablePDEV(&NewDC->DMW, 
                                 L"",
                                 HS_DDI_MAX,
                                 NewDC->FillPatternSurfaces,
                                 sizeof(NewDC->GDIInfo),
                                 &NewDC->GDIInfo,
                                 sizeof(NewDC->DevInfo),
                                 &NewDC->DevInfo,
                                 NULL,
                                 L"",
                                 NewDC->DeviceDriver);
  if (NewDC->PDev == NULL)
    {
      DbgPrint("DrvEnablePDEV failed\n");
          
      return  STATUS_UNSUCCESSFUL;
    }
  /* FIXME: get func pointer from DED  */
  GDCompletePDEV(NewDC->PDev, NewDC);
  
  /* FIXME: get func pointer from DED  */
  NewDC->Surface = GDEnableSurface(NewDC->PDev);
  *DC = NewDC;
  
  return  STATUS_SUCCESS;
}

NTSTATUS  W32kDeleteDC(HDC  DC)
{
  UNIMPLEMENTED;
}

