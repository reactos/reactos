/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Prefix.c

Abstract:

    This module implements the prefix table utility.  The two structures
    used in a prefix table are the PREFIX_TABLE and PREFIX_TABLE_ENTRY.
    Each table has one prefix table and multiple prefix table entries
    corresponding to each prefix stored in the table.

    A prefix table is a list of prefix trees, where each tree contains
    the prefixes corresponding to a particular name length (i.e., all
    prefixes of length 1 are stored in one tree, prefixes of length 2
    are stored in another tree, and so forth).  A prefixes name length
    is the number of separate names that appear in the string, and not
    the number of characters in the string (e.g., Length("\alpha\beta") = 2).

    The elements of each tree are ordered lexicalgraphically (case blind)
    using a splay tree data structure.  If two or more prefixes are identical
    except for case then one of the corresponding table entries is actually
    in the tree, while the other entries are in a circular linked list joined
    with the tree member.

Author:

    Gary Kimura     [GaryKi]    3-Aug-1989

Environment:

    Pure utility routine

Revision History:

    08-Mar-1993    JulieB    Moved Upcase Macro to ntrtlp.h.

--*/

#include "ntrtlp.h"

//
//  Local procedures and types used only in this package
//

typedef enum _COMPARISON {
    IsLessThan,
    IsPrefix,
    IsEqual,
    IsGreaterThan
} COMPARISON;

CLONG
ComputeNameLength(
    IN PSTRING Name
    );

COMPARISON
CompareNamesCaseSensitive (
    IN PSTRING Prefix,
    IN PSTRING Name
    );

CLONG
ComputeUnicodeNameLength(
    IN PUNICODE_STRING Name
    );

COMPARISON
CompareUnicodeStrings (
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Name,
    IN ULONG CaseInsensitiveIndex
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,ComputeNameLength)
#pragma alloc_text(PAGE,CompareNamesCaseSensitive)
#pragma alloc_text(PAGE,PfxInitialize)
#pragma alloc_text(PAGE,PfxInsertPrefix)
#pragma alloc_text(PAGE,PfxRemovePrefix)
#pragma alloc_text(PAGE,PfxFindPrefix)
#pragma alloc_text(PAGE,ComputeUnicodeNameLength)
#pragma alloc_text(PAGE,CompareUnicodeStrings)
#pragma alloc_text(PAGE,RtlInitializeUnicodePrefix)
#pragma alloc_text(PAGE,RtlInsertUnicodePrefix)
#pragma alloc_text(PAGE,RtlRemoveUnicodePrefix)
#pragma alloc_text(PAGE,RtlFindUnicodePrefix)
#pragma alloc_text(PAGE,RtlNextUnicodePrefix)
#endif


//
//  The node type codes for the prefix data structures
//

#define RTL_NTC_PREFIX_TABLE             ((CSHORT)0x0200)
#define RTL_NTC_ROOT                     ((CSHORT)0x0201)
#define RTL_NTC_INTERNAL                 ((CSHORT)0x0202)


VOID
PfxInitialize (
    IN PPREFIX_TABLE PrefixTable
    )

/*++

Routine Description:

    This routine initializes a prefix table record to the empty state.

Arguments:

    PrefixTable - Supplies the prefix table being initialized

Return Value:

    None.

--*/

{
    RTL_PAGED_CODE();

    PrefixTable->NodeTypeCode = RTL_NTC_PREFIX_TABLE;

    PrefixTable->NameLength = 0;

    PrefixTable->NextPrefixTree = (PPREFIX_TABLE_ENTRY)PrefixTable;

    //
    //  return to our caller
    //

    return;
}


BOOLEAN
PfxInsertPrefix (
    IN PPREFIX_TABLE PrefixTable,
    IN PSTRING Prefix,
    IN PPREFIX_TABLE_ENTRY PrefixTableEntry
    )

/*++

Routine Description:

    This routine inserts a new prefix into the specified prefix table

Arguments:

    PrefixTable - Supplies the target prefix table

    Prefix - Supplies the string to be inserted in the prefix table

    PrefixTableEntry - Supplies the entry to use to insert the prefix

Return Value:

    BOOLEAN - TRUE if the Prefix is not already in the table, and FALSE
        otherwise

--*/

