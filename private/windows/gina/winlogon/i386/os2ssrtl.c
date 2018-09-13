/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    os2ssrtl.c

Abstract:

    This module contains CONFIG.SYS related parsing routines, and some other
    routines needed to support os/2ss migration.

Author:

    Ofer Porat (oferp) 8-Nov-1992

Environment:

    User Mode Only

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define IF_OD2_DEBUG(x)                         // permanently enable debug prints

#define WBLANK(ch)  ((ch) == L' ' || (ch) == L'\t')

//
// used for text processing in Or2ReplacePathByPath()
//
typedef struct _INDEX_T {

    UNICODE_STRING Src;
    UNICODE_STRING Dest;

} INDEX_T, *PINDEX_T;



//
// This routine skips over white space in a unicode string
// either forward or backward.
// Str -- address of pointer to string.
// Direction -- either FWD or BWD.
//

VOID
Or2SkipWWS(
    IN OUT PWSTR *Str,
    IN LONG Direction
    )
{
    PWSTR q = *Str;

    while (WBLANK(*q)) {
        q += Direction;
    }
    *Str = q;
}


//
// This routine converts a null terminated unicode string to upper case.
// It is similar to wcsupr().  It exists because I'm not sure wcsupr()
// actually uses the Unicode convention.
//
// Str -- string to convert.
//

VOID
Or2UnicodeStrupr(
    IN OUT PWSTR Str
    )
{
    while (*Str != UNICODE_NULL) {
        *Str = RtlUpcaseUnicodeChar(*Str);
        Str++;
    }
}


//
// This routine compares 2 null-terminated unicode strings.  The comparison
// has a count limiting the number of chars compared, and the comparison is
// case insensitive.  It is similar to wcsnicmp().  It exists because I'm not
// sure wcsnicmp() actually uses the Unicode convention.
//
// Str1, Str2 -- strings to compare.
// Count -- max # of chars to compare.
// return value -- TRUE if they're equal to within Count characters.  FALSE otherwise.
//

BOOLEAN
Or2UnicodeEqualCI(
    IN PWSTR Str1,
    IN PWSTR Str2,
    IN ULONG Count
    )
{
    while (Count != 0) {

        if (*Str1 == UNICODE_NULL) {

            if (*Str2 == UNICODE_NULL) {
                return(TRUE);
            }

            return(FALSE);
        }

        if (RtlUpcaseUnicodeChar(*Str1) != RtlUpcaseUnicodeChar(*Str2)) {

            return(FALSE);
        }

        Str1++; Str2++; Count--;
    }
    return(TRUE);
}


//
// This routine appends a source path list to a destination path list.  multiple occurences
// of the same path are removed.
//
// HeapHandle -- a handle to a heap for temporary allocations
// SrcPath -- a null terminated unicode string.  It must be in uppercase for elimination of
//            multiple paths to occur.  This path list is appended to DestPath.
// DestPath -- an already existing counted unicode string containing the path list to be appended
//             to.  The case is unimportant.  The result is stored back in this string.
// ExpandIt -- Should be TRUE in DestPath contains unexpanded %...% type strings.  FALSE otherwise.
// return value -- TRUE on success, FALSE otherwise.
//
// Possible Errors              Effect
// ===============              ======
// allocation failure           Does nothing, return value FALSE
// can't expand the
//   DestPath                   Does nothing, return value FALSE
// DestPath MaxLen
//   exceeded during
//   append                     Stop on the last path that fits, return value TRUE
//

BOOLEAN
Or2AppendPathToPath(
    IN PVOID HeapHandle,
    IN PWSTR SrcPath,
    IN OUT PUNICODE_STRING DestPath,
    IN BOOLEAN ExpandIt
    )
{
    WCHAR wch;
    WCHAR wch1;
    WCHAR wch2;
    PWSTR p;
    PWSTR q;
    PWSTR r;
    PWSTR t;
    USHORT l;
    USHORT addsemi;
    UNICODE_STRING Expanded;
    UNICODE_STRING Tmp;
    BOOLEAN     Found;
    NTSTATUS    Status;

    Expanded.Buffer = (PWSTR) RtlAllocateHeap(HeapHandle, 0, DestPath->MaximumLength + sizeof(WCHAR));

    if (Expanded.Buffer == NULL) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("Or2AppendPathPath: can't allocate Expanded from heap\n"));
        }
