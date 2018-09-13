//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cpath.c
//
//  This files contains code for the cached briefcase paths.
//
// History:
//  01-31-94 ScottH     Created
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers

CACHE g_cacheCPATH = {0, 0, 0};       // Briefcase path cache

#define CPATH_EnterCS()    EnterCriticalSection(&g_cacheCPATH.cs)
#define CPATH_LeaveCS()    LeaveCriticalSection(&g_cacheCPATH.cs)



#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dumps a CPATH entry
Returns: 
Cond:    --
*/
void PRIVATE CPATH_DumpEntry(
    CPATH  * pcpath)
    {
    ASSERT(pcpath);

    TRACE_MSG(TF_ALWAYS, TEXT("CPATH:  Atom %d: %s"), pcpath->atomPath, Atom_GetName(pcpath->atomPath));
    TRACE_MSG(TF_ALWAYS, TEXT("               Ref [%u]  "), 
        Cache_GetRefCount(&g_cacheCPATH, pcpath->atomPath));
    }


/*----------------------------------------------------------
Purpose: Dumps all CPATH cache
Returns: 
Cond:    --
*/
void PUBLIC CPATH_DumpAll()
    {
    CPATH  * pcpath;
    int atom;
    BOOL bDump;

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CPATH);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    atom = Cache_FindFirstKey(&g_cacheCPATH);
    while (atom != ATOM_ERR)
        {
        pcpath = Cache_GetPtr(&g_cacheCPATH, atom);
        ASSERT(pcpath);
        if (pcpath)
            {
            CPATH_DumpEntry(pcpath);
            Cache_DeleteItem(&g_cacheCPATH, atom, FALSE, NULL, CPATH_Free);    // Decrement count
            }

        atom = Cache_FindNextKey(&g_cacheCPATH, atom);
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Release the volume ID handle
Returns: --

Cond:    hwndOwner is not used.

         This function is serialized by the caller (Cache_Term or
         Cache_DeleteItem).
*/
void CALLBACK CPATH_Free(
    LPVOID lpv,
    HWND hwndOwner)
    {
    CPATH  * pcpath = (CPATH  *)lpv;

    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CPATH   Freeing Briefcase path %s"), Atom_GetName(pcpath->atomPath)); )

    // Delete the atom one extra time, because we explicitly added
    // it for this cache.
    Atom_Delete(pcpath->atomPath);

    SharedFree(&pcpath);
    }


/*----------------------------------------------------------
Purpose: Add the atomPath to the cache.  
          If atomPath is already in the cache, we replace it
          with a newly obtained path.

Returns: Pointer to CPATH
         NULL on OOM

Cond:    --
*/
CPATH  * PUBLIC CPATH_Replace(
    int atomPath)
    {
    CPATH  * pcpath;
    BOOL bJustAllocd;
    
    CPATH_EnterCS();
        {
        pcpath = Cache_GetPtr(&g_cacheCPATH, atomPath);
        if (pcpath)
            bJustAllocd = FALSE;
        else
            {
            // Allocate using commctrl's Alloc, so the structure will be in
            // shared heap space across processes.
            pcpath = SharedAllocType(CPATH);
            bJustAllocd = TRUE;
            }

        if (pcpath)
            {
            LPCTSTR pszPath = Atom_GetName(atomPath);

            ASSERT(pszPath);

            DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CPATH  Adding known Briefcase %s"), pszPath); )

            pcpath->atomPath = atomPath;

            if (bJustAllocd)
                {
                if (!Cache_AddItem(&g_cacheCPATH, atomPath, (LPVOID)pcpath))
                    {
                    // Cache_AddItem failed here
                    //
                    SharedFree(&pcpath);
                    }
                }
            else
                Cache_DeleteItem(&g_cacheCPATH, atomPath, FALSE, NULL, CPATH_Free);    // Decrement count
            }
        }
    CPATH_LeaveCS();

    return pcpath;
    }


/*----------------------------------------------------------
Purpose: Search for the given path in the cache.  If the path
         exists, its locality will be returned.

         If it is not found, its locality is not known (but
         PL_FALSE is returned).

Returns: path locality (PL_) value
Cond:    --
*/
UINT PUBLIC CPATH_GetLocality(
    LPCTSTR pszPath,
    LPTSTR pszBuf)           // Can be NULL, or must be MAXPATHLEN
    {
    UINT uRet = PL_FALSE;
    LPCTSTR pszBrf;
    int atom;

    ASSERT(pszPath);

    CPATH_EnterCS();
        {
        atom = Cache_FindFirstKey(&g_cacheCPATH);
        while (atom != ATOM_ERR)
            {
            pszBrf = Atom_GetName(atom);

            ASSERT(pszBrf);

            if (IsSzEqual(pszBrf, pszPath))
                {
                uRet = PL_ROOT;
                break;
                }
            else if (PathIsPrefix(pszBrf, pszPath))
                {
                uRet = PL_INSIDE;
                break;
                }

            atom = Cache_FindNextKey(&g_cacheCPATH, atom);
            }

        if (uRet != PL_FALSE && pszBuf)
            lstrcpy(pszBuf, pszBrf);
        }
    CPATH_LeaveCS();
        
    return uRet;
    }



