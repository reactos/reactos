
/***************************************************************************\
* MapPropertyKey
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Maps a property key string into an atom.
*
* History:
* 21-Dec-1994   JimA    Created.
\***************************************************************************/

__inline ATOM MapPropertyKey(
    PCWSTR pszKey)
{
#ifdef _USERK_
    /*
     * Internal properties must use atoms, not strings.
     */
    UserAssert(!IS_PTR(pszKey));
#else
    /*
     * Is pszKey an atom?  If not, find the atom that matches the string.
     * If one doesn't exist, bail out.
     */
    if (IS_PTR(pszKey))
        return GlobalFindAtomW(pszKey);
#endif

    return PTR_TO_ID(pszKey);
}

/***************************************************************************\
* FindProp
*
* Search the window's property list for the specified property.  pszKey
* could be a string or an atom.  If it is a string, convert it to an atom
* before lookup.  FindProp will only find internal or external properties
* depending on the fInternal flag.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

PPROP _FindProp(
    PWND pwnd,
    PCWSTR pszKey,
    BOOL fInternal)
{
    UINT i;
    PPROPLIST ppropList;
    PPROP pprop;
    ATOM atomKey;

    /*
     * Make sure we have a property list.
     */
    ppropList = REBASE(pwnd, ppropList);
    if (ppropList == NULL)
        return NULL;

    /*
     * Call to the appropriate routine to verify the key name.
     */
    atomKey = MapPropertyKey(pszKey);
    if (atomKey == 0)
        return NULL;

    /*
     * Now we've got the atom, search the list for a property with the
     * same atom/name.  Make sure to only return internal properties if
     * the fInternal flag is set.  Do the same for external properties.
     */
    pprop = ppropList->aprop;
    for (i = ppropList->iFirstFree; i > 0; i--) {
        if (pprop->atomKey == atomKey) {
            if (fInternal) {
                if (pprop->fs & PROPF_INTERNAL)
                    return pprop;
            } else {
                if (!(pprop->fs & PROPF_INTERNAL))
                    return pprop;
            }
        }
        pprop++;
    }

    /*
     * Property not found, too bad.
     */
    return NULL;
}

/***************************************************************************\
* InternalGetProp
*
* Search the window's property list for the specified property and return
* the hData handle from it.  If the property is not found, NULL is returned.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

HANDLE _GetProp(
    PWND pwnd,
    PCWSTR pszKey,
    BOOL fInternal)
{
    PPROP pprop;

    /*
     * A quick little optimization for that case where the window has no
     * properties at all.
     */
    if (pwnd->ppropList == NULL)
        return NULL;

    /*
     * FindProp does all the work, including converting pszKey to an atom
     * (if necessary) for property lookup.
     */
    pprop = _FindProp(pwnd, pszKey, fInternal);
    if (pprop == NULL)
        return NULL;

    return pprop->hData;
}
