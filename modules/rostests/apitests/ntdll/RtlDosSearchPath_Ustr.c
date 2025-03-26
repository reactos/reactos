/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosSearchPath_Ustr
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/*
NTSTATUS
NTAPI
RtlDosSearchPath_Ustr(
    IN ULONG Flags,
    IN PUNICODE_STRING PathString,
    IN PUNICODE_STRING FileNameString,
    IN PUNICODE_STRING ExtensionString,
    IN PUNICODE_STRING CallerBuffer,
    IN OUT PUNICODE_STRING DynamicString OPTIONAL,
    OUT PUNICODE_STRING *FullNameOut OPTIONAL,
    OUT PSIZE_T FilePartSize OPTIONAL,
    OUT PSIZE_T LengthNeeded OPTIONAL
);
*/

#define ok_eq_ustr(str1, str2) do {                                                     \
        ok((str1)->Buffer        == (str2)->Buffer,        "Buffer modified\n");        \
        ok((str1)->Length        == (str2)->Length,        "Length modified\n");        \
        ok((str1)->MaximumLength == (str2)->MaximumLength, "MaximumLength modified\n"); \
    } while (0)

START_TEST(RtlDosSearchPath_Ustr)
{
    NTSTATUS Status;
    UNICODE_STRING PathString;
    UNICODE_STRING FileNameString;
    UNICODE_STRING ExtensionString;
    UNICODE_STRING CallerBuffer;
    UNICODE_STRING DynamicString;
    PUNICODE_STRING FullNameOut;
    UNICODE_STRING EmptyString;
    SIZE_T FilePartSize;
    SIZE_T LengthNeeded;
    INT i;

    RtlInitUnicodeString(&EmptyString, NULL);

    /* NULLs */
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);

    RtlInitUnicodeString(&FileNameString, NULL);
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0, NULL, &FileNameString, NULL, NULL, NULL, NULL, NULL, NULL);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);

    RtlInitUnicodeString(&PathString, NULL);
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0, &PathString, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);

    /* Minimal valid set of parameters */
    RtlInitUnicodeString(&PathString, NULL);
    RtlInitUnicodeString(&FileNameString, NULL);
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0, &PathString, &FileNameString, NULL, NULL, NULL, NULL, NULL, NULL);
        ok_eq_hex(Status, STATUS_NO_SUCH_FILE);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);
    ok_eq_ustr(&FileNameString, &EmptyString);

    /* Check valid flags */
    for (i = 0; i < 32; i++)
    {
        RtlInitUnicodeString(&PathString, NULL);
        RtlInitUnicodeString(&FileNameString, NULL);
        StartSeh()
            Status = RtlDosSearchPath_Ustr(1 << i, &PathString, &FileNameString, NULL, NULL, NULL, NULL, NULL, NULL);
            ok_eq_hex(Status, i > 2 ? STATUS_INVALID_PARAMETER : STATUS_NO_SUCH_FILE);
        EndSeh(STATUS_SUCCESS);
        ok_eq_ustr(&PathString, &EmptyString);
        ok_eq_ustr(&FileNameString, &EmptyString);
    }

    RtlInitUnicodeString(&PathString, NULL);
    RtlInitUnicodeString(&FileNameString, NULL);
    StartSeh()
        Status = RtlDosSearchPath_Ustr(7, &PathString, &FileNameString, NULL, NULL, NULL, NULL, NULL, NULL);
        ok_eq_hex(Status, STATUS_NO_SUCH_FILE);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);
    ok_eq_ustr(&FileNameString, &EmptyString);

    /* Everything except PathString */
    RtlInitUnicodeString(&FileNameString, NULL);
    RtlInitUnicodeString(&ExtensionString, NULL);
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = InvalidPointer;
    FilePartSize = (SIZE_T)-1;
    LengthNeeded = (SIZE_T)-1;
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0,
                                       NULL,
                                       &FileNameString,
                                       &ExtensionString,
                                       &CallerBuffer,
                                       &DynamicString,
                                       &FullNameOut,
                                       &FilePartSize,
                                       &LengthNeeded);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileNameString, &EmptyString);
    ok_eq_ustr(&ExtensionString, &EmptyString);
    ok_eq_ustr(&CallerBuffer, &EmptyString);
    ok_eq_ustr(&DynamicString, &EmptyString);
    ok_eq_pointer(FullNameOut, NULL);
    ok_eq_ulong(FilePartSize, 0UL);
    ok_eq_ulong(LengthNeeded, 0UL);

    /* Everything except FileNameString */
    RtlInitUnicodeString(&PathString, NULL);
    RtlInitUnicodeString(&ExtensionString, NULL);
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = InvalidPointer;
    FilePartSize = (SIZE_T)-1;
    LengthNeeded = (SIZE_T)-1;
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0,
                                       &PathString,
                                       NULL,
                                       &ExtensionString,
                                       &CallerBuffer,
                                       &DynamicString,
                                       &FullNameOut,
                                       &FilePartSize,
                                       &LengthNeeded);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);
    ok_eq_ustr(&ExtensionString, &EmptyString);
    ok_eq_ustr(&CallerBuffer, &EmptyString);
    ok_eq_ustr(&DynamicString, &EmptyString);
    ok_eq_pointer(FullNameOut, NULL);
    ok_eq_ulong(FilePartSize, 0UL);
    ok_eq_ulong(LengthNeeded, 0UL);

    /* Passing CallerBuffer and DynamicString, but not FullNameOut is invalid */
    RtlInitUnicodeString(&PathString, NULL);
    RtlInitUnicodeString(&FileNameString, NULL);
    RtlInitUnicodeString(&ExtensionString, NULL);
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = InvalidPointer;
    FilePartSize = (SIZE_T)-1;
    LengthNeeded = (SIZE_T)-1;
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0,
                                       &PathString,
                                       &FileNameString,
                                       &ExtensionString,
                                       &CallerBuffer,
                                       &DynamicString,
                                       NULL,
                                       &FilePartSize,
                                       &LengthNeeded);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);
    ok_eq_ustr(&FileNameString, &EmptyString);
    ok_eq_ustr(&ExtensionString, &EmptyString);
    ok_eq_ustr(&CallerBuffer, &EmptyString);
    ok_eq_ustr(&DynamicString, &EmptyString);
    ok_eq_ulong(FilePartSize, 0UL);
    ok_eq_ulong(LengthNeeded, 0UL);

    /* All parameters given */
    RtlInitUnicodeString(&PathString, NULL);
    RtlInitUnicodeString(&FileNameString, NULL);
    RtlInitUnicodeString(&ExtensionString, NULL);
    RtlInitUnicodeString(&CallerBuffer, NULL);
    RtlInitUnicodeString(&DynamicString, NULL);
    FullNameOut = InvalidPointer;
    FilePartSize = (SIZE_T)-1;
    LengthNeeded = (SIZE_T)-1;
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0,
                                       &PathString,
                                       &FileNameString,
                                       &ExtensionString,
                                       &CallerBuffer,
                                       &DynamicString,
                                       &FullNameOut,
                                       &FilePartSize,
                                       &LengthNeeded);
        ok_eq_hex(Status, STATUS_NO_SUCH_FILE);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&PathString, &EmptyString);
    ok_eq_ustr(&FileNameString, &EmptyString);
    ok_eq_ustr(&ExtensionString, &EmptyString);
    ok_eq_ustr(&CallerBuffer, &EmptyString);
    ok_eq_ustr(&DynamicString, &EmptyString);
    ok_eq_pointer(FullNameOut, NULL);
    ok_eq_ulong(FilePartSize, 0UL);
    ok_eq_ulong(LengthNeeded, 0UL);

    /* Buffer overflow test
     * length(longDirName) + length(longFileName) + length(ext) = MAX_PATH
     */
    RtlInitUnicodeString(&PathString, L"C:\\Program Files\\Very_long_test_path_which_can_trigger_heap_overflow_test_1234567890______________________________________________________AB");
    RtlInitUnicodeString(&FileNameString, L"this_is_long_file_name_for_checking______________________________________________________________________________CD");
    RtlInitUnicodeString(&ExtensionString, L".txt");
    StartSeh()
        Status = RtlDosSearchPath_Ustr(0,
                                       &PathString,
                                       &FileNameString,
                                       &ExtensionString,
                                       &CallerBuffer,
                                       &DynamicString,
                                       &FullNameOut,
                                       &FilePartSize,
                                       &LengthNeeded);
        ok_eq_hex(Status, STATUS_NO_SUCH_FILE);
    EndSeh(STATUS_SUCCESS);
}
