//
//    CardLib - CardStack class
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include <stdlib.h>

#include "cardstack.h"

Card &CardStack::operator[] (size_t index)
{
    if(index >= (size_t)nNumCards) index = nNumCards - 1;
    return cardlist[nNumCards - index - 1];
}

const Card &CardStack::operator[] (size_t index) const
{
    if(index >= (size_t)nNumCards) index = nNumCards - 1;
    return cardlist[nNumCards - index - 1];
}

//    Subscripting operator for a constant sequence
//
/*Card CardStack::operator[] (size_t index) const
{
    return cardlist[index];
}*/

//
//    Subscripting operator for a non-const sequence
//
/*CardStack::ref  CardStack::operator[] (size_t index)
{
    return ref(this, index);
}*/

void CardStack::Clear()
{
    nNumCards = 0;
}

void CardStack::NewDeck()
{
    nNumCards = 52;

    for(int i = 0; i < 52; i++)
        cardlist[i].nValue = i;
}

void CardStack::Shuffle()
{
    int src, dest;
    Card temp;

    //shuffle 8 times..
    for(int i = 0; i < 8; i++)
        for(dest = nNumCards - 1; dest > 0; dest--)
        {
            //want to do this:
            //  bad:   src = rand() % (dest + 1)
            //  good:  src = rand() / (RAND_MAX / (dest+1) + 1)

            //positions from 0 to dest
            src = rand() / (RAND_MAX / (dest+1) + 1);

            //swap the cards
            temp           = cardlist[src];
            cardlist[src]  = cardlist[dest];
            cardlist[dest] = temp;
        }
}

void CardStack::Reverse()
{
    for(int i = 0; i < nNumCards / 2; i++)
    {
        Card temp                   = cardlist[i];
        cardlist[i]                 = cardlist[nNumCards - i - 1];
        cardlist[nNumCards - i - 1] = temp;
    }
}

void CardStack::Push(const Card card)
{
    if(nNumCards < MAX_CARDSTACK_SIZE)
        cardlist[nNumCards++] = card;
}

void CardStack::Push(const CardStack &cardstack)
{
    if(nNumCards + cardstack.nNumCards < MAX_CARDSTACK_SIZE)
    {
        int num = cardstack.NumCards();

        for(int i = 0; i < num; i++)
            cardlist[nNumCards++] = cardstack.cardlist[i];
    }
}

CardStack& CardStack::operator += (Card card)
{
    Push(card);
    return *this;
}

CardStack& CardStack::operator += (CardStack &cs)
{
    Push(cs);
    return *this;
}

CardStack CardStack::operator +  (Card card)
{
    CardStack poo = *this;
    poo.Push(card);
    return poo;
}

CardStack CardStack::operator + (CardStack &cs)
{
    CardStack poo = *this;
    poo.Push(cs);
    return poo;
}


Card CardStack::Pop()
{
    if(nNumCards > 0)
        return cardlist[--nNumCards];
    else
        return 0;
}

CardStack CardStack::Pop(int items)
{
    if(items <= nNumCards && nNumCards > 0)
    {
        CardStack cs(*this, nNumCards - items);

        nNumCards -= items;

        return cs;
    }
    else
    {
        return CardStack();
    }
}

Card CardStack::Top()
{
    if(nNumCards > 0)
        return cardlist[nNumCards - 1];
    else
        return 0;
}

CardStack CardStack::Top(int items)
{
    if(items <= nNumCards && nNumCards > 0)
    {
        return CardStack (*this, nNumCards - items);
    }
    else
    {
        return CardStack();
    }

}

Card CardStack::RemoveCard(size_t index)
{
    if(nNumCards == 0 || index >= (size_t)nNumCards)
        return 0;

    //put index into reverse range..
    index = nNumCards - index - 1;

    Card temp = cardlist[index];

    nNumCards--;

    for(size_t i = index; i < (size_t)nNumCards; i++)
    {
        cardlist[i] = cardlist[i+1];
    }

    return temp;
}

void CardStack::InsertCard(size_t index, Card card)
{
    if(nNumCards == MAX_CARDSTACK_SIZE)
        return;

    if(index > (size_t)nNumCards)
        return;

    if((size_t)nNumCards == index)
    {
        cardlist[nNumCards] = card;
        nNumCards++;
        return;
    }

    //put index into reverse range..
    index = nNumCards - index - 1;

    nNumCards++;

    //make room for the card
    for(size_t i = nNumCards; i > index; i--)
    {
        cardlist[i] = cardlist[i - 1];
    }

    cardlist[index] = card;
}


void CardStack::Print()
{
//    for(int i = 0; i < nNumCards; i++)
//        cout << cardlist[i].HiVal() << " ";
}

CardStack::CardStack(CardStack &copythis, size_t fromindex)
{
    nNumCards = copythis.nNumCards - fromindex;

    for(int i = 0; i < nNumCards; i++)
        cardlist[i] = copythis.cardlist[fromindex + i];
}

