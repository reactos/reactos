/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/path.c
 * PURPOSE:         Path and current directory functions
 * PROGRAMMERS:     Wine team
 *                  Thomas Weidenmueller
 *                  Gunnar Dalsnes
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* DEFINITONS and MACROS ******************************************************/

#define MAX_PFX_SIZE       16

#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))

#define RTL_CURDIR_IS_REMOVABLE 0x1
#define RTL_CURDIR_DROP_OLD_HANDLE 0x2
#define RTL_CURDIR_ALL_FLAGS (RTL_CURDIR_DROP_OLD_HANDLE | RTL_CURDIR_IS_REMOVABLE) // 0x3
C_ASSERT(RTL_CURDIR_ALL_FLAGS == OBJ_HANDLE_TAGBITS);


/* GLOBALS ********************************************************************/

const UNICODE_STRING DeviceRootString = RTL_CONSTANT_STRING(L"\\\\.\\");

const UNICODE_STRING RtlpDosDevicesUncPrefix = RTL_CONSTANT_STRING(L"\\??\\UNC\\");
const UNICODE_STRING RtlpWin32NtRootSlash    = RTL_CONSTANT_STRING(L"\\\\?\\");
const UNICODE_STRING RtlpDosSlashCONDevice   = RTL_CONSTANT_STRING(L"\\\\.\\CON");
const UNICODE_STRING RtlpDosDevicesPrefix    = RTL_CONSTANT_STRING(L"\\??\\");

const UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING(L"LPT");
const UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING(L"COM");
const UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING(L"PRN");
const UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING(L"AUX");
const UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING(L"CON");
const UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING(L"NUL");

const UNICODE_STRING RtlpDoubleSlashPrefix   = RTL_CONSTANT_STRING(L"\\\\");

static const UNICODE_STRING RtlpDefaultExtension = RTL_CONSTANT_STRING(L".DLL");
static const UNICODE_STRING RtlpDotLocal = RTL_CONSTANT_STRING(L".Local\\");
static const UNICODE_STRING RtlpPathDividers = RTL_CONSTANT_STRING(L"\\/");


PRTLP_CURDIR_REF RtlpCurDirRef;

/* PRIVATE FUNCTIONS **********************************************************/

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_Ustr(IN PCUNICODE_STRING PathString)
{
    PWCHAR Path;
    ULONG Chars;

    Path = PathString->Buffer;
    Chars = PathString->Length / sizeof(WCHAR);

    /* Return if there are no characters */
    if (!Chars) return RtlPathTypeRelative;

    /*
     * The algorithm is similar to RtlDetermineDosPathNameType_U but here we
     * actually check for the path length before touching the characters
     */
    if (IS_PATH_SEPARATOR(Path[0]))
    {
        if ((Chars < 2) || !(IS_PATH_SEPARATOR(Path[1]))) return RtlPathTypeRooted;                /* \x             */
        if ((Chars < 3) || ((Path[2] != L'.') && (Path[2] != L'?'))) return RtlPathTypeUncAbsolute;/* \\x            */
        if ((Chars >= 4) && (IS_PATH_SEPARATOR(Path[3]))) return RtlPathTypeLocalDevice;           /* \\.\x or \\?\x */
        if (Chars != 3) return RtlPathTypeUncAbsolute;                                             /* \\.x or \\?x   */
        return RtlPathTypeRootLocalDevice;                                                         /* \\. or \\?     */
    }
    else
    {
        if ((Chars < 2) || (Path[1] != L':')) return RtlPathTypeRelative;                          /* x              */
        if ((Chars < 3) || !(IS_PATH_SEPARATOR(Path[2]))) return RtlPathTypeDriveRelative;         /* x:             */
        return RtlPathTypeDriveAbsolute;                                                           /* x:\            */
    }
}

ULONG
NTAPI
RtlIsDosDeviceName_Ustr(IN PCUNICODE_STRING PathString)
{
    UNICODE_STRING PathCopy;
    PWCHAR Start, End;
    USHORT PathChars, ColonCount = 0;
    USHORT ReturnOffset = 0, ReturnLength, OriginalLength;
    WCHAR c;

    /* Validate the input */
    if (!PathString) return 0;

    /* Check what type of path this is */
    switch (RtlDetermineDosPathNameType_Ustr(PathString))
    {
        /* Fail for UNC or unknown paths */
        case RtlPathTypeUnknown:
        case RtlPathTypeUncAbsolute:
            return 0;

        /* Make special check for the CON device */
        case RtlPathTypeLocalDevice:
            if (RtlEqualUnicodeString(PathString, &RtlpDosSlashCONDevice, TRUE))
            {
                /* This should return 0x80006 */
                return MAKELONG(RtlpDosCONDevice.Length, DeviceRootString.Length);
            }
            return 0;

        default:
            break;
    }

    /* Make a copy of the string */
    PathCopy = *PathString;
    OriginalLength = PathString->Length;

    /* Return if there's no characters */
    PathChars = PathCopy.Length / sizeof(WCHAR);
    if (!PathChars) return 0;

    /* Check for drive path and truncate */
    if (PathCopy.Buffer[PathChars - 1] == L':')
    {
        /* Fixup the lengths */
        PathCopy.Length -= sizeof(WCHAR);
        if (!--PathChars) return 0;

        /* Remember this for later */
        ColonCount = 1;
    }

    /* Check for extension or space, and truncate */
    do
    {
        /* Stop if we hit something else than a space or period */
        c = PathCopy.Buffer[PathChars - 1];
        if ((c != L'.') && (c != L' ')) break;

        /* Fixup the lengths */
        PathCopy.Length -= sizeof(WCHAR);

        /* Remember this for later */
        ColonCount++;
    } while (--PathChars);

    /* Anything still left? */
    if (PathChars)
    {
        /* Loop from the end */
        for (End = &PathCopy.Buffer[PathChars - 1];
             End >= PathCopy.Buffer;
             --End)
        {
            /* Check if the character is a path or drive separator */
            c = *End;
            if (IS_PATH_SEPARATOR(c) || ((c == L':') && (End == PathCopy.Buffer + 1)))
            {
                /* Get the next lower case character */
                End++;
                c = RtlpDowncaseUnicodeChar(*End);

                /* Check if it's a DOS device (LPT, COM, PRN, AUX, or NUL) */
                if ((End < &PathCopy.Buffer[OriginalLength / sizeof(WCHAR)]) &&
                    ((c == L'l') || (c == L'c') || (c == L'p') || (c == L'a') || (c == L'n')))
                {
                    /* Calculate the offset */
                    ReturnOffset = (USHORT)((PCHAR)End - (PCHAR)PathCopy.Buffer);

                    /* Build the final string */
                    PathCopy.Length = OriginalLength - ReturnOffset - (ColonCount * sizeof(WCHAR));
                    PathCopy.Buffer = End;

                    /* Save new amount of chars in the path */
                    PathChars = PathCopy.Length / sizeof(WCHAR);

                    break;
                }
                else
                {
                    return 0;
                }
            }
        }

        /* Get the next lower case character and check if it's a DOS device */
        c = RtlpDowncaseUnicodeChar(*PathCopy.Buffer);
        if ((c != L'l') && (c != L'c') && (c != L'p') && (c != L'a') && (c != L'n'))
        {
            /* Not LPT, COM, PRN, AUX, or NUL */
            return 0;
        }
    }

    /* Now skip past any extra extension or drive letter characters */
    Start = PathCopy.Buffer;
    End = &Start[PathChars];
    while (Start < End)
    {
        c = *Start;
        if ((c == L'.') || (c == L':')) break;
        Start++;
    }

    /* And then go backwards to get rid of spaces */
    while ((Start > PathCopy.Buffer) && (Start[-1] == L' ')) --Start;

    /* Finally see how many characters are left, and that's our size */
    PathChars = (USHORT)(Start - PathCopy.Buffer);
    PathCopy.Length = PathChars * sizeof(WCHAR);

    /* Check if this is a COM or LPT port, which has a digit after it */
    if ((PathChars == 4) &&
        (iswdigit(PathCopy.Buffer[3]) && (PathCopy.Buffer[3] != L'0')))
    {
        /* Don't compare the number part, just check for LPT or COM */
        PathCopy.Length -= sizeof(WCHAR);
        if ((RtlEqualUnicodeString(&PathCopy, &RtlpDosLPTDevice, TRUE)) ||
            (RtlEqualUnicodeString(&PathCopy, &RtlpDosCOMDevice, TRUE)))
        {
            /* Found it */
            ReturnLength = sizeof(L"COM1") - sizeof(WCHAR);
            return MAKELONG(ReturnLength, ReturnOffset);
        }
    }
    else if ((PathChars == 3) &&
             ((RtlEqualUnicodeString(&PathCopy, &RtlpDosPRNDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosAUXDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosNULDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosCONDevice, TRUE))))
    {
        /* Otherwise this was something like AUX, NUL, PRN, or CON */
        ReturnLength = sizeof(L"AUX") - sizeof(WCHAR);
        return MAKELONG(ReturnLength, ReturnOffset);
    }

    /* Otherwise, this is not a valid DOS device */
    return 0;
}

NTSTATUS
NTAPI
RtlpCheckDeviceName(IN PUNICODE_STRING FileName,
                    IN ULONG Length,
                    OUT PBOOLEAN NameInvalid)
{
    PWCHAR Buffer;
    NTSTATUS Status;

    /* Allocate a large enough buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, FileName->Length);
    if (Buffer)
    {
        /* Assume failure */
        *NameInvalid = TRUE;

        /* Copy the filename */
        RtlCopyMemory(Buffer, FileName->Buffer, FileName->Length);

        /* And add a dot at the end */
        Buffer[Length / sizeof(WCHAR)] = L'.';
        Buffer[(Length / sizeof(WCHAR)) + 1] = UNICODE_NULL;

        /* Check if the file exists or not */
        *NameInvalid = RtlDoesFileExists_U(Buffer) ? FALSE: TRUE;

        /* Get rid of the buffer now */
        Status = RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    }
    else
    {
        /* Assume the name is ok, but fail the call */
        *NameInvalid = FALSE;
        Status = STATUS_NO_MEMORY;
    }

    /* Return the status */
    return Status;
}



/******************************************************************
 *    RtlpCollapsePath (from WINE)
 *
 * Helper for RtlGetFullPathName_U
 *
 * 1) Converts slashes into backslashes and gets rid of duplicated ones;
 * 2) Gets rid of . and .. components in the path.
 *
 * Returns the full path length without its terminating NULL character.
 */
