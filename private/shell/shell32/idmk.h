typedef HRESULT (CALLBACK *LPFNENUMCALLBACK)(LPARAM lParam, void *pvData, UINT ecid, UINT index);

#define ECID_SETNEXTID  1
#define ECID_RESET      2
#define ECID_RELEASE    3

STDAPI SHCreateEnumObjects(HWND hwndOwner, void *pvData, LPFNENUMCALLBACK lpfn, IEnumIDList **ppeunk);
STDAPI_(void) CDefEnum_SetReturn(LPARAM lParam, LPITEMIDLIST pidl);

STDMETHODIMP CDefEnum_Skip(IEnumIDList *peunk, ULONG celt);
STDMETHODIMP CDefEnum_Reset(IEnumIDList *peunk);
STDMETHODIMP CDefEnum_Clone(IEnumIDList *peunk, IEnumIDList **ppenm);
