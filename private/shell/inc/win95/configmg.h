/*****************************************************************************
 *
 *	(C) Copyright MICROSOFT Corp., 1993-1998
 *
 *	Title:		CONFIGMG.H - Configuration manager header file
 *
 *	Version:	1.00
 *
 *	Date:		02-Feb-1993
 *
 *	Authors:	PYS & RAL
 *
 *------------------------------------------------------------------------------
 *
 *	Change log:
 *
 *	   DATE     REV DESCRIPTION
 *	----------- --- -----------------------------------------------------------
 *	02-Feb-1993 PYS Original
 *****************************************************************************/

#ifndef _CONFIGMG_H
#define	_CONFIGMG_H

#define	CONFIGMG_VERSION	0x0400

#define	PNPDRVS_Major_Ver	0x0004
#define	PNPDRVS_Minor_Ver	0x0000

#ifdef	MAXDEBUG
#define	CM_PERFORMANCE_INFO
#endif

#ifdef	GOLDEN	
#ifdef	RETAIL
#define	CM_GOLDEN_RETAIL
#endif
#endif

/*XLATOFF*/
#pragma	pack(1)
/*XLATON*/

#ifndef	NORESDES

/****************************************************************************
 *
 *			EQUATES FOR RESOURCE DESCRIPTOR
 *
 *	The equates for resource descriptor work the exact same way as those
 *	for VxD IDs, which is:
 *
 *	Device ID's are a combination of OEM # and device # in the form:
 *
 *		xOOOOOOOOOODDDDD
 *
 *	The high bit of the device ID is reserved for future use.  The next
 *	10 bits are the OEM # which is assigned by Microsoft.  The last 5 bits
 *	are the device #.  This allows each OEM to create 32 unique devices.
 *	If an OEM is creating a replacement for a standard device, then it
 *	should re-use the standard ID listed below.  Microsoft reserves the
 *	first 16 OEM #'s (0 thru 0Fh)
 *
 *	To make your resource ID, you must use the same 10 OEMs bit that
 *	have been given by Microsoft as OEM VxD ID range. You can then tag
 *	any of the 32 unique number in that range (it does not have to be
 *	the same as the VxD as some VxD may have mupltiple arbitrators).
 *
 *	If the ResType_Ignored_Bit is set, the resource is not arbitrated.
 *	You cannot register a handler for such a resource.
 *
 ***************************************************************************/
#define	ResType_All		0x00000000	// Return all resource types.
#define	ResType_None		0x00000000	// Arbitration always succeeded.
#define	ResType_Mem		0x00000001	// Physical address resource.
#define	ResType_IO		0x00000002	// Physical IO address resource.
#define	ResType_DMA		0x00000003	// DMA channels 0-7 resource.
#define	ResType_IRQ		0x00000004	// IRQ 0-15 resource.
#define	ResType_Max		0x00000004	// Max KNOWN ResType (for DEBUG).
#define	ResType_Ignored_Bit	0x00008000	// This resource is to be ignored.

#define	DEBUG_RESTYPE_NAMES \
char	CMFAR *lpszResourceName[ResType_Max+1]= \
{ \
	"All/None", \
	"Mem     ", \
	"IO      ", \
	"DMA     ", \
	"IRQ     ", \
};

/************************************************************************
 *									*
 *	OEMS WHO WANT A VXD DEVICE ID ASSIGNED TO THEM,  		*
 *	PLEASE CONTACT MICROSOFT PRODUCT SUPPORT 			*
 *									*
 ************************************************************************/

/****************************************************************************
 *
 * RESOURCE DESCRIPTORS
 *
 *	Each resource descriptor consists of an array of resource requests.
 *	Exactly one element of the array must be satisfied. The data
 *	of each array element is resource specific an described below.
 *	The data may specify one or more resource requests. At least
 *	one element of a Res_Des must be satisfied to satisfy the request
 *	represented by the Res_Des. The values allocated to the Res_Des
 *	are stored within the Res_Des.
 *	Each subarray (OR element) is a single Res_Des followed
 *	by data specific to the type of resource. The data includes the
 *	allocated resource (if any) followed by resource requests (which
 *	will include the values indicated by the allocated resource.
 *
 ***************************************************************************/

/****************************************************************************
 * Memory resource requests consist of ranges of pages
 ***************************************************************************/
#define	MType_Range		sizeof(struct Mem_Range_s)

#define	fMD_MemoryType		1		// Memory range is ROM/RAM
#define	fMD_ROM			0		// Memory range is ROM
#define	fMD_RAM			1		// Memory range is RAM
#define	fMD_32_24		2		// Memory range is 32/24 (for ISAPNP only)
#define	fMD_24			0		// Memory range is 24
#define	fMD_32			2		// Memory range is 32

/* Memory Range descriptor data
 */
struct	Mem_Range_s {
	ULONG			MR_Align;	// Mask for base alignment
	ULONG			MR_nBytes;	// Count of bytes
	ULONG			MR_Min;		// Min Address
	ULONG			MR_Max;		// Max Address
	WORD			MR_Flags;	// Flags
	WORD			MR_Reserved;
};

typedef	struct Mem_Range_s	MEM_RANGE;

/* Mem Resource descriptor header structure
 *	MD_Count * MD_Type bytes of data follow header in an
 *	array of MEM_RANGE structures. When an allocation is made,
 *	the allocated value is stored in the MD_Alloc_... variables.
 *
 *	Example for memory Resource Description:
 *		Mem_Des_s {
 *			MD_Count = 1;
 *			MD_Type = MTypeRange;
 *			MD_Alloc_Base = 0;
 *			MD_Alloc_End = 0;
 *			MD_Flags = 0;
 *			MD_Reserved = 0;
 *			};
 *		Mem_Range_s {
 *			MR_Align = 0xFFFFFF00;	// 256 byte alignment
 *			MR_nBytes = 32;		// 32 bytes needed
 *			MR_Min = 0;
 *			MR_Max = 0xFFFFFFFF;	// Any place in address space
 *			MR_Flags = 0;
 *			MR_Reserved = 0;
 *			};
 */
struct	Mem_Des_s {
	WORD			MD_Count;
	WORD			MD_Type;
	ULONG			MD_Alloc_Base;
	ULONG			MD_Alloc_End;
	WORD			MD_Flags;
	WORD			MD_Reserved;
};

typedef	struct Mem_Des_s 	MEM_DES;

/****************************************************************************
 * IO resource allocations consist of fixed ranges or variable ranges
 *	The Alias and Decode masks provide additional flexibility
 *	in specifying how the address is handled. They provide a convenient
 *	method for specifying what port aliases a card responds to. An alias
 *	is a port address that is responded to as if it were another address.
 *	Additionally, some cards will actually use additional ports for
 *	different purposes, but use a decoding scheme that makes it look as
 *	though it were using aliases. E.G., an ISA card may decode 10 bits
 *	and require port 03C0h. It would need to specify an Alias offset of
 *	04h and a Decode of 3 (no aliases are used as actual ports). For
 *	convenience, the alias field can be set to zero indicate no aliases
 *	are required and then decode is ignored.
 *	If the card were to use the ports at 7C0h, 0BC0h and 0FC0h, where these
 *	ports have different functionality, the Alias would be the same and the
 *	the decode would be 0Fh indicating bits 11 and 12 of the port address
 *	are significant. Thus, the allocation is for all of the ports
 *	(PORT[i] + (n*Alias*256)) & (Decode*256 | 03FFh), where n is
 *	any integer and PORT is the range specified by the nPorts, Min and
 *	Max fields. Note that the minimum Alias is 4 and the minimum
 *	Decode is 3.
 *	Because of the history of the ISA bus, all ports that can be described
 *	by the formula PORT = n*400h + zzzz, where "zzzz" is a port in the
 *	range 100h - 3FFh, will be checked for compatibility with the port
 *	zzzz, assuming that the port zzzz is using a 10 bit decode. If a card
 *	is on a local bus that can prevent the IO address from appearing on
 *	the ISA bus (e.g. PCI), then the logical configuration should specify
 *	an alias of IOA_Local which will prevent the arbitrator from checking
 *	for old ISA bus compatibility.
 */
#define	IOType_Range		sizeof(struct IO_Range_s) // Variable range

/* IO Range descriptor data */
struct	IO_Range_s {
	WORD			IOR_Align;	// Mask for base alignment
	WORD			IOR_nPorts;	// Number of ports
	WORD			IOR_Min;	// Min port address
	WORD			IOR_Max;	// Max port address
	WORD			IOR_RangeFlags;	// Flags
	BYTE			IOR_Alias;	// Alias offset
	BYTE			IOR_Decode;	// Address specified
};

typedef	struct IO_Range_s	IO_RANGE;

/* IO Resource descriptor header structure
 *	IOD_Count * IOD_Type bytes of data follow header in an
 *	array of IO_RANGE structures. When an allocation is made,
 *	the allocated value is stored in the IOD_Alloc_... variables.
 *
 *	Example for IO Resource Description:
 *		IO_Des_s {
 *			IOD_Count = 1;
 *			IOD_Type = IOType_Range;
 *			IOD_Alloc_Base = 0;
 *			IOD_Alloc_End = 0;
 *			IOD_Alloc_Alias = 0;
 *			IOD_Alloc_Decode = 0;
 *			IOD_DesFlags = 0;
 *			IOD_Reserved = 0;
 *			};
 *		IO_Range_s {
 *			IOR_Align = 0xFFF0;	// 16 byte alignment
 *			IOR_nPorts = 16;	// 16 ports required
 *			IOR_Min = 0x0100;
 *			IOR_Max = 0x03FF;	// Anywhere in ISA std ports
 *			IOR_RangeFlags = 0;
 *			IOR_Alias = 0004;	// Standard ISA 10 bit aliasing
 *			IOR_Decode = 0x000F;	// Use first 3 aliases (e.g. if
 *						// 0x100 were base port, 0x500
 *						// 0x900, and 0xD00 would
 *						// also be allocated)
 *			};
 */
struct	IO_Des_s {
	WORD			IOD_Count;
	WORD			IOD_Type;
	WORD			IOD_Alloc_Base;
	WORD			IOD_Alloc_End;
	WORD			IOD_DesFlags;
	BYTE			IOD_Alloc_Alias;
	BYTE			IOD_Alloc_Decode;
};

typedef	struct IO_Des_s 	IO_DES;

/* Definition for special alias value indicating card on PCI or similar local bus
 *  This value should used for the IOR_Alias and IOD_Alias fields
 */
#define	IOA_Local		0xff

