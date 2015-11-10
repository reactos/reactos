/******************************************************************************
 *
 * Name: acwin.h - OS specific defines, etc.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2014, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACWIN_H__
#define __ACWIN_H__

/*! [Begin] no source code translation (Keep the include) */

/* Windows uses VC */
#ifdef _MSC_VER
#include "acmsvc.h"
#endif
#ifdef __REACTOS__
#if !defined(_MSC_VER) && defined(__GNUC__)
#include "acgcc.h"
#endif

#define ACPI_USE_SYSTEM_INTTYPES
#endif /* __REACTOS __ */
/*! [End] no source code translation !*/

#define ACPI_MACHINE_WIDTH      32

#define ACPI_USE_STANDARD_HEADERS

#ifdef ACPI_DEFINE_ALTERNATE_TYPES
/*
 * Types used only in (Linux) translated source, defined here to enable
 * cross-platform compilation (i.e., generate the Linux code on Windows,
 * for test purposes only)
 */
typedef int                             s32;
typedef unsigned char                   u8;
typedef unsigned short                  u16;
typedef unsigned int                    u32;
typedef COMPILER_DEPENDENT_UINT64       u64;
#endif


/*
 * Handle platform- and compiler-specific assembly language differences.
 *
 * Notes:
 * 1) Interrupt 3 is used to break into a debugger
 * 2) Interrupts are turned off during ACPI register setup
 */

/*! [Begin] no source code translation  */

#ifndef __REACTOS__
#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()  __asm {WBINVD}
#endif
#endif /* __REACTOS__ */

#ifdef _DEBUG
#define ACPI_SIMPLE_RETURN_MACROS
#endif

/*! [End] no source code translation !*/

#ifndef __REACTOS__
/*
 * Global Lock acquire/release code
 *
 * Note: Handles case where the FACS pointer is null
 */
#define ACPI_ACQUIRE_GLOBAL_LOCK(FacsPtr, Acq)  __asm \
{                                                   \
        __asm mov           eax, 0xFF               \
        __asm mov           ecx, FacsPtr            \
        __asm or            ecx, ecx                \
        __asm jz            exit_acq                \
        __asm lea           ecx, [ecx].GlobalLock   \
                                                    \
        __asm acq10:                                \
        __asm mov           eax, [ecx]              \
        __asm mov           edx, eax                \
        __asm and           edx, 0xFFFFFFFE         \
        __asm bts           edx, 1                  \
        __asm adc           edx, 0                  \
        __asm lock cmpxchg  dword ptr [ecx], edx    \
        __asm jnz           acq10                   \
                                                    \
        __asm cmp           dl, 3                   \
        __asm sbb           eax, eax                \
                                                    \
        __asm exit_acq:                             \
        __asm mov           Acq, al                 \
}

#define ACPI_RELEASE_GLOBAL_LOCK(FacsPtr, Pnd) __asm \
{                                                   \
        __asm xor           eax, eax                \
        __asm mov           ecx, FacsPtr            \
        __asm or            ecx, ecx                \
        __asm jz            exit_rel                \
        __asm lea           ecx, [ecx].GlobalLock   \
                                                    \
        __asm Rel10:                                \
        __asm mov           eax, [ecx]              \
        __asm mov           edx, eax                \
        __asm and           edx, 0xFFFFFFFC         \
        __asm lock cmpxchg  dword ptr [ecx], edx    \
        __asm jnz           Rel10                   \
                                                    \
        __asm cmp           dl, 3                   \
        __asm and           eax, 1                  \
                                                    \
        __asm exit_rel:                             \
        __asm mov           Pnd, al                 \
}
#endif /* __REACTOS__ */

#endif /* __ACWIN_H__ */
