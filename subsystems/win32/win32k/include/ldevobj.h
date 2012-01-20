
#ifdef __GNUC__
/* HACK, for bug in ld.  Will be removed soon.  */
#define __ImageBase _image_base__
#endif
extern IMAGE_DOS_HEADER __ImageBase;

#define GDI_ENGINE_VERSION DDI_DRIVER_VERSION_NT5_01

typedef enum
{
    LDEV_DEVICE_DISPLAY = 1,
    LDEV_DEVICE_PRINTER = 2,
    LDEV_DEVICE_META = 3,
    LDEV_DEVICE_MIRROR = 4,
    LDEV_IMAGE = 5,
    LDEV_FONT = 6,
} LDEVTYPE;

typedef struct _LDEVOBJ
{
    struct _LDEVOBJ *pldevNext;
    struct _LDEVOBJ *pldevPrev;
    SYSTEM_GDI_DRIVER_INFORMATION *pGdiDriverInfo;
    LDEVTYPE ldevtype;
    ULONG cRefs;
    ULONG ulDriverVersion;

    union
    {
        PVOID apfn[INDEX_LAST];
        DRIVER_FUNCTIONS pfn;
    };

} LDEVOBJ, *PLDEVOBJ;

extern PLDEVOBJ gpldevHead;
extern HSEMAPHORE ghsemDriverMgmt;

PLDEVOBJ
NTAPI
LDEVOBJ_pldevLoadImage(
    PUNICODE_STRING pusPathName,
    LDEVTYPE ldevtype);

BOOL
NTAPI
LDEVOBJ_bLoadDriver(
    IN PLDEVOBJ pldev);

PVOID
NTAPI
LDEVOBJ_pvFindImageProcAddress(
    IN PLDEVOBJ pldev,
    IN LPSTR    lpProcName);

PDEVMODEINFO
NTAPI
LDEVOBJ_pdmiGetModes(
    PLDEVOBJ pldev,
    HANDLE hDriver);

INIT_FUNCTION
NTSTATUS
NTAPI
InitLDEVImpl(VOID);

PLDEVOBJ
APIENTRY
EngLoadImageEx(
    LPWSTR pwszDriverName,
    ULONG ldevtype);

PLDEVOBJ
NTAPI
EngGetLDEV(
    PDEVMODEW pdm);

NTSTATUS
APIENTRY
DriverEntry (
  IN	PDRIVER_OBJECT	DriverObject,
  IN	PUNICODE_STRING	RegistryPath);