{
    ULONG PrefixNameLength;

    PPREFIX_TABLE_ENTRY PreviousTree;
    PPREFIX_TABLE_ENTRY CurrentTree;
    PPREFIX_TABLE_ENTRY NextTree;

    PPREFIX_TABLE_ENTRY Node;

    COMPARISON Comparison;

    RTL_PAGED_CODE();

    //
    //  Determine the name length of the input string
    //

    PrefixNameLength = ComputeNameLength(Prefix);

    //
    //  Setup parts of the prefix table entry that we will always need
    //

    PrefixTableEntry->NameLength = (CSHORT)PrefixNameLength;
    PrefixTableEntry->Prefix = Prefix;

    RtlInitializeSplayLinks(&PrefixTableEntry->Links);

    //
    //  find the corresponding tree, or find where the tree should go
    //

    PreviousTree = (PPREFIX_TABLE_ENTRY)PrefixTable;
    CurrentTree = PreviousTree->NextPrefixTree;

    while (CurrentTree->NameLength > (CSHORT)PrefixNameLength) {

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;

    }

    //
    //  If the name length of the current tree is not equal to the
    //  prefix name length then the tree does not exist and we need
    //  to make a new tree node.
    //

    if (CurrentTree->NameLength != (CSHORT)PrefixNameLength) {

        //
        //  Insert the new prefix entry to the list between
        //  previous and current tree
        //

        PreviousTree->NextPrefixTree = PrefixTableEntry;
        PrefixTableEntry->NextPrefixTree = CurrentTree;

        //
        //  And set the node type code
        //

        PrefixTableEntry->NodeTypeCode = RTL_NTC_ROOT;

        //
        //  And tell our caller everything worked fine
        //

        return TRUE;

    }

    //
    //  The tree does exist so now search the tree for our
    //  position in it.  We only exit the loop if we've inserted
    //  a new node, and node is left is left pointing to the
    //  tree position
    //

    Node = CurrentTree;

    while (TRUE) {

        //
        //  Compare the prefix in the tree with the prefix we want
        //  to insert
        //

        Comparison = CompareNamesCaseSensitive(Node->Prefix, Prefix);

        //
        //  If we do match case sensitive then we cannot add
        //  this prefix so we return false.  Note this is the
        //  only condition where we return false
        //

        if (Comparison == IsEqual) {

            return FALSE;
        }

        //
        //  If the tree prefix is greater than the new prefix then
        //  we go down the left subtree
        //

        if (Comparison == IsGreaterThan) {

            //
            //  We want to go down the left subtree, first check to see
            //  if we have a left subtree
            //

            if (RtlLeftChild(&Node->Links) == NULL) {

                //
                //  there isn't a left child so we insert ourselves as the
                //  new left child
                //

                PrefixTableEntry->NodeTypeCode = RTL_NTC_INTERNAL;
                PrefixTableEntry->NextPrefixTree = NULL;

                RtlInsertAsLeftChild(&Node->Links, &PrefixTableEntry->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a left child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlLeftChild(&Node->Links),
                                          PREFIX_TABLE_ENTRY,
                                          Links );

            }

        } else {

            //
            //  The tree prefix is either less than or a proper prefix
            //  of the new string.  We treat both cases a less than when
            //  we do insert.  So we want to go down the right subtree,
            //  first check to see if we have a right subtree
            //

            if (RtlRightChild(&Node->Links) == NULL) {

                //
                //  These isn't a right child so we insert ourselves as the
                //  new right child
                //

                PrefixTableEntry->NodeTypeCode = RTL_NTC_INTERNAL;
                PrefixTableEntry->NextPrefixTree = NULL;

                RtlInsertAsRightChild(&Node->Links, &PrefixTableEntry->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a right child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlRightChild(&Node->Links),
                                          PREFIX_TABLE_ENTRY,
                                          Links );
            }

        }

    }

    //
    //  Now that we've inserted the new node we can splay the tree.
    //  To do this we need to remember how we find this tree in the root
    //  tree list, set the root to be an internal, splay, the tree, and
    //  then setup the new root node.  Note: we cannot splay the prefix table
    //  entry because it might be a case match node so we only splay
    //  the Node variable, which for case match insertions is the
    //  internal node for the case match and for non-case match insertions
    //  the Node variable is the parent node.
    //

    //
    //  Save a pointer to the next tree, we already have the previous tree
    //

    NextTree = CurrentTree->NextPrefixTree;

    //
    //  Reset the current root to be an internal node
    //

    CurrentTree->NodeTypeCode = RTL_NTC_INTERNAL;
    CurrentTree->NextPrefixTree = NULL;

    //
    //  Splay the tree and get the root
    //

    Node = CONTAINING_RECORD(RtlSplay(&Node->Links), PREFIX_TABLE_ENTRY, Links);

    //
    //  Set the new root's node type code and make it part of the
    //  root tree list
    //

    Node->NodeTypeCode = RTL_NTC_ROOT;
    PreviousTree->NextPrefixTree = Node;
    Node->NextPrefixTree = NextTree;

    //
    //  tell our caller everything worked fine
    //

    return TRUE;
}


VOID
PfxRemovePrefix (
    IN PPREFIX_TABLE PrefixTable,
    IN PPREFIX_TABLE_ENTRY PrefixTableEntry
    )

/*++

Routine Description:

    This routine removes the indicated prefix table entry from
    the prefix table

Arguments:

    PrefixTable - Supplies the prefix table affected

    PrefixTableEntry - Supplies the prefix entry to remove

Return Value:

    None.

--*/

{
    PRTL_SPLAY_LINKS Links;

    PPREFIX_TABLE_ENTRY Root;
    PPREFIX_TABLE_ENTRY NewRoot;

    PPREFIX_TABLE_ENTRY PreviousTree;

    RTL_PAGED_CODE();

    //
    //  case on the type of node that we are trying to delete
    //

    switch (PrefixTableEntry->NodeTypeCode) {

    case RTL_NTC_INTERNAL:
    case RTL_NTC_ROOT:

        //
        //  The node is internal or root node so we need to delete it from
        //  the tree, but first find the root of the tree
        //

        Links = &PrefixTableEntry->Links;

        while (!RtlIsRoot(Links)) {

            Links = RtlParent(Links);
        }

        Root = CONTAINING_RECORD( Links, PREFIX_TABLE_ENTRY, Links );

        //
        //  Now delete the node
        //

        Links = RtlDelete(&PrefixTableEntry->Links);

        //
        //  Now see if the tree is deleted
        //

        if (Links == NULL) {

            //
            //  The tree is now empty so remove this tree from
            //  the tree list, by first finding the previous tree that
            //  references us
            //

            PreviousTree = Root->NextPrefixTree;

            while ( PreviousTree->NextPrefixTree != Root ) {

                PreviousTree = PreviousTree->NextPrefixTree;
            }

            //
            //  We've located the previous tree so now just have it
            //  point around the deleted node
            //

            PreviousTree->NextPrefixTree = Root->NextPrefixTree;

            //
            //  and return the our caller
            //

            return;
        }

        //
        //  The tree is not deleted but see if we changed roots
        //

        if (&Root->Links != Links) {

            //
            //  Get a pointer to the new root
            //

            NewRoot = CONTAINING_RECORD(Links, PREFIX_TABLE_ENTRY, Links);

            //
            //  We changed root so we better need to make the new
            //  root part of the prefix data structure, by
            //  first finding the previous tree that
            //  references us
            //

            PreviousTree = Root->NextPrefixTree;

            while ( PreviousTree->NextPrefixTree != Root ) {

                PreviousTree = PreviousTree->NextPrefixTree;
            }

            //
            //  Set the new root
            //

            NewRoot->NodeTypeCode = RTL_NTC_ROOT;

            PreviousTree->NextPrefixTree = NewRoot;
            NewRoot->NextPrefixTree = Root->NextPrefixTree;

            //
            //  Set the old root to be an internal node
            //

            Root->NodeTypeCode = RTL_NTC_INTERNAL;

            Root->NextPrefixTree = NULL;

            //
            //  And return to our caller
            //

            return;
        }

        //
        //  We didn't change roots so everything is fine and we can
        //  simply return to our caller
        //

        return;

    default:

        //
        //  If we get here then there was an error and the node type
        //  code is unknown
        //

        return;
    }
}


PPREFIX_TABLE_ENTRY
PfxFindPrefix (
    IN PPREFIX_TABLE PrefixTable,
    IN PSTRING FullName
    )

/*++

Routine Description:

    This routine finds if a full name has a prefix in a prefix table.
    It returns a pointer to the largest proper prefix found if one exists.

Arguments:

    PrefixTable - Supplies the prefix table to search

    FullString - Supplies the name to search for

Return Value:

    PPREFIX_TABLE_ENTRY - a pointer to the longest prefix found if one
        exists, and NULL otherwise

--*/

