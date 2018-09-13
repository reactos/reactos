/*++ BUILD Version: 0001

Copyright (c) 1990  Microsoft Corporation

Module Name:

    V86PC.H

Abstract:

    This file contains macros, function prototypes, and externs for the
    v86-mode NT version of SoftPC v3.0.

Author:

    Dave Hastings (daveh) 4-11-91

Revision History:

    Jeff Parsons (jeffpar) 14-May-1991
    Added X86CONTEXT, which is identical to CONTEXT when running on an
    x86 platform.

--*/


// Define X86CONTEXT structure

typedef CONTEXT X86CONTEXT, *PX86CONTEXT;
