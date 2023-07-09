extern HRESULT UnkHasFocusIO(IUnknown *punkThis);
extern HRESULT UnkTranslateAcceleratorIO(IUnknown *punkThis, LPMSG lpMsg);
extern HRESULT UnkUIActivateIO(IUnknown *punkThis, BOOL fActivate, LPMSG lpMsg);

extern HRESULT UnkOnFocusChangeIS(IUnknown *punkThis, IUnknown *punkSrc, BOOL fSetFocus);

extern HRESULT _MayUIActTAB(IOleWindow *powEtc, LPMSG lpMsg, BOOL fShowing, HWND *phwnd);
