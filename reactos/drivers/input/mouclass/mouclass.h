typedef struct _DEVICE_EXTENSION {
   BOOLEAN ReadIsPending;
   ULONG InputCount;
   PMOUSE_INPUT_DATA PortData;
   PDEVICE_OBJECT PortDeviceObject; // FIXME: Expand this to handle multiple port drivers (make *PortDeviceObject)
   GDI_INFORMATION GDIInformation;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
