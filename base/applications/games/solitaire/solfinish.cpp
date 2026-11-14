/* Optional, fully checked end-game collection. */
#include "solitaire.h"
#include "resource.h"

static bool collecting;

bool IsAutoFinishActive(void)
{
    return collecting;
}

// Work exclusively on copies: checking availability must not start the clock,
// change the score, create undo entries or partially collect an unsolvable board.
static bool PlanAutoFinish(int *firstSource, int *firstDestination)
{
    if (pDeck->NumCards() != 0)
        return false;

    CardStack sources[NUM_ROW_STACKS + 1], foundations[4];
    int remaining = activepile.NumCards(), total = remaining;
    sources[NUM_ROW_STACKS] = activepile;
    for (int i = 0; i < NUM_ROW_STACKS; ++i)
    {
        sources[i] = pRowStack[i]->GetCardStack();
        for (int j = 0; j < sources[i].NumCards(); ++j)
            if (sources[i][j].FaceDown())
                return false;
        remaining += sources[i].NumCards();
        total += sources[i].NumCards();
    }
    for (int i = 0; i < 4; ++i)
    {
        foundations[i] = pSuitStack[i]->GetCardStack();
        total += foundations[i].NumCards();
    }
    if (!remaining || total != 52)
        return false;

    bool first = true;
    while (remaining)
    {
        bool moved = false;
        for (int i = 0; i <= NUM_ROW_STACKS && !moved; ++i)
        {
            if (!sources[i].NumCards())
                continue;
            Card card = sources[i][0];
            for (int j = 0; j < 4; ++j)
            {
                const CardStack &target = foundations[j];
                bool legal = target.NumCards()
                    ? card.Suit() == target[0].Suit() && card.LoVal() == target[0].LoVal() + 1
                    : card.LoVal() == 1;
                if (!legal)
                    continue;
                if (first)
                {
                    if (firstSource) *firstSource = i;
                    if (firstDestination) *firstDestination = j;
                    first = false;
                }
                foundations[j].Push(sources[i].Pop());
                --remaining;
                moved = true;
                break;
            }
        }
        if (!moved)
            return false;
    }
    return true;
}

void UpdateAutoFinishMenu(void)
{
    EnableMenuItem(GetMenu(hwndMain), IDM_GAME_FINISH,
                   MF_BYCOMMAND | (!collecting && PlanAutoFinish(NULL, NULL) ? MF_ENABLED : MF_GRAYED));
    DrawMenuBar(hwndMain);
}

void StopAutoFinish(void)
{
    if (collecting)
    {
        KillTimer(hwndMain, IDT_AUTOFINISH);
        collecting = false;
        PostMessage(hwndMain, WM_SOL_STATECHANGED, 0, 0);
    }
}

void StartAutoFinish(void)
{
    if (collecting || !PlanAutoFinish(NULL, NULL))
        return;
    collecting = SetTimer(hwndMain, IDT_AUTOFINISH, 60, NULL) != 0;
    UpdateAutoFinishMenu();
}

void AutoFinishStep(void)
{
    int source, destination;
    if (!collecting)
        return;
    if (!PlanAutoFinish(&source, &destination))
    {
        StopAutoFinish();
        return;
    }
    CardRegion *from = source == NUM_ROW_STACKS ? pPile : pRowStack[source];
    extern int LastId;
    extern bool bAutoroute;
    LastId = from->Id();
    fGameStarted = true;
    bAutoroute = true;
    // Use the same callbacks as a double-click for scoring, waste and undo.
    // The short timer interval adds no long pause between animated flights.
    int before = pSuitStack[destination]->NumCards();
    from->SimulateDrag(pSuitStack[destination], 1, true);
    bAutoroute = false;
    if (pSuitStack[destination]->NumCards() != before + 1)
        StopAutoFinish();
}
