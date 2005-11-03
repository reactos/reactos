/*******************************************************************************
 *
 * Module Name: rsutils - Utilities for the resource manager
 *              $Revision: 1.1 $
 *
 ******************************************************************************/

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


#include "acpi.h"
#include "acnamesp.h"
#include "acresrc.h"


#define _COMPONENT          ACPI_RESOURCES
	 MODULE_NAME         ("rsutils")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_get_prt_method_data
 *
 * PARAMETERS:  Handle          - a handle to the containing object
 *              Ret_buffer      - a pointer to a buffer structure for the
 *                                  results
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get the _PRT value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_get_prt_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_OPERAND_OBJECT     *ret_obj;
	ACPI_STATUS             status;
	u32                     buffer_space_needed;


	/* already validated params, so we won't repeat here */

	buffer_space_needed = ret_buffer->length;

	/*
	 *  Execute the method, no parameters
	 */
	status = acpi_ns_evaluate_relative (handle, "_PRT", NULL, &ret_obj);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	if (!ret_obj) {
		/* Return object is required */

		return (AE_TYPE);
	}


	/*
	 * The return object will be a package, so check the
	 *  parameters.  If the return object is not a package,
	 *  then the underlying AML code is corrupt or improperly
	 *  written.
	 */
	if (ACPI_TYPE_PACKAGE != ret_obj->common.type) {
		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}

	/*
	 * Make the call to create a resource linked list from the
	 *  byte stream buffer that comes back from the _CRS method
	 *  execution.
	 */
	status = acpi_rs_create_pci_routing_table (ret_obj,
			  ret_buffer->pointer,
			  &buffer_space_needed);

	/*
	 * Tell the user how much of the buffer we have used or is needed
	 *  and return the final status.
	 */
	ret_buffer->length = buffer_space_needed;


	/* On exit, we must delete the object returned by evaluate_object */

cleanup:

	acpi_cm_remove_reference (ret_obj);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_get_crs_method_data
 *
 * PARAMETERS:  Handle          - a handle to the containing object
 *              Ret_buffer      - a pointer to a buffer structure for the
 *                                  results
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get the _CRS value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_get_crs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_OPERAND_OBJECT     *ret_obj;
	ACPI_STATUS             status;
	u32                     buffer_space_needed = ret_buffer->length;


	/* already validated params, so we won't repeat here */

	/*
	 *  Execute the method, no parameters
	 */
	status = acpi_ns_evaluate_relative (handle, "_CRS", NULL, &ret_obj);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	if (!ret_obj) {
		/* Return object is required */

		return (AE_TYPE);
	}

	/*
	 * The return object will be a buffer, but check the
	 *  parameters.  If the return object is not a buffer,
	 *  then the underlying AML code is corrupt or improperly
	 *  written.
	 */
	if (ACPI_TYPE_BUFFER != ret_obj->common.type) {
		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}

	/*
	 * Make the call to create a resource linked list from the
	 *  byte stream buffer that comes back from the _CRS method
	 *  execution.
	 */
	status = acpi_rs_create_resource_list (ret_obj,
			  ret_buffer->pointer,
			  &buffer_space_needed);



	/*
	 * Tell the user how much of the buffer we have used or is needed
	 *  and return the final status.
	 */
	ret_buffer->length = buffer_space_needed;


	/* On exit, we must delete the object returned by evaluate_object */

cleanup:

	acpi_cm_remove_reference (ret_obj);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_get_prs_method_data
 *
 * PARAMETERS:  Handle          - a handle to the containing object
 *              Ret_buffer      - a pointer to a buffer structure for the
 *                                  results
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get the _PRS value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_get_prs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_OPERAND_OBJECT     *ret_obj;
	ACPI_STATUS             status;
	u32                     buffer_space_needed = ret_buffer->length;


	/* already validated params, so we won't repeat here */

	/*
	 *  Execute the method, no parameters
	 */
	status = acpi_ns_evaluate_relative (handle, "_PRS", NULL, &ret_obj);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	if (!ret_obj) {
		/* Return object is required */

		return (AE_TYPE);
	}

	/*
	 * The return object will be a buffer, but check the
	 *  parameters.  If the return object is not a buffer,
	 *  then the underlying AML code is corrupt or improperly
	 *  written..
	 */
	if (ACPI_TYPE_BUFFER != ret_obj->common.type) {
		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}

	/*
	 * Make the call to create a resource linked list from the
	 *  byte stream buffer that comes back from the _CRS method
	 *  execution.
	 */
	status = acpi_rs_create_resource_list (ret_obj,
			  ret_buffer->pointer,
			  &buffer_space_needed);

	/*
	 * Tell the user how much of the buffer we have used or is needed
	 *  and return the final status.
	 */
	ret_buffer->length = buffer_space_needed;


	/* On exit, we must delete the object returned by evaluate_object */

cleanup:

	acpi_cm_remove_reference (ret_obj);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_set_srs_method_data
 *
 * PARAMETERS:  Handle          - a handle to the containing object
 *              In_buffer       - a pointer to a buffer structure of the
 *                                  parameter
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to set the _SRS of an object contained
 *              in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_set_srs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *in_buffer)
{
	ACPI_OPERAND_OBJECT     *params[2];
	ACPI_OPERAND_OBJECT     param_obj;
	ACPI_STATUS             status;
	u8                      *byte_stream = NULL;
	u32                     buffer_size_needed = 0;


	/* already validated params, so we won't repeat here */

	/*
	 * The In_buffer parameter will point to a linked list of
	 *  resource parameters.  It needs to be formatted into a
	 *  byte stream to be sent in as an input parameter.
	 */
	buffer_size_needed = 0;

	/*
	 * First call is to get the buffer size needed
	 */
	status = acpi_rs_create_byte_stream (in_buffer->pointer,
			   byte_stream,
			   &buffer_size_needed);
	/*
	 * We expect a return of AE_BUFFER_OVERFLOW
	 *  if not, exit with the error
	 */
	if (AE_BUFFER_OVERFLOW != status) {
		return (status);
	}

	/*
	 * Allocate the buffer needed
	 */
	byte_stream = acpi_cm_callocate(buffer_size_needed);
	if (NULL == byte_stream) {
		return (AE_NO_MEMORY);
	}

	/*
	 * Now call to convert the linked list into a byte stream
	 */
	status = acpi_rs_create_byte_stream (in_buffer->pointer,
			   byte_stream,
			   &buffer_size_needed);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	/*
	 *  Init the param object
	 */
	acpi_cm_init_static_object (&param_obj);

	/*
	 *  Method requires one parameter.  Set it up
	 */
	params [0] = &param_obj;
	params [1] = NULL;

	/*
	 *  Set up the parameter object
	 */
	param_obj.common.type   = ACPI_TYPE_BUFFER;
	param_obj.buffer.length = buffer_size_needed;
	param_obj.buffer.pointer = byte_stream;

	/*
	 *  Execute the method, no return value
	 */
	status = acpi_ns_evaluate_relative (handle, "_SRS", params, NULL);

	/*
	 *  Clean up and return the status from Acpi_ns_evaluate_relative
	 */

cleanup:

	acpi_cm_free (byte_stream);
	return (status);
}

