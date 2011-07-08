

#include <asm.inc>

EXTERN BootMain:PROC

.code64

/* 64 bit entry point */
PUBLIC mainCRTStartup
mainCRTStartup:
    jmp BootMain

PUBLIC Int386
Int386:
    ret

PUBLIC __lgdt
__lgdt:

PUBLIC __ltr
__ltr:

PUBLIC PxeCallApi
PxeCallApi:
    .long 0

PUBLIC PageDirectoryStart
PageDirectoryStart:
    .long 0

PUBLIC PageDirectoryEnd
PageDirectoryEnd:
    .long 0

PUBLIC PnpBiosGetDeviceNode
PnpBiosGetDeviceNode:
    .long 0

PUBLIC PnpBiosGetDeviceNodeCount
PnpBiosGetDeviceNodeCount:
    .long 0

PUBLIC PnpBiosSupported
PnpBiosSupported:
    .long 0

END
