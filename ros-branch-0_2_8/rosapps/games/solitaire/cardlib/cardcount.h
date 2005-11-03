#ifndef _CARDCOUNT_INCLUDED
#define _CARDCOUNT_INCLUDED

#include <windows.h>

#include "cardstack.h"

class CardCount
{
public:
	CardCount();
	CardCount(const CardStack &cs);

	void Init(const CardStack &cs);
	void Clear();
	void Add(const CardStack &cs);
	void Sub(const CardStack &cs);

	void Dec(size_t index);

	int operator[] (size_t index) const;

	CardCount &operator =  (const CardStack &cs);
	CardCount &operator += (const CardStack &cs);

private:
	int count[13];	//13 different card values 
					//(ace,2,3,4,5,6,7,8,9,10,J,Q,K)
};

#endif
