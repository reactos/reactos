#define MOUSE_BUFFER_SIZE 100

typedef struct _DEVICE_EXTENSION {
   PIO_WORKITEM WorkItem;
   KSPIN_LOCK SpinLock;
   BOOLEAN PassiveCallbackQueued;
   BOOLEAN ReadIsPending;
   ULONG InputCount;
   PMOUSE_INPUT_DATA PortData;
   PDEVICE_OBJECT PortDeviceObject; // FIXME: Expand this to handle multiple port drivers (make *PortDeviceObject)
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
