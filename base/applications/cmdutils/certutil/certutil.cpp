/*
 * PROJECT:     ReactOS certutil
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CertUtil commandline handling
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 *
 * Note: Only -hashfile and -asn are implemented for now, the rest is not present!
 */

#include "precomp.h"
#include <wincrypt.h>
#include <stdlib.h>

typedef struct
{
    LPCWSTR Name;
    BOOL (*pfn)(LPCWSTR Filename);
} Verb;


Verb verbs[] = {
    { L"hashfile", hash_file },
    { L"asn", asn_dump },
};

static void print_usage()
{
    ConPuts(StdOut, L"Verbs:\n");
    ConPuts(StdOut, L"  -hashfile           -- Display cryptographic hash over a file\n");
    ConPuts(StdOut, L"  -asn                -- Display ASN.1 encoding of a file\n");
    ConPuts(StdOut, L"\n");
    ConPuts(StdOut, L"CertUtil -?           -- Display a list of all verbs\n");
    ConPuts(StdOut, L"CertUtil -hashfile -? -- Display help text for the 'hashfile' verb\n");
}


Verb* MatchVerb(LPCWSTR arg)
{
    if (arg[0] != '-' && arg[0] != '/')
        return NULL;

    for (size_t n = 0; n < RTL_NUMBER_OF(verbs); ++n)
    {
        if (!_wcsicmp(verbs[n].Name, arg + 1))
        {
            return verbs + n;
        }
    }

    return NULL;
}

int wmain(int argc, WCHAR *argv[])
{
    int n;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1) /* i.e. no commandline arguments given */
    {
        print_usage();
        return EXIT_SUCCESS;
    }

    for (n = 1; n < argc; ++n)
    {
        if (!_wcsicmp(argv[n], L"-?"))
        {
            print_usage();
            return EXIT_SUCCESS;
        }

        Verb* verb = MatchVerb(argv[n]);

        if (verb)
        {
            if (argc != 3)
            {
                ConPrintf(StdOut, L"CertUtil: -%s expected 1 argument, got %d\n", verb->Name, argc - 2);
                return EXIT_FAILURE;
            }

            if (!_wcsicmp(argv[n+1], L"-?"))
            {
                print_usage();
                return EXIT_SUCCESS;
            }

            if (!verb->pfn(argv[n+1]))
            {
                /* The verb prints the failure */
                return EXIT_FAILURE;
            }

            ConPrintf(StdOut, L"CertUtil: -%s command completed successfully\n", verb->Name);
            return EXIT_SUCCESS;
        }
        else
        {
            ConPrintf(StdOut, L"CertUtil: Unknown verb: %s\n", argv[n]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
