
typedef struct _ENGSECTION
{
    PVOID pvSectionObject;
    PVOID pvMappedBase;
    SIZE_T cjViewSize;
    ULONG ulTag;
} ENGSECTION, *PENGSECTION;

typedef struct _FILEVIEW
{
    LARGE_INTEGER  LastWriteTime;
    PVOID          pvKView;
    PVOID          pvViewFD;
    SIZE_T         cjView;
    PVOID          pSection;
} FILEVIEW, *PFILEVIEW;

typedef struct _FONTFILEVIEW
{
    FILEVIEW;
    DWORD          reserved[2];
    PWSTR          pwszPath;
    SIZE_T         ulRegionSize;
    ULONG          cKRefCount;
    ULONG          cRefCountFD;
    PVOID          pvSpoolerBase;
    DWORD          dwSpoolerPid;
} FONTFILEVIEW, *PFONTFILEVIEW;

enum
{
    FVF_SYSTEMROOT = 1,
    FVF_READONLY = 2,
    FVF_FONTFILE = 4,
};

PVOID
NTAPI
EngMapSectionView(
    _In_ HANDLE hSection,
    _In_ SIZE_T cjSize,
    _In_ ULONG cjOffset,
    _Out_ PHANDLE phSecure);

VOID
NTAPI
EngUnmapSectionView(
    _In_ PVOID pvBits,
    _In_ ULONG cjOffset,
    _In_ HANDLE hSecure);

PVOID
NTAPI
EngCreateSection(
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag);

BOOL
APIENTRY
EngMapSection(
    IN PVOID pvSection,
    IN BOOL bMap,
    IN HANDLE hProcess,
    OUT PVOID* pvBaseAddress);

PVOID
APIENTRY
EngAllocSectionMem(
    OUT PVOID *ppvSection,
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag);

BOOL
APIENTRY
EngFreeSectionMem(
    IN PVOID pvSection OPTIONAL,
    IN PVOID pvMappedBase OPTIONAL);

PFILEVIEW
NTAPI
EngLoadModuleEx(
    LPWSTR pwsz,
    ULONG cjSizeOfModule,
    FLONG fl);

