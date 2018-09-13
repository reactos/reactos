//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cvol.c
//
//  This files contains code for the cached volume ID structs.
//
// History:
//  09-02-93 ScottH     Created
//  01-31-94 ScottH     Moved from cache.c
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers

/////////////////////////////////////////////////////  TYPEDEFS

/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

/////////////////////////////////////////////////////  MODULE DATA

CACHE g_cacheCVOL = {0, 0, 0};       // Volume ID cache

/////////////////////////////////////////////////////  Generic Cache Routines


#ifdef DEBUG
void PRIVATE CVOL_DumpEntry(
    CVOL  * pcvol)
    {
    ASSERT(pcvol);

    TRACE_MSG(TF_ALWAYS, TEXT("CVOL:  Atom %d: %s"), pcvol->atomPath, Atom_GetName(pcvol->atomPath));
    TRACE_MSG(TF_ALWAYS, TEXT("               Ref [%u]  Hvid = %lx"), 
        Cache_GetRefCount(&g_cacheCVOL, pcvol->atomPath),
        pcvol->hvid);
    }


void PUBLIC CVOL_DumpAll()
    {
    CVOL  * pcvol;
    int atom;
    BOOL bDump;

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CVOL);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    atom = Cache_FindFirstKey(&g_cacheCVOL);
    while (atom != ATOM_ERR)
        {
        pcvol = Cache_GetPtr(&g_cacheCVOL, atom);
        ASSERT(pcvol);
        if (pcvol)
            {
            CVOL_DumpEntry(pcvol);
            Cache_DeleteItem(&g_cacheCVOL, atom, FALSE);    // Decrement count
            }

        atom = Cache_FindNextKey(&g_cacheCVOL, atom);
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Release the volume ID handle
Returns: --
Cond:    --
*/
void CALLBACK CVOL_Free(
    LPVOID lpv)
    {
    CVOL  * pcvol = (CVOL  *)lpv;

    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CVOL  Releasing volume ID %s"), Atom_GetName(pcvol->atomPath)); )

    ASSERT(Sync_IsEngineLoaded());

    Sync_ReleaseVolumeIDHandle(pcvol->hvid);

    SharedFree(&pcvol);
    }


/*----------------------------------------------------------
Purpose: Add the atomPath to the cache.  We add the volume ID.
          If atomPath is already in the cache, we replace it
          with a newly obtained volume ID.

Returns: Pointer to CVOL
         NULL on OOM

Cond:    --
*/
CVOL  * PUBLIC CVOL_Replace(
    int atomPath)
    {
    CVOL  * pcvol;
    BOOL bJustAllocd;
    
    pcvol = Cache_GetPtr(&g_cacheCVOL, atomPath);
    if (pcvol)
        bJustAllocd = FALSE;
    else
        {
        // Allocate using commctrl's Alloc, so the structure will be in
        // shared heap space across processes.
        pcvol = SharedAllocType(CVOL);
        bJustAllocd = TRUE;
        }

    if (pcvol)
        {
        HVOLUMEID hvid;
        LPCTSTR pszPath = Atom_GetName(atomPath);

        ASSERT(pszPath);

        DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CVOL  Adding volume ID %s"), pszPath); )

        if (Sync_GetVolumeIDHandle(pszPath, &hvid) != TR_SUCCESS)
            {
            if (bJustAllocd)
                SharedFree(&pcvol);
            else
                Cache_DeleteItem(&g_cacheCVOL, atomPath, FALSE);    // Decrement count

            pcvol = NULL;       // Fail
            }
        else
            {
            ENTEREXCLUSIVE()
                {
                pcvol->atomPath = atomPath;
                pcvol->hvid = hvid;
                }
            LEAVEEXCLUSIVE()

            if (bJustAllocd)
                {
                if (!Cache_AddItem(&g_cacheCVOL, atomPath, (LPVOID)pcvol))
                    {
                    // Cache_AddItem failed here
                    //
                    Sync_ReleaseVolumeIDHandle(hvid);
                    SharedFree(&pcvol);
                    }
                }
            else
                Cache_DeleteItem(&g_cacheCVOL, atomPath, FALSE);    // Decrement count
            }
        }
    return pcvol;
    }


/*----------------------------------------------------------
Purpose: Search for the given volume ID in the cache.  Return
          the atomKey if it exists, otherwise ATOM_ERR.

Returns: atom
         ATOM_ERR if not found
Cond:    --
*/
int PUBLIC CVOL_FindID(
    HVOLUMEID hvid)
    {
    int atom;
    CVOL  * pcvol;

    atom = Cache_FindFirstKey(&g_cacheCVOL);
    while (atom != ATOM_ERR)
        {
        LPCTSTR pszPath = Atom_GetName(atom);

        ASSERT(pszPath);

        ENTEREXCLUSIVE()
            {
            pcvol = CVOL_Get(atom);
            ASSERT(pcvol);
            if (pcvol)
                {
                int nCmp;
    
                Sync_CompareVolumeIDs(pcvol->hvid, hvid, &nCmp);
                if (Sync_GetLastError() == TR_SUCCESS && nCmp == 0)
                    {
                    // We found it
                    CVOL_Delete(atom);
                    LEAVEEXCLUSIVE()
                    return atom;
                    }
    
                CVOL_Delete(atom);       // decrement count
                }
            }
        LEAVEEXCLUSIVE()

        atom = Cache_FindNextKey(&g_cacheCVOL, atom);
        }

    return ATOM_ERR;
    }



