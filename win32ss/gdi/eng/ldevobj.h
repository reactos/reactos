
extern IMAGE_DOS_HEADER __ImageBase;

static const unsigned GDI_ENGINE_VERSION = DDI_DRIVER_VERSION_NT5_01;

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
    LIST_ENTRY leLink;
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

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitLDEVImpl(VOID);

PDEVMODEINFO
NTAPI
LDEVOBJ_pdmiGetModes(
    _In_ PLDEVOBJ pldev,
    _In_ HANDLE hDriver);

PLDEVOBJ
APIENTRY
LDEVOBJ_pLoadDriver(
    _In_z_ LPWSTR pwszDriverName,
    _In_ ULONG ldevtype);

PLDEVOBJ
NTAPI
EngGetLDEV(
    PDEVMODEW pdm);

CODE_SEG("INIT")
NTSTATUS
APIENTRY
DriverEntry (
    _In_ PDRIVER_OBJECT	DriverObject,
    _In_ PUNICODE_STRING RegistryPath);

