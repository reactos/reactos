
/******************************************************************************
 *
 * Module Name: amlresrc.h - AML resource descriptors
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code.  No other license or right
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
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
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
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
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

/* acpisrc:StructDefs -- for acpisrc conversion */

#ifndef __AMLRESRC_H
#define __AMLRESRC_H


/*
 * Resource descriptor tags, as defined in the ACPI specification.
 * Used to symbolically reference fields within a descriptor.
 */
#define ACPI_RESTAG_ADDRESS                     "_ADR"
#define ACPI_RESTAG_ALIGNMENT                   "_ALN"
#define ACPI_RESTAG_ADDRESSSPACE                "_ASI"
#define ACPI_RESTAG_ACCESSSIZE                  "_ASZ"
#define ACPI_RESTAG_TYPESPECIFICATTRIBUTES      "_ATT"
#define ACPI_RESTAG_BASEADDRESS                 "_BAS"
#define ACPI_RESTAG_BUSMASTER                   "_BM_"  /* Master(1), Slave(0) */
#define ACPI_RESTAG_DECODE                      "_DEC"
#define ACPI_RESTAG_DMA                         "_DMA"
#define ACPI_RESTAG_DMATYPE                     "_TYP"  /* Compatible(0), A(1), B(2), F(3) */
#define ACPI_RESTAG_GRANULARITY                 "_GRA"
#define ACPI_RESTAG_INTERRUPT                   "_INT"
#define ACPI_RESTAG_INTERRUPTLEVEL              "_LL_"  /* ActiveLo(1), ActiveHi(0) */
#define ACPI_RESTAG_INTERRUPTSHARE              "_SHR"  /* Shareable(1), NoShare(0) */
#define ACPI_RESTAG_INTERRUPTTYPE               "_HE_"  /* Edge(1), Level(0) */
#define ACPI_RESTAG_LENGTH                      "_LEN"
#define ACPI_RESTAG_MEMATTRIBUTES               "_MTP"  /* Memory(0), Reserved(1), ACPI(2), NVS(3) */
#define ACPI_RESTAG_MEMTYPE                     "_MEM"  /* NonCache(0), Cacheable(1) Cache+combine(2), Cache+prefetch(3) */
#define ACPI_RESTAG_MAXADDR                     "_MAX"
#define ACPI_RESTAG_MINADDR                     "_MIN"
#define ACPI_RESTAG_MAXTYPE                     "_MAF"
#define ACPI_RESTAG_MINTYPE                     "_MIF"
#define ACPI_RESTAG_REGISTERBITOFFSET           "_RBO"
#define ACPI_RESTAG_REGISTERBITWIDTH            "_RBW"
#define ACPI_RESTAG_RANGETYPE                   "_RNG"
#define ACPI_RESTAG_READWRITETYPE               "_RW_"  /* ReadOnly(0), Writeable (1) */
#define ACPI_RESTAG_TRANSLATION                 "_TRA"
#define ACPI_RESTAG_TRANSTYPE                   "_TRS"  /* Sparse(1), Dense(0) */
#define ACPI_RESTAG_TYPE                        "_TTP"  /* Translation(1), Static (0) */
#define ACPI_RESTAG_XFERTYPE                    "_SIZ"  /* 8(0), 8And16(1), 16(2) */


/* Default sizes for "small" resource descriptors */

#define ASL_RDESC_IRQ_SIZE                      0x02
#define ASL_RDESC_DMA_SIZE                      0x02
#define ASL_RDESC_ST_DEPEND_SIZE                0x00
#define ASL_RDESC_END_DEPEND_SIZE               0x00
#define ASL_RDESC_IO_SIZE                       0x07
#define ASL_RDESC_FIXED_IO_SIZE                 0x03
#define ASL_RDESC_END_TAG_SIZE                  0x01


typedef struct asl_resource_node
{
    UINT32                          BufferLength;
    void                            *Buffer;
    struct asl_resource_node        *Next;

} ASL_RESOURCE_NODE;


/* Macros used to generate AML resource length fields */