{
    CLONG NameLength;

    PPREFIX_TABLE_ENTRY PreviousTree;
    PPREFIX_TABLE_ENTRY CurrentTree;
    PPREFIX_TABLE_ENTRY NextTree;

    PRTL_SPLAY_LINKS Links;

    PPREFIX_TABLE_ENTRY Node;

    COMPARISON Comparison;

    RTL_PAGED_CODE();

    //
    //  Determine the name length of the input string
    //

    NameLength = ComputeNameLength(FullName);

    //
    //  Locate the first tree that can contain a prefix
    //

    PreviousTree = (PPREFIX_TABLE_ENTRY)PrefixTable;
    CurrentTree = PreviousTree->NextPrefixTree;

    while (CurrentTree->NameLength > (CSHORT)NameLength) {

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;
    }

    //
    //  Now search for a prefix until we find one or until we exhaust
    //  the prefix trees
    //

    while (CurrentTree->NameLength > 0) {

        Links = &CurrentTree->Links;

        while (Links != NULL) {

            Node = CONTAINING_RECORD(Links, PREFIX_TABLE_ENTRY, Links);

            //
            //  Compare the prefix in the tree with the full name
            //

            Comparison = CompareNamesCaseSensitive(Node->Prefix, FullName);

            //
            //  See if they don't match
            //

            if (Comparison == IsGreaterThan) {

                //
                //  The prefix is greater than the full name
                //  so we go down the left child
                //

                Links = RtlLeftChild(Links);

                //
                //  And continue searching down this tree
                //

            } else if (Comparison == IsLessThan) {

                //
                //  The prefix is less than the full name
                //  so we go down the right child
                //

                Links = RtlRightChild(Links);

                //
                //  And continue searching down this tree
                //

            } else {

                //
                //  We found it.
                //
                //  Now that we've located the node we can splay the tree.
                //  To do this we need to remember how we find this tree in the root
                //  tree list, set the root to be an internal, splay, the tree, and
                //  then setup the new root node.
                //

                if (Node->NodeTypeCode == RTL_NTC_INTERNAL) {

                    //DbgPrint("PrefixTable  = %08lx\n", PrefixTable);
                    //DbgPrint("Node         = %08lx\n", Node);
                    //DbgPrint("CurrentTree  = %08lx\n", CurrentTree);
                    //DbgPrint("PreviousTree = %08lx\n", PreviousTree);
                    //DbgBreakPoint();

                    //
                    //  Save a pointer to the next tree, we already have the previous tree
                    //

                    NextTree = CurrentTree->NextPrefixTree;

                    //
                    //  Reset the current root to be an internal node
                    //

                    CurrentTree->NodeTypeCode = RTL_NTC_INTERNAL;
                    CurrentTree->NextPrefixTree = NULL;

                    //
                    //  Splay the tree and get the root
                    //

                    Node = CONTAINING_RECORD(RtlSplay(&Node->Links), PREFIX_TABLE_ENTRY, Links);

                    //
                    //  Set the new root's node type code and make it part of the
                    //  root tree list
                    //

                    Node->NodeTypeCode = RTL_NTC_ROOT;
                    PreviousTree->NextPrefixTree = Node;
                    Node->NextPrefixTree = NextTree;
                }

                return Node;
            }
        }

        //
        //  This tree is done so now find the next tree
        //

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;
    }

    //
    //  We sesarched everywhere and didn't find a prefix so tell the
    //  caller none was found
    //

    return NULL;
}


CLONG
ComputeNameLength(
    IN PSTRING Name
    )

/*++

Routine Description:

    This routine counts the number of names appearing in the input string.
    It does this by simply counting the number of backslashes in the string.
    To handle ill-formed names (i.e., names that do not contain a backslash)
    this routine really returns the number of backslashes plus 1.

Arguments:

    Name - Supplies the input name to examine

Returns Value:

    CLONG - the number of names in the input string

--*/

{
    ULONG NameLength;
    ULONG i;
    ULONG Count;

    extern PUSHORT NlsLeadByteInfo;  // Lead byte info. for ACP ( nlsxlat.c )
    extern BOOLEAN NlsMbCodePageTag;

    RTL_PAGED_CODE();

    //
    //  Save the name length, this should make the compiler be able to
    //  optimize not having to reload the length each time
    //

    NameLength = Name->Length - 1;

    //
    //  Now loop through the input string counting back slashes
    //

    if (NlsMbCodePageTag) {

        //
        // ComputeNameLength() skip DBCS character when counting '\'
        //

        for (i = 0, Count = 1; i < NameLength; ) {

            if (NlsLeadByteInfo[(UCHAR)Name->Buffer[i]]) {

                i += 2;

            } else {

                if (Name->Buffer[i] == '\\') {

                    Count += 1;
                }

                i += 1;
            }
        }

    } else {

        for (i = 0, Count = 1; i < NameLength; i += 1) {

            //
            //  check for a back slash
            //

            if (Name->Buffer[i] == '\\') {

                Count += 1;
            }
        }
    }

    //
    //  return the number of back slashes we found
    //

    //DbgPrint("ComputeNameLength(%s) = %x\n", Name->Buffer, Count);

    return Count;
}


COMPARISON
CompareNamesCaseSensitive (
    IN PSTRING Prefix,
    IN PSTRING Name
    )

/*++

Routine Description:

    This routine takes a prefix string and a full name string and determines
    if the prefix string is a proper prefix of the name string (case sensitive)

Arguments:

    Prefix - Supplies the input prefix string

    Name - Supplies the full name input string

Return Value:

    COMPARISON - returns

        IsLessThan    if Prefix < Name lexicalgraphically,
        IsPrefix      if Prefix is a proper prefix of Name
        IsEqual       if Prefix is equal to Name, and
        IsGreaterThan if Prefix > Name lexicalgraphically

--*/

