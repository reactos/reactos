typedef struct _DEVICE_EXTENSION {

  PDEVICE_OBJECT DeviceObject;

  ULONG ActiveQueue;
  ULONG InputDataCount[2];
  MOUSE_INPUT_DATA MouseInputData[2][MOUSE_BUFFER_SIZE];
  
  CLASS_INFORMATION ClassInformation;
  
  PKINTERRUPT MouseInterrupt;
  KDPC IsrDpc;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

