

#include <asm.inc>
#include <arch/pc/x86common.h>

EXTERN BootMain:PROC

.code64

PUBLIC RealEntryPoint
RealEntryPoint:
    //mov ax, LMODE_DS
    //mov ds, ax
    //mov word ptr [HEX(b8000)], HEX(0e00) + '1'

    /* GO! */
    xor rcx, rcx
    call BootMain


PUBLIC FrldrBootDrive
FrldrBootDrive:

PUBLIC FrldrBootPartition
FrldrBootPartition:

PUBLIC PageDirectoryEnd
PageDirectoryEnd:
PUBLIC PageDirectoryStart
PageDirectoryStart:
PUBLIC PnpBiosGetDeviceNode
PnpBiosGetDeviceNode:
PUBLIC PnpBiosGetDeviceNodeCount
PnpBiosGetDeviceNodeCount:
PUBLIC PnpBiosSupported
PnpBiosSupported:
PUBLIC Int386
Int386:
PUBLIC PxeCallApi
PxeCallApi:
PUBLIC __lgdt
__lgdt:
PUBLIC __ltr
__ltr:

nop


END
