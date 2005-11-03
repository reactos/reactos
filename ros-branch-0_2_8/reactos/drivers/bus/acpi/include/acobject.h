
/******************************************************************************
 *
 * Name: acobject.h - Definition of ACPI_OPERAND_OBJECT  (Internal object only)
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

#ifndef _ACOBJECT_H
#define _ACOBJECT_H


/*
 * The ACPI_OPERAND_OBJECT  is used to pass AML operands from the dispatcher
 * to the interpreter, and to keep track of the various handlers such as
 * address space handlers and notify handlers.  The object is a constant
 * size in order to allow them to be cached and reused.
 *
 * All variants of the ACPI_OPERAND_OBJECT  are defined with the same
 * sequence of field types, with fields that are not used in a particular
 * variant being named "Reserved".  This is not strictly necessary, but
 * may in some circumstances simplify understanding if these structures
 * need to be displayed in a debugger having limited (or no) support for
 * union types.  It also simplifies some debug code in Dump_table() which
 * dumps multi-level values: fetching Buffer.Pointer suffices to pick up
 * the value or next level for any of several types.
 */

/******************************************************************************
 *
 * Common Descriptors
 *
 *****************************************************************************/

/*
 * Common area for all objects.
 *
 * Data_type is used to differentiate between internal descriptors, and MUST
 * be the first byte in this structure.
 */


#define ACPI_OBJECT_COMMON_HEADER           /* 32-bits plus 8-bit flag */\
	u8                          data_type;          /* To differentiate various internal objs */\
	u8                          type;               /* ACPI_OBJECT_TYPE */\
	u16                         reference_count;    /* For object deletion management */\
	u8                          flags; \

/* Defines for flag byte above */

#define AOPOBJ_STATIC_ALLOCATION    0x1
#define AOPOBJ_DATA_VALID           0x2
#define AOPOBJ_INITIALIZED          0x4


/*
 * Common bitfield for the field objects
 */
#define ACPI_COMMON_FIELD_INFO              /* Three 32-bit values plus 8*/\
	u8                          granularity;\
	u16                         length; \
	u32                         offset;             /* Byte offset within containing object */\
	u8                          bit_offset;         /* Bit offset within min read/write data unit */\
	u8                          access;             /* Access_type */\
	u8                          lock_rule;\
	u8                          update_rule;\
	u8                          access_attribute;


/******************************************************************************
 *
 * Individual Object Descriptors
 *
 *****************************************************************************/


typedef struct /* COMMON */
{
	ACPI_OBJECT_COMMON_HEADER

} ACPI_OBJECT_COMMON;


typedef struct /* CACHE_LIST */
{
	ACPI_OBJECT_COMMON_HEADER
	union acpi_operand_obj      *next;              /* Link for object cache and internal lists*/

} ACPI_OBJECT_CACHE_LIST;


typedef struct /* NUMBER - has value */
{
	ACPI_OBJECT_COMMON_HEADER

	ACPI_INTEGER                value;

} ACPI_OBJECT_INTEGER;


typedef struct /* STRING - has length and pointer - Null terminated, ASCII characters only */
{
	ACPI_OBJECT_COMMON_HEADER

	u32                         length;
	NATIVE_CHAR                 *pointer;       /* String value in AML stream or in allocated space */

} ACPI_OBJECT_STRING;


typedef struct /* BUFFER - has length and pointer - not null terminated */
{
	ACPI_OBJECT_COMMON_HEADER

	u32                         length;
	u8                          *pointer;       /* points to the buffer in allocated space */

} ACPI_OBJECT_BUFFER;


typedef struct /* PACKAGE - has count, elements, next element */
{
	ACPI_OBJECT_COMMON_HEADER

	u32                         count;          /* # of elements in package */

	union acpi_operand_obj      **elements;     /* Array of pointers to Acpi_objects */
	union acpi_operand_obj      **next_element; /* used only while initializing */

} ACPI_OBJECT_PACKAGE;


typedef struct /* FIELD UNIT */
{
	ACPI_OBJECT_COMMON_HEADER

	ACPI_COMMON_FIELD_INFO

	union acpi_operand_obj      *extra;             /* Pointer to executable AML (in field definition) */
	ACPI_NAMESPACE_NODE         *node;              /* containing object */
	union acpi_operand_obj      *container;         /* Containing object (Buffer) */

} ACPI_OBJECT_FIELD_UNIT;


