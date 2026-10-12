/*
 * PROJECT:     ReactOS Settings
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Settings search engine (portable, unit-testable on any host)
 */

#include "search.h"

#define MAX_WORD        32
#define MAX_QUERY_WORDS 8
#define MAX_TEXT        512

/* Score of the best match of one query word against one item word */
#define SCORE_EXACT     100
#define SCORE_PREFIX    70
#define SCORE_SUBSTRING 40
#define SCORE_TYPO      30

/* Field weights, in tenths */
#define WEIGHT_TITLE       30
#define WEIGHT_KEYWORDS    20
#define WEIGHT_CATEGORY    15
#define WEIGHT_DESCRIPTION 10

typedef struct _WORD
{
    SCHAR Text[MAX_WORD];
    int Length;
} WORD_T;

static SCHAR
FoldChar(SCHAR c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    /* Latin-1 uppercase letters (except the multiplication sign) */
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7)
        return c + 0x20;
    return c;
}

static int
IsWordChar(SCHAR c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c >= 0xC0;
}

/* Splits Text into lowercase words. Returns the number of words. */
static int
SplitWords(const SCHAR *Text, WORD_T *Words, int MaxWords)
{
    int Count = 0;
    WORD_T *Current = NULL;

    if (!Text)
        return 0;

    for (; *Text; Text++)
    {
        SCHAR c = FoldChar(*Text);
        if (!IsWordChar(c))
        {
            Current = NULL;
            continue;
        }
        if (!Current)
        {
            if (Count == MaxWords)
                break;
            Current = &Words[Count++];
            Current->Length = 0;
        }
        if (Current->Length < MAX_WORD - 1)
            Current->Text[Current->Length++] = c;
        Current->Text[Current->Length] = 0;
    }
    return Count;
}

/* Words that carry no meaning in a settings search ("how do I change my wallpaper") */
static int
IsStopWord(const WORD_T *Word)
{
    static const char *StopWords[] = {
        "a", "an", "the", "to", "my", "of", "for", "in", "how", "do", "i", "is", "can",
        "settings", "setting", "options", "option", "configure", "change", "set", "adjust", "edit"
    };
    size_t i;
    int j;

    for (i = 0; i < sizeof(StopWords) / sizeof(StopWords[0]); i++)
    {
        const char *s = StopWords[i];
        for (j = 0; j < Word->Length && s[j] && (SCHAR)s[j] == Word->Text[j]; j++)
            ;
        if (j == Word->Length && !s[j])
            return 1;
    }
    return 0;
}

/* Length without a plural ending, so that "fonts" matches "font" and "boxes" matches "box" */
static int
StemLength(const WORD_T *Word)
{
    int n = Word->Length;
    if (n > 4 && Word->Text[n - 1] == 's' && Word->Text[n - 2] == 'e' &&
        (Word->Text[n - 3] == 'x' || Word->Text[n - 3] == 's' || Word->Text[n - 3] == 'h'))
        return n - 2;
    if (n > 3 && Word->Text[n - 1] == 's' && Word->Text[n - 2] != 's')
        return n - 1;
    return n;
}

static int
StartsWith(const SCHAR *Text, int TextLength, const SCHAR *Prefix, int PrefixLength)
{
    int i;
    if (PrefixLength > TextLength)
        return 0;
    for (i = 0; i < PrefixLength; i++)
        if (Text[i] != Prefix[i])
            return 0;
    return 1;
}

static int
Contains(const SCHAR *Text, int TextLength, const SCHAR *Needle, int NeedleLength)
{
    int i;
    for (i = 0; i + NeedleLength <= TextLength; i++)
        if (StartsWith(Text + i, TextLength - i, Needle, NeedleLength))
            return 1;
    return 0;
}

/* Optimal string alignment distance (Levenshtein + adjacent swaps), capped at Max + 1 */
static int
EditDistance(const WORD_T *a, const WORD_T *b, int Max)
{
    int d[MAX_WORD + 1][MAX_WORD + 1];
    int i, j;

    if (a->Length - b->Length > Max || b->Length - a->Length > Max)
        return Max + 1;

    for (i = 0; i <= a->Length; i++)
        d[i][0] = i;
    for (j = 0; j <= b->Length; j++)
        d[0][j] = j;

    for (i = 1; i <= a->Length; i++)
    {
        int RowMin = Max + 1;
        for (j = 1; j <= b->Length; j++)
        {
            int Cost = (a->Text[i - 1] == b->Text[j - 1]) ? 0 : 1;
            int v = d[i - 1][j] + 1;
            if (d[i][j - 1] + 1 < v)
                v = d[i][j - 1] + 1;
            if (d[i - 1][j - 1] + Cost < v)
                v = d[i - 1][j - 1] + Cost;
            if (i > 1 && j > 1 && a->Text[i - 1] == b->Text[j - 2] && a->Text[i - 2] == b->Text[j - 1] &&
                d[i - 2][j - 2] + 1 < v)
                v = d[i - 2][j - 2] + 1;
            d[i][j] = v;
            if (v < RowMin)
                RowMin = v;
        }
        if (RowMin > Max)
            return Max + 1;
    }
    return d[a->Length][b->Length];
}