{
    ULONG PrefixLength;
    ULONG NameLength;
    ULONG MinLength;
    ULONG i;

    UCHAR PrefixChar;
    UCHAR NameChar;

    extern PUSHORT NlsLeadByteInfo;  // Lead byte info. for ACP ( nlsxlat.c )
    extern BOOLEAN NlsMbCodePageTag;

    RTL_PAGED_CODE();

    //DbgPrint("CompareNamesCaseSensitive(\"%s\", \"%s\") = ", Prefix->Buffer, Name->Buffer);

    //
    //  Save the length of the prefix and name string, this should allow
    //  the compiler to not need to reload the length through a pointer every
    //  time we need their values
    //

    PrefixLength = Prefix->Length;
    NameLength = Name->Length;

    //
    //  Special case the situation where the prefix string is simply "\" and
    //  the name starts with an "\"
    //

    if ((Prefix->Length == 1) && (Prefix->Buffer[0] == '\\') &&
        (Name->Length > 1) && (Name->Buffer[0] == '\\')) {
        //DbgPrint("IsPrefix\n");
        return IsPrefix;
    }

    //
    //  Figure out the minimum of the two lengths
    //

    MinLength = (PrefixLength < NameLength ? PrefixLength : NameLength);

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty, less than, and greater than
    //

    i = (ULONG) RtlCompareMemory( &Prefix->Buffer[0], &Name->Buffer[0], MinLength );

    if (i < MinLength) {

        UCHAR c;

        //
        //  Get both characters to examine and keep their case
        //

        PrefixChar = ((c = Prefix->Buffer[i]) == '\\' ? (CHAR)0 : c);
        NameChar   = ((c = Name->Buffer[i])   == '\\' ? (CHAR)0 : c);

        //
        //  Unfortunately life is not so easy in DBCS land.
        //

        if (NlsMbCodePageTag) {

            //
            // CompareNamesCaseSensitive(): check backslash in trailing bytes
            //

            if (Prefix->Buffer[i] == '\\') {

                ULONG j;
                extern PUSHORT   NlsLeadByteInfo;  // Lead byte info. for ACP ( nlsxlat.c )

                for (j = 0; j < i;) {

                    j += NlsLeadByteInfo[(UCHAR)Prefix->Buffer[j]] ? 2 : 1;
                }

                if (j != i) {

                    PrefixChar = '\\';
                    //DbgPrint("RTL:CompareNamesCaseSensitive encountered a fake backslash!\n");
                }
            }

            if (Name->Buffer[i] == '\\') {

                ULONG j;
                extern PUSHORT   NlsLeadByteInfo;  // Lead byte info. for ACP ( nlsxlat.c )

                for (j = 0; j < i;) {

                    j += NlsLeadByteInfo[(UCHAR)Name->Buffer[j]] ? 2 : 1;
                }

                if (j != i) {

                    NameChar = '\\';
                    //DbgPrint("RTL:CompareNamesCaseSensitive encountered a fake backslash!\n");
                }
            }
        }

        //
        //  Now compare the characters
        //

        if (PrefixChar < NameChar) {

            return IsLessThan;

        } else if (PrefixChar > NameChar) {

            return IsGreaterThan;
        }
    }

    //
    //  They match upto the minimum length so now figure out the largest string
    //  and see if one is a proper prefix of the other
    //

    if (PrefixLength < NameLength) {

        //
        //  The prefix string is shorter so if it is a proper prefix we
        //  return prefix otherwise we return less than (e.g., "\a" < "\ab")
        //

        if (Name->Buffer[PrefixLength] == '\\') {

            return IsPrefix;

        } else {

            return IsLessThan;
        }

    } else if (PrefixLength > NameLength) {

        //
        //  The Prefix string is longer so we say that the prefix is
        //  greater than the name (e.g., "\ab" > "\a")
        //

        return IsGreaterThan;

    } else {

        //
        //  They lengths are equal so the strings are equal
        //

        return IsEqual;
    }
}


//
//  The node type codes for the prefix data structures
//

#define RTL_NTC_UNICODE_PREFIX_TABLE     ((CSHORT)0x0800)
#define RTL_NTC_UNICODE_ROOT             ((CSHORT)0x0801)
#define RTL_NTC_UNICODE_INTERNAL         ((CSHORT)0x0802)
#define RTL_NTC_UNICODE_CASE_MATCH       ((CSHORT)0x0803)


VOID
RtlInitializeUnicodePrefix (
    IN PUNICODE_PREFIX_TABLE PrefixTable
    )

/*++

Routine Description:

    This routine initializes a unicode prefix table record to the empty state.

Arguments:

    PrefixTable - Supplies the prefix table being initialized

Return Value:

    None.

--*/

{
    RTL_PAGED_CODE();

    PrefixTable->NodeTypeCode = RTL_NTC_UNICODE_PREFIX_TABLE;
    PrefixTable->NameLength = 0;
    PrefixTable->NextPrefixTree = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    PrefixTable->LastNextEntry = NULL;

    //
    //  return to our caller
    //

    return;
}


BOOLEAN
RtlInsertUnicodePrefix (
    IN PUNICODE_PREFIX_TABLE PrefixTable,
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    )

/*++

Routine Description:

    This routine inserts a new unicode prefix into the specified prefix table

Arguments:

    PrefixTable - Supplies the target prefix table

    Prefix - Supplies the string to be inserted in the prefix table

    PrefixTableEntry - Supplies the entry to use to insert the prefix

Return Value:

    BOOLEAN - TRUE if the Prefix is not already in the table, and FALSE
        otherwise

--*/

