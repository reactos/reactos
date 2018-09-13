/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    strngtab.c

Abstract:

    String table functions for Windows NT Setup API dll

    A string table is a block of memory that contains a bunch of strings.
    Hashing is used, and each hash table entry points to a linked list
    of strings within the string table. Strings within each linked list
    are sorted in ascending order. A node in the linked list consists of
    a pointer to the next node, followed by the string itself. Nodes
    are manually aligned to start on DWORD boundaries so we don't have to
    resort to using unaligned pointers.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (jamiehun) 15-Jan-1997

        fixed minor bug regarding use of STRTAB_NEW_EXTRADATA

--*/

#include "precomp.h"
#pragma hdrstop


//
// Values used for the initial and growth size
// of the string table data area
//
// (We start out with 6K, but remember that this includes the hash buckets.
// After you subtract their part of the buffer, you're left with ~4K bytes.)
//
#define STRING_TABLE_INITIAL_SIZE   6144
#define STRING_TABLE_GROWTH_SIZE    2048

#include "pshpack1.h"

typedef struct _STRING_NODE {
    //
    // This is stored as an offset instead of a pointer
    // because the table can move as it's built
    // The offset is from the beginning of the table
    //
    LONG NextOffset;
    //
    // This field must be last
    //
    TCHAR String[ANYSIZE_ARRAY];
} STRING_NODE, *PSTRING_NODE;

#include "poppack.h"


typedef struct _STRING_TABLE {
    PUCHAR Data;    // First HASH_BUCKET_COUNT DWORDS are StringNodeOffset array.
    DWORD DataSize;
    DWORD BufferSize;
    MYLOCK Lock;
    UINT ExtraDataSize;
    LCID Locale;
} STRING_TABLE, *PSTRING_TABLE;

#define LockTable(table)    BeginSynchronizedAccess(&((table)->Lock))
#define UnlockTable(table)  EndSynchronizedAccess(&((table)->Lock))


#ifdef UNICODE

#define FixedCompareString      CompareString

#else

#include <locale.h>
#include <mbctype.h>

INT
FixedCompareString (
    IN      LCID Locale,
    IN      DWORD Flags,
    IN      PCSTR FirstString,
    IN      INT Count1,
    IN      PCSTR SecondString,
    IN      INT Count2
    )
{
    LCID OldLocale;
    INT Result = 0;
    INT OldMbcp = 0;

    //
    // This routine uses the C runtime to compare the strings, because
    // the Win32 APIs are broken on some versions of Win95
    //

    OldLocale = GetThreadLocale();

    if (OldLocale != Locale) {
        SetThreadLocale (Locale);
        setlocale(LC_ALL,"");
        OldMbcp=_getmbcp();
        _setmbcp(_MB_CP_ANSI);
    }

    __try {
        if (Count1 == -1) {
            Count1 = strlen (FirstString);
        }

        if (Count2 == -1) {
            Count2 = strlen (SecondString);
        }

        //
        // The C runtime compares strings differently than the CompareString
        // API.  Most importantly, the C runtime considers uppercase to be
        // less than lowercase; the CompareString API is the opposite.
        //

        if (Flags & NORM_IGNORECASE) {
            Result = _mbsnbicmp (FirstString, SecondString, min (Count1, Count2));
        } else {
            Result = _mbsnbcmp (FirstString, SecondString, min (Count1, Count2));
        }

        //
        // We now convert the C runtime result into the CompareString result.
        // This means making the comparison a Z to A ordering, with lowercase
        // coming before uppercase. The length comparison does not get reversed.
        //

        if(Result == _NLSCMPERROR) {

            Result = 0;                         // zero returned if _mbsnbicmp could not compare

        } else if (Result < 0) {

            Result = CSTR_GREATER_THAN;

        } else if (Result == 0) {

            if (Count1 < Count2) {
                Result = CSTR_LESS_THAN;         // first string shorter than second
            } else if (Count1 > Count2) {
                Result = CSTR_GREATER_THAN;      // first string longer than second
            } else {
                Result = CSTR_EQUAL;
            }

        } else {
            Result = CSTR_LESS_THAN;
        }
    }
    __except (TRUE) {
        Result = 0;
    }

    if (OldLocale != Locale) {
        SetThreadLocale (OldLocale);
        setlocale(LC_ALL,"");
        _setmbcp(OldMbcp);
    }

    return Result;
}

#endif


DWORD
pStringTableCheckFlags(
    IN DWORD FlagsIn
    )
/*++

Routine Description:

    Pre-process flags, called by exported routines we want to handle the
    combination of CASE_INSENSITIVE, CASE_SENSITIVE and BUFFER_WRITEABLE
    and keep all other flags as is.

Arguments:

    FlagsIn - flags as supplied

Return Value:

    Flags out

--*/

{
    DWORD FlagsOut;
    DWORD FlagsSpecial;

    //
    // we're just interested in these flags for the switch
    //
    FlagsSpecial = FlagsIn & (STRTAB_CASE_SENSITIVE | STRTAB_BUFFER_WRITEABLE);

    //
    // strip these off FlagsIn to create initial FlagsOut
    //
    FlagsOut = FlagsIn ^ FlagsSpecial;

    switch (FlagsSpecial) {

    case STRTAB_CASE_INSENSITIVE :
    case STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE :
        //
        // these cases ok
        //
        FlagsOut |= FlagsSpecial;
        break;

    default :
        //
        // any other combination is treated as STRTAB_CASE_SENSITIVE (and so
        // WRITEABLE doesn't matter)
        //
        FlagsOut |= STRTAB_CASE_SENSITIVE;
    }

    return FlagsOut;
}


VOID
pStringTableComputeHashValue(
    IN  PTSTR  String,
    OUT PDWORD StringLength,
    IN  DWORD  Flags,
    OUT PDWORD HashValue
    )

/*++

Routine Description:

    Compute a hash value for a given string.

    The algorithm simply adds up the unicode values for each
    character in the string and then takes the result mod the
    number of hash buckets.

Arguments:

    String - supplies the string for which a hash value is desired.

    StringLength - receives the number of characters in the string,
        not including the terminating nul.

    Flags - supplies flags controlling how the hashing is to be done.  May be
        a combination of the following values (all other bits ignored):

        STRTAB_BUFFER_WRITEABLE  - The caller-supplied buffer may be written to during
                                   the string look-up.  Specifying this flag improves the
                                   performance of this API for case-insensitive string
                                   additions.  This flag is ignored for case-sensitive
                                   string additions.

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

    HashValue - receives the hash value.

Return Value:

    None.

--*/

