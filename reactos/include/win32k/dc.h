

typedef struct _WIN_DC_INFO
{
  
} WIN_DC_INFO;
typedef struct _DC
{
  DHPDEV  PDev;  
  DEVMODEW  DMW;
  HSURF  FillPatternSurfaces[HS_DDI_MAX];
  GDIINFO  GDIInfo;
  DEVINFO  DevInfo;
  HSURF  Surface = NULL;

  DRIVER_FUNCTIONS  DriverFunctions;
  HANDLE  DeviceDriver;
  
  WIN_DC_INFO  WinDCInfo;
} DC, *PDC;