{
    ULONG PrefixNameLength;

    PUNICODE_PREFIX_TABLE_ENTRY PreviousTree;
    PUNICODE_PREFIX_TABLE_ENTRY CurrentTree;
    PUNICODE_PREFIX_TABLE_ENTRY NextTree;

    PUNICODE_PREFIX_TABLE_ENTRY Node;

    COMPARISON Comparison;

    RTL_PAGED_CODE();

    //
    //  Determine the name length of the input string
    //

    PrefixNameLength = ComputeUnicodeNameLength(Prefix);

    //
    //  Setup parts of the prefix table entry that we will always need
    //

    PrefixTableEntry->NameLength = (CSHORT)PrefixNameLength;
    PrefixTableEntry->Prefix = Prefix;

    RtlInitializeSplayLinks(&PrefixTableEntry->Links);

    //
    //  find the corresponding tree, or find where the tree should go
    //

    PreviousTree = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    CurrentTree = PreviousTree->NextPrefixTree;

    while (CurrentTree->NameLength > (CSHORT)PrefixNameLength) {

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;
    }

    //
    //  If the name length of the current tree is not equal to the
    //  prefix name length then the tree does not exist and we need
    //  to make a new tree node.
    //

    if (CurrentTree->NameLength != (CSHORT)PrefixNameLength) {

        //
        //  Insert the new prefix entry to the list between
        //  previous and current tree
        //

        PreviousTree->NextPrefixTree = PrefixTableEntry;
        PrefixTableEntry->NextPrefixTree = CurrentTree;

        //
        //  And set the node type code, case match for the root tree node
        //

        PrefixTableEntry->NodeTypeCode = RTL_NTC_UNICODE_ROOT;
        PrefixTableEntry->CaseMatch = PrefixTableEntry;

        //
        //  And tell our caller everything worked fine
        //

        return TRUE;
    }

    //
    //  The tree does exist so now search the tree for our
    //  position in it.  We only exit the loop if we've inserted
    //  a new node, and node is left is left pointing to the
    //  tree position
    //

    Node = CurrentTree;

    while (TRUE) {

        //
        //  Compare the prefix in the tree with the prefix we want
        //  to insert.  Do the compare case blind
        //

        Comparison = CompareUnicodeStrings(Node->Prefix, Prefix, 0);

        //
        //  If they are equal then this node gets added as a case
        //  match, provided it doesn't case sensitive match anyone
        //

        if (Comparison == IsEqual) {

            PUNICODE_PREFIX_TABLE_ENTRY Next;

            //
            //  Loop through the case match list checking to see if we
            //  match case sensitive with anyone.  Get the first node
            //

            Next = Node;

            //
            //  And loop checking each node until we're back to where
            //  we started
            //

            do {

                //
                //  If we do match case sensitive then we cannot add
                //  this prefix so we return false.  Note this is the
                //  only condition where we return false
                //

                if (CompareUnicodeStrings(Next->Prefix, Prefix, MAXULONG) == IsEqual) {

                    return FALSE;
                }

                //
                //  Get the next node in the case match list
                //

                Next = Next->CaseMatch;

                //
                //  And continue looping until we're back where we started
                //

            } while ( Next != Node );

            //
            //  We've searched the case match and didn't find an exact match
            //  so we can insert this node in the case match list
            //

            PrefixTableEntry->NodeTypeCode = RTL_NTC_UNICODE_CASE_MATCH;
            PrefixTableEntry->NextPrefixTree = NULL;

            PrefixTableEntry->CaseMatch = Node->CaseMatch;
            Node->CaseMatch = PrefixTableEntry;

            //
            //  And exit out of the while loop
            //

            break;
        }

        //
        //  If the tree prefix is greater than the new prefix then
        //  we go down the left subtree
        //

        if (Comparison == IsGreaterThan) {

            //
            //  We want to go down the left subtree, first check to see
            //  if we have a left subtree
            //

            if (RtlLeftChild(&Node->Links) == NULL) {

                //
                //  there isn't a left child so we insert ourselves as the
                //  new left child
                //

                PrefixTableEntry->NodeTypeCode = RTL_NTC_UNICODE_INTERNAL;
                PrefixTableEntry->NextPrefixTree = NULL;
                PrefixTableEntry->CaseMatch = PrefixTableEntry;

                RtlInsertAsLeftChild(&Node->Links, &PrefixTableEntry->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a left child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlLeftChild(&Node->Links),
                                          UNICODE_PREFIX_TABLE_ENTRY,
                                          Links );
            }

        } else {

            //
            //  The tree prefix is either less than or a proper prefix
            //  of the new string.  We treat both cases a less than when
            //  we do insert.  So we want to go down the right subtree,
            //  first check to see if we have a right subtree
            //

            if (RtlRightChild(&Node->Links) == NULL) {

                //
                //  These isn't a right child so we insert ourselves as the
                //  new right child
                //

                PrefixTableEntry->NodeTypeCode = RTL_NTC_UNICODE_INTERNAL;
                PrefixTableEntry->NextPrefixTree = NULL;
                PrefixTableEntry->CaseMatch = PrefixTableEntry;

                RtlInsertAsRightChild(&Node->Links, &PrefixTableEntry->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a right child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlRightChild(&Node->Links),
                                          UNICODE_PREFIX_TABLE_ENTRY,
                                          Links );
            }
        }
    }

    //
    //  Now that we've inserted the new node we can splay the tree.
    //  To do this we need to remember how we find this tree in the root
    //  tree list, set the root to be an internal, splay, the tree, and
    //  then setup the new root node.  Note: we cannot splay the prefix table
    //  entry because it might be a case match node so we only splay
    //  the Node variable, which for case match insertions is the
    //  internal node for the case match and for non-case match insertions
    //  the Node variable is the parent node.
    //

    //
    //  Save a pointer to the next tree, we already have the previous tree
    //

    NextTree = CurrentTree->NextPrefixTree;

    //
    //  Reset the current root to be an internal node
    //

    CurrentTree->NodeTypeCode = RTL_NTC_UNICODE_INTERNAL;
    CurrentTree->NextPrefixTree = NULL;

    //
    //  Splay the tree and get the root
    //

    Node = CONTAINING_RECORD(RtlSplay(&Node->Links), UNICODE_PREFIX_TABLE_ENTRY, Links);

    //
    //  Set the new root's node type code and make it part of the
    //  root tree list
    //

    Node->NodeTypeCode = RTL_NTC_UNICODE_ROOT;
    PreviousTree->NextPrefixTree = Node;
    Node->NextPrefixTree = NextTree;

    //
    //  tell our caller everything worked fine
    //

    return TRUE;
}


VOID
RtlRemoveUnicodePrefix (
    IN PUNICODE_PREFIX_TABLE PrefixTable,
    IN PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    )

/*++

Routine Description:

    This routine removes the indicated prefix table entry from
    the prefix table

Arguments:

    PrefixTable - Supplies the prefix table affected

    PrefixTableEntry - Supplies the prefix entry to remove

Return Value:

    None.

--*/

