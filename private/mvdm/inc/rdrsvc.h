/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rdrsvc.h

Abstract:

    Contains BOP codes for Vdm Redir (Vr) BOP dispatcher

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Revision History:

    13-Sep-1991 rfirth
        Created

--*/



/* ASM
include bop.inc

SVC     macro   SvcNum
        BOP     BOP_REDIR
        db      SvcNum
endm

*/



//
// Note: the order has no bearing on the order of the 5f dispatch table or
// vice versa. However, the order must be contiguous
//

#define SVC_RDRINITIALIZE       0x00    // redir loaded
#define SVC_RDRUNINITIALIZE     0x01    // redir unloaded
#define SVC_RDRQNMPIPEINFO      0x02    // DosQNmPipeInfo
#define SVC_RDRQNMPHANDSTATE    0x03    // DosQNmpHandState
#define SVC_RDRSETNMPHANDSTATE  0x04    // DosSetNmpHandState
#define SVC_RDRPEEKNMPIPE       0x05    // DosPeekNmPipe
#define SVC_RDRTRANSACTNMPIPE   0x06    // DosTransactNmPipe
#define SVC_RDRCALLNMPIPE       0x07    // DosCallNmPipe
#define SVC_RDRWAITNMPIPE       0x08    // DosWaitNmPipe
#define SVC_RDRDELETEMAILSLOT   0x09    // DosDeleteMailslot
#define SVC_RDRGETMAILSLOTINFO  0x0a    // DosMailslotInfo
#define SVC_RDRMAKEMAILSLOT     0x0b    // DosMakeMailslot
#define SVC_RDRPEEKMAILSLOT     0x0c    // DosPeekMailslot
#define SVC_RDRREADMAILSLOT     0x0d    // DosReadMailslot
#define SVC_RDRWRITEMAILSLOT    0x0e    // DosWriteMailslot
#define SVC_RDRTERMINATE        0x0f    // NetResetEnvironment for mailslots
#define SVC_RDRTRANSACTAPI      0x10    // NetTransactAPI
#define SVC_RDRIREMOTEAPI       0x11    // NetIRemoteAPI
#define SVC_RDRNULLTRANSACTAPI  0x12    // NetTransactAPI
#define SVC_RDRSERVERENUM       0x13    // NetServerEnum (remoted)
#define SVC_RDRUSEADD           0x14    // NetUseAdd (local)
#define SVC_RDRUSEDEL           0x15    // NetUseDel (local)
#define SVC_RDRUSEENUM          0x16    // NetUseEnum (local)
#define SVC_RDRUSEGETINFO       0x17    // NetUseGetInfo (local)
#define SVC_RDRWKSTAGETINFO     0x18    // NetWkstaGetInfo (local)
#define SVC_RDRWKSTASETINFO     0x19    // NetWkstaSetInfo (local)
#define SVC_RDRMESSAGEBUFFERSEND 0x1a    // NetMessageBufferSend (local)
#define SVC_RDRGETCDNAMES       0x1b    // NetGetEnumInfo.CDNames
#define SVC_RDRGETCOMPUTERNAME  0x1c    // NetGetEnumInfo.ComputerName
#define SVC_RDRGETUSERNAME      0x1d    // NetGetEnumInfo.UserName
#define SVC_RDRGETDOMAINNAME    0x1e    // NetGetEnumInfo.DomainName
#define SVC_RDRGETLOGONSERVER   0x1f    // NetGetEnumInfo.LogonServer
#define SVC_RDRHANDLEGETINFO    0x20    // NetHandleGetInfo
#define SVC_RDRHANDLESETINFO    0x21    // NetHandleSetInfo
#define SVC_RDRGETDCNAME        0x22    // NetGetDCName
#define SVC_RDRREADASYNCNMPIPE  0x23    // DosReadAsyncNmPipe
#define SVC_RDRWRITEASYNCNMPIPE 0x24    // DosWriteAsyncNmPipe
#define SVC_NETBIOS5C           0x25    // Netbios request handler
#define SVC_NETBIOS5CINTERRUPT  0x26    // Netbios/Dlc post routine request
#define SVC_DLC_5C              0x27    // Dlc request handler
#define SVC_VDM_WINDOW_INIT     0x28    // Inits memory window
#define SVC_RDRRETURN_MODE      0x29    // returns pause/continue state
#define SVC_RDRSET_MODE         0x2a    // sets pause/continue state
#define SVC_RDRGET_ASG_LIST     0x2b    // old NetUseGetInfo
#define SVC_RDRDEFINE_MACRO     0x2c    // old NetUseAdd
#define SVC_RDRBREAK_MACRO      0x2d    // old NetUseDel
#define SVC_RDRSERVICECONTROL   0x2e    // NetServiceControl
#define SVC_RDRINTACK           0x2f    // VrDismissInterrupt
#define SVC_RDRINTACK2          0x30    // VrDismissInterrupt2
#define SVC_NETBIOSCHECK        0x31    // VrCheckPmNetbiosAnr

#define MAX_REDIR_SVC           SVC_NETBIOSCHECK
