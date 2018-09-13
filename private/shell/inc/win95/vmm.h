/*****************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1988-1990
 *
 *   Title: VMM.H - Include file for Virtual Machine Manager
 *
 *   Version:	1.00
 *
 *   Date:  05-May-1988
 *
 *   Author:	RAL
 *
 *-----------------------------------------------------------------------------
 *
 *   Change log:
 *
 *   DATE    REV DESCRIPTION
 *   ----------- --- -----------------------------------------------------------
 *   05-May-1988 RAL Original
 *   13-Nov-1991 PBS C version
 *   17-Dec-1993     Adds Far East VxDs identifiers
 */

#ifndef _VMM_
#define _VMM_


/*
 *  NON Windows/386 Virtual Device sources can include this file to get
 *  some useful equates by declaring the symbol "Not_VxD" If this symbol
 *  is defined, then everything that has to do with the specifics of the
 *  32 bit environment for virtual devices is removed.	Useful equates
 *  include: device ID's, pushad structure, BeginDoc, EndDoc, BeginMsg,
 *  EndMsg, page table equates, etc.
 */

#define FALSE	    0	    // False
#define VMM_TRUE    (~FALSE)	// The opposite of False!

#define DEBLEVELRETAIL	0
#define DEBLEVELNORMAL	1
#define DEBLEVELMAX 2

#ifndef DEBLEVEL
#ifdef DEBUG
#define DEBLEVEL DEBLEVELNORMAL
#else
#define DEBLEVEL DEBLEVELRETAIL
#endif
#endif

#ifndef WIN31COMPAT
#define WIN40SERVICES
#define WIN403SERVICES		/*OPK-3 Services*/
#endif

#ifndef WIN40COMPAT
#define WIN41SERVICES
#endif

/* ASM
ifdef MASM6
ifndef NO_MASM6_OPTIONS
;
;   option switches necessary to build VMM/VxD sources with MASM 6
;
    option oldmacros
ifndef	NEWSTRUCTS	; define NEWSTRUCTS for MASM6 struct semantics
    option oldstructs
endif
    option noscoped
    option segment:flat
    option offset:flat
    option proc:private
endif
endif
;
;   These null macros are recognized by a utility program that produces
;   documentation files.
;
IFDEF MASM6
BeginDoc MACRO
     ENDM
EndDoc MACRO
       ENDM

BeginMsg MACRO
     ENDM
EndMsg MACRO
       ENDM
ELSE
BeginDoc EQU <>
EndDoc EQU <>

BeginMsg EQU <>
EndMsg EQU <>
ENDIF
*/


/******************************************************************************
 *
 *	    EQUATES FOR REQUIRED DEVICES
 *
 *   Device ID formulation note:
 *
 *  The high bit of the device ID is reserved for future use.
 *  Microsoft reserves the device ID's 0-1FFh for standard devices.  If
 *  an OEM VxD is a replacement for a standard VxD, then it must use the
 *  standard VxD ID.
 *
 *  OEMS WHO WANT A VXD DEVICE ID ASSIGNED TO THEM,
 *  PLEASE CONTACT MICROSOFT PRODUCT SUPPORT.  ID's are only required for
 *  devices which provide services, V86 API's or PM API's.  Also, calling
 *  services or API's by VxD name is now supported in version 4.0, so an
 *  ID may not be necessary as long as a unique 8 character name is used.
 *
 *****************************************************************************/

#define UNDEFINED_DEVICE_ID 0x00000
#define VMM_DEVICE_ID	    0x00001 /* Used for dynalink table */
#define DEBUG_DEVICE_ID     0x00002
#define VPICD_DEVICE_ID     0x00003
#define VDMAD_DEVICE_ID     0x00004
#define VTD_DEVICE_ID	    0x00005
#define V86MMGR_DEVICE_ID   0x00006
#define PAGESWAP_DEVICE_ID  0x00007
#define PARITY_DEVICE_ID    0x00008
#define REBOOT_DEVICE_ID    0x00009
#define VDD_DEVICE_ID	    0x0000A
#define VSD_DEVICE_ID	    0x0000B
#define VMD_DEVICE_ID	    0x0000C
#define VKD_DEVICE_ID	    0x0000D
#define VCD_DEVICE_ID	    0x0000E
#define VPD_DEVICE_ID	    0x0000F
#define BLOCKDEV_DEVICE_ID  0x00010
#define VMCPD_DEVICE_ID     0x00011
#define EBIOS_DEVICE_ID     0x00012
#define BIOSXLAT_DEVICE_ID  0x00013
#define VNETBIOS_DEVICE_ID  0x00014
#define DOSMGR_DEVICE_ID    0x00015
#define WINLOAD_DEVICE_ID   0x00016
#define SHELL_DEVICE_ID     0x00017
#define VMPOLL_DEVICE_ID    0x00018
#define VPROD_DEVICE_ID     0x00019
#define DOSNET_DEVICE_ID    0x0001A
#define VFD_DEVICE_ID	    0x0001B
#define VDD2_DEVICE_ID	    0x0001C /* Secondary display adapter */
#define WINDEBUG_DEVICE_ID  0x0001D
#define TSRLOAD_DEVICE_ID   0x0001E /* TSR instance utility ID */
#define BIOSHOOK_DEVICE_ID  0x0001F /* Bios interrupt hooker VxD */
#define INT13_DEVICE_ID     0x00020
#define PAGEFILE_DEVICE_ID  0x00021 /* Paging File device */
#define SCSI_DEVICE_ID	    0x00022 /* SCSI device */
#define MCA_POS_DEVICE_ID   0x00023 /* MCA_POS device */
#define SCSIFD_DEVICE_ID    0x00024 /* SCSI FastDisk device */
#define VPEND_DEVICE_ID     0x00025 /* Pen device */
#define APM_DEVICE_ID	    0x00026 /* Power Management device */
#define VPOWERD_DEVICE_ID   APM_DEVICE_ID   /* We overload APM since we replace it */
#define VXDLDR_DEVICE_ID    0x00027 /* VxD Loader device */
#define NDIS_DEVICE_ID	    0x00028 /* NDIS wrapper */
#define BIOS_EXT_DEVICE_ID   0x00029 /* Fix Broken BIOS device */
#define VWIN32_DEVICE_ID	0x0002A /* for new WIN32-VxD */
#define VCOMM_DEVICE_ID 	0x0002B /* New COMM device driver */
#define SPOOLER_DEVICE_ID	0x0002C /* Local Spooler */
#define WIN32S_DEVICE_ID    0x0002D /* Win32S on Win 3.1 driver */
#define DEBUGCMD_DEVICE_ID	0x0002E /* Debug command extensions */
/* #define RESERVED_DEVICE_ID	0x0002F /* Not currently in use */
/* #define ATI_HELPER_DEVICE_ID    0x00030 /* grabbed by ATI */

/* 31-32 USED BY WFW NET COMPONENTS	*/
/* #define VNB_DEVICE_ID	   0x00031 /* Netbeui of snowball */
/* #define SERVER_DEVICE_ID	   0x00032 /* Server of snowball */

#define CONFIGMG_DEVICE_ID  0x00033 /* Configuration manager (Plug&Play) */
#define DWCFGMG_DEVICE_ID   0x00034 /* Configuration manager for win31 and DOS */
#define SCSIPORT_DEVICE_ID  0x00035 /* Dragon miniport loader/driver */
#define VFBACKUP_DEVICE_ID  0x00036 /* allows backup apps to work with NEC */
#define ENABLE_DEVICE_ID    0x00037 /* for access VxD */
#define VCOND_DEVICE_ID     0x00038 /* Virtual Console Device - check vcond.inc */
/* 39 used by WFW VFat Helper device */

/* 3A used by WFW E-FAX */
/* #define EFAX_DEVICE_ID   0x0003A /* EFAX VxD ID	*/

/* 3B used by MS-DOS 6.1 for the DblSpace VxD which has APIs */
/* #define DSVXD_DEVICE_ID  0x0003B /* Dbl Space VxD ID */

#define ISAPNP_DEVICE_ID    0x0003C /* ISA P&P Enumerator */
#define BIOS_DEVICE_ID	    0x0003D /* BIOS P&P Enumerator */
/* #define WINSOCK_DEVICE_ID	   0x0003E  /* WinSockets */
/* #define WSIPX_DEVICE_ID     0x0003F	/* WinSockets for IPX */

#define IFSMgr_Device_ID    0x00040 /* Installable File System Manager */
#define VCDFSD_DEVICE_ID    0x00041 /* Static CDFS ID */
#define MRCI2_DEVICE_ID     0x00042 /* DrvSpace compression engine */
#define PCI_DEVICE_ID	    0x00043 /* PCI P&P Enumerator */
#define PELOADER_DEVICE_ID  0x00044 /* PE Image Loader */
#define EISA_DEVICE_ID	    0x00045 /* EISA P&P Enumerator */
#define DRAGCLI_DEVICE_ID   0x00046 /* Dragon network client */
#define DRAGSRV_DEVICE_ID   0x00047 /* Dragon network server */
#define PERF_DEVICE_ID	    0x00048 /* Config/stat info */

#define AWREDIR_DEVICE_ID   0x00049 /* AtWork Network FSD */
#define DDS_DEVICE_ID	    0x0004A /* Device driver services */
#define NTKERN_DEVICE_ID    0x0004B /* NT kernel device id */
#define VDOSKEYD_DEVICE_ID  0x0004B /* DOSKEY device id */

/*
 *   Far East DOS support VxD ID
 */

#define ETEN_Device_ID	    0x00060 /* ETEN DOS (Taiwan) driver */
#define CHBIOS_Device_ID    0x00061 /* CHBIOS DOS (Korean) driver */
#define VMSGD_Device_ID    0x00062 /* DBCS Message Mode driver */
#define VPPID_Device_ID     0x00063 /* PC-98 System Control PPI */
#define VIME_Device_ID	    0x00064 /* Virtual DOS IME */
#define VHBIOSD_Device_ID   0x00065 /* HBIOS (Korean) for HWin31 driver */

#define BASEID_FOR_NAMEBASEDVXD        0xf000 /* Name based VxD IDs start here */
#define BASEID_FOR_NAMEBASEDVXD_MASK   0x0fff /* Mask to get the real vxd id */
/*
 *   Initialization order equates.  Devices are initialized in order from
 *   LOWEST to HIGHEST. If 2 or more devices have the same initialization
 *   order value, then they are initialized in order of occurance, so a
 *   specific order is not guaranteed.	Holes have been left to allow maximum
 *   flexibility in ordering devices.
 */

#define VMM_INIT_ORDER	    0x000000000
#define DEBUG_INIT_ORDER    0x000000000 /* normally using 0 is bad */
#define DEBUGCMD_INIT_ORDER	0x000000000 /*	but debug must be first */
#define PERF_INIT_ORDER     0x000900000
#define APM_INIT_ORDER		0x001000000
#define VPOWERD_INIT_ORDER  APM_INIT_ORDER  /* We overload APM since we replace it */
#define BIOSHOOK_INIT_ORDER 0x006000000
#define VPROD_INIT_ORDER    0x008000000
#define VPICD_INIT_ORDER    0x00C000000
#define VTD_INIT_ORDER	    0x014000000
#define VWIN32_INIT_ORDER   0x014100000
#define NTKERN_INIT_ORDER   0x015000000 /* Must be before VxDLdr (so that it is ready for devnodes) */
#define VXDLDR_INIT_ORDER   0x016000000

#define ENUMERATOR_INIT_ORDER	0x016800000 /* Should be before IOS */
#define ISAPNP_INIT_ORDER   ENUMERATOR_INIT_ORDER
#define EISA_INIT_ORDER     ENUMERATOR_INIT_ORDER
#define PCI_INIT_ORDER	    ENUMERATOR_INIT_ORDER
#define BIOS_INIT_ORDER     ENUMERATOR_INIT_ORDER+1 /* To simplify reenumeration */
#define CONFIGMG_INIT_ORDER ENUMERATOR_INIT_ORDER+0xFFFF    /* After all enumerators */

#define VCDFSD_INIT_ORDER   0x016F00000
#define IOS_INIT_ORDER	    0x017000000
#define PAGEFILE_INIT_ORDER 0x018000000
#define PAGESWAP_INIT_ORDER 0x01C000000
#define PARITY_INIT_ORDER   0x020000000
#define REBOOT_INIT_ORDER   0x024000000
#define EBIOS_INIT_ORDER    0x026000000
#define VDD_INIT_ORDER	    0x028000000
#define VSD_INIT_ORDER	    0x02C000000

#define VCD_INIT_ORDER	    0x030000000
#define COMMDRVR_INIT_ORDER (VCD_INIT_ORDER - 1)
#define PRTCL_INIT_ORDER    (COMMDRVR_INIT_ORDER - 2)
#define MODEM_INIT_ORDER    (COMMDRVR_INIT_ORDER - 3)
#define PORT_INIT_ORDER     (COMMDRVR_INIT_ORDER - 4)

#define VMD_INIT_ORDER	    0x034000000
#define VKD_INIT_ORDER	    0x038000000
#define VPD_INIT_ORDER	    0x03C000000
#define BLOCKDEV_INIT_ORDER 0x040000000
#define MCA_POS_INIT_ORDER  0x041000000
#define SCSIFD_INIT_ORDER   0x041400000
#define SCSIMASTER_INIT_ORDER	0x041800000
#define INT13_INIT_ORDER    0x042000000
#define VMCPD_INIT_ORDER    0x048000000
#define BIOSXLAT_INIT_ORDER 0x050000000
#define VNETBIOS_INIT_ORDER 0x054000000
#define DOSMGR_INIT_ORDER   0x058000000
#define DOSNET_INIT_ORDER   0x05C000000
#define WINLOAD_INIT_ORDER  0x060000000
#define VMPOLL_INIT_ORDER   0x064000000

#define UNDEFINED_INIT_ORDER	0x080000000
#define VCOND_INIT_ORDER    UNDEFINED_INIT_ORDER

#define WINDEBUG_INIT_ORDER 0x081000000
#define VDMAD_INIT_ORDER    0x090000000
#define V86MMGR_INIT_ORDER  0x0A0000000

#define IFSMgr_Init_Order   0x10000 + V86MMGR_Init_Order
#define FSD_Init_Order	    0x00100 + IFSMgr_Init_Order
#define VFD_INIT_ORDER	    0x50000 + IFSMgr_Init_Order

/* Device that must touch memory in 1st Mb at crit init (after V86mmgr) */
#define UNDEF_TOUCH_MEM_INIT_ORDER  0x0A8000000
#define SHELL_INIT_ORDER    0x0B0000000

/* ASM
;******************************************************************************
;
;   Macro to cause a delay in between I/O accesses to the same device.
;
;------------------------------------------------------------------------------

IO_Delay    macro
jmp $+2
ENDM
*/

#define VXD_FAILURE 0
#define VXD_SUCCESS 1

typedef ULONG HVM;	    /* VM handle typedef */

/*
 *  Registers as they appear on the stack after a PUSHAD.
 */

struct Pushad_Struc {
    ULONG Pushad_EDI;		/* Client's EDI */
    ULONG Pushad_ESI;		/* Client's ESI */
    ULONG Pushad_EBP;		/* Client's EBP */
    ULONG Pushad_ESP;		/* ESP before pushad */
    ULONG Pushad_EBX;		/* Client's EBX */
    ULONG Pushad_EDX;		/* Client's EDX */
    ULONG Pushad_ECX;		/* Client's ECX */
    ULONG Pushad_EAX;		/* Client's EAX */
};

/* XLATOFF */

#ifdef RC_INVOKED
#define NOBASEDEFS
#endif

#ifndef NOBASEDEFS

#pragma warning (disable:4209)	// turn off redefinition warning

typedef unsigned char	UCHAR;
typedef unsigned short	USHORT;

#pragma warning (default:4209)	// turn off redefinition warning

#endif

#define GetVxDServiceOrdinal(service)	__##service

#define Begin_Service_Table(device, seg) \
    enum device##_SERVICES { \
    device##_dummy = (device##_DEVICE_ID << 16) - 1,

#define Declare_Service(service, local) \
    GetVxDServiceOrdinal(service),

#define Declare_SCService(service, args, local) \
    GetVxDServiceOrdinal(service),

#define End_Service_Table(device, seg) \
    Num_##device##_Services};

#define VXDINLINE static __inline
/* XLATON */

#ifndef Not_VxD

/* XLATOFF */
#define VxD_LOCKED_CODE_SEG code_seg("_LTEXT", "LCODE")
#define VxD_LOCKED_DATA_SEG data_seg("_LDATA", "LCODE")
#define VxD_INIT_CODE_SEG   code_seg("_ITEXT", "ICODE")
#define VxD_INIT_DATA_SEG   data_seg("_IDATA", "ICODE")
#define VxD_ICODE_SEG	    code_seg("_ITEXT", "ICODE")
#define VxD_IDATA_SEG	    data_seg("_IDATA", "ICODE")
#define VxD_PAGEABLE_CODE_SEG	code_seg("_PTEXT", "PCODE")
#define VxD_PAGEABLE_DATA_SEG	data_seg("_PDATA", "PDATA")
#define VxD_STATIC_CODE_SEG code_seg("_STEXT", "SCODE")
#define VxD_STATIC_DATA_SEG data_seg("_SDATA", "SCODE")
#define VxD_DEBUG_ONLY_CODE_SEG code_seg("_DBOCODE", "DBOCODE")
#define VxD_DEBUG_ONLY_DATA_SEG data_seg("_DBODATA", "DBOCODE")

#define VxD_SYSEXIT_CODE_SEG	code_seg("SYSEXIT", "SYSEXITCODE")
#define VxD_INT21_CODE_SEG  code_seg("INT21", "INT21CODE")
#define VxD_RARE_CODE_SEG   code_seg("RARE", "RARECODE")
#define VxD_W16_CODE_SEG    code_seg("W16", "W16CODE")
#define VxD_W32_CODE_SEG    code_seg("W32", "W32CODE")
#define VxD_VMCREATE_CODE_SEG	code_seg("VMCREATE", "VMCREATECODE")
#define VxD_VMDESTROY_CODE_SEG	code_seg("VMDESTROY", "VMDESTROYCODE")
#define VxD_THCREATE_CODE_SEG	code_seg("THCREATE", "THCREATECODE")
#define VxD_THDESTROY_CODE_SEG	code_seg("THDESTROY", "THDESTROYCODE")
#define VxD_VMSUSPEND_CODE_SEG	code_seg("VMSUSPEND", "VMSUSPENDCODE")
#define VxD_VMRESUME_CODE_SEG	code_seg("VMRESUME", "VMRESUMECODE")
#define VxD_PNP_CODE_SEG    code_seg("PNP", "PNPCODE")
#define VxD_DOSVM_CODE_SEG  code_seg("DOSVM", "DOSVMCODE")
#define VxD_LOCKABLE_CODE_SEG	code_seg("LOCKABLE", "LOCKABLECODE")
/* XLATON */

