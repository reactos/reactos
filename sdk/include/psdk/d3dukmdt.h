/******************************Module*Header************************************\
*
* Module Name: d3dukmdt.h
*
* Content: Windows Display Driver Model (WDDM) user/kernel mode
*          shared data type definitions.
*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*
\*******************************************************************************/
#ifndef _D3DUKMDT_H_
#define _D3DUKMDT_H_
#include <winapifamily.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if !defined(_D3DKMDT_H)       && \
    !defined(_D3DKMTHK_H_)     && \
    !defined(_D3DUMDDI_H_)     && \
    !defined(__DXGKRNLETW_H__)
   #error This header should not be included directly!
#endif

#pragma warning(push)
#pragma warning(disable:4201) // anonymous unions warning
#pragma warning(disable:4214)   // nonstandard extension used: bit field types other than int


//
// WDDM DDI Interface Version
//

#define DXGKDDI_INTERFACE_VERSION_VISTA      0x1052
#define DXGKDDI_INTERFACE_VERSION_VISTA_SP1  0x1053
#define DXGKDDI_INTERFACE_VERSION_WIN7       0x2005
#define DXGKDDI_INTERFACE_VERSION_WIN8       0x300E
#define DXGKDDI_INTERFACE_VERSION_WDDM1_3    0x4002
#define DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION  0x4003
#define DXGKDDI_INTERFACE_VERSION_WDDM2_0    0x5023
#define DXGKDDI_INTERFACE_VERSION_WDDM2_1    0x6003
#define DXGKDDI_INTERFACE_VERSION_WDDM2_1_5  0x6010     // Used in RS1.7 for GPU-P
#define DXGKDDI_INTERFACE_VERSION_WDDM2_1_6  0x6011     // Used in RS1.8 for GPU-P
#define DXGKDDI_INTERFACE_VERSION_WDDM2_2    0x700A
#define DXGKDDI_INTERFACE_VERSION_WDDM2_3    0x8001
#define DXGKDDI_INTERFACE_VERSION_WDDM2_4    0x9006
#define DXGKDDI_INTERFACE_VERSION_WDDM2_5    0xA00B
#define DXGKDDI_INTERFACE_VERSION_WDDM2_6    0xB004
#define DXGKDDI_INTERFACE_VERSION_WDDM2_7    0xC004
#define DXGKDDI_INTERFACE_VERSION_WDDM2_8    0xD001
#define DXGKDDI_INTERFACE_VERSION_WDDM2_9    0xE003
#define DXGKDDI_INTERFACE_VERSION_WDDM3_0    0xF003


#define IS_OFFICIAL_DDI_INTERFACE_VERSION(version)                 \
            (((version) == DXGKDDI_INTERFACE_VERSION_VISTA) ||     \
             ((version) == DXGKDDI_INTERFACE_VERSION_VISTA_SP1) || \
             ((version) == DXGKDDI_INTERFACE_VERSION_WIN7) ||      \
             ((version) == DXGKDDI_INTERFACE_VERSION_WIN8) ||      \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM1_3) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION) || \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_0) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_1) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_1_5) || \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_1_6) || \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_2) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_3) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_4) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_5) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_6) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_7) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_8) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM2_9) ||   \
             ((version) == DXGKDDI_INTERFACE_VERSION_WDDM3_0)      \
            )

#if !defined(DXGKDDI_INTERFACE_VERSION)
#define DXGKDDI_INTERFACE_VERSION           DXGKDDI_INTERFACE_VERSION_WDDM3_0
#endif // !defined(DXGKDDI_INTERFACE_VERSION)

#define D3D_UMD_INTERFACE_VERSION_VISTA      0x000C
#define D3D_UMD_INTERFACE_VERSION_WIN7       0x2003
#define D3D_UMD_INTERFACE_VERSION_WIN8_M3    0x3001
#define D3D_UMD_INTERFACE_VERSION_WIN8_CP    0x3002
#define D3D_UMD_INTERFACE_VERSION_WIN8_RC    0x3003
#define D3D_UMD_INTERFACE_VERSION_WIN8       0x3004
#define D3D_UMD_INTERFACE_VERSION_WDDM1_3    0x4002

#define D3D_UMD_INTERFACE_VERSION_WDDM2_0_M1    0x5000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_0_M1_3  0x5001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_0_M2_2  0x5002
#define D3D_UMD_INTERFACE_VERSION_WDDM2_0       0x5002

#define D3D_UMD_INTERFACE_VERSION_WDDM2_1_1     0x6000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_1_2     0x6001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_1_3     0x6002
#define D3D_UMD_INTERFACE_VERSION_WDDM2_1_4     0x6003
#define D3D_UMD_INTERFACE_VERSION_WDDM2_1       D3D_UMD_INTERFACE_VERSION_WDDM2_1_4

#define D3D_UMD_INTERFACE_VERSION_WDDM2_2_1     0x7000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_2_2     0x7001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_2       D3D_UMD_INTERFACE_VERSION_WDDM2_2_2

#define D3D_UMD_INTERFACE_VERSION_WDDM2_3_1     0x8000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_3_2     0x8001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_3       D3D_UMD_INTERFACE_VERSION_WDDM2_3_2

#define D3D_UMD_INTERFACE_VERSION_WDDM2_4_1     0x9000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_4_2     0x9001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_4       D3D_UMD_INTERFACE_VERSION_WDDM2_4_2

#define D3D_UMD_INTERFACE_VERSION_WDDM2_5_1     0xA000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_5_2     0xA001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_5_3     0xA002
#define D3D_UMD_INTERFACE_VERSION_WDDM2_5       D3D_UMD_INTERFACE_VERSION_WDDM2_5_3

#define D3D_UMD_INTERFACE_VERSION_WDDM2_6_1     0xB000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_6_2     0xB001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_6_3     0xB002
#define D3D_UMD_INTERFACE_VERSION_WDDM2_6_4     0xB003
#define D3D_UMD_INTERFACE_VERSION_WDDM2_6       D3D_UMD_INTERFACE_VERSION_WDDM2_6_4

#define D3D_UMD_INTERFACE_VERSION_WDDM2_7_1     0xC000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_7_2     0xC001
#define D3D_UMD_INTERFACE_VERSION_WDDM2_7       D3D_UMD_INTERFACE_VERSION_WDDM2_7_2

#define D3D_UMD_INTERFACE_VERSION_WDDM2_8_1     0xD000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_8       D3D_UMD_INTERFACE_VERSION_WDDM2_8_1

#define D3D_UMD_INTERFACE_VERSION_WDDM2_9_1     0xE000
#define D3D_UMD_INTERFACE_VERSION_WDDM2_9       D3D_UMD_INTERFACE_VERSION_WDDM2_9_1

#define D3D_UMD_INTERFACE_VERSION_WDDM3_0_1     0xF000
#define D3D_UMD_INTERFACE_VERSION_WDDM3_0       D3D_UMD_INTERFACE_VERSION_WDDM3_0_1

// Components which depend on D3D_UMD_INTERFACE_VERSION need to be updated, static assert validation present.
// Search for D3D_UMD_INTERFACE_VERSION across all depots to ensure all dependencies are updated.

#if !defined(D3D_UMD_INTERFACE_VERSION)
#define D3D_UMD_INTERFACE_VERSION           D3D_UMD_INTERFACE_VERSION_WDDM3_0
#endif // !defined(D3D_UMD_INTERFACE_VERSION)

//
// These macros are used to enable non-Windows 32-bit usermode to fill out a fixed-size
// structure for D3DKMT structures, so that they can be sent to a 64-bit kernel
// without a thunking layer for translation.
//
// Note that a thunking layer is still used for wchar_t translation, where Windows uses
// 16-bit characters and non-Windows uses 32-bit characters.
//
// If brace initialization is used (e.g. D3DKMT_FOO foo = { a, b, c }), be aware that for
// non-Windows, pointers will be unioned such that a 64-bit integer is the member that is
// initialized. It is not possible to achieve both type safety and proper zero-initialization
// of the high bits of pointers with brace initialization in this model. Use D3DKMT_PTR_INIT(ptr)
// to appropriately cast to a 64-bit integer for non-Windows, or to pass the pointer unchanged for
// Windows. To maintain type safety, manually assign the fields after zero-initializing the struct.
//
#ifdef _WIN32
// For Windows, don't enforce any unnatural alignment or data sizes/types.
// The WOW64 thunking layer will handle translation.
#define D3DKMT_ALIGN64
#define D3DKMT_PTR_HELPER(Name)
#define D3DKMT_PTR(Type, Name) Type Name
#define D3DKMT_PTR_INIT(x) (x)
typedef SIZE_T D3DKMT_SIZE_T;
typedef UINT_PTR D3DKMT_UINT_PTR;
typedef ULONG_PTR D3DKMT_ULONG_PTR;
typedef HANDLE D3DKMT_PTR_TYPE;
#else
// For other platforms, struct layout should be fixed-size, x64-compatible
#if __cplusplus >= 201103L
 #define D3DKMT_ALIGN64 alignas(8)
#else
 #define D3DKMT_ALIGN64 __attribute__((aligned(8)))
