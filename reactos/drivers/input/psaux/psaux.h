typedef struct _DEVICE_EXTENSION {

  PDEVICE_OBJECT DeviceObject;

  ULONG ActiveQueue;
  ULONG InputDataCount[2];
  MOUSE_INPUT_DATA MouseInputData[2][MOUSE_BUFFER_SIZE];
  
  unsigned char MouseBuffer[8];
  unsigned char pkt[8];
  unsigned char MouseType;
  unsigned char MouseModel;
  unsigned char ack, acking;
  ULONG SmartScroll;
  ULONG NoExtensions;
  UINT MouseBufferPosition;
  UINT MouseBufferSize;
  UINT Resolution;
  UINT RepliesExpected;
  ULONG PreviousButtons;

  
  CLASS_INFORMATION ClassInformation;
  
  PKINTERRUPT MouseInterrupt;
  KDPC IsrDpc;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