/* ASM
??_CUR_CODE_SEG = 0

??_LCODE    =	1
??_ICODE    =	2
??_PCODE    =	3
??_SCODE    =	4
??_DBOCODE  =	5
??_16ICODE  =	6
??_RCODE    =	7
??_LOCKABLECODE =   8

?_LCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_LCODE>
?_ICODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_ICODE>
?_PCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_PCODE>
?_SCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_SCODE>
?_DBOCODE   equ <(??_CUR_CODE_SEG MOD 16) - ??_DBOCODE>
?_16ICODE   equ <(??_CUR_CODE_SEG MOD 16) - ??_16ICODE>
?_RCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_RCODE>
?_LOCKABLECODE	equ <(??_CUR_CODE_SEG MOD 16) - ??_LOCKABLECODE>

ifndef NO_SEGMENTS

;
;  SEGMENT definitions and order
;

IFDEF	MASM6
_FLAT	EQU FLAT
ELSE
_FLAT	EQU USE32
ENDIF

;*  32 bit locked code
_LTEXT	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_LTEXT	    ENDS

_TEXT	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_TEXT	    ENDS

;*  32 bit pageable code
_PTEXT	    SEGMENT DWORD PUBLIC _FLAT 'PCODE'
_PTEXT	    ENDS



MakeCodeSeg MACRO seglist, classname, grpname, iseg

    IRP segname,<seglist>   ;; For each name in the list

IFNB	<classname>
    segname	SEGMENT DWORD PUBLIC _FLAT "&classname&CODE"
ELSE
    segname	SEGMENT DWORD PUBLIC _FLAT "&segname&CODE"
ENDIF

IFB <iseg>
VxD_&&segname&&_CODE_SEG MACRO
segname  SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_PCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM
ELSE
VxD_&&segname&&_CODE_SEG MACRO
segname  SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + iseg
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM
ENDIF

VxD_&&segname&&_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
segname ENDS
	ENDM

segname     ENDS

IFNDEF BLD_COFF
IFNB	<grpname>
    _&grpname GROUP segname
ELSE
    _&&segname GROUP segname
ENDIF
ENDIF

    ENDM		;; End for each segment

    ENDM

MakeCodeSeg <LOCKABLE_BEGIN, LOCKABLE, LOCKABLE_END>, \
    LOCKABLE, LOCKABLE, ??_LOCKABLECODE
MakeCodeSeg INT21
MakeCodeSeg SYSEXIT
MakeCodeSeg RARE
MakeCodeSeg W16
MakeCodeSeg W32
MakeCodeSeg VMCREATE
MakeCodeSeg VMDESTROY
MakeCodeSeg THCREATE
MakeCodeSeg THDESTROY
MakeCodeSeg VMSUSPEND
MakeCodeSeg VMRESUME
MakeCodeSeg PNP
MakeCodeSeg DOSVM


;***	DefLockableCodeBegin - define beginning of lockable code
;
;   Defines a label with the given name to mark the beginning
;   of the lockable code area for this VxD.  In the debug version,
;   also defines a DWORD containing DFS_TEST_BLOCK so that
;   procedures in the lockable code segment defined with
;   BeginProc may call _Debug_Flags_Service with flags appropriate
;   to the code's current state.

DefLockableCodeBegin MACRO name, private
VxD_LOCKABLE_BEGIN_CODE_SEG
IFB <private>
    PUBLIC  name
ENDIF
name	LABEL	NEAR
VxD_LOCKABLE_BEGIN_CODE_ENDS
ifndef WIN31COMPAT
if DEBLEVEL
VxD_LOCKED_DATA_SEG
    PUBLIC name&_Debug_Flags
name&_Debug_Flags DD DFS_TEST_BLOCK
VxD_LOCKED_DATA_ENDS
??_debug_flags equ <name&_Debug_Flags>
endif
endif
    ENDM

;***	DefLockableCodeEnd - define end of lockable code
;
;   Defines a label with the given name to mark the end
;   of the lockable code area for this VxD.  By subtracting
;   the offset of the beginning label from the offset of
;   the ending label, the VxD may determine how many bytes
;   of memory to lock or unlock.

DefLockableCodeEnd MACRO name, private
VxD_LOCKABLE_END_CODE_SEG
IFB <private>
    PUBLIC  name
ENDIF
name	LABEL	NEAR
VxD_LOCKABLE_END_CODE_ENDS
    ENDM

;***	CodeLockFlags - declare locked code debug flags
;
;   This macro declares the locked code debug flags.

CodeLockFlags MACRO name
ifndef WIN31COMPAT
if DEBLEVEL
    ifndef name&_Debug_Flags
    VxD_LOCKED_DATA_SEG
	extrn	name&_Debug_Flags:dword
    VxD_LOCKED_DATA_ENDS
    ??_debug_flags equ <name&_Debug_Flags>
    endif
endif
endif
    ENDM

;***	MarkCodeLocked - signify that lockable code is locked
;
;   This macro clears DFS_TEST_BLOCK in the debug flags
;   DWORD.

MarkCodeLocked MACRO
ifndef WIN31COMPAT
if DEBLEVEL
ifdef ??_debug_flags
    pushfd
    and ??_debug_flags,NOT DFS_TEST_BLOCK
    popfd
endif
endif
endif
    ENDM

;***	MarkCodeUnlocked - signify that lockable code is unlocked
;
;   This macro sets DFS_TEST_BLOCK in the debug flags
;   DWORD.

MarkCodeUnlocked MACRO
ifndef WIN31COMPAT
if DEBLEVEL
ifdef ??_debug_flags
    pushfd
    or	??_debug_flags,DFS_TEST_BLOCK
    popfd
endif
endif
endif
    ENDM


;*  32 bit initialization code
_ITEXT	    SEGMENT DWORD PUBLIC _FLAT 'ICODE'
_ITEXT	    ENDS

;*  32 bit locked data
_LDATA	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_LDATA	    ENDS

_DATA	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_DATA	    ENDS

;*  32 bit pageable data
_PDATA	    SEGMENT DWORD PUBLIC _FLAT 'PDATA'
_PDATA	    ENDS

;*  32 Bit initialization data
_IDATA	    SEGMENT DWORD PUBLIC _FLAT 'ICODE'
_IDATA	    ENDS

;*  Created by C8
_BSS	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_BSS	    ENDS

CONST	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
CONST	    ENDS

_TLS	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_TLS	    ENDS

;*  32 Bit static code for DL-VxDs
_STEXT	    SEGMENT DWORD PUBLIC _FLAT 'SCODE'
_STEXT	    ENDS

;*  32 Bit static data for DL-VxDs
_SDATA	    SEGMENT DWORD PUBLIC _FLAT 'SCODE'
_SDATA	    ENDS

;*	dummy segment for IsDebugOnlyLoaded
_DBOSTART   SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DBOSTART   ENDS

;*	32 bit debug only code; loaded only if debugger is present
_DBOCODE    SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DBOCODE    ENDS

;*	32 bit debug only data; loaded only if debugger is present
_DBODATA    SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DBODATA    ENDS

if DEBLEVEL
;*  Start of 32 bit path coverage data
_PATHSTART  SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHSTART  ENDS

;*  32 bit path coverage data
_PATHDATA   SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHDATA   ENDS

;*  End of 32 bit path coverage data
_PATHEND    SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHEND    ENDS
endif

;*  16 bit code/data that is put into IGROUP automaticly
_16ICODE    SEGMENT WORD USE16 PUBLIC '16ICODE'
_16ICODE    ENDS

;*  Real Mode initialization code/data for devices
_RCODE	    SEGMENT WORD USE16 PUBLIC 'RCODE'
_RCODE	    ENDS

IFNDEF BLD_COFF
_LGROUP   GROUP _LTEXT, _TEXT, _LDATA, _DATA, _BSS, CONST, _TLS
_IGROUP   GROUP _ITEXT, _IDATA
_SGROUP   GROUP _STEXT, _SDATA
_DBOGROUP GROUP _DBOSTART, _DBOCODE, _DBODATA
IF DEBLEVEL
_PGROUP   GROUP _PATHSTART, _PATHDATA, _PATHEND
ENDIF
ENDIF

endif ; NO_SEGMENTS

    ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>


BeginDoc
;==============================================================================
; The following macros are used in defining the routines
;   in a VxD which are going to be registered with VMM as callable entry
;   points. Once registered, the entry points can be called by any other
;   devices via the "VxDCall" macro, defined below. In the comments below,
;   replace "VxD" with the appropriate device name.
;
;*******
;   In the VxD.INC file, put the following lines, replacing <function_name>
;   with an appropriate name describing the function of the routine.
;
;   Begin_Service_Table VxD[,<segname>]
;   VxD_Service <function_name>[,<local segname>]
;   VxD_Service <function_name>[,<local segname>]
;	. . .
;   VxD_Service <function_name>[,<local segname>]
;   End_Service_Table	VxD[,<segname>]
;
;   Note that <segname> is an optional argument and, if specified, the
;   table is put in the segment defined by the macro "yyy_Data_Seg",
;   where yyy=segname. Otherwise the segment is defined by the
;   "VxD_Data_Seg" macro, defined below.
;   Note that <local segname> is an optional argument and, if specified,
;   the procedure's segment is defined by the macro "zzz_Code_Seg",
;   where zzz=segname. Otherwise the segment is defined by the
;   "VxD_Code_Seg" macro, defined below.
;
;*******
; One VxD module should have the following in order to define the entry points:
;Create_VxD_Service_Table = 1		; Only in module where table is
;   INCLUDE	VxD.INC 	; Include the table definition
;
;*******
; All modules that want to call the services defined in the table should include
;   VxD.INC, but not define the label "Create_VxD_Service_Table". This
;   will define the service names to be used with the VxDCall macro.
;
EndDoc

Begin_Service_Table MACRO Device_Name, Def_Segment

IFDEF	Device_Name&_Name_Based
 IFNDEF @@NextInternalID
    @@NextInternalID	= 0
 ENDIF
 @@NextInternalID = (@@NextInternalID + 1)
 Device_Name&_Internal_ID   = @@NextInternalID + BASEID_FOR_NAMEBASEDVXD
 DefineVxDName	Device_Name, %Device_Name&_Internal_ID
ENDIF

IFB <Def_Segment>
    BST2 Device_Name, VxD
ELSE
    BST2 Device_Name, Def_Segment
ENDIF
    ENDM

DefineVxDName	MACRO Device_Name, InternalID
 @@VxDName&InternalID EQU   <___&Device_Name&STable>
ENDM


BST2 MACRO Device_Name, Def_Segment

Num_&Device_Name&_Services = 0

IFDEF Create_&Device_Name&_Service_Table


Def_Segment&_LOCKED_DATA_SEG

Device_Name&_Service_Table LABEL DWORD

Device_Name&_Service MACRO Procedure, Local_Seg, Condition, StdCallBytes, fastcall
LOCAL $$&Procedure, extrnproc, tableproc

  extrnproc MACRO
    IFNB <fastcall>
      IFB <StdCallBytes>
	.err	;StdCallBytes required
      ENDIF
      EXTRN @&&Procedure&&@&&StdCallBytes:NEAR
    ELSE
      IFNB <StdCallBytes>
	EXTRN _&&Procedure&&@&&StdCallBytes:NEAR
      ELSE
	EXTRN Procedure:NEAR
      ENDIF
    ENDIF
    ENDM

  tableproc MACRO
    IFNB <fastcall>
      dd  OFFSET32 @&&Procedure&&@&&StdCallBytes
    ELSE
      IFNB <StdCallBytes>
	dd  OFFSET32 _&&Procedure&&@&&StdCallBytes
      ELSE
	dd  OFFSET32 Procedure
      ENDIF
    ENDIF
    ENDM

  IFNB <Condition>
  $$&&Procedure MACRO extern
    IFDEF &Condition
      IFNB <extern>
	extrnproc
      ELSE
	tableproc
      ENDIF
    ELSE
      IFB <extern>
      dd      0
      ENDIF
    ENDIF
    ENDM
  ENDIF

  IFDIFI <Procedure>, <RESERVED>
    PUBLIC _&&Procedure
     IF1
    _&&Procedure LABEL DWORD
     IFNB <fastcall>
    PUBLIC __&&Procedure
     __&&Procedure LABEL DWORD
     ENDIF
     ENDIF
     IFDIFI <Local_Seg>, <LOCAL>
	IFNB <Local_Seg>
Local_Seg&&_SEG
       ELSE
Def_Segment&_CODE_SEG
	ENDIF
	IFNB <Condition>
    $$&&Procedure extern
       ELSE
	extrnproc
	ENDIF
	IFNB <Local_Seg>
Local_Seg&&_ENDS
	ELSE
Def_Segment&_CODE_ENDS
	ENDIF
     ENDIF
      IFNB <Condition>
    $$&&Procedure
      ELSE
	tableproc
      ENDIF

	  IFDEF Device_Name&_Name_Based
	@@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
	  ELSE
	@@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
	  ENDIF
  ELSE
    dd	0
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1
  IFNB <Condition>
    Purge $$&&Procedure
  ENDIF
    Purge extrnproc
    Purge tableproc
    ENDM

  Device_Name&_StdCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition, %Args*4
    ??_standardccall&&_Procedure = Args
    ENDM

  Device_Name&_FastCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition, %Args*4, TRUE
    ??_fastcall&&_Procedure = Args
    ENDM

ELSE

; Local_Seg and Condition are placeholders only in this form

IFDEF	Device_Name&_Name_Based

Device_Name&_Service MACRO Procedure, Local_Seg, Condition


  IFDIFI <Procedure>, <RESERVED>
    @@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

    ENDM
ELSE

Device_Name&_Service MACRO Procedure, Local_Seg, Condition

  IFDIFI <Procedure>, <RESERVED>
    @@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

    ENDM

ENDIF

  Device_Name&_StdCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition
    ??_standardccall_&&Procedure = Args
    ENDM

  Device_Name&_FastCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition
    ??_fastcall_&&Procedure = Args
    ENDM

ENDIF

    ENDM


;------------------------------------------------------------------------------

End_Service_Table MACRO Device_Name, Def_Segment

    PURGE   Device_Name&_Service

IFDEF Create_&Device_Name&_Service_Table

IFB <Def_Segment>
VxD_LOCKED_DATA_ENDS
ELSE
Def_Segment&_LOCKED_DATA_ENDS
ENDIF

ENDIF

    ENDM

GetVxDServiceOrdinal	macro	reg,service
    mov reg,@@&service
    endm

GetVxDServiceAddress	macro	reg,service
    mov reg,OFFSET32 service
    endm


;***	Begin_Win32_Services - begin defining Win32 Service Table
;
;   This macro is used to begin the definition of the Win32
;   Service table.  It is modelled after, but not identical
;   to, the Begin_Service_Table macro.	If the the special
;   symbol Create_Win32_Services is defined to be true, then
;   the actual table is emitted.  Otherwise, only the service
;   numbers are defined.
;
;   ENTRY   VxDName	- the name of the VxD; it is assumed
;		  that a corresponding Device_ID is
;		  also defined.
;   EXIT    The macro VxDName&_Win32_Sevice is defined; it
;	accepts a service name as its only parameter.
;	This macro is then used to define each service.

Begin_Win32_Services MACRO VxDName
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
    .errb <VxDName>, <VxD name missing>
    ??w32svcno = 0
if Create_Win32_Services
VxDName&_Win32_Services label dword
    dd	csvc&VxDName, 0
endif
    ??inw32svc = 1

    VxDName&_Win32_Service MACRO Name
	.erre ??inw32svc, <Missing Begin_Win32_Services>
    if Create_Win32_Services
	dd  OFFSET32 Name,cparm&&Name
    endif
	@32&&Name equ	((VxDName&_Device_ID SHL 16) + ??w32svcno)
	??w32svcno = ??w32svcno + 1
	ENDM
    ENDM


;***	End_Win32_Services - mark end of Win32 Service Table
;
;   This macro completes initialization of the Win32
;   Service table.
;
;   ENTRY   VxDName	- the same name passed to
;		  Begin_Win32_services

End_Win32_Services MACRO VxDName
    .errb <VxDName>, <VxD name misssing>
if Create_Win32_Services
    csvc&VxDName    equ ($ - VxDName&_Win32_Services)/8 - 1
endif
    ??inw32svc = 0
    PURGE VxDName&_Win32_Service
    ENDM


;***	Declare_Win32_Service - declare an external Win32 Service
;
;   This macro is used to declare a Win32 service that
;   is defined elsewhere, perhaps in a C module.
;
;   ENTRY   Name	- the service name
;	cParms	    - the number of DWORD parameters
;   EXIT    The name is defined as external

Declare_Win32_Service MACRO Name, cParms
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
if Create_Win32_Services
    ?merge  <Name>,,,,<EQU>,<_>,<Name>,<@>,%(cParms*4 + 8)
    ?merge  <cparm>,<Name>,,,<EQU>,<cParms>
VxD_CODE_SEG
    ?merge  <EXTRN>,,,,,<_>,<Name>,<@>,%(cParms*4 + 8),<:NEAR>
VxD_CODE_ENDS
endif
    ENDM


;***	Win32call - call a Win32 service from a ring 3 thunk
;
;   This macro is used to call a Win32 service from
;   a ring 3 thunk.  Note that control will not return
;   to the instruction following the call, but to the
;   instruction following the call to the thunk.
;
;   ENTRY   Service	- the name of the service
;	CallBack    - the fword containing the callback

Win32call MACRO Service, CallBack
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
ife Create_Win32_Services
    mov eax,@32&Service
ifdef IS_16
    movzx   esp,sp
endif
    call    fword ptr [CallBack]
ifdef DEBUG
    int 3
endif
endif
    ENDM
*/

/*XLATOFF*/
#define GetVxDServiceAddress(service)	service

#define VxDCall(service) \
    _asm _emit 0xcd \
    _asm _emit 0x20 \
    _asm _emit (GetVxDServiceOrdinal(service) & 0xff) \
    _asm _emit (GetVxDServiceOrdinal(service) >> 8) & 0xff \
    _asm _emit (GetVxDServiceOrdinal(service) >> 16) & 0xff \
    _asm _emit (GetVxDServiceOrdinal(service) >> 24) & 0xff \

#define VMMCall VxDCall

#define VxDJmp(service) \
    _asm _emit 0xcd \
    _asm _emit 0x20 \
    _asm _emit (GetVxDServiceOrdinal(service) & 0xff) \
    _asm _emit ((GetVxDServiceOrdinal(service) >> 8) & 0xff) | 0x80 \
    _asm _emit (GetVxDServiceOrdinal(service) >> 16) & 0xff \
    _asm _emit (GetVxDServiceOrdinal(service) >> 24) & 0xff \

#define VMMJmp	VxDJmp

#define SERVICE 	__cdecl
#define ASYNC_SERVICE	__cdecl
#define WIN32_SERVICE	void __stdcall

#ifndef FASTCALL
#define FASTCALL	__fastcall
#endif
/*XLATON*/

/* ASM
;******************************************************************************
;
;   Dword_Align -- Aligns code to dword boundry by inserting nops
;
;------------------------------------------------------------------------------

Dword_Align MACRO Seg_Name
    LOCAL segn
IFDEF MASM6
    align 4
ELSE
IFNB <Seg_Name>
    segn equ Seg_Name
ELSE
IFE ?_LCODE
    segn equ <_LTEXT>
ELSE
IFE ?_ICODE
    segn equ <_ITEXT>
ELSE
IFE ?_PCODE
    segn equ <_PTEXT>
ELSE
IFE ?_SCODE
    segn equ <_STEXT>
ELSE
.err <Dword_Align not supported>
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
IF (($-OFFSET segn:0) MOD 4)
db 4 - (($-OFFSET segn:0) MOD 4) DUP (90h)
ENDIF
ENDIF
	ENDM


BeginDoc
;******************************************************************************
;
;   Fatal_Error
;
;   DESCRIPTION:
;   This macro is used to crash Windows/386 when an unrecoverable error
;   is detected.  If Msg_Ptr is ommitted then no error message will be
;   displayed, otherwise Msg_Ptr is the address
;   when the
;
;   PARAMETERS:
;   Msg_Ptr (OPTIONAL) - Points to an ASCIIZ string to display.
;
;   EXIT:
;   To DOS (hopefully).  This macro never returns.
;
;==============================================================================
EndDoc

Fatal_Error MACRO Msg_Ptr, Exit_Flags
    pushad
IFB <Msg_Ptr>
    xor esi, esi
ELSE
    mov esi, Msg_Ptr
IFB <Exit_Flags>
    xor eax, eax
ELSE
    mov eax, Exit_Flags
ENDIF
ENDIF
    VMMCall Fatal_Error_Handler
    ENDM

EF_Hang_On_Exit     EQU     1h
*/


/******************************************************************************
 *
 *   The following are control block headers and flags of interest to VxDs.
 *
 *****************************************************************************/

struct cb_s {
    ULONG CB_VM_Status; 	/* VM status flags */
    ULONG CB_High_Linear;	/* Address of VM mapped high */
    ULONG CB_Client_Pointer;
    ULONG CB_VMID;
    ULONG CB_Signature;
};

#define VMCB_ID 0x62634D56	/* VMcb */

/*
 *  VM status indicates globally interesting VM states
 */

#define VMSTAT_EXCLUSIVE_BIT	0x00	/* VM is exclusive mode */
#define VMSTAT_EXCLUSIVE	(1L << VMSTAT_EXCLUSIVE_BIT)
#define VMSTAT_BACKGROUND_BIT	0x01	/* VM runs in background */
#define VMSTAT_BACKGROUND	(1L << VMSTAT_BACKGROUND_BIT)
#define VMSTAT_CREATING_BIT 0x02    /* In process of creating */
#define VMSTAT_CREATING 	(1L << VMSTAT_CREATING_BIT)
#define VMSTAT_SUSPENDED_BIT	0x03	/* VM not scheduled */
#define VMSTAT_SUSPENDED	(1L << VMSTAT_SUSPENDED_BIT)
#define VMSTAT_NOT_EXECUTEABLE_BIT 0x04 /* VM partially destroyed */
#define VMSTAT_NOT_EXECUTEABLE	(1L << VMSTAT_NOT_EXECUTEABLE_BIT)
#define VMSTAT_PM_EXEC_BIT  0x05    /* Currently in PM app */
#define VMSTAT_PM_EXEC		(1L << VMSTAT_PM_EXEC_BIT)
#define VMSTAT_PM_APP_BIT   0x06    /* PM app present in VM */
#define VMSTAT_PM_APP		(1L << VMSTAT_PM_APP_BIT)
#define VMSTAT_PM_USE32_BIT 0x07    /* PM app is 32-bit */
#define VMSTAT_PM_USE32 	(1L << VMSTAT_PM_USE32_BIT)
#define VMSTAT_VXD_EXEC_BIT 0x08    /* Call from VxD */
#define VMSTAT_VXD_EXEC 	(1L << VMSTAT_VXD_EXEC_BIT)
#define VMSTAT_HIGH_PRI_BACK_BIT 0x09	/* High pri background */
#define VMSTAT_HIGH_PRI_BACK	(1L << VMSTAT_HIGH_PRI_BACK_BIT)
#define VMSTAT_BLOCKED_BIT  0x0A    /* Blocked on semaphore */
#define VMSTAT_BLOCKED		(1L << VMSTAT_BLOCKED_BIT)
#define VMSTAT_AWAKENING_BIT	0x0B	/* Woke up after blocked */
#define VMSTAT_AWAKENING	(1L << VMSTAT_AWAKENING_BIT)
#define VMSTAT_PAGEABLEV86BIT	0x0C	/* part of V86 is pageable (PM app) */
#define VMSTAT_PAGEABLEV86_BIT	VMSTAT_PAGEABLEV86BIT
#define VMSTAT_PAGEABLEV86	(1L << VMSTAT_PAGEABLEV86BIT)
#define VMSTAT_V86INTSLOCKEDBIT 0x0D	/* Locked regardless of pager type */
#define VMSTAT_V86INTSLOCKED_BIT VMSTAT_V86INTSLOCKEDBIT
#define VMSTAT_V86INTSLOCKED	(1L << VMSTAT_V86INTSLOCKEDBIT)
#define VMSTAT_IDLE_TIMEOUT_BIT 0x0E	/* Scheduled by time-slicer */
#define VMSTAT_IDLE_TIMEOUT	(1L << VMSTAT_IDLE_TIMEOUT_BIT)
#define VMSTAT_IDLE_BIT 	0x0F	/* VM has released time slice */
#define VMSTAT_IDLE		(1L << VMSTAT_IDLE_BIT)
#define VMSTAT_CLOSING_BIT  0x10    /* Close_VM called for VM */
#define VMSTAT_CLOSING		(1L << VMSTAT_CLOSING_BIT)
#define VMSTAT_TS_SUSPENDED_BIT 0x11	/* VM suspended by */
#define VMSTAT_TS_SUSPENDED	(1L << VMSTAT_TS_SUSPENDED_BIT)
#define VMSTAT_TS_MAXPRI_BIT	0x12	/* this is fgd_pri 10,000 internally*/
#define VMSTAT_TS_MAXPRI	(1L << VMSTAT_TS_MAXPRI_BIT)

#define VMSTAT_USE32_MASK   (VMSTAT_PM_USE32 | VMSTAT_VXD_EXEC)

struct tcb_s {
    ULONG   TCB_Flags;		/* Thread status flags */
    ULONG   TCB_Reserved1;	/* Used internally by VMM */
    ULONG   TCB_Reserved2;	/* Used internally by VMM */
    ULONG   TCB_Signature;
    ULONG   TCB_ClientPtr;	/* Client registers of thread */
    ULONG   TCB_VMHandle;	/* VM that thread is part of */
    USHORT  TCB_ThreadId;	/* Unique Thread ID */
    USHORT  TCB_PMLockOrigSS;	    /* Original SS:ESP before lock stack */
    ULONG   TCB_PMLockOrigESP;
    ULONG   TCB_PMLockOrigEIP;	    /* Original CS:EIP before lock stack */
    ULONG   TCB_PMLockStackCount;
    USHORT  TCB_PMLockOrigCS;
    USHORT  TCB_PMPSPSelector;
    ULONG   TCB_ThreadType;	/* dword passed to VMMCreateThread */
    USHORT  TCB_pad1;		/* reusable; for dword align */
    UCHAR   TCB_pad2;		/* reusable; for dword align */
    UCHAR   TCB_extErrLocus;	    /* extended error Locus */
    USHORT  TCB_extErr; 	/* extended error Code */
    UCHAR   TCB_extErrAction;	    /*	    "   "   Action */
    UCHAR   TCB_extErrClass;	    /*	    "   "   Class */
    ULONG   TCB_extErrPtr;	/*	"   pointer */

};

typedef struct tcb_s TCB;
typedef TCB *PTCB;

#define SCHED_OBJ_ID_THREAD	    0x42434854	  // THCB in ASCII

/*
 *  Thread status indicates globally interesting thread states.
 *  Flags are for information only and must not be modified.
 */

#define THFLAG_SUSPENDED_BIT	    0x03   // Thread not scheduled
#define THFLAG_SUSPENDED		   (1L << THFLAG_SUSPENDED_BIT)
#define THFLAG_NOT_EXECUTEABLE_BIT  0x04   // Thread partially destroyed
#define THFLAG_NOT_EXECUTEABLE		   (1L << THFLAG_NOT_EXECUTEABLE_BIT)
#define THFLAG_THREAD_CREATION_BIT  0x08   // Thread in status nascendi
#define THFLAG_THREAD_CREATION		   (1L << THFLAG_THREAD_CREATION_BIT)
#define THFLAG_THREAD_BLOCKED_BIT   0x0A   // Blocked on semaphore
#define THFLAG_THREAD_BLOCKED		   (1L << THFLAG_THREAD_BLOCKED_BIT)
#define THFLAG_RING0_THREAD_BIT     0x1C   // thread runs only at ring 0
#define THFLAG_RING0_THREAD		   (1L << THFLAG_RING0_THREAD_BIT)
#define THFLAG_ASYNC_THREAD_BIT	    0x1F   // thread is asynchronous
#define THFLAG_ASYNC_THREAD	       	   (1L << THFLAG_ASYNC_THREAD_BIT)
#define THFLAG_CHARSET_BITS	0x10   // Default character set
#define THFLAG_CHARSET_MASK	   (3L << THFLAG_CHARSET_BITS)
#define THFLAG_ANSI	       (0L << THFLAG_CHARSET_BITS)
#define THFLAG_OEM	       (1L << THFLAG_CHARSET_BITS)
#define THFLAG_UNICODE		   (2L << THFLAG_CHARSET_BITS)
#define THFLAG_RESERVED 	   (3L << THFLAG_CHARSET_BITS)
#define THFLAG_EXTENDED_HANDLES_BIT 0x12   // Thread uses extended file handles
#define THFLAG_EXTENDED_HANDLES 	   (1L << THFLAG_EXTENDED_HANDLES_BIT)
/* the win32 loader opens win32 exes with this bit set to notify IFS
 * so a defragger won't move these files
 * the bit is turned off once the open completes.
 * file open flags are overloaded which is why this is here
 */
#define THFLAG_OPEN_AS_IMMOVABLE_FILE_BIT 0x13	 // File thus opened not moved
#define THFLAG_OPEN_AS_IMMOVABLE_FILE		 (1L << THFLAG_OPEN_AS_IMMOVABLE_FILE_BIT)

/*
 *   Protected mode application control blocks
 */
struct pmcb_s {
    ULONG PMCB_Flags;
    ULONG PMCB_Parent;
};

/*
 *  The reference data for fault error codes 1-5 (GSDVME_PRIVINST through
 *  GSDVME_INVALFLT) is a pointer to the following fault information structure.
 */
struct VMFaultInfo {
    ULONG VMFI_EIP;		// faulting EIP
    WORD  VMFI_CS;		// faulting CS
    WORD  VMFI_Ints;		// interrupts in service, if any
};

typedef struct VMFaultInfo *PVMFaultInfo;

/******************************************************************************
 *		V M M	S E R V I C E S
 ******************************************************************************/

/*XLATOFF*/
#define VMM_Service Declare_Service
#define VMM_StdCall_Service Declare_SCService
#define VMM_FastCall_Service Declare_SCService
#pragma warning (disable:4003)	    // turn off not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VMM, VMM)

VMM_Service (Get_VMM_Version, LOCAL)	// MUST REMAIN SERVICE 0!

VMM_Service (Get_Cur_VM_Handle)
VMM_Service (Test_Cur_VM_Handle)
VMM_Service (Get_Sys_VM_Handle)
VMM_Service (Test_Sys_VM_Handle)
VMM_Service (Validate_VM_Handle)

VMM_Service (Get_VMM_Reenter_Count)
VMM_Service (Begin_Reentrant_Execution)
VMM_Service (End_Reentrant_Execution)

VMM_Service (Install_V86_Break_Point)
VMM_Service (Remove_V86_Break_Point)
VMM_Service (Allocate_V86_Call_Back)
VMM_Service (Allocate_PM_Call_Back)

VMM_Service (Call_When_VM_Returns)

VMM_Service (Schedule_Global_Event)
VMM_Service (Schedule_VM_Event)
VMM_Service (Call_Global_Event)
VMM_Service (Call_VM_Event)
VMM_Service (Cancel_Global_Event)
VMM_Service (Cancel_VM_Event)
VMM_Service (Call_Priority_VM_Event)
VMM_Service (Cancel_Priority_VM_Event)

VMM_Service (Get_NMI_Handler_Addr)
VMM_Service (Set_NMI_Handler_Addr)
VMM_Service (Hook_NMI_Event)

VMM_Service (Call_When_VM_Ints_Enabled)
VMM_Service (Enable_VM_Ints)
VMM_Service (Disable_VM_Ints)

VMM_Service (Map_Flat)
VMM_Service (Map_Lin_To_VM_Addr)

//   Scheduler services

VMM_Service (Adjust_Exec_Priority)
VMM_Service (Begin_Critical_Section)
VMM_Service (End_Critical_Section)
VMM_Service (End_Crit_And_Suspend)
VMM_Service (Claim_Critical_Section)
VMM_Service (Release_Critical_Section)
VMM_Service (Call_When_Not_Critical)
VMM_Service (Create_Semaphore)
VMM_Service (Destroy_Semaphore)
VMM_Service (Wait_Semaphore)
VMM_Service (Signal_Semaphore)
VMM_Service (Get_Crit_Section_Status)
VMM_Service (Call_When_Task_Switched)
VMM_Service (Suspend_VM)
VMM_Service (Resume_VM)
VMM_Service (No_Fail_Resume_VM)
VMM_Service (Nuke_VM)
VMM_Service (Crash_Cur_VM)

VMM_Service (Get_Execution_Focus)
VMM_Service (Set_Execution_Focus)
VMM_Service (Get_Time_Slice_Priority)
VMM_Service (Set_Time_Slice_Priority)
VMM_Service (Get_Time_Slice_Granularity)
VMM_Service (Set_Time_Slice_Granularity)
VMM_Service (Get_Time_Slice_Info)
VMM_Service (Adjust_Execution_Time)
VMM_Service (Release_Time_Slice)
VMM_Service (Wake_Up_VM)
VMM_Service (Call_When_Idle)

VMM_Service (Get_Next_VM_Handle)

//   Time-out and system timer services

VMM_Service (Set_Global_Time_Out)
VMM_Service (Set_VM_Time_Out)
VMM_Service (Cancel_Time_Out)
VMM_Service (Get_System_Time)
VMM_Service (Get_VM_Exec_Time)

VMM_Service (Hook_V86_Int_Chain)
VMM_Service (Get_V86_Int_Vector)
VMM_Service (Set_V86_Int_Vector)
VMM_Service (Get_PM_Int_Vector)
VMM_Service (Set_PM_Int_Vector)

VMM_Service (Simulate_Int)
VMM_Service (Simulate_Iret)
VMM_Service (Simulate_Far_Call)
VMM_Service (Simulate_Far_Jmp)
VMM_Service (Simulate_Far_Ret)
VMM_Service (Simulate_Far_Ret_N)
VMM_Service (Build_Int_Stack_Frame)

VMM_Service (Simulate_Push)
VMM_Service (Simulate_Pop)

// Heap Manager

VMM_Service (_HeapAllocate)
VMM_Service (_HeapReAllocate)
VMM_Service (_HeapFree)
VMM_Service (_HeapGetSize)

/*ENDMACROS*/

