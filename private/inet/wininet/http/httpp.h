/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    httpp.h

Abstract:

    Private master include file for the HTTP API project.

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

--*/

//
//  Local include files.
//

#include "proc.h"
#include "headers.h"

// Beta logging
#ifdef BETA_LOGGING
#define BETA_LOG(stat) \
    {DWORD dw; IncrementUrlCacheHeaderData (CACHE_HEADER_DATA_##stat, &dw);}
#else
#define BETA_LOG(stat) do { } while(0)
#endif

