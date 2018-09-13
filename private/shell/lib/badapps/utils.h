/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    utils.h

Abstract:

    Declares various function.

Author:

    Calin Negreanu (calinn) 01/20/1999

Revision History:

--*/

#pragma once

#include <windows.h>
#include <winnt.h>

BOOL
ShIsPatternMatch (
    IN     PCTSTR wstrPattern,
    IN     PCTSTR wstrStr
    );

PCTSTR
ShGetFileNameFromPath (
    IN      PCTSTR PathSpec
    );