/****************************************************
 *
 *   Flags for heap allocator calls
 *
 *   NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

//
// Flags affecting the returned block
//

#define HEAPZEROINIT        0x00000001
#define HEAPZEROREINIT      0x00000002
#define HEAPNOCOPY          0x00000004

//
// Alignment flags
//

#define HEAPALIGN_SHIFT     16
#define HEAPALIGN_MASK      0x000F0000

#define HEAPALIGN_4         0x00000000                // dword aligned
#define HEAPALIGN_8         0x00000000                // quadword aligned
#define HEAPALIGN_16        0x00000000                // paragraph aligned
#define HEAPALIGN_32        0x00010000                // etc.
#define HEAPALIGN_64        0x00020000
#define HEAPALIGN_128       0x00030000
#define HEAPALIGN_256       0x00040000
#define HEAPALIGN_512       0x00050000
#define HEAPALIGN_1K        0x00060000
#define HEAPALIGN_2K        0x00070000
#define HEAPALIGN_4K        0x00080000
#define HEAPALIGN_8K        0x00090000
#define HEAPALIGN_16K       0x000A0000
#define HEAPALIGN_32K       0x000B0000
#define HEAPALIGN_64K       0x000C0000
#define HEAPALIGN_128K      0x000D0000

//
// Flags indicating which system heap to use.  There are four bits reserved
// to identify the heap to use.  Four are currently defined by the system.
//

#define HEAPTYPESHIFT       8
#define HEAPTYPEMASK        0x00000700

#define HEAPLOCKEDHIGH      0x00000000
#define HEAPLOCKEDIFDP      0x00000100
#define HEAPSWAP            0x00000200
#define HEAPLOCKEDLOW       0x00000300
#define HEAPINIT            0x00000400  // will be automatically freed after
                                        // init complete
#define HEAPSYSVM           0x00000500

//
// other flags
//

#define HEAPCLEAN           0x00000800
#define HEAPCONTIG          0x00001000  // memory must be physically contiguous
#define HEAPFORGET          0x00002000  // this memory will never be freed

// Page Manager

/*MACROS*/
VMM_Service (_PageAllocate)
VMM_Service (_PageReAllocate)
VMM_Service (_PageFree)
VMM_Service (_PageLock)
VMM_Service (_PageUnLock)
VMM_Service (_PageGetSizeAddr)
VMM_Service (_PageGetAllocInfo)
VMM_Service (_GetFreePageCount)
VMM_Service (_GetSysPageCount)
VMM_Service (_GetVMPgCount)
VMM_Service (_MapIntoV86)
VMM_Service (_PhysIntoV86)
VMM_Service (_TestGlobalV86Mem)
VMM_Service (_ModifyPageBits)
VMM_Service (_CopyPageTable)
VMM_Service (_LinMapIntoV86)
VMM_Service (_LinPageLock)
VMM_Service (_LinPageUnLock)
VMM_Service (_SetResetV86Pageable)
VMM_Service (_GetV86PageableArray)
VMM_Service (_PageCheckLinRange)
VMM_Service (_PageOutDirtyPages)
VMM_Service (_PageDiscardPages)
/*ENDMACROS*/

/****************************************************
 *
 *  Flags for other page allocator calls
 *
 *  NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

#define PAGEZEROINIT		0x00000001
#define PAGEUSEALIGN		0x00000002
#define PAGECONTIG		0x00000004
#define PAGEFIXED		0x00000008
#define PAGEDEBUGNULFAULT	0x00000010
#define PAGEZEROREINIT		0x00000020
#define PAGENOCOPY		0x00000040
#define PAGELOCKED		0x00000080
#define PAGELOCKEDIFDP		0x00000100
#define PAGESETV86PAGEABLE	0x00000200
#define PAGECLEARV86PAGEABLE	0x00000400
#define PAGESETV86INTSLOCKED	0x00000800
#define PAGECLEARV86INTSLOCKED	0x00001000
#define PAGEMARKPAGEOUT 	0x00002000
#define PAGEPDPSETBASE		0x00004000
#define PAGEPDPCLEARBASE	0x00008000
#define PAGEDISCARD		0x00010000
#define PAGEPDPQUERYDIRTY	0x00020000
#define PAGEMAPFREEPHYSREG	0x00040000
#define PAGEPHYSONLY		0x04000000
//efine PAGEDONTUSE		0x08000000  // ;Internal
#define PAGENOMOVE		0x10000000
#define PAGEMAPGLOBAL		0x40000000
#define PAGEMARKDIRTY		0x80000000

/****************************************************
 *
 *	Flags for _PhysIntoV86,
 *	_MapIntoV86, and _LinMapIntoV86
 *
 ***************************************************/

#define MAPV86_IGNOREWRAP	0x00000001


// Informational services

/*MACROS*/
VMM_Service (_GetNulPageHandle)
VMM_Service (_GetFirstV86Page)
VMM_Service (_MapPhysToLinear)
VMM_Service (_GetAppFlatDSAlias)
VMM_Service (_SelectorMapFlat)
VMM_Service (_GetDemandPageInfo)
VMM_Service (_GetSetPageOutCount)
/*ENDMACROS*/

/*
 *  Flags bits for _GetSetPageOutCount
 */
#define GSPOC_F_GET 0x00000001

// Device VM page manager

/*MACROS*/
VMM_Service (Hook_V86_Page)
VMM_Service (_Assign_Device_V86_Pages)
VMM_Service (_DeAssign_Device_V86_Pages)
VMM_Service (_Get_Device_V86_Pages_Array)
VMM_Service (MMGR_SetNULPageAddr)

// GDT/LDT management

VMM_Service (_Allocate_GDT_Selector)
VMM_Service (_Free_GDT_Selector)
VMM_Service (_Allocate_LDT_Selector)
VMM_Service (_Free_LDT_Selector)
VMM_Service (_BuildDescriptorDWORDs)
VMM_Service (_GetDescriptor)
VMM_Service (_SetDescriptor)
/*ENDMACROS*/

/*
 *  Flag equates for _BuildDescriptorDWORDs
 */
#define BDDEXPLICITDPL	0x00000001

/*
 *  Flag equates for _Allocate_LDT_Selector
 */
#define ALDTSPECSEL 0x00000001

/*MACROS*/
VMM_Service (_MMGR_Toggle_HMA)
/*ENDMACROS*/

/*
 *  Flag equates for _MMGR_Toggle_HMA
 */
#define MMGRHMAPHYSICAL 0x00000001
#define MMGRHMAENABLE	0x00000002
#define MMGRHMADISABLE	0x00000004
#define MMGRHMAQUERY	0x00000008

/*MACROS*/
VMM_Service (Get_Fault_Hook_Addrs)
VMM_Service (Hook_V86_Fault)
VMM_Service (Hook_PM_Fault)
VMM_Service (Hook_VMM_Fault)
VMM_Service (Begin_Nest_V86_Exec)
VMM_Service (Begin_Nest_Exec)
VMM_Service (Exec_Int)
VMM_Service (Resume_Exec)
VMM_Service (End_Nest_Exec)

VMM_Service (Allocate_PM_App_CB_Area, VMM_ICODE)
VMM_Service (Get_Cur_PM_App_CB)

VMM_Service (Set_V86_Exec_Mode)
VMM_Service (Set_PM_Exec_Mode)

VMM_Service (Begin_Use_Locked_PM_Stack)
VMM_Service (End_Use_Locked_PM_Stack)

VMM_Service (Save_Client_State)
VMM_Service (Restore_Client_State)

VMM_Service (Exec_VxD_Int)

VMM_Service (Hook_Device_Service)

VMM_Service (Hook_Device_V86_API)
VMM_Service (Hook_Device_PM_API)

VMM_Service (System_Control)

//   I/O and software interrupt hooks

VMM_Service (Simulate_IO)
VMM_Service (Install_Mult_IO_Handlers)
VMM_Service (Install_IO_Handler)
VMM_Service (Enable_Global_Trapping)
VMM_Service (Enable_Local_Trapping)
VMM_Service (Disable_Global_Trapping)
VMM_Service (Disable_Local_Trapping)

//   Linked List Abstract Data Type Services

VMM_Service (List_Create)
VMM_Service (List_Destroy)
VMM_Service (List_Allocate)
VMM_Service (List_Attach)
VMM_Service (List_Attach_Tail)
VMM_Service (List_Insert)
VMM_Service (List_Remove)
VMM_Service (List_Deallocate)
VMM_Service (List_Get_First)
VMM_Service (List_Get_Next)
VMM_Service (List_Remove_First)
/*ENDMACROS*/

/*
 *   Flags used by List_Create
 */
#define LF_ASYNC_BIT	    0
#define LF_ASYNC	(1 << LF_ASYNC_BIT)
#define LF_USE_HEAP_BIT     1
#define LF_USE_HEAP	(1 << LF_USE_HEAP_BIT)
#define LF_ALLOC_ERROR_BIT  2
#define LF_ALLOC_ERROR	    (1 << LF_ALLOC_ERROR_BIT)
/*
 * Swappable lists must use the heap.
 */
#define LF_SWAP 	(LF_USE_HEAP + (1 << 3))

/******************************************************************************
 *  I N I T I A L I Z A T I O N   P R O C E D U R E S
 ******************************************************************************/

// Instance data manager

/*MACROS*/
VMM_Service (_AddInstanceItem)

// System structure data manager

