/*
 * cfgmgr32.h
 *
 * PnP configuration manager
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __CFGMGR32_H
#define __CFGMGR32_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#if defined(_CFGMGR32_)
#define CMAPI DECLSPEC_EXPORT
#else
#define CMAPI DECLSPEC_IMPORT
#endif

#include "cfg.h"

#include <pshpack1.h>

#define CR_SUCCESS                  			0x00000000
#define CR_DEFAULT                        0x00000001
#define CR_OUT_OF_MEMORY                  0x00000002
#define CR_INVALID_POINTER                0x00000003
#define CR_INVALID_FLAG                   0x00000004
#define CR_INVALID_DEVNODE                0x00000005
#define CR_INVALID_DEVINST          			CR_INVALID_DEVNODE
#define CR_INVALID_RES_DES                0x00000006
#define CR_INVALID_LOG_CONF               0x00000007
#define CR_INVALID_ARBITRATOR             0x00000008
#define CR_INVALID_NODELIST               0x00000009
#define CR_DEVNODE_HAS_REQS               0x0000000A
#define CR_DEVINST_HAS_REQS         			CR_DEVNODE_HAS_REQS
#define CR_INVALID_RESOURCEID             0x0000000B
#define CR_DLVXD_NOT_FOUND                0x0000000C
#define CR_NO_SUCH_DEVNODE                0x0000000D
#define CR_NO_SUCH_DEVINST          			CR_NO_SUCH_DEVNODE
#define CR_NO_MORE_LOG_CONF               0x0000000E
#define CR_NO_MORE_RES_DES                0x0000000F
#define CR_ALREADY_SUCH_DEVNODE           0x00000010
#define CR_ALREADY_SUCH_DEVINST     			CR_ALREADY_SUCH_DEVNODE
#define CR_INVALID_RANGE_LIST             0x00000011
#define CR_INVALID_RANGE                  0x00000012
#define CR_FAILURE                        0x00000013
#define CR_NO_SUCH_LOGICAL_DEV            0x00000014
#define CR_CREATE_BLOCKED                 0x00000015
#define CR_NOT_SYSTEM_VM                  0x00000016
#define CR_REMOVE_VETOED                  0x00000017
#define CR_APM_VETOED                     0x00000018
#define CR_INVALID_LOAD_TYPE              0x00000019
#define CR_BUFFER_SMALL                   0x0000001A
#define CR_NO_ARBITRATOR                  0x0000001B
#define CR_NO_REGISTRY_HANDLE             0x0000001C
#define CR_REGISTRY_ERROR                 0x0000001D
#define CR_INVALID_DEVICE_ID              0x0000001E
#define CR_INVALID_DATA                   0x0000001F
#define CR_INVALID_API                    0x00000020
#define CR_DEVLOADER_NOT_READY            0x00000021
#define CR_NEED_RESTART                   0x00000022
#define CR_NO_MORE_HW_PROFILES            0x00000023
#define CR_DEVICE_NOT_THERE               0x00000024
#define CR_NO_SUCH_VALUE                  0x00000025
#define CR_WRONG_TYPE                     0x00000026
#define CR_INVALID_PRIORITY               0x00000027
#define CR_NOT_DISABLEABLE                0x00000028
#define CR_FREE_RESOURCES                 0x00000029
#define CR_QUERY_VETOED                   0x0000002A
#define CR_CANT_SHARE_IRQ                 0x0000002B
#define CR_NO_DEPENDENT                   0x0000002C
#define CR_SAME_RESOURCES                 0x0000002D
#define CR_NO_SUCH_REGISTRY_KEY           0x0000002E
#define CR_INVALID_MACHINENAME            0x0000002F
#define CR_REMOTE_COMM_FAILURE            0x00000030
#define CR_MACHINE_UNAVAILABLE            0x00000031
#define CR_NO_CM_SERVICES                 0x00000032
#define CR_ACCESS_DENIED                  0x00000033
#define CR_CALL_NOT_IMPLEMENTED           0x00000034
#define CR_INVALID_PROPERTY               0x00000035
#define CR_DEVICE_INTERFACE_ACTIVE        0x00000036
#define CR_NO_SUCH_DEVICE_INTERFACE       0x00000037
#define CR_INVALID_REFERENCE_STRING       0x00000038
#define CR_INVALID_CONFLICT_LIST          0x00000039
#define CR_INVALID_INDEX                  0x0000003A
#define CR_INVALID_STRUCTURE_SIZE         0x0000003B


typedef DWORD RETURN_TYPE;
typedef RETURN_TYPE	CONFIGRET;

typedef HANDLE HMACHINE;
typedef HMACHINE *PHMACHINE;

typedef DWORD_PTR RES_DES;
typedef RES_DES *PRES_DES;

typedef DWORD_PTR RANGE_ELEMENT;
typedef RANGE_ELEMENT *PRANGE_ELEMENT;

typedef ULONG_PTR CONFLICT_LIST;
typedef CONFLICT_LIST *PCONFLICT_LIST;

typedef DWORD_PTR LOG_CONF;
typedef LOG_CONF *PLOG_CONF;

typedef ULONG PRIORITY;
typedef PRIORITY *PPRIORITY;

typedef DWORD_PTR RANGE_LIST;
typedef RANGE_LIST *PRANGE_LIST;

typedef DWORD DEVNODE, DEVINST;
typedef DEVNODE *PDEVNODE, *PDEVINST;

typedef CHAR *DEVNODEID_A, *DEVINSTID_A;
typedef WCHAR *DEVNODEID_W, *DEVINSTID_W;

#ifdef UNICODE
typedef DEVNODEID_W DEVNODEID;
typedef DEVINSTID_W DEVINSTID;
#else
typedef DEVNODEID_A DEVNODEID;
typedef DEVINSTID_A DEVINSTID;
#endif

typedef ULONG REGDISPOSITION;

typedef ULONG RESOURCEID;
typedef RESOURCEID *PRESOURCEID;

#define CM_RESDES_WIDTH_DEFAULT 					0x00000000
#define CM_RESDES_WIDTH_32      					0x00000001
#define CM_RESDES_WIDTH_64      					0x00000002
#define CM_RESDES_WIDTH_BITS    					0x00000003


#define MAX_CONFIG_VALUE      						9999
#define MAX_INSTANCE_VALUE    						9999

#define MAX_DEVICE_ID_LEN     						200
#define MAX_DEVNODE_ID_LEN    						MAX_DEVICE_ID_LEN

#define MAX_CLASS_NAME_LEN    						32
#define MAX_GUID_STRING_LEN   						39
#define MAX_PROFILE_LEN       						80


#define ResType_All                       0x00000000
#define ResType_None                      0x00000000
#define ResType_Mem                       0x00000001
#define ResType_IO                        0x00000002
#define ResType_DMA                       0x00000003
#define ResType_IRQ                       0x00000004
#define ResType_DoNotUse                  0x00000005
#define ResType_BusNumber                 0x00000006
#define ResType_MAX                       0x00000006
#define ResType_Ignored_Bit               0x00008000
#define ResType_ClassSpecific             0x0000FFFF
#define ResType_Reserved                  0x00008000
#define ResType_DevicePrivate             0x00008001
#define ResType_PcCardConfig              0x00008002
#define ResType_MfCardConfig              0x00008003

#define CM_GETIDLIST_FILTER_NONE          		0x00000000
#define CM_GETIDLIST_FILTER_ENUMERATOR        0x00000001
#define CM_GETIDLIST_FILTER_SERVICE           0x00000002
#define CM_GETIDLIST_FILTER_EJECTRELATIONS    0x00000004
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS  0x00000008
#define CM_GETIDLIST_FILTER_POWERRELATIONS    0x00000010
#define CM_GETIDLIST_FILTER_BUSRELATIONS      0x00000020
#define CM_GETIDLIST_DONOTGENERATE            0x10000040
#define CM_GETIDLIST_FILTER_BITS              0x1000007F

#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT     	0x00000000
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES 	0x00000001
#define CM_GET_DEVICE_INTERFACE_LIST_BITS        	0x00000001


typedef struct BusNumber_Des_s {
  DWORD  BUSD_Count;
  DWORD  BUSD_Type;
  DWORD  BUSD_Flags;
  ULONG  BUSD_Alloc_Base;
  ULONG  BUSD_Alloc_End;
} BUSNUMBER_DES, *PBUSNUMBER_DES;

typedef struct BusNumber_Range_s {
  ULONG  BUSR_Min;
  ULONG  BUSR_Max;
  ULONG  BUSR_nBusNumbers;
  ULONG  BUSR_Flags;
} BUSNUMBER_RANGE, *PBUSNUMBER_RANGE;

#define BusNumberType_Range sizeof(BUSNUMBER_RANGE)

typedef struct BusNumber_Resource_s {
  BUSNUMBER_DES  BusNumber_Header;
  BUSNUMBER_RANGE  BusNumber_Data[ANYSIZE_ARRAY];
} BUSNUMBER_RESOURCE, *PBUSNUMBER_RESOURCE;

typedef struct CS_Des_s {
  DWORD  CSD_SignatureLength;
  DWORD  CSD_LegacyDataOffset;
  DWORD  CSD_LegacyDataSize;
  DWORD  CSD_Flags;
  GUID  CSD_ClassGuid;
  BYTE  CSD_Signature[ANYSIZE_ARRAY];
} CS_DES, *PCS_DES;

typedef struct CS_Resource_s {
  CS_DES  CS_Header;
} CS_RESOURCE, *PCS_RESOURCE;

typedef struct DevPrivate_Des_s {
  DWORD  PD_Count;
  DWORD  PD_Type;
  DWORD  PD_Data1;
  DWORD  PD_Data2;
  DWORD  PD_Data3;
  DWORD  PD_Flags;
} DEVPRIVATE_DES, *PDEVPRIVATE_DES;

typedef struct DevPrivate_Range_s {
	DWORD  PR_Data1;
	DWORD  PR_Data2;
	DWORD  PR_Data3;
} DEVPRIVATE_RANGE, *PDEVPRIVATE_RANGE;

#define PType_Range sizeof(DEVPRIVATE_RANGE)

typedef struct DevPrivate_Resource_s {
  DEVPRIVATE_DES  PRV_Header;
  DEVPRIVATE_RANGE  PRV_Data[ANYSIZE_ARRAY];
} DEVPRIVATE_RESOURCE, *PDEVPRIVATE_RESOURCE;

/* DMA_DES.DD_Flags constants and masks */
#define mDD_Width         								0x3
#define fDD_BYTE          								0x0
#define fDD_WORD          								0x1
#define fDD_DWORD         								0x2
#define fDD_BYTE_AND_WORD 								0x3