/****************************************************************************
 * DMA channel resource allocations consist of one WORD channel bit masks.
 *	The mask indcates alternative channel allocations,
 *	one bit for each alternative (only one is allocated per mask).
 */

/*DMA flags
 *First two are DMA channel width: BYTE, WORD or DWORD
 */
#define	mDD_Width		0003h		// Mask for channel width
#define	fDD_BYTE		0
#define	fDD_WORD		1
#define	fDD_DWORD		2
#define	szDMA_Des_Flags		"WD"

/* DMA Resource descriptor structure
 *
 *	Example for DMA Resource Description:
 *
 *		DMA_Des_s {
 *			DD_Flags = fDD_Byte;	// Byte transfer
 *			DD_Alloc_Chan = 0;
 *			DD_Req_Mask = 0x60;	// Channel 5 or 6
 *			DD_Reserved = 0;
 *			};
 */
struct	DMA_Des_s {
	BYTE			DD_Flags;
	BYTE			DD_Alloc_Chan;	// Channel number allocated
	BYTE			DD_Req_Mask;	// Mask of possible channels
	BYTE			DD_Reserved;
};


typedef	struct DMA_Des_s 	DMA_DES;

/****************************************************************************
 * IRQ resource allocations consist of two WORD IRQ bit masks.
 *	The first mask indcates alternatives for IRQ allocation,
 *	one bit for each alternative (only one is allocated per mask). The
 *	second mask is used to specify that the IRQ can be shared.
 */

/*
 * IRQ flags
 */
#define	fIRQD_Share		1			// IRQ can be shared
#define	cIRQ_Des_Flags		'S'

/* IRQ Resource descriptor structure
 *
 *	Example for IRQ Resource Description:
 *
 *		IRQ_Des_s {
 *			IRQD_Flags = fIRQD_Share	// IRQ can be shared
 *			IRQD_Alloc_Num = 0;
 *			IRQD_Req_Mask = 0x18;		// IRQ 3 or 4
 *			IRQD_Reserved = 0;
 *			};
 */
struct	IRQ_Des_s {
	WORD			IRQD_Flags;
	WORD			IRQD_Alloc_Num;		// Allocated IRQ number
	WORD			IRQD_Req_Mask;		// Mask of possible IRQs
	WORD			IRQD_Reserved;
};

typedef	struct IRQ_Des_s 	IRQ_DES;

/*XLATOFF*/

/****************************************************************************
 *
 * 'C'-only defined total resource structure. Since a resource consists of
 * one resource header followed by an undefined number of resource data
 * structure, we use the undefined array size [] on the *_DATA structure
 * member. Unfortunately, this does not H2INC since the total size of the
 * array cannot be computed from the definition.
 *
 ***************************************************************************/

#pragma warning (disable:4200)			// turn off undefined array size

typedef	MEM_DES			*PMEM_DES;
typedef	MEM_RANGE		*PMEM_RANGE;
typedef	IO_DES			*PIO_DES;
typedef	IO_RANGE		*PIO_RANGE;
typedef	DMA_DES			*PDMA_DES;
typedef	IRQ_DES			*PIRQ_DES;

struct	MEM_Resource_s {
	MEM_DES			MEM_Header;
	MEM_RANGE		MEM_Data[];
};

typedef	struct MEM_Resource_s	MEM_RESOURCE;
typedef	MEM_RESOURCE		*PMEM_RESOURCE;

struct	MEM_Resource1_s {
	MEM_DES			MEM_Header;
	MEM_RANGE		MEM_Data;
};

typedef	struct MEM_Resource1_s	MEM_RESOURCE1;
typedef	MEM_RESOURCE1		*PMEM_RESOURCE1;

#define	SIZEOF_MEM(x)		(sizeof(MEM_DES)+(x)*sizeof(MEM_RANGE))

struct	IO_Resource_s {
	IO_DES			IO_Header;
	IO_RANGE		IO_Data[];
};

typedef	struct IO_Resource_s	IO_RESOURCE;
typedef	IO_RESOURCE		*PIO_RESOURCE;

struct	IO_Resource1_s {
	IO_DES			IO_Header;
	IO_RANGE		IO_Data;
};

typedef	struct IO_Resource1_s	IO_RESOURCE1;
typedef	IO_RESOURCE1		*PIO_RESOURCE1;

#define	SIZEOF_IORANGE(x)	(sizeof(IO_DES)+(x)*sizeof(IO_RANGE))

struct	DMA_Resource_s {
	DMA_DES			DMA_Header;
};

typedef	struct DMA_Resource_s	DMA_RESOURCE;

#define	SIZEOF_DMA		sizeof(DMA_DES)

struct	IRQ_Resource_s {
	IRQ_DES			IRQ_Header;
};

typedef	struct IRQ_Resource_s	IRQ_RESOURCE;

#define	SIZEOF_IRQ		sizeof(IRQ_DES)

#pragma warning (default:4200)			// turn on undefined array size

/*XLATON*/

#endif	// ifndef NORESDES

#define	LCPRI_FORCECONFIG	0x00000000	// Logical configuration priorities.
#define	LCPRI_BOOTCONFIG	0x00000001
#define	LCPRI_HARDWIRED		0x00001000
#define	LCPRI_DESIRED		0x00002000
#define	LCPRI_NORMAL		0x00003000
#define	LCPRI_LASTBESTCONFIG	0x00003FFF	// CM ONLY, DO NOT USE.
#define	LCPRI_SUBOPTIMAL	0x00005000
#define	LCPRI_LASTSOFTCONFIG	0x00007FFF	// CM ONLY, DO NOT USE.
#define	LCPRI_RESTART		0x00008000
#define	LCPRI_REBOOT		0x00009000
#define	LCPRI_POWEROFF		0x0000A000
#define	LCPRI_HARDRECONFIG	0x0000C000
#define	LCPRI_DISABLED		0x0000FFFF
#define	MAX_LCPRI		0x0000FFFF

#define	MAX_MEM_REGISTERS		9
#define	MAX_IO_PORTS			20
#define	MAX_IRQS			7
#define	MAX_DMA_CHANNELS		7

struct Config_Buff_s {
WORD	wNumMemWindows;			// Num memory windows
DWORD	dMemBase[MAX_MEM_REGISTERS];	// Memory window base
DWORD	dMemLength[MAX_MEM_REGISTERS];	// Memory window length
WORD	wMemAttrib[MAX_MEM_REGISTERS];	// Memory window Attrib
WORD	wNumIOPorts;			// Num IO ports
WORD	wIOPortBase[MAX_IO_PORTS];	// I/O port base
WORD	wIOPortLength[MAX_IO_PORTS];	// I/O port length
WORD	wNumIRQs;			// Num IRQ info
BYTE	bIRQRegisters[MAX_IRQS];	// IRQ list
BYTE	bIRQAttrib[MAX_IRQS];		// IRQ Attrib list
WORD	wNumDMAs;			// Num DMA channels
BYTE	bDMALst[MAX_DMA_CHANNELS];	// DMA list
WORD	wDMAAttrib[MAX_DMA_CHANNELS];	// DMA Attrib list
BYTE	bReserved1[3];			// Reserved
};

typedef	struct Config_Buff_s	CMCONFIG;	// Config buffer info

#ifndef	CMJUSTRESDES

#define	MAX_DEVICE_ID_LEN	200

#include <vmmreg.h>

/*XLATOFF*/

#ifdef	Not_VxD

#include <dbt.h>

#pragma warning(disable:4001)	// Non-standard extensions
#pragma warning(disable:4505)	// Unreferenced local functions

#ifdef	IS_32

#define	CMFAR

#else

#define	CMFAR	_far

#endif

#else	// Not_VxD

#define	CMFAR

#endif	// Not_VxD

#ifdef	IS_32

typedef	DWORD			RETURN_TYPE;

#else	// IS_32

typedef	WORD			RETURN_TYPE;

#endif	// IS_32

#define	CONFIGMG_Service	Declare_Service
/*XLATON*/

/*MACROS*/
Begin_Service_Table(CONFIGMG, VxD)
CONFIGMG_Service	(_CONFIGMG_Get_Version, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Initialize, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Locate_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Parent, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Child, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Sibling, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_ID_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_ID, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Depth, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Create_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Remove_SubTree, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Remove_SubTree, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Device_Driver, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Enumerator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Arbitrator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Deregister_Arbitrator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Arbitrator_Free_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Arbitrator_Free_Data, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Sort_NodeList, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Yield, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Lock, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Unlock, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Empty_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_First_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Next_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Modify_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Next_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Performance_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Res_Des_Data_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Res_Des_Data, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Process_Events_Now, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Create_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Delete_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Test_Range_Available, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Dup_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Invert_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Intersect_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_First_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Next_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Dump_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Load_DLVxDs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DDBs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_CRC_CheckSum, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_DevLoader, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Reenumerate_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Setup_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Reset_Children_Marks, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Status, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Remove_Unmarked_Children, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_ISAPNP_To_CM, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CallBack_Device_Driver, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CallBack_Enumerator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Alloc_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Key_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Key, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Read_Registry_Value, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Write_Registry_Value, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Disable_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Enable_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Move_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Bus_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Bus_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Recompute_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Read_Registry_Log_Confs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Run_Detection, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Call_At_Appy_Time, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Fail_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Private_Problem, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Debug_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Hardware_Profile_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Enumerator_Function, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Call_Enumerator_Function, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_ID, VxD_CODE)
End_Service_Table(CONFIGMG, VxD)

/*ENDMACROS*/

/*XLATOFF*/

#define	NUM_CM_SERVICES		((WORD)(Num_CONFIGMG_Services & 0xFFFF))

