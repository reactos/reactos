
// helper functions for people working with data objects

typedef void (CALLBACK *LPFNCIDLPOINTS)(LPCITEMIDLIST, LPPOINT, LPARAM);

typedef struct
{
    int xMul;
    int xDiv;
    int yMul;
    int yDiv;
} SCALEINFO;


STDAPI          DataObj_SetPoints(IDataObject *pdtobj, LPFNCIDLPOINTS pfnPts, LPARAM lParam, SCALEINFO *si);
STDAPI_(LPIDA)  DataObj_GetHIDA(IDataObject * pdtobj, STGMEDIUM *pmedium);
STDAPI_(UINT)   DataObj_GetHIDACount(IDataObject *pdtobj);
STDAPI          DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal);
STDAPI          DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw);
STDAPI          DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD *pdw);
STDAPI          DataObj_SetDropTarget(IDataObject *pdtobj, const CLSID *pclsid);
STDAPI          DataObj_SetPreferredEffect(IDataObject *pdtobj, DWORD dwEffect);
STDAPI_(DWORD)  DataObj_GetPreferredEffect(IDataObject *pdtobj, DWORD dwDefault);
STDAPI          DataObj_SetPerformedEffect(IDataObject *pdtobj, DWORD dwEffect);
STDAPI_(DWORD)  DataObj_GetPerformedEffect(IDataObject *pdtobj);
STDAPI          DataObj_SetPasteSucceeded(IDataObject *pdtobj, DWORD dwEffect);
STDAPI_(void *) DataObj_SaveShellData(IDataObject *pdtobj, BOOL fShared);
STDAPI          DataObj_GetShellURL(IDataObject *pdtobj, STGMEDIUM *pmedium, LPCSTR *ppszURL);
STDAPI_(void)   HIDA_ReleaseStgMedium(LPIDA pida, STGMEDIUM *pmedium);
STDAPI_(void)   ReleaseStgMediumHGLOBAL(void *pv, STGMEDIUM *pmedium);
STDAPI          DataObj_SaveToFile(IDataObject *pdtobj, UINT cf, LONG lindex, LPCTSTR pszFile, DWORD dwFileSize);
STDAPI          DataObj_GetOFFSETs(IDataObject *pdtobj, POINT * ppt);

