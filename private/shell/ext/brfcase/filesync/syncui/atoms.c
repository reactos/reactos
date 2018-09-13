//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: atoms.c
//
//  This files contains the atom list code.
//
// History:
//  01-31-94 ScottH     Moved from cache.c
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers

/////////////////////////////////////////////////////  TYPEDEFS

typedef struct tagA_ITEM
    {
    int atom;           // index into hdsa
    LPTSTR psz;          // allocated
    UINT ucRef;
    } A_ITEM;       // item for atom table

typedef struct tagATOMTABLE
    {
    CRITICAL_SECTION cs;
    HDSA hdsa;          // Actual list of A_ITEMs
    HDPA hdpa;          // List into hdsa (sorted).  Values are indexes, not pointers
    HDPA hdpaFree;      // Free list.  Values are indexes, not pointers.
    } ATOMTABLE;

#define Atom_EnterCS(this)    EnterCriticalSection(&(this)->cs)
#define Atom_LeaveCS(this)    LeaveCriticalSection(&(this)->cs)

#define ATOM_GROW   32


#define Cache_Bogus(this)  (!(this)->hdpa || !(this)->hdpaFree || !(this)->hdsa)

// Given an index into the DPA, get the pointer to the DSA
//
#define MyGetPtr(this, idpa)     DSA_GetItemPtr((this)->hdsa, PtrToUlong(DPA_FastGetPtr((this)->hdpa, idpa)))

/////////////////////////////////////////////////////  MODULE DATA

static ATOMTABLE s_atomtable;

#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Validates the given atom is within the atomtable's range
Returns: --
Cond:    --
*/
void PUBLIC Atom_ValidateFn(
    int atom)
    {
    ATOMTABLE  * this = &s_atomtable;
    BOOL bError = FALSE;

    Atom_EnterCS(this);
        {
        if (atom >= DSA_GetItemCount(this->hdsa) ||
            atom < 0)
            {
            bError = TRUE;
            }
        }
    Atom_LeaveCS(this);

    if (bError)
        {
        // This is a problem!
        //
        DEBUG_MSG(TF_ERROR, TEXT("err BRIEFCASE: atom %d is out of range!"), atom);
        DEBUG_BREAK(BF_ONVALIDATE);
        }
    }


/*----------------------------------------------------------
Purpose: Dump the table contents
Returns: --
Cond:    For debugging purposes
*/
void PUBLIC Atom_DumpAll()
    {
    ATOMTABLE  * this = &s_atomtable;
    Atom_EnterCS(this);
        {
        if (IsFlagSet(g_uDumpFlags, DF_ATOMS))
            {
            A_ITEM  * pitem;
            int idpa;
            int cItem;

            ASSERT(this);
            ASSERT(this->hdsa != NULL);

            cItem = DPA_GetPtrCount(this->hdpa);
            for (idpa = 0; idpa < cItem; idpa++)
                {
                pitem = MyGetPtr(this, idpa);

                // The zero'th entry is reserved, so skip it
                if (pitem->atom == 0)
                    continue;

                TRACE_MSG(TF_ALWAYS, TEXT("ATOM:  Atom %d [%u]: %s"),
                    pitem->atom, pitem->ucRef, pitem->psz);
                }
            }
        }
    Atom_LeaveCS(this);
    }
#endif


/*----------------------------------------------------------
Purpose: Compare A_ITEMs
Returns: -1 if <, 0 if ==, 1 if >
Cond:    --
*/
int CALLBACK _export Atom_CompareIndexes(
    LPVOID lpv1,
    LPVOID lpv2,
    LPARAM lParam)
    {
    int i1 = PtrToUlong(lpv1);
    int i2 = PtrToUlong(lpv2);
    HDSA hdsa = (HDSA)lParam;
    A_ITEM  * pitem1 = DSA_GetItemPtr(hdsa, i1);
    A_ITEM  * pitem2 = DSA_GetItemPtr(hdsa, i2);

    ASSERT(pitem1);
    ASSERT(pitem2);

    return lstrcmpi(pitem1->psz, pitem2->psz);
    }