#define	DEBUG_SERVICE_NAMES \
char	CMFAR *lpszServiceName[NUM_CM_SERVICES]= \
{ \
	"Get_Version", \
	"Initialize", \
	"Locate_DevNode", \
	"Get_Parent", \
	"Get_Child", \
	"Get_Sibling", \
	"Get_Device_ID_Size", \
	"Get_Device_ID", \
	"Get_Depth", \
	"Get_Private_DWord", \
	"Set_Private_DWord", \
	"Create_DevNode", \
	"Query_Remove_SubTree", \
	"Remove_SubTree", \
	"Register_Device_Driver", \
	"Register_Enumerator", \
	"Register_Arbitrator", \
	"Deregister_Arbitrator", \
	"Query_Arbitrator_Free_Size", \
	"Query_Arbitrator_Free_Data", \
	"Sort_NodeList", \
	"Yield", \
	"Lock", \
	"Unlock", \
	"Add_Empty_Log_Conf", \
	"Free_Log_Conf", \
	"Get_First_Log_Conf", \
	"Get_Next_Log_Conf", \
	"Add_Res_Des", \
	"Modify_Res_Des", \
	"Free_Res_Des", \
	"Get_Next_Res_Des", \
	"Get_Performance_Info", \
	"Get_Res_Des_Data_Size", \
	"Get_Res_Des_Data", \
	"Process_Events_Now", \
	"Create_Range_List", \
	"Add_Range", \
	"Delete_Range", \
	"Test_Range_Available", \
	"Dup_Range_List", \
	"Free_Range_List", \
	"Invert_Range_List", \
	"Intersect_Range_List", \
	"First_Range", \
	"Next_Range", \
	"Dump_Range_List", \
	"Load_DLVxDs", \
	"Get_DDBs", \
	"Get_CRC_CheckSum", \
	"Register_DevLoader", \
	"Reenumerate_DevNode", \
	"Setup_DevNode", \
	"Reset_Children_Marks", \
	"Get_DevNode_Status", \
	"Remove_Unmarked_Children", \
	"ISAPNP_To_CM", \
	"CallBack_Device_Driver", \
	"CallBack_Enumerator", \
	"Get_Alloc_Log_Conf", \
	"Get_DevNode_Key_Size", \
	"Get_DevNode_Key", \
	"Read_Registry_Value", \
	"Write_Registry_Value", \
	"Disable_DevNode", \
	"Enable_DevNode", \
	"Move_DevNode", \
	"Set_Bus_Info", \
	"Get_Bus_Info", \
	"Set_HW_Prof", \
	"Recompute_HW_Prof", \
	"Query_Change_HW_Prof", \
	"Get_Device_Driver_Private_DWord", \
	"Set_Device_Driver_Private_DWord", \
	"Get_HW_Prof_Flags", \
	"Set_HW_Prof_Flags", \
	"Read_Registry_Log_Confs", \
	"Run_Detection", \
	"Call_At_Appy_Time", \
	"Fail_Change_HW_Prof", \
	"Set_Private_Problem", \
	"Debug_DevNode", \
	"Get_Hardware_Profile_Info", \
	"Register_Enumerator_Function", \
	"Call_Enumerator_Function", \
	"Add_ID", \
};

/*XLATON*/

/****************************************************************************
 *
 *				GLOBALLY DEFINED TYPEDEFS
 *
 ***************************************************************************/
typedef	RETURN_TYPE		CONFIGRET;	// Standardized return value.
typedef	PPVMMDDB		*PPPVMMDDB;	// Too long to describe.
typedef	VOID		CMFAR	*PFARVOID;	// Pointer to a VOID.
typedef	ULONG		CMFAR	*PFARULONG;	// Pointer to a ULONG.
typedef	char		CMFAR	*PFARCHAR;	// Pointer to a string.
typedef	VMMHKEY		CMFAR	*PFARHKEY;	// Pointer to a HKEY.
typedef	char		CMFAR	*DEVNODEID;	// Device ID ANSI name.
typedef	DWORD			LOG_CONF;	// Logical configuration.
typedef	LOG_CONF	CMFAR	*PLOG_CONF;	// Pointer to logical configuration.
typedef	DWORD			RES_DES;	// Resource descriptor.
typedef	RES_DES		CMFAR	*PRES_DES;	// Pointer to resource descriptor.
typedef	DWORD			DEVNODE;	// Devnode.
typedef	DEVNODE		CMFAR	*PDEVNODE;	// Pointer to devnode.
typedef	DWORD			NODELIST;	// Pointer to a nodelist element.
typedef	DWORD			NODELIST_HEADER;// Pointer to a nodelist header.
typedef	DWORD			REGISTERID;	// Arbitartor registration.
typedef	REGISTERID	CMFAR	*PREGISTERID;	// Pointer to arbitartor registration.
typedef	ULONG			RESOURCEID;	// Resource type ID.
typedef	RESOURCEID	CMFAR	*PRESOURCEID;	// Pointer to resource type ID.
typedef	ULONG			PRIORITY;	// Priority number.
typedef	DWORD			RANGE_LIST;	// Range list handle.
typedef	RANGE_LIST	CMFAR	*PRANGE_LIST;	// Pointer to a range list handle.
typedef	DWORD			RANGE_ELEMENT;	// Range list element handle.
typedef	RANGE_ELEMENT	CMFAR	*PRANGE_ELEMENT;// Pointer to a range element handle.
typedef	DWORD			LOAD_TYPE;	// For the loading function.
typedef	CMCONFIG	CMFAR	*PCMCONFIG;	// Pointer to a config buffer info.
typedef	DWORD			CMBUSTYPE;	// Type of the bus.
typedef	CMBUSTYPE	CMFAR	*PCMBUSTYPE;	// Pointer to a bus type.
typedef	double			VMM_TIME;	// Time in microticks.
#define	LODWORD(x)		((DWORD)(x))
#define	HIDWORD(x)		(*(PDWORD)(PDWORD(&x)+1))

typedef	ULONG			CONFIGFUNC;
typedef	ULONG			SUBCONFIGFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMCONFIGHANDLER)(CONFIGFUNC, SUBCONFIGFUNC, DEVNODE, ULONG, ULONG);
typedef	CONFIGRET		(CMFAR _cdecl *CMENUMHANDLER)(CONFIGFUNC, SUBCONFIGFUNC, DEVNODE, DEVNODE, ULONG);
typedef	VOID			(CMFAR _cdecl *CMAPPYCALLBACKHANDLER)(ULONG);

typedef	ULONG			ENUMFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMENUMFUNCTION)(ENUMFUNC, ULONG, DEVNODE, PFARVOID, ULONG);

typedef	ULONG			ARBFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMARBHANDLER)(ARBFUNC, ULONG, DEVNODE, NODELIST_HEADER);

/****************************************************************************
 *
 *				CONFIGURATION MANAGER BUS TYPE
 *
 ***************************************************************************/
#define	BusType_None		0x00000000
#define	BusType_ISA		0x00000001
#define	BusType_EISA		0x00000002
#define	BusType_PCI		0x00000004
#define	BusType_PCMCIA		0x00000008
#define	BusType_ISAPNP		0x00000010
#define	BusType_MCA		0x00000020

/****************************************************************************
 *
 *				CONFIGURATION MANAGER RETURN VALUES
 *
 ***************************************************************************/
#define	CR_SUCCESS		0x00000000
#define	CR_DEFAULT		0x00000001
#define	CR_OUT_OF_MEMORY	0x00000002
#define	CR_INVALID_POINTER	0x00000003
#define	CR_INVALID_FLAG		0x00000004
#define	CR_INVALID_DEVNODE	0x00000005
#define	CR_INVALID_RES_DES	0x00000006
#define	CR_INVALID_LOG_CONF	0x00000007
#define	CR_INVALID_ARBITRATOR	0x00000008
#define	CR_INVALID_NODELIST	0x00000009
#define	CR_DEVNODE_HAS_REQS	0x0000000A
#define	CR_INVALID_RESOURCEID	0x0000000B
#define	CR_DLVXD_NOT_FOUND	0x0000000C
#define	CR_NO_SUCH_DEVNODE	0x0000000D
#define	CR_NO_MORE_LOG_CONF	0x0000000E
#define	CR_NO_MORE_RES_DES	0x0000000F
#define	CR_ALREADY_SUCH_DEVNODE	0x00000010
#define	CR_INVALID_RANGE_LIST	0x00000011
#define	CR_INVALID_RANGE	0x00000012
#define	CR_FAILURE		0x00000013
#define	CR_NO_SUCH_LOGICAL_DEV	0x00000014
#define	CR_CREATE_BLOCKED	0x00000015
#define	CR_NOT_SYSTEM_VM	0x00000016
#define	CR_REMOVE_VETOED	0x00000017
#define	CR_APM_VETOED		0x00000018
#define	CR_INVALID_LOAD_TYPE	0x00000019
#define	CR_BUFFER_SMALL		0x0000001A
#define	CR_NO_ARBITRATOR	0x0000001B
#define	CR_NO_REGISTRY_HANDLE	0x0000001C
#define	CR_REGISTRY_ERROR	0x0000001D
#define	CR_INVALID_DEVICE_ID	0x0000001E
#define	CR_INVALID_DATA		0x0000001F
#define	CR_INVALID_API		0x00000020
#define	CR_DEVLOADER_NOT_READY	0x00000021
#define	CR_NEED_RESTART		0x00000022
#define	CR_INTERRUPTS_DISABLED	0x00000023
#define	CR_DEVICE_NOT_THERE	0x00000024
#define	CR_NO_SUCH_VALUE	0x00000025
#define	CR_WRONG_TYPE		0x00000026
#define	CR_INVALID_PRIORITY	0x00000027
#define	CR_NOT_DISABLEABLE	0x00000028
#define	CR_NO_MORE_HW_PROFILES	0x00000029
#define	NUM_CR_RESULTS		0x0000002A

/*XLATOFF*/

#define	DEBUG_RETURN_CR_NAMES \
char	CMFAR *lpszReturnCRName[NUM_CR_RESULTS]= \
{ \
	"CR_SUCCESS", \
	"CR_DEFAULT", \
	"CR_OUT_OF_MEMORY", \
	"CR_INVALID_POINTER", \
	"CR_INVALID_FLAG", \
	"CR_INVALID_DEVNODE", \
	"CR_INVALID_RES_DES", \
	"CR_INVALID_LOG_CONF", \
	"CR_INVALID_ARBITRATOR", \
	"CR_INVALID_NODELIST", \
	"CR_DEVNODE_HAS_REQS", \
	"CR_INVALID_RESOURCEID", \
	"CR_DLVXD_NOT_FOUND", \
	"CR_NO_SUCH_DEVNODE", \
	"CR_NO_MORE_LOG_CONF", \
	"CR_NO_MORE_RES_DES", \
	"CR_ALREADY_SUCH_DEVNODE", \
	"CR_INVALID_RANGE_LIST", \
	"CR_INVALID_RANGE", \
	"CR_FAILURE", \
	"CR_NO_SUCH_LOGICAL_DEVICE", \
	"CR_CREATE_BLOCKED", \
	"CR_NOT_SYSTEM_VM", \
	"CR_REMOVE_VETOED", \
	"CR_APM_VETOED", \
	"CR_INVALID_LOAD_TYPE", \
	"CR_BUFFER_SMALL", \
	"CR_NO_ARBITRATOR", \
	"CR_NO_REGISTRY_HANDLE", \
	"CR_REGISTRY_ERROR", \
	"CR_INVALID_DEVICE_ID", \
	"CR_INVALID_DATA", \
	"CR_INVALID_API", \
	"CR_DEVLOADER_NOT_READY", \
	"CR_NEED_RESTART", \
	"CR_INTERRUPTS_DISABLED", \
	"CR_DEVICE_NOT_THERE", \
	"CR_NO_SUCH_VALUE", \
	"CR_WRONG_TYPE", \
	"CR_INVALID_PRIORITY", \
	"CR_NOT_DISABLEABLE", \
	"CR_NO_MORE_HW_PROFILES", \
};