static ULONG
RtlpCollapsePath(PWSTR Path, /* ULONG PathBufferSize, ULONG PathLength, */ ULONG mark, BOOLEAN SkipTrailingPathSeparators)
{
    PWSTR p, next;

    // FIXME: Do not suppose NULL-terminated strings!!

    SIZE_T PathLength = wcslen(Path);
    PWSTR EndBuffer = Path + PathLength; // Path + PathBufferSize / sizeof(WCHAR);
    PWSTR EndPath;

    /* Convert slashes into backslashes */
    for (p = Path; *p; p++)
    {
        if (*p == L'/') *p = L'\\';
    }

    /* Collapse duplicate backslashes */
    next = Path + max( 1, mark );
    for (p = next; *p; p++)
    {
        if (*p != L'\\' || next[-1] != L'\\') *next++ = *p;
    }
    *next = UNICODE_NULL;
    EndPath = next;

    p = Path + mark;
    while (*p)
    {
        if (*p == L'.')
        {
            switch (p[1])
            {
                case UNICODE_NULL:  /* final . */
                    if (p > Path + mark) p--;
                    *p = UNICODE_NULL;
                    EndPath = p;
                    continue;

                case L'\\': /* .\ component */
                    next = p + 2;
                    // ASSERT(EndPath - next == wcslen(next));
                    RtlMoveMemory(p, next, (EndPath - next + 1) * sizeof(WCHAR));
                    EndPath -= (next - p);
                    continue;

                case L'.':
                    if (p[2] == L'\\')  /* ..\ component */
                    {
                        next = p + 3;
                        if (p > Path + mark)
                        {
                            p--;
                            while (p > Path + mark && p[-1] != L'\\') p--;
                        }
                        // ASSERT(EndPath - next == wcslen(next));
                        RtlMoveMemory(p, next, (EndPath - next + 1) * sizeof(WCHAR));
                        EndPath -= (next - p);
                        continue;
                    }
                    else if (p[2] == UNICODE_NULL)  /* final .. */
                    {
                        if (p > Path + mark)
                        {
                            p--;
                            while (p > Path + mark && p[-1] != L'\\') p--;
                            if (p > Path + mark) p--;
                        }
                        *p = UNICODE_NULL;
                        EndPath = p;
                        continue;
                    }
                    break;
            }
        }

        /* Skip to the next component */
        while (*p && *p != L'\\') p++;
        if (*p == L'\\')
        {
            /* Remove last dot in previous dir name */
            if (p > Path + mark && p[-1] == L'.')
            {
                // ASSERT(EndPath - p == wcslen(p));
                RtlMoveMemory(p - 1, p, (EndPath - p + 1) * sizeof(WCHAR));
                EndPath--;
            }
            else
            {
                p++;
            }
        }
    }

    /* Remove trailing backslashes if needed (after the UNC part if it exists) */
    if (SkipTrailingPathSeparators)
    {
        while (p > Path + mark && IS_PATH_SEPARATOR(p[-1])) p--;
    }

    /* Remove trailing spaces and dots (for all the path) */
    while (p > Path && (p[-1] == L' ' || p[-1] == L'.')) p--;

    /*
     * Zero-out the discarded buffer zone, starting just after
     * the path string and going up to the end of the buffer.
     * It also NULL-terminate the path string.
     */
    ASSERT(EndBuffer >= p);
    RtlZeroMemory(p, (EndBuffer - p + 1) * sizeof(WCHAR));

    /* Return the real path length */
    PathLength = (p - Path);
    // ASSERT(PathLength == wcslen(Path));
    return (PathLength * sizeof(WCHAR));
}

/******************************************************************
 *    RtlpSkipUNCPrefix (from WINE)
 *
 * Helper for RtlGetFullPathName_U
 *
 * Skips the \\share\dir part of a file name and returns the new position
 * (which can point on the last backslash of "dir\").
 */
static SIZE_T
RtlpSkipUNCPrefix(PCWSTR FileNameBuffer)
{
    PCWSTR UncPath = FileNameBuffer + 2;
    DPRINT("RtlpSkipUNCPrefix(%S)\n", FileNameBuffer);

    while (*UncPath && !IS_PATH_SEPARATOR(*UncPath)) UncPath++;  /* share name */
    while (IS_PATH_SEPARATOR(*UncPath)) UncPath++;
    while (*UncPath && !IS_PATH_SEPARATOR(*UncPath)) UncPath++;  /* dir name */
    /* while (IS_PATH_SEPARATOR(*UncPath)) UncPath++; */

    return (UncPath - FileNameBuffer);
}

