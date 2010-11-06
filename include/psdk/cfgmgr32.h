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

typedef DWORD RETURN_TYPE;
typedef RETURN_TYPE CONFIGRET;

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
CMP_Init_Detection(IN DWORD dwMagic);

CMAPI
CONFIGRET
WINAPI
CMP_RegisterNotification(
  IN HANDLE hRecipient,
  IN LPVOID lpvNotificationFilter,
  IN DWORD dwFlags,
  OUT PULONG pluhDevNotify);

CMAPI
CONFIGRET
WINAPI
CMP_Report_LogOn(
  IN DWORD dwMagic,
  IN DWORD dwProcessId);

CMAPI
CONFIGRET
WINAPI
CMP_UnregisterNotification(IN ULONG luhDevNotify);

*/

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
  OUT PLOG_CONF plcLogConf,
  IN DEVINST dnDevInst,
  IN PRIORITY Priority,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf_Ex(
  OUT PLOG_CONF plcLogConf,
  IN DEVINST dnDevInst,
  IN PRIORITY Priority,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_IDA(
  IN DEVINST dnDevInst,
  IN PSTR pszID,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExA(
  IN DEVINST dnDevInst,
  IN PSTR pszID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_ID_ExW(
  IN DEVINST dnDevInst,
  IN PWSTR pszID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Add_IDW(
  IN DEVINST dnDevInst,
  IN PWSTR pszID,
  IN ULONG ulFlags);

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
  IN DWORDLONG ullStartValue,
  IN DWORDLONG ullEndValue,
  IN RANGE_LIST rlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des(
  OUT PRES_DES prdResDes OPTIONAL,
  IN LOG_CONF lcLogConf,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Add_Res_Des_Ex(
  OUT PRES_DES prdResDes OPTIONAL,
  IN LOG_CONF lcLogConf,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineA(
  IN PCSTR UNCServerName OPTIONAL,
  OUT PHMACHINE phMachine);

CMAPI
CONFIGRET
WINAPI
CM_Connect_MachineW(
  IN PCWSTR UNCServerName OPTIONAL,
  OUT PHMACHINE phMachine);

#ifdef UNICODE
#define CM_Connect_Machine CM_Connect_MachineW
#else
#define CM_Connect_Machine CM_Connect_MachineA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeA(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_A pDeviceID,
  IN DEVINST dnParent,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNodeW(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_W pDeviceID,
  IN DEVINST dnParent,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExA(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_A pDeviceID,
  IN DEVINST dnParent,
  IN ULONG ulFlags,
  IN HANDLE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Create_DevNode_ExW(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_W pDeviceID,
  IN DEVINST dnParent,
  IN ULONG ulFlags,
  IN HANDLE hMachine);

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
  OUT PRANGE_LIST prlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key(
  IN LPGUID ClassGuid,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Class_Key_Ex(
  IN LPGUID ClassGuid,
  IN ULONG ulFlags,
  IN HANDLE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key(
  IN DEVNODE dnDevNode,
  IN ULONG ulHardwareProfile,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_DevNode_Key_Ex(
  IN DEVNODE dnDevNode,
  IN ULONG ulHardwareProfile,
  IN ULONG ulFlags,
  IN HANDLE hMachine);

#define CM_Delete_DevInst_Key CM_Delete_DevNode_Key
#define CM_Delete_DevInst_Key_Ex CM_Delete_DevNode_Key_Ex

CMAPI
CONFIGRET
WINAPI
CM_Delete_Range(
  IN DWORDLONG ullStartValue,
  IN DWORDLONG ullEndValue,
  IN RANGE_LIST rlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict(
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  OUT PBOOL pbConflictDetected,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict_Ex(
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  OUT PBOOL pbConflictDetected,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Disable_DevNode_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

#define CM_Disable_DevInst CM_Disable_DevNode
#define CM_Disable_DevInst_Ex CM_Disable_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Disconnect_Machine(
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Dup_Range_List(
  IN RANGE_LIST rlhOld,
  IN RANGE_LIST rlhNew,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enable_DevNode_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

#define CM_Enable_DevInst CM_Enable_DevNode
#define CM_Enable_DevInst_Ex CM_Enable_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes(
  IN ULONG ulClassIndex,
  OUT LPGUID ClassGuid,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Classes_Ex(
  IN ULONG ulClassIndex,
  OUT LPGUID ClassGuid,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsA(
  IN ULONG ulEnumIndex,
  OUT PCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExA(
  IN ULONG ulEnumIndex,
  OUT PCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExW(
  IN ULONG ulEnumIndex,
  OUT PWCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsW(
  IN ULONG ulEnumIndex,
  OUT PWCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

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
  OUT PDWORDLONG pullStart,
  IN DWORDLONG ullStart,
  IN ULONG ulLength,
  IN DWORDLONG ullAlignment,
  IN DWORDLONG ullEnd,
  IN RANGE_LIST rlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_First_Range(
  IN RANGE_LIST rlh,
  OUT PDWORDLONG pullStart,
  OUT PDWORDLONG pullEnd,
  OUT PRANGE_ELEMENT preElement,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf(
  IN LOG_CONF lcLogConfToBeFreed,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Ex(
  IN LOG_CONF lcLogConfToBeFreed,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
  IN LOG_CONF lcLogConf);

CMAPI
CONFIGRET
WINAPI
CM_Free_Range_List(
  IN RANGE_LIST rlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Ex(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Free_Res_Des_Handle(
  IN RES_DES rdResDes);

CMAPI
CONFIGRET
WINAPI
CM_Free_Resource_Conflict_Handle(
  IN CONFLICT_LIST clConflictList);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child(
  OUT PDEVINST pdnDevInst,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Child_Ex(
  OUT PDEVINST pdnDevInst,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameA(
  IN LPGUID ClassGuid,
  OUT PCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_NameW(
  IN LPGUID ClassGuid,
  OUT PWCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExA(
  IN LPGUID ClassGuid,
  OUT PCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Name_ExW(
  IN LPGUID ClassGuid,
  OUT PWCHAR Buffer,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN LPGUID ClassGuid,
  OUT LPSTR pszKeyName,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_NameW(
  IN LPGUID ClassGuid,
  OUT LPWSTR pszKeyName,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExA(
  IN LPGUID ClassGuid,
  OUT LPSTR pszKeyName,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExW(
  IN LPGUID ClassGuid,
  OUT LPWSTR pszKeyName,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  OUT PULONG pulDepth,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Depth_Ex(
  OUT PULONG pulDepth,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDA(
  IN DEVINST dnDevInst,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExA(
  IN DEVINST dnDevInst,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
  IN DEVINST dnDevInst,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_IDW(
  IN DEVINST dnDevInst,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

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
  IN PCSTR pszFilter OPTIONAL,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExA(
  IN PCSTR pszFilter OPTIONAL,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExW(
  IN PCWSTR pszFilter OPTIONAL,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_ListW(
  IN PCWSTR pszFilter OPTIONAL,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

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
  OUT PULONG pulLen,
  IN PCSTR pszFilter OPTIONAL,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExA(
  OUT PULONG pulLen,
  IN PCSTR pszFilter OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExW(
  OUT PULONG pulLen,
  IN PCWSTR pszFilter OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeW(
  OUT PULONG pulLen,
  IN PCWSTR pszFilter OPTIONAL,
  IN ULONG ulFlags);

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
  OUT PULONG pulLen,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_ID_Size_Ex(
  OUT PULONG pulLen,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyA(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyW(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExA(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExW(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN DEVINST dnDevInst,
  IN PCSTR pszCustomPropertyName,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyW(
  IN DEVINST dnDevInst,
  IN PCWSTR pszCustomPropertyName,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExA(
  IN DEVINST dnDevInst,
  IN PCSTR pszCustomPropertyName,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExW(
  IN DEVINST dnDevInst,
  IN PCWSTR pszCustomPropertyName,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  OUT PULONG pulStatus,
  OUT PULONG pulProblemNumber,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
  OUT PULONG pulStatus,
  OUT PULONG pulProblemNumber,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

#define CM_Get_DevInst_Status CM_Get_DevNode_Status
#define CM_Get_DevInst_Status_Ex CM_Get_DevNode_Status_Ex

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
  OUT PLOG_CONF plcLogConf OPTIONAL,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_First_Log_Conf_Ex(
  OUT PLOG_CONF plcLogConf OPTIONAL,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State(
  OUT PULONG pulState,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Global_State_Ex(
  OUT PULONG pulState,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoA(
  IN ULONG ulIndex,
  OUT PHWPROFILEINFO_A pHWProfileInfo,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExA(
  IN ULONG ulIndex,
  OUT PHWPROFILEINFO_A pHWProfileInfo,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoW(
  IN ULONG ulIndex,
  OUT PHWPROFILEINFO_W pHWProfileInfo,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExW(
  IN ULONG ulIndex,
  OUT PHWPROFILEINFO_W pHWProfileInfo,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN DEVINSTID_A szDevInstName,
  IN ULONG ulHardwareProfile,
  OUT PULONG pulValue,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsW(
  IN DEVINSTID_W szDevInstName,
  IN ULONG ulHardwareProfile,
  OUT PULONG pulValue,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExA(
  IN DEVINSTID_A szDevInstName,
  IN ULONG ulHardwareProfile,
  OUT PULONG pulValue,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExW(
  IN DEVINSTID_W szDevInstName,
  IN ULONG ulHardwareProfile,
  OUT PULONG pulValue,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN LPCSTR pszDeviceInterface,
  IN LPGUID AliasInterfaceGuid,
  OUT LPSTR pszAliasDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasW(
  IN LPCWSTR pszDeviceInterface,
  IN LPGUID AliasInterfaceGuid,
  OUT LPWSTR pszAliasDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExA(
  IN LPCSTR pszDeviceInterface,
  IN LPGUID AliasInterfaceGuid,
  OUT LPSTR pszAliasDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExW(
  IN LPCWSTR pszDeviceInterface,
  IN LPGUID AliasInterfaceGuid,
  OUT LPWSTR pszAliasDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExA(
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  OUT PCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  OUT PWCHAR Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  OUT PULONG pulLen,
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
  OUT PULONG pulLen,
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExA(
  OUT PULONG pulLen,
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
  OUT PULONG pulLen,
  IN LPGUID InterfaceClassGuid,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN LOG_CONF lcLogConf,
  OUT PPRIORITY pPriority,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
  IN LOG_CONF lcLogConf,
  OUT PPRIORITY pPriority,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
  OUT PLOG_CONF plcLogConf OPTIONAL,
  IN LOG_CONF lcLogConf,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf_Ex(
  OUT PLOG_CONF plcLogConf OPTIONAL,
  IN LOG_CONF lcLogConf,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN RESOURCEID ForResource,
  OUT PRESOURCEID pResourceID,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Next_Res_Des_Ex(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN RESOURCEID ForResource,
  OUT PRESOURCEID pResourceID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent(
  OUT PDEVINST pdnDevInst,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
  OUT PDEVINST pdnDevInst,
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
  IN RES_DES rdResDes,
  OUT PVOID Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Ex(
  IN RES_DES rdResDes,
  OUT PVOID Buffer,
  IN ULONG BufferLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
  OUT PULONG pulSize,
  IN RES_DES rdResDes,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size_Ex(
  OUT PULONG pulSize,
  IN RES_DES rdResDes,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_Count(
  IN CONFLICT_LIST clConflictList,
  OUT PULONG pulCount);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsA(
  IN CONFLICT_LIST clConflictList,
  IN ULONG ulIndex,
  IN OUT PCONFLICT_DETAILS_A pConflictDetails);

CMAPI
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsW(
  IN CONFLICT_LIST clConflictList,
  IN ULONG ulIndex,
  IN OUT PCONFLICT_DETAILS_W pConflictDetails);

#ifdef UNICODE
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsW
#else
#define CM_Get_Resource_Conflict_Details CM_Get_Resource_Conflict_DetailsA
#endif /* UNICODE */

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyW(
  IN LPGUID ClassGuid,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyW(
  IN LPGUID ClassGuid,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN ULONG ulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyA(
  IN LPGUID ClassGuid,
  IN ULONG ulProperty,
  OUT PULONG pulRegDataType OPTIONAL,
  OUT PVOID Buffer OPTIONAL,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyA(
  IN LPGUID ClassGuid,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN  ULONG ulLength,
  IN  ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  OUT PDEVINST pdnDevInst,
  IN DEVINST DevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Get_Sibling_Ex(
  OUT PDEVINST pdnDevInst,
  IN DEVINST DevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
WORD
WINAPI
CM_Get_Version(VOID);

CMAPI
WORD
WINAPI
CM_Get_Version_Ex(
  IN HMACHINE hMachine);

#if (WINVER >= _WIN32_WINNT_WINXP)

CMAPI
BOOL
WINAPI
CM_Is_Version_Available(
  IN WORD wVersion);

CMAPI
BOOL
WINAPI
CM_Is_Version_Available_Ex(
  IN WORD wVersion,
  IN HMACHINE hMachine OPTIONAL);

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

CMAPI
CONFIGRET
WINAPI
CM_Intersect_Range_List(
  IN RANGE_LIST rlhOld1,
  IN RANGE_LIST rlhOld2,
  IN RANGE_LIST rlhNew,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Invert_Range_List(
  IN RANGE_LIST rlhOld,
  IN RANGE_LIST rlhNew,
  IN DWORDLONG ullMaxValue,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present(
  OUT PBOOL pbPresent);

CMAPI
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present_Ex(
  OUT PBOOL pbPresent,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeA(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExA(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_A pDeviceID OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNode_ExW(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
  OUT PDEVINST pdnDevInst,
  IN DEVINSTID_W pDeviceID OPTIONAL,
  IN ULONG ulFlags);

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
  IN RANGE_LIST rlhOld1,
  IN RANGE_LIST rlhOld2,
  IN RANGE_LIST rlhNew,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Modify_Res_Des_Ex(
  OUT PRES_DES prdResDes,
  IN RES_DES rdResDes,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode(
  IN DEVINST dnFromDevInst,
  IN DEVINST dnToDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Move_DevNode_Ex(
  IN DEVINST dnFromDevInst,
  IN DEVINST dnToDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

#define CM_Move_DevInst                CM_Move_DevNode
#define CM_Move_DevInst_Ex             CM_Move_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Next_Range(
  IN OUT PRANGE_ELEMENT preElement,
  OUT PDWORDLONG pullStart,
  OUT PDWORDLONG pullEnd,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyA(
  IN LPGUID ClassGuid OPTIONAL,
  IN LPCSTR pszClassName OPTIONAL,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkClass,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_KeyW(
  IN LPGUID ClassGuid OPTIONAL,
  IN LPCWSTR pszClassName OPTIONAL,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkClass,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExA(
  IN LPGUID pszClassGuid OPTIONAL,
  IN LPCSTR pszClassName OPTIONAL,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkClass,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Open_Class_Key_ExW(
  IN LPGUID pszClassGuid OPTIONAL,
  IN LPCWSTR pszClassName OPTIONAL,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkClass,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN DEVINST dnDevNode,
  IN REGSAM samDesired,
  IN ULONG ulHardwareProfile,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDevice,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_DevNode_Key_Ex(
  IN DEVINST dnDevNode,
  IN REGSAM samDesired,
  IN ULONG ulHardwareProfile,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDevice,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

#define CM_Open_DevInst_Key CM_Open_DevNode_Key
#define CM_Open_DevInst_Key_Ex CM_Open_DevNode_Key_Ex

#if (WINVER >= _WIN32_WINNT_VISTA)

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_KeyA(
  IN LPCSTR pszDeviceInterface,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_KeyW(
  IN LPCWSTR pszDeviceInterface,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_Key_ExA(
  IN LPCSTR pszDeviceInterface,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Open_Device_Interface_Key_ExW(
  IN LPCWSTR pszDeviceInterface,
  IN REGSAM samDesired,
  IN REGDISPOSITION Disposition,
  OUT PHKEY phkDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN LPCSTR pszDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_KeyW(
  IN LPCWSTR pszDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_Key_ExA(
  IN LPCSTR pszDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Delete_Device_Interface_Key_ExW(
  IN LPCWSTR pszDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  OUT PVOID pData,
  IN ULONG DataLen,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data_Ex(
  OUT PVOID pData,
  IN ULONG DataLen,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size(
  OUT PULONG pulSize,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size_Ex(
  OUT PULONG pulSize,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree(
  IN DEVINST dnAncestor,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_Remove_SubTree_Ex(
  IN DEVINST dnAncestor,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeA(
  IN DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeW(
  IN DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExA(
  IN DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExW(
  IN DEVINST dnAncestor,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  OUT PCONFLICT_LIST pclConflictList,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN PCVOID ResourceData,
  IN ULONG ResourceLen,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Reenumerate_DevNode_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

#define CM_Reenumerate_DevInst CM_Reenumerate_DevNode
#define CM_Reenumerate_DevInst_Ex CM_Reenumerate_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceA(
  IN DEVINST dnDevInst,
  IN LPGUID InterfaceClassGuid,
  IN LPCSTR pszReference OPTIONAL,
  OUT LPSTR pszDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_InterfaceW(
  IN DEVINST dnDevInst,
  IN LPGUID InterfaceClassGuid,
  IN LPCWSTR pszReference OPTIONAL,
  OUT LPWSTR pszDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExA(
  IN DEVINST dnDevInst,
  IN LPGUID InterfaceClassGuid,
  IN LPCSTR pszReference OPTIONAL,
  OUT LPSTR pszDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExW(
  IN DEVINST dnDevInst,
  IN LPGUID InterfaceClassGuid,
  IN LPCWSTR pszReference OPTIONAL,
  OUT LPWSTR pszDeviceInterface,
  IN OUT PULONG pulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN DEVINST dnDevInst,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExW(
  IN DEVINST dnDevInst,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExA(
  IN DEVINST dnDevInst,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Request_Device_EjectW(
  IN DEVINST dnDevInst,
  OUT PPNP_VETO_TYPE pVetoType,
  OUT LPWSTR pszVetoName,
  IN ULONG ulNameLength,
  IN ULONG ulFlags);

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
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Run_Detection(
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Run_Detection_Ex(
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

#if (WINVER >= _WIN32_WINNT_VISTA)

CONFIGRET
CM_Apply_PowerScheme(VOID);

CONFIGRET
CM_Write_UserPowerKey(
  IN CONST GUID *SchemeGuid OPTIONAL,
  IN CONST GUID *SubGroupOfPowerSettingsGuid OPTIONAL,
  IN CONST GUID *PowerSettingGuid OPTIONAL,
  IN ULONG AccessFlags,
  IN ULONG Type,
  IN UCHAR *Buffer,
  IN DWORD BufferSize,
  OUT PDWORD Error);

CONFIGRET
CM_Set_ActiveScheme(
  IN CONST GUID *SchemeGuid,
  OUT PDWORD Error);

CONFIGRET
CM_Restore_DefaultPowerScheme(
  IN CONST GUID *SchemeGuid,
  OUT PDWORD Error);

CONFIGRET
CM_RestoreAll_DefaultPowerSchemes(
  OUT PDWORD Error);

CONFIGRET
CM_Duplicate_PowerScheme(
  IN CONST GUID *SourceSchemeGuid,
  IN GUID **DestinationSchemeGuid,
  OUT PDWORD Error);

CONFIGRET
CM_Delete_PowerScheme(
  IN CONST GUID *SchemeGuid,
  OUT PDWORD Error);

CONFIGRET
CM_Import_PowerScheme(
  IN LPCWSTR ImportFileNamePath,
  IN OUT GUID **DestinationSchemeGuid,
  OUT PDWORD Error);

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem(
  IN DEVINST dnDevInst,
  IN ULONG ulProblem,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Problem_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulProblem,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

#define CM_Set_DevInst_Problem CM_Set_DevNode_Problem
#define CM_Set_DevInst_Problem_Ex CM_Set_DevNode_Problem_Ex

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceA(
  IN LPCSTR pszDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceW(
  IN LPCWSTR pszDeviceInterface,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExA(
  IN LPCSTR pszDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExW(
  IN LPCWSTR pszDeviceInterface,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

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
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Register_Device_Driver_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree(
  IN DEVINST dnAncestor,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Remove_SubTree_Ex(
  IN DEVINST dnAncestor,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyA(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN ULONG ulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyW(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN ULONG ulLength,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExA(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN ULONG ulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExW(
  IN DEVINST dnDevInst,
  IN ULONG ulProperty,
  IN PCVOID Buffer OPTIONAL,
  IN ULONG ulLength,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN ULONG ulHardwareProfile,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Ex(
  IN ULONG ulHardwareProfile,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsA(
  IN DEVINSTID_A szDevInstName,
  IN ULONG ulConfig,
  IN ULONG ulValue,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsW(
  IN DEVINSTID_W szDevInstName,
  IN ULONG ulConfig,
  IN ULONG ulValue,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExA(
  IN DEVINSTID_A szDevInstName,
  IN ULONG ulConfig,
  IN ULONG ulValue,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

CMAPI
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExW(
  IN DEVINSTID_W szDevInstName,
  IN ULONG ulConfig,
  IN ULONG ulValue,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

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
  IN DEVINST dnDevInst,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Setup_DevNode_Ex(
  IN DEVINST dnDevInst,
  IN ULONG ulFlags,
  IN HMACHINE hMachine OPTIONAL);

#define CM_Setup_DevInst         CM_Setup_DevNode
#define CM_Setup_DevInst_Ex      CM_Setup_DevNode_Ex

CMAPI
CONFIGRET
WINAPI
CM_Test_Range_Available(
  IN DWORDLONG ullStartValue,
  IN DWORDLONG ullEndValue,
  IN RANGE_LIST rlh,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode(
  IN DEVINST dnPhantom,
  IN ULONG ulFlags);

CMAPI
CONFIGRET
WINAPI
CM_Uninstall_DevNode_Ex(
  IN DEVINST dnPhantom,
  IN ULONG ulFlags,
  IN HMACHINE hMachine);

#define CM_Uninstall_DevInst     CM_Uninstall_DevNode
#define CM_Uninstall_DevInst_Ex  CM_Uninstall_DevNode_Ex


#if (WINVER >= _WIN32_WINNT_WIN2K)

#define CM_WaitNoPendingInstallEvents CMP_WaitNoPendingInstallEvents

CMAPI
DWORD
WINAPI
CMP_WaitNoPendingInstallEvents(
  IN DWORD dwTimeout);

#endif /* (WINVER >= _WIN32_WINNT_WIN2K) */

#ifdef __cplusplus
}
#endif
