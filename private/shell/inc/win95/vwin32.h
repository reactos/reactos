/******************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp.  All Rights Reserved, 1989-1995
 *
 *   Title: vwin32.h -
 *
 *   Version:   4.00
 *
 *   Date:  24-May-1993
 *
 ******************************************************************************/

/*INT32*/

#ifndef _VWIN32_H_
#define _VWIN32_H_

// ;BeginInternal
// Note that this ID has been reserved for us in VMM.H

#define VWIN32_DEVICE_ID    0x0002A

#define VWIN32_VER_MAJOR    1
#define VWIN32_VER_MINOR    4

#define THREAD_TYPE_WIN32 VWIN32_DEVICE_ID

// ;EndInternal

#ifndef Not_VxD

/*XLATOFF*/
#define VWIN32_Service  Declare_Service
#define VWIN32_StdCall_Service Declare_SCService
#pragma warning (disable:4003)      // turn off not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VWIN32)

VWIN32_Service  (VWIN32_Get_Version, LOCAL)
VWIN32_Service  (VWIN32_DIOCCompletionRoutine, LOCAL)
VWIN32_Service  (_VWIN32_QueueUserApc)
VWIN32_Service  (_VWIN32_Get_Thread_Context)
VWIN32_Service  (_VWIN32_Set_Thread_Context)
VWIN32_Service  (_VWIN32_CopyMem, LOCAL)
VWIN32_Service  (_VWIN32_Npx_Exception)
VWIN32_Service  (_VWIN32_Emulate_Npx)
VWIN32_Service  (_VWIN32_CheckDelayedNpxTrap)
VWIN32_Service  (VWIN32_EnterCrstR0)
VWIN32_Service  (VWIN32_LeaveCrstR0)
VWIN32_Service  (_VWIN32_FaultPopup)
VWIN32_Service  (VWIN32_GetContextHandle)
VWIN32_Service  (VWIN32_GetCurrentProcessHandle, LOCAL)
VWIN32_Service  (_VWIN32_SetWin32Event)
VWIN32_Service  (_VWIN32_PulseWin32Event)
VWIN32_Service  (_VWIN32_ResetWin32Event)
VWIN32_Service  (_VWIN32_WaitSingleObject)
VWIN32_Service  (_VWIN32_WaitMultipleObjects)
VWIN32_Service  (_VWIN32_CreateRing0Thread)
VWIN32_Service  (_VWIN32_CloseVxDHandle)
VWIN32_Service  (VWIN32_ActiveTimeBiasSet, LOCAL)
VWIN32_Service  (VWIN32_GetCurrentDirectory, LOCAL)
VWIN32_Service  (VWIN32_BlueScreenPopup)
VWIN32_Service  (VWIN32_TerminateApp)
VWIN32_Service  (_VWIN32_QueueKernelAPC)
VWIN32_Service  (VWIN32_SysErrorBox)
VWIN32_Service  (_VWIN32_IsClientWin32)
VWIN32_Service  (VWIN32_IFSRIPWhenLev2Taken, LOCAL)
VWIN32_Service  (_VWIN32_InitWin32Event)
VWIN32_Service  (_VWIN32_InitWin32Mutex)
VWIN32_Service  (_VWIN32_ReleaseWin32Mutex)
VWIN32_Service  (_VWIN32_BlockThreadEx)
VWIN32_Service  (VWIN32_GetProcessHandle, LOCAL)
VWIN32_Service  (_VWIN32_InitWin32Semaphore)
VWIN32_Service  (_VWIN32_SignalWin32Sem)
VWIN32_Service  (_VWIN32_QueueUserApcEx)
VWIN32_Service	(_VWIN32_OpenVxDHandle)
VWIN32_Service	(_VWIN32_CloseWin32Handle)
VWIN32_Service	(_VWIN32_AllocExternalHandle)
VWIN32_Service	(_VWIN32_UseExternalHandle)
VWIN32_Service	(_VWIN32_UnuseExternalHandle)
VWIN32_StdCall_Service	(KeInitializeTimer, 1)
VWIN32_StdCall_Service	(KeSetTimer, 4)
VWIN32_StdCall_Service	(KeCancelTimer, 1)
VWIN32_StdCall_Service	(KeReadStateTimer, 1)
VWIN32_Service	(_VWIN32_ReferenceObject)
VWIN32_Service	(_VWIN32_GetExternalHandle)

