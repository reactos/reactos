/*
 * propstg.c - Property storage ADT 
 */


#include "priv.h"
#include "propstg.h"

#ifndef UNIX
// SafeGetItemObject
//
// Since the GetItemObject member of IShellView was added late in the game
// during Win95 development we have found at least one example (rnaui.dll)
// of an application that built an IShellView with a NULL member for
// GetItemObject.  Fearing more applications that may have the same
// problem, this wrapper function was added to catch bad apps like rnaui.
// Thus, we check here for NULL before calling the member.
//
STDAPI SafeGetItemObject(LPSHELLVIEW psv, UINT uItem, REFIID riid, LPVOID *ppv)
{
#ifdef __cplusplus
#error THIS_MUST_STAY_C
    // read the comment above
#endif
    if (!psv->lpVtbl->GetItemObject)
        return E_FAIL;

    return (HRESULT)(psv->lpVtbl->GetItemObject(psv, uItem, riid, ppv));
}
#endif


// This structure is a dictionary element. It maps a name to a propid.  

typedef struct tagDICTEL
    {
    PROPID  propid;
    WCHAR   wszName[MAX_PATH];
    } DICTEL, * PDICTEL;


// This structure is a propvariant element.
typedef struct tagPROPEL
    {
    PROPVARIANT propvar;
    DWORD       dwFlags;        // PEF_*
    } PROPEL, * PPROPEL;

// Flags for PROPEL structure
#define PEF_VALID           0x00000001
#define PEF_DIRTY           0x00000002

// This structure is the ADT for property storage

typedef struct tagPROPSTG
    {
    DWORD   cbSize;
    DWORD   dwFlags;
    HDSA    hdsaProps;          // array of properties (indexed by propid) 
    HDPA    hdpaDict;           // dictionary of names mapped to propid 
                                //  (each element is a DICTEL) 
    int     idsaLastValid;
    } PROPSTG, * PPROPSTG;


// The first two entries in hdsaProps are reserved.  When we enumerate
// thru the list, we skip these entries.
#define PROPID_DICT     0
#define PROPID_CODEPAGE 1

#define IDSA_START      2
#define CDSA_RESERVED   2


/*----------------------------------------------------------
Purpose: Validates a PROPSTG structure

Returns: TRUE if valid
Cond:    --
*/
BOOL
IsValidHPROPSTG(
    HPROPSTG hstg)
    {
    PPROPSTG pstg = (PPROPSTG)hstg;

    return (IS_VALID_WRITE_PTR(pstg, PROPSTG) &&
            SIZEOF(*pstg) == pstg->cbSize && 
            NULL != pstg->hdsaProps &&
            NULL != pstg->hdpaDict);
    }


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Validates a PROPSPEC structure 

Returns: TRUE if valid 
Cond:    --
*/
BOOL
IsValidPPROPSPEC(
    PROPSPEC * ppropspec)
    {
    return (ppropspec &&
            PRSPEC_PROPID == ppropspec->ulKind ||
            (PRSPEC_LPWSTR == ppropspec->ulKind && 
             IS_VALID_STRING_PTRW(ppropspec->DUMMYUNION_MEMBER(lpwstr), -1)));
    }
#endif



