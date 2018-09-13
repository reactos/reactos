/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    canonapi.c

Abstract:

    This file contains the remotable API wrappers for the canonicalization
    functions. Now that remotable canonicalization has been moved into the
    server service, these canonicalization routines (in NETAPI.DLL) simply
    decide whether a function should be remoted or runs the local routine

    The canonicalization functions have been split into these wrappers, the
    local versions and the remote RPC routines to avoid the cylical dependency
    of SRVSVC.DLL/.LIB and NETAPI.DLL/.LIB

    Contents:
        NetpListCanonicalize
        NetpListTraverse
        NetpNameCanonicalize
        NetpNameCompare
        NetpNameValidate
        NetpPathCanonicalize
        NetpPathCompare
        NetpPathType

Author:

    Richard L Firth (rfirth) 15-May-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <tstring.h>
#include <icanon.h>
#include <netcan.h>


NET_API_STATUS
NET_API_FUNCTION
NetpListCanonicalize(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  List,
    IN  LPTSTR  Delimiters OPTIONAL,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Converts a list to its canonical form. If ServerName is non-NULL then the
    RPC function is called (in SRVSVC.DLL) else the local worker function (in
    NETLIB.LIB)

Arguments:

    ServerName      - where to remote this function. May be NULL
    List            - input list to canonicalize
    Delimiters      - optional list of delimiter characters. May be NULL
    Outbuf          - place to write output
    OutbufLen       - length of Outbuf
    OutCount        - returned number of items in Outbuf
    PathTypes       - returned list of types of entries in Outbuf
    PathTypesLen    - size of PathTypes array
    Flags           - control flags

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;
    BOOL nullDelimiter = FALSE;
    TCHAR ch;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(Delimiters)) {
            val = STRLEN(Delimiters);
            nullDelimiter = (val == 0);
        } else {
            nullDelimiter = TRUE;
        }
        val = STRLEN(List);

        //
        // if Delimiters is a NULL pointer or NUL string, then List is a
        // NULL-NULL input list
        //

        if (nullDelimiter) {
            LPTSTR str = List + val + 1;

            do {
                val = STRLEN(str);
                str += val + 1;
            } while ( val );
        }
        ch = (volatile TCHAR)*Outbuf;
        *Outbuf = ch;
        ch = (volatile TCHAR)*(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf));
        *(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf)) = ch;
        *OutCount = 0;
        if (ARGUMENT_PRESENT(PathTypes)) {
            PathTypes[0] = 0;
            PathTypes[PathTypesLen - 1] = 0;
        } else if ((Flags & INLC_FLAGS_MASK_NAMETYPE) == NAMETYPE_PATH) {

            //
            // NAMETYPE_PATH and NULL PathTypes is illegal
            //

            status = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INLC_FLAGS_MASK_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }

    //
    // due to historic precedent, we don't remote this function
    //

    if (location == ISREMOTE) {
        return ERROR_NOT_SUPPORTED;
    } else {
        return NetpwListCanonicalize(List,
                                        Delimiters,
                                        Outbuf,
                                        OutbufLen,
                                        OutCount,
                                        PathTypes,
                                        PathTypesLen,
                                        Flags
                                        );
    }
}


LPTSTR
NET_API_FUNCTION
NetpListTraverse(
    IN  LPTSTR  Reserved OPTIONAL,
    IN  LPTSTR* pList,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    This just calls the local traverse function

Arguments:

    Reserved    - MBZ
    pList       - pointer to list to traverse
    Flags       - MBZ

Return Value:

    LPTSTR

--*/

{
    return NetpwListTraverse(Reserved, pList, Flags);
}


NET_API_STATUS
NET_API_FUNCTION
NetpNameCanonicalize(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Canonicalizes a name

Arguments:

    ServerName  - where to run this API
    Name        - name to canonicalize
    Outbuf      - where to put canonicalized name
    OutbufLen   - length of Outbuf
    NameType    - type of name to canonicalize
    Flags       - control flags

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;
    TCHAR ch;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(Name)) {
            val = STRLEN(Name);
        }
        if (ARGUMENT_PRESENT(Outbuf)) {
            ch = (volatile TCHAR)*Outbuf;
            *Outbuf = ch;
            ch = (volatile TCHAR)*(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf));
            *(Outbuf + OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf)) = ch;
        } else {
            status = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INNCA_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsNameCanonicalize(serverName,
                                        Name,
                                        Outbuf,
                                        OutbufLen,
                                        NameType,
                                        Flags
                                        );
    } else {
        return NetpwNameCanonicalize(Name, Outbuf, OutbufLen, NameType, Flags);
    }
}


