
// HACK!!!
#define MmMapViewInSessionSpace MmMapViewInSystemSpace
#define MmUnmapViewInSessionSpace MmUnmapViewInSystemSpace

extern PEPROCESS gpepCSRSS;

typedef struct _ENGSECTION
{
    PVOID pvSectionObject;
    PVOID pvMappedBase;
    SIZE_T cjViewSize;
    ULONG ulTag;
} ENGSECTION, *PENGSECTION;

typedef struct _FILEVIEW
{
    LIST_ENTRY     leLink;
    PWSTR          pwszPath;
    ULONG          cRefs;
    LARGE_INTEGER  LastWriteTime;
    PVOID          pvKView;
    PVOID          pvViewFD;
    SIZE_T         cjView;
    PVOID          pSection;
} FILEVIEW, *PFILEVIEW;

typedef struct _FONTFILEVIEW
{
    FILEVIEW;
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

BOOL
APIENTRY
EngFreeSectionMem(
    IN PVOID pvSection OPTIONAL,
    IN PVOID pvMappedBase OPTIONAL);

PVOID
APIENTRY
EngAllocSectionMem(
    OUT PVOID *ppvSection,
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag);

PFILEVIEW
NTAPI
EngLoadModuleEx(
    LPWSTR pwsz,
    ULONG cjSizeOfModule,
    FLONG fl);
