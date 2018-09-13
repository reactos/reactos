#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>
#include <winineti.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

void _CRTAPI1 main(int, char**);
void usage(void);

void _CRTAPI1 main(int argc, char** argv) {

    LPSTR name = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
                break;
            }
        } else if (!name) {
            name = *argv;
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    if (!name) {
        usage();
    }

    char buffer[1024];
    DWORD buflen = sizeof(buffer);
    BOOL ok = CreateUrlCacheContainer(name,
                                      "CreatConTest",
                                      "",
                                      10,
                                      INTERNET_CACHE_CONTAINER_AUTODELETE,
                                      0,
                                      (LPVOID)buffer,
                                      &buflen
                                      );

    if (!ok) {
        printf("CreateUrlCacheContainer() returns %d\n", GetLastError());
    }
    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("usage: creatcon <cache container name>\n"
           );
    exit(1);
}