{
    PUNICODE_PREFIX_TABLE_ENTRY PreviousCaseMatch;

    PRTL_SPLAY_LINKS Links;

    PUNICODE_PREFIX_TABLE_ENTRY Root;
    PUNICODE_PREFIX_TABLE_ENTRY NewRoot;

    PUNICODE_PREFIX_TABLE_ENTRY PreviousTree;

    RTL_PAGED_CODE();

    //
    //  Wipe out the next last entry field of the prefix table
    //

    PrefixTable->LastNextEntry = NULL;

    //
    //  case on the type of node that we are trying to delete
    //

    switch (PrefixTableEntry->NodeTypeCode) {

    case RTL_NTC_UNICODE_CASE_MATCH:

        //
        //  The prefix entry is a case match record so
        //  we only need to remove it from the case match list.
        //  Locate the previous PrefixTableEntry that reference this
        //  case match record
        //

        PreviousCaseMatch = PrefixTableEntry->CaseMatch;

        while ( PreviousCaseMatch->CaseMatch != PrefixTableEntry ) {

             PreviousCaseMatch = PreviousCaseMatch->CaseMatch;
        }

        //
        //  Now that we have the previous record just have it point
        //  around the case match that is being deleted
        //

        PreviousCaseMatch->CaseMatch = PrefixTableEntry->CaseMatch;

        //
        //  And return to our caller
        //

        return;

    case RTL_NTC_UNICODE_INTERNAL:
    case RTL_NTC_UNICODE_ROOT:

        //
        //  The prefix entry is an internal/root node so check to see if it
        //  has any case match nodes with it
        //

        if (PrefixTableEntry->CaseMatch != PrefixTableEntry) {

            //
            //  There is at least one case match that goes with this
            //  node, so we need to make the next case match the
            //  new node and remove this node.
            //  Locate the previous prefix table entry that references this
            //  case match record
            //

            PreviousCaseMatch = PrefixTableEntry->CaseMatch;

            while ( PreviousCaseMatch->CaseMatch != PrefixTableEntry ) {

                PreviousCaseMatch = PreviousCaseMatch->CaseMatch;
            }

            //
            //  Now that we have the previous record just have it point
            //  around the node being deleted
            //

            PreviousCaseMatch->CaseMatch = PrefixTableEntry->CaseMatch;

            //
            //  Now make the previous case match in the new node
            //

            PreviousCaseMatch->NodeTypeCode = PrefixTableEntry->NodeTypeCode;
            PreviousCaseMatch->NextPrefixTree = PrefixTableEntry->NextPrefixTree;
            PreviousCaseMatch->Links = PrefixTableEntry->Links;

            //
            //  Now take care of the back pointers to this new internal
            //  node in the splay tree, first do the parent's pointer to us.
            //

            if (RtlIsRoot(&PrefixTableEntry->Links)) {

                //
                //  This is the root so make this new node the root
                //

                PreviousCaseMatch->Links.Parent = &PreviousCaseMatch->Links;

                //
                //  Fix up the root tree list, by first finding the previous
                //  pointer to us

                PreviousTree = PrefixTableEntry->NextPrefixTree;

                while ( PreviousTree->NextPrefixTree != PrefixTableEntry ) {

                    PreviousTree = PreviousTree->NextPrefixTree;
                }

                //
                //  We've located the previous tree so now have the previous
                //  tree point to our new root
                //

                PreviousTree->NextPrefixTree = PreviousCaseMatch;

            } else if (RtlIsLeftChild(&PrefixTableEntry->Links)) {

                //
                //  The node was the left child so make the new node the
                //  left child
                //

                RtlParent(&PrefixTableEntry->Links)->LeftChild = &PreviousCaseMatch->Links;

            } else {

                //
                //  The node was the right child so make the new node the
                //  right child
                //

                RtlParent(&PrefixTableEntry->Links)->RightChild = &PreviousCaseMatch->Links;
            }

            //
            //  Now update the parent pointer for our new children
            //

            if (RtlLeftChild(&PreviousCaseMatch->Links) != NULL) {

                RtlLeftChild(&PreviousCaseMatch->Links)->Parent = &PreviousCaseMatch->Links;
            }

            if (RtlRightChild(&PreviousCaseMatch->Links) != NULL) {

                RtlRightChild(&PreviousCaseMatch->Links)->Parent = &PreviousCaseMatch->Links;
            }

            //
            //  And return to our caller
            //

            return;
        }

        //
        //  The node is internal or root node and does not have any case match
        //  nodes so we need to delete it from the tree, but first find
        //  the root of the tree
        //

        Links = &PrefixTableEntry->Links;

        while (!RtlIsRoot(Links)) {

            Links = RtlParent(Links);
        }

        Root = CONTAINING_RECORD( Links, UNICODE_PREFIX_TABLE_ENTRY, Links );

        //
        //  Now delete the node
        //

        Links = RtlDelete(&PrefixTableEntry->Links);

        //
        //  Now see if the tree is deleted
        //

        if (Links == NULL) {

            //
            //  The tree is now empty so remove this tree from
            //  the tree list, by first finding the previous tree that
            //  references us
            //

            PreviousTree = Root->NextPrefixTree;

            while ( PreviousTree->NextPrefixTree != Root ) {

                PreviousTree = PreviousTree->NextPrefixTree;
            }

            //
            //  We've located the previous tree so now just have it
            //  point around the deleted node
            //

            PreviousTree->NextPrefixTree = Root->NextPrefixTree;

            //
            //  and return the our caller
            //

            return;
        }

        //
        //  The tree is not deleted but see if we changed roots
        //

        if (&Root->Links != Links) {

            //
            //  Get a pointer to the new root
            //

            NewRoot = CONTAINING_RECORD(Links, UNICODE_PREFIX_TABLE_ENTRY, Links);

            //
            //  We changed root so we better need to make the new
            //  root part of the prefix data structure, by
            //  first finding the previous tree that
            //  references us
            //

            PreviousTree = Root->NextPrefixTree;

            while ( PreviousTree->NextPrefixTree != Root ) {

                PreviousTree = PreviousTree->NextPrefixTree;
            }

            //
            //  Set the new root
            //

            NewRoot->NodeTypeCode = RTL_NTC_UNICODE_ROOT;

            PreviousTree->NextPrefixTree = NewRoot;
            NewRoot->NextPrefixTree = Root->NextPrefixTree;

            //
            //  Set the old root to be an internal node
            //

            Root->NodeTypeCode = RTL_NTC_UNICODE_INTERNAL;

            Root->NextPrefixTree = NULL;

            //
            //  And return to our caller
            //

            return;
        }

        //
        //  We didn't change roots so everything is fine and we can
        //  simply return to our caller
        //

        return;

    default:

        //
        //  If we get here then there was an error and the node type
        //  code is unknown
        //

        return;
    }
}


PUNICODE_PREFIX_TABLE_ENTRY
RtlFindUnicodePrefix (
    IN PUNICODE_PREFIX_TABLE PrefixTable,
    IN PUNICODE_STRING FullName,
    IN ULONG CaseInsensitiveIndex
    )

/*++

Routine Description:

    This routine finds if a full name has a prefix in a prefix table.
    It returns a pointer to the largest proper prefix found if one exists.

Arguments:

    PrefixTable - Supplies the prefix table to search

    FullString - Supplies the name to search for

    CaseInsensitiveIndex - Indicates the wchar index at which to do a case
        insensitive search.  All characters before the index are searched
        case sensitive and all characters at and after the index are searched
        insensitive.

Return Value:

    PPREFIX_TABLE_ENTRY - a pointer to the longest prefix found if one
        exists, and NULL otherwise

--*/

