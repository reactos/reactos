#ifndef I386BOOT_H_INCLUDED
#define I386BOOT_H_INCLUDED

#define KernelEntryPoint            (KernelEntry - KERNEL_BASE_PHYS) + KernelBase

/* Unrelocated Kernel Base in Virtual Memory */
extern ULONG_PTR KernelBase;

/* Kernel Entrypoint in Physical Memory */
extern ULONG_PTR KernelEntry;

/* Page Directory and Tables for non-PAE Systems */
extern ULONG_PTR startup_pagedirectory;
extern ULONG_PTR lowmem_pagetable;
extern ULONG_PTR kernel_pagetable;
extern ULONG_PTR hyperspace_pagetable;
extern ULONG_PTR _pae_pagedirtable;
extern ULONG_PTR apic_pagetable;
extern ULONG_PTR kpcr_pagetable;

/* Page Directory and Tables for PAE Systems */
extern ULONG_PTR startup_pagedirectorytable_pae;
extern ULONG_PTR startup_pagedirectory_pae;
extern ULONG_PTR lowmem_pagetable_pae;
extern ULONG_PTR kernel_pagetable_pae;
extern ULONG_PTR hyperspace_pagetable_pae;
extern ULONG_PTR pagedirtable_pae;
extern ULONG_PTR apic_pagetable_pae;
extern ULONG_PTR kpcr_pagetable_pae;

VOID
STDCALL
i386BootStartup(ULONG Magic);

VOID
FASTCALL
i386BootSetupPae(BOOLEAN PaeModeEnabled, ULONG Magic);

BOOLEAN
FASTCALL
i386BootGetPaeMode(VOID);

VOID
FASTCALL
i386BootSetupPageDirectory(BOOLEAN PaeModeEnabled, BOOLEAN SetupApic,
                           ULONG (STDCALL *AddrToPfn)(ULONG_PTR Addr));

#endif /* ! defined I386BOOT_H_INCLUDED */
