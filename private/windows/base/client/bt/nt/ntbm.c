#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>


#define ONEK    1000
#define FIVEK   5000
#define TENK    10000
#define ONEHUNK 100000
#define ONEMIL  1000000


//
// Define local types.
//

typedef struct _PERFINFO {
    DWORD StartTime;
    DWORD StopTime;
    LPSTR Title;
    DWORD Iterations;
} PERFINFO, *PPERFINFO;

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    DWORD ContextSwitches;
    DWORD Duration;
    DWORD Performance;


    //
    // Print results and announce end of test.
    //

    PerfInfo->StopTime = GetTickCount();

    Duration = PerfInfo->StopTime - PerfInfo->StartTime;
    printf("        Test time in milliseconds %d\n", Duration);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);

    Performance = PerfInfo->Iterations * 1000 / Duration;
    printf("        Iterations per second     %d\n", Performance);


    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN DWORD Iterations,
    IN PPERFINFO PerfInfo
    )

{

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    PerfInfo->StartTime = GetTickCount();
    return;
}

HANDLE
APIENTRY
FastFindOpenDir(
    LPCWSTR lpFileName
    )

{

    HANDLE hFindFile;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING UnicodeInput;
    BOOLEAN EndsInDot;

    RtlInitUnicodeString(&UnicodeInput,lpFileName);

    //
    // Bogus code to workaround ~* problem
    //

    if ( UnicodeInput.Buffer[(UnicodeInput.Length>>1)-1] == (WCHAR)'.' ) {
        EndsInDot = TRUE;
        }
    else {
        EndsInDot = FALSE;
        }


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        return NULL;
        }

    FreeBuffer = PathName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the directory for list access
    //

    Status = NtOpenFile(
                &hFindFile,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if ( !NT_SUCCESS(Status) ) {
        return NULL;
        }
    return hFindFile;
}

BOOL
FastFind(
    HANDLE hFindFile,
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING UnicodeInput;
    BOOLEAN EndsInDot;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    CHAR Buffer[MAX_PATH*2 + sizeof(FILE_BOTH_DIR_INFORMATION)];

    RtlInitUnicodeString(&FileName,lpFileName);
    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;

    Status = NtQueryDirectoryFile(
                hFindFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                DirectoryInfo,
                sizeof(Buffer),
                FileBothDirectoryInformation,
                TRUE,
                &FileName,
                TRUE
                );

    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
        }

    //
    // Attributes are composed of the attributes returned by NT.
    //

    lpFindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
    lpFindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
    lpFindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
    lpFindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
    lpFindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
    lpFindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

    RtlMoveMemory( lpFindFileData->cFileName,
                   DirectoryInfo->FileName,
                   DirectoryInfo->FileNameLength );

    lpFindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlMoveMemory( lpFindFileData->cAlternateFileName,
                   DirectoryInfo->ShortName,
                   DirectoryInfo->ShortNameLength );

    lpFindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

    return TRUE;
}


VOID
FastFindTest(
    VOID

    )
{
    PERFINFO PerfInfo;
    int i;
    HANDLE hFind;
    WIN32_FIND_DATAW FindFileData;
    BOOL b;


    hFind = FastFindOpenDir(L"d:\\testdir\\client1\\bench");

    if ( !hFind ) {
        printf("Failed\n");
        return;
        }

    StartBenchMark(
        "FastFind Test",
        ONEK,
        &PerfInfo
        );

    for ( i=0;i<5*ONEK;i++) {

        //
        // do 5 calls 3 work, 2 don't
        //

        b = FastFind(hFind,L"a",&FindFileData);
        if ( !b ) {
            printf("Test Failure a\n");
            ExitProcess(0);
            }
        b = FastFind(hFind,L"ab",&FindFileData);
        if ( b ) {
            printf("Test Failure ab\n");
            ExitProcess(0);
            }
        b = FastFind(hFind,L"abc",&FindFileData);
        if ( !b ) {
            printf("Test Failure abc\n");
            ExitProcess(0);
            }
        b = FastFind(hFind,L"da",&FindFileData);
        if ( b ) {
            printf("Test Failure da\n");
            ExitProcess(0);
            }
        b = FastFind(hFind,L"dxxa",&FindFileData);
        if ( !b ) {
            printf("Test Failure dxxa\n");
            ExitProcess(0);
            }
        }

    FinishBenchMark(&PerfInfo);
}

