/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Prefix implementation
 * FILE:              lib/rtl/unicodeprfx.c
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
* @unimplemented
*/
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING FullName,
	ULONG CaseInsensitiveIndex
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
NTAPI
RtlInitializeUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
BOOLEAN
NTAPI
RtlInsertUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING Prefix,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
NTAPI
RtlRemoveUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	)
{
	UNIMPLEMENTED;
}

/* EOF */
