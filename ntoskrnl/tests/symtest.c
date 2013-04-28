// ntest.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "windows.h"
#include "ntndk.h"

ULONG Runs;
ULONG Failures;

#define TEST_ASSERT( exp ) \
    ((!(exp)) ? \
    (printf("%s, %s, %d\n", #exp, __FILE__, __LINE__), Failures++, FALSE) : \
    TRUE), Runs++

HANDLE
SymlinkCreateTests(OUT PHANDLE OddLink)
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    UNICODE_STRING TargetName = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING TargetName2 = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING TargetName3 = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING TargetName4 = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING TargetName5 = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING OkName = RTL_CONSTANT_STRING(L"\\OddLink");
    UNICODE_STRING OkName2 = RTL_CONSTANT_STRING(L"\\TestLink");

    //
    // Test1: Empty Attributes
    //
    {
    OBJECT_ATTRIBUTES Test1 = RTL_INIT_OBJECT_ATTRIBUTES(NULL, 0);
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test1,
                                        &TargetName);
    TEST_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Test2: No Attributes
    //
    {
    POBJECT_ATTRIBUTES Test2 = NULL;
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        Test2,
                                        &TargetName);
    TEST_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Test3: Attributes with an empty name
    //
    {
    UNICODE_STRING TestName1 = {0, 0, NULL};
    OBJECT_ATTRIBUTES Test3 = RTL_INIT_OBJECT_ATTRIBUTES(&TestName1, 0);
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test3,
                                        &TargetName);
    TEST_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Test4: Attributes with an invalid name
    //
    {
    UNICODE_STRING TestName2 = {10, 12, UlongToPtr(0x81000000)};
    OBJECT_ATTRIBUTES Test4 = RTL_INIT_OBJECT_ATTRIBUTES(&TestName2, 0);
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test4,
                                        &TargetName);
    TEST_ASSERT(Status == STATUS_ACCESS_VIOLATION);
    }

    //
    // Test5: Target with an odd name len
    //
    {
    UNICODE_STRING OddName = RTL_CONSTANT_STRING(L"\\TestLink");
    OBJECT_ATTRIBUTES Test5 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName, 0);
    TargetName3.Length--;
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test5,
                                        &TargetName3);
    TEST_ASSERT(Status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test6: Target with an emtpy name len
    //
    {
    OBJECT_ATTRIBUTES Test5 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName, 0);
    TargetName4.MaximumLength = 0;
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test5,
                                        &TargetName4);
    TEST_ASSERT(Status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test7: Target with an name len larger then maxlen
    //
    {
    OBJECT_ATTRIBUTES Test6 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName, 0);
    TargetName5.Length = TargetName5.MaximumLength + 2;
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test6,
                                        &TargetName5);
    TEST_ASSERT(Status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test5: Target with an odd name maxlen
    //
    {
    OBJECT_ATTRIBUTES Test5 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName, 0);
    TargetName2.MaximumLength--;
    TEST_ASSERT(TargetName2.MaximumLength % sizeof(WCHAR));
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test5,
                                        &TargetName2);
    *OddLink = LinkHandle;
    TEST_ASSERT(NT_SUCCESS(Status));
    TEST_ASSERT(TargetName2.MaximumLength % sizeof(WCHAR));
    }

    //
    // Test6: collission
    //
    {
    OBJECT_ATTRIBUTES Test6 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName, 0);
    TargetName2.MaximumLength++;
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test6,
                                        &TargetName2);
    TEST_ASSERT(Status == STATUS_OBJECT_NAME_COLLISION);
    }

    //
    // Test7: OK!
    //
    {
    OBJECT_ATTRIBUTES Test7 = RTL_INIT_OBJECT_ATTRIBUTES(&OkName2, 0);
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &Test7,
                                        &TargetName2);
    TEST_ASSERT(NT_SUCCESS(Status));
    }

    //
    // Return the last valid handle
    //
    return LinkHandle;
};


int _tmain(int argc, _TCHAR* argv[])
{
    NTSTATUS Status;
    HANDLE LinkHandle, OddHandle;
    WCHAR TargetBuffer[MAX_PATH] = {0};
    WCHAR TargetBuffer2[MAX_PATH] = {0};
    UNICODE_STRING TargetName;
    UNICODE_STRING TargetName2;
    UNICODE_STRING OkName = RTL_CONSTANT_STRING(L"\\TestLink");
    ULONG NameSize;

    //
    // Start with the create tests
    //
    LinkHandle = SymlinkCreateTests(&OddHandle);

    //
    // Setup the two empty strings. One will have a magic-char at the end
    //
    RtlInitEmptyUnicodeString(&TargetName, TargetBuffer, sizeof(TargetBuffer));
    RtlInitEmptyUnicodeString(&TargetName2, TargetBuffer2, sizeof(TargetBuffer2));

    //
    // Now query the odd link
    //
    Status = NtQuerySymbolicLinkObject(OddHandle,
                                       &TargetName,
                                       &NameSize);
    TEST_ASSERT(NT_SUCCESS(Status));
    TEST_ASSERT(NameSize == sizeof(WCHAR));
    TEST_ASSERT(TargetName.Length == NameSize);
    TEST_ASSERT(TargetName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR);
    NtClose(OddHandle);

    //
    // Now query the test link
    //
    Status = NtQuerySymbolicLinkObject(LinkHandle,
                                       &TargetName,
                                       &NameSize);
    TEST_ASSERT(NT_SUCCESS(Status));
    TEST_ASSERT(NameSize == 2 * sizeof(WCHAR));
    TEST_ASSERT(TargetName.Length == NameSize - sizeof(WCHAR));
    TEST_ASSERT(TargetName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR);

    //
    // Corrupt the characters
    //
    TargetName.Buffer[(NameSize - 2) / sizeof(WCHAR)] = 'v';
    TargetName.Buffer[(NameSize - 3) / sizeof(WCHAR)] = 'v';

    //
    // Now query the test link
    //
    Status = NtQuerySymbolicLinkObject(LinkHandle,
                                       &TargetName,
                                       &NameSize);
    TEST_ASSERT(NT_SUCCESS(Status));
    TEST_ASSERT(TargetName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR);
    TEST_ASSERT(TargetName.Buffer[1] == UNICODE_NULL);

    //
    // Corrupt the characters
    //
    TargetName.Buffer[(NameSize - 2) / sizeof(WCHAR)] = 'v';
    TargetName.Buffer[(NameSize - 3) / sizeof(WCHAR)] = 'v';

    //
    // Now query the test link
    //
    Status = NtQuerySymbolicLinkObject(LinkHandle,
                                       &TargetName,
                                       NULL);
    TEST_ASSERT(NT_SUCCESS(Status));
    TEST_ASSERT(TargetName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR);
    TEST_ASSERT(TargetName.Buffer[1] == 'v');

    //
    // Print out results
    //
    {
    RTL_OSVERSIONINFOW VersionInfo;
    VersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
    RtlGetVersion(&VersionInfo);
    printf("Test complete on: %d\n", VersionInfo.dwBuildNumber);
    printf("Number of Failures: %d\n", Failures);
    printf("Number of Runs: %d\n", Runs);
    }
	return 0;
}

