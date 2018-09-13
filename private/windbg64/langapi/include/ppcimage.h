/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppcimage.h

Abstract:

    This is the include file that describes ppc-specific image info

Author:

    James Stulz (v-james)  July 1993

Revision History:

--*/

#ifndef _PPCIMAGE_
#define _PPCIMAGE_

#define IMAGE_FILE_MACHINE_MPPC_601         0x601   // PowerPC 601.
#define IMAGE_FILE_MPPC_DLL                 0x4000

//
// Power Macintosh relocation types
//

#define IMAGE_REL_MPPC_DESCREL              0x0000
#define IMAGE_REL_MPPC_LCALL                0x0001
#define IMAGE_REL_MPPC_DATAREL              0x0002
#define IMAGE_REL_MPPC_JMPADDR              0x0003
#define IMAGE_REL_MPPC_CREATEDESCRREL       0x0004
#define IMAGE_REL_MPPC_DATADESCRREL         0x0005
#define IMAGE_REL_MPPC_TOCREL               0x0006
#define IMAGE_REL_MPPC_SECTION              0x000A
#define IMAGE_REL_MPPC_SECREL               0x000B
#define IMAGE_REL_MPPC_ADDR24               0x000C // 26-bit address, shifted left 2 (branch absolute)
#define IMAGE_REL_MPPC_ADDR14               0x000D // 16-bit address, shifted left 2 (load doubleword)
#define IMAGE_REL_MPPC_REL24                0x000E // 26-bit PC-relative offset, shifted left 2 (branch relative)
#define IMAGE_REL_MPPC_REL14                0x000F // 16-bit PC-relative offset, shifted left 2 (br cond relative)
#define IMAGE_REL_MPPC_CV                   0x0013
#define IMAGE_REL_MPPC_TOCINDIRCALL         0x0022
#define IMAGE_REL_MPPC_TOCCALLREL           0x0025
#define IMAGE_REL_MPPC_PCODECALL            0x0028
#define IMAGE_REL_MPPC_PCODECALLTONATIVE    0x0029
#define IMAGE_REL_MPPC_PCODENEPE            0x002A

#endif // _PPCIMAGE_
