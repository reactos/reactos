/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            asmxtras.inc
 * PURPOSE:         Extended ASM macros for GAS and MASM/ML64
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: This file is an extension to our well-known asm.inc that defines
 *       a set of macros allowing us to assemble specially-crafted ASM files
 *       with both GAS and MASM/ML.
 *
 *       The additions introduced here are:
 *       - a 'long' define for MASM/ML that aliases to DWORD, and a 'dword'
 *         and 'DWORD' defines for GAS that alias to long.
 *       - an OFF(...) macro that is used for initializing global symbols with
 *         the offset value of another symbol.
 *       - a set of macros for defining and using structures.
 */

#ifndef __ASMXTRAS_INC__
#define __ASMXTRAS_INC__

/* 'long' / 'dword'|'DWORD' macros for MASM/ML and GAS */
#ifdef _USE_ML
    #define long dword
#else
    #define dword long
    #define DWORD long
#endif

/* OFFset macro */
#ifdef _USE_ML
    #define OFF(x) offset x
#else
    #define OFF(x) x
#endif

/*
 * Set of macros for defining and using structures:
 * - STRUCT(name, ...) defines a structure of name 'name'.
 * - FIELD_DECL(field, type, value) adds a new structure member 'field'
 *   of type 'type', with the default value 'value'.
 * - ENDS(name) terminates the definition of the structure 'name'.
 * - A symbol 'name' of type 'struct' is declared with VAR_STRUCT(name, struct).
 * - Referencing a member 'field' of a symbol 'name' is done with FIELD(name, field).
 */
#ifdef _USE_ML

#define STRUCT(name, ...) \
    name STRUCT __VA_ARGS__

#define FIELD_DECL(field, type, value) \
    field   type   value

#define ENDS(name) \
    name ENDS

#define VAR_STRUCT(name, struct) \
    name struct <>

#define FIELD(name, field) \
    name##.##field

#else

#define STRUCT(name, ...) \
    MACRO(name, VarName)

#define FIELD_DECL(field, type, value) \
    VarName\()_\()field:   .type   value

#define ENDS(...) ENDM

#define VAR_STRUCT(name, struct) \
    name: struct name

#define FIELD(name, field) \
    name##_##field

#endif

#endif /* __ASMXTRAS_INC__ */
