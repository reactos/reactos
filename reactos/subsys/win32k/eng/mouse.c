#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include "..\..\services\input\include\mouse.h"
#include "objects.h"

LONG mouse_x, mouse_y;

VOID MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
  ULONG i;
  LONG mouse_cx = 0, mouse_cy = 0;
  HDC hDC = RetrieveDisplayHDC();
  PDC dc = DC_HandleToPtr(hDC);
  PSURFOBJ SurfObj = AccessUserObject(dc->Surface);
  PSURFGDI SurfGDI = AccessInternalObject(dc->Surface);
  RECTL MouseRect;

  PDEVICE_OBJECT ClassDeviceObject = NULL;
  PFILE_OBJECT FileObject = NULL;
  NTSTATUS status;
  UNICODE_STRING ClassName;
  IO_STATUS_BLOCK ioStatus;
  KEVENT event;
  PIRP irp;

  // Compile the total mouse movement change
  for (i=0; i<InputCount; i++)
  {
    mouse_cx += Data[i].LastX;
    mouse_cy += Data[i].LastY;
  }

  if((mouse_cx != 0) || (mouse_cy != 0))
  {
    mouse_x += mouse_cx;
    mouse_y += mouse_cy;

    if(mouse_x < 0) mouse_x = 0;
    if(mouse_y < 0) mouse_y = 0;
    if(mouse_x > 620) mouse_x = 620;
    if(mouse_y > 460) mouse_y = 460;

    SurfGDI->MovePointer(SurfObj, mouse_x, mouse_y, &MouseRect);
  }
}

NTSTATUS ConnectMouseClassDriver()
{
   PDEVICE_OBJECT ClassDeviceObject = NULL;
   PFILE_OBJECT FileObject = NULL;
   NTSTATUS status;
   UNICODE_STRING ClassName;
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;
   PIRP irp;
   GDI_INFORMATION GDIInformation;

   RtlInitUnicodeString(&ClassName, L"\\Device\\MouseClass");

   status = IoGetDeviceObjectPointer(&ClassName, FILE_READ_ATTRIBUTES, &FileObject, &ClassDeviceObject);

   if(status != STATUS_SUCCESS)
   {
      DbgPrint("Win32k: Could not connect to mouse class driver\n");
      return status;
   }

   // Connect our callback to the class driver

   KeInitializeEvent(&event, NotificationEvent, FALSE);

   GDIInformation.CallBack     = MouseGDICallBack;

   irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_MOUSE_CONNECT,
      ClassDeviceObject, &GDIInformation, sizeof(CLASS_INFORMATION), NULL, 0, TRUE, &event, &ioStatus);

   status = IoCallDriver(ClassDeviceObject, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
   } else {
      ioStatus.Status = status;
   }

   return ioStatus.Status;
}

void TestMouse()
{
  HDC hDC = RetrieveDisplayHDC(RetrieveDisplayHDC());
  PDC dc = DC_HandleToPtr(hDC);
  PSURFOBJ SurfObj = AccessUserObject(dc->Surface);
  PSURFGDI SurfGDI = AccessInternalObject(dc->Surface);
  BOOL txt;

  BRUSHOBJ Brush;
  HBITMAP hMouseSurf;
  PSURFOBJ MouseSurf;
  SIZEL MouseSize;
  POINTL ZeroPoint;
  RECTL MouseRect;

  // Draw a test mouse cursor
  Brush.iSolidColor = 1;
  EngLineTo(SurfObj, NULL, &Brush, 0, 0, 15, 0, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 0, 0, 0, 15, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 0, 15, 15, 0, NULL, 0);
  Brush.iSolidColor = 15;
  EngLineTo(SurfObj, NULL, &Brush, 1, 1, 13, 1, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 1, 1, 1, 13, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 1, 13, 13, 1, NULL, 0);

  // Create the bitmap for the mouse cursor data
  MouseSize.cx = 16;
  MouseSize.cy = 16;
  hMouseSurf = EngCreateBitmap(MouseSize, 16, BMF_4BPP, 0, NULL);
  MouseSurf = AccessUserObject(hMouseSurf);

  // Capture the cursor we drew in the mouse cursor buffer
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;
  MouseRect.top = 0;
  MouseRect.left = 0;
  MouseRect.bottom = 16;
  MouseRect.right = 16;
  EngBitBlt(MouseSurf, SurfObj, NULL, NULL, NULL, &MouseRect, &ZeroPoint, NULL, NULL, NULL, 0);
  SurfGDI->SetPointerShape(SurfObj, MouseSurf, NULL, NULL, 0, 0, 50, 50, &MouseRect, 0);

  // Connect the mouse class driver to the GDI
  mouse_x = 50;
  mouse_y = 50;
  ConnectMouseClassDriver();

  DbgPrint("OK\n");
}
