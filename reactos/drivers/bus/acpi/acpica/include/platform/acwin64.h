/******************************************************************************
 *
 * Name: acwin64.h - OS specific defines, etc.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACWIN64_H__
#define __ACWIN64_H__

/*! [Begin] no source code translation (Keep the include) */

#include "acintel.h"
/*! [End] no source code translation !*/

#define ACPI_MACHINE_WIDTH          64

#define ACPI_USE_STANDARD_HEADERS

/*
 * Handle platform- and compiler-specific assembly language differences.
 *
 * Notes:
 * 1) Interrupt 3 is used to break into a debugger
 * 2) Interrupts are turned off during ACPI register setup
 */

/*! [Begin] no source code translation  */

#define ACPI_FLUSH_CPU_CACHE()

/*
 * For Acpi applications, we don't want to try to access the global lock
 */
#ifdef ACPI_APPLICATION
#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)       if (AcpiGbl_GlobalLockPresent) {Acq = 0xFF;} else {Acq = 0;}
#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pnd)       if (AcpiGbl_GlobalLockPresent) {Pnd = 0xFF;} else {Pnd = 0;}
#else

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pnd)

#endif

/*! [End] no source code translation !*/

#endif /* __ACWIN_H__ */
