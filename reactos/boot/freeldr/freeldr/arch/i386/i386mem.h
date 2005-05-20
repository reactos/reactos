#ifndef I386MEM_H_INCLUDED
#define I386MEM_H_INCLUDED

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDN_SHIFT 22


/* Converts a Phsyical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Phsyical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

typedef struct _HARDWARE_PTE_X86 {
    union {
        struct {
            ULONG Valid             : 1;
            ULONG Write             : 1;
            ULONG Owner             : 1;
            ULONG WriteThrough      : 1;
            ULONG CacheDisable      : 1;
            ULONG Accessed          : 1;
            ULONG Dirty             : 1;
            ULONG LargePage         : 1;
            ULONG Global            : 1;
            ULONG CopyOnWrite       : 1;
            ULONG Prototype         : 1;
            ULONG reserved          : 1;
            ULONG PageFrameNumber   : 20;
        };
        ULONG Val;
    };
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86, HARDWARE_PDE_X86, *PHARDWARE_PDE_X86;

typedef struct _HARDWARE_PTE_X64 {
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
    ULONG reserved2         : 31;
    ULONG NoExecute         : 1;
} HARDWARE_PTE_X64, *PHARDWARE_PTE_X64;

#define PTRS_PER_PD_X86   (PAGE_SIZE / sizeof(HARDWARE_PDE_X86))
#define PTRS_PER_PT_X86   (PAGE_SIZE / sizeof(HARDWARE_PTE_X86))

/* Page Directory Index of a given virtual address */
#define PD_IDX(Va) ((((ULONG_PTR) Va) >> PDN_SHIFT) & (PTRS_PER_PD_X86 - 1))
/* Page Table Index of a give virtual address */
#define PT_IDX(Va) ((((ULONG_PTR) Va) >> PFN_SHIFT) & (PTRS_PER_PT_X86 - 1))
/* Convert a Page Directory or Page Table entry to a (machine) address */
#define PAGE_MASK  (~(PAGE_SIZE-1))

typedef struct _PAGE_DIRECTORY_X86 {
    HARDWARE_PDE_X86 Pde[PTRS_PER_PD_X86];
} PAGE_DIRECTORY_X86, *PPAGE_DIRECTORY_X86;

typedef struct _PAGE_TABLE_X86 {
    HARDWARE_PTE_X86 Pte[PTRS_PER_PT_X86];
} PAGE_TABLE_X86, *PPAGE_TABLE_X86;

typedef struct _PAGE_DIRECTORY_X64 {
    HARDWARE_PTE_X64 Pde[2048];
} PAGE_DIRECTORY_X64, *PPAGE_DIRECTORY_X64;

typedef struct _PAGE_DIRECTORY_TABLE_X64 {
    HARDWARE_PTE_X64 Pde[4];
} PAGE_DIRECTORY_TABLE_X64, *PPAGE_DIRECTORY_TABLE_X64;

#endif /* ! defined I386MEM_H_INCLUDED */
