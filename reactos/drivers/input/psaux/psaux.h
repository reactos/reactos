#include <ddk/ntddk.h>
#include <ddk/iotypes.h>
typedef struct _DEVICE_EXTENSION {

  PDEVICE_OBJECT DeviceObject;

  ULONG ActiveQueue;
  ULONG InputDataCount[2];
  MOUSE_INPUT_DATA MouseInputData[2][MOUSE_BUFFER_SIZE];
  BOOL HasMouse;
  
  unsigned char MouseType;
  unsigned char model;
  
  unsigned char MouseBuffer[8];
  UINT MouseBufferPosition;
  UINT MouseBufferSize;
  UINT Resolution;
  
  unsigned char cmdbuf[8];
  unsigned char cmdcnt;
  unsigned char pktcnt;
  char acking;
  volatile char ack;
  
  int psmouse_noext;
  int psmouse_smartscroll;
  
  ULONG PreviousButtons;
  
  CLASS_INFORMATION ClassInformation;
  
  PKINTERRUPT MouseInterrupt;
  KDPC IsrDpc;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
