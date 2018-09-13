// File memblk.hpp
// This class represents the memory block in debug build
#if !defined(MEMBLK_HPP_INCLUDED)
#define MEMBLK_HPP_INCLUDED

#if defined(_DEBUG)

#include "memstngs.h"

class CMemBlk
{
public:
    CMemBlk(UINT uSize, LPCSTR szFile, UINT uLine);      // Create memory block for given size
    ~CMemBlk();                        // Create memory block for given size
    BOOL  FreeMem();                   // Free the memory and fill the block with known value

     void const Dump(int nMargin = 0); // Send memory block information to debug output

    // Returns the memory block class from the external pointer
    static CMemBlk *GetCMemBlockFromMemory(void *pMem)
    {
        return *(CMemBlk **)(((BYTE *)pMem) - NUMPADBYTES - sizeof(CMemBlk *));
    }

    // Get the external address of the memory block
    inline  void * const GetAddress() { return _pMemBlock + NUMPADBYTES + sizeof(CMemBlk *);}

    // Get the allocated size of the block
    inline ULONG const GetSize() { return _uRequestedSize;}

    // Get the real size of the block
    inline  ULONG const GetRealSize() { return _uRequestedSize + 2 * NUMPADBYTES + sizeof(CMemBlk *);}

    // True if the block has been freed
    inline BOOL const IsFree() {return !_fAllocated;}

	void *operator new( size_t stAllocateBlock);
	void operator delete(void *pMem);

private:
    // Check for memory writes outside allocated block
    BOOL const ArePadBytesOK(UINT *puFirstOffset = NULL); 
    // Check for free bytes 
    BOOL const AreFillBytesOK(UINT *puFirstOffset = NULL);

    BOOL    _fAllocated;                  // TRUE if memory has been allocated
    BYTE    *_pMemBlock;                  // Memory block
    ULONG   _uRequestedSize;              // The requested size of the block
    ULONG   _uBlockID;                    // Unique memory block ID number
    LPCSTR  _szFile;                      // File that made the allocation
    UINT    _uLine;                       // Line that allocation was made

    static ULONG _uLastID;                 // last block ID used
};



#endif

#endif