VMM_Service (_Allocate_Device_CB_Area)
VMM_Service (_Allocate_Global_V86_Data_Area, VMM_ICODE)
VMM_Service (_Allocate_Temp_V86_Data_Area, VMM_ICODE)
VMM_Service (_Free_Temp_V86_Data_Area, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Flag bits for _Allocate_Global_V86_Data_Area
 */
#define GVDAWordAlign	    0x00000001
#define GVDADWordAlign	    0x00000002
#define GVDAParaAlign	    0x00000004
#define GVDAPageAlign	    0x00000008
#define GVDAInstance	    0x00000100
#define GVDAZeroInit	    0x00000200
#define GVDAReclaim	0x00000400
#define GVDAInquire	0x00000800
#define GVDAHighSysCritOK   0x00001000
#define GVDAOptInstance     0x00002000
#define GVDAForceLow	    0x00004000

/*
 *  Flag bits for _Allocate_Temp_V86_Data_Area
 */
#define TVDANeedTilInitComplete 0x00000001

// Initialization information calls (win.ini and environment parameters)

/*MACROS*/
VMM_Service (Get_Profile_Decimal_Int, VMM_ICODE)
VMM_Service (Convert_Decimal_String, VMM_ICODE)
VMM_Service (Get_Profile_Fixed_Point, VMM_ICODE)
VMM_Service (Convert_Fixed_Point_String, VMM_ICODE)
VMM_Service (Get_Profile_Hex_Int, VMM_ICODE)
VMM_Service (Convert_Hex_String, VMM_ICODE)
VMM_Service (Get_Profile_Boolean, VMM_ICODE)
VMM_Service (Convert_Boolean_String, VMM_ICODE)
VMM_Service (Get_Profile_String, VMM_ICODE)
VMM_Service (Get_Next_Profile_String, VMM_ICODE)
VMM_Service (Get_Environment_String, VMM_ICODE)
VMM_Service (Get_Exec_Path, VMM_ICODE)
VMM_Service (Get_Config_Directory, VMM_ICODE)
VMM_Service (OpenFile, VMM_ICODE)
/*ENDMACROS*/

// OpenFile, if called after init, must point EDI to a buffer of at least
// this size.

#define VMM_OPENFILE_BUF_SIZE	    260

/*MACROS*/
VMM_Service (Get_PSP_Segment, VMM_ICODE)
VMM_Service (GetDOSVectors, VMM_ICODE)
VMM_Service (Get_Machine_Info)
/*ENDMACROS*/

#define GMIF_80486_BIT	0x10
#define GMIF_80486  (1 << GMIF_80486_BIT)
#define GMIF_PCXT_BIT	0x11
#define GMIF_PCXT   (1 << GMIF_PCXT_BIT)
#define GMIF_MCA_BIT	0x12
#define GMIF_MCA    (1 << GMIF_MCA_BIT)
#define GMIF_EISA_BIT	0x13
#define GMIF_EISA   (1 << GMIF_EISA_BIT)
#define GMIF_CPUID_BIT	0x14
#define GMIF_CPUID  (1 << GMIF_CPUID_BIT)

// Following service is not restricted to initialization

/*MACROS*/
VMM_Service (GetSet_HMA_Info)
VMM_Service (Set_System_Exit_Code)

VMM_Service (Fatal_Error_Handler)
VMM_Service (Fatal_Memory_Error)

//   Called by VTD only

VMM_Service (Update_System_Clock)

/******************************************************************************
 *	    D E B U G G I N G	E X T E R N S
 ******************************************************************************/

VMM_Service (Test_Debug_Installed)	// Valid call in retail also

VMM_Service (Out_Debug_String)
VMM_Service (Out_Debug_Chr)
VMM_Service (In_Debug_Chr)
VMM_Service (Debug_Convert_Hex_Binary)
VMM_Service (Debug_Convert_Hex_Decimal)

VMM_Service (Debug_Test_Valid_Handle)
VMM_Service (Validate_Client_Ptr)
VMM_Service (Test_Reenter)
VMM_Service (Queue_Debug_String)
VMM_Service (Log_Proc_Call)
VMM_Service (Debug_Test_Cur_VM)

VMM_Service (Get_PM_Int_Type)
VMM_Service (Set_PM_Int_Type)

VMM_Service (Get_Last_Updated_System_Time)
VMM_Service (Get_Last_Updated_VM_Exec_Time)

VMM_Service (Test_DBCS_Lead_Byte)	// for DBCS Enabling
/*ENDMACROS*/

/* ASM
.errnz	@@Test_DBCS_Lead_Byte - 100D1h	 ; VMM service table changed above this service
*/

/*************************************************************************
 *************************************************************************
 * END OF 3.00 SERVICE TABLE MUST NOT SHUFFLE SERVICES BEFORE THIS POINT
 *  FOR COMPATIBILITY.
 *************************************************************************
 *************************************************************************/

/*MACROS*/
VMM_Service (_AddFreePhysPage, VMM_ICODE)
VMM_Service (_PageResetHandlePAddr)
VMM_Service (_SetLastV86Page, VMM_ICODE)
VMM_Service (_GetLastV86Page)
VMM_Service (_MapFreePhysReg)
VMM_Service (_UnmapFreePhysReg)
VMM_Service (_XchgFreePhysReg)
VMM_Service (_SetFreePhysRegCalBk, VMM_ICODE)
VMM_Service (Get_Next_Arena, VMM_ICODE)
VMM_Service (Get_Name_Of_Ugly_TSR, VMM_ICODE)
VMM_Service (Get_Debug_Options, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Flags for AddFreePhysPage
 */
#define AFPP_SWAPOUT	 0x0001 // physical memory that must be swapped out
				// and subsequently restored at system exit
/*
 *  Flags for PageChangePager
 */
#define PCP_CHANGEPAGER     0x1 // change the pager for the page range
#define PCP_CHANGEPAGERDATA 0x2 // change the pager data dword for the pages
#define PCP_VIRGINONLY	    0x4 // make the above changes to virgin pages only


/*
 *  Bits for the ECX return of Get_Next_Arena
 */
#define GNA_HIDOSLINKED  0x0002 // High DOS arenas linked when WIN386 started
#define GNA_ISHIGHDOS	 0x0004 // High DOS arenas do exist

/*MACROS*/
VMM_Service (Set_Physical_HMA_Alias, VMM_ICODE)
VMM_Service (_GetGlblRng0V86IntBase, VMM_ICODE)
VMM_Service (_Add_Global_V86_Data_Area, VMM_ICODE)

VMM_Service (GetSetDetailedVMError)
/*ENDMACROS*/

/*
 *  Error code values for the GetSetDetailedVMError service. PLEASE NOTE
 *  that all of these error code values need to have bits set in the high
 *  word. This is to prevent collisions with other VMDOSAPP standard errors.
 *  Also, the low word must be non-zero.
 *
 *  First set of errors (high word = 0001) are intended to be used
 *  when a VM is CRASHED (VNE_Crashed or VNE_Nuked bit set on
 *  VM_Not_Executeable).
 *
 *  PLEASE NOTE that each of these errors (high word == 0001) actually
 *  has two forms:
 *
 *  0001xxxxh
 *  8001xxxxh
 *
 *  The device which sets the error initially always sets the error with
 *  the high bit CLEAR. The system will then optionally set the high bit
 *  depending on the result of the attempt to "nicely" crash the VM. This
 *  bit allows the system to tell the user whether the crash is likely or
 *  unlikely to destabalize the system.
 */
#define GSDVME_PRIVINST     0x00010001	/* Privledged instruction */
#define GSDVME_INVALINST    0x00010002	/* Invalid instruction */
#define GSDVME_INVALPGFLT   0x00010003	/* Invalid page fault */
#define GSDVME_INVALGPFLT   0x00010004	/* Invalid GP fault */
#define GSDVME_INVALFLT     0x00010005	/* Unspecified invalid fault */
#define GSDVME_USERNUKE     0x00010006	/* User requested NUKE of VM */
#define GSDVME_DEVNUKE	    0x00010007	/* Device specific problem */
#define GSDVME_DEVNUKEHDWR  0x00010008	/* Device specific problem:
			 *   invalid hardware fiddling
			 *   by VM (invalid I/O)
			 */
#define GSDVME_NUKENOMSG    0x00010009	/* Supress standard messages:
			 *   SHELL_Message used for
			 *   custom msg.
			 */
#define GSDVME_OKNUKEMASK   0x80000000	/* "Nice nuke" bit */

/*
 *  Second set of errors (high word = 0002) are intended to be used
 *  when a VM start up is failed (VNE_CreateFail, VNE_CrInitFail, or
 *  VNE_InitFail bit set on VM_Not_Executeable).
 */
#define GSDVME_INSMEMV86    0x00020001	/* base V86 mem    - V86MMGR */
#define GSDVME_INSV86SPACE  0x00020002	/* Kb Req too large - V86MMGR */
#define GSDVME_INSMEMXMS    0x00020003	/* XMS Kb Req	   - V86MMGR */
#define GSDVME_INSMEMEMS    0x00020004	/* EMS Kb Req	   - V86MMGR */
#define GSDVME_INSMEMV86HI  0x00020005	/* Hi DOS V86 mem   - DOSMGR
			 *	     V86MMGR
			 */
#define GSDVME_INSMEMVID    0x00020006	/* Base Video mem   - VDD */
#define GSDVME_INSMEMVM     0x00020007	/* Base VM mem	   - VMM
			 *   CB, Inst Buffer
			 */
#define GSDVME_INSMEMDEV    0x00020008	/* Couldn't alloc base VM
			 * memory for device.
			 */
#define GSDVME_CRTNOMSG     0x00020009	/* Supress standard messages:
			 *   SHELL_Message used for
			 *   custom msg.
			 */

/*MACROS*/
VMM_Service (Is_Debug_Chr)

//   Mono_Out services

VMM_Service (Clear_Mono_Screen)
VMM_Service (Out_Mono_Chr)
VMM_Service (Out_Mono_String)
VMM_Service (Set_Mono_Cur_Pos)
VMM_Service (Get_Mono_Cur_Pos)
VMM_Service (Get_Mono_Chr)

//   Service locates a byte in ROM

VMM_Service (Locate_Byte_In_ROM, VMM_ICODE)

VMM_Service (Hook_Invalid_Page_Fault)
VMM_Service (Unhook_Invalid_Page_Fault)
/*ENDMACROS*/

/*
 *  Flag bits of IPF_Flags
 */
#define IPF_PGDIR   0x00000001	/* Page directory entry not-present */
#define IPF_V86PG   0x00000002	/* Unexpected not present Page in V86 */
#define IPF_V86PGH  0x00000004	/* Like IPF_V86PG at high linear */
#define IPF_INVTYP  0x00000008	/* page has invalid not present type */
#define IPF_PGERR   0x00000010	/* pageswap device failure */
#define IPF_REFLT   0x00000020	/* re-entrant page fault */
#define IPF_VMM     0x00000040	/* Page fault caused by a VxD */
#define IPF_PM	    0x00000080	/* Page fault by VM in Prot Mode */
#define IPF_V86     0x00000100	/* Page fault by VM in V86 Mode */

/*MACROS*/
VMM_Service (Set_Delete_On_Exit_File)

VMM_Service (Close_VM)
/*ENDMACROS*/

/*
 *   Flags for Close_VM service
 */

#define CVF_CONTINUE_EXEC_BIT	0
#define CVF_CONTINUE_EXEC   (1 << CVF_CONTINUE_EXEC_BIT)

/*MACROS*/
VMM_Service (Enable_Touch_1st_Meg)	// Debugging only
VMM_Service (Disable_Touch_1st_Meg)	// Debugging only

VMM_Service (Install_Exception_Handler)
VMM_Service (Remove_Exception_Handler)

VMM_Service (Get_Crit_Status_No_Block)
/*ENDMACROS*/

/* ASM
; Check if VMM service table has changed above this service
.errnz	 @@Get_Crit_Status_No_Block - 100F1h
*/

#ifdef WIN40SERVICES

/*************************************************************************
 *************************************************************************
 *
 * END OF 3.10 SERVICE TABLE MUST NOT SHUFFLE SERVICES BEFORE THIS POINT
 *  FOR COMPATIBILITY.
 *************************************************************************
 *************************************************************************/

/*MACROS*/
VMM_Service (_GetLastUpdatedThreadExecTime)

VMM_Service (_Trace_Out_Service)
VMM_Service (_Debug_Out_Service)
VMM_Service (_Debug_Flags_Service)
/*ENDMACROS*/

#endif /* WIN40SERVICES */


/*
 *   Flags for _Debug_Flags_Service service.
 *
 *   Don't change these unless you really really know what you're doing.
 *   We need to define these even if we are in WIN31COMPAT mode.
 */

#define DFS_LOG_BIT	    0
#define DFS_LOG 	    (1 << DFS_LOG_BIT)
#define DFS_PROFILE_BIT 	1
#define DFS_PROFILE	    (1 << DFS_PROFILE_BIT)
#define DFS_TEST_CLD_BIT	2
#define DFS_TEST_CLD		(1 << DFS_TEST_CLD_BIT)
#define DFS_NEVER_REENTER_BIT	    3
#define DFS_NEVER_REENTER	(1 << DFS_NEVER_REENTER_BIT)
#define DFS_TEST_REENTER_BIT	    4
#define DFS_TEST_REENTER	(1 << DFS_TEST_REENTER_BIT)
#define DFS_NOT_SWAPPING_BIT	    5
#define DFS_NOT_SWAPPING	(1 << DFS_NOT_SWAPPING_BIT)
#define DFS_TEST_BLOCK_BIT	6
#define DFS_TEST_BLOCK		(1 << DFS_TEST_BLOCK_BIT)

#define DFS_RARE_SERVICES   0xFFFFFF80

#define DFS_EXIT_NOBLOCK	(DFS_RARE_SERVICES+0)
#define DFS_ENTER_NOBLOCK	(DFS_RARE_SERVICES+DFS_TEST_BLOCK)

#define DFS_TEST_NEST_EXEC  (DFS_RARE_SERVICES+1)

#ifdef WIN40SERVICES

/*MACROS*/
VMM_Service (VMMAddImportModuleName)

VMM_Service (VMM_Add_DDB)
VMM_Service (VMM_Remove_DDB)

VMM_Service (Test_VM_Ints_Enabled)
VMM_Service (_BlockOnID)

VMM_Service (Schedule_Thread_Event)
VMM_Service (Cancel_Thread_Event)
VMM_Service (Set_Thread_Time_Out)
VMM_Service (Set_Async_Time_Out)

VMM_Service (_AllocateThreadDataSlot)
VMM_Service (_FreeThreadDataSlot)
/*ENDMACROS*/

/*
 *  Flag equates for _CreateMutex
 */
#define MUTEX_MUST_COMPLETE	1L
#define MUTEX_NO_CLEANUP_THREAD_STATE	2L

/*MACROS*/
VMM_Service (_CreateMutex)

VMM_Service (_DestroyMutex)
VMM_Service (_GetMutexOwner)
VMM_Service (Call_When_Thread_Switched)

VMM_Service (VMMCreateThread)
VMM_Service (_GetThreadExecTime)
VMM_Service (VMMTerminateThread)

VMM_Service (Get_Cur_Thread_Handle)
VMM_Service (Test_Cur_Thread_Handle)
VMM_Service (Get_Sys_Thread_Handle)
VMM_Service (Test_Sys_Thread_Handle)
VMM_Service (Validate_Thread_Handle)
VMM_Service (Get_Initial_Thread_Handle)
VMM_Service (Test_Initial_Thread_Handle)
VMM_Service (Debug_Test_Valid_Thread_Handle)
VMM_Service (Debug_Test_Cur_Thread)

VMM_Service (VMM_GetSystemInitState)

VMM_Service (Cancel_Call_When_Thread_Switched)
VMM_Service (Get_Next_Thread_Handle)
VMM_Service (Adjust_Thread_Exec_Priority)

VMM_Service (_Deallocate_Device_CB_Area)
VMM_Service (Remove_IO_Handler)
VMM_Service (Remove_Mult_IO_Handlers)
VMM_Service (Unhook_V86_Int_Chain)
VMM_Service (Unhook_V86_Fault)
VMM_Service (Unhook_PM_Fault)
VMM_Service (Unhook_VMM_Fault)
VMM_Service (Unhook_Device_Service)

VMM_Service (_PageReserve)
VMM_Service (_PageCommit)
VMM_Service (_PageDecommit)
VMM_Service (_PagerRegister)
VMM_Service (_PagerQuery)
VMM_Service (_PagerDeregister)
VMM_Service (_ContextCreate)
VMM_Service (_ContextDestroy)
VMM_Service (_PageAttach)
VMM_Service (_PageFlush)
VMM_Service (_SignalID)
VMM_Service (_PageCommitPhys)

VMM_Service (_Register_Win32_Services)

VMM_Service (Cancel_Call_When_Not_Critical)
VMM_Service (Cancel_Call_When_Idle)
VMM_Service (Cancel_Call_When_Task_Switched)

VMM_Service (_Debug_Printf_Service)
VMM_Service (_EnterMutex)
VMM_Service (_LeaveMutex)
VMM_Service (Simulate_VM_IO)
VMM_Service (Signal_Semaphore_No_Switch)

VMM_Service (_ContextSwitch)
VMM_Service (_PageModifyPermissions)
VMM_Service (_PageQuery)

VMM_Service (_EnterMustComplete)
VMM_Service (_LeaveMustComplete)
VMM_Service (_ResumeExecMustComplete)
/*ENDMACROS*/

/*
 *  Flag equates for _GetThreadTerminationStatus
 */
#define THREAD_TERM_STATUS_CRASH_PEND	    1L
#define THREAD_TERM_STATUS_NUKE_PEND	    2L
#define THREAD_TERM_STATUS_SUSPEND_PEND     4L

/*MACROS*/
VMM_Service (_GetThreadTerminationStatus)
VMM_Service (_GetInstanceInfo)
/*ENDMACROS*/

/*
 *  Return values for _GetInstanceInfo
 */
#define INSTINFO_NONE	0	/* no data instanced in range */
#define INSTINFO_SOME	1	/* some data instanced in range */
#define INSTINFO_ALL	2	/* all data instanced in range */

/*MACROS*/
VMM_Service (_ExecIntMustComplete)
VMM_Service (_ExecVxDIntMustComplete)

VMM_Service (Begin_V86_Serialization)

VMM_Service (Unhook_V86_Page)
VMM_Service (VMM_GetVxDLocationList)
VMM_Service (VMM_GetDDBList)
VMM_Service (Unhook_NMI_Event)

VMM_Service (Get_Instanced_V86_Int_Vector)
VMM_Service (Get_Set_Real_DOS_PSP)
/*ENDMACROS*/

#define GSRDP_Set   0x0001

/*MACROS*/
VMM_Service (Call_Priority_Thread_Event)
VMM_Service (Get_System_Time_Address)
VMM_Service (Get_Crit_Status_Thread)

VMM_Service (Get_DDB)
VMM_Service (Directed_Sys_Control)
/*ENDMACROS*/

// Registry APIs for VxDs
/*MACROS*/
VMM_Service (_RegOpenKey)
VMM_Service (_RegCloseKey)
VMM_Service (_RegCreateKey)
VMM_Service (_RegDeleteKey)
VMM_Service (_RegEnumKey)
VMM_Service (_RegQueryValue)
VMM_Service (_RegSetValue)
VMM_Service (_RegDeleteValue)
VMM_Service (_RegEnumValue)
VMM_Service (_RegQueryValueEx)
VMM_Service (_RegSetValueEx)
/*ENDMACROS*/

#ifndef REG_SZ	    // define only if not there already

#define REG_SZ	    0x0001
#define REG_BINARY  0x0003

#endif

#ifndef HKEY_LOCAL_MACHINE  // define only if not there already

#define HKEY_CLASSES_ROOT	0x80000000
#define HKEY_CURRENT_USER	0x80000001
#define HKEY_LOCAL_MACHINE	0x80000002
#define HKEY_USERS		0x80000003
#define HKEY_PERFORMANCE_DATA	0x80000004
#define HKEY_CURRENT_CONFIG	0x80000005
#define HKEY_DYN_DATA		0x80000006

#endif

/*MACROS*/
VMM_Service (_CallRing3)
VMM_Service (Exec_PM_Int)
VMM_Service (_RegFlushKey)
VMM_Service (_PageCommitContig)
VMM_Service (_GetCurrentContext)

VMM_Service (_LocalizeSprintf)
VMM_Service (_LocalizeStackSprintf)

VMM_Service (Call_Restricted_Event)
VMM_Service (Cancel_Restricted_Event)

VMM_Service (Register_PEF_Provider, VMM_ICODE)

VMM_Service (_GetPhysPageInfo)

VMM_Service (_RegQueryInfoKey)
VMM_Service (MemArb_Reserve_Pages)
/*ENDMACROS*/

/*
 *  Return values for _GetPhysPageInfo
 */
#define PHYSINFO_NONE	0	/* no pages in the specified range exist */
#define PHYSINFO_SOME	1	/* some pages in the specified range exist */
#define PHYSINFO_ALL	2	/* all pages in the specified range exist */

// New timeslicer services
/*MACROS*/
VMM_Service (Time_Slice_Sys_VM_Idle)
VMM_Service (Time_Slice_Sleep)
VMM_Service (Boost_With_Decay)
VMM_Service (Set_Inversion_Pri)
VMM_Service (Reset_Inversion_Pri)
VMM_Service (Release_Inversion_Pri)
VMM_Service (Get_Thread_Win32_Pri)
VMM_Service (Set_Thread_Win32_Pri)
VMM_Service (Set_Thread_Static_Boost)
VMM_Service (Set_VM_Static_Boost)
VMM_Service (Release_Inversion_Pri_ID)
VMM_Service (Attach_Thread_To_Group)
VMM_Service (Detach_Thread_From_Group)
VMM_Service (Set_Group_Static_Boost)

VMM_Service (_GetRegistryPath, VMM_ICODE)
VMM_Service (_GetRegistryKey)
/*ENDMACROS*/

// TYPE definitions for _GetRegistryKey

#define REGTYPE_ENUM	0
#define REGTYPE_CLASS	1
#define REGTYPE_VXD	2

// Flag definitions for _GetRegistryKey
#define REGKEY_OPEN		    0
#define REGKEY_CREATE_IFNOTEXIST    1

// Flag definitions for _Assert_Range
#define ASSERT_RANGE_NULL_BAD	    0x00000000
#define ASSERT_RANGE_NULL_OK	    0x00000001
#define ASSERT_RANGE_IS_ASCIIZ	    0x00000002
#define ASSERT_RANGE_IS_NOT_ASCIIZ  0x00000000
#define ASSERT_RANGE_NO_DEBUG	    0x80000000
#define ASSERT_RANGE_BITS	    0x80000003

/*MACROS*/
VMM_Service (Cleanup_Thread_State)
VMM_Service (_RegRemapPreDefKey)
VMM_Service (End_V86_Serialization)
VMM_Service (_Assert_Range)
VMM_Service (_Sprintf)
VMM_Service (_PageChangePager)
VMM_Service (_RegCreateDynKey)
VMM_Service (_RegQueryMultipleValues)

// Additional timeslicer services
VMM_Service (Boost_Thread_With_VM)
/*ENDMACROS*/

// Flag definitions for Get_Boot_Flags

#define BOOT_CLEAN		0x00000001
#define BOOT_DOSCLEAN		0x00000002
#define BOOT_NETCLEAN		0x00000004
#define BOOT_INTERACTIVE	0x00000008

/*MACROS*/
VMM_Service (Get_Boot_Flags)
VMM_Service (Set_Boot_Flags)

// String and memory services
VMM_Service (_lstrcpyn)
VMM_Service (_lstrlen)
VMM_Service (_lmemcpy)

VMM_Service (_GetVxDName)

// For vwin32 use only
VMM_Service (Force_Mutexes_Free)
VMM_Service (Restore_Forced_Mutexes)
/*ENDMACROS*/

// Reclaimable low memory services
/*MACROS*/
VMM_Service (_AddReclaimableItem)
VMM_Service (_SetReclaimableItem)
VMM_Service (_EnumReclaimableItem)
/*ENDMACROS*/

// completely wake sys VM from idle state
/*MACROS*/
VMM_Service (Time_Slice_Wake_Sys_VM)
VMM_Service (VMM_Replace_Global_Environment)
VMM_Service (Begin_Non_Serial_Nest_V86_Exec)
VMM_Service (Get_Nest_Exec_Status)
/*ENDMACROS*/

// Bootlogging services

/*MACROS*/
VMM_Service (Open_Boot_Log)
VMM_Service (Write_Boot_Log)
VMM_Service (Close_Boot_Log)
VMM_Service (EnableDisable_Boot_Log)
VMM_Service (_Call_On_My_Stack)
/*ENDMACROS*/

// Another instance data service

/*MACROS*/
VMM_Service (Get_Inst_V86_Int_Vec_Base)
/*ENDMACROS*/

// Case insensitive functions -- SEE WARNINGS IN DOCS BEFORE USING!
/*MACROS*/
VMM_Service (_lstrcmpi)
VMM_Service (_strupr)
/*ENDMACROS*/

/*MACROS*/
VMM_Service (Log_Fault_Call_Out)
VMM_Service (_AtEventTime)
/*ENDMACROS*/

#endif /* WIN40SERVICES */

#ifdef WIN403SERVICES
//
// 4.03 Services
//

/*MACROS*/
VMM_Service (_PageOutPages)
/*ENDMACROS*/

// Flag definitions for _PageOutPages

#define PAGEOUT_PRIVATE 0x00000001
#define PAGEOUT_SHARED	0x00000002
#define PAGEOUT_SYSTEM	0x00000004
#define PAGEOUT_REGION	0x00000008
#define PAGEOUT_ALL	(PAGEOUT_PRIVATE | PAGEOUT_SHARED | PAGEOUT_SYSTEM)

/*MACROS*/
VMM_Service (_Call_On_My_Not_Flat_Stack)
VMM_Service (_LinRegionLock)
VMM_Service (_LinRegionUnLock)
VMM_Service (_AttemptingSomethingDangerous)
VMM_Service (_Vsprintf)
VMM_Service (_Vsprintfw)
VMM_Service (Load_FS_Service)
VMM_Service (Assert_FS_Service)
VMM_StdCall_Service (RtlUnwind, 4)
VMM_StdCall_Service (RtlRaiseException, 1)
VMM_StdCall_Service (RtlRaiseStatus, 1)

VMM_StdCall_Service (KeGetCurrentIrql, 0)
VMM_FastCall_Service (KfRaiseIrql, 1)
VMM_FastCall_Service (KfLowerIrql, 1)

VMM_Service (_Begin_Preemptable_Code)
VMM_Service (_End_Preemptable_Code)
VMM_FastCall_Service (Set_Preemptable_Count, 1)

VMM_StdCall_Service (KeInitializeDpc, 3)
VMM_StdCall_Service (KeInsertQueueDpc, 3)
VMM_StdCall_Service (KeRemoveQueueDpc, 1)

VMM_StdCall_Service (HeapAllocateEx, 4)
VMM_StdCall_Service (HeapReAllocateEx, 5)
VMM_StdCall_Service (HeapGetSizeEx, 2)
VMM_StdCall_Service (HeapFreeEx, 2)
//VMM_Service (_Get_CPUID_Flags)

/*ENDMACROS*/

#endif /* WIN403SERVICES */

/*MACROS*/
End_Service_Table(VMM, VMM)
/*ENDMACROS*/

/*XLATOFF*/
#pragma warning (default:4003)		// turn on not enough params warning

#ifndef try
#define try				__try
#define except				__except
#define finally 			__finally
#define leave				__leave
#define exception_code			__exception_code
#endif

#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER	1
#define EXCEPTION_CONTINUE_SEARCH	0
#define EXCEPTION_CONTINUE_EXECUTION	-1
#endif
/*XLATON*/

#define COMNFS_FLAT	0xFFFFFFFF

#define ASD_MAX_REF_DATA    64	    // If bigger than this, a checksum is used

struct	_vmmguid {
unsigned long Data1;
unsigned short Data2;
unsigned short Data3;
unsigned char Data4[8];
};

typedef struct _vmmguid VMMGUID;
typedef VMMGUID     *VMMREFIID;

typedef DWORD		ASD_RESULT;

#define ASD_ERROR_NONE	    0x00000000
#define ASD_CHECK_FAIL	    0x00000001	// The flag is set that this failed before
#define ASD_CHECK_SUCCESS   0x00000002	// The flag is set that this succeeded before
#define ASD_CHECK_UNKNOWN   0x00000003	// No flag is set
#define ASD_ERROR_BAD_TIME  0x00000004	// Under cli
#define ASD_REGISTRY_ERROR  0x00000005	// Unknown registry error
#define ASD_CLEAN_BOOT	    0x00000006	// Clean booting fails everything
#define ASD_OUT_OF_MEMORY   0x00000007	// Ran out of memory (extremely rare)
#define ASD_FILE_ERROR	    0x00000008	// Int 21 to flush the info file failed
#define ASD_ALREADY_SET     0x00000009	// ASD_CHECK* done twice on same vgOperation/pRefData
#define ASD_MISSING_CHECK   0x0000000A	// ASD_DONE* on something not set
#define ASD_BAD_PARAMETER   0x0000000B	// Invalid operation, refiid or ref pointer

#define ASD_OP_CHECK_AND_WRITE_FAIL_IF_UNKNOWN	0x00000000
#define ASD_OP_CHECK_AND_ALWAYS_WRITE_FAIL	0x00000001
#define ASD_OP_CHECK				0x00000002
#define ASD_OP_DONE_AND_SET_SUCCESS		0x00000003
#define ASD_OP_SET_FAIL 			0x00000004
#define ASD_OP_SET_SUCCESS			0x00000005
#define ASD_OP_SET_UNKNOWN			0x00000006
#define ASD_OP_DONE				0x00000007

// Flag definitions for _Add/_Set/_EnumReclaimableItem

#define RS_RECLAIM		0x00000001
#define RS_RESTORE		0x00000002
#define RS_DOSARENA		0x00000004

// Structure definition for _EnumReclaimableItem

struct ReclaimStruc {
    ULONG   RS_Linear;			// low (< 1meg) address of item
    ULONG   RS_Bytes;			// size of item in bytes
    ULONG   RS_CallBack;		// callback, if any (zero if none)
    ULONG   RS_RefData; 		// reference data for callback, if any
    ULONG   RS_HookTable;		// real-mode hook table (zero if none)
    ULONG   RS_Flags;			// 0 or more of the RS_* equates
};

typedef struct ReclaimStruc *PReclaimStruc;

//
// Structures for Force_Mutexes_Free/Restore_Forced_Mutexes
//
typedef struct frmtx {
    struct frmtx *frmtx_pfrmtxNext;
    DWORD frmtx_hmutex;
    DWORD frmtx_cEnterCount;
    DWORD frmtx_pthcbOwner;
    DWORD frmtx_htimeout;
} FRMTX;

typedef struct vmmfrinfo {
    struct frmtx vmmfrinfo_frmtxDOS;
    struct frmtx vmmfrinfo_frmtxV86;
    struct frmtx vmmfrinfo_frmtxOther;
} VMMFRINFO;

/*
 *  Data structure for _GetDemandPageInfo
 */
struct DemandInfoStruc {
    ULONG DILin_Total_Count;	/* # pages in linear address space */
    ULONG DIPhys_Count; 	/* Count of phys pages */
    ULONG DIFree_Count; 	/* Count of free phys pages */
    ULONG DIUnlock_Count;	/* Count of unlocked Phys Pages */
    ULONG DILinear_Base_Addr;	/* Base of pageable address space */
    ULONG DILin_Total_Free;	/* Total Count of free linear pages */

    /*
     *	The following 5 fields are all running totals, kept from the time
     *	the system was started
     */
    ULONG DIPage_Faults;	/* total page faults */
    ULONG DIPage_Ins;		/* calls to pagers to page in a page */
    ULONG DIPage_Outs;		/* calls to pagers to page out a page*/
    ULONG DIPage_Discards;	/* pages discarded w/o calling pager */
    ULONG DIInstance_Faults;	/* instance page faults */

    ULONG DIPagingFileMax;	/* maximum # of pages that could be in paging file */
    ULONG DIPagingFileInUse;	/* # of pages of paging file currently in use */

    ULONG DICommit_Count;	/* Total committed memory, in pages */

    ULONG DIReserved[2];	/* Reserved for expansion */
};

/*
 *  Data structure for _AddInstanceItem
 */
struct InstDataStruc {
    ULONG InstLinkF;	    /* INIT <0> RESERVED */
    ULONG InstLinkB;	    /* INIT <0> RESERVED */
    ULONG InstLinAddr;	    /* Linear address of start of block */
    ULONG InstSize;	    /* Size of block in bytes */
    ULONG InstType;	    /* Type of block */
};

/*
 *  Values for InstType
 */
#define INDOS_FIELD	0x100	/* Bit indicating INDOS switch requirements */
#define ALWAYS_FIELD	0x200	/* Bit indicating ALWAYS switch requirements */
#define OPTIONAL_FIELD	0x400	/* Bit indicating optional instancing requirements */

/*
 *  Data structure for Hook_Invalid_Page_Fault handlers.
 *
 *  This is the structure of the "invalid page fault information"
 *  which is pointed to by EDI when Invalid page fault hookers
 *  are called.
 *
 *  Page faults can occur on a VM which is not current by touching the VM at
 *  its high linear address.  In this case, IPF_FaultingVM may not be the
 *  current VM, it will be set to the VM whos high linear address was touched.
 */

struct IPF_Data {
    ULONG IPF_LinAddr;	    /* CR2 address of fault */
    ULONG IPF_MapPageNum;   /* Possible converted page # of fault */
    ULONG IPF_PTEEntry;     /* Contents of PTE that faulted */
    ULONG IPF_FaultingVM;   /* May not = Current VM (IPF_V86PgH set) */
    ULONG IPF_Flags;	    /* Flags */
};

/*
 *
 * Install_Exception_Handler data structure
 *
 */

struct Exception_Handler_Struc {
    ULONG EH_Reserved;
    ULONG EH_Start_EIP;
    ULONG EH_End_EIP;
    ULONG EH_Handler;
};

/*
 *  Flags passed in new memory manager functions
 */

/* PageReserve arena values */
#define PR_PRIVATE  0x80000400	/* anywhere in private arena */
#define PR_SHARED   0x80060000	/* anywhere in shared arena */
#define PR_SYSTEM   0x80080000	/* anywhere in system arena */

/* PageReserve flags */
#define PR_FIXED    0x00000008	/* don't move during PageReAllocate */
#define PR_4MEG     0x00000001	/* allocate on 4mb boundary */
#define PR_STATIC   0x00000010	/* see PageReserve documentation */

/* PageCommit default pager handle values */
#define PD_ZEROINIT 0x00000001	/* swappable zero-initialized pages */
#define PD_NOINIT   0x00000002	/* swappable uninitialized pages */
#define PD_FIXEDZERO	0x00000003  /* fixed zero-initialized pages */
#define PD_FIXED    0x00000004	/* fixed uninitialized pages */

/* PageCommit flags */
#define PC_FIXED    0x00000008	/* pages are permanently locked */
#define PC_LOCKED   0x00000080	/* pages are made present and locked*/
#define PC_LOCKEDIFDP	0x00000100  /* pages are locked if swap via DOS */
#define PC_WRITEABLE	0x00020000  /* make the pages writeable */
#define PC_USER     0x00040000	/* make the pages ring 3 accessible */
#define PC_INCR     0x40000000	/* increment "pagerdata" each page */
#define PC_PRESENT  0x80000000	/* make pages initially present */
#define PC_STATIC   0x20000000	/* allow commit in PR_STATIC object */
#define PC_DIRTY    0x08000000	/* make pages initially dirty */
#define PC_CACHEDIS 0x00100000  /* Allocate uncached pages - new for WDM */
#define PC_CACHEWT  0x00080000  /* Allocate write through cache pages - new for WDM */

/* PageCommitContig additional flags */
#define PCC_ZEROINIT	0x00000001  /* zero-initialize new pages */
#define PCC_NOLIN   0x10000000	/* don't map to any linear address */


/*MTRR type flags */
#define MTRR_UC 0
#define MTRR_WC 1
#define	MTRR_WT 4
#define	MTRR_WP 5
#define	MTRR_WB 6

/*
 *  Structure and flags for PageQuery
 */
#ifndef _WINNT_
typedef struct _MEMORY_BASIC_INFORMATION {
    ULONG mbi_BaseAddress;
    ULONG mbi_AllocationBase;
    ULONG mbi_AllocationProtect;
    ULONG mbi_RegionSize;
    ULONG mbi_State;
    ULONG mbi_Protect;
    ULONG mbi_Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

#define PAGE_NOACCESS	       0x01
#define PAGE_READONLY	       0x02
#define PAGE_READWRITE	       0x04
#define MEM_COMMIT	     0x1000
#define MEM_RESERVE	     0x2000
#define MEM_FREE	    0x10000
#define MEM_PRIVATE	    0x20000
#endif


/***ET+ PD - Pager Descriptor
 *
 *  A PD describes a set of routines to call to bring a page into
 *  the system or to get it out.  Each committed page in the system
 *  has an associated PD, a handle to which is stored in the page's
 *  VP.
 *
 *  For any field that is 0, the pager will not be notified
 *  when that action takes place.
 *
 *  For the purpose of pagers, a page can be in one of the two states
 *  describing its current contents:
 *
 *	clean - page has not been written to since its last page out
 *	dirty - page has been written to since its last page out
 *
 *  A page also is in one of two persistent states:
 *
 *	virgin - page has never been written to since it was committed
 *	tainted - page has been written to since it was committed
 *
 *  Note that a tainted page may be either dirty or clean, but a
 *  virgin page is by definition clean.
 *
 *  Examples of PDs:
 *
 *	For 32-bit EXE code or read-only data:
 *
 *	  pd_virginin = routine to load page from an exe file
 *	  pd_taintedin = 0
 *	  pd_cleanout = 0
 *	  pd_dirtyout = 0
 *	  pd_virginfree = 0
 *    pd_taintedfree = 0
 *    pd_dirty = 0
 *	  pd_type = PD_PAGERONLY
 *
 *	For 32-bit EXE writeable data:
 *
 *	  pd_virginin = routine to load page from an exe file
 *	  pd_taintedin = routine to load page from swap file
 *	  pd_cleanout = 0
 *	  pd_dirtyout = routine to write a page out to the swap file
 *	  pd_virginfree = 0
 *	  pd_taintedfree = routine to free page from the swap file
 *	  pd_dirty = routine to free page from the swap file
 *	  pd_type = PD_SWAPPER
 *
 *	For zero-initialized swappable data:
 *
 *	  pd_virginin = routine to zero-fill a page
 *	  pd_taintedin = routine to load page from swap file
 *	  pd_cleanout = 0
 *	  pd_dirtyout = routine to write a page out to the swap file
 *	  pd_virginfree = 0
 *	  pd_taintedfree = routine to free page from the swap file
 *	  pd_dirty = routine to free page from the swap file
 *	  pd_type = PD_SWAPPER
 */
/* typedefs for various pager functions */

typedef ULONG _cdecl FUNPAGE(PULONG ppagerdata, PVOID ppage, ULONG faultpage);

typedef FUNPAGE * PFUNPAGE;

struct pd_s {
    /*
     *	The following four fields are entry points in the pager which
     *	we call to page in or page out a page.	The following parameters
     *	are passed to the pager during these calls:
     *
     *	ppagerdata - pointer to the pager-specific dword of data
     *		 stored with the virtual page.	The pager is
     *		 free to modify the contents of this dword
     *		 DURING the page in or out, but not afterwards.
     *
     *	ppage - pointer to page going in or out (a ring 0 alias
     *	    to the physical page).  The pager should use this
     *	    address to access the contents of the page.
     *
     *	faultpage - faulting linear page number for page-ins, -1 for
     *		page-outs.  This address should not be accessed
     *		by the pager.  It is provided for information
     *		only.  Note that a single page can be mapped at
     *		more than one linear address because of the
     *		MapIntoV86 and LinMapIntoV86 services.
     *
     *	The pager should return non-0 if the page was successfully
     *	paged, or 0 if it failed.
     */
    PFUNPAGE pd_virginin;   /* in - while page has never been written to */
    PFUNPAGE pd_taintedin;  /* in - page written to at least once */
    PFUNPAGE pd_cleanout;   /* out - page not written to since last out */
    PFUNPAGE pd_dirtyout;   /* out - page was written to since last out */

    /*
     *	The pd_*free routines are used to inform the pager when the last
     *	reference to a virtual page controlled by the pager is
     *	decommitted.  A common use of this notification is to
     *	free space in a backing file, or write the page contents
     *	into the backing file.
     *
     *	These calls take the same parameters as the page-out and -in
     *	functions, but no return value is recognized.  The "ppage"
     *	and "faultpage" parameters will always be 0.
     */
    PFUNPAGE pd_virginfree;  /* decommit of never-written-to page */
    PFUNPAGE pd_taintedfree; /* decommit of page written to at least once*/

    /*
     *	The pd_dirty routine is used to inform the pager when the
     *	memory manager detects that a page has been written to.  The memory
     *	manager does not detect the write at the instant it occurs, so
     *	the pager should not depend upon prompt notification.  A common
     *	use of this notification might be to invalidate cached data.
     *	If the page was dirtied in more than one memory context,
     *	the pager's pd_dirty routine will be called once for each
     *	context.
     *
     *	These calls take the same parameters as the page-out and -in
     *	functions except that the "ppage" parameter isn't valid and
     *	no return value is recognized.
     */
    PFUNPAGE pd_dirty;

    /*
     *	The pd_type field gives the sytem information about the
     *	overcommit characteristics of pages controlled by this pager.
     *	The following are allowable values for the field:
     *
     *	PD_SWAPPER - under some conditions, pages of this type
     *	    may be paged out into the swap file
     *	PD_PAGERONLY - pages controlled by this pager will never
     *	    be paged out to the swap file
     *
     *	In addition, the following value may be or'ed in to the pd_type field:
     *
     *	PD_NESTEXEC - must be specified if either the pd_cleanout or pd_dirtyout
     *	    functions perform nested excecution or block using the
     *	    BLOCK_SVC_INTS flag.  To be safe, this flag should always be
     *	    specified if the pager does any sort of file i/o to anything
     *	    other than the default paging file.
     */
    ULONG pd_type;
};
typedef struct pd_s PD;
typedef PD * PPD;

/* values for pd_type */
#define PD_SWAPPER  0	/* pages need direct accounting in swap file */
#define PD_PAGERONLY	1   /* pages will never be swapped */
#define PD_NESTEXEC 2	/* page out funtion uses nested execution */

#endif // Not_VxD

/*
 *  The size of a page of memory
 */
#define PAGESHIFT   12
#define PAGESIZE    (1 << PAGESHIFT)
#define PAGEMASK    (PAGESIZE - 1)

#define PAGE(p) ((DWORD)(p) >> PAGESHIFT)
#define NPAGES(cb) (((DWORD)(cb) + PAGEMASK) >> PAGESHIFT)

/*
 *  Address space (arena) boundaries
 */
#define MAXSYSTEMLADDR	    ((ULONG) 0xffbfffff)    /* 4 gig - 4meg */
#define MINSYSTEMLADDR	    ((ULONG) 0xc0000000)    /* 3 gig */
#define MAXSHAREDLADDR	    ((ULONG) 0xbfffffff)
#define MINSHAREDLADDR	    ((ULONG) 0x80000000)    /* 2   gig */
#define MAXPRIVATELADDR     ((ULONG) 0x7fffffff)
#define MINPRIVATELADDR     ((ULONG) 0x00400000)    /* 4 meg */
#define MAXDOSLADDR	((ULONG) 0x003fffff)
#define MINDOSLADDR	((ULONG) 0x00000000)

#define MAXSYSTEMPAGE	    (MAXSYSTEMLADDR >> PAGESHIFT)
#define MINSYSTEMPAGE	    (MINSYSTEMLADDR >> PAGESHIFT)
#define MAXSHAREDPAGE	    (MAXSHAREDLADDR >> PAGESHIFT)
#define MINSHAREDPAGE	    (MINSHAREDLADDR >> PAGESHIFT)
#define MAXPRIVATEPAGE	    (MAXPRIVATELADDR >> PAGESHIFT)
#define MINPRIVATEPAGE	    (MINPRIVATELADDR >> PAGESHIFT)
#define MAXDOSPAGE	(MAXDOSLADDR >> PAGESHIFT)
#define MINDOSPAGE	(MINDOSLADDR >> PAGESHIFT)

#define CBPRIVATE	(1 + MAXPRIVATELADDR - MINPRIVATELADDR)
#define CBSHARED	(1 + MAXSHAREDLADDR - MINSHAREDLADDR)
#define CBSYSTEM	(1 + MAXSYSTEMLADDR - MINSYSTEMLADDR)
#define CBDOS		(1 + MAXDOSLADDR - MINDOSLADDR)

#define CPGPRIVATE	(1 + MAXPRIVATEPAGE - MINPRIVATEPAGE)
#define CPGSHARED	(1 + MAXSHAREDPAGE - MINSHAREDPAGE)
#define CPGSYSTEM	(1 + MAXSYSTEMPAGE - MINSYSTEMPAGE)
#define CPGDOS		(1 + MAXDOSPAGE - MINDOSPAGE)

/*XLATOFF*/
/*
 *  Largest object that could theoretically be allocated
 */
#define CBMAXALLOC	(max(CBSHARED,max(CBPRIVATE, CBSYSTEM)))
#define CPGMAXALLOC	(max(CPGSHARED,max(CPGPRIVATE, CPGSYSTEM)))

/*XLATON*/

/* ASM
IFDEF DEBUG
DebFar	EQU NEAR PTR
ELSE
DebFar	EQU SHORT
ENDIF
*/

#ifndef Not_VxD

/******************************************************************************
 *
 *	     EQUATES FOR SYSTEM_CONTROL CALLS
 *
 *****************************************************************************/

/*
 *  SYS_CRITICAL_INIT is a device init call.  Devices that have a
 *  critical function that needs initializing before interrupts are
 *  enabled should do it at Sys_Critical_Init.	Devices which REQUIRE a
 *  certain range of V86 pages to operate (such as the VDD video memory)
 *  should claim them at Sys_Critical_Init.  SYS VM Simulate_Int,
 *  Exec_Int ACTIVITY IS NOT ALLOWED.  Returning carry aborts device
 *  load only.
 */
#define SYS_CRITICAL_INIT   0x0000	/* Devices req'd for virt mode */

/*
 *  DEVICE_INIT is where most devices do the bulk of their initialization.
 *  SYS VM Simulate_Int, Exec_Int activity is allowed. Returning carry
 *  aborts device load only.
 */
#define DEVICE_INIT	0x0001	    /* All other devices init */

/*
 *  INIT_COMPLETE is the final phase of device init called just before the
 *  WIN386 INIT pages are released and the Instance snapshot is taken.
 *  Devices which wish to search for a region of V86 pages >= A0h to use
 *  should do it at INIT_COMPLETE.
 *  SYS VM Simulate_Int, Exec_Int activity is allowed.	Returning carry
 *  aborts device load only.
 */
#define INIT_COMPLETE	    0x0002	/* All devices initialized */

/* --------------- INITIALIZATION CODE AND DATA DISCARDED ------------------ */

/*
 *  Same as VM_Init, except for SYS VM.
 */
#define SYS_VM_INIT	0x0003	    /* Execute the system VM */

/*
 *  Same as VM_Terminate, except for SYS VM (Normal WIN386 exit ONLY, on a crash
 *  exit this call is not made).  SYS VM Simulate_Int, Exec_Int activity is
 *  allowed.  This and Sys_VM_Terminate2 are your last chances to access
 *  and/or lock pageable data.
 */
#define SYS_VM_TERMINATE    0x0004	/* System VM terminated */

/*
 *  System_Exit call is made when WIN386 is exiting either normally or via
 *  a crash.  INTERRUPTS ARE ENABLED.  Instance snapshot has been restored.
 *  SYS VM Simulate_Int, Exec_Int ACTIVITY IS NOT ALLOWED.
 */
#define SYSTEM_EXIT	0x0005	    /* Devices prepare to exit */

/*
 *  SYS_CRITICAL_EXIT call is made when WIN386 is exiting either normally or via
 *  a crash.  INTERRUPTS ARE DISABLED.	SYS VM Simulate_Int, Exec_Int ACTIVITY
 *   IS NOT ALLOWED.
 */
#define SYS_CRITICAL_EXIT   0x0006	/* System critical devices reset */


/*
 *  Create_VM creates a new VM.  EBX = VM handle of new VM.  Returning
 *  Carry will fail the Create_VM.
 */
#define CREATE_VM	0x0007

/*
 *  Second phase of Create_VM.	EBX = VM handle of new VM.  Returning
 *  Carry will cause the VM to go Not_Executeable, then be destroyed.
 *  VM Simulate_Int, Exec_Int activity is NOT allowed.
 */
#define VM_CRITICAL_INIT    0x0008

/*
 *  Third phase of Create_VM.  EBX = VM handle of new VM.  Returning
 *  Carry will cause the VM to go Not_Executeable, then be destroyed.
 *  VM Simulate_Int, Exec_Int activity is allowed.
 */
#define VM_INIT 	0x0009

/*
 *  NORMAL (First phase) of Destroy_VM.  EBX = VM Hanlde.  This occurs
 *  on normal termination of the VM.  Call cannot be failed.  VM
 *  Simulate_Int, Exec_Int activity is allowed.
 */
#define VM_TERMINATE	    0x000A	/* Still in VM -- About to die */

/*
 *  Second phase of Destroy_VM.  EBX = VM Handle, EDX = Flags (see
 *  below).  Note that in the case of destroying a running VM, this is
 *  the first call made (VM_Terminate call does not occur).  Call cannot
 *  be failed.	VM Simulate_Int, Exec_Int activity is NOT allowed.
 */
#define VM_NOT_EXECUTEABLE  0x000B	/* Most devices die (except VDD) */

/*
 *  Final phase of Destroy_VM.	EBX = VM Handle.  Note that considerable
 *  time can elaps between the VM_Not_Executeable call and this call.
 *  Call cannot be failed.  VM Simulate_Int, Exec_Int activity is NOT
 *  allowed.
 */
#define DESTROY_VM	0x000C	    /* VM's control block about to go */


/*
 *  Flags for VM_Not_Executeable control call (passed in EDX)
 */
#define VNE_CRASHED_BIT     0x00	/* VM was crashed */
#define VNE_CRASHED	(1 << VNE_CRASHED_BIT)
#define VNE_NUKED_BIT	    0x01	/* VM was destroyed while active */
#define VNE_NUKED	(1 << VNE_NUKED_BIT)
#define VNE_CREATEFAIL_BIT  0x02	/* Some device failed Create_VM */
#define VNE_CREATEFAIL	    (1 << VNE_CREATEFAIL_BIT)
#define VNE_CRINITFAIL_BIT  0x03	/* Some device failed VM_Critical_Init */
#define VNE_CRINITFAIL	    (1 << VNE_CRINITFAIL_BIT)
#define VNE_INITFAIL_BIT    0x04	/* Some device failed VM_Init */
#define VNE_INITFAIL	    (1 << VNE_INITFAIL_BIT)
#define VNE_CLOSED_BIT	    0x05
#define VNE_CLOSED	(1 << VNE_CLOSED_BIT)


/*
 *  EBX = VM Handle. Call cannot be failed.
 */
#define VM_SUSPEND	0x000D	    /* VM not runnable until resume */

/*
 *  EBX = VM Handle. Returning carry fails and backs out the resume.
 */
#define VM_RESUME	0x000E	    /* VM is leaving suspended state */


/*
 *  EBX = VM Handle to set device focus to.  EDX = Device ID if device
 *  specific setfocus, == 0 if device critical setfocus (all devices).
 *  THIS CALL CANNOT BE FAILED.
 *
 *  NOTE: In case where EDX == 0, ESI is a FLAG word that indicates
 *  special functions.	Currently Bit 0 being set indicates that this
 *  Device critical set focus is also "VM critical".  It means that we
 *  do not want some other VM to take the focus from this app now.  This
 *  is primarily used when doing a device critical set focus to Windows
 *  (the SYS VM) it is interpreted by the SHELL to mean "if an old app
 *  currently has the Windows activation, set the activation to the
 *  Windows Shell, not back to the old app".  ALSO in the case where Bit
 *  0 is set, EDI = The VM handle of the VM that is "having trouble".
 *  Set this to 0 if there is no specific VM associated with the
 *  problem.
 */
#define SET_DEVICE_FOCUS    0x000F


/*
 *  EBX = VM Handle going into message mode.  THIS CALL CANNOT BE FAILED.
 */
#define BEGIN_MESSAGE_MODE  0x0010

/*
 *  EBX = VM Handle leaving message mode.  THIS CALL CANNOT BE FAILED.
 */
#define END_MESSAGE_MODE    0x0011


/* ----------------------- SPECIAL CONTROL CALLS --------------------------- */

/*
 *  Request for reboot.  Call cannot be failed.
 */
#define REBOOT_PROCESSOR    0x0012	/* Request a machine reboot */

/*
 *  Query_Destroy is an information call made by the SHELL device before
 *  an attempt is made to initiate a destroy VM sequence on a running VM
 *  which has not exited normally.  EBX = VM Handle.  Returning carry
 *  indicates that a device "has a problem" with allowing this.  THE
 *  DESTROY SEQUENCE CANNOT BE ABORTED HOWEVER, this decision is up to
 *  the user.  All this does is indicate that there is a "problem" with
 *  allowing the destroy.  The device which returns carry should call
 *  the SHELL_Message service to post an informational dialog about the
 *  reason for the problem.
 */
#define QUERY_DESTROY	    0x0013	/* OK to destroy running VM? */


/* ----------------------- DEBUGGING CONTROL CALL -------------------------- */

/*
 *  Special call for device specific DEBUG information display and activity.
 */
#define DEBUG_QUERY	0x0014


/* -------- CALLS FOR BEGIN/END OF PROTECTED MODE VM EXECUTION ------------- */

/*
 *   About to run a protected mode application.
 *   EBX = Current VM handle.
 *   EDX = Flags
 *   EDI -> Application Control Block
 *   Returning with carry set fails the call.
 */
#define BEGIN_PM_APP	    0x0015

/*
 *  Flags for Begin_PM_App (passed in EDX)
 */
#define BPA_32_BIT	0x01
#define BPA_32_BIT_FLAG     1

/*
 *  Protected mode application is terminating.
 *  EBX = Current VM handle.  THIS CALL CAN NOT FAIL.
 *  EDI -> Application Control Block
 */
#define END_PM_APP	0x0016

/*
 *  Called whenever system is about to be rebooted.  Allows VxDs to clean
 *  up in preperation for reboot.
 */
#define DEVICE_REBOOT_NOTIFY	0x0017
#define CRIT_REBOOT_NOTIFY  0x0018

/*
 *  Called when VM is about to be terminated using the Close_VM service
 *  EBX = Current VM handle (Handle of VM to close)
 *  EDX = Flags
 *	  CVNF_CRIT_CLOSE = 1 if VM is in critical section while closing
 */
#define CLOSE_VM_NOTIFY     0x0019

#define CVNF_CRIT_CLOSE_BIT 0
#define CVNF_CRIT_CLOSE     (1 << CVNF_CRIT_CLOSE_BIT)

/*
 *  Power management event notification.
 *  EBX = 0
 *  ESI = event notification message
 *  EDI -> DWORD return value; VxD's modify the DWORD to return info, not EDI
 *  EDX is reserved
 */
#define POWER_EVENT	0x001A

#define SYS_DYNAMIC_DEVICE_INIT 0x001B
#define SYS_DYNAMIC_DEVICE_EXIT 0x001C

/*
 *  Create_THREAD creates a new thread.  EDI = handle of new thread.
 *  Returning Carry will fail the Create_THREAD. Message is sent in the
 *  context of the creating thread.
 *
 */
#define  CREATE_THREAD	0x001D

/*
 *  Second phase of creating a thread.	EDI = handle of new thread.  Call cannot
 *  be failed. VM Simulate_Int, Exec_Int activity is not allowed (because
 *  never allowed in non-initial threads). Message is sent in the context
 *  of the newly created thread.
 *
 */
#define  THREAD_INIT	0x001E

/*
 *  Normal (first) phase of Destroy_THREAD. EDI = handle of thread.
 *  This occurs on normal termination of the thread.  Call cannot be failed.
 *  Simulate_Int, Exec_Int activity is allowed.
 */
#define  TERMINATE_THREAD  0x001F

/*
 *  Second phase of Destroy_THREAD.  EDI = Handle of thread,
 *  EDX = flags (see below).  Note that in the case of destroying a
 *  running thread, this is the first call made (THREAD_Terminate call
 *  does not occur).  Call cannot be failed.  VM Simulate_Int, Exec_Int
 *  activity is NOT allowed.
 *
 */
#define  THREAD_Not_Executeable  0x0020

/*
 *  Final phase of Destroy_THREAD.  EDI = Thread Handle.  Note that considerable
 *  time can elapse between the THREAD_Not_Executeable call and this call.
 *  Call cannot be failed.  VM Simulate_Int, Exec_Int activity is NOT
 *  allowed.
 *
 */
#define  DESTROY_THREAD    0x0021

/* -------------------- CALLS FOR PLUG&PLAY ------------------------- */

/*
 *  Configuration manager or a devloader is telling a DLVxD that a new devnode
 *  has been created. EBX is the handle of the new devnode and EDX is the load
 *  type (one of the DLVxD_LOAD_* defined in CONFIGMG.H). This is a 'C'
 *  system control call. Contrarily to the other calls, carry flags must be
 *  set if any error code other than CR_SUCCESS is to be return.
 *
 */
#define PNP_NEW_DEVNODE     0x0022


/* -------------------- CALLS FOR Win32  ------------------------- */

/* vWin32 communicates with Vxds on behalf of Win32 apps thru this mechanism.
 * BUGBUG: need more doc here, describing the interface
 */

#define W32_DEVICEIOCONTROL 0x0023

/* sub-functions */
#define DIOC_GETVERSION     0x0
#define DIOC_OPEN	DIOC_GETVERSION
#define DIOC_CLOSEHANDLE    -1

/* -------------------- MORE SYSTEM CALLS ------------------------- */

/*
 * All these messages are sent immediately following the corresponding
 * message of the same name, except that the "2" messages are sent
 * in *reverse* init order.
 */

#define SYS_VM_TERMINATE2   0x0024
#define SYSTEM_EXIT2	    0x0025
#define SYS_CRITICAL_EXIT2  0x0026
#define VM_TERMINATE2	    0x0027
#define VM_NOT_EXECUTEABLE2 0x0028
#define DESTROY_VM2	0x0029
#define VM_SUSPEND2	0x002A
#define END_MESSAGE_MODE2   0x002B
#define END_PM_APP2	0x002C
#define DEVICE_REBOOT_NOTIFY2	0x002D
#define CRIT_REBOOT_NOTIFY2 0x002E
#define CLOSE_VM_NOTIFY2    0x002F

/*
 * VCOMM gets Address of Contention handler from VxDs by sending this
 * control message
 */

#define GET_CONTENTION_HANDLER	0x0030

#define KERNEL32_INITIALIZED	0x0031

#define KERNEL32_SHUTDOWN	0x0032

#define CREATE_PROCESS		0x0033
#define DESTROY_PROCESS 	0x0034

#define MAX_SYSTEM_CONTROL	0x0034

/*
 * Dynamic VxD's can communicate with each other using Directed_Sys_Control
 * and a private control message in the following range:
 */

#define BEGIN_RESERVED_PRIVATE_SYSTEM_CONTROL	0x70000000
#define END_RESERVED_PRIVATE_SYSTEM_CONTROL 0x7FFFFFFF

#endif // Not_VxD

/*
 * Values returned from VMM_GetSystemInitState in EAX.
 *
 * Comments represent operations performed by VMM; #define's indicate
 * what VMM_GetSystemInitState will return if you call it between the
 * previous operation and the next.
 *
 * Future versions of Windows may have additional init states between the
 * ones defined here, so you should be careful to use range checks instead
 * of test for equality.
 */

		    /* Protected mode is entered */
#define SYSSTATE_PRESYSCRITINIT     0x00000000
		    /* SYS_CRITICAL_INIT is broadcast */
#define SYSSTATE_PREDEVICEINIT	    0x10000000
		    /* DEVICE_INIT is broadcast */
#define SYSSTATE_PREINITCOMPLETE    0x20000000
		    /* INIT_COMPLETE is broadcast */
		    /* VxD initialization complete */
#define SYSSTATE_VXDINITCOMPLETED   0x40000000
		    /* KERNEL32_INITIALIZED is broadcast */
#define SYSSTATE_KERNEL32INITED     0x50000000
		    /* All initialization completed */
		    /* System running normally */
		    /* System shutdown initiated */
		    /* KERNEL32_SHUTDOWN is broadcast */
#define SYSSTATE_KERNEL32TERMINATED 0xA0000000
		    /* System shutdown continues */
#define SYSSTATE_PRESYSVMTERMINATE  0xB0000000
		    /* SYS_VM_TERMINATE is broadcast */
#define SYSSTATE_PRESYSTEMEXIT	    0xE0000000
		    /* SYSTEM_EXIT is broadcast */
#define SYSSTATE_PRESYSTEMEXIT2     0xE4000000
		    /* SYSTEM_EXIT2 is broadcast */
#define SYSSTATE_PRESYSCRITEXIT     0xF0000000
		    /* SYS_CRITICAL_EXIT is broadcast */
#define SYSSTATE_PRESYSCRITEXIT2    0xF4000000
		    /* SYS_CRITICAL_EXIT2 is broadcast */
#define SYSSTATE_POSTSYSCRITEXIT2   0xFFF00000
		    /* Return to real mode */
		    /* Alternate path: CAD reboot */
#define SYSSTATE_PREDEVICEREBOOT    0xFFFF0000
		    /* DEVICE_REBOOT_NOTIFY is broadcast */
#define SYSSTATE_PRECRITREBOOT	    0xFFFFF000
		    /* CRIT_REBOOT_NOTIFY is broadcast */
#define SYSSTATE_PREREBOOTCPU	    0xFFFFFF00
		    /* REBOOT_PROCESSOR is broadcast */
		    /* Return to real mode */

/* ASM
BeginDoc
;******************************************************************************
; BeginProc is a macro for defining entry points to routines in VMM and in the
;   VxDs. It correctly defines the procedure name for VxD services, DWORD
;   aligns the procedure, takes care of public declaration and does some
;   calling verification for debug versions of the software. EndProc is a
;   macro which defines the end of the procedure.
;
; Valid parameters to the BeginProc macro are:
;   PUBLIC		; Used outside this module (default)
;   LOCAL		; Local to this module
;   HIGH_FREQ		; DWORD align procedure
;   SERVICE		; Routine is called via VxDCall
;   ASYNC_SERVICE	    ; Same as "SERVICE" plus routine can
;		    ;	be called under interrupt.
;   HOOK_PROC		; Proc is a handler installed with
;		    ;	with a call to Hook_xxx_Fault
;		    ;	or Hook_Device_Service.  The
;		    ;	following parameter must be
;		    ;	the label of a DWORD location
;		    ;	which will hold the ptr to next
;		    ;	hook proc. e.g.
;
;		   ;BeginProc foo, SERVICE, HOOK_PROC, foo_next_ptr
;
;   NO_LOG		; Disable Queue_Out call logging
;   NO_PROFILE		; Disable DynaLink profile counts
;   NO_TEST_CLD 	; Disable direction flag check
;
;   TEST_BLOCK		; Trap if in NOBLOCK state
;		    ;  (default if in pageable code seg)
;   TEST_REENTER	    ; Trap if Get_VMM_Reenter_Count != 0
;		    ;  (default for non-async services)
;   NEVER_REENTER	    ; Trap if VMM has been reentered
;   NOT_SWAPPING	    ; Trap if this thread is swapping
;
;   NO_PROLOG		; Disable all prolog tests
;
;   ESP 	    ; Use ESP instead of EBP for stack
;		    ;  frame base
;   PCALL		; pascal calling convention
;   SCALL		; stdcall calling convention
;   FASTCALL		; stdcall, but first 2 parameters are passed in ECX & EDX
;   CCALL		; "C" calling convention
;   ICALL		; default calling convention
;   W32SVC		; Win32 service
;
;   segment type	    ; Place function in specified segment
;
; The NO_PROFILE flag merely suppresses incrementing the profile count.
; The DWORD of profiling information will still be emitted to appease
; the debugger.  If you want to increment the profile count manually,
; use the IncProfileCount macro.
;
; TEST_REENTER and NEVER_REENTER differ in that the VMM reentry count
; returned by Get_VMM_Reenter_Count is artifically forced to zero by
; Begin_Reentrant_Execution, whereas the counter checked by NEVER_REENTER
; reflects the genuine count of VMM reentry.
;
; A segment type (such as LOCKED, PAGEABLE, STATIC, INIT, DEBUG_ONLY) can be
; provided, in which case the BeginProc and EndProc macros will
; automatically place the appropriate segment directives around the
; definition of the function.
;
;   segment type	    ; Place function in specified segment
;
; After the routine header in which the routine entry conditions, exit
;   conditions, side affects and functionality are specified, the BeginProc
;   macro should be used to define the routine's entry point. It has up to
;   four parameters as specified below. For example:
;
;BeginProc  <Function_Name>,PUBLIC, HIGH_FREQ, SERVICE, ASYNC_SERVICE, ESP
;
;   <code>
;
;EndProc    <Function_Name>
;==============================================================================
EndDoc
;
; BeginProc handling takes place in the following phases:
;
;   Phase 1:  Parsing the arguments.
;   Phase 2:  Setting default flags.
;   Phase 3:  Combining the flags.
;   Phase 4:  Code emitted before the label
;   Phase 5:  Munge the name as exported to C/Pascal/whoever
;   Phase 6:  _Debug_Flags_Service prolog
;   Phase 7:  Code emitted after the label
;

??_pf_Check equ 1	;; Do Enter/LeaveProc checking?
??_pf_ArgsUsed	equ 2	    ;; ArgVars were used
??_pf_Entered	equ 4	    ;; EnterProc performed
??_pf_Left  equ 8	;; LeaveProc performed
??_pf_Returned	equ    16		;; Return performed

??_pushed	=	0		;; For WIN31COMPAT
??_align    =	0	;; For WIN31COMPAT
??_ends     equ <>	;; BeginProc segment

BeginProc macro Name, P1, P2, P3, P4, P5, P6, P7, LastArg
    local   Profile_Data, prelabeldata, ??_hookvar
    ??_frame = 0	    ;; local frame base
    ??_aframe = 0	    ;; argument frame base
    ??_taframe = 0	    ;; true argument frame base
    ??_initaframe = 0	    ;; initial aframe value
    ??_numargs = 0	    ;; number of argvars
    ??_numlocals = 0	    ;; number of localvars
    ??_numlocalsymbols = 0	;; number of local symbols
    ??_procflags = 0	    ;; misc. Enter/LeaveProc flags
    ??_esp = 0		;; if VMM_TRUE, use esp instead of ebp
    ??_pushed = 0	    ;; number of bytes pushed
    ??_align = 0	    ;; set if proc should be dword aligned
    ??_hook = 0 	;; set if proc is a Hook_Proc
    ??_hookarg = 0
    ??_service = 0
    ??_async_service = 0
IF DEBLEVEL GT DEBLEVELNORMAL
    ??_log = DFS_LOG	    ;; logging on by default
    ??_profile = DFS_PROFILE	;; service profiling on by default
    ??_test_cld = DFS_TEST_CLD	;; test that direction is clear
ELSE
    ??_log = 0		;; logging off
IFDEF DEBUG
IFDEF profileall
IF ?_ICODE
    ??_profile = DFS_PROFILE	;; service profiling on by default
ELSE
    ??_profile = 0	    ;; service profiling off
ENDIF
ELSE
    ??_profile = 0	    ;; service profiling off
ENDIF
ELSE
    ??_profile = 0	    ;; service profiling off
ENDIF
    ??_test_cld = 0	    ;; test that direction is clear
ENDIF
    ??_might_block = 0	    ;; entering fn might cause VM to block
    ??_test_reenter = 0     ;; don't test for VMM reentry
    ??_never_reenter = 0	;; don't test for genuine VMM reentry
    ??_not_swapping = 0     ;; don't test that we're not swapping
    ??_prolog_disabled = 0	;; use a prolog by default
    ??_public = 1	    ;; everything's public by default
    ??_cleanoff = 0	    ;; don't cleanoff parameters
    ??_ccall = 0
    ??_pcall = 0
    ??_scall = 0
    ??_fastcall = 0
    ??_w32svc = 0
    ??_fleave = FALSE
;   ??_dfs = 0		;; parm for _Debug_Flags_Service
    ??_name equ <Name>

    .errnb ??_ends, <Cannot nest functions with named segments>
    .errnb <LastArg>, <Too many arguments to BeginProc>

    ;; Phase 1: Parsing the arguments
    irp arg, <P1, P2, P3, P4, P5, P6, P7>
	if ??_hookarg
	??_hookarg = 0
	??_hookvar equ <arg>
	elseifdef ?&&arg&&_BeginProc
	    ?&&arg&&_BeginProc
	elseifdef VxD_&&arg&&_CODE_SEG
	??_ends textequ <VxD_&&arg&&_CODE_ENDS>
	VxD_&&arg&&_CODE_SEG
	else
	.err <Bad param "&arg" to BeginProc>
	endif
    endm

    ;; Phase 2:  Setting default flags
    ifndef Not_VxD
    ife ??_service
	ifndef profileall
	  ??_profile = 0      ;; only services can be profiled
	endif
	ifdef VMMSYS
	??_prolog_disabled = 1
	endif
    else
	??_test_cld = DFS_TEST_CLD
    endif	; ife ??_service

    ife ?_16ICODE
	??_prolog_disabled = 1
    else
    ife ?_RCODE 	    ;; if real-mode code segment
	??_prolog_disabled = 1	;; don't do anything stupid
    else		;; else protected mode code segment
	ife ?_PCODE	;; if swappable code
	??_might_block = DFS_TEST_BLOCK
	endif
	if ??_service
	ife ??_async_service
	    ??_test_reenter = DFS_TEST_REENTER
	endif
	endif
    endif	; ife ?_RCODE
    endif	; ife ?_16ICODE
    endif   ; Not_VxD

    if ??_esp
    ;; just return address on stack
    ??_basereg equ <esp+??_pushed>
    ??_initaframe = 4
    else
    ;; ret addr and EBP on stack
    ??_basereg equ <ebp>
    ??_initaframe = 8
    endif
    @Caller equ <dword ptr [??_basereg+??_initaframe-4]>

    ??_cleanoff = ??_pcall or ??_scall or ??_fastcall

    ;; Phase 3:  Combining the flags
    ??_dfs = ??_never_reenter + ??_test_reenter + ??_not_swapping + \
	 ??_log + ??_profile + ??_test_cld + ??_might_block

    if ??_prolog_disabled
	??_dfs = 0
    endif

    ;; Phase 4:  Pre-label code

    ifndef Not_VxD

    if ??_hook
	if ??_align
	Dword_Align
	endif
	prelabeldata:
	ifndef ??_hookvar
	.err <HOOK_PROC requires next arg to be name of dword location>
	endif
	jmp short Name
	jmp [??_hookvar]
	ifdef DEBUG
	Profile_Data dd  0
	endif
	if ??_align
	.errnz ($ - prelabeldata) mod 4
	endif
    endif

    ifdef DEBUG
	?prolog_&Name label near
	if (??_service OR ??_profile) AND (??_hook EQ 0)
	jmp short Name
	if ??_align
	Dword_Align	; This also aligns the proc
	endif		;   since Profile_Data is a dd

	IF ?_ICODE
	ifdef profileall
	  ?ProfileHeader_BeginProc Profile_Data, %@filename
	else
	  Profile_Data dd 0
	endif
	ELSE
	  Profile_Data dd 0
	ENDIF

	endif
    endif

    if ??_align
	Dword_Align
    endif

    endif   ; Not_VxD

    Name proc near	;; The label

    ;; Phase 5:  Munge the name as exported to C/Pascal/whoever
    ;;	     Warning!  Phase 5 cannot emit code!
    ife ??_pcall or ??_ccall or ??_scall or ??_fastcall    ;; if no munging
	if ??_public
	    public Name
	else
	    ifdef DEBUG
		% ?merge @FileName,$,Name,:
		% ?merge public,,,,,@FileName,$,Name
	    endif
	endif
    endif
    if ??_ccall
	if ??_public
	    _&Name equ Name
	ifdef Not_VxD
	 public C Name
	else
		 public _&Name
	endif
	endif
    endif
    if ??_pcall
	if ??_public
	    ?toupper Name
	    ?merge  public,,,,%?upper
	endif
    endif
    ;; Phase 6:  _Debug_Flags_Service prolog
    ;;	     DO NOT CHANGE UNTIL YOU UNDERSTAND _Debug_Flags_Service

    ife ??_scall or ??_fastcall
    ?_BeginProc_Debug_Prologue
    endif

    ;; Phase 7:  Post-label code
    ;;	     <none>
endm

?_BeginProc_Debug_Prologue MACRO
    ifndef Not_VxD
    ifdef DEBUG
	if ??_dfs EQ DFS_LOG
	VMMCall Log_Proc_Call	;; no test, just log
	else
	if ??_dfs EQ DFS_TEST_REENTER
	VMMCall Test_Reenter	;; no log, just reenter
	else
	if ??_dfs or ?_LOCKABLECODE eq 0
	ifdef WIN31COMPAT
	    if ??_dfs AND DFS_LOG
	    VMMCall Log_Proc_Call
	    endif
	    if ??_dfs AND DFS_TEST_REENTER
	    VMMCall Test_Reenter
	    endif
	else
	    ife ?_LOCKABLECODE
	    ifdef ??_debug_flags
	    push    ??_debug_flags
	    if ??_dfs
	    pushfd
	    or	dword ptr [esp+4],??_dfs
	    popfd
	    endif
	    VMMCall _Debug_Flags_Service
	    elseif ??_dfs
	    push    ??_dfs
	    VMMCall _Debug_Flags_Service
	    endif
	    else
	    push    ??_dfs
	    VMMCall _Debug_Flags_Service
	    endif
	endif
	else
	  ifdef profileall
	IncProfileCount
	  endif
	endif		;if ??_dfs
	endif		; if ??_dfs EQ DFS_TEST_REENTER
	endif		; if ??_dfs EQ DFS_LOG
    endif ; DEBUG
    endif ; Not_VxD
ENDM

;
; For each BeginProc keyword, there is a corresponding macro ?XX_BeginProc.
;
; The macro ?_BeginProc is so that the null keyword is not an error.

?_BeginProc macro
endm

?PUBLIC_BeginProc macro
    ??_public = 1
endm

?LOCAL_BeginProc macro
    ??_public = 0
endm

?HIGH_FREQ_BeginProc macro
    ??_align = 1
endm

?HOOK_PROC_BeginProc macro
    ??_hook = 1
    ??_hookarg = 1  ; next arg is dword storage location
endm

?SERVICE_BeginProc macro
    ??_service = 1
    .erre ?_16ICODE, <SERVICEs must be in 32 bit code>
    .erre ?_RCODE, <SERVICEs must be in 32 bit code>
endm

?ASYNC_SERVICE_BeginProc macro
    ??_service = 1
    ??_async_service = 1
    .errnz ?_LCODE, <ASYNC_SERVICE's must be in LOCKED code>
endm

?NO_LOG_BeginProc macro
    ??_log = 0
endm

?NO_PROFILE_BeginProc macro
    ??_profile = 0
endm

?NO_TEST_CLD_BeginProc macro
    ??_test_cld = 0
endm

?TEST_BLOCK_BeginProc macro
    ??_might_block = DFS_TEST_BLOCK
endm

?TEST_REENTER_BeginProc macro
    ??_test_reenter = DFS_TEST_REENTER
endm

?NEVER_REENTER_BeginProc macro
    ??_never_reenter = DFS_NEVER_REENTER
endm

?NOT_SWAPPING_BeginProc macro
    ??_not_swapping = DFS_NOT_SWAPPING
endm

?NO_PROLOG_BeginProc macro
    ??_prolog_disabled = 1
endm

?ESP_BeginProc macro
    ??_esp = VMM_TRUE
    ifndef Not_VxD
    .erre ?_16ICODE, <Beginproc ESP attribute invalid in 16 bit seg.>
    .erre ?_RCODE, <Beginproc ESP attribute invalid in real-mode seg.>
    endif
endm

?CCALL_BeginProc macro
    ??_ccall = 1
endm

?PCALL_BeginProc macro
    ??_pcall = 1
endm

?SCALL_BeginProc macro
    ??_scall = 1
endm

?FASTCALL_BeginProc macro
    ??_fastcall = 1
endm

?ICALL_BeginProc macro
    ??_scall = 1    ;; internal calling convention is StdCall
endm

?W32SVC_BeginProc macro
    ??_scall = 1
    ??_w32svc = 1
endm

ifdef DEBUG
ifdef profileall
?ProfileHeader_BeginProc macro PL, filename
ifndef _&filename&__proc_list
  _&filename&__proc_list = 0
  PUBLIC _&filename&__proc_list
endif
    dd OFFSET32 _&filename&__proc_list
PL  dd 0
_&filename&__proc_list = PL
endm
endif

IncProfileCount macro
    if ??_service OR ??_profile
	inc dword ptr [??_name-4]
    else
	ifndef profileall
	.err <IncProfileCount can be used only in services.>
	endif
    endif
endm
else
IncProfileCount macro
endm
endif

;***	ArgVar - declares stack arguments
;
; Usage:
;
;   name   = name of argument.
;   length = a numeric expression denoting the size (in bytes)
;	 of the argument.  The symbols BYTE, WORD, and DWORD
;	 are synonyms for 1, 2, and 4 respectively.
;	 NB!  All arguments sizes are rounded up to the nearest
;	 multiple of 4.
;   used   = usually blank, but can be the symbol NOTUSED
;	 to indicate that the argument will not be used
;	 by the procedure.
;

ArgVar	macro	name,length,used
    ??_numargs = ??_numargs + 1
    if ??_pcall
	?mkarg	<name>, <length>, <used>, %??_numargs
    else
	?arg <name>, <length>, <used>
    endif
    ??_procflags = ??_procflags OR ??_pf_Check
    endm

?mkarg	macro	name, length, used, num
    .xcref  ?MKA&num
    ?deflocal <name>
    ?MKA&num &macro
	?argvar <name>, <length>, <used>
	&endm
    ??_aframe = ??_aframe + 4
    endm
    .xcref  ?mkarg

?argvar macro	name,length,used
    local   a
    a = ??_taframe
    ??_aframe =  ??_aframe + 4
    ??_taframe =  ??_taframe + 4
    ifidni  <length>,<BYTE>
	?setname <name>, <byte ptr [??_basereg+??_initaframe+a]>, <used>
    elseifidni <length>,<WORD>
	?setname <name>, <word ptr [??_basereg+??_initaframe+a]>, <used>
    elseifidni <length>,<DWORD>
	?setname <name>,  <dword ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&l>,<word ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&ll>,<byte ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&lh>,<byte ptr [??_basereg+??_initaframe+a+1]>, <used>
	?setname <name&h>,<word ptr [??_basereg+??_initaframe+a+2]>, <used>
	?setname <name&hl>,<byte ptr [??_basereg+??_initaframe+a+2]>, <used>
	?setname <name&hh>,<byte ptr [??_basereg+??_initaframe+a+3]>, <used>
    else
	??_aframe =  ??_aframe - 4 + ((length + 3)/4)*4
	??_taframe =  ??_taframe - 4 + ((length + 3)/4)*4
	?setname <name>, <[??_basereg+??_initaframe+a]>, <used>
    endif
endm

?arg macro   name,length,used
  if ??_fastcall
    if ??_numargs le 2
      if length gt 4
	.err <First 2 parameters are dwords (ecx,edx) for fastcall functions>
      endif
      ??_aframe =  ??_aframe + 4
      if ??_numargs eq 1
	?merge ecx_,name,,,equ,ecx
      else
	?merge edx_,name,,,equ,edx
      endif
    else
      ?argvar name, length, used
    endif
  else
    ?argvar name, length, used
  endif
endm

;***	?setname - optionally creates the name of an ArgVar
;
;   If <used> is <NOTUSED>, then the name is defined to something
;   bogus.

?setname macro name, value, used
    ?deflocal <name>
    ifidni <used>, <NOTUSED>
	name equ _inaccessible_NOTUSED_
    else
	name equ value
	??_procflags = ??_procflags OR ??_pf_ArgsUsed OR ??_pf_Check
    endif
endm


;***	LocalVar - declares local stack variables
;
; Usage:
;
;   name   = name of local variable
;   length = a numeric expression denoting the size (in bytes)
;	 of the argument.  The symbols BYTE, WORD, and DWORD
;	 are synonyms for 1, 2, and 4 respectively.
;	 NB!  All arguments sizes are rounded up to the nearest
;	 multiple of 4 (unless PACK is indicated)
;   flag   = usually blank, but can be the symbol PACK
;	 to suppress the usual padding and aligning of variables
;	 PACK is typically used when declaring a bunch of
;	 byte or word variables.  Make sure that the total
;	 size of PACKed variables is a multiple of 4.
;

LocalVar    macro   name,length,flag
    local   a
    ??_numlocals = ??_numlocals + 1
    ??_pad = 1
    ifidni <flag>, <PACK>
	??_pad = 0
    endif
    ifidni  <length>,<BYTE>
	??_frame = ??_frame + 1 + 3 * ??_pad
	a = ??_frame
	?deflocal <name>
	name equ byte ptr [??_basereg-a]
    elseifidni <length>,<WORD>
	??_frame =  ??_frame + 2 + 2 * ??_pad
	a = ??_frame
	?deflocal <name>
	name equ word ptr [??_basereg-a]
    elseifidni <length>,<DWORD>
	??_frame = ??_frame + 4
	a = ??_frame
	?deflocal <name, name&l, name&ll, name&lh, name&h, name&hl, name&hh>
	name equ dword ptr [??_basereg-a]
	name&l equ word ptr [??_basereg-a]
	name&ll equ byte ptr [??_basereg-a]
	name&lh equ byte ptr [??_basereg-a+1]
	name&h equ word ptr [??_basereg-a+2]
	name&hl equ byte ptr [??_basereg-a+2]
	name&hh equ byte ptr [??_basereg-a+3]
    else
	??_frame =  ??_frame + ((length + 3)/4)*4
	a = ??_frame
	?deflocal <name>
	name equ [??_basereg-a]
    endif
    ??_procflags = ??_procflags OR ??_pf_Check
endm

?deflocal macro name
    irp nm, <name>
	??_numlocalsymbols = ??_numlocalsymbols + 1
	?dodeflocal <nm>, %(??_numlocalsymbols)
    endm
endm
    .xcref  ?deflocal

?dodeflocal macro name, num
    .xcref  ?LOC&num
    ?LOC&num &macro
	name	equ <__inaccessible__NOTINSCOPE__>
	&endm
    endm
    .xcref  ?dodeflocal

;***	EnterProc - generates stack frame on entry

EnterProc macro
    .errnz ??_frame and 3, <Total size of local variables not a multiple of 4.>
    if ??_scall
	if ??_public
	ifdef Not_VxD
		?merge	%??_name,@,%(??_aframe),,label,near
		?merge	public,,,,C,%??_name,@,%(??_aframe)
	else
		?merge	_,%??_name,@,%(??_aframe),label,near
		?merge	public,,,,,_,%??_name,@,%(??_aframe)
	endif
	endif
	?_BeginProc_Debug_Prologue
    endif
    if ??_fastcall
	if ??_public
	ifdef Not_VxD
		?merge	%??_name,@,%(??_aframe),,label,near
		?merge	public,,,,C,%??_name,@,%(??_aframe)
	else
		?merge	@,%??_name,@,%(??_aframe),label,near
		?merge	public,,,,,@,%??_name,@,%(??_aframe)
	endif
	endif
	?_BeginProc_Debug_Prologue
    endif
    if ??_pcall
	??_aframe = 0
	?count = ??_numargs
	rept	??_numargs
	    ?invprg <?MKA>,%?count
	    ?count = ?count - 1
	endm
    endif
    ??_fleave = FALSE
    if ??_esp
	if  ??_frame
	    sub esp, ??_frame
	    ??_pushed = ??_pushed + ??_frame
	    ??_fleave = VMM_TRUE
	endif
    else
	if  ??_frame eq 0
	    if (??_taframe eq 0) OR ((??_procflags AND ??_pf_ArgsUsed) EQ 0)
		ifdef DEBUG
		    push    ebp
		    mov ebp,esp
		    ??_fleave = VMM_TRUE
		endif
	    else
		push	ebp
		mov ebp,esp
		??_fleave = VMM_TRUE
	    endif
	else
	    enter   ??_frame, 0
	    ??_fleave = VMM_TRUE
	endif
    endif
    ??_procflags = ??_procflags OR ??_pf_Entered
endm

;***	LeaveProc - removes stack frame on exit
;
;   NOTE:   If there are localvar and ESP kind of stack frame
;	LeaveProc will destroy flags unless the "PRESERVE_FLAGS"
;	flag is given.	PRESERVE_FLAGS generates bigger, slower
;	code, so use it only when necessary.
;
;   WARNING: For "ESP" type stack frames, this macro DOES NOT adjust
;	 the internal stack depth for the local frame.	This is
;	 to allow jumping around the LeaveProc/Return to code
;	 after the LeaveProc/Return to use args/local variables,
;	 but code that uses the stack frame executed after the
;	 LeaveProc won't work.

LeaveProc macro flags
    if ??_fleave
	if ??_esp
	    ifidni <flags>,<PRESERVE_FLAGS>
		lea esp,[esp + ??_frame]
	    else
		add esp,??_frame
	    endif
	else
	    leave
	endif
    endif
    ??_procflags = ??_procflags OR ??_pf_Left
endm

;***	Return - return appropriately from a procedure
;
;   For "ccall" functions it's just a ret; for "pcall" and "scall"
;   it cleans the parameters off.
;

Return	macro
    if	??_cleanoff OR ??_w32svc
	if  ??_w32svc AND (??_taframe LT 8)
	    ret 8
	else
	    ret ??_taframe
	endif
    else
	ret
    endif
    ??_procflags = ??_procflags OR ??_pf_Returned
    endm

;***	EndProc - end the procedure
;

EndProc macro Name, Flag
    Name endp		;; Masm will provide error msg for us
if ??_w32svc
    if ??_taframe lt 8
	cparm&Name equ 0
    else
	cparm&Name equ (??_taframe/4 - 2)
    endif
endif
if ??_procflags AND ??_pf_Left
if ??_fleave
if ??_esp
    ??_pushed = ??_pushed - ??_frame
endif
endif
endif
ifdifi	<Flag>,<NOCHECK>
    if ??_pushed ne 0
	%out Warning: stack not balanced in Name
    endif
    if ??_procflags AND ??_pf_Check
	ife ??_procflags AND ??_pf_Entered
	    %out Warning: ArgVar/LocalVar without EnterProc in Name
	endif
	ife ??_procflags AND ??_pf_Left
	    %out Warning: ArgVar/LocalVar without LeaveProc in Name
	endif
	ife ??_procflags AND ??_pf_Returned
	    %out Warning: ArgVar/LocalVar without Return in Name
	endif
    endif
endif
ifdifi	<Flag>,<KEEPFRAMEVARS>
    ?count = 0
    rept    ??_numlocalsymbols
	?count = ?count + 1
	?invprg <?LOC>,%?count
    endm
endif
    ??_ends
    ??_ends equ <>
    endm

;***	cCall - "C" call
;
;   Arguments pushed in "C" order, caller cleans stack
;
;   USES: Flags.

cCall	macro	name, arglst, flags
    ife .TYPE name
       CondExtern name, near
    endif
    ifdef ??_nonstandardccall_&name
    PushCParams <arglst>, <FAST>
    else
    PushCParams <arglst>, <flags>
    endif
    call    name
    ifdef ??_nonstandardccall_&name
    ClearCParams PRESERVE_FLAGS
    else
    ClearCParams <flags>
    endif
    endm
    .xcref  cCall

;***	pCall - pascal call
;
;   Arguments pushed in pascal order, callee cleans stack
;

pCall	macro	name, arglst
    local   ??saved
    ife .TYPE name
	?toupper name
    else
	?upper equ <name>
    endif
    CondExtern %?upper, near
    ??saved = ??_pushed
    irp x,<arglst>
	push	x
	??_pushed = ??_pushed + 4
    endm
    call    ?upper
    ??_pushed = ??saved
    endm
    .xcref  pCall

;***	sCall - standard call
;
;   Arguments pushed in "C" order, callee cleans stack,
;   @argc appended to name
;

sCall	macro	name, arglst
    local   ??saved
    ??saved = ??_pushed
    PushCParams <arglst>
    ?scall  _, name, %(??_argc * 4)
    ??_pushed = ??saved
    endm
    .xcref  sCall

;***	fCall - fastcall call
;
;   Arguments pushed in "C" order (except first two parms,
;   which are passed in ECX and EDX), callee cleans stack, and
;   @argc appended to name.
;
;   The only useful value for flags is PRESERVE_FLAGS,
;   which can also be achieved by simply declaring the function
;   as non-standard, like so:
;
;	DeclareNonstandardCcallService <functionname>
;

fCall	macro	name, arglst, flags
    local   ??saved
    ??saved = ??_pushed
    ife .TYPE name
       CondExtern name, near
    endif
    PushCParams <arglst>, <FASTCALL>
    ?scall  @, name, %(??_argc * 4)
    ifdef ??_nonstandardccall_&name
    ClearCParams PRESERVE_FLAGS
    else
    ClearCParams <flags>
    endif
    ??_pushed = ??saved
    endm
    .xcref  fCall

;***	iCall - internal routine call
;
;   Set to whatever type we want to use as a default.

iCall	equ <sCall>

;***	PushCParams
;
;   Processes argument list
;
;   arglist = <arg1, arg2, arg3, ...>
;   flags = the word SMALL if we should prefer size over speed
;	the word FAST if we should prefer speed over size
;
;	The default flag is SMALL, unless the current procedure
;	is High_Freq, in which case we default to FAST.
;
;   To disable this optimization, define the symbol NONSTANDARD_CCALL.
;
IFNDEF	STANDARD_CCALL
NONSTANDARD_CCALL = 1		;; disabled by default for now
ENDIF

PushCParams macro arglst, flags
    LOCAL ??_pushedargs

    ??_argc = 0 	;; number of dwords on stack (global)
IFDEF	NONSTANDARD_CCALL
    ??_popargs = 0		;; establish default
ELSE
    ??_popargs = ??_align EQ 0	;; establish default
ENDIF
    ifidni  <flags>, <SMALL>
	??_popargs = 1		;; size, not speed
    elseifidni <flags>, <FAST>
	??_popargs = 0		;; speed, not size
    elseifidni <flags>, <FASTCALL>
	??_popargs = 0		;; speed, not size
    endif

    irp x,<arglst>
	??_argc = ??_argc + 1
	ifidni <flags>, <FASTCALL>
	  if ??_argc eq 1
	    ifdifi <x>, <ecx>
	      .err <first parameter must be ECX for fastcall functions>
	    endif
	  elseif ??_argc eq 2
	    ifdifi <x>, <edx>
	      .err <first parameter must be EDX for fastcall functions>
	    endif
	  else
	    ?marg   <x>,%??_argc
	  endif
	else
	  ?marg   <x>,%??_argc
	endif
    endm
    ?count = ??_argc
    ifidni <flags>, <FASTCALL>
      ??_pushedargs = ??_argc-2
    else
      ??_pushedargs = ??_argc
    endif
    if ??_pushedargs GT 0
      rept    ??_argc
	?invprg <?AM>,%?count
	?count = ?count - 1
      endm
    endif
    endm

;***	ClearCParams
;
;   Processes stack clean up
;
;   This routine will trade size for speed (if requested)
;   by using `pop ecx' to clean off one or two arguments.
;   This relies on the convention that C-call routines do
;   not return useful information in ECX.
;
;   To disable this optimization, define the symbol NONSTANDARD_CCALL.
;
;   If flags must be preserved, pass PRESERVE_FLAGS as an argument.
;   This will generate bigger, slower code, so use it only when
;   necessary.

ClearCParams macro fPreserveFlags
    if	??_argc ne 0
	if (??_popargs) AND (??_argc LE 2)
	  rept ??_argc
	  pop ecx
	  endm
	elseifidni <fPreserveFlags>, <PRESERVE_FLAGS>
	  lea esp, [esp][??_argc * 4]
	else
	  add esp,??_argc * 4
	endif
    endif
    ??_pushed = ??_pushed - (??_argc * 4)
    endm

; Makes a macro that will push argment when invoke - used by cCall only

?marg	macro	name, num
    .xcref
    .xcref  ?AM&num
    .cref
    ?AM&num &macro
	push	name
	??_pushed = ??_pushed + 4
	&endm
    endm
    .xcref  ?marg

; Concatenates, invokes and purges a macro name - used by PushCParams

?invprg macro	name1, name2
    name1&name2
    purge   name1&name2
    endm
    .xcref  ?invprg

; Calls a concatenated standard call name and makes it external

?scall	macro	prefix, name1, name2
    CondExtern prefix&name1&@&name2, near
    call    prefix&name1&@&name2
    endm
    .xcref  ?scall

; Equates name to a name

?merge	macro	l1, l2, l3, l4, op, r1, r2, r3, r4, r5, r6, r7, r8, r9
    l1&l2&l3&l4 op r1&r2&r3&r4&r5&r6&r7&r8&r9
    endm

; Converts string to upper-case, returned in ?upper

?toupper macro s
      ?upper equ <>
      irpc x,<s>
	if '&x' GE 'a'
	  if '&x' LE 'z'
	?t1 substr <ABCDEFGHIJKLMNOPQRSTUVWXYZ>,'&x'-'a'+1,1
	?upper catstr ?upper,?t1
	  else
	?upper catstr ?upper,<&x>
	  endif
	else
	  ?upper catstr ?upper,<&x>
	endif
      endm
    endm
    .xcref

;***	CondExtern - Make name external if not already defined
;
;   This operation is quite different between MASM 5.1 and 6.0.
;

CondExtern macro name,dist
    ifdef MASM6
	ifndef name
	externdef name:dist
	endif
    else
	if2
	ifndef name
	    extrn name:dist
	endif
	endif
    endif
endm

;***	SaveReg - Save register, "fd" pushes flags, "ad" pushes all

SaveReg macro	reglist 	;; push those registers
    irp reg,<reglist>
	ifidni <reg>, <fd>
	    pushfd
	    ??_pushed = ??_pushed + 4
	else
	ifidni <reg>, <ad>
	    pushad
	    ??_pushed = ??_pushed + SIZE Pushad_Struc
	else
	    push    reg
	    ??_pushed = ??_pushed + 4
	endif
	endif
    endm
endm

;***	RestoreReg - Restore register, "fd" pops flags, "ad" pops all
;
;   Note that registers must be restored in reverse order that they
;   were saved.
;

RestoreReg macro     reglist	;; pop those registers
    irp reg,<reglist>
	ifidni <reg>, <fd>
	    popfd
	    ??_pushed = ??_pushed - 4
	else
	ifidni <reg>, <ad>
	    popad
	    ??_pushed = ??_pushed - SIZE Pushad_Struc
	else
	    pop reg
	    ??_pushed = ??_pushed - 4
	endif
	endif
    endm
endm
*/

#ifdef DEBUG
/******************************************************************************
*   The following macros are for enabling procedure call profile counting
*   of VxD's written in assembler.
*
*   Begin_Profile_List needs to be used in the file that declares the device
*   immediately after the Declare_Virtual_Device line.	Then one Profile_Link
*   line is required for each individual source file.  The list is ended with
*   the End_Profile_List macro.  Profiling only works for debug builds and
*   the sources must all be built with "-Dprofileall" masm switch.
******************************************************************************/

/* ASM
Begin_Profile_List macro devname
ifdef profileall
VxD_DATA_SEG
    db	'PROCLIST'
PUBLIC devname&_Proc_Profile_List
devname&_Proc_Profile_List label dword
endif
endm

Profile_Link macro modname
ifdef profileall
ifdifi <modname>,@filename
EXTRN _&modname&__proc_list:near
endif
    dd	OFFSET32 _&modname&__proc_list
endif
endm

End_Profile_List macro
ifdef profileall
    dd	0
VxD_DATA_ENDS
endif
endm

*/
#endif

#ifndef Not_VxD

/******************************************************************************
 *	   S C H E D U L E R   B O O S T   V A L U E S
 *****************************************************************************/

#define RESERVED_LOW_BOOST  0x00000001
#define CUR_RUN_VM_BOOST    0x00000004
#define LOW_PRI_DEVICE_BOOST	0x00000010
#define HIGH_PRI_DEVICE_BOOST	0x00001000
#define CRITICAL_SECTION_BOOST	0x00100000
#define TIME_CRITICAL_BOOST 0x00400000
#define RESERVED_HIGH_BOOST 0x40000000


/******************************************************************************
 *   F L A G S	 F O R	 C A L L _ P R I O R I T Y _ V M _ E V E N T
 *****************************************************************************/

#define PEF_WAIT_FOR_STI_BIT	    0
#define PEF_WAIT_FOR_STI	(1 << PEF_WAIT_FOR_STI_BIT)
#define PEF_WAIT_NOT_CRIT_BIT	    1
#define PEF_WAIT_NOT_CRIT	(1 << PEF_WAIT_NOT_CRIT_BIT)

#define PEF_DONT_UNBOOST_BIT	    2
#define PEF_DONT_UNBOOST	(1 << PEF_DONT_UNBOOST_BIT)
#define PEF_ALWAYS_SCHED_BIT	    3
#define PEF_ALWAYS_SCHED	(1 << PEF_ALWAYS_SCHED_BIT)
#define PEF_TIME_OUT_BIT	4
#define PEF_TIME_OUT		(1 << PEF_TIME_OUT_BIT)

#define PEF_WAIT_NOT_HW_INT_BIT     5
#define PEF_WAIT_NOT_HW_INT	(1 << PEF_WAIT_NOT_HW_INT_BIT)
#define PEF_WAIT_NOT_NESTED_EXEC_BIT	6
#define PEF_WAIT_NOT_NESTED_EXEC    (1 << PEF_WAIT_NOT_NESTED_EXEC_BIT)
#define PEF_WAIT_IN_PM_BIT	7
#define PEF_WAIT_IN_PM		(1 << PEF_WAIT_IN_PM_BIT)

#define PEF_THREAD_EVENT_BIT	    8
#define PEF_THREAD_EVENT	(1 << PEF_THREAD_EVENT_BIT)

#define PEF_WAIT_FOR_THREAD_STI_BIT 9
#define PEF_WAIT_FOR_THREAD_STI (1 << PEF_WAIT_FOR_THREAD_STI_BIT)

#define PEF_RING0_EVENT_BIT	    10
#define PEF_RING0_EVENT 	(1 << PEF_RING0_EVENT_BIT)

#define PEF_WAIT_CRIT_BIT	11
#define PEF_WAIT_CRIT	    (1 << PEF_WAIT_CRIT_BIT)

#define PEF_WAIT_CRIT_VM_BIT	    12
#define PEF_WAIT_CRIT_VM    (1 << PEF_WAIT_CRIT_VM_BIT)

#define PEF_PROCESS_LAST_BIT	    13
#define PEF_PROCESS_LAST    (1 << PEF_PROCESS_LAST_BIT)

#define PEF_WAIT_PREEMPTABLE_BIT    14
#define PEF_WAIT_PREEMPTABLE (1 << PEF_WAIT_PREEMPTABLE_BIT)

// synonyms for event restrictions above

#define PEF_WAIT_NOT_TIME_CRIT_BIT   PEF_WAIT_NOT_HW_INT_BIT
#define PEF_WAIT_NOT_TIME_CRIT	     PEF_WAIT_NOT_HW_INT
#define PEF_WAIT_NOT_PM_LOCKED_STACK_BIT PEF_WAIT_NOT_NESTED_EXEC_BIT
#define PEF_WAIT_NOT_PM_LOCKED_STACK	 PEF_WAIT_NOT_NESTED_EXEC


/******************************************************************************
 *	 F L A G S   F O R   B E G I N _ C R I T I C A L _ S E C T I O N,
 *			     E N T E R _ M U T E X
 *	       A N D   W A I T _ S E M A P H O R E
 *****************************************************************************/

#define BLOCK_SVC_INTS_BIT	0
#define BLOCK_SVC_INTS		(1 << BLOCK_SVC_INTS_BIT)
#define BLOCK_SVC_IF_INTS_LOCKED_BIT	1
#define BLOCK_SVC_IF_INTS_LOCKED    (1 << BLOCK_SVC_IF_INTS_LOCKED_BIT)
#define BLOCK_ENABLE_INTS_BIT	    2
#define BLOCK_ENABLE_INTS	(1 << BLOCK_ENABLE_INTS_BIT)
#define BLOCK_POLL_BIT		3
#define BLOCK_POLL	    (1 << BLOCK_POLL_BIT)
#define BLOCK_THREAD_IDLE_BIT		4
#define BLOCK_THREAD_IDLE		(1 << BLOCK_THREAD_IDLE_BIT)
#define BLOCK_FORCE_SVC_INTS_BIT	5
#define BLOCK_FORCE_SVC_INTS	    (1 << BLOCK_FORCE_SVC_INTS_BIT)

/******************************************************************************
 *  The following structures are pointed to by EBP when VxD routines are
 *  entered, both for VxD control calls and traps(I/O traps, software INT
 *  traps, etc.).  The first structure as DWORD values, the second WORD
 *  values and the last has BYTE values.
 *****************************************************************************/

struct Client_Reg_Struc {
    ULONG Client_EDI;		/* Client's EDI */
    ULONG Client_ESI;		/* Client's ESI */
    ULONG Client_EBP;		/* Client's EBP */
    ULONG Client_res0;		/* ESP at pushall */
    ULONG Client_EBX;		/* Client's EBX */
    ULONG Client_EDX;		/* Client's EDX */
    ULONG Client_ECX;		/* Client's ECX */
    ULONG Client_EAX;		/* Client's EAX */
    ULONG Client_Error; 	/* Dword error code */
    ULONG Client_EIP;		/* EIP */
    USHORT Client_CS;		/* CS */
    USHORT Client_res1; 	/*   (padding) */
    ULONG Client_EFlags;	/* EFLAGS */
    ULONG Client_ESP;		/* ESP */
    USHORT Client_SS;		/* SS */
    USHORT Client_res2; 	/*   (padding) */
    USHORT Client_ES;		/* ES */
    USHORT Client_res3; 	/*   (padding) */
    USHORT Client_DS;		/* DS */
    USHORT Client_res4; 	/*   (padding) */
    USHORT Client_FS;		/* FS */
    USHORT Client_res5; 	/*   (padding) */
    USHORT Client_GS;		/* GS */
    USHORT Client_res6; 	/*   (padding) */
    ULONG Client_Alt_EIP;
    USHORT Client_Alt_CS;
    USHORT Client_res7;
    ULONG Client_Alt_EFlags;
    ULONG Client_Alt_ESP;
    USHORT Client_Alt_SS;
    USHORT Client_res8;
    USHORT Client_Alt_ES;
    USHORT Client_res9;
    USHORT Client_Alt_DS;
    USHORT Client_res10;
    USHORT Client_Alt_FS;
    USHORT Client_res11;
    USHORT Client_Alt_GS;
    USHORT Client_res12;
};


struct Client_Word_Reg_Struc {
    USHORT Client_DI;		/* Client's DI */
    USHORT Client_res13;	/*   (padding) */
    USHORT Client_SI;		/* Client's SI */
    USHORT Client_res14;	/*   (padding) */
    USHORT Client_BP;		/* Client's BP */
    USHORT Client_res15;	/*   (padding) */
    ULONG Client_res16; 	/* ESP at pushall */
    USHORT Client_BX;		/* Client's BX */
    USHORT Client_res17;	/*   (padding) */
    USHORT Client_DX;		/* Client's DX */
    USHORT Client_res18;	/*   (padding) */
    USHORT Client_CX;		/* Client's CX */
    USHORT Client_res19;	/*   (padding) */
    USHORT Client_AX;		/* Client's AX */
    USHORT Client_res20;	/*   (padding) */
    ULONG Client_res21; 	/* Dword error code */
    USHORT Client_IP;		/* Client's IP */
    USHORT Client_res22;	/*   (padding) */
    ULONG Client_res23; 	/* CS */
    USHORT Client_Flags;	/* Client's flags (low) */
    USHORT Client_res24;	/*   (padding) */
    USHORT Client_SP;		/* SP */
    USHORT Client_res25;
    ULONG Client_res26[5];
    USHORT Client_Alt_IP;
    USHORT Client_res27;
    ULONG Client_res28;
    USHORT Client_Alt_Flags;
    USHORT Client_res29;
    USHORT Client_Alt_SP;
};



struct Client_Byte_Reg_Struc {
    ULONG Client_res30[4];	/* EDI, ESI, EBP, ESP at pushall */
    UCHAR Client_BL;		/* Client's BL */
    UCHAR Client_BH;		/* Client's BH */
    USHORT Client_res31;
    UCHAR Client_DL;		/* Client's DL */
    UCHAR Client_DH;		/* Client's DH */
    USHORT Client_res32;
    UCHAR Client_CL;		/* Client's CL */
    UCHAR Client_CH;		/* Client's CH */
    USHORT Client_res33;
    UCHAR Client_AL;		/* Client's AL */
    UCHAR Client_AH;		/* Client's AH */
};


typedef union tagCLIENT_STRUC { /* */
    struct Client_Reg_Struc	  CRS;
    struct Client_Word_Reg_Struc  CWRS;
    struct Client_Byte_Reg_Struc  CBRS;
    } CLIENT_STRUCT;

typedef struct Client_Reg_Struc CRS;
typedef CRS *PCRS;

#if 0	/* causes problems with MASM 6 */
/* ASM
.ERRNZ Client_SP - Client_ESP
.ERRNZ Client_AL - Client_EAX
*/
#endif

#define DYNA_LINK_INT	0x20

/* ASM

;***	DeclareNonstandardCcallService
;
;   Declare services as conforming to the C calling convention
;   for parameter-passing, but *not* conforming to the C calling
;   convention for register usage.
;
;   Services which do not use the C calling convention for
;   parameter-passing need not be declared as nonstandard.
;
;   arglst - list of services to declare as nonstandard
;
DeclareNonstandardCcallService macro arglst
    irp x,<arglst>
	??_nonstandardccall_&&x = 1
    endm
endm

;
; The following VMM services are nonstandard:
;	_BlockOnID and _LocalizeSprintf modify no registers except flags.
;	_SetLastV86Page modifies no registers except EAX and flags.
;
DeclareNonstandardCcallService <_BlockOnID, _LocalizeSprintf>
DeclareNonstandardCcallService <_SetLastV86Page>

BeginDoc
;******************************************************************************
; The VMMCall and VxDCall macros provide a dynamic link to the VMM and VxD
;   service routines. For example:
;
;   VMMCall Enable_VM_Ints	; Equivalent to STI in VM code
;
;   mov     eax,[My_IRQ_Handle]
;   VxDCall VPICD_Set_Int_Request   ; Set IRQ for my device's interrupt
;
; Note that Enable_VM_Ints is defined in VMM.INC and VPICD_Set_Int_Request is
;   defined in VPICD.INC
;
;==============================================================================
EndDoc


BeginDoc
;******************************************************************************
; VxDCall
;==============================================================================
;
;   BlockOnID is always FAST because it doesn't
;   conform to the C calling convention.  (It preserves
;   all registers.)

EndDoc

DefTable MACRO vt, vn
    vt EQU <vn>
ENDM

GenDD2 MACRO vt, sn, jf
    dd	OFFSET32 vt[sn+jf]
ENDM

GenDD	MACRO	P, vid, snum, jflag
    LOCAL   vtable
IFDEF	@@VxDName&vid
    Deftable	vtable, %@@VxDName&vid
    EXTRN   vtable:DWORD
    GenDD2 %vtable, snum, jflag
ELSE
    dd	@@&P+jflag
ENDIF

ENDM


VxDCall MACRO P, Param, flags
    ??_vxdid = (@@&P SHR 16)
    ??_servicenum = (@@&P AND 0FFFFh)

    ifdef ??_standardccall_&P
      PushCParams <Param>, <FAST>
      .errnz ??_argc ne ??_standardccall_&P, <wrong # of parameters passed to &P&>
    else
      ifdef ??_fastcall_&P
	PushCParams <Param>, <FASTCALL>
	.errnz ??_argc ne (??_fastcall_&P), <wrong # of parameters passed to fastcall function &P&>
      else
	ifdef ??_nonstandardccall_&P
	  PushCParams <Param>, <flags>
	else
	  PushCParams <Param>, <FAST>
	endif
      endif
    endif
    int Dyna_Link_Int
    GenDD   P, %??_vxdid, %??_servicenum, 0
    ifndef ??_standardccall_&P
      ifndef ??_fastcall_&P
	ifdef ??_nonstandardccall_&P
	ClearCParams PRESERVE_FLAGS
	else
	ClearCParams
	endif
      else
	if(??_argc gt 2)
	    ??_pushed = ??_pushed - ((??_argc - 2) * 4)
	endif
      endif
    else
      ??_pushed = ??_pushed - (??_argc * 4)
    endif
    ENDM

VxDJmp	MACRO P, Param
    ??_vxdid = (@@&P SHR 16)
    ??_servicenum = (@@&P AND 0FFFFh)
    ifdef ??_fastcall_&P
      PushCParams <Param>, <FASTCALL>
      .errnz ??_argc gt 2, <More than 2 parameters may not be passed to fastcall functions thru VxDJmp>
    else
      .errnb <Param>, <Parameters may not be passed to VxDJmp or VMMJmp>
    endif
    int Dyna_Link_Int
    GenDD   P, %??_vxdid, %??_servicenum, DL_Jmp_Mask
    ENDM

DL_Jmp_Mask EQU 8000h
DL_Jmp_Bit  EQU 0Fh

VMMCall MACRO P, Param
    .ERRNZ (@@&P SHR 16) - VMM_DEVICE_ID
    VxDCall <P>, <Param>
    ENDM

VMMJmp MACRO P, Param
    .ERRNZ (@@&P SHR 16) - VMM_DEVICE_ID
    VxDJmp <P>, <Param>
    ENDM

BeginDoc
;******************************************************************************
; Segment definition macros
;
; The segment definition macros are a convenience used in defining the
;   segments used by the device driver. They are:
;VxD_INIT_CODE_SEG defines start of initialization code segment
;VxD_INIT_CODE_ENDS defines end of initialization code segment
;VxD_ICODE_SEG is an alias for VxD_INIT_CODE_SEG
;VxD_ICODE_ENDS is an alias for VxD_INIT_CODE_ENDS
;VxD_IDATA_SEG	 defines start of initialization data segment
;VxD_IDATA_ENDS  defines end of initialization data segment
;VxD_CODE_SEG	 defines start of always present code segment
;VxD_CODE_ENDS	 defines end of always present code segment
;VxD_DATA_SEG	 defines start of always present data segment
;VxD_DATA_ENDS	 defines end of always present data segment
;VxD_LOCKED_CODE_SEG	defines start of always present code segment
;VxD_LOCKED_CODE_ENDS	defines end of always present code segment
;VxD_PAGEABLE_CODE_SEG	defines start of swappable code segment
;VxD_PAGEABLE_CODE_ENDS defines end of swappable code segment
;VxD_DEBUG_ONLY_CODE_SEG defines code only loaded if debugger is present
;VxD_DEBUG_ONLY_CODE_ENDS
;VxD_DEBUG_ONLY_DATA_SEG defines data only loaded if debugger is present
;VxD_DEBUG_ONLY_DATA_ENDS
;==============================================================================




EndDoc


;   Resident protected mode code

VxD_CODE_SEG	EQU <VxD_LOCKED_CODE_SEG>
VxD_CODE_ENDS	EQU <VxD_LOCKED_CODE_ENDS>


VxD_LOCKED_CODE_SEG MACRO
_LTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_LCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_LOCKED_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_LTEXT	 ENDS
	ENDM


;   Pageable protected mode code

VxD_PAGEABLE_CODE_SEG MACRO
_PTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_PCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_PAGEABLE_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_PTEXT	 ENDS
	ENDM


;   Debug only protected mode code

VxD_DEBUG_ONLY_CODE_SEG MACRO
_DBOCODE    SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_DBOCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
	ENDM

VxD_DEBUG_ONLY_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_DBOCODE    ENDS
	ENDM


;   Protected mode initialization code

VxD_INIT_CODE_SEG   MACRO
_ITEXT	SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_ICODE
    ASSUME  cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    ENDM

VxD_INIT_CODE_ENDS  MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_ITEXT	ENDS
	ENDM

VxD_ICODE_SEG equ VxD_INIT_CODE_SEG
VxD_ICODE_ENDS equ VxD_INIT_CODE_ENDS


;   Resident protected mode data

VxD_DATA_SEG	EQU <VxD_LOCKED_DATA_SEG>
VxD_DATA_ENDS	EQU <VxD_LOCKED_DATA_ENDS>

VxD_LOCKED_DATA_SEG MACRO NO_ALIGN
_LDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_LOCKED_DATA_ENDS MACRO
_LDATA	 ENDS
	ENDM


;   Protected mode initialization data

VxD_IDATA_SEG	MACRO
_IDATA	SEGMENT
	ENDM
VxD_IDATA_ENDS	MACRO
_IDATA	ENDS
	ENDM


;   Pageable protected mode data

VxD_PAGEABLE_DATA_SEG MACRO NO_ALIGN
_PDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_PAGEABLE_DATA_ENDS MACRO
_PDATA	 ENDS
	ENDM


;   Static code segment for DL-VxDs

VxD_STATIC_CODE_SEG MACRO
_STEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_SCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_STATIC_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_STEXT	 ENDS
	ENDM


;   Static data segment for DL-VxDs

VxD_STATIC_DATA_SEG MACRO NO_ALIGN
_SDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_STATIC_DATA_ENDS MACRO
_SDATA	 ENDS
	ENDM

;   Debug only protected mode data

VxD_DEBUG_ONLY_DATA_SEG MACRO NO_ALIGN
_DBODATA    SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_DEBUG_ONLY_DATA_ENDS MACRO
_DBODATA    ENDS
	ENDM


;   16 bit code/data put in the init group (IGROUP)

VxD_16BIT_INIT_SEG  MACRO
_16ICODE SEGMENT
ASSUME CS:_16ICODE, DS:NOTHING, ES:NOTHING, SS:NOTHING
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_16ICODE
	  ENDM

VxD_16BIT_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_16ICODE ENDS
	   ENDM

;   Real mode segment (16 bit)

VxD_REAL_INIT_SEG  MACRO
_RCODE SEGMENT
ASSUME CS:_RCODE, DS:_RCODE, ES:_RCODE, SS:_RCODE
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_RCODE
	  ENDM

VxD_REAL_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_RCODE ENDS
	   ENDM
*/

#endif // Not_VxD

#ifndef DDK_VERSION

#ifdef WIN31COMPAT
#define DDK_VERSION 0x30A	    /* 3.10 */
#else  // WIN31COMPAT

#ifdef WIN40COMPAT
#define DDK_VERSION 0x400	    /* 4.00 */
#else  // WIN40COMPAT

#ifdef OPK3
#define DDK_VERSION 0x403	    /* 4.03 */
#else  // OPK3
#define DDK_VERSION 0x40A	    /*Memphis is 4.1 */
#endif // OPK3

#endif // WIN40COMPAT

#endif // WIN31COMPAT

#endif // DDK_VERSION

struct VxD_Desc_Block {
    ULONG DDB_Next;	    /* VMM RESERVED FIELD */
    USHORT DDB_SDK_Version;	/* INIT <DDK_VERSION> RESERVED FIELD */
    USHORT DDB_Req_Device_Number;   /* INIT <UNDEFINED_DEVICE_ID> */
    UCHAR DDB_Dev_Major_Version;    /* INIT <0> Major device number */
    UCHAR DDB_Dev_Minor_Version;    /* INIT <0> Minor device number */
    USHORT DDB_Flags;		/* INIT <0> for init calls complete */
    UCHAR DDB_Name[8];		/* AINIT <"        "> Device name */
    ULONG DDB_Init_Order;	/* INIT <UNDEFINED_INIT_ORDER> */
    ULONG DDB_Control_Proc;	/* Offset of control procedure */
    ULONG DDB_V86_API_Proc;	/* INIT <0> Offset of API procedure */
    ULONG DDB_PM_API_Proc;	/* INIT <0> Offset of API procedure */
    ULONG DDB_V86_API_CSIP;	/* INIT <0> CS:IP of API entry point */
    ULONG DDB_PM_API_CSIP;	/* INIT <0> CS:IP of API entry point */
    ULONG DDB_Reference_Data;	    /* Reference data from real mode */
    ULONG DDB_Service_Table_Ptr;    /* INIT <0> Pointer to service table */
    ULONG DDB_Service_Table_Size;   /* INIT <0> Number of services */
    ULONG DDB_Win32_Service_Table;  /* INIT <0> Pointer to Win32 services */
    ULONG DDB_Prev;	    /* INIT <'Prev'> Ptr to prev 4.0 DDB */
    ULONG DDB_Size;	/* INIT <SIZE(VxD_Desc_Block)> Reserved */
    ULONG DDB_Reserved1;	/* INIT <'Rsv1'> Reserved */
    ULONG DDB_Reserved2;	/* INIT <'Rsv2'> Reserved */
    ULONG DDB_Reserved3;	/* INIT <'Rsv3'> Reserved */
};

typedef struct VxD_Desc_Block	    *PVMMDDB;
typedef PVMMDDB 	    *PPVMMDDB;

#ifndef Not_VxD

/*
 *  Flag values for DDB_Flags
 */

#define DDB_SYS_CRIT_INIT_DONE_BIT  0
#define DDB_SYS_CRIT_INIT_DONE	    (1 << DDB_SYS_CRIT_INIT_DONE_BIT)
#define DDB_DEVICE_INIT_DONE_BIT    1
#define DDB_DEVICE_INIT_DONE	    (1 << DDB_DEVICE_INIT_DONE_BIT)

#define DDB_HAS_WIN32_SVCS_BIT	    14
#define DDB_HAS_WIN32_SVCS	(1 << DDB_HAS_WIN32_SVCS_BIT)
#define DDB_DYNAMIC_VXD_BIT	15
#define DDB_DYNAMIC_VXD 	(1 << DDB_DYNAMIC_VXD_BIT)

#define DDB_DEVICE_DYNALINKED_BIT   13
#define DDB_DEVICE_DYNALINKED	    (1 << DDB_DEVICE_DYNALINKED_BIT)


/* ASM
BeginDoc
;******************************************************************************
;
;   Declare_Virtual_Device macro
;
; ???? Write something here ????
;
;==============================================================================
EndDoc
Declare_Virtual_Device MACRO Name, Major_Ver, Minor_Ver, Ctrl_Proc, Device_Num, Init_Order, V86_Proc, PM_Proc, Reference_Data
    LOCAL   V86_API_Offset, PM_API_Offset, Serv_Tab_Offset, Serv_Tab_Len, Ref_Data_Offset

dev_id_err MACRO

IFNDEF Name&_Name_Based
.err <Device ID required when providing services>
ENDIF
    ENDM

IFB <V86_Proc>
    V86_API_Offset EQU 0
ELSE
 IFB <Device_Num>
    dev_id_err
 ENDIF
    V86_API_Offset EQU <OFFSET32 V86_Proc>
ENDIF

IFB <PM_Proc>
    PM_API_Offset EQU 0
ELSE
 IFB <Device_Num>
    dev_id_err
 ENDIF
    PM_API_Offset EQU <OFFSET32 PM_Proc>
ENDIF

IFDEF Name&_Service_Table
 IFB <Device_Num>
    dev_id_err
 ELSE
  IFE Device_Num - UNDEFINED_DEVICE_ID
    dev_id_err
  ENDIF
 ENDIF
    Serv_Tab_Offset EQU <OFFSET32 Name&_Service_Table>
    Serv_Tab_Len    EQU Num_&Name&_Services
ELSE
    Serv_Tab_Offset EQU 0
    Serv_Tab_Len    EQU 0
ENDIF

IFNB	<Device_Num>
  .erre (Device_Num LT BASEID_FOR_NAMEBASEDVXD), <Device ID  must be less than BASEID_FOR_NAMEBASEDVXD>
ENDIF

IFB <Reference_Data>
	Ref_Data_Offset EQU 0
ELSE
	Ref_Data_Offset EQU   <OFFSET32 Reference_Data>
ENDIF

IFDEF DEBUG
VxD_IDATA_SEG
    db	0dh, 0ah, 'D_E_B_U_G===>'
	db	"&Name", '<===', 0dh, 0ah
VxD_IDATA_ENDS
ENDIF

VxD_LOCKED_DATA_SEG

PUBLIC Name&_DDB
Name&_DDB VxD_Desc_Block <,,Device_Num,Major_Ver,Minor_Ver,,"&Name",Init_Order,\
	     OFFSET32 Ctrl_Proc, V86_API_Offset, PM_API_Offset, \
	     ,,Ref_Data_Offset,Serv_Tab_Offset, Serv_Tab_Len>

VxD_LOCKED_DATA_ENDS

    ENDM

;BeginDoc   ; comment out to make masm work ???
;******************************************************************************
; The Begin_Control_Dispatch macro is used for building a table for dispatching
; messages passed to the VxD_Control procedure.  It is used with
; Control_Dispatch and End_Control_Dispatch.  The only parameter is used to
; contruct the procedure label by adding "_Control" to the end (normally the
; device name is used i.e. VKD results in creating the procedure VKD_Control,
; this created procedure label must be included in Declare_Virtual_Device)
;
; An example of building a complete dispatch table:
;
; Begin_Control_Dispatch MyDevice
; Control_Dispatch  Device_Init, MyDeviceInitProcedure
; Control_Dispatch  Sys_VM_Init, MyDeviceSysInitProcedure
; Control_Dispatch  Create_VM,	 MyDeviceCreateVMProcedure
; End_Control_Dispatch MyDevice
;
; (NOTE: Control_Dispatch can be used without Begin_Control_Dispatch, but
;    then it is the programmer's responsibility for declaring a procedure
;    in locked code (VxD_LOCKED_CODE_SEG) and returning Carry clear for
;    any messages not processed.  The advantage in using
;    Begin_Control_Dispatch is when a large # of messages are processed by
;    a device, because a jump table is built which will usually require
;    less code space then the compares and jumps that are done when
;    Control_Dispatch is used alone.
;
;==============================================================================
;EndDoc
Begin_Control_Dispatch MACRO VxD_Name, p1, p2
??_cd_low = 0FFFFFFFFh
??_cd_high = 0

BeginProc VxD_Name&_Control, p1, p2, LOCKED
ENDM

End_Control_Dispatch   MACRO VxD_Name
    LOCAL ignore, table

procoff MACRO num
IFDEF ??_cd_&&num
    dd	OFFSET32 ??_cd_&&num
ELSE
    dd	OFFSET32 ignore
ENDIF
ENDM

IF ??_cd_low EQ ??_cd_high
    cmp eax, ??_cd_low
    ?merge  <jz>,,,,,<??_cd_>, %(??_cd_low)
    clc
    ret
ELSE
IF ??_cd_low GT 0
    sub eax, ??_cd_low
ENDIF ; ??cd_low GT 0
    cmp eax, ??_cd_high - ??_cd_low + 1
    jae short ignore
    jmp [eax*4+table]
ignore:
    clc 	    ;; this is not redundant
    ret

table label dword
    REPT   ??_cd_high - ??_cd_low + 1
    procoff %(??_cd_low)
    ??_cd_low = ??_cd_low + 1
    ENDM
ENDIF

EndProc VxD_Name&_Control

PURGE procoff
PURGE Begin_Control_Dispatch
PURGE Control_Dispatch
PURGE End_Control_Dispatch
ENDM

BeginDoc
;******************************************************************************
; The Control_Dispatch macro is used for dispatching based on message
;   passed to the VxD_Control procedure. E.G.:
;
; Control_Dispatch  Device_Init, MyDeviceInitProcedure
;
; For "C" control functions:
;
; Control_Dispatch  Device_Init, MyDeviceInitProcedure, sCall, <arglst>
;
; The "callc" can be sCall, cCall or pCall depending on the calling
; convention.  "arglst" is the list of registers to pass as parameters
; to "C" control procedure.  The "C" control procedure returns VXD_SUCCESS
; or VXD_FAILURE and the carry flag gets set appropriately.
;
; (NOTE: Control_Dispatch can be used with Begin_Control_Dispatch and
;    End_Control_Dispatch to create a jump table for dispatching messages,
;    when a large # of messages are processed.)
;
;==============================================================================
EndDoc
Control_Dispatch MACRO Service, Procedure, callc, arglst
    LOCAL Skip_Interseg_Jump

.errnz ?_LCODE, <Control_Dispatch must be in VxD_LOCKED_CODE_SEG.>

IFB <callc>

IFDEF ??_cd_low
Equate_Service MACRO Serv
??_cd_&&Serv equ Procedure
ENDM

Equate_Service %(Service)

IF Service LT ??_cd_low
??_cd_low = Service
ENDIF
IF Service GT ??_cd_high
??_cd_high = Service
ENDIF

PURGE Equate_Service

ELSE
    cmp eax, Service
    jz	Procedure
ENDIF

ELSE ; ifb callc

    cmp eax, Service
    jne SHORT Skip_Interseg_Jump
    callc   Procedure, <arglst>
IF Service EQ PNP_NEW_DEVNODE
    stc
ELSE
    cmp eax,1
ENDIF
    ret
Skip_Interseg_Jump:

ENDIF ; ifb callc

    ENDM
*/


/******************************************************************************
 *  The following are the definitions for the "type of I/O" parameter passed
 *  to a I/O trap routine.
 *****************************************************************************/

#define BYTE_INPUT  0x000
#define BYTE_OUTPUT 0x004
#define WORD_INPUT  0x008
#define WORD_OUTPUT 0x00C
#define DWORD_INPUT 0x010
#define DWORD_OUTPUT	0x014

#define OUTPUT_BIT  2
#define OUTPUT	    (1 << OUTPUT_BIT)
#define WORD_IO_BIT 3
#define WORD_IO     (1 << WORD_IO_BIT)
#define DWORD_IO_BIT	4
#define DWORD_IO    (1 << DWORD_IO_BIT)

#define STRING_IO_BIT	5
#define STRING_IO   (1 << STRING_IO_BIT)
#define REP_IO_BIT  6
#define REP_IO	    (1 << REP_IO_BIT)
#define ADDR_32_IO_BIT	7
#define ADDR_32_IO  (1 << ADDR_32_IO_BIT)
#define REVERSE_IO_BIT	8
#define REVERSE_IO  (1 << REVERSE_IO_BIT)

#define IO_SEG_MASK 0x0FFFF0000     /* Use this to get segment */
#define IO_SEG_SHIFT	0x10		/* Must shift right this many */


/* ASM
BeginDoc
;******************************************************************************
;
;   Dispatch_Byte_IO macro
;
; Dispatch_Byte_IO Byte_In_Proc, Byte_Out_Proc
;==============================================================================
EndDoc
Dispatch_Byte_IO MACRO In_Proc, Out_Proc
    LOCAL   Byte_IO
    cmp ecx, Byte_Output
    jbe SHORT Byte_IO
    VMMJmp  Simulate_IO
Byte_IO:
IFIDNI <In_Proc>, <Fall_Through>
    je	Out_Proc
ELSE
IFIDNI <Out_Proc>, <Fall_Through>
    jb	In_Proc
ELSE
    je	Out_Proc
    jmp In_Proc
ENDIF
ENDIF
    ENDM

BeginDoc
;******************************************************************************
;
;   Emulate_Non_Byte_IO
;
; Emulate_Non_Byte_IO
;
;==============================================================================
EndDoc
Emulate_Non_Byte_IO MACRO
    LOCAL   Byte_IO
    cmp ecx, Byte_Output
    jbe SHORT Byte_IO
    VMMJmp  Simulate_IO
Byte_IO:
    ENDM
*/


/* ASM
BeginDoc
;******************************************************************************
;
; Begin_VxD_IO_Table
;
;   Example:
; Begin_VxD_IO_Table MyTableName
;
;==============================================================================
EndDoc
*/


struct VxD_IOT_Hdr {
    USHORT VxD_IO_Ports;
};

struct VxD_IO_Struc {
    USHORT VxD_IO_Port;
    ULONG VxD_IO_Proc;
};


/* ASM
.ERRNZ SIZE VxD_IOT_Hdr - 2 ; Begin_VxD_IO_Table creates a 1 word count hdr
Begin_VxD_IO_Table MACRO Table_Name
PUBLIC Table_Name
Table_Name LABEL WORD

ifndef MASM6
IF2
IFNDEF Table_Name&_Entries
.err <No End_VxD_IO_Table for &Table_Name>
ENDIF
    dw	Table_Name&_Entries
ELSE
    dw	?
ENDIF
ELSE  ; MASM6 - skip the warning message - we'll get it anyway
    dw	Table_Name&_Entries
ENDIF ; MASM6

    ENDM

.ERRNZ SIZE VxD_IO_Struc - 6	; VxD_IO creates 6 byte I/O port entries
VxD_IO MACRO Port, Proc_Name
    dw	Port
    dd	OFFSET32 Proc_Name
    ENDM

End_VxD_IO_Table MACRO Table_Name

IFNDEF Table_Name
.err <No Begin_VxD_IO_Table for &Table_Name>
ELSE
    Table_Name&_Entries EQU (($-Table_Name)-2) / (SIZE VxD_IO_Struc)
IF Table_Name&_Entries LE 0
.err <Invalid number of port traps in &Table_Name>
ENDIF
ENDIF
	ENDM


;******************************************************************************
;
; Push_Client_State takes an optional argument which if equal to the symbol
; USES_EDI saves code size by suppressing the preservation of the EDI register.
;
; Similarly, Pop_Client_State takes an optional argument which if equal to
; the symbol USES_ESI saves code size by suppressing the preservation of
; the ESI register.
;
;******************************************************************************

Push_Client_State MACRO Can_Trash_EDI
    sub esp, SIZE Client_Reg_Struc
    ??_pushed = ??_pushed + SIZE Client_Reg_Struc
    ifidni <Can_Trash_EDI>, <USES_EDI>
    mov edi, esp
    VMMCall Save_Client_State
    else
    push    edi
    lea edi, [esp+4]
    VMMCall Save_Client_State
    pop edi
    endif
    ENDM

Pop_Client_State MACRO Can_Trash_ESI
    ifdifi <Can_Trash_ESI>, <USES_ESI>
    push    esi
    lea esi, [esp+4]
    VMMCall Restore_Client_State
    pop esi
    else
    mov esi, esp
    VMMCall Restore_Client_State
    endif
    add esp, SIZE Client_Reg_Struc
    ??_pushed = ??_pushed - SIZE Client_Reg_Struc
    ENDM

BeginDoc
;******************************************************************************
;
;   CallRet -- Call procedure and return.  For debugging purposes only.
;	   If compiled with debugging then this will generate a call
;	   followed by a return.  If non-debugging version then the
;	   specified label will be jumped to.
;
;   PARAMETERS:
;   Label_Name = Procedure to be called
;
;   EXIT:
;   Return from current procedure
;
;------------------------------------------------------------------------------
EndDoc

CallRet MACRO P1, P2
IFDEF DEBUG
IFIDNI <P1>, <SHORT>
    call    P2
ELSE
    call    P1
ENDIF
    ret
ELSE
    jmp P1 P2
ENDIF
    ENDM

BeginDoc
;******************************************************************************
;
;   VxDCallRet
;   VMMCallRet -- CallRet for VxDCall and VMMCall.
;
;------------------------------------------------------------------------------
EndDoc

IFDEF	DEBUG

VxDCallRet macro p:req
    VxDCall p
    ret
endm

VMMCallRet macro p:req
    VMMCall p
    ret
endm

ELSE ; RETAIL

VxDCallRet equ <VxDJmp>
VMMCallRet equ <VMMJmp>

ENDIF


; ebp offsets to segments pushed by PMode_Fault in Fault_Dispatch
PClient_DS equ WORD PTR -4
PClient_ES equ WORD PTR -8
PClient_FS equ WORD PTR -12
PClient_GS equ WORD PTR -16


;******************************************************************************
;
; Client_Ptr_Flat takes an optional third argument which if equal to the
; symbol USES_EAX saves code size by supressing the preservation of the
; EAX register.  The USES_EAX flag is ignored if the destination register
; is itself EAX.
;
;******************************************************************************

Client_Ptr_Flat MACRO Reg_32, Cli_Seg, Cli_Off, Can_Trash_EAX

IFDIFI <Reg_32>, <EAX>
    IFDIFI <Can_Trash_EAX>, <USES_EAX>
    xchg    Reg_32, eax
    ENDIF
ENDIF
IFB <Cli_Off>
    mov ax, (Client_&Cli_Seg * 100h) + 0FFh
ELSE
    mov ax, (Client_&Cli_Seg * 100h) + Client_&Cli_Off
ENDIF
    VMMCall Map_Flat

IFDIFI <Reg_32>, <EAX>
    xchg    Reg_32, eax
ENDIF

    ENDM

;------------------------------------------------------------------------------

VxDint	MACRO	Int_Number
    if	(OPATTR Int_Number) AND 4
    push    Int_Number
    else
    push    DWORD PTR Int_Number
    endif
    VMMCall Exec_VxD_Int
    ENDM

VxDintMustComplete MACRO   Int_Number
    if	(OPATTR Int_Number) AND 4
    push    Int_Number
    else
    push    DWORD PTR Int_Number
    endif
	VMMCall _ExecVxDIntMustComplete
    ENDM

Load_FS 	macro
	VMMCall Load_FS_Service
endm
*/

/*XLATOFF*/
#define Load_FS VMMCall(Load_FS_Service)
/*XLATON*/

#endif // Not_VxD


/******************************************************************************
 *
 *  The following equates are for flags sent to the real mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/
#define DUPLICATE_DEVICE_ID_BIT     0	/* loaded */
#define DUPLICATE_DEVICE_ID	(1 << DUPLICATE_DEVICE_ID_BIT)
#define DUPLICATE_FROM_INT2F_BIT    1	/* loaded from INT 2F list */
#define DUPLICATE_FROM_INT2F	    (1 << DUPLICATE_FROM_INT2F_BIT)
#define LOADING_FROM_INT2F_BIT	    2	/* in the INT 2F device list */
#define LOADING_FROM_INT2F	(1 << LOADING_FROM_INT2F_BIT)


/******************************************************************************
 *
 *  The following equates are used to indicate the result of the real mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/

#define DEVICE_LOAD_OK	    0	/* load protected mode portion */
#define ABORT_DEVICE_LOAD   1	/* don't load protected mode portion */
#define ABORT_WIN386_LOAD   2	/* fatal-error: abort load of Win386 */
#define DEVICE_NOT_NEEDED   3	/* don't load protected mode portion */
				/* b/c the driver's presence is not needed */



#define NO_FAIL_MESSAGE_BIT 15	/* set bit to suppress error message */
#define NO_FAIL_MESSAGE     (1 << NO_FAIL_MESSAGE_BIT)


/******************************************************************************
 *
 *  The following equates define the loader services available to the real-mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/

#define LDRSRV_GET_PROFILE_STRING   0	/* search SYSTEM.INI for string */
#define LDRSRV_GET_NEXT_PROFILE_STRING	1   /* search for next string */
#define LDRSRV_RESERVED 	2   /* RESERVED */
#define LDRSRV_GET_PROFILE_BOOLEAN  3	/* search SYSTEM.INI for boolean */
#define LDRSRV_GET_PROFILE_DECIMAL_INT	4   /* search SYSTEM.INI for integer */
#define LDRSRV_GET_PROFILE_HEX_INT  5	/* search SYSTEM.INI for hex int */
#define LDRSRV_COPY_EXTENDED_MEMORY 6	/* allocate/init extended memory */
#define LDRSRV_GET_MEMORY_INFO	    7	/* get info about machine memory */

/* Add the new loader services contiguously here */

/****** Registry services for Real mode init time *************
 * The parameters for these are as defined in Windows.h for the
 * corresponding Win Reg API and should be on Stack. These are
 * C Callable except that the function no has to be in AX
 * ************************************************************
*/

#define LDRSRV_RegOpenKey	0x100
#define LDRSRV_RegCreateKey	0x101
#define LDRSRV_RegCloseKey	0x102
#define LDRSRV_RegDeleteKey	0x103
#define LDRSRV_RegSetValue	0x104
#define LDRSRV_RegQueryValue	    0x105
#define LDRSRV_RegEnumKey	0x106
#define LDRSRV_RegDeleteValue	    0x107
#define LDRSRV_RegEnumValue	0x108
#define LDRSRV_RegQueryValueEx	    0x109
#define LDRSRV_RegSetValueEx	    0x10A
#define LDRSRV_RegFlushKey	0x10B


/*
 *  For the Copy_Extended_Memory service, the following types of memory can be
 *  requested:
 */

#define LDRSRV_COPY_INIT	1   /* memory discarded after init */
#define LDRSRV_COPY_LOCKED	2   /* locked memory */
#define LDRSRV_COPY_PAGEABLE	    3	/* pageable memory */

/****************************************************************************
*
*   Object types supported by the vxd loader
*
*  Notes : Low bit of all CODE type objects should be set (VXDLDR uses this)
*	    Also Init type objects should be added to the second part of the
*	    list (which starts with ICODE_OBJ).
*
*****************************************************************************/

#define RCODE_OBJ	-1

#define LCODE_OBJ	0x01
#define LDATA_OBJ	0x02
#define PCODE_OBJ	0x03
#define PDATA_OBJ	0x04
#define SCODE_OBJ	0x05
#define SDATA_OBJ	0x06
#define CODE16_OBJ	0x07
#define LMSG_OBJ	0x08
#define PMSG_OBJ	0x09

#define DBOC_OBJ    0x0B
#define DBOD_OBJ    0x0C

#define PLCODE_OBJ	0x0D
#define PPCODE_OBJ	0x0F

#define ICODE_OBJ	0x11
#define IDATA_OBJ	0x12
#define ICODE16_OBJ	0x13
#define IMSG_OBJ	0x14


struct ObjectLocation {
    ULONG OL_LinearAddr ;
    ULONG OL_Size ;
    UCHAR  OL_ObjType ;
} ;

#define MAXOBJECTS  25

/*****************************************************************************
 *
 *	Device_Location structure
 *
 *****************************************************************************/

struct Device_Location_List {
    ULONG DLL_DDB ;
    UCHAR DLL_NumObjects ;
    struct ObjectLocation DLL_ObjLocation[1];
};


/* ========================================================================= */

/*
 *  CR0 bit assignments
 */
#define PE_BIT	    0	/* 1 = Protected Mode */
#define PE_MASK     (1 << PE_BIT)
#define MP_BIT	    1	/* 1 = Monitor Coprocessor */
#define MP_MASK     (1 << MP_BIT)
#define EM_BIT	    2	/* 1 = Emulate Math Coprocessor */
#define EM_MASK     (1 << EM_BIT)
#define TS_BIT	    3	/* 1 = Task Switch occured */
#define TS_MASK     (1 << TS_BIT)
#define ET_BIT	    4	/* 1 = 387 present, 0 = 287 present */
#define ET_MASK     (1 << ET_BIT)
#define PG_BIT	    31	/* 1 = paging enabled, 0 = paging disabled */
#define PG_MASK     (1 << PG_BIT)


/*
 *  EFLAGs bit assignments
 */
#define CF_BIT	    0
#define CF_MASK     (1 << CF_BIT)
#define PF_BIT	    2
#define PF_MASK     (1 << PF_BIT)
#define AF_BIT	    4
#define AF_MASK     (1 << AF_BIT)
#define ZF_BIT	    6
#define ZF_MASK     (1 << ZF_BIT)
#define SF_BIT	    7
#define SF_MASK     (1 << SF_BIT)
#define TF_BIT	    8
#define TF_MASK     (1 << TF_BIT)
#define IF_BIT	    9
#define IF_MASK     (1 << IF_BIT)
#define DF_BIT	    10
#define DF_MASK     (1 << DF_BIT)
#define OF_BIT	    11	/* Overflow flag */
#define OF_MASK     (1 << OF_BIT)
#define IOPL_MASK   0x3000  /* IOPL flags */
#define IOPL_BIT0   12
#define IOPL_BIT1   13
#define NT_BIT	    14	/* Nested task flag */
#define NT_MASK     (1 << NT_BIT)
#define RF_BIT	    16	/* Resume flag */
#define RF_MASK     (1 << RF_BIT)
#define VM_BIT	    17	/* Virtual Mode flag */
#define VM_MASK     (1 << VM_BIT)
#define AC_BIT	    18	/* Alignment check */
#define AC_MASK     (1 << AC_BIT)
#define VIF_BIT     19	/* Virtual Interrupt flag */
#define VIF_MASK    (1 << VIF_BIT)
#define VIP_BIT     20	/* Virtual Interrupt pending */
#define VIP_MASK    (1 << VIP_BIT)



/* ASM
;------------------------------------------------------------------------------
;
;     Temporary MASM macros (to be removed when supported by MASM)
;
;------------------------------------------------------------------------------

IFDEF MASM6
loopde EQU <looped>
loopdne EQU <loopned>
loopdz EQU <loopzd>
loopdnz EQU <loopnzd>
ELSE
loopd EQU <loop>
loopde EQU <loope>
loopdne EQU <loopne>
loopdz EQU <loopz>
loopdnz EQU <loopnz>
ENDIF
*/


/******************************************************************************
 *		PAGE TABLE EQUATES
 *****************************************************************************/


#define P_SIZE	    0x1000	/* page size */

/******************************************************************************
 *
 *		PAGE TABLE ENTRY BITS
 *
 *****************************************************************************/

#define P_PRESBIT   0
#define P_PRES	    (1 << P_PRESBIT)
#define P_WRITEBIT  1
#define P_WRITE     (1 << P_WRITEBIT)
#define P_USERBIT   2
#define P_USER	    (1 << P_USERBIT)
#define P_ACCBIT    5
#define P_ACC	    (1 << P_ACCBIT)
#define P_DIRTYBIT  6
#define P_DIRTY     (1 << P_DIRTYBIT)

#define P_AVAIL     (P_PRES+P_WRITE+P_USER) /* avail to user & present */

/****************************************************
 *
 *  Page types for page allocator calls
 *
 ***************************************************/

#define PG_VM	    0
#define PG_SYS	    1
#define PG_RESERVED1	2
#define PG_PRIVATE  3
#define PG_RESERVED2	4
#define PG_RELOCK   5	    /* PRIVATE to MMGR */
#define PG_INSTANCE 6
#define PG_HOOKED   7
#define PG_IGNORE   0xFFFFFFFF

/****************************************************
 *
 *  Definitions for the access byte in a descriptor
 *
 ***************************************************/

/*
 *  Following fields are common to segment and control descriptors
 */
#define D_PRES	    0x080	/* present in memory */
#define D_NOTPRES   0	    /* not present in memory */

#define D_DPL0	    0	    /* Ring 0 */
#define D_DPL1	    0x020	/* Ring 1 */
#define D_DPL2	    0x040	/* Ring 2 */
#define D_DPL3	    0x060	/* Ring 3 */

#define D_SEG	    0x010	/* Segment descriptor */
#define D_CTRL	    0	    /* Control descriptor */

#define D_GRAN_BYTE 0x000	/* Segment length is byte granular */
#define D_GRAN_PAGE 0x080	/* Segment length is page granular */
#define D_DEF16     0x000	/* Default operation size is 16 bits */
#define D_DEF32     0x040	/* Default operation size is 32 bits */


/*
 *  Following fields are specific to segment descriptors
 */
#define D_CODE	    0x08	/* code */
#define D_DATA	    0	    /* data */

#define D_X	0	/* if code, exec only */
#define D_RX	    0x02	/* if code, readable */
#define D_C	0x04	    /* if code, conforming */

#define D_R	0	/* if data, read only */
#define D_W	0x02	    /* if data, writable */
#define D_ED	    0x04	/* if data, expand down */

#define D_ACCESSED  1	    /* segment accessed bit */


/*
 *  Useful combination access rights bytes
 */
#define RW_DATA_TYPE	(D_PRES+D_SEG+D_DATA+D_W)
#define R_DATA_TYPE (D_PRES+D_SEG+D_DATA+D_R)
#define CODE_TYPE   (D_PRES+D_SEG+D_CODE+D_RX)

#define D_PAGE32    (D_GRAN_PAGE+D_DEF32)   /* 32 bit Page granular */

/*
 * Masks for selector fields
 */
#define SELECTOR_MASK	0xFFF8	    /* selector index */
#define SEL_LOW_MASK	0xF8	    /* mask for low byte of sel indx */
#define TABLE_MASK  0x04	/* table bit */
#define RPL_MASK    0x03	/* privilige bits */
#define RPL_CLR     (~RPL_MASK) /* clear ring bits */

#define IVT_ROM_DATA_SIZE   0x500

/*XLATOFF*/

#ifndef Not_VxD

#define ENABLE_INTERRUPTS() {__asm sti}
#define DISABLE_INTERRUPTS()	{__asm cli}

#define SAVE_FLAGS(flags) {\
    {__asm pushfd}; \
    {__asm pop flags}}

#define RESTORE_FLAGS(flags) {\
    {__asm push flags}; \
    {__asm popfd}}

#define IO_Delay() {\
    {__asm _emit 0xeb}; \
    {__asm _emit 0x00}}

