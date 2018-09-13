/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    mem.c

Abstract:

    Contains memory allocation routines.

        SnmpUtilMemAlloc
        SnmpUtilMemReAlloc
        SnmpUtilMemFree

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT g_nBytesTotal = 0;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
SNMP_FUNC_TYPE
SnmpUtilMemFree(
    LPVOID pMem
    )

/*++

Routine Description:

    Releases memory used by SNMP entities.

Arguments:

    pMem - pointer to memory to release.

Return Values:

    None.

--*/

{
    // validate
    if (pMem != NULL) {

#if defined(DBG) && defined(_SNMPDLL_)

        // substract memory from global count
        g_nBytesTotal -= (UINT)GlobalSize((HGLOBAL)pMem);

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: releasing 0x%08lx (%d bytes, %d total).\n",
            pMem, GlobalSize((HGLOBAL)pMem), g_nBytesTotal
            ));

#endif

        // release memory
        GlobalFree((HGLOBAL)pMem);
    }
}


LPVOID
SNMP_FUNC_TYPE
SnmpUtilMemAlloc(
    UINT nBytes
    )

/*++

Routine Description:

    Allocates memory used by SNMP entities.

Arguments:

    nBytes - number of bytes to allocate.

Return Values:

    Returns pointer to memory.

--*/

{
    LPVOID pMem;

    // attempt to allocate memory from process heap
    pMem = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, (DWORD)nBytes);

#if defined(DBG) && defined(_SNMPDLL_)

    // add allocated memory to global count if successful
    g_nBytesTotal += (UINT)((pMem != NULL) ? GlobalSize((HGLOBAL)pMem) : 0);

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: MEM: allocated 0x%08lx (%d bytes, %d total).\n",
        pMem, (pMem != NULL) ? GlobalSize((HGLOBAL)pMem) : 0, g_nBytesTotal
        ));

#endif

    return pMem;
}


LPVOID
SNMP_FUNC_TYPE
SnmpUtilMemReAlloc(
    LPVOID pMem,
    UINT   nBytes
    )

/*++

Routine Description:

    Reallocates memory used by SNMP entities.

Arguments:

    pMem - pointer to memory to reallocate.

    nBytes - number of bytes to allocate.

Return Values:

    Returns pointer to memory.

--*/

{
    LPVOID pNew;

    // validate
    if (pMem == NULL) {

        // forward to alloc routine
        pNew = SnmpUtilMemAlloc(nBytes);

    } else {

#if defined(DBG) && defined(_SNMPDLL_)

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: expanding 0x%08lx (%d bytes) to %d bytes.\n",
            pMem, GlobalSize((HGLOBAL)pMem), nBytes
            ));

        // substract current memory from total
        g_nBytesTotal -= (UINT)GlobalSize((HGLOBAL)pMem);

#endif

        // reallocate memory
        pNew = GlobalReAlloc(
                    (HGLOBAL)pMem,
                    (DWORD)nBytes,
                    GMEM_MOVEABLE |
                    GMEM_ZEROINIT
                    );

#if defined(DBG) && defined(_SNMPDLL_)

        // add new memory to total count
        g_nBytesTotal += (UINT)((pNew != NULL) ? GlobalSize((HGLOBAL)pNew) : 0);

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: allocated 0x%08lx (%d bytes, %d total).\n",
            pNew, (pNew != NULL) ? GlobalSize((HGLOBAL)pNew) : 0, g_nBytesTotal
            ));

#endif

    }

    return pNew;
}

