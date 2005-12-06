/******************************************************************************
 *
 * Module Name: cmalloc - local memory allocation routines
 *              $Revision: 1.1 $
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


#include "acpi.h"
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acglobal.h"

#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmalloc")


/*****************************************************************************
 *
 * FUNCTION:    _Cm_allocate
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: The subsystem's equivalent of malloc.
 *
 ****************************************************************************/

void *
_cm_allocate (
	u32                     size,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line)
{
	void                    *address = NULL;


	/* Check for an inadvertent size of zero bytes */

	if (!size) {
		_REPORT_ERROR (module, line, component,
				("Cm_allocate: Attempt to allocate zero bytes\n"));
		size = 1;
	}

	address = acpi_os_allocate (size);
	if (!address) {
		/* Report allocation error */

		_REPORT_ERROR (module, line, component,
				("Cm_allocate: Could not allocate size %X\n", size));

		return (NULL);
	}


	return (address);
}


/*****************************************************************************
 *
 * FUNCTION:    _Cm_callocate
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: Subsystem equivalent of calloc.
 *
 ****************************************************************************/

void *
_cm_callocate (
	u32                     size,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line)
{
	void                    *address = NULL;


	/* Check for an inadvertent size of zero bytes */

	if (!size) {
		_REPORT_ERROR (module, line, component,
				("Cm_callocate: Attempt to allocate zero bytes\n"));
		return (NULL);
	}


	address = acpi_os_callocate (size);

	if (!address) {
		/* Report allocation error */

		_REPORT_ERROR (module, line, component,
				("Cm_callocate: Could not allocate size %X\n", size));
		return (NULL);
	}


	return (address);
}


/*****************************************************************************
 *
 * FUNCTION:    _Cm_free
 *
 * PARAMETERS:  Address             - Address of the memory to deallocate
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      None
 *
 * DESCRIPTION: Frees the memory at Address
 *
 ****************************************************************************/

void
_cm_free (
	void                    *address,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line)
{

	if (NULL == address) {
		_REPORT_ERROR (module, line, component,
			("_Cm_free: Trying to delete a NULL address\n"));

		return;
	}


	acpi_os_free (address);

	return;
}


