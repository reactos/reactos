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

#include <ntddk.h>

#include <acpi.h>
#include <acpi_bus.h>
#include <acpi_drivers.h>
#include <glue.h>

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
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid package element [%d]: got number, expecing [%c].\n", i, format_string[i]));
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
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid package element [%d] got string/buffer, expecing [%c].\n", i, format_string[i]));
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
	tail = buffer->Pointer + tail_offset;

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

	AcpiOsFree(buffer.Pointer);

	return_ACPI_STATUS(status);
}


