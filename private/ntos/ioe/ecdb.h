/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ioep.h

Abstract:

    This module contains the definitions for Error Case Database.

Author:
    Michael Tsang (MikeTs) 16-Oct-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _ECDB_H
#define _ECBD_H

//
// Type Definitions
//
typedef struct _errcasedb {
    ULONG Signature;                    //signature of error case database
    ULONG Version;                      //database file format version
    ULONG Checksum;                     //checksum of error case database
    ULONG Length;                       //length of the database file
    ULONG ErrCaseOffset;                //file offset to first error case
    ULONG NumErrCases;                  //number of error cases in database
    ULONG ErrIDPathBlkOffset;           //file offset to ErrIDPathBlk
    ULONG DataBlkOffset;                //file offset to DataBlk
} ERRCASEDB, *PERRCASEDB;

typedef struct _errcase {
    ULONG ErrCaseID;                    //error case ID
    ULONG ErrIDPathOffset;              //offset to ErrIDPath in ErrIDPathBlk
    ULONG NumErrIDs;                    //number of error IDs in ErrIDPath
    ULONG MethodOffset;                 //offset to case methods in DataBlk
    ULONG NumMethods;                   //number of case methods
} ERRCASE, *PERRCASE;

typedef struct _method {
    ULONG MethodType;                   //method type
    ULONG MethodDataOffset;             //offset to method data in DataBlk
} METHOD, *PMETHOD;

typedef struct _msgarg {
    ULONG ErrIDIndex;                   //associated error ID index
    ULONG ArgType;                      //argument type
    ULONG ArgDataOffset;                //offset to argument data in DataBlk
} MSGARG, *PMSGARG;

typedef struct _msgdata {
    ULONG  MsgTemplateOffset;           //offset to message template in DataBlk
    ULONG  NumArgs;                     //number of arguments in message
    MSGARG Args[1];                     //array of message arguments
} MSGDATA, *PMSGDATA;

typedef struct _handlerdata {
    GUID  ComponentGuid;                //handler module GUID
    ULONG HandlerIndex;                 //handler index of the module
    ULONG Param;                        //optional parameter
} HANDLERDATA, *PHANDLERDATA;

#endif  //ifndef _ECDB_H
