//
//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "precomp.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "deskcpl"
#define SZ_MODULE           "DESKCPL"
#define DECLARE_DEBUG
#include "debug.h"


// Include the standard helper functions to dump common ADTs
//#include "..\lib\dump.c"


#ifdef DEBUG

//
// Typedefs
//
typedef struct _ALLOCHEADER {
    LIST_ENTRY  ListEntry;
    PTCHAR      File;
    ULONG       Line;
    LONG        AllocNumber;
    ULONG       Size;
} ALLOCHEADER, *PALLOCHEADER;


//
// Globals
//
LIST_ENTRY AllocListHead =
{
    &AllocListHead,
    &AllocListHead
};

#undef LocalAlloc
#undef LocalReAlloc
#undef LocalFree

INT g_BreakAtAlloc = -1;
INT g_BreakAtFree = -1;
ULONG g_AllocNumber = 0;

#ifdef WINNT
    #define TRAP() DbgBreakPoint()
#else
    #define TRAP() _asm {int 3};
#endif

//*****************************************************************************
//
// MyAlloc()
//
//*****************************************************************************

HLOCAL
DeskAllocPrivate (
    const TCHAR *File,
    ULONG       Line,
    ULONG       Flags,
    DWORD       dwBytes
)
{
    static ULONG allocNumber = 0;
    DWORD bytes;
    PALLOCHEADER header;

    if (dwBytes) {
        bytes = dwBytes + sizeof(ALLOCHEADER);

        header = (PALLOCHEADER)LocalAlloc(Flags, bytes);

        if (header != NULL) {
            InsertTailList(&AllocListHead, &header->ListEntry);

            header->File = (TCHAR*) File;
            header->Line = Line;
            header->AllocNumber = ++allocNumber;
            header->Size = dwBytes;

            if (header->AllocNumber == g_BreakAtAlloc) {
                // user set assert
                TRAP();
            }

            return (HLOCAL)(header + 1);
        }
    }

    return NULL;
}

//*****************************************************************************
//
// MyReAlloc()
//
//*****************************************************************************

HLOCAL
DeskReAllocPrivate (
    const TCHAR *File,
    ULONG       Line,
    HLOCAL      hMem,
    DWORD       dwBytes,
    ULONG       Flags
    )
{
    PALLOCHEADER header;
    PALLOCHEADER headerNew;

    if (hMem)
    {
        header = (PALLOCHEADER)hMem;

        header--;

        // Remove the old address from the allocation list
        //
        RemoveEntryList(&header->ListEntry);

        headerNew = LocalReAlloc((HLOCAL)header, dwBytes, Flags);

        if (headerNew != NULL)
        {
            // Add the new address to the allocation list
            //
            headerNew->File = (TCHAR*) File;
            headerNew->Line = Line;
            headerNew->AllocNumber = ++g_AllocNumber;
            headerNew->Size = dwBytes;

            if (headerNew->AllocNumber == g_BreakAtAlloc) {
                // user set assert
                TRAP();
            }

            InsertTailList(&AllocListHead, &headerNew->ListEntry);

            return (HLOCAL)(headerNew + 1);
        }
        else
        {
            // If GlobalReAlloc fails, the original memory is not freed,
            // and the original handle and pointer are still valid.
            // Add the old address back to the allocation list.
            //
            InsertTailList(&AllocListHead, &header->ListEntry);
        }

    }

    return NULL;
}


//*****************************************************************************
//
// MyFree()
//
//*****************************************************************************

HLOCAL
DeskFreePrivate (
    HLOCAL hMem
)
{
    PALLOCHEADER header;
    TCHAR buf[128];

    if (hMem)
    {
        header = (PALLOCHEADER)hMem;
        header--;

        if (header->AllocNumber == g_BreakAtFree) {
            TRAP();
        }

        wsprintf(buf, TEXT("free alloc number %d, size %d\r\n"), 
                 header->AllocNumber, header->Size);

        RemoveEntryList(&header->ListEntry);

        return LocalFree((HLOCAL)header);
    }
 
    return LocalFree(hMem);
}

HLOCAL  DeskFreeDirect (HLOCAL hMem)

{
    return(LocalFree(hMem));
}

//*****************************************************************************
//
// MyCheckForLeaks()
//
//*****************************************************************************

VOID
DeskCheckForLeaksPrivate (
    VOID
)
{
    PALLOCHEADER  header;
    TCHAR         buf[1024+40], tmpBuf[512];
    unsigned int  i, size, size2, ic;
    DWORD         *pdw;
    char          *pch, *pch2;
    LPVOID        mem;

#if UNICODE 
    #define DeskIsPrintable iswprint
#else
    #define DeskIsPrintable isprint
#endif

    while (!IsListEmpty(&AllocListHead))
    {
        header = (PALLOCHEADER)RemoveHeadList(&AllocListHead);
        mem = header + 1;
            
        wsprintf(buf, TEXT("Desk.cpl mem leak in File:  %s\r\n Line: %d Size:  %d  Allocation:  %d Buffer:  0x%x\r\n"),
                 header->File, header->Line, header->Size, header->AllocNumber, mem);
        OutputDebugString(buf);

        //
        // easy stuff, print out all the 4 DWORDS we can 
        //
        pdw = (DWORD *) mem;
        pch = (char *) mem;
        *buf = TEXT('\0');
        for (i = 0; i < header->Size/16; i++, pdw += 4) {
            wsprintf(tmpBuf, TEXT(" %08x %08x %08x %08x   "),
                     pdw[0], pdw[1], pdw[2], pdw[3]);
            lstrcat(buf, tmpBuf);

            for (ic = 0; ic < 16; ic++, pch++) {
                tmpBuf[ic] = DeskIsPrintable(*pch) ? *pch : TEXT('.');
            }
            tmpBuf[ic] =  TEXT('\0');
            lstrcat(buf, tmpBuf);
            OutputDebugString(buf);
            OutputDebugString(TEXT("\n"));

            *buf = TEXT('\0');
        }

        //
        // Is there less than a 16 byte chunk left?
        //
        size = header->Size % 16;
        if (size) {
            //
            // Print all the DWORDs we can
            //
            for (i = 0; i < size / 4; i++, pdw++) {
                wsprintf(tmpBuf, TEXT(" %08x"), *pdw);
                lstrcat(buf, tmpBuf);
            }

            if (size % 4) {
                // 
                // Print the remaining bytes
                // 
                lstrcat(buf, TEXT(" "));

                pch2 = (char*) pdw;
                for (i = 0; i < size % 4; i++, pch2++) {
                    wsprintf(tmpBuf, TEXT("%02x"), (DWORD) *pch2);
                    lstrcat(buf, tmpBuf);
                }

                //
                // Align with 4 bytes
                //
                for ( ; i < 4; i++) {
                    lstrcat(buf, TEXT("  "));
                }
            }

            //
            // Print blanks for any remaining DWORDs (ie to match the 4 above)
            //
            size2 = (16 - (header->Size % 16)) / 4;
            for (i = 0; i < size2; i++) {
                lstrcat(buf, TEXT("         "));
            }

            lstrcat(buf, TEXT("   "));
            
            //
            // Print the actual remain bytes as chars
            //
            for (i = 0; i < size; i++, pch++) {
                tmpBuf[i] = DeskIsPrintable(*pch) ? *pch : TEXT('.');
            }
            tmpBuf[i] = TEXT('\0');
            lstrcat(buf, tmpBuf);

            OutputDebugString(buf);
            OutputDebugString(TEXT("\n"));
        }

        OutputDebugString(TEXT("\n"));
    }
}

#endif