#endif
#define D3DKMT_PTR_HELPER(Name) D3DKMT_ALIGN64 UINT64 Name;
#define D3DKMT_PTR(Type, Name)       \
union D3DKMT_ALIGN64                 \
{                                    \
    D3DKMT_PTR_HELPER(Name##_Align)  \
    Type Name;                       \
}
#define D3DKMT_PTR_INIT(x) { (UINT64)(SIZE_T)(x) }
typedef UINT64 D3DKMT_SIZE_T, D3DKMT_UINT_PTR, D3DKMT_ULONG_PTR;
typedef union _D3DKMT_PTR_TYPE
{
    D3DKMT_PTR_HELPER(Ptr_Align);
    HANDLE Ptr;
} D3DKMT_PTR_TYPE;
#endif

//
// Available only for Vista (LONGHORN) and later and for
// multiplatform tools such as debugger extensions
//
#if (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)

typedef ULONGLONG D3DGPU_VIRTUAL_ADDRESS;
typedef ULONGLONG D3DGPU_SIZE_T;
#define D3DGPU_UNIQUE_DRIVER_PROTECTION 0x8000000000000000ULL

#define DXGK_MAX_PAGE_TABLE_LEVEL_COUNT 6
#define DXGK_MIN_PAGE_TABLE_LEVEL_COUNT 2

//
// IOCTL_GPUP_DRIVER_ESCAPE - The user mode emulation DLL calls this IOCTL
// to exchange information with the kernel mode driver.
//
#define IOCTL_GPUP_DRIVER_ESCAPE CTL_CODE(FILE_DEVICE_UNKNOWN, (8 + 0x910), METHOD_BUFFERED, FILE_READ_DATA)
typedef struct _GPUP_DRIVER_ESCAPE_INPUT
{
   LUID        vfLUID; //LUID that was returned from SET_PARTITION_DETAILS
} GPUP_DRIVER_ESCAPE_INPUT, *PGPUP_DRIVER_ESCAPE_INPUT;

typedef enum _DXGKVGPU_ESCAPE_TYPE
{
    DXGKVGPU_ESCAPE_TYPE_READ_PCI_CONFIG            = 0,
    DXGKVGPU_ESCAPE_TYPE_WRITE_PCI_CONFIG           = 1,
    DXGKVGPU_ESCAPE_TYPE_INITIALIZE                 = 2,
    DXGKVGPU_ESCAPE_TYPE_RELEASE                    = 3,
    DXGKVGPU_ESCAPE_TYPE_GET_VGPU_TYPE              = 4,
    DXGKVGPU_ESCAPE_TYPE_POWERTRANSITIONCOMPLETE    = 5,
} DXGKVGPU_ESCAPE_TYPE;

typedef struct _DXGKVGPU_ESCAPE_HEAD
{
    GPUP_DRIVER_ESCAPE_INPUT    Luid;
    DXGKVGPU_ESCAPE_TYPE        Type;
} DXGKVGPU_ESCAPE_HEAD;

typedef struct _DXGKVGPU_ESCAPE_READ_PCI_CONFIG
{
    DXGKVGPU_ESCAPE_HEAD Header;

    UINT  Offset;           // Offset in bytes in the PCI config space
    UINT  Size;             // Size in bytes to read
} DXGKVGPU_ESCAPE_READ_PCI_CONFIG;

typedef struct _DXGKVGPU_ESCAPE_WRITE_PCI_CONFIG
{
    DXGKVGPU_ESCAPE_HEAD Header;

    UINT  Offset;           // Offset in bytes in the PCI config space
    UINT  Size;             // Size in bytes to write
    // "Size" number of bytes follow
} DXGKVGPU_ESCAPE_WRITE_PCI_CONFIG;

typedef struct _DXGKVGPU_ESCAPE_READ_VGPU_TYPE
{
    DXGKVGPU_ESCAPE_HEAD Header;
} DXGKVGPU_ESCAPE_READ_VGPU_TYPE;

typedef struct _DXGKVGPU_ESCAPE_POWERTRANSITIONCOMPLETE
{
    DXGKVGPU_ESCAPE_HEAD Header;
    UINT PowerState;
} DXGKVGPU_ESCAPE_POWERTRANSITIONCOMPLETE;

typedef struct _DXGKVGPU_ESCAPE_INITIALIZE
{
    DXGKVGPU_ESCAPE_HEAD    Header;
    GUID                    VmGuid;
} DXGKVGPU_ESCAPE_INITIALIZE;

typedef struct _DXGKVGPU_ESCAPE_RELEASE
{
    DXGKVGPU_ESCAPE_HEAD Header;
} DXGKVGPU_ESCAPE_RELEASE;


typedef enum _DXGK_PTE_PAGE_SIZE
{
    DXGK_PTE_PAGE_TABLE_PAGE_4KB       = 0,
    DXGK_PTE_PAGE_TABLE_PAGE_64KB      = 1,
} DXGK_PTE_PAGE_SIZE;

//
//  Page Table Entry structure. Contains segment/physical address pointing to a page
//

typedef struct _DXGK_PTE
{
    union
    {
        struct
        {
            ULONGLONG Valid                 :  1;
            ULONGLONG Zero                  :  1;
            ULONGLONG CacheCoherent         :  1;
            ULONGLONG ReadOnly              :  1;
            ULONGLONG NoExecute             :  1;
            ULONGLONG Segment               :  5;
            ULONGLONG LargePage             :  1;
            ULONGLONG PhysicalAdapterIndex  :  6;
            ULONGLONG PageTablePageSize     :  2;       // DXGK_PTE_PAGE_SIZE
            ULONGLONG SystemReserved0       :  1;
            ULONGLONG Reserved              :  44;
        };
        ULONGLONG Flags;
    };
    union
    {
        ULONGLONG PageAddress;      // High 52 bits of 64 bit physical address. Low 12 bits are zero.
        ULONGLONG PageTableAddress; // High 52 bits of 64 bit physical address. Low 12 bits are zero.
    };
} DXGK_PTE;

#define D3DGPU_NULL 0
#define D3DDDI_MAX_WRITTEN_PRIMARIES 16
#define D3DDDI_MAX_MPO_PRESENT_DIRTY_RECTS  0xFFF

typedef struct _D3DGPU_PHYSICAL_ADDRESS
{
    UINT    SegmentId;
    UINT    Padding;
    UINT64  SegmentOffset;
} D3DGPU_PHYSICAL_ADDRESS;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source unique identification number descriptor type
//

typedef UINT  D3DDDI_VIDEO_PRESENT_SOURCE_ID;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source unique identification number descriptor type.
//
typedef UINT  D3DDDI_VIDEO_PRESENT_TARGET_ID;

//
// DDI level handle that represents a kernel mode object (allocation, device, etc)
//
typedef UINT D3DKMT_HANDLE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode fractional frequency descriptor type.
//
// Remarks: Fractional value used to represent vertical and horizontal frequencies of a video mode
//          (i.e. VSync and HSync). Vertical frequencies are stored in Hz. Horizontal frequencies
//          are stored in Hz.
//          The dynamic range of this encoding format, given 10^-7 resolution is {0..(2^32 - 1) / 10^7},
//          which translates to {0..428.4967296} [Hz] for vertical frequencies and {0..428.4967296} [Hz]
//          for horizontal frequencies. This sub-microseconds precision range should be acceptable even
//          for a pro-video application (error in one microsecond for video signal synchronization would
//          imply a time drift with a cycle of 10^7/(60*60*24) = 115.741 days.
//
//          If rational number with a finite fractional sequence, use denominator of form 10^(length of fractional sequence).
//          If rational number without a finite fractional sequence, or a sequence exceeding the precision allowed by the
//          dynamic range of the denominator, or an irrational number, use an appropriate ratio of integers which best
//          represents the value.
//
typedef struct _D3DDDI_RATIONAL
{
    UINT    Numerator;
    UINT    Denominator;
} D3DDDI_RATIONAL;

typedef struct _D3DDDI_ALLOCATIONINFO
{
    D3DKMT_HANDLE                   hAllocation;           // out: Private driver data for allocation
    D3DKMT_PTR(CONST VOID*,         pSystemMem);           // in: Pointer to pre-allocated sysmem
    D3DKMT_PTR(VOID*,               pPrivateDriverData);   // in(out optional): Private data for each allocation
    UINT                            PrivateDriverDataSize; // in: Size of the private data
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;         // in: VidPN source ID if this is a primary
    union
    {
        struct
        {
            UINT    Primary         : 1;    // 0x00000001
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8))
            UINT    Stereo          : 1;    // 0x00000002
            UINT    Reserved        :30;    // 0xFFFFFFFC
#else
            UINT    Reserved        :31;    // 0xFFFFFFFE
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        };
        UINT        Value;
    } Flags;
} D3DDDI_ALLOCATIONINFO;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7))

typedef struct _D3DDDI_ALLOCATIONINFO2
{
    D3DKMT_HANDLE                   hAllocation;           // out: Private driver data for allocation
    union D3DKMT_ALIGN64
    {
        D3DKMT_PTR_HELPER(pSystemMem_hSection_Align)
        HANDLE                      hSection;              // in: Handle to valid section object
        CONST VOID*                 pSystemMem;            // in: Pointer to pre-allocated sysmem
    };
    D3DKMT_PTR(VOID*,               pPrivateDriverData);   // in(out optional): Private data for each allocation
    UINT                            PrivateDriverDataSize; // in: Size of the private data
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;         // in: VidPN source ID if this is a primary
    union
    {
        struct
        {
            UINT    Primary          : 1;    // 0x00000001
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8))
            UINT    Stereo           : 1;    // 0x00000002
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2))
            UINT    OverridePriority : 1;    // 0x00000004
            UINT    Reserved         : 29;   // 0xFFFFFFF8
#else
            UINT    Reserved         : 30;    // 0xFFFFFFFC
#endif
#else
            UINT    Reserved          :31;    // 0xFFFFFFFE
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        };
        UINT        Value;
    } Flags;
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;    // out: GPU Virtual address of the allocation created.
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2))
    union
    {
        UINT                        Priority;             // in: priority of allocation
        D3DKMT_ALIGN64 ULONG_PTR    Unused;
    };
    D3DKMT_ALIGN64 ULONG_PTR        Reserved[5];          // Reserved
#else
    D3DKMT_ALIGN64 ULONG_PTR        Reserved[6];          // Reserved
#endif
} D3DDDI_ALLOCATIONINFO2;

#endif

typedef struct _D3DDDI_OPENALLOCATIONINFO
{
    D3DKMT_HANDLE   hAllocation;                // in: Handle for this allocation in this process
    D3DKMT_PTR(CONST VOID*, pPrivateDriverData);         // in: Ptr to driver private buffer for this allocations
    UINT            PrivateDriverDataSize;      // in: Size in bytes of driver private buffer for this allocations

} D3DDDI_OPENALLOCATIONINFO;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7))

typedef struct _D3DDDI_OPENALLOCATIONINFO2
{
    D3DKMT_HANDLE   hAllocation;                // in: Handle for this allocation in this process
    D3DKMT_PTR(CONST VOID*, pPrivateDriverData);// in: Ptr to driver private buffer for this allocations
    UINT            PrivateDriverDataSize;      // in: Size in bytes of driver private buffer for this allocations
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress;   // out: GPU Virtual address of the allocation opened.
    D3DKMT_ALIGN64 ULONG_PTR Reserved[6];       // Reserved
} D3DDDI_OPENALLOCATIONINFO2;

