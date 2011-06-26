

#include <asm.inc>
#include <arch/pc/x86common.h>

.code64

PUBLIC RealEntryPoint
RealEntryPoint:

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

nop


END
