#include "precomp.hxx"
#pragma hdrstop



HFONT g_hBigBoldFont;
HFONT g_hBoldFont;
TSingle_List<PAGEID> g_stack;

TList<PSTR> g_SymPaths;

CFGDATA g_CfgData;

// Used the stub and 
PAGEID g_nNewPageIdx = FIRST_PAGEID;

PAGE_DEF * g_rgpPageDefs[MAX_NUM_PAGEID] = {0};

HINSTANCE g_hInst = NULL;

// BUGBUG
BOOL g_bRunning_IE_3;

 