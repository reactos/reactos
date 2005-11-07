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

/*STATIC*/
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
     * - get name length (number of names)
     * - init splay links
     * - find a matching tree
     * - if !found, insert a new NTC_ROOT entry and return TRUE;
     * - if found, loop tree and compare strings:
     *   if equal, handle casematch/nomatch
     *   if greater or lesser equal, then add left/right childs accordingly
     * - splay the tree
     */
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
    PUNICODE_PREFIX_TABLE_ENTRY Entry;
    PUNICODE_PREFIX_TABLE_ENTRY CaseMatchEntry;

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
* @unimplemented
*/
VOID
NTAPI
RtlRemoveUnicodePrefix(PUNICODE_PREFIX_TABLE PrefixTable,
                       PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry)
{
	UNIMPLEMENTED;
}

/* EOF */
