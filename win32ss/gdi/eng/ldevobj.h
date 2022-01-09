
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

/* Get all available device modes from a driver
 * - pwszDriverName: name of the driver
 * - hDriver: handle of the driver
 * - ppdm: allocated memory containing driver modes or NULL on error
 * Return value: number of bytes allocated for *ppdm buffer or 0 on error
 */
ULONG
LDEVOBJ_ulGetDriverModes(
    _In_ LPWSTR pwszDriverName,
    _In_ HANDLE hDriver,
    _Out_ PDEVMODEW *ppdm);

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

