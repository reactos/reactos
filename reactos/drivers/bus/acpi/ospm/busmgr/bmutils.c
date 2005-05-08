/*****************************************************************************
 *
 * Module Name: bmutils.c
 *   $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 Andrew Grover
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


#include <acpi.h>
#include "bm.h"


#define _COMPONENT		ACPI_BUS_MANAGER
	MODULE_NAME		("bmutils")


#ifdef ACPI_DEBUG
#define DEBUG_EVAL_ERROR(l,h,p,s)    bm_print_eval_error(l,h,p,s)
#else
#define DEBUG_EVAL_ERROR(l,h,p,s)
#endif


/****************************************************************************
 *                            External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_print_eval_error
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

void
bm_print_eval_error (
	u32                     debug_level,
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	ACPI_STATUS             status)
{
	ACPI_BUFFER             buffer;
	ACPI_STRING             status_string = NULL;

	buffer.length = 256;
	buffer.pointer = acpi_os_callocate(buffer.length);
	if (!buffer.pointer) {
		return;
	}

	status_string = acpi_cm_format_exception(status);

	status = acpi_get_name(acpi_handle, ACPI_FULL_PATHNAME, &buffer);
	if (ACPI_FAILURE(status)) {
		DEBUG_PRINT(debug_level, ("Evaluate object [0x%08x], %s\n", acpi_handle, status_string));
		return;
	}

	if (pathname) {
		DEBUG_PRINT(ACPI_INFO, ("Evaluate object [%s.%s], %s\n", buffer.pointer, pathname, status_string));
	}
	else {
		DEBUG_PRINT(ACPI_INFO, ("Evaluate object [%s], %s\n", buffer.pointer, status_string));
	}

	acpi_os_free(buffer.pointer);
}


/****************************************************************************
 *
 * FUNCTION:    bm_copy_to_buffer
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_copy_to_buffer (
	ACPI_BUFFER             *buffer,
	void                    *data,
	u32                     length)
{
	FUNCTION_TRACE("bm_copy_to_buffer");

	if (!buffer || (!buffer->pointer) || !data || (length == 0)) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (length > buffer->length) {
		buffer->length = length;
		return_ACPI_STATUS(AE_BUFFER_OVERFLOW);
	}

	buffer->length = length;
	MEMCPY(buffer->pointer, data, length);

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_cast_buffer
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_cast_buffer (
	ACPI_BUFFER             *buffer,
	void                    **pointer,
	u32                     length)
{
	FUNCTION_TRACE("bm_cast_buffer");

	if (!buffer || !buffer->pointer || !pointer || length == 0) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (length > buffer->length) {
		return_ACPI_STATUS(AE_BAD_DATA);
	}

	*pointer = buffer->pointer;

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_extract_package_data
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

/*
  TODO: Don't assume numbers (in ASL) are 32-bit values!!!!  (IA64)
  TODO: Issue with 'assumed' types coming out of interpreter...
        (e.g. toshiba _BIF)
*/

