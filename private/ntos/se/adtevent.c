/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    adtevent.c - Audit Event Management

Abstract:

    This module contains functions that update the Audit Event Information.

Author:

    Scott Birrell       (ScottBi)       November 14, 1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include <nt.h>
#include <ntos.h>
#include "rmp.h"
#include "adt.h"
#include "adtp.h"