LONG
NET_API_FUNCTION
NetpNameCompare(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Compares two names. Must be of same type

Arguments:

    ServerName  - where to run this API
    Name1       - 1st name to compare
    Name2       - 2nd
    NameType    - type of names
    Flags       - control flags

Return Value:

    LONG

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        val = STRLEN(Name1);
        val = STRLEN(Name2);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return ERROR_INVALID_PARAMETER;
    }
    if (Flags & INNC_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsNameCompare(serverName, Name1, Name2, NameType, Flags);
    } else {
        return NetpwNameCompare(Name1, Name2, NameType, Flags);
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetpNameValidate(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Validates a name - checks whether a name of a certain type conforms to
    canonicalization rules for that name type. Canonicalization rules mean
    character set, name syntax and length

Arguments:

    ServerName  - where to perform this function
    Name        - name to validate
    NameType    - what type of name it is
    Flags       - MBZ

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(Name)) {
            val = STRLEN(Name);
        } else {
            status = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INNV_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsNameValidate(serverName, Name, NameType, Flags);
    } else {
        return NetpwNameValidate(Name, NameType, Flags);
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetpPathCanonicalize(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  PathName,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix OPTIONAL,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Canonicalizes a directory path or a device name

Arguments:

    ServerName  - where to run this API
    PathName    - path to canonicalize
    Outbuf      - where to write the canonicalized version
    OutbufLen   - length of Outbuf in bytes
    Prefix      - optional prefix which will be prepended to Path
    PathType    - the type of path to canonicalize. May be different at output
    Flags       - control flags

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;
    TCHAR ch;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(PathName)) {
            val = STRLEN(PathName);
        }
        if (ARGUMENT_PRESENT(Prefix)) {
            val = STRLEN(Prefix);
        }
        if (ARGUMENT_PRESENT(Outbuf)) {
            ch = (volatile TCHAR)*Outbuf;
            *Outbuf = ch;
            ch = (volatile TCHAR)*(Outbuf+OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf));
            *(Outbuf+OutbufLen/sizeof(*Outbuf) - sizeof(*Outbuf)) = ch;
        } else {
            status = ERROR_INVALID_PARAMETER;
        }
        val = *PathType ^ 0xf0f0f0f0;
        *PathType = val ^ 0xf0f0f0f0;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INPCA_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsPathCanonicalize(serverName,
                                        PathName,
                                        Outbuf,
                                        OutbufLen,
                                        Prefix,
                                        PathType,
                                        Flags
                                        );
    } else {
        return NetpwPathCanonicalize(PathName,
                                        Outbuf,
                                        OutbufLen,
                                        Prefix,
                                        PathType,
                                        Flags
                                        );
    }
}


LONG
NET_API_FUNCTION
NetpPathCompare(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Compares two paths. The paths are assumed to be of the same type

Arguments:

    ServerName  - where to run this API
    PathName1   - 1st path to compare
    PathName2   - 2nd
    PathType    - types of paths
    Flags       - control flags

Return Value:

    LONG

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(PathName1)) {
            val = STRLEN(PathName1);
        }
        if (ARGUMENT_PRESENT(PathName2)) {
            val = STRLEN(PathName2);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INPC_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsPathCompare(serverName, PathName1, PathName2, PathType, Flags);
    } else {
        return NetpwPathCompare(PathName1, PathName2, PathType, Flags);
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetpPathType(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Determines the type of a path

Arguments:

    ServerName  - where to run this API
    PathName    - to find type of
    PathType    - returned path type
    Flags       - control flags

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS status = 0;
    DWORD location;
    TCHAR serverName[MAX_PATH];
    DWORD val;

    //
    // validate parameters
    //

    try {
        if (ARGUMENT_PRESENT(ServerName)) {
            val = STRLEN(ServerName);
        }
        if (ARGUMENT_PRESENT(PathName)) {
            val = STRLEN(PathName);
        } else {
            val = 0;
        }
        if (!val || (val > MAX_PATH - 1)) {
            status = ERROR_INVALID_NAME;
        }
        *PathType = 0;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_PARAMETER;
    }
    if (status) {
        return status;
    }
    if (Flags & INPT_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // call client-side RPC routine or local canonicalization routine
    //

    status = NetpIsRemote(ServerName, &location, serverName, 0);
    if (status != NERR_Success) {
        return status;
    }
    if (location == ISREMOTE) {
        return NetpsPathType(serverName, PathName, PathType, Flags);
    } else {
        return NetpwPathType(PathName, PathType, Flags);
    }
}
