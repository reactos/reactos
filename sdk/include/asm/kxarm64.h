/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactOS ARM64 architecture asm macros for armasm64 and GAS
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#ifndef __KXARM64_H__
#define __KXARM64_H__

/* No ARM64EC support, plain ARM64 names */
#define A64NAME(Name) Name

#ifdef _MSC_VER

    /* armasm64 labels start in the first column and take no colon */
    #define LABEL(Name) Name

    GBLA __ComdatAlignment
__ComdatAlignment SETA 2

    MACRO
    TEXTAREA
        AREA |.text|,ALIGN=2,CODE,READONLY
    MEND

    MACRO
    DATAAREA
        AREA |.data|,DATA
    MEND

    MACRO
    RODATAAREA
        AREA |.rdata|,DATA,READONLY
    MEND

    MACRO
    SET_COMDAT_ALIGNMENT $Alignment
__ComdatAlignment SETA $Alignment
    MEND

    MACRO
    NESTED_ENTRY $FuncName
        LCLS Name
Name    SETS "|$FuncName|"
        ALIGN 4
        EXPORT $Name
$Name   PROC
    MEND

    MACRO
    NESTED_END $FuncName
        LTORG
        ENDP
    MEND

    MACRO
    LEAF_ENTRY $FuncName
        NESTED_ENTRY $FuncName
    MEND

    MACRO
    LEAF_END $FuncName
        NESTED_END $FuncName
    MEND

    /* Each function gets its own section, aligned by SET_COMDAT_ALIGNMENT */
    MACRO
    LEAF_ENTRY_COMDAT $FuncName
        LCLS Area
Area    SETS "|.text$$mn$$$FuncName|"
        AREA $Area,ALIGN=$__ComdatAlignment,CODE,READONLY
        NESTED_ENTRY $FuncName
    MEND

    MACRO
    ALTERNATE_ENTRY $FuncName
        LCLS Name
Name    SETS "|$FuncName|"
        EXPORT $Name
$Name
    MEND

    MACRO
    ARM64EC_ENTRY_THUNK $A64Name, $Results, $Params
    MEND

#else

    #define LABEL(Name) Name:
    #define EQU =

.macro TEXTAREA
    .section .text, "xr"
    .p2align 2
.endm

.macro DATAAREA
    .section .data, "dw"
.endm

.macro RODATAAREA
    .section .rdata, "dr"
.endm

/* The armasm [FUNC] attribute has no GAS counterpart */
.macro EXPORT Name, Attribute
    .global \Name
.endm

/* Only weak aliases need anything, plain imports resolve at link time */
.macro IMPORT Name, Kind, Target
    .ifc \Kind,WEAK
        .weak \Name
        .set \Name, \Target
    .endif
.endm

.set __ComdatAlignment, 2

.macro SET_COMDAT_ALIGNMENT Alignment
    .set __ComdatAlignment, \Alignment
.endm

.macro NESTED_ENTRY Name
    .global \Name
    .p2align 2
\Name:
.endm

.macro NESTED_END Name
    .ltorg
.endm

.macro LEAF_ENTRY Name
    NESTED_ENTRY \Name
.endm

.macro LEAF_END Name
    NESTED_END \Name
.endm

.macro LEAF_ENTRY_COMDAT Name
    .section .text$mn$\Name, "xr"
    .p2align __ComdatAlignment
    NESTED_ENTRY \Name
.endm

.macro ALTERNATE_ENTRY Name
    .global \Name
\Name:
.endm

.macro ARM64EC_ENTRY_THUNK A64Name, Results, Params
.endm

/* armasm ALIGN takes a byte count, code sections pad with nop */
.macro ALIGN Alignment, Offset, Pad, PadSize
    .balign \Alignment
.endm

.macro dcb Values:vararg
    .byte \Values
.endm

.macro dcw Values:vararg
    .short \Values
.endm

.macro dcd Values:vararg
    .long \Values
.endm

.macro dcq Values:vararg
    .quad \Values
.endm

.macro END
.endm

#endif

#endif /* __KXARM64_H__ */
