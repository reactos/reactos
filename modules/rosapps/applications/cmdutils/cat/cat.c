/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS conCATenation tool
 * FILE:            cmdutils/cat/cat.c
 * PURPOSE:         Concatenates STDIN or an arbitrary number of files to STDOUT
 * PROGRAMMERS:     David Welch
 *                  Semyon Novikov (tappak)
 *                  Hermès Bélusca - Maïto
 */

#include <stdio.h>

#ifdef _WIN32
#include <string.h>         // Required for _stricmp()
#include <fcntl.h>          // Required for _setmode flags
#include <io.h>             // Required for _setmode()
#else
#include <strings.h>        // Required for strcasecmp()
#define O_TEXT   0x4000
#define O_BINARY 0x8000
#define _setmode(fd, mode)  // This function is useless in *nix world
#define _stricmp strcasecmp
#endif

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

void help(void)
{
    fprintf(stdout,
            "\n"
            "ReactOS File Concatenation Tool\n"
            "\n"
            "Usage: cat [options] [file [...]]\n"
            "options - Currently ignored\n");
}

int main(int argc, char* argv[])
{
    int i;
    FILE* in;
    unsigned char buff[512];
    size_t cnt, readcnt;

    if (argc >= 2)
    {
        if (_stricmp(argv[1], "-h"    ) == 0 ||
            _stricmp(argv[1], "--help") == 0 ||
            _stricmp(argv[1], "/?"    ) == 0 ||
            _stricmp(argv[1], "/help" ) == 0)
        {
            help();
            return 0;
        }
    }

    /* Set STDOUT to binary */
    _setmode(_fileno(stdout), _O_BINARY);

    /* Special case where we run 'cat' without any argument: we use STDIN */
    if (argc <= 1)
    {
        unsigned int ch;

        /* Set STDIN to binary */
        _setmode(_fileno(stdin), _O_BINARY);

#if 0 // Version using feof()
        ch = fgetc(stdin);
        while (!feof(stdin))
        {
            putchar(ch);
            ch = fgetc(stdin);
        }
#else
        while ((ch = fgetc(stdin)) != EOF)
        {
            putchar(ch);
        }
#endif

        return 0;
    }

    /* We have files: read them and output them to STDOUT */
    for (i = 1; i < argc; i++)
    {
        /* Open the file in binary read mode */
        in = fopen(argv[i], "rb");
        if (in == NULL)
        {
            fprintf(stderr, "Failed to open file '%s'\n", argv[i]);
            return -1;
        }

        /* Dump the file to STDOUT */
        cnt = 0; readcnt = 0;
        while (readcnt == cnt)
        {
            /* Read data from the input file */
            cnt = ARRAYSIZE(buff);
            readcnt = fread(&buff, sizeof(buff[0]), cnt, in);
            if (readcnt != cnt)
            {
                /*
                 * The real number of read bytes differs from the number of bytes
                 * we wanted to read, so either a reading error occurred, or EOF
                 * was reached while reading. Bail out if it is a reading error.
                 */
                if (!feof(in))
                {
                    fprintf(stderr, "Error while reading file '%s'\n", argv[i]);
                    fclose(in);
                    return -1;
                }
            }

            /* Nothing to be read anymore, so we can gracefully break */
            if (readcnt == 0) break;

            /* Write data to STDOUT */
            fwrite(&buff, sizeof(buff[0]), readcnt, stdout);
        }

        /* Finally close the file */
        fclose(in);
    }

    return 0;
}

/* EOF */