#endif

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8))

typedef enum _D3DDDI_OFFER_PRIORITY
{
    D3DDDI_OFFER_PRIORITY_NONE=0,               // Do not offer
    D3DDDI_OFFER_PRIORITY_LOW=1,                // Content is not useful
    D3DDDI_OFFER_PRIORITY_NORMAL,               // Content is useful but easy to regenerate
    D3DDDI_OFFER_PRIORITY_HIGH,                 // Content is useful and difficult to regenerate
    D3DDDI_OFFER_PRIORITY_AUTO,                 // Let VidMm decide offer priority based on eviction priority
} D3DDDI_OFFER_PRIORITY;

#endif

typedef struct _D3DDDI_ALLOCATIONLIST
{
    D3DKMT_HANDLE       hAllocation;
    union
    {
        struct
        {
            UINT                  WriteOperation      : 1; // 0x00000001
            UINT                  DoNotRetireInstance : 1; // 0x00000002

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8))
            UINT                  OfferPriority       : 3; // 0x0000001C D3DDDI_OFFER_PRIORITY
            UINT                  Reserved            :27; // 0xFFFFFFE0
#else
            UINT                  Reserved            :30; // 0xFFFFFFFC
#endif
        };
        UINT                Value;
    };
} D3DDDI_ALLOCATIONLIST;

typedef struct _D3DDDI_PATCHLOCATIONLIST
{
    UINT                AllocationIndex;
    union
    {
        struct
        {
            UINT            SlotId          : 24;   // 0x00FFFFFF
            UINT            Reserved        : 8;    // 0xFF000000
        };
        UINT                Value;
    };
    UINT                DriverId;
    UINT                AllocationOffset;
    UINT                PatchOffset;
    UINT                SplitOffset;
} D3DDDI_PATCHLOCATIONLIST;

typedef struct _D3DDDICB_LOCKFLAGS
{
    union
    {
        struct
        {
            UINT    ReadOnly            : 1;    // 0x00000001
            UINT    WriteOnly           : 1;    // 0x00000002
            UINT    DonotWait           : 1;    // 0x00000004
            UINT    IgnoreSync          : 1;    // 0x00000008
            UINT    LockEntire          : 1;    // 0x00000010
            UINT    DonotEvict          : 1;    // 0x00000020
            UINT    AcquireAperture     : 1;    // 0x00000040
            UINT    Discard             : 1;    // 0x00000080
            UINT    NoExistingReference : 1;    // 0x00000100
            UINT    UseAlternateVA      : 1;    // 0x00000200
            UINT    IgnoreReadSync      : 1;    // 0x00000400
            UINT    Reserved            :21;    // 0xFFFFF800
        };
        UINT        Value;
    };
} D3DDDICB_LOCKFLAGS;

typedef struct _D3DDDICB_LOCK2FLAGS
{
    union
    {
        struct
        {
            UINT    Reserved            : 32;    // 0xFFFFFFFF
        };
        UINT        Value;
    };
} D3DDDICB_LOCK2FLAGS;

typedef struct _D3DDDICB_DESTROYALLOCATION2FLAGS
{
    union
    {
        struct
        {
            UINT    AssumeNotInUse      :  1;    // 0x00000001
            UINT    SynchronousDestroy  :  1;    // 0x00000002
            UINT    Reserved            : 29;    // 0x7FFFFFFC
            UINT    SystemUseOnly       :  1;    // 0x80000000  // Should not be set by the UMD
        };
        UINT        Value;
    };
} D3DDDICB_DESTROYALLOCATION2FLAGS;

typedef struct _D3DDDI_ESCAPEFLAGS
{
    union
    {
        struct
        {
            UINT    HardwareAccess      : 1;    // 0x00000001
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3))
            UINT    DeviceStatusQuery   : 1;    // 0x00000002
            UINT    ChangeFrameLatency  : 1;    // 0x00000004
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
            UINT    NoAdapterSynchronization    : 1; // 0x00000008
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
            UINT    Reserved            : 1;    // 0x00000010   Used internally by DisplayOnly present
            UINT    VirtualMachineData  : 1;    // 0x00000020   Cannot be set from user mode
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
            UINT    DriverKnownEscape   : 1;    // 0x00000040       // Driver private data points to a well known structure
            UINT    DriverCommonEscape  : 1;    // 0x00000080       // Private data points runtime defined structure
            UINT    Reserved2           :24;    // 0xFFFFFF00
#else  // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
            UINT    Reserved2           :26;    // 0xFFFFFFC0
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
#else
            UINT    Reserved            :28;    // 0xFFFFFFF0
#endif //  (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
#else
            UINT    Reserved            :29;    // 0xFFFFFFF8
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
#else
            UINT    Reserved            :31;    // 0xFFFFFFFE
#endif // WDDM1_3
        };
        UINT        Value;
    };
} D3DDDI_ESCAPEFLAGS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

typedef enum _D3DDDI_DRIVERESCAPETYPE
{
    D3DDDI_DRIVERESCAPETYPE_TRANSLATEALLOCATIONHANDLE   = 0,
    D3DDDI_DRIVERESCAPETYPE_TRANSLATERESOURCEHANDLE     = 1,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    D3DDDI_DRIVERESCAPETYPE_CPUEVENTUSAGE               = 2,
#endif
    D3DDDI_DRIVERESCAPETYPE_MAX,
} D3DDDI_DRIVERESCAPETYPE;

typedef struct _D3DDDI_DRIVERESCAPE_TRANSLATEALLOCATIONEHANDLE
{
     D3DDDI_DRIVERESCAPETYPE  EscapeType;
     D3DKMT_HANDLE            hAllocation;
} D3DDDI_DRIVERESCAPE_TRANSLATEALLOCATIONEHANDLE;

typedef struct _D3DDDI_DRIVERESCAPE_TRANSLATERESOURCEHANDLE
{
    D3DDDI_DRIVERESCAPETYPE  EscapeType;
    D3DKMT_HANDLE            hResource;
} D3DDDI_DRIVERESCAPE_TRANSLATERESOURCEHANDLE;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)

typedef struct _D3DDDI_DRIVERESCAPE_CPUEVENTUSAGE
{
    D3DDDI_DRIVERESCAPETYPE EscapeType;
    D3DKMT_HANDLE           hSyncObject;
    D3DKMT_ALIGN64 UINT64   hKmdCpuEvent;
    UINT                    Usage[8];
} D3DDDI_DRIVERESCAPE_CPUEVENTUSAGE;

#endif

typedef struct _D3DDDI_CREATECONTEXTFLAGS
{
    union
    {
        struct
        {
            UINT    NullRendering       : 1;      // 0x00000001
            UINT    InitialData         : 1;      // 0x00000002
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))
            UINT    DisableGpuTimeout   : 1;      // 0x00000004
            UINT    SynchronizationOnly : 1;      // 0x00000008
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_3_1))
            UINT    HwQueueSupported    : 1;      // 0x00000010
            UINT    NoKmdAccess         : 1;      // 0x00000020
            UINT    Reserved            :26;      // 0xFFFFFFC0
#else
            UINT    Reserved            :28;      // 0xFFFFFFF0
#endif // DXGKDDI_INTERFACE_VERSION

#else
            UINT    Reserved            :30;      // 0xFFFFFFFC
#endif // DXGKDDI_INTERFACE_VERSION
        };
        UINT Value;
    };
} D3DDDI_CREATECONTEXTFLAGS;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1))

typedef struct _D3DDDI_CREATEHWCONTEXTFLAGS
{
    union
    {
        struct
        {
            UINT    Reserved            :32;      // 0xFFFFFFFF
        };
        UINT Value;
    };
} D3DDDI_CREATEHWCONTEXTFLAGS;

typedef struct _D3DDDI_CREATEHWQUEUEFLAGS
{
    union
    {
        struct
        {
            UINT    DisableGpuTimeout   : 1;      // 0x00000001
            UINT    NoBroadcastSignal   : 1;      // 0x00000002
            UINT    NoBroadcastWait     : 1;      // 0x00000004
            UINT    NoKmdAccess         : 1;      // 0x00000008
            UINT    Reserved            :28;      // 0xFFFFFFF0
        };
        UINT Value;
    };
} D3DDDI_CREATEHWQUEUEFLAGS;

#endif // ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _D3DDDI_SEGMENTPREFERENCE
{
    union
    {
        struct
        {
            UINT SegmentId0 : 5;                // 0x0000001F
            UINT Direction0 : 1;                // 0x00000020
            UINT SegmentId1 : 5;                // 0x000007C0
            UINT Direction1 : 1;                // 0x00000800
            UINT SegmentId2 : 5;                // 0x0001F000
            UINT Direction2 : 1;                // 0x00020000
            UINT SegmentId3 : 5;                // 0x007C0000
            UINT Direction3 : 1;                // 0x00800000
            UINT SegmentId4 : 5;                // 0x1F000000
            UINT Direction4 : 1;                // 0x20000000
            UINT Reserved   : 2;                // 0xC0000000
        };
        UINT Value;
    };
} D3DDDI_SEGMENTPREFERENCE;

/* Formats
 * Most of these names have the following convention:
 *      A = Alpha
 *      R = Red
 *      G = Green
 *      B = Blue
 *      X = Unused Bits
 *      P = Palette
 *      L = Luminance
 *      U = dU coordinate for BumpMap
 *      V = dV coordinate for BumpMap
 *      S = Stencil
 *      D = Depth (e.g. Z or W buffer)
 *      C = Computed from other channels (typically on certain read operations)
 *
 *      Further, the order of the pieces are from MSB first; hence
 *      D3DFMT_A8L8 indicates that the high byte of this two byte
 *      format is alpha.
 *
 *      D16 indicates:
 *           - An integer 16-bit value.
 *           - An app-lockable surface.
 *
 *      All Depth/Stencil formats except D3DFMT_D16_LOCKABLE indicate:
 *          - no particular bit ordering per pixel, and
 *          - are not app lockable, and
 *          - the driver is allowed to consume more than the indicated
 *            number of bits per Depth channel (but not Stencil channel).
 */
#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */


