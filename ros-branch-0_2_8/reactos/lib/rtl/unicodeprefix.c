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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
RtlRemoveUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	)
{
	UNIMPLEMENTED;
}

/* EOF */