/*----------------------------------------------------------
Purpose: Compare A_ITEMs
Returns: -1 if <, 0 if ==, 1 if >
Cond:    --
*/
int CALLBACK _export Atom_Compare(
    LPVOID lpv1,
    LPVOID lpv2,
    LPARAM lParam)
    {
    // HACK: we know the first param is the address to a struct
    //  that contains the search criteria.  The second is an index
    //  into the DSA.
    //
    int i2 = PtrToUlong(lpv2);
    HDSA hdsa = (HDSA)lParam;
    A_ITEM  * pitem1 = (A_ITEM  *)lpv1;
    A_ITEM  * pitem2 = DSA_GetItemPtr(hdsa, i2);

    ASSERT(pitem1);
    ASSERT(pitem2);

    return lstrcmpi(pitem1->psz, pitem2->psz);
    }


/*----------------------------------------------------------
Purpose: Initialize the atom table
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Atom_Init()
    {
    BOOL bRet;
    ATOMTABLE  * this = &s_atomtable;

    ASSERT(this);
    ZeroInit(this, ATOMTABLE);

    InitializeCriticalSection(&this->cs);

    Atom_EnterCS(this);
        {
        if ((this->hdsa = DSA_Create(sizeof(A_ITEM), ATOM_GROW)) != NULL)
            {
            if ((this->hdpa = DPA_Create(ATOM_GROW)) == NULL)
                {
                DSA_Destroy(this->hdsa);
                this->hdsa = NULL;
                }
            else
                {
                if ((this->hdpaFree = DPA_Create(ATOM_GROW)) == NULL)
                    {
                    DPA_Destroy(this->hdpa);
                    DSA_Destroy(this->hdsa);
                    this->hdpa = NULL;
                    this->hdsa = NULL;
                    }
                else
                    {
                    // We've successfully initialized.  Keep the zero'th
                    //  atom reserved.  This way null atoms will not accidentally
                    //  munge data.
                    //
                    int atom = Atom_Add(TEXT("SHDD"));
                    ASSERT(atom == 0);
                    }
                }
            }
        bRet = this->hdsa != NULL;
        }
    Atom_LeaveCS(this);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Destroy the atom table
Returns: --
Cond:    --
*/
void PUBLIC Atom_Term()
    {
    ATOMTABLE  * this = &s_atomtable;

    Atom_EnterCS(this);
        {
        if (this->hdpa != NULL)
            {
            A_ITEM  * pitem;
            int idpa;
            int cItem;

            ASSERT(this->hdsa != NULL);

            cItem = DPA_GetPtrCount(this->hdpa);
            for (idpa = 0; idpa < cItem; idpa++)
                {
                pitem = MyGetPtr(this, idpa);

                // The zero'th entry is reserved, so skip it
                if (pitem->atom == 0)
                    continue;

                Str_SetPtr(&pitem->psz, NULL);
                }
            DPA_Destroy(this->hdpa);
            this->hdpa = NULL;
            }

        if (this->hdpaFree != NULL)
            {
            DPA_Destroy(this->hdpaFree);
            this->hdpaFree = NULL;
            }

        if (this->hdsa != NULL)
            {
            DSA_Destroy(this->hdsa);
            this->hdsa = NULL;
            }
        }
    Atom_LeaveCS(this);

    DeleteCriticalSection(&this->cs);
    }


