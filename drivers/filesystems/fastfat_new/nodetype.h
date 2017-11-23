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

#ifndef _FATNODETYPE_
#define _FATNODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                    ((NODE_TYPE_CODE)0x0000)

#define FAT_NTC_DATA_HEADER              ((NODE_TYPE_CODE)0x0500)
#define FAT_NTC_VCB                      ((NODE_TYPE_CODE)0x0501)
#define FAT_NTC_FCB                      ((NODE_TYPE_CODE)0x0502)
#define FAT_NTC_DCB                      ((NODE_TYPE_CODE)0x0503)
#define FAT_NTC_ROOT_DCB                 ((NODE_TYPE_CODE)0x0504)
#define FAT_NTC_CCB                      ((NODE_TYPE_CODE)0x0507)
#define FAT_NTC_IRP_CONTEXT              ((NODE_TYPE_CODE)0x0508)

typedef CSHORT NODE_BYTE_SIZE;

#ifndef NodeType

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

#define NodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))
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
//  FAT_BUG_CHECK_ values defined below and then use FatBugCheck to bugcheck
//  the system.
//


#define FAT_BUG_CHECK_ACCHKSUP           (0x00010000)
#define FAT_BUG_CHECK_ALLOCSUP           (0x00020000)
#define FAT_BUG_CHECK_CACHESUP           (0x00030000)
#define FAT_BUG_CHECK_CLEANUP            (0x00040000)
#define FAT_BUG_CHECK_CLOSE              (0x00050000)
#define FAT_BUG_CHECK_CREATE             (0x00060000)
#define FAT_BUG_CHECK_DEVCTRL            (0x00070000)
#define FAT_BUG_CHECK_DEVIOSUP           (0x00080000)
#define FAT_BUG_CHECK_DIRCTRL            (0x00090000)
#define FAT_BUG_CHECK_DIRSUP             (0x000a0000)
#define FAT_BUG_CHECK_DUMPSUP            (0x000b0000)
#define FAT_BUG_CHECK_EA                 (0x000c0000)
#define FAT_BUG_CHECK_EASUP              (0x000d0000)
#define FAT_BUG_CHECK_FATDATA            (0x000e0000)
#define FAT_BUG_CHECK_FATINIT            (0x000f0000)
#define FAT_BUG_CHECK_FILEINFO           (0x00100000)
#define FAT_BUG_CHECK_FILOBSUP           (0x00110000)
#define FAT_BUG_CHECK_FLUSH              (0x00120000)
#define FAT_BUG_CHECK_FSCTRL             (0x00130000)
#define FAT_BUG_CHECK_FSPDISP            (0x00140000)
#define FAT_BUG_CHECK_LOCKCTRL           (0x00150000)
#define FAT_BUG_CHECK_NAMESUP            (0x00160000)
#define FAT_BUG_CHECK_PNP                (0x00170000)
#define FAT_BUG_CHECK_READ               (0x00180000)
#define FAT_BUG_CHECK_RESRCSUP           (0x00190000)
#define FAT_BUG_CHECK_SHUTDOWN           (0x001a0000)
#define FAT_BUG_CHECK_SPLAYSUP           (0x001b0000)
#define FAT_BUG_CHECK_STRUCSUP           (0x001c0000)
#define FAT_BUG_CHECK_TIMESUP            (0x001d0000)
#define FAT_BUG_CHECK_VERFYSUP           (0x001e0000)
#define FAT_BUG_CHECK_VOLINFO            (0x001f0000)
#define FAT_BUG_CHECK_WORKQUE            (0x00200000)
#define FAT_BUG_CHECK_WRITE              (0x00210000)

#define FatBugCheck(A,B,C) { KeBugCheckEx(FAT_FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }


//
//  In this module we'll also define some globally known constants
//

#define UCHAR_NUL                        0x00
#define UCHAR_SOH                        0x01
#define UCHAR_STX                        0x02
#define UCHAR_ETX                        0x03
#define UCHAR_EOT                        0x04
#define UCHAR_ENQ                        0x05
#define UCHAR_ACK                        0x06
#define UCHAR_BEL                        0x07
#define UCHAR_BS                         0x08
#define UCHAR_HT                         0x09
#define UCHAR_LF                         0x0a
#define UCHAR_VT                         0x0b
#define UCHAR_FF                         0x0c
#define UCHAR_CR                         0x0d
#define UCHAR_SO                         0x0e
#define UCHAR_SI                         0x0f
#define UCHAR_DLE                        0x10
#define UCHAR_DC1                        0x11
#define UCHAR_DC2                        0x12
#define UCHAR_DC3                        0x13
#define UCHAR_DC4                        0x14
#define UCHAR_NAK                        0x15
#define UCHAR_SYN                        0x16
#define UCHAR_ETB                        0x17
#define UCHAR_CAN                        0x18
#define UCHAR_EM                         0x19
#define UCHAR_SUB                        0x1a
#define UCHAR_ESC                        0x1b
#define UCHAR_FS                         0x1c
#define UCHAR_GS                         0x1d
#define UCHAR_RS                         0x1e
#define UCHAR_US                         0x1f
#define UCHAR_SP                         0x20

#ifndef BUILDING_FSKDEXT

//
//  These are the tags we use to uniquely identify pool allocations.
//

#define TAG_CCB                         'CtaF'
#define TAG_FCB                         'FtaF'
#define TAG_FCB_NONPAGED                'NtaF'
#define TAG_ERESOURCE                   'EtaF'
#define TAG_IRP_CONTEXT                 'ItaF'

#define TAG_BCB                         'btaF'
#define TAG_DIRENT                      'DtaF'
#define TAG_DIRENT_BITMAP               'TtaF'
#define TAG_EA_DATA                     'dtaF'
#define TAG_EA_SET_HEADER               'etaF'
#define TAG_EVENT                       'vtaF'
#define TAG_FAT_BITMAP                  'BtaF'
#define TAG_FAT_CLOSE_CONTEXT           'xtaF'
#define TAG_FAT_IO_CONTEXT              'XtaF'
#define TAG_FAT_WINDOW                  'WtaF'
#define TAG_FILENAME_BUFFER             'ntaF'
#define TAG_IO_RUNS                     'itaF'
#define TAG_REPINNED_BCB                'RtaF'
#define TAG_STASHED_BPB                 'StaF'
#define TAG_VCB_STATS                   'VtaF'
#define TAG_DEFERRED_FLUSH_CONTEXT      'ftaF'

#define TAG_VPB                         'vtaF'

#define TAG_VERIFY_BOOTSECTOR           'staF'
#define TAG_VERIFY_ROOTDIR              'rtaF'

#define TAG_OUTPUT_MAPPINGPAIRS         'PtaF'

#define TAG_ENTRY_LOOKUP_BUFFER         'LtaF'

#define TAG_IO_BUFFER                   'OtaF'
#define TAG_IO_USER_BUFFER              'QtaF'

#define TAG_DYNAMIC_NAME_BUFFER         'ctaF'

#define TAG_DEFRAG_BUFFER               'GtaF'

#endif

#endif // _NODETYPE_


