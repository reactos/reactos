/******************************************************************************
 *
 * Module Name: bmsearch.c
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
	MODULE_NAME		("bmsearch")


/****************************************************************************
 *                            External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_compare
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_compare (
	BM_DEVICE               *device,
	BM_DEVICE_ID            *criteria)
{
	if (!device || !criteria) {
		return AE_BAD_PARAMETER;
	}

	/* 
	 * Present?
	 * --------
	 * We're only going to match on devices that are present.
	 * TODO: Optimize in bm_search (don't have to call here).
	 */
	if (!BM_DEVICE_PRESENT(device)) {
		return AE_NOT_FOUND;
	}

	/* 
	 * type?
	 */
	if (criteria->type && !(criteria->type & device->id.type)) {
		return AE_NOT_FOUND;
	}

	/* 
	 * hid?
	 */
	if ((criteria->hid[0]) && (0 != STRNCMP(criteria->hid, 
		device->id.hid, sizeof(BM_DEVICE_HID)))) {
		return AE_NOT_FOUND;
	}

	/* 
	 * adr?
	 */
	if ((criteria->adr) && (criteria->adr != device->id.adr)) {
		return AE_NOT_FOUND;
	}

	return AE_OK;
}


/****************************************************************************
 *
 * FUNCTION:    bm_search
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      AE_BAD_PARAMETER- invalid input parameter
 *              AE_NOT_EXIST    - start_device_handle doesn't exist
 *              AE_NOT_FOUND    - no matches to Search_info.criteria found
 *              AE_OK           - success
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_search(
	BM_HANDLE               device_handle,
	BM_DEVICE_ID            *criteria,
	BM_HANDLE_LIST          *results)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;

	FUNCTION_TRACE("bm_search");

	if (!criteria || !results) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	results->count = 0;

	/*
	 * Locate Starting Point:
	 * ----------------------
	 * Locate the node in the hierarchy where we'll begin our search.
	 */
	status = bm_get_node(device_handle, 0, &node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Parse Hierarchy:
	 * ----------------
	 * Parse through the node hierarchy looking for matches.
	 */
	while (node && (results->count<=BM_HANDLES_MAX)) {
		/*
		 * Depth-first:
		 * ------------
		 * Searches are always performed depth-first.
		 */
		if (node->scope.head) {
			status = bm_compare(&(node->device), criteria);
			if (ACPI_SUCCESS(status)) {
				results->handles[results->count++] = 
					node->device.handle;
			}
			node = node->scope.head;
		}

		/*
		 * Now Breadth:
		 * ------------
		 * Search all peers until scope is exhausted.
		 */
		else {
			status = bm_compare(&(node->device), criteria);
			if (ACPI_SUCCESS(status)) {
				results->handles[results->count++] = 
					node->device.handle;
			}

			/*
			 * Locate Next Device:
			 * -------------------
			 * The next node is either a peer at this level 
			 * (node->next is valid), or we work are way back 
			 * up the tree until we either find a non-parsed 
			 * peer or hit the top (node->parent is NULL).
			 */
			while (!node->next && node->parent) {
				node = node->parent;
			}
			node = node->next;
		}
	}

	if (results->count == 0) {
		return_ACPI_STATUS(AE_NOT_FOUND);
	}
	else {
		return_ACPI_STATUS(AE_OK);
	}
}