#endif
        return(FALSE);
    }

    Expanded.MaximumLength = DestPath->MaximumLength;

    if (ExpandIt) {

        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               DestPath,
                                               &Expanded,
                                               NULL);

        if (!NT_SUCCESS(Status)) {
#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("Or2AppendPathToPath: can't expand environment strings, rc = %lx\n", Status));
            }
#endif
            RtlFreeHeap(HeapHandle, 0, Expanded.Buffer);
            return(FALSE);
        }

    } else {

        RtlCopyUnicodeString(&Expanded, DestPath);
    }

    Expanded.Buffer[Expanded.Length/sizeof(WCHAR)] = UNICODE_NULL;

    Or2UnicodeStrupr(Expanded.Buffer);

    addsemi = 2;

    if (Expanded.Length == 0 ||
        Expanded.Buffer[Expanded.Length/sizeof(WCHAR) - 1] == L';') {

        addsemi = 0;
    }

    while (TRUE) {

        Or2SkipWWS(&SrcPath, FWD);

        wch = *SrcPath;

        if (wch == UNICODE_NULL) {
            break;
        }

        if (wch == L';') {
            SrcPath++;
            continue;
        }

        p = wcschr(SrcPath, L';');

        if (p == NULL) {

            p = SrcPath + wcslen(SrcPath);
            q = p - 1;

        } else {

            q = p - 1;
            p++;
        }

        Or2SkipWWS(&q, BWD);

        l = q - SrcPath + 1;

        wch = *(q+1);
        *(q+1) = UNICODE_NULL;
        t = Expanded.Buffer;
        Found = FALSE;

        while (TRUE) {

            r = wcsstr(t, SrcPath);

            if (r == NULL) {
                break;
            }

            if (r == Expanded.Buffer) {
                wch1 = L';';
            } else {
                wch1 = *(r-1);
            }

            wch2 = r[l];

            if ((wch1 == L';' || WBLANK(wch1)) &&
                (wch2 == L';' || WBLANK(wch2) || wch2 == UNICODE_NULL)) {

                Found = TRUE;
                break;
            }

            t = r + l;
        }

        *(q+1) = wch;

        if (Found) {
            SrcPath = p;
            continue;
        }

        Tmp.Buffer = SrcPath;
        Tmp.Length = l * sizeof(WCHAR);
        Tmp.MaximumLength = Tmp.Length;

        if (Expanded.Length + addsemi + Tmp.Length > Expanded.MaximumLength ||
            DestPath->Length + addsemi + Tmp.Length > DestPath->MaximumLength) {

#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("Or2AppendPathToPath: Path buffer overflow\n"));
            }
#endif
            break;      // out of string space, terminate processing.
        }

        if (addsemi) {

            RtlAppendUnicodeToString(&Expanded, L";");
            RtlAppendUnicodeToString(DestPath, L";");

        } else {

            addsemi = 2;
        }

        RtlAppendUnicodeStringToString(&Expanded, &Tmp);
        RtlAppendUnicodeStringToString(DestPath, &Tmp);

        Expanded.Buffer[Expanded.Length/sizeof(WCHAR)] = UNICODE_NULL;

        SrcPath = p;
    }

    DestPath->Buffer[DestPath->Length / sizeof(WCHAR)] = UNICODE_NULL;

    RtlFreeHeap(HeapHandle, 0, Expanded.Buffer);
    return(TRUE);
}


