#ifndef SOLITAIRE_INCLUDED
#define SOLITAIRE_INCLUDED

extern CardWindow SolWnd;
extern TCHAR szAppName[];
extern bool fGameStarted;

#define OPTION_SHOW_STATUS   4
#define OPTION_THREE_CARDS   8
#define CARDBACK_START IDC_CARDBACK1
#define CARDBACK_END IDC_CARDBACK4
#define NUM_CARDBACKS (CARDBACK_END - CARDBACK_START + 1)
#define CARDBACK_RES_START 53

extern DWORD dwOptions;

void CreateSol();
void NewGame(void);

#define NUM_ROW_STACKS 7
#define DECK_ID			1
#define PILE_ID			2
#define SUIT_ID			4
#define ROW_ID			10

extern CardRegion *pDeck;
extern CardRegion *pPile;
extern CardRegion *pSuitStack[];
extern CardRegion *pRowStack[];


bool CARDLIBPROC RowStackDragProc(CardRegion &stackobj, int iNumCards);
bool CARDLIBPROC RowStackDropProc(CardRegion &stackobj,  const CardStack &dragcards);

bool CARDLIBPROC SuitStackDropProc(CardRegion &stackobj, const CardStack &dragcards);
void CARDLIBPROC SuitStackAddProc(CardRegion &stackobj, const CardStack &added);

void CARDLIBPROC RowStackClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC RowStackDblClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC PileDblClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC PileRemoveProc(CardRegion &stackobj, int iRemoved);

#endif
