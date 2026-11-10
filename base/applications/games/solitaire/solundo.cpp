/*
 * PROJECT:      Solitaire
 * LICENSE:      See COPYING in top level directory
 * FILE:         base/applications/games/solitaire/solundo.cpp
 * PURPOSE:      Undo module for Solitaire
 * PROGRAMMER:   Tibor Lajos Füzi
 */

#include "solitaire.h"

// A bounded history of moves, draws, stock recycling and exposed-card flips.
struct UndoEntry
{
    int source_id, destination_id, number_of_cards;
    int prev_score, prev_visible_pile_cards;
    DWORD prev_waste_count;
    int face_down; // -1 for a move; otherwise the old number of face-down cards.
    int penalty;
};
DWORD dwUndoLimit = UNDO_LIMIT_DEFAULT;
static UndoEntry history[UNDO_LIMIT_MAX];
static int historyCount;

static UndoEntry &PushUndo(void)
{
    if (historyCount == (int)dwUndoLimit)
    {
        for (int i = 1; i < historyCount; ++i)
            history[i - 1] = history[i];
        --historyCount;
    }
    UndoEntry &entry = history[historyCount++];
    entry.prev_waste_count = dwWasteCount;
    entry.face_down = -1;
    entry.penalty = 0;
    SetUndoMenuState(true);
    return entry;
}

void SetUndo(int source, int destination, int count, int score, int visible)
{
    if (!dwUndoLimit || source == destination || count == 0)
        return;
    UndoEntry &entry = PushUndo();
    entry.source_id = source;
    entry.destination_id = destination;
    entry.number_of_cards = count;
    entry.prev_score = score;
    entry.prev_visible_pile_cards = visible;
}

void SetFlipUndo(int row, int faceDown)
{
    if (!dwUndoLimit)
        return;
    UndoEntry &entry = PushUndo();
    entry.source_id = entry.destination_id = row;
    entry.number_of_cards = 0;
    entry.prev_score = lScore;
    entry.prev_visible_pile_cards = VisiblePileCards;
    entry.face_down = faceDown;
}

void SetUndoLimit(DWORD limit)
{
    dwUndoLimit = min(limit, UNDO_LIMIT_MAX);
    if (historyCount > (int)dwUndoLimit)
    {
        int discard = historyCount - dwUndoLimit;
        for (int i = discard; i < historyCount; ++i)
            history[i - discard] = history[i];
        historyCount = dwUndoLimit;
    }
    SetUndoMenuState(historyCount != 0);
}

void ClearUndo(void)
{
    historyCount = 0;
    SetUndoMenuState(false);
}

void Undo(void)
{
    if (!historyCount)
        return;
    UndoEntry entry = history[--historyCount];
    int source_id = entry.source_id;
    int destination_id = entry.destination_id;
    int number_of_cards = entry.number_of_cards;
    int prev_visible_pile_cards = entry.prev_visible_pile_cards;
    CardRegion *source = NULL;
    CardRegion *destination = NULL;

    if ((source_id < 1)                                  ||
        (source_id > (ROW_ID + NUM_ROW_STACKS - 1))      ||
        (destination_id < 1)                             ||
        (destination_id > (ROW_ID + NUM_ROW_STACKS - 1)) ||
        (number_of_cards < 1 && entry.face_down < 0))
    {
        ClearUndo();
        return;
    }

    if (source_id >= ROW_ID)
        source = pRowStack[source_id - ROW_ID];
    else if ((source_id >= SUIT_ID) && (source_id < SUIT_ID + 4))
        source = pSuitStack[source_id - SUIT_ID];
    else if (source_id == PILE_ID)
        source = pPile;
    else if (source_id == DECK_ID)
        source = pDeck;

    if (destination_id >= ROW_ID)
        destination = pRowStack[destination_id - ROW_ID];
    else if ((destination_id >= SUIT_ID) && (destination_id < SUIT_ID + 4))
        destination = pSuitStack[destination_id - SUIT_ID];
    else if (destination_id == PILE_ID)
        destination = pPile;
    else if (destination_id == DECK_ID)
        destination = pDeck;

    if (destination == NULL || source == NULL)
    {
        ClearUndo();
        return;
    }

    // If the player clicked on the deck.
    if (entry.face_down >= 0)
    {
        source->SetFaceDirection(CS_FACE_DOWNUP, entry.face_down);
    }
    else if (destination == pPile && source == pDeck)
    {
        // Put back the cards on the deck in reversed order.
        CardStack tmp = activepile.Pop(number_of_cards);
        tmp.Reverse();
        source->Push(tmp);
        // Restore the pile to be the top cards in the active pile.
        destination->Clear();
        if (prev_visible_pile_cards <= 1)
        {
            destination->SetOffsets(0,0);
            destination->SetCardStack(activepile);
        }
        else
        {
            tmp = activepile.Top(prev_visible_pile_cards);
            destination->SetOffsets(CS_DEFXOFF, 1);
            destination->Push(tmp);
        }
        VisiblePileCards = prev_visible_pile_cards;
    }

    // If the player clicked on the empty deck.
    else if (source == pPile && destination == pDeck)
    {
        // Put back all the cards from the deck to the active pile in reversed order.
        destination->Reverse();
        activepile.Push(destination->GetCardStack());
        destination->Clear();
        if (prev_visible_pile_cards <= 1)
        {
            source->SetOffsets(0,0);
            source->SetCardStack(activepile);
        }
        else
        {
            CardStack tmp = activepile.Top(prev_visible_pile_cards);
            source->SetOffsets(CS_DEFXOFF, 1);
            source->Push(tmp);
        }
        VisiblePileCards = prev_visible_pile_cards;
    }

    // If the player moved one card from the pile.
    else if (source == pPile)
    {
        CardStack tmp = destination->Pop(1);
        activepile.Push(tmp);
        if (prev_visible_pile_cards <= 1)
        {
            source->SetOffsets(0, 0);
            source->Push(tmp);
        }
        else
        {
            source->Clear();
            tmp = activepile.Top(prev_visible_pile_cards);
            source->Push(tmp);
            source->SetOffsets(CS_DEFXOFF, 1);
        }
        VisiblePileCards = prev_visible_pile_cards;
    }

    // If the player moved cards between row stacks / suit stacks.
    else
    {
        destination->MoveCard(source, number_of_cards, false);
    }

    dwWasteCount = entry.prev_waste_count;
    lScore = entry.prev_score;

    // -2 points for the undo in standard score mode.
    if (GetScoreMode() == SCORE_STD)
    {
        lScore = max(0, lScore - 2 - entry.penalty);
        // Older snapshots must not refund penalties from later undo operations.
        for (int i = 0; i < historyCount; ++i)
            history[i].penalty += 2;
    }

    UpdateStatusBar();

    SolWnd.Redraw();
    SetUndoMenuState(historyCount != 0);
}
