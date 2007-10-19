#ifndef CARDSTACK_INCLUDED
#define CARDSTACK_INCLUDED

#include "card.h"

#define MAX_CARDSTACK_SIZE 128

class CardStack
{
	friend class CardRegion;

public:
	CardStack() : nNumCards(0) { }

	void		NewDeck();
	int			NumCards() const { return nNumCards; }
	void		Shuffle();
	void		Clear();
	void		Reverse();

	void		Push(const Card card);
	void		Push(const CardStack &cardstack);

	Card		Pop();
	CardStack	Pop(int items);

	Card		Top();
	CardStack	Top(int items);

	void Print();

	Card		RemoveCard(size_t index);
	void		InsertCard(size_t index, Card card);

	//subscript capability!!
	      Card & operator[] (size_t index);
	const Card & operator[] (size_t index) const;

	CardStack &operator += (Card card);
	CardStack &operator += (CardStack &cs);

	CardStack operator +  (Card card);
	CardStack operator +  (CardStack &cs);

private:

	CardStack(CardStack &copythis, size_t fromindex);

	Card cardlist[MAX_CARDSTACK_SIZE];
	int  nNumCards;
};

#endif