/*----------------------------------------------------------
Purpose: Create a property storage ADT 

Returns: S_OK
         STG_E_INVALIDPARAMETER
         STG_E_INSUFFICIENTMEMORY

Cond:    --
*/
HRESULT
WINAPI
PropStg_Create(
    OUT HPROPSTG * phstg,
    IN  DWORD      dwFlags)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_WRITE_PTR(phstg, HPROPSTG)))
        {
        PPROPSTG pstg = (PPROPSTG)LocalAlloc(LPTR, SIZEOF(*pstg));

        hres = STG_E_INSUFFICIENTMEMORY;       // assume error 

        if (pstg) 
            {
            pstg->cbSize = SIZEOF(*pstg);
            pstg->dwFlags = dwFlags;
            pstg->idsaLastValid = PROPID_CODEPAGE;

            pstg->hdsaProps = DSA_Create(SIZEOF(PROPEL), 8);
            pstg->hdpaDict = DPA_Create(8);

            if (pstg->hdsaProps && pstg->hdpaDict)
                {
                // The first two propids are reserved, so insert
                // placeholders.
                PROPEL propel;

                propel.propvar.vt = VT_EMPTY;
                propel.dwFlags = 0;

                DSA_SetItem(pstg->hdsaProps, PROPID_DICT, &propel);
                DSA_SetItem(pstg->hdsaProps, PROPID_CODEPAGE, &propel);

                hres = S_OK;
                }
            else
                {
                // Clean up because something failed 
                if (pstg->hdsaProps)
                    DSA_Destroy(pstg->hdsaProps);

                if (pstg->hdpaDict)
                    DPA_Destroy(pstg->hdpaDict);

                LocalFree(pstg);
                pstg = NULL;
                }
            }

        *phstg = (HPROPSTG)pstg;

        // Validate return values
        ASSERT((SUCCEEDED(hres) && 
                IS_VALID_WRITE_PTR(*phstg, PPROPSTG)) ||
               (FAILED(hres) && NULL == *phstg));
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Destroys a given property storage ADT 

Returns: S_OK 
         STG_E_INVALIDPARAMETER 
Cond:    --
*/
HRESULT
WINAPI
PropStg_Destroy(
    IN HPROPSTG hstg)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
    
        if (pstg->hdsaProps)
            {
            int cdsa = DSA_GetItemCount(pstg->hdsaProps) - CDSA_RESERVED;

            // The first two elements are not cleared, because they
            // are just place-holders.

            if (0 < cdsa)
                {
                PPROPEL ppropel = DSA_GetItemPtr(pstg->hdsaProps, IDSA_START);

                ASSERT(ppropel);

                while (0 < cdsa--)
                    {
                    PropVariantClear(&ppropel->propvar);
                    ppropel++;
                    }
                }
            
            DSA_Destroy(pstg->hdsaProps);
            }

        if (pstg->hdpaDict)
            {
            int i;
            int cel = DPA_GetPtrCount(pstg->hdpaDict);

            for (i = 0; i < cel; i++)
                {
                LocalFree(DPA_FastGetPtr(pstg->hdpaDict, i));
                }
            DPA_Destroy(pstg->hdpaDict);
            }
            
        LocalFree(pstg);

        hres = S_OK;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Compare names

Returns: standard -1, 0, 1
Cond:    --
*/
int
CALLBACK
PropStg_Compare(
    IN LPVOID pv1,
    IN LPVOID pv2,
    IN LPARAM lParam)
    {
    LPCWSTR psz1 = pv1;
    LPCWSTR psz2 = pv2;

    // Case insensitive 
    return StrCmpW(psz1, psz2);
    }


/*----------------------------------------------------------
Purpose: Returns TRUE if the property exists in this storage.
         If it does exist, the propid is returned.

Returns: TRUE 
         FALSE 

Cond:    --
*/
BOOL
PropStg_PropertyExists(
    IN  PPROPSTG   pstg,
    IN  const PROPSPEC * ppropspec,
    OUT PROPID *   ppropid)
    {
    BOOL bRet;
    PPROPEL ppropel;
    HDSA hdsaProps;

    ASSERT(pstg);
    ASSERT(ppropspec);
    ASSERT(ppropid);

    hdsaProps = pstg->hdsaProps;

    switch (ppropspec->ulKind)
        {
    case PRSPEC_PROPID:
        *ppropid = ppropspec->DUMMYUNION_MEMBER(propid);

        bRet = (*ppropid < (PROPID)DSA_GetItemCount(hdsaProps));
        if (bRet)
            {
            ppropel = DSA_GetItemPtr(hdsaProps, *ppropid);
            bRet = (ppropel && IsFlagSet(ppropel->dwFlags, PEF_VALID));
            }
        break;

    case PRSPEC_LPWSTR:
        // Key off whether the name exists 
        *ppropid = DPA_Search(pstg->hdpaDict, ppropspec->DUMMYUNION_MEMBER(lpwstr), 0, PropStg_Compare, 0, DPAS_SORTED);

#ifdef DEBUG
        // Sanity check that the property actually exists
        ppropel = DSA_GetItemPtr(hdsaProps, *ppropid);
        ASSERT(-1 == *ppropid ||
               (ppropel && IsFlagSet(ppropel->dwFlags, PEF_VALID)));
#endif

        bRet = (-1 != *ppropid);
        break;

    default:
        bRet = FALSE;
        break;
        }

    // Propid values 0 and 1 are reserved, as are values >= 0x80000000 
    if (0 == *ppropid || 1 == *ppropid || 0x80000000 <= *ppropid)
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Create a new propid and assign the given name to
         it.  

         The propid is an index into hdsaProps.

Returns: S_OK
         STG_E_INSUFFICIENTMEMORY

Cond:    --
*/
HRESULT
PropStg_NewPropid(
    IN  PPROPSTG pstg,
    IN  LPCWSTR  pwsz,
    IN  PROPID   propidFirst,
    OUT PROPID * ppropid)           OPTIONAL
    {
    HRESULT hres = STG_E_INVALIDPOINTER;        // assume error
    DICTEL * pdictel;
    PROPID propid = (PROPID)-1;
    HDPA hdpa;

    ASSERT(pstg);
    ASSERT(ppropid);

    if (EVAL(IS_VALID_STRING_PTRW(pwsz, -1)))
        {
        hres = STG_E_INSUFFICIENTMEMORY;            // assume error

        hdpa = pstg->hdpaDict;

        // The name shouldn't be in the list yet 
        ASSERT(-1 == DPA_Search(hdpa, (LPVOID)pwsz, 0, PropStg_Compare, 0, DPAS_SORTED));

        pdictel = LocalAlloc(LPTR, SIZEOF(*pdictel));
        if (pdictel)
            {
            // Determine the propid for this 
            PROPID propidNew = max(propidFirst, (PROPID)pstg->idsaLastValid + 1);

            pdictel->propid = propidNew;

            StrCpyNW(pdictel->wszName, pwsz, ARRAYSIZE(pdictel->wszName));
            
            if (-1 != DPA_AppendPtr(hdpa, pdictel))
                {
                // Sort it by name
                DPA_Sort(hdpa, PropStg_Compare, 0);
                hres = S_OK;
                propid = propidNew;
                }
            }
        }

    *ppropid = propid;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Read a set of properties given their propids.  If the propid 
         doesn't exist in this property storage, then set the
         value type to VT_EMPTY but return success; unless
         all the properties in rgpropspec don't exist, in which
         case also return S_FALSE.  

Returns: S_OK
         S_FALSE
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_ReadMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec,
    IN PROPVARIANT * rgpropvar)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropspec, PROPSPEC, cpspec)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropvar, PROPVARIANT, cpspec)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        ULONG cpspecSav = cpspec;
        const PROPSPEC * ppropspec = rgpropspec;
        PROPVARIANT * ppropvar = rgpropvar;
        int idsa;
        BOOL bPropertyExists;
        ULONG cpspecIllegal = 0;

        hres = S_OK;        // assume success

        if (0 < cpspec)
            {
            // Read the list of property specs 
            while (0 < cpspec--)
                {
                bPropertyExists = PropStg_PropertyExists(pstg, ppropspec, (LPDWORD)&idsa);

                // Does this property exist? 
                if ( !bPropertyExists )
                    {
                    // No
                    ppropvar->vt = VT_ILLEGAL;
                    cpspecIllegal++;
                    }
                else
                    {
                    // Yes; is the element a valid property? 
                    PPROPEL ppropel = DSA_GetItemPtr(pstg->hdsaProps, idsa);

                    ASSERT(ppropel);

                    if (IsFlagSet(ppropel->dwFlags, PEF_VALID))
                        {
                        // Yes; copy the variant value
                        hres = PropVariantCopy(ppropvar, &ppropel->propvar);
                        }
                    else
                        {
                        // No
                        ppropvar->vt = VT_ILLEGAL;
                        cpspecIllegal++;
                        }
                    }

                ppropspec++;
                ppropvar++;

                //  Bail out of loop if something failed 
                if (FAILED(hres))
                    break;
                }

            // Are all the property specs illegal? 
            if (cpspecIllegal == cpspecSav)
                {
                hres = S_FALSE;     // yes
                }

            // Did anything fail above?
            if (FAILED(hres))
                {
                // Yes; clean up -- no properties will be retrieved 
                FreePropVariantArray(cpspecSav, rgpropvar);
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Add a set of property values given their propids.  
         If the propid doesn't exist in this property storage, 
         then add the propid as a legal ID and set the value. 

         On error, some properties may or may not have been 
         written.

         If pfn is non-NULL, this callback will get called
         to optionally "massage" the propvariant value or to
         validate it.  The rules for the callback are:

            1)  It can change the value directly if it is not
                allocated

            2)  If the value is allocated, the callback must 
                replace the pointer with a newly allocated 
                buffer that it allocates.  It must not try
                to free the value coming in, since it doesn't
                know how it was allocated.  It must also use
                CoTaskMemAlloc to allocate its buffer. 

            3)  If the callback returns an error, this function
                will stop writing properties and return that
                error.

            4)  If the callback returns S_FALSE, this function
                will not write that particular property and 
                continue on to the next property.  The function
                then returns S_FALSE once it is finished.

