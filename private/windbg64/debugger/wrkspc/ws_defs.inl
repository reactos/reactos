//
// CGen_WKSP implementation
//
template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Containers_DeleteList()
{
    TListEntry<TCont *> * pEntry = NULL;
    while (!m_listConts.IsEmpty()) {
        pEntry = m_listConts.FirstEntry();

        // delete the container
        delete pEntry->m_tData;
        // delete the list entry
        delete pEntry;
    }
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Items_DeleteList()
{
    TListEntry<TItem *> * pEntry = NULL;
    while (!m_listItems.IsEmpty()) {
        pEntry = m_listItems.FirstEntry();

        delete pEntry;
    }
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
AddToContainerList(
    CGenInterface_WKSP * p
    )
{
    AssertChildOf(p, TCont);

    TCont * pCont = (TCont *) p;
    m_listConts.InsertTail(pCont);
}

template<class TCont, class TItem>
CItemInterface_WKSP *
CGen_WKSP<TCont, TItem>::
GetDataItemInterface(
    PVOID pv
    ) const
{
    TListEntry<TItem *> * pItemEntry = m_listItems.FirstEntry();
    for (; pItemEntry != m_listItems.Stop(); pItemEntry = pItemEntry->Flink) {
        if (pItemEntry->m_tData->DoDataPointersMatch(pv)) {
            return pItemEntry->m_tData;
        }
    }
    return NULL;
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
CloseRegistryKeys()
/*++
Routine Description:
    Closes the main and mirrored reg keys.

    Will recursively close the the keys of the child containers.
--*/
{
    LONG lres;

    // Close the keys of the child containers
    {
        TListEntry<TCont *> * pContEntry = m_listConts.FirstEntry();
        for (; pContEntry != m_listConts.Stop(); pContEntry = pContEntry->Flink) {
            pContEntry->m_tData->CloseRegistryKeys();
        }
    }

    if (m_hkeyRegistry) {
        lres = RegCloseKey(m_hkeyRegistry);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif
        m_hkeyRegistry = NULL;
    }

    if (m_hkeyMirror) {
        Assert(m_bMirror);

        lres = RegCloseKey(m_hkeyMirror);
#ifdef DBG
        if (ERROR_SUCCESS != lres) {
            WKSP_DisplayLastErrorMsgBox();
        }
#endif
        m_hkeyMirror = NULL;
    }
}

template<class TCont, class TItem>
HKEY
CGen_WKSP<TCont, TItem>::
GetRegistryKey(
    BOOL * const pbRegKeyCreated
    )
/*++
Routine Description:
    Will return the MAIN reg key that corresponds to this container. If the reg
    key is not open, it is opened.

Arguments:
    pbRegKeyCreated - Can be NULL. If not NULL, it is assigned TRUE if the
        reg key was created, FALSE if it already existed.

Returns:
    An HKEY to the current reg key.
--*/
{
    if (m_hkeyRegistry) {
        if (pbRegKeyCreated) {
            // Already existed.
            *pbRegKeyCreated = FALSE;
        }
        return m_hkeyRegistry;
    }

    Assert(m_pParent);
    m_hkeyRegistry = WKSP_RegKeyOpenCreate(m_pParent->GetRegistryKey(), 
                                           m_pszRegistryName,
                                           pbRegKeyCreated);

    Assert(m_hkeyRegistry);
    return m_hkeyRegistry;
}

template<class TCont, class TItem>
HKEY
CGen_WKSP<TCont, TItem>::
GetMirrorKey(
    PBOOL   pbRegKeyCreated,
    PSTR    pszSubstituteRegistryName
    )
/*++
Routine Description:
    Will return the MIRROR reg key that corresponds to this container. If the reg
    key is not open, it is opened.

Arguments:
    pbRegKeyCreated - Can be NULL. If not NULL, it is assigned TRUE if the
        reg key was created, FALSE if it already existed.

    pszSubstituteRegistryName -
        NULL - Will use the value in m_pszRegistryName
        else - It will use this name to create a mirror registry key

Returns:
    An HKEY to the current reg key.
--*/
{
    // If it is mirrored, it must have a parent.
    Assert(m_pParent);
    Assert(m_bMirror);

    if (m_hkeyMirror) {
        if (pbRegKeyCreated) {
            // Already existed.
            pbRegKeyCreated = FALSE;
        }
        return m_hkeyMirror;
    }

    // We haven't opened/created the key yet.
    HKEY hkeyParent = NULL;
    if (m_pParent->m_bMirror) {
        // The parent is also mirrored, let's get his key,
        // so we can open ours.
        hkeyParent = m_pParent->GetMirrorKey();
    } else {
        // This is the beginning of the mirrored branch. The
        // parent for the mirror, is going to be a branch beneath the root of the tree.
        hkeyParent = GetRootParent()->GetRegistryKey();
    }

    Assert(hkeyParent);

    if (pszSubstituteRegistryName) {
        m_hkeyMirror = WKSP_RegKeyOpenCreate(hkeyParent, 
                                             pszSubstituteRegistryName, 
                                             pbRegKeyCreated
                                             );
    } else {
        m_hkeyMirror = WKSP_RegKeyOpenCreate(hkeyParent, 
                                             m_pszRegistryName,
                                             pbRegKeyCreated
                                             );
    }

    Assert(m_hkeyMirror);
    return m_hkeyMirror;
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Duplicate(
    const CGenInterface_WKSP & argTmp
    )
/*++
Routine Description:
    Recursively duplicates the contents of another class. However, the
    class is not attached the parent. In order words, this copy
    is not contained in the parent's list of containers.

    The registry name and pointer to the parent are also duplicated.

    NOTE: CGen_WKSP & CGen_WKSP

Arguments:
    arg - Cannot be NULL. Must point to another container.
--*/
{
    const CGen_WKSP<TCont, TItem> & arg = *(const CGen_WKSP<TCont, TItem> *) &argTmp;

    // Make sure that we dealing with another container.
    AssertType(*this, *(&arg));
    Assert(m_bDynamicList == arg.m_bDynamicList);

    SetRegistryName(arg.m_pszRegistryName);

    m_bMirror = arg.m_bMirror;

    // Copy the items. They should be in the same order.
    {
        Assert(m_listItems.Size() == arg.m_listItems.Size());

        TListEntry<TItem *> * pItemEntry = m_listItems.FirstEntry();
        TListEntry<TItem *> * pArgItemEntry = arg.m_listItems.FirstEntry();

        for (; pItemEntry != m_listItems.Stop();
            pItemEntry = pItemEntry->Flink, pArgItemEntry = pArgItemEntry->Flink) {

            AssertType(*pItemEntry, *pArgItemEntry);
            pItemEntry->m_tData->Duplicate(*pArgItemEntry->m_tData);
        }
    }

    // Copy the containers. They should be in the same order.
    if (m_bDynamicList) {
        Containers_DeleteList();
        Assert(0 == m_listConts.Size());

        TListEntry<TCont *> * pArgContEntry = arg.m_listConts.FirstEntry();

        for (; pArgContEntry != arg.m_listConts.Stop();
            pArgContEntry = pArgContEntry->Flink) {

            TCont * pCont = new TCont();

            AssertType(*pCont, *pArgContEntry->m_tData);

            pCont->Init(this, pArgContEntry->m_tData->m_pszRegistryName,
                pArgContEntry->m_tData->m_bMirror, pArgContEntry->m_tData->m_bDynamicList);


            pCont->Duplicate(*pArgContEntry->m_tData);
        }
    } else {
        Assert(m_listConts.Size() == arg.m_listConts.Size());

        TListEntry<TCont *> * pContEntry = m_listConts.FirstEntry();
        TListEntry<TCont *> * pArgContEntry = arg.m_listConts.FirstEntry();

        for (; pContEntry != m_listConts.Stop();
            pContEntry = pContEntry->Flink, pArgContEntry = pArgContEntry->Flink) {

            AssertType(*pContEntry, *pArgContEntry);
            pContEntry->m_tData->Duplicate(*pArgContEntry->m_tData);
        }
    }
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Restore(
    BOOL bOnlyItems
    )
/*++
Routine Description:
    Will try to restore the tree and values from the main reg location.

    If the main reg location does not exist, but the tree is mirrored, the values
    will be loaded from the mirror. If neither the main or mirrored keys exist,
    then both are created.

    If the tree does not exist and it is not mirrored, then it is created and
    the values are written out.    

Arguments

    bOnlyItems
        TRUE - Only Items are restored
        FALSE - Containers and Items are restored
--*/
{
    BOOL bRegistryKeyCreated;
    HKEY hkey = NULL;

    // Try to restore the info from the main reg tree
    hkey = GetRegistryKey(&bRegistryKeyCreated);

    if (!bRegistryKeyCreated) {
        // We have data in the main reg tree, let's read it.
        Restore_Read(hkey, bOnlyItems);
    } else {
        // We have nothing in the main reg tree

        // Do we have default data in the mirror
        if (!m_bMirror) {
            // No mirror, then just save the values.
            Save(FALSE, FALSE);
        } else {
            // Try to restore it from the mirror.
            hkey = GetMirrorKey(&bRegistryKeyCreated);

            if (!bRegistryKeyCreated) {
                // We have data in the mirrored reg tree, let's read it.
                Restore_Read(hkey, bOnlyItems);
            } else {
                // We have nothing in the registry and nothing
                //  in the mirror. Create all of it.
                Save(FALSE, FALSE);
            }
        }
    }
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Save(
    BOOL bOnlySaveMirror,
    BOOL bOnlyItems
     )
/*++
Routine Description:
    Will save all of the items in the container, and in the child containers.

    If the tree does not exist it is created and the values are written out.

Arguments:
    bOnlySaveMirror -
        TRUE  - only save the mirrored values.
        FALSE - saves both the registry and mirrored values.

    bOnlyItems
        TRUE - Only Items are restored
        FALSE - Containers and Items are restored

--*/
{
    // Close everything before we start deleting things
    //  and shoot ourselves in the foot.
    if (m_bDynamicList) {
        CloseRegistryKeys();

        if (m_bMirror) {
            WKSP_RegDeleteContents(GetMirrorKey());
        }

        if ( !bOnlySaveMirror ) {
            WKSP_RegDeleteContents(GetRegistryKey());
        }
    }

    // Make sure everything has been created and/or reopened.
    GetRegistryKey();
    if (m_bMirror) {
        GetMirrorKey();
    }

    // Save the data items
    {
        TListEntry<TItem *> * pItemEntry = m_listItems.FirstEntry();
        for (; pItemEntry != m_listItems.Stop(); pItemEntry = pItemEntry->Flink) {
            // Are both the registry and mirrored values to be written out.
            if (!bOnlySaveMirror) {
                pItemEntry->m_tData->Save(m_hkeyRegistry);
            }

            if (m_hkeyMirror) {
                pItemEntry->m_tData->Save(m_hkeyMirror);
            }
        }
    }

    // Save the owned containers
    if ( !bOnlyItems ) {
        TListEntry<TCont *> * pContEntry = m_listConts.FirstEntry();
        for (; pContEntry != m_listConts.Stop(); pContEntry = pContEntry->Flink) {
            pContEntry->m_tData->Save(bOnlySaveMirror, bOnlyItems);
        }
    }
}

template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
Restore_Read(
    const HKEY hkey,
    BOOL bOnlyItems
    )
/*++
Routine Description:
    Actually reads the values from the reg key. Dynamically creates a list of
    workspace items based on the types of values if the reg key.

Arguments:
    hkey - Can't be NULL. Must be an open registry key.

    bOnlyItems
        TRUE - Only Items are restored
        FALSE - Containers and Items are restored
--*/
{
    // Restore the data items
    TListEntry<TItem *> * pItemEntry = m_listItems.FirstEntry();
    for (; pItemEntry != m_listItems.Stop(); pItemEntry = pItemEntry->Flink) {
        pItemEntry->m_tData->Restore(hkey);
    }

    //
    // Already restored data items, time to bail
    //
    if (bOnlyItems) {
        return;
    }

    if (!m_bDynamicList) {

        // Restore the owned containers
        if ( !bOnlyItems ) {
            TListEntry<TCont *> * pContEntry = m_listConts.FirstEntry();
            for (; pContEntry != m_listConts.Stop(); pContEntry = pContEntry->Flink) {
                pContEntry->m_tData->Restore(FALSE); // Restore everything
            }
        }

    } else {

        //
        // Dynamice containers 
        //

        // Clean everything out
        Containers_DeleteList();
        //Assert(0 == m_listItems.Size());

        // Get the name of the longest key name and value name
        DWORD dwMaxSubKeyLen;
        DWORD dwMaxValueNameLen;
        DWORD dwNumSubKeys;
        DWORD dwNumValues;

        WKSP_RegKeyValueInfo(hkey, &dwNumSubKeys, &dwNumValues, &dwMaxSubKeyLen, &dwMaxValueNameLen);

        // Include space for the null terminator.
        DWORD dwMaxSize = max(dwMaxSubKeyLen, dwMaxValueNameLen) +1;
        PSTR pszBuffer = (PSTR) calloc(dwMaxSize, 1);
        Assert(pszBuffer);

        // Recreate the dynamic containers
        for (DWORD dwCounter = 0; dwCounter < dwNumSubKeys; dwCounter++) {
            // Reset the size of the buffer
            DWORD dwLen = dwMaxSize;

            WKSP_RegGetKeyName(hkey, dwCounter, pszBuffer, &dwLen);

            // Create a new dynamic container
            TCont * pCont = new TCont();
            Assert(pCont);

            pCont->Init(this, pszBuffer);

            pCont->Restore(bOnlyItems);
        }

        // Cleanup
        free(pszBuffer);
    }
}


template<class TCont, class TItem>
void
CGen_WKSP<TCont, TItem>::
SetMirrorFlagForChildren()
/*++
Routine Description:
    If a parent if mirrored, then it's children
    must also be mirrored.

    We add this in because the order of construction of classes
    doesn't always assure that the mirrored flag is properly set.

    Meant to be called from the top the class hierachy so that it 
    can call of its children.

Returns:
    
--*/
{
    TListEntry<TCont *> * pContEntry = m_listConts.FirstEntry();
    for (; pContEntry != m_listConts.Stop(); pContEntry = pContEntry->Flink) {
        
        if (m_bMirror) {
            pContEntry->m_tData->m_bMirror = TRUE;
        }

        pContEntry->m_tData->SetMirrorFlagForChildren();

    }
}