/*----------------------------------------------------------
Purpose: Add a string to the atom table.  If the string already
          exists, return its atom.
Returns: Atom
         ATOM_ERR on failure

Cond:    Reference count is incremented always
*/
int PUBLIC Atom_Add(
    LPCTSTR psz)
    {
    ATOMTABLE  * this = &s_atomtable;
    A_ITEM  * pitem = NULL;
    A_ITEM item;
    int atomRet = ATOM_ERR;
    int idpa;
    int cItem;
    int cFree;

    ASSERT(psz);

    Atom_EnterCS(this);
        {
        int iItem;

        DEBUG_CODE( iItem = -1; )

        // Search for the string in the atom table first.
        //  If we find it, return the atom.
        //
        item.psz = (LPTSTR)(LPVOID)psz;
        idpa = DPA_Search(this->hdpa, &item, 0, Atom_Compare, (LPARAM)this->hdsa, DPAS_SORTED);
        if (idpa != -1)
            {
            // String is already in table
            //
            pitem = MyGetPtr(this, idpa);
            pitem->ucRef++;
            atomRet = pitem->atom;

            ASSERT(IsSzEqual(psz, pitem->psz));

            VALIDATE_ATOM(pitem->atom);
            }
        else
            {
            // Add the string to the table.  Take any available entry
            //  from the free list first.  Otherwise allocate more space
            //  in the table.  Then add a ptr to the sorted ptr list.
            //
            cFree = DPA_GetPtrCount(this->hdpaFree);
            if (cFree > 0)
                {
                // Use a free entry
                //
                cFree--;
                iItem = PtrToUlong(DPA_DeletePtr(this->hdpaFree, cFree));
                pitem = DSA_GetItemPtr(this->hdsa, iItem);

                // atom field for pitem should already be set

                VALIDATE_ATOM(pitem->atom);
                }
            else
                {
                // Allocate a new entry.  item has bogus data in it.
                //  That's okay, we fill in good stuff below.
                //
                cItem = DSA_GetItemCount(this->hdsa);
                if ((iItem = DSA_InsertItem(this->hdsa, cItem+1, &item)) != -1)
                    {
                    pitem = DSA_GetItemPtr(this->hdsa, iItem);
                    pitem->atom = iItem;

                    VALIDATE_ATOM(pitem->atom);
                    }
                }

            // Fill in the info
            //
            if (pitem)
                {
                pitem->ucRef = 1;
                pitem->psz = 0;
                if (!Str_SetPtr(&pitem->psz, psz))
                    goto Add_Fail;

                // Add the new entry to the ptr list and sort
                //
                cItem = DPA_GetPtrCount(this->hdpa);
                if (DPA_InsertPtr(this->hdpa, cItem+1, (LPVOID)iItem) == -1)
                    goto Add_Fail;
                DPA_Sort(this->hdpa, Atom_CompareIndexes, (LPARAM)this->hdsa);
                atomRet = pitem->atom;

                TRACE_MSG(TF_ATOM, TEXT("ATOM  Adding %d [%u]: %s"), atomRet, pitem->ucRef, pitem->psz);
                }
            }

Add_Fail:
        // Add the entry to the free list and fail.  If even this fails,
        //  then we simply lose some slight efficiency, but this is not
        //  a memory leak.
        //
#ifdef DEBUG
        if (atomRet == ATOM_ERR)
            TRACE_MSG(TF_ATOM, TEXT("ATOM  **Failed adding %s"), psz);
#endif
        if (atomRet == ATOM_ERR && pitem)
            {
            ASSERT(iItem != -1);

            DPA_InsertPtr(this->hdpaFree, cFree+1, (LPVOID)iItem);
            }
        }
    Atom_LeaveCS(this);

    return atomRet;
    }


/*----------------------------------------------------------
Purpose: Increment the reference count of this atom.

Returns: Last count
         0 if the atom doesn't exist
Cond:    --
*/
UINT PUBLIC Atom_AddRef(
    int atom)
    {
    ATOMTABLE  * this = &s_atomtable;
    UINT cRef;

    if (!Atom_IsValid(atom))
        {
        ASSERT(0);
        return 0;
        }

    VALIDATE_ATOM(atom);

    Atom_EnterCS(this);
        {
        A_ITEM * pitem = DSA_GetItemPtr(this->hdsa, atom);
        if (pitem)
            {
            cRef = pitem->ucRef++;
            }
        else
            {
            cRef = 0;
            }
        }
    Atom_LeaveCS(this);

    return cRef;
    }


