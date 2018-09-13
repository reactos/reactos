// Exported for inetCPL
EXTERN_C HRESULT ClearAutoSuggestForForms(DWORD dwClear);

// called from iedisp.cpp
void AttachIntelliForms(void *pOmWindow, HWND hwnd, IHTMLDocument2 *pDoc2, void **ppIntelliForms);
void ReleaseIntelliForms(void *pIntelliForms);
HRESULT IntelliFormsDoAskUser(HWND hwndBrowser, void *pv);

// called from shuioc.cpp
HRESULT IntelliFormsSaveForm(IHTMLDocument2 *pDoc2, VARIANT *pvarForm);


