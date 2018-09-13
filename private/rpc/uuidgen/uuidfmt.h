/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    uuidfmt.h {v1.00}

Abstract:

    This module is used by uuidfmt.c and any other module which
    uses uuidfmt.c.
    It prototypes the entry into uuidfmt.c, I_UuidStringGenerate,
    and contains the set of flag codes used by I_UuidStringGenerate.

Author:

    Joev Dubach (t-joevd) 6/11/92

Revision History:

--*/

#ifndef __UUIDGEN_H__
#define __UUIDGEN_H__

//
// Defines (flags for I_UuidStringGenerate)
//

#define UUIDGEN_FORMAT_IDL 0
#define UUIDGEN_FORMAT_CSTRUCT 1
#define UUIDGEN_FORMAT_PLAIN 2

//
// Function prototypes
//

RPC_STATUS I_UuidStringGenerate(
    int Flag,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    );

#endif /* __UUIDGEN_H__ */

