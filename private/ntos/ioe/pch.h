/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    This is the pre-compile header file.

Author:
    Michael Tsang (MikeTs) 1-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME "IOERR"

#if DBG
  #define TRACING
#endif

#include <ntos.h>
#include <zwapi.h>
#include <wchar.h>
#include <wmistr.h>
#include <pnpiop.h>
#include <trackirp.h>
#include "ecdb.h"
#include "ioep.h"
#include "trace.h"
#include "data.h"

#endif  //ifndef _PCH_H
