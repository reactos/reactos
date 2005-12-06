/*******************************************************************************
 *
 * Module Name: nsalloc - Namespace allocation and deletion utilities
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
#include "acinterp.h"


#define _COMPONENT          ACPI_NAMESPACE
	 MODULE_NAME         ("nsalloc")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_create_node
 *
 * PARAMETERS:
 *
 * RETURN:      None
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
acpi_ns_create_node (
	u32                     acpi_name)
{
	ACPI_NAMESPACE_NODE     *node;


	node = acpi_cm_callocate (sizeof (ACPI_NAMESPACE_NODE));
	if (!node) {
		return (NULL);
	}

	INCREMENT_NAME_TABLE_METRICS (sizeof (ACPI_NAMESPACE_NODE));

	node->data_type      = ACPI_DESC_TYPE_NAMED;
	node->name           = acpi_name;
	node->reference_count = 1;

	return (node);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_delete_node
 *
 * PARAMETERS:
 *
 * RETURN:      None
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

void
acpi_ns_delete_node (
	ACPI_NAMESPACE_NODE     *node)
{
	ACPI_NAMESPACE_NODE     *parent_node;
	ACPI_NAMESPACE_NODE     *prev_node;
	ACPI_NAMESPACE_NODE     *next_node;


	parent_node = acpi_ns_get_parent_object (node);

	prev_node = NULL;
	next_node = parent_node->child;

	while (next_node != node) {
		prev_node = next_node;
		next_node = prev_node->peer;
	}

	if (prev_node) {
		prev_node->peer = next_node->peer;
		if (next_node->flags & ANOBJ_END_OF_PEER_LIST) {
			prev_node->flags |= ANOBJ_END_OF_PEER_LIST;
		}
	}
	else {
		parent_node->child = next_node->peer;
	}


	DECREMENT_NAME_TABLE_METRICS (sizeof (ACPI_NAMESPACE_NODE));

	/*
	 * Detach an object if there is one
	 */

	if (node->object) {
		acpi_ns_detach_object (node);
	}

	acpi_cm_free (node);


	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_install_node
 *
 * PARAMETERS:  Walk_state      - Current state of the walk
 *              Parent_node     - The parent of the new Node
 *              Node        - The new Node to install
 *              Type            - ACPI object type of the new Node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a new entry within a namespace table.
 *
 ******************************************************************************/

