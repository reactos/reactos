
#define MAX_IDENTITYMAP_ALLOCATIONS 30

typedef struct _IDENTITY_MAP  {
    PHARDWARE_PTE   TopLevelDirectory;
    ULONG           IdentityCR3;
    ULONG           IdentityAddr;
    ULONG           PagesAllocated;
    PVOID           PageList[ MAX_IDENTITYMAP_ALLOCATIONS ];
} IDENTITY_MAP, *PIDENTITY_MAP;


VOID
Ki386ClearIdentityMap(
    PIDENTITY_MAP IdentityMap
    );

VOID
Ki386EnableTargetLargePage(
    PIDENTITY_MAP IdentityMap
    );

BOOLEAN
Ki386CreateIdentityMap(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PVOID StartVa,
    IN     PVOID EndVa
    );

BOOLEAN
Ki386EnableCurrentLargePage (
    IN ULONG IdentityAddr,
    IN ULONG IdentityCr3
    );

extern PVOID Ki386EnableCurrentLargePageEnd;

#if defined(_X86PAE_)
#define PPI_BITS    2
#define PDI_BITS    9
#define PTI_BITS    9
#else
#define PPI_BITS    0
#define PDI_BITS    10
#define PTI_BITS    10
#endif

#define PPI_MASK    ((1 << PPI_BITS) - 1)
#define PDI_MASK    ((1 << PDI_BITS) - 1)
#define PTI_MASK    ((1 << PTI_BITS) - 1)

#define KiGetPpeIndex(va) ((((ULONG)(va)) >> PPI_SHIFT) & PPI_MASK)
#define KiGetPdeIndex(va) ((((ULONG)(va)) >> PDI_SHIFT) & PDI_MASK)
#define KiGetPteIndex(va) ((((ULONG)(va)) >> PTI_SHIFT) & PTI_MASK)
