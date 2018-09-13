//
// CDPA Class.
// Based on DPA's from the shell
//
// FelixA
//

#ifndef __LISTH
#define __LISTH

class CDPA
{
public:
	int GetCount();
	CDPA();
	~CDPA();

	BOOL	Append(LPVOID);
	LPVOID	GetPointer(int iItem) const;
	void	Remove(int iItem);

protected:
	int GetNextFree() const { return m_iCurrentTop; }

private:
	int GetAllocated() const {return m_iAllocated;}
	void SetAllocated(int i) { m_iAllocated=i; }

	void SetNextFree(int i) { m_iCurrentTop=i; }

	void FAR * FAR * GetData() const { return m_pData; };
	void SetData(void FAR * FAR * pD) { m_pData=pD; }

	HANDLE GetHeap() const { return m_Heap; }
	void SetHeap( HANDLE h) { m_Heap = h; }

	int m_iAllocated; // Number of items in the list.
	int m_iCurrentTop;// Next item to use.
	void FAR * FAR * m_pData;	// Pointer to the pointer array.
	HANDLE m_Heap;	// Handle for the heap we're using.
};

#endif
