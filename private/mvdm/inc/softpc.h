/*++ BUILD Version: 0001

Copyright (c) 1990  Microsoft Corporation

Module Name:

    SOFTPC.H

Abstract:

    High-level include file for components interfacing to SoftPC

Author:

    Dave Hastings (daveh) 25-Apr-1991

Revision History:
    Sudeep Bharati        23-Aug-1991  Added SOFTPC_BLD
    Matt Felton            8-FEB-1992  Added getIntelRegistersPointer
    Jonle                 18-Sep-1992  Add popup for unsupported functionality
    Jonle                 21-Nov-1992  Add Standard Resource Error Dialog Box
                                           GetPIFConfigFiles

--*/


/********** COMMON STUFF FOR MIPS AND V86 *************/

/* XLATOFF */
#ifdef i386
#include "v86def.h"
#endif


/* XLATON */
extern VOID nt_block_event_thread(ULONG);
extern VOID nt_resume_event_thread(VOID);
/* notification of PDB(Process Data Block, A.K.A PSP) termination */
extern VOID HostTerminatePDB(USHORT pdb);
/* disk subsystem reset notification. These functions will close all
 * outstanding opened handles for DASD(Direct AcceS Disk) */
extern VOID HostFloppyReset(VOID);
extern VOID HostFdiskReset(VOID);

#if defined(NEC_98)
extern USHORT nt_get_cursor_pos(VOID);   
#endif

// unsupported services dialog box
extern VOID host_direct_access_error(ULONG);

extern VOID host_lpt_flush_initialize(VOID);

#define NOSUPPORT_FLOPPY      0
#define NOSUPPORT_HARDDISK    1
#define NOSUPPORT_DRIVER      2
#define NOSUPPORT_OLDPIF      3
#define NOSUPPORT_ILLBOP      4
#define NOSUPPORT_NOLIM       5
#define NOSUPPORT_MOUSEDRVR   6

// standard error dialog box using resources
void RcErrorDialogBox(USHORT wId, CHAR *msg1, CHAR *msg2);


/*
 *  RcMessageBox\EditBox stuff
 */
#define RMB_ABORT        1
#define RMB_RETRY        2
#define RMB_IGNORE       4
#define RMB_ICON_INFO    8
#define RMB_ICON_BANG   16
#define RMB_ICON_STOP   32
#define RMB_ICON_WHAT   64
#define RMB_EDIT       128
#define RMB_FLAGS_MASK 0x0000FFFF
#define RMB_EDITBUFFERSIZE_MASK 0xFFFF0000
// hiword of dwOptions is reserved for RMB_EDIT text buffer size

int RcMessageBox(USHORT wId, CHAR *msg1, CHAR *msg2, ULONG dwOptions);


// sudeepb 02-May-1993 these following defines are actually defined
// in host\inc\error.h and host\inc\nt_uis.h. this stuff needs
// major cleanup after product 1.0.

#define EG_MALLOC_FAILURE       7
#define EG_PIF_BAD_FORMAT      18
#define EG_PIF_STARTDIR_ERR    19
#define	EG_PIF_STARTFILE_ERR   20
#define EG_PIF_CMDLINE_ERR     21
#define EG_PIF_ASK_CMDLINE     22
#define EG_ENVIRONMENT_ERR     23
#define EG_BAD_FAULT           27
#define EG_DOS_PROG_EXTENSION  28

#define ED_BADSYSFILE		336
#define ED_INITMEMERR           337
#define ED_INITTMPFILE          338


//
// SysErrorBox stuff -- duplicated in usersrv.h *and* kernel.inc
//
#define  SEB_OK         1  /* Button with "OK".     */
#define  SEB_CANCEL     2  /* Button with "Cancel"  */
#define  SEB_YES        3  /* Button with "&Yes"     */
#define  SEB_NO         4  /* Button with "&No"      */
#define  SEB_RETRY      5  /* Button with "&Retry"   */
#define  SEB_ABORT      6  /* Button with "&Abort"   */
#define  SEB_IGNORE     7  /* Button with "&Ignore"  */
#define  SEB_CLOSE      8  /* Button with "&Close"   */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

ULONG WOWSysErrorBox(
    LPSTR  szTitle,
    LPSTR  szMessage,
    USHORT wBtn1,
    USHORT wBtn2,
    USHORT wBtn3
    );

// Called by WOW to force VdmAllocateVirtualMemory to alloc blocks of
// memory with ever increasing linear address's.
VOID SetWOWforceIncrAlloc(
    BOOL iEnable
    );