NTSTATUS
NTAPI
RtlpApplyLengthFunction(IN ULONG Flags,
                        IN ULONG Type,
                        IN PVOID UnicodeStringOrUnicodeStringBuffer,
                        IN NTSTATUS(NTAPI* LengthFunction)(ULONG, PUNICODE_STRING, PULONG))
{
    NTSTATUS Status;
    PUNICODE_STRING String;
    ULONG Length;

    if (Flags || UnicodeStringOrUnicodeStringBuffer == NULL || LengthFunction == NULL)
    {
        DPRINT1("ERROR: Flags=0x%x, UnicodeStringOrUnicodeStringBuffer=%p, LengthFunction=%p\n",
            Flags, UnicodeStringOrUnicodeStringBuffer, LengthFunction);
        return STATUS_INVALID_PARAMETER;
    }

    if (Type == sizeof(UNICODE_STRING))
    {
        String = (PUNICODE_STRING)UnicodeStringOrUnicodeStringBuffer;
    }
    else if (Type == sizeof(RTL_UNICODE_STRING_BUFFER))
    {
        String = &((PRTL_UNICODE_STRING_BUFFER)UnicodeStringOrUnicodeStringBuffer)->String;
    }
    else
    {
        DPRINT1("ERROR: Type = %u\n", Type);
        return STATUS_INVALID_PARAMETER;
    }

    Length = 0;
    Status = LengthFunction(0, String, &Length);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Length > UNICODE_STRING_MAX_CHARS)
        return STATUS_NAME_TOO_LONG;

    String->Length = (USHORT)(Length * sizeof(WCHAR));

    if (Type == sizeof(RTL_UNICODE_STRING_BUFFER))
    {
        String->Buffer[Length] = UNICODE_NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(IN ULONG Flags,
                                              IN PCUNICODE_STRING Path,
                                              OUT PULONG LengthOut)
{
    static const UNICODE_STRING PathDividers = RTL_CONSTANT_STRING(L"\\/");
    USHORT Position;
    RTL_PATH_TYPE PathType;

    /* All failure paths have this in common, so simplify code */
    if (LengthOut)
        *LengthOut = 0;

    if (Flags || !Path || !LengthOut)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((Path->Length / sizeof(WCHAR)) == 0)
    {
        /* Nothing to do here */
        return STATUS_SUCCESS;
    }


    PathType = RtlDetermineDosPathNameType_Ustr(Path);
    switch (PathType)
    {
    case RtlPathTypeLocalDevice:
        // Handle \\\\?\\C:\\ with the last ':' or '\\' missing:
        if (Path->Length / sizeof(WCHAR) < 7 ||
            Path->Buffer[5] != ':' ||
            !IS_PATH_SEPARATOR(Path->Buffer[6]))
        {
            return STATUS_INVALID_PARAMETER;
        }
        break;
    case RtlPathTypeRooted:
        // "\\??\\"
        break;
    case RtlPathTypeUncAbsolute:
        // "\\\\"
        break;
    case RtlPathTypeDriveAbsolute:
        // "C:\\"
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    /* Find the last path separator */
    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, Path, &PathDividers, &Position)))
        Position = 0;

    /* Is it the last char of the string? */
    if (Position && Position + sizeof(WCHAR) == Path->Length)
    {
        UNICODE_STRING Tmp = *Path;
        Tmp.Length = Position;

        /* Keep walking path separators to eliminate multiple next to eachother */
        while (Tmp.Length > sizeof(WCHAR) && IS_PATH_SEPARATOR(Tmp.Buffer[Tmp.Length / sizeof(WCHAR)]))
            Tmp.Length -= sizeof(WCHAR);

        /* Find the previous occurence */
        if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, &Tmp, &PathDividers, &Position)))
            Position = 0;
    }

    /* Simplify code by working in chars instead of bytes */
    if (Position)
        Position /= sizeof(WCHAR);

    if (Position)
    {
        // Keep walking path separators to eliminate multiple next to eachother, but ensure we leave one in place!
        while (Position > 1 && IS_PATH_SEPARATOR(Path->Buffer[Position - 1]))
            Position--;
    }

    if (Position > 0)
    {
        /* Return a length, not an index */
        *LengthOut = Position + 1;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlComputePrivatizedDllName_U(
    _In_ PUNICODE_STRING DllName,
    _Inout_ PUNICODE_STRING RealName,
    _Inout_ PUNICODE_STRING LocalName)
{
    static const UNICODE_STRING ExtensionChar = RTL_CONSTANT_STRING(L".");

    USHORT Position;
    UNICODE_STRING ImagePathName, DllNameOnly, CopyRealName, CopyLocalName;
    BOOLEAN HasExtension;
    ULONG RequiredSize;
    NTSTATUS Status;
    C_ASSERT(sizeof(UNICODE_NULL) == sizeof(WCHAR));

    CopyRealName = *RealName;
    CopyLocalName = *LocalName;


    /* Get the image path */
    ImagePathName = RtlGetCurrentPeb()->ProcessParameters->ImagePathName;

    /* Check if it's not normalized */
    if (!(RtlGetCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED))
    {
        /* Normalize it */
        ImagePathName.Buffer = (PWSTR)((ULONG_PTR)ImagePathName.Buffer + (ULONG_PTR)RtlGetCurrentPeb()->ProcessParameters);
    }


    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                               DllName, &RtlpPathDividers, &Position)))
    {
        DllNameOnly = *DllName;
    }
    else
    {
        /* Just keep the dll name, ignore path components */
        Position += sizeof(WCHAR);
        DllNameOnly.Buffer = DllName->Buffer + Position / sizeof(WCHAR);
        DllNameOnly.Length = DllName->Length - Position;
        DllNameOnly.MaximumLength = DllName->MaximumLength - Position;
    }

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                               &DllNameOnly, &ExtensionChar, &Position)))
    {
        Position = 0;
    }

    HasExtension = Position > 1;

    /* First we create the c:\path\processname.exe.Local\something.dll path */
    RequiredSize = ImagePathName.Length + RtlpDotLocal.Length + DllNameOnly.Length +
        (HasExtension ? 0 : RtlpDefaultExtension.Length) + sizeof(UNICODE_NULL);

    /* This is not going to work out */
    if (RequiredSize > UNICODE_STRING_MAX_BYTES)
        return STATUS_NAME_TOO_LONG;

    /* We need something extra */
    if (RequiredSize > CopyLocalName.MaximumLength)
    {
        CopyLocalName.Buffer = RtlpAllocateStringMemory(RequiredSize, TAG_USTR);
        if (CopyLocalName.Buffer == NULL)
            return STATUS_NO_MEMORY;
        CopyLocalName.MaximumLength = RequiredSize;
    }
    /* Now build the entire path */
    CopyLocalName.Length = 0;
    Status = RtlAppendUnicodeStringToString(&CopyLocalName, &ImagePathName);
    ASSERT(NT_SUCCESS(Status));
    if (NT_SUCCESS(Status))
    {
        Status = RtlAppendUnicodeStringToString(&CopyLocalName, &RtlpDotLocal);
        ASSERT(NT_SUCCESS(Status));
    }
    if (NT_SUCCESS(Status))
    {
        Status = RtlAppendUnicodeStringToString(&CopyLocalName, &DllNameOnly);
        ASSERT(NT_SUCCESS(Status));
    }
    /* Do we need to append an extension? */
    if (NT_SUCCESS(Status) && !HasExtension)
    {
        Status = RtlAppendUnicodeStringToString(&CopyLocalName, &RtlpDefaultExtension);
        ASSERT(NT_SUCCESS(Status));
    }

    if (NT_SUCCESS(Status))
    {
        /* then we create the c:\path\something.dll path */
        if (NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                                  &ImagePathName, &RtlpPathDividers, &Position)))
        {
            ImagePathName.Length = Position + sizeof(WCHAR);
        }

        RequiredSize = ImagePathName.Length + DllNameOnly.Length +
            (HasExtension ? 0 : RtlpDefaultExtension.Length) + sizeof(UNICODE_NULL);

        if (RequiredSize >= UNICODE_STRING_MAX_BYTES)
        {
            Status = STATUS_NAME_TOO_LONG;
        }
        else
        {
            if (RequiredSize > CopyRealName.MaximumLength)
            {
                CopyRealName.Buffer = RtlpAllocateStringMemory(RequiredSize, TAG_USTR);
                if (CopyRealName.Buffer == NULL)
                    Status = STATUS_NO_MEMORY;
                CopyRealName.MaximumLength = RequiredSize;
            }
            CopyRealName.Length = 0;
            if (NT_SUCCESS(Status))
            {
                Status = RtlAppendUnicodeStringToString(&CopyRealName, &ImagePathName);
                ASSERT(NT_SUCCESS(Status));
            }
            if (NT_SUCCESS(Status))
            {
                Status = RtlAppendUnicodeStringToString(&CopyRealName, &DllNameOnly);
                ASSERT(NT_SUCCESS(Status));
            }
            if (NT_SUCCESS(Status) && !HasExtension)
            {
                Status = RtlAppendUnicodeStringToString(&CopyRealName, &RtlpDefaultExtension);
                ASSERT(NT_SUCCESS(Status));
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        if (CopyRealName.Buffer && CopyRealName.Buffer != RealName->Buffer)
            RtlpFreeStringMemory(CopyRealName.Buffer, TAG_USTR);
        if (CopyLocalName.Buffer && CopyLocalName.Buffer != LocalName->Buffer)
            RtlpFreeStringMemory(CopyLocalName.Buffer, TAG_USTR);
        return Status;
    }

    *RealName = CopyRealName;
    *LocalName = CopyLocalName;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
RtlGetFullPathName_Ustr(
    _In_ PUNICODE_STRING FileName,
    _In_ ULONG Size,
    _Out_z_bytecap_(Size) PWSTR Buffer,
    _Out_opt_ PCWSTR *ShortName,
    _Out_opt_ PBOOLEAN InvalidName,
    _Out_ RTL_PATH_TYPE *PathType)
{
    NTSTATUS Status;
    PWCHAR FileNameBuffer;
    ULONG FileNameLength, FileNameChars, DosLength, DosLengthOffset, FullLength;
    BOOLEAN SkipTrailingPathSeparators;
    WCHAR c;


    ULONG               reqsize = 0;
    PCWSTR              ptr;

    PCUNICODE_STRING    CurDirName;
    UNICODE_STRING      EnvVarName, EnvVarValue;
    WCHAR EnvVarNameBuffer[4];

    ULONG  PrefixCut    = 0;    // Where the path really starts (after the skipped prefix)
    PWCHAR Prefix       = NULL; // pointer to the string to be inserted as the new path prefix
    ULONG  PrefixLength = 0;
    PWCHAR Source;
    ULONG  SourceLength;


    /* For now, assume the name is valid */
    DPRINT("Filename: %wZ\n", FileName);
    DPRINT("Size and buffer: %lx %p\n", Size, Buffer);
    if (InvalidName) *InvalidName = FALSE;

    /* Handle initial path type and failure case */
    *PathType = RtlPathTypeUnknown;
    if ((FileName->Length == 0) || (FileName->Buffer[0] == UNICODE_NULL)) return 0;

    /* Break filename into component parts */
    FileNameBuffer = FileName->Buffer;
    FileNameLength = FileName->Length;
    FileNameChars  = FileNameLength / sizeof(WCHAR);

    /* Kill trailing spaces */
    c = FileNameBuffer[FileNameChars - 1];
    while ((FileNameLength != 0) && (c == L' '))
    {
        /* Keep going, ignoring the spaces */
        FileNameLength -= sizeof(WCHAR);
        if (FileNameLength != 0) c = FileNameBuffer[FileNameLength / sizeof(WCHAR) - 1];
    }

    /* Check if anything is left */
    if (FileNameLength == 0) return 0;

    /*
     * Check whether we'll need to skip trailing path separators in the
     * computed full path name. If the original file name already contained
     * trailing separators, then we keep them in the full path name. On the
     * other hand, if the original name didn't contain any trailing separators
     * then we'll skip it in the full path name.
     */
    SkipTrailingPathSeparators = !IS_PATH_SEPARATOR(FileNameBuffer[FileNameChars - 1]);

    /* Check if this is a DOS name */
    DosLength = RtlIsDosDeviceName_Ustr(FileName);
    DPRINT("DOS length for filename: %lx %wZ\n", DosLength, FileName);
    if (DosLength != 0)
    {
        /* Zero out the short name */
        if (ShortName) *ShortName = NULL;

        /* See comment for RtlIsDosDeviceName_Ustr if this is confusing... */
        DosLengthOffset = HIWORD(DosLength);
        DosLength       = LOWORD(DosLength);

        /* Do we have a DOS length, and does the caller want validity? */
        if (InvalidName && (DosLengthOffset != 0))
        {
            /* Do the check */
            Status = RtlpCheckDeviceName(FileName, DosLengthOffset, InvalidName);

            /* If the check failed, or the name is invalid, fail here */
            if (!NT_SUCCESS(Status)) return 0;
            if (*InvalidName) return 0;
        }

        /* Add the size of the device root and check if it fits in the size */
        FullLength = DosLength + DeviceRootString.Length;
        if (FullLength < Size)
        {
            /* Add the device string */
            RtlMoveMemory(Buffer, DeviceRootString.Buffer, DeviceRootString.Length);

            /* Now add the DOS device name */
            RtlMoveMemory((PCHAR)Buffer + DeviceRootString.Length,
                          (PCHAR)FileNameBuffer + DosLengthOffset,
                          DosLength);

            /* Null terminate */
            *(PWCHAR)((ULONG_PTR)Buffer + FullLength) = UNICODE_NULL;
            return FullLength;
        }

        /* Otherwise, there's no space, so return the buffer size needed */
        if ((FullLength + sizeof(UNICODE_NULL)) > UNICODE_STRING_MAX_BYTES) return 0;
        return FullLength + sizeof(UNICODE_NULL);
    }

    /* Zero-out the destination buffer. FileName must be different from Buffer */
    RtlZeroMemory(Buffer, Size);

    /* Get the path type */
    *PathType = RtlDetermineDosPathNameType_U(FileNameBuffer);



    /**********************************************
     **    CODE REWRITING IS HAPPENING THERE     **
     **********************************************/
    Source       = FileNameBuffer;
    SourceLength = FileNameLength;
    EnvVarValue.Buffer = NULL;

    /* Lock the PEB to get the current directory */
    RtlAcquirePebLock();
    CurDirName = &NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath;

    switch (*PathType)
    {
        case RtlPathTypeUncAbsolute:        /* \\foo   */
        {
            PrefixCut = RtlpSkipUNCPrefix(FileNameBuffer);
            break;
        }

        case RtlPathTypeLocalDevice:        /* \\.\foo */
        {
            PrefixCut = 4;
            break;
        }

        case RtlPathTypeDriveAbsolute:      /* c:\foo  */
        {
            ASSERT(FileNameBuffer[1] == L':');
            ASSERT(IS_PATH_SEPARATOR(FileNameBuffer[2]));

            // FileNameBuffer[0] = RtlpUpcaseUnicodeChar(FileNameBuffer[0]);
            Prefix = FileNameBuffer;
            PrefixLength = 3 * sizeof(WCHAR);
            Source += 3;
            SourceLength -= 3 * sizeof(WCHAR);

            PrefixCut = 3;
            break;
        }

        case RtlPathTypeDriveRelative:      /* c:foo   */
        {
            WCHAR CurDrive, NewDrive;

            Source += 2;
            SourceLength -= 2 * sizeof(WCHAR);

            CurDrive = RtlpUpcaseUnicodeChar(CurDirName->Buffer[0]);
            NewDrive = RtlpUpcaseUnicodeChar(FileNameBuffer[0]);

            if ((NewDrive != CurDrive) || CurDirName->Buffer[1] != L':')
            {
                EnvVarNameBuffer[0] = L'=';
                EnvVarNameBuffer[1] = NewDrive;
                EnvVarNameBuffer[2] = L':';
                EnvVarNameBuffer[3] = UNICODE_NULL;

                EnvVarName.Length = 3 * sizeof(WCHAR);
                EnvVarName.MaximumLength = EnvVarName.Length + sizeof(WCHAR);
                EnvVarName.Buffer = EnvVarNameBuffer;

                // FIXME: Is it possible to use the user-given buffer ?
                // RtlInitEmptyUnicodeString(&EnvVarValue, NULL, Size);
                EnvVarValue.Length = 0;
                EnvVarValue.MaximumLength = (USHORT)Size;
                EnvVarValue.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
                if (EnvVarValue.Buffer == NULL)
                {
                    Prefix       = NULL;
                    PrefixLength = 0;
                    goto Quit;
                }

                Status = RtlQueryEnvironmentVariable_U(NULL, &EnvVarName, &EnvVarValue);
                switch (Status)
                {
                    case STATUS_SUCCESS:
                        /*
                         * (From Wine)
                         * FIXME: Win2k seems to check that the environment
                         * variable actually points to an existing directory.
                         * If not, root of the drive is used (this seems also
                         * to be the only place in RtlGetFullPathName that the
                         * existence of a part of a path is checked).
                         */
                        EnvVarValue.Buffer[EnvVarValue.Length / sizeof(WCHAR)] = L'\\';
                        Prefix       = EnvVarValue.Buffer;
                        PrefixLength = EnvVarValue.Length + sizeof(WCHAR); /* Append trailing '\\' */
                        break;

                    case STATUS_BUFFER_TOO_SMALL:
                        reqsize = EnvVarValue.Length + SourceLength + sizeof(UNICODE_NULL);
                        goto Quit;

                    default:
                        DPRINT1("RtlQueryEnvironmentVariable_U(\"%wZ\") returned 0x%08lx\n", &EnvVarName, Status);

                        EnvVarNameBuffer[0] = NewDrive;
                        EnvVarNameBuffer[1] = L':';
                        EnvVarNameBuffer[2] = L'\\';
                        EnvVarNameBuffer[3] = UNICODE_NULL;
                        Prefix       = EnvVarNameBuffer;
                        PrefixLength = 3 * sizeof(WCHAR);

                        RtlFreeHeap(RtlGetProcessHeap(), 0, EnvVarValue.Buffer);
                        EnvVarValue.Buffer = NULL;
                        break;
                }
                PrefixCut = 3;
                break;
            }
            /* Fall through */
            DPRINT("RtlPathTypeDriveRelative - Using fall-through to RtlPathTypeRelative\n");
        }

        case RtlPathTypeRelative:           /* foo     */
        {
            Prefix       = CurDirName->Buffer;
            PrefixLength = CurDirName->Length;
            if (CurDirName->Buffer[1] != L':')
            {
                PrefixCut = RtlpSkipUNCPrefix(CurDirName->Buffer);
            }
            else
            {
                PrefixCut = 3;
            }
            break;
        }

        case RtlPathTypeRooted:             /* \xxx    */
        {
            if (CurDirName->Buffer[1] == L':')
            {
                // The path starts with "C:\"
                ASSERT(CurDirName->Buffer[1] == L':');
                ASSERT(IS_PATH_SEPARATOR(CurDirName->Buffer[2]));

                Prefix = CurDirName->Buffer;
                PrefixLength = 3 * sizeof(WCHAR); // Skip "C:\"

                PrefixCut = 3;      // Source index location incremented of + 3
            }
            else
            {
                PrefixCut = RtlpSkipUNCPrefix(CurDirName->Buffer);
                PrefixLength = PrefixCut * sizeof(WCHAR);
                Prefix = CurDirName->Buffer;
            }
            break;
        }

        case RtlPathTypeRootLocalDevice:    /* \\.     */
        {
            Prefix       = DeviceRootString.Buffer;
            PrefixLength = DeviceRootString.Length;
            Source += 3;
            SourceLength -= 3 * sizeof(WCHAR);

            PrefixCut = 4;
            break;
        }

        case RtlPathTypeUnknown:
            goto Quit;
    }

    /* Do we have enough space for storing the full path? */
    reqsize = PrefixLength;
    if (reqsize + SourceLength + sizeof(WCHAR) > Size)
    {
        /* Not enough space, return needed size (including terminating '\0') */
        reqsize += SourceLength + sizeof(WCHAR);
        goto Quit;
    }

    /*
     * Build the full path
     */
    /* Copy the path's prefix */
    if (PrefixLength) RtlMoveMemory(Buffer, Prefix, PrefixLength);
    /* Copy the remaining part of the path */
    RtlMoveMemory(Buffer + PrefixLength / sizeof(WCHAR), Source, SourceLength + sizeof(WCHAR));

    /* Some cleanup */
    Prefix = NULL;
    if (EnvVarValue.Buffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EnvVarValue.Buffer);
        EnvVarValue.Buffer = NULL;
    }

    /*
     * Finally, put the path in canonical form (remove redundant . and ..,
     * (back)slashes...) and retrieve the length of the full path name
     * (without its terminating null character) (in chars).
     */
    reqsize = RtlpCollapsePath(Buffer, /* Size, reqsize, */ PrefixCut, SkipTrailingPathSeparators);

    /* Find the file part, which is present after the last path separator */
    if (ShortName)
    {
        ptr = wcsrchr(Buffer, L'\\');
        if (ptr) ++ptr; // Skip it

        /*
         * For UNC paths, the file part is after the \\share\dir part of the path.
         */
        PrefixCut = (*PathType == RtlPathTypeUncAbsolute ? PrefixCut : 3);

        if (ptr && *ptr && (ptr >= Buffer + PrefixCut))
        {
            *ShortName = ptr;
        }
        else
        {
            /* Zero-out the short name */
            *ShortName = NULL;
        }
    }

Quit:
    /* Release PEB lock */
    RtlReleasePebLock();

    return reqsize;
}

