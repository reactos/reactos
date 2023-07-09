#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

BOOL gfNotepad = FALSE;

VOID ErrorExit(VOID) {
    fprintf(stderr, "Usage: \"tdisp [-n] hex1 [hex2] [width]\"\n");
    fprintf(stderr, "  \"tdisp hex\"\n");
    fprintf(stderr, "      Displays single Unicode char <hex>\n");
    fprintf(stderr, "  \"tdisp hex1 hex2\"\n");
    fprintf(stderr, "      Displays Unicode char <hex1> through <hex2>\n");
    fprintf(stderr, "  \"tdisp hex1 hex2 n\"\n");
    fprintf(stderr, "      Displays <hex1> thru <hex2> in lines of <width> chars\n");
    fprintf(stderr, "  -n   generates a Notepad Unicode text file on stdout\n");
    exit(2);
}

VOID NotepadPrint(LPWSTR pwsz) {
    while (*pwsz) {
        if (*pwsz == 0x000a) {
            printf("%c%c", 0x0d, 0x00);
        }
        printf("%c%c", LOBYTE(*pwsz), HIBYTE(*pwsz));
        pwsz++;
    }
}

int _cdecl main(int argc, char *argv[]) {
    WCHAR wch1, wch2;
    int n = 1;
    int i;
    LPWSTR p;
    WCHAR awch[100];
    DWORD cchWritten;

    if (argc < 2) {
        ErrorExit();
    }

    if ((*argv[1] == '/') || (*argv[1] == '-')) {
        switch (argv[1][1]) {
        case 'n':
            gfNotepad = TRUE;
            /* Set "stdin" to have binary mode: */
            _setmode( _fileno( stdout ), _O_BINARY );
            printf("%c%c", 0xff, 0xfe);  // Unicode BOM
            argv++;
            break;
        default:
            ErrorExit();
        }
    }

    wch1 = (WCHAR)strtol(argv[1], NULL, 16);
    if (argc > 2) {
        wch2 = (WCHAR)strtol(argv[2], NULL, 16);
        if (argc > 3) {
            n = atoi(argv[3]);
        }
    } else {
        wch2 = wch1;
    }

    if (gfNotepad) {
        NotepadPrint(L"=======(begin)=======\n");
    } else {
        printf("=======(begin)=======\n");
    }

    while (wch1 <= wch2) {
        i = swprintf(awch, L"0x%04x : --->", wch1);
        p = &awch[i];
        for (i = 0; i < n; i++) {
            *p++ = wch1++;
        }
        *p = L'\0';
        wcscat(awch, L"<---\n");
        if (gfNotepad) {
            NotepadPrint(awch);
        } else {
            WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), awch,
                    wcslen(awch), &cchWritten, NULL);
        }
    }

    if (gfNotepad) {
        NotepadPrint(L"========(end)========\n");
    } else {
        printf("========(end)========\n");
    }

    return 0;
}
