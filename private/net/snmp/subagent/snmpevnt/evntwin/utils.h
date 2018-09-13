#ifndef _utils_h
#define _utils_h


void MapEventToSeverity(DWORD dwEvent, CString& sResult);
extern LONG FindWholeWord(LPCTSTR pszTemplate, LPCTSTR pszText);
extern LONG FindSubstring(LPCTSTR pszTemplate, LPCTSTR pszText);
extern void DecString(CString& sValue, int iValue);
extern void DecString(CString& sValue, long lValue);
extern void DecString(CString& sValue, DWORD dwValue);

extern void GenerateRangeMessage(CString& sMessage, LONG nMin, LONG nMax);
extern SCODE GetThousandSeparator(TCHAR* pch);
extern BOOL IsDecimalInteger(LPCTSTR pszValue);
extern SCODE AsciiToLong(LPCTSTR pszValue, LONG* plResult);


//-------------------------------------------------------
// Class: CList
//
// Description:
//    This class implements a circularly linked list.
//
// Methods:
//-------------------------------------------------------
// CList::CList(void* pValue)
//
// Construct a node and associate the pointer value
// with it.  The pointer is declared to be type
// void* so that this class is as generic as possible.
//
// Input:
//    pValue = Pointer to the value to associate with this node
//
// Returns: Nothing
//
//-------------------------------------------------------
// void Link(CList*& pndHead)
//
// Add this node to the end of the list pointed to by
// pndHead.  If pndHead is NULL, then set pndHead to
// the address of this node.
//
// Input:
//    pndHead = A reference to the head node pointer
//
// Returns: Nothing
//
//-------------------------------------------------------
// void Unlink(CList*& pndHead)
//
// Unlink this node from the list it is on.  If this node
// is the only element on the list, set the value of pndHead
// to NULL to indicate that the list is empty.
//
// Input:
//    pndHead = A reference to the head node pointer.
//
// Returns: nothing
//
//-------------------------------------------------------
// CList* Next()
//
// Return a pointer to the next node in the list.
//
// Input: None
//
// Returns: A pointer to the next node on the list
//
//-------------------------------------------------------
// CList* Prev()
//
// Return a pointer to the previous node in the list.
//
// Input: None
//
// Returns: A pointer to the previous node on the list.
//
//-------------------------------------------------------
// void* Value()
//
// Return the "value pointer" attached to this node.
//
// Input: None
//
// Returns: The node's value.
//
//--------------------------------------------------------
class CList
{
public:
    CList();
    void Link(CList*& pndHead);
	void Unlink(CList*& pndHead);
	CList* Next() {return m_pndNext;}
	CList* Prev() {return m_pndPrev;}
private:
    CList* m_pndPrev;
	CList* m_pndNext;
};

#endif
