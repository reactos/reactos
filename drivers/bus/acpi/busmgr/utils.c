/*
 *  acpi_utils.c - ACPI Utility Functions ($Revision: 10 $)
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <precomp.h>
#include <ntstrsafe.h>

#define NDEBUG
#include <debug.h>

 /* Modified for ReactOS and latest ACPICA
  * Copyright (C)2009  Samuel Serapion 
  */

#define _COMPONENT		ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		("acpi_utils")

static void
acpi_util_eval_error(ACPI_HANDLE h, ACPI_STRING p, ACPI_STATUS s)
{
#ifdef ACPI_DEBUG_OUTPUT
	char prefix[80] = {'\0'};
	ACPI_BUFFER buffer = {sizeof(prefix), prefix};
	AcpiGetName(h, ACPI_FULL_PATHNAME, &buffer);
	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Evaluate [%s.%s]: %s\n",
		(char *) prefix, p, AcpiFormatException(s)));
#else
	return;
#endif
}


/* --------------------------------------------------------------------------
                            Object Evaluation Helpers
   -------------------------------------------------------------------------- */


ACPI_STATUS
acpi_extract_package (
	ACPI_OBJECT	*package,
	ACPI_BUFFER	*format,
	ACPI_BUFFER	*buffer)
{
	UINT32			size_required = 0;
	UINT32			tail_offset = 0;
	char			*format_string = NULL;
	UINT32			format_count = 0;
	UINT32			i = 0;
	UINT8			*head = NULL;
	UINT8			*tail = NULL;

	if (!package || (package->Type != ACPI_TYPE_PACKAGE) || (package->Package.Count < 1)) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid 'package' argument\n"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (!format || !format->Pointer || (format->Length < 1)) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid 'format' argument\n"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (!buffer) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid 'buffer' argument\n"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	format_count = (format->Length/sizeof(char)) - 1;
	if (format_count > package->Package.Count) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Format specifies more objects [%d] than exist in package [%d].", format_count, package->package.count));
		return_ACPI_STATUS(AE_BAD_DATA);
	}

	format_string = format->Pointer;

	/*
	 * Calculate size_required.
	 */
	for (i=0; i<format_count; i++) {

		ACPI_OBJECT *element = &(package->Package.Elements[i]);

		if (!element) {
			return_ACPI_STATUS(AE_BAD_DATA);
		}

		switch (element->Type) {

		case ACPI_TYPE_INTEGER:
			switch (format_string[i]) {
			case 'N':
				size_required += sizeof(ACPI_INTEGER);
				tail_offset += sizeof(ACPI_INTEGER);
				break;
			case 'S':
				size_required += sizeof(char*) + sizeof(ACPI_INTEGER) + sizeof(char);
				tail_offset += sizeof(char*);
				break;
			default:
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid package element [%d]: got number, expecting [%c].\n", i, format_string[i]));
				return_ACPI_STATUS(AE_BAD_DATA);
				break;
			}
			break;

		case ACPI_TYPE_STRING:
		case ACPI_TYPE_BUFFER:
			switch (format_string[i]) {
			case 'S':
				size_required += sizeof(char*) + (element->String.Length * sizeof(char)) + sizeof(char);
				tail_offset += sizeof(char*);
				break;
			case 'B':
				size_required += sizeof(UINT8*) + (element->Buffer.Length * sizeof(UINT8));
				tail_offset += sizeof(UINT8*);
				break;
			default:
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid package element [%d] got string/buffer, expecting [%c].\n", i, format_string[i]));
				return_ACPI_STATUS(AE_BAD_DATA);
				break;
			}
			break;

		case ACPI_TYPE_PACKAGE:
		default:
			ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Found unsupported element at index=%d\n", i));
			/* TBD: handle nested packages... */
			return_ACPI_STATUS(AE_SUPPORT);
			break;
		}
	}

	/*
	 * Validate output buffer.
	 */
	if (buffer->Length < size_required) {
		buffer->Length = size_required;
		return_ACPI_STATUS(AE_BUFFER_OVERFLOW);
	}
	else if (buffer->Length != size_required || !buffer->Pointer) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	head = buffer->Pointer;
	tail = ((PUCHAR)buffer->Pointer) + tail_offset;

	/*
	 * Extract package data.
	 */
	for (i=0; i<format_count; i++) {

		UINT8 **pointer = NULL;
		ACPI_OBJECT *element = &(package->Package.Elements[i]);

		if (!element) {
			return_ACPI_STATUS(AE_BAD_DATA);
		}

		switch (element->Type) {

		case ACPI_TYPE_INTEGER:
			switch (format_string[i]) {
			case 'N':
				*((ACPI_INTEGER*)head) = element->Integer.Value;
				head += sizeof(ACPI_INTEGER);
				break;
			case 'S':
				pointer = (UINT8**)head;
				*pointer = tail;
				*((ACPI_INTEGER*)tail) = element->Integer.Value;
				head += sizeof(ACPI_INTEGER*);
				tail += sizeof(ACPI_INTEGER);
				/* NULL terminate string */
				*tail = (char)0;
				tail += sizeof(char);
				break;
			default:
				/* Should never get here */
				break;
			}
			break;

		case ACPI_TYPE_STRING:
		case ACPI_TYPE_BUFFER:
			switch (format_string[i]) {
			case 'S':
				pointer = (UINT8**)head;
				*pointer = tail;
				memcpy(tail, element->String.Pointer, element->String.Length);
				head += sizeof(char*);
				tail += element->String.Length * sizeof(char);
				/* NULL terminate string */
				*tail = (char)0;
				tail += sizeof(char);
				break;
			case 'B':
				pointer = (UINT8**)head;
				*pointer = tail;
				memcpy(tail, element->Buffer.Pointer, element->Buffer.Length);
				head += sizeof(UINT8*);
				tail += element->Buffer.Length * sizeof(UINT8);
				break;
			default:
				/* Should never get here */
				break;
			}
			break;

		case ACPI_TYPE_PACKAGE:
			/* TBD: handle nested packages... */
		default:
			/* Should never get here */
			break;
		}
	}

	return_ACPI_STATUS(AE_OK);
}


