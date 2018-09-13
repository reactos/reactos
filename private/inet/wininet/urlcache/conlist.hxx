/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  conlist.hxx

Abstract:

    Linked list of URL_CONTAINERs
    
Author:
    Adriaan Canter (adriaanc) 04-02-97
    
--*/

#ifndef _CONLIST_HXX
#define _CONLIST_HXX

#define NOT_AN_INDEX (0xFFFFFFFF)
#define LARGEST_INDEX (0xFFFFFFFF-0x1)

/*-----------------------------------------------------------------------------
class CConElem - Element in linked list of containers.
  ---------------------------------------------------------------------------*/
class CConElem
{
public:
    URL_CONTAINER*  _pUrlCon;
    CConElem       *_pNext;

    CConElem(URL_CONTAINER* pNew);
    ~CConElem();
};


/*-----------------------------------------------------------------------------
class CConList - Linked list of containers.
    Stores pointer to current element for fast incrementing accesses.
  ---------------------------------------------------------------------------*/
class CConList
{
private:
    DWORD            _n;                 // Index of last element.
    DWORD            _nCur;              // Index of current element.
    CConElem        *_pCur;              // Pointer to current element.
    CConElem        *_pHead;             // Pointer to head element.

    BOOL             Seek(DWORD nElem);  // Seek to element with index nElem.
    BOOL             Remove(DWORD nElem);// Removes and destructs element.
    
public:
    CConList();
    ~CConList();

    DWORD Size();
    BOOL  Free();
    BOOL            Add(URL_CONTAINER* pNew);     // Appends a URL_CONTAINER* to list.
    URL_CONTAINER*  Get(DWORD nElem);             // Returns an AddRef'ed ref to URL_CONTAINER
};

#endif // _CONLIST_HXX
