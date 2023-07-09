#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>

void __cdecl main(void);
void foo (void);
void foo1(void);
void foo2(void);
void foo3(void);
void WalkTheStack(void);

void TestFindExecutableImage( void );

void __cdecl
main(void)
{
    puts("Entering main");
    foo();

    TestFindExecutableImage();

    puts("Ending main");
}

void
TestFindExecutableImage(
    void
    )
{
    HANDLE Handle;
    CHAR szCorrectName[MAX_PATH];
    CHAR szActualName [MAX_PATH];
    CHAR szTestPath[MAX_PATH];
    CHAR szDrive[_MAX_DRIVE];
    CHAR szDir[_MAX_DIR];
    CHAR *FilePart;
    DWORD ErrorCount = 0;

    _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);

    strcpy(szTestPath, szDrive);
    strcat(szTestPath, szDir);

    GetFullPathName(_pgmptr, MAX_PATH, szCorrectName, &FilePart);

    __try {
        Handle = FindExecutableImage(FilePart, szTestPath, szActualName);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = NULL;
        printf("ERROR: FindExecutableImage GPF (test %d)\n", 1);
        ErrorCount++;
    }

    if (Handle == NULL) {
        printf("ERROR: FindExecutableImage (test %d) failed\n", 1);
        ErrorCount++;
    } else {
        CloseHandle(Handle);
        if (strcmp(szCorrectName, szActualName)) {
            printf("ERROR: FindExecutableImage() (test %d) found wrong image.\nExpected: %s\nFound: %s\n", 1, szCorrectName, szActualName);
            ErrorCount++;
        }
    }

    // Test long paths to ExpandPath()
    strcat(szTestPath, ";%path%;%path%;%path%;%path%;%path%;%path%;%path%;%path%");

    __try {
        Handle = FindExecutableImage(FilePart, szTestPath, szActualName);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = NULL;
        printf("ERROR: FindExecutableImage GPF (test %d)\n", 2);
        ErrorCount++;
    }

    if (Handle == NULL) {
        printf("ERROR: FindExecutableImage (test %d) failed\n", 2);
        ErrorCount++;
    } else {
        CloseHandle(Handle);
        if (strcmp(szCorrectName, szActualName)) {
            printf("ERROR: FindExecutableImage() (test %d) found wrong image.\nExpected: %s\nFound: %s\n", 2, szCorrectName, szActualName);
            ErrorCount++;
        }
    }

    // Test invalid paths (should return failure)
    szTestPath[0] = '\0';

    __try {
        Handle = FindExecutableImage(FilePart, szTestPath, szActualName);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = NULL;
        printf("ERROR: FindExecutableImage GPF (test %d)\n", 3);
        ErrorCount++;
    }

    if (Handle != NULL) {
        CloseHandle(Handle);
        printf("ERROR: FindExecutableImage (test %d) failed - Expected: <nothing>\nFound: %s\n", 3, szActualName);
        ErrorCount++;
    } else {
        if (strlen(szActualName)) {
            printf("ERROR: FindExecutableImage() (test %d) failed to clear ImageName on failure\n", 3);
            ErrorCount++;
        }
    }

    // Test NULL name (should return failure)

    __try {
        Handle = FindExecutableImage(NULL, szTestPath, szActualName);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = NULL;
        printf("ERROR: FindExecutableImage GPF (test %d)\n", 4);
        ErrorCount++;
    }

    if (Handle != NULL) {
        CloseHandle(Handle);
        printf("ERROR: FindExecutableImage (test %d) failed - Expected: <nothing>\nFound: %s\n", 4, szActualName);
        ErrorCount++;
    } else {
        if (strlen(szActualName)) {
            printf("ERROR: FindExecutableImage() (test %d) failed to clear ImageName on failure\n", 4);
            ErrorCount++;
        }
    }

    // Valid name and path, invalid end result.

    strcpy(szTestPath, szDrive);
    strcat(szTestPath, szDir);

    __try {
        Handle = FindExecutableImage(FilePart, szTestPath, NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Handle = NULL;
        printf("ERROR: FindExecutableImage GPF (test %d)\n", 5);
        ErrorCount++;
    }

    if (Handle != NULL) {
        CloseHandle(Handle);
        printf("ERROR: FindExecutableImage (test %d) failed - Supposed to fail if filepath is invalid\n", 5);
        ErrorCount++;
    }

    printf("FindExecutableImage - %s\n", ErrorCount ? "Failed" : "Passed");

    return;
}


void foo(void) {
    puts("Entering foo");
    foo1();
    puts("Ending foo");
}

void foo1(void) {
    puts("Entering foo1");
    foo2();
    puts("Ending foo1");
}

void foo2(void) {
    puts("Entering foo2");
    foo3();
    puts("Ending foo2");
}

void foo3(void) {
    puts("Entering foo3");
    WalkTheStack();
    puts("Ending foo2");
}

void
WalkTheStack(){

}