/*----------------------------------------------------------
Purpose: Delete a string from the atom table.

         If the reference count is not zero, we do nothing.
Returns: --

Cond:    N.b.  Decrements the reference count.
*/
void PUBLIC Atom_Delete(
    int atom)
    {
    ATOMTABLE  * this = &s_atomtable;
    A_ITEM  * pitem;

    if (!Atom_IsValid(atom))
        {
        ASSERT(0);
        return;
        }

    VALIDATE_ATOM(atom);

    Atom_EnterCS(this);
        {
        pitem = DSA_GetItemPtr(this->hdsa, atom);
        if (pitem)
            {
            int idpa;
            int cFree;

            ASSERT(pitem->atom == atom);

            // Is the reference count already at zero?
            if (0 == pitem->ucRef)
                {
                // Yes; somebody is calling Atom_Delete one-too-many times!
                DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Deleting %d once-too-many!!"),
                    pitem->atom); )
                ASSERT(0);
                }
            else if (0 == --pitem->ucRef)
                {
                // Yes
                idpa = DPA_GetPtrIndex(this->hdpa, (LPVOID)atom);     // linear search

                DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Deleting %d: %s"),
                    pitem->atom, pitem->psz ? pitem->psz : (LPCTSTR)TEXT("NULL")); )

                ASSERT(atom == (int)DPA_GetPtr(this->hdpa, idpa));
                if (DPA_ERR != idpa)
                    {
                    DPA_DeletePtr(this->hdpa, idpa);

                    ASSERT(pitem->psz);
                    Str_SetPtr(&pitem->psz, NULL);

                    DEBUG_CODE( pitem->psz = NULL; )
                    }
                else
                    {
                    ASSERT(0);      // Should never get here
                    }

                // Add ptr to the free list.  If this fails, we simply
                //  lose some efficiency in reusing this portion of the cache.
                //  This is not a memory leak.
                //
                cFree = DPA_GetPtrCount(this->hdpaFree);
                DPA_InsertPtr(this->hdpaFree, cFree+1, (LPVOID)atom);
                }
            }
        }
    Atom_LeaveCS(this);
    }


/*----------------------------------------------------------
Purpose: Replace the string corresponding with the atom with
         another string.  The atom will not change.
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Atom_Replace(
    int atom,
    LPCTSTR pszNew)
    {
    ATOMTABLE  * this = &s_atomtable;
    BOOL bRet = FALSE;
    A_ITEM  * pitem;

    ASSERT(pszNew);

    if (!Atom_IsValid(atom))
        {
        return FALSE;
        }

    VALIDATE_ATOM(atom);

    Atom_EnterCS(this);
        {
        pitem = DSA_GetItemPtr(this->hdsa, atom);
        if (pitem)
            {
            ASSERT(atom == pitem->atom);
            ASSERT(pitem->psz);
            DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Change %d [%u]: %s -> %s"),
                atom, pitem->ucRef, pitem->psz, pszNew); )

            if (Str_SetPtr(&pitem->psz, pszNew))
                {
                DPA_Sort(this->hdpa, Atom_CompareIndexes, (LPARAM)this->hdsa);
                bRet = TRUE;
                }
#ifdef DEBUG
            else
                TRACE_MSG(TF_ATOM, TEXT("ATOM  **Change failed"));
#endif
            }
        }
    Atom_LeaveCS(this);
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Translate all atoms with that contain the partial
          string atomOld with the partial string atomNew.
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Atom_Translate(
    int atomOld,
    int atomNew)
    {
    BOOL bRet = FALSE;
    ATOMTABLE  * this = &s_atomtable;
    A_ITEM  * pitem;
    int idpa;
    int cItem;
    int atomSave = 0;
    int cchOld;
    LPCTSTR psz;
    LPCTSTR pszOld;
    LPCTSTR pszNew;
    LPCTSTR pszRest;
    TCHAR sz[MAXPATHLEN];

    if ( !(Atom_IsValid(atomOld) && Atom_IsValid(atomNew)) )
        {
        return FALSE;
        }

    Atom_EnterCS(this);
        {
        pszOld = Atom_GetName(atomOld);
        cchOld = lstrlen(pszOld);
        pszNew = Atom_GetName(atomNew);

        cItem = DPA_GetPtrCount(this->hdpa);
        for (idpa = 0; idpa < cItem; idpa++)
            {
            pitem = MyGetPtr(this, idpa);
            ASSERT(pitem);

            if (pitem->atom == 0)
                continue;                   // skip reserved atom

            if (atomOld == pitem->atom)
                {
                atomSave = pitem->atom;     // Save this one for last
                continue;
                }

            psz = Atom_GetName(pitem->atom);
            ASSERT(psz);

            if (PathIsPrefix(psz, pszOld) && lstrlen(psz) >= cchOld)
                {
                // Translate this atom
                //
                pszRest = psz + cchOld;     // whack up the path

                PathCombine(sz, pszNew, pszRest);

                DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Translate %d [%u]: %s -> %s"),
                    pitem->atom, pitem->ucRef, pitem->psz, (LPCTSTR)sz); )

                if (!Str_SetPtr(&pitem->psz, sz))
                    goto Translate_Fail;
                }
            }

        ASSERT(Atom_IsValid(atomSave));      // this means trouble

        VALIDATE_ATOM(atomSave);

        pitem = DSA_GetItemPtr(this->hdsa, atomSave);
        if (pitem)
            {
            ASSERT(atomSave == pitem->atom);
            ASSERT(pitem->psz);

            DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Translate %d [%u]: %s -> %s"),
                pitem->atom, pitem->ucRef, pitem->psz, pszNew); )

            if (!Str_SetPtr(&pitem->psz, pszNew))
                goto Translate_Fail;
            }
        bRet = TRUE;

Translate_Fail:
        ASSERT(bRet);

        // Sort here, even on a fail, so we correctly sort whatever
        //  got translated before the failure.
        //
        DPA_Sort(this->hdpa, Atom_CompareIndexes, (LPARAM)this->hdsa);
        }
    Atom_LeaveCS(this);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Search for a string in the atom table and return the atom
Returns: Atom
         ATOM_ERR if the string is not in the table

Cond:    Reference count is NOT incremented
*/
int PUBLIC Atom_Find(
    LPCTSTR psz)
    {
    ATOMTABLE  * this = &s_atomtable;
    A_ITEM item;
    A_ITEM  * pitem;
    int atomRet = ATOM_ERR;
    int idpa;

    ASSERT(psz);

    Atom_EnterCS(this);
        {
        item.psz = (LPTSTR)(LPVOID)psz;
        idpa = DPA_Search(this->hdpa, &item, 0, Atom_Compare, (LPARAM)this->hdsa,
                          DPAS_SORTED);
        if (idpa != -1)
            {
            pitem = MyGetPtr(this, idpa);
            atomRet = pitem->atom;

            DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Find %s.  Found %d [%u]: %s"),
                psz, pitem->atom, pitem->ucRef, pitem->psz); )
            ASSERT(IsSzEqual(psz, pitem->psz));
            }