#define ACPI_AML_SIZE_LARGE(r)      (sizeof (r) - sizeof (AML_RESOURCE_LARGE_HEADER))
#define ACPI_AML_SIZE_SMALL(r)      (sizeof (r) - sizeof (AML_RESOURCE_SMALL_HEADER))

/*
 * Resource descriptors defined in the ACPI specification.
 *
 * Packing/alignment must be BYTE because these descriptors
 * are used to overlay the raw AML byte stream.
 */
#pragma pack(1)

/*
 * SMALL descriptors
 */
#define AML_RESOURCE_SMALL_HEADER_COMMON \
    UINT8                           DescriptorType;

typedef struct aml_resource_small_header
{
    AML_RESOURCE_SMALL_HEADER_COMMON

} AML_RESOURCE_SMALL_HEADER;


typedef struct aml_resource_irq
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT16                          IrqMask;
    UINT8                           Flags;

} AML_RESOURCE_IRQ;


typedef struct aml_resource_irq_noflags
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT16                          IrqMask;

} AML_RESOURCE_IRQ_NOFLAGS;


typedef struct aml_resource_dma
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT8                           DmaChannelMask;
    UINT8                           Flags;

} AML_RESOURCE_DMA;


typedef struct aml_resource_start_dependent
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT8                           Flags;

} AML_RESOURCE_START_DEPENDENT;


typedef struct aml_resource_start_dependent_noprio
{
    AML_RESOURCE_SMALL_HEADER_COMMON

} AML_RESOURCE_START_DEPENDENT_NOPRIO;


typedef struct aml_resource_end_dependent
{
    AML_RESOURCE_SMALL_HEADER_COMMON

} AML_RESOURCE_END_DEPENDENT;


typedef struct aml_resource_io
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT8                           Flags;
    UINT16                          Minimum;
    UINT16                          Maximum;
    UINT8                           Alignment;
    UINT8                           AddressLength;

} AML_RESOURCE_IO;


typedef struct aml_resource_fixed_io
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT16                          Address;
    UINT8                           AddressLength;

} AML_RESOURCE_FIXED_IO;


typedef struct aml_resource_vendor_small
{
    AML_RESOURCE_SMALL_HEADER_COMMON

} AML_RESOURCE_VENDOR_SMALL;


typedef struct aml_resource_end_tag
{
    AML_RESOURCE_SMALL_HEADER_COMMON
    UINT8                           Checksum;

} AML_RESOURCE_END_TAG;


/*
 * LARGE descriptors
 */
#define AML_RESOURCE_LARGE_HEADER_COMMON \
    UINT8                           DescriptorType;\
    UINT16                          ResourceLength;

typedef struct aml_resource_large_header
{
    AML_RESOURCE_LARGE_HEADER_COMMON

} AML_RESOURCE_LARGE_HEADER;


typedef struct aml_resource_memory24
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    UINT8                           Flags;
    UINT16                          Minimum;
    UINT16                          Maximum;
    UINT16                          Alignment;
    UINT16                          AddressLength;

} AML_RESOURCE_MEMORY24;


typedef struct aml_resource_vendor_large
{
    AML_RESOURCE_LARGE_HEADER_COMMON

} AML_RESOURCE_VENDOR_LARGE;


typedef struct aml_resource_memory32
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    UINT8                           Flags;
    UINT32                          Minimum;
    UINT32                          Maximum;
    UINT32                          Alignment;
    UINT32                          AddressLength;

} AML_RESOURCE_MEMORY32;


typedef struct aml_resource_fixed_memory32
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    UINT8                           Flags;
    UINT32                          Address;
    UINT32                          AddressLength;

} AML_RESOURCE_FIXED_MEMORY32;


#define AML_RESOURCE_ADDRESS_COMMON \
    UINT8                           ResourceType; \
    UINT8                           Flags; \
    UINT8                           SpecificFlags;


typedef struct aml_resource_address
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    AML_RESOURCE_ADDRESS_COMMON

} AML_RESOURCE_ADDRESS;