#define mDD_BusMaster     								0x4
#define fDD_NoBusMaster   								0x0
#define fDD_BusMaster     								0x4

#define mDD_Type         									0x18
#define fDD_TypeStandard 									0x00
#define fDD_TypeA        									0x08
#define fDD_TypeB        									0x10
#define fDD_TypeF        									0x18

typedef struct DMA_Des_s {
  DWORD  DD_Count;
  DWORD  DD_Type;
  DWORD  DD_Flags;
  ULONG  DD_Alloc_Chan;
} DMA_DES, *PDMA_DES;

typedef struct DMA_Range_s {
  ULONG  DR_Min;
  ULONG  DR_Max;
  ULONG  DR_Flags;
} DMA_RANGE, *PDMA_RANGE;

#define DType_Range sizeof(DMA_RANGE)

typedef struct DMA_Resource_s {
  DMA_DES  DMA_Header;
  DMA_RANGE  DMA_Data[ANYSIZE_ARRAY];
} DMA_RESOURCE, *PDMA_RESOURCE;

/* IO_DES.Type constants and masks */
#define fIOD_PortType   									0x1
#define fIOD_Memory     									0x0
#define fIOD_IO         									0x1
#define fIOD_DECODE     									0x00fc
#define fIOD_10_BIT_DECODE    						0x0004
#define fIOD_12_BIT_DECODE    						0x0008
#define fIOD_16_BIT_DECODE    						0x0010
#define fIOD_POSITIVE_DECODE  						0x0020
#define fIOD_PASSIVE_DECODE   						0x0040
#define fIOD_WINDOW_DECODE    						0x0080

