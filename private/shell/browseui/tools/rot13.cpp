//***
// SYNOPSIS
//  rot13 [-q] [-p:prefix] [-l:#] < file
//
//  -p prefix   only munge stuff between <prefix> and end-of-line
//  -q          only munge stuff between quotes
//  -l #        only munge stuff >= # chars long
//
//  -q is good for registry dumps.
//  -p prefix is good for ???.
// NOTES
//  NYI: -l #
//  NYI: ':' between arg and modifier
#include <stdlib.h>
#include <stdio.h>

#define TRUE        1
#define FALSE       0

#define TEXT(x)     x

void rot13(FILE *fpIn, FILE *fpOut);

char *PszPrefix;
int FQuote;

void usage()
{
    fprintf(stderr, "usage: rot13 [-p prefix] [-q] < file");
    exit(2);
}

int _cdecl main(int argc, char **argv)
{
    --argc; ++argv;

    for ( ; *argv != NULL; --argc, ++argv) {
        if (argv[0][0] != TEXT('-'))
            break;
        switch (argv[0][1]) {
        case TEXT('p'):
            --argc; ++argv;
            PszPrefix = *argv;
            break;
        case TEXT('q'):
            FQuote = TRUE;
            break;
        default:
            usage();
            break;
        }
    }

    rot13(stdin, stdout);
    return 0;
}

#define ROT13(i)    (((i) + 13) % 26)

#define ST_BEG  1
#define ST_MID  2
#define ST_END  3

void rot13(FILE *fpIn, FILE *fpOut)
{
    int fRot;
    int state;
    int fInQuote;
    char *pszPre;
    int ch;

    state = ST_BEG;
    fInQuote = FALSE;
    while ((ch = getc(fpIn)) != EOF) {
        fRot = !(PszPrefix || FQuote);
        if (PszPrefix) {
            switch (state) {
            case ST_BEG:
                if (ch == *PszPrefix) {
                    pszPre = PszPrefix + 1;
                    state = ST_MID;
                }
                break;
            case ST_MID:
                if (*pszPre == 0) {
                    state = ST_END;
                    goto Lend;
                }
                else if (*pszPre++ == ch)
                    ;
                else
                    state = ST_BEG;
                break;
            case ST_END:
        Lend:
                if (ch == TEXT('\n'))
                    state = ST_BEG;
                break;
            }

            if (state == ST_END)
                fRot = TRUE;
        }

        if (FQuote) {
            // todo: <\">, <\'>
            if (ch == TEXT('"') || ch == TEXT('\''))
                fInQuote = !fInQuote;
            if (fInQuote)
                fRot = TRUE;
        }

        if (fRot) {
            if (TEXT('a') <= ch && ch <= TEXT('z'))
                ch = TEXT('a') + ROT13(ch - TEXT('a'));
            else if (TEXT('A') <= ch && ch <= TEXT('Z'))
                ch = TEXT('A') + ROT13(ch - TEXT('A'));
            else
                ;
        }
        putc(ch, fpOut);
    }

    return;
}