/*XLATON*/

#define	CM_PROB_NOT_CONFIGURED			0x00000001
#define	CM_PROB_DEVLOADER_FAILED		0x00000002
#define	CM_PROB_OUT_OF_MEMORY			0x00000003
#define	CM_PROB_ENTRY_IS_WRONG_TYPE		0x00000004
#define	CM_PROB_LACKED_ARBITRATOR		0x00000005
#define	CM_PROB_BOOT_CONFIG_CONFLICT		0x00000006
#define	CM_PROB_FAILED_FILTER			0x00000007
#define	CM_PROB_DEVLOADER_NOT_FOUND		0x00000008
#define	CM_PROB_INVALID_DATA			0x00000009
#define	CM_PROB_FAILED_START			0x0000000A
#define	CM_PROB_LIAR				0x0000000B
#define	CM_PROB_NORMAL_CONFLICT			0x0000000C
#define	CM_PROB_NOT_VERIFIED			0x0000000D
#define	CM_PROB_NEED_RESTART			0x0000000E
#define	CM_PROB_REENUMERATION			0x0000000F
#define	CM_PROB_PARTIAL_LOG_CONF		0x00000010
#define	CM_PROB_UNKNOWN_RESOURCE		0x00000011
#define	CM_PROB_REINSTALL			0x00000012
#define	CM_PROB_REGISTRY			0x00000013
#define	CM_PROB_VXDLDR				0x00000014
#define	CM_PROB_WILL_BE_REMOVED			0x00000015
#define	CM_PROB_DISABLED			0x00000016
#define	CM_PROB_DEVLOADER_NOT_READY		0x00000017
#define	CM_PROB_DEVICE_NOT_THERE		0x00000018
#define	CM_PROB_MOVED				0x00000019
#define	CM_PROB_TOO_EARLY			0x0000001A
#define	CM_PROB_NO_VALID_LOG_CONF		0x0000001B
#define	NUM_CM_PROB				0x0000001C

/*XLATOFF*/

#define	DEBUG_CM_PROB_NAMES \
char	CMFAR *lpszCMProbName[NUM_CM_PROB]= \
{ \
	"No Problem", \
	"No ConfigFlags (not configured)", \
	"Devloader failed", \
	"Run out of memory", \
	"Devloader/StaticVxD/Configured is of wrong type", \
	"Lacked an arbitrator", \
	"Boot config conflicted", \
	"Filtering failed", \
	"Devloader not found", \
	"Invalid data in registry", \
	"Device failed to start", \
	"Device failed something not failable", \
	"Was normal conflicting", \
	"Did not verified", \
	"Need restart", \
	"Is probably reenumeration", \
	"Was not fully detected", \
	"Resource number was not found", \
	"Reinstall", \
	"Registry returned unknown result", \
	"VxDLdr returned unknown result", \
	"Will be removed", \
	"Disabled", \
	"Devloader was not ready", \
	"Device not there", \
	"Was moved", \
	"Too early", \
	"No valid log conf", \
};

/*XLATON*/

#define	CM_INITIALIZE_VMM			0x00000000
#define	CM_INITIALIZE_VXDLDR			0x00000001
#define	CM_INITIALIZE_BITS			0x00000001

#define	CM_YIELD_NO_RESUME_EXEC			0x00000000
#define	CM_YIELD_RESUME_EXEC			0x00000001
#define	CM_YIELD_BITS				0x00000001

#define	CM_CREATE_DEVNODE_NORMAL		0x00000000
#define	CM_CREATE_DEVNODE_NO_WAIT_INSTALL	0x00000001
#define	CM_CREATE_DEVNODE_BITS			0x00000001

#define	CM_REGISTER_DEVICE_DRIVER_STATIC	0x00000000
#define	CM_REGISTER_DEVICE_DRIVER_DISABLEABLE	0x00000001
#define	CM_REGISTER_DEVICE_DRIVER_REMOVABLE	0x00000002
#define	CM_REGISTER_DEVICE_DRIVER_BITS		0x00000003

#define	CM_REGISTER_ENUMERATOR_SOFTWARE		0x00000000
#define	CM_REGISTER_ENUMERATOR_HARDWARE		0x00000001
#define	CM_REGISTER_ENUMERATOR_BITS		0x00000001

#define	CM_SETUP_DEVNODE_READY			0x00000000
#define	CM_SETUP_DOWNLOAD			0x00000001
#define	CM_SETUP_BITS				0x00000001

#define	CM_ADD_RANGE_ADDIFCONFLICT		0x00000000
#define	CM_ADD_RANGE_DONOTADDIFCONFLICT		0x00000001
#define	CM_ADD_RANGE_BITS			0x00000001

#define	CM_ISAPNP_ADD_RES_DES			0x00000000
#define	CM_ISAPNP_SETUP				0x00000001
#define	CM_ISAPNP_ADD_BOOT_RES_DES		0x00000002
#define	CM_ISAPNP_INVALID			0x00000003
#define	CM_ISAPNP_BITS				0x00000003

#define	CM_GET_PERFORMANCE_INFO_DATA		0x00000000
#define	CM_GET_PERFORMANCE_INFO_RESET		0x00000001
#define	CM_GET_PERFORMANCE_INFO_START		0x00000002
#define	CM_GET_PERFORMANCE_INFO_STOP		0x00000003
#define	CM_RESET_HIT_DATA			0x00000004
#define	CM_GET_HIT_DATA 			0x00000005
#define	CM_GET_PERFORMANCE_INFO_BITS		0x0000000F
#define	CM_HIT_DATA_FILES			0xFFFF0000
#define	CM_HIT_DATA_SIZE			((256*8)+8)  // magic number!

#define	CM_GET_ALLOC_LOG_CONF_ALLOC		0x00000000
#define	CM_GET_ALLOC_LOG_CONF_BOOT_ALLOC	0x00000001
#define	CM_GET_ALLOC_LOG_CONF_BITS		0x00000001

#define	CM_REGISTRY_HARDWARE			0x00000000	// Select hardware branch if NULL subkey
#define	CM_REGISTRY_SOFTWARE			0x00000001	// Select software branch if NULL subkey
#define	CM_REGISTRY_USER			0x00000100	// Use HKEY_CURRENT_USER
#define	CM_REGISTRY_CONFIG			0x00000200	// Use HKEY_CURRENT_CONFIG
#define	CM_REGISTRY_BITS			0x00000301	// The bits for the registry functions

#define	CM_DISABLE_POLITE			0x00000000	// Ask the driver
#define	CM_DISABLE_ABSOLUTE			0x00000001	// Don't ask the driver
#define	CM_DISABLE_BITS				0x00000001	// The bits for the disable function

#define	CM_HW_PROF_UNDOCK			0x00000000	// Computer not in a dock.
#define	CM_HW_PROF_DOCK				0x00000001	// Computer in a docking station
#define	CM_HW_PROF_RECOMPUTE_BITS		0x00000001	// RecomputeConfig
#define	CM_HW_PROF_DOCK_KNOWN			0x00000002	// Computer in a known docking station
#define	CM_HW_PROF_QUERY_CHANGE_BITS		0x00000003	// QueryChangeConfig

#define	CM_DETECT_NEW_PROFILE			0x00000001	// run detect for a new profile
#define	CM_DETECT_CRASHED			0x00000002	// detection crashed before
#define	CM_DETECT_HWPROF_FIRST_BOOT		0x00000004	// first boot in a new profile
#define	CM_DETECT_RUN				0x80000000	// run detection for new hardware

#define	CM_ADD_ID_HARDWARE			0x00000000
#define	CM_ADD_ID_COMPATIBLE			0x00000001
#define	CM_ADD_ID_BITS				0x00000001

#define	CM_REENUMERATE_NORMAL			0x00000000
#define	CM_REENUMERATE_SYNCHRONOUS		0x00000001
#define	CM_REENUMERATE_BITS			0x00000001

