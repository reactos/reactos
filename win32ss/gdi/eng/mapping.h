
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
    _In_ ULONG fl,
    _In_ SIZE_T cjSize,
    _In_ ULONG ulTag);

_Success_(return!=FALSE)
BOOL
APIENTRY
EngMapSection(
    _In_ PVOID pvSection,
    _In_ BOOL bMap,
    _In_ HANDLE hProcess,
    _When_(bMap, _Outptr_) PVOID* pvBaseAddress);

_Check_return_
_Success_(return!=NULL)
__drv_allocatesMem(Mem)
_Post_writable_byte_size_(cjSize)
PVOID
APIENTRY
EngAllocSectionMem(
    _Outptr_ PVOID *ppvSection,
    _In_ ULONG fl,
    _In_ SIZE_T cjSize,
    _In_ ULONG ulTag);

BOOL
APIENTRY
EngFreeSectionMem(
    _In_opt_ PVOID pvSection,
    _In_opt_ PVOID pvMappedBase);

_Check_return_
PFILEVIEW
NTAPI
EngLoadModuleEx(
    _In_z_ LPWSTR pwsz,
    _In_ ULONG cjSizeOfModule,
    _In_ FLONG fl);

