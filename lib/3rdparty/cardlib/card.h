//
//	CardLib - Card class
//
//	Freeware
//	Copyright J Brown 2001
//

#ifndef _CARD_INCLUDED
#define _CARD_INCLUDED

enum eSuit  { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };
enum eValue { Ace = 1, Two = 2, Three = 3, Four = 4, Five = 5, Six = 6, Seven = 7, 
              Eight = 8, Nine = 9, Ten = 10, Jack = 11, Queen = 12, King = 13 };

inline int MAKE_CARD(int Value, int Suit)
{
	if(Value < 1)	Value = 1;
	if(Value == 14) Value = 1;
	if(Value >  13) Value = 13;

	if(Suit < 0)	Suit = 0;
	if(Suit > 3)	Suit = 3;

	return ((Value - 1) * 4 + Suit);
}

class Card
{
	friend class CardStack;

public:

	Card() 
	{ 
		nValue = 0;		//ace of spades by default
		fFaceUp = true; 
	}
	
	Card(int value, int suit)	//specify a face value [1-13] and suit [0-3]
	{ 
		nValue = MAKE_CARD(value, suit);
		fFaceUp = true; 
	}
	
	Card(int index)				//specify a 0-51 index
	{ 
		if(index < 0)  index = 0;
		if(index > 51) index = 51;

		nValue = index; 
		fFaceUp = true;
	}
	
	int Suit() const
	{ 
		return (nValue % 4); 
	}

	int LoVal() const
	{ 
		return (nValue / 4) + 1; 
	}

	int HiVal() const
	{ 
		return ((nValue < 4) ? 14 : (nValue / 4) + 1); 
	}

	int Idx() const //unique value (0-51 etc)
	{
		return nValue;
	}

	bool FaceUp() const
	{
		return fFaceUp;
	}
	
	bool FaceDown() const
	{
		return !fFaceUp;
	}

	void SetFaceUp(bool fTrue)
	{
		fFaceUp = fTrue;
	}

	bool IsBlack() const
	{
		return Suit() == 0 || Suit() == 3;
	}

	bool IsRed() const
	{
		return !IsBlack();
	}

private:

	int  nValue;
	bool fFaceUp;
};

#endif