/****************************************************************************
 *
 *				CONFIGURATION MANAGER FUNCTIONS
 *
 ****************************************************************************
 *
 *	Each devnode has a config handler field and a enum handler field
 *	which are getting called every time Configuration Manager wants a
 *	devnode to perform some configuration related function. The handler
 *	is registered with CM_Register_Device_Driver or
 *	CM_Register_Enumerator, depending if the handler is for the device
 *	itself or for one of the children of the devnode.
 *
 *	The registered handler is called with:
 *
 *	result=dnToDevNode->dn_Config(if dnToDevNode==dnAboutDevNode)
 *	result=dnToDevNode->dn_Enum(if dnToDevNode!=dnAboutDevNode)
 *					(	FuncName,
 *					 	SubFuncName,
 *						dnToDevNode,
 *						dnAboutDevNode, (if enum)
 *						dwRefData, (if driver)
 *						ulFlags);
 *	Where:
 *
 *	FuncName is one of CONFIG_FILTER, CONFIG_START, CONFIG_STOP,
 *	CONFIG_TEST, CONFIG_REMOVE, CONFIG_ENUMERATE, CONFIG_SETUP or
 *	CONFIG_CALLBACK.
 *
 *	SubFuncName is the specific CONFIG_xxxx_* that further describe
 *	we START, STOP or TEST.
 *
 *	dnToDevNode is the devnode we are calling. This is given so that
 *	a signle handler can handle multiple devnodes.
 *
 *	dnAboutDevNode specifies which devnode the function is about. For
 *	a config handler, this is necessarily the same as dnToDevNode. For
 *	an enumerator handler, this devnode is necessarily different as it
 *	is a child of the dnToDevNode (special case: CONFIG_ENUMERATE
 *	necessarily has dnAboutDevNode==NULL). For instance, when starting
 *	a COM devnode under a BIOS enumerator, we would make the following
 *	two calls:
 *
 *		To BIOS with (CONFIG_START, ?, BIOS, COM, ?, 0).
 *
 *		To COM with (CONFIG_START, ?, COM, COM, ?, 0).
 *
 *	dwRefData is a dword of reference data. For a config handler, it is
 *	the DWORD passed on the CONFIGMG_Register_Device_Driver call. For an
 *	enumerator, it is the same as CONFIGMG_Get_Private_DWord(?,
 *	dnToDevNode, dnToDevNode, 0).
 *
 *	ulFlags is 0 and is reserved for future extensions.
 *
 *	Here is the explanation of each event, in parenthesis I put the
 *	order the devnodes will be called:
 *
 *	CONFIG_FILTER (BRANCH GOING UP) is the first thing called when a new
 *	insertion or change of configuration need to be processed. First
 *	CM copies the requirement list (BASIC_LOG_CONF) onto the filtered
 *	requirement list (FILTER_LOG_CONF) so that they are originally
 *	the same. CM then calls every node up, giving them the chance to
 *	patch the requirement of the dnAboutDevNode (they can also
 *	alter their own requirement). Examples are PCMCIA which would
 *	remove some IRQ that the adapter can't do, prealloc some IO
 *	windows and memory windows. ISA which would limit address space
 *	to being <16Meg. A device driver should look only at
 *	FILTER_LOG_CONF during this call.
 *
 *	CONFIG_START (BRANCH GOING DOWN) are called to change the
 *	configuration. A config handler/enumerator hander should look
 *	only at the allocated list (ALLOC_LOG_CONF).
 *
 *	CONFIG_STOP (WHOLE TREE BUT ONLY DEVNODES THAT CHANGE
 *	CONFIGURATION (FOR EACH DEVNODE, BRANCH GOING UP)) is called
 *	for two reasons:
 *
 *		1) Just after the rebalance algorithm came up with a
 *		solution and we want to stop all devnodes that will be
 *		rebalance. This is to avoid the problem of having two cards
 *		that can respond to 110h and 220h and that need to toggle
 *		their usage. We do not want two people responding to 220h,
 *		even for a brief amount of time. This is the normal call
 *		though.
 *
 *		2) There was a conflict and the user selected this device
 *		to kill.
 *
 *	CONFIG_TEST (WHOLE TREE) is called before starting the rebalance
 *	algorithm. Device drivers that fail this call will be considered
 *	worst than jumpered configured for the reminder of this balancing
 *	process.
 *
 *	CONFIG_REMOVE (FOR EACH SUB TREE NODE, DOING BRANCH GOING UP), is
 *	called when someone notify CM via CM_Remove_SubTree that a devnode
 *	is not needed anymore. A static VxD probably has nothing to do. A
 *	dynamic VxD should check whether it should unload itself (return
 *	CR_SUCCESS_UNLOAD) or not (CR_SUCCESS).
 *
 *	Note, failing any of CONFIG_START, or CONFIG_STOP is really bad,
 *	both in terms of performance and stability. Requirements for a
 *	configuration to succeed should be noted/preallocated during
 *	CONFIG_FILTER. Failing CONFIG_TEST is less bad as what basically
 *	happens is that the devnode is considered worst than jumpered
 *	configured for the reminder of this pass of the balancing algorithm.
 *
 *	COMFIG_ENUMERATE, the called node should create children devnodes
 *	using CM_Create_DevNode (but no need for grand children) and remove
 *	children using CM_Remove_SubTree as appropriate. Config Manager
 *	will recurse calling the children until nothing new appears. During
 *	this call, dnAboutDevNode will be NULL. Note that there is an easy
 *	way for buses which do not have direct children accessibility to
 *	detect (ISAPNP for instance will isolate one board at a time and
 *	there is no way to tell one specific board not to participate in
 *	the isolation sequence):
 *
 *	If some children have soft-eject capability, check those first.
 *	If the user is pressing the eject button, call Query_Remove_SubTree
 *	and if that succeed, call Remove_SubTree.
 *
 *	Do a CM_Reset_Children_Marks on the bus devnode.
 *
 *	Do the usual sequence doing CM_Create_DevNode calls. If a devnode
 *	was already there, CR_ALREADY_SUCH_DEVNODE is returned and this
 *	devnode's DN_HAS_MARK will be set. There is nothing more to do with
 *	this devnode has it should just continue running. If the devnode
 *	was not previously there, CR_SUCCESS will be return, in which case
 *	the enumerator should add the logical configurations.
 *
 *	Once all the devnode got created. The enumerator can call
 *	CM_Remove_Unmarked_Children to remove the devnode that are now gone.
 *	Essentially, this is a for loop thru all the children of the bus
 *	devnode, doing Remove_SubTree on the the devnode which have their
 *	mark cleared. Alternatively, an enumerator can use CM_Get_Child,
 *	CM_Get_Sibling, CM_Remove_SubTree and CM_Get_DevNode_Status.
 *
 *	For CONFIG_SETUP, the called node should install drivers if it
 *	know out to get them. This is mostly for drivers imbeded in the
 *	cards (ISA_RTR, PCI or PCMCIA). For most old cards/driver, this
 *	should return CR_NO_DRIVER.
 *
 *	WARNING: For any non-defined service, the enumertor / device
 *	driver handler should return CR_DEFAULT. This will be treated
 *	as the compatibility case in future version.
 *
 *	So normally what happens is as follows:
 *
 *	- Some detection code realize there is a new device. This can be at
 *	initialization time or at run-time (usually during a media_change
 *	interrupt). The code does a CM_Reenumerate_DevNode(dnBusDevNode)
 *	asynchronous call.
 *
 *	- During appy time event, CM gets notified.
 *
 *	- CM calls the enumerator with:
 *
 *		BusEnumHandler(CONFIG_ENUMERATE, 0, dnBusDevNode, NULL, ?, 0);
 *
 *	- The parent uses CM_Create_DevNode and CM_Remove_SubTree as
 *	appropriate, usually for only its immediate children.
 *
 *	- The parent return to CM from the enumerator call.
 *
 *	- CM walks the children, first loading their device driver if
 *	needed, then calling their enumerators. Thus the whole process
 *	will terminate only when all grand-...-grand-children have stopped
 *	using CM_Create_DevNode.
 *
 *	If rebalance is called (a new devnode is conflicting):
 *
 *	- All devnode receives the CONFIG_TEST. Devnodes that
 *	fail it are considered worst than jumpered configured.
 *
 *	- CM does the rebalance algorithm.
 *
 *	- All affected devnodes that where previously loaded get the
 *	CONFIG_STOP event.
 *
 *	- All affected devnode and the new devnodes receives a CONFIG_START.
 *
 *	If rebalancing failed (couldn't make one or more devnodes work):
 *
 *	- Device installer is called which will present the user with a
 *	choice of devnode to kill.
 *
 *	- Those devnodes will received a CONFIG_STOP message.
 *	
 ***************************************************************************/

// Possible CONFIGFUNC FuncNames:

#define	CONFIG_FILTER		0x00000000	// Ancestors must filter requirements.
#define	CONFIG_START		0x00000001	// Devnode dynamic initialization.
#define	CONFIG_STOP		0x00000002	// Devnode must stop using config.
#define	CONFIG_TEST		0x00000003	// Can devnode change state now.
#define	CONFIG_REMOVE		0x00000004	// Devnode must stop using config.
#define	CONFIG_ENUMERATE	0x00000005	// Devnode must enumerated.
#define	CONFIG_SETUP		0x00000006	// Devnode should download driver.
#define	CONFIG_CALLBACK		0x00000007	// Devnode is being called back.
#define	CONFIG_APM		0x00000008	// APM functions.
#define	CONFIG_TEST_FAILED	0x00000009	// Continue as before after a TEST.
#define	CONFIG_TEST_SUCCEEDED	0x0000000A	// Prepare for the STOP/REMOVE.
#define	CONFIG_VERIFY_DEVICE	0x0000000B	// Insure the legacy card is there.
#define	CONFIG_PREREMOVE	0x0000000C	// Devnode must stop using config.
#define	CONFIG_SHUTDOWN		0x0000000D	// We are shutting down.
#define	CONFIG_PREREMOVE2	0x0000000E	// Devnode must stop using config.
#define	CONFIG_READY		0x0000000F	// The devnode has been setup.

#define	NUM_CONFIG_COMMANDS	0x00000010	// For DEBUG.

/*XLATOFF*/

#define	DEBUG_CONFIG_NAMES \
char	CMFAR *lpszConfigName[NUM_CONFIG_COMMANDS]= \
{ \
	"CONFIG_FILTER", \
	"CONFIG_START", \
	"CONFIG_STOP", \
	"CONFIG_TEST", \
	"CONFIG_REMOVE", \
	"CONFIG_ENUMERATE", \
	"CONFIG_SETUP", \
	"CONFIG_CALLBACK", \
	"CONFIG_APM", \
	"CONFIG_TEST_FAILED", \
	"CONFIG_TEST_SUCCEEDED", \
	"CONFIG_VERIFY_DEVICE", \
	"CONFIG_PREREMOVE", \
	"CONFIG_SHUTDOWN", \
	"CONFIG_PREREMOVE2", \
	"CONFIG_READY", \
};

/*XLATON*/

// Possible SUBCONFIGFUNC SubFuncNames:

#define	CONFIG_START_DYNAMIC_START			0x00000000
#define	CONFIG_START_FIRST_START			0x00000001

#define	CONFIG_STOP_DYNAMIC_STOP			0x00000000
#define	CONFIG_STOP_HAS_PROBLEM				0x00000001

//
// For both CONFIG_REMOVE and CONFIG_POSTREMOVE
//
#define	CONFIG_REMOVE_DYNAMIC				0x00000000
#define	CONFIG_REMOVE_SHUTDOWN				0x00000001
#define	CONFIG_REMOVE_REBOOT				0x00000002

#define	CONFIG_TEST_CAN_STOP				0x00000000
#define	CONFIG_TEST_CAN_REMOVE				0x00000001

#define	CONFIG_APM_TEST_STANDBY				0x00000000
#define	CONFIG_APM_TEST_SUSPEND				0x00000001
#define	CONFIG_APM_TEST_STANDBY_FAILED			0x00000002
#define	CONFIG_APM_TEST_SUSPEND_FAILED			0x00000003
#define	CONFIG_APM_TEST_STANDBY_SUCCEEDED		0x00000004
#define	CONFIG_APM_TEST_SUSPEND_SUCCEEDED		0x00000005
#define	CONFIG_APM_RESUME_STANDBY			0x00000006
#define	CONFIG_APM_RESUME_SUSPEND			0x00000007
#define	CONFIG_APM_RESUME_CRITICAL			0x00000008
#define	CONFIG_APM_UI_ALLOWED                  		0x80000000