End_Service_Table(VWIN32)
/*ENDMACROS*/

/*XLATOFF*/
#pragma warning (default:4003)      // turn on not enough params warning
/*XLATON*/

#endif // Not_VxD

// ;BeginInternal

// PM API list

#define VWIN32_GET_VER      0
#define VWIN32_THREAD_SWITCH    1   // ECX = wake param, EBX = ring 0 handle
#define VWIN32_DPMI_FAULT   2   // SS:BP = FAULTSTACKFRAME, AL = ignore
#define VWIN32_MMGR_FUNCTIONS   3
#define VWIN32_EVENT_CREATE 4
#define VWIN32_EVENT_DESTROY    5
#define VWIN32_EVENT_WAIT   6
#define VWIN32_EVENT_SET    7
#define VWIN32_RealNetx_Info    8
#define VWIN32_THREAD_BOOST_PRI 9
#define VWIN32_WAIT_CRST    10
#define VWIN32_WAKE_CRST    11
#define VWIN32_SET_FAULT_INFO   12
#define VWIN32_EXIT_TIME    13
#define VWIN32_BOOST_THREAD_GROUP 14
#define VWIN32_BOOST_THREAD_STATIC 15
#define VWIN32_WAKE_IDLE_SYS    16
#define VWIN32_MAKE_IDLE_SYS    17
#define VWIN32_DELIVER_PENDING_KERNEL_APCS 18
#define VWIN32_SYS_ERROR_BOX    19
#define VWIN32_GET_IFSMGR_XLAT_PTR 20
#define VWIN32_BOOST_THREAD_DECAY 21
#define VWIN32_LAST_CMD     21

#define VWIN32_MMGR_RESERVE ((VWIN32_MMGR_FUNCTIONS << 8) + 0)
#define VWIN32_MMGR_COMMIT  ((VWIN32_MMGR_FUNCTIONS << 8) + 1)
#define VWIN32_MMGR_DECOMMIT    ((VWIN32_MMGR_FUNCTIONS << 8) + 2)
#define VWIN32_MMGR_PAGEFREE    ((VWIN32_MMGR_FUNCTIONS << 8) + 3)

//
// Current Win32 thread/process handles.
//
// Updated every context switch.
//

typedef struct _K32CURRENT {
    DWORD   CurThreadHandle;    // win32 thread handle
    DWORD   CurProcessHandle;   // win32 process handle
    DWORD   CurTDBX;        // current TDBX
    DWORD   pCurK16Task;        // flat pointer to kernel 16 CurTDB
    DWORD   CurContextHandle;   // win32 memory context handle
} K32CURRENT;

//
// Flag values for CreateThread
//
#define VWIN32_CT_EMULATE_NPX   0x01    // set EM bit in CR0 for thread
#define VWIN32_CT_WIN32_NPX 0x02    // use Win32 FP exception model
#define VWIN32_CT_WIN32     0x04    // thread is Win32 (not Win16)

//
// Return values from VWIN32_CheckDelayedNpxTrap
//
#define CX_RAISE    0       // instruction raises exception
#define CX_IGNORE   1       // instruction ignores exception
#define CX_CLEAR    2       // instruction clears or masks exception

// flags to use for win32 blocking
#define WIN32_BLOCK_FLAGS (BLOCK_FORCE_SVC_INTS+BLOCK_SVC_INTS+BLOCK_THREAD_IDLE+BLOCK_ENABLE_INTS)

//
// Flags for VWIN32_BlueScreenPopup
//

#define VBSP_CANCEL     0x00000001
#define VBSP_DISPLAY_VXD_NAME   0x00000002

//
// Fault stack frame structure
//