//
// This routine takes a counted unicode string containing a path-list with %...% type strings in it.
// It prepares a pool of characters containing the expansions of paths with %...% strings in them.
// Also prepared is an index table.  Each entry in the index table contains a counted string pointing
// to the point in DestPath where the unexpanded path is, and a counted string pointing to the the pool
// where the expanded path is.
//
// SrcPath -- a counted unicode string containing the path-list to generate an index for.
// Pool -- a buffer containing space for SrcPath.MaximumLength wide characters.  This is where
//         the expanded strings will be stored.
// Ind -- a buffer containing enough space to allocate as many index entries as necessary to index
//        the path list.  One way to know how many may be necessary is to count the # of semicolons in
//        SrcPath.
// IndSize -- will contain the number of entries placed in Ind on exit.
//
// Possible Errors              Effect
// ===============              ======
// SrcPath.MaximumLength
// characters are not
// enough to store the
// pool of expanded env
// strings.                     Stops after the last expanded env string that fits in the pool.
//

static
VOID
Or2IndexPath(
    IN PUNICODE_STRING SrcPath,
    OUT PWSTR Pool,
    OUT PINDEX_T Ind,
    OUT PULONG IndSize
    )
{
    PWSTR CurPool = Pool;
    USHORT PoolQuota = SrcPath->MaximumLength;
    ULONG IndEx = 0;
    PWCHAR wp = SrcPath->Buffer;
    PWCHAR wq, wr;
    BOOLEAN HasPercent;
    NTSTATUS Status;

    for (; ; ) {

        Or2SkipWWS(&wp, FWD);

        if (*wp == UNICODE_NULL) {
            break;
        }

        HasPercent = FALSE;

        wq = wp;

        while (*wq != L';' && *wq != UNICODE_NULL) {
            if (*wq == L'%') {
                HasPercent = TRUE;
            }
            wq++;
        }

        if (*wq == L';') {
            wr = wq + 1;
        } else {
            wr = wq;
        }

        if (!HasPercent) {
            wp = wr;
            continue;
        }

        wq--;

        Or2SkipWWS(&wq, BWD);

        Ind[IndEx].Dest.Buffer = wp;
        Ind[IndEx].Dest.Length = (wq - wp + 1) * sizeof(WCHAR);
        Ind[IndEx].Dest.MaximumLength = Ind[IndEx].Dest.Length;

        wp = wr;

        Ind[IndEx].Src.Buffer = CurPool;
        Ind[IndEx].Src.MaximumLength = PoolQuota;

        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               &Ind[IndEx].Dest,
                                               &Ind[IndEx].Src,
                                               NULL);
        if (!NT_SUCCESS(Status)) {
#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("Or2IndexPath: can't expand environment strings, rc = %lx\n", Status));
            }
#endif
            break;
        }

        PoolQuota -= Ind[IndEx].Src.Length;
        CurPool += Ind[IndEx].Src.Length / sizeof(WCHAR);

        Ind[IndEx].Src.MaximumLength = Ind[IndEx].Src.Length;

        IndEx++;
    }

    *IndSize = IndEx;
}


//
// This routine takes a destination path-list and a source path list.  The destination path-list
// is completely replaced with the source path list.  The destination list may contain %...% strings.
// The source list is not expected to contain %...% strings.  While the replacement is done, any
// paths in the source which already exist in the destination in a form containing %...% strings, are
// retained with their original %...% form.
//
// HeapHandle -- a handle to a heap for temporary allocations
// SrcPath -- a null terminated unicode string containing the replacing path-list.
// DestPath -- an already existing counted unicode string containing the path list to be replaced
//             to.  The result is stored back in this string.
// return value -- TRUE on success, FALSE otherwise.
//
// Possible Errors              Effect
// ===============              ======
// allocation failure           Does nothing, return value FALSE
// DestPath MaxLen
//   exceeded during
//   replace                    Stop on the last path that fits, return value TRUE
//

BOOLEAN
Or2ReplacePathByPath(
    IN PVOID HeapHandle,
    IN PWSTR SrcPath,
    IN OUT PUNICODE_STRING DestPath
    )
{
    PINDEX_T Ind;
    PWSTR Pool;
    PWSTR p;
    PWSTR q;
    WCHAR wch;
    ULONG IndSize;
    ULONG SemiCount;
    ULONG i;
    USHORT addsemi;
    UNICODE_STRING Target;
    UNICODE_STRING Tmp;

    Target.Buffer = (PWSTR) RtlAllocateHeap(HeapHandle, 0, DestPath->MaximumLength + sizeof(WCHAR));

    if (Target.Buffer == NULL) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("Or2ReplacePathByPath: can't allocate Target from heap\n"));
        }
