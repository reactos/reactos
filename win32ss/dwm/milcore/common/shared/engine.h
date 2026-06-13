// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Abstract:
*
*   Contains simple engine-wide prototypes and helper functions and
*   compile time flags.
*
*
\**************************************************************************/

#pragma once

MtExtern(MILImaging);
MtExtern(MIL);

ExternTag(tagMILWarning);
ExternTag(tagMILVerbose);

#define WIN32_OSMAJORVER(Version)  ((Version) >> 8)
#define WIN32_OSMINORVER(Version)  ((Version) & 0xFF)

#define WIN32_VISTA_MAJORVERSION   WIN32_OSMAJORVER(_WIN32_WINNT_LONGHORN)
#define WIN32_VISTA_MINORVERSION   WIN32_OSMINORVER(_WIN32_WINNT_LONGHORN)

namespace OSVersion {
    C_ASSERT( WIN32_VISTA_MINORVERSION == 0 );
}

// the path code is very keen on the following macro

#if DBG
    #define GOTO(label) \
    { \
        TraceTag((tagMILWarning, "Goto to Exit")); \
        goto label; \
    }
#else
    #define GOTO(label) goto label
#endif



