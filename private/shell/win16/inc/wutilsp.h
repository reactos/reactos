#define MAX_PRN_PAGES 10

//
// This data structure is privately shared with 
// prtprop.c in SHELLDLL
// prt16.c in LIBRARY
//
typedef struct // apg
{
    DWORD cpages;
    HPROPSHEETPAGE ahpage[MAX_PRN_PAGES];
} PAGEARRAY, FAR * LPPAGEARRAY;

// thunk from shell232.dll -> shell.dll
VOID WINAPI CallAddPropSheetPages16(LPFNADDPROPSHEETPAGES lpfn16, LPVOID hdrop, LPPAGEARRAY papg);