ACPI_STATUS
acpi_evaluate_integer (
	ACPI_HANDLE		handle,
	ACPI_STRING		pathname,
	ACPI_OBJECT_LIST	*arguments,
	unsigned long long		*data)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OBJECT	element;
	ACPI_BUFFER	buffer = {sizeof(ACPI_OBJECT), &element};

	ACPI_FUNCTION_TRACE("acpi_evaluate_integer");

	if (!data)
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	status = AcpiEvaluateObject(handle, pathname, arguments, &buffer);
	if (ACPI_FAILURE(status)) {
		acpi_util_eval_error(handle, pathname, status);
		return_ACPI_STATUS(status);
	}

	if (element.Type != ACPI_TYPE_INTEGER) {
		acpi_util_eval_error(handle, pathname, AE_BAD_DATA);
		return_ACPI_STATUS(AE_BAD_DATA);
	}

	*data = element.Integer.Value;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Return value [%lu]\n", *data));

	return_ACPI_STATUS(AE_OK);
}


ACPI_STATUS
acpi_evaluate_reference (
	ACPI_HANDLE		handle,
	ACPI_STRING		pathname,
	ACPI_OBJECT_LIST	*arguments,
	struct acpi_handle_list	*list)
{
	ACPI_STATUS		status = AE_OK;
	ACPI_OBJECT	*package = NULL;
	ACPI_OBJECT	*element = NULL;
	ACPI_BUFFER	buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	UINT32			i = 0;

	ACPI_FUNCTION_TRACE("acpi_evaluate_reference");

	if (!list) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Evaluate object. */

	status = AcpiEvaluateObject(handle, pathname, arguments, &buffer);
	if (ACPI_FAILURE(status))
		goto end;

	package = (ACPI_OBJECT *) buffer.Pointer;

	if ((buffer.Length == 0) || !package) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, 
			"No return object (len %X ptr %p)\n", 
			buffer.Length, package));
		status = AE_BAD_DATA;
		acpi_util_eval_error(handle, pathname, status);
		goto end;
	}
	if (package->Type != ACPI_TYPE_PACKAGE) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, 
			"Expecting a [Package], found type %X\n", 
			package->Type));
		status = AE_BAD_DATA;
		acpi_util_eval_error(handle, pathname, status);
		goto end;
	}
	if (!package->Package.Count) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, 
			"[Package] has zero elements (%p)\n", 
			package));
		status = AE_BAD_DATA;
		acpi_util_eval_error(handle, pathname, status);
		goto end;
	}

	if (package->Package.Count > ACPI_MAX_HANDLES) {
		return AE_NO_MEMORY;
	}
	list->count = package->Package.Count;

	/* Extract package data. */

	for (i = 0; i < list->count; i++) {

		element = &(package->Package.Elements[i]);

		if (element->Type != ACPI_TYPE_LOCAL_REFERENCE) {
			status = AE_BAD_DATA;
			ACPI_DEBUG_PRINT((ACPI_DB_ERROR, 
				"Expecting a [Reference] package element, found type %X\n",
				element->type));
			acpi_util_eval_error(handle, pathname, status);
			break;
		}
		
		if (!element->Reference.Handle) {
			ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Invalid reference in"
			       " package %s\n", pathname));
			status = AE_NULL_ENTRY;
			break;
		}
		/* Get the  ACPI_HANDLE. */

		list->handles[i] = element->Reference.Handle;
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Found reference [%p]\n",
			list->handles[i]));
	}

