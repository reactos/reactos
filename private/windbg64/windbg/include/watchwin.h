/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/


// Watch Tree access routines:

PTRVIB AddCVWatch(PTRVIT pvit, PSTR szExpStr);
BOOL   ReplaceCVWatch( PTRVIT pvit, PTRVIB pvib, PSTR newstr);
LTS    DeleteCVWatch( PTRVIT pvit, PTRVIB pvib);
VOID   UpdateCVWatchs(VOID);
BOOL   AcceptWatchUpdate(PTRVIT pVit,PPANE p, WPARAM wParam);

LRESULT CALLBACK WatchEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND PASCAL GetWatchHWND(VOID);
PTRVIT PASCAL GetWatchVit(VOID);
PTRVIT PASCAL InitWatchVit(void);

void ReloadAllWatchVariables();

