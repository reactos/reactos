/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    certify.cxx

Abstract:

    This is the command line tool to manipulate certificates on an executable image.

Author:

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#include <private.h>

#if 1
#define TEST 1
#else
#define TEST 0
#endif

void
PrintUsage(
    VOID)
{
    fputs("usage: CERTIFY [switches] image-names... \n"
          "            [-?] display this message\n"
          "            [-l] list the certificates in an image\n"
          "            [-a:<Filename>] add a certificate file to an image\n"
          "            [-r:<index>]    remove certificate <index> from an image\n"
          "            [-g:<Filename>] update any associated .DBG file\n"
          "            [-s:<Filename>] used with -r to save the removed certificate\n",
          stderr
         );
    exit(-1);
}

#if TEST
// Test routine
BOOL  fAllDataReturned;
PVOID pvDataRefTest;
DWORD FileSize;
DWORD DataRead;

BOOL
WINAPI
DigestRoutine (
    DIGEST_HANDLE   DataReference,
    PBYTE           pData,
    DWORD           dwLength
    )
{
    if (DataReference != pvDataRefTest) {
        return(FALSE);
    }

    // Attempt to read the range

    if (IsBadReadPtr(pData, dwLength)) {
        return(FALSE);
    }

    DataRead += dwLength;
    if (DataRead > FileSize) {
        return(FALSE);
    }

    return(TRUE);
}

#endif


int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *p;

    if (argc < 2) {
        PrintUsage();
    }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
                case '?':
                    PrintUsage();
                    break;

                case 'A':
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {

                        // Add a certificate file to an image.
#if TEST
                        // Test code

                        WIN_CERTIFICATE wc;
                        HANDLE  Handle;
                        DWORD   Index;

                        wc.dwLength = sizeof(WIN_CERTIFICATE);
                        wc.wCertificateType = WIN_CERT_TYPE_X509;

                        if ((Handle = CreateFile(TEXT("test.exe"),
                                    GENERIC_WRITE | GENERIC_READ,
                                    0,
                                    0,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL)) == INVALID_HANDLE_VALUE)
                        {
                            fputs("Unable to open test.exe", stderr);
                            exit(1);
                        }

                        printf("ImageAddCertificate on test.exe returned: %d\n",
                                ImageAddCertificate(Handle, &wc, &Index));

                        printf("Index #: %d\n", Index);

                        CloseHandle(Handle);

                        exit(0);
#else
                    // The real code
#endif
                    }
                    break;

                case 'L':
                    // List the certificates in an image.
#if TEST
                    // Test code
                    WIN_CERTIFICATE wc;
                    HANDLE  Handle;
                    DWORD   Index;

                    if ((Handle = CreateFile(TEXT("test.exe"),
                                GENERIC_READ,
                                0,
                                0,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL)) == INVALID_HANDLE_VALUE)
                    {
                        fputs("Unable to open test.exe", stderr);
                        exit(1);
                    }

                    ImageEnumerateCertificates(Handle, CERT_SECTION_TYPE_ANY, &Index, NULL, 0);

                    printf("Enumerate lists: %d\n", Index);

                    Index--;

                    while (ImageGetCertificateHeader(Handle, Index, &wc)) {
                        printf("Index: %d\n", Index);
                        Index--;
                    }

                    CloseHandle(Handle);

                    exit(0);

#else
                    // The real code
#endif
                    break;

                case 'R':
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {
                        // Remove a specific certificate from an image.
#if TEST
                        // Test code

                        HANDLE  Handle;

                        if ((Handle = CreateFile(TEXT("test.exe"),
                                    GENERIC_WRITE | GENERIC_READ,
                                    0,
                                    0,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL)) == INVALID_HANDLE_VALUE)
                        {
                            fputs("Unable to open test.exe", stderr);
                            exit(1);
                        }

                        printf("ImageRemoveCertificate(0) on test.exe returned: %d\n",
                            ImageRemoveCertificate(Handle, 0));
                        exit(0);
#else
                        // The real code
#endif
                    }
                    break;

                case 'G':
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {
                        // Generate a certificate from an image.
#if TEST
                        // Test code

                        HANDLE  Handle;

                        if ((Handle = CreateFile(TEXT("test.exe"),
                                    GENERIC_READ,
                                    0,
                                    0,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL)) == INVALID_HANDLE_VALUE)
                        {
                            fputs("Unable to open test.exe", stderr);
                            exit(1);
                        }

                        FileSize = GetFileSize(Handle, NULL);
                        DataRead = 0;

                        pvDataRefTest = (PVOID) 1;
                        printf("ImageGetDigestStream debug w/o resources on test.exe returned: %s\tGetLastError(): %d\n",
                            ImageGetDigestStream(Handle,
                                                 CERT_PE_IMAGE_DIGEST_DEBUG_INFO,
                                                 DigestRoutine, pvDataRefTest) ? "TRUE" : "FALSE",
                            GetLastError());
                        printf("Message Stream Size: %d\n", DataRead);

                        DataRead = 0;
                        pvDataRefTest = (PVOID) 2;
                        printf("ImageGetDigestStream debug w/ resources test.exe returned: %s\tGetLastError(): %d\n",
                            ImageGetDigestStream(Handle,
                                                 CERT_PE_IMAGE_DIGEST_DEBUG_INFO | CERT_PE_IMAGE_DIGEST_RESOURCES,
                                                 DigestRoutine, pvDataRefTest) ? "TRUE" : "FALSE",
                            GetLastError());
                        printf("Message Stream Size: %d\n", DataRead);

                        DataRead = 0;
                        pvDataRefTest = (PVOID) 3;
                        printf("ImageGetDigestStream w/o debug w/o resources on test.exe returned: %s\tGetLastError(): %d\n",
                            ImageGetDigestStream(Handle,
                                                 0,
                                                 DigestRoutine, pvDataRefTest) ? "TRUE" : "FALSE",
                            GetLastError());
                        printf("Message Stream Size: %d\n", DataRead);

                        DataRead = 0;
                        pvDataRefTest = (PVOID) 4;
                        printf("ImageGetDigestStream w/o debug w/ resources test.exe returned: %s\tGetLastError(): %d\n",
                            ImageGetDigestStream(Handle,
                                                 CERT_PE_IMAGE_DIGEST_RESOURCES,
                                                 DigestRoutine, pvDataRefTest) ? "TRUE" : "FALSE",
                            GetLastError());
                        printf("Message Stream Size: %d\n", DataRead);

                        exit(0);

#else
                        // Real code
#endif
                    }
                    break;

                case 'S':
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {
                        // Save the certificate in some file.
                    }
                    break;

                default:
                    fprintf( stderr, "CERTIFY: Invalid switch - /%c\n", c );
                    PrintUsage();
                    break;
            }
        }
    }

    return 0;
}