{
    DWORD Length;
    DWORD Value = 0;
    PCTSTR p, q;
    DWORD Char;

    try {

        if((Flags & (STRTAB_BUFFER_WRITEABLE | STRTAB_ALREADY_LOWERCASE)) == STRTAB_BUFFER_WRITEABLE) {
            //
            // Then the buffer is writeable, but isn't yet lower-case.  Take care of that right now.
            //

#ifndef UNICODE
            _mbslwr (String);
#else
            CharLower(String);
#endif

            Flags |= STRTAB_ALREADY_LOWERCASE;
        }

//
// Define a macro to ensure we don't get sign-extension when adding up character values.
//
#ifdef UNICODE
    #define DWORD_FROM_TCHAR(x)   ((DWORD)((WCHAR)(x)))
#else
    #define DWORD_FROM_TCHAR(x)   ((DWORD)((UCHAR)(x)))
#endif

        p = String;

        if(Flags & STRTAB_ALREADY_LOWERCASE) {

            while (*p) {
                Value += DWORD_FROM_TCHAR (*p);
                p++;
            }

        } else {
            //
            // Make sure we don't get sign-extension on extended chars
            // in String -- otherwise we get values like 0xffffffe4 passed
            // to CharLower(), which thinks it's a pointer and faults.
            //

#ifdef UNICODE
            //
            // The WCHAR case is trivial
            //

            while (*p) {
                Value += DWORD_FROM_TCHAR(CharLower((PWSTR)(WORD) (*p)));
                p++;
            }

#else
            //
            // The DBCS case is a mess because of the possibility of CharLower
            // altering a two-byte character
            // Standardize to use _mbslwr as that is used elsewhere
            // ie, if we did _mbslwr, & called this function with
            // flag set to say "already lower", vs we called function
            // with buffer writable, vs calling with neither
            // we should ensure we get same hash in each case
            // it may fail, but at least it will fail *universally* and
            // generate the same hash
            //
            PTSTR copy = DuplicateString(String);
            if(copy) {
                //
                // do conversion on copied string
                //
                _mbslwr(copy);
                p = copy;
                while (*p) {
                    Value += DWORD_FROM_TCHAR (*p);
                    p++;
                }
                MyFree(copy);
                p = String+lstrlen(String);
            } else {
                //
                // we had a memory failure
                //
                *HashValue = 0;
                *StringLength = 0;
                leave;
            }
#endif

        }

        *HashValue = Value % HASH_BUCKET_COUNT;
        *StringLength = (DWORD)(p - String);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Inbound string was bogus
        //

        *HashValue = 0;
        *StringLength = 0;
        MYASSERT(FALSE);
    }
}


LONG
pStringTableLookUpString(
    IN     PVOID   StringTable,
    IN OUT PTSTR   String,
    OUT    PDWORD  StringLength,
    OUT    PDWORD  HashValue,           OPTIONAL
    OUT    PVOID  *FindContext,         OPTIONAL
    IN     DWORD   Flags,
    OUT    PVOID   ExtraData,           OPTIONAL
    IN     UINT    ExtraDataBufferSize  OPTIONAL
    )

/*++

Routine Description:

    Locates a string in the string table, if present.
    If the string is not present, this routine may optionally tell its
    caller where the search stopped. This is useful for maintaining a
    sorted order for the strings.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be looked up

    StringLength - receives number of characters in the string, not
        including the terminating nul.

    HashValue - Optionally, receives hash value for the string.

    FindContext - Optionally, receives the context at which the search was
        terminated.

        (NOTE: This is actually a PSTRING_NODE pointer, that is used
        during new string addition.  Since this routine has wider exposure
        than just internal string table usage, this parameter is made into
        a PVOID, so no one else has to have access to string table-internal
        structures.

        On return, this variable receives a pointer to the string node of
        the node where the search stopped. If the string was found, then
        this is a pointer to the string's node. If the string was not found,
        then this is a pointer to the last string node whose string is
        'less' (based on lstrcmpi) than the string we're looking for.
        Note that this value may be NULL.)

    Flags - supplies flags controlling how the string is to be located.  May be
        a combination of the following values:

        STRTAB_CASE_INSENSITIVE  - Search for the string case-insensitively.

        STRTAB_CASE_SENSITIVE    - Search for the string case-sensitively.  This flag
                                   overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE  - The caller-supplied buffer may be written to during
                                   the string look-up.  Specifying this flag improves the
                                   performance of this API for case-insensitive string
                                   additions.  This flag is ignored for case-sensitive
                                   string additions.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

    ExtraData - if specified, receives extra data associated with the string
        if the string is found.

    ExtraDataBufferSize - if ExtraData is specified, then this parameter
        specifies the size of the buffer, in bytes. As much extra data as will fit
        is stored here.

Return Value:

    The return value is a value that uniquely identifies the string
    within the string table, namely the offset of the string node
    within the string table.

    If the string could not be found the value is -1.

--*/

