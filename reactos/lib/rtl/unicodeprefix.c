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

STATIC
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

/*
 * @unimplemented
 */
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                     PUNICODE_STRING FullName,
                     ULONG CaseInsensitiveIndex)
{
	UNIMPLEMENTED;
	return 0;
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
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                       PUNICODE_STRING Prefix,
                       PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry)
{
    /*
     * implementation notes:
     * - get name length (number of names) DONE
     * - init splay links DONE
     * - find a matching tree DONE
     * - if !found, insert a new NTC_ROOT entry and return TRUE; DONE
     * - if found, loop tree and compare strings:
     *   if equal, handle casematch/nomatch
     *   if greater or lesser equal, then add left/right childs accordingly
     * - splay the tree
     */
    PUNICODE_PREFIX_TABLE_ENTRY CurrentEntry, PreviousEntry;
    ULONG NameCount;

    /* Find out how many names there are */
    NameCount = ComputeUnicodeNameLength(Prefix);

    /* Set up the initial entry data */
    PrefixTableEntry->NameLength = NameCount;
    PrefixTableEntry->Prefix = Prefix;
    RtlInitializeSplayLinks(&PrefixTableEntry->Links);

    /* Find the right spot where to insert this entry */
    PreviousEntry = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    CurrentEntry = PreviousEntry->NextPrefixTree;
    while (CurrentEntry->NameLength > NameCount)
    {
        /* Not a match, move to the next entry */
        PreviousEntry = CurrentEntry;
        CurrentEntry = CurrentEntry->NextPrefixTree;
    }

    /* Check if we did find a tree by now */
    if (CurrentEntry->NameLength != NameCount)
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

    /* FIXME */
	UNIMPLEMENTED;
	return FALSE;
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
                PrefixTableEntry->Links.LeftChild = &Entry->Links;
            }
            else
            {
                /* We were the right child, so make us as well */
                PrefixTableEntry->Links.RightChild = &Entry->Links;
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
