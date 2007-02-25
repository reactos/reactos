typedef struct _RAMDRV_DEVICE_EXTENSION {
  void *Buffer;
  unsigned long Size;
} RAMDRV_DEVICE_EXTENSION, *PRAMDRV_DEVICE_EXTENSION;

NTSTATUS STDCALL DriverEntry(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath);

