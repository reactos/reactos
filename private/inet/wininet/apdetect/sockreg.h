/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sockreg.h

Abstract:

    Prototypes for sockreg.cxx

    stolen from Win95 winsock project (& modified)

Author:

    Richard L Firth (rfirth) 10-Feb-1994

Environment:

    Chicago/Snowball (i.e. Win32/Win16)

Revision History:

    10-Feb-1994 (rfirth)
        Created

--*/

//
// registry/config/ini items
//

#define CONFIG_HOSTNAME     1
#define CONFIG_DOMAIN       2
#define CONFIG_SEARCH_LIST  3
#define CONFIG_NAME_SERVER  4

//
// prototypes
//

UINT
SockGetSingleValue(
    IN UINT ParameterId,
    OUT LPBYTE Data,
    IN UINT DataLength
    );