NTSTATUS
NTAPI
RtlpWin32NTNameToNtPathName_U(IN PUNICODE_STRING DosPath,
                              OUT PUNICODE_STRING NtPath,
                              OUT PCWSTR *PartName,
                              OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    ULONG DosLength;
    PWSTR NewBuffer, p;

    /* Validate the input */
    if (!DosPath) return STATUS_OBJECT_NAME_INVALID;

    /* Validate the DOS length */
    DosLength = DosPath->Length;
    if (DosLength >= UNICODE_STRING_MAX_BYTES) return STATUS_NAME_TOO_LONG;

    /* Make space for the new path */
    NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                DosLength + sizeof(UNICODE_NULL));
    if (!NewBuffer) return STATUS_NO_MEMORY;

    /* Copy the prefix, and then the rest of the DOS path, and NULL-terminate */
    RtlCopyMemory(NewBuffer, RtlpDosDevicesPrefix.Buffer, RtlpDosDevicesPrefix.Length);
    RtlCopyMemory((PCHAR)NewBuffer + RtlpDosDevicesPrefix.Length,
                  DosPath->Buffer + RtlpDosDevicesPrefix.Length / sizeof(WCHAR),
                  DosPath->Length - RtlpDosDevicesPrefix.Length);
    NewBuffer[DosLength / sizeof(WCHAR)] = UNICODE_NULL;

    /* Did the caller send a relative name? */
    if (RelativeName)
    {
        /* Zero initialize it */
        RtlInitEmptyUnicodeString(&RelativeName->RelativeName, NULL, 0);
        RelativeName->ContainingDirectory = NULL;
        RelativeName->CurDirRef = 0;
    }

    /* Did the caller request a partial name? */
    if (PartName)
    {
        /* Loop from the back until we find a path separator */
        p = &NewBuffer[DosLength / sizeof(WCHAR)];
        while (--p > NewBuffer)
        {
            /* We found a path separator, move past it */
            if (*p == OBJ_NAME_PATH_SEPARATOR)
            {
                ++p;
                break;
            }
        }

        /* Check whether a separator was found and if something remains */
        if ((p > NewBuffer) && *p)
        {
            /* What follows the path separator is the partial name */
            *PartName = p;
        }
        else
        {
            /* The path ends with a path separator, no partial name */
            *PartName = NULL;
        }
    }

    /* Build the final NT path string */
    NtPath->Buffer = NewBuffer;
    NtPath->Length = (USHORT)DosLength;
    NtPath->MaximumLength = (USHORT)DosLength + sizeof(UNICODE_NULL);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpDosPathNameToRelativeNtPathName_Ustr(IN BOOLEAN HaveRelative,
                                         IN PCUNICODE_STRING DosName,
                                         OUT PUNICODE_STRING NtName,
                                         OUT PCWSTR *PartName,
                                         OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    WCHAR BigBuffer[MAX_PATH + 1];
    PWCHAR PrefixBuffer, NewBuffer, Buffer;
    ULONG MaxLength, PathLength, PrefixLength, PrefixCut, LengthChars, Length;
    UNICODE_STRING CapturedDosName, PartNameString, FullPath;
    BOOLEAN QuickPath;
    RTL_PATH_TYPE InputPathType, BufferPathType;
    NTSTATUS Status;
    BOOLEAN NameInvalid;
    PCURDIR CurrentDirectory;

    /* Assume MAX_PATH for now */
    DPRINT("Relative: %lx DosName: %wZ NtName: %p, PartName: %p, RelativeName: %p\n",
            HaveRelative, DosName, NtName, PartName, RelativeName);
    MaxLength = sizeof(BigBuffer);

    /* Validate the input */
    if (!DosName) return STATUS_OBJECT_NAME_INVALID;

    /* Capture input string */
    CapturedDosName = *DosName;

    /* Check for the presence or absence of the NT prefix "\\?\" form */
    // if (!RtlPrefixUnicodeString(&RtlpWin32NtRootSlash, &CapturedDosName, FALSE))
    if ((CapturedDosName.Length <= RtlpWin32NtRootSlash.Length) ||
        (CapturedDosName.Buffer[0] != RtlpWin32NtRootSlash.Buffer[0]) ||
        (CapturedDosName.Buffer[1] != RtlpWin32NtRootSlash.Buffer[1]) ||
        (CapturedDosName.Buffer[2] != RtlpWin32NtRootSlash.Buffer[2]) ||
        (CapturedDosName.Buffer[3] != RtlpWin32NtRootSlash.Buffer[3]))
    {
        /* NT prefix not present */

        /* Quick path won't be used */
        QuickPath = FALSE;

        /* Use the static buffer */
        Buffer = BigBuffer;
        MaxLength += RtlpDosDevicesUncPrefix.Length;

        /* Allocate a buffer to hold the path */
        NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MaxLength);
        DPRINT("MaxLength: %lx\n", MaxLength);
        if (!NewBuffer) return STATUS_NO_MEMORY;
    }
    else
    {
        /* NT prefix present */

        /* Use the optimized path after acquiring the lock */
        QuickPath = TRUE;
        NewBuffer = NULL;
    }

    /* Lock the PEB and check if the quick path can be used */
    RtlAcquirePebLock();
    if (QuickPath)
    {
        /* Some simple fixups will get us the correct path */
        DPRINT("Quick path\n");
        Status = RtlpWin32NTNameToNtPathName_U(&CapturedDosName,
                                               NtName,
                                               PartName,
                                               RelativeName);

        /* Release the lock, we're done here */
        RtlReleasePebLock();
        return Status;
    }

    /* Call the main function to get the full path name and length */
    PathLength = RtlGetFullPathName_Ustr(&CapturedDosName,
                                         MAX_PATH * sizeof(WCHAR),
                                         Buffer,
                                         PartName,
                                         &NameInvalid,
                                         &InputPathType);
    if ((NameInvalid) || !(PathLength) || (PathLength > (MAX_PATH * sizeof(WCHAR))))
    {
        /* Invalid name, fail */
        DPRINT("Invalid name: %lx Path Length: %lx\n", NameInvalid, PathLength);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
        RtlReleasePebLock();
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* Start by assuming the path starts with \??\ (DOS Devices Path) */
    PrefixLength = RtlpDosDevicesPrefix.Length;
    PrefixBuffer = RtlpDosDevicesPrefix.Buffer;
    PrefixCut = 0;

    /* Check where it really is */
    BufferPathType = RtlDetermineDosPathNameType_U(Buffer);
    DPRINT("Buffer: %S Type: %lx\n", Buffer, BufferPathType);
    switch (BufferPathType)
    {
        /* It's actually a UNC path in \??\UNC\ */
        case RtlPathTypeUncAbsolute:
            PrefixLength = RtlpDosDevicesUncPrefix.Length;
            PrefixBuffer = RtlpDosDevicesUncPrefix.Buffer;
            PrefixCut = 2;
            break;

        case RtlPathTypeLocalDevice:
            /* We made a good guess, go with it but skip the \??\ */
            PrefixCut = 4;
            break;

        case RtlPathTypeDriveAbsolute:
        case RtlPathTypeDriveRelative:
        case RtlPathTypeRooted:
        case RtlPathTypeRelative:
            /* Our guess was good, roll with it */
            break;

        /* Nothing else is expected */
        default:
            ASSERT(FALSE);
    }

    /* Now copy the prefix and the buffer */
    RtlCopyMemory(NewBuffer, PrefixBuffer, PrefixLength);
    RtlCopyMemory((PCHAR)NewBuffer + PrefixLength,
                  Buffer + PrefixCut,
                  PathLength - (PrefixCut * sizeof(WCHAR)));

    /* Compute the length */
    Length = PathLength + PrefixLength - PrefixCut * sizeof(WCHAR);
    LengthChars = Length / sizeof(WCHAR);

    /* Setup the actual NT path string and terminate it */
    NtName->Buffer = NewBuffer;
    NtName->Length = (USHORT)Length;
    NtName->MaximumLength = (USHORT)MaxLength;
    NewBuffer[LengthChars] = UNICODE_NULL;
    DPRINT("New buffer: %S\n", NewBuffer);
    DPRINT("NT Name: %wZ\n", NtName);

    /* Check if a partial name was requested */
    if ((PartName) && (*PartName))
    {
        /* Convert to Unicode */
        Status = RtlInitUnicodeStringEx(&PartNameString, *PartName);
        if (NT_SUCCESS(Status))
        {
            /* Set the partial name */
            *PartName = &NewBuffer[LengthChars - (PartNameString.Length / sizeof(WCHAR))];
        }
        else
        {
            /* Fail */
            RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
            RtlReleasePebLock();
            return Status;
        }
    }

    /* Check if a relative name was asked for */
    if (RelativeName)
    {
        /* Setup the structure */
        RtlInitEmptyUnicodeString(&RelativeName->RelativeName, NULL, 0);
        RelativeName->ContainingDirectory = NULL;
        RelativeName->CurDirRef = NULL;

        /* Check if the input path itself was relative */
        if (InputPathType == RtlPathTypeRelative)
        {
            /* Get current directory */
            CurrentDirectory = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);
            if (CurrentDirectory->Handle)
            {
                Status = RtlInitUnicodeStringEx(&FullPath, Buffer);
                if (!NT_SUCCESS(Status))
                {
                    RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
                    RtlReleasePebLock();
                    return Status;
                }

                /* If current directory is bigger than full path, there's no way */
                if (CurrentDirectory->DosPath.Length > FullPath.Length)
                {
                    RtlReleasePebLock();
                    return Status;
                }

                /* File is in current directory */
                if (RtlEqualUnicodeString(&FullPath, &CurrentDirectory->DosPath, TRUE))
                {
                    /* Make relative name string */
                    RelativeName->RelativeName.Buffer = (PWSTR)((ULONG_PTR)NewBuffer + PrefixLength + FullPath.Length - PrefixCut * sizeof(WCHAR));
                    RelativeName->RelativeName.Length = (USHORT)(PathLength - FullPath.Length);
                    /* If relative name starts with \, skip it */
                    if (RelativeName->RelativeName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
                    {
                        RelativeName->RelativeName.Buffer++;
                        RelativeName->RelativeName.Length -= sizeof(WCHAR);
                    }
                    RelativeName->RelativeName.MaximumLength = RelativeName->RelativeName.Length;
                    DPRINT("RelativeName: %wZ\n", &(RelativeName->RelativeName));

                    if (!HaveRelative)
                    {
                        RelativeName->ContainingDirectory = CurrentDirectory->Handle;
                        return Status;
                    }

                    /* Give back current directory data & reference counter */
                    RelativeName->CurDirRef = RtlpCurDirRef;
                    if (RelativeName->CurDirRef)
                    {
                        InterlockedIncrement(&RtlpCurDirRef->RefCount);
                    }

                    RelativeName->ContainingDirectory = CurrentDirectory->Handle;
                }
            }
        }
    }

    /* Done */
    RtlReleasePebLock();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpDosPathNameToRelativeNtPathName_U(IN BOOLEAN HaveRelative,
                                      IN PCWSTR DosName,
                                      OUT PUNICODE_STRING NtName,
                                      OUT PCWSTR *PartName,
                                      OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    NTSTATUS Status;
    UNICODE_STRING NameString;

    /* Create the unicode name */
    Status = RtlInitUnicodeStringEx(&NameString, DosName);
    if (NT_SUCCESS(Status))
    {
        /* Call the unicode function */
        Status = RtlpDosPathNameToRelativeNtPathName_Ustr(HaveRelative,
                                                          &NameString,
                                                          NtName,
                                                          PartName,
                                                          RelativeName);
    }

    /* Return status */
    return Status;
}

BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_Ustr(IN PCUNICODE_STRING DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_Ustr(TRUE,
                                                               DosName,
                                                               NtName,
                                                               PartName,
                                                               RelativeName));
}

