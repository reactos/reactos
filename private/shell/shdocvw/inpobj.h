extern HRESULT UnkHasFocusIO(IUnknown *punkThis);
extern HRESULT UnkTranslateAcceleratorIO(IUnknown *punkThis, LPMSG lpMsg);
extern HRESULT UnkUIActivateIO(IUnknown *punkThis, BOOL fActivate, LPMSG lpMsg);

extern HRESULT UnkOnFocusChangeIS(IUnknown *punkThis, IUnknown *punkSrc, BOOL fSetFocus);

