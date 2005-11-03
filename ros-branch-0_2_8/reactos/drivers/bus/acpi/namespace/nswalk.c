/******************************************************************************
 *
 * Module Name: nswalk - Functions for walking the APCI namespace
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
#include "acinterp.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_NAMESPACE
	 MODULE_NAME         ("nswalk")


/****************************************************************************
 *
 * FUNCTION:    Acpi_get_next_object
 *
 * PARAMETERS:  Type                - Type of object to be searched for
 *              Parent              - Parent object whose children we are
 *                                      getting
 *              Last_child          - Previous child that was found.
 *                                    The NEXT child will be returned
 *
 * RETURN:      ACPI_NAMESPACE_NODE - Pointer to the NEXT child or NULL if
 *                                      none is found.
 *
 * DESCRIPTION: Return the next peer object within the namespace.  If Handle
 *              is valid, Scope is ignored.  Otherwise, the first object
 *              within Scope is returned.
 *
 ****************************************************************************/

ACPI_NAMESPACE_NODE *
acpi_ns_get_next_object (
	OBJECT_TYPE_INTERNAL    type,
	ACPI_NAMESPACE_NODE     *parent_node,
	ACPI_NAMESPACE_NODE     *child_node)
{
	ACPI_NAMESPACE_NODE     *next_node = NULL;


	if (!child_node) {

		/* It's really the parent's _scope_ that we want */

		if (parent_node->child) {
			next_node = parent_node->child;
		}
	}

	else {
		/* Start search at the NEXT object */

		next_node = acpi_ns_get_next_valid_object (child_node);
	}


	/* If any type is OK, we are done */

	if (type == ACPI_TYPE_ANY) {
		/* Next_node is NULL if we are at the end-of-list */

		return (next_node);
	}


	/* Must search for the object -- but within this scope only */

	while (next_node) {
		/* If type matches, we are done */

		if (next_node->type == type) {
			return (next_node);
		}

		/* Otherwise, move on to the next object */

		next_node = acpi_ns_get_next_valid_object (next_node);
	}


	/* Not found */

	return (NULL);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ns_walk_namespace
 *
 * PARAMETERS:  Type                - ACPI_OBJECT_TYPE to search for
 *              Start_node          - Handle in namespace where search begins
 *              Max_depth           - Depth to which search is to reach
 *              Unlock_before_callback- Whether to unlock the NS before invoking
 *                                    the callback routine
 *              User_function       - Called when an object of "Type" is found
 *              Context             - Passed to user function
 *
 * RETURNS      Return value from the User_function if terminated early.
 *              Otherwise, returns NULL.
 *
 * DESCRIPTION: Performs a modified depth-first walk of the namespace tree,
 *              starting (and ending) at the object specified by Start_handle.
 *              The User_function is called whenever an object that matches
 *              the type parameter is found.  If the user function returns
 *              a non-zero value, the search is terminated immediately and this
 *              value is returned to the caller.
 *
 *              The point of this procedure is to provide a generic namespace
 *              walk routine that can be called from multiple places to
 *              provide multiple services;  the User Function can be tailored
 *              to each task, whether it is a print function, a compare
 *              function, etc.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_walk_namespace (
	OBJECT_TYPE_INTERNAL    type,
	ACPI_HANDLE             start_node,
	u32                     max_depth,
	u8                      unlock_before_callback,
	WALK_CALLBACK           user_function,
	void                    *context,
	void                    **return_value)
{
	ACPI_STATUS             status;
	ACPI_NAMESPACE_NODE     *child_node;
	ACPI_NAMESPACE_NODE     *parent_node;
	OBJECT_TYPE_INTERNAL    child_type;
	u32                     level;


	/* Special case for the namespace Root Node */

	if (start_node == ACPI_ROOT_OBJECT) {
		start_node = acpi_gbl_root_node;
	}


	/* Null child means "get first object" */

	parent_node = start_node;
	child_node = 0;
	child_type  = ACPI_TYPE_ANY;
	level       = 1;

	/*
	 * Traverse the tree of objects until we bubble back up to where we
	 * started. When Level is zero, the loop is done because we have
	 * bubbled up to (and passed) the original parent handle (Start_entry)
	 */

	while (level > 0) {
		/*
		 * Get the next typed object in this scope.  Null returned
		 * if not found
		 */

		status = AE_OK;
		child_node = acpi_ns_get_next_object (ACPI_TYPE_ANY,
				 parent_node,
				 child_node);

		if (child_node) {
			/*
			 * Found an object, Get the type if we are not
			 * searching for ANY
			 */

			if (type != ACPI_TYPE_ANY) {
				child_type = child_node->type;
			}

			if (child_type == type) {
				/*
				 * Found a matching object, invoke the user
				 * callback function
				 */

				if (unlock_before_callback) {
					acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
				}

				status = user_function (child_node, level,
						 context, return_value);

				if (unlock_before_callback) {
					acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);
				}

				switch (status) {
				case AE_OK:
				case AE_CTRL_DEPTH:
					/* Just keep going */
					break;

				case AE_CTRL_TERMINATE:
					/* Exit now, with OK status */
					return (AE_OK);
					break;

				default:
					/* All others are valid exceptions */
					return (status);
					break;
				}
			}

			/*
			 * Depth first search:
			 * Attempt to go down another level in the namespace
			 * if we are allowed to.  Don't go any further if we
			 * have reached the caller specified maximum depth
			 * or if the user function has specified that the
			 * maximum depth has been reached.
			 */

			if ((level < max_depth) && (status != AE_CTRL_DEPTH)) {
				if (acpi_ns_get_next_object (ACPI_TYPE_ANY,
						 child_node, 0)) {
					/*
					 * There is at least one child of this
					 * object, visit the object
					 */
					level++;
					parent_node   = child_node;
					child_node    = 0;
				}
			}
		}

		else {
			/*
			 * No more children in this object (Acpi_ns_get_next_object
			 * failed), go back upwards in the namespace tree to
			 * the object's parent.
			 */
			level--;
			child_node = parent_node;
			parent_node = acpi_ns_get_parent_object (parent_node);
		}
	}

	/* Complete walk, not terminated by user function */
	return (AE_OK);
}


