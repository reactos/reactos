/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    classfwd.h

Abstract:

    This  module  contains "forward" declarations for major types used commonly
    within the WinSock 2 DLL.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 08-July-1995

Notes:

    $Revision:   1.9  $

    $Modtime:   08 Mar 1996 04:52:58  $

Revision History:

    most-recent-revision-date email-name
        description

    25-July-1995 dirk@mink.intel.com
        Added forward definintions for DCATALOG

    07-09-1995  drewsxpa@ashland.intel.com
        Completed  first  complete  version with clean compile and released for
        subsequent implementation.

    07-08-1995  drewsxpa@ashland.intel.com
        Original version

--*/

#ifndef _CLASSFWD_
#define _CLASSFWD_

#include <windows.h>

class DTHREAD;
typedef DTHREAD FAR * PDTHREAD;

class DPROCESS;
typedef DPROCESS FAR * PDPROCESS;

class DSOCKET;
typedef DSOCKET FAR * PDSOCKET;

class DPROVIDER;
typedef DPROVIDER FAR * PDPROVIDER;

class PROTO_CATALOG_ITEM;
typedef PROTO_CATALOG_ITEM  FAR * PPROTO_CATALOG_ITEM;

class DCATALOG;
typedef DCATALOG FAR * PDCATALOG;

class NSPROVIDER;
typedef NSPROVIDER FAR * PNSPROVIDER;

class NSPSTATE;
typedef NSPSTATE FAR * PNSPSTATE;

class NSCATALOG;
typedef NSCATALOG FAR * PNSCATALOG;

class NSCATALOGENTRY;
typedef NSCATALOGENTRY FAR * PNSCATALOGENTRY;

class NSQUERY;
typedef NSQUERY FAR * PNSQUERY;

class NSPROVIDERSTATE;
typedef NSPROVIDERSTATE FAR * PNSPROVIDERSTATE;


#endif  // _CLASSFWD_
