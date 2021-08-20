/*
 * PROJECT:      Solitaire
 * LICENSE:      See COPYING in top level directory
 * FILE:         base/applications/games/solitaire/solundo.cpp
 * PURPOSE:      Undo module for Solitaire
 * PROGRAMMER:   Tibor Lajos Füzi
 */

#include "solitaire.h"

// source_id and destination_id store the source and destination of the cards
// that were moved. These ids are defined in solitaire.h and can be DECK_ID, PILE_ID,
// [SUIT_ID..SUIT_ID + 3], [ROW_ID..ROW_ID + NUM_ROW_STACKS - 1].
// -1 means that there is no action stored in the undo module.
static int source_id = -1;
static int destination_id = -1;

// Number of cards that were moved.
static int number_of_cards = 0;

// The score before the action was taken.
static int prev_score = 0;

// The number of visible pile cards before the action was taken.
static int prev_visible_pile_cards = 0;

void SetUndo(
    int set_source_id,
    int set_destination_id,
    int set_number_of_cards,
    int set_prev_score,
    int set_prev_visible_pile_cards)
{
    if ((set_source_id == set_destination_id) || (set_number_of_cards == 0))
        return;

    source_id = set_source_id;
    destination_id = set_destination_id;
    number_of_cards = set_number_of_cards;
    prev_score = set_prev_score;
    prev_visible_pile_cards = set_prev_visible_pile_cards;
    SetUndoMenuState(true);
}

void ClearUndo(void)
{
    source_id = -1;
    destination_id = -1;
    number_of_cards = 0;
    SetUndoMenuState(false);
}

void Undo(void)
{
    CardRegion *source = NULL;
    CardRegion *destination = NULL;

    if ((source_id < 1)                                  ||
        (source_id > (ROW_ID + NUM_ROW_STACKS - 1))      ||
        (destination_id < 1)                             ||
        (destination_id > (ROW_ID + NUM_ROW_STACKS - 1)) ||
        (number_of_cards < 1))
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
    if (destination == pPile && source == pDeck)
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

    lScore = prev_score;

    // -2 points for the undo in standard score mode.
    if (GetScoreMode() == SCORE_STD)
        lScore = lScore >= 2 ? lScore - 2 : 0;

    UpdateStatusBar();

    SolWnd.Redraw();
    ClearUndo();
}