// call out to softpc to get config.sys\autoexec.bat file names
#ifdef DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName, BOOL bFreMem);
#else // !DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName);
#endif // !DBCS

// exported interfaces
extern VOID    TerminateVDM (VOID);
extern ULONG   DosSessionId;

// VDD idle callouts
void WaitIfIdle(void);
void WakeUpNow(void);


//
// Constants
//
#define MSW_PE              0x1

/* XLATOFF */
#define ISPESET             (UCHAR) (getMSW() & MSW_PE ? 1 : 0)
/* XLATON */

//
// Macros
//

#define EXPORT

// Flag Register constants

#define FLG_CARRY           0x00000001
#define FLG_CARRY_BIT       0x00000000
#define FLG_PARITY          0x00000004
#define FLG_PARITY_BIT      0x00000003
#define FLG_AUXILIARY       0x00000010
#define FLG_AUXILIARY_BIT   0x00000005
#define FLG_ZERO            0x00000040
#define FLG_ZERO_BIT        0x00000006
#define FLG_SIGN            0x00000080
#define FLG_SIGN_BIT        0x00000007
#define FLG_TRAP            0x00000100

#define FLG_INTERRUPT       0x00000200
#define FLG_INTERRUPT_BIT   0x00000009
#define FLG_DIRECTION       0x00000400
#define FLG_DIRECTION_BIT   0x0000000A
#define FLG_OVERFLOW        0x00000800
#define FLG_OVERFLOW_BIT    0x0000000B

EXPORT
VOID
host_cpu_init(
     VOID
     );

//EXPORT
//VOID
//sas_init(
//    IN sys_addr Size
//    );

EXPORT
VOID
host_simulate(
    VOID
    );

HANDLE
host_CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );

VOID
cpu_createthread(
    HANDLE  hThread
    );

VOID
host_ExitThread(
    DWORD dwExitCode
    );


EXPORT VOID host_com_close(int PortId);
EXPORT int SuspendTimerThread(VOID);
EXPORT int ResumeTimerThread(VOID);

void *ch_malloc(unsigned int NumBytes);


UCHAR *Sim32pGetVDMPointer(ULONG addr, UCHAR pm);
#define Sim32GetVDMPointer(Addr,Size,Mode) Sim32pGetVDMPointer(Addr,Mode)

#ifdef i386

/********** FOR V86 BUILD *************/

BOOL
ThreadSetDebugContext(
    PULONG pDebugRegisters
    );

BOOL
ThreadGetDebugContext(
    PULONG pDebugRegisters
    );

// external data

extern ULONG      IntelBase;        // used by memory access macros
extern X86CONTEXT IntelRegisters;   // used by register access macros
extern ULONG      VdmDebugLevel;    // used to control debugging
extern ULONG      IntelMSW;         // used by getMSW/setMSW macs.
extern ULONG      VdmFeatureBits;


// Register access macros

#ifdef LINKED_INTO_MONITOR

#include <vdm.h>
extern VDM_TIB VdmTib;

