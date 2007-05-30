/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Prefix implementation
 * FILE:              lib/rtl/unicodeprefix.c
 * PROGRAMMER:        Alex Ionescu (alex@relsoft.net) 
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/*
 * FIXME: Try to find the official names and add to NDK
 * Definitions come from fastfat driver.
 */
typedef USHORT NODE_TYPE_CODE;
#define PFX_NTC_TABLE       ((NODE_TYPE_CODE)0x0800)
#define PFX_NTC_ROOT        ((NODE_TYPE_CODE)0x0801)
#define PFX_NTC_CHILD       ((NODE_TYPE_CODE)0x0802)
#define PFX_NTC_CASE_MATCH  ((NODE_TYPE_CODE)0x0803)

/* FUNCTIONS ***************************************************************/

ULONG
NTAPI
ComputeUnicodeNameLength(IN PUNICODE_STRING UnicodeName)
{
    ULONG Chars = UnicodeName->Length / sizeof(WCHAR);
    ULONG i, NamesFound = 1;

    /* Loop the string */
    for (i = 0; i < (Chars - 1); i++)
    {
        /* Check if we found a backslash, meaning another name */
        if (UnicodeName->Buffer[i] == '\\') NamesFound++;
    }

    /* Return the number of names found */
    return NamesFound;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
CompareUnicodeStrings(IN PUNICODE_STRING Prefix,
                      IN PUNICODE_STRING String,
                      IN ULONG CaseCheckChar)
{
    ULONG StringLength = String->Length / sizeof(WCHAR);
    ULONG PrefixLength = Prefix->Length / sizeof(WCHAR);
    ULONG ScanLength = min(StringLength, PrefixLength);
    ULONG i;
    WCHAR FoundPrefix, FoundString;
    PWCHAR p, p1;

    /* Handle case noticed in npfs when Prefix = '\' and name starts with '\' */
    if ((PrefixLength == 1) &&
        (Prefix->Buffer[0] == '\\') &&
        (StringLength > 1) &&
        (String->Buffer[0] == '\\'))
    {
        /* The string is actually a prefix */
        return -1;
    }

    /* Validate the Case Check Character Position */
    if (CaseCheckChar > ScanLength) CaseCheckChar = ScanLength;

    /* Do the case sensitive comparison first */
    for (i = 0; i < CaseCheckChar; i++)
    {
        /* Compare the two characters */
        if (Prefix->Buffer[i] != String->Buffer[i]) break;
    }

    /* Save the characters we found */
    FoundPrefix = Prefix->Buffer[i];
    FoundString = String->Buffer[i];

    /* Check if we exausted the search above */
    if (i == CaseCheckChar)
    {
        /* Do a case-insensitive search */
        p = &Prefix->Buffer[i];
        p1 = &String->Buffer[i];
        do
        {
            /* Move to the next character */
            FoundPrefix = *p++;
            FoundString = *p1++;

            /* Compare it */
            if (FoundPrefix != FoundString)
            {
                /* Upcase the characters */
                FoundPrefix = RtlUpcaseUnicodeChar(FoundPrefix);
                FoundString = RtlUpcaseUnicodeChar(FoundString);

                /* Compare them again */
                if (FoundPrefix != FoundString) break;
            }

            /* Move to the next char */
            i++;
        } while (i < ScanLength);
    }

    /* Check if we weren't able to find a match in the loops */
    if (i < ScanLength)
    {
        /* If the prefix character found was a backslash, this is a less */
        if (FoundPrefix == '\\') return GenericLessThan;

        /* If the string character found was a backslack, then this is a more */
        if (FoundString == '\\') return GenericGreaterThan;

        /* None of those two special cases, do a normal check */
        if (FoundPrefix < FoundString) return GenericLessThan;

        /* The only choice left is that Prefix > String */
        return GenericGreaterThan;
    }

    /* If we got here, a match was found. Check if the prefix is smaller */
    if (PrefixLength < StringLength)
    {
        /* Check if the string is actually a prefix */
        if (String->Buffer[PrefixLength] == '\\') return -1;

        /* It's not a prefix, and it's shorter, so it's a less */
        return GenericLessThan;
    }
    
    /* Check if the prefix is longer */
    if (PrefixLength > StringLength) return GenericGreaterThan;

    /* If we got here, then they are 100% equal */
    return GenericEqual;
}

/*
 * @implemented
 */
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                     PUNICODE_STRING FullName,
                     ULONG CaseInsensitiveIndex)
{
    ULONG NameCount;
    PUNICODE_PREFIX_TABLE_ENTRY CurrentEntry, PreviousEntry, Entry, NextEntry;
    PRTL_SPLAY_LINKS SplayLinks;
    RTL_GENERIC_COMPARE_RESULTS Result;

    /* Find out how many names there are */
    NameCount = ComputeUnicodeNameLength(FullName);

    /* Find the right spot where to start looking for this entry */
    PreviousEntry = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    CurrentEntry = PreviousEntry->NextPrefixTree;
    while (CurrentEntry->NameLength > (CSHORT)NameCount)
    {
        /* Not a match, move to the next entry */
        PreviousEntry = CurrentEntry;
        CurrentEntry = CurrentEntry->NextPrefixTree;
    }

    /* Loop every entry which has valid entries */
    while (CurrentEntry->NameLength)
    {
        /* Get the splay links and loop */
        SplayLinks = &CurrentEntry->Links;
        while (SplayLinks)
        {
            /* Get the entry */
            Entry = CONTAINING_RECORD(SplayLinks,
                                      UNICODE_PREFIX_TABLE_ENTRY,
                                      Links);

            /* Do the comparison */
            Result = CompareUnicodeStrings(Entry->Prefix, FullName, 0);
            if (Result == GenericGreaterThan)
            {
                /* Prefix is greater, so restart on the left child */
                SplayLinks = RtlLeftChild(SplayLinks);
                continue;
            }
            else if (Result == GenericLessThan)
            {
                /* Prefix is smaller, so restart on the right child */
                SplayLinks = RtlRightChild(SplayLinks);
                continue;
            }

            /*
             * We have a match, check if this was a case-sensitive search
             * NOTE: An index of 0 means case-insensitive(ie, we'll be case
             * insensitive since index 0, ie, all the time)
             */
            if (!CaseInsensitiveIndex)
            {
                /* 
                 * Check if this entry was a child. We need to return the root,
                 * so if this entry was a child, we'll splay the tree and get
                 * the root, and set the current entry as a child.
                 */
                if (Entry->NodeTypeCode == PFX_NTC_CHILD)
                {
                    /* Get the next entry */
                    NextEntry = CurrentEntry->NextPrefixTree;

                    /* Make the current entry become a child */
                    CurrentEntry->NodeTypeCode = PFX_NTC_CHILD;
                    CurrentEntry->NextPrefixTree = NULL;

                    /* Splay the tree */
                    SplayLinks = RtlSplay(&Entry->Links);

                    /* Get the new root entry */
                    Entry = CONTAINING_RECORD(SplayLinks,
                                              UNICODE_PREFIX_TABLE_ENTRY,
                                              Links);

                    /* Set it as a root entry */
                    Entry->NodeTypeCode = PFX_NTC_ROOT;

                    /* Add it to the root entries list */
                    PreviousEntry->NextPrefixTree = Entry;
                    Entry->NextPrefixTree = NextEntry;
                }

                /* Return the entry */
                return Entry;
            }

            /* We'll do a case-sensitive search if we've reached this point */
            NextEntry = Entry;
            do
            {
                /* Do the case-sensitive search */
                Result = CompareUnicodeStrings(NextEntry->Prefix,
                                               FullName,
                                               CaseInsensitiveIndex);
                if ((Result != GenericLessThan) &&
                    (Result != GenericGreaterThan))
                {
                    /* This is a positive match, return it */
                    return NextEntry;
                }

                /* No match yet, continue looping the circular list */
                NextEntry = NextEntry->CaseMatch;
            } while (NextEntry != Entry);

            /*
             * If we got here, then we found a non-case-sensitive match, but
             * we need to find a case-sensitive match, so we'll just keep 
             * searching the next tree (NOTE: we need to break out for this).
             */
            break;
        }

        /* Splay links exhausted, move to next entry */
        PreviousEntry = CurrentEntry;
        CurrentEntry = CurrentEntry->NextPrefixTree;
    }

    /* If we got here, nothing was found */
    return NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable)
{
    /* Setup the table */
    PrefixTable->NameLength = 0;
    PrefixTable->LastNextEntry = NULL;
    PrefixTable->NodeTypeCode = PFX_NTC_TABLE;
    PrefixTable->NextPrefixTree = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                       PUNICODE_STRING Prefix,
                       PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry)
{
    PUNICODE_PREFIX_TABLE_ENTRY CurrentEntry, PreviousEntry, Entry, NextEntry;
    ULONG NameCount;
    RTL_GENERIC_COMPARE_RESULTS Result;
    PRTL_SPLAY_LINKS SplayLinks;

    /* Find out how many names there are */
    NameCount = ComputeUnicodeNameLength(Prefix);

    /* Set up the initial entry data */
    PrefixTableEntry->NameLength = (CSHORT)NameCount;
    PrefixTableEntry->Prefix = Prefix;
    RtlInitializeSplayLinks(&PrefixTableEntry->Links);

    /* Find the right spot where to insert this entry */
    PreviousEntry = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    CurrentEntry = PreviousEntry->NextPrefixTree;
    while (CurrentEntry->NameLength > (CSHORT)NameCount)
    {
        /* Not a match, move to the next entry */
        PreviousEntry = CurrentEntry;
        CurrentEntry = CurrentEntry->NextPrefixTree;
    }

    /* Check if we did find a tree by now */
    if (CurrentEntry->NameLength != (CSHORT)NameCount)
    {
        /* We didn't, so insert a new entry in the list */
        PreviousEntry->NextPrefixTree = PrefixTableEntry;
        PrefixTableEntry->NextPrefixTree = CurrentEntry;

        /* This is now a root entry with case match */
        PrefixTableEntry->NodeTypeCode = PFX_NTC_ROOT;
        PrefixTableEntry->CaseMatch = PrefixTableEntry;

        /* Quick return */
        return TRUE;
    }

    /* We found a tree, so start the search loop */
    Entry = CurrentEntry;
    while (TRUE)
    {
        /* Do a case-insensitive comparison to find out the match level */
        Result = CompareUnicodeStrings(Entry->Prefix, Prefix, 0);
        if (Result == GenericEqual)
        {
            /* We have a match, start doing a case-sensitive search */
            NextEntry = Entry;

            /* Search the circular case-match list */
            do
            {
                /* Check if we found a match */
                if (CompareUnicodeStrings(NextEntry->Prefix, Prefix, -1) ==
                    (GenericEqual))
                {
                    /* We must fail the insert: it already exists */
                    return FALSE;
                }

                /* Move to the next entry in the circular list */
                NextEntry = NextEntry->CaseMatch;
            }
            while (NextEntry != Entry);

            /*
             * No match found, so we can safely insert it. Remember that a
             * case insensitive match was found, so this is not a ROOT NTC
             * but a Case Match NTC instead.
             */
            PrefixTableEntry->NodeTypeCode = PFX_NTC_CASE_MATCH;
            PrefixTableEntry->NextPrefixTree = NULL;
            
            /* Insert it into the circular list */
            PrefixTableEntry->CaseMatch = Entry->CaseMatch;
            Entry->CaseMatch = PrefixTableEntry;
            break;
        }

        /* Check if the result was greater or lesser than */
        if (Result == GenericGreaterThan)
        {
            /* Check out if we have a left child */
            if (RtlLeftChild(&Entry->Links))
            {
                /* We do, enter it and restart the loop */
                SplayLinks = RtlLeftChild(&Entry->Links);
                Entry = CONTAINING_RECORD(SplayLinks,
                                          UNICODE_PREFIX_TABLE_ENTRY,
                                          Links);
            }
            else
            {
                /* We don't, set this entry as a child */
                PrefixTableEntry->NodeTypeCode = PFX_NTC_CHILD;
                PrefixTableEntry->NextPrefixTree = NULL;
                PrefixTableEntry->CaseMatch = PrefixTableEntry;

                /* Insert us into the tree */
                RtlInsertAsLeftChild(&Entry->Links, &PrefixTableEntry->Links);
                break;
            }
        }
        else
        {
            /* Check out if we have a right child */
            if (RtlRightChild(&Entry->Links))
            {
                /* We do, enter it and restart the loop */
                SplayLinks = RtlLeftChild(&Entry->Links);
                Entry = CONTAINING_RECORD(SplayLinks,
                                          UNICODE_PREFIX_TABLE_ENTRY,
                                          Links);
            }
            else
            {
                /* We don't, set this entry as a child */
                PrefixTableEntry->NodeTypeCode = PFX_NTC_CHILD;
                PrefixTableEntry->NextPrefixTree = NULL;
                PrefixTableEntry->CaseMatch = PrefixTableEntry;

                /* Insert us into the tree */
                RtlInsertAsRightChild(&Entry->Links, &PrefixTableEntry->Links);
                break;
            }
        }
    }

    /* Get the next tree entry */
    NextEntry = CurrentEntry->NextPrefixTree;

    /* Set what was the current entry to a child entry */
    CurrentEntry->NodeTypeCode = PFX_NTC_CHILD;
    CurrentEntry->NextPrefixTree = NULL;

    /* Splay the tree */
    SplayLinks = RtlSplay(&Entry->Links);

    /* The link points to the root, get it */
    Entry = CONTAINING_RECORD(SplayLinks, UNICODE_PREFIX_TABLE_ENTRY, Links);

    /* Mark the root as a root entry */
    Entry->NodeTypeCode = PFX_NTC_ROOT;

    /* Add it to the tree list */
    PreviousEntry->NextPrefixTree = Entry;
    Entry->NextPrefixTree = NextEntry;

    /* Return success */
    return TRUE;
}

