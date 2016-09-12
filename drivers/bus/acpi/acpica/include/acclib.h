/******************************************************************************
 *
 * Name: acclib.h -- C library support. Prototypes for the (optional) local
 *                   implementations of required C library functions.
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

#ifndef _ACCLIB_H
#define _ACCLIB_H


/*
 * Prototypes and macros for local implementations of C library functions
 */

/* is* functions. The AcpiGbl_Ctypes array is defined in utclib.c */

extern const UINT8 AcpiGbl_Ctypes[];

#define _ACPI_XA     0x00    /* extra alphabetic - not supported */
#define _ACPI_XS     0x40    /* extra space */
#define _ACPI_BB     0x00    /* BEL, BS, etc. - not supported */
#define _ACPI_CN     0x20    /* CR, FF, HT, NL, VT */
#define _ACPI_DI     0x04    /* '0'-'9' */
#define _ACPI_LO     0x02    /* 'a'-'z' */
#define _ACPI_PU     0x10    /* punctuation */
#define _ACPI_SP     0x08    /* space, tab, CR, LF, VT, FF */
#define _ACPI_UP     0x01    /* 'A'-'Z' */
#define _ACPI_XD     0x80    /* '0'-'9', 'A'-'F', 'a'-'f' */

#define isdigit(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_DI))
#define isspace(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_SP))
#define isxdigit(c) (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_XD))
#define isupper(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_UP))
#define islower(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_LO))
#define isprint(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_LO | _ACPI_UP | _ACPI_DI | _ACPI_XS | _ACPI_PU))
#define isalpha(c)  (AcpiGbl_Ctypes[(unsigned char)(c)] & (_ACPI_LO | _ACPI_UP))


/* Strings */

char *
strcat (
    char                    *DstString,
    const char              *SrcString);

char *
strchr (
    const char              *String,
    int                     ch);

char *
strcpy (
    char                    *DstString,
    const char              *SrcString);

int
strcmp (
    const char              *String1,
    const char              *String2);

ACPI_SIZE
strlen (
    const char              *String);

char *
strncat (
    char                    *DstString,
    const char              *SrcString,
    ACPI_SIZE               Count);

int
strncmp (
    const char              *String1,
    const char              *String2,
    ACPI_SIZE               Count);

char *
strncpy (
    char                    *DstString,
    const char              *SrcString,
    ACPI_SIZE               Count);

char *
strstr (
    char                    *String1,
    char                    *String2);


/* Conversion */

UINT32
strtoul (
    const char              *String,
    char                    **Terminator,
    UINT32                  Base);


/* Memory */

int
memcmp (
    void                    *Buffer1,
    void                    *Buffer2,
    ACPI_SIZE               Count);

void *
memcpy (
    void                    *Dest,
    const void              *Src,
    ACPI_SIZE               Count);

void *
memset (
    void                    *Dest,
    int                     Value,
    ACPI_SIZE               Count);


/* upper/lower case */

int
tolower (
    int                     c);

int
toupper (
    int                     c);

#endif /* _ACCLIB_H */
