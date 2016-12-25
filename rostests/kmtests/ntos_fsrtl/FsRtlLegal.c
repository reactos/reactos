/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for FsRtlIsFatDbcsLegal/FsRtlIsHpfsDbcsLegal
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

static struct
{
    PCSTR Dbcs;
    BOOLEAN LegalFat;
    BOOLEAN LegalHpfs;
    BOOLEAN HasWildCards;
    BOOLEAN IsPath;
    BOOLEAN LeadingBackslash;
} Tests[] =
{
    { "",                   FALSE, FALSE                      },
    { "a",                  TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "A",                  TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { ".",                  TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "..",                 TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "...",                FALSE, FALSE                      },
    { "A ",                 FALSE, FALSE                      },
    { " ",                  FALSE, FALSE                      },
    { " A",                 TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { " A ",                FALSE, FALSE                      },
    { "A.",                 FALSE, FALSE                      },
    { ".A",                 FALSE, TRUE,  FALSE, FALSE, FALSE },
    { ".A.",                FALSE, FALSE                      },
    { "*",                  TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "?",                  TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "????????.???",       TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "????????????",       TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "????????.????",      TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "??????????????????", TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "?*?*?*?*?*?*?*?*?*", TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "ABCDEFGHIJKLMNOPQ*", TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "ABCDEFGHI\\*",       FALSE, TRUE,  TRUE,  TRUE,  FALSE },
    { "*\\ABCDEFGHI",       FALSE, TRUE,  TRUE,  TRUE,  FALSE },
    { "?.?.?",              TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "?..?",               TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "? ",                 TRUE,  FALSE, TRUE,  FALSE, FALSE },
    { " ?",                 TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "?.",                 TRUE,  FALSE, TRUE,  FALSE, FALSE },
    { ".?",                 TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "? .?",               TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A?A",                TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A*A",                TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A<A",                TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A>A",                TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A\"A",               TRUE,  TRUE,  TRUE,  FALSE, FALSE },
    { "A'A",                TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "A:A",                FALSE, FALSE                      },
    { "A\x1fG",             FALSE, FALSE                      },
    { "A\x01G",             FALSE, FALSE                      },
    { "A\x0aG",             FALSE, FALSE                      },
    { "A\x0dG",             FALSE, FALSE                      },
    { "\x7f",               TRUE,  TRUE,  FALSE, FALSE, FALSE },
    /* FIXME: these two are probably locale-specific */
    { "\x80",               TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "\xff",               TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGH.IJK",       TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGHX.IJK",      FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGHX.IJ",       FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGH.IJKX",      FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFG.IJKX",       FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGH",           TRUE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGHX",          FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "A.B",                TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "A..B",               FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "A.B.",               FALSE, FALSE                      },
    { "A.B.C",              FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "A .B",               FALSE, TRUE,  FALSE, FALSE, FALSE },
    { " A .B",              FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "A. B",               TRUE,  TRUE,  FALSE, FALSE, FALSE },
    { "A. B ",              FALSE, FALSE                      },
    { "A. B ",              FALSE, FALSE                      },
    { " A . B",             FALSE, TRUE,  FALSE, FALSE, FALSE },
    { " A . B ",            FALSE, FALSE                      },
    { "\\ABCDEFGH.IJK",     TRUE,  TRUE,  FALSE, FALSE, TRUE  },
    { "ABCDEFGH.IJK\\",     TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    { "\\ABCDEFGH.IJK\\",   TRUE,  TRUE,  FALSE, TRUE,  TRUE  },
    { "\\",                 TRUE,  TRUE,  FALSE, FALSE, TRUE  },
    { "\\\\",               FALSE, FALSE                      },
    { "\\\\B",              FALSE, FALSE                      },
    { "A\\",                TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    { "A\\B",               TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    { "A\\\\",              FALSE, FALSE                      },
    { "A\\\\B",             FALSE, FALSE                      },
    /* We can exceed MAX_PATH (260) characters */
    { "0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI",    TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    { "0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\",  TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    { "0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7BCDEF.HI\\8BCDEF.HI\\9BCDEF.HI\\0BCDEF.HI\\1BCDEF.HI\\2BCDEF.HI\\3BCDEF.HI\\4BCDEF.HI\\5BCDEF.HI\\6BCDEF.HI\\7", TRUE,  TRUE,  FALSE, TRUE,  FALSE },
    /* Max component length for HPFS is 255 characters */
    { "ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDE",                                            FALSE, TRUE,  FALSE, FALSE, FALSE },
    { "ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEF",                                           FALSE, FALSE                      },
    { "ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDEFGHI6ABCDEFGHI7ABCDEFGHI8ABCDEFGHI9ABCDEFGHI0ABCDEFGHI1ABCDEFGHI2ABCDEFGHI3ABCDEFGHI4ABCDEFGHI5ABCDE\\ABC",                                       FALSE, TRUE,  FALSE, TRUE,  FALSE },
};

START_TEST(FsRtlLegal)
{
    ULONG i;
    BOOLEAN Result;
    ANSI_STRING DbcsName;
    ULONG Flags;

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        RtlInitAnsiString(&DbcsName, Tests[i].Dbcs);
        for (Flags = 0; Flags < 8; Flags++)
        {
            Result = FsRtlIsFatDbcsLegal(DbcsName,
                                         BooleanFlagOn(Flags, 1),
                                         BooleanFlagOn(Flags, 2),
                                         BooleanFlagOn(Flags, 4));
            if (Tests[i].HasWildCards && !FlagOn(Flags, 1))
                ok(Result == FALSE, "[%s] Result = %u but has wildcards\n", Tests[i].Dbcs, Result);
            else if (Tests[i].IsPath && !FlagOn(Flags, 2))
                ok(Result == FALSE, "[%s] Result = %u but is path\n", Tests[i].Dbcs, Result);
            else if (Tests[i].LeadingBackslash && !FlagOn(Flags, 4))
                ok(Result == FALSE, "[%s] Result = %u but has leading backslash\n", Tests[i].Dbcs, Result);
            else if (!Tests[i].LegalFat)
                ok(Result == FALSE, "[%s] Result = %u but is NOT legal FAT\n", Tests[i].Dbcs, Result);
            else
                ok(Result == TRUE, "[%s] Result = %u but IS legal FAT\n", Tests[i].Dbcs, Result);

            Result = FsRtlIsHpfsDbcsLegal(DbcsName,
                                          BooleanFlagOn(Flags, 1),
                                          BooleanFlagOn(Flags, 2),
                                          BooleanFlagOn(Flags, 4));
            if (Tests[i].HasWildCards && !FlagOn(Flags, 1))
                ok(Result == FALSE, "[%s] Result = %u but has wildcards\n", Tests[i].Dbcs, Result);
            else if (Tests[i].IsPath && !FlagOn(Flags, 2))
                ok(Result == FALSE, "[%s] Result = %u but is path\n", Tests[i].Dbcs, Result);
            else if (Tests[i].LeadingBackslash && !FlagOn(Flags, 4))
                ok(Result == FALSE, "[%s] Result = %u but has leading backslash\n", Tests[i].Dbcs, Result);
            else if (!Tests[i].LegalHpfs)
                ok(Result == FALSE, "[%s] Result = %u but is NOT legal HPFS\n", Tests[i].Dbcs, Result);
            else
                ok(Result == TRUE, "[%s] Result = %u but IS legal HPFS\n", Tests[i].Dbcs, Result);
        }
    }
}