typedef enum _D3DDDIFORMAT
{

    D3DDDIFMT_UNKNOWN           =  0,

    D3DDDIFMT_R8G8B8            = 20,
    D3DDDIFMT_A8R8G8B8          = 21,
    D3DDDIFMT_X8R8G8B8          = 22,
    D3DDDIFMT_R5G6B5            = 23,
    D3DDDIFMT_X1R5G5B5          = 24,
    D3DDDIFMT_A1R5G5B5          = 25,
    D3DDDIFMT_A4R4G4B4          = 26,
    D3DDDIFMT_R3G3B2            = 27,
    D3DDDIFMT_A8                = 28,
    D3DDDIFMT_A8R3G3B2          = 29,
    D3DDDIFMT_X4R4G4B4          = 30,
    D3DDDIFMT_A2B10G10R10       = 31,
    D3DDDIFMT_A8B8G8R8          = 32,
    D3DDDIFMT_X8B8G8R8          = 33,
    D3DDDIFMT_G16R16            = 34,
    D3DDDIFMT_A2R10G10B10       = 35,
    D3DDDIFMT_A16B16G16R16      = 36,

    D3DDDIFMT_A8P8              = 40,
    D3DDDIFMT_P8                = 41,

    D3DDDIFMT_L8                = 50,
    D3DDDIFMT_A8L8              = 51,
    D3DDDIFMT_A4L4              = 52,

    D3DDDIFMT_V8U8              = 60,
    D3DDDIFMT_L6V5U5            = 61,
    D3DDDIFMT_X8L8V8U8          = 62,
    D3DDDIFMT_Q8W8V8U8          = 63,
    D3DDDIFMT_V16U16            = 64,
    D3DDDIFMT_W11V11U10         = 65,
    D3DDDIFMT_A2W10V10U10       = 67,

    D3DDDIFMT_UYVY              = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DDDIFMT_R8G8_B8G8         = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DDDIFMT_YUY2              = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DDDIFMT_G8R8_G8B8         = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DDDIFMT_DXT1              = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DDDIFMT_DXT2              = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DDDIFMT_DXT3              = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DDDIFMT_DXT4              = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DDDIFMT_DXT5              = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DDDIFMT_D16_LOCKABLE      = 70,
    D3DDDIFMT_D32               = 71,
    D3DDDIFMT_D15S1             = 73,
    D3DDDIFMT_D24S8             = 75,
    D3DDDIFMT_D24X8             = 77,
    D3DDDIFMT_D24X4S4           = 79,
    D3DDDIFMT_D16               = 80,

    D3DDDIFMT_D32F_LOCKABLE     = 82,
    D3DDDIFMT_D24FS8            = 83,

    D3DDDIFMT_D32_LOCKABLE      = 84,
    D3DDDIFMT_S8_LOCKABLE       = 85,

    D3DDDIFMT_S1D15             = 72,
    D3DDDIFMT_S8D24             = 74,
    D3DDDIFMT_X8D24             = 76,
    D3DDDIFMT_X4S4D24           = 78,

    D3DDDIFMT_L16               = 81,
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
    D3DDDIFMT_G8R8              = 91,
    D3DDDIFMT_R8                = 92,
#endif

    D3DDDIFMT_VERTEXDATA        =100,
    D3DDDIFMT_INDEX16           =101,
    D3DDDIFMT_INDEX32           =102,

    D3DDDIFMT_Q16W16V16U16      =110,

    D3DDDIFMT_MULTI2_ARGB8      = MAKEFOURCC('M','E','T','1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DDDIFMT_R16F              = 111,
    D3DDDIFMT_G16R16F           = 112,
    D3DDDIFMT_A16B16G16R16F     = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DDDIFMT_R32F              = 114,
    D3DDDIFMT_G32R32F           = 115,
    D3DDDIFMT_A32B32G32R32F     = 116,

    D3DDDIFMT_CxV8U8            = 117,

    // Monochrome 1 bit per pixel format
    D3DDDIFMT_A1                = 118,

    // 2.8 biased fixed point
    D3DDDIFMT_A2B10G10R10_XR_BIAS = 119,

    // Decode compressed buffer formats
    D3DDDIFMT_DXVACOMPBUFFER_BASE     = 150,
    D3DDDIFMT_PICTUREPARAMSDATA       = D3DDDIFMT_DXVACOMPBUFFER_BASE+0,    // 150
    D3DDDIFMT_MACROBLOCKDATA          = D3DDDIFMT_DXVACOMPBUFFER_BASE+1,    // 151
    D3DDDIFMT_RESIDUALDIFFERENCEDATA  = D3DDDIFMT_DXVACOMPBUFFER_BASE+2,    // 152
    D3DDDIFMT_DEBLOCKINGDATA          = D3DDDIFMT_DXVACOMPBUFFER_BASE+3,    // 153
    D3DDDIFMT_INVERSEQUANTIZATIONDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE+4,    // 154
    D3DDDIFMT_SLICECONTROLDATA        = D3DDDIFMT_DXVACOMPBUFFER_BASE+5,    // 155
    D3DDDIFMT_BITSTREAMDATA           = D3DDDIFMT_DXVACOMPBUFFER_BASE+6,    // 156
    D3DDDIFMT_MOTIONVECTORBUFFER      = D3DDDIFMT_DXVACOMPBUFFER_BASE+7,    // 157
    D3DDDIFMT_FILMGRAINBUFFER         = D3DDDIFMT_DXVACOMPBUFFER_BASE+8,    // 158
    D3DDDIFMT_DXVA_RESERVED9          = D3DDDIFMT_DXVACOMPBUFFER_BASE+9,    // 159
    D3DDDIFMT_DXVA_RESERVED10         = D3DDDIFMT_DXVACOMPBUFFER_BASE+10,   // 160
    D3DDDIFMT_DXVA_RESERVED11         = D3DDDIFMT_DXVACOMPBUFFER_BASE+11,   // 161
    D3DDDIFMT_DXVA_RESERVED12         = D3DDDIFMT_DXVACOMPBUFFER_BASE+12,   // 162
    D3DDDIFMT_DXVA_RESERVED13         = D3DDDIFMT_DXVACOMPBUFFER_BASE+13,   // 163
    D3DDDIFMT_DXVA_RESERVED14         = D3DDDIFMT_DXVACOMPBUFFER_BASE+14,   // 164
    D3DDDIFMT_DXVA_RESERVED15         = D3DDDIFMT_DXVACOMPBUFFER_BASE+15,   // 165
    D3DDDIFMT_DXVA_RESERVED16         = D3DDDIFMT_DXVACOMPBUFFER_BASE+16,   // 166
    D3DDDIFMT_DXVA_RESERVED17         = D3DDDIFMT_DXVACOMPBUFFER_BASE+17,   // 167
    D3DDDIFMT_DXVA_RESERVED18         = D3DDDIFMT_DXVACOMPBUFFER_BASE+18,   // 168
    D3DDDIFMT_DXVA_RESERVED19         = D3DDDIFMT_DXVACOMPBUFFER_BASE+19,   // 169
    D3DDDIFMT_DXVA_RESERVED20         = D3DDDIFMT_DXVACOMPBUFFER_BASE+20,   // 170
    D3DDDIFMT_DXVA_RESERVED21         = D3DDDIFMT_DXVACOMPBUFFER_BASE+21,   // 171
    D3DDDIFMT_DXVA_RESERVED22         = D3DDDIFMT_DXVACOMPBUFFER_BASE+22,   // 172
    D3DDDIFMT_DXVA_RESERVED23         = D3DDDIFMT_DXVACOMPBUFFER_BASE+23,   // 173
    D3DDDIFMT_DXVA_RESERVED24         = D3DDDIFMT_DXVACOMPBUFFER_BASE+24,   // 174
    D3DDDIFMT_DXVA_RESERVED25         = D3DDDIFMT_DXVACOMPBUFFER_BASE+25,   // 175
    D3DDDIFMT_DXVA_RESERVED26         = D3DDDIFMT_DXVACOMPBUFFER_BASE+26,   // 176
    D3DDDIFMT_DXVA_RESERVED27         = D3DDDIFMT_DXVACOMPBUFFER_BASE+27,   // 177
    D3DDDIFMT_DXVA_RESERVED28         = D3DDDIFMT_DXVACOMPBUFFER_BASE+28,   // 178
    D3DDDIFMT_DXVA_RESERVED29         = D3DDDIFMT_DXVACOMPBUFFER_BASE+29,   // 179
    D3DDDIFMT_DXVA_RESERVED30         = D3DDDIFMT_DXVACOMPBUFFER_BASE+30,   // 180
    D3DDDIFMT_DXVA_RESERVED31         = D3DDDIFMT_DXVACOMPBUFFER_BASE+31,   // 181
    D3DDDIFMT_DXVACOMPBUFFER_MAX      = D3DDDIFMT_DXVA_RESERVED31,

    D3DDDIFMT_BINARYBUFFER            = 199,

    D3DDDIFMT_FORCE_UINT        =0x7fffffff
} D3DDDIFORMAT;

typedef enum D3DDDI_COLOR_SPACE_TYPE
{
    D3DDDI_COLOR_SPACE_RGB_FULL_G22_NONE_P709             = 0,
    D3DDDI_COLOR_SPACE_RGB_FULL_G10_NONE_P709             = 1,
    D3DDDI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709           = 2,
    D3DDDI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020          = 3,
    D3DDDI_COLOR_SPACE_RESERVED                           = 4,
    D3DDDI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601      = 5,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601         = 6,
    D3DDDI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601           = 7,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709         = 8,
    D3DDDI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709           = 9,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020        = 10,
    D3DDDI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020          = 11,
    D3DDDI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020          = 12,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020      = 13,
    D3DDDI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020        = 14,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020     = 15,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020   = 16,
    D3DDDI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020            = 17,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020    = 18,
    D3DDDI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020      = 19,
    D3DDDI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709           = 20,
    D3DDDI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020          = 21,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709         = 22,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020        = 23,
    D3DDDI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020     = 24,
    D3DDDI_COLOR_SPACE_CUSTOM                             = 0xFFFFFFFF
} D3DDDI_COLOR_SPACE_TYPE;

//
// Note: This enum is intended to specify the final wire signaling
// colorspace values. Do not mix it with enum values defined in
// D3DDDI_COLOR_SPACE_TYPE which are used to specify
// input colorspace for MPOs and other surfaces.
//
typedef enum _D3DDDI_OUTPUT_WIRE_COLOR_SPACE_TYPE
{
    // We are using the same values for these first two enums for
    // backward compatibility to WDDM2.2 drivers which used
    // to get these 2 values from D3DDDI_COLOR_SPACE_TYPE
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G22_P709               = 0,
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_RESERVED               = 4,
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G2084_P2020            = 12,

    // We are starting the new enum value at 30 just to make sure it
    // is not confused with the existing D3DDDI_COLOR_SPACE_TYPE
    // in the short term
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G22_P709_WCG           = 30,

    // OS only intend to use the _G22_P2020 value in future,
    // for now graphics drivers should not expect it.
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G22_P2020              = 31,

    // OS only intend to use the _G2084_P2020_HDR10PLUS value in future,
    // for now graphics drivers should not expect it.
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G2084_P2020_HDR10PLUS  = 32,

    // OS only intend to use the _G2084_P2020_DVLL value in future,
    // for now graphics drivers should not expect it.
    D3DDDI_OUTPUT_WIRE_COLOR_SPACE_G2084_P2020_DVLL       = 33,
} D3DDDI_OUTPUT_WIRE_COLOR_SPACE_TYPE;

typedef struct _D3DDDIRECT
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} D3DDDIRECT;

