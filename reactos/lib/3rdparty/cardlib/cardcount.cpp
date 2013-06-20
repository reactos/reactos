//
//    CardCount is a helper library for CardStacks.
//
//    When you initialize a CardCount object with a
//  cardstack, it keeps track of the number of cards
//    the stack contains.
//
//    e.g. CardCount count(cardstack);
//
//    Then you can do:
//
//        int num_fives = count[5]
//
//        count.Add(cardstack2);        - combine with another stack
//
//        int num_aces = count[1]        - aces low
//        int num_aces = count[14]    - aces high
//
//        count.Clear();
//
#include "cardcount.h"

CardCount::CardCount()
{
    Clear();
}

CardCount::CardCount(const CardStack &cs)
{
    Init(cs);
}

void CardCount::Clear()
{
    for(int i = 0; i < 13; i++)
        count[i] = 0;
}

void CardCount::Add(const CardStack &cs)
{
    for(int i = 0; i < cs.NumCards(); i++)
    {
        Card card = cs[i];

        int val = card.LoVal();
        count[val - 1]++;
    }
}

void CardCount::Sub(const CardStack &cs)
{
    for(int i = 0; i < cs.NumCards(); i++)
    {
        Card card = cs[i];
        int val = card.LoVal();

        if(count[val - 1] > 0)
            count[val - 1]--;
    }
}

void CardCount::Init(const CardStack &cs)
{
    Clear();
    Add(cs);
}

int CardCount::operator [] (size_t index) const
{
    if(index < 1) return 0;
    else if(index > 14)  return 0;    //if out of range
    else if(index == 14) index = 1;    //if a "ace-high"

    return count[index - 1];
}

//
//    Decrement specified item by one
//
void CardCount::Dec(size_t index)
{
    if(index < 1) return;
    else if(index > 14)  return;    //if out of range
    else if(index == 14) index = 1;    //if a "ace-high"

    index -= 1;

    if(count[index] > 0)
        count[index]--;
}
