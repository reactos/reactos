/********************************************************************
*	Class:	CList.cpp. This is part of WinUI.
*
*	Purpose:	A simple way to create doubly linked lists.
*
*	Authors:	Originally coded by Claude Catonio.
*
*	License:	Original code was public domain.
*			Present revised CList classes are covered by GNU General Public License.
*
*	Revisions:	
*			Manu B. 10/05/01	Terms was translated to English.
*			Manu B. 10/16/01	Add CList::Destroy(CNode *node) method.
*			Manu B. 11/12/01	Add InsertBefore/InsertAfter methods.
*			Manu B. 11/17/01	First() & Last() now returns an integer value.
*			Manu B. 11/19/01	CNode::Destroy() returns next node by default.
*			Manu B. 12/28/01	Simplify CList, add InsertSorted().
*			Manu B. 03/13/02	Suppress CNode methods, next and prev are public.
*			Manu B. 03/28/02	Add Compare().
*
********************************************************************/
#include "CList.h"

/********************************************************************
*	Class:	CList.
*
*	Purpose:	List management.
*
*	Revisions:	
*
********************************************************************/
CList::~CList(){
	//MessageBox (0, "CList", "destructor", MB_OK);
	while (first != NULL){
		current = first;
		first = first->next;
		delete current;
	}
	current = last = first;
	count = 0;
}

/********************************************************************
*	Browse the list.
********************************************************************/
CNode * CList::First(){
	current = first;
return current;
}

CNode * CList::Last(){
	current = last;
return current;
}

CNode * CList::Prev(){
	// Empty list ?
	if (first != NULL){
		if(current->prev == NULL){
			// No previous node.
			return NULL;
		}else{
			// A previous one.
			current = current->prev;
			return current;
		}
	}
return NULL;
}

CNode * CList::Next(){
	// Empty list ?
	if (first != NULL){
		if(current->next == NULL){
			// No next node.
			return NULL;
		}else{
			// A next one.
			current = current->next;
			return current;
		}
	}
return NULL;
}

/********************************************************************
*	Insert nodes.
********************************************************************/
void CList::InsertFirst(CNode *node){
	if(first == NULL){
		// Empty list.
		first = last = node;
	}else{
		// Set node pointers.
		node->prev	= NULL;
		node->next 	= first;
		// Insert in the list.
		first->prev		= node;
		first 			= node;
	}
	// Set current node, increment the node counter.
	current = node;
	count++;
}

void CList::InsertLast(CNode *node){
	if(first == NULL){
		// Empty list.
		first = last = node;
	}else{
		// Set node pointers.
		node->prev 	= last;
		node->next 	= NULL;
		// Insert in the list.
		last->next 		= node;
		last 			= node;
	}
	// Set current node, increment the node counter.
	current = node;
	count++;
}

void CList::InsertBefore(CNode *node){
	if(first == NULL){
		// Empty list.
		first = last = node;
	}else{
		if (current == first)
			first = node;
		// Set node pointers.
		node->prev = current->prev;
		node->next = current;
		// Insert in the list.
		if (node->prev)
			node->prev->next = node;
		current->prev = node;
	}
	// Set current node, increment the node counter.
	current = node;
	count++;
}

void CList::InsertAfter(CNode *node){
	if(first == NULL){
		// Empty list.
		first = last = node;
	}else{
		if (current == last)
			last = node;
		// Set node pointers.
		node->prev = current;
		node->next = current->next;
		// Insert in the list.
		if (node->next)
			node->next->prev = node;
		current->next = node;
	}
	// Set current node, increment the node counter.
	current = node;
	count++;
}

bool CList::InsertSorted(CNode * newNode){
	// Get the current node.
	CNode * currentNode = GetCurrent();
	int cmpResult;

	if(!currentNode){
		// The list is empty, InsertFirst() and return.
		InsertFirst(newNode);
		return true;
	}

	// Compare new node and current node data to know if we must parse Up 
	// or Down from current node.
	cmpResult = Compare(newNode, currentNode);

	// Search Up -----------------------------------------------------------------
	if (cmpResult == -1){
		// Parse the list while there's a previous node.
		while (Prev()){
			currentNode = GetCurrent();
			cmpResult = Compare(newNode, currentNode);

			if (cmpResult == 1){
				// Correct position found.
				InsertAfter(newNode);
				return true;
			}else if (cmpResult == 0){
				// Don't add a file twice.
				return false;
			}
		}
		// There's no previous node, so insert first.
		InsertFirst(newNode);
		return true;
	}
	// Search Down --------------------------------------------------------------
	else if (cmpResult == 1){
		// Parse the list while there's a next node.
		while (Next()){
			currentNode = GetCurrent();
			cmpResult = Compare(newNode, currentNode);

			if (cmpResult == -1){
				// Correct position found.
				InsertBefore(newNode);
				return true;
			}else if (cmpResult == 0){
				// Don't add a file twice.
				return false;
			}
		}
		// There's no next node, so insert last.
		InsertLast(newNode);
		return true;
	}
	// Don't add a file twice (cmpResult == 0) -------------------------------------
return false;
}


int CList::InsertSorted_New(CNode * newNode){
	int cmpResult;

	/* Get the current node */
	CNode * currentNode = GetCurrent();
	if(!currentNode)
		return EMPTY_LIST;

	/* Parse up or down ? */
	cmpResult = Compare(newNode, currentNode);

	/* -Up- */
	if (cmpResult == -1){
		// Parse the list while there's a previous node.
		while (Prev()){
			currentNode = GetCurrent();
			cmpResult = Compare(newNode, currentNode);

			if (cmpResult == 1){
				// Correct position found.
				return INSERT_AFTER;
			}else if (cmpResult == 0){
				// Don't add a file twice.
				return FILE_FOUND;
			}
		}
		// There's no previous node, so insert first.
		return INSERT_FIRST;

	/* -Down- */
	}else if (cmpResult == 1){
		// Parse the list while there's a next node.
		while (Next()){
			currentNode = GetCurrent();
			cmpResult = Compare(newNode, currentNode);

			if (cmpResult == -1){
				// Correct position found.
				return INSERT_BEFORE;
			}else if (cmpResult == 0){
				// Don't add a file twice.
				return FILE_FOUND;
			}
		}
		// There's no next node, so insert last.
		return INSERT_LAST;
	}
	// Don't add a file twice (cmpResult == 0) -------------------------------------
return FILE_FOUND;
}


/********************************************************************
*	Destroy nodes.
********************************************************************/
void CList::DestroyCurrent(){
//	CNode * node = current;
	Destroy(current);
}

void CList::Destroy(CNode * node){
	// Empty list ?
	if (current != NULL){
		// Detach node from the list.
		if (node->next != NULL)
			node->next->prev = node->prev;
		if (node->prev != NULL)
			node->prev->next = node->next;
	
		// Set current node.
		if(node->next != NULL)
			current = node->next;
		else
			current = node->prev;

		if (current == NULL){
			// Now, the list is empty.
			first = last = NULL;

		}else if (first == node){
			// Detached node was first.
			first = current;

		}else if (last == node){
			// Detached node was last.
			last = current;
		}
		delete node;
		count--;
	}
}

void CList::DestroyList(){
	while (first != NULL){
		current = first;
		first = first->next;
		delete current;
	}
	current = last = first;
	count = 0;
}

int CList::Length(){
return count;
}