typedef struct _D3DDDI_KERNELOVERLAYINFO
{
    D3DKMT_HANDLE        hAllocation;           // in: Allocation to be displayed
    D3DDDIRECT           DstRect;               // in: Dest rect
    D3DDDIRECT           SrcRect;               // in: Source rect
    D3DKMT_PTR(VOID*,    pPrivateDriverData);   // in: Private driver data
    UINT                 PrivateDriverDataSize; // in: Size of private driver data
} D3DDDI_KERNELOVERLAYINFO;

typedef enum _D3DDDI_GAMMARAMP_TYPE
{
    D3DDDI_GAMMARAMP_UNINITIALIZED = 0,
    D3DDDI_GAMMARAMP_DEFAULT       = 1,
    D3DDDI_GAMMARAMP_RGB256x3x16   = 2,
    D3DDDI_GAMMARAMP_DXGI_1        = 3,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    D3DDDI_GAMMARAMP_MATRIX_3x4    = 4,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    D3DDDI_GAMMARAMP_MATRIX_V2     = 5,
#endif // DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6
} D3DDDI_GAMMARAMP_TYPE;

typedef struct _D3DDDI_GAMMA_RAMP_RGB256x3x16
{
    USHORT  Red[256];
    USHORT  Green[256];
    USHORT  Blue[256];
} D3DDDI_GAMMA_RAMP_RGB256x3x16;

typedef struct D3DDDI_DXGI_RGB
{
    float   Red;
    float   Green;
    float   Blue;
} D3DDDI_DXGI_RGB;

typedef struct _D3DDDI_GAMMA_RAMP_DXGI_1
{
    D3DDDI_DXGI_RGB    Scale;
    D3DDDI_DXGI_RGB    Offset;
    D3DDDI_DXGI_RGB    GammaCurve[1025];
} D3DDDI_GAMMA_RAMP_DXGI_1;

typedef struct _D3DKMDT_3X4_COLORSPACE_TRANSFORM
{
    float               ColorMatrix3x4[3][4];
    float               ScalarMultiplier;
    D3DDDI_DXGI_RGB     LookupTable1D[4096];
} D3DKMDT_3x4_COLORSPACE_TRANSFORM, *PD3DDDI_3x4_COLORSPACE_TRANSFORM;

typedef enum _D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL
{
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL_NO_CHANGE = 0,
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL_ENABLE    = 1,
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL_BYPASS    = 2,
}D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL, *PD3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL;

typedef struct _D3DKMDT_COLORSPACE_TRANSFORM_MATRIX_V2
{
    // stage of 1D Degamma.
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL  StageControlLookupTable1DDegamma;
    D3DDDI_DXGI_RGB                             LookupTable1DDegamma[4096];

    // stage of 3x3 matrix
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL  StageControlColorMatrix3x3;
    float                                       ColorMatrix3x3[3][3];

    // stage of 1D Regamma.
    D3DKMDT_COLORSPACE_TRANSFORM_STAGE_CONTROL  StageControlLookupTable1DRegamma;
    D3DDDI_DXGI_RGB                             LookupTable1DRegamma[4096];
} D3DKMDT_COLORSPACE_TRANSFORM_MATRIX_V2, *PD3DKMDT_COLORSPACE_TRANSFORM_MATRIX_V2;

typedef enum _D3DDDI_HDR_METADATA_TYPE
{
    D3DDDI_HDR_METADATA_TYPE_NONE               = 0,
    D3DDDI_HDR_METADATA_TYPE_HDR10              = 1,
    D3DDDI_HDR_METADATA_TYPE_HDR10PLUS         = 2,
} D3DDDI_HDR_METADATA_TYPE;

typedef struct _D3DDDI_HDR_METADATA_HDR10
{
    // Color gamut
    UINT16 RedPrimary[2];
    UINT16 GreenPrimary[2];
    UINT16 BluePrimary[2];
    UINT16 WhitePoint[2];

    // Luminance
    UINT   MaxMasteringLuminance;
    UINT   MinMasteringLuminance;
    UINT16 MaxContentLightLevel;
    UINT16 MaxFrameAverageLightLevel;
} D3DDDI_HDR_METADATA_HDR10;

typedef struct D3DDDI_HDR_METADATA_HDR10PLUS
{
    BYTE Data[72];
} D3DDDI_HDR_METADATA_HDR10PLUS;

// Used as a value for D3DDDI_VIDEO_PRESENT_SOURCE_ID and D3DDDI_VIDEO_PRESENT_TARGET_ID types to specify
// that the respective video present source/target ID hasn't been initialized.
#define D3DDDI_ID_UNINITIALIZED (UINT)(~0)

// TODO:[mmilirud] Define this as (UINT)(~1) to avoid collision with valid source ID equal to 0.
//
// Used as a value for D3DDDI_VIDEO_PRESENT_SOURCE_ID and D3DDDI_VIDEO_PRESENT_TARGET_ID types to specify
// that the respective video present source/target ID isn't applicable for the given execution context.
#define D3DDDI_ID_NOTAPPLICABLE (UINT)(0)

// Indicates that a resource can be associated with "any" VidPn source, even none at all.
#define D3DDDI_ID_ANY (UINT)(~1)

// Used as a value for D3DDDI_VIDEO_PRESENT_SOURCE_ID and D3DDDI_VIDEO_PRESENT_TARGET_ID types to specify
// that the respective video present source/target ID describes every VidPN source/target in question.
#define D3DDDI_ID_ALL (UINT)(~2)

//
// Hardcoded VidPnSource count
//
#define D3DKMDT_MAX_VIDPN_SOURCES_BITCOUNT      4
#define D3DKMDT_MAX_VIDPN_SOURCES               (1 << D3DKMDT_MAX_VIDPN_SOURCES_BITCOUNT)


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Multi-sampling method descriptor type.
//
// Remarks: Driver is free to partition its quality levels for a given multi-sampling method into as many
//          increments as it likes, with the condition that each incremental step does noticably improve
//          quality of the presented image.
//
typedef struct _D3DDDI_MULTISAMPLINGMETHOD
{
    // Number of sub-pixels employed in this multi-sampling method (e.g. 2 for 2x and 8 for 8x multi-sampling)
    UINT  NumSamples;

    // Upper bound on the quality range supported for this multi-sampling method. The range starts from 0
    // and goes upto and including the reported maximum quality setting.
    UINT  NumQualityLevels;
}
D3DDDI_MULTISAMPLINGMETHOD;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video signal scan line ordering descriptor type.
//
// Remarks: Scan-line ordering of the video mode, specifies whether each field contains the entire
//          content of a frame, or only half of it (i.e. even/odd lines interchangeably).
//          Note that while for standard interlaced modes, what field comes first can be inferred
//          from the mode, specifying this characteristic explicitly with an enum both frees up the
//          client from having to maintain mode-based look-up tables and is extensible for future
//          standard modes not listed in the D3DKMDT_VIDEO_SIGNAL_STANDARD enum.
//
typedef enum _D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING
{
    D3DDDI_VSSLO_UNINITIALIZED              = 0,
    D3DDDI_VSSLO_PROGRESSIVE                = 1,
    D3DDDI_VSSLO_INTERLACED_UPPERFIELDFIRST = 2,
    D3DDDI_VSSLO_INTERLACED_LOWERFIELDFIRST = 3,
    D3DDDI_VSSLO_OTHER                      = 255
}
D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING;


typedef enum D3DDDI_FLIPINTERVAL_TYPE
{
    D3DDDI_FLIPINTERVAL_IMMEDIATE = 0,
    D3DDDI_FLIPINTERVAL_ONE       = 1,
    D3DDDI_FLIPINTERVAL_TWO       = 2,
    D3DDDI_FLIPINTERVAL_THREE     = 3,
    D3DDDI_FLIPINTERVAL_FOUR      = 4,

    // This value is only valid for the D3D9 runtime PresentCb SyncIntervalOverride field.
    // For this field, IMMEDIATE means the API semantic of sync interval 0, where
    // IMMEDIATE_ALLOW_TEARING is equivalent to the addition of the DXGI ALLOW_TEARING API flags.
    D3DDDI_FLIPINTERVAL_IMMEDIATE_ALLOW_TEARING = 5,
} D3DDDI_FLIPINTERVAL_TYPE;


typedef enum _D3DDDI_POOL
{
     D3DDDIPOOL_SYSTEMMEM      = 1,
     D3DDDIPOOL_VIDEOMEMORY    = 2,
     D3DDDIPOOL_LOCALVIDMEM    = 3,
     D3DDDIPOOL_NONLOCALVIDMEM = 4,
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
     D3DDDIPOOL_STAGINGMEM     = 5,
#endif
} D3DDDI_POOL;


