#ifndef _SOL_PCH_
#define _SOL_PCH_

#include <cardlib.h>

extern CardWindow SolWnd;
extern TCHAR szAppName[];
extern bool fGameStarted;

#define OPTION_SHOW_STATUS       4
#define OPTION_THREE_CARDS       8
#define OPTION_SHOW_TIME         16
#define OPTION_KEEP_SCORE       32
#define OPTION_SCORE_STD        64
#define OPTION_SCORE_VEGAS      128
#define CARDBACK_START           IDC_CARDBACK1
#define CARDBACK_END             IDC_CARDBACK12
#define NUM_CARDBACKS            (CARDBACK_END - CARDBACK_START + 1)
#define CARDBACK_RES_START       53
#define CARDBACK_OPTIONS_WIDTH   72
#define CARDBACK_OPTIONS_HEIGHT  112

extern DWORD dwOptions;

extern DWORD dwTime;
extern DWORD dwWasteCount;
extern DWORD dwWasteTreshold;
extern DWORD dwPrevMode;
extern long lScore;
extern HWND hwndMain;
extern UINT_PTR PlayTimer;

#define IDT_PLAYTIMER 1000

void CreateSol(void);
void NewGame(void);

#define NUM_ROW_STACKS     7
#define DECK_ID            1
#define PILE_ID            2
#define SUIT_ID            4
#define ROW_ID             10

#define SCORE_NONE  0
#define SCORE_STD   1
#define SCORE_VEGAS 2

// Various metrics used for placing the objects and computing the minimum window size
#define X_BORDER                 20
#define X_PILE_BORDER            18
#define X_ROWSTACK_BORDER        10
#define X_SUITSTACK_BORDER       10
#define Y_BORDER                 30
#define Y_BORDERWITHFRAME        20
#define Y_ROWSTACK_BORDER        32
extern int yRowStackCardOffset;
extern int VisiblePileCards;

extern CardRegion *pDeck;
extern CardRegion *pPile;
extern CardRegion *pSuitStack[];
extern CardRegion *pRowStack[];
extern CardStack   activepile;

extern void UpdateStatusBar(void);
extern void SetPlayTimer(void);
extern int GetScoreMode(void);
extern void SetUndoMenuState(bool enable);

bool CARDLIBPROC RowStackDragProc(CardRegion &stackobj, int iNumCards);
bool CARDLIBPROC RowStackDropProc(CardRegion &stackobj,  CardStack &dragcards);

bool CARDLIBPROC SuitStackDropProc(CardRegion &stackobj, CardStack &dragcards);
void CARDLIBPROC SuitStackAddProc(CardRegion &stackobj, const CardStack &added);
void CARDLIBPROC SuitStackClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC RowStackClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC RowStackDblClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC PileDblClickProc(CardRegion &stackobj, int iNumClicked);
void CARDLIBPROC PileClickProc(CardRegion &stackobj, int iNumClicked);

void CARDLIBPROC PileRemoveProc(CardRegion &stackobj, int iRemoved);

void SetUndo(int set_source_id, int set_destination_id, int set_number_of_cards, int set_prev_score, int set_prev_visible_pile_cards);
void ClearUndo(void);
void Undo(void);


#endif /* _SOL_PCH_ */
