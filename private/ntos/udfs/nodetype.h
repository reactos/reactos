/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code.  This code is the first CSHORT in the structure and is followed
    by a CSHORT containing the size, in bytes, of the structure.

    A single structure can fake polymorphism by using a set of node type codes.
    This is what the two FCB types do.

Author:

    Dan Lovinger    [DanLo]   20-May-1996

Revision History:

--*/

#ifndef _UDFNODETYPE_
#define _UDFNODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

#define UDFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0901)
#define UDFS_NTC_VCB                    ((NODE_TYPE_CODE)0x0902)
#define UDFS_NTC_FCB_INDEX              ((NODE_TYPE_CODE)0x0903)
#define UDFS_NTC_FCB_DATA               ((NODE_TYPE_CODE)0x0904)
#define UDFS_NTC_FCB_NONPAGED           ((NODE_TYPE_CODE)0x0905)
#define UDFS_NTC_CCB                    ((NODE_TYPE_CODE)0x0906)
#define UDFS_NTC_IRP_CONTEXT            ((NODE_TYPE_CODE)0x0907)
#define UDFS_NTC_IRP_CONTEXT_LITE       ((NODE_TYPE_CODE)0x0908)
#define UDFS_NTC_LCB                    ((NODE_TYPE_CODE)0x0909)
#define UDFS_NTC_PCB                    ((NODE_TYPE_CODE)0x090a)

typedef CSHORT NODE_BYTE_SIZE;

//
//  So all records start with
//
//  typedef struct _RECORD_NAME {
//      NODE_TYPE_CODE NodeTypeCode;
//      NODE_BYTE_SIZE NodeByteSize;
//          :
//  } RECORD_NAME;
//  typedef RECORD_NAME *PRECORD_NAME;
//

#ifndef NodeType
#define NodeType(P) ((P) != NULL ? (*((PNODE_TYPE_CODE)(P))) : NTC_UNDEFINED)
#endif
#ifndef SafeNodeType
#define SafeNodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))
#endif

//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  UDFS_BUG_CHECK_ values defined below and then use UdfBugCheck to bugcheck
//  the system.
//
//  We also will define the debug trace level masks here.  Set UdfsDebugTraceLevel
//  to include a given filemask to see debug information from that module when
//  compiled with debugging.
//

//
//  Not all of these are actually used in UDFS. Perhaps this list will be
//  optimized when UDFS is functionally complete.
//

#define UDFS_BUG_CHECK_ALLOCSUP          (0x00010000)
#define UDFS_BUG_CHECK_CACHESUP          (0x00020000)
#define UDFS_BUG_CHECK_CLEANUP           (0x00030000)
#define UDFS_BUG_CHECK_CLOSE             (0x00040000)
#define UDFS_BUG_CHECK_CREATE            (0x00050000)
#define UDFS_BUG_CHECK_DEVCTRL           (0x00060000)
#define UDFS_BUG_CHECK_DEVIOSUP          (0x00070000)
#define UDFS_BUG_CHECK_DIRCTRL           (0x00080000)
#define UDFS_BUG_CHECK_DIRSUP            (0x00090000)
#define UDFS_BUG_CHECK_FILEINFO          (0x000a0000)
#define UDFS_BUG_CHECK_FILOBSUP          (0x000b0000)
#define UDFS_BUG_CHECK_FSCTRL            (0x000c0000)
#define UDFS_BUG_CHECK_FSPDISP           (0x000d0000)
#define UDFS_BUG_CHECK_LOCKCTRL          (0x000e0000)
#define UDFS_BUG_CHECK_NAMESUP           (0x000f0000)
#define UDFS_BUG_CHECK_PREFXSUP          (0x00100000)
#define UDFS_BUG_CHECK_READ              (0x00110000)
#define UDFS_BUG_CHECK_RESRCSUP          (0x00120000)
#define UDFS_BUG_CHECK_STRUCSUP          (0x00130000)
#define UDFS_BUG_CHECK_UDFDATA           (0x00140000)
#define UDFS_BUG_CHECK_UDFINIT           (0x00150000)
#define UDFS_BUG_CHECK_VERFYSUP          (0x00160000)
#define UDFS_BUG_CHECK_VMCBSUP           (0x00170000)
#define UDFS_BUG_CHECK_VOLINFO           (0x00180000)
#define UDFS_BUG_CHECK_WORKQUE           (0x00190000)
#define UDFS_BUG_CHECK_COMMON            (0x001a0000)

