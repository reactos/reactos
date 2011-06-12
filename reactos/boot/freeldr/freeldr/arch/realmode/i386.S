
#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16

.code16

/* fat helper code */
#include "fathelp.inc"

.org 512
RealEntryPoint:

    /* Get address of optional header */
    mov eax, dword ptr ds:[FREELDR_PE_BASE + IMAGE_DOS_HEADER_e_lfanew]
    add eax, FREELDR_PE_BASE + 4 + IMAGE_FILE_HEADER_SIZE

    /* Jump to address of entry point */
    mov eax, dword ptr ds:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE
    jmp ax


#include "helpers.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE)
.endcode16

END