typedef struct fsf_s {
    WORD fsf_GS;
    WORD fsf_FS;
    WORD fsf_ES;
    WORD fsf_DS;
    DWORD fsf_EDI;
    DWORD fsf_ESI;
    DWORD fsf_EBP;
    DWORD fsf_locked_ESP;
    DWORD fsf_EBX;
    DWORD fsf_EDX;
    DWORD fsf_ECX;
    DWORD fsf_EAX;
    WORD fsf_num;       // Fault number
    WORD fsf_prev_IP;   // IP of previous fault handler
    WORD fsf_prev_CS;   // CS of previous fault handler
    WORD fsf_ret_IP;    // DPMI fault handler frame follows
    WORD fsf_ret_CS;
    WORD fsf_err_code;
    WORD fsf_faulting_IP;
    WORD fsf_faulting_CS;
    WORD fsf_flags;
    WORD fsf_SP;
    WORD fsf_SS;
} FAULTSTACKFRAME;

typedef FAULTSTACKFRAME *PFAULTSTACKFRAME;

// ;EndInternal

//
// structure for VWIN32_SysErrorBox
//

typedef struct vseb_s {
    DWORD vseb_resp;
    WORD vseb_b3;
    WORD vseb_b2;
    WORD vseb_b1;
    DWORD vseb_pszCaption;
    DWORD vseb_pszText;
} VSEB;

typedef VSEB *PVSEB;

#define SEB_ANSI    0x4000      // ANSI strings if set on vseb_b1
#define SEB_TERMINATE   0x2000      // forces termination if button pressed

// VWIN32_QueueKernelAPC flags

#define KERNEL_APC_IGNORE_MC        0x00000001
#define KERNEL_APC_STATIC       0x00000002
#define KERNEL_APC_WAKE         0x00000004

// for DeviceIOControl support
// On a DeviceIOControl call vWin32 will pass following parameters to
// the Vxd that is specified by hDevice. hDevice is obtained thru an
// earlier call to hDevice = CreateFile("\\.\vxdname", ...);
// ESI = DIOCParams STRUCT (defined below)
typedef struct DIOCParams   {
    DWORD   Internal1;      // ptr to client regs
    DWORD   VMHandle;       // VM handle
    DWORD   Internal2;      // DDB
    DWORD   dwIoControlCode;
    DWORD   lpvInBuffer;
    DWORD   cbInBuffer;
    DWORD   lpvOutBuffer;
    DWORD   cbOutBuffer;
    DWORD   lpcbBytesReturned;
    DWORD   lpoOverlapped;
    DWORD   hDevice;
    DWORD   tagProcess;
} DIOCPARAMETERS;

typedef DIOCPARAMETERS *PDIOCPARAMETERS;

// dwIoControlCode values for vwin32's DeviceIOControl Interface
// all VWIN32_DIOC_DOS_ calls require lpvInBuffer abd lpvOutBuffer to be
// struct * DIOCRegs
#define VWIN32_DIOC_GETVERSION DIOC_GETVERSION
#define VWIN32_DIOC_DOS_IOCTL       1
#define VWIN32_DIOC_DOS_INT25       2
#define VWIN32_DIOC_DOS_INT26       3
#define VWIN32_DIOC_DOS_INT13       4
#define VWIN32_DIOC_SIMCTRLC        5
#define VWIN32_DIOC_DOS_DRIVEINFO   6
#define VWIN32_DIOC_CLOSEHANDLE DIOC_CLOSEHANDLE

// DIOCRegs
// Structure with i386 registers for making DOS_IOCTLS
// vwin32 DIOC handler interprets lpvInBuffer , lpvOutBuffer to be this struc.
// and does the int 21
// reg_flags is valid only for lpvOutBuffer->reg_Flags
typedef struct DIOCRegs {
    DWORD   reg_EBX;
    DWORD   reg_EDX;
    DWORD   reg_ECX;
    DWORD   reg_EAX;
    DWORD   reg_EDI;
    DWORD   reg_ESI;
    DWORD   reg_Flags;      
} DIOC_REGISTERS;

