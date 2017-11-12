/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code that is.  This code is the first CSHORT in the structure and is
    followed by a CSHORT containing the size, in bytes, of the structure.


--*/

#ifndef _CDNODETYPE_
#define _CDNODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

#define CDFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0301)
#define CDFS_NTC_VCB                    ((NODE_TYPE_CODE)0x0302)
#define CDFS_NTC_FCB_PATH_TABLE         ((NODE_TYPE_CODE)0x0303)
#define CDFS_NTC_FCB_INDEX              ((NODE_TYPE_CODE)0x0304)
#define CDFS_NTC_FCB_DATA               ((NODE_TYPE_CODE)0x0305)
#define CDFS_NTC_FCB_NONPAGED           ((NODE_TYPE_CODE)0x0306)
#define CDFS_NTC_CCB                    ((NODE_TYPE_CODE)0x0307)
#define CDFS_NTC_IRP_CONTEXT            ((NODE_TYPE_CODE)0x0308)
#define CDFS_NTC_IRP_CONTEXT_LITE       ((NODE_TYPE_CODE)0x0309)

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
//  CDFS_BUG_CHECK_ values defined below and then use CdBugCheck to bugcheck
//  the system.
//

#define CDFS_BUG_CHECK_ACCHKSUP          (0x00010000)
#define CDFS_BUG_CHECK_ALLOCSUP          (0x00020000)
#define CDFS_BUG_CHECK_CACHESUP          (0x00030000)
#define CDFS_BUG_CHECK_CDDATA            (0x00040000)
#define CDFS_BUG_CHECK_CDINIT            (0x00050000)
#define CDFS_BUG_CHECK_CLEANUP           (0x00060000)
#define CDFS_BUG_CHECK_CLOSE             (0x00070000)
#define CDFS_BUG_CHECK_CREATE            (0x00080000)
#define CDFS_BUG_CHECK_DEVCTRL           (0x00090000)
#define CDFS_BUG_CHECK_DEVIOSUP          (0x000a0000)
#define CDFS_BUG_CHECK_DIRCTRL           (0x000b0000)
#define CDFS_BUG_CHECK_DIRSUP            (0x000c0000)
#define CDFS_BUG_CHECK_FILEINFO          (0x000d0000)
#define CDFS_BUG_CHECK_FILOBSUP          (0x000e0000)
#define CDFS_BUG_CHECK_FSCTRL            (0x000f0000)
#define CDFS_BUG_CHECK_FSPDISP           (0x00100000)
#define CDFS_BUG_CHECK_LOCKCTRL          (0x00110000)
#define CDFS_BUG_CHECK_NAMESUP           (0x00120000)
#define CDFS_BUG_CHECK_PATHSUP           (0x00130000)
#define CDFS_BUG_CHECK_PNP               (0x00140000)
#define CDFS_BUG_CHECK_PREFXSUP          (0x00150000)
#define CDFS_BUG_CHECK_READ              (0x00160000)
#define CDFS_BUG_CHECK_RESRCSUP          (0x00170000)
#define CDFS_BUG_CHECK_STRUCSUP          (0x00180000)
#define CDFS_BUG_CHECK_TIMESUP           (0x00190000)
#define CDFS_BUG_CHECK_VERFYSUP          (0x001a0000)
#define CDFS_BUG_CHECK_VOLINFO           (0x001b0000)
#define CDFS_BUG_CHECK_WORKQUE           (0x001c0000)

#define CdBugCheck(A,B,C) { KeBugCheckEx(CDFS_FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }

#endif // _NODETYPE_

