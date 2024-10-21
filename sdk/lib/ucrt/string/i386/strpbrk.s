#if 0
;***
;strpbrk.asm -
;
;       Copyright (c) Microsoft Corporation.  All rights reserved.
;
;Purpose:
;       defines strpbrk()- finds the index of the first character in a string
;       that is not in a control string
;
;       NOTE:  This stub module scheme is compatible with NT build
;       procedure.
;
;*******************************************************************************

SSTRPBRK EQU 1
INCLUDE STRSPN.ASM
#else
#define SSTRPBRK 1
#include "strspn.s"
#endif