#define Touch_Register(Register) {_asm xor Register, Register}

typedef DWORD	HEVENT;

#define VMM_GET_DDB_NAMED 0

#pragma warning (disable:4209)	// turn off redefine warning (with basedef.h)

typedef ULONG HTIMEOUT;     // timeout handle
typedef ULONG CMS;	// count of milliseconds

#pragma warning (default:4209)	// turn on redefine warning (with basedef.h)

typedef DWORD	VMM_SEMAPHORE;


#ifndef WANTVXDWRAPS

WORD VXDINLINE
Get_VMM_Version()
{
    WORD w;
    VMMCall(Get_VMM_Version);
    _asm mov [w], ax
    return(w);
}

PVOID VXDINLINE
_HeapAllocate(ULONG Bytes, ULONG Flags)
{
    PVOID p;
    Touch_Register(eax)
    Touch_Register(ecx)
    Touch_Register(edx)
    _asm push [Flags]
    _asm push [Bytes]
    VMMCall(_HeapAllocate)
    _asm add esp, 8
    _asm mov [p], eax
    return(p);
}

ULONG VXDINLINE
_HeapFree(PVOID Address, ULONG Flags)
{
    ULONG ul;
    Touch_Register(eax)
    Touch_Register(ecx)
    Touch_Register(edx)
    _asm push [Flags]
    _asm push [Address]
    VMMCall(_HeapFree)
    _asm add esp, 8
    _asm mov [ul], eax
    return(ul);
}