ACPI_STATUS
bm_extract_package_data (
	ACPI_OBJECT             *package,
	ACPI_BUFFER             *package_format,
	ACPI_BUFFER             *buffer)
{
	ACPI_STATUS             status = AE_OK;
	u8                      *head = NULL;
	u8                      *tail = NULL;
	u8                      **pointer = NULL;
	u32                     tail_offset = 0;
	ACPI_OBJECT             *element = NULL;
	u32                     size_required = 0;
	char*                   format = NULL;
	u32                     format_count = 0;
	u32                     i = 0;

	FUNCTION_TRACE("bm_extract_package_data");

	if (!package || (package->type != ACPI_TYPE_PACKAGE) ||
		(package->package.count == 0) || !package_format ||
		(package_format->length < 1) || 
		(!package_format->pointer) || !buffer) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	format_count = package_format->length - 1;

	if (format_count > package->package.count) {
		DEBUG_PRINT(ACPI_WARN, ("Format specifies more objects [%d] than exist in package [%d].", format_count, package->package.count));
		return_ACPI_STATUS(AE_BAD_DATA);
	}

	format = (char*)package_format->pointer;

	/*
	 * Calculate size_required.
	 */
	for (i=0; i<format_count; i++) {
		element = &(package->package.elements[i]);

		switch (element->type) {

		case ACPI_TYPE_INTEGER:
			switch (format[i]) {
			case 'N':
				size_required += sizeof(ACPI_INTEGER);
				tail_offset += sizeof(ACPI_INTEGER);
				break;
			case 'S':
				size_required += sizeof(u8*) + 
					sizeof(ACPI_INTEGER) + 1;
				tail_offset += sizeof(ACPI_INTEGER);
				break;
			default:
				DEBUG_PRINT(ACPI_WARN, ("Invalid package element [%d]: got number, expecing [%c].\n", i, format[i]));
				return_ACPI_STATUS(AE_BAD_DATA);
				break;
			}
			break;

		case ACPI_TYPE_STRING:
		case ACPI_TYPE_BUFFER:
			switch (format[i]) {
			case 'S':
				size_required += sizeof(u8*) + 
					element->string.length + 1;
				tail_offset += sizeof(u8*);
				break;
			case 'B':
				size_required += sizeof(u8*) + 
					element->buffer.length;
				tail_offset += sizeof(u8*);
				break;
			default:
				DEBUG_PRINT(ACPI_WARN, ("Invalid package element [%d] got string/buffer, expecing [%c].\n", i, format[i]));
				return_ACPI_STATUS(AE_BAD_DATA);
				break;
			}
			break;

		case ACPI_TYPE_PACKAGE:
		default:
			/* TODO: handle nested packages... */
			return_ACPI_STATUS(AE_SUPPORT);
			break;
		}
	}

	if (size_required > buffer->length) {
		buffer->length = size_required;
		return_ACPI_STATUS(AE_BUFFER_OVERFLOW);
	}

	buffer->length = size_required;

	if (!buffer->pointer) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	head = buffer->pointer;
	tail = buffer->pointer + tail_offset;

	/*
	 * Extract package data:
	 */
	for (i=0; i<format_count; i++) {

		element = &(package->package.elements[i]);

		switch (element->type) {

		case ACPI_TYPE_INTEGER:
			switch (format[i]) {
			case 'N':
				*((ACPI_INTEGER*)head) = 
					element->integer.value;
				head += sizeof(ACPI_INTEGER);
				break;
			case 'S':
				pointer = (u8**)head;
				*pointer = tail;
				*((ACPI_INTEGER*)tail) = 
					element->integer.value;
				head += sizeof(ACPI_INTEGER*);
				tail += sizeof(ACPI_INTEGER);
				/* NULL terminate string */
				*tail = 0;
				tail++;
				break;
			default:
				/* Should never get here */
				break;
			}
			break;

		case ACPI_TYPE_STRING:
		case ACPI_TYPE_BUFFER:
			switch (format[i]) {
			case 'S':
				pointer = (u8**)head;
				*pointer = tail;
				memcpy(tail, element->string.pointer, 
					element->string.length);
				head += sizeof(u8*);
				tail += element->string.length;
				/* NULL terminate string */
				*tail = 0;
				tail++;
				break;
			case 'B':
				pointer = (u8**)head;
				*pointer = tail;
				memcpy(tail, element->buffer.pointer, 
					element->buffer.length);
				head += sizeof(u8*);
				tail += element->buffer.length;
				break;
			default:
				/* Should never get here */
				break;
			}
			break;

		case ACPI_TYPE_PACKAGE:
			/* TODO: handle nested packages... */
		default:
			/* Should never get here */
			break;
		}
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_evaluate_object
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      AE_OK
 *              AE_BUFFER_OVERFLOW  Evaluated object returned data, but 
 *                                  caller did not provide buffer.
 *
 * DESCRIPTION: Helper for acpi_evaluate_object that handles buffer
 *              allocation.  Note that the caller is responsible for 
 *              freeing buffer->pointer!
 *
 ****************************************************************************/

ACPI_STATUS
bm_evaluate_object (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	ACPI_OBJECT_LIST        *arguments,
	ACPI_BUFFER             *buffer)
{
	ACPI_STATUS             status = AE_OK;

	FUNCTION_TRACE("bm_evaluate_object");

	/* If caller provided a buffer it must be unallocated/zero'd. */
	if ((buffer) && (buffer->length != 0 || buffer->pointer)) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Evalute Object:
	 * ---------------
	 * The first attempt is just to get the size of the object data 
	 * (that is unless there's no return data, e.g. _INI); the second 
	 * gets the data.
	 */
	status = acpi_evaluate_object(acpi_handle, pathname, arguments, buffer);
	if (ACPI_SUCCESS(status)) {
		return_ACPI_STATUS(status);
	}

	else if ((buffer) && (status == AE_BUFFER_OVERFLOW)) {

		/* Gotta allocate -- CALLER MUST FREE! */
		buffer->pointer = acpi_os_callocate(buffer->length);
		if (!buffer->pointer) {
			return_ACPI_STATUS(AE_NO_MEMORY);
		}

		/* Re-evaluate -- this time it should work */
		status = acpi_evaluate_object(acpi_handle, pathname, 
			arguments, buffer);
	}

	if (ACPI_FAILURE(status)) {
		DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
		if (buffer && buffer->pointer) {
			acpi_os_free(buffer->pointer); 
			buffer->pointer = NULL;
			buffer->length = 0;
		}
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_evaluate_simple_integer
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_evaluate_simple_integer (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	u32                     *data)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OBJECT             *element = NULL;
	ACPI_BUFFER             buffer;

	FUNCTION_TRACE("bm_evaluate_simple_integer");

	if (!data) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&buffer, 0, sizeof(ACPI_BUFFER));

	/*
	 * Evaluate Object:
	 * ----------------
	 */
	status = bm_evaluate_object(acpi_handle, pathname, NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		goto end;
	}

	/*
	 * Validate Data:
	 * --------------
	 */
	status = bm_cast_buffer(&buffer, (void**)&element, 
		sizeof(ACPI_OBJECT));
	if (ACPI_FAILURE(status)) {
		DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
		goto end;
	}

	if (element->type != ACPI_TYPE_INTEGER) {
		status = AE_BAD_DATA;
		DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
		goto end;
	}

	*data = element->integer.value;

end:
	acpi_os_free(buffer.pointer);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_evaluate_reference_list
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_evaluate_reference_list (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	BM_HANDLE_LIST          *reference_list)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OBJECT             *package = NULL;
	ACPI_OBJECT             *element = NULL;
	ACPI_HANDLE		reference_handle = NULL;
	ACPI_BUFFER             buffer;
	u32                     i = 0;

	FUNCTION_TRACE("bm_evaluate_reference_list");

	if (!reference_list) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&buffer, 0, sizeof(ACPI_BUFFER));

	/*
	 * Evaluate Object:
	 * ----------------
	 */
	status = bm_evaluate_object(acpi_handle, pathname, NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		goto end;
	}

	/*
	 * Validate Package:
	 * -----------------
	 */
	status = bm_cast_buffer(&buffer, (void**)&package, 
		sizeof(ACPI_OBJECT));
	if (ACPI_FAILURE(status)) {
		DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
		goto end;
	}

	if (package->type != ACPI_TYPE_PACKAGE) {
		status = AE_BAD_DATA;
		DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
		goto end;
	}

	if (package->package.count > BM_HANDLES_MAX) {
		package->package.count = BM_HANDLES_MAX;
	}

	/*
	 * Parse Package Data:
	 * -------------------
	 */
	for (i = 0; i < package->package.count; i++) {

		element = &(package->package.elements[i]);

		if (!element || (element->type != ACPI_TYPE_STRING)) {
			status = AE_BAD_DATA;
			DEBUG_PRINT(ACPI_WARN, ("Invalid element in package (not a device reference).\n"));
			DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
			break;
		}

		/*
		 * Resolve reference string (e.g. "\_PR_.CPU_") to an
		 * ACPI_HANDLE.
		 */
		status = acpi_get_handle(acpi_handle, 
			element->string.pointer, &reference_handle);
		if (ACPI_FAILURE(status)) {
			status = AE_BAD_DATA;
			DEBUG_PRINT(ACPI_WARN, ("Unable to resolve device reference [%s].\n", element->string.pointer));
			DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
			break;
		}

		/*
		 * Resolve ACPI_HANDLE to BM_HANDLE.
		 */
		status = bm_get_handle(reference_handle, 
			&(reference_list->handles[i]));
		if (ACPI_FAILURE(status)) {
			status = AE_BAD_DATA;
			DEBUG_PRINT(ACPI_WARN, ("Unable to resolve device reference for [0x%08x].\n", reference_handle));
			DEBUG_EVAL_ERROR(ACPI_WARN, acpi_handle, pathname, status);
			break;
		}

		DEBUG_PRINT(ACPI_INFO, ("Resolved reference [%s]->[0x%08x]->[0x%02x]\n", element->string.pointer, reference_handle, reference_list->handles[i]));

		(reference_list->count)++;
	}

end:
	acpi_os_free(buffer.pointer);

	return_ACPI_STATUS(status);
}


