/* Optional legal-move hints. This is not a solver or a promise of progress. */
#include "solitaire.h"
#include "resource.h"

static bool FitsFoundation(Card card, const CardStack &target)
{
    return target.NumCards() ? card.Suit() == target[0].Suit() && card.LoVal() == target[0].LoVal() + 1
                             : card.LoVal() == 1;
}

static bool FitsColumn(Card card, const CardStack &target)
{
    return target.NumCards() ? target[0].FaceUp() && card.IsBlack() != target[0].IsBlack() &&
                               card.LoVal() + 1 == target[0].LoVal()
                             : card.LoVal() == 13;
}

bool FindHint(CardRegion **source, CardRegion **destination)
{
    CardRegion *available[NUM_ROW_STACKS + 5];
    for (int i = 0; i < NUM_ROW_STACKS; ++i) available[i] = pRowStack[i];
    available[NUM_ROW_STACKS] = pPile;
    for (int i = 0; i < 4; ++i) available[NUM_ROW_STACKS + 1 + i] = pSuitStack[i];
    *source = *destination = NULL;
    for (int i = 0; i <= NUM_ROW_STACKS; ++i)
    {
        if (!available[i]->NumCards() || available[i]->Top().FaceDown()) continue;
        for (int j = 0; j < 4; ++j)
            if (FitsFoundation(available[i]->Top(), pSuitStack[j]->GetCardStack()))
            {
                *source = available[i]; *destination = pSuitStack[j]; return true;
            }
    }
    for (int i = 0; i < NUM_ROW_STACKS; ++i)
        if (pRowStack[i]->NumCards() && pRowStack[i]->Top().FaceDown())
        {
            *source = pRowStack[i]; return true;
        }
    for (int i = 0; i < NUM_ROW_STACKS + 5; ++i)
    {
        const CardStack &cards = available[i]->GetCardStack();
        int count = i < NUM_ROW_STACKS ? cards.NumCards() : min(1, cards.NumCards());
        for (int n = 0; n < count; ++n)
        {
            if (cards[n].FaceDown()) break;
            if (n && (cards[n].LoVal() != cards[n - 1].LoVal() + 1 ||
                      cards[n].IsBlack() == cards[n - 1].IsBlack())) break;
            for (int j = 0; j < NUM_ROW_STACKS; ++j)
            {
                if (available[i] == pRowStack[j]) continue;
                // Moving an entire exposed king column to an empty column adds nothing.
                if (!pRowStack[j]->NumCards() && i < NUM_ROW_STACKS && n + 1 == cards.NumCards()) continue;
                if (FitsColumn(cards[n], pRowStack[j]->GetCardStack()))
                {
                    *source = available[i]; *destination = pRowStack[j]; return true;
                }
            }
        }
    }
    if (pDeck->NumCards() || (activepile.NumCards() &&
        (GetScoreMode() != SCORE_VEGAS || dwWasteCount < dwWasteTreshold)))
    {
        *source = pDeck; return true;
    }
    return false;
}

void ShowHint(void)
{
    CardRegion *source, *destination;
    if (FindHint(&source, &destination))
    {
        source->Flash(3, 150);
        if (destination) destination->Flash(3, 150);
    }
    else
    {
        TCHAR text[256];
        LoadString(GetModuleHandle(NULL), IDS_SOL_NO_HINT, text, 256);
        MessageBox(hwndMain, text, szAppName, MB_OK | MB_ICONINFORMATION);
    }
}
