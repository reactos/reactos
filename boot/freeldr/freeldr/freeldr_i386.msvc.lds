/*****************************************************************************\
 ***      MSVC Linker Script File -- Sections specifications & layout      ***
\*****************************************************************************/

#ifndef _MSC_VER
#error This file must be used only with the MSVC compiler and linker.
#endif

#if defined(__LINKER__)
/*********************************\
 **    Linker response file     **
\*********************************/

// /LAST:.bss
/LAST:.data

#elif !defined(_USE_ML)
/*********************************\
 ** C Linker #pragma directives **
\*********************************/

/*
 * Notes about automatic LINK.EXE behaviour that cannot be changed:
 *
 * - If a .data section is present, the .bss section is appended to it in case
 *   both of these have either Initialized or Uninitialized data (which is the
 *   case, unless one has manually modified the attributes).
 *
 * - If the /DRIVER flag is NOT used, and if both the .rdata and the .edata
 *   sections (read-only data and exports, respectively) have the same
 *   attributes, the .edata section is appended to the .rdata section.
 *   Otherwise they are kept separated, unless an explicit /MERGE is used.
 */

#pragma comment(linker, "/SECTION:.text,ERW /SECTION:.data,RW /MERGE:.MBDATA=.text /MERGE:INIT=.text /MERGE:.edata=.rdata")

#endif


#if defined(_USE_ML) // && !defined(__LINKER__)
/************************\
 **  ASM definitions   **
\************************/

#include <asm.inc>

/*
#pragma bss_seg(".bss")
// __declspec(allocate(".bss"))
void* __bss_start__;
#pragma bss_seg()
*/
_BSS1   SEGMENT 'BSS' ALIAS('.bss')
PUBLIC __bss_start__
__bss_start__:
    .long ?
_BSS1   ENDS

/*
#pragma bss_seg(".bss$ZZZ")
// __declspec(allocate(".bss$ZZZ"))
void* __bss_end__;
#pragma bss_seg()
*/
_BSS2   SEGMENT 'BSS' ALIAS('.bss$ZZZ')
PUBLIC __bss_end__
__bss_end__:
    .long ?
_BSS2   ENDS

END

#elif !defined(__LINKER__)
/************************\
 **   C definitions    **
\************************/

#endif
