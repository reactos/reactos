/*
 * PROJECT:     ReactOS Setup Library - Unit-tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for IsValidInstallDirectory
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

// SPFILE_EXPORTS SpFileExports = {NULL};
// SPINF_EXPORTS SpInfExports = {NULL};

START_TEST(IsValidInstallDirectory)
{
    static const struct
    {
        PCWSTR path;
        BOOLEAN result;
    } tests[] =
    {
        { L"",              FALSE },
        { L" ",             FALSE },
        { L"\\",            FALSE },
        { L"\\\\",          FALSE },
        { L".",             FALSE },
        { L"..",            FALSE },
        { L"...",           FALSE },
        { L"....",          FALSE },
        { L" WINNT",        FALSE },
        { L"WINNT ",        FALSE },
        { L".WINNT",        FALSE },
        { L"..WINNT",       FALSE },
        { L"W.INNT",        FALSE },
        { L"WI.NNT",        TRUE  },
        { L"WIN.NT",        TRUE  },
        { L"WINNT.",        TRUE  },
        { L"WINNT..",       FALSE },
        { L"WINNT. ",       FALSE },
        { L"WINNT.e e",     FALSE },
        { L"WIN^`NT",       FALSE },

        { L"WINNT",         TRUE  },
        { L"\\WINNT",       TRUE  },
        { L"\\WINNT\\",     TRUE  },
        { L"\\WINNT\\.",    FALSE },
        { L"WIN\\NT",       TRUE  },
        { L"WIN\\NT.",      TRUE  },
        { L"\\WIN\\NT\\",   TRUE  },
        { L"\\WIN\\NT.\\",  TRUE  },
        { L"\\WIN.\\NT52",  TRUE  },
        { L"\\WIN\\.\\NT52",    FALSE },
        { L"\\WIN\\..\\NT52",   FALSE },
        { L"\\WIN\\...\\NT52",  FALSE },
        { L"\\WIN\\....\\NT52", FALSE },

        { L"win.nt.5",      FALSE },
        { L"win.ntX5",      FALSE },
        { L"winn.tX5",      TRUE  },
        { L"winnt.X5",      TRUE  },
        { L"winntX.5",      TRUE  },
        { L"winntX5.",      TRUE  },
        { L"filenamee.xte", FALSE },
        { L"filenamee.xt",  FALSE },
        { L"filename.ext",  TRUE  },
        { L"file.ame.ext",  FALSE },
        { L"filenam.eext",  FALSE },

        { L"1 3 5 7 .abc",  FALSE },
        { L"12345678.  c",  FALSE },
        { L"123456789.a",   FALSE },
        { L"12345.abcd",    FALSE },
        { L"12345.ab d",    FALSE },
        { L".abc",          FALSE },
        { L"12.abc.d",      FALSE },
        { L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", FALSE },
    };

#define BOOL_TO_STR(b) ((b) ? "TRUE" : "FALSE")

    UINT i;
    for (i = 0; i < _countof(tests); ++i)
    {
        BOOLEAN ret = IsValidInstallDirectory(tests[i].path);
        ok(ret == tests[i].result,
           "Unexpected result for '%S', got %s, expected %s\n",
           tests[i].path, BOOL_TO_STR(ret), BOOL_TO_STR(tests[i].result));
    }

#undef BOOL_TO_STR
}

/* EOF */