Returns: S_OK
         S_FALSE
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_WriteMultipleEx(
    IN HPROPSTG          hstg,
    IN ULONG             cpspec,
    IN const PROPSPEC *  rgpropspec,
    IN const PROPVARIANT * rgpropvar,
    IN PROPID            propidFirst,   OPTIONAL
    IN PFNPROPVARMASSAGE pfn,           OPTIONAL
    IN LPARAM            lParam)        OPTIONAL
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropspec, PROPSPEC, cpspec)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropvar, PROPVARIANT, cpspec)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        const PROPSPEC * ppropspec = rgpropspec;
        const PROPVARIANT * ppropvar = rgpropvar;
        int idsa;
        PROPEL propel;
        BOOL bPropertyExists;
        BOOL bSkippedProperty = FALSE;

        if (0 == cpspec)
            {
            hres = S_OK;
            }
        else
            {
            // Write the list of property specs 
            while (0 < cpspec--)
                {
                bPropertyExists = PropStg_PropertyExists(pstg, ppropspec, (LPDWORD)&idsa);

                hres = S_OK;

                // If this is an illegal variant type and yet a valid 
                // property, then return an error.  Otherwise, ignore it
                // and move on.
                if (VT_ILLEGAL == ppropvar->vt)
                    {
                    if (bPropertyExists)
                        hres = STG_E_INVALIDPARAMETER;
                    else
                        goto NextDude;
                    }

                if (SUCCEEDED(hres))
                    {
                    // Add the property.  If it doesn't exist, add it.

                    // Is this a propid or a name? 
                    switch (ppropspec->ulKind)
                        {
                    case PRSPEC_PROPID:
                        idsa = ppropspec->DUMMYUNION_MEMBER(propid);
                        break;

                    case PRSPEC_LPWSTR:
                        if ( !bPropertyExists )
                            {
                            hres = PropStg_NewPropid(pstg, ppropspec->DUMMYUNION_MEMBER(lpwstr), 
                                                     propidFirst, (LPDWORD)&idsa);
                            }
                        break;

                    default:
                        hres = STG_E_INVALIDNAME;
                        break;
                        }

                    if (SUCCEEDED(hres))
                        {
                        PROPVARIANT propvarT;

                        ASSERT(S_OK == hres);   // we're assuming this on entry 

                        // Save a copy of the original in case the
                        // callback changes it.
                        CopyMemory(&propvarT, ppropvar, SIZEOF(propvarT));

                        // How did the callback like it? 
                        if (pfn)
                            hres = pfn(idsa, ppropvar, lParam);

                        if (S_OK == hres)
                            {
                            // Fine; make a copy of the (possibly changed)
                            // propvariant value
                            hres = PropVariantCopy(&propel.propvar, ppropvar);
                            if (SUCCEEDED(hres))
                                {
                                propel.dwFlags = PEF_VALID | PEF_DIRTY;

                                hres = (DSA_SetItem(pstg->hdsaProps, idsa, &propel) ? 
                                            S_OK : STG_E_INSUFFICIENTMEMORY);

                                if (SUCCEEDED(hres) && idsa > pstg->idsaLastValid)
                                    {
                                    pstg->idsaLastValid = idsa;
                                    }
                                }
                            }
                        else if (S_FALSE == hres)
                            {
                            bSkippedProperty = TRUE;
                            }

                        // Restore the propvariant value to its original
                        // value. But first, did the callback allocate a
                        // new buffer? 
                        if (propvarT.DUMMYUNION_MEMBER(pszVal) != ppropvar->DUMMYUNION_MEMBER(pszVal))
                            {
                            // Yes; clear it (this function is safe for
                            // non-allocated values too).
                            PropVariantClear((PROPVARIANT *)ppropvar);
                            }

                        // Restore
                        CopyMemory((PROPVARIANT *)ppropvar, &propvarT, SIZEOF(*ppropvar));
                        hres = S_OK;
                        }
                    }

NextDude:
                ppropspec++;
                ppropvar++;

                //  Bail out of loop if something failed 
                if (FAILED(hres))
                    break;
                }

            if (bSkippedProperty)
                hres = S_FALSE;
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Add a set of property values given their propids.  
         If the propid doesn't exist in this property storage, 
         then add the propid as a legal ID and set the value. 

         On error, some properties may or may not have been 
         written.

Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_WriteMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec,
    IN const PROPVARIANT * rgpropvar,
    IN PROPID        propidFirst)      OPTIONAL
    {
    return PropStg_WriteMultipleEx(hstg, cpspec, rgpropspec, rgpropvar,
                                   propidFirst, NULL, 0);
    }