#define UDFS_DEBUG_LEVEL_ALLOCSUP        (0x00000001)
#define UDFS_DEBUG_LEVEL_CACHESUP        (0x00000002)
#define UDFS_DEBUG_LEVEL_CLEANUP         (0x00000004)
#define UDFS_DEBUG_LEVEL_CLOSE           (0x00000008)
#define UDFS_DEBUG_LEVEL_CREATE          (0x00000010)
#define UDFS_DEBUG_LEVEL_DEVCTRL         (0x00000020)
#define UDFS_DEBUG_LEVEL_DEVIOSUP        (0x00000040)
#define UDFS_DEBUG_LEVEL_DIRCTRL         (0x00000080)
#define UDFS_DEBUG_LEVEL_DIRSUP          (0x00000100)
#define UDFS_DEBUG_LEVEL_FILEINFO        (0x00000200)
#define UDFS_DEBUG_LEVEL_FILOBSUP        (0x00000400)
#define UDFS_DEBUG_LEVEL_FSCTRL          (0x00000800)
#define UDFS_DEBUG_LEVEL_FSPDISP         (0x00001000)
#define UDFS_DEBUG_LEVEL_LOCKCTRL        (0x00002000)
#define UDFS_DEBUG_LEVEL_NAMESUP         (0x00004000)
#define UDFS_DEBUG_LEVEL_PREFXSUP        (0x00008000)
#define UDFS_DEBUG_LEVEL_READ            (0x00010000)
#define UDFS_DEBUG_LEVEL_RESRCSUP        (0x00020000)
#define UDFS_DEBUG_LEVEL_STRUCSUP        (0x00040000)
#define UDFS_DEBUG_LEVEL_UDFDATA         (0x00080000)
#define UDFS_DEBUG_LEVEL_UDFINIT         (0x00100000)
#define UDFS_DEBUG_LEVEL_VERFYSUP        (0x00200000)
#define UDFS_DEBUG_LEVEL_VMCBSUP         (0x00400000)
#define UDFS_DEBUG_LEVEL_VOLINFO         (0x00800000)
#define UDFS_DEBUG_LEVEL_WORKQUE         (0x01000000)
#define UDFS_DEBUG_LEVEL_COMMON          (0x02000000)

//
//  Use UNWIND for reports from exception handlers.
//

#define UDFS_DEBUG_LEVEL_UNWIND          (0x80000000)

#define UdfBugCheck(A,B,C) { KeBugCheckEx(UDFS_FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }

#ifndef BUILDING_FSKDEXT

//
//  The following are the pool tags for UDFS memory allocations
//

#define TAG_CCB                         'xfdU'
#define TAG_CDROM_TOC                   'tfdU'
#define TAG_CRC_TABLE                   'CfdU'
#define TAG_ENUM_EXPRESSION             'efdU'
#define TAG_FCB_DATA                    'dfdU'
#define TAG_FCB_INDEX                   'ifdU'
#define TAG_FCB_NONPAGED                'FfdU'
#define TAG_FID_BUFFER                  'DfdU'
#define TAG_FILE_NAME                   'ffdU'
#define TAG_GENERIC_TABLE               'TfdU'
#define TAG_IO_BUFFER                   'bfdU'
#define TAG_IO_CONTEXT                  'IfdU'
#define TAG_IRP_CONTEXT                 'cfdU'
#define TAG_IRP_CONTEXT_LITE            'LfdU'
#define TAG_LCB                         'lfdU'
#define TAG_PCB                         'pfdU'
#define TAG_SHORT_FILE_NAME             'SfdU'
#define TAG_VPB                         'vfdU'
#define TAG_SPARING_MCB                 'sfdU'

#define TAG_NSR_FSD                     '1fdU'
#define TAG_NSR_VSD                     '2fdU'
#define TAG_NSR_VDSD                    '3fdU'

#endif

#endif // _UDFNODETYPE_