#endif
        return(FALSE);
    }

    Target.MaximumLength = DestPath->MaximumLength;
    Target.Length = 0;

    Pool = (PWSTR) RtlAllocateHeap(HeapHandle, 0, (ULONG) DestPath->MaximumLength);

    if (Pool == NULL) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("Or2ReplacePathByPath: can't allocate Pool from heap\n"));
        }
#endif
        RtlFreeHeap(HeapHandle, 0, Target.Buffer);
        return(FALSE);
    }

    SemiCount = 1;

    for (i = 0; i < DestPath->Length/sizeof(WCHAR); i++) {
        if (DestPath->Buffer[i] == L';') {
            SemiCount++;
        }
    }

    Ind = (PINDEX_T) RtlAllocateHeap(HeapHandle, 0, SemiCount * sizeof(INDEX_T));

    if (Ind == NULL) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("Or2ReplacePathByPath: can't allocate Ind from heap\n"));
        }
#endif
        RtlFreeHeap(HeapHandle, 0, Pool);
        RtlFreeHeap(HeapHandle, 0, Target.Buffer);
        return(FALSE);
    }

    Or2IndexPath(DestPath, Pool, Ind, &IndSize);

    addsemi = 0;

    while (TRUE) {

        Or2SkipWWS(&SrcPath, FWD);

        wch = *SrcPath;

        if (wch == UNICODE_NULL) {
            break;
        }

        if (wch == L';') {
            SrcPath++;
            continue;
        }

        p = wcschr(SrcPath, L';');

        if (p == NULL) {

            p = SrcPath + wcslen(SrcPath);
            q = p - 1;

        } else {

            q = p - 1;
            p++;
        }

        Or2SkipWWS(&q, BWD);

        Tmp.Buffer = SrcPath;
        Tmp.Length = (q - SrcPath + 1) * sizeof(WCHAR);
        Tmp.MaximumLength = Tmp.Length;

        for (i = 0; i < IndSize; i++) {

            if (RtlEqualUnicodeString(&Tmp, &Ind[i].Src, TRUE)) {

                Tmp = Ind[i].Dest;
                break;
            }
        }

        if (Target.Length + addsemi + Tmp.Length > Target.MaximumLength) {

#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("Or2ReplacePathByPath: Path buffer overflow\n"));
            }
#endif
            break;      // out of string space, terminate processing.
        }

        if (addsemi) {

            RtlAppendUnicodeToString(&Target, L";");

        } else {

            addsemi = 2;
        }

        RtlAppendUnicodeStringToString(&Target, &Tmp);

        SrcPath = p;
    }

    RtlCopyUnicodeString(DestPath, &Target);

    DestPath->Buffer[DestPath->Length/sizeof(WCHAR)] = UNICODE_NULL;

    RtlFreeHeap(HeapHandle, 0, Ind);
    RtlFreeHeap(HeapHandle, 0, Pool);
    RtlFreeHeap(HeapHandle, 0, Target.Buffer);
    return(TRUE);
}


//
// This routine appends a terminating semicolon to a path list, if that path list
// does not have a semicolon in it (in other words, if the path list contains only
// one path).
//
// Str - The string to do semicolon processing on.
//

VOID
Or2CheckSemicolon(
    IN OUT PUNICODE_STRING Str
    )
{
    USHORT i;
    BOOLEAN flag;

    if (Str->Length == 0 || Str->Length >= Str->MaximumLength) {
        return;
    }

    flag = FALSE;

    for(i = 0; i < Str->Length/ (USHORT) sizeof(WCHAR); i++) {
        if (Str->Buffer[i] == L';') {
            flag = TRUE;
            break;
        }
    }

    if (!flag) {
        Str->Buffer[i++] = L';';
        Str->Buffer[i] = UNICODE_NULL;
        Str->Length += sizeof(WCHAR);
    }
}