void
acpi_ns_install_node (
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     *parent_node,   /* Parent */
	ACPI_NAMESPACE_NODE     *node,      /* New Child*/
	OBJECT_TYPE_INTERNAL    type)
{
	u16                     owner_id = TABLE_ID_DSDT;
	ACPI_NAMESPACE_NODE     *child_node;


	/*
	 * Get the owner ID from the Walk state
	 * The owner ID is used to track table deletion and
	 * deletion of objects created by methods
	 */
	if (walk_state) {
		owner_id = walk_state->owner_id;
	}


	/* link the new entry into the parent and existing children */

	/* TBD: Could be first, last, or alphabetic */

	child_node = parent_node->child;
	if (!child_node) {
		parent_node->child = node;
	}

	else {
		while (!(child_node->flags & ANOBJ_END_OF_PEER_LIST)) {
			child_node = child_node->peer;
		}

		child_node->peer = node;

		/* Clear end-of-list flag */

		child_node->flags &= ~ANOBJ_END_OF_PEER_LIST;
	}

	/* Init the new entry */

	node->owner_id  = owner_id;
	node->flags     |= ANOBJ_END_OF_PEER_LIST;
	node->peer      = parent_node;


	/*
	 * If adding a name with unknown type, or having to
	 * add the region in order to define fields in it, we
	 * have a forward reference.
	 */

	if ((ACPI_TYPE_ANY == type) ||
		(INTERNAL_TYPE_DEF_FIELD_DEFN == type) ||
		(INTERNAL_TYPE_BANK_FIELD_DEFN == type)) {
		/*
		 * We don't want to abort here, however!
		 * We will fill in the actual type when the
		 * real definition is found later.
		 */

	}

	/*
	 * The Def_field_defn and Bank_field_defn cases are actually
	 * looking up the Region in which the field will be defined
	 */

	if ((INTERNAL_TYPE_DEF_FIELD_DEFN == type) ||
		(INTERNAL_TYPE_BANK_FIELD_DEFN == type)) {
		type = ACPI_TYPE_REGION;
	}

	/*
	 * Scope, Def_any, and Index_field_defn are bogus "types" which do
	 * not actually have anything to do with the type of the name
	 * being looked up.  Save any other value of Type as the type of
	 * the entry.
	 */

	if ((type != INTERNAL_TYPE_SCOPE) &&
		(type != INTERNAL_TYPE_DEF_ANY) &&
		(type != INTERNAL_TYPE_INDEX_FIELD_DEFN)) {
		node->type = (u8) type;
	}

	/*
	 * Increment the reference count(s) of all parents up to
	 * the root!
	 */

	while ((node = acpi_ns_get_parent_object (node)) != NULL) {
		node->reference_count++;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_delete_children
 *
 * PARAMETERS:  Parent_node     - Delete this objects children
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete all children of the parent object. Deletes a
 *              "scope".
 *
 ******************************************************************************/

void
acpi_ns_delete_children (
	ACPI_NAMESPACE_NODE     *parent_node)
{
	ACPI_NAMESPACE_NODE     *child_node;
	ACPI_NAMESPACE_NODE     *next_node;
	u8                      flags;


	if (!parent_node) {
		return;
	}

	/* If no children, all done! */

	child_node = parent_node->child;
	if (!child_node) {
		return;
	}

	/*
	 * Deallocate all children at this level
	 */
	do {
		/* Get the things we need */

		next_node   = child_node->peer;
		flags       = child_node->flags;

		/* Grandchildren should have all been deleted already */


		/* Now we can free this child object */

		DECREMENT_NAME_TABLE_METRICS (sizeof (ACPI_NAMESPACE_NODE));

		/*
		 * Detach an object if there is one
		 */

		if (child_node->object) {
			acpi_ns_detach_object (child_node);
		}

		acpi_cm_free (child_node);

		/* And move on to the next child in the list */

		child_node = next_node;

	} while (!(flags & ANOBJ_END_OF_PEER_LIST));


	/* Clear the parent's child pointer */

	parent_node->child = NULL;

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_delete_namespace_subtree
 *
 * PARAMETERS:  None.
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a subtree of the namespace.  This includes all objects
 *              stored within the subtree.  Scope tables are deleted also
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_delete_namespace_subtree (
	ACPI_NAMESPACE_NODE     *parent_node)
{
	ACPI_NAMESPACE_NODE     *child_node;
	ACPI_OPERAND_OBJECT     *obj_desc;
	u32                     level;


	if (!parent_node) {
		return (AE_OK);
	}


	child_node  = 0;
	level       = 1;

	/*
	 * Traverse the tree of objects until we bubble back up
	 * to where we started.
	 */

	while (level > 0) {
		/*
		 * Get the next typed object in this scope.
		 * Null returned if not found
		 */

		child_node = acpi_ns_get_next_object (ACPI_TYPE_ANY, parent_node,
				 child_node);
		if (child_node) {
			/*
			 * Found an object - delete the object within
			 * the Value field
			 */

			obj_desc = acpi_ns_get_attached_object (child_node);
			if (obj_desc) {
				acpi_ns_detach_object (child_node);
				acpi_cm_remove_reference (obj_desc);
			}


			/* Check if this object has any children */

			if (acpi_ns_get_next_object (ACPI_TYPE_ANY, child_node, 0)) {
				/*
				 * There is at least one child of this object,
				 * visit the object
				 */

				level++;
				parent_node   = child_node;
				child_node    = 0;
			}
		}

		else {
			/*
			 * No more children in this object.
			 * We will move up to the grandparent.
			 */
			level--;

			/*
			 * Now delete all of the children of this parent
			 * all at the same time.
			 */
			acpi_ns_delete_children (parent_node);

			/* New "last child" is this parent object */

			child_node = parent_node;

			/* Now we can move up the tree to the grandparent */

			parent_node = acpi_ns_get_parent_object (parent_node);
		}
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_remove_reference
 *
 * PARAMETERS:  Node           - Named object whose reference count is to be
 *                                decremented
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Remove a Node reference.  Decrements the reference count
 *              of all parent Nodes up to the root.  Any object along
 *              the way that reaches zero references is freed.
 *
 ******************************************************************************/

static void
acpi_ns_remove_reference (
	ACPI_NAMESPACE_NODE     *node)
{
	ACPI_NAMESPACE_NODE     *next_node;


	/*
	 * Decrement the reference count(s) of this object and all
	 * objects up to the root,  Delete anything with zero remaining references.
	 */
	next_node = node;
	while (next_node) {
		/* Decrement the reference count on this object*/

		next_node->reference_count--;

		/* Delete the object if no more references */

		if (!next_node->reference_count) {
			/* Delete all children and delete the object */

			acpi_ns_delete_children (next_node);
			acpi_ns_delete_node (next_node);
		}

		/* Move up to parent */

		next_node = acpi_ns_get_parent_object (next_node);
	}
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_delete_namespace_by_owner
 *
 * PARAMETERS:  None.
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete entries within the namespace that are owned by a
 *              specific ID.  Used to delete entire ACPI tables.  All
 *              reference counts are updated.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_delete_namespace_by_owner (
	u16                     owner_id)
{
	ACPI_NAMESPACE_NODE     *child_node;
	u32                     level;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_NAMESPACE_NODE     *parent_node;


	parent_node = acpi_gbl_root_node;
	child_node  = 0;
	level       = 1;

	/*
	 * Traverse the tree of objects until we bubble back up
	 * to where we started.
	 */

	while (level > 0) {
		/*
		 * Get the next typed object in this scope.
		 * Null returned if not found
		 */

		child_node = acpi_ns_get_next_object (ACPI_TYPE_ANY, parent_node,
				 child_node);

		if (child_node) {
			if (child_node->owner_id == owner_id) {
				/*
				 * Found an object - delete the object within
				 * the Value field
				 */

				obj_desc = acpi_ns_get_attached_object (child_node);
				if (obj_desc) {
					acpi_ns_detach_object (child_node);
					acpi_cm_remove_reference (obj_desc);
				}
			}

			/* Check if this object has any children */

			if (acpi_ns_get_next_object (ACPI_TYPE_ANY, child_node, 0)) {
				/*
				 * There is at least one child of this object,
				 * visit the object
				 */

				level++;
				parent_node   = child_node;
				child_node    = 0;
			}

			else if (child_node->owner_id == owner_id) {
				acpi_ns_remove_reference (child_node);
			}
		}

		else {
			/*
			 * No more children in this object.  Move up to grandparent.
			 */
			level--;

			if (level != 0) {
				if (parent_node->owner_id == owner_id) {
					acpi_ns_remove_reference (parent_node);
				}
			}

			/* New "last child" is this parent object */

			child_node = parent_node;

			/* Now we can move up the tree to the grandparent */

			parent_node = acpi_ns_get_parent_object (parent_node);
		}
	}


	return (AE_OK);
}