typedef struct aml_resource_extended_address64
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    AML_RESOURCE_ADDRESS_COMMON
    UINT8                           RevisionID;
    UINT8                           Reserved;
    UINT64                          Granularity;
    UINT64                          Minimum;
    UINT64                          Maximum;
    UINT64                          TranslationOffset;
    UINT64                          AddressLength;
    UINT64                          TypeSpecific;

} AML_RESOURCE_EXTENDED_ADDRESS64;

#define AML_RESOURCE_EXTENDED_ADDRESS_REVISION          1       /* ACPI 3.0 */


typedef struct aml_resource_address64
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    AML_RESOURCE_ADDRESS_COMMON
    UINT64                          Granularity;
    UINT64                          Minimum;
    UINT64                          Maximum;
    UINT64                          TranslationOffset;
    UINT64                          AddressLength;

} AML_RESOURCE_ADDRESS64;


typedef struct aml_resource_address32
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    AML_RESOURCE_ADDRESS_COMMON
    UINT32                          Granularity;
    UINT32                          Minimum;
    UINT32                          Maximum;
    UINT32                          TranslationOffset;
    UINT32                          AddressLength;

} AML_RESOURCE_ADDRESS32;


typedef struct aml_resource_address16
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    AML_RESOURCE_ADDRESS_COMMON
    UINT16                          Granularity;
    UINT16                          Minimum;
    UINT16                          Maximum;
    UINT16                          TranslationOffset;
    UINT16                          AddressLength;

} AML_RESOURCE_ADDRESS16;


typedef struct aml_resource_extended_irq
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    UINT8                           Flags;
    UINT8                           InterruptCount;
    UINT32                          Interrupts[1];
    /* ResSourceIndex, ResSource optional fields follow */

} AML_RESOURCE_EXTENDED_IRQ;


typedef struct aml_resource_generic_register
{
    AML_RESOURCE_LARGE_HEADER_COMMON
    UINT8                           AddressSpaceId;
    UINT8                           BitWidth;
    UINT8                           BitOffset;
    UINT8                           AccessSize; /* ACPI 3.0, was previously Reserved */
    UINT64                          Address;

} AML_RESOURCE_GENERIC_REGISTER;

/* restore default alignment */

#pragma pack()

/* Union of all resource descriptors, so we can allocate the worst case */

typedef union aml_resource
{
    /* Descriptor headers */

    UINT8                                   DescriptorType;
    AML_RESOURCE_SMALL_HEADER               SmallHeader;
    AML_RESOURCE_LARGE_HEADER               LargeHeader;

    /* Small resource descriptors */

    AML_RESOURCE_IRQ                        Irq;
    AML_RESOURCE_DMA                        Dma;
    AML_RESOURCE_START_DEPENDENT            StartDpf;
    AML_RESOURCE_END_DEPENDENT              EndDpf;
    AML_RESOURCE_IO                         Io;
    AML_RESOURCE_FIXED_IO                   FixedIo;
    AML_RESOURCE_VENDOR_SMALL               VendorSmall;
    AML_RESOURCE_END_TAG                    EndTag;

    /* Large resource descriptors */

    AML_RESOURCE_MEMORY24                   Memory24;
    AML_RESOURCE_GENERIC_REGISTER           GenericReg;
    AML_RESOURCE_VENDOR_LARGE               VendorLarge;
    AML_RESOURCE_MEMORY32                   Memory32;
    AML_RESOURCE_FIXED_MEMORY32             FixedMemory32;
    AML_RESOURCE_ADDRESS16                  Address16;
    AML_RESOURCE_ADDRESS32                  Address32;
    AML_RESOURCE_ADDRESS64                  Address64;
    AML_RESOURCE_EXTENDED_ADDRESS64         ExtAddress64;
    AML_RESOURCE_EXTENDED_IRQ               ExtendedIrq;

    /* Utility overlays */

    AML_RESOURCE_ADDRESS                    Address;
    UINT32                                  DwordItem;
    UINT16                                  WordItem;
    UINT8                                   ByteItem;

} AML_RESOURCE;

#endif

