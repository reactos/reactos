/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    scihelpr.h

Abstract:

    This  file contains the definitions of the procedures exported by the
    service class info helper module for internal use within the WinSock 2
    DLL. 

Author:

    Dirk Brandewie (dirk@mink.intel.com)  25-Jan-1996

Notes:

    $Revision:   1.0  $

    $Modtime:   25 Jan 1996 11:08:36  $

Revision History:

    most-recent-revision-date email-name
        description

    25-Jan-1996 dirk@mink.intel.com
        Created original version

--*/

#ifndef _SCIHELPR_
#define _SCIHELPR_

#include "winsock2.h"
#include <windows.h>

INT
MapAnsiServiceClassInfoToUnicode(
    IN     LPWSASERVICECLASSINFOA Source,
    IN OUT LPDWORD                lpTargetSize,
    IN     LPWSASERVICECLASSINFOW Target
    );

INT
MapUnicodeServiceClassInfoToAnsi(
    IN     LPWSASERVICECLASSINFOW Source,
    IN OUT LPDWORD                lpTargetSize,
    IN     LPWSASERVICECLASSINFOA Target
    );

#endif // _SCIHELPR_