VOID
FindFirstTest(
    VOID

    )
{
    PERFINFO PerfInfo;
    int i;
    HANDLE hFind;
    WIN32_FIND_DATAW FindFileData;
    BOOL b;


    StartBenchMark(
        "Stock FindFirst Test",
        ONEK,
        &PerfInfo
        );

    for ( i=0;i<5*ONEK;i++) {

        //
        // do 5 calls 3 work, 2 don't
        //

        hFind = FindFirstFileW(L"d:\\testdir\\client1\\bench\\a",&FindFileData);
        if ( hFind == INVALID_HANDLE_VALUE ) {
            printf("Test Failure a\n");
            ExitProcess(0);
            }
        FindClose(hFind);

        hFind = FindFirstFileW(L"d:\\testdir\\client1\\bench\\ab",&FindFileData);
        if ( hFind != INVALID_HANDLE_VALUE ) {
            printf("Test Failure ab\n");
            ExitProcess(0);
            }

        hFind = FindFirstFileW(L"d:\\testdir\\client1\\bench\\abc",&FindFileData);
        if ( hFind == INVALID_HANDLE_VALUE ) {
            printf("Test Failure abc\n");
            ExitProcess(0);
            }
        FindClose(hFind);


        hFind = FindFirstFileW(L"d:\\testdir\\client1\\bench\\da",&FindFileData);
        if ( hFind != INVALID_HANDLE_VALUE ) {
            printf("Test Failure da\n");
            ExitProcess(0);
            }


        hFind = FindFirstFileW(L"d:\\testdir\\client1\\bench\\dxxa",&FindFileData);
        if ( hFind == INVALID_HANDLE_VALUE ) {
            printf("Test Failure dxxa\n");
            ExitProcess(0);
            }
        FindClose(hFind);

        }

    FinishBenchMark(&PerfInfo);
}

VOID
APIENTRY
CreateOpenDirObja(
    LPCWSTR lpFileName,
    POBJECT_ATTRIBUTES Obja,
    PUNICODE_STRING PathName,
    LPCWSTR DirName
    )

{

    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    HANDLE hDir;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(DirName) ) {


        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                DirName,
                                PathName,
                                &FileName.Buffer,
                                &RelativeName
                                );

        if ( TranslationStatus ) {


            //
            // Open the directory for list access
            //

            InitializeObjectAttributes(
                Obja,
                PathName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            Status = NtOpenFile(
                        &hDir,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                        );

            if ( !NT_SUCCESS(Status) ) {
                printf("Open faild %x\n",Status);
                ExitProcess(1);
                }
            }

        }
    else {
        hDir = NULL;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        return;
        }

    if ( hDir ) {
        PathName->Buffer = PathName->Buffer + 15;
        PathName->Length -= 30;
        PathName->MaximumLength -= 30;
        }

    FreeBuffer = PathName->Buffer;

    InitializeObjectAttributes(
        Obja,
        PathName,
        OBJ_CASE_INSENSITIVE,
        hDir,
        NULL
        );
}

VOID
APIENTRY
OpenCloseDir(
    POBJECT_ATTRIBUTES Obja
    )

{

    HANDLE hDir;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Open the directory for list access
    //

    Status = NtOpenFile(
                &hDir,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Open faild %x\n",Status);
        ExitProcess(1);
        }
    NtClose(hDir);
}

VOID
OpenDirTest(
    VOID

    )
{
    PERFINFO PerfInfo;
    int i;
    HANDLE hDir;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING PathName;
    FILE_BASIC_INFORMATION BasicInfo;

#if 0
    CreateOpenDirObja(
        L"d:\\testdir\\client1\\bench",
        &Obja,
        &PathName,
        NULL
        );

    StartBenchMark(
        "Open Dir NTFS d:\\testdir\\client1\\bench",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {

        OpenCloseDir(&Obja);
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "NtQueryAttributes Dir NTFS d:\\testdir\\client1\\bench",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {

        NtQueryAttributesFile( &Obja, &BasicInfo );

        }

    FinishBenchMark(&PerfInfo);


    CreateOpenDirObja(
        L"c:\\testdir\\client1\\bench",
        &Obja,
        &PathName,
        NULL
        );

    StartBenchMark(
        "Open Dir FAT c:\\testdir\\client1\\bench",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {

        OpenCloseDir(&Obja);
        }

    FinishBenchMark(&PerfInfo);
#endif

    CreateOpenDirObja(
        L"d:\\testdir\\client1\\bench",
        &Obja,
        &PathName,
        L"d:\\"
        );

    StartBenchMark(
        "VOL Rel Open Dir NTFS d:\\testdir\\client1\\bench",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {

        OpenCloseDir(&Obja);
        }

    FinishBenchMark(&PerfInfo);


    CreateOpenDirObja(
        L"c:\\testdir\\client1\\bench",
        &Obja,
        &PathName,
        L"c:\\"
        );

    StartBenchMark(
        "Vol Rel Open Dir FAT c:\\testdir\\client1\\bench",
        FIVEK,
        &PerfInfo
        );

    for ( i=0;i<FIVEK;i++) {

        OpenCloseDir(&Obja);
        }

    FinishBenchMark(&PerfInfo);
}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    DWORD CryptoKey;
    PDWORD p;

    printf("sixeof teb %x\n",sizeof(TEB));

    CryptoKey = USER_SHARED_DATA->CryptoExponent;

    printf("Key %x\n",CryptoKey);

    p = &(USER_SHARED_DATA->CryptoExponent);
    *p = 1;

    //OpenDirTest();
    //FastFindTest();
    //FindFirstTest();

    return 0;
}