{
    PSTRING_NODE node,prev;
    int i;
    PSTRING_TABLE stringTable = StringTable;
    DWORD hashValue;
    PSTRING_NODE FinalNode;
    LONG rc = -1;
    LCID Locale;
    DWORD CompareFlags;
    BOOL CollateEnded = FALSE;

    //
    // If this is a case-sensitive lookup, then we want to reset the STRTAB_BUFFER_WRITEABLE
    // flag, if present, since otherwise the string will get replaced with its all-lowercase
    // counterpart.
    //
    if(Flags & STRTAB_CASE_SENSITIVE) {
        Flags &= ~STRTAB_BUFFER_WRITEABLE;
    }

    //
    // Compute hash value
    //
    pStringTableComputeHashValue(String,StringLength,Flags,&hashValue);

    if(((PLONG)(stringTable->Data))[hashValue] == -1) {
        //
        // The string table contains no strings at the computed hash value.
        //
        FinalNode = NULL;
        goto clean0;
    }

    //
    // We know there's at least one string in the table with the computed
    // hash value, so go find it. There's no previous node yet.
    //
    node = (PSTRING_NODE)(stringTable->Data + ((PLONG)(stringTable->Data))[hashValue]);
    prev = NULL;

    //
    // Go looking through the string nodes for that hash value,
    // looking through the string.
    //
    Locale = stringTable->Locale;

    CompareFlags = (Flags & STRTAB_CASE_SENSITIVE) ? 0 : NORM_IGNORECASE;

    while(1) {

        if(i = FixedCompareString(Locale,CompareFlags,String,-1,node->String,-1)) {
            i -= 2;
        } else {
            //
            // Failure, try system default locale
            //
            if(i = FixedCompareString(LOCALE_SYSTEM_DEFAULT,CompareFlags,String,-1,node->String,-1)) {
                i -= 2;
            } else {
                //
                // Failure, just use CRTs
                //
                // BugBug(jamiehun) 9/10/99 - this could give wrong collation order?
                //
                i = (Flags & STRTAB_CASE_SENSITIVE)
                  ? _tcscmp(String,node->String)
                  : _tcsicmp(String,node->String);
            }
        }

        if(i == 0) {
            FinalNode = node;
            rc = (LONG)((PUCHAR)node - stringTable->Data);
            break;
        }

        //
        // If the string we are looking for is 'less' than the current
        // string, mark it's position so we can insert a new string before here
        // (ANSI) but keep searching (UNICODE) we can abort - old behaviour
        //
        if((i < 0) && !CollateEnded) {
            CollateEnded = TRUE;
            FinalNode = prev;
#if UNICODE
            break;
#endif
        }

        //
        // The string we are looking for is 'greater' than the current string.
        // Keep looking, unless we've reached the end of the table.
        //
        if(node->NextOffset == -1) {
            if(!CollateEnded)
            {
                //
                // unless we found a more ideal position
                // return the end of the list
                //
                FinalNode = node;
            }
            break;
        } else {
            prev = node;
            node = (PSTRING_NODE)(stringTable->Data + node->NextOffset);
        }
    }

clean0:

    if((rc != -1) && ExtraData) {
        //
        // Extra data is stored immediately following the string.
        //
        CopyMemory(
            ExtraData,
            FinalNode->String + *StringLength + 1,
            min(ExtraDataBufferSize,stringTable->ExtraDataSize)
            );
    }

    if(HashValue) {
        *HashValue = hashValue;
    }
    if(FindContext) {
        *FindContext = FinalNode;
    }

    return rc;
}


LONG
StringTableLookUpString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags
    )

/*++

Routine Description:

    Locates a string in the string table, if present.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be looked up.  If STRTAB_BUFFER_WRITEABLE is
        specified and a case-insensitive lookup is requested, then this buffer
        will be all lower-case upon return.

    Flags - supplies flags controlling how the string is to be located.  May be
        a combination of the following values:

        STRTAB_CASE_INSENSITIVE  - Search for the string case-insensitively.

        STRTAB_CASE_SENSITIVE    - Search for the string case-sensitively.  This flag
                                   overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE  - The caller-supplied buffer may be written to during
                                   the string look-up.  Specifying this flag improves the
                                   performance of this API for case-insensitive string
                                   additions.  This flag is ignored for case-sensitive
                                   string additions.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

Return Value:

    The return value is a value that uniquely identifies the string
    within the string table.

    If the string could not be found the value is -1.

--*/