BOOLEAN
NTAPI
RtlDoesFileExists_UstrEx(IN PCUNICODE_STRING FileName,
                         IN BOOLEAN SucceedIfBusy)
{
    BOOLEAN Result;
    RTL_RELATIVE_NAME_U RelativeName;
    UNICODE_STRING NtPathName;
    PVOID Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    FILE_BASIC_INFORMATION BasicInformation;

    /* Get the NT Path */
    Result = RtlDosPathNameToRelativeNtPathName_Ustr(FileName,
                                                     &NtPathName,
                                                     NULL,
                                                     &RelativeName);
    if (!Result) return FALSE;

    /* Save the buffer */
    Buffer = NtPathName.Buffer;

    /* Check if we have a relative name */
    if (RelativeName.RelativeName.Length)
    {
        /* Use it */
        NtPathName = RelativeName.RelativeName;
    }
    else
    {
        /* Otherwise ignore it */
        RelativeName.ContainingDirectory = NULL;
    }

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    /* Query the attributes and free the buffer now */
    Status = ZwQueryAttributesFile(&ObjectAttributes, &BasicInformation);
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we failed because the file is in use */
        if ((Status == STATUS_SHARING_VIOLATION) ||
            (Status == STATUS_ACCESS_DENIED))
        {
            /* Check if the caller wants this to be considered OK */
            Result = SucceedIfBusy ? TRUE : FALSE;
        }
        else
        {
            /* A failure because the file didn't exist */
            Result = FALSE;
        }
    }
    else
    {
        /* The file exists */
        Result = TRUE;
    }

    /* Return the result */
    return Result;
}

BOOLEAN
NTAPI
RtlDoesFileExists_UStr(IN PUNICODE_STRING FileName)
{
    /* Call the updated API */
    return RtlDoesFileExists_UstrEx(FileName, TRUE);
}

