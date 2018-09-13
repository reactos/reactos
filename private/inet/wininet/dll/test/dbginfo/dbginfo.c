#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <windows.h>
#include <wininet.h>
#include <wininetd.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

void _CRTAPI1 main(int, char**);
LPVOID get_and_dump_debug_info(LPDWORD);
void dump_internet_debug_info(LPINTERNET_DEBUG_INFO);

void _CRTAPI1 main(int argc, char** argv) {

    BOOL ok;
    DWORD length;
    LPVOID buf;
    DWORD inlength;
    LPVOID buf2;
    HINTERNET hInternet;
    HINTERNET hGopher;
    char locator[MAX_GOPHER_LOCATOR_LENGTH + 1];
    HINTERNET hFind;
    GOPHER_FIND_DATA data;

    buf = get_and_dump_debug_info(&length);
    inlength = length;
    length += sizeof("mydebug.log");
    buf2 = malloc(length);
    if (!buf2) {
        printf("error: failed to allocate %d bytes\n", length);
        exit(1);
    }

    memcpy(buf2, buf, inlength);
    free(buf);

    ((LPINTERNET_DEBUG_INFO)buf2)->CategoryFlags = 0xffffffff;
    ((LPINTERNET_DEBUG_INFO)buf2)->ControlFlags = 0x7df;
    strcpy(((LPINTERNET_DEBUG_INFO)buf2)->Filename, "mydebug.log");

    ok = InternetSetOption(NULL,
                           INTERNET_OPTION_SET_DEBUG_INFO,
                           buf2,
                           length
                           );
    if (!ok) {
        printf("error: InternetSetOption() returns %d\n", GetLastError());
        exit(1);
    }

    //
    // make sure we set it
    //

    buf = get_and_dump_debug_info(&length);
    free(buf);

    //
    // try generating some debug info
    //

    hInternet = InternetOpen("dbginfo",
                             PRE_CONFIG_INTERNET_ACCESS,
                             NULL,
                             0,
                             0
                             );
    if (!hInternet) {
        printf("error: InternetOpen() returns %d\n", GetLastError());
        exit(1);
    }

    hGopher = InternetConnect(hInternet,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              INTERNET_SERVICE_GOPHER,
                              0,
                              0
                              );
    if (!hGopher) {
        printf("error: InternetConnect() returns %d\n", GetLastError());
    }

    length = sizeof(locator);
    if (!GopherCreateLocator("rfirthmips",
                             0,
                             NULL,
                             NULL,
                             GOPHER_TYPE_DIRECTORY,
                             locator,
                             &length
                             )) {
        printf("error: GopherCreateLocator() returns %d\n", GetLastError());
        exit(1);
    }

    hFind = GopherFindFirstFile(hGopher,
                                locator,
                                NULL,
                                &data,
                                0
                                );
    if (!hFind) {
        printf("error: GopherFindFirstFile() returns %d\n", GetLastError());
    }

    free(buf2);
    exit(0);
}

LPVOID get_and_dump_debug_info(LPDWORD outLen) {

    BOOL ok;
    DWORD length;
    DWORD error;
    LPVOID buf;
    DWORD inlength;

    length = 0;
    ok = InternetQueryOption(NULL,
                             INTERNET_OPTION_GET_DEBUG_INFO,
                             (LPVOID)0x1234,
                             &length
                             );
    if (ok) {
        printf("error: InternetQueryOption() with zero length returns TRUE\n");
        exit(1);
    }

    error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER) {
        printf("error: InternetQueryOption() with zero length returns %d\n", error);
    }

    printf("InternetQueryOption() with zero length returns %d\n", error);
    printf("length = %d\n", length);

    length = 65535;
    ok = InternetQueryOption(NULL,
                             INTERNET_OPTION_GET_DEBUG_INFO,
                             NULL,
                             &length
                             );
    if (ok) {
        printf("error: InternetQueryOption() with no buffer returns %d\n", error);
        exit(1);
    }

    error = GetLastError();
    if (error != ERROR_INSUFFICIENT_BUFFER) {
        printf("error: InternetQueryOption() with no buffer returns %d\n", error);
    }

    printf("InternetQueryOption() with no buffer returns %d\n", error);
    printf("length = %d\n", length);

    buf = malloc(length);
    if (!buf) {
        printf("error: failed to allocate %d bytes\n", length);
        exit(1);
    }

    inlength = length;
    ok = InternetQueryOption(NULL,
                             INTERNET_OPTION_GET_DEBUG_INFO,
                             buf,
                             &length
                             );
    if (!ok) {
        printf("error: InternetQueryOption() with %d byte buffer returns %d. Length = %d\n",
                inlength,
                GetLastError(),
                length
                );
        exit(1);
    }
    printf("InternetQueryOption() returns %d byte buffer\n", length);
    dump_internet_debug_info((LPINTERNET_DEBUG_INFO)buf);
    *outLen = length;
    return buf;
}

void dump_internet_debug_info(LPINTERNET_DEBUG_INFO info) {
    printf("ErrorLevel      = %d\n"
           "ControlFlags    = %x\n"
           "CategoryFlags   = %x\n"
           "BreakFlags      = %x\n"
           "IndentIncrement = %d\n"
           "Filename        = \"%s\"\n"
           "\n",
           info->ErrorLevel,
           info->ControlFlags,
           info->CategoryFlags,
           info->BreakFlags,
           info->IndentIncrement,
           info->Filename
           );
}
