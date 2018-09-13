#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <imagehlp.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

typedef BOOL (* SYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL (* SYMLOADMODULE)(HANDLE, HANDLE, PSTR, PSTR, DWORD, DWORD);
typedef BOOL (* SYMGETSYMFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (* SYMCLEANUP)(HANDLE);

void _CRTAPI1 main(int, char**);
void usage(void);
VOID InitSymLib(VOID);
VOID TermSymLib(VOID);
LPSTR GetDebugSymbol(DWORD Address, LPDWORD Offset);

void _CRTAPI1 main(int argc, char** argv) {

    char * fni = NULL;
    char * fno = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
                break;
            }
        } else if (fno) {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        } else if (fni) {
            fno = *argv;
        } else {
            fni = *argv;
        }
    }

    if (!fni || !fno) {
        usage();
    }

    FILE * fpi = fopen(fni, "rt");

    if (!fpi) {
        printf("error: cannot open file \"%s\" for read\n", fni);
        exit(1);
    }

    FILE * fpo = fopen(fno, "wt");

    if (!fpo) {
        printf("error: cannot open file \"%s\" for write\n", fno);
        exit(1);
    }

    InitSymLib();

    while (!feof(fpi)) {

        char buf[1024];

        if (!fgets(buf, sizeof(buf), fpi)) {
            break;
        }

        char * p = strstr(buf, "+0x");

        if (p) {

            DWORD val = (DWORD)strtoul(p, NULL, 0);
            DWORD offset;
            char * str = GetDebugSymbol(val, &offset);

            if (str) {
                sprintf(p, "%s+%#x\n", str, offset);
            }
        }
        fwrite(buf, strlen(buf), 1, fpo);
    }
    fclose(fpi);
    fclose(fpo);
    TermSymLib();
}

void usage() {
    printf("usage: convsym <input_file> <output_file>\n"
           );
    exit(1);
}

HMODULE hSymLib = NULL;
SYMINITIALIZE pSymInitialize = NULL;
SYMLOADMODULE pSymLoadModule = NULL;
SYMGETSYMFROMADDR pSymGetSymFromAddr = NULL;
SYMCLEANUP pSymCleanup = NULL;

VOID InitSymLib(VOID) {
    if (hSymLib == NULL) {
        hSymLib = LoadLibrary("IMAGEHLP.DLL");
        if (hSymLib != NULL) {
            pSymInitialize = (SYMINITIALIZE)GetProcAddress(hSymLib,
                                                           "SymInitialize"
                                                           );
            pSymLoadModule = (SYMLOADMODULE)GetProcAddress(hSymLib,
                                                           "SymLoadModule"
                                                           );
            pSymGetSymFromAddr = (SYMGETSYMFROMADDR)GetProcAddress(hSymLib,
                                                                   "SymGetSymFromAddr"
                                                                   );
            pSymCleanup = (SYMCLEANUP)GetProcAddress(hSymLib,
                                                     "SymCleanup"
                                                     );
            if (!pSymInitialize
            || !pSymLoadModule
            || !pSymGetSymFromAddr
            || !pSymCleanup) {
                FreeLibrary(hSymLib);
                hSymLib = NULL;
                pSymInitialize = NULL;
                pSymLoadModule = NULL;
                pSymGetSymFromAddr = NULL;
                pSymCleanup = NULL;
                return;
            }
        }
        pSymInitialize(GetCurrentProcess(), NULL, FALSE);
        //SymInitialize(GetCurrentProcess(), NULL, TRUE);
        pSymLoadModule(GetCurrentProcess(), NULL, "WININET.DLL", "WININET", 0, 0);
    }
}

VOID TermSymLib(VOID) {
    if (pSymCleanup) {
        pSymCleanup(GetCurrentProcess());
        FreeLibrary(hSymLib);
    }
}

LPSTR GetDebugSymbol(DWORD Address, LPDWORD Offset) {
    *Offset = Address;
    if (!pSymGetSymFromAddr) {
        return "";
    }

    static char symBuf[512];

    //((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    ((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(symBuf);
    ((PIMAGEHLP_SYMBOL)symBuf)->MaxNameLength = sizeof(symBuf) - sizeof(IMAGEHLP_SYMBOL);
    if (!pSymGetSymFromAddr(GetCurrentProcess(),
                            Address,
                            Offset,
                            (PIMAGEHLP_SYMBOL)symBuf)) {
        ((PIMAGEHLP_SYMBOL)symBuf)->Name[0] = '\0';
    }
    return ((PIMAGEHLP_SYMBOL)symBuf)->Name;
}