BOOLEAN
NTAPI
RtlDoesFileExists_UEx(IN PCWSTR FileName,
                      IN BOOLEAN SucceedIfBusy)
{
    UNICODE_STRING NameString;

    /* Create the unicode name*/
    if (NT_SUCCESS(RtlInitUnicodeStringEx(&NameString, FileName)))
    {
        /* Call the unicode function */
        return RtlDoesFileExists_UstrEx(&NameString, SucceedIfBusy);
    }

    /* Fail */
    return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlReleaseRelativeName(IN PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Check if a directory reference was grabbed */
    if (RelativeName->CurDirRef)
    {
        /* Decrease reference count */
        if (!InterlockedDecrement(&RelativeName->CurDirRef->RefCount))
        {
            /* If no one uses it any longer, close handle & free */
            NtClose(RelativeName->CurDirRef->Handle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeName->CurDirRef);
        }
        RelativeName->CurDirRef = NULL;
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetLongestNtPathLength(VOID)
{
    /*
     * The longest NT path is a DOS path that actually sits on a UNC path (ie:
     * a mapped network drive), which is accessed through the DOS Global?? path.
     * This is, and has always been equal to, 269 characters, except in Wine
     * which claims this is 277. Go figure.
     */
    return MAX_PATH + RtlpDosDevicesUncPrefix.Length / sizeof(WCHAR) + sizeof(ANSI_NULL);
}

/*
 * @implemented
 * @note: the export is called RtlGetLengthWithoutTrailingPathSeperators
 *        (with a 'e' instead of a 'a' in "Seperators").
 */
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeparators(IN  ULONG Flags,
                                          IN  PCUNICODE_STRING PathString,
                                          OUT PULONG Length)
{
    ULONG NumChars;

    /* Parameters validation */
    if (Length == NULL) return STATUS_INVALID_PARAMETER;

    *Length = 0;

    if (PathString == NULL) return STATUS_INVALID_PARAMETER;

    /* No flags are supported yet */
    if (Flags != 0) return STATUS_INVALID_PARAMETER;

    NumChars = PathString->Length / sizeof(WCHAR);

    /*
     * Notice that we skip the last character, therefore:
     * - if we have: "some/path/f" we test for: "some/path/"
     * - if we have: "some/path/"  we test for: "some/path"
     * - if we have: "s" we test for: ""
     * - if we have: "" then NumChars was already zero and we aren't there
     */

    while (NumChars > 0 && IS_PATH_SEPARATOR(PathString->Buffer[NumChars - 1]))
    {
        --NumChars;
    }

    *Length = NumChars;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(IN PCWSTR Path)
{
    DPRINT("RtlDetermineDosPathNameType_U %S\n", Path);

    /* Unlike the newer RtlDetermineDosPathNameType_U we assume 4 characters */
    if (IS_PATH_SEPARATOR(Path[0]))
    {
        if (!IS_PATH_SEPARATOR(Path[1])) return RtlPathTypeRooted;                /* \x             */
        if ((Path[2] != L'.') && (Path[2] != L'?')) return RtlPathTypeUncAbsolute;/* \\x            */
        if (IS_PATH_SEPARATOR(Path[3])) return RtlPathTypeLocalDevice;            /* \\.\x or \\?\x */
        if (Path[3]) return RtlPathTypeUncAbsolute;                               /* \\.x or \\?x   */
        return RtlPathTypeRootLocalDevice;                                        /* \\. or \\?     */
    }
    else
    {
        if (!(Path[0]) || (Path[1] != L':')) return RtlPathTypeRelative;          /* x              */
        if (IS_PATH_SEPARATOR(Path[2])) return RtlPathTypeDriveAbsolute;          /* x:\            */
        return RtlPathTypeDriveRelative;                                          /* x:             */
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlIsDosDeviceName_U(IN PCWSTR Path)
{
    UNICODE_STRING PathString;
    NTSTATUS Status;

    /* Build the string */
    Status = RtlInitUnicodeStringEx(&PathString, Path);
    if (!NT_SUCCESS(Status)) return 0;

    /*
     * Returns 0 if name is not valid DOS device name, or DWORD with
     * offset in bytes to DOS device name from beginning of buffer in high word
     * and size in bytes of DOS device name in low word
     */
     return RtlIsDosDeviceName_Ustr(&PathString);
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    _In_ ULONG MaximumLength,
    _Out_bytecap_(MaximumLength) PWSTR Buffer)
{
    ULONG Length, Bytes;
    PCURDIR CurDir;
    PWSTR CurDirName;
    DPRINT("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

    /* Lock the PEB to get the current directory */
    RtlAcquirePebLock();
    CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;

    /* Get the buffer and character length */
    CurDirName = CurDir->DosPath.Buffer;
    Length = CurDir->DosPath.Length / sizeof(WCHAR);
    ASSERT((CurDirName != NULL) && (Length > 0));

    /*
     * DosPath.Buffer should always have a trailing slash. There is an assert
     * below which checks for this.
     *
     * This function either returns x:\ for a root (keeping the original buffer)
     * or it returns x:\path\foo for a directory (replacing the trailing slash
     * with a NULL.
     */
    Bytes = Length * sizeof(WCHAR);
    if ((Length <= 1) || (CurDirName[Length - 2] == L':'))
    {
        /* Check if caller does not have enough space */
        if (MaximumLength <= Bytes)
        {
            /* Call has no space for it, fail, add the trailing slash */
            RtlReleasePebLock();
            return Bytes + sizeof(OBJ_NAME_PATH_SEPARATOR);
        }
    }
    else
    {
        /* Check if caller does not have enough space */
        if (MaximumLength < Bytes)
        {
            /* Call has no space for it, fail */
            RtlReleasePebLock();
            return Bytes;
        }
    }

    /* Copy the buffer since we seem to have space */
    RtlCopyMemory(Buffer, CurDirName, Bytes);

    /* The buffer should end with a path separator */
    ASSERT(Buffer[Length - 1] == OBJ_NAME_PATH_SEPARATOR);

    /* Again check for our two cases (drive root vs path) */
    if ((Length <= 1) || (Buffer[Length - 2] != L':'))
    {
        /* Replace the trailing slash with a null */
        Buffer[Length - 1] = UNICODE_NULL;
        --Length;
    }
    else
    {
        /* Append the null char since there's no trailing slash */
        Buffer[Length] = UNICODE_NULL;
    }

    /* Release PEB lock */
    RtlReleasePebLock();
    DPRINT("CurrentDirectory %S\n", Buffer);
    return Length * sizeof(WCHAR);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(IN PUNICODE_STRING Path)
{
    PCURDIR CurDir;
    NTSTATUS Status;
    RTL_PATH_TYPE PathType;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FullPath, NtName;
    PRTLP_CURDIR_REF OldCurDir = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_FS_DEVICE_INFORMATION FileFsDeviceInfo;
    ULONG SavedLength, CharLength, FullPathLength;
    HANDLE OldHandle = NULL, CurDirHandle = NULL, OldCurDirHandle = NULL;

    DPRINT("RtlSetCurrentDirectory_U %wZ\n", Path);

    /* Initialize for failure case */
    RtlInitEmptyUnicodeString(&NtName, NULL, 0);

    /* Can't set current directory on DOS device */
    if (RtlIsDosDeviceName_Ustr(Path))
    {
        return STATUS_NOT_A_DIRECTORY;
    }

    /* Get current directory */
    RtlAcquirePebLock();
    CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;

    /* Check if we have to drop current handle */
    if (((ULONG_PTR)(CurDir->Handle) & RTL_CURDIR_ALL_FLAGS) == RTL_CURDIR_DROP_OLD_HANDLE)
    {
        OldHandle = CurDir->Handle;
        CurDir->Handle = NULL;
    }

    /* Allocate a buffer for full path (using max possible length */
    FullPath.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, CurDir->DosPath.MaximumLength);
    if (!FullPath.Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Leave;
    }

    /* Init string */
    FullPath.Length = 0;
    FullPath.MaximumLength = CurDir->DosPath.MaximumLength;

    /* Get new directory full path */
    FullPathLength = RtlGetFullPathName_Ustr(Path, FullPath.MaximumLength, FullPath.Buffer, NULL, NULL, &PathType);
    if (!FullPathLength)
    {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Leave;
    }

    SavedLength = FullPath.MaximumLength;
    CharLength = FullPathLength / sizeof(WCHAR);

    if (FullPathLength > FullPath.MaximumLength)
    {
        Status = STATUS_NAME_TOO_LONG;
        goto Leave;
    }

    /* Translate it to NT name */
    if (!RtlDosPathNameToNtPathName_U(FullPath.Buffer, &NtName, NULL, NULL))
    {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Leave;
    }

   InitializeObjectAttributes(&ObjectAttributes, &NtName,
                              OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                              NULL, NULL);

    /* If previous current directory was removable, then check it for dropping */
    if (((ULONG_PTR)(CurDir->Handle) & RTL_CURDIR_ALL_FLAGS) == RTL_CURDIR_ALL_FLAGS)
    {
        /* Get back normal handle */
        CurDirHandle = (HANDLE)((ULONG_PTR)(CurDir->Handle) & ~RTL_CURDIR_ALL_FLAGS);
        CurDir->Handle = NULL;

        /* Get device information */
        Status = NtQueryVolumeInformationFile(CurDirHandle,
                                              &IoStatusBlock,
                                              &FileFsDeviceInfo,
                                              sizeof(FileFsDeviceInfo),
                                              FileFsDeviceInformation);
        /* Retry without taking care of removable device */
        if (!NT_SUCCESS(Status))
        {
            Status = RtlSetCurrentDirectory_U(Path);
            goto Leave;
        }
    }
    else
    {
        /* Open directory */
        Status = NtOpenFile(&CurDirHandle,
                            SYNCHRONIZE | FILE_TRAVERSE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(Status)) goto Leave;

        /* Get device information */
        Status = NtQueryVolumeInformationFile(CurDirHandle,
                                              &IoStatusBlock,
                                              &FileFsDeviceInfo,
                                              sizeof(FileFsDeviceInfo),
                                              FileFsDeviceInformation);
        if (!NT_SUCCESS(Status)) goto Leave;
    }

    /* If device is removable, mark handle */
    if (FileFsDeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA)
    {
        CurDirHandle = (HANDLE)((ULONG_PTR)CurDirHandle | RTL_CURDIR_IS_REMOVABLE);
    }

    FullPath.Length = (USHORT)FullPathLength;

    /* If full path isn't \ terminated, do it */
    if (FullPath.Buffer[CharLength - 1] != OBJ_NAME_PATH_SEPARATOR)
    {
        if ((CharLength + 1) * sizeof(WCHAR) > SavedLength)
        {
            Status = STATUS_NAME_TOO_LONG;
            goto Leave;
        }

        FullPath.Buffer[CharLength] = OBJ_NAME_PATH_SEPARATOR;
        FullPath.Buffer[CharLength + 1] = UNICODE_NULL;
        FullPath.Length += sizeof(WCHAR);
    }

    /* If we have previous current directory with only us as reference, save it */
    if (RtlpCurDirRef != NULL && RtlpCurDirRef->RefCount == 1)
    {
        OldCurDirHandle = RtlpCurDirRef->Handle;
    }
    else
    {
        /* Allocate new current directory struct saving previous one */
        OldCurDir = RtlpCurDirRef;
        RtlpCurDirRef = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(RTLP_CURDIR_REF));
        if (!RtlpCurDirRef)
        {
            RtlpCurDirRef = OldCurDir;
            OldCurDir = NULL;
            Status = STATUS_NO_MEMORY;
            goto Leave;
        }

        /* Set reference to 1 (us) */
        RtlpCurDirRef->RefCount = 1;
    }

    /* Save new data */
    CurDir->Handle = CurDirHandle;
    RtlpCurDirRef->Handle = CurDirHandle;
    CurDirHandle = NULL;

    /* Copy full path */
    RtlCopyMemory(CurDir->DosPath.Buffer, FullPath.Buffer, FullPath.Length + sizeof(WCHAR));
    CurDir->DosPath.Length = FullPath.Length;

    Status = STATUS_SUCCESS;

Leave:
    RtlReleasePebLock();

    if (FullPath.Buffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FullPath.Buffer);
    }

    if (NtName.Buffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtName.Buffer);
    }

    if (CurDirHandle) NtClose(CurDirHandle);

    if (OldHandle) NtClose(OldHandle);

    if (OldCurDirHandle) NtClose(OldCurDirHandle);

    if (OldCurDir && InterlockedDecrement(&OldCurDir->RefCount) == 0)
    {
        NtClose(OldCurDir->Handle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, OldCurDir);
    }

    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetFullPathName_UEx(
    _In_ PWSTR FileName,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ RTL_PATH_TYPE *InputPathType)
{
    UNICODE_STRING FileNameString;
    NTSTATUS status;

    if (InputPathType)
        *InputPathType = 0;

    /* Build the string */
    status = RtlInitUnicodeStringEx(&FileNameString, FileName);
    if (!NT_SUCCESS(status)) return 0;

    /* Call the extended function */
    return RtlGetFullPathName_Ustr(
        &FileNameString,
        BufferLength,
        Buffer,
        (PCWSTR*)FilePart,
        NULL,
        InputPathType);
}

/******************************************************************
 *    RtlGetFullPathName_U  (NTDLL.@)
 *
 * Returns the number of bytes written to buffer (not including the
 * terminating NULL) if the function succeeds, or the required number of bytes
 * (including the terminating NULL) if the buffer is too small.
 *
 * file_part will point to the filename part inside buffer (except if we use
 * DOS device name, in which case file_in_buf is NULL)
 *
 * @implemented
 */

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetFullPathName_U(
    _In_ PCWSTR FileName,
    _In_ ULONG Size,
    _Out_z_bytecap_(Size) PWSTR Buffer,
    _Out_opt_ PWSTR *ShortName)
{
    RTL_PATH_TYPE PathType;

    /* Call the extended function */
    return RtlGetFullPathName_UEx((PWSTR)FileName,
                                   Size,
                                   Buffer,
                                   ShortName,
                                   &PathType);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(IN PCWSTR DosName,
                             OUT PUNICODE_STRING NtName,
                             OUT PCWSTR *PartName,
                             OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDosPathNameToNtPathName_U_WithStatus(IN PCWSTR DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    return RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(IN PCWSTR DosName,
                                     OUT PUNICODE_STRING NtName,
                                     OUT PCWSTR *PartName,
                                     OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDosPathNameToRelativeNtPathName_U_WithStatus(IN PCWSTR DosName,
                                                OUT PUNICODE_STRING NtName,
                                                OUT PCWSTR *PartName,
                                                OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*
 * @implemented
 */
NTSTATUS NTAPI RtlNtPathNameToDosPathName(IN ULONG Flags,
                                          IN OUT PRTL_UNICODE_STRING_BUFFER Path,
                                          OUT PULONG PathType,
                                          PULONG Unknown)
{
    PCUNICODE_STRING UsePrefix = NULL, AlternatePrefix = NULL;

    if (PathType)
        *PathType = 0;

    if (!Path || Flags)
        return STATUS_INVALID_PARAMETER;

    /* The initial check is done on Path->String */
    if (RtlPrefixUnicodeString(&RtlpDosDevicesUncPrefix, &Path->String, TRUE))
    {
        UsePrefix = &RtlpDosDevicesUncPrefix;
        AlternatePrefix = &RtlpDoubleSlashPrefix;
        if (PathType)
            *PathType = RTL_CONVERTED_UNC_PATH;
    }
    else if (RtlPrefixUnicodeString(&RtlpDosDevicesPrefix, &Path->String, FALSE))
    {
        UsePrefix = &RtlpDosDevicesPrefix;
        if (PathType)
            *PathType = RTL_CONVERTED_NT_PATH;
    }

    if (UsePrefix)
    {
        NTSTATUS Status;

        USHORT Len = Path->String.Length - UsePrefix->Length;
        if (AlternatePrefix)
            Len += AlternatePrefix->Length;

        Status = RtlEnsureBufferSize(0, &Path->ByteBuffer, Len);
        if (!NT_SUCCESS(Status))
            return Status;

        if (Len + sizeof(UNICODE_NULL) <= Path->ByteBuffer.Size)
        {
            /* Then, the contents of Path->ByteBuffer are always used... */
            if (AlternatePrefix)
            {
                memcpy(Path->ByteBuffer.Buffer, AlternatePrefix->Buffer, AlternatePrefix->Length);
                memmove(Path->ByteBuffer.Buffer + AlternatePrefix->Length, Path->ByteBuffer.Buffer + UsePrefix->Length,
                    Len - AlternatePrefix->Length);
            }
            else
            {
                memmove(Path->ByteBuffer.Buffer, Path->ByteBuffer.Buffer + UsePrefix->Length, Len);
            }
            Path->String.Buffer = (PWSTR)Path->ByteBuffer.Buffer;
            Path->String.Length = Len;
            Path->String.MaximumLength = Path->ByteBuffer.Size;
            Path->String.Buffer[Len / sizeof(WCHAR)] = UNICODE_NULL;
        }
        return STATUS_SUCCESS;
    }

    if (PathType)
    {
        switch (RtlDetermineDosPathNameType_Ustr(&Path->String))
        {
        case RtlPathTypeUncAbsolute:
        case RtlPathTypeDriveAbsolute:
        case RtlPathTypeLocalDevice:
        case RtlPathTypeRootLocalDevice:
            *PathType = RTL_UNCHANGED_DOS_PATH;
            break;
        case RtlPathTypeUnknown:
        case RtlPathTypeDriveRelative:
        case RtlPathTypeRooted:
        case RtlPathTypeRelative:
            *PathType = RTL_UNCHANGED_UNK_PATH;
            break;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlDosSearchPath_U(IN PCWSTR Path,
                   IN PCWSTR FileName,
                   IN PCWSTR Extension,
                   IN ULONG Size,
                   IN PWSTR Buffer,
                   OUT PWSTR *PartName)
{
    NTSTATUS Status;
    ULONG ExtensionLength, Length, FileNameLength, PathLength;
    UNICODE_STRING TempString;
    PWCHAR NewBuffer, BufferStart;
    PCWSTR p;

    /* Check if this is an absolute path */
    if (RtlDetermineDosPathNameType_U(FileName) != RtlPathTypeRelative)
    {
        /* Check if the file exists */
        if (RtlDoesFileExists_UEx(FileName, TRUE))
        {
            /* Get the full name, which does the DOS lookup */
            return RtlGetFullPathName_U(FileName, Size, Buffer, PartName);
        }

        /* Doesn't exist, so fail */
        return 0;
    }

    /* Scan the filename */
    p = FileName;
    while (*p)
    {
        /* Looking for an extension */
        if (*p == L'.')
        {
            /* No extension string needed -- it's part of the filename */
            Extension = NULL;
            break;
        }

        /* Next character */
        p++;
    }

    /* Do we have an extension? */
    if (!Extension)
    {
        /* Nope, don't worry about one */
        ExtensionLength = 0;
    }
    else
    {
        /* Build a temporary string to get the extension length */
        Status = RtlInitUnicodeStringEx(&TempString, Extension);
        if (!NT_SUCCESS(Status)) return 0;
        ExtensionLength = TempString.Length;
    }

    /* Build a temporary string to get the path length */
    Status = RtlInitUnicodeStringEx(&TempString, Path);
    if (!NT_SUCCESS(Status)) return 0;
    PathLength = TempString.Length;

    /* Build a temporary string to get the filename length */
    Status = RtlInitUnicodeStringEx(&TempString, FileName);
    if (!NT_SUCCESS(Status)) return 0;
    FileNameLength = TempString.Length;

    /* Allocate the buffer for the new string name */
    NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                FileNameLength +
                                ExtensionLength +
                                PathLength +
                                3 * sizeof(WCHAR));
    if (!NewBuffer)
    {
        /* Fail the call */
        DbgPrint("%s: Failing due to out of memory (RtlAllocateHeap failure)\n",
                 __FUNCTION__);
        return 0;
    }

    /* Final loop to build the path */
    while (TRUE)
    {
        /* Check if we have a valid character */
        BufferStart = NewBuffer;
        if (*Path)
        {
            /* Loop as long as there's no semicolon */
            while (*Path != L';')
            {
                /* Copy the next character */
                *BufferStart++ = *Path++;
                if (!*Path) break;
            }

            /* We found a semi-colon, to stop path processing on this loop */
            if (*Path == L';') ++Path;
        }

        /* Add a terminating slash if needed */
        if ((BufferStart != NewBuffer) && (BufferStart[-1] != OBJ_NAME_PATH_SEPARATOR))
        {
            *BufferStart++ = OBJ_NAME_PATH_SEPARATOR;
        }

        /* Bail out if we reached the end */
        if (!*Path) Path = NULL;

        /* Copy the file name and check if an extension is needed */
        RtlCopyMemory(BufferStart, FileName, FileNameLength);
        if (ExtensionLength)
        {
            /* Copy the extension too */
            RtlCopyMemory((PCHAR)BufferStart + FileNameLength,
                          Extension,
                          ExtensionLength + sizeof(WCHAR));
        }
        else
        {
            /* Just NULL-terminate */
            *(PWCHAR)((PCHAR)BufferStart + FileNameLength) = UNICODE_NULL;
        }

        /* Now, does this file exist? */
        if (RtlDoesFileExists_UEx(NewBuffer, FALSE))
        {
            /* Call the full-path API to get the length */
            Length = RtlGetFullPathName_U(NewBuffer, Size, Buffer, PartName);
            break;
        }

        /* If we got here, path doesn't exist, so fail the call */
        Length = 0;
        if (!Path) break;
    }

    /* Free the allocation and return the length */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
    return Length;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetFullPathName_UstrEx(IN PUNICODE_STRING FileName,
                          IN PUNICODE_STRING StaticString,
                          IN PUNICODE_STRING DynamicString,
                          IN PUNICODE_STRING *StringUsed,
                          IN PSIZE_T FilePartSize,
                          OUT PBOOLEAN NameInvalid,
                          OUT RTL_PATH_TYPE* PathType,
                          OUT PSIZE_T LengthNeeded)
{
    NTSTATUS Status;
    PWCHAR StaticBuffer;
    PCWCH ShortName;
    ULONG Length;
    USHORT StaticLength;
    UNICODE_STRING TempDynamicString;

    /* Initialize all our locals */
    ShortName = NULL;
    StaticBuffer = NULL;
    TempDynamicString.Buffer = NULL;

    /* Initialize the input parameters */
    if (StringUsed) *StringUsed = NULL;
    if (LengthNeeded) *LengthNeeded = 0;
    if (FilePartSize) *FilePartSize = 0;

    /* Check for invalid parameters */
    if ((DynamicString) && !(StringUsed) && (StaticString))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we did not get an input string */
    if (!StaticString)
    {
        /* Allocate one */
        StaticLength = MAX_PATH * sizeof(WCHAR);
        StaticBuffer = RtlpAllocateStringMemory(MAX_PATH * sizeof(WCHAR), TAG_USTR);
        if (!StaticBuffer) return STATUS_NO_MEMORY;
    }
    else
    {
        /* Use the one we received */
        StaticBuffer = StaticString->Buffer;
        StaticLength = StaticString->MaximumLength;
    }

    /* Call the lower-level function */
    Length = RtlGetFullPathName_Ustr(FileName,
                                     StaticLength,
                                     StaticBuffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    DPRINT("Length: %u StaticBuffer: %S\n", Length, StaticBuffer);
    if (!Length)
    {
        /* Fail if it failed */
        DbgPrint("%s(%d) - RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__,
                 __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    /* Check if it fits inside our static string */
    if ((StaticString) && (Length < StaticLength))
    {
        /* Set the final length */
        StaticString->Length = (USHORT)Length;

        /* Set the file part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticString->Buffer) : 0;

        /* Return the static string if requested */
        if (StringUsed) *StringUsed = StaticString;

        /* We are done with success */
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* Did we not have an input dynamic string ?*/
    if (!DynamicString)
    {
        /* Return the length we need */
        if (LengthNeeded) *LengthNeeded = Length;

        /* And fail such that the caller can try again */
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Quickie;
    }

    /* Check if it fits in our static buffer */
    if ((StaticBuffer) && (Length < StaticLength))
    {
        /* NULL-terminate it */
        StaticBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Set the settings for the dynamic string the caller sent */
        DynamicString->MaximumLength = StaticLength;
        DynamicString->Length = (USHORT)Length;
        DynamicString->Buffer = StaticBuffer;

        /* Set the part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticBuffer) : 0;

        /* Return the dynamic string if requested */
        if (StringUsed) *StringUsed = DynamicString;

        /* Do not free the static buffer on exit, and return success */
        StaticBuffer = NULL;
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* Now try again under the PEB lock */
    RtlAcquirePebLock();
    Length = RtlGetFullPathName_Ustr(FileName,
                                     StaticLength,
                                     StaticBuffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    if (!Length)
    {
        /* It failed */
        DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__, __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Release;
    }

    /* Check if it fits inside our static string now */
    if ((StaticString) && (Length < StaticLength))
    {
        /* Set the final length */
        StaticString->Length = (USHORT)Length;

        /* Set the file part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticString->Buffer) : 0;

        /* Return the static string if requested */
        if (StringUsed) *StringUsed = StaticString;

        /* We are done with success */
        Status = STATUS_SUCCESS;
        goto Release;
    }

    /* Check if the path won't even fit in a real string */
    if ((Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES)
    {
        /* Name is way too long, fail */
        Status = STATUS_NAME_TOO_LONG;
        goto Release;
    }

    /* Allocate the string to hold the path name now */
    TempDynamicString.Buffer = RtlpAllocateStringMemory(Length + sizeof(WCHAR),
                                                        TAG_USTR);
    if (!TempDynamicString.Buffer)
    {
        /* Out of memory, fail */
        Status = STATUS_NO_MEMORY;
        goto Release;
    }

    /* Add space for a NULL terminator, and now check the full path */
    TempDynamicString.MaximumLength = (USHORT)Length + sizeof(UNICODE_NULL);
    Length = RtlGetFullPathName_Ustr(FileName,
                                     Length,
                                     TempDynamicString.Buffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    if (!Length)
    {
        /* Some path error, so fail out */
        DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__, __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Release;
    }

    /* It should fit in the string we just allocated */
    ASSERT(Length < (TempDynamicString.MaximumLength - sizeof(WCHAR)));
    if (Length > TempDynamicString.MaximumLength)
    {
        /* This is really weird and would mean some kind of race */
        Status = STATUS_INTERNAL_ERROR;
        goto Release;
    }

    /* Return the file part size */
    if (FilePartSize) *FilePartSize = ShortName ? (ShortName - TempDynamicString.Buffer) : 0;

    /* Terminate the whole string now */
    TempDynamicString.Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Finalize the string and return it to the user */
    DynamicString->Buffer = TempDynamicString.Buffer;
    DynamicString->Length = (USHORT)Length;
    DynamicString->MaximumLength = TempDynamicString.MaximumLength;
    if (StringUsed) *StringUsed = DynamicString;

    /* Return success and make sure we don't free the buffer on exit */
    TempDynamicString.Buffer = NULL;
    Status = STATUS_SUCCESS;

Release:
    /* Release the PEB lock */
    RtlReleasePebLock();

Quickie:
    /* Free any buffers we should be freeing */
    DPRINT("Status: %lx %S %S\n", Status, StaticBuffer, TempDynamicString.Buffer);
    if ((StaticString) && (StaticBuffer) && (StaticBuffer != StaticString->Buffer))
    {
        RtlpFreeStringMemory(StaticBuffer, TAG_USTR);
    }
    if (TempDynamicString.Buffer)
    {
        RtlpFreeStringMemory(TempDynamicString.Buffer, TAG_USTR);
    }

    /* Print out any unusual errors */
    if ((NT_ERROR(Status)) &&
        (Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        DbgPrint("RTL: %s - failing on filename %wZ with status %08lx\n",
                __FUNCTION__, FileName, Status);
    }

    /* Return, we're all done */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDosSearchPath_Ustr(IN ULONG Flags,
                      IN PUNICODE_STRING PathString,
                      IN PUNICODE_STRING FileNameString,
                      IN PUNICODE_STRING ExtensionString,
                      IN PUNICODE_STRING CallerBuffer,
                      IN OUT PUNICODE_STRING DynamicString OPTIONAL,
                      OUT PUNICODE_STRING* FullNameOut OPTIONAL,
                      OUT PSIZE_T FilePartSize OPTIONAL,
                      OUT PSIZE_T LengthNeeded OPTIONAL)
{
    WCHAR StaticCandidateBuffer[MAX_PATH];
    UNICODE_STRING StaticCandidateString;
    NTSTATUS Status;
    RTL_PATH_TYPE PathType;
    PWCHAR p, End, CandidateEnd, SegmentEnd;
    SIZE_T WorstCaseLength, NamePlusExtLength, SegmentSize, ByteCount, PathSize, MaxPathSize = 0;
    USHORT ExtensionLength = 0;
    PUNICODE_STRING FullIsolatedPath;
    DPRINT("DOS Path Search: %lx %wZ %wZ %wZ %wZ %wZ\n",
            Flags, PathString, FileNameString, ExtensionString, CallerBuffer, DynamicString);

    /* Initialize the input string */
    RtlInitEmptyUnicodeString(&StaticCandidateString,
                              StaticCandidateBuffer,
                              sizeof(StaticCandidateBuffer));

    /* Initialize optional arguments */
    if (FullNameOut ) *FullNameOut  = NULL;
    if (FilePartSize) *FilePartSize = 0;
    if (LengthNeeded) *LengthNeeded = 0;
    if (DynamicString)
    {
        DynamicString->Length = DynamicString->MaximumLength = 0;
        DynamicString->Buffer = NULL;
    }

    /* Check for invalid parameters */
    if ((Flags & ~7) ||
        !(PathString) ||
        !(FileNameString) ||
        ((CallerBuffer) && (DynamicString) && !(FullNameOut)))
    {
        /* Fail */
        DbgPrint("%s: Invalid parameters passed\n", __FUNCTION__);
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* First check what kind of path this is */
    PathType = RtlDetermineDosPathNameType_Ustr(FileNameString);

    /* Check if the caller wants to prevent relative .\ and ..\ paths */
    if ((Flags & 2) &&
         (PathType == RtlPathTypeRelative) &&
         (FileNameString->Length >= (2 * sizeof(WCHAR))) &&
         (FileNameString->Buffer[0] == L'.') &&
         ((IS_PATH_SEPARATOR(FileNameString->Buffer[1])) ||
          ((FileNameString->Buffer[1] == L'.') &&
           ((FileNameString->Length >= (3 * sizeof(WCHAR))) &&
           (IS_PATH_SEPARATOR(FileNameString->Buffer[2]))))))
    {
        /* Yes, and this path is like that, so make it seem unknown */
        PathType = RtlPathTypeUnknown;
    }

    /* Now check relative vs non-relative paths */
    if (PathType == RtlPathTypeRelative)
    {
        /* Does the caller want SxS? */
        if (Flags & 1)
        {
            /* Apply the SxS magic */
            FullIsolatedPath = NULL;
            Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                              FileNameString,
                                                              ExtensionString,
                                                              CallerBuffer,
                                                              DynamicString,
                                                              &FullIsolatedPath,
                                                              NULL,
                                                              FilePartSize,
                                                              LengthNeeded);
            if (NT_SUCCESS(Status))
            {
                /* We found the SxS path, return it */
                if (FullNameOut) *FullNameOut = FullIsolatedPath;
                goto Quickie;
            }
            else if (Status != STATUS_SXS_KEY_NOT_FOUND)
            {
                /* Critical SxS error, fail */
                DbgPrint("%s: Failing because call to "
                         "RtlDosApplyIsolationRedirection_Ustr(%wZ) failed with "
                          "status 0x%08lx\n",
                         __FUNCTION__,
                         FileNameString,
                         Status);
                goto Quickie;
            }
        }

        /* No SxS key found, or not requested, check if there's an extension */
        if (ExtensionString)
        {
            /* Save the extension length, and check if there's a file name */
            ExtensionLength = ExtensionString->Length;
            if (FileNameString->Length)
            {
                /* Start parsing the file name */
                End = &FileNameString->Buffer[FileNameString->Length / sizeof(WCHAR)];
                while (End > FileNameString->Buffer)
                {
                    /* If we find a path separator, there's no extension */
                    --End;
                    if (IS_PATH_SEPARATOR(*End)) break;

                    /* Otherwise, did we find an extension dot? */
                    if (*End == L'.')
                    {
                        /* Ignore what the caller sent it, use the filename's */
                        ExtensionString = NULL;
                        ExtensionLength = 0;
                        break;
                    }
                }
            }
        }

        /* Check if we got a path */
        if (PathString->Length)
        {
            /* Start parsing the path name, looking for path separators */
            End = &PathString->Buffer[PathString->Length / sizeof(WCHAR)];
            p = End;
            while (p > PathString->Buffer)
            {
                if (*--p == L';')
                {
                    /* This is the size of the path -- handle a trailing slash */
                    PathSize = End - p - 1;
                    if ((PathSize) && !(IS_PATH_SEPARATOR(*(End - 1)))) PathSize++;

                    /* Check if we found a bigger path than before */
                    if (PathSize > MaxPathSize) MaxPathSize = PathSize;

                    /* Keep going with the path after this path separator */
                    End = p;
                }
            }

            /* This is the trailing path, run the same code as above */
            PathSize = End - p;
            if ((PathSize) && !(IS_PATH_SEPARATOR(*(End - 1)))) PathSize++;
            if (PathSize > MaxPathSize) MaxPathSize = PathSize;

            /* Finally, convert the largest path size into WCHAR */
            MaxPathSize *= sizeof(WCHAR);
        }

        /* Use the extension, the file name, and the largest path as the size */
        WorstCaseLength = ExtensionLength +
                          FileNameString->Length +
                          (USHORT)MaxPathSize +
                          sizeof(UNICODE_NULL);
        if (WorstCaseLength > UNICODE_STRING_MAX_BYTES)
        {
            /* It has to fit in a registry string, if not, fail here */
            DbgPrint("%s returning STATUS_NAME_TOO_LONG because the computed "
                     "worst case file name length is %Iu bytes\n",
                     __FUNCTION__,
                     WorstCaseLength);
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }

        /* Scan the path now, to see if we can find the file */
        p = PathString->Buffer;
        End = &p[PathString->Length / sizeof(WCHAR)];
        while (p < End)
        {
            /* Find out where this path ends */
            for (SegmentEnd = p;
                 ((SegmentEnd != End) && (*SegmentEnd != L';'));
                 SegmentEnd++);

            /* Compute the size of this path */
            ByteCount = SegmentSize = (SegmentEnd - p) * sizeof(WCHAR);

            /* Handle trailing slash if there isn't one */
            if ((SegmentSize) && !(IS_PATH_SEPARATOR(*(SegmentEnd - 1))))
            {
                /* Add space for one */
                SegmentSize += sizeof(OBJ_NAME_PATH_SEPARATOR);
            }

            /* Now check if our initial static buffer is too small */
            if (StaticCandidateString.MaximumLength <
                (SegmentSize + ExtensionLength + FileNameString->Length + sizeof(UNICODE_NULL)))
            {
                /* At this point we should've been using our static buffer */
                ASSERT(StaticCandidateString.Buffer == StaticCandidateBuffer);
                if (StaticCandidateString.Buffer != StaticCandidateBuffer)
                {
                    /* Something is really messed up if this was the dynamic string */
                    DbgPrint("%s: internal error #1; "
                             "CandidateString.Buffer = %p; "
                             "StaticCandidateBuffer = %p\n",
                            __FUNCTION__,
                            StaticCandidateString.Buffer,
                            StaticCandidateBuffer);
                    Status = STATUS_INTERNAL_ERROR;
                    goto Quickie;
                }

                /* We checked before that the maximum possible size shoudl fit! */
                ASSERT((SegmentSize + FileNameString->Length + ExtensionLength) <
                        UNICODE_STRING_MAX_BYTES);
                if ((SegmentSize + ExtensionLength + FileNameString->Length) >
                    (UNICODE_STRING_MAX_BYTES - sizeof(WCHAR)))
                {
                    /* For some reason it's not fitting anymore. Something messed up */
                    DbgPrint("%s: internal error #2; SegmentSize = %u, "
                             "FileName->Length = %u, DefaultExtensionLength = %u\n",
                             __FUNCTION__,
                             SegmentSize,
                             FileNameString->Length,
                             ExtensionLength);
                    Status = STATUS_INTERNAL_ERROR;
                    goto Quickie;
                }

                /* Now allocate the dynamic string */
                StaticCandidateString.MaximumLength = WorstCaseLength;
                StaticCandidateString.Buffer = RtlpAllocateStringMemory(WorstCaseLength,
                                                                        TAG_USTR);
                if (!StaticCandidateString.Buffer)
                {
                    /* Out of memory, fail */
                    DbgPrint("%s: Unable to allocate %u byte buffer for path candidate\n",
                             __FUNCTION__,
                             StaticCandidateString.MaximumLength);
                    Status = STATUS_NO_MEMORY;
                    goto Quickie;
                }
            }

            /* Copy the path in the string */
            RtlCopyMemory(StaticCandidateString.Buffer, p, ByteCount);

            /* Get to the end of the string, and add the trailing slash if missing */
            CandidateEnd = &StaticCandidateString.Buffer[ByteCount / sizeof(WCHAR)];
            if ((SegmentSize) && (SegmentSize != ByteCount))
            {
                *CandidateEnd++ = OBJ_NAME_PATH_SEPARATOR;
            }

            /* Copy the filename now */
            RtlCopyMemory(CandidateEnd,
                          FileNameString->Buffer,
                          FileNameString->Length);
            CandidateEnd += (FileNameString->Length / sizeof(WCHAR));

            /* Check if there was an extension */
            if (ExtensionString)
            {
                /* Copy the extension too */
                RtlCopyMemory(CandidateEnd,
                              ExtensionString->Buffer,
                              ExtensionString->Length);
                          CandidateEnd += (ExtensionString->Length / sizeof(WCHAR));
            }

            /* We are done, terminate it */
            *CandidateEnd = UNICODE_NULL;

            /* Now set the final length of the string so it becomes valid */
            StaticCandidateString.Length = (USHORT)(CandidateEnd -
                                            StaticCandidateString.Buffer) *
                                           sizeof(WCHAR);
            ASSERT(StaticCandidateString.Length < StaticCandidateString.MaximumLength);

            /* Check if this file exists */
            DPRINT("BUFFER: %S\n", StaticCandidateString.Buffer);
            if (RtlDoesFileExists_UEx(StaticCandidateString.Buffer, FALSE))
            {
                /* Awesome, it does, now get the full path */
                Status = RtlGetFullPathName_UstrEx(&StaticCandidateString,
                                                   CallerBuffer,
                                                   DynamicString,
                                                   (PUNICODE_STRING*)FullNameOut,
                                                   FilePartSize,
                                                   NULL,
                                                   &PathType,
                                                   LengthNeeded);
                if (!(NT_SUCCESS(Status)) &&
                    ((Status != STATUS_NO_SUCH_FILE) &&
                     (Status != STATUS_BUFFER_TOO_SMALL)))
                {
                    DbgPrint("%s: Failing because we thought we found %wZ on "
                             "the search path, but RtlGetfullPathNameUStrEx() "
                             "returned %08lx\n",
                             __FUNCTION__,
                             &StaticCandidateString,
                             Status);
                }
                DPRINT("Status: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
                goto Quickie;
            }
            else
            {
                /* Otherwise, move to the next path */
                if (SegmentEnd != End)
                {
                    /* Handle the case of the path separator trailing */
                    p = SegmentEnd + 1;
                }
                else
                {
                    p = SegmentEnd;
                }
            }
        }

        /* Loop finished and we didn't break out -- fail */
        Status = STATUS_NO_SUCH_FILE;
    }
    else
    {
        /* We have a full path, so check if it does exist */
        DPRINT("%wZ\n", FileNameString);
        if (!RtlDoesFileExists_UstrEx(FileNameString, TRUE))
        {
            /* It doesn't exist, did we have an extension? */
            if (!(ExtensionString) || !(ExtensionString->Length))
            {
                /* No extension, so just fail */
                Status = STATUS_NO_SUCH_FILE;
                goto Quickie;
            }

            /* There was an extension, check if the filename already had one */
            if (!(Flags & 4) && (FileNameString->Length))
            {
                /* Parse the filename */
                p = FileNameString->Buffer;
                End = &p[FileNameString->Length / sizeof(WCHAR)];
                while (End > p)
                {
                    /* If there's a path separator, there's no extension */
                    --End;
                    if (IS_PATH_SEPARATOR(*End)) break;

                    /* Othwerwise, did we find an extension dot? */
                    if (*End == L'.')
                    {
                        /* File already had an extension, so fail */
                        Status = STATUS_NO_SUCH_FILE;
                        goto Quickie;
                    }
                }
            }

            /* So there is an extension, we'll try again by adding it */
            NamePlusExtLength = FileNameString->Length +
                                ExtensionString->Length +
                                sizeof(UNICODE_NULL);
            if (NamePlusExtLength > UNICODE_STRING_MAX_BYTES)
            {
                /* It won't fit in any kind of valid string, so fail */
                DbgPrint("%s: Failing because filename plus extension (%Iu bytes) is too big\n",
                         __FUNCTION__,
                         NamePlusExtLength);
                Status = STATUS_NAME_TOO_LONG;
                goto Quickie;
            }

            /* Fill it fit in our temporary string? */
            if (NamePlusExtLength > StaticCandidateString.MaximumLength)
            {
                /* It won't fit anymore, allocate a dynamic string for it */
                StaticCandidateString.MaximumLength = NamePlusExtLength;
                StaticCandidateString.Buffer = RtlpAllocateStringMemory(NamePlusExtLength,
                                                                        TAG_USTR);
                if (!StaticCandidateString.Buffer)
                {
                    /* Ran out of memory, so fail */
                    DbgPrint("%s: Failing because allocating the dynamic filename buffer failed\n",
                             __FUNCTION__);
                    Status = STATUS_NO_MEMORY;
                    goto Quickie;
                }
            }

            /* Copy the filename */
            RtlCopyUnicodeString(&StaticCandidateString, FileNameString);

            /* Copy the extension */
            RtlAppendUnicodeStringToString(&StaticCandidateString,
                                           ExtensionString);

            DPRINT("SB: %wZ\n", &StaticCandidateString);

            /* And check if this file now exists */
            if (!RtlDoesFileExists_UstrEx(&StaticCandidateString, TRUE))
            {
                /* Still no joy, fail out */
                Status = STATUS_NO_SUCH_FILE;
                goto Quickie;
            }

            /* File was found, get the final full path */
            Status = RtlGetFullPathName_UstrEx(&StaticCandidateString,
                                               CallerBuffer,
                                               DynamicString,
                                               (PUNICODE_STRING*)FullNameOut,
                                               FilePartSize,
                                               NULL,
                                               &PathType,
                                               LengthNeeded);
            if (!(NT_SUCCESS(Status)) && (Status != STATUS_NO_SUCH_FILE))
            {
                DbgPrint("%s: Failing on \"%wZ\" because RtlGetfullPathNameUStrEx() "
                         "failed with status %08lx\n",
                         __FUNCTION__,
                         &StaticCandidateString,
                         Status);
            }
            DPRINT("Status: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
        }
        else
        {
            /* File was found on the first try, get the final full path */
            Status = RtlGetFullPathName_UstrEx(FileNameString,
                                               CallerBuffer,
                                               DynamicString,
                                               (PUNICODE_STRING*)FullNameOut,
                                               FilePartSize,
                                               NULL,
                                               &PathType,
                                               LengthNeeded);
            if (!(NT_SUCCESS(Status)) &&
                ((Status != STATUS_NO_SUCH_FILE) &&
                (Status != STATUS_BUFFER_TOO_SMALL)))
            {
                DbgPrint("%s: Failing because RtlGetfullPathNameUStrEx() on %wZ "
                         "failed with status %08lx\n",
                         __FUNCTION__,
                         FileNameString,
                         Status);
            }
            DPRINT("Status: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
        }
    }

Quickie:
    /* Anything that was not an error, turn into STATUS_SUCCESS */
    if (NT_SUCCESS(Status)) Status = STATUS_SUCCESS;

    /* Check if we had a dynamic string */
    if ((StaticCandidateString.Buffer) &&
        (StaticCandidateString.Buffer != StaticCandidateBuffer))
    {
        /* Free it */
        RtlFreeUnicodeString(&StaticCandidateString);
    }

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDoesFileExists_U(IN PCWSTR FileName)
{
    /* Call the new function */
    return RtlDoesFileExists_UEx(FileName, TRUE);
}

/* EOF */