typedef struct IO_Des_s {
  DWORD  IOD_Count;
  DWORD  IOD_Type;
  DWORDLONG  IOD_Alloc_Base;
  DWORDLONG  IOD_Alloc_End;
  DWORD  IOD_DesFlags;
} IO_DES, *PIO_DES;

/* IO_RANGE.IOR_Alias constants */
#define IO_ALIAS_10_BIT_DECODE      			0x00000004
#define IO_ALIAS_12_BIT_DECODE      			0x00000010
#define IO_ALIAS_16_BIT_DECODE      			0x00000000
#define IO_ALIAS_POSITIVE_DECODE    			0x000000FF

typedef struct IO_Range_s {
  DWORDLONG  IOR_Align;
  DWORD  IOR_nPorts;
  DWORDLONG  IOR_Min;
  DWORDLONG  IOR_Max;
  DWORD  IOR_RangeFlags;
  DWORDLONG  IOR_Alias;
} IO_RANGE, *PIO_RANGE;

#define IOType_Range sizeof(IO_RANGE)

typedef struct IO_Resource_s {
  IO_DES  IO_Header;
  IO_RANGE  IO_Data[ANYSIZE_ARRAY];
} IO_RESOURCE, *PIO_RESOURCE;

/* IRQ_DES.IRQD_flags constants */
#define mIRQD_Share        								0x1
#define fIRQD_Exclusive    								0x0
#define fIRQD_Share        								0x1

#define fIRQD_Share_Bit    								0
#define fIRQD_Level_Bit    							  1

#define mIRQD_Edge_Level   								0x2
#define fIRQD_Level        								0x0
#define fIRQD_Edge         								0x2

typedef struct IRQ_Des_32_s {
  DWORD  IRQD_Count;
  DWORD  IRQD_Type;
  DWORD  IRQD_Flags;
  ULONG  IRQD_Alloc_Num;
  ULONG32  IRQD_Affinity;
} IRQ_DES_32, *PIRQ_DES_32;

typedef struct IRQ_Des_64_s {
  DWORD  IRQD_Count;
  DWORD  IRQD_Type;
  DWORD  IRQD_Flags;
  ULONG  IRQD_Alloc_Num;
  ULONG64  IRQD_Affinity;
} IRQ_DES_64, *PIRQ_DES_64;

#ifdef _WIN64
typedef IRQ_DES_64 IRQ_DES;
typedef PIRQ_DES_64 PIRQ_DES;
#else
typedef IRQ_DES_32 IRQ_DES;
typedef PIRQ_DES_32 PIRQ_DES;
#endif

typedef struct IRQ_Range_s {
  ULONG  IRQR_Min;
  ULONG  IRQR_Max;
  ULONG  IRQR_Flags;
} IRQ_RANGE, *PIRQ_RANGE;

#define IRQType_Range sizeof(IRQ_RANGE)

typedef struct IRQ_Resource_s {
  IRQ_DES  IRQ_Header;
  IRQ_RANGE  IRQ_Data[ANYSIZE_ARRAY];
} IRQ_RESOURCE, *PIRQ_RESOURCE;

/* MEM_DES.MD_Flags constants */
#define mMD_MemoryType              0x1
#define fMD_MemoryType              mMD_MemoryType
#define fMD_ROM                     0x0
#define fMD_RAM                     0x1

#define mMD_32_24                   0x2
#define fMD_32_24                   mMD_32_24
#define fMD_24                      0x0
#define fMD_32                      0x2

#define mMD_Prefetchable            0x4
#define fMD_Prefetchable            mMD_Prefetchable
#define fMD_Pref                    mMD_Prefetchable
#define fMD_PrefetchDisallowed      0x0
#define fMD_PrefetchAllowed         0x4

#define mMD_Readable                0x8
#define fMD_Readable                mMD_Readable
#define fMD_ReadAllowed             0x0
#define fMD_ReadDisallowed          0x8

#define mMD_CombinedWrite           0x10
#define fMD_CombinedWrite           mMD_CombinedWrite
#define fMD_CombinedWriteDisallowed 0x0
#define fMD_CombinedWriteAllowed    0x10

#define mMD_Cacheable               0x20
#define fMD_NonCacheable            0x0
#define fMD_Cacheable               0x20

