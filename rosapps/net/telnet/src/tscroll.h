#ifndef __TSCROLL_H
#define __TSCROLL_H

#include "tconsole.h"
#include "tmouse.h"

typedef int(stripfunc)(char *, char *, int);

class TScroller {
private:
	char *pcScrollData;
	long iScrollSize;
	long iScrollEnd;
	int iPastEnd;
	int iDisplay;
	stripfunc *strip;
	TMouse &Mouse;
public:
	void init(stripfunc *s);
	void update(const char *pszBegin, const char *pszEnd);
	void ScrollBack();
	TScroller(TMouse &M, int size=20000);
	~TScroller();
};

#endif