typedef struct _DEVICE_EXTENSION {

   PDEVICE_OBJECT DeviceObject;
   ULONG InputDataCount;
   PMOUSE_INPUT_DATA MouseInputData;
   CLASS_INFORMATION ClassInformation;

   PKINTERRUPT MouseInterrupt;
   KDPC IsrDpc;
   KDPC IsrDpcRetry;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
