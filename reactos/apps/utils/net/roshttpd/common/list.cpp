/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        list.cpp
 * PURPOSE:     A doubly linked list implementation
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 * NOTES:       The linked list does it's own heap management for
 *              better performance
 * TODO:        - InsertBefore(), InsertAfter(), Move()
 */
#include <windows.h>
#include <list.h>

// **************************** CListNode ****************************

HANDLE CListNode::hHeap = NULL;
INT    CListNode::nRef = 0;

// Default constructor
CListNode::CListNode()
{
	Element = NULL;
	Next = NULL;
	Prev = NULL;
}

// Constructor with element and next as starter values
CListNode::CListNode(PVOID element, CListNode *next, CListNode *prev)
{
	Element = element;
	Next = next;
	Prev = prev;
}

PVOID CListNode::operator new(/*size_t*/ UINT size)
{
    PVOID p;
    if (hHeap == NULL) {
        SYSTEM_INFO inf;
        GetSystemInfo(&inf);
        hHeap = HeapCreate(0, inf.dwAllocationGranularity, 0);
    }
    if ((p = HeapAlloc(hHeap, 0, size)) != NULL)
        nRef++;
    return p;
}

VOID CListNode::operator delete(PVOID p)
{
    if (HeapFree(hHeap, 0, p) != FALSE)
        nRef--;
    if (nRef == 0) {
        HeapDestroy(hHeap);
        hHeap = NULL;
	}
}

// Set element
VOID CListNode::SetElement(PVOID element)
{
	Element = element;
}

// Set pointer to next node in list
VOID CListNode::SetNext(CListNode *next)
{
	Next = next;
}

// Set pointer to previous node in list
VOID CListNode::SetPrev(CListNode *prev)
{
	Prev = prev;
}

// Get element of node
PVOID CListNode::GetElement()
{
	return Element;
}

// Get pointer to next node in list
CListNode *CListNode::GetNext()
{
	return Next;
}

// Get pointer to previous node in list
CListNode *CListNode::GetPrev()
{
	return Prev;
}