{
    DWORD StringLength, PrivateFlags, AlreadyLcFlag;
    LONG rc = -1;
    BOOL locked = FALSE;

    try {
        if (!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        PrivateFlags = pStringTableCheckFlags(Flags);

        rc = pStringTableLookUpString(
                                     StringTable,
                                     String,
                                     &StringLength,
                                     NULL,
                                     NULL,
                                     PrivateFlags,
                                     NULL,
                                     0
                                     );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        rc = -1;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return (rc);
}


LONG
StringTableLookUpStringEx(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags,
       OUT PVOID ExtraData,             OPTIONAL
    IN     UINT  ExtraDataBufferSize    OPTIONAL
    )

/*++

Routine Description:

    Locates a string in the string table, if present.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be looked up.  If STRTAB_BUFFER_WRITEABLE is
        specified and a case-insensitive lookup is requested, then this buffer
        will be all lower-case upon return.

    Flags - supplies flags controlling how the string is to be located.  May be
        a combination of the following values:

        STRTAB_CASE_INSENSITIVE  - Search for the string case-insensitively.

        STRTAB_CASE_SENSITIVE    - Search for the string case-sensitively.  This flag
                                   overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE  - The caller-supplied buffer may be written to during
                                   the string look-up.  Specifying this flag improves the
                                   performance of this API for case-insensitive string
                                   additions.  This flag is ignored for case-sensitive
                                   string additions.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

    ExtraData - if specified, receives extra data associated with the string
        if the string is found.

    ExtraDataBufferSize - if ExtraData is specified, then this parameter
        specifies the size of the buffer, in bytes. As much extra data as will fit
        is stored here.

Return Value:

    The return value is a value that uniquely identifies the string
    within the string table.

    If the string could not be found the value is -1.

--*/

{
    DWORD StringLength, PrivateFlags, AlreadyLcFlag;
    LONG rc = -1;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        PrivateFlags = pStringTableCheckFlags(Flags);

        rc = pStringTableLookUpString(
                                     StringTable,
                                     String,
                                     &StringLength,
                                     NULL,
                                     NULL,
                                     PrivateFlags,
                                     ExtraData,
                                     ExtraDataBufferSize
                                     );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        rc = -1;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return (rc);
}


BOOL
StringTableGetExtraData(
    IN  PVOID StringTable,
    IN  LONG  StringId,
    OUT PVOID ExtraData,
    IN  UINT  ExtraDataBufferSize
    )

/*++

Routine Description:

    Get arbitrary data associated with a string table entry.

Arguments:

    StringTable - supplies handle to string table containing the string
        whose associated data is to be returned.

    String - supplies the id of the string whose associated data is to be returned.

    ExtraData - receives the data associated with the string. Data is truncated
        to fit, if necessary.

    ExtraDataBufferSize - supplies the size in bytes of the buffer specified
        by ExtraData. If this value is smaller than the extra data size for
        the string table, data is truncated to fit.

Return Value:

    Boolean value indicating outcome.

--*/

{
    BOOL b = FALSE;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        b = pStringTableGetExtraData(StringTable,StringId,ExtraData,ExtraDataBufferSize);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return(b);
}


BOOL
pStringTableGetExtraData(
    IN  PVOID StringTable,
    IN  LONG  StringId,
    OUT PVOID ExtraData,
    IN  UINT  ExtraDataBufferSize
    )

/*++

Routine Description:

    Get arbitrary data associated with a string table entry.
    THIS ROUTINE DOES NOT DO LOCKING and IT DOES NOT HANDLE EXCEPTIONS!

Arguments:

    StringTable - supplies handle to string table containing the string
        whose associated data is to be returned.

    String - supplies the id of the string whose associated data is to be returned.

    ExtraData - receives the data associated with the string. Data is truncated
        to fit, if necessary.

    ExtraDataBufferSize - supplies the size in bytes of the buffer specified
        by ExtraData. If this value is smaller than the extra data size for
        the string table, data is truncated to fit.

Return Value:

    Boolean value indicating outcome.

--*/

{
    PSTRING_TABLE stringTable = StringTable;
    PSTRING_NODE stringNode;
    PVOID p;

    stringNode = (PSTRING_NODE)(stringTable->Data + StringId);
    p = stringNode->String + lstrlen(stringNode->String) + 1;

    CopyMemory(ExtraData,p,min(ExtraDataBufferSize,stringTable->ExtraDataSize));

    return(TRUE);
}


BOOL
StringTableSetExtraData(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN PVOID ExtraData,
    IN UINT  ExtraDataSize
    )

/*++

Routine Description:

    Associate arbitrary data with a string table entry.

Arguments:

    StringTable - supplies handle to string table containing the string
        with which the data is to be associated.

    String - supplies the id of the string with which the data is to be associated.

    ExtraData - supplies the data to be associated with the string.

    ExtraDataSize - specifies the size in bytes of the data. If the data is
        larger than the extra data size for this string table, then the routine fails.

Return Value:

    Boolean value indicating outcome.

--*/

{
    BOOL b = FALSE;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        b = pStringTableSetExtraData(StringTable,StringId,ExtraData,ExtraDataSize);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return(b);
}


BOOL
pStringTableSetExtraData(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN PVOID ExtraData,
    IN UINT  ExtraDataSize
    )

/*++

Routine Description:

    Associate arbitrary data with a string table entry.

Arguments:

    StringTable - supplies handle to string table containing the string
        with which the data is to be associated.

    String - supplies the id of the string with which the data is to be associated.

    ExtraData - supplies the data to be associated with the string.

    ExtraDataSize - specifies the size in bytes of the data. If the data is
        larger than the extra data size for this string table, then the routine fails.

Return Value:

    Boolean value indicating outcome.

--*/

{
    PSTRING_TABLE stringTable = StringTable;
    PSTRING_NODE stringNode;
    BOOL b;
    PVOID p;

    if(ExtraDataSize <= stringTable->ExtraDataSize) {

        stringNode = (PSTRING_NODE)(stringTable->Data + StringId);

        p = stringNode->String + lstrlen(stringNode->String) + 1;

        ZeroMemory(p,stringTable->ExtraDataSize);
        CopyMemory(p,ExtraData,ExtraDataSize);

        b = TRUE;

    } else {
        b = FALSE;
    }

    return(b);
}


LONG
pStringTableAddString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags,
    IN     PVOID ExtraData,     OPTIONAL
    IN     UINT  ExtraDataSize  OPTIONAL
    )

/*++

Routine Description:

    Adds a string to the string table if the string is not already
    in the string table.  (Does not do locking!)

    If the string is to be added case-insensitively, then it is
    lower-cased, and added case-sensitively.  Since lower-case characters
    are 'less than' lower case ones (according to lstrcmp), this ensures that
    a case-insensitive string will always appear in front of any of its
    case-sensitive counterparts.  This ensures that we always find the correct
    string ID for things like section names.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be added

    Flags - supplies flags controlling how the string is to be added, and
        whether the caller-supplied buffer may be modified.  May be a combination
        of the following values:

        STRTAB_CASE_INSENSITIVE  - Add the string case-insensitively.  The
                                   specified string will be added to the string
                                   table as all lower-case.  This flag is overridden
                                   if STRTAB_CASE_SENSITIVE is specified.

        STRTAB_CASE_SENSITIVE    - Add the string case-sensitively.  This flag
                                   overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE  - The caller-supplied buffer may be written to during
                                   the string-addition process.  Specifying this flag
                                   improves the performance of this API for case-
                                   insensitive string additions.  This flag is ignored
                                   for case-sensitive string additions.

        STRTAB_NEW_EXTRADATA     - if the string already exists in the table
                                   and ExtraData is specified (see below) then
                                   the new ExtraData overwrites any existing extra data.
                                   Otherwise any existing extra data is left alone.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

    ExtraData - if supplied, specifies extra data to be associated with the string
        in the string table. If the string already exists in the table, the Flags
        field controls whether the new data overwrites existing data already
        associated with the string.

    ExtraDataSize - if ExtraData is supplied, then this value supplies the size
        in bytes of the buffer pointed to by ExtraData. If the data is larger than
        the extra data size for the string table, the routine fails.

Return Value:

    The return value uniquely identifes the string within the string table.
    It is -1 if the string was not in the string table but could not be added
    (out of memory).

--*/

{
    LONG rc;
    PSTRING_TABLE stringTable = StringTable;
    DWORD StringLength;
    DWORD HashValue;
    PSTRING_NODE PreviousNode,NewNode;
    DWORD SpaceRequired;
    PTSTR TempString = String;
    BOOL FreeTempString = FALSE;
    PVOID p;

    if (!(Flags & STRTAB_CASE_SENSITIVE)) {
        //
        // not case sensitive ( = insensitive)
        //
        if (!(Flags & STRTAB_ALREADY_LOWERCASE)) {
            //
            // not already lowercase
            //
            if (!(Flags & STRTAB_BUFFER_WRITEABLE)) {
                //
                // not writable
                //
                //
                // Then the string is to be added case-insensitively, but the caller
                // doesn't want us to write to their buffer.  Allocate one of our own.
                //
                if (TempString = DuplicateString(String)) {
                    FreeTempString = TRUE;
                } else {
                    //
                    // We couldn't allocate space for our duplicated string.  Since we'll
                    // only consider exact matches (where the strings are all lower-case),
                    // we're stuck, since we can't lower-case the buffer in place.
                    //
                    return -1;
                }
            }
            //
            // Lower-case the buffer.
            //
#ifndef UNICODE
            _mbslwr (TempString);
#else
            CharLower(TempString);
#endif
        }

        //
        // we know that the string is now lower-case
        // we no longer need "Writable" flag
        // searches will be case sensitive
        //
        Flags &= ~ (STRTAB_BUFFER_WRITEABLE | STRTAB_CASE_INSENSITIVE);
        Flags |= STRTAB_CASE_SENSITIVE | STRTAB_ALREADY_LOWERCASE;
    }

    try {
        if (ExtraData && (ExtraDataSize > stringTable->ExtraDataSize)) {
            //
            // Force us into the exception handler -- sort of a non-local goto.
            //
            RaiseException(0,0,0,NULL);
        }

        //
        // The string might already be in there.
        //
        rc = pStringTableLookUpString(
                                     StringTable,
                                     TempString,
                                     &StringLength,
                                     &HashValue,
                                     &PreviousNode,
                                     Flags,
                                     NULL,
                                     0
                                     );

        if (rc != -1) {
            if (ExtraData && (Flags & STRTAB_NEW_EXTRADATA)) {
                //
                // Overwrite extra data. We know the data is small enough to fit
                // because we checked for this above.
                //
                p = PreviousNode->String + StringLength + 1;

                ZeroMemory(p,stringTable->ExtraDataSize);
                CopyMemory(p,ExtraData,ExtraDataSize);
            }

            if (FreeTempString) {
                MyFree(TempString);
            }
            return (rc);
        }

        //
        // Figure out how much space is required to hold this entry.
        // This is the size of a STRING_NODE plus the length of the string
        // plus space for extra per-element data.
        //
        SpaceRequired = offsetof(STRING_NODE,String)
                        + ((StringLength+1)*sizeof(TCHAR))
                        + stringTable->ExtraDataSize;

        //
        // Make sure things stay aligned within the table
        //
        if (SpaceRequired % sizeof(DWORD)) {
            SpaceRequired += sizeof(DWORD) - (SpaceRequired % sizeof(DWORD));
        }

        //
        // See if there is currently enough room to add the string to the table.
        //
        while (stringTable->DataSize + SpaceRequired > stringTable->BufferSize) {

            //
            // Grow the string table.
            //
            PVOID p;
            p = MyRealloc(stringTable->Data,stringTable->BufferSize+STRING_TABLE_GROWTH_SIZE);
            if (!p) {

                if (FreeTempString) {
                    MyFree(TempString);
                }
                return (-1);
            }

            //
            // Adjust previous node pointer.
            //
            if (PreviousNode) {
                PreviousNode = (PSTRING_NODE)((PUCHAR)p + ((PUCHAR)PreviousNode-(PUCHAR)stringTable->Data));
            }
            stringTable->Data = p;
            stringTable->BufferSize += STRING_TABLE_GROWTH_SIZE;
        }

        //
        // Stick the string and extra data, if any, in the string table buffer.
        //
        NewNode = (PSTRING_NODE)(stringTable->Data + stringTable->DataSize);

        if (PreviousNode) {
            NewNode->NextOffset = PreviousNode->NextOffset;
            PreviousNode->NextOffset = (LONG)((LONG_PTR)NewNode - (LONG_PTR)stringTable->Data);
        } else {
            NewNode->NextOffset = ((PLONG)(stringTable->Data))[HashValue];
            ((PLONG)(stringTable->Data))[HashValue] = (LONG)((LONG_PTR)NewNode - (LONG_PTR)stringTable->Data);
        }

        lstrcpy(NewNode->String,TempString);

        p = NewNode->String + StringLength + 1;

        ZeroMemory(p,stringTable->ExtraDataSize);
        if (ExtraData) {
            CopyMemory(p,ExtraData,ExtraDataSize);
        }

        stringTable->DataSize += SpaceRequired;

        rc = (LONG)((LONG_PTR)NewNode - (LONG_PTR)stringTable->Data);

    }except(EXCEPTION_EXECUTE_HANDLER) {
        rc = -1;
    }

    if (FreeTempString) {
        MyFree(TempString);
    }

    return rc;
}


LONG
StringTableAddString(
    IN PVOID StringTable,
    IN PTSTR String,
    IN DWORD Flags
    )

/*++

Routine Description:

    Adds a string to the string table if the string is not already
    in the string table.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be added

    Flags - supplies flags controlling how the string is to be added, and
        whether the caller-supplied buffer may be modified.  May be a combination
        of the following values:

        STRTAB_CASE_INSENSITIVE - Add the string case-insensitively.  The
                                  specified string will be added to the string
                                  table as all lower-case.  This flag is overridden
                                  if STRTAB_CASE_SENSITIVE is specified.

        STRTAB_CASE_SENSITIVE   - Add the string case-sensitively.  This flag
                                  overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE - The caller-supplied buffer may be written to during
                                  the string-addition process.  Specifying this flag
                                  improves the performance of this API for case-
                                  insensitive string additions.  This flag is ignored
                                  for case-sensitive string additions.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

Return Value:

    The return value uniquely identifes the string within the string table.
    It is -1 if the string was not in the string table but could not be added
    (out of memory).

--*/

{
    LONG rc = -1;
    BOOL locked = FALSE;
    DWORD PrivateFlags;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        PrivateFlags = pStringTableCheckFlags(Flags);

        rc = pStringTableAddString(StringTable, String, PrivateFlags, NULL, 0);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        rc = -1;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return(rc);
}


LONG
StringTableAddStringEx(
    IN PVOID StringTable,
    IN PTSTR String,
    IN DWORD Flags,
    IN PVOID ExtraData,     OPTIONAL
    IN UINT  ExtraDataSize  OPTIONAL
    )

/*++

Routine Description:

    Adds a string to the string table if the string is not already
    in the string table.

Arguments:

    StringTable - supplies handle to string table to be searched
        for the string

    String - supplies the string to be added

    Flags - supplies flags controlling how the string is to be added, and
        whether the caller-supplied buffer may be modified.  May be a combination
        of the following values:

        STRTAB_CASE_INSENSITIVE - Add the string case-insensitively.  The
                                  specified string will be added to the string
                                  table as all lower-case.  This flag is overridden
                                  if STRTAB_CASE_SENSITIVE is specified.

        STRTAB_CASE_SENSITIVE   - Add the string case-sensitively.  This flag
                                  overrides the STRTAB_CASE_INSENSITIVE flag.

        STRTAB_BUFFER_WRITEABLE - The caller-supplied buffer may be written to during
                                  the string-addition process.  Specifying this flag
                                  improves the performance of this API for case-
                                  insensitive string additions.  This flag is ignored
                                  for case-sensitive string additions.

        STRTAB_NEW_EXTRADATA    - If the string already exists in the table
                                  and ExtraData is specified (see below) then
                                  the new ExtraData overwrites any existing extra data.
                                  Otherwise any existing extra data is left alone.

        In addition to the above public flags, the following private flag is also
        allowed:

        STRTAB_ALREADY_LOWERCASE - The supplied string has already been converted to
                                   all lower-case (e.g., by calling CharLower), and
                                   therefore doesn't need to be lower-cased in the
                                   hashing routine.  If this flag is supplied, then
                                   STRTAB_BUFFER_WRITEABLE is ignored, since modifying
                                   the caller's buffer is not required.

    ExtraData - if supplied, specifies extra data to be associated with the string
        in the string table. If the string already exists in the table, the Flags
        field controls whether the new data overwrites existing data already
        associated with the string.

    ExtraDataSize - if ExtraData is supplied, then this value supplies the size
        in bytes of the buffer pointed to by ExtraData. If the data is larger than
        the extra data size for the string table, the routine fails.

Return Value:

    The return value uniquely identifes the string within the string table.
    It is -1 if the string was not in the string table but could not be added
    (out of memory).

--*/

{
    LONG rc = -1;
    BOOL locked = FALSE;
    DWORD PrivateFlags;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        PrivateFlags = pStringTableCheckFlags(Flags);

        rc = pStringTableAddString(StringTable, String, PrivateFlags, ExtraData, ExtraDataSize);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        rc = -1;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
    return (rc);
}


BOOL
StringTableEnum(
    IN  PVOID                StringTable,
    OUT PVOID                ExtraDataBuffer,     OPTIONAL
    IN  UINT                 ExtraDataBufferSize, OPTIONAL
    IN  PSTRTAB_ENUM_ROUTINE Callback,
    IN  LPARAM               lParam               OPTIONAL
    )

/*++

Routine Description:

    For every string in a string table, inform a callback routine of
    the stirng's id, it's value, and any associated data.

Arguments:

    StringTable - supplies a pointer to the string table to be enumerated.

    ExtraDataBuffer - supplies the address of a buffer to be passed to
        the callback routine for each string, which will be filled in
        with the associated data of each string.

    ExtraDataBufferSize - if ExtraDataBuffer is specified then this
        supplies the size of that buffer in bytes. If this value is
        smaller than the size of the extra data for the string table,
        the enumeration fails.

    Callback - supplies the routine to be notified of each string.

    lParam - supplies an optional parameter meaningful to the caller
        which is passed on to the callback unchanged.

Return Value:

    Boolean value indicating outcome. TRUE unless ExtraDataBufferSize
    is too small.

--*/

{
    BOOL b = FALSE;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        b = pStringTableEnum(StringTable,ExtraDataBuffer,ExtraDataBufferSize,Callback,lParam);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }

    return(b);
}