/****************************************************************************
 *
 *				ARBITRATOR FUNCTIONS
 *
 ****************************************************************************
 *
 *	Each arbitrator has a handler field which is getting called every
 *	time Configuration Manager wants it to perform a function. The
 *	handler is called with:
 *
 *	result=paArbitrator->Arbitrate(	EventName,
 *					paArbitrator->DWordToBePassed,
 *					paArbitrator->dnItsDevNode,
 *					pnlhNodeListHeader);
 *
 *	ENTRY:	NodeListHeader contains a logical configuration for all
 *		devices the configuration manager would like to reconfigure.
 *		DWordToBePassed is the arbitrator reference data.
 *		ItsDevNode is the pointer to arbitrator's devnode.
 *		EventName is one of the following:
 *
 *	ARB_TEST_ALLOC - Test allocation of resource
 *
 *	DESC:	The arbitration routine will attempt to satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. See individual arbitrator for the algorithm
 *		employed. Generally, the arbitration consists
 *		of sorting the list according to most likely succesful
 *		allocation order, making a copy of the current allocation
 *		data strucuture(s), releasing all resource currently
 *		allocated to devnodes on the list from the copy data structure
 *		and then attempting to satisfy allocation requests
 *		by passing through the entire list, trying all possible
 *		combinations of allocations before failing. The arbitrator
 *		saves the resultant successful allocations, both in the node
 *		list per device and the copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_FAILURE if unsuccessful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	ARB_RETEST_ALLOC - Retest allocation of resource
 *
 *	DESC:	The arbitration routine will attempt to satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. It will take the result of a previous TEST_ALLOC
 *		and attempt to allocate that resource for each allcoation in
 *		the list. It will not sort the node list. It will make a copy
 *		of the current allocation data strucuture(s), release all
 *		resource currently allocated to devnodes on the list from
 *		the copy data structure and then attempt to satisfy the
 *		allocations from the previous TEST_ALLOC. The arbitrator
 *		saves the resultant copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_FAILURE if unsuccessful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	ARB_FORCE_ALLOC - Retest allocation of resource, always succeed
 *
 *	DESC:	The arbitration routine will satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. It will take the result of a previous TEST_ALLOC
 *		and allocate that resource for each allocation in
 *		the list. It will not sort the node list. It will make a copy
 *		of the current allocation data strucuture(s), release all
 *		resource currently allocated to devnodes on the list from
 *		the copy data structure and then satisfy the
 *		allocations from the previous TEST_ALLOC. The arbitrator
 *		saves the resultant copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	ARB_SET_ALLOC - Makes a test allocation the real allocation
 *
 *	DESC:	Makes the copy of the allocation data structure the
 *		current valid allocation.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	ARB_RELEASE_ALLOC - Clean up after failed test allocation
 *
 *	DESC:	Free all allocation that were allocated by the previous
 *		ARB_TEST_ALLOC.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	ARB_QUERY_FREE - Add all free resource logical configuration
 *
 *	DESC:	Return resource specific data on the free element. Note
 *		than the pnlhNodeListHeader is a cast of an arbitfree_s.
 *
 *	EXIT:	CR_SUCCESS if successful
 *		CR_FAILURE if the request makles no sense.
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	ARB_REMOVE - The devnode the arbitrator registered with is going away
 *
 *	DESC:	Arbitrator registered with a non-NULL devnode (thus is
 *		normally local), and the devnode is being removed. Arbitrator
 *		should do appropriate cleanup.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	WARNING: For any non-defined service, the arbitrator should return
 *	CR_DEFAULT. This will be treated as the compatibility case in future
 *	version.
 *
 ***************************************************************************/
#define	ARB_TEST_ALLOC		0x00000000	// Check if can make alloc works.
#define	ARB_RETEST_ALLOC	0x00000001	// Check if can take previous alloc.
#define	ARB_SET_ALLOC		0x00000002	// Set the tested allocation.
#define	ARB_RELEASE_ALLOC	0x00000003	// Release the tested allocation.
#define	ARB_QUERY_FREE		0x00000004	// Return free resource.
#define	ARB_REMOVE		0x00000005	// DevNode is gone.
#define	ARB_FORCE_ALLOC		0x00000006	// Force previous TEST_ALLOC
#define	NUM_ARB_COMMANDS	0x00000007	// Number of arb commands

#define	DEBUG_ARB_NAMES \
char	CMFAR *lpszArbFuncName[NUM_ARB_COMMANDS]= \
{ \
	"ARB_TEST_ALLOC",\
	"ARB_RETEST_ALLOC",\
	"ARB_SET_ALLOC",\
	"ARB_RELEASE_ALLOC",\
	"ARB_QUERY_FREE",\
	"ARB_REMOVE",\
	"ARB_FORCE_ALLOC",\
};

/****************************************************************************
 *
 *				DEVNODE STATUS
 *
 ****************************************************************************
 *
 *	These are the bits in the devnode's status that someone can query
 *	with a CM_Get_DevNode_Status. The A/S column tells wheter the flag
 *	cann be change asynchronously or not.
 *
 ***************************************************************************/
#define	DN_ROOT_ENUMERATED	0x00000001	// S: Was enumerated by ROOT
#define	DN_DRIVER_LOADED	0x00000002	// S: Has Register_Device_Driver
#define	DN_ENUM_LOADED		0x00000004	// S: Has Register_Enumerator
#define	DN_STARTED		0x00000008	// S: Is currently configured
#define	DN_MANUAL		0x00000010	// S: Manually installed
#define	DN_NEED_TO_ENUM		0x00000020	// A: May need reenumeration
#define	DN_NOT_FIRST_TIME	0x00000040	// S: Has received a config
#define	DN_HARDWARE_ENUM	0x00000080	// S: Enum generates hardware ID
#define	DN_LIAR 		0x00000100	// S: Lied about can reconfig once
#define	DN_HAS_MARK		0x00000200	// S: Not CM_Create_DevNode lately
#define	DN_HAS_PROBLEM		0x00000400	// S: Need device installer
#define	DN_FILTERED		0x00000800	// S: Is filtered
#define	DN_MOVED		0x00001000	// S: Has been moved
#define	DN_DISABLEABLE		0x00002000	// S: Can be rebalanced
#define	DN_REMOVABLE		0x00004000	// S: Can be removed
#define	DN_PRIVATE_PROBLEM	0x00008000	// S: Has a private problem
#define	DN_MF_PARENT		0x00010000	// S: Multi function parent
#define	DN_MF_CHILD		0x00020000	// S: Multi function child
#define	DN_WILL_BE_REMOVED	0x00040000	// S: Devnode is being removed

/*XLATOFF*/

#define	NUM_DN_FLAG		0x00000013	// DEBUG: maximum flag (number)
#define	DN_FLAG_LEN		0x00000002	// DEBUG: flag length

#define	DEBUG_DN_FLAGS_NAMES \
char	CMFAR lpszDNFlagsName[NUM_DN_FLAG][DN_FLAG_LEN]= \
{ \
	"rt", \
	"dl", \
	"el", \
	"st", \
	"mn", \
	"ne", \
	"fs", \
	"hw", \
	"lr", \
	"mk", \
	"pb", \
	"ft", \
	"mv", \
	"db", \
	"rb", \
	"pp", \
	"mp", \
	"mc", \
	"rm", \
};

struct vmmtime_s {
DWORD		vmmtime_lo;
DWORD		vmmtime_hi;
};

typedef	struct vmmtime_s	VMMTIME;
typedef	VMMTIME			*PVMMTIME;

struct cmtime_s {
DWORD		dwAPICount;
VMMTIME		vtAPITime;
};

typedef	struct cmtime_s		CMTIME;
typedef	CMTIME			*PCMTIME;

struct cm_performance_info_s {
CMTIME		ctBoot;
CMTIME		ctAPI[NUM_CM_SERVICES];
CMTIME		ctRing3;
CMTIME		ctProcessTree;
CMTIME		ctAssignResources;
CMTIME		ctSort;
CMTIME		ctRegistry;
CMTIME		ctVxDLdr;
CMTIME		ctNewDevNode;
CMTIME		ctSendMessage;
CMTIME		ctShell;
CMTIME		ctReceiveMessage;
CMTIME		ctAppyTime;
CMTIME		ctConfigMessage[NUM_CONFIG_COMMANDS];
CMTIME		ctArbTime[ResType_Max+1][NUM_ARB_COMMANDS];
DWORD		dwStackSize;
DWORD		dwMaxProcessTreePasses;
DWORD		dwStackAlloc;
};

typedef	struct	cm_performance_info_s	CMPERFINFO;
typedef	CMPERFINFO		CMFAR	*PCMPERFINFO;

/*XLATON*/

/****************************************************************************
 *
 *				DLVXD FUNCTIONS
 *
 ****************************************************************************
 *
 *	We load a Dynamically loaded VxD when there is a DEVLOADER=... line
 *	in the registry, or when someone calls CM_Load_Device. We then do
 *	a direct system control call (PNP_NEW_DEVNODE) to it, telling the
 *	DLVXD whether we loaded it to be an enumerator, a driver or a
 *	devloader (config manager does only deal with devloaders, but the
 *	default devloaders does CM_Load_Device with DLVXD_LOAD_ENUMERATOR
 *	and DLVXD_LOAD_DRIVER).
 *
 ***************************************************************************/
#define	DLVXD_LOAD_ENUMERATOR	0x00000000	// We loaded DLVxD as an enumerator.
#define	DLVXD_LOAD_DEVLOADER	0x00000001	// We loaded DLVxD as a devloader.
#define	DLVXD_LOAD_DRIVER	0x00000002	// We loaded DLVxD as a device driver.
#define	NUM_DLVXD_LOAD_TYPE	0x00000003	// Number of DLVxD load type.

/****************************************************************************
 *
 *				GLOBALLY DEFINED FLAGS
 *
 ***************************************************************************/
#define	ARB_GLOBAL		0x00000001	// Arbitrator is global.
#define	ARB_LOCAL		0x00000000	// Arbitrator is local.
#define	ARB_SCOPE_BIT		0x00000001	// Arbitrator is global/local bit.

#define	BASIC_LOG_CONF		0x00000000	// Specifies the req list.
#define	FILTERED_LOG_CONF	0x00000001	// Specifies the filtered req list.
#define	ALLOC_LOG_CONF		0x00000002	// Specifies the Alloc Element.
#define	BOOT_LOG_CONF		0x00000003	// Specifies the RM Alloc Element.
#define	FORCED_LOG_CONF		0x00000004	// Specifies the Forced Log Conf
#define	NUM_LOG_CONF		0x00000005	// Number of Log Conf type
#define	LOG_CONF_BITS		0x00000007	// The bits of the log conf type.