#define getEAX()    (VdmTib.VdmContext.Eax)
#define getAX()     ((USHORT)(VdmTib.VdmContext.Eax))
#define getAL()     ((BYTE)(VdmTib.VdmContext.Eax))
#define getAH()     ((BYTE)(VdmTib.VdmContext.Eax >> 8))
#define getEBX()    (VdmTib.VdmContext.Ebx)
#define getBX()     ((USHORT)(VdmTib.VdmContext.Ebx))
#define getBL()     ((BYTE)(VdmTib.VdmContext.Ebx))
#define getBH()     ((BYTE)(VdmTib.VdmContext.Ebx >> 8))
#define getECX()    (VdmTib.VdmContext.Ecx)
#define getCX()     ((USHORT)(VdmTib.VdmContext.Ecx))
#define getCL()     ((BYTE)(VdmTib.VdmContext.Ecx))
#define getCH()     ((BYTE)(VdmTib.VdmContext.Ecx >> 8))
#define getEDX()    (VdmTib.VdmContext.Edx)
#define getDX()     ((USHORT)(VdmTib.VdmContext.Edx))
#define getDL()     ((BYTE)(VdmTib.VdmContext.Edx))
#define getDH()     ((BYTE)(VdmTib.VdmContext.Edx >> 8))
#define getESP()    (VdmTib.VdmContext.Esp)
#define getSP()     ((USHORT)VdmTib.VdmContext.Esp)
#define getEBP()    (VdmTib.VdmContext.Ebp)
#define getBP()     ((USHORT)VdmTib.VdmContext.Ebp)
#define getESI()    (VdmTib.VdmContext.Esi)
#define getSI()     ((USHORT)VdmTib.VdmContext.Esi)
#define getEDI()    (VdmTib.VdmContext.Edi)
#define getDI()     ((USHORT)VdmTib.VdmContext.Edi)
#define getEIP()    (VdmTib.VdmContext.Eip)
#define getIP()     ((USHORT)VdmTib.VdmContext.Eip)
#define getCS()     ((USHORT)VdmTib.VdmContext.SegCs)
#define getSS()     ((USHORT)VdmTib.VdmContext.SegSs)
#define getDS()     ((USHORT)VdmTib.VdmContext.SegDs)
#define getES()     ((USHORT)VdmTib.VdmContext.SegEs)
#define getFS()     ((USHORT)VdmTib.VdmContext.SegFs)
#define getGS()     ((USHORT)VdmTib.VdmContext.SegGs)
#define getCF()     ((VdmTib.VdmContext.EFlags & FLG_CARRY) ? 1 : 0)
#define getPF()     ((VdmTib.VdmContext.EFlags & FLG_PARITY) ? 1 : 0)
#define getAF()     ((VdmTib.VdmContext.EFlags & FLG_AUXILIARY) ? 1 : 0)
#define getZF()     ((VdmTib.VdmContext.EFlags & FLG_ZERO) ? 1 : 0)
#define getSF()     ((VdmTib.VdmContext.EFlags & FLG_SIGN) ? 1 : 0)
#define getTF()     ((VdmTib.VdmContext.EFlags & FLG_TRAP) ? 1 : 0)
#define getIF()     ((VdmTib.VdmContext.EFlags & FLG_INTERRUPT) ? 1 : 0)
#define getDF()     ((VdmTib.VdmContext.EFlags & FLG_DIRECTION) ? 1 : 0)
#define getOF()     ((VdmTib.VdmContext.EFlags & FLG_OVERFLOW) ? 1 : 0)
#define getMSW()    (IntelMSW)
#define getSTATUS() (USHORT)(VdmTib.VdmContext.EFlags)
#define getEFLAGS() (VdmTib.VdmContext.EFlags)

#define setEAX(val) {    VdmTib.VdmContext.Eax = val;}

#define setAX(val) {    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFF0000) |                            ((ULONG)val & 0x0000FFFF);}

#define setAH(val) {    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFF00FF) |                            ((ULONG)(val << 8) & 0x0000FF00);}

#define setAL(val) {    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFFFF00) |                            ((ULONG)val & 0x000000FF);}

#define setEBX(val) {    VdmTib.VdmContext.Ebx = val ;}

#define setBX(val) {    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFF0000) |                            ((ULONG)val & 0x0000FFFF);}

#define setBH(val) {    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFF00FF) |                            ((ULONG)(val << 8) & 0x0000FF00);}

#define setBL(val) {    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFFFF00) |                            ((ULONG)val & 0x000000FF);}

#define setECX(val) {    VdmTib.VdmContext.Ecx = val ;}

#define setCX(val) {    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFF0000) |                            ((ULONG)val & 0x0000FFFF);}

#define setCH(val) {    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFF00FF) |                            ((ULONG)(val << 8) & 0x0000FF00);}

#define setCL(val) {    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFFFF00) |                            ((ULONG)val & 0x000000FF);}

#define setEDX(val) {    VdmTib.VdmContext.Edx = val ;}

#define setDX(val) {    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFF0000) |                            ((ULONG)val & 0x0000FFFF);}

#define setDH(val) {    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFF00FF) |                            ((ULONG)(val << 8) & 0x0000FF00);}

#define setDL(val) {    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFFFF00) |                                ((ULONG)val & 0x000000FF);}

#define setESP(val) {    VdmTib.VdmContext.Esp = val ;}

#define setSP(val) {    VdmTib.VdmContext.Esp = (VdmTib.VdmContext.Esp & 0xFFFF0000) |                                ((ULONG)val & 0x0000FFFF);}

#define setEBP(val) {    VdmTib.VdmContext.Ebp = val;}

#define setBP(val) {    VdmTib.VdmContext.Ebp = (VdmTib.VdmContext.Ebp & 0xFFFF0000) |                                ((ULONG)val & 0x0000FFFF);}