end:
	if (ACPI_FAILURE(status)) {
		list->count = 0;
		//ExFreePool(list->handles);
	}

    if (buffer.Pointer)
        AcpiOsFree(buffer.Pointer);

	return_ACPI_STATUS(status);
}

NTSTATUS
acpi_create_registry_table(HANDLE ParentKeyHandle, ACPI_TABLE_HEADER *OutTable, PCWSTR KeyName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING HardwareKeyName, ValueName;
    ANSI_STRING HardwareKeyNameA;
    HANDLE KeyHandle = NULL, SubKeyHandle = NULL;
    NTSTATUS Status;
    char OemId[7] = { 0 }; /* exactly one byte more than ACPI_TABLE_HEADER->OemId */
    char OemTableId[9] = { 0 }; /* exactly one byte more than ACPI_TABLE_HEADER->OemTableId */
    WCHAR OemRevision[9] = { 0 }; /* enough to accept hex DWORD */

    C_ASSERT(sizeof(OemId) == RTL_FIELD_SIZE(ACPI_TABLE_HEADER, OemId) + 1);
    C_ASSERT(sizeof(OemTableId) == RTL_FIELD_SIZE(ACPI_TABLE_HEADER, OemTableId) + 1);
    /* Copy OEM data from the table */
    RtlCopyMemory(OemId, OutTable->OemId, sizeof(OutTable->OemId));
    RtlCopyMemory(OemTableId, OutTable->OemTableId, sizeof(OutTable->OemTableId));
    /* Create table subkey */
    RtlInitUnicodeString(&HardwareKeyName, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &HardwareKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               ParentKeyHandle,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwCreateKey() for %ws failed (Status 0x%08lx)\n", KeyName, Status);
        return Status;
    }

    if (OutTable->OemRevision != 0)
    {
        /* We have OEM info in table, so create other OEM subkeys */
        RtlInitAnsiString(&HardwareKeyNameA, OemId);
        Status = RtlAnsiStringToUnicodeString(&HardwareKeyName, &HardwareKeyNameA, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlAnsiStringToUnicodeString() for %s failed (Status 0x%08lx)\n", HardwareKeyNameA, Status);
            ZwClose(KeyHandle);
            return Status;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &HardwareKeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   KeyHandle,
                                   NULL);
        Status = ZwCreateKey(&SubKeyHandle,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        RtlFreeUnicodeString(&HardwareKeyName);
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateKey() for %s failed (Status 0x%08lx)\n", HardwareKeyNameA, Status);
            return Status;
        }
        KeyHandle = SubKeyHandle;

        RtlInitAnsiString(&HardwareKeyNameA, OemTableId);
        Status = RtlAnsiStringToUnicodeString(&HardwareKeyName, &HardwareKeyNameA, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlAnsiStringToUnicodeString() for %s failed (Status 0x%08lx)\n", HardwareKeyNameA, Status);
            ZwClose(KeyHandle);
            return Status;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &HardwareKeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   KeyHandle,
                                   NULL);
        Status = ZwCreateKey(&SubKeyHandle,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        RtlFreeUnicodeString(&HardwareKeyName);
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateKey() for %s failed (Status 0x%08lx)\n", HardwareKeyNameA, Status);
            return Status;
        }
        KeyHandle = SubKeyHandle;

        Status = RtlStringCbPrintfW(OemRevision,
                                    sizeof(OemRevision),
                                    L"%08X",
                                    OutTable->OemRevision);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlStringCbPrintfW() for 0x%08lx failed (Status 0x%08lx)\n", OutTable->OemRevision, Status);
            ZwClose(KeyHandle);
            return Status;
        }
        RtlInitUnicodeString(&HardwareKeyName, OemRevision);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &HardwareKeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   KeyHandle,
                                   NULL);
        Status = ZwCreateKey(&SubKeyHandle,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateKey() for %ws failed (Status 0x%08lx)\n", KeyName, Status);
            return Status;
        }
        KeyHandle = SubKeyHandle;
    }
    /* Table reg value name is always '00000000' */
    RtlInitUnicodeString(&ValueName,
                         L"00000000");
    Status = ZwSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_BINARY,
                           OutTable,
                           OutTable->Length);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetValueKey() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
