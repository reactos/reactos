/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    cons.h

Abstract:

    Global constant definitions for the MSAFD NTSD Debugger Extensions.

Author:

    Keith Mnoore (keithmo) 20-May-1996.

Environment:

    User Mode.

--*/


#ifndef _CONS_H_
#define _CONS_H_


#define UC(x)               ((UINT)((x) & 0xFF))
#define NTOHS(x)            ( (UC(x) * 256) + UC((x) >> 8) )


#endif  // _CONS_H_