static int
MatchWord(const WORD_T *Query, const WORD_T *Word)
{
    int QueryStem = StemLength(Query), WordStem = StemLength(Word);
    int MaxTypos;

    if (QueryStem == WordStem && StartsWith(Word->Text, WordStem, Query->Text, QueryStem))
        return SCORE_EXACT;
    if (Query->Length >= 2 && StartsWith(Word->Text, Word->Length, Query->Text, Query->Length))
        return SCORE_PREFIX;
    if (Query->Length >= 3 && Contains(Word->Text, Word->Length, Query->Text, Query->Length))
        return SCORE_SUBSTRING;

    /* Typos keep the first letter: people rarely mistype it, and this stops
     * "uninstall" from matching "install" */
    MaxTypos = (Query->Length >= 8) ? 2 : (Query->Length >= 4) ? 1 : 0;
    if (MaxTypos && Query->Text[0] == Word->Text[0])
    {
        /* Also tolerate typos in a partially typed word: compare against the word's prefix */
        WORD_T Prefix = *Word;
        if (Prefix.Length > Query->Length + 1)
            Prefix.Length = Query->Length + 1;
        if (EditDistance(Query, Word, MaxTypos) <= MaxTypos ||
            (Word->Length > Query->Length && EditDistance(Query, &Prefix, MaxTypos) <= MaxTypos &&
             Query->Length >= 5))
            return SCORE_TYPO;
    }
    return 0;
}

static int
MatchField(const WORD_T *Query, const SCHAR *Field, int Weight)
{
    WORD_T Words[64];
    int Count = SplitWords(Field, Words, 64);
    int i, Best = 0;

    for (i = 0; i < Count; i++)
    {
        int s = MatchWord(Query, &Words[i]);
        if (s > Best)
            Best = s;
    }
    return Best * Weight;
}

/* Lowercased copy with every run of non-word characters turned into one space */
static int
NormalizeText(const SCHAR *Text, SCHAR *Out, int OutSize)
{
    int n = 0, Space = 1;
    if (!Text)
        Text = L"";
    for (; *Text && n < OutSize - 2; Text++)
    {
        SCHAR c = FoldChar(*Text);
        if (IsWordChar(c))
        {
            Out[n++] = c;
            Space = 0;
        }
        else if (!Space)
        {
            Out[n++] = ' ';
            Space = 1;
        }
    }
    while (n > 0 && Out[n - 1] == ' ')
        n--;
    Out[n] = 0;
    return n;
}

/* Bonus when the whole query appears as a phrase (e.g. "mouse speed", "screen saver") */
static int
PhraseBonus(const SCHAR *Query, int QueryLength, const SCHAR *Field, int Bonus)
{
    SCHAR Text[MAX_TEXT];
    int Length;

    if (QueryLength < 3 || !Field)
        return 0;
    Length = NormalizeText(Field, Text, MAX_TEXT);
    return Contains(Text, Length, Query, QueryLength) ? Bonus : 0;
}

/*
 * Bonus when the query matches one whole entry of a field, word for word: the
 * title, or one comma separated keyword. "password" then ranks the setting
 * whose keyword IS "password" above one mentioning "password on resume".
 */
static int
EntryBonus(const WORD_T *QueryWords, int WordCount, const SCHAR *Field, int SplitOnComma)
{
    SCHAR Entry[MAX_TEXT];
    WORD_T EntryWords[MAX_QUERY_WORDS * 2];
    int Best = 0;

    while (Field && *Field)
    {
        int n = 0, Count, Kept = 0, i, AllExact = 1, AllMatch = 1;
        WORD_T Words[MAX_QUERY_WORDS * 2];

        while (*Field && (!SplitOnComma || *Field != ',') && n < MAX_TEXT - 1)
            Entry[n++] = *Field++;
        Entry[n] = 0;
        if (*Field == ',')
            Field++;

        Count = SplitWords(Entry, EntryWords, MAX_QUERY_WORDS * 2);
        for (i = 0; i < Count; i++)
        {
            if (!IsStopWord(&EntryWords[i]))
                Words[Kept++] = EntryWords[i];
        }
        if (Kept != WordCount)
            continue;

        for (i = 0; i < Kept; i++)
        {
            int s = MatchWord(&QueryWords[i], &Words[i]);
            if (s != SCORE_EXACT)
                AllExact = 0;
            if (s < SCORE_TYPO || s == SCORE_SUBSTRING)
                AllMatch = 0;
        }
        if (AllMatch)
        {
            int Bonus = AllExact ? 4000 : 2500;
            if (Bonus > Best)
                Best = Bonus;
        }
    }
    return Best;
}

