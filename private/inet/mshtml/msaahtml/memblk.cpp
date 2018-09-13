#define OLDNEW
#include "stdafx.h"
#undef MYnew
#undef new

#if defined( _DEBUG)

#include "windows.h"
#include "assert.h"
#include "memblk.hpp"
#include "malloc.h"


// set the block counter to 0
ULONG CMemBlk::_uLastID = 0;


CMemBlk::CMemBlk(UINT uSize, LPCSTR szFile, UINT uLine)
{
    UINT     i;

    // Get the next unique block number
    _uBlockID = ++_uLastID;

    // Save the file name and line number information
    _szFile = szFile;
    _uLine = uLine;

    _fAllocated = TRUE;
    if(uSize == 0)
    {
        OutputDebugString(_T("\r\nMEMMGR ERROR: 0 size memory block allocation\r\n"));
        assert(0);
        _pMemBlock = NULL;
        return;
    }

    _uRequestedSize = uSize;
    
    _pMemBlock = (BYTE *)malloc(GetRealSize());
    if(_pMemBlock ==  NULL)
        return;

    // Do the padding (skip the class pointer at the beginning
    for(i = sizeof(CMemBlk *); i < sizeof(CMemBlk *) + NUMPADBYTES; i++)
    {
        _pMemBlock[i] = PADVALUE;
    }

    for(i = sizeof(CMemBlk *) + NUMPADBYTES + uSize; i < GetRealSize(); i++)
    {
        _pMemBlock[i] = PADVALUE;
    }

    if(FILLMEMORY)
    {
        for(i = sizeof(CMemBlk *) + NUMPADBYTES; i < sizeof(CMemBlk *) + NUMPADBYTES + uSize; i++)
        {
            if(RANDOMFILL)
                _pMemBlock[i] = rand();
            else
                _pMemBlock[i] = FILLVALUE;
        }
    }

    if(TRACEMEMALLOCS)
    {
        OutputDebugString(_T("   MEMMGR TRACE:The following memory block is allocated: \r\n  "));
        Dump(3);
    }

    // Keep the pointer to the class inside the memory block
    *((CMemBlk **)_pMemBlock) = this;
}


// Check the memory block, fill the it with known value and mark it as released.
// We actually release the memory in destructor
BOOL CMemBlk::FreeMem()
{
    if(!_fAllocated)
    {
        OutputDebugString(_T("MEMMGR ERROR:Trying to free memory block that was already released:\r\n  "));
        Dump();
        return FALSE;
    }


    if(!ArePadBytesOK())
    {
        OutputDebugString(_T("MEMMGR ERROR:Padded bytes have been overwritten for following memory block:\r\n  "));
        Dump();
    }


    if(TRACEMEMALLOCS)
    {
        OutputDebugString(_T("  MEMMGR TRACE:The following memory block is released: \r\n  "));
        Dump(3);
    }

    _fAllocated = FALSE;

    for(UINT i = sizeof(CMemBlk *) + NUMPADBYTES; i < sizeof(CMemBlk *) + NUMPADBYTES + _uRequestedSize; i++)
    {
        _pMemBlock[i] = FREEVALUE;
    }

    return TRUE;
}


// Check for memory writes outside allocated block and 
// returns the first padding byte that is different
BOOL const CMemBlk::ArePadBytesOK(UINT *puFirstOffset)
{
    UINT i;

    // Do the padding
    for(i = sizeof(CMemBlk *); i < sizeof(CMemBlk *) + NUMPADBYTES; i++)
    {
        if(_pMemBlock[i] != PADVALUE)
        {
            if(puFirstOffset != NULL)
                *puFirstOffset = i;
            return FALSE;
        }
    }

    for(i = sizeof(CMemBlk *) + NUMPADBYTES + _uRequestedSize; i < GetRealSize(); i++)
    {
        if(_pMemBlock[i] != PADVALUE)
        {
            if(puFirstOffset != NULL)
                *puFirstOffset = i;
            return FALSE;
        }
    }

    return TRUE;
}
// Check for free bytes 
BOOL const CMemBlk::AreFillBytesOK(UINT *puFirstOffset)
{
    assert(!_fAllocated);

    for(UINT i = sizeof(CMemBlk *) + NUMPADBYTES; i < sizeof(CMemBlk *) + NUMPADBYTES + _uRequestedSize; i++)
    {
        if(_pMemBlock[i] != FREEVALUE)
        {
            if(puFirstOffset != NULL)
                *puFirstOffset = i;
            return FALSE;
        }
    }

    return TRUE;
}


// Send memory block information to debug output
void const CMemBlk::Dump(int nMargin /* = 0 */)
{
    char szBuf[200 + 3 * MAXBYTESTODUMP], szHex[10];
    int  nNumChars;

    for(int i = 0; i < nMargin; i++)
        OutputDebugString(_T(" "));

    if(_fAllocated)
    {
        nNumChars = sprintf(szBuf, "Allocated Block #%lu, Size %lu, Address %#lx, File %s, Line %lu\r\n",
            _uBlockID,  _uRequestedSize, (ULONG)_pMemBlock, _szFile, _uLine);

        for(int j = 0; j < nMargin; j++)
            lstrcatA(szBuf, " ");
        UINT uNumToDump = min(_uRequestedSize, MAXBYTESTODUMP);
        for(UINT i = sizeof(CMemBlk *) + NUMPADBYTES; i < sizeof(CMemBlk *) + NUMPADBYTES + uNumToDump; i++)
        {
            sprintf(szHex, "%02lx ", (ULONG)_pMemBlock[i]);
            lstrcatA(szBuf, szHex);
        }
        lstrcatA(szBuf, "\r\n");

    }
    else
    {
        nNumChars = sprintf(szBuf, "Free Block #%lu, Size %lu, Address %#lx, File %s, Line %lu\r\n",
            _uBlockID,  _uRequestedSize, (ULONG)_pMemBlock, _szFile, _uLine);
        
    }

    assert(nNumChars > 0);

    OutputDebugStringA(szBuf);
}


CMemBlk::~CMemBlk()
{
    if(_fAllocated)
    {
        OutputDebugString(_T("MEMMGR ERROR:Following memory block has not been released:\r\n"));
        Dump();
    }
    else
    {
        if(!AreFillBytesOK())
        {
            OutputDebugString(_T("MEMMGR ERROR:Released memory has been changed for following memory block:\r\n  "));
            Dump();
        }
    }

    if(!ArePadBytesOK())
    {
        OutputDebugString(_T("MEMMGR ERROR:Padded bytes have been overwritten for following memory block:\r\n  "));
        Dump();
    }
        
    free(_pMemBlock);
}

void *CMemBlk::operator new( size_t stAllocateBlock)
{
     return malloc( stAllocateBlock );
}

void CMemBlk::operator delete( void *pMem)
{
     free(pMem);
}

#endif
