
typedef struct _VIDEOPORT_EXTENSTION_DATA
{
  PDEVICE_OBJECT  DeviceObject;
  KINTERRUPT  InterruptObject;
  KSPIN_LOCK  InterruptSpinLock;
  ULONG  InterruptLevel;
  KIRQL  IRQL;
  KAFFINITY  Affinity;
} VIDEOPORT_EXTENSION_DATA, *PVIDEOPORT_EXTENSION_DATA;

#define MPExtensionToVPExtension(MPX) \
  ((PVIDEOPORT_EXTENSION_DATA) ((DWORD) (MPX) - sizeof(VIDEOPORT_EXTENSION_DATA)))
#define VPExtensionToMPExtension(VPX) \
  ((PVOID) ((DWORD) (VPX) + sizeof(VIDEOPORT_EXTENSION_DATA)))