int
SearchItems(
    const SEARCH_ITEM *Items,
    int ItemCount,
    const SCHAR *Query,
    int *Results,
    int MaxResults)
{
    WORD_T AllWords[MAX_QUERY_WORDS], QueryWords[MAX_QUERY_WORDS];
    SCHAR Phrase[MAX_TEXT];
    int PhraseLength, WordCount = 0, AllCount, i, w, Found = 0, BestMatched = 1;
    static int Matched[1024], Strong[1024], Score[1024];

    if (ItemCount > 1024)
        ItemCount = 1024;

    AllCount = SplitWords(Query, AllWords, MAX_QUERY_WORDS);
    for (w = 0; w < AllCount; w++)
    {
        if (!IsStopWord(&AllWords[w]))
            QueryWords[WordCount++] = AllWords[w];
    }
    /* A query made only of stop words ("settings") still searches for them */
    if (WordCount == 0)
    {
        for (w = 0; w < AllCount; w++)
            QueryWords[WordCount++] = AllWords[w];
    }
    if (WordCount == 0)
        return 0;

    PhraseLength = NormalizeText(Query, Phrase, MAX_TEXT);

    for (i = 0; i < ItemCount; i++)
    {
        const SEARCH_ITEM *Item = &Items[i];
        Matched[i] = 0;
        Strong[i] = 0;
        Score[i] = 0;

        for (w = 0; w < WordCount; w++)
        {
            int Best = MatchField(&QueryWords[w], Item->Title, WEIGHT_TITLE), s;
            if ((s = MatchField(&QueryWords[w], Item->Keywords, WEIGHT_KEYWORDS)) > Best)
                Best = s;
            if ((s = MatchField(&QueryWords[w], Item->Category, WEIGHT_CATEGORY)) > Best)
                Best = s;
            if ((s = MatchField(&QueryWords[w], Item->Description, WEIGHT_DESCRIPTION)) > Best)
                Best = s;
            if (Best > 0)
            {
                Matched[i]++;
                Score[i] += Best;
                /* Weights are applied, so compare against the weakest field */
                if (Best >= SCORE_PREFIX * WEIGHT_DESCRIPTION)
                    Strong[i]++;
            }
        }

        if (Matched[i])
        {
            int Title = EntryBonus(QueryWords, WordCount, Item->Title, 0);
            int Keyword = EntryBonus(QueryWords, WordCount, Item->Keywords, 1);
            Score[i] += (Title > Keyword) ? Title + 500 : Keyword;
            if (AllCount > 1)
            {
                Score[i] += PhraseBonus(Phrase, PhraseLength, Item->Title, 3000);
                Score[i] += PhraseBonus(Phrase, PhraseLength, Item->Keywords, 2000);
            }
            if (Matched[i] > BestMatched)
                BestMatched = Matched[i];
        }
    }

    /* Keep the items matching the most query words. For longer queries also keep
     * those missing one word ("uninstall firefox" still shows "Installed apps"),
     * always below the ones matching more words. */
    if (WordCount >= 2)
        BestMatched--;
    else
        BestMatched = 1;
    for (i = 0; i < ItemCount; i++)
    {
        if (Matched[i])
            Score[i] += Matched[i] * 100000;
    }

    /* Sort by score (insertion sort, small n) */
    for (i = 0; i < ItemCount; i++)
    {
        int Pos;
        if (Matched[i] < BestMatched || !Matched[i])
            continue;
        /* Near misses (one word short) only count when their words matched for real, not by typo */
        if (WordCount >= 2 && Matched[i] < WordCount && Strong[i] < Matched[i])
            continue;
        Pos = (Found < MaxResults) ? Found++ : MaxResults;
        while (Pos > 0 && Score[Results[Pos - 1]] < Score[i])
        {
            if (Pos < MaxResults)
                Results[Pos] = Results[Pos - 1];
            Pos--;
        }
        if (Pos < MaxResults)
            Results[Pos] = i;
    }
    return Found;
}