typedef struct /* DEVICE - has handle and notification handler/context */
{
	ACPI_OBJECT_COMMON_HEADER

	union acpi_operand_obj      *sys_handler;        /* Handler for system notifies */
	union acpi_operand_obj      *drv_handler;        /* Handler for driver notifies */
	union acpi_operand_obj      *addr_handler;       /* Handler for Address space */

} ACPI_OBJECT_DEVICE;


typedef struct /* EVENT */
{
	ACPI_OBJECT_COMMON_HEADER
	void                        *semaphore;

} ACPI_OBJECT_EVENT;


#define INFINITE_CONCURRENCY        0xFF

typedef struct /* METHOD */
{
	ACPI_OBJECT_COMMON_HEADER
	u8                          method_flags;
	u8                          param_count;

	u32                         pcode_length;

	void                        *semaphore;
	u8                          *pcode;

	u8                          concurrency;
	u8                          thread_count;
	ACPI_OWNER_ID               owning_id;

} ACPI_OBJECT_METHOD;


typedef struct acpi_obj_mutex /* MUTEX */
{
	ACPI_OBJECT_COMMON_HEADER
	u16                         sync_level;
	u16                         acquisition_depth;

	void                        *semaphore;
	void                        *owner;
	union acpi_operand_obj      *prev;              /* Link for list of acquired mutexes */
	union acpi_operand_obj      *next;              /* Link for list of acquired mutexes */

} ACPI_OBJECT_MUTEX;


typedef struct /* REGION */
{
	ACPI_OBJECT_COMMON_HEADER

	u8                          space_id;
	u32                         length;
	ACPI_PHYSICAL_ADDRESS       address;
	union acpi_operand_obj      *extra;             /* Pointer to executable AML (in region definition) */

	union acpi_operand_obj      *addr_handler;      /* Handler for system notifies */
	ACPI_NAMESPACE_NODE         *node;              /* containing object */
	union acpi_operand_obj      *next;

} ACPI_OBJECT_REGION;


typedef struct /* POWER RESOURCE - has Handle and notification handler/context*/
{
	ACPI_OBJECT_COMMON_HEADER

	u32                         system_level;
	u32                         resource_order;

	union acpi_operand_obj      *sys_handler;       /* Handler for system notifies */
	union acpi_operand_obj      *drv_handler;       /* Handler for driver notifies */

} ACPI_OBJECT_POWER_RESOURCE;


typedef struct /* PROCESSOR - has Handle and notification handler/context*/
{
	ACPI_OBJECT_COMMON_HEADER

	u32                         proc_id;
	u32                         length;
	ACPI_IO_ADDRESS             address;

	union acpi_operand_obj      *sys_handler;       /* Handler for system notifies */
	union acpi_operand_obj      *drv_handler;       /* Handler for driver notifies */
	union acpi_operand_obj      *addr_handler;      /* Handler for Address space */

} ACPI_OBJECT_PROCESSOR;


typedef struct /* THERMAL ZONE - has Handle and Handler/Context */
{
	ACPI_OBJECT_COMMON_HEADER

	union acpi_operand_obj      *sys_handler;       /* Handler for system notifies */
	union acpi_operand_obj      *drv_handler;       /* Handler for driver notifies */
	union acpi_operand_obj      *addr_handler;      /* Handler for Address space */

} ACPI_OBJECT_THERMAL_ZONE;


/*
 * Internal types
 */


typedef struct /* FIELD */
{
	ACPI_OBJECT_COMMON_HEADER

	ACPI_COMMON_FIELD_INFO

	union acpi_operand_obj      *container;         /* Containing object */

} ACPI_OBJECT_FIELD;


typedef struct /* BANK FIELD */
{
	ACPI_OBJECT_COMMON_HEADER

	ACPI_COMMON_FIELD_INFO
	u32                         value;              /* Value to store into Bank_select */

	ACPI_HANDLE                 bank_select;        /* Bank select register */
	union acpi_operand_obj      *container;         /* Containing object */

} ACPI_OBJECT_BANK_FIELD;