#define	DEBUG_LOG_CONF_NAMES \
char	CMFAR *lpszLogConfName[NUM_LOG_CONF]= \
{ \
	"BASIC_LOG_CONF",\
	"FILTERED_LOG_CONF",\
	"ALLOC_LOG_CONF",\
	"BOOT_LOG_CONF",\
	"FORCED_LOG_CONF",\
};

#define	PRIORITY_EQUAL_FIRST	0x00000008	// Same priority, new one is first.
#define	PRIORITY_EQUAL_LAST	0x00000000	// Same priority, new one is last.
#define	PRIORITY_BIT		0x00000008	// The bit of priority.

#ifndef	Not_VxD

/****************************************************************************
 *
 * Arbitration list structures
 *
 ***************************************************************************/
struct	nodelist_s {
	struct nodelist_s	*nl_Next;		// Next node element
	struct nodelist_s	*nl_Previous;		// Previous node element
	struct devnode_s	*nl_ItsDevNode;		// The dev node it represent

	// You can add fields to this structure, but the first three
	// fields must NEVER be changed.

	struct Log_Conf 	*nl_Test_Req;		// Test resource alloc request
	ULONG			nl_ulSortDWord;		// Specifies the sort order
};

struct	nodelistheader_s {
	struct	nodelist_s	*nlh_Head;		// First node element
	struct	nodelist_s	*nlh_Tail;		// Last node element
};

struct	arbitfree_s {
	PVOID			*af_PointerToInfo;	// the arbitrator info
	ULONG			af_SizeOfInfo;		// size of the info
};

#endif

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for memory returns a Range List (see
 *	configuration manager for APIs to use with Range Lists). The values
 *	in the Range List are ranges of taken memory address space.
 */
struct	MEM_Arb_s {
	RANGE_LIST		MEMA_Alloc;
};

typedef	struct MEM_Arb_s	MEMA_ARB;

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for IO returns a Range List (see
 *	configuration manager for APIs to use with Range Lists). The values
 *	in the Range List are ranges of taken IO address space.
 */
struct	IO_Arb_s {
	RANGE_LIST		IOA_Alloc;
};

typedef	struct IO_Arb_s		IOA_ARB;

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for DMA returns the DMA_Arb_s,
 *	16 bits of allocation bit mask, where DMAA_Alloc is inverted
 *	(set bit indicates free port).
 */
struct	DMA_Arb_s {
	WORD			DMAA_Alloc;
};

typedef	struct DMA_Arb_s	DMA_ARB;

/***************************************************************************
 * ARB_QUERY_FREE arbitrator function for IRQ returns the IRQ_Arb_s,
 *	16 bits of allocation bit mask, 16 bits of share bit mask and 16
 *	BYTES of share count. IRQA_Alloc is inverted (bit set indicates free
 *	port). If port is not free, IRQA_Share bit set indicates port
 *	that is shareable. For shareable IRQs, IRQA_Share_Count indicates
 *	number of devices that are sharing an IRQ.
 */
struct	IRQ_Arb_s {
	WORD			IRQA_Alloc;
	WORD			IRQA_Share;
	BYTE			IRQA_Share_Count[16];
};

typedef	struct IRQ_Arb_s	IRQ_ARB;

/* ASM
DebugCommand	Macro	FuncName
		local	DC_01
ifndef	CM_GOLDEN_RETAIL
ifdef	retail
 	IsDebugOnlyLoaded	DC_01
endif
	Control_Dispatch	DEBUG_QUERY, FuncName, sCall
endif
DC_01:
endm
IFDEF CM_PERFORMANCE_INFO
CM_PAGEABLE_CODE_SEG	TEXTEQU	<VxD_LOCKED_CODE_SEG>
CM_PAGEABLE_CODE_ENDS	TEXTEQU	<VxD_LOCKED_CODE_ENDS>
CM_PAGEABLE_DATA_SEG	TEXTEQU	<VxD_LOCKED_DATA_SEG>
CM_PAGEABLE_DATA_ENDS	TEXTEQU	<VxD_LOCKED_DATA_ENDS>
ELSE
CM_PAGEABLE_CODE_SEG	TEXTEQU <VxD_PNP_CODE_SEG>
CM_PAGEABLE_CODE_ENDS	TEXTEQU <VxD_PNP_CODE_ENDS>
CM_PAGEABLE_DATA_SEG	TEXTEQU	<VxD_PAGEABLE_DATA_SEG>
CM_PAGEABLE_DATA_ENDS	TEXTEQU	<VxD_PAGEABLE_DATA_ENDS>
ENDIF
*/

struct	CM_API_s {
DWORD		pCMAPIStack;
DWORD		dwCMAPIService;
DWORD		dwCMAPIRet;
};

typedef	struct	CM_API_s	CMAPI;

/*XLATOFF*/

#define	CM_VXD_RESULT		int

#define	CM_EXTERNAL		_cdecl
#define	CM_HANDLER		_cdecl
#define	CM_SYSCTRL		_stdcall
#define	CM_GLOBAL_DATA
#define	CM_LOCAL_DATA		static

#define	CM_OFFSET_OF(type, id)	((DWORD)(&(((type)0)->id)))

#define	CM_BUGBUG(d, id, msg)	message("BUGBUG: "##d##", "##id##": "##msg)

#if	DEBLEVEL==DEBLEVELRETAIL

#define	CM_WARN(strings)
#define	CM_ERROR(strings)

#else

#if	DEBLEVEL==DEBLEVELNORMAL

#define	CM_WARN(strings)
#define	CM_ERROR(strings) {\
	_Debug_Printf_Service(WARNNAME " ERROR: "); \
	_Debug_Printf_Service##strings; \
	_Debug_Printf_Service("\n");}

#else

#define	CM_WARN(strings) {\
	_Debug_Printf_Service(WARNNAME " WARNS: "); \
	_Debug_Printf_Service##strings; \
	_Debug_Printf_Service("\n");}

#define	CM_ERROR(strings) {\
	_Debug_Printf_Service(WARNNAME " ERROR: "); \
	_Debug_Printf_Service##strings; \
	_Debug_Printf_Service("\n"); \
	{_asm	int	3}}
#endif

#endif

#ifdef	DEBUG
#define	CM_DEBUG_CODE		VxD_LOCKED_CODE_SEG
#define	CM_DEBUG_DATA		VxD_LOCKED_DATA_SEG
#else
#define	CM_DEBUG_CODE		VxD_DEBUG_ONLY_CODE_SEG
#define	CM_DEBUG_DATA		VxD_DEBUG_ONLY_DATA_SEG
#endif

#ifdef	CM_PERFORMANCE_INFO

#define	CM_PAGEABLE_CODE	VxD_LOCKED_CODE_SEG
#define	CM_PAGEABLE_DATA	VxD_LOCKED_DATA_SEG
#define	CM_INIT_CODE		VxD_INIT_CODE_SEG
#define	CM_INIT_DATA		VxD_INIT_DATA_SEG
#define	CURSEG()		LCODE

#else

#define	CM_PAGEABLE_CODE	VxD_PNP_CODE_SEG
#define	CM_PAGEABLE_DATA	VxD_PAGEABLE_DATA_SEG
#define	CM_INIT_CODE		VxD_INIT_CODE_SEG
#define	CM_INIT_DATA		VxD_INIT_DATA_SEG

#pragma warning (disable:4005)			// turn off redefinition

#define	CURSEG()		CCODE

#pragma warning (default:4005)			// turn on redefinition

#endif

#ifndef	MAX_PROFILE_LEN
#define	MAX_PROFILE_LEN	80
#endif

struct	HWProfileInfo_s {
ULONG	HWPI_ulHWProfile;			// the profile handle
char	HWPI_szFriendlyName[MAX_PROFILE_LEN];	// the friendly name
DWORD	HWPI_dwFlags;				// CM_HWPI_* flags
};

typedef	struct	HWProfileInfo_s	       HWPROFILEINFO;
typedef	struct	HWProfileInfo_s	      *PHWPROFILEINFO;
typedef	struct	HWProfileInfo_s	CMFAR *PFARHWPROFILEINFO;

#define	CM_HWPI_NOT_DOCKABLE	0x00000000
#define	CM_HWPI_UNDOCKED	0x00000001
#define	CM_HWPI_DOCKED		0x00000002

#ifdef	DEBUG

#define	CM_INTERNAL		_cdecl

#else

#define	CM_INTERNAL		_fastcall

#endif

#define	CM_NAKED		__declspec ( naked )
#define	CM_LOCAL		CM_INTERNAL
#define	CM_UNIQUE		static CM_INTERNAL

#define	CM_BEGIN_CRITICAL {\
_asm	pushfd	\
_asm	cli	\
}

#define	CM_END_CRITICAL \
_asm	popfd\

#define	CM_FOREVER		for (;;)

#ifndef	No_CM_Calls

#ifdef	Not_VxD

/****************************************************************************
 *
 *	CONFIGMG_Get_Entry_Point - Return the address to call to get in
 *				   Config Manager.
 *
 *	Exported.
 *
 *	ENTRY:	None.
 *
 *	EXIT:	None.
 *
 *	On return, the variable CMEntryPoint has been updated with the
 *	proper address to call to get to Configuration Manager.
 *
 ***************************************************************************/
DWORD static
CM_Get_Entry_Point(void)
{
	static	DWORD		CMEntryPoint=NULL;

	if (CMEntryPoint)
		return(CMEntryPoint);

	_asm	push	bx
	_asm	push	es
	_asm	push	di
	_asm	xor	di, di

	_asm	mov	ax, 0x1684
	_asm	mov	bx, 0x33
	_asm	mov	es, di
	_asm	int	0x2f

	_asm	mov	word ptr [CMEntryPoint+2], es
	_asm	mov	word ptr [CMEntryPoint], di

	_asm	pop	di
	_asm	pop	es
	_asm	pop	bx

	return(CMEntryPoint);
}