#define setESI(val) {    VdmTib.VdmContext.Esi = val ;}

#define setSI(val) {    VdmTib.VdmContext.Esi = (VdmTib.VdmContext.Esi & 0xFFFF0000) |                                ((ULONG)val & 0x0000FFFF);}
#define setEDI(val) {    VdmTib.VdmContext.Edi = val ;}

#define setDI(val) {    VdmTib.VdmContext.Edi = (VdmTib.VdmContext.Edi & 0xFFFF0000) |                                ((ULONG)val & 0x0000FFFF);}

#define setEIP(val) {    VdmTib.VdmContext.Eip = val ;}

#define setIP(val) {    VdmTib.VdmContext.Eip = (VdmTib.VdmContext.Eip & 0xFFFF0000) |                                ((ULONG)val & 0x0000FFFF);}

#define setCS(val) {    VdmTib.VdmContext.SegCs = (ULONG) val & 0x0000FFFF ;}

#define setSS(val) {    VdmTib.VdmContext.SegSs = (ULONG) val & 0x0000FFFF ;}

#define setDS(val) {    VdmTib.VdmContext.SegDs = (ULONG) val & 0x0000FFFF ;}

#define setES(val) {    VdmTib.VdmContext.SegEs = (ULONG) val & 0x0000FFFF ;}

#define setFS(val) {    VdmTib.VdmContext.SegFs = (ULONG) val & 0x0000FFFF ;}

#define setGS(val) {    VdmTib.VdmContext.SegGs = (ULONG) val & 0x0000FFFF ;}

#define setCF(val)  {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_CARRY) |                                (((ULONG)val << FLG_CARRY_BIT) & FLG_CARRY);}

#define setPF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_PARITY) |                                (((ULONG)val << FLG_PARITY_BIT) & FLG_PARITY);}

#define setAF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_AUXILIARY) |                                (((ULONG)val << FLG_AUXILIARY_BIT) & FLG_AUXILIARY);}

#define setZF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_ZERO) |                                (((ULONG)val << FLG_ZERO_BIT) & FLG_ZERO);}

#define setSF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_SIGN) |                                (((ULONG)val << FLG_SIGN_BIT) & FLG_SIGN);}

#define setIF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_INTERRUPT) |                                (((ULONG)val << FLG_INTERRUPT_BIT) & FLG_INTERRUPT);}

#define setDF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_DIRECTION) |                                (((ULONG)val << FLG_DIRECTION_BIT) & FLG_DIRECTION);}

#define setOF(val) {    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_OVERFLOW) |                                (((ULONG)val << FLG_OVERFLOW_BIT) & FLG_OVERFLOW);}

#define setMSW(val) {    IntelMSW = val ;}

#define setSTATUS(val) { VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & 0xFFFF0000) | val;}

#define setEFLAGS(val) { VdmTib.VdmContext.EFlags = val;}

#else // not linked into monitor

extern ULONG  getEAX(VOID);
extern USHORT getAX(VOID);
extern UCHAR  getAL(VOID);
extern UCHAR  getAH(VOID);
extern ULONG  getEBX(VOID);
extern USHORT getBX(VOID);
extern UCHAR  getBL(VOID);
extern UCHAR  getBH(VOID);
extern ULONG  getECX(VOID);
extern USHORT getCX(VOID);
extern UCHAR  getCL(VOID);
extern UCHAR  getCH(VOID);
extern ULONG  getEDX(VOID);
extern USHORT getDX(VOID);
extern UCHAR  getDL(VOID);
extern UCHAR  getDH(VOID);
extern ULONG  getESP(VOID);
extern USHORT getSP(VOID);
extern ULONG  getEBP(VOID);
extern USHORT getBP(VOID);
extern ULONG  getESI(VOID);
extern USHORT getSI(VOID);
extern ULONG  getEDI(VOID);
extern USHORT getDI(VOID);
extern ULONG  getEIP(VOID);
extern USHORT getIP(VOID);
extern USHORT getCS(VOID);
extern USHORT getSS(VOID);
extern USHORT getDS(VOID);
extern USHORT getES(VOID);
extern USHORT getFS(VOID);
extern USHORT getGS(VOID);
extern ULONG  getCF(VOID);
extern ULONG  getPF(VOID);
extern ULONG  getAF(VOID);
extern ULONG  getZF(VOID);
extern ULONG  getSF(VOID);
extern ULONG  getIF(VOID);
extern ULONG  getDF(VOID);
extern ULONG  getOF(VOID);
extern USHORT getMSW(VOID);
extern USHORT getSTATUS(VOID);
extern ULONG  getEFLAGS(VOID);

