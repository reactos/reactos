/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    ntcab.h

Abstract:

    Private header file for ntcab compression support.
    
Author:

    Andrew Ritz (andrewr) 5-Oct-1998

Revision History:

    Andrew Ritz (andrewr) 5-Oct-1998 Created it.

--*/

typedef struct _NTCABCONTEXT {
  PVOID     hCab;
  PVOID     UserContext;
  PVOID     MsgHandler;
  PCWSTR    CabFile;
  PWSTR     FilePart;
  PWSTR     PathPart;
  BOOL      IsMsgHandlerNativeCharWidth;
  DWORD     LastError;
  PWSTR     CurrentTargetFile;
  //WCHAR   UserPath[MAX_PATH];
  //BOOL    SwitchedCabinets
  

} NTCABCONTEXT, *PNTCABCONTEXT;

BOOL
NtCabIsCabinet(
    PCWSTR CabinetName
    );


DWORD
NtCabProcessCabinet(
    //IN PVOID  InCabHandle, OPTIONAL
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsMsgHandlerNativeCharWidth
    );

typedef UINT (CALLBACK* PSP_NTCAB_CALLBACK)(
    IN PNTCAB_ENUM_DATA EnumData,
    IN PNTCABCONTEXT    Context,
    OUT PDWORD          Operation
    );