//
// This routine fetches a path-list type environment variable from the registry.
//
// Data - an unallocated counted unicode string to hold the result.
// HeapHandle -- a handle to a heap where we allocate from.
// MaxSiz -- The MaximumLength Data should be given.
// EnvKey -- a handle to the registry key in which the environment to be searched is.
// ValueName -- a null-terminated unicode name of the registry value to fetch.
// ExpandIt -- If TRUE, the registry value is expanded for %..% type strings before returning it.
// return value -- TRUE on success, FALSE otherwise.
//
// Possible Errors              Effect
// ===============              ======
// allocation failure           Data.Buffer = NULL, return value FALSE
// requested env variable
//   doesn't exist, or
//   can't query its value      Data is allocated and given 0 length, return value TRUE
// can't expand the
//   env variable               Data is allocated and given 0 length, return value TRUE
//

BOOLEAN
Or2GetEnvPath(
    OUT PUNICODE_STRING Data,
    IN PVOID HeapHandle,
    IN USHORT MaxSiz,
    IN HANDLE EnvKey,
    IN PWSTR ValueName,
    IN BOOLEAN ExpandIt
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION Buf;
    UNICODE_STRING ExpBuf;
    UNICODE_STRING ValueName_U;
    UNICODE_STRING tmp;
    ULONG ResultLength;
    ULONG l1, l2;
    NTSTATUS Status;

    Data->Buffer = NULL;

    l1 = ((ULONG) MaxSiz + 1) * sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    Buf = (PKEY_VALUE_PARTIAL_INFORMATION) RtlAllocateHeap(HeapHandle, 0, l1);

    if (Buf == NULL) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("Or2GetEnvPath: can't allocate Buf from heap\n"));
        }
#endif
        return(FALSE);
    }

    if (ExpandIt) {

        l2 = ((ULONG) MaxSiz + 1) * sizeof(WCHAR);

        ExpBuf.Buffer = (PWSTR) RtlAllocateHeap(HeapHandle, 0, l2);

        if (ExpBuf.Buffer == NULL) {
#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("Or2GetEnvPath: can't allocate ExpBuf from heap\n"));
            }
#endif
            RtlFreeHeap(HeapHandle, 0, Buf);
            return(FALSE);
        }

        ExpBuf.MaximumLength = MaxSiz * sizeof(WCHAR);
    }

    RtlInitUnicodeString(&ValueName_U, ValueName);
    Status = NtQueryValueKey(EnvKey,
                             &ValueName_U,
                             KeyValuePartialInformation,
                             Buf,
                             l1,
                             &ResultLength
                            );

    if (!NT_SUCCESS(Status)) {
#if DBG
        IF_OD2_DEBUG( INIT ) {
            KdPrint(("OS2SSRTL(Or2GetEnvPath): FAILED - NtQueryValueKey %lx\n", Status));
        }
#endif
        if (ExpandIt) {
            RtlFreeHeap(HeapHandle, 0, Buf);
            ExpBuf.Length = 0;
            ExpBuf.Buffer[0] = UNICODE_NULL;
            *Data = ExpBuf;
        } else {
            *((PWSTR) Buf) = UNICODE_NULL;
            Data->Buffer = (PWSTR) Buf;
            Data->Length = 0;
            Data->MaximumLength = MaxSiz * sizeof(WCHAR);
        }

        return(TRUE);
    }

    if (ExpandIt) {
        RtlInitUnicodeString(&tmp, (PWSTR) Buf->Data);

        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               &tmp,
                                               &ExpBuf,
                                               NULL);

        RtlFreeHeap(HeapHandle, 0, Buf);

        if (!NT_SUCCESS(Status)) {
#if DBG
            IF_OD2_DEBUG( INIT ) {
                KdPrint(("OS2SSRTL(Or2GetEnvPath): FAILED - RtlExpandEnvironmentStrings_U %lx\n", Status));
            }
#endif
            ExpBuf.Buffer[0] = UNICODE_NULL;
            ExpBuf.Length = 0;
            *Data = ExpBuf;
            return(TRUE);
        }

        ExpBuf.Buffer[ExpBuf.Length/sizeof(WCHAR)] = UNICODE_NULL;
        *Data = ExpBuf;

    } else {

        l2 = Buf->DataLength;
        RtlMoveMemory(Buf, Buf->Data, l2);
        Data->Buffer = (PWSTR) Buf;
        Data->Length = (USHORT) (l2 - sizeof(WCHAR));
        Data->MaximumLength = MaxSiz * sizeof(WCHAR);
    }

    return(TRUE);
}


