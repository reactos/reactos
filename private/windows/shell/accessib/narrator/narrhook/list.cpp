#include <windows.h>

#include "list.h"

//----------------------------------------------------------------------------
// 
// Description:
//   this implements a simple C++ list class routine that will let us build
//   up a list of an arbitrary long number of items.  An item can be any 
//   simple type or structure.  
//
//----------------------------------------------------------------------------

CList::CList()
{
    m_pListHead=m_pListCurr=m_pListTail=NULL;
}

CList::~CList()
{
    RemoveAll();
}

//----------------------------------------------------------------------------
// 
// Description:
//   Add an item to the list.  The length of the item is assumed correct
//   for the passed in item.  Items are added to the end of the list.
//
// Arguments:
//   pData  - pointer to the data to add
//   nBytes - the number of bytes of pData
// 
// Returns: TRUE if the data is succesfully added, otherwise FALSE.
//           a-anilk; Just donot absolutely Duplicate Entries
//   
//----------------------------------------------------------------------------
BOOL
CList::Add(PVOID pData, UINT nBytes)
{
    PLIST tmp;

    if ((nBytes == 0) || (pData == NULL))
        return FALSE;

    tmp=new LIST;

    if (NULL == tmp)
        return FALSE;

    tmp->pData=new BYTE[nBytes];

    if (NULL == tmp->pData)
    {
        delete tmp;
        return FALSE;
    }

	// Donot add duplicate entries that come one after another...
	if ( m_pListHead != NULL )
	{
		if (! memcmp(m_pListHead->pData, pData, nBytes ) )
		{
			delete tmp;
			return FALSE;
		}
	}

    CopyMemory(tmp->pData,pData,nBytes);

    tmp->nBytes=nBytes;
    tmp->next=NULL;
    tmp->prev=m_pListTail;

    if (IsEmpty())
    {
        m_pListHead=tmp;
    }
    else
    {
        if (m_pListTail != NULL)
            m_pListTail->next=tmp;
    }

    m_pListTail=tmp;

    return TRUE;
}


//----------------------------------------------------------------------------
// 
// Description:
//   Remove all the items from the list.
//
//----------------------------------------------------------------------------
void
CList::RemoveAll()
{
    while(!IsEmpty())
        RemoveHead(NULL);
}


//----------------------------------------------------------------------------
//
// Description:
//   Remove just the item at the head of the list.  If the passed in buffer
//   is not NULL, it will overwrite the buffer with the contents of the data.
//   This code assumes the passed in pData buffer is large enough for the
//   stored data item.  If the passed in pData is NULL, then the head item
//   is simply discarded.
//
// Arguments:
//   pData - a buffer to overwrite with the head item. Can be NULL.
//  
//----------------------------------------------------------------------------
void
CList::RemoveHead(PVOID pData)
{
    PLIST tmp;

    if (!IsEmpty())
    {
        // make sure m_pListCurr is always NULL or someplace valid

        if (m_pListCurr == m_pListHead)
            m_pListCurr=m_pListHead->next;

        tmp=m_pListHead;
        m_pListHead=m_pListHead->next;

        if (tmp->pData != NULL)
        {
            if (pData != NULL)
                CopyMemory(pData,tmp->pData,tmp->nBytes);

            delete[] (tmp->pData);
        }

        delete tmp;

        if (!IsEmpty())
            m_pListHead->prev=NULL;
        else
            m_pListTail=NULL;
    }
}


//----------------------------------------------------------------------------
//
// Description:
//    RemoveHead(NULL, NULL)  <=>   RemoveHead(NULL)
//
//    RemoveHead(pData,NULL)  <=>   RemoveHead(pData)
//
//    RemoveHead(NULL,&nBytes) - sets nBytes to the size of the data in the 
//                               head of the list, nothing is removed
//
//    RemoveHead(pData,&nBytes)- copies the data in the head of the list into
//                               pData up to the min of the size of data in the
//                               head of the list or nBytes. The head of the
//                               list is removed. 
//
//----------------------------------------------------------------------------
void
CList::RemoveHead(PVOID pData, PUINT pnBytes)
{
    PLIST tmp;
    UINT  nBytes;

    if (pnBytes == NULL)
    {
        RemoveHead(pData);
        return;
    }

    if (pData == NULL)
    {
        // they just want the size, so return it.

        if (IsEmpty())
            *pnBytes=0;
        else
            *pnBytes=m_pListHead->nBytes;

        return;
    }

    if (IsEmpty())
    {
        *pnBytes=0;
        return;
    }

    // make sure m_pListCurr is always NULL or someplace valid

    if (m_pListCurr == m_pListHead)
        m_pListCurr=m_pListHead->next;

    tmp=m_pListHead;
    m_pListHead=m_pListHead->next;

    //
    // only copy the min size of the two
    //
        
    nBytes=min((*pnBytes),tmp->nBytes);

    if (tmp->pData != NULL)
    {
        CopyMemory(pData,tmp->pData,nBytes);

        *pnBytes=nBytes;

        delete[] (tmp->pData);
    }
    else
        *pnBytes=0;

    delete tmp;

    if (!IsEmpty())
        m_pListHead->prev=NULL;
    else
        m_pListTail=NULL;
}



