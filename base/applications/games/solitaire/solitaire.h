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
#define WM_SOL_WINSTART (WM_APP + 1)
#define WM_SOL_WINFINISH (WM_APP + 2)
#define WM_SOL_STATECHANGED (WM_APP + 3)
#define IDT_AUTOFINISH 1001
bool IsAutoFinishActive(void);
void UpdateAutoFinishMenu(void);
void StartAutoFinish(void);
void StopAutoFinish(void);
void AutoFinishStep(void);

enum WinAnimationMode { WIN_ANIMATION_WINDOW, WIN_ANIMATION_MONITOR, WIN_ANIMATION_DESKTOP };
#define WIN_SPEED_MIN 50
#define WIN_SPEED_DEFAULT 100
#define WIN_SPEED_MAX 200
extern DWORD dwWinAnimationMode;
extern DWORD dwWinAnimationSpeed;
bool IsDesktopWinAnimation(void);
void QueueWinAnimation(void);
void StartWinAnimation(void);
void StopWinAnimation(bool prompt);
void FinishWinAnimation(void);

void CreateSol(void);
void NewGame(void);
void SaveGame(void);
void StartScoreRound(LONG base);
void FinishScoreRound(bool won);
DWORD ScoreRoundOptions(void);
void GetScoreRound(LONG *base, FILETIME *id);
void RestoreScoreRound(LONG base, const FILETIME &id);
void SaveScoreBalance(void);
bool ReadVegasBalance(DWORD options, LONG *balance);
void ShowHighScores(HWND owner);
bool IsScoreNameInput(void);
int PromptScoreName(void);
bool LoadGame(void);
bool FindHint(CardRegion **source, CardRegion **destination);
void ShowHint(void);
void PreviewWinAnimation(DWORD mode, DWORD speed, HWND owner);

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
#define UNDO_LIMIT_DEFAULT 3
#define UNDO_LIMIT_MAX 9
extern DWORD dwUndoLimit;
void SetUndoLimit(DWORD limit);
void SetFlipUndo(int row, int faceDown);
void ClearUndo(void);
void Undo(void);


#endif /* _SOL_PCH_ */