typedef enum _D3DDDIMULTISAMPLE_TYPE
{
    D3DDDIMULTISAMPLE_NONE         =  0,
    D3DDDIMULTISAMPLE_NONMASKABLE  =  1,
    D3DDDIMULTISAMPLE_2_SAMPLES    =  2,
    D3DDDIMULTISAMPLE_3_SAMPLES    =  3,
    D3DDDIMULTISAMPLE_4_SAMPLES    =  4,
    D3DDDIMULTISAMPLE_5_SAMPLES    =  5,
    D3DDDIMULTISAMPLE_6_SAMPLES    =  6,
    D3DDDIMULTISAMPLE_7_SAMPLES    =  7,
    D3DDDIMULTISAMPLE_8_SAMPLES    =  8,
    D3DDDIMULTISAMPLE_9_SAMPLES    =  9,
    D3DDDIMULTISAMPLE_10_SAMPLES   = 10,
    D3DDDIMULTISAMPLE_11_SAMPLES   = 11,
    D3DDDIMULTISAMPLE_12_SAMPLES   = 12,
    D3DDDIMULTISAMPLE_13_SAMPLES   = 13,
    D3DDDIMULTISAMPLE_14_SAMPLES   = 14,
    D3DDDIMULTISAMPLE_15_SAMPLES   = 15,
    D3DDDIMULTISAMPLE_16_SAMPLES   = 16,

    D3DDDIMULTISAMPLE_FORCE_UINT   = 0x7fffffff
} D3DDDIMULTISAMPLE_TYPE;

typedef struct _D3DDDI_RESOURCEFLAGS
{
    union
    {
        struct
        {
            UINT    RenderTarget            : 1;    // 0x00000001
            UINT    ZBuffer                 : 1;    // 0x00000002
            UINT    Dynamic                 : 1;    // 0x00000004
            UINT    HintStatic              : 1;    // 0x00000008
            UINT    AutogenMipmap           : 1;    // 0x00000010
            UINT    DMap                    : 1;    // 0x00000020
            UINT    WriteOnly               : 1;    // 0x00000040
            UINT    NotLockable             : 1;    // 0x00000080
            UINT    Points                  : 1;    // 0x00000100
            UINT    RtPatches               : 1;    // 0x00000200
            UINT    NPatches                : 1;    // 0x00000400
            UINT    SharedResource          : 1;    // 0x00000800
            UINT    DiscardRenderTarget     : 1;    // 0x00001000
            UINT    Video                   : 1;    // 0x00002000
            UINT    CaptureBuffer           : 1;    // 0x00004000
            UINT    Primary                 : 1;    // 0x00008000
            UINT    Texture                 : 1;    // 0x00010000
            UINT    CubeMap                 : 1;    // 0x00020000
            UINT    Volume                  : 1;    // 0x00040000
            UINT    VertexBuffer            : 1;    // 0x00080000
            UINT    IndexBuffer             : 1;    // 0x00100000
            UINT    DecodeRenderTarget      : 1;    // 0x00200000
            UINT    DecodeCompressedBuffer  : 1;    // 0x00400000
            UINT    VideoProcessRenderTarget: 1;    // 0x00800000
            UINT    CpuOptimized            : 1;    // 0x01000000
            UINT    MightDrawFromLocked     : 1;    // 0x02000000
            UINT    Overlay                 : 1;    // 0x04000000
            UINT    MatchGdiPrimary         : 1;    // 0x08000000
            UINT    InterlacedRefresh       : 1;    // 0x10000000
            UINT    TextApi                 : 1;    // 0x20000000
            UINT    RestrictedContent       : 1;    // 0x40000000
            UINT    RestrictSharedAccess    : 1;    // 0x80000000
        };
        UINT        Value;
    };
} D3DDDI_RESOURCEFLAGS;

typedef struct _D3DDDI_SURFACEINFO
{
    UINT                Width;              // in: For linear, surface and volume
    UINT                Height;             // in: For surface and volume
    UINT                Depth;              // in: For volume
    CONST VOID*         pSysMem;
    UINT                SysMemPitch;
    UINT                SysMemSlicePitch;
} D3DDDI_SURFACEINFO;

typedef enum _D3DDDI_ROTATION
{
    D3DDDI_ROTATION_IDENTITY        = 1,    // No rotation.
    D3DDDI_ROTATION_90              = 2,    // Rotated 90 degrees.
    D3DDDI_ROTATION_180             = 3,    // Rotated 180 degrees.
    D3DDDI_ROTATION_270             = 4     // Rotated 270 degrees.
} D3DDDI_ROTATION;

typedef enum D3DDDI_SCANLINEORDERING
{
    D3DDDI_SCANLINEORDERING_UNKNOWN                    = 0,
    D3DDDI_SCANLINEORDERING_PROGRESSIVE                = 1,
    D3DDDI_SCANLINEORDERING_INTERLACED                 = 2,
} D3DDDI_SCANLINEORDERING;

typedef struct _D3DDDIARG_CREATERESOURCE
{
    D3DDDIFORMAT                    Format;
    D3DDDI_POOL                     Pool;
    D3DDDIMULTISAMPLE_TYPE          MultisampleType;
    UINT                            MultisampleQuality;
    CONST D3DDDI_SURFACEINFO*       pSurfList;          // in: List of sub resource objects to create
    UINT                            SurfCount;          // in: Number of sub resource objects
    UINT                            MipLevels;
    UINT                            Fvf;                // in: FVF format for vertex buffers
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: VidPnSourceId on which the primary surface is created
    D3DDDI_RATIONAL                 RefreshRate;        // in: RefreshRate that this primary surface is to be used with
    HANDLE                          hResource;          // in/out: D3D runtime handle/UM driver handle
    D3DDDI_RESOURCEFLAGS            Flags;
    D3DDDI_ROTATION                 Rotation;           // in: The orientation of the resource. (0, 90, 180, 270)
} D3DDDIARG_CREATERESOURCE;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8))

typedef struct _D3DDDI_RESOURCEFLAGS2
{
    union
    {
        struct
        {
            UINT    VideoEncoder            : 1;    // 0x00000001
            UINT    UserMemory              : 1;    // 0x00000002
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3)) // M1
            UINT    CrossAdapter            : 1;    // 0x00000004
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
        (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))
            UINT    IsDisplayable           : 1;    // 0x00000008
            UINT    Reserved                : 28;
#else
            UINT    Reserved                : 29;
#endif
#else
            UINT    Reserved                : 30;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
         };
        UINT        Value;
    };
} D3DDDI_RESOURCEFLAGS2;


typedef struct _D3DDDIARG_CREATERESOURCE2
{
    D3DDDIFORMAT                    Format;
    D3DDDI_POOL                     Pool;
    D3DDDIMULTISAMPLE_TYPE          MultisampleType;
    UINT                            MultisampleQuality;
    CONST D3DDDI_SURFACEINFO*       pSurfList;          // in: List of sub resource objects to create
    UINT                            SurfCount;          // in: Number of sub resource objects
    UINT                            MipLevels;
    UINT                            Fvf;                // in: FVF format for vertex buffers
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in: VidPnSourceId on which the primary surface is created
    D3DDDI_RATIONAL                 RefreshRate;        // in: RefreshRate that this primary surface is to be used with
    HANDLE                          hResource;          // in/out: D3D runtime handle/UM driver handle
    D3DDDI_RESOURCEFLAGS            Flags;
    D3DDDI_ROTATION                 Rotation;           // in: The orientation of the resource. (0, 90, 180, 270)
    D3DDDI_RESOURCEFLAGS2           Flags2;
} D3DDDIARG_CREATERESOURCE2;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

typedef struct _D3DDDICB_SIGNALFLAGS
{
    union
    {
        struct
        {
            UINT SignalAtSubmission : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
            UINT EnqueueCpuEvent    : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
            UINT AllowFenceRewind   : 1;
            UINT Reserved           : 28;
            UINT DXGK_SIGNAL_FLAG_INTERNAL0 : 1;
#else
            UINT Reserved           : 30;
#endif
#else
            UINT Reserved           : 31;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        };
        UINT Value;
    };
} D3DDDICB_SIGNALFLAGS;

#define D3DDDI_MAX_OBJECT_WAITED_ON 32
#define D3DDDI_MAX_OBJECT_SIGNALED  32

typedef enum _D3DDDI_SYNCHRONIZATIONOBJECT_TYPE
{
    D3DDDI_SYNCHRONIZATION_MUTEX    = 1,
    D3DDDI_SEMAPHORE                = 2,
    D3DDDI_FENCE                    = 3,
    D3DDDI_CPU_NOTIFICATION         = 4,

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))
    D3DDDI_MONITORED_FENCE          = 5,
#endif // DXGKDDI_INTERFACE_VERSION

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2))
    D3DDDI_PERIODIC_MONITORED_FENCE = 6,
#endif // DXGKDDI_INTERFACE_VERSION

    D3DDDI_SYNCHRONIZATION_TYPE_LIMIT

} D3DDDI_SYNCHRONIZATIONOBJECT_TYPE;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))

#define D3DDDI_SYNC_OBJECT_WAIT    0x1
#define D3DDDI_SYNC_OBJECT_SIGNAL  0x2
#define D3DDDI_SYNC_OBJECT_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | D3DDDI_SYNC_OBJECT_WAIT | D3DDDI_SYNC_OBJECT_SIGNAL)

#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

typedef union
{
    struct
    {
        UINT64 FrameNumber : 40;
        UINT64 PartNumber  : 24;
    };
    UINT64 Value;
}DXGK_MIRACAST_CHUNK_ID;

typedef enum _DXGK_MIRACAST_CHUNK_TYPE
{
    DXGK_MIRACAST_CHUNK_TYPE_UNKNOWN = 0,
    DXGK_MIRACAST_CHUNK_TYPE_COLOR_CONVERT_COMPLETE = 1,
    DXGK_MIRACAST_CHUNK_TYPE_ENCODE_COMPLETE = 2,
    DXGK_MIRACAST_CHUNK_TYPE_FRAME_START = 3,
    DXGK_MIRACAST_CHUNK_TYPE_FRAME_DROPPED = 4,
    DXGK_MIRACAST_CHUNK_TYPE_ENCODE_DRIVER_DEFINED_1 = 0x80000000,
    DXGK_MIRACAST_CHUNK_TYPE_ENCODE_DRIVER_DEFINED_2 = 0x80000001,
} DXGK_MIRACAST_CHUNK_TYPE;

typedef struct
{
    DXGK_MIRACAST_CHUNK_TYPE ChunkType; // Type of chunk info
    DXGK_MIRACAST_CHUNK_ID ChunkId;     // Identifier for this chunk
    UINT  ProcessingTime;               // Time the process took to complete in microsecond
    UINT  EncodeRate;                   // Encode bitrate driver reported for the chunk, kilobits per second
} DXGK_MIRACAST_CHUNK_INFO;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))