HEVENT VXDINLINE
Call_Global_Event(void (__cdecl *pfnEvent)(), ULONG ulRefData)
{
    HEVENT hevent;
    _asm mov edx, [ulRefData]
    _asm mov esi, [pfnEvent]
    VMMCall(Call_Global_Event)
    _asm mov [hevent], esi
    return(hevent);
}

HEVENT VXDINLINE
Schedule_Global_Event(void (__cdecl *pfnEvent)(), ULONG ulRefData)
{
    HEVENT hevent;
    _asm mov edx, [ulRefData]
    _asm mov esi, [pfnEvent]
    VMMCall(Schedule_Global_Event)
    _asm mov [hevent], esi
    return(hevent);
}

void VXDINLINE
Cancel_Global_Event( HEVENT hevent )
{
    _asm mov esi, hevent
    VMMCall( Cancel_Global_Event );
}

HVM VXDINLINE
Get_Sys_VM_Handle(VOID)
{
    HVM hvm;
    Touch_Register(ebx)
    VxDCall(Get_Sys_VM_Handle);
    _asm mov [hvm], ebx
    return(hvm);
}

VOID VXDINLINE
Fatal_Error_Handler(PCHAR pszMessage, DWORD dwExitFlag)
{
    _asm mov esi, [pszMessage]
    _asm mov eax, [dwExitFlag]
    VMMCall(Fatal_Error_Handler);
}