// if we are not included along with winbase.h
#ifndef FILE_FLAG_OVERLAPPED
  // OVERLAPPED structure for DeviceIOCtl VxDs
  typedef struct _OVERLAPPED {
          DWORD O_Internal;
          DWORD O_InternalHigh;
          DWORD O_Offset;
          DWORD O_OffsetHigh;
          HANDLE O_hEvent;
  } OVERLAPPED;
#endif

//  Parameters for _VWIN32_OpenVxDHandle to validate the Win32 handle type.
#define OPENVXD_TYPE_SEMAPHORE  0
#define OPENVXD_TYPE_EVENT      1
#define OPENVXD_TYPE_MUTEX      2
#define	OPENVXD_TYPE_ANY	3
  
// ;BeginInternal
#define OPENVXD_TYPE_MAXIMUM    3
// ;EndInternal

//
//  Object type table declaration for _VWIN32_AllocExternalHandle
//
/*XLATOFF*/
#define R0OBJCALLBACK           __stdcall
typedef VOID    (R0OBJCALLBACK *R0OBJFREE)(PVOID pR0ObjBody);
typedef PVOID   (R0OBJCALLBACK *R0OBJDUP)(PVOID pR0ObjBody, DWORD hDestProc);
/*XLATON*/
/* ASM
R0OBJFREE   TYPEDEF     DWORD
R0OBJDUP    TYPEDEF     DWORD
*/

typedef struct _R0OBJTYPETABLE {
    DWORD       ott_dwSize;             //  sizeof(R0OBJTYPETABLE)
    R0OBJFREE   ott_pfnFree;            //  called by Win32 CloseHandle
    R0OBJDUP    ott_pfnDup;             //  called by Win32 DuplicateHandle
} R0OBJTYPETABLE, *PR0OBJTYPETABLE;
/* ASM
R0OBJTYPETABLE  typedef _R0OBJTYPETABLE;
*/

#define R0EHF_INHERIT   0x00000001      //  Handle is inheritable
#define R0EHF_GLOBAL    0x00000002      //  Handle is valid in all contexts
// ;BeginInternal
#define R0EHF_ALL       (R0EHF_INHERIT | R0EHF_GLOBAL)
// ;EndInternal

// ;BeginInternal

/* ASM

FSF_CLEANUP_RETURN  EQU fsf_ret_IP - fsf_num
FSF_CLEANUP_CHAIN   EQU fsf_prev_IP - fsf_num

K32CURRENT typedef _K32CURRENT

;***LT  W32Fun - macro to make a function callable from Kernel32
;
;   This macro will create a stub of the format that the Ring0/Ring3
;   Win32 calling interface likes.
;   It plays around with the stack so that the arguments are set
;   up right and clean off right and it also sets the ring 3
;   registers to reflect the outcome of the operation.
;
;   This macro is taken from VMM's memory manager, file: mma.asm
;
;   ENTRY:  fun - function name
;       cargs - number of dword arguments it has
;       prefix - prefix for function
;   EXIT:   none
;
;   Note that when the function is called:
;       EBX is the VM handle
;       ESI points to client registers
;       EDI points to the return address
;   THESE REGISTERS MUST NOT BE TRASHED!
;
W32Fun  MACRO   fun, cbargs, prefix

BeginProc   VW32&fun, esp, W32SVC, public
    ArgVar  pcrs,dword
    ArgVar  hvm,dword
    x = 0
    REPT    cbargs
        x = x + 1
        ArgVar  arg&x,dword
    ENDM

    EnterProc
    pop edi     ;Save and remove return address
    pop esi     ;Save and remove client regs
    pop ebx     ;Save and remove hvm

    call    prefix&fun  ;Call function somewhere in VxD
                ;Note that this function may be a C function

    mov [esi].Client_EAX,eax    ;Put return values into client regs

    push    ebx     ;Put hvm back on stack
    push    esi     ;Put client regs back on stack
    push    edi     ;Restore return address

    LeaveProc
    Return
EndProc VW32&fun

ENDM
*/

// ;EndInternal

#endif  // _VWIN32_H_