{
    CLONG NameLength;

    PUNICODE_PREFIX_TABLE_ENTRY PreviousTree;
    PUNICODE_PREFIX_TABLE_ENTRY CurrentTree;
    PUNICODE_PREFIX_TABLE_ENTRY NextTree;

    PRTL_SPLAY_LINKS Links;

    PUNICODE_PREFIX_TABLE_ENTRY Node;
    PUNICODE_PREFIX_TABLE_ENTRY Next;

    COMPARISON Comparison;

    RTL_PAGED_CODE();

    //
    //  Determine the name length of the input string
    //

    NameLength = ComputeUnicodeNameLength(FullName);

    //
    //  Locate the first tree that can contain a prefix
    //

    PreviousTree = (PUNICODE_PREFIX_TABLE_ENTRY)PrefixTable;
    CurrentTree = PreviousTree->NextPrefixTree;

    while (CurrentTree->NameLength > (CSHORT)NameLength) {

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;
    }

    //
    //  Now search for a prefix until we find one or until we exhaust
    //  the prefix trees
    //

    while (CurrentTree->NameLength > 0) {

        Links = &CurrentTree->Links;

        while (Links != NULL) {

            Node = CONTAINING_RECORD(Links, UNICODE_PREFIX_TABLE_ENTRY, Links);

            //
            //  Compare the prefix in the tree with the full name, do the
            //  compare case blind
            //

            Comparison = CompareUnicodeStrings(Node->Prefix, FullName, 0);

            //
            //  See if they don't match
            //

            if (Comparison == IsGreaterThan) {

                //
                //  The prefix is greater than the full name
                //  so we go down the left child
                //

                Links = RtlLeftChild(Links);

                //
                //  And continue searching down this tree
                //

            } else if (Comparison == IsLessThan) {

                //
                //  The prefix is less than the full name
                //  so we go down the right child
                //

                Links = RtlRightChild(Links);

                //
                //  And continue searching down this tree
                //

            } else {

                //
                //  We have either a prefix or a match either way
                //  we need to check if we should do case sensitive
                //  seearches
                //

                if (CaseInsensitiveIndex == 0) {

                    //
                    //  The caller wants case insensitive so we'll
                    //  return the first one we found
                    //
                    //  Now that we've located the node we can splay the tree.
                    //  To do this we need to remember how we find this tree in the root
                    //  tree list, set the root to be an internal, splay, the tree, and
                    //  then setup the new root node.
                    //

                    if (Node->NodeTypeCode == RTL_NTC_UNICODE_INTERNAL) {

                        //DbgPrint("PrefixTable  = %08lx\n", PrefixTable);
                        //DbgPrint("Node         = %08lx\n", Node);
                        //DbgPrint("CurrentTree  = %08lx\n", CurrentTree);
                        //DbgPrint("PreviousTree = %08lx\n", PreviousTree);
                        //DbgBreakPoint();

                        //
                        //  Save a pointer to the next tree, we already have the previous tree
                        //

                        NextTree = CurrentTree->NextPrefixTree;

                        //
                        //  Reset the current root to be an internal node
                        //

                        CurrentTree->NodeTypeCode = RTL_NTC_UNICODE_INTERNAL;
                        CurrentTree->NextPrefixTree = NULL;

                        //
                        //  Splay the tree and get the root
                        //

                        Node = CONTAINING_RECORD(RtlSplay(&Node->Links), UNICODE_PREFIX_TABLE_ENTRY, Links);

                        //
                        //  Set the new root's node type code and make it part of the
                        //  root tree list
                        //

                        Node->NodeTypeCode = RTL_NTC_UNICODE_ROOT;
                        PreviousTree->NextPrefixTree = Node;
                        Node->NextPrefixTree = NextTree;
                    }

                    //
                    //  Now return the root to our caller
                    //

                    return Node;
                }

                //
                //  The caller wants an exact match so search the case match
                //  until we find a complete match.  Get the first node
                //

                Next = Node;

                //
                //  Loop through the case match list checking to see if we
                //  match case sensitive with anyone.
                //

                do {

                    //
                    //  If we do match case sensitive then we found one
                    //  and we return it to our caller
                    //

                    Comparison = CompareUnicodeStrings( Next->Prefix,
                                                        FullName,
                                                        CaseInsensitiveIndex );

                    if ((Comparison == IsEqual) || (Comparison == IsPrefix)) {

                        //
                        //  We found a good one, so return it to our caller
                        //

                        return Next;
                    }

                    //
                    //  Get the next case match record
                    //

                    Next = Next->CaseMatch;

                    //
                    //  And continue the loop until we reach the original
                    //  node again
                    //

                } while ( Next != Node );

                //
                //  We found a case blind prefix but the caller wants
                //  case sensitive and we weren't able to find one of those
                //  so we need to go on to the next tree, by breaking out
                //  of the inner while-loop
                //

                break;
            }
        }

        //
        //  This tree is done so now find the next tree
        //

        PreviousTree = CurrentTree;
        CurrentTree = CurrentTree->NextPrefixTree;
    }

    //
    //  We sesarched everywhere and didn't find a prefix so tell the
    //  caller none was found
    //

    return NULL;
}


PUNICODE_PREFIX_TABLE_ENTRY
RtlNextUnicodePrefix (
    IN PUNICODE_PREFIX_TABLE PrefixTable,
    IN BOOLEAN Restart
    )

/*++

Routine Description:

    This routine returns the next prefix entry stored in the prefix table

Arguments:

    PrefixTable - Supplies the prefix table to enumerate

    Restart - Indicates if the enumeration should start over

Return Value:

    PPREFIX_TABLE_ENTRY - A pointer to the next prefix table entry if
        one exists otherwise NULL

--*/

{
    PUNICODE_PREFIX_TABLE_ENTRY Node;

    PRTL_SPLAY_LINKS Links;

    RTL_PAGED_CODE();

    //
    //  See if we are restarting the sequence
    //

    if (Restart || (PrefixTable->LastNextEntry == NULL)) {

        //
        //  we are restarting the sequence so locate the first entry
        //  in the first tree
        //

        Node = PrefixTable->NextPrefixTree;

        //
        //  Make sure we've pointing at a prefix tree
        //

        if (Node->NodeTypeCode == RTL_NTC_UNICODE_PREFIX_TABLE) {

            //
            //  No we aren't so the table must be empty
            //

            return NULL;
        }

        //
        //  Find the first node in the tree
        //

        Links = &Node->Links;

        while (RtlLeftChild(Links) != NULL) {

            Links = RtlLeftChild(Links);
        }

        //
        //  Set it as our the node we're returning
        //

        Node = CONTAINING_RECORD( Links, UNICODE_PREFIX_TABLE_ENTRY, Links);

    } else if (PrefixTable->LastNextEntry->CaseMatch->NodeTypeCode == RTL_NTC_UNICODE_CASE_MATCH) {

        //
        //  The last node has a case match that we should be returning
        //  this time around
        //

        Node = PrefixTable->LastNextEntry->CaseMatch;

    } else {

        //
        //  Move over the last node returned by the case match link, this
        //  will enable us to finish off the last case match node if there
        //  was one, and go to the next internal/root node. If this node
        //  does not have a case match then we simply circle back to ourselves
        //

        Node = PrefixTable->LastNextEntry->CaseMatch;

        //
        //  Find the successor for the last node we returned
        //

        Links = RtlRealSuccessor(&Node->Links);

        //
        //  If links is null then we've exhausted this tree and need to
        //  the the next tree to use
        //

        if (Links == NULL) {

            Links = &PrefixTable->LastNextEntry->Links;

            while (!RtlIsRoot(Links)) {

                Links = RtlParent(Links);
            }

            Node = CONTAINING_RECORD(Links, UNICODE_PREFIX_TABLE_ENTRY, Links);

            //
            //  Now we've found the root see if there is another
            //  tree to enumerate
            //

            Node = Node->NextPrefixTree;

            if (Node->NameLength <= 0) {

                //
                //  We've run out of tree so tell our caller there
                //  are no more
                //

                return NULL;
            }

            //
            //  We have another tree to go down
            //

            Links = &Node->Links;

            while (RtlLeftChild(Links) != NULL) {

                Links = RtlLeftChild(Links);
            }
        }

        //
        //  Set it as our the node we're returning
        //

        Node = CONTAINING_RECORD( Links, UNICODE_PREFIX_TABLE_ENTRY, Links);
    }

    //
    //  Save node as the last next entry
    //

    PrefixTable->LastNextEntry = Node;

    //
    //  And return this entry to our caller
    //

    return Node;
}