VMM_SEMAPHORE VXDINLINE
Create_Semaphore(LONG lTokenCount)
{
    VMM_SEMAPHORE vmm_semaphore;
    _asm mov ecx, [lTokenCount]
    VMMCall(Create_Semaphore)
    _asm cmc
    _asm sbb ecx, ecx
    _asm and eax, ecx
    _asm mov [vmm_semaphore], eax
    return(vmm_semaphore);
}

void VXDINLINE
Destroy_Semaphore(VMM_SEMAPHORE vsSemaphore)
{
    _asm mov eax, [vsSemaphore]
    VMMCall(Destroy_Semaphore)
}

void VXDINLINE
Signal_Semaphore(VMM_SEMAPHORE vsSemaphore)
{
    _asm mov eax, [vsSemaphore]
    VMMCall(Signal_Semaphore)
}

void VXDINLINE
Wait_Semaphore(VMM_SEMAPHORE vsSemaphore, DWORD dwFlags)
{
    _asm mov eax, [vsSemaphore]
    _asm mov ecx, [dwFlags]
    VMMCall(Wait_Semaphore)
}

HVM VXDINLINE
Get_Execution_Focus(void)
{
    HVM hvm;
    Touch_Register(ebx)
    VMMCall(Get_Execution_Focus)
    _asm mov [hvm], ebx
    return(hvm);
}

