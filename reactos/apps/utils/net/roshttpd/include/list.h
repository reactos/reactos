/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/list.h
 */
#ifndef __LIST_H
#define __LIST_H

#include <windows.h>
#include <iterator.h>

class CListNode {
public:
	CListNode();
	CListNode(VOID *element, CListNode *next, CListNode *prev);
	~CListNode() {};
    void* operator new(/*size_t s*/ UINT s);
    VOID  operator delete(void* p);

	VOID SetElement(PVOID element);
	VOID SetNext(CListNode *next);
	VOID SetPrev(CListNode *prev);
	PVOID GetElement();
	CListNode *GetNext();
	CListNode *GetPrev();
private:
	PVOID Element;
	CListNode *Next; 
	CListNode *Prev;
    static HANDLE hHeap;
    static INT nRef;
};

template <class Item> class CList {
public:
	//CList(CList&);
	CList();
	~CList();
	CList& operator=(CList&);
	
	CIterator<Item> *CreateIterator() const;
	LONG Count() const;
	Item& Get(const LONG index) const;
	// Can throw bad_alloc
	VOID Insert(Item& element);
	VOID Remove(Item& element);
	VOID RemoveAll();
	CListNode *GetHeader() const;
	CListNode *GetTrailer() const;
private:
	CListNode *Search(Item& element) const;
	LONG NodeCount;
	CListNode *Header;
	CListNode *Trailer;
};

template <class Item> class CListIterator : public CIterator<Item> {
public:
	CListIterator(const CList<Item> *list);
	virtual VOID First();
	virtual VOID Next();
	virtual BOOL IsDone() const;
	virtual Item CurrentItem() const;
private:
	const CList<Item> *List;
	CListNode *Current;
};

// ****************************** CList ******************************

// Default constructor
template <class Item>
CList<Item>::CList()
{
	// Create dummy nodes
	Trailer = new CListNode;
	Header = new CListNode;
	Header->SetNext(Trailer);
	Trailer->SetPrev(Header);
}

// Default destructor
template <class Item>
CList<Item>::~CList()
{	
	RemoveAll();
	delete Trailer;
	delete Header;
}

// Create an iterator for the list
template <class Item>
CIterator<Item> *CList<Item>::CreateIterator() const
{
	return new CListIterator<Item>((CList<Item> *) this);
}

// Return number of elements in list
template <class Item>
LONG CList<Item>::Count() const
{
	return NodeCount;
}

// Return element at index
template <class Item>
Item& CList<Item>::Get(const LONG index) const
{
	CListNode *node;

	if ((index < 0) || (index >= NodeCount))
		return NULL;

	node = Header;
	for (i = 0; i <= index; i++)
		node = node->GetNext();

	return (Item *) node->GetElement();
}

// Insert an element into the list
template <class Item>
VOID CList<Item>::Insert(Item& element)
{
	CListNode *node;

	node = new CListNode((PVOID)element, Trailer, Trailer->GetPrev());
	Trailer->GetPrev()->SetNext(node);
	Trailer->SetPrev(node);
	NodeCount++;
}

// Remove an element from the list
template <class Item>
VOID CList<Item>::Remove(Item& element)
{
	CListNode *node;

	node = Search(element);
	if (node != NULL) {
		node->GetPrev()->SetNext(node->GetNext());
		node->GetNext()->SetPrev(node->GetPrev());
		NodeCount--;
		delete node;
	}
}

// Remove all elements in list
template <class Item>
VOID CList<Item>::RemoveAll()
{
	CListNode *node;
	CListNode *tmp;

	node = Header->GetNext();
	while (node != Trailer) {
		tmp = node->GetNext();
		delete node;
		node = tmp;
	} 
	Header->SetNext(Trailer);
	Trailer->SetPrev(Header);
	NodeCount = 0;
}

// Return header node
template <class Item>
CListNode *CList<Item>::GetHeader() const
{
	return Header;
}

// Return trailer node
template <class Item>
CListNode *CList<Item>::GetTrailer() const
{
	return Trailer;
}

// Searches for a node that contains the element. Returns NULL if element is not found
template <class Item>
CListNode *CList<Item>::Search(Item& element) const
{
	CListNode *node;

	node = Header;
	while (((node = node->GetNext()) != Trailer) && (node->GetElement() != element));
	if (node != Trailer)
		return node;
	else
		return NULL;
}


// ************************** CListIterator **************************

// Default constructor
template <class Item>
CListIterator<Item>::CListIterator(const CList<Item> *list) : List(list)
{
	First();
}

// Go to first element in list
template <class Item>
VOID CListIterator<Item>::First()
{
	Current = List->GetHeader()->GetNext();
}

// Go to next element in list
template <class Item>
VOID CListIterator<Item>::Next()
{
	if (!IsDone())
		Current = Current->GetNext();
}

// Return FALSE when there are more elements in list and TRUE when there are no more
template <class Item>
BOOL CListIterator<Item>::IsDone() const
{
	return (Current == List->GetTrailer());
}

// Return current element
template <class Item>
Item CListIterator<Item>::CurrentItem() const
{
	return IsDone()? NULL : (Item) Current->GetElement();
}

#endif /* __LIST_H */
