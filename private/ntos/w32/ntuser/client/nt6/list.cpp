//
// CDPA - Shell like DPA stuff
//
// FelixA
//

#include "pch.h"
#include "list.h"

/////////////////////////////////////////////////////////////////////////////
//
//
CDPA::CDPA()
: m_iAllocated(0),
  m_iCurrentTop(0),
  m_pData(NULL),
  m_Heap(NULL)	//Allocate from global heap.
{
}

CDPA::~CDPA()
{
	LPVOID lpV;
	int i=0;
	while( lpV=GetPointer(i++) )
		delete lpV;

	//
	// BugBug - you must delete all the data that we allocated in the list
	//
	if(GetData())
		HeapFree(GetHeap(), 0, GetData());
}

/////////////////////////////////////////////////////////////////////////////
//
// Inserts at the end - no mechanism for inserting in the middle.
// Grows the Heap if you add more items than currently have spave for
//
BOOL CDPA::Append(LPVOID lpData)
{
	if(GetNextFree()==GetAllocated())
	{
		int iNewSize = GetAllocated()+32;	// Grow in 32 allocations;
        void FAR * FAR * ppNew;

		if(GetData())
            ppNew = (void FAR * FAR *)HeapReAlloc(GetHeap(), HEAP_ZERO_MEMORY, GetData(), iNewSize * sizeof(LPVOID));
		else
		{
			if(!GetHeap())
				SetHeap(HeapCreate(0,0x4000,0));
            ppNew = (void FAR * FAR *)HeapAlloc(GetHeap(), HEAP_ZERO_MEMORY, iNewSize * sizeof(LPVOID));
		}

		SetData(ppNew);
		if(ppNew)
		{
			SetAllocated(iNewSize);
		}
		else
			return FALSE;
	}

	*(GetData()+GetNextFree())=lpData;
	SetNextFree(GetNextFree()+1);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Given the index into the list, returns its value.
//
LPVOID CDPA::GetPointer(int iItem) const
{
	if(GetData())
		if(iItem<GetAllocated())	// mCurrentTop is not active.
			return *(GetData()+iItem);
	return NULL;
}

int CDPA::GetCount()
{
	return GetNextFree();
}

void CDPA::Remove(int iItem) 
{
	if(GetData())
		if(iItem<GetAllocated())	// mCurrentTop is not active.
			*(GetData()+iItem)=NULL;
}