void VXDINLINE
Begin_Critical_Section(ULONG Flags)
{
    _asm mov ecx, [Flags]
    VMMCall(Begin_Critical_Section)
}

void VXDINLINE
End_Critical_Section(void)
{
    VMMCall(End_Critical_Section)
}

void VXDINLINE
Fatal_Memory_Handler(void)
{
    VMMCall(Fatal_Memory_Error);
}

void VXDINLINE
Begin_Nest_Exec(void)
{
    VMMCall(Begin_Nest_Exec)
}

void VXDINLINE
End_Nest_Exec(void)
{
    VMMCall(End_Nest_Exec)
}

void VXDINLINE
Resume_Exec(void)
{
    VMMCall(Resume_Exec)
}

HTIMEOUT VXDINLINE
Set_VM_Time_Out(void (*pfnTimeout)(), CMS cms, ULONG ulRefData)
{
    HTIMEOUT htimeout;
    _asm mov eax, [cms]
    _asm mov edx, [ulRefData]
    _asm mov esi, [pfnTimeout]
    VMMCall(Set_VM_Time_Out)
    _asm mov [htimeout], esi
    return(htimeout);
}

HTIMEOUT VXDINLINE
Set_Global_Time_Out(void (__cdecl *pfnTimeout)(), CMS cms, ULONG ulRefData)
{
    HTIMEOUT htimeout;
    _asm mov eax, [cms]
    _asm mov edx, [ulRefData]
    _asm mov esi, [pfnTimeout]
    VMMCall(Set_Global_Time_Out)
    _asm mov [htimeout], esi
    return(htimeout);
}

void VXDINLINE
Cancel_Time_Out(HTIMEOUT htimeout)
{
    _asm mov esi, htimeout
    VMMCall(Cancel_Time_Out)
}


void VXDINLINE
Update_System_Clock(ULONG msElapsed)
{
    __asm mov ecx,[msElapsed]
    VMMCall(Update_System_Clock)
}

void VXDINLINE
Enable_Touch_1st_Meg(void)
{
    VMMCall(Enable_Touch_1st_Meg)
}

void VXDINLINE
Disable_Touch_1st_Meg(void)
{
    VMMCall(Disable_Touch_1st_Meg)
}

void VXDINLINE
Out_Debug_String(char *psz)
{
    __asm pushad
    __asm mov esi, [psz]
    VMMCall(Out_Debug_String)
    __asm popad
}

void VXDINLINE
Queue_Debug_String(char *psz, ULONG ulEAX, ULONG ulEBX)
{
    _asm push esi
    _asm push [ulEAX]
    _asm push [ulEBX]
    _asm mov esi, [psz]
    VMMCall(Queue_Debug_String)
    _asm pop esi
}

#ifdef WIN40SERVICES

HTIMEOUT VXDINLINE
Set_Async_Time_Out(void (*pfnTimeout)(), CMS cms, ULONG ulRefData)
{
    HTIMEOUT htimeout;
    _asm mov eax, [cms]
    _asm mov edx, [ulRefData]
    _asm mov esi, [pfnTimeout]
    VMMCall(Set_Async_Time_Out)
    _asm mov [htimeout], esi
    return(htimeout);
}

VXDINLINE struct VxD_Desc_Block *
VMM_Get_DDB(WORD DeviceID, PCHAR Name)
{
    struct VxD_Desc_Block *p;
    _asm movzx eax, [DeviceID]
    _asm mov edi, [Name]
    VMMCall(Get_DDB);
    _asm mov [p], ecx
    return(p);
}

DWORD VXDINLINE
VMM_Directed_Sys_Control(struct VxD_Desc_Block *DDB, DWORD SysControl, DWORD rEBX, DWORD rEDX, DWORD rESI, DWORD rEDI)
{
    DWORD dw;
    _asm mov eax, [SysControl]
    _asm mov ebx, [rEBX]
    _asm mov ecx, [DDB]
    _asm mov edx, [rEDX]
    _asm mov esi, [rESI]
    _asm mov edi, [rEDI]
    VMMCall(Directed_Sys_Control);
    _asm mov [dw], eax
    return(dw);
}

void VXDINLINE
_Trace_Out_Service(char *psz)
{
    __asm push psz
    VMMCall(_Trace_Out_Service)
}

void VXDINLINE
_Debug_Out_Service(char *psz)
{
    __asm push psz
    VMMCall(_Debug_Out_Service)
}

void VXDINLINE
_Debug_Flags_Service(ULONG flags)
{
    __asm push flags
    VMMCall(_Debug_Flags_Service)
}

void VXDINLINE _cdecl
_Debug_Printf_Service(char *pszfmt, ...)
{
    __asm lea  eax,(pszfmt + 4)
    __asm push eax
    __asm push pszfmt
    VMMCall(_Debug_Printf_Service)
    __asm add esp, 2*4
}

#endif // WIN40SERVICES

#endif // WANTVXDWRAPS

#endif // Not_VxD

/*XLATON*/

#endif /* _VMM_ */