typedef struct Mem_Des_s {
  DWORD  MD_Count;
  DWORD  MD_Type;
  DWORDLONG  MD_Alloc_Base;
  DWORDLONG  MD_Alloc_End;
  DWORD  MD_Flags;
  DWORD  MD_Reserved;
} MEM_DES, *PMEM_DES;

typedef struct Mem_Range_s {
  DWORDLONG  MR_Align;
  ULONG  MR_nBytes;
  DWORDLONG  MR_Min;
  DWORDLONG  MR_Max;
  DWORD  MR_Flags;
  DWORD  MR_Reserved;
} MEM_RANGE, *PMEM_RANGE;

#define MType_Range sizeof(MEM_RANGE)

typedef struct Mem_Resource_s {
  MEM_DES  MEM_Header;
  MEM_RANGE  MEM_Data[ANYSIZE_ARRAY];
} MEM_RESOURCE, *PMEM_RESOURCE;

/* MFCARD_DES.PMF_Flags constants */
#define fPMF_AUDIO_ENABLE   							0x8
#define mPMF_AUDIO_ENABLE   							fPMF_AUDIO_ENABLE

typedef struct MfCard_Des_s {
  DWORD  PMF_Count;
  DWORD  PMF_Type;
  DWORD  PMF_Flags;
  BYTE  PMF_ConfigOptions;
  BYTE  PMF_IoResourceIndex;
  BYTE  PMF_Reserved[2];
  DWORD  PMF_ConfigRegisterBase;
} MFCARD_DES, *PMFCARD_DES;

typedef struct MfCard_Resource_s {
  MFCARD_DES  MfCard_Header;
} MFCARD_RESOURCE, *PMFCARD_RESOURCE;

/* PCCARD_DES.PCD_Flags constants */

typedef struct PcCard_Des_s {
  DWORD  PCD_Count;
  DWORD  PCD_Type;
  DWORD  PCD_Flags;
  BYTE  PCD_ConfigIndex;
  BYTE  PCD_Reserved[3];
  DWORD  PCD_MemoryCardBase1;
  DWORD  PCD_MemoryCardBase2;
} PCCARD_DES, *PPCCARD_DES;

#define mPCD_IO_8_16        							0x1
#define fPCD_IO_8           							0x0
#define fPCD_IO_16          							0x1
#define mPCD_MEM_8_16       							0x2
#define fPCD_MEM_8          							0x0
#define fPCD_MEM_16         							0x2
#define mPCD_MEM_A_C        							0xC
#define fPCD_MEM1_A         							0x4
#define fPCD_MEM2_A         							0x8
#define fPCD_IO_ZW_8        							0x10
#define fPCD_IO_SRC_16      							0x20
#define fPCD_IO_WS_16       							0x40
#define mPCD_MEM_WS         							0x300
#define fPCD_MEM_WS_ONE     							0x100
#define fPCD_MEM_WS_TWO     							0x200
#define fPCD_MEM_WS_THREE   							0x300

#define fPCD_MEM_A          							0x4

#define fPCD_ATTRIBUTES_PER_WINDOW 				0x8000

#define fPCD_IO1_16         							0x00010000
#define fPCD_IO1_ZW_8       							0x00020000
#define fPCD_IO1_SRC_16     							0x00040000
#define fPCD_IO1_WS_16      							0x00080000

#define fPCD_IO2_16         							0x00100000
#define fPCD_IO2_ZW_8       							0x00200000
#define fPCD_IO2_SRC_16     							0x00400000
#define fPCD_IO2_WS_16      							0x00800000

#define mPCD_MEM1_WS        							0x03000000
#define fPCD_MEM1_WS_ONE    							0x01000000
#define fPCD_MEM1_WS_TWO    							0x02000000
#define fPCD_MEM1_WS_THREE  							0x03000000
#define fPCD_MEM1_16        							0x04000000

#define mPCD_MEM2_WS        							0x30000000
#define fPCD_MEM2_WS_ONE    							0x10000000
#define fPCD_MEM2_WS_TWO    							0x20000000
#define fPCD_MEM2_WS_THREE  							0x30000000
#define fPCD_MEM2_16        							0x40000000

#define PCD_MAX_MEMORY   									2
#define PCD_MAX_IO       									2

typedef struct PcCard_Resource_s {
  PCCARD_DES  PcCard_Header;
} PCCARD_RESOURCE, *PPCCARD_RESOURCE;


/* CONFLICT_DETAILS.CD.ulMask constants */
#define CM_CDMASK_DEVINST      						0x00000001
#define CM_CDMASK_RESDES       						0x00000002
#define CM_CDMASK_FLAGS        						0x00000004
#define CM_CDMASK_DESCRIPTION  						0x00000008
#define CM_CDMASK_VALID        					  0x0000000F

/* CONFLICT_DETAILS.CD.ulFlags constants */
#define CM_CDFLAGS_DRIVER      						0x00000001
#define CM_CDFLAGS_ROOT_OWNED  						0x00000002
#define CM_CDFLAGS_RESERVED    						0x00000004

