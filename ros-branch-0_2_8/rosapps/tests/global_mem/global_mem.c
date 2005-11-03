/* File: global_mem.c
**
** This program is a test application used for testing correctness of the
** GlobalXXX memory API implementation.
**
** Programmer: Mark Tempel
*/
#include <windows.h>
#include <stdio.h>
#include <string.h>

/*
** All output is line wrapped to fit a 80 column screen.
** these defines control that formatting. To shrink the
** screen columns, just change the DISPLAY_COMUMNS macro.
*/
#define DISPLAY_COLUMNS 78
#define LINE_BUFFER_SIZE DISPLAY_COLUMNS + 2

/*
** This define controls the size of a memory allocation request.
** For this test suite to really be comprehensive, we should
** probably be testing many different block sizes.
*/
#define MEM_BLOCK_SIZE 0x80000

/*
** This enumeration is really the return value for the program.
** All test return a TestStatus and test statuses can be combined
** with the relation TEST_CombineStatus.
*/
typedef enum TestStatus
{
    FAILED  = 0,
    PASSED  = 1,
    SKIPPED = -1
} TEST_STATUS;

/*---------------------------------------------------------------------------
** This is a relation used to combine two test statuses.
** The combine rules are as follows:
**                   FAIL & Anything == FAIL
**                SKIPPED & Anything == Anything
**
*/
TEST_STATUS TEST_CombineStatus(TEST_STATUS a, TEST_STATUS b)
{
    TEST_STATUS result = a;

    switch (a)
    {
    case PASSED:  result = (PASSED == b || SKIPPED == b) ? (PASSED) : (FAILED); break;
    case FAILED:  result = FAILED; break;
    case SKIPPED: result = b; break;
    }

    return result;
}

/*---------------------------------------------------------------------------
** This outputs the banner border lines.
*/
void OUTPUT_BannerLine()
{
    int c = 0;
    printf("+");
    for (c = 1; c < DISPLAY_COLUMNS; c++)
    {
        printf("-");
    }
    printf("\n");
}


/*---------------------------------------------------------------------------
** This method prints a line that has a | on the left, and is line wrapped
** to be no more that DISPLAY_COLUMNS +2 wide.
*/
void OUTPUT_Line(const char *szLine)
{
    int spaceIndex = 0;
    char output[LINE_BUFFER_SIZE];

    memset(output, 0, DISPLAY_COLUMNS + 2);

    /*If this line is longer than DISPLAY_COLUMNS,
    * break it at the first space.
    */
    if (DISPLAY_COLUMNS - 2 < strlen(szLine))
    {
        for (spaceIndex = DISPLAY_COLUMNS / 2; spaceIndex < DISPLAY_COLUMNS - 2; spaceIndex++)
        {
            if (' ' == szLine[spaceIndex])
            {
                break;
            }
        }

        memcpy(output + 2, szLine, spaceIndex + 1);
        output[0] = '|';
        output[1] = ' ';
        output[strlen(output)] = '\n';
        printf(output);

        OUTPUT_Line(szLine + spaceIndex + 1);
    }
    else
    {
        sprintf(output,"| %s\n", szLine);
        printf(output);
    }

}

/*---------------------------------------------------------------------------
**
*/
void OUTPUT_Banner(const char *szBanner)
{
    OUTPUT_BannerLine();
    OUTPUT_Line(szBanner);
    OUTPUT_BannerLine();
}

/*---------------------------------------------------------------------------
**
*/
void OUTPUT_Result(TEST_STATUS status)
{
    switch (status)
    {
    case PASSED:  OUTPUT_Line("==> PASSED"); break;
    case FAILED:  OUTPUT_Line("*** FAILED"); break;
    case SKIPPED: OUTPUT_Line("==> SKIPPED"); break;
    }
    OUTPUT_Line("");
}

/*---------------------------------------------------------------------------
**
*/
void OUTPUT_HexDword(DWORD dw)
{
    char buffer[32];
    sprintf(buffer, "0x%lX",dw);
    OUTPUT_Line(buffer);
}