typedef enum D3DDDI_PAGINGQUEUE_PRIORITY
{
    D3DDDI_PAGINGQUEUE_PRIORITY_BELOW_NORMAL = -1,
    D3DDDI_PAGINGQUEUE_PRIORITY_NORMAL = 0,
    D3DDDI_PAGINGQUEUE_PRIORITY_ABOVE_NORMAL = 1,
} D3DDDI_PAGINGQUEUE_PRIORITY;

typedef struct D3DDDI_MAKERESIDENT_FLAGS
{
    union
    {
        struct
        {
            UINT CantTrimFurther    : 1;    // When set, MakeResidentCb will succeed even if the request puts the application over the current budget.
                                            // MakeResidentCb will still fail if the request puts the application over the maximum budget.
            UINT MustSucceed        : 1;    // When set, instructs MakeResidentCb to put the device in error if the resource cannot be made resident.
            UINT Reserved           : 30;
        };
        UINT Value;
    };
} D3DDDI_MAKERESIDENT_FLAGS;

typedef struct D3DDDI_MAKERESIDENT
{
    D3DKMT_HANDLE               hPagingQueue;       // [in] Handle to the paging queue used to synchronize paging operations for this call.
    UINT                        NumAllocations;     // [in/out] On input, the number of allocation handles om the AllocationList array. On output,
                                                    //          the number of allocations successfully made resident.
    D3DKMT_PTR(_Field_size_(NumAllocations)
    CONST D3DKMT_HANDLE*,       AllocationList);    // [in] An array of NumAllocations allocation handles
    D3DKMT_PTR(CONST UINT*,     PriorityList);      // [in] Residency priority array for each of the allocations in the resource or allocation list
    D3DDDI_MAKERESIDENT_FLAGS   Flags;              // [in] Residency flags
    D3DKMT_ALIGN64 UINT64       PagingFenceValue;   // [out] Paging fence value to synchronize on before submitting the command
                                                    //      that uses above resources to the GPU. This value applies to the monitored fence
                                                    //      synchronization object associated with hPagingQueue.
    D3DKMT_ALIGN64 UINT64       NumBytesToTrim;     // [out] When MakeResident fails due to being over budget, this value
                                                    //      indicates how much to trim in order for the call to succeed on a retry.
} D3DDDI_MAKERESIDENT;

typedef struct D3DDDI_EVICT_FLAGS
{
    union
    {
        struct
        {
            UINT EvictOnlyIfNecessary   : 1;
            UINT NotWrittenTo           : 1;
            UINT Reserved               : 30;
        };
        UINT Value;
    };
} D3DDDI_EVICT_FLAGS;

typedef struct D3DDDI_TRIMRESIDENCYSET_FLAGS
{
    union
    {
        struct
        {
            UINT PeriodicTrim           : 1;    // When PeriodicTrim flag is set, the driver is required to performed the following operations:
                                                // a) trim all allocations that were not referenced since the previous periodic trim request
                                                // by comparing the allocation last referenced fence with the last periodic trim context fence
                                                // b) Refresh the last periodic trim context fence with the last completed context fence.
            UINT RestartPeriodicTrim    : 1;    // May not be set together with PeriodicTrim flag.
                                                // Reset the last periodic trim context fence to the last completed context fence.
            UINT TrimToBudget           : 1;    // Indicates that the application usage is over the memory budget,
                                                // and NumBytesToTrim bytes should be trimmed to fit in the new memory budget.
            UINT Reserved               : 29;
        };
        UINT Value;
    };
} D3DDDI_TRIMRESIDENCYSET_FLAGS;

typedef struct _D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE
{
    union
    {
        struct
        {
            UINT64 Write            : 1;
            UINT64 Execute          : 1;
            UINT64 Zero             : 1;
            UINT64 NoAccess         : 1;
            UINT64 SystemUseOnly    : 1;    // Should not be set by the UMD
            UINT64 Reserved         : 59;
        };
        D3DKMT_ALIGN64 UINT64 Value;
    };
} D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE;

typedef enum _D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION_TYPE
{
   D3DDDI_UPDATEGPUVIRTUALADDRESS_MAP               = 0,
   D3DDDI_UPDATEGPUVIRTUALADDRESS_UNMAP             = 1,
   D3DDDI_UPDATEGPUVIRTUALADDRESS_COPY              = 2,
   D3DDDI_UPDATEGPUVIRTUALADDRESS_MAP_PROTECT       = 3,
} D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION_TYPE;

typedef struct _D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION
{
    D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION_TYPE OperationType;
    union
    {
        struct
        {
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  BaseAddress;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           SizeInBytes;
            D3DKMT_HANDLE                          hAllocation;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           AllocationOffsetInBytes;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           AllocationSizeInBytes;
        } Map;
        struct
        {
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  BaseAddress;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           SizeInBytes;
            D3DKMT_HANDLE                          hAllocation;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           AllocationOffsetInBytes;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           AllocationSizeInBytes;
            D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE Protection;
            D3DKMT_ALIGN64 UINT64                  DriverProtection;
        } MapProtect;
        struct
        {
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  BaseAddress;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           SizeInBytes;
            D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE Protection;
        } Unmap;    // Used for UNMAP_NOACCESS as well
        struct
        {
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  SourceAddress;
            D3DKMT_ALIGN64 D3DGPU_SIZE_T           SizeInBytes;
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS  DestAddress;
        } Copy;
    };
} D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION;

typedef enum _D3DDDIGPUVIRTUALADDRESS_RESERVATION_TYPE
{
    D3DDDIGPUVIRTUALADDRESS_RESERVE_NO_ACCESS  = 0,
    D3DDDIGPUVIRTUALADDRESS_RESERVE_ZERO       = 1,
    D3DDDIGPUVIRTUALADDRESS_RESERVE_NO_COMMIT  = 2      // Reserved for system use
} D3DDDIGPUVIRTUALADDRESS_RESERVATION_TYPE;

typedef struct D3DDDI_MAPGPUVIRTUALADDRESS
{
    D3DKMT_HANDLE                   hPagingQueue;                   // in: Paging queue to synchronize the operation on.
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS BaseAddress;              // in_opt: Base virtual address to map
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS MinimumAddress;           // in_opt: Minimum virtual address
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS MaximumAddress;           // in_opt: Maximum virtual address
    D3DKMT_HANDLE                   hAllocation;                    // in: Allocation handle to map
    D3DKMT_ALIGN64 D3DGPU_SIZE_T    OffsetInPages;                  // in: Offset in 4 KB pages from the start of the allocation
    D3DKMT_ALIGN64 D3DGPU_SIZE_T    SizeInPages;                    // in: Size in 4 KB pages to map
    D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE    Protection;          // in: Virtual address protection
    D3DKMT_ALIGN64 UINT64           DriverProtection;               // in: Driver specific protection
    UINT                            Reserved0;                      // in:
    D3DKMT_ALIGN64 UINT64           Reserved1;                      // in:
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS VirtualAddress;           // out: Virtual address
    D3DKMT_ALIGN64 UINT64           PagingFenceValue;               // out: Paging fence Id for synchronization
} D3DDDI_MAPGPUVIRTUALADDRESS;

typedef struct D3DDDI_RESERVEGPUVIRTUALADDRESS
{
    union
    {
        D3DKMT_HANDLE               hPagingQueue;                   // in: Paging queue to synchronize the operation on.
        D3DKMT_HANDLE               hAdapter;                       // in: DXG adapter handle. (M2)
    };
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS          BaseAddress;     // in_opt: Base virtual address to map
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS          MinimumAddress;  // in_opt: Minimum virtual address
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS          MaximumAddress;  // in_opt: Maximum virtual address
    D3DKMT_ALIGN64 D3DGPU_SIZE_T                   Size;            // in: Size to reserve in bytes
    union
    {
        D3DDDIGPUVIRTUALADDRESS_RESERVATION_TYPE   ReservationType; // in: Reservation type
        UINT                        Reserved0;                      // M2
    };
    union
    {
        D3DKMT_ALIGN64 UINT64                      DriverProtection;// in: Driver specific protection
        D3DKMT_ALIGN64 UINT64                      Reserved1;       // M2
    };
    D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS          VirtualAddress;  // out: Virtual address
    union
    {
        D3DKMT_ALIGN64 UINT64                      PagingFenceValue;// out: Paging fence Id for synchronization
        D3DKMT_ALIGN64 UINT64                      Reserved2;       // M2
    };
} D3DDDI_RESERVEGPUVIRTUALADDRESS;

typedef struct _D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA
{
    D3DKMT_HANDLE    hResource;
    UINT             PrivateDriverDataSize;
    D3DKMT_PTR(PVOID, pPrivateDriverData);
} D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA;

typedef struct D3DDDI_DESTROYPAGINGQUEUE
{
    D3DKMT_HANDLE           hPagingQueue;   // in: handle to the paging queue to destroy
} D3DDDI_DESTROYPAGINGQUEUE;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1))

typedef struct D3DDDI_UPDATEALLOCPROPERTY_FLAGS
{
    union
    {
        struct
        {
            UINT AccessedPhysically :  1; // The new value for AccessedPhysically on an allocation
            UINT Unmoveable         :  1; // Indicates an allocation cannot be moved while pinned in a memory segment
            UINT Reserved           : 30;
        };
        UINT Value;
    };
} D3DDDI_UPDATEALLOCPROPERTY_FLAGS;

