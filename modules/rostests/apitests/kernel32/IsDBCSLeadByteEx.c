/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for IsDBCSLeadByteEx
 * PROGRAMMER:      Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#define MAX_RANGE   4

typedef struct RANGE
{
    int StartIndex;
    int EndIndex;
} RANGE;

typedef struct ENTRY
{
    const char *Name;
    int CodePage;
    int RangeCount;
    RANGE Ranges[MAX_RANGE];
} ENTRY;

START_TEST(IsDBCSLeadByteEx)
{
    static const ENTRY Entries[] =
    {
        {
            "English", 437,
            0
        },
        {
            "ChineseSimpilified", 936,
            1,
            {
                { 0x81, 0xFE }
            }
        },
        {
            "ChineseTraditional", 950,
            1,
            {
                { 0x81, 0xFE }
            }
        },
        {
            "Japanese", 932,
            2,
            {
                { 0x81, 0x9F }, { 0xE0, 0xFC }
            }
        },
        {
            "Korean", 949,
            1,
            {
                { 0x81, 0xFE }
            }
        }
    };
    int i;

    for (i = 0; i < _countof(Entries); ++i)
    {
        int Index, iRange;
        int CodePage = Entries[i].CodePage, StartIndex = 0, RangeCount = 0;
        BOOL InRange = FALSE;
        RANGE Ranges[MAX_RANGE];
        const char *Name = Entries[i].Name;

        ZeroMemory(&Ranges, sizeof(Ranges));

        for (Index = 0; Index < 256; ++Index)
        {
            if (InRange)
            {
                if (!IsDBCSLeadByteEx(CodePage, Index))
                {
                    Ranges[RangeCount].StartIndex = StartIndex;
                    Ranges[RangeCount].EndIndex = Index - 1;
                    ++RangeCount;
                    InRange = FALSE;
                }
            }
            else
            {
                if (IsDBCSLeadByteEx(CodePage, Index))
                {
                    StartIndex = Index;
                    InRange = TRUE;
                }
            }
        }
        if (InRange)
        {
            Ranges[RangeCount].StartIndex = StartIndex;
            Ranges[RangeCount].EndIndex = Index - 1;
            ++RangeCount;
        }

        ok(RangeCount == Entries[i].RangeCount,
           "%s: RangeCount expected %d, was %d\n",
           Name, Entries[i].RangeCount, RangeCount);
        for (iRange = 0; iRange < Entries[i].RangeCount; ++iRange)
        {
            const RANGE *pRange = &Entries[i].Ranges[iRange];
            int iStart = Ranges[iRange].StartIndex;
            int iEnd = Ranges[iRange].EndIndex;
            ok(iStart == pRange->StartIndex,
               "%s: Ranges[%d].StartIndex expected: 0x%02X, was: 0x%02X\n",
               Name, iRange, pRange->StartIndex, iStart);
            ok(iEnd == pRange->EndIndex,
               "%s: Ranges[%d].EndIndex was expected: 0x%02X, was: 0x%02X\n",
               Name, iRange, pRange->EndIndex, iEnd);
        }
    }
}