//
// This is a general routine to parse an environment.  It loops over the lines in the environment, and
// identifies the variable in each line.  For each variable, it searches a user dispatch table, and if
// that dispatch table contains an entry for this variable, it calls the user supplied routine with the
// name and value of the variable.
//
// Environment -- The environment to process.  This is either in multi-string format, or a
//                (null|^Z)-terminated string consisting of lines separated by crlfs.
// DispatchTable -- A dispatch table used to process the environment.
// NumberOfDispatchItems -- Contains the number of entries in DispatchTable.
// DelimOption -- specifies whether Environment is a multi-string (NULL_DELIM) or a string of crlf
//                separated lines (CRLF_DELIM).
//
// Description of the DispatchTable:
// This is an array of structures of type ENVIRONMENT_DISPATCH_TABLE_ENTRY.
// Each entry specifies a particular variable which should be handled.
// an entry contains:
//   -- the name of the variable to operate on (this is case-insensitive).
//   -- a string of possible characters that serve as delimiters for the variable name (e.g. =, space, tab).
//   -- a DispatchFunction.  This function is dispatched with 6 parameters:
//        -- the index in the dispatch table which triggered this call.
//        -- a user defined pointer, see below.
//        -- a pointer to the variable name in the environment.
//        -- length of the variable name (in chars).
//        -- a pointer to the variable value in the environment.
//        -- length of the variable value (in chars).
//   -- a user defined pointer that is passed to the dispatch function.
//
// The dispatch function may do anything it likes with the information it gets, and it
// is not expected to return any value.
//
// A special value of "*" in the variable name field of an entry indicates that any variable name
// is to be processed by the DispatchFunction.  The DispatchFunction can figure out the name of
// the variable it was called for by examining its parameters.  If this option is used together
// with a Delimeters value of NULL, no delimiter will be searched for, the line will be passed
// as is with Name and Value both pointing to the beginning of the line (and ValueLen containing
// its length)
//