BOOL
pStringTableEnum(
    IN  PVOID                StringTable,
    OUT PVOID                ExtraDataBuffer,     OPTIONAL
    IN  UINT                 ExtraDataBufferSize, OPTIONAL
    IN  PSTRTAB_ENUM_ROUTINE Callback,
    IN  LPARAM               lParam               OPTIONAL
    )

/*++

Routine Description:

    For every string in a string table, inform a callback routine of
    the stirng's id, its value, and any associated data.

    THIS ROUTINE DOES NOT DO LOCKING.

Arguments:

    StringTable - supplies a pointer to the string table to be enumerated.

    ExtraDataBuffer - supplies the address of a buffer to be passed to
        the callback routine for each string, which will be filled in
        with the associated data of each string.

    ExtraDataBufferSize - if ExtraDataBuffer is specified then this
        supplies the size of that buffer in bytes. If this value is
        smaller than the size of the extra data for the string table,
        the enumeration fails.

    Callback - supplies the routine to be notified of each string.

    lParam - supplies an optional parameter meaningful to the caller
        which is passed on to the callback unchanged.

Return Value:

    Boolean value indicating outcome. TRUE unless ExtraDataBufferSize
    is too small.

--*/

{
    UINT u;
    PSTRING_TABLE stringTable = StringTable;
    PSTRING_NODE stringNode;
    LONG FirstOffset;
    BOOL b;

    //
    // Validate buffer size.
    //
    if(ExtraDataBuffer && (ExtraDataBufferSize < stringTable->ExtraDataSize)) {
        return(FALSE);
    }

    for(b=TRUE,u=0; u<HASH_BUCKET_COUNT; u++) {

        FirstOffset = ((PLONG)stringTable->Data)[u];

        if(FirstOffset == -1) {
            continue;
        }

        stringNode = (PSTRING_NODE)(stringTable->Data + FirstOffset);

        do {

            if(ExtraDataBuffer) {
                CopyMemory(
                    ExtraDataBuffer,
                    stringNode->String + lstrlen(stringNode->String) + 1,
                    stringTable->ExtraDataSize
                    );
            }

            b = Callback(
                    StringTable,
                    (LONG)((PUCHAR)stringNode - stringTable->Data),
                    stringNode->String,
                    ExtraDataBuffer,
                    ExtraDataBuffer ? stringTable->ExtraDataSize : 0,
                    lParam
                    );

            stringNode = (stringNode->NextOffset == -1)
                       ? NULL
                       : (PSTRING_NODE)(stringTable->Data + stringNode->NextOffset);

        } while(b && stringNode);
    }

    return(TRUE);
}