typedef struct /* INDEX FIELD */
{
	/*
	 * No container pointer needed since the index and data register definitions
	 * will define how to access the respective registers
	 */
	ACPI_OBJECT_COMMON_HEADER

	ACPI_COMMON_FIELD_INFO
	u32                         value;              /* Value to store into Index register */

	ACPI_HANDLE                 index;              /* Index register */
	ACPI_HANDLE                 data;               /* Data register */

} ACPI_OBJECT_INDEX_FIELD;


typedef struct /* NOTIFY HANDLER */
{
	ACPI_OBJECT_COMMON_HEADER

	ACPI_NAMESPACE_NODE         *node;               /* Parent device */
	NOTIFY_HANDLER              handler;
	void                        *context;

} ACPI_OBJECT_NOTIFY_HANDLER;


/* Flags for address handler */

#define ADDR_HANDLER_DEFAULT_INSTALLED  0x1


typedef struct /* ADDRESS HANDLER */
{
	ACPI_OBJECT_COMMON_HEADER

	u8                          space_id;
	u16                         hflags;
	ADDRESS_SPACE_HANDLER       handler;

	ACPI_NAMESPACE_NODE         *node;              /* Parent device */
	void                        *context;
	ADDRESS_SPACE_SETUP         setup;
	union acpi_operand_obj      *region_list;       /* regions using this handler */
	union acpi_operand_obj      *next;

} ACPI_OBJECT_ADDR_HANDLER;


/*
 * The Reference object type is used for these opcodes:
 * Arg[0-6], Local[0-7], Index_op, Name_op, Zero_op, One_op, Ones_op, Debug_op
 */

typedef struct /* Reference - Local object type */
{
	ACPI_OBJECT_COMMON_HEADER

	u8                          target_type;        /* Used for Index_op */
	u16                         opcode;
	u32                         offset;             /* Used for Arg_op, Local_op, and Index_op */

	void                        *object;            /* Name_op=>HANDLE to obj, Index_op=>ACPI_OPERAND_OBJECT */
	ACPI_NAMESPACE_NODE         *node;
	union acpi_operand_obj      **where;

} ACPI_OBJECT_REFERENCE;


/*
 * Extra object is used as additional storage for types that
 * have AML code in their declarations (Term_args) that must be
 * evaluated at run time.
 *
 * Currently: Region and Field_unit types
 */

typedef struct /* EXTRA */
{
	ACPI_OBJECT_COMMON_HEADER
	u8                          byte_fill1;
	u16                         word_fill1;
	u32                         pcode_length;
	u8                          *pcode;
	ACPI_NAMESPACE_NODE         *method_REG;        /* _REG method for this region (if any) */
	void                        *region_context;    /* Region-specific data */

} ACPI_OBJECT_EXTRA;


/******************************************************************************
 *
 * ACPI_OPERAND_OBJECT  Descriptor - a giant union of all of the above
 *
 *****************************************************************************/

typedef union acpi_operand_obj
{
	ACPI_OBJECT_COMMON          common;
	ACPI_OBJECT_CACHE_LIST      cache;
	ACPI_OBJECT_INTEGER         integer;
	ACPI_OBJECT_STRING          string;
	ACPI_OBJECT_BUFFER          buffer;
	ACPI_OBJECT_PACKAGE         package;
	ACPI_OBJECT_FIELD_UNIT      field_unit;
	ACPI_OBJECT_DEVICE          device;
	ACPI_OBJECT_EVENT           event;
	ACPI_OBJECT_METHOD          method;
	ACPI_OBJECT_MUTEX           mutex;
	ACPI_OBJECT_REGION          region;
	ACPI_OBJECT_POWER_RESOURCE  power_resource;
	ACPI_OBJECT_PROCESSOR       processor;
	ACPI_OBJECT_THERMAL_ZONE    thermal_zone;
	ACPI_OBJECT_FIELD           field;
	ACPI_OBJECT_BANK_FIELD      bank_field;
	ACPI_OBJECT_INDEX_FIELD     index_field;
	ACPI_OBJECT_REFERENCE       reference;
	ACPI_OBJECT_NOTIFY_HANDLER  notify_handler;
	ACPI_OBJECT_ADDR_HANDLER    addr_handler;
	ACPI_OBJECT_EXTRA           extra;

} ACPI_OPERAND_OBJECT;

#endif /* _ACOBJECT_H */