extern VOID setEAX(ULONG);
extern VOID setAX(USHORT);
extern VOID setAH(UCHAR);
extern VOID setAL(UCHAR);
extern VOID setEBX(ULONG);
extern VOID setBX(USHORT);
extern VOID setBH(UCHAR);
extern VOID setBL(UCHAR);
extern VOID setECX(ULONG);
extern VOID setCX(USHORT);
extern VOID setCH(UCHAR);
extern VOID setCL(UCHAR);
extern VOID setEDX(ULONG);
extern VOID setDX(USHORT);
extern VOID setDH(UCHAR);
extern VOID setDL(UCHAR);
extern VOID setESP(ULONG);
extern VOID setSP(USHORT);
extern VOID setEBP(ULONG);
extern VOID setBP(USHORT);
extern VOID setESI(ULONG);
extern VOID setSI(USHORT);
extern VOID setEDI(ULONG);
extern VOID setDI(USHORT);
extern VOID setEIP(ULONG);
extern VOID setIP(USHORT);
extern VOID setCS(USHORT);
extern VOID setSS(USHORT);
extern VOID setDS(USHORT);
extern VOID setES(USHORT);
extern VOID setFS(USHORT);
extern VOID setGS(USHORT);
extern VOID setCF(ULONG);
extern VOID setPF(ULONG);
extern VOID setAF(ULONG);
extern VOID setZF(ULONG);
extern VOID setSF(ULONG);
extern VOID setIF(ULONG);
extern VOID setDF(ULONG);
extern VOID setOF(ULONG);
extern VOID setMSW(USHORT);
extern VOID setSTATUS(USHORT);
extern VOID setEFLAGS(ULONG);
#endif

//
// Sim32 macros
//

// no action is required for this macro.
#define Sim32FlushVDMPointer( address, size, buffer, mode ) TRUE

// no action is required for this macro.
#define Sim32FreeVDMPointer( address, size, buffer, mode) TRUE

#define Sim32GetVDMMemory( address, size, buffer, mode) (memcpy(  \
    buffer, Sim32pGetVDMPointer(address, mode), size), TRUE)

#define Sim32SetVDMMemory( address, size, buffer, mode) (memcpy( \
    Sim32pGetVDMPointer(address, mode), buffer, size), TRUE)

// Address conversion macros

#define RMOFF(address) (WORD)((ULONG)address & 0x0000FFFF)
#define RMSEG(address) (WORD)(((ULONG)address - ((ULONG)address & 0x0000FFFF)) >> 4)
#define RMSEGOFFTOLIN(seg, off) (PVOID)(IntelBase + ((ULONG)(seg) << 4) + (ULONG)(off))

// Debugging Macros

#define VDprint(value, arg) { if (value <= (VdmDebugLevel & VDP_LEVEL_MASK)) {\
                                         DbgPrint arg ; }}

#define VDbreak(value) { if (value <= (VDB_LEVEL_MASK & VdmDebugLevel)) {\
                                         DbgBreakPoint(); }}

#define VDP_LEVEL_MASK          0x0F
#define VDP_LEVEL_NONE          0x0
#define VDP_LEVEL_ERROR         0x2
#define VDP_LEVEL_WARNING       0x4
#define VDP_LEVEL_INFO          0x8

#define VDB_LEVEL_MASK          0xF0
#define VDB_LEVEL_NONE          0x00
#define VDB_LEVEL_ERROR         0x20
#define VDB_LEVEL_WARNING       0x40
#define VDB_LEVEL_INFO          0x80

//
//  Function prototypes
//

ULONG
DbgPrint(
    PCH Format,
    ...
    );

char *nt_fgets(char *buffer, int len, void *input_stream);
char *nt_gets(char *buffer);


#define GetVDMAddr(usSeg,usOff)  (((ULONG)usSeg << 4) + usOff)

PX86CONTEXT getIntelRegistersPointer(VOID);

#else

/*********************** FOR MIPS BUILD ***************************/


#define GetVDMAddr(usSeg,usOff) Sim32pGetVDMPointer((ULONG)(((ULONG)usSeg << 16) | usOff),FALSE)

#define RMSEGOFFTOLIN(seg, off) (PVOID)(((ULONG)(seg) << 4) + (ULONG)(off))