CLONG
ComputeUnicodeNameLength(
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine counts the number of names appearing in the input string.
    It does this by simply counting the number of backslashes in the string.
    To handle ill-formed names (i.e., names that do not contain a backslash)
    this routine really returns the number of backslashes plus 1.

Arguments:

    Name - Supplies the input name to examine

Returns Value:

    CLONG - the number of names in the input string

--*/

{
    WCHAR UnicodeBackSlash = '\\';
    ULONG NameLength;
    ULONG i;
    ULONG Count;

    RTL_PAGED_CODE();

    //
    //  Save the name length, this should make the compiler be able to
    //  optimize not having to reload the length each time
    //

    NameLength = (ULONG)Name->Length/2;

    //
    //  Now loop through the input string counting back slashes
    //

    for (i = 0, Count = 1; i < (ULONG)NameLength - 1; i += 1) {

        //
        //  check for a back slash
        //

        if (Name->Buffer[i] == UnicodeBackSlash) {

            Count += 1;
        }
    }

    //
    //  return the number of back slashes we found
    //

    //DbgPrint("ComputeUnicodeNameLength(%Z) = %x\n", Name, Count);

    return Count;
}


COMPARISON
CompareUnicodeStrings (
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Name,
    IN ULONG CaseInsensitiveIndex
    )

/*++

Routine Description:

    This routine takes a prefix string and a full name string and determines
    if the prefix string is a proper prefix of the name string (case sensitive)

Arguments:

    Prefix - Supplies the input prefix string

    Name - Supplies the full name input string

    CaseInsensitiveIndex - Indicates the wchar index at which to do a case
        insensitive search.  All characters before the index are searched
        case sensitive and all characters at and after the index are searched

Return Value:

    COMPARISON - returns

        IsLessThan    if Prefix < Name lexicalgraphically,
        IsPrefix      if Prefix is a proper prefix of Name
        IsEqual       if Prefix is equal to Name, and
        IsGreaterThan if Prefix > Name lexicalgraphically

--*/

{
    WCHAR UnicodeBackSlash = '\\';
    ULONG PrefixLength;
    ULONG NameLength;
    ULONG MinLength;
    ULONG i;

    WCHAR PrefixChar;
    WCHAR NameChar;

    RTL_PAGED_CODE();

    //DbgPrint("CompareUnicodeStrings(\"%Z\", \"%Z\") = ", Prefix, Name);

    //
    //  Save the length of the prefix and name string, this should allow
    //  the compiler to not need to reload the length through a pointer every
    //  time we need their values
    //

    PrefixLength = (ULONG)Prefix->Length/2;
    NameLength = (ULONG)Name->Length/2;

    //
    //  Special case the situation where the prefix string is simply "\" and
    //  the name starts with an "\"
    //

    if ((PrefixLength == 1) && (Prefix->Buffer[0] == UnicodeBackSlash) &&
        (NameLength > 1) && (Name->Buffer[0] == UnicodeBackSlash)) {
        //DbgPrint("IsPrefix\n");
        return IsPrefix;
    }

    //
    //  Figure out the minimum of the two lengths
    //

    MinLength = (PrefixLength < NameLength ? PrefixLength : NameLength);

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty.  First to the CaseSensitive part, then the
    //  CaseInsensitive part.
    //

    if (CaseInsensitiveIndex > MinLength) {

        CaseInsensitiveIndex = MinLength;
    }

    //
    //  CaseSensitive compare
    //

    for (i = 0; i < CaseInsensitiveIndex; i += 1) {

        PrefixChar = Prefix->Buffer[i];
        NameChar   = Name->Buffer[i];

        if (PrefixChar != NameChar) {

            break;
        }
    }

    //
    //  If we didn't break out of the above loop, do the
    //  CaseInsensitive compare.
    //

    if (i == CaseInsensitiveIndex) {

        WCHAR *s1 = &Prefix->Buffer[i];
        WCHAR *s2 = &Name->Buffer[i];

        for (; i < MinLength; i += 1) {

            PrefixChar = *s1++;
            NameChar = *s2++;

            if (PrefixChar != NameChar) {

                PrefixChar = NLS_UPCASE(PrefixChar);
                NameChar   = NLS_UPCASE(NameChar);

                if (PrefixChar != NameChar) {
                    break;
                }
            }
        }
    }

    //
    //  If we broke out of the above loop because of a mismatch, determine
    //  the result of the comparison.
    //

    if (i < MinLength) {

        //
        //  We also need to treat "\" as less than all other characters, so
        //  if the char is a "\" we'll drop it down to a value of zero.
        //

        if (PrefixChar == UnicodeBackSlash) {

            return IsLessThan;
        }

        if (NameChar == UnicodeBackSlash) {

            return IsGreaterThan;
        }

        //
        //  Now compare the characters
        //

        if (PrefixChar < NameChar) {

            return IsLessThan;

        } else if (PrefixChar > NameChar) {

            return IsGreaterThan;
        }
    }

    //
    //  They match upto the minimum length so now figure out the largest string
    //  and see if one is a proper prefix of the other
    //

    if (PrefixLength < NameLength) {

        //
        //  The prefix string is shorter so if it is a proper prefix we
        //  return prefix otherwise we return less than (e.g., "\a" < "\ab")
        //

        if (Name->Buffer[PrefixLength] == UnicodeBackSlash) {

            //DbgPrint("IsPrefix\n");

            return IsPrefix;

        } else {

            //DbgPrint("IsLessThan\n");

            return IsLessThan;
        }

    } else if (PrefixLength > NameLength) {

        //
        //  The Prefix string is longer so we say that the prefix is
        //  greater than the name (e.g., "\ab" > "\a")
        //

        //DbgPrint("IsGreaterThan\n");

        return IsGreaterThan;

    } else {

        //
        //  They lengths are equal so the strings are equal
        //

        //DbgPrint("IsEqual\n");

        return IsEqual;
    }
}

