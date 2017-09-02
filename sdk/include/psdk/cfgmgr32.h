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

#pragma once

#ifndef _CFGMGR32_H_
#define _CFGMGR32_H_

#include <cfg.h>

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_CFGMGR32_)
#define CMAPI
#else
#define CMAPI DECLSPEC_IMPORT
#endif

typedef CONST VOID *PCVOID;

#define MAX_CONFIG_VALUE         9999
#define MAX_INSTANCE_VALUE       9999

#define MAX_DEVICE_ID_LEN        200
#define MAX_DEVNODE_ID_LEN       MAX_DEVICE_ID_LEN

#define MAX_CLASS_NAME_LEN       32
#define MAX_GUID_STRING_LEN      39
#define MAX_PROFILE_LEN          80

#define MAX_MEM_REGISTERS        9
#define MAX_IO_PORTS             20
#define MAX_IRQS                 7
#define MAX_DMA_CHANNELS         7

#define DWORD_MAX                0xffffffffUL
#define DWORDLONG_MAX            0xffffffffffffffffui64

#define CONFIGMG_VERSION         0x0400

#ifdef NT_INCLUDED

typedef unsigned __int64 DWORDLONG;
typedef DWORDLONG *PDWORDLONG;

#endif /* NT_INCLUDED */

typedef _Return_type_success_(return == 0) DWORD RETURN_TYPE;
typedef RETURN_TYPE CONFIGRET;

typedef DWORD DEVNODE, DEVINST;
typedef DEVNODE *PDEVNODE, *PDEVINST;

typedef _Null_terminated_ CHAR *DEVNODEID_A, *DEVINSTID_A;
typedef _Null_terminated_ WCHAR *DEVNODEID_W, *DEVINSTID_W;

#ifdef UNICODE
typedef DEVNODEID_W DEVNODEID;
typedef DEVINSTID_W DEVINSTID;
#else
typedef DEVNODEID_A DEVNODEID;
typedef DEVINSTID_A DEVINSTID;
#endif

typedef DWORD_PTR LOG_CONF;
typedef LOG_CONF *PLOG_CONF;

typedef DWORD_PTR RES_DES;
typedef RES_DES *PRES_DES;

typedef ULONG RESOURCEID;
typedef RESOURCEID *PRESOURCEID;

typedef ULONG PRIORITY;
typedef PRIORITY *PPRIORITY;

typedef DWORD_PTR RANGE_LIST;
typedef RANGE_LIST *PRANGE_LIST;

typedef DWORD_PTR RANGE_ELEMENT;
typedef RANGE_ELEMENT *PRANGE_ELEMENT;

typedef HANDLE HMACHINE;
typedef HMACHINE *PHMACHINE;

typedef ULONG_PTR CONFLICT_LIST;
typedef CONFLICT_LIST *PCONFLICT_LIST;