PTSTR
pStringTableStringFromId(
    IN PVOID StringTable,
    IN LONG  StringId
    )

/*++

Routine Description:

    Given a string ID returned when a string was added or looked up,
    return a pointer to the actual string.  (This is exactly the same
    as StringTableStringFromId, except that it doesn't do locking.)

Arguments:

    StringTable - supplies a pointer to the string table containing the
        string to be retrieved.

    StringId - supplies a string id returned from StringTableAddString
        or StringTableLookUpString.

Return Value:

    Pointer to string data. The caller must not write into or otherwise
    alter the string.

--*/

{
    return ((PSTRING_NODE)(((PSTRING_TABLE)StringTable)->Data + StringId))->String;
}


PTSTR
StringTableStringFromId(
    IN PVOID StringTable,
    IN LONG  StringId
    )

/*++

Routine Description:

    Given a string ID returned when a string was added or looked up,
    return a pointer to the actual string.

Arguments:

    StringTable - supplies a pointer to the string table containing the
        string to be retrieved.

    StringId - supplies a string id returned from StringTableAddString
        or StringTableLookUpString.

Return Value:

    Pointer to string data. The caller must not write into or otherwise
    alter the string.

    BugBug!!! (jamiehun) this function is fundamentally not thread-safe
    since the ptr we return could be modified by another thread
    if string table is accessed by more than one thread
    Hence new API below StringTableStringFromIdEx

--*/

