/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    qshelpr.h

Abstract:

    This  file contains the definitions of the procedures exported by the query
    set helper module for internal use within the WinSock 2 DLL.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 11-Jan-1996

Notes:

    $Revision:   1.2  $

    $Modtime:   18 Jan 1996 11:22:32  $

Revision History:

    most-recent-revision-date email-name
        description

    11-Jan-1996 drewsxpa@ashland.intel.com
        Created original version

--*/

#ifndef _QSHELPR_
#define _QSHELPR_

#include "winsock2.h"
#include <windows.h>


INT
MapAnsiQuerySetToUnicode(
    IN     LPWSAQUERYSETA  Source,
    IN OUT LPDWORD         lpTargetSize,
    OUT    LPWSAQUERYSETW  Target
    );


INT
MapUnicodeQuerySetToAnsi(
    IN     LPWSAQUERYSETW  Source,
    IN OUT LPDWORD         lpTargetSize,
    OUT    LPWSAQUERYSETA  Target
    );

INT
CopyQuerySetA(
    IN LPWSAQUERYSETA  Source,
    OUT LPWSAQUERYSETA *Target
    );


CopyQuerySetW(
    IN LPWSAQUERYSETW  Source,
    OUT LPWSAQUERYSETW *Target
    );

LPWSTR
wcs_dup_from_ansi(
    IN LPSTR  Source
    );

LPSTR
ansi_dup_from_wcs(
    IN LPWSTR  Source
    );


#endif // _QSHELPR_