#define	MAKE_CM_HEADER(Function, Parameters) \
CONFIGRET static _near _cdecl \
CM_##Function##Parameters \
{ \
	CONFIGRET	CMRetValue=0; \
	DWORD		CMEntryPoint; \
	WORD		wCMAPIService=GetVxDServiceOrdinal(_CONFIGMG_##Function); \
	if ((CMEntryPoint=CM_Get_Entry_Point())==0) \
		return(0); \
	_asm	{mov	ax, wCMAPIService};\
	_asm	{call	CMEntryPoint}; \
	_asm	{mov	CMRetValue, ax};\
	return(CMRetValue); \
}

#else	// Not_VxD

#define	MAKE_CM_HEADER(Function, Parameters) \
MAKE_HEADER(CONFIGRET, _cdecl, CAT(_CONFIGMG_, Function), Parameters)

#endif	// Not_VxD

/****************************************************************************
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 * Each of the following functions must match their equivalent service
 * and the parameter table in dos386\vmm\configmg\services.*.
 *
 * Except for the Get_Version, each function return a CR_* result in EAX
 * (AX for non IS_32 app) and can trash ECX and/or EDX as they are 'C'
 * callable.
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 ***************************************************************************/

#pragma warning (disable:4100)		// Param not used

#ifdef	Not_VxD

MAKE_CM_HEADER(Get_Version, (VOID))

#else

WORD VXDINLINE
CONFIGMG_Get_Version(VOID)
{
	WORD	w;
	VxDCall(_CONFIGMG_Get_Version);
	_asm mov [w], ax
	return(w);
}

#endif

MAKE_CM_HEADER(Initialize, (ULONG ulFlags))
MAKE_CM_HEADER(Locate_DevNode, (PDEVNODE pdnDevNode, DEVNODEID pDeviceID, ULONG ulFlags))
MAKE_CM_HEADER(Get_Parent, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Child, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Sibling, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_ID_Size, (PFARULONG pulLen, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_ID, (DEVNODE dnDevNode, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Get_Depth, (PFARULONG pulDepth, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Private_DWord, (PFARULONG pulPrivate, DEVNODE dnInDevNode, DEVNODE dnForDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Private_DWord, (DEVNODE dnInDevNode, DEVNODE dnForDevNode, ULONG ulValue, ULONG ulFlags))
MAKE_CM_HEADER(Create_DevNode, (PDEVNODE pdnDevNode, DEVNODEID pDeviceID, DEVNODE dnParent, ULONG ulFlags))
MAKE_CM_HEADER(Query_Remove_SubTree, (DEVNODE dnAncestor, ULONG ulFlags))
MAKE_CM_HEADER(Remove_SubTree, (DEVNODE dnAncestor, ULONG ulFlags))
MAKE_CM_HEADER(Register_Device_Driver, (DEVNODE dnDevNode, CMCONFIGHANDLER Handler, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Register_Enumerator, (DEVNODE dnDevNode, CMENUMHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(Register_Arbitrator, (PREGISTERID pRid, RESOURCEID id, CMARBHANDLER Handler, ULONG ulDWordToBePassed, DEVNODE dnArbitratorNode, ULONG ulFlags))
MAKE_CM_HEADER(Deregister_Arbitrator, (REGISTERID id, ULONG ulFlags))
MAKE_CM_HEADER(Query_Arbitrator_Free_Size, (PFARULONG pulSize, DEVNODE dnDevNode, RESOURCEID ResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Query_Arbitrator_Free_Data, (PFARVOID pData, ULONG DataLen, DEVNODE dnDevNode, RESOURCEID ResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Sort_NodeList, (NODELIST_HEADER nlhNodeListHeader, ULONG ulFlags))
MAKE_CM_HEADER(Yield, (ULONG ulMicroseconds, ULONG ulFlags))
MAKE_CM_HEADER(Lock, (ULONG ulFlags))
MAKE_CM_HEADER(Unlock, (ULONG ulFlags))
MAKE_CM_HEADER(Add_Empty_Log_Conf, (PLOG_CONF plcLogConf, DEVNODE dnDevNode, PRIORITY Priority, ULONG ulFlags))
MAKE_CM_HEADER(Free_Log_Conf, (LOG_CONF lcLogConfToBeFreed, ULONG ulFlags))
MAKE_CM_HEADER(Get_First_Log_Conf, (PLOG_CONF plcLogConf, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Next_Log_Conf, (PLOG_CONF plcLogConf, LOG_CONF lcLogConf, ULONG ulFlags))
MAKE_CM_HEADER(Add_Res_Des, (PRES_DES prdResDes, LOG_CONF lcLogConf, RESOURCEID ResourceID, PFARVOID ResourceData, ULONG ResourceLen, ULONG ulFlags))
MAKE_CM_HEADER(Modify_Res_Des, (PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ResourceID, PFARVOID ResourceData, ULONG ResourceLen, ULONG ulFlags))
MAKE_CM_HEADER(Free_Res_Des, (PRES_DES prdResDes, RES_DES rdResDes, ULONG ulFlags))
MAKE_CM_HEADER(Get_Next_Res_Des, (PRES_DES prdResDes, RES_DES CurrentResDesOrLogConf, RESOURCEID ForResource, PRESOURCEID pResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Get_Performance_Info, (PCMPERFINFO pPerfInfo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Res_Des_Data_Size, (PFARULONG pulSize, RES_DES rdResDes, ULONG ulFlags))
MAKE_CM_HEADER(Get_Res_Des_Data, (RES_DES rdResDes, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Process_Events_Now, (ULONG ulFlags))
MAKE_CM_HEADER(Create_Range_List, (PRANGE_LIST prlh, ULONG ulFlags))
MAKE_CM_HEADER(Add_Range, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Delete_Range, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Test_Range_Available, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Dup_Range_List, (RANGE_LIST rlhOld, RANGE_LIST rlhNew, ULONG ulFlags))
MAKE_CM_HEADER(Free_Range_List, (RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Invert_Range_List, (RANGE_LIST rlhOld, RANGE_LIST rlhNew, ULONG ulMaxVal, ULONG ulFlags))
MAKE_CM_HEADER(Intersect_Range_List, (RANGE_LIST rlhOld1, RANGE_LIST rlhOld2, RANGE_LIST rlhNew, ULONG ulFlags))
MAKE_CM_HEADER(First_Range, (RANGE_LIST rlh, PFARULONG pulStart, PFARULONG pulEnd, PRANGE_ELEMENT preElement, ULONG ulFlags))
MAKE_CM_HEADER(Next_Range, (PRANGE_ELEMENT preElement, PFARULONG pulStart, PFARULONG pulEnd, ULONG ulFlags))
MAKE_CM_HEADER(Dump_Range_List, (RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Load_DLVxDs, (DEVNODE dnDevNode, PFARCHAR FileNames, LOAD_TYPE LoadType, ULONG ulFlags))
MAKE_CM_HEADER(Get_DDBs, (PPPVMMDDB ppDDB, PFARULONG pulCount, LOAD_TYPE LoadType, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_CRC_CheckSum, (PFARVOID pBuffer, ULONG ulSize, PFARULONG pulSeed, ULONG ulFlags))
MAKE_CM_HEADER(Register_DevLoader, (PVMMDDB pDDB, ULONG ulFlags))
MAKE_CM_HEADER(Reenumerate_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Setup_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Reset_Children_Marks, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Status, (PFARULONG pulStatus, PFARULONG pulProblemNumber, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Remove_Unmarked_Children, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(ISAPNP_To_CM, (PFARVOID pBuffer, DEVNODE dnDevNode, ULONG ulLogDev, ULONG ulFlags))
MAKE_CM_HEADER(CallBack_Device_Driver, (CMCONFIGHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(CallBack_Enumerator, (CMENUMHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(Get_Alloc_Log_Conf, (PCMCONFIG pccBuffer, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Key_Size, (PFARULONG pulLen, DEVNODE dnDevNode, PFARCHAR pszSubKey, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Key, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Read_Registry_Value, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARCHAR pszValueName, ULONG ulExpectedType, PFARVOID pBuffer, PFARULONG pulLength, ULONG ulFlags))
MAKE_CM_HEADER(Write_Registry_Value, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARCHAR pszValueName, ULONG ulType, PFARVOID pBuffer, ULONG ulLength, ULONG ulFlags))
MAKE_CM_HEADER(Disable_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Enable_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Move_DevNode, (DEVNODE dnFromDevNode, DEVNODE dnToDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Bus_Info, (DEVNODE dnDevNode, CMBUSTYPE btBusType, ULONG ulSizeOfInfo, PFARVOID pInfo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Bus_Info, (DEVNODE dnDevNode, PCMBUSTYPE pbtBusType, PFARULONG pulSizeOfInfo, PFARVOID pInfo, ULONG ulFlags))
MAKE_CM_HEADER(Set_HW_Prof, (ULONG ulConfig, ULONG ulFlags))
MAKE_CM_HEADER(Recompute_HW_Prof, (ULONG ulDock, ULONG ulSerialNo, ULONG ulFlags))
MAKE_CM_HEADER(Query_Change_HW_Prof, (ULONG ulDock, ULONG ulSerialNo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_Driver_Private_DWord, (DEVNODE dnDevNode, PFARULONG pulDWord, ULONG ulFlags))
MAKE_CM_HEADER(Set_Device_Driver_Private_DWord, (DEVNODE dnDevNode, ULONG ulDword, ULONG ulFlags))
MAKE_CM_HEADER(Get_HW_Prof_Flags, (PFARCHAR szDevNodeName, ULONG ulConfig, PFARULONG pulValue, ULONG ulFlags))
MAKE_CM_HEADER(Set_HW_Prof_Flags, (PFARCHAR szDevNodeName, ULONG ulConfig, ULONG ulValue, ULONG ulFlags))
MAKE_CM_HEADER(Read_Registry_Log_Confs, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Run_Detection, (ULONG ulFlags))
MAKE_CM_HEADER(Call_At_Appy_Time, (CMAPPYCALLBACKHANDLER Handler, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Fail_Change_HW_Prof, (ULONG ulFlags))
MAKE_CM_HEADER(Set_Private_Problem, (DEVNODE dnDevNode, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Debug_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Hardware_Profile_Info, (ULONG ulIndex, PFARHWPROFILEINFO pHWProfileInfo, ULONG ulFlags))
MAKE_CM_HEADER(Register_Enumerator_Function, (DEVNODE dnDevNode, CMENUMFUNCTION Handler, ULONG ulFlags))
MAKE_CM_HEADER(Call_Enumerator_Function, (DEVNODE dnDevNode, ENUMFUNC efFunc, ULONG ulRefData, PFARVOID pBuffer, ULONG ulBufferSize, ULONG ulFlags))
MAKE_CM_HEADER(Add_ID, (DEVNODE dnDevNode, PFARCHAR pszID, ULONG ulFlags))

#pragma warning (default:4100)		// Param not used

#endif	// ifndef No_CM_Calls

/*XLATON*/

#endif	// ifndef CMJUSTRESDES

#endif	// _CONFIGMG_H