typedef struct _CONFLICT_DETAILS_A {
  ULONG  CD_ulSize;
  ULONG  CD_ulMask;
  DEVINST  CD_dnDevInst;
  RES_DES  CD_rdResDes;
  ULONG  CD_ulFlags;
  CHAR  CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_A , *PCONFLICT_DETAILS_A;

typedef struct _CONFLICT_DETAILS_W {
  ULONG  CD_ulSize;
  ULONG  CD_ulMask;
  DEVINST  CD_dnDevInst;
  RES_DES  CD_rdResDes;
  ULONG  CD_ulFlags;
  WCHAR  CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_W , *PCONFLICT_DETAILS_W;

#ifdef UNICODE
typedef CONFLICT_DETAILS_W CONFLICT_DETAILS;
typedef PCONFLICT_DETAILS_W PCONFLICT_DETAILS;
#else
typedef CONFLICT_DETAILS_A CONFLICT_DETAILS;
typedef PCONFLICT_DETAILS_A PCONFLICT_DETAILS;
#endif



/* CM_Add_Empty_Log_Conf.ulFlags constants */
#define PRIORITY_EQUAL_FIRST  						0x00000008
#define PRIORITY_EQUAL_LAST   						0x00000000
#define PRIORITY_BIT          						0x00000008

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
  OUT PLOG_CONF  plcLogConf,
  IN DEVINST  dnDevInst,
  IN PRIORITY  Priority,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf_Ex(
  OUT PLOG_CONF  plcLogConf,
  IN DEVINST  dnDevInst,
  IN PRIORITY  Priority,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* CM_Add_ID.ulFlags constants */
#define CM_ADD_ID_HARDWARE              	0x00000000
#define CM_ADD_ID_COMPATIBLE              0x00000001
#define CM_ADD_ID_BITS                    0x00000001

CMAPI
CONFIGRET
WINAPI
CM_Add_IDA(
  IN DEVINST  dnDevInst,
  IN PSTR  pszID,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExA(
  IN DEVINST  dnDevInst,
  IN PSTR  pszID,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExW(
  IN DEVINST  dnDevInst,
  IN PWSTR  pszID,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_IDW(
  IN DEVINST  dnDevInst,
  IN PWSTR  pszID,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

#ifdef UNICODE
#define CM_Add_ID CM_Add_IDW
#define CM_Add_ID_Ex CM_Add_ID_ExW
#else
#define CM_Add_ID CM_Add_IDA
#define CM_Add_ID_Ex CM_Add_ID_ExA
#endif /* UNICODE */

/* FIXME: Obsolete CM_Add_Range */

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des(
  OUT PRES_DES  prdResDes,
  IN LOG_CONF  lcLogConf,
  IN RESOURCEID  ResourceID,
  IN PCVOID  ResourceData,
  IN ULONG  ResourceLen,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des_Ex(
  OUT PRES_DES  prdResDes,
  IN LOG_CONF  lcLogConf,
  IN RESOURCEID  ResourceID,
  IN PCVOID  ResourceData,
  IN ULONG  ResourceLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineA(
  IN PCSTR  UNCServerName,
  OUT PHMACHINE  phMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineW(
  IN PCWSTR  UNCServerName,
  OUT PHMACHINE  phMachine);

#ifdef UNICODE
#define CM_Connect_Machine CM_Connect_MachineW
#else
#define CM_Connect_Machine CM_Connect_MachineA
#endif /* UNICODE */

/* FIXME: Obsolete CM_Create_DevNode */
/* FIXME: Obsolete CM_Create_DevNodeEx */
/* FIXME: Obsolete CM_Create_Range_List */
/* FIXME: Obsolete CM_Delete_Class_Key */
/* FIXME: Obsolete CM_Delete_Class_Key_Ex */
/* FIXME: Obsolete CM_Delete_DevNode_Key */
/* FIXME: Obsolete CM_Delete_DevNode_Key_Ex */
/* FIXME: Obsolete CM_Delete_Range */
/* FIXME: Obsolete CM_Detected_Resource_Conflict */
/* FIXME: Obsolete CM_Detected_Resource_Conflict_Ex */
/* FIXME: Obsolete CM_Disable_DevNode */
/* FIXME: Obsolete CM_Disable_DevNodeEx */

CMAPI
CONFIGRET
WINAPI
CM_Disconnect_Machine(
  IN HMACHINE  hMachine); 

/* FIXME: Obsolete CM_Enable_DevNode */
/* FIXME: Obsolete CM_Enable_DevNodeEx */

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes(
  IN ULONG  ulClassIndex,
  OUT LPGUID  ClassGuid,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes_Ex(
  IN ULONG  ulClassIndex,
  OUT LPGUID  ClassGuid,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsA(
  IN ULONG  ulEnumIndex,
  OUT PCHAR  Buffer,
  IN OUT PULONG  pulLength,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExA(
  IN ULONG  ulEnumIndex,
  OUT PCHAR  Buffer,
  IN OUT PULONG  pulLength,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExW(
  IN ULONG  ulEnumIndex,
  OUT PWCHAR  Buffer,
  IN OUT PULONG  pulLength,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsW(
  IN ULONG  ulEnumIndex,
  OUT PWCHAR  Buffer,
  IN OUT PULONG  pulLength,
  IN ULONG  ulFlags);

#ifdef UNICODE
#define CM_Enumerate_Enumerators CM_Enumerate_EnumeratorsW
#define CM_Enumerate_Enumerators_Ex CM_Enumerate_Enumerators_ExW
#else
#define CM_Enumerate_Enumerators CM_Enumerate_EnumeratorsA
#define CM_Enumerate_Enumerators_Ex CM_Enumerate_Enumerators_ExW
#endif /* UNICODE */

/* FIXME: Obsolete CM_Find_Range */
/* FIXME: Obsolete CM_First_Range */

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf(
  IN LOG_CONF  lcLogConfToBeFreed,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Ex(
  IN LOG_CONF  lcLogConfToBeFreed,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
  IN LOG_CONF  lcLogConf);

/* FIXME: Obsolete CM_Free_Range_List */

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Ex(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Handle(
  IN RES_DES  rdResDes);

CMAPI
CONFIGRET
WINAPI
CM_Free_Resource_Conflict_Handle(
  IN CONFLICT_LIST  clConflictList);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child_Ex(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* FIXME: Obsolete CM_Get_Class_Name */
/* FIXME: Obsolete CM_Get_Class_Name_Ex */
/* FIXME: Obsolete CM_Get_Class_Key_Name */
/* FIXME: Obsolete CM_Get_Class_Key_Name_Ex */
/* FIXME: Obsolete CM_Get_Class_Registry_Property */

CMAPI
CONFIGRET
WINAPI
CM_Get_Depth(
  OUT PULONG  pulDepth,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Depth_Ex(
  OUT PULONG  pulDepth,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDA(
  IN DEVINST  dnDevInst,
  OUT PCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExA(
  IN DEVINST  dnDevInst,
  OUT PCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
  IN DEVINST  dnDevInst,
  OUT PWCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDW(
  IN DEVINST  dnDevInst,
  OUT PWCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags);

#ifdef UNICODE
#define CM_Get_Device_ID CM_Get_Device_IDW
#define CM_Get_Device_ID_Ex CM_Get_Device_ID_ExW
#else
#define CM_Get_Device_ID CM_Get_Device_IDA
#define CM_Get_Device_ID_Ex CM_Get_Device_ID_ExW
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListA(
  IN PCSTR  pszFilter,  OPTIONAL
  OUT PCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExA(
  IN PCSTR  pszFilter,  OPTIONAL
  OUT PCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExW(
  IN PCWSTR  pszFilter,  OPTIONAL
  OUT PWCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListW(
  IN PCWSTR  pszFilter,  OPTIONAL
  OUT PWCHAR  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags);

#ifdef UNICODE
#define CM_Get_Device_ID_List CM_Get_Device_ID_ListW
#define CM_Get_Device_ID_List_Ex CM_Get_Device_ID_List_ExW
#else
#define CM_Get_Device_ID_List CM_Get_Device_ID_ListA
#define CM_Get_Device_ID_List_Ex CM_Get_Device_ID_List_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeA(
  OUT PULONG  pulLen,
  IN PCSTR  pszFilter,  OPTIONAL
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExA(
  OUT PULONG  pulLen,
  IN PCSTR  pszFilter,  OPTIONAL
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExW(
  OUT PULONG  pulLen,
  IN PCWSTR  pszFilter,  OPTIONAL
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeW(
  OUT PULONG  pulLen,
  IN PCWSTR  pszFilter,  OPTIONAL
  IN ULONG  ulFlags);

#ifdef UNICODE
#define CM_Get_Device_ID_List_Size CM_Get_Device_ID_List_SizeW
#define CM_Get_Device_ID_List_Size_Ex CM_Get_Device_ID_List_Size_ExW
#else
#define CM_Get_Device_ID_List_Size CM_Get_Device_ID_List_SizeA
#define CM_Get_Device_ID_List_Size_Ex CM_Get_Device_ID_List_Size_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size(
  OUT PULONG  pulLen,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size_Ex(
  OUT PULONG  pulLen,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* FIXME: Obsolete CM_Get_Device_Interface_Alias */
/* FIXME: Obsolete CM_Get_Device_Interface_Alias_Ex */
/* FIXME: Obsolete CM_Get_Device_Interface_List */
/* FIXME: Obsolete CM_Get_Device_Interface_List_Ex */
/* FIXME: Obsolete CM_Get_Device_Interface_List_Size */
/* FIXME: Obsolete CM_Get_Device_Interface_List_Size_Ex */
/* FIXME: Obsolete CM_Get_DevNode_Custom_Property */
/* FIXME: Obsolete CM_Get_DevNode_Custom_Property_Ex */
/* FIXME: Obsolete CM_Get_DevNode_Registry_Property */

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status(
  OUT PULONG  pulStatus,
  OUT PULONG  pulProblemNumber,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
  OUT PULONG  pulStatus,
  OUT PULONG  pulProblemNumber,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

#define CM_Get_DevInst_Status CM_Get_DevNode_Status
#define CM_Get_DevInst_Status_Ex CM_Get_DevNode_Status_Ex

/* CM_Get_First_Log_Conf.ulFlags constants */
#define BASIC_LOG_CONF    0x00000000  /* Specifies the req list. */
#define FILTERED_LOG_CONF 0x00000001  /* Specifies the filtered req list. */
#define ALLOC_LOG_CONF    0x00000002  /* Specifies the Alloc Element. */
#define BOOT_LOG_CONF     0x00000003  /* Specifies the RM Alloc Element. */
#define FORCED_LOG_CONF   0x00000004  /* Specifies the Forced Log Conf */
#define OVERRIDE_LOG_CONF 0x00000005  /* Specifies the Override req list. */
#define NUM_LOG_CONF      0x00000006  /* Number of Log Conf type */
#define LOG_CONF_BITS     0x00000007  /* The bits of the log conf type. */

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
  OUT PLOG_CONF  plcLogConf,  OPTIONAL
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf_Ex(
  OUT PLOG_CONF  plcLogConf,  OPTIONAL
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* FIXME: Obsolete CM_Get_Global_State */
/* FIXME: Obsolete CM_Get_Global_State_Ex */
/* FIXME: Obsolete CM_Get_Hardware_Profile_Info */
/* FIXME: Obsolete CM_Get_Hardware_Profile_Info_Ex */
/* FIXME: Obsolete CM_Get_HW_Prof_Flags */
/* FIXME: Obsolete CM_Get_HW_Prof_Flags_Ex */

CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority(
  IN LOG_CONF  lcLogConf,
  OUT PPRIORITY  pPriority,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
  IN LOG_CONF  lcLogConf,
  OUT PPRIORITY  pPriority,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
  OUT PLOG_CONF  plcLogConf,  OPTIONAL
  IN LOG_CONF  lcLogConf,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf_Ex(
  OUT PLOG_CONF  plcLogConf,  OPTIONAL
  IN LOG_CONF  lcLogConf,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN RESOURCEID  ForResource,
  OUT PRESOURCEID  pResourceID,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des_Ex(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN RESOURCEID  ForResource,
  OUT PRESOURCEID  pResourceID,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
  IN RES_DES  rdResDes,
  OUT PVOID  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Ex(
  IN RES_DES  rdResDes,
  OUT PVOID  Buffer,
  IN ULONG  BufferLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
  OUT PULONG  pulSize,
  IN RES_DES  rdResDes,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size_Ex(
  OUT PULONG  pulSize,
  IN RES_DES  rdResDes,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_Count(
  IN CONFLICT_LIST  clConflictList,
  OUT PULONG  pulCount);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsA(
  IN CONFLICT_LIST  clConflictList,
  IN ULONG  ulIndex,
  IN OUT PCONFLICT_DETAILS_A  pConflictDetails);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsW(
  IN CONFLICT_LIST  clConflictList,
  IN ULONG  ulIndex,
  IN OUT PCONFLICT_DETAILS_W  pConflictDetails);

#ifdef UNICODE
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsW
#else
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  DevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling_Ex(
  OUT PDEVINST  pdnDevInst,
  IN DEVINST  DevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
WORD
WINAPI
CM_Get_Version(
  VOID);

CMAPI
WORD
WINAPI
CM_Get_Version_Ex(
  IN HMACHINE  hMachine);

/* FIXME: Obsolete CM_Intersect_Range_List */
/* FIXME: Obsolete CM_Invert_Range_List */
/* FIXME: Obsolete CM_Is_Dock_Station_Present */
/* FIXME: Obsolete CM_Is_Dock_Station_Present_Ex */

/* CM_Locate_DevNode.ulFlags constants */
#define CM_LOCATE_DEVNODE_NORMAL       		0x00000000
#define CM_LOCATE_DEVNODE_PHANTOM      		0x00000001
#define CM_LOCATE_DEVNODE_CANCELREMOVE 		0x00000002
#define CM_LOCATE_DEVNODE_NOVALIDATION 		0x00000004
#define CM_LOCATE_DEVNODE_BITS         		0x00000007

#define CM_LOCATE_DEVINST_NORMAL       		CM_LOCATE_DEVNODE_NORMAL
#define CM_LOCATE_DEVINST_PHANTOM      		CM_LOCATE_DEVNODE_PHANTOM
#define CM_LOCATE_DEVINST_CANCELREMOVE 		CM_LOCATE_DEVNODE_CANCELREMOVE
#define CM_LOCATE_DEVINST_NOVALIDATION 		CM_LOCATE_DEVNODE_NOVALIDATION
#define CM_LOCATE_DEVINST_BITS         		CM_LOCATE_DEVNODE_BITS

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeA(
  OUT PDEVINST  pdnDevInst,
  IN DEVINSTID_A  pDeviceID,  OPTIONAL
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExA(
  OUT PDEVINST  pdnDevInst,
  IN DEVINSTID_A  pDeviceID,  OPTIONAL
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExW(
  OUT PDEVINST  pdnDevInst,
  IN DEVINSTID_W  pDeviceID,  OPTIONAL
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
  OUT PDEVINST  pdnDevInst,
  IN DEVINSTID_W  pDeviceID,  OPTIONAL
  IN ULONG  ulFlags);

#define CM_Locate_DevInstA CM_Locate_DevNodeA
#define CM_Locate_DevInstW CM_Locate_DevNodeW
#define CM_Locate_DevInst_ExA CM_Locate_DevNode_ExA
#define CM_Locate_DevInst_ExW CM_Locate_DevNode_ExW

#ifdef UNICODE
#define CM_Locate_DevNode CM_Locate_DevNodeW
#define CM_Locate_DevInst CM_Locate_DevNodeW
#define CM_Locate_DevNode_Ex CM_Locate_DevNode_ExW
#define CM_Locate_DevInst_Ex CM_Locate_DevNode_ExW
#else
#define CM_Locate_DevNode CM_Locate_DevNodeA
#define CM_Locate_DevInst CM_Locate_DevNodeA
#define CM_Locate_DevNode_Ex CM_Locate_DevNode_ExA
#define CM_Locate_DevInst_Ex CM_Locate_DevNode_ExA
#endif /* UNICODE */

/* FIXME: Obsolete CM_Merge_Range_List */

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN RESOURCEID  ResourceID,
  IN PCVOID  ResourceData,
  IN ULONG  ResourceLen,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des_Ex(
  OUT PRES_DES  prdResDes,
  IN RES_DES  rdResDes,
  IN RESOURCEID  ResourceID,
  IN PCVOID  ResourceData,
  IN ULONG  ResourceLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* FIXME: Obsolete CM_Move_DevNode */
/* FIXME: Obsolete CM_Move_DevNode_Ex */
/* FIXME: Obsolete CM_Next_Range */
/* FIXME: Obsolete CM_Open_Class_Key */
/* FIXME: Obsolete CM_Open_Class_Key_Ex */
/* FIXME: Obsolete CM_Open_DevNode_Key */
/* FIXME: Obsolete CM_Open_DevNode_Key_Ex */

/* CM_Query_And_Remove_SubTree.ulFlags constants */
#define CM_REMOVE_UI_OK             			0x00000000
#define CM_REMOVE_UI_NOT_OK         			0x00000001
#define CM_REMOVE_NO_RESTART        			0x00000002
#define CM_REMOVE_BITS              			0x00000003

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeA(
  IN  DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN  ULONG ulNameLength,
  IN  ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeW(
  IN  DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN  ULONG ulNameLength,
  IN  ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExA(
  IN  DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN  ULONG ulNameLength,
  IN  ULONG ulFlags,
  IN  HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExW(
  IN  DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN  ULONG ulNameLength,
  IN  ULONG ulFlags,
  IN  HMACHINE hMachine);

/* FIXME: Obsolete CM_Query_Arbitrator_Free_Data */
/* FIXME: Obsolete CM_Query_Arbitrator_Free_Data_Ex */
/* FIXME: Obsolete CM_Query_Arbitrator_Free_Size */
/* FIXME: Obsolete CM_Query_Arbitrator_Free_Size_Ex */
/* FIXME: Obsolete CM_Query_Arbitrator_Free_Size_Ex */
/* FIXME: Obsolete CM_Query_Remove_SubTree */
/* FIXME: Obsolete CM_Query_Remove_SubTree_Ex */

CMAPI
CONFIGRET
WINAPI
CM_Query_Resource_Conflict_List(
  OUT PCONFLICT_LIST  pclConflictList,
  IN DEVINST  dnDevInst,
  IN RESOURCEID  ResourceID,
  IN PCVOID  ResourceData,
  IN ULONG  ResourceLen,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

/* CM_Reenumerate_DevNode.ulFlags constants */
#define CM_REENUMERATE_NORMAL             0x00000000
#define CM_REENUMERATE_SYNCHRONOUS        0x00000001
#define CM_REENUMERATE_RETRY_INSTALLATION 0x00000002
#define CM_REENUMERATE_ASYNCHRONOUS       0x00000004
#define CM_REENUMERATE_BITS               0x00000007

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode(
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode_Ex(
  IN DEVINST  dnDevInst,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

#define CM_Reenumerate_DevInst CM_Reenumerate_DevNode
#define CM_Reenumerate_DevInst_Ex CM_Reenumerate_DevNode_Ex

/* FIXME: Obsolete CM_Register_Device_Driver */
/* FIXME: Obsolete CM_Register_Device_Driver_Ex */
/* FIXME: Obsolete CM_Register_Device_Interface */
/* FIXME: Obsolete CM_Register_Device_Interface_Ex */
/* FIXME: Obsolete CM_Remove_SubTree */
/* FIXME: Obsolete CM_Remove_SubTree_Ex */

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_EjectA(
  IN DEVINST  dnDevInst,
  OUT PPNP_VETO_TYPE  pVetoType,
  OUT LPSTR  pszVetoName,
  IN ULONG  ulNameLength,
  IN ULONG  ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExW(
  IN DEVINST  dnDevInst,
  OUT PPNP_VETO_TYPE  pVetoType,
  OUT LPWSTR  pszVetoName,
  IN ULONG  ulNameLength,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExA(
  IN DEVINST  dnDevInst,
  OUT PPNP_VETO_TYPE  pVetoType,
  OUT LPSTR  pszVetoName,
  IN ULONG  ulNameLength,
  IN ULONG  ulFlags,
  IN HMACHINE  hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_EjectW(
  IN DEVINST  dnDevInst,
  OUT PPNP_VETO_TYPE  pVetoType,
  OUT LPWSTR  pszVetoName,
  IN ULONG  ulNameLength,
  IN ULONG  ulFlags);

#ifdef UNICODE
#define CM_Request_Device_Eject CM_Request_Device_EjectW
#define CM_Request_Device_Eject_Ex CM_Request_Device_Eject_ExW
#else
#define CM_Request_Device_Eject CM_Request_Device_EjectA
#define CM_Request_Device_Eject_Ex CM_Request_Device_Eject_ExA
#endif /* UNICODE */

/* FIXME: Obsolete CM_Request_Eject_PC */
/* FIXME: Obsolete CM_Request_Eject_PC_Ex */
/* FIXME: Obsolete CM_Run_Detection */
/* FIXME: Obsolete CM_Run_Detection_Ex */
/* FIXME: Obsolete CM_Set_Class_Registry_Property */
/* FIXME: Obsolete CM_Set_DevNode_Problem */
/* FIXME: Obsolete CM_Set_DevNode_Problem_Ex */
/* FIXME: Obsolete CM_Set_DevNode_Registry_Property */
/* FIXME: Obsolete CM_Set_DevNode_Registry_Property_Ex */
/* FIXME: Obsolete CM_Set_HW_Prof */
/* FIXME: Obsolete CM_Set_HW_Prof_Ex */
/* FIXME: Obsolete CM_Set_HW_Prof_Flags */
/* FIXME: Obsolete CM_Set_HW_Prof_Flags_Ex */
/* FIXME: Obsolete CM_Setup_DevNode */
/* FIXME: Obsolete CM_Setup_DevNode_Ex */
/* FIXME: Obsolete CM_Test_Range_Available */
/* FIXME: Obsolete CM_Uninstall_DevNode */
/* FIXME: Obsolete CM_Uninstall_DevNode_Ex */
/* FIXME: Obsolete CM_Unregister_Device_Interface */
/* FIXME: Obsolete CM_Unregister_Device_Interface_Ex */

#define CM_WaitNoPendingInstallEvents CMP_WaitNoPendingInstallEvents

CMAPI
DWORD
WINAPI
CMP_WaitNoPendingInstallEvents(
  IN DWORD dwTimeout);

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif /* __CFGMGR32_H */
