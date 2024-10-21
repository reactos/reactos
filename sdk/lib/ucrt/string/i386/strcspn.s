#if 0
;***
;strcspn.asm -
;
;       Copyright (c) Microsoft Corporation.  All rights reserved.
;
;Purpose:
;       defines strcspn()- finds the length of the initial substring of
;       a string consisting entirely of characters not in a control string.
;
;       NOTE:  This stub module scheme is compatible with NT build
;       procedure.
;
;*******************************************************************************

SSTRCSPN EQU 1
INCLUDE STRSPN.ASM
#else
#define SSTRCSPN 1
#include "strspn.s"
#endif