typedef struct D3DDDI_UPDATEALLOCPROPERTY
{
    D3DKMT_HANDLE                       hPagingQueue;           // [in] Handle to the paging queue used to synchronize paging operations for this call.
    D3DKMT_HANDLE                       hAllocation;            // [in] Handle to the allocation to be updated.
    UINT                                SupportedSegmentSet;    // [in] New supported segment set, ignored if the same.
    D3DDDI_SEGMENTPREFERENCE            PreferredSegment;       // [in] New preferred segment set, ignored if the same.
    D3DDDI_UPDATEALLOCPROPERTY_FLAGS    Flags;                  // [in] Flags to set on the allocation, ignored if the same.
    D3DKMT_ALIGN64 UINT64               PagingFenceValue;       // [out] Paging fence value to synchronize on before using the above allocation.
                                                                //       This value applies to the monitored fence synchronization
                                                                //       object associated with hPagingQueue.
    union
    {
        struct
        {
            UINT SetAccessedPhysically  :  1; // [in] When set to 1, will set AccessedPhysically to new value
            UINT SetSupportedSegmentSet :  1; // [in] When set to 1, will set SupportedSegmentSet to new value
            UINT SetPreferredSegment    :  1; // [in] When set to 1, will set PreferredSegment to new value
            UINT SetUnmoveable          :  1; // [in] When set to 1, will set Unmoveable to new value
            UINT Reserved               : 28;
        };
        UINT PropertyMaskValue;
    };
} D3DDDI_UPDATEALLOCPROPERTY;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if(D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

typedef struct D3DDDI_OFFER_FLAGS
{
    union
    {
        struct
        {
            UINT AllowDecommit :  1;
            UINT Reserved      : 31;
        };
        UINT Value;
    };
} D3DDDI_OFFER_FLAGS;

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

#if(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1 || \
    D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1)

typedef enum _D3DDDI_RECLAIM_RESULT
{
    D3DDDI_RECLAIM_RESULT_OK = 0,
    D3DDDI_RECLAIM_RESULT_DISCARDED = 1,
    D3DDDI_RECLAIM_RESULT_NOT_COMMITTED = 2,
} D3DDDI_RECLAIM_RESULT;

#endif

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECTINFO
{
    D3DDDI_SYNCHRONIZATIONOBJECT_TYPE    Type;      // in: Type of synchronization object to create.
    union
    {
        struct
        {
            BOOL InitialState;                      // in: Initial state of a synchronization mutex.
        } SynchronizationMutex;

        struct
        {
            UINT MaxCount;                          // in: Max count of the semaphore.
            UINT InitialCount;                      // in: Initial count of the semaphore.
        } Semaphore;


        struct
        {
            UINT Reserved[16];                      // Reserved for future use.
        } Reserved;
    };
} D3DDDI_SYNCHRONIZATIONOBJECTINFO;

#ifndef D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_EXT
#define D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_EXT
#define D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_RESERVED0        Reserved0
#endif

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS
{
    union
    {
        struct
        {
            UINT Shared                                         :  1;
            UINT NtSecuritySharing                              :  1;
#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3)) // M1

            UINT CrossAdapter                                   :  1;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))

            // When set, the sync object is signaled as soon as the contents of command buffers preceding it
            // is entirely copied to the GPU pipeline, but not necessarily completed execution.
            // This flag can be set in order to start reusing command buffers as soon as possible.
            UINT TopOfPipeline                                  :  1;

            // When set, the device this sync object is created or opened on
            // can only submit wait commands for it.
            UINT NoSignal                                       :  1;

            // When set, the device this sync object is created or opened on
            // can only submit signal commands for it. This flag cannot be set
            // simultaneously with NoSignal.
            UINT NoWait                                         :  1;

            // When set, instructs the GPU scheduler to bypass signaling of the monitored fence
            // to the maximum value when the device is affected by the GPU reset.
            UINT NoSignalMaxValueOnTdr                          :  1;

            // When set, the fence will not be mapped into the GPU virtual address space.
            // Only packet-based signal/wait operations are supported
            // When this is set, the fence is always stored as a 64-bit value (regardless of adapter caps)
            UINT NoGPUAccess                                    :  1;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
            // When set, the fence can be signaled by KMD.
            // The flag can be used only with D3DDDI_CPU_NOTIFICATION objects.
            UINT SignalByKmd                                    :  1;
            UINT Reserved                                       : 22;
#else
            UINT Reserved                                       : 23;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)

#else
            UINT Reserved                                       : 28;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#else
            UINT Reserved                                       : 29;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
            UINT D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS_RESERVED0   :  1;
        };
        UINT Value;
    };
} D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS;

typedef struct _D3DDDI_SYNCHRONIZATIONOBJECTINFO2
{
    D3DDDI_SYNCHRONIZATIONOBJECT_TYPE    Type;      // in: Type of synchronization object to create.
    D3DDDI_SYNCHRONIZATIONOBJECT_FLAGS   Flags;     // in: flags.
    union
    {
        struct
        {
            BOOL InitialState;                      // in: Initial state of a synchronization mutex.
        } SynchronizationMutex;

        struct
        {
            UINT MaxCount;                          // in: Max count of the semaphore.
            UINT InitialCount;                      // in: Initial count of the semaphore.
        } Semaphore;

        struct
        {
            D3DKMT_ALIGN64 UINT64 FenceValue;       // in: inital fence value.
        } Fence;

        struct
        {
            D3DKMT_PTR(HANDLE, Event);                           // in: Handle to the event
        } CPUNotification;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))
        struct
        {
            D3DKMT_ALIGN64 UINT64   InitialFenceValue;                      // in: inital fence value.
            D3DKMT_PTR(VOID*,       FenceValueCPUVirtualAddress);           // out: Read-only mapping of the fence value for the CPU
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress; // out: Read/write mapping of the fence value for the GPU
            UINT                    EngineAffinity;                         // in: Defines physical adapters where the GPU VA will be mapped
            UINT                    Padding;
        } MonitoredFence;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2))
        struct
        {
            D3DKMT_HANDLE                   hAdapter;                               // in: A handle to the adapter associated with VidPnTargetId
            D3DDDI_VIDEO_PRESENT_TARGET_ID  VidPnTargetId;                          // in: The output that the compositor wishes to receive notifications for
            D3DKMT_ALIGN64 UINT64           Time;                                   // in: Represents an offset before the VSync.
                                                                                    // The Time value may not be longer than a VSync interval. In units of 100ns.
            D3DKMT_PTR(VOID*,               FenceValueCPUVirtualAddress);           // out: Read-only mapping of the fence value for the CPU
            D3DKMT_ALIGN64 D3DGPU_VIRTUAL_ADDRESS FenceValueGPUVirtualAddress;      // out: Read-only mapping of the fence value for the GPU
            UINT                            EngineAffinity;                         // in: Defines physical adapters where the GPU VA will be mapped
            UINT                            Padding;
        } PeriodicMonitoredFence;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)


        struct
        {
            D3DKMT_ALIGN64 UINT64 Reserved[8];                     // Reserved for future use.
        } Reserved;
    };

    D3DKMT_HANDLE  SharedHandle;                    // out: global shared handle (when requested to be shared)

} D3DDDI_SYNCHRONIZATIONOBJECTINFO2;

#if ((DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0) || \
     (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0))

typedef struct _D3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPU_FLAGS
{
    union
    {
        struct
        {
            UINT WaitAny        : 1;    // when waiting for multiple objects, signal the wait event if any
                                        // of the wait array conditions is satisfied as opposed to all conditions.
            UINT Reserved       : 31;
        };
        UINT Value;
    };
} D3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPU_FLAGS;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

typedef struct _D3DDDI_QUERYREGISTRY_FLAGS
{
    union
    {
        struct
        {
            UINT   TranslatePath    :  1;
            UINT   MutableValue     :  1;
            UINT   Reserved         : 30;
        };
        UINT Value;
    };
} D3DDDI_QUERYREGISTRY_FLAGS;

typedef enum _D3DDDI_QUERYREGISTRY_TYPE
{
   D3DDDI_QUERYREGISTRY_SERVICEKEY      = 0,
   D3DDDI_QUERYREGISTRY_ADAPTERKEY      = 1,
   D3DDDI_QUERYREGISTRY_DRIVERSTOREPATH = 2,
   D3DDDI_QUERYREGISTRY_DRIVERIMAGEPATH = 3,
   D3DDDI_QUERYREGISTRY_MAX,
} D3DDDI_QUERYREGISTRY_TYPE;

typedef enum _D3DDDI_QUERYREGISTRY_STATUS
{
   D3DDDI_QUERYREGISTRY_STATUS_SUCCESS              = 0,
   D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW      = 1,
   D3DDDI_QUERYREGISTRY_STATUS_FAIL                 = 2,
   D3DDDI_QUERYREGISTRY_STATUS_MAX,
} D3DDDI_QUERYREGISTRY_STATUS;

//
// Output data value follows this structure.
// PrivateDriverSize must be sizeof(D3DDDI_QUERYREGISTRY_INFO) + (size of the the key value in bytes)
//
typedef struct _D3DDDI_QUERYREGISTRY_INFO
{
   D3DDDI_QUERYREGISTRY_TYPE    QueryType;              // In
   D3DDDI_QUERYREGISTRY_FLAGS   QueryFlags;             // In
   WCHAR                        ValueName[MAX_PATH];    // In
   ULONG                        ValueType;              // In
   ULONG                        PhysicalAdapterIndex;   // In
   ULONG                        OutputValueSize;        // Out. Number of bytes written to the output value or required in case of D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW.
   D3DDDI_QUERYREGISTRY_STATUS  Status;                 // Out
   union {
        DWORD   OutputDword;                            // Out
        D3DKMT_ALIGN64 UINT64  OutputQword;             // Out
        WCHAR   OutputString[1];                        // Out. Dynamic array
        BYTE    OutputBinary[1];                        // Out. Dynamic array
   };
 } D3DDDI_QUERYREGISTRY_INFO;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

//
// Defines the maximum number of context a particular command buffer can
// be broadcast to.
//
#define D3DDDI_MAX_BROADCAST_CONTEXT        64

//
// Allocation priorities.
//
#define D3DDDI_ALLOCATIONPRIORITY_MINIMUM       0x28000000
#define D3DDDI_ALLOCATIONPRIORITY_LOW           0x50000000
#define D3DDDI_ALLOCATIONPRIORITY_NORMAL        0x78000000
#define D3DDDI_ALLOCATIONPRIORITY_HIGH          0xa0000000
#define D3DDDI_ALLOCATIONPRIORITY_MAXIMUM       0xc8000000

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

// Cross adapter resource pitch alignment in bytes.
// Must be power of 2.
//
#define D3DKMT_CROSS_ADAPTER_RESOURCE_PITCH_ALIGNMENT 128

// Cross adapter resource height alignment in rows.
//
#define D3DKMT_CROSS_ADAPTER_RESOURCE_HEIGHT_ALIGNMENT 4

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#endif // (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)

#pragma warning(pop)


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* _D3DUKMDT_H_ */