VOID
Or2IterateEnvironment(
    IN PWSTR Environment,
    IN ENVIRONMENT_DISPATCH_TABLE DispatchTable,
    IN ULONG NumberOfDispatchItems,
    IN ULONG DelimOption
    )
{
    PWSTR Src;
    PWSTR Tmp;
    PWSTR Eol;
    PWSTR NextLine;
    ULONG i, m;
    LONG l;
    BOOLEAN Flag;

    if (NumberOfDispatchItems == 0 || Environment == NULL || DispatchTable == NULL) {
        return;
    }

    Src = Environment;
    while (*Src != UNICODE_NULL) {
        Or2SkipWWS(&Src, FWD);

        if (DelimOption == CRLF_DELIM &&
            *Src == L'\32') {                   // ^Z terminates file on CRLF delim texts
            break;
        }

        for (i = 0; i < NumberOfDispatchItems; i++) {

            if (DispatchTable[i].VarName[0] == L'*') {

                if (DispatchTable[i].Delimiters == NULL) {

                    //
                    // No delimeter required. If the line is empty
                    // we don't process it.
                    //

                    if (*Src == UNICODE_NULL) {
                        continue;
                    }

                    if (DelimOption == CRLF_DELIM &&
                        (*Src == L'\r' || *Src == L'\n')) {
                        continue;
                    }

                    l = -1;     // process the line from start
                    break;
                }

                Tmp = Src;
                l = 0;
                while (TRUE) {
                    if (*Tmp == UNICODE_NULL ||
                        (DelimOption == CRLF_DELIM && (*Tmp == L'\r' || *Tmp == L'\n' || *Tmp == L'\32'))) {
                        Flag = FALSE;
                        break;
                    }
                    if (wcschr(DispatchTable[i].Delimiters, RtlUpcaseUnicodeChar(*Tmp)) != NULL) {
                        Flag = TRUE;
                        break;
                    }
                    Tmp++;
                    l++;
                }

                if (Flag) {
                    break;
                }
                continue;
            }

            l = wcslen(DispatchTable[i].VarName);

            if (Or2UnicodeEqualCI(Src, DispatchTable[i].VarName, (ULONG)l) &&
                Src[l] != UNICODE_NULL &&
                wcschr(DispatchTable[i].Delimiters, RtlUpcaseUnicodeChar(Src[l])) != NULL) {
                break;
            }
        }

        if (DelimOption == NULL_DELIM) {
            Eol = Src + wcslen(Src);
            NextLine = Eol + 1;
        } else {
            Eol = wcspbrk(Src, L"\r\n\32");
            if (Eol == NULL) {
                NextLine = Eol = Src + wcslen(Src);
            } else {
                NextLine = Eol;
                if (*Eol != L'\32') {
                    while (*NextLine == L'\r' || *NextLine == L'\n') {
                        NextLine++;
                    }
                }
            }
        }

        if (i == NumberOfDispatchItems || DispatchTable[i].DispatchFunction == NULL) {
            Src = NextLine;
            continue;
        }

        Tmp = Src + l + 1;
        Or2SkipWWS(&Tmp, FWD);
        if (Eol != Tmp) {
            Eol--;
            Or2SkipWWS(&Eol, BWD);
            m = (Eol - Tmp) + 1;
        } else {
            m = 0;
        }

        DispatchTable[i].DispatchFunction(i, DispatchTable[i].UserParameter, Src, l, Tmp, m);

        Src = NextLine;
    }
}


//
// Following is useful Dispatch Function for use with Or2IterateEnvironment.
// It copies the parameters passed to it into a structure of type
// ENVIRONMENT_SEARCH_RECORD.  A pointer to the structure to fill must be
// passed through the UserParameter in the Dispatch Table.
//

VOID
Or2FillInSearchRecordDispatchFunction(
    IN ULONG DispatchTableIndex,
    IN PVOID UserParameter,
    IN PWSTR Name,
    IN ULONG NameLen,
    IN PWSTR Value,
    IN ULONG ValueLen
    )
{
    PENVIRONMENT_SEARCH_RECORD p = (PENVIRONMENT_SEARCH_RECORD) UserParameter;

    p->DispatchTableIndex = DispatchTableIndex;
    p->Name = Name;
    p->NameLen = NameLen;
    p->Value = Value;
    p->ValueLen = ValueLen;
}


NTSTATUS
Or2GetFileStamps(
    IN HANDLE hFile,
    OUT PLARGE_INTEGER pTimeStamp,
    OUT PLARGE_INTEGER pSizeStamp
    )

/*++

Routine Description:

    This function reads a file's size and time of last write.

Arguments:

    hFile -- handle of file to read.
    pTimeStamp -- returns file time of last write.
    pSizeStamp -- returns file size.

Return Value:

    NT error code.  The return parameters are valid only if this is
    success.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION BasicInfo;
    FILE_STANDARD_INFORMATION StandardInfo;

    Status = NtQueryInformationFile(hFile,
                                    &IoStatus,
                                    &BasicInfo,
                                    sizeof(BasicInfo),
                                    FileBasicInformation);

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Or2GetFileStamps: Unable to query file time, rc = %lx\n", Status));
#endif
        return(Status);
    }

    *pTimeStamp = BasicInfo.LastWriteTime;

    Status = NtQueryInformationFile(hFile,
                                    &IoStatus,
                                    &StandardInfo,
                                    sizeof(StandardInfo),
                                    FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
#if DBG
        KdPrint(("Or2GetFileStamps: Unable to query file size, rc = %lx\n", Status));
#endif
        return(Status);
    }

    *pSizeStamp = StandardInfo.EndOfFile;

    return(STATUS_SUCCESS);
}