#ifdef DEBUG
        else
            TRACE_MSG(TF_ATOM, TEXT("ATOM  **Not found %s"), psz);
#endif
        }
    Atom_LeaveCS(this);

    return atomRet;
    }


/*----------------------------------------------------------
Purpose: Get the string for this atom
Returns: Ptr to the string
         NULL if the atom is bogus

Cond:    The caller must serialize this.
*/
LPCTSTR PUBLIC Atom_GetName(
    int atom)
    {
    ATOMTABLE  * this = &s_atomtable;
    LPCTSTR pszRet = NULL;
    A_ITEM  * pitem;

    VALIDATE_ATOM(atom);

    Atom_EnterCS(this);
        {
        pitem = DSA_GetItemPtr(this->hdsa, atom);
        if (pitem)
            {
            pszRet = pitem->psz;

            DEBUG_CODE( TRACE_MSG(TF_ATOM, TEXT("ATOM  Getting name %d [%u]: %s"),
                atom, pitem->ucRef, pszRet); )
            ASSERT(atom == pitem->atom);
            }
#ifdef DEBUG
        else
            TRACE_MSG(TF_ATOM, TEXT("ATOM  **Cannot get %d"), atom);
#endif
        }
    Atom_LeaveCS(this);

    return pszRet;
    }


/*----------------------------------------------------------
Purpose: Return TRUE if atom2 is a partial path match of atom1.

Returns: boolean

Cond:    Requires atom1 and atom2 to be valid.
*/
BOOL PUBLIC Atom_IsPartialMatch(
    int atom1,
    int atom2)
    {
    LPCTSTR psz1 = Atom_GetName(atom1);
    LPCTSTR psz2 = Atom_GetName(atom2);

    ASSERT(psz1);
    ASSERT(psz2);

    return PathIsPrefix(psz2, psz1);
    }
