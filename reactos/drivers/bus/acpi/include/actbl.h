/******************************************************************************
 *
 * Name: actbl.h - Table data structures defined in ACPI specification
 *       $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ACTBL_H__
#define __ACTBL_H__


/*
 *  Values for description table header signatures
 */

#define RSDP_NAME               "RSDP"
#define RSDP_SIG                "RSD PTR "  /* RSDT Pointer signature */
#define APIC_SIG                "APIC"      /* Multiple APIC Description Table */
#define DSDT_SIG                "DSDT"      /* Differentiated System Description Table */
#define FADT_SIG                "FACP"      /* Fixed ACPI Description Table */
#define FACS_SIG                "FACS"      /* Firmware ACPI Control Structure */
#define PSDT_SIG                "PSDT"      /* Persistent System Description Table */
#define RSDT_SIG                "RSDT"      /* Root System Description Table */
#define XSDT_SIG                "XSDT"      /* Extended  System Description Table */
#define SSDT_SIG                "SSDT"      /* Secondary System Description Table */
#define SBST_SIG                "SBST"      /* Smart Battery Specification Table */
#define SPIC_SIG                "SPIC"      /* iosapic table */
#define BOOT_SIG                "BOOT"      /* Boot table */


#define GL_OWNED                0x02        /* Ownership of global lock is bit 1 */

/* values of Mapic.Model */

#define DUAL_PIC                0
#define MULTIPLE_APIC           1

/* values of Type in APIC_HEADER */

#define APIC_PROC               0
#define APIC_IO                 1


/*
 * Common table types.  The base code can remain
 * constant if the underlying tables are changed
 */
#define RSDT_DESCRIPTOR         RSDT_DESCRIPTOR_REV2
#define XSDT_DESCRIPTOR         XSDT_DESCRIPTOR_REV2
#define FACS_DESCRIPTOR         FACS_DESCRIPTOR_REV2
#define FADT_DESCRIPTOR         FADT_DESCRIPTOR_REV2


#pragma pack(1)

/*
 * Architecture-independent tables
 * The architecture dependent tables are in separate files
 */

typedef struct  /* Root System Descriptor Pointer */
{
	NATIVE_CHAR             signature [8];          /* contains "RSD PTR " */
	u8                      checksum;               /* to make sum of struct == 0 */
	NATIVE_CHAR             oem_id [6];             /* OEM identification */
	u8                      revision;               /* Must be 0 for 1.0, 2 for 2.0 */
	u32                     rsdt_physical_address;  /* 32-bit physical address of RSDT */
	u32                     length;                 /* XSDT Length in bytes including hdr */
	UINT64                  xsdt_physical_address;  /* 64-bit physical address of XSDT */
	u8                      extended_checksum;      /* Checksum of entire table */
	NATIVE_CHAR             reserved [3];           /* reserved field must be 0 */

} RSDP_DESCRIPTOR;


typedef struct  /* ACPI common table header */
{
	NATIVE_CHAR             signature [4];          /* identifies type of table */
	u32                     length;                 /* length of table, in bytes,
			  * including header */
	u8                      revision;               /* specification minor version # */
	u8                      checksum;               /* to make sum of entire table == 0 */
	NATIVE_CHAR             oem_id [6];             /* OEM identification */
	NATIVE_CHAR             oem_table_id [8];       /* OEM table identification */
	u32                     oem_revision;           /* OEM revision number */
	NATIVE_CHAR             asl_compiler_id [4];    /* ASL compiler vendor ID */
	u32                     asl_compiler_revision;  /* ASL compiler revision number */

} ACPI_TABLE_HEADER;


typedef struct  /* Common FACS for internal use */
{
	u32                     *global_lock;
	UINT64                  *firmware_waking_vector;
	u8                      vector_width;

} ACPI_COMMON_FACS;


typedef struct  /* APIC Table */
{
	ACPI_TABLE_HEADER       header;                 /* table header */
	u32                     local_apic_address;     /* Physical address for accessing local APICs */
	u32                     PCATcompat      : 1;    /* a one indicates system also has dual 8259s */
	u32                     reserved1       : 31;

} APIC_TABLE;


typedef struct  /* APIC Header */
{
	u8                      type;                   /* APIC type.  Either APIC_PROC or APIC_IO */
	u8                      length;                 /* Length of APIC structure */

} APIC_HEADER;


typedef struct  /* Processor APIC */
{
	APIC_HEADER             header;
	u8                      processor_apic_id;      /* ACPI processor id */
	u8                      local_apic_id;          /* processor's local APIC id */
	u32                     processor_enabled: 1;   /* Processor is usable if set */
	u32                     reserved1       : 32;

} PROCESSOR_APIC;


typedef struct  /* IO APIC */
{
	APIC_HEADER             header;
	u8                      io_apic_id;             /* I/O APIC ID */
	u8                      reserved;               /* reserved - must be zero */
	u32                     io_apic_address;        /* APIC's physical address */
	u32                     vector;                 /* interrupt vector index where INTI
			  * lines start */
} IO_APIC;


/*
**  IA64 TODO:  Add SAPIC Tables
*/

/*
**  IA64 TODO:  Modify Smart Battery Description to comply with ACPI IA64
**              extensions.
*/
typedef struct  /* Smart Battery Description Table */
{
	ACPI_TABLE_HEADER       header;
	u32                     warning_level;
	u32                     low_level;
	u32                     critical_level;

} SMART_BATTERY_DESCRIPTION_TABLE;


#pragma pack()


/*
 * ACPI Table information.  We save the table address, length,
 * and type of memory allocation (mapped or allocated) for each
 * table for 1) when we exit, and 2) if a new table is installed
 */

#define ACPI_MEM_NOT_ALLOCATED  0
#define ACPI_MEM_ALLOCATED      1
#define ACPI_MEM_MAPPED         2

/* Definitions for the Flags bitfield member of ACPI_TABLE_SUPPORT */

#define ACPI_TABLE_SINGLE       0
#define ACPI_TABLE_MULTIPLE     1


/* Data about each known table type */

typedef struct _acpi_table_support
{
	NATIVE_CHAR             *name;
	NATIVE_CHAR             *signature;
	u8                      sig_length;
	u8                      flags;
	u16                     status;
	void                    **global_ptr;

} ACPI_TABLE_SUPPORT;

/*
 * Get the architecture-specific tables
 */

#include "actbl1.h"   /* Acpi 1.0 table defintions */
#include "actbl71.h"  /* Acpi 0.71 IA-64 Extension table defintions */
#include "actbl2.h"   /* Acpi 2.0 table definitions */

#endif /* __ACTBL_H__ */