/*
* @implemented
*/
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                     BOOLEAN Restart)
{
    PRTL_SPLAY_LINKS SplayLinks;
    PUNICODE_PREFIX_TABLE_ENTRY Entry, CaseMatchEntry;

    /* We might need this entry 2/3rd of the time, so cache it now */
    CaseMatchEntry = PrefixTable->LastNextEntry->CaseMatch;

    /* Check if this is a restart or if we don't have a last entry */
    if ((Restart) || !(PrefixTable->LastNextEntry))
    {
        /* Get the next entry and validate it */
        Entry = PrefixTable->NextPrefixTree;
        if (Entry->NodeTypeCode == PFX_NTC_TABLE) return NULL;

        /* Now get the Splay Tree Links */
        SplayLinks = &Entry->Links;

        /* Loop until we get the first node in the tree */
        while (RtlLeftChild(SplayLinks)) SplayLinks = RtlLeftChild(SplayLinks);

        /* Get the entry from it */
        Entry = CONTAINING_RECORD(SplayLinks,
                                  UNICODE_PREFIX_TABLE_ENTRY,
                                  Links);
    }
    else if (CaseMatchEntry->NodeTypeCode == PFX_NTC_CASE_MATCH)
    {
        /* If the last entry was a Case Match, then return it */
        Entry = CaseMatchEntry;
    }
    else
    {
        /* Find the successor */
        SplayLinks = RtlRealSuccessor(&CaseMatchEntry->Links);
        if (!SplayLinks)
        {
            /* Didn't find one, we'll have to search the tree */
            SplayLinks = &PrefixTable->LastNextEntry->Links;

            /* Get the topmost node (root) */
            while (!RtlIsRoot(SplayLinks)) SplayLinks = RtlParent(SplayLinks);
            Entry = CONTAINING_RECORD(SplayLinks,
                                      UNICODE_PREFIX_TABLE_ENTRY,
                                      Links);

            /* Get its tree and make sure somethign is in it */
            Entry = Entry->NextPrefixTree;
            if (Entry->NameLength <= 0) return NULL;

            /* Select these new links and find the first node */
            while (RtlLeftChild(SplayLinks)) SplayLinks = RtlLeftChild(SplayLinks);
        }

        /* Get the entry from it */
        Entry = CONTAINING_RECORD(SplayLinks,
                                  UNICODE_PREFIX_TABLE_ENTRY,
                                  Links);
    }

    /* Save this entry as the last one returned, and return it */
    PrefixTable->LastNextEntry = Entry;
    return Entry;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlRemoveUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                       PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry)
{
    PUNICODE_PREFIX_TABLE_ENTRY Entry, RefEntry, NewEntry;
    PRTL_SPLAY_LINKS SplayLinks;

    /* Erase the last entry */
    PrefixTable->LastNextEntry = NULL;

    /* Check if this was a Case Match Entry */
    if (PrefixTableEntry->NodeTypeCode == PFX_NTC_CASE_MATCH)
    {
        /* Get the case match entry */
        Entry = PrefixTableEntry->CaseMatch;

        /* Now loop until we find one referencing what the caller sent */
        while (Entry->CaseMatch != PrefixTableEntry) Entry = Entry->CaseMatch;

        /* We found the entry that was sent, link them to delete this entry */
        Entry->CaseMatch = PrefixTableEntry->CaseMatch;
    }
    else if ((PrefixTableEntry->NodeTypeCode == PFX_NTC_ROOT) ||
            (PrefixTableEntry->NodeTypeCode == PFX_NTC_CHILD))
    {
        /* Check if this entry is a case match */
        if (PrefixTableEntry->CaseMatch != PrefixTableEntry)
        {
            /* Get the case match entry */
            Entry = PrefixTableEntry->CaseMatch;

            /* Now loop until we find one referencing what the caller sent */
            while (Entry->CaseMatch != PrefixTableEntry) Entry = Entry->CaseMatch;

            /* We found the entry that was sent, link them to delete this entry */
            Entry->CaseMatch = PrefixTableEntry->CaseMatch;

            /* Copy the data */
            Entry->NodeTypeCode = PrefixTableEntry->NodeTypeCode;
            Entry->NextPrefixTree = PrefixTableEntry->NextPrefixTree;
            Entry->Links = PrefixTableEntry->Links;

            /* Now check if we are a root entry */
            if (RtlIsRoot(&PrefixTableEntry->Links))
            {
                /* We are, so make this entry root as well */
                Entry->Links.Parent = &Entry->Links;

                /* Find the entry referencing us */
                RefEntry = Entry->NextPrefixTree;
                while (RefEntry->NextPrefixTree != Entry)
                {
                    /* Not this one, move to the next entry */
                    RefEntry = RefEntry->NextPrefixTree;
                }

                /* Link them to us now */
                RefEntry->NextPrefixTree = Entry;
            }
            else if (RtlIsLeftChild(&PrefixTableEntry->Links))
            {
                /* We were the left child, so make us as well */
                RtlParent(&PrefixTableEntry->Links)->LeftChild = &Entry->Links;
            }
            else
            {
                /* We were the right child, so make us as well */
                RtlParent(&PrefixTableEntry->Links)->RightChild = &Entry->Links;
            }

            /* Check if we have a left child */
            if (RtlLeftChild(&Entry->Links))
            {
                /* Update its parent link */
                RtlLeftChild(&Entry->Links)->Parent = &Entry->Links;
            }
            /* Check if we have a right child */
            if (RtlRightChild(&Entry->Links))
            {
                /* Update its parent link */
                RtlRightChild(&Entry->Links)->Parent = &Entry->Links;
            }
        }
        else
        {
            /* It's not a case match, so we'll delete the actual entry */
            SplayLinks = &PrefixTableEntry->Links;

            /* Find the root entry */
            while (!RtlIsRoot(SplayLinks)) SplayLinks = RtlParent(SplayLinks);
            Entry = CONTAINING_RECORD(SplayLinks,
                                      UNICODE_PREFIX_TABLE_ENTRY,
                                      Links);

            /* Delete the entry and check if the whole tree is gone */
            SplayLinks = RtlDelete(&PrefixTableEntry->Links);
            if (!SplayLinks)
            {
                /* The tree is also gone now, find the entry referencing us */
                RefEntry = Entry->NextPrefixTree;
                while (RefEntry->NextPrefixTree != Entry)
                {
                    /* Not this one, move to the next entry */
                    RefEntry = RefEntry->NextPrefixTree;
                }

                /* Link them so this entry stops being referenced */
                RefEntry->NextPrefixTree = Entry->NextPrefixTree;
            }
            else if (&Entry->Links != SplayLinks)
            {
                /* The tree is still here, but we got moved to a new one */
                NewEntry = CONTAINING_RECORD(SplayLinks,
                                             UNICODE_PREFIX_TABLE_ENTRY,
                                             Links);

                /* Find the entry referencing us */
                RefEntry = Entry->NextPrefixTree;
                while (RefEntry->NextPrefixTree != Entry)
                {
                    /* Not this one, move to the next entry */
                    RefEntry = RefEntry->NextPrefixTree;
                }

                /* Since we got moved, make us the new root entry */
                NewEntry->NodeTypeCode = PFX_NTC_ROOT;

                /* Link us with the entry referencing the old root */
                RefEntry->NextPrefixTree = NewEntry;

                /* And link us with the old tree */
                NewEntry->NextPrefixTree = Entry->NextPrefixTree;

                /* Set the old tree as a child */
                Entry->NodeTypeCode = PFX_NTC_CHILD;
                Entry->NextPrefixTree = NULL;
            }
        }
    }
}

/* EOF */