typedef struct _CONFLICT_DETAILS_A {
  ULONG CD_ulSize;
  ULONG CD_ulMask;
  DEVINST CD_dnDevInst;
  RES_DES CD_rdResDes;
  ULONG CD_ulFlags;
  CHAR CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_A, *PCONFLICT_DETAILS_A;

typedef struct _CONFLICT_DETAILS_W {
  ULONG CD_ulSize;
  ULONG CD_ulMask;
  DEVINST CD_dnDevInst;
  RES_DES CD_rdResDes;
  ULONG CD_ulFlags;
  WCHAR CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_W, *PCONFLICT_DETAILS_W;

#ifdef UNICODE
typedef CONFLICT_DETAILS_W CONFLICT_DETAILS;
typedef PCONFLICT_DETAILS_W PCONFLICT_DETAILS;
#else
typedef CONFLICT_DETAILS_A CONFLICT_DETAILS;
typedef PCONFLICT_DETAILS_A PCONFLICT_DETAILS;
#endif

/* CONFLICT_DETAILS.CD.ulMask constants */
#define CM_CDMASK_DEVINST                 0x00000001
#define CM_CDMASK_RESDES                  0x00000002
#define CM_CDMASK_FLAGS                   0x00000004
#define CM_CDMASK_DESCRIPTION             0x00000008
#define CM_CDMASK_VALID                   0x0000000F

/* CONFLICT_DETAILS.CD.ulFlags constants */
#define CM_CDFLAGS_DRIVER                 0x00000001
#define CM_CDFLAGS_ROOT_OWNED             0x00000002
#define CM_CDFLAGS_RESERVED               0x00000004

typedef ULONG REGDISPOSITION;

#include <pshpack1.h>

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
#define fMD_WINDOW_DECODE           0x40
#define fMD_MEMORY_BAR              0x80

typedef struct Mem_Range_s {
  DWORDLONG MR_Align;
  ULONG MR_nBytes;
  DWORDLONG MR_Min;
  DWORDLONG MR_Max;
  DWORD MR_Flags;
  DWORD MR_Reserved;
} MEM_RANGE, *PMEM_RANGE;

typedef struct Mem_Des_s {
  DWORD MD_Count;
  DWORD MD_Type;
  DWORDLONG MD_Alloc_Base;
  DWORDLONG MD_Alloc_End;
  DWORD MD_Flags;
  DWORD MD_Reserved;
} MEM_DES, *PMEM_DES;

typedef struct Mem_Resource_s {
  MEM_DES MEM_Header;
  MEM_RANGE MEM_Data[ANYSIZE_ARRAY];
} MEM_RESOURCE, *PMEM_RESOURCE;

#define MType_Range sizeof(MEM_RANGE)

typedef struct Mem_Large_Range_s {
  DWORDLONG MLR_Align;
  ULONGLONG MLR_nBytes;
  DWORDLONG MLR_Min;
  DWORDLONG MLR_Max;
  DWORD MLR_Flags;
  DWORD MLR_Reserved;
} MEM_LARGE_RANGE, *PMEM_LARGE_RANGE;

typedef struct Mem_Large_Des_s {
  DWORD MLD_Count;
  DWORD MLD_Type;
  DWORDLONG MLD_Alloc_Base;
  DWORDLONG MLD_Alloc_End;
  DWORD MLD_Flags;
  DWORD MLD_Reserved;
} MEM_LARGE_DES, *PMEM_LARGE_DES;

typedef struct Mem_Large_Resource_s {
  MEM_LARGE_DES MEM_LARGE_Header;
  MEM_LARGE_RANGE MEM_LARGE_Data[ANYSIZE_ARRAY];
} MEM_LARGE_RESOURCE, *PMEM_LARGE_RESOURCE;

#define MLType_Range     sizeof(struct Mem_Large_Range_s)

/* IO_DES.Type constants and masks */
#define fIOD_PortType                     0x1
#define fIOD_Memory                       0x0
#define fIOD_IO                           0x1
#define fIOD_DECODE                       0x00fc
#define fIOD_10_BIT_DECODE                0x0004
#define fIOD_12_BIT_DECODE                0x0008
#define fIOD_16_BIT_DECODE                0x0010
#define fIOD_POSITIVE_DECODE              0x0020
#define fIOD_PASSIVE_DECODE               0x0040
#define fIOD_WINDOW_DECODE                0x0080
#define fIOD_PORT_BAR                     0x0100

/* IO_RANGE.IOR_Alias constants */
#define IO_ALIAS_10_BIT_DECODE            0x00000004
#define IO_ALIAS_12_BIT_DECODE            0x00000010
#define IO_ALIAS_16_BIT_DECODE            0x00000000
#define IO_ALIAS_POSITIVE_DECODE          0x000000FF

typedef struct IO_Range_s {
  DWORDLONG IOR_Align;
  DWORD IOR_nPorts;
  DWORDLONG IOR_Min;
  DWORDLONG IOR_Max;
  DWORD IOR_RangeFlags;
  DWORDLONG IOR_Alias;
} IO_RANGE, *PIO_RANGE;

typedef struct IO_Des_s {
  DWORD IOD_Count;
  DWORD IOD_Type;
  DWORDLONG IOD_Alloc_Base;
  DWORDLONG IOD_Alloc_End;
  DWORD IOD_DesFlags;
} IO_DES, *PIO_DES;

typedef struct IO_Resource_s {
  IO_DES IO_Header;
  IO_RANGE IO_Data[ANYSIZE_ARRAY];
} IO_RESOURCE, *PIO_RESOURCE;

#define IOA_Local                         0xff

#define IOType_Range sizeof(IO_RANGE)

/* DMA_DES.DD_Flags constants and masks */
#define mDD_Width                         0x3
#define fDD_BYTE                          0x0
#define fDD_WORD                          0x1
#define fDD_DWORD                         0x2
#define fDD_BYTE_AND_WORD                 0x3

#define mDD_BusMaster                     0x4
#define fDD_NoBusMaster                   0x0
#define fDD_BusMaster                     0x4

#define mDD_Type                          0x18
#define fDD_TypeStandard                  0x00
#define fDD_TypeA                         0x08
#define fDD_TypeB                         0x10
#define fDD_TypeF                         0x18

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

/* IRQ_DES.IRQD_flags constants */
#define mIRQD_Share                       0x1
#define fIRQD_Exclusive                   0x0
#define fIRQD_Share                       0x1

#define fIRQD_Share_Bit                   0
#define fIRQD_Level_Bit                   1

#define mIRQD_Edge_Level                  0x2
#define fIRQD_Level                       0x0
#define fIRQD_Edge                        0x2

typedef struct IRQ_Range_s {
  ULONG IRQR_Min;
  ULONG IRQR_Max;
#if defined(NT_PROCESSOR_GROUPS)
  USHORT IRQR_Flags;
  USHORT IRQR_Rsvdz;
#else
  ULONG IRQR_Flags;
#endif
} IRQ_RANGE, *PIRQ_RANGE;

typedef struct IRQ_Des_32_s {
  DWORD IRQD_Count;
  DWORD IRQD_Type;
#if defined(NT_PROCESSOR_GROUPS)
  USHORT IRQD_Flags;
  USHORT IRQD_Group;
#else
  DWORD IRQD_Flags;
#endif
  ULONG IRQD_Alloc_Num;
  ULONG32 IRQD_Affinity;
} IRQ_DES_32, *PIRQ_DES_32;

typedef struct IRQ_Des_64_s {
  DWORD IRQD_Count;
  DWORD IRQD_Type;
#if defined(NT_PROCESSOR_GROUPS)
  USHORT IRQD_Flags;
  USHORT IRQD_Group;
#else
  DWORD IRQD_Flags;
#endif
  ULONG IRQD_Alloc_Num;
  ULONG64 IRQD_Affinity;
} IRQ_DES_64, *PIRQ_DES_64;

#ifdef _WIN64
typedef IRQ_DES_64 IRQ_DES;
typedef PIRQ_DES_64 PIRQ_DES;
#else
typedef IRQ_DES_32 IRQ_DES;
typedef PIRQ_DES_32 PIRQ_DES;
#endif

typedef struct IRQ_Resource_32_s {
  IRQ_DES_32 IRQ_Header;
  IRQ_RANGE IRQ_Data[ANYSIZE_ARRAY];
} IRQ_RESOURCE_32, *PIRQ_RESOURCE_32;

typedef struct IRQ_Resource_64_s {
  IRQ_DES_64 IRQ_Header;
  IRQ_RANGE IRQ_Data[ANYSIZE_ARRAY];
} IRQ_RESOURCE_64, *PIRQ_RESOURCE_64;

#ifdef _WIN64
typedef IRQ_RESOURCE_64 IRQ_RESOURCE;
typedef PIRQ_RESOURCE_64 PIRQ_RESOURCE;
#else
typedef IRQ_RESOURCE_32 IRQ_RESOURCE;
typedef PIRQ_RESOURCE_32 PIRQ_RESOURCE;
#endif

#define IRQType_Range sizeof(IRQ_RANGE)

#if (WINVER >= _WIN32_WINNT_WINXP)
#define CM_RESDES_WIDTH_DEFAULT  0x00000000
#define CM_RESDES_WIDTH_32       0x00000001
#define CM_RESDES_WIDTH_64       0x00000002
#define CM_RESDES_WIDTH_BITS     0x00000003
#endif

typedef struct DevPrivate_Range_s {
  DWORD PR_Data1;
  DWORD PR_Data2;
  DWORD PR_Data3;
} DEVPRIVATE_RANGE, *PDEVPRIVATE_RANGE;

typedef struct DevPrivate_Des_s {
  DWORD PD_Count;
  DWORD PD_Type;
  DWORD PD_Data1;
  DWORD PD_Data2;
  DWORD PD_Data3;
  DWORD PD_Flags;
} DEVPRIVATE_DES, *PDEVPRIVATE_DES;

#define PType_Range sizeof(DEVPRIVATE_RANGE)

typedef struct DevPrivate_Resource_s {
  DEVPRIVATE_DES PRV_Header;
  DEVPRIVATE_RANGE PRV_Data[ANYSIZE_ARRAY];
} DEVPRIVATE_RESOURCE, *PDEVPRIVATE_RESOURCE;

typedef struct CS_Des_s {
  DWORD CSD_SignatureLength;
  DWORD CSD_LegacyDataOffset;
  DWORD CSD_LegacyDataSize;
  DWORD CSD_Flags;
  GUID CSD_ClassGuid;
  BYTE CSD_Signature[ANYSIZE_ARRAY];
} CS_DES, *PCS_DES;

typedef struct CS_Resource_s {
  CS_DES CS_Header;
} CS_RESOURCE, *PCS_RESOURCE;

#define mPCD_IO_8_16                      0x1
#define fPCD_IO_8                         0x0
#define fPCD_IO_16                        0x1
#define mPCD_MEM_8_16                     0x2
#define fPCD_MEM_8                        0x0
#define fPCD_MEM_16                       0x2
#define mPCD_MEM_A_C                      0xC
#define fPCD_MEM1_A                       0x4
#define fPCD_MEM2_A                       0x8
#define fPCD_IO_ZW_8                      0x10
#define fPCD_IO_SRC_16                    0x20
#define fPCD_IO_WS_16                     0x40
#define mPCD_MEM_WS                       0x300
#define fPCD_MEM_WS_ONE                   0x100
#define fPCD_MEM_WS_TWO                   0x200
#define fPCD_MEM_WS_THREE                 0x300

#if (WINVER >= _WIN32_WINNT_WINXP)

#define fPCD_MEM_A                        0x4

#define fPCD_ATTRIBUTES_PER_WINDOW        0x8000

#define fPCD_IO1_16                       0x00010000
#define fPCD_IO1_ZW_8                     0x00020000
#define fPCD_IO1_SRC_16                   0x00040000
#define fPCD_IO1_WS_16                    0x00080000

#define fPCD_IO2_16                       0x00100000
#define fPCD_IO2_ZW_8                     0x00200000
#define fPCD_IO2_SRC_16                   0x00400000
#define fPCD_IO2_WS_16                    0x00800000

#define mPCD_MEM1_WS                      0x03000000
#define fPCD_MEM1_WS_ONE                  0x01000000
#define fPCD_MEM1_WS_TWO                  0x02000000
#define fPCD_MEM1_WS_THREE                0x03000000
#define fPCD_MEM1_16                      0x04000000

#define mPCD_MEM2_WS                      0x30000000
#define fPCD_MEM2_WS_ONE                  0x10000000
#define fPCD_MEM2_WS_TWO                  0x20000000
#define fPCD_MEM2_WS_THREE                0x30000000
#define fPCD_MEM2_16                      0x40000000

#define PCD_MAX_MEMORY                    2
#define PCD_MAX_IO                        2

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

typedef struct PcCard_Des_s {
  DWORD PCD_Count;
  DWORD PCD_Type;
  DWORD PCD_Flags;
  BYTE PCD_ConfigIndex;
  BYTE PCD_Reserved[3];
  DWORD PCD_MemoryCardBase1;
  DWORD PCD_MemoryCardBase2;
#if (WINVER >= _WIN32_WINNT_WINXP)
  DWORD PCD_MemoryCardBase[PCD_MAX_MEMORY];
  WORD PCD_MemoryFlags[PCD_MAX_MEMORY];
  BYTE PCD_IoFlags[PCD_MAX_IO];
#endif
} PCCARD_DES, *PPCCARD_DES;

typedef struct PcCard_Resource_s {
  PCCARD_DES PcCard_Header;
} PCCARD_RESOURCE, *PPCCARD_RESOURCE;

/* MFCARD_DES.PMF_Flags constants */
#define fPMF_AUDIO_ENABLE                 0x8
#define mPMF_AUDIO_ENABLE                 fPMF_AUDIO_ENABLE

typedef struct MfCard_Des_s {
  DWORD PMF_Count;
  DWORD PMF_Type;
  DWORD PMF_Flags;
  BYTE PMF_ConfigOptions;
  BYTE PMF_IoResourceIndex;
  BYTE PMF_Reserved[2];
  DWORD PMF_ConfigRegisterBase;
} MFCARD_DES, *PMFCARD_DES;

typedef struct MfCard_Resource_s {
  MFCARD_DES MfCard_Header;
} MFCARD_RESOURCE, *PMFCARD_RESOURCE;

typedef struct BusNumber_Des_s {
  DWORD BUSD_Count;
  DWORD BUSD_Type;
  DWORD BUSD_Flags;
  ULONG BUSD_Alloc_Base;
  ULONG BUSD_Alloc_End;
} BUSNUMBER_DES, *PBUSNUMBER_DES;

typedef struct BusNumber_Range_s {
  ULONG BUSR_Min;
  ULONG BUSR_Max;
  ULONG BUSR_nBusNumbers;
  ULONG BUSR_Flags;
} BUSNUMBER_RANGE, *PBUSNUMBER_RANGE;

#define BusNumberType_Range sizeof(BUSNUMBER_RANGE)

typedef struct BusNumber_Resource_s {
  BUSNUMBER_DES BusNumber_Header;
  BUSNUMBER_RANGE BusNumber_Data[ANYSIZE_ARRAY];
} BUSNUMBER_RESOURCE, *PBUSNUMBER_RESOURCE;

#define CM_HWPI_NOT_DOCKABLE           0x00000000
#define CM_HWPI_UNDOCKED               0x00000001
#define CM_HWPI_DOCKED                 0x00000002

typedef struct HWProfileInfo_sA {
  ULONG HWPI_ulHWProfile;
  CHAR HWPI_szFriendlyName[MAX_PROFILE_LEN];
  DWORD HWPI_dwFlags;
} HWPROFILEINFO_A, *PHWPROFILEINFO_A;

typedef struct HWProfileInfo_sW {
  ULONG HWPI_ulHWProfile;
  WCHAR HWPI_szFriendlyName[MAX_PROFILE_LEN];
  DWORD HWPI_dwFlags;
} HWPROFILEINFO_W, *PHWPROFILEINFO_W;

#ifdef UNICODE
typedef HWPROFILEINFO_W HWPROFILEINFO;
typedef PHWPROFILEINFO_W PHWPROFILEINFO;
#else
typedef HWPROFILEINFO_A HWPROFILEINFO;
typedef PHWPROFILEINFO_A PHWPROFILEINFO;
#endif

#include <poppack.h>

#define ResType_All                       0x00000000
#define ResType_None                      0x00000000
#define ResType_Mem                       0x00000001
#define ResType_IO                        0x00000002
#define ResType_DMA                       0x00000003
#define ResType_IRQ                       0x00000004
#define ResType_DoNotUse                  0x00000005
#define ResType_BusNumber                 0x00000006
#define ResType_MemLarge                  0x00000007
#define ResType_MAX                       0x00000007
#define ResType_Ignored_Bit               0x00008000
#define ResType_ClassSpecific             0x0000FFFF
#define ResType_Reserved                  0x00008000
#define ResType_DevicePrivate             0x00008001
#define ResType_PcCardConfig              0x00008002
#define ResType_MfCardConfig              0x00008003

#define CM_ADD_RANGE_ADDIFCONFLICT        0x00000000
#define CM_ADD_RANGE_DONOTADDIFCONFLICT   0x00000001
#define CM_ADD_RANGE_BITS                 0x00000001

#define BASIC_LOG_CONF                    0x00000000
#define FILTERED_LOG_CONF                 0x00000001
#define ALLOC_LOG_CONF                    0x00000002
#define BOOT_LOG_CONF                     0x00000003
#define FORCED_LOG_CONF                   0x00000004
#define OVERRIDE_LOG_CONF                 0x00000005
#define NUM_LOG_CONF                      0x00000006
#define LOG_CONF_BITS                     0x00000007

#define PRIORITY_EQUAL_FIRST              0x00000008
#define PRIORITY_EQUAL_LAST               0x00000000
#define PRIORITY_BIT                      0x00000008

#define RegDisposition_OpenAlways         0x00000000
#define RegDisposition_OpenExisting       0x00000001
#define RegDisposition_Bits               0x00000001

/* CM_Add_ID.ulFlags constants */
#define CM_ADD_ID_HARDWARE                0x00000000
#define CM_ADD_ID_COMPATIBLE              0x00000001
#define CM_ADD_ID_BITS                    0x00000001

/* Flags for CM_Create_DevNode[_Ex].ulFlags constants */
#define CM_CREATE_DEVNODE_NORMAL          0x00000000
#define CM_CREATE_DEVNODE_NO_WAIT_INSTALL 0x00000001
#define CM_CREATE_DEVNODE_PHANTOM         0x00000002
#define CM_CREATE_DEVNODE_GENERATE_ID     0x00000004
#define CM_CREATE_DEVNODE_DO_NOT_INSTALL  0x00000008
#define CM_CREATE_DEVNODE_BITS            0x0000000F

#define CM_CREATE_DEVINST_NORMAL          CM_CREATE_DEVNODE_NORMAL
#define CM_CREATE_DEVINST_NO_WAIT_INSTALL CM_CREATE_DEVNODE_NO_WAIT_INSTALL
#define CM_CREATE_DEVINST_PHANTOM         CM_CREATE_DEVNODE_PHANTOM
#define CM_CREATE_DEVINST_GENERATE_ID     CM_CREATE_DEVNODE_GENERATE_ID
#define CM_CREATE_DEVINST_DO_NOT_INSTALL  CM_CREATE_DEVNODE_DO_NOT_INSTALL
#define CM_CREATE_DEVINST_BITS            CM_CREATE_DEVNODE_BITS

/* Flags for CM_Delete_Class_Key.ulFlags constants */
#define CM_DELETE_CLASS_ONLY              0x00000000
#define CM_DELETE_CLASS_SUBKEYS           0x00000001
#if (WINVER >= _WIN32_WINNT_VISTA)
#define CM_DELETE_CLASS_INTERFACE         0x00000002
#endif
#define CM_DELETE_CLASS_BITS              0x00000003

/* CM_Run_Detection[_Ex].ulFlags constants */
#define CM_DETECT_NEW_PROFILE       0x00000001
#define CM_DETECT_CRASHED           0x00000002
#define CM_DETECT_HWPROF_FIRST_BOOT 0x00000004
#define CM_DETECT_RUN               0x80000000
#define CM_DETECT_BITS              0x80000007

#define CM_DISABLE_POLITE              0x00000000
#define CM_DISABLE_ABSOLUTE            0x00000001
#define CM_DISABLE_HARDWARE            0x00000002
#define CM_DISABLE_UI_NOT_OK           0x00000004
#define CM_DISABLE_BITS                0x00000007

#define CM_GETIDLIST_FILTER_NONE               0x00000000
#define CM_GETIDLIST_FILTER_ENUMERATOR         0x00000001
#define CM_GETIDLIST_FILTER_SERVICE            0x00000002
#define CM_GETIDLIST_FILTER_EJECTRELATIONS     0x00000004
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS   0x00000008
#define CM_GETIDLIST_FILTER_POWERRELATIONS     0x00000010
#define CM_GETIDLIST_FILTER_BUSRELATIONS       0x00000020
#define CM_GETIDLIST_DONOTGENERATE             0x10000040
#if (WINVER <= _WIN32_WINNT_VISTA)
#define CM_GETIDLIST_FILTER_BITS               0x1000007F
#endif
#if (WINVER >= _WIN32_WINNT_WIN7)
#define CM_GETIDLIST_FILTER_TRANSPORTRELATIONS 0x00000080
#define CM_GETIDLIST_FILTER_PRESENT            0x00000100
#define CM_GETIDLIST_FILTER_CLASS              0x00000200
#define CM_GETIDLIST_FILTER_BITS               0x100003FF
#endif

#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT      0x00000000
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES  0x00000001
#define CM_GET_DEVICE_INTERFACE_LIST_BITS         0x00000001

#define CM_DRP_DEVICEDESC                  0x00000001
#define CM_DRP_HARDWAREID                  0x00000002
#define CM_DRP_COMPATIBLEIDS               0x00000003
#define CM_DRP_UNUSED0                     0x00000004
#define CM_DRP_SERVICE                     0x00000005
#define CM_DRP_UNUSED1                     0x00000006
#define CM_DRP_UNUSED2                     0x00000007
#define CM_DRP_CLASS                       0x00000008
#define CM_DRP_CLASSGUID                   0x00000009
#define CM_DRP_DRIVER                      0x0000000A
#define CM_DRP_CONFIGFLAGS                 0x0000000B
#define CM_DRP_MFG                         0x0000000C
#define CM_DRP_FRIENDLYNAME                0x0000000D
#define CM_DRP_LOCATION_INFORMATION        0x0000000E
#define CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME 0x0000000F
#define CM_DRP_CAPABILITIES                0x00000010
#define CM_DRP_UI_NUMBER                   0x00000011
#define CM_DRP_UPPERFILTERS                0x00000012
#if (WINVER >= _WIN32_WINNT_VISTA)
#define CM_CRP_UPPERFILTERS                CM_DRP_UPPERFILTERS
#endif
#define CM_DRP_LOWERFILTERS                0x00000013
#if (WINVER >= _WIN32_WINNT_VISTA)
#define CM_CRP_LOWERFILTERS                CM_DRP_LOWERFILTERS
#endif
#define CM_DRP_BUSTYPEGUID                 0x00000014
#define CM_DRP_LEGACYBUSTYPE               0x00000015
#define CM_DRP_BUSNUMBER                   0x00000016
#define CM_DRP_ENUMERATOR_NAME             0x00000017
#define CM_DRP_SECURITY                    0x00000018
#define CM_CRP_SECURITY                    CM_DRP_SECURITY
#define CM_DRP_SECURITY_SDS                0x00000019
#define CM_CRP_SECURITY_SDS                CM_DRP_SECURITY_SDS
#define CM_DRP_DEVTYPE                     0x0000001A
#define CM_CRP_DEVTYPE                     CM_DRP_DEVTYPE
#define CM_DRP_EXCLUSIVE                   0x0000001B
#define CM_CRP_EXCLUSIVE                   CM_DRP_EXCLUSIVE
#define CM_DRP_CHARACTERISTICS             0x0000001C
#define CM_CRP_CHARACTERISTICS             CM_DRP_CHARACTERISTICS
#define CM_DRP_ADDRESS                     0x0000001D
#define CM_DRP_UI_NUMBER_DESC_FORMAT       0x0000001E
#if (WINVER >= _WIN32_WINNT_WINXP)
#define CM_DRP_DEVICE_POWER_DATA           0x0000001F
#define CM_DRP_REMOVAL_POLICY              0x00000020
#define CM_DRP_REMOVAL_POLICY_HW_DEFAULT   0x00000021
#define CM_DRP_REMOVAL_POLICY_OVERRIDE     0x00000022
#define CM_DRP_INSTALL_STATE               0x00000023
#endif
#if (WINVER >= _WIN32_WINNT_WS03)
#define CM_DRP_LOCATION_PATHS              0x00000024
#endif
#if (WINVER >= _WIN32_WINNT_WIN7)
#define CM_DRP_BASE_CONTAINERID            0x00000025
#endif
#define CM_DRP_MIN                         0x00000001
#define CM_CRP_MIN                         CM_DRP_MIN
#define CM_DRP_MAX                         0x00000025
#define CM_CRP_MAX                         CM_DRP_MAX

#define CM_DEVCAP_LOCKSUPPORTED            0x00000001
#define CM_DEVCAP_EJECTSUPPORTED           0x00000002
#define CM_DEVCAP_REMOVABLE                0x00000004
#define CM_DEVCAP_DOCKDEVICE               0x00000008
#define CM_DEVCAP_UNIQUEID                 0x00000010
#define CM_DEVCAP_SILENTINSTALL            0x00000020
#define CM_DEVCAP_RAWDEVICEOK              0x00000040
#define CM_DEVCAP_SURPRISEREMOVALOK        0x00000080
#define CM_DEVCAP_HARDWAREDISABLED         0x00000100
#define CM_DEVCAP_NONDYNAMIC               0x00000200

#if (WINVER >= _WIN32_WINNT_WINXP)

#define CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL           1
#define CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL      2
#define CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL     3

#define CM_INSTALL_STATE_INSTALLED                    0
#define CM_INSTALL_STATE_NEEDS_REINSTALL              1
#define CM_INSTALL_STATE_FAILED_INSTALL               2
#define CM_INSTALL_STATE_FINISH_INSTALL               3

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

/* CM_Locate_DevNode.ulFlags constants */
#define CM_LOCATE_DEVNODE_NORMAL         0x00000000
#define CM_LOCATE_DEVNODE_PHANTOM        0x00000001
#define CM_LOCATE_DEVNODE_CANCELREMOVE   0x00000002
#define CM_LOCATE_DEVNODE_NOVALIDATION   0x00000004
#define CM_LOCATE_DEVNODE_BITS           0x00000007

#define CM_LOCATE_DEVINST_NORMAL         CM_LOCATE_DEVNODE_NORMAL
#define CM_LOCATE_DEVINST_PHANTOM        CM_LOCATE_DEVNODE_PHANTOM
#define CM_LOCATE_DEVINST_CANCELREMOVE   CM_LOCATE_DEVNODE_CANCELREMOVE
#define CM_LOCATE_DEVINST_NOVALIDATION   CM_LOCATE_DEVNODE_NOVALIDATION
#define CM_LOCATE_DEVINST_BITS           CM_LOCATE_DEVNODE_BITS

#define CM_OPEN_CLASS_KEY_INSTALLER        0x00000000
#define CM_OPEN_CLASS_KEY_INTERFACE        0x00000001
#define CM_OPEN_CLASS_KEY_BITS             0x00000001

/* CM_Query_And_Remove_SubTree.ulFlags constants */
#define CM_REMOVE_UI_OK                    0x00000000
#define CM_REMOVE_UI_NOT_OK                0x00000001
#define CM_REMOVE_NO_RESTART               0x00000002
#define CM_REMOVE_BITS                     0x00000003

#define CM_QUERY_REMOVE_UI_OK (CM_REMOVE_UI_OK)
#define CM_QUERY_REMOVE_UI_NOT_OK (CM_REMOVE_UI_NOT_OK)
#define CM_QUERY_REMOVE_BITS (CM_QUERY_REMOVE_UI_OK|CM_QUERY_REMOVE_UI_NOT_OK)

/* CM_Reenumerate_DevNode.ulFlags constants */
#define CM_REENUMERATE_NORMAL              0x00000000
#define CM_REENUMERATE_SYNCHRONOUS         0x00000001
#if (WINVER >= _WIN32_WINNT_WINXP)
#define CM_REENUMERATE_RETRY_INSTALLATION  0x00000002
#define CM_REENUMERATE_ASYNCHRONOUS        0x00000004
#endif
#define CM_REENUMERATE_BITS                0x00000007

#define CM_REGISTER_DEVICE_DRIVER_STATIC         0x00000000
#define CM_REGISTER_DEVICE_DRIVER_DISABLEABLE    0x00000001
#define CM_REGISTER_DEVICE_DRIVER_REMOVABLE      0x00000002
#define CM_REGISTER_DEVICE_DRIVER_BITS           0x00000003

#define CM_REGISTRY_HARDWARE               0x00000000
#define CM_REGISTRY_SOFTWARE               0x00000001
#define CM_REGISTRY_USER                   0x00000100
#define CM_REGISTRY_CONFIG                 0x00000200
#define CM_REGISTRY_BITS                   0x00000301

#define CM_SET_DEVNODE_PROBLEM_NORMAL      0x00000000
#define CM_SET_DEVNODE_PROBLEM_OVERRIDE    0x00000001
#define CM_SET_DEVNODE_PROBLEM_BITS        0x00000001

#define CM_SET_DEVINST_PROBLEM_NORMAL CM_SET_DEVNODE_PROBLEM_NORMAL
#define CM_SET_DEVINST_PROBLEM_OVERRIDE CM_SET_DEVNODE_PROBLEM_OVERRIDE
#define CM_SET_DEVINST_PROBLEM_BITS CM_SET_DEVNODE_PROBLEM_BITS

/* CM_Set_HW_Prof_Flags[_Ex].ulFlags constants */
#define CM_SET_HW_PROF_FLAGS_UI_NOT_OK     0x00000001
#define CM_SET_HW_PROF_FLAGS_BITS          0x00000001

/* CM_Setup_DevInst[_Ex].ulFlags constants */
#define CM_SETUP_DEVNODE_READY             0x00000000
#define CM_SETUP_DEVINST_READY             CM_SETUP_DEVNODE_READY
#define CM_SETUP_DOWNLOAD                  0x00000001
#define CM_SETUP_WRITE_LOG_CONFS           0x00000002
#define CM_SETUP_PROP_CHANGE               0x00000003
#if (WINVER >= _WIN32_WINNT_WINXP)
#define CM_SETUP_DEVNODE_RESET             0x00000004
#define CM_SETUP_DEVINST_RESET             CM_SETUP_DEVNODE_RESET
#endif
#define CM_SETUP_BITS                      0x00000007

#define CM_QUERY_ARBITRATOR_RAW            0x00000000
#define CM_QUERY_ARBITRATOR_TRANSLATED     0x00000001
#define CM_QUERY_ARBITRATOR_BITS           0x00000001

#if (WINVER >= _WIN32_WINNT_WINXP)
#define CM_CUSTOMDEVPROP_MERGE_MULTISZ     0x00000001
#define CM_CUSTOMDEVPROP_BITS              0x00000001
#endif

#define CM_NAME_ATTRIBUTE_NAME_RETRIEVED_FROM_DEVICE 0x1
#define CM_NAME_ATTRIBUTE_USER_ASSIGNED_NAME         0x2

#define CR_SUCCESS                        0x00000000
#define CR_DEFAULT                        0x00000001
#define CR_OUT_OF_MEMORY                  0x00000002
#define CR_INVALID_POINTER                0x00000003
#define CR_INVALID_FLAG                   0x00000004
#define CR_INVALID_DEVNODE                0x00000005
#define CR_INVALID_DEVINST                CR_INVALID_DEVNODE
#define CR_INVALID_RES_DES                0x00000006
#define CR_INVALID_LOG_CONF               0x00000007
#define CR_INVALID_ARBITRATOR             0x00000008
#define CR_INVALID_NODELIST               0x00000009
#define CR_DEVNODE_HAS_REQS               0x0000000A
#define CR_DEVINST_HAS_REQS               CR_DEVNODE_HAS_REQS
#define CR_INVALID_RESOURCEID             0x0000000B
#define CR_DLVXD_NOT_FOUND                0x0000000C
#define CR_NO_SUCH_DEVNODE                0x0000000D
#define CR_NO_SUCH_DEVINST                CR_NO_SUCH_DEVNODE
#define CR_NO_MORE_LOG_CONF               0x0000000E
#define CR_NO_MORE_RES_DES                0x0000000F
#define CR_ALREADY_SUCH_DEVNODE           0x00000010
#define CR_ALREADY_SUCH_DEVINST           CR_ALREADY_SUCH_DEVNODE
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
#define NUM_CR_RESULTS                    0x0000003C

#define CM_GLOBAL_STATE_CAN_DO_UI            0x00000001
#define CM_GLOBAL_STATE_ON_BIG_STACK         0x00000002
#define CM_GLOBAL_STATE_SERVICES_AVAILABLE   0x00000004
#define CM_GLOBAL_STATE_SHUTTING_DOWN        0x00000008
#define CM_GLOBAL_STATE_DETECTION_PENDING    0x00000010
#if (WINVER >= _WIN32_WINNT_WIN7)
#define CM_GLOBAL_STATE_REBOOT_REQUIRED      0x00000020
#endif

/* FIXME : These definitions don't exist in the official header

#define CMP_MAGIC  0x01234567

CMAPI
CONFIGRET
WINAPI
CMP_GetServerSideDeviceInstallFlags(
  _Out_ PULONG pulSSDIFlags,
  _In_ ULONG ulFlags,
  _In_ HMACHINE hMachine)

CMAPI
CONFIGRET
WINAPI
CMP_Init_Detection(
  _In_ DWORD dwMagic);

CMAPI
CONFIGRET
WINAPI
CMP_RegisterNotification(
  _In_ HANDLE hRecipient,
  _In_ LPVOID lpvNotificationFilter,
  _In_ DWORD dwFlags,
  _Out_ PULONG pluhDevNotify);

CMAPI
CONFIGRET
WINAPI
CMP_Report_LogOn(
  _In_ DWORD dwMagic,
  _In_ DWORD dwProcessId);

CMAPI
CONFIGRET
WINAPI
CMP_UnregisterNotification(
  _In_ ULONG luhDevNotify);

CMAPI
CONFIGRET
WINAPI
CMP_WaitServicesAvailable(
  _In_ IN HMACHINE hMachine);
*/

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
  _Out_ PLOG_CONF plcLogConf,
  _In_ DEVINST dnDevInst,
  _In_ PRIORITY Priority,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf_Ex(
  _Out_ PLOG_CONF plcLogConf,
  _In_ DEVINST dnDevInst,
  _In_ PRIORITY Priority,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_IDA(
  _In_ DEVINST dnDevInst,
  _In_ PSTR pszID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExA(
  _In_ DEVINST dnDevInst,
  _In_ PSTR pszID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExW(
  _In_ DEVINST dnDevInst,
  _In_ PWSTR pszID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_IDW(
  _In_ DEVINST dnDevInst,
  _In_ PWSTR pszID,
  _In_ ULONG ulFlags);

#ifdef UNICODE
#define CM_Add_ID CM_Add_IDW
#define CM_Add_ID_Ex CM_Add_ID_ExW
#else
#define CM_Add_ID CM_Add_IDA
#define CM_Add_ID_Ex CM_Add_ID_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Add_Range(
  _In_ DWORDLONG ullStartValue,
  _In_ DWORDLONG ullEndValue,
  _In_ RANGE_LIST rlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des(
  _Out_opt_ PRES_DES prdResDes,
  _In_ LOG_CONF lcLogConf,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des_Ex(
  _Out_opt_ PRES_DES prdResDes,
  _In_ LOG_CONF lcLogConf,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineA(
  _In_opt_ PCSTR UNCServerName,
  _Out_ PHMACHINE phMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineW(
  _In_opt_ PCWSTR UNCServerName,
  _Out_ PHMACHINE phMachine);

#ifdef UNICODE
#define CM_Connect_Machine CM_Connect_MachineW
#else
#define CM_Connect_Machine CM_Connect_MachineA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeA(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINSTID_A pDeviceID,
  _In_ DEVINST dnParent,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeW(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINSTID_W pDeviceID,
  _In_ DEVINST dnParent,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExA(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINSTID_A pDeviceID,
  _In_ DEVINST dnParent,
  _In_ ULONG ulFlags,
  _In_opt_ HANDLE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExW(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINSTID_W pDeviceID,
  _In_ DEVINST dnParent,
  _In_ ULONG ulFlags,
  _In_opt_ HANDLE hMachine);

#define CM_Create_DevInstW CM_Create_DevNodeW
#define CM_Create_DevInstA CM_Create_DevNodeA
#define CM_Create_DevInst_ExW CM_Create_DevNode_ExW
#define CM_Create_DevInst_ExA CM_Create_DevNode_ExA
#ifdef UNICODE
#define CM_Create_DevNode CM_Create_DevNodeW
#define CM_Create_DevInst CM_Create_DevNodeW
#define CM_Create_DevNode_Ex CM_Create_DevNode_ExW
#define CM_Create_DevInst_Ex CM_Create_DevInst_ExW
#else
#define CM_Create_DevNode CM_Create_DevNodeA
#define CM_Create_DevInst CM_Create_DevNodeA
#define CM_Create_DevNode_Ex CM_Create_DevNode_ExA
#define CM_Create_DevInst_Ex CM_Create_DevNode_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Create_Range_List(
  _Out_ PRANGE_LIST prlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key_Ex(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulFlags,
  _In_opt_ HANDLE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key(
  _In_ DEVNODE dnDevNode,
  _In_ ULONG ulHardwareProfile,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key_Ex(
  _In_ DEVNODE dnDevNode,
  _In_ ULONG ulHardwareProfile,
  _In_ ULONG ulFlags,
  _In_opt_ HANDLE hMachine);

#define CM_Delete_DevInst_Key CM_Delete_DevNode_Key
#define CM_Delete_DevInst_Key_Ex CM_Delete_DevNode_Key_Ex

CMAPI
CONFIGRET
WINAPI
CM_Delete_Range(
  _In_ DWORDLONG ullStartValue,
  _In_ DWORDLONG ullEndValue,
  _In_ RANGE_LIST rlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict(
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _Out_ PBOOL pbConflictDetected,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict_Ex(
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _Out_ PBOOL pbConflictDetected,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Disable_DevInst CM_Disable_DevNode
#define CM_Disable_DevInst_Ex CM_Disable_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Disconnect_Machine(
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Dup_Range_List(
  _In_ RANGE_LIST rlhOld,
  _In_ RANGE_LIST rlhNew,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Enable_DevInst CM_Enable_DevNode
#define CM_Enable_DevInst_Ex CM_Enable_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes(
  _In_ ULONG ulClassIndex,
  _Out_ LPGUID ClassGuid,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes_Ex(
  _In_ ULONG ulClassIndex,
  _Out_ LPGUID ClassGuid,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsA(
  _In_ ULONG ulEnumIndex,
  _Out_writes_(*pulLength) PCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExA(
  _In_ ULONG ulEnumIndex,
  _Out_writes_(*pulLength) PCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExW(
  _In_ ULONG ulEnumIndex,
  _Out_writes_(*pulLength) PWCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsW(
  _In_ ULONG ulEnumIndex,
  _Out_writes_(*pulLength) PWCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

#ifdef UNICODE
#define CM_Enumerate_Enumerators CM_Enumerate_EnumeratorsW
#define CM_Enumerate_Enumerators_Ex CM_Enumerate_Enumerators_ExW
#else
#define CM_Enumerate_Enumerators CM_Enumerate_EnumeratorsA
#define CM_Enumerate_Enumerators_Ex CM_Enumerate_Enumerators_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Find_Range(
  _Out_ PDWORDLONG pullStart,
  _In_ DWORDLONG ullStart,
  _In_ ULONG ulLength,
  _In_ DWORDLONG ullAlignment,
  _In_ DWORDLONG ullEnd,
  _In_ RANGE_LIST rlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_First_Range(
  _In_ RANGE_LIST rlh,
  _Out_ PDWORDLONG pullStart,
  _Out_ PDWORDLONG pullEnd,
  _Out_ PRANGE_ELEMENT preElement,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf(
  _In_ LOG_CONF lcLogConfToBeFreed,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Ex(
  _In_ LOG_CONF lcLogConfToBeFreed,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
  _In_ LOG_CONF lcLogConf);

CMAPI
CONFIGRET
WINAPI
CM_Free_Range_List(
  _In_ RANGE_LIST rlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des(
  _Out_opt_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Ex(
  _Out_opt_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Handle(
  _In_ RES_DES rdResDes);

CMAPI
CONFIGRET
WINAPI
CM_Free_Resource_Conflict_Handle(
  _In_ CONFLICT_LIST clConflictList);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child_Ex(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameA(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) PCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameW(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) PWCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExA(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) PCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExW(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) PWCHAR Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Class_Name CM_Get_Class_NameW
#define CM_Get_Class_Name_Ex CM_Get_Class_Name_ExW
#else
#define CM_Get_Class_Name CM_Get_Class_NameA
#define CM_Get_Class_Name_Ex CM_Get_Class_Name_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_NameA(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) LPSTR pszKeyName,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_NameW(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) LPWSTR pszKeyName,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExA(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) LPSTR pszKeyName,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExW(
  _In_ LPGUID ClassGuid,
  _Out_writes_opt_(*pulLength) LPWSTR pszKeyName,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Class_Key_Name CM_Get_Class_Key_NameW
#define CM_Get_Class_Key_Name_Ex CM_Get_Class_Key_Name_ExW
#else
#define CM_Get_Class_Key_Name CM_Get_Class_Key_NameA
#define CM_Get_Class_Key_Name_Ex CM_Get_Class_Key_Name_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Depth(
  _Out_ PULONG pulDepth,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Depth_Ex(
  _Out_ PULONG pulDepth,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDA(
  _In_ DEVINST dnDevInst,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExA(
  _In_ DEVINST dnDevInst,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
  _In_ DEVINST dnDevInst,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDW(
  _In_ DEVINST dnDevInst,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

#ifdef UNICODE
#define CM_Get_Device_ID CM_Get_Device_IDW
#define CM_Get_Device_ID_Ex CM_Get_Device_ID_ExW
#else
#define CM_Get_Device_ID CM_Get_Device_IDA
#define CM_Get_Device_ID_Ex CM_Get_Device_ID_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListA(
  _In_opt_ PCSTR pszFilter,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExA(
  _In_opt_ PCSTR pszFilter,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExW(
  _In_opt_ PCWSTR pszFilter,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListW(
  _In_opt_ PCWSTR pszFilter,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

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
  _Out_ PULONG pulLen,
  _In_opt_ PCSTR pszFilter,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExA(
  _Out_ PULONG pulLen,
  _In_opt_ PCSTR pszFilter,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExW(
  _Out_ PULONG pulLen,
  _In_opt_ PCWSTR pszFilter,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeW(
  _Out_ PULONG pulLen,
  _In_opt_ PCWSTR pszFilter,
  _In_ ULONG ulFlags);

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
  _Out_ PULONG pulLen,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size_Ex(
  _Out_ PULONG pulLen,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyA(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyW(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExA(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExW(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Get_DevInst_Registry_PropertyW CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevInst_Registry_PropertyA CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevInst_Registry_Property_ExW CM_Get_DevNode_Registry_Property_ExW
#define CM_Get_DevInst_Registry_Property_ExA CM_Get_DevNode_Registry_Property_ExA

#ifdef UNICODE
#define CM_Get_DevInst_Registry_Property CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevInst_Registry_Property_Ex CM_Get_DevNode_Registry_Property_ExW
#define CM_Get_DevNode_Registry_Property CM_Get_DevNode_Registry_PropertyW
#define CM_Get_DevNode_Registry_Property_Ex CM_Get_DevNode_Registry_Property_ExW
#else
#define CM_Get_DevInst_Registry_Property CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevInst_Registry_Property_Ex CM_Get_DevNode_Registry_Property_ExA
#define CM_Get_DevNode_Registry_Property CM_Get_DevNode_Registry_PropertyA
#define CM_Get_DevNode_Registry_Property_Ex CM_Get_DevNode_Registry_Property_ExA
#endif /* UNICODE */

#if (WINVER >= _WIN32_WINNT_WINXP)

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyA(
  _In_ DEVINST dnDevInst,
  _In_ PCSTR pszCustomPropertyName,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyW(
  _In_ DEVINST dnDevInst,
  _In_ PCWSTR pszCustomPropertyName,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExA(
  _In_ DEVINST dnDevInst,
  _In_ PCSTR pszCustomPropertyName,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExW(
  _In_ DEVINST dnDevInst,
  _In_ PCWSTR pszCustomPropertyName,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Get_DevInst_Custom_PropertyW     CM_Get_DevNode_Custom_PropertyW
#define CM_Get_DevInst_Custom_PropertyA     CM_Get_DevNode_Custom_PropertyA
#define CM_Get_DevInst_Custom_Property_ExW  CM_Get_DevNode_Custom_Property_ExW
#define CM_Get_DevInst_Custom_Property_ExA  CM_Get_DevNode_Custom_Property_ExA
#ifdef UNICODE
#define CM_Get_DevInst_Custom_Property      CM_Get_DevNode_Custom_PropertyW
#define CM_Get_DevInst_Custom_Property_Ex   CM_Get_DevNode_Custom_Property_ExW
#define CM_Get_DevNode_Custom_Property      CM_Get_DevNode_Custom_PropertyW
#define CM_Get_DevNode_Custom_Property_Ex   CM_Get_DevNode_Custom_Property_ExW
#else
#define CM_Get_DevInst_Custom_Property      CM_Get_DevNode_Custom_PropertyA
#define CM_Get_DevInst_Custom_Property_Ex   CM_Get_DevNode_Custom_Property_ExA
#define CM_Get_DevNode_Custom_Property      CM_Get_DevNode_Custom_PropertyA
#define CM_Get_DevNode_Custom_Property_Ex   CM_Get_DevNode_Custom_Property_ExA
#endif

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status(
  _Out_ PULONG pulStatus,
  _Out_ PULONG pulProblemNumber,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
  _Out_ PULONG pulStatus,
  _Out_ PULONG pulProblemNumber,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Get_DevInst_Status CM_Get_DevNode_Status
#define CM_Get_DevInst_Status_Ex CM_Get_DevNode_Status_Ex

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
  _Out_opt_ PLOG_CONF plcLogConf,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf_Ex(
  _Out_opt_ PLOG_CONF plcLogConf,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State(
  _Out_ PULONG pulState,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State_Ex(
  _Out_ PULONG pulState,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoA(
  _In_ ULONG ulIndex,
  _Out_ PHWPROFILEINFO_A pHWProfileInfo,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExA(
  _In_ ULONG ulIndex,
  _Out_ PHWPROFILEINFO_A pHWProfileInfo,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoW(
  _In_ ULONG ulIndex,
  _Out_ PHWPROFILEINFO_W pHWProfileInfo,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExW(
  _In_ ULONG ulIndex,
  _Out_ PHWPROFILEINFO_W pHWProfileInfo,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Hardware_Profile_Info      CM_Get_Hardware_Profile_InfoW
#define CM_Get_Hardware_Profile_Info_Ex   CM_Get_Hardware_Profile_Info_ExW
#else
#define CM_Get_Hardware_Profile_Info      CM_Get_Hardware_Profile_InfoA
#define CM_Get_Hardware_Profile_Info_Ex   CM_Get_Hardware_Profile_Info_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsA(
  _In_ DEVINSTID_A szDevInstName,
  _In_ ULONG ulHardwareProfile,
  _Out_ PULONG pulValue,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsW(
  _In_ DEVINSTID_W szDevInstName,
  _In_ ULONG ulHardwareProfile,
  _Out_ PULONG pulValue,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExA(
  _In_ DEVINSTID_A szDevInstName,
  _In_ ULONG ulHardwareProfile,
  _Out_ PULONG pulValue,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExW(
  _In_ DEVINSTID_W szDevInstName,
  _In_ ULONG ulHardwareProfile,
  _Out_ PULONG pulValue,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_HW_Prof_Flags CM_Get_HW_Prof_FlagsW
#define CM_Get_HW_Prof_Flags_Ex CM_Get_HW_Prof_Flags_ExW
#else
#define CM_Get_HW_Prof_Flags CM_Get_HW_Prof_FlagsA
#define CM_Get_HW_Prof_Flags_Ex CM_Get_HW_Prof_Flags_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ LPGUID AliasInterfaceGuid,
  _Out_writes_(*pulLength) LPSTR pszAliasDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ LPGUID AliasInterfaceGuid,
  _Out_writes_(*pulLength) LPWSTR pszAliasDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ LPGUID AliasInterfaceGuid,
  _Out_writes_(*pulLength) LPSTR pszAliasDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ LPGUID AliasInterfaceGuid,
  _Out_writes_(*pulLength) LPWSTR pszAliasDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Device_Interface_Alias     CM_Get_Device_Interface_AliasW
#define CM_Get_Device_Interface_Alias_Ex  CM_Get_Device_Interface_Alias_ExW
#else
#define CM_Get_Device_Interface_Alias     CM_Get_Device_Interface_AliasA
#define CM_Get_Device_Interface_Alias_Ex  CM_Get_Device_Interface_Alias_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListA(
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_A pDeviceID,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_W pDeviceID,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExA(
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_A pDeviceID,
  _Out_writes_(BufferLen) PCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_W pDeviceID,
  _Out_writes_(BufferLen) PWCHAR Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Device_Interface_List     CM_Get_Device_Interface_ListW
#define CM_Get_Device_Interface_List_Ex  CM_Get_Device_Interface_List_ExW
#else
#define CM_Get_Device_Interface_List     CM_Get_Device_Interface_ListA
#define CM_Get_Device_Interface_List_Ex  CM_Get_Device_Interface_List_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeA(
  _Out_ PULONG pulLen,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_A pDeviceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
  _Out_ PULONG pulLen,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_W pDeviceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExA(
  _Out_ PULONG pulLen,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_A pDeviceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
  _Out_ PULONG pulLen,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ DEVINSTID_W pDeviceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Device_Interface_List_Size     CM_Get_Device_Interface_List_SizeW
#define CM_Get_Device_Interface_List_Size_Ex  CM_Get_Device_Interface_List_Size_ExW
#else
#define CM_Get_Device_Interface_List_Size     CM_Get_Device_Interface_List_SizeA
#define CM_Get_Device_Interface_List_Size_Ex  CM_Get_Device_Interface_List_Size_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority(
  _In_ LOG_CONF lcLogConf,
  _Out_ PPRIORITY pPriority,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
  _In_ LOG_CONF lcLogConf,
  _Out_ PPRIORITY pPriority,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
  _Out_opt_ PLOG_CONF plcLogConf,
  _In_ LOG_CONF lcLogConf,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf_Ex(
  _Out_opt_ PLOG_CONF plcLogConf,
  _In_ LOG_CONF lcLogConf,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
  _Out_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ RESOURCEID ForResource,
  _Out_opt_ PRESOURCEID pResourceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des_Ex(
  _Out_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ RESOURCEID ForResource,
  _Out_opt_ PRESOURCEID pResourceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
  _In_ RES_DES rdResDes,
  _Out_writes_bytes_(BufferLen) PVOID Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Ex(
  _In_ RES_DES rdResDes,
  _Out_writes_bytes_(BufferLen) PVOID Buffer,
  _In_ ULONG BufferLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
  _Out_ PULONG pulSize,
  _In_ RES_DES rdResDes,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size_Ex(
  _Out_ PULONG pulSize,
  _In_ RES_DES rdResDes,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_Count(
  _In_ CONFLICT_LIST clConflictList,
  _Out_ PULONG pulCount);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsA(
  _In_ CONFLICT_LIST clConflictList,
  _In_ ULONG ulIndex,
  _Inout_ PCONFLICT_DETAILS_A pConflictDetails);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsW(
  _In_ CONFLICT_LIST clConflictList,
  _In_ ULONG ulIndex,
  _Inout_ PCONFLICT_DETAILS_W pConflictDetails);

#ifdef UNICODE
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsW
#else
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyW(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyW(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyA(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulProperty,
  _Out_opt_ PULONG pulRegDataType,
  _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyA(
  _In_ LPGUID ClassGuid,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Get_Class_Registry_Property CM_Get_Class_Registry_PropertyW
#define CM_Set_Class_Registry_Property CM_Set_Class_Registry_PropertyW
#else
#define CM_Get_Class_Registry_Property CM_Get_Class_Registry_PropertyA
#define CM_Set_Class_Registry_Property CM_Set_Class_Registry_PropertyA
#endif // UNICODE

CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST DevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling_Ex(
  _Out_ PDEVINST pdnDevInst,
  _In_ DEVINST DevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
WORD
WINAPI
CM_Get_Version(VOID);

CMAPI
WORD
WINAPI
CM_Get_Version_Ex(
  _In_opt_ HMACHINE hMachine);

#if (WINVER >= _WIN32_WINNT_WINXP)

CMAPI
BOOL
WINAPI
CM_Is_Version_Available(
  _In_ WORD wVersion);

CMAPI
BOOL
WINAPI
CM_Is_Version_Available_Ex(
  _In_ WORD wVersion,
  _In_opt_ HMACHINE hMachine);

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

CMAPI
CONFIGRET
WINAPI
CM_Intersect_Range_List(
  _In_ RANGE_LIST rlhOld1,
  _In_ RANGE_LIST rlhOld2,
  _In_ RANGE_LIST rlhNew,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Invert_Range_List(
  _In_ RANGE_LIST rlhOld,
  _In_ RANGE_LIST rlhNew,
  _In_ DWORDLONG ullMaxValue,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present(
  _Out_ PBOOL pbPresent);

CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present_Ex(
  _Out_ PBOOL pbPresent,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeA(
  _Out_ PDEVINST pdnDevInst,
  _In_opt_ DEVINSTID_A pDeviceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExA(
  _Out_ PDEVINST pdnDevInst,
  _In_opt_ DEVINSTID_A pDeviceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExW(
  _Out_ PDEVINST pdnDevInst,
  _In_opt_ DEVINSTID_W pDeviceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
  _Out_ PDEVINST pdnDevInst,
  _In_opt_ DEVINSTID_W pDeviceID,
  _In_ ULONG ulFlags);

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

CMAPI
CONFIGRET
WINAPI
CM_Merge_Range_List(
  _In_ RANGE_LIST rlhOld1,
  _In_ RANGE_LIST rlhOld2,
  _In_ RANGE_LIST rlhNew,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des(
  _Out_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des_Ex(
  _Out_ PRES_DES prdResDes,
  _In_ RES_DES rdResDes,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode(
  _In_ DEVINST dnFromDevInst,
  _In_ DEVINST dnToDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode_Ex(
  _In_ DEVINST dnFromDevInst,
  _In_ DEVINST dnToDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Move_DevInst                CM_Move_DevNode
#define CM_Move_DevInst_Ex             CM_Move_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Next_Range(
  _Inout_ PRANGE_ELEMENT preElement,
  _Out_ PDWORDLONG pullStart,
  _Out_ PDWORDLONG pullEnd,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyA(
  _In_opt_ LPGUID ClassGuid,
  _In_opt_ LPCSTR pszClassName,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkClass,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyW(
  _In_opt_ LPGUID ClassGuid,
  _In_opt_ LPCWSTR pszClassName,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkClass,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExA(
  _In_opt_ LPGUID pszClassGuid,
  _In_opt_ LPCSTR pszClassName,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkClass,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExW(
  _In_opt_ LPGUID pszClassGuid,
  _In_opt_ LPCWSTR pszClassName,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkClass,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Open_Class_Key CM_Open_Class_KeyW
#define CM_Open_Class_Key_Ex CM_Open_Class_Key_ExW
#else
#define CM_Open_Class_Key CM_Open_Class_KeyA
#define CM_Open_Class_Key_Ex CM_Open_Class_Key_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key(
  _In_ DEVINST dnDevNode,
  _In_ REGSAM samDesired,
  _In_ ULONG ulHardwareProfile,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDevice,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key_Ex(
  _In_ DEVINST dnDevNode,
  _In_ REGSAM samDesired,
  _In_ ULONG ulHardwareProfile,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDevice,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Open_DevInst_Key CM_Open_DevNode_Key
#define CM_Open_DevInst_Key_Ex CM_Open_DevNode_Key_Ex

#if (WINVER >= _WIN32_WINNT_VISTA)

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_KeyA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_KeyW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_Key_ExA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_Key_ExW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ REGSAM samDesired,
  _In_ REGDISPOSITION Disposition,
  _Out_ PHKEY phkDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Open_Device_Interface_Key    CM_Open_Device_Interface_KeyW
#define CM_Open_Device_Interface_Key_Ex CM_Open_Device_Interface_Key_ExW
#else
#define CM_Open_Device_Interface_Key    CM_Open_Device_Interface_KeyA
#define CM_Open_Device_Interface_Key_Ex CM_Open_Device_Interface_Key_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_KeyA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_KeyW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_Key_ExA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_Key_ExW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Delete_Device_Interface_Key    CM_Delete_Device_Interface_KeyW
#define CM_Delete_Device_Interface_Key_Ex CM_Delete_Device_Interface_Key_ExW
#else
#define CM_Delete_Device_Interface_Key    CM_Delete_Device_Interface_KeyA
#define CM_Delete_Device_Interface_Key_Ex CM_Delete_Device_Interface_Key_ExA
#endif

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data(
  _Out_writes_bytes_(DataLen) PVOID pData,
  _In_ ULONG DataLen,
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data_Ex(
  _Out_writes_bytes_(DataLen) PVOID pData,
  _In_ ULONG DataLen,
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size(
  _Out_ PULONG pulSize,
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size_Ex(
  _Out_ PULONG pulSize,
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree(
  _In_ DEVINST dnAncestor,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree_Ex(
  _In_ DEVINST dnAncestor,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeA(
  _In_ DEVINST dnAncestor,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeW(
  _In_ DEVINST dnAncestor,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExA(
  _In_ DEVINST dnAncestor,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExW(
  _In_ DEVINST dnAncestor,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Query_And_Remove_SubTree     CM_Query_And_Remove_SubTreeW
#define CM_Query_And_Remove_SubTree_Ex  CM_Query_And_Remove_SubTree_ExW
#else
#define CM_Query_And_Remove_SubTree     CM_Query_And_Remove_SubTreeA
#define CM_Query_And_Remove_SubTree_Ex  CM_Query_And_Remove_SubTree_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Query_Resource_Conflict_List(
  _Out_ PCONFLICT_LIST pclConflictList,
  _In_ DEVINST dnDevInst,
  _In_ RESOURCEID ResourceID,
  _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
  _In_ ULONG ResourceLen,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Reenumerate_DevInst CM_Reenumerate_DevNode
#define CM_Reenumerate_DevInst_Ex CM_Reenumerate_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceA(
  _In_ DEVINST dnDevInst,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ LPCSTR pszReference,
  _Out_writes_(*pulLength) LPSTR pszDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceW(
  _In_ DEVINST dnDevInst,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ LPCWSTR pszReference,
  _Out_writes_(*pulLength) LPWSTR pszDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExA(
  _In_ DEVINST dnDevInst,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ LPCSTR pszReference,
  _Out_writes_(*pulLength) LPSTR pszDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExW(
  _In_ DEVINST dnDevInst,
  _In_ LPGUID InterfaceClassGuid,
  _In_opt_ LPCWSTR pszReference,
  _Out_writes_(*pulLength) LPWSTR pszDeviceInterface,
  _Inout_ PULONG pulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Register_Device_Interface    CM_Register_Device_InterfaceW
#define CM_Register_Device_Interface_Ex CM_Register_Device_Interface_ExW
#else
#define CM_Register_Device_Interface    CM_Register_Device_InterfaceA
#define CM_Register_Device_Interface_Ex CM_Register_Device_Interface_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_EjectA(
  _In_ DEVINST dnDevInst,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExW(
  _In_ DEVINST dnDevInst,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExA(
  _In_ DEVINST dnDevInst,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_EjectW(
  _In_ DEVINST dnDevInst,
  _Out_opt_ PPNP_VETO_TYPE pVetoType,
  _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
  _In_ ULONG ulNameLength,
  _In_ ULONG ulFlags);

#ifdef UNICODE
#define CM_Request_Device_Eject CM_Request_Device_EjectW
#define CM_Request_Device_Eject_Ex CM_Request_Device_Eject_ExW
#else
#define CM_Request_Device_Eject CM_Request_Device_EjectA
#define CM_Request_Device_Eject_Ex CM_Request_Device_Eject_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC(VOID);

CMAPI
CONFIGRET
WINAPI
CM_Request_Eject_PC_Ex(
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Run_Detection(
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Run_Detection_Ex(
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#if (WINVER >= _WIN32_WINNT_VISTA)

CONFIGRET
CM_Apply_PowerScheme(VOID);

CONFIGRET
CM_Write_UserPowerKey(
  _In_opt_ CONST GUID *SchemeGuid,
  _In_opt_ CONST GUID *SubGroupOfPowerSettingsGuid,
  _In_opt_ CONST GUID *PowerSettingGuid,
  _In_ ULONG AccessFlags,
  _In_ ULONG Type,
  _In_reads_bytes_(BufferSize) UCHAR *Buffer,
  _In_ DWORD BufferSize,
  _Out_ PDWORD Error);

CONFIGRET
CM_Set_ActiveScheme(
  _In_ CONST GUID *SchemeGuid,
  _Out_ PDWORD Error);

CONFIGRET
CM_Restore_DefaultPowerScheme(
  _In_ CONST GUID *SchemeGuid,
  _Out_ PDWORD Error);

CONFIGRET
CM_RestoreAll_DefaultPowerSchemes(
  _Out_ PDWORD Error);

CONFIGRET
CM_Duplicate_PowerScheme(
  _In_ CONST GUID *SourceSchemeGuid,
  _Inout_ GUID **DestinationSchemeGuid,
  _Out_ PDWORD Error);

CONFIGRET
CM_Delete_PowerScheme(
  _In_ CONST GUID *SchemeGuid,
  _Out_ PDWORD Error);

CONFIGRET
CM_Import_PowerScheme(
  _In_ LPCWSTR ImportFileNamePath,
  _Inout_ GUID **DestinationSchemeGuid,
  _Out_ PDWORD Error);

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProblem,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProblem,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Set_DevInst_Problem CM_Set_DevNode_Problem
#define CM_Set_DevInst_Problem_Ex CM_Set_DevNode_Problem_Ex

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExA(
  _In_ LPCSTR pszDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExW(
  _In_ LPCWSTR pszDeviceInterface,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Unregister_Device_Interface    CM_Unregister_Device_InterfaceW
#define CM_Unregister_Device_Interface_Ex CM_Unregister_Device_Interface_ExW
#else
#define CM_Unregister_Device_Interface    CM_Unregister_Device_InterfaceA
#define CM_Unregister_Device_Interface_Ex CM_Unregister_Device_Interface_ExA
#endif

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Driver(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Driver_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree(
  _In_ DEVINST dnAncestor,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree_Ex(
  _In_ DEVINST dnAncestor,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyA(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyW(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExA(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExW(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Set_DevInst_Registry_PropertyW CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevInst_Registry_PropertyA CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevInst_Registry_Property_ExW CM_Set_DevNode_Registry_Property_ExW
#define CM_Set_DevInst_Registry_Property_ExA CM_Set_DevNode_Registry_Property_ExA

#ifdef UNICODE
#define CM_Set_DevInst_Registry_Property CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevInst_Registry_Property_Ex CM_Set_DevNode_Registry_Property_ExW
#define CM_Set_DevNode_Registry_Property CM_Set_DevNode_Registry_PropertyW
#define CM_Set_DevNode_Registry_Property_Ex CM_Set_DevNode_Registry_Property_ExW
#else
#define CM_Set_DevInst_Registry_Property CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevInst_Registry_Property_Ex CM_Set_DevNode_Registry_Property_ExA
#define CM_Set_DevNode_Registry_Property CM_Set_DevNode_Registry_PropertyA
#define CM_Set_DevNode_Registry_Property_Ex CM_Set_DevNode_Registry_Property_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof(
  _In_ ULONG ulHardwareProfile,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Ex(
  _In_ ULONG ulHardwareProfile,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsA(
  _In_ DEVINSTID_A szDevInstName,
  _In_ ULONG ulConfig,
  _In_ ULONG ulValue,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsW(
  _In_ DEVINSTID_W szDevInstName,
  _In_ ULONG ulConfig,
  _In_ ULONG ulValue,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExA(
  _In_ DEVINSTID_A szDevInstName,
  _In_ ULONG ulConfig,
  _In_ ULONG ulValue,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExW(
  _In_ DEVINSTID_W szDevInstName,
  _In_ ULONG ulConfig,
  _In_ ULONG ulValue,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#ifdef UNICODE
#define CM_Set_HW_Prof_Flags CM_Set_HW_Prof_FlagsW
#define CM_Set_HW_Prof_Flags_Ex CM_Set_HW_Prof_Flags_ExW
#else
#define CM_Set_HW_Prof_Flags CM_Set_HW_Prof_FlagsA
#define CM_Set_HW_Prof_Flags_Ex CM_Set_HW_Prof_Flags_ExA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Setup_DevNode(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Setup_DevNode_Ex(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Setup_DevInst         CM_Setup_DevNode
#define CM_Setup_DevInst_Ex      CM_Setup_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Test_Range_Available(
  _In_ DWORDLONG ullStartValue,
  _In_ DWORDLONG ullEndValue,
  _In_ RANGE_LIST rlh,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode(
  _In_ DEVINST dnPhantom,
  _In_ ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode_Ex(
  _In_ DEVINST dnPhantom,
  _In_ ULONG ulFlags,
  _In_opt_ HMACHINE hMachine);

#define CM_Uninstall_DevInst     CM_Uninstall_DevNode
#define CM_Uninstall_DevInst_Ex  CM_Uninstall_DevNode_Ex


#if (WINVER >= _WIN32_WINNT_WIN2K)

#define CM_WaitNoPendingInstallEvents CMP_WaitNoPendingInstallEvents

CMAPI
DWORD
WINAPI
CMP_WaitNoPendingInstallEvents(
  _In_ DWORD dwTimeout);

#endif /* (WINVER >= _WIN32_WINNT_WIN2K) */

#ifdef __cplusplus
}
#endif

#endif /* _CFGMGR32_H_ */