acpi_create_volatile_registry_tables(void)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING HardwareKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\ACPI");
    HANDLE KeyHandle = NULL;
    NTSTATUS Status;
    ACPI_STATUS AcpiStatus;
    ACPI_TABLE_HEADER *OutTable;
    ACPI_PHYSICAL_ADDRESS RsdpAddress;
    ACPI_TABLE_RSDP *Rsdp;
    ACPI_PHYSICAL_ADDRESS Address;
    UINT32 TableEntrySize;

    /* Create Main Hardware ACPI key*/
    InitializeObjectAttributes(&ObjectAttributes,
                               &HardwareKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwCreateKey() for ACPI failed (Status 0x%08lx)\n", Status);
        return Status;
    }
    /* Read DSDT table */
    AcpiStatus = AcpiGetTable(ACPI_SIG_DSDT, 0, &OutTable);
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("AcpiGetTable() for DSDT failed (Status 0x%08lx)\n", AcpiStatus);
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }
    /* Dump DSDT table */
    Status = acpi_create_registry_table(KeyHandle, OutTable, L"DSDT");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("acpi_dump_table_to_registry() for DSDT failed (Status 0x%08lx)\n", Status);
        goto done;
    }
    /* Read FACS table */
    AcpiStatus = AcpiGetTable(ACPI_SIG_FACS, 0, &OutTable);
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("AcpiGetTable() for FACS failed (Status 0x%08lx)\n", AcpiStatus);
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }
    /* Dump FACS table */
    Status = acpi_create_registry_table(KeyHandle, OutTable, L"FACS");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("acpi_dump_table_to_registry() for FACS failed (Status 0x%08lx)\n", Status);
        goto done;
    }
    /* Read FACS table */
    AcpiStatus = AcpiGetTable(ACPI_SIG_FADT, 0, &OutTable);
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("AcpiGetTable() for FADT failed (Status 0x%08lx)\n", AcpiStatus);
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }
    /* Dump FADT table */
    Status = acpi_create_registry_table(KeyHandle, OutTable, L"FADT");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("acpi_dump_table_to_registry() for FADT failed (Status 0x%08lx)\n", Status);
        goto done;
    }
    /* This is a rough copy from ACPICA reading of RSDT/XSDT and added to avoid patching acpica */
    RsdpAddress = AcpiOsGetRootPointer();
    /* Map the entire RSDP and extract the address of the RSDT or XSDT */
    Rsdp = AcpiOsMapMemory(RsdpAddress, sizeof(ACPI_TABLE_RSDP));
    if (!Rsdp)
    {
        DPRINT1("AcpiOsMapMemory() failed\n");
        Status = STATUS_NO_MEMORY;
        goto done;
    }
    /* Use XSDT if present and not overridden. Otherwise, use RSDT */
    if ((Rsdp->Revision > 1) &&
        Rsdp->XsdtPhysicalAddress &&
        !AcpiGbl_DoNotUseXsdt)
    {
        /*
        * RSDP contains an XSDT (64-bit physical addresses). We must use
        * the XSDT if the revision is > 1 and the XSDT pointer is present,
        * as per the ACPI specification.
        */
        Address = (ACPI_PHYSICAL_ADDRESS)Rsdp->XsdtPhysicalAddress;
        TableEntrySize = ACPI_XSDT_ENTRY_SIZE;
    }
    else
    {
        /* Root table is an RSDT (32-bit physical addresses) */
        Address = (ACPI_PHYSICAL_ADDRESS)Rsdp->RsdtPhysicalAddress;
        TableEntrySize = ACPI_RSDT_ENTRY_SIZE;
    }
    /*
    * It is not possible to map more than one entry in some environments,
    * so unmap the RSDP here before mapping other tables
    */
    AcpiOsUnmapMemory(Rsdp, sizeof(ACPI_TABLE_RSDP));
    OutTable = AcpiOsMapMemory(Address, TableEntrySize);
    if (!OutTable)
    {
        DPRINT1("AcpiOsMapMemory() failed\n");
        Status = STATUS_NO_MEMORY;
        goto done;
    }
    /* Dump RSDT table */
    Status = acpi_create_registry_table(KeyHandle, OutTable, L"RSDT");
    AcpiOsUnmapMemory(OutTable, TableEntrySize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("acpi_dump_table_to_registry() for RSDT failed (Status 0x%08lx)\n", Status);
    }

done:
    ZwClose(KeyHandle);
    return Status;
}