extern BOOL     Sim32FlushVDMPointer (ULONG, USHORT, PBYTE , BOOL);
extern BOOL     Sim32FreeVDMPointer (ULONG, USHORT, PBYTE , BOOL);
extern BOOL     Sim32GetVDMMemory (ULONG, USHORT, PBYTE , BOOL);
extern BOOL     Sim32SetVDMMemory (ULONG, USHORT, PBYTE , BOOL);
extern VOID     sas_overwrite_memory(PBYTE, ULONG);

extern UCHAR getAL(VOID);
extern UCHAR getCL(VOID);
extern UCHAR getDL(VOID);
extern UCHAR getBL(VOID);
extern UCHAR getAH(VOID);
extern UCHAR getCH(VOID);
extern UCHAR getDH(VOID);
extern UCHAR getBH(VOID);

extern VOID setAL(UCHAR val);
extern VOID setCL(UCHAR val);
extern VOID setDL(UCHAR val);
extern VOID setBL(UCHAR val);
extern VOID setAH(UCHAR val);
extern VOID setCH(UCHAR val);
extern VOID setDH(UCHAR val);
extern VOID setBH(UCHAR val);

extern USHORT getAX(VOID);
extern USHORT getCX(VOID);
extern USHORT getDX(VOID);
extern USHORT getBX(VOID);
extern USHORT getSP(VOID);
extern USHORT getBP(VOID);
extern USHORT getSI(VOID);
extern USHORT getDI(VOID);
extern USHORT getIP(VOID);

extern VOID setAX(USHORT val);
extern VOID setCX(USHORT val);
extern VOID setDX(USHORT val);
extern VOID setBX(USHORT val);
extern VOID setSP(USHORT val);
extern VOID setBP(USHORT val);
extern VOID setSI(USHORT val);
extern VOID setDI(USHORT val);
extern VOID setIP(USHORT val);

extern ULONG getEAX(VOID);
extern ULONG getECX(VOID);
extern ULONG getEDX(VOID);
extern ULONG getEBX(VOID);
extern ULONG getESP(VOID);
extern ULONG getEBP(VOID);
extern ULONG getESI(VOID);
extern ULONG getEDI(VOID);
extern ULONG getEIP(VOID);

extern VOID setEAX(ULONG val);
extern VOID setECX(ULONG val);
extern VOID setEDX(ULONG val);
extern VOID setEBX(ULONG val);
extern VOID setESP(ULONG val);
extern VOID setEBP(ULONG val);
extern VOID setESI(ULONG val);
extern VOID setEDI(ULONG val);
extern VOID setEIP(ULONG val);

extern USHORT getES(VOID);
extern USHORT getCS(VOID);
extern USHORT getSS(VOID);
extern USHORT getDS(VOID);
extern USHORT getFS(VOID);
extern USHORT getGS(VOID);

extern VOID setES(USHORT val);
extern VOID setCS(USHORT val);
extern VOID setSS(USHORT val);
extern VOID setDS(USHORT val);
extern VOID setFS(USHORT val);
extern VOID setGS(USHORT val);

extern ULONG getAF(VOID);
extern ULONG getCF(VOID);
extern ULONG getDF(VOID);
extern ULONG getIF(VOID);
extern ULONG getOF(VOID);
extern ULONG getPF(VOID);
extern ULONG getSF(VOID);
extern ULONG getTF(VOID);
extern ULONG getZF(VOID);
extern ULONG getIOPL(VOID);
extern ULONG getNT(VOID);
extern USHORT getSTATUS(VOID);
extern ULONG getEFLAGS(VOID);
extern USHORT getMSW(VOID);

extern VOID setAF(ULONG val);
extern VOID setCF(ULONG val);
extern VOID setDF(ULONG val);
extern VOID setIF(ULONG val);
extern VOID setOF(ULONG val);
extern VOID setPF(ULONG val);
extern VOID setSF(ULONG val);
extern VOID setTF(ULONG val);
extern VOID setZF(ULONG val);
extern VOID setIOPL(ULONG val);
extern VOID setNT(ULONG val);
extern VOID setSTATUS(USHORT val);
extern VOID setEFLAGS(ULONG val);
extern VOID setMSW(USHORT val);

extern ULONG getCR0(VOID);
extern VOID setCR0(ULONG val);
extern ULONG getSS_AR(VOID);
extern ULONG getSS_BASE(VOID);
extern VOID setSS_BASE_LIMIT_AR(ULONG base, ULONG limit, ULONG ar);
extern VOID setCPL(ULONG val);


#endif
