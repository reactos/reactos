/*++
  Copyright (c) 1990  Microsoft Corporation
  Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    genalpha.c

Abstract:

    This module implements a program which generates ALPHA machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990
    Joe Notarangelo 26-Mar-1992

Revision History:

    Thomas Van Baak (tvb) 10-Jul-1992

        Modified CONTEXT, TRAP, and EXCEPTION frames according to the new
        Alpha calling standard.

    Forrest Foltz (forrestf) 24-Jan-1998

        Modified format to use new obj-based procedure, all contained
	in genalpha.m4.

--*/

include(`..\alpha\genalpha.m4')
