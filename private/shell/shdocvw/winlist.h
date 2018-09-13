
//--------------------------------------------------------------------------
// Manage the windows list, such that we can get the IDispatch for each of 
// the shell windows to be marshalled to different processes
//---------------------------------------------------------------------------

HRESULT VariantClearLazy(VARIANTARG *pvarg);

STDAPI NavigateToPIDL(IWebBrowser2* pwb, LPCITEMIDLIST pidl);

STDAPI_(BOOL) WinList_Init(void);
STDAPI_(void) WinList_Terminate(void);



extern DWORD g_dwWinListCFRegister;     // CoRegisterClassObject Registration DWORD
