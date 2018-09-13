/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code that is.  This code is the first CSHORT in the structure and is
    followed by a CSHORT containing the size, in bytes, of the structure.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#ifndef _NODETYPE_
#define _NODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                       ((NODE_TYPE_CODE)0x0000)

#define LFS_NTC_LCB                         ((NODE_TYPE_CODE)0x800)
#define LFS_NTC_LCH                         ((NODE_TYPE_CODE)0x801)
#define LFS_NTC_LBCB                        ((NODE_TYPE_CODE)0x802)
#define LFS_NTC_LFCB                        ((NODE_TYPE_CODE)0x803)
#define LFS_NTC_DATA                        ((NODE_TYPE_CODE)0x804)

typedef CSHORT NODE_BYTE_SIZE;

//
//  So all records start with
//
//  typedef struct _RECORD_NAME {
//      NODE_TYPE_CODE NodeTypeCode;
//      NODE_BYTE_SIZE NodeByteSize;
//
//  } RECORD_NAME;
//  typedef RECORD_NAME *PRECORD_NAME;
//

#define NodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))

#endif // _NODETYPE_

