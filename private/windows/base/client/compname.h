/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    COMPNAME.H

Abstract:

    Contains the common data structures for the Get/SetComputerName API

Author:

    Dan Hinsley  (DanHi)   16-Apr-1992

Revision History:

--*/

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\Current_Control_Set\\Services\\ComputerName"

#define NON_VOLATILE_COMPUTERNAME_NODE \
    L"\\Registry\\Machine\\System\\Current_Control_Set\\Services\\ComputerName\\ComputerName"

#define VOLATILE_COMPUTERNAME L"ActiveComputerName"
#define NON_VOLATILE_COMPUTERNAME L"ComputerName"
#define COMPUTERNAME_VALUE_NAME L"ComputerName"
#define CLASS_STRING L"Network ComputerName"