{
    PTSTR p = NULL;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        p = ((PSTRING_NODE)(((PSTRING_TABLE)StringTable)->Data + StringId))->String;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }

    return(p);
}


BOOL
StringTableStringFromIdEx(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN OUT PTSTR pBuffer,
    IN OUT PULONG pBufSize
    )

/*++

Routine Description:

    Given a string ID returned when a string was added or looked up,
    return a pointer to the actual string.

Arguments:

    StringTable - supplies a pointer to the string table containing the
        string to be retrieved.

    StringId - supplies a string id returned from StringTableAddString
        or StringTableLookUpString.

    pBuffer - points to a buffer that will be filled out with the string
        to be retrieved

    pBufSize - supplies a pointer to an input/output parameter that contains
        the size of the buffer on entry, and the number of chars. written on
        exit.

Return Value:

    TRUE if the string was written. pBufSize contains the length of the string
    FALSE if the buffer was invalid, or the string ID was invalid, or the buffer
          wasn't big enough. If pBufSize non-zero, then it is the required bufsize.
          currently caller can tell the difference between invalid param and buffer
          size by checking pBufSize

    BUGBUG!!! (jamiehun) should/can we set LastError?

--*/

{
    PTSTR p;
    ULONG len;
    PSTRING_TABLE stringTable = (PSTRING_TABLE)StringTable;
    DWORD status = ERROR_INVALID_DATA;
    BOOL locked = FALSE;

    try {
        if  (!pBufSize) {
            status = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(!LockTable(stringTable)) {
            if (pBuffer != NULL && *pBufSize > 0) {
                pBuffer[0]=0;
            }
            *pBufSize = 0;
            status = ERROR_INVALID_HANDLE;
            leave;
        }
        locked = TRUE;

        //
        // CFGMGR calls this with an ID passed by it's caller
        // we have to check bounds here (while table is locked)
        // the check has to do:
        //
        // (1) StringId must be > 0 (0 is hash-bucket 0)
        // (2) StringId must be < size of string table
        // BUGBUG!!! (jamiehun) this isn't really good enough but should
        // catch common errors.
        //
        // the check is here since Id validity requires access to Opaque pointer
        //
        if(StringId <= 0 || StringId >= (LONG)(stringTable->DataSize)) {
            if (pBuffer != NULL && *pBufSize > 0) {
                pBuffer[0]=0;
            }
            *pBufSize = 0;
            status = ERROR_INVALID_PARAMETER;
            leave;
        }

        len = lstrlen( ((PSTRING_NODE)(stringTable->Data + StringId))->String);
        len ++; // account for terminating NULL

        if (len > *pBufSize || pBuffer == NULL) {
            MYASSERT ( len > *pBufSize);
            if (pBuffer != NULL && *pBufSize > 0) {
                pBuffer[0]=0;
            }
            *pBufSize = len;
            status = ERROR_INSUFFICIENT_BUFFER;
            leave;

        }
        lstrcpy (pBuffer,((PSTRING_NODE)(stringTable->Data + StringId))->String);

        *pBufSize = len;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = ERROR_INVALID_DATA;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }

    if(status == NO_ERROR) {
        //
        // if success, return TRUE without modifying error code
        //
        return TRUE;
    }
    //
    // if error, we may be interested in cause
    //
    // SetLastError(status); // BUGBUG!!! (jamiehun) left disabled till I know this is safe to do
    return FALSE;
}



VOID
StringTableTrim(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Free any memory currently allocated for the string table
    but not currently used.

    This is useful after all strings have been added to a string table
    because the string table grows by a fixed block size as it's being built.

Arguments:

    StringTable - supplies a string table handle returned from
        a call to StringTableInitialize().

Return Value:

    None.

--*/

{
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        pStringTableTrim(StringTable);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }
}

VOID
pStringTableTrim(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Free any memory currently allocated for the string table
    but not currently used.

    This is useful after all strings have been added to a string table
    because the string table grows by a fixed block size as it's being built.

    THIS ROUTINE DOES NOT DO LOCKING!

Arguments:

    StringTable - supplies a string table handle returned from
        a call to StringTableInitialize().

Return Value:

    None.

--*/

{
    PSTRING_TABLE stringTable = StringTable;
    PVOID p;

    //
    // If the realloc failed the original block is not freed,
    // so we don't really care.
    //

    if(p = MyRealloc(stringTable->Data, stringTable->DataSize)) {
        stringTable->Data = p;
        stringTable->BufferSize = stringTable->DataSize;
    }
}


PVOID
StringTableInitialize(
    VOID
    )

/*++

Routine Description:

    Create and initialize a string table.

Arguments:

    None.

Return Value:

    NULL if the string table could not be created (out of memory).
    Otherwise returns an opaque value that references the string
    table in other StringTable calls.

Remarks:

    This routine returns a string table with synchronization locks
    required by all public StringTable APIs.  If the string table
    is to be enclosed in a structure that has its own locking
    (e.g., HINF, HDEVINFO), then the private version of this API
    may be called, which will not create locks for the string table.

--*/

{
    PSTRING_TABLE StringTable;

    if(StringTable = (PSTRING_TABLE)pStringTableInitialize(0)) {

        if(InitializeSynchronizedAccess(&StringTable->Lock)) {
            return StringTable;
        }

        pStringTableDestroy(StringTable);
    }

    return NULL;
}


PVOID
StringTableInitializeEx(
    IN UINT ExtraDataSize,  OPTIONAL
    IN UINT Reserved
    )

/*++

Routine Description:

    Create and initialize a string table, where each string can have
    some arbitrary data associated with it.

Arguments:

    ExtraDataSize - supplies maximum size of arbitrary data that can be
        associated with strings in the string table that will be created.

    Reserved - unused, must be 0.

Return Value:

    NULL if the string table could not be created (out of memory).
    Otherwise returns an opaque value that references the string
    table in other StringTable calls.

Remarks:

    This routine returns a string table with synchronization locks
    required by all public StringTable APIs.  If the string table
    is to be enclosed in a structure that has its own locking
    (e.g., HINF, HDEVINFO), then the private version of this API
    may be called, which will not create locks for the string table.

--*/

{
    PSTRING_TABLE StringTable;

    if(Reserved) {
        return(NULL);
    }

    if(StringTable = (PSTRING_TABLE)pStringTableInitialize(ExtraDataSize)) {

        if(InitializeSynchronizedAccess(&StringTable->Lock)) {
            return StringTable;
        }

        pStringTableDestroy(StringTable);
    }

    return NULL;
}


PVOID
pStringTableInitialize(
    IN UINT ExtraDataSize   OPTIONAL
    )

/*++

Routine Description:

    Create and initialize a string table. Each string can optionally have
    some arbitrary data associated with it.

    THIS ROUTINE DOES NOT INITIALIZE STRING TABLE SYNCHRONIZATION LOCKS!

Arguments:

    ExtraDataSize - supplies maximum size of arbitrary data that can be
        associated with strings in the string table that will be created.

Return Value:

    NULL if the string table could not be created (out of memory).
    Otherwise returns an opaque value that references the string
    table in other StringTable calls.

Remarks:

    The string table returned from this API may not be used as-is with the
    public StringTable APIs--it must have its synchronization locks initialized
    by the public form of this API.

--*/

{
    UINT u;
    PSTRING_TABLE stringTable;

    //
    // Allocate a string table
    //
    if(stringTable = MyMalloc(sizeof(STRING_TABLE))) {

        ZeroMemory(stringTable,sizeof(STRING_TABLE));

        stringTable->ExtraDataSize = ExtraDataSize;
        stringTable->Locale = GetThreadLocale();
        if(PRIMARYLANGID(LANGIDFROMLCID(stringTable->Locale)) == LANG_TURKISH) {
            //
            // Turkish has a problem with i and dotted i's.
            // Do comparison in English.
            //
            stringTable->Locale = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
        }

        //
        // Allocate space for the string table data.
        //
        if(stringTable->Data = MyMalloc(STRING_TABLE_INITIAL_SIZE)) {

            stringTable->BufferSize = STRING_TABLE_INITIAL_SIZE;

            //
            // Initialize the hash table
            //
            for(u=0; u<HASH_BUCKET_COUNT; u++) {
                ((PLONG)(stringTable->Data))[u] = -1;
            }

            //
            // Set the DataSize to the size of the StringNodeOffset list, so
            // we'll start adding new strings after it.
            //
            stringTable->DataSize = HASH_BUCKET_COUNT * sizeof(LONG);

            return(stringTable);
        }

        MyFree(stringTable);
    }

    return(NULL);
}


VOID
StringTableDestroy(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Destroy a string table, freeing all resources it uses.

Arguments:

    StringTable - supplies a string table handle returned from
        a call to StringTableInitialize().

Return Value:

    None.

--*/

{
    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }

        DestroySynchronizedAccess(&(((PSTRING_TABLE)StringTable)->Lock));

        pStringTableDestroy(StringTable);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
    }
}


VOID
pStringTableDestroy(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Destroy a string table, freeing all resources it uses.
    THIS ROUTINE DOES NOT DO LOCKING!

Arguments:

    StringTable - supplies a string table handle returned from
        a call to StringTableInitialize().

Return Value:

    None.

--*/

{
    MyFree(((PSTRING_TABLE)StringTable)->Data);
    MyFree(StringTable);
}


PVOID
StringTableDuplicate(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Create an independent duplicate of a string table.

Arguments:

    StringTable - supplies a string table handle of string table to duplicate.

Return Value:

    Handle for new string table, NULL if out of memory.

--*/

{
    PSTRING_TABLE New = NULL;
    BOOL locked = FALSE;

    try {
        if(!LockTable((PSTRING_TABLE)StringTable)) {
            leave;
        }
        locked = TRUE;

        if(New = (PSTRING_TABLE)pStringTableDuplicate(StringTable)) {

            if(!InitializeSynchronizedAccess(&New->Lock)) {
                pStringTableDestroy(New);
                New = NULL;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        New = NULL;
    }
    if (locked) {
        UnlockTable((PSTRING_TABLE)StringTable);
    }

    return New;
}


PVOID
pStringTableDuplicate(
    IN PVOID StringTable
    )

/*++

Routine Description:

    Create an independent duplicate of a string table.
    THIS ROUTINE DOES NOT DO LOCKING!

Arguments:

    StringTable - supplies a string table handle of string table to duplicate.

Return Value:

    Handle for new string table, NULL if out of memory or buffer copy failure.

Remarks:

    This routine does not initialize synchronization locks for the duplicate--these
    fields are initialized to NULL.

--*/

{
    PSTRING_TABLE New;
    PSTRING_TABLE stringTable = StringTable;
    BOOL Success;

    if(New = MyMalloc(sizeof(STRING_TABLE))) {

        CopyMemory(New,StringTable,sizeof(STRING_TABLE));

        //
        // Allocate space for the string table data.
        //
        if(New->Data = MyMalloc(stringTable->DataSize)) {
            //
            // Surround memory copy in try/except, since we may be dealing with
            // a string table contained in a PNF, in which case the buffer is
            // in a memory-mapped file.
            //
            Success = TRUE; // assume success unless we get an inpage error...
            try {
                CopyMemory(New->Data, stringTable->Data, stringTable->DataSize);
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Success = FALSE;
            }

            if(Success) {
                New->BufferSize = New->DataSize;
                ZeroMemory(&New->Lock, sizeof(MYLOCK));
                return New;
            }

            MyFree(New->Data);
        }

        MyFree(New);
    }

    return NULL;
}


PVOID
InitializeStringTableFromPNF(
    IN PPNF_HEADER PnfHeader,
    IN LCID        Locale
    )
{
    PSTRING_TABLE StringTable;
    BOOL WasLoaded = TRUE;

    //
    // Allocate a string table
    //
    if(!(StringTable = MyMalloc(sizeof(STRING_TABLE)))) {
        return NULL;
    }

    try {

        StringTable->Data = (PUCHAR)PnfHeader + PnfHeader->StringTableBlockOffset;

        StringTable->DataSize = StringTable->BufferSize = PnfHeader->StringTableBlockSize;

        //
        // Clear the Lock structure, because PNF string tables can only be accessed
        // internally, via their associated INF.
        //
        StringTable->Lock.Handles[0] = StringTable->Lock.Handles[1] = NULL;

        StringTable->ExtraDataSize = 0;

        StringTable->Locale = Locale;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        WasLoaded = FALSE;
    }

    if(WasLoaded) {
        return StringTable;
    } else {
        MyFree(StringTable);
        return NULL;
    }
}


DWORD
pStringTableGetDataBlock(
    IN  PVOID  StringTable,
    OUT PVOID *StringTableBlock
    )
{
    *StringTableBlock = (PVOID)(((PSTRING_TABLE)StringTable)->Data);

    return ((PSTRING_TABLE)StringTable)->DataSize;
}