/*---------------------------------------------------------------------------
**
*/
void OutputAllocFlags(UINT pFlags)
{
    if (pFlags & GMEM_MOVEABLE)
    {
        OUTPUT_Line("Movable Memory");
    }
    else
    {
        OUTPUT_Line("Fixed Memory");
    }

    if (pFlags & GMEM_ZEROINIT)
    {
        OUTPUT_Line("Zero Initialized Memory");
    }
}

/*---------------------------------------------------------------------------
**
*/
void OutputErrorCode()
{
    char buffer[256];

    sprintf(buffer,"GetLastError() returned %lu", GetLastError());

    OUTPUT_Line(buffer);
}
/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TEST_MemoryWrite(LPVOID mem, DWORD cbSize)
{
    TEST_STATUS result = FAILED;

    if (0 == IsBadWritePtr(mem, cbSize))
    {
        result = PASSED;
    }
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TEST_MemoryRead(LPVOID mem, DWORD cbSize)
{
    TEST_STATUS result = FAILED;

    if (0 == IsBadReadPtr(mem, cbSize))
    {
        result = PASSED;
    }
    return result;
}

/*---------------------------------------------------------------------------
** This method tests to see if a block of global memory is movable
** by seeing if the value returned from GlobalLock is different from
** the passed in value.
*/
int IsMovable(HGLOBAL hMem)
{
    LPVOID pMem = 0;
    int    rc   = 0;

    pMem = GlobalLock(hMem);
    if (pMem != hMem)
    {
        rc = 1;
    }
    GlobalUnlock(hMem);

    return rc;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalAllocNFree(UINT allocFlags)
{
    TEST_STATUS status = SKIPPED;
    HGLOBAL hTest = 0;
    OUTPUT_Banner("Testing the GlobalAlloc and GlobalFree calls");
    OUTPUT_Line("Allocate a buffer");

    OutputAllocFlags(allocFlags);

    status = FAILED;
    hTest = GlobalAlloc(allocFlags, MEM_BLOCK_SIZE);
    if (0 != hTest)
    {
        if (0 == GlobalFree(hTest));
        {
            status = PASSED;
        }
    }

    OUTPUT_Line("Result for this run:");
    OUTPUT_Result(status);
    OUTPUT_Line("");

    return status;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalLockNUnlock(UINT allocFlags)
{
    HGLOBAL     hMem      = 0;
    LPVOID      pMem      = 0;
    TEST_STATUS subtest   = SKIPPED;
    TEST_STATUS result    = FAILED;

    OUTPUT_Banner("Testing the GlobalLock/Unlock functions.");
    OutputAllocFlags(allocFlags);
    OUTPUT_Line("");

    hMem = GlobalAlloc(allocFlags, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Allocated a memory block");

        OUTPUT_Line("Testing Lock");
        pMem = GlobalLock(hMem);
        if (0 != pMem)
        {
            OUTPUT_Result(PASSED);

            OUTPUT_Line("Testing memory for read.");
            subtest = TEST_MemoryRead(pMem, MEM_BLOCK_SIZE);
            OUTPUT_Result(subtest);
            result = TEST_CombineStatus(PASSED, subtest);


            OUTPUT_Line("Testing memory for write.");
            subtest = TEST_MemoryRead(pMem, MEM_BLOCK_SIZE);
            OUTPUT_Result(subtest);
            result = TEST_CombineStatus(result, subtest);


            OUTPUT_Line("Unlocking memory");
            if (GlobalUnlock(hMem))
            {
                OUTPUT_Result(PASSED);
                result = TEST_CombineStatus(result, PASSED);
            }
            else
            {
                if (NO_ERROR == GetLastError())
                {
                    OUTPUT_Result(PASSED);
                    result = TEST_CombineStatus(result, PASSED);
                }
                else
                {
                    OutputErrorCode();
                    OUTPUT_Result(FAILED);
                    result = TEST_CombineStatus(result, FAILED);
                }
            }
        }

        OUTPUT_Line("Freeing memory");
        if (0 == GlobalFree(hMem))
        {
            OUTPUT_Result(PASSED);
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            OutputErrorCode();
            OUTPUT_Result(FAILED);
            result = TEST_CombineStatus(result, FAILED);
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalReAllocFixed()
{
    HGLOBAL     hMem       = 0;
    HGLOBAL     hReAlloced = 0;
    LPVOID      pMem       = 0;
    TEST_STATUS subtest    = SKIPPED;
    TEST_STATUS result     = SKIPPED;

    OUTPUT_Line("Testing GlobalReAlloc() on memory allocated as GMEM_FIXED");

    /* Case 1: convert a fixed block to a movable block. */
    OUTPUT_Line("Allocating buffer");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Testing to see if this is converted into a movable block");
        hReAlloced = GlobalReAlloc(hMem, MEM_BLOCK_SIZE + 100, GMEM_MOVEABLE | GMEM_MODIFY);
        if (0 == hReAlloced)
        {
            OUTPUT_Line("GlobalReAlloc failed-- returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
            OUTPUT_Result(subtest);
        }
        else
        {
            hMem = hReAlloced;
            if (0 == IsMovable(hMem))
            {
                OUTPUT_Line("GlobalReAlloc returned a fixed pointer.");
                subtest = TEST_CombineStatus(subtest, FAILED);
                OUTPUT_Result(subtest);
            }
            else
            {
                pMem = GlobalLock(hMem);
                subtest = TEST_CombineStatus(subtest, PASSED);
                subtest = TEST_CombineStatus(subtest, TEST_MemoryRead(pMem, MEM_BLOCK_SIZE + 100));
                if (FAILED == subtest)
                {
                    OUTPUT_Line("Memory Read failed.");
                }
                subtest = TEST_CombineStatus(subtest, TEST_MemoryWrite(pMem, MEM_BLOCK_SIZE + 100));
                if (FAILED == subtest)
                {
                    OUTPUT_Line("Memory Write failed.");
                }
                GlobalUnlock(hMem);
            }
        }

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("Global Alloc Failed.");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }
    OUTPUT_Line("Subtest result:");
    OUTPUT_Result(subtest);
    OUTPUT_Line("");

    result = TEST_CombineStatus(result, subtest);
    subtest = SKIPPED;

    /* case 2 only move a fixed block */
    OUTPUT_Line("Allocating buffer");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Testing to see if a new fixed block is returned.");
        hReAlloced = GlobalReAlloc(hMem, MEM_BLOCK_SIZE - 100, GMEM_MOVEABLE);
        if (0 == hReAlloced)
        {
            OUTPUT_Line("GlobalReAlloc failed-- returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
            OUTPUT_Result(subtest);
        }
        else
        {
            OUTPUT_Line("Alloced Handle: ");
            OUTPUT_HexDword((DWORD)hMem);
            OUTPUT_Line("ReAlloced Handle: ");
            OUTPUT_HexDword((DWORD)hReAlloced);
            if (hMem == hReAlloced)
            {
                OUTPUT_Line("GlobalReAlloc returned the same pointer.  The documentation states that this is wrong, but Windows NT works this way.");
            }

            hMem = hReAlloced;
            subtest = TEST_CombineStatus(subtest, PASSED);
            subtest = TEST_CombineStatus(subtest, TEST_MemoryRead((LPVOID)hMem, MEM_BLOCK_SIZE - 100));
            subtest = TEST_CombineStatus(subtest, TEST_MemoryWrite((LPVOID)hMem, MEM_BLOCK_SIZE - 100));
        }

        GlobalFree(hMem);
    }
    else
    {
        subtest = TEST_CombineStatus(subtest, FAILED);
    }
    OUTPUT_Line("Subtest result:");
    OUTPUT_Result(subtest);
    OUTPUT_Line("");

    result = TEST_CombineStatus(result, subtest);
    subtest = SKIPPED;

    /* Case 3: re-allocate a fixed block in place */
    OUTPUT_Line("Allocating buffer");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Testing to see if a fixed pointer is reallocated in place.");
        hReAlloced = GlobalReAlloc(hMem, MEM_BLOCK_SIZE - 100, 0);
        if (0 == hReAlloced)
        {
            OUTPUT_Line("GlobalReAlloc failed-- returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
            OUTPUT_Result(subtest);
        }
        else
        {
            OUTPUT_Line("Alloced Handle: ");
            OUTPUT_HexDword((DWORD)hMem);
            OUTPUT_Line("ReAlloced Handle: ");
            OUTPUT_HexDword((DWORD)hReAlloced);
            if (hMem != hReAlloced)
            {
                OUTPUT_Line("GlobalReAlloc returned a different.");
                subtest = TEST_CombineStatus(subtest, FAILED);
                OUTPUT_Result(subtest);
            }
            else
            {
                subtest = TEST_CombineStatus(subtest, PASSED);
                subtest = TEST_CombineStatus(subtest, TEST_MemoryRead((LPVOID)hMem, MEM_BLOCK_SIZE - 100));
                subtest = TEST_CombineStatus(subtest, TEST_MemoryWrite((LPVOID)hMem, MEM_BLOCK_SIZE - 100));
            }
        }

        GlobalFree(hMem);
    }
    else
    {
        subtest = TEST_CombineStatus(subtest, FAILED);
    }
    OUTPUT_Line("Subtest result:");
    OUTPUT_Result(subtest);
    OUTPUT_Line("");

    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Line("");
    return result;
}
/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalReAllocMovable()
{
    HGLOBAL     hMem       = 0;
    HGLOBAL     hReAlloced = 0;
    LPVOID      pMem       = 0;
    TEST_STATUS subtest    = SKIPPED;
    TEST_STATUS result     = SKIPPED;

    OUTPUT_Line("Testing GlobalReAlloc() on memory allocated as GMGM_MOVEABLE");

    /* case 1 test reallocing a movable block that is unlocked. */
    OUTPUT_Line("Allocating buffer");
    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Testing GlobalReAlloc on a unlocked block.");
        hReAlloced = GlobalReAlloc(hMem, MEM_BLOCK_SIZE - 100, 0);
        if (0 == hReAlloced)
        {
            OUTPUT_Line("GlobalReAlloc failed-- returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
            OUTPUT_Result(subtest);
        }
        else
        {
            OUTPUT_Line("Alloced Handle: ");
            OUTPUT_HexDword((DWORD)hMem);
            OUTPUT_Line("ReAlloced Handle: ");
            OUTPUT_HexDword((DWORD)hReAlloced);

            pMem = GlobalLock(hReAlloced);
            hMem = hReAlloced;
            subtest = TEST_CombineStatus(subtest, PASSED);
            subtest = TEST_CombineStatus(subtest, TEST_MemoryRead(pMem, MEM_BLOCK_SIZE - 100));
            subtest = TEST_CombineStatus(subtest, TEST_MemoryWrite(pMem, MEM_BLOCK_SIZE - 100));
            GlobalUnlock(hReAlloced);
        }

        GlobalFree(hMem);
    }
    else
    {
        subtest = TEST_CombineStatus(subtest, FAILED);
    }
    OUTPUT_Line("Subtest result:");
    OUTPUT_Result(subtest);
    OUTPUT_Line("");

    result = TEST_CombineStatus(result, subtest);
    subtest = SKIPPED;

    /* Case 2: re-allocate a moveable block that is locked */
    OUTPUT_Line("Allocating buffer");
    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {

        OUTPUT_Line("Testing GlobalReAlloc on a locked block.");
        pMem = GlobalLock(hMem);
        hReAlloced = GlobalReAlloc(hMem, MEM_BLOCK_SIZE - 100, 0);
        if (0 == hReAlloced)
        {
            OUTPUT_Line("GlobalReAlloc failed-- returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
            OUTPUT_Result(subtest);
        }
        else
        {
            OUTPUT_Line("Alloced Handle: ");
            OUTPUT_HexDword((DWORD)hMem);
            OUTPUT_Line("ReAlloced Handle: ");
            OUTPUT_HexDword((DWORD)hReAlloced);
            if (hMem != hReAlloced)
            {
                OUTPUT_Line("GlobalReAlloc returned a different block.");
            }
            pMem = GlobalLock(hReAlloced);
            subtest = TEST_CombineStatus(subtest, PASSED);
            subtest = TEST_CombineStatus(subtest, TEST_MemoryRead(pMem, MEM_BLOCK_SIZE - 100));
            subtest = TEST_CombineStatus(subtest, TEST_MemoryWrite(pMem , MEM_BLOCK_SIZE - 100));
            GlobalUnlock(hReAlloced);

        }

        GlobalUnlock(hMem);

        GlobalFree(hMem);
    }
    else
    {
        subtest = TEST_CombineStatus(subtest, FAILED);
    }
    OUTPUT_Line("Subtest result:");
    OUTPUT_Result(subtest);
    OUTPUT_Line("");

    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Line("");
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalReAlloc()
{
    OUTPUT_Banner("Testing GlobalReAlloc()");
    TEST_STATUS result = SKIPPED;

    result = TEST_CombineStatus(result, TestGlobalReAllocFixed());
    result = TEST_CombineStatus(result, TestGlobalReAllocMovable());

    OUTPUT_Line("GlobalReAlloc test result:");
    OUTPUT_Result(result);
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalFlagsMoveable()
{
    HGLOBAL     hMem   = 0;
    UINT        uFlags = 0;
    TEST_STATUS result = SKIPPED;

    OUTPUT_Line("Test for the proper lock count");

    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {

        OUTPUT_Line("Testing initial allocation");

        OUTPUT_Line("Testing for a lock of 0");
        uFlags = GlobalFlags(hMem);
        if (((GMEM_LOCKCOUNT & uFlags) == 0)) /*no locks*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Result(result);

        OUTPUT_Line("Pointer from handle: ");
        OUTPUT_HexDword((DWORD)GlobalLock(hMem));

        OUTPUT_Line("Testing after a lock");
        OUTPUT_Line("Testing for a lock of 1");
        uFlags = GlobalFlags(hMem);
        if (((GMEM_LOCKCOUNT & uFlags) == 1)) /*no locks*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Result(result);

        GlobalUnlock(hMem);
        OUTPUT_Line("Testing after an unlock");
        OUTPUT_Line("Testing for a lock of 0");
        uFlags = GlobalFlags(hMem);
        if (((GMEM_LOCKCOUNT & uFlags) == 0)) /*no locks*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Result(result);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        result = TEST_CombineStatus(result, FAILED);
    }

    OUTPUT_Line("Test for discarded memory");
    OUTPUT_Line("Allocating an empty moveable block---automatically marked as discarded");
    hMem = GlobalAlloc(GMEM_MOVEABLE, 0); /*allocate a discarded block*/
    if (0 != hMem)
    {
        OUTPUT_Line("Allocation handle: ");
        OUTPUT_HexDword((DWORD)hMem);
        OUTPUT_Line("Testing for a discarded flag");
        uFlags = GlobalFlags(hMem);
        if (0 != (uFlags & GMEM_DISCARDED)) /*discarded*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Line("Flags:");
        OUTPUT_HexDword(uFlags);
        OUTPUT_Result(result);

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        result = TEST_CombineStatus(result, FAILED);
    }
    return result;
}


/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalFlagsFixed()
{
    HGLOBAL     hMem   = 0;
    UINT        uFlags = 0;
    TEST_STATUS result = SKIPPED;

    OUTPUT_Line("Testing for correct handling of GMEM_FIXED memory");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {

        OUTPUT_Line("Allocation handle: ");
        OUTPUT_HexDword((DWORD)hMem);

        OUTPUT_Line("Testing initial allocation");
        OUTPUT_Line("Testing for non-discarded and lock of 0");
        uFlags = GlobalFlags(hMem);
        if (((GMEM_LOCKCOUNT & uFlags) == 0) && /*no locks*/
            (((uFlags >> 8) & 0xff) == 0 ))   /*not discarded*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Result(result);

        OUTPUT_Line("Pointer from handle: ");
        OUTPUT_HexDword((DWORD)GlobalLock(hMem));
        OUTPUT_Line("Testing after a lock");
        OUTPUT_Line("Testing for non-discarded and lock of 0");
        uFlags = GlobalFlags(hMem);
        if (((GMEM_LOCKCOUNT & uFlags) == 0) && /*no locks*/
            (((uFlags >> 8) & 0xff) == 0 ))   /*not discarded*/
        {
            result = TEST_CombineStatus(result, PASSED);
        }
        else
        {
            result = TEST_CombineStatus(result, FAILED);
        }
        OUTPUT_Result(result);

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        result = TEST_CombineStatus(result, FAILED);
    }

    return result;
}
/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalFlags()
{
    OUTPUT_Banner("Testing GlobalFlags()");
    TEST_STATUS result = SKIPPED;

    result = TEST_CombineStatus(result, TestGlobalFlagsFixed());
    result = TEST_CombineStatus(result, TestGlobalFlagsMoveable());

    OUTPUT_Line("GlobalFlags result:");
    OUTPUT_Result(result);
    return result;
}
/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalHandle()
{
    HGLOBAL     hMem    = 0;
    HGLOBAL     hTest   = 0;
    PVOID       pMem    = 0;
    TEST_STATUS subtest = SKIPPED;
    TEST_STATUS result  = SKIPPED;

    OUTPUT_Banner("Testing GlobalHandle()");

    OUTPUT_Line("Testing GlobalHandle with a block of GMEM_FIXED memory");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {

        OUTPUT_Line("Allocation handle: ");
        OUTPUT_HexDword((DWORD)hMem);

        hTest = GlobalHandle(hMem);
        if (hMem == hTest)
        {
            subtest = TEST_CombineStatus(subtest, PASSED);
        }
        else
        {
            OUTPUT_Line("GlobalHandle returned:");
            OUTPUT_HexDword((DWORD)hTest);
            subtest = TEST_CombineStatus(subtest, FAILED);
        }

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);
    result = TEST_CombineStatus(result, subtest);


    subtest = SKIPPED;
    OUTPUT_Line("Testing GlobalHandle with a block of GMEM_MOVEABLE memory");
    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {

        OUTPUT_Line("Allocation handle: ");
        OUTPUT_HexDword((DWORD)hMem);
        pMem = GlobalLock(hMem);
        hTest = GlobalHandle(pMem);
        if (hMem == hTest)
        {
            subtest = TEST_CombineStatus(subtest, PASSED);
        }
        else
        {
            OUTPUT_Line("GlobalHandle returned:");
            OUTPUT_HexDword((DWORD)hTest);
            subtest = TEST_CombineStatus(subtest, FAILED);
        }

        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);
    result = TEST_CombineStatus(result, subtest);


    OUTPUT_Line("Global Handle test results:");
    OUTPUT_Result(result);
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalSize()
{
    HGLOBAL hMem = 0;
    SIZE_T  size = 0;
    TEST_STATUS subtest = SKIPPED;
    TEST_STATUS result  = SKIPPED;
    OUTPUT_Banner("Testing GlobalSize()");

    OUTPUT_Line("Testing GlobalSize with a block of GMEM_FIXED memory");
    hMem = GlobalAlloc(GMEM_FIXED, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        size = GlobalSize(hMem);
        if (MEM_BLOCK_SIZE <= size)
        {
            subtest = TEST_CombineStatus(subtest, PASSED);
        }
        else
        {
            OUTPUT_Line("GlobalSize returned:");
            OUTPUT_HexDword(size);
            subtest = TEST_CombineStatus(subtest, FAILED);
        }

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);
    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Line("Testing GlobalSize with a block of GMEM_MOVEABLE memory");
    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        size = GlobalSize(hMem);
        if (MEM_BLOCK_SIZE <= size)
        {
            subtest = TEST_CombineStatus(subtest, PASSED);
        }
        else
        {
            OUTPUT_Line("GlobalSize returned:");
            OUTPUT_HexDword(size);
            subtest = TEST_CombineStatus(subtest, FAILED);
        }

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);
    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Line("Testing GlobalSize with discarded memory");
    hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (0 != hMem)
    {
        size = GlobalSize(hMem);
        if (0 == size)
        {
            subtest = TEST_CombineStatus(subtest, PASSED);
        }
        else
        {
            OUTPUT_Line("GlobalSize returned:");
            OUTPUT_HexDword(size);
            subtest = TEST_CombineStatus(subtest, FAILED);
        }

        GlobalFree(hMem);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);
    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Line("Test result:");
    OUTPUT_Result(result);
    return result;
}

/*---------------------------------------------------------------------------
**
*/
TEST_STATUS TestGlobalDiscard()
{
    HGLOBAL     hMem    = 0;
    HGLOBAL     hTest   = 0;
    DWORD       uFlags  = 0;
    TEST_STATUS subtest = SKIPPED;
    TEST_STATUS result  = SKIPPED;

    OUTPUT_Banner("Testing GlobalDiscard()");
    hMem = GlobalAlloc(GMEM_MOVEABLE, MEM_BLOCK_SIZE);
    if (0 != hMem)
    {
        OUTPUT_Line("Allocation handle: ");
        OUTPUT_HexDword((DWORD)hMem);

        hTest = GlobalDiscard(hMem);
        if (0 == hTest)
        {
            OUTPUT_Line("GlobalDiscard returned NULL");
            subtest = TEST_CombineStatus(subtest, FAILED);
        }
        else
        {
            uFlags = GlobalFlags(hTest);
            OUTPUT_Line("Flags from the new memory block.");
            OUTPUT_HexDword(uFlags);
            if (0 != (uFlags & GMEM_DISCARDED))
            {
                subtest = TEST_CombineStatus(subtest, PASSED);
            }
            else
            {
                OUTPUT_Line("Block is not marked as discardable.");
                subtest = TEST_CombineStatus(subtest, FAILED);
            }
        }

        GlobalFree(hTest);
    }
    else
    {
        OUTPUT_Line("GlobalAlloc failed!");
        subtest = TEST_CombineStatus(subtest, FAILED);
    }

    OUTPUT_Line("Result from subtest:");
    OUTPUT_Result(subtest);

    result = TEST_CombineStatus(result, subtest);

    OUTPUT_Result(result);
    return result;
}

/*---------------------------------------------------------------------------
**
*/
int main(int argc, char ** argv)
{
    TEST_STATUS test_set = SKIPPED;
    OUTPUT_Banner("Testing GlobalXXX memory management functions.");

    test_set = TEST_CombineStatus(test_set, TestGlobalAllocNFree(GPTR));
    test_set = TEST_CombineStatus(test_set, TestGlobalAllocNFree(GHND));
    test_set = TEST_CombineStatus(test_set, TestGlobalAllocNFree(GMEM_FIXED));
    test_set = TEST_CombineStatus(test_set, TestGlobalAllocNFree(GMEM_MOVEABLE));

    if (FAILED == test_set)
    {
        OUTPUT_Line("Skipping any further tests.  GlobalAlloc/Free fails.");
        OUTPUT_Result(test_set);
        return test_set;
    }

    test_set = TEST_CombineStatus(test_set, TestGlobalLockNUnlock(GPTR));
    test_set = TEST_CombineStatus(test_set, TestGlobalLockNUnlock(GHND));
    test_set = TEST_CombineStatus(test_set, TestGlobalLockNUnlock(GMEM_FIXED));
    test_set = TEST_CombineStatus(test_set, TestGlobalLockNUnlock(GMEM_MOVEABLE));

    test_set = TEST_CombineStatus(test_set, TestGlobalReAlloc());

    test_set = TEST_CombineStatus(test_set, TestGlobalFlags());

    test_set = TEST_CombineStatus(test_set, TestGlobalHandle());

    test_set = TEST_CombineStatus(test_set, TestGlobalSize());

    test_set = TEST_CombineStatus(test_set, TestGlobalDiscard());

    /* output the result for the entire set of tests*/
    OUTPUT_Banner("Test suite result");
    OUTPUT_Result(test_set);
    return test_set;
}