/*----------------------------------------------------------
Purpose: Delete a set of properties.

Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_DeleteMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropspec, PROPSPEC, cpspec)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        const PROPSPEC * ppropspec = rgpropspec;
        HDSA hdsaProps = pstg->hdsaProps;
        PPROPEL ppropel;
        int idsa;
        int cdsa;

        hres = S_OK;

        if (0 < cpspec)
            {
            BOOL bDeletedLastValid = FALSE;

            // Delete the list of property specs
            while (0 < cpspec--)
                {
                if (PropStg_PropertyExists(pstg, ppropspec, (LPDWORD)&idsa))
                    {
                    // Delete the property.  Zero out the existing
                    // propel.  Don't call DSA_DeleteItem, otherwise
                    // we'll move the positions of any remaining
                    // properties following this one, thus changing their
                    // propids.
                    ppropel = DSA_GetItemPtr(hdsaProps, idsa);
                    ASSERT(ppropel);
                    
                    PropVariantClear(&ppropel->propvar);
                    ppropel->dwFlags = 0;

                    // Our idsaLastValid is messed up if we hit this
                    // assert
                    ASSERT(idsa <= pstg->idsaLastValid);

                    if (idsa == pstg->idsaLastValid)
                        bDeletedLastValid = TRUE;

                    // Delete the names associated with the property 
                    // BUGBUG (scotth): implement this
                    }

                ppropspec++;
                }

            // Did we delete the property that was marked as the terminating
            // valid property in the list? 
            if (bDeletedLastValid)
                {
                // Yes; go back and search for the new terminating index 
                ppropel = DSA_GetItemPtr(hdsaProps, pstg->idsaLastValid);
                cdsa = pstg->idsaLastValid + 1 - CDSA_RESERVED;
                ASSERT(ppropel);

                while (0 < cdsa--)
                    {
                    if (IsFlagSet(ppropel->dwFlags, PEF_VALID))
                        {
                        pstg->idsaLastValid = cdsa - 1;
                        break;
                        }
                    ppropel--;
                    }

                if (0 == cdsa)
                    pstg->idsaLastValid = PROPID_CODEPAGE;
                }
            
            // Since we didn't delete any items from hdsaProps (we freed 
            // the variant value and zeroed it out), this structure 
            // may have a bunch of unused elements at the end.  
            // Compact now if necessary.

            // Do we have a bunch of trailing, empty elements? 
            cdsa = DSA_GetItemCount(hdsaProps);

            if (cdsa > pstg->idsaLastValid + 1)
                {
                // Yes; compact.  Start from the end and go backwards
                // so DSA_DeleteItem doesn't have to move memory blocks.
                for (idsa = cdsa-1; idsa > pstg->idsaLastValid; idsa--)
                    {
#ifdef DEBUG
                    ppropel = DSA_GetItemPtr(hdsaProps, idsa);
                    ASSERT(IsFlagClear(ppropel->dwFlags, PEF_VALID));
#endif
                    DSA_DeleteItem(hdsaProps, idsa);
                    }
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Marks the specified properties dirty or undirty, depending
         on the value of bDirty.  
         
Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_DirtyMultiple(
    IN HPROPSTG    hstg,
    IN ULONG       cpspec,
    IN const PROPSPEC * rgpropspec,
    IN BOOL        bDirty)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)) &&
        EVAL(IS_VALID_READ_BUFFER(rgpropspec, PROPSPEC, cpspec)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        const PROPSPEC * ppropspec = rgpropspec;
        HDSA hdsaProps = pstg->hdsaProps;
        PPROPEL ppropel;
        int idsa;

        hres = S_OK;

        if (0 < cpspec)
            {
            // Mark the list of property specs
            while (0 < cpspec--)
                {
                // Does it exist?
                if (PropStg_PropertyExists(pstg, ppropspec, (LPDWORD)&idsa))
                    {
                    // Yes; mark it
                    ppropel = DSA_GetItemPtr(hdsaProps, idsa);
                    ASSERT(ppropel);

                    if (bDirty)
                        {
                        SetFlag(ppropel->dwFlags, PEF_DIRTY);
                        }
                    else
                        {
                        ClearFlag(ppropel->dwFlags, PEF_DIRTY);
                        }
                    }

                ppropspec++;
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Marks or unmarks all the property values.
         
Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_DirtyAll(
    IN HPROPSTG hstg,
    IN BOOL     bDirty)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        int cdsa = pstg->idsaLastValid + 1 - CDSA_RESERVED;

        hres = S_OK;
        
        if (0 < cdsa)
            {
            PPROPEL ppropel = DSA_GetItemPtr(pstg->hdsaProps, IDSA_START);

            ASSERT(ppropel);

            while (0 < cdsa--)
                {
                if (bDirty)
                    SetFlag(ppropel->dwFlags, PEF_DIRTY);
                else
                    ClearFlag(ppropel->dwFlags, PEF_DIRTY);
                ppropel++;
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Returns S_OK if at least one property value is dirty
         in the storage.  Otherwise, this function returns 
         S_FALSE.
         
Returns: S_OK if it is dirty
         S_FALSE if not
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_IsDirty(
    IN HPROPSTG hstg)
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        int cdsa = pstg->idsaLastValid + 1 - CDSA_RESERVED;

        hres = S_FALSE;

        if (0 < cdsa)
            {
            PPROPEL ppropel = DSA_GetItemPtr(pstg->hdsaProps, IDSA_START);

            ASSERT(ppropel);

            while (0 < cdsa--)
                {
                if (IsFlagSet(ppropel->dwFlags, PEF_DIRTY))
                    {
                    hres = S_OK;
                    break;
                    }
                ppropel++;
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Enumerates thru the list of properties.
         
Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_Enum(
    IN HPROPSTG       hstg,
    IN DWORD          dwFlags,      // One of PSTGEF_ 
    IN PFNPROPSTGENUM pfnEnum,
    IN LPARAM         lParam)       OPTIONAL
    {
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (EVAL(IS_VALID_HANDLE(hstg, PROPSTG)) &&
        EVAL(IS_VALID_CODE_PTR(pfnEnum, PFNPROPSTGENUM)))
        {
        PPROPSTG pstg = (PPROPSTG)hstg;
        int cdsa = pstg->idsaLastValid + 1 - CDSA_RESERVED;
        DWORD dwFlagsPEF = 0;

        hres = S_OK;

        // Set the filter flags
        if (dwFlags & PSTGEF_DIRTY)
            SetFlag(dwFlagsPEF, PEF_DIRTY);

        if (0 < cdsa)
            {
            PPROPEL ppropel = DSA_GetItemPtr(pstg->hdsaProps, IDSA_START);
            int idsa = IDSA_START;

            ASSERT(ppropel);

            while (0 < cdsa--)
                {
                // Does it pass thru filter? 
                if (IsFlagSet(ppropel->dwFlags, PEF_VALID) &&
                    (0 == dwFlagsPEF || (dwFlagsPEF & ppropel->dwFlags)))
                    {
                    // Yes, call callback
                    HRESULT hresT = pfnEnum(idsa, &ppropel->propvar, lParam);
                    if (S_OK != hresT)
                        {
                        if (FAILED(hresT))
                            hres = hresT;
                        break;      // stop enumeration
                        }
                    }
                ppropel++;
                idsa++;
                }
            }
        }

    return hres;
    }


#ifdef DEBUG


/*----------------------------------------------------------
Purpose: Dump propvariant 

Returns: 
Cond:    --
*/
HRESULT
CALLBACK
PropStg_DumpVar(
    IN PROPID        propid,
    IN PROPVARIANT * ppropvar,
    IN LPARAM        lParam)
    {
    TCHAR sz[MAX_PATH];
    PPROPEL ppropel = (PPROPEL)ppropvar;        // we're cheating here

    if (IsFlagSet(ppropel->dwFlags, PEF_DIRTY))
        wnsprintf(sz, ARRAYSIZE(sz), TEXT("    *id:%#lx\t%s"), propid, Dbg_GetVTName(ppropvar->vt));
    else
        wnsprintf(sz, ARRAYSIZE(sz), TEXT("     id:%#lx\t%s"), propid, Dbg_GetVTName(ppropvar->vt));


    switch (ppropvar->vt)
        {
    case VT_EMPTY:
    case VT_NULL:
    case VT_ILLEGAL:
        TraceMsg(TF_ALWAYS, "     %s", sz);
        break;

    case VT_I2:
    case VT_I4:
        TraceMsg(TF_ALWAYS, "     %s\t%d", sz, ppropvar->DUMMYUNION_MEMBER(lVal));
        break;

    case VT_UI1:
        TraceMsg(TF_ALWAYS, "     %s\t%#02x '%c'", sz, ppropvar->DUMMYUNION_MEMBER(bVal), ppropvar->DUMMYUNION_MEMBER(bVal));
        break;
    case VT_UI2:
        TraceMsg(TF_ALWAYS, "     %s\t%#04x", sz, ppropvar->DUMMYUNION_MEMBER(uiVal));
        break;
    case VT_UI4:
        TraceMsg(TF_ALWAYS, "     %s\t%#08x", sz, ppropvar->DUMMYUNION_MEMBER(ulVal));
        break;

    case VT_LPSTR:
        TraceMsg(TF_ALWAYS, "     %s\t\"%S\"", sz, Dbg_SafeStrA(ppropvar->DUMMYUNION_MEMBER(pszVal)));
        break;
    case VT_LPWSTR:
        TraceMsg(TF_ALWAYS, "     %s\t\"%ls\"", sz, Dbg_SafeStrW(ppropvar->DUMMYUNION_MEMBER(pwszVal)));
        break;

    default:
#if defined(_WIN64)
        TraceMsg(TF_ALWAYS, "     %s\t0x%p", sz, (DWORD_PTR)ppropvar->DUMMYUNION_MEMBER(pszVal)); 
#else
	TraceMsg(TF_ALWAYS, "     s\t%#08lx", sz, (DWORD)ppropvar->DUMMYUNION_MEMBER(pszVal));
#endif
        
        break;
        }
    return S_OK;
    }


/*----------------------------------------------------------
Purpose: Enumerates thru the list of properties.
         
Returns: S_OK
         STG_E_INVALIDPARAMETER 
         STG_E_INSUFFICIENTMEMORY 

Cond:    --
*/
HRESULT
WINAPI
PropStg_Dump(
    IN HPROPSTG       hstg,
    IN DWORD          dwFlags)      // One of PSTGDF_ 
    {
    TraceMsg(TF_ALWAYS, "  Property storage 0x%08lx = {", hstg);

    PropStg_Enum(hstg, 0, PropStg_DumpVar, 0);

    TraceMsg(TF_ALWAYS, "  }");

    return NOERROR;
    }

#endif


