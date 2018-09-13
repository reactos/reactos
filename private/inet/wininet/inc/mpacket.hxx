#ifndef MPACKET

/* Copyright (c) 1998  Microsoft Corporation

Module Name:

    mpacket.hxx

Abstract:

    class and macros for managing allocations off the heap and stack

    Contents:
        MEMORYPACKET
        ALLOC_BYTES       
        
Author:

    Ahsan S. Kabir  

Revision History:

    10Jan98 akabir
        Created

*/

// ---- Memory allocation ------------------

#define MP_MAX_STACK_USE    1024

class MEMORYPACKET
{
public:
    DWORD dwAlloc;
    DWORD dwSize;
    LPSTR psStr;

    MEMORYPACKET()
    { 
        psStr = NULL;
        dwAlloc = dwSize = 0;
    }
    ~MEMORYPACKET()
    { 
        if (psStr && dwAlloc>MP_MAX_STACK_USE)
        {
            FREE_FIXED_MEMORY(psStr);
        }
    }
};

// -- ALLOC_BYTES -----

#define ALLOC_BYTES(cb) ((cb<=MP_MAX_STACK_USE) ? \
        _alloca(cb) : \
        ALLOCATE_FIXED_MEMORY(cb))


class MEMORYPACKETTABLE
{
public:
    WORD dwNum;
    LPDWORD pdwSize;
    LPDWORD pdwAlloc;
    LPSTR* ppsStr;

    MEMORYPACKETTABLE()
    { 
        pdwAlloc = pdwSize = NULL;
        ppsStr = NULL;
        dwNum = 0;
    }
    ~MEMORYPACKETTABLE()
    { 
        if (pdwSize)
        {
            for (WORD i=0; i<dwNum; i++)
                if (pdwAlloc[i]>MP_MAX_STACK_USE)
                {
                    FREE_FIXED_MEMORY(ppsStr[i]);
                }
            FREE_FIXED_MEMORY(pdwSize);
        }
    }
    BOOL SetUpFor(WORD dwElem)
    {
        DWORD dwErr = ERROR_SUCCESS;
        BOOL fResult = FALSE;
        WORD wcE;
        
        dwNum = dwElem + 1;
        pdwSize = (LPDWORD)ALLOCATE_FIXED_MEMORY((sizeof(DWORD)*2+sizeof(LPSTR))*dwNum);
        if (!pdwSize)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        pdwAlloc = pdwSize + dwNum;
        ppsStr = (LPSTR*)(pdwAlloc + dwNum);
        for (wcE = 0; wcE < dwNum; wcE++)
        {
            ppsStr[wcE] = NULL;
            pdwSize[wcE] = 0;
            pdwAlloc[wcE] = 0;
        }
        fResult = TRUE;
        
    cleanup: 
        if (dwErr!=ERROR_SUCCESS) 
        { 
            SetLastError(dwErr); 
        }
        return fResult;
    }
};


#endif MPACKET
