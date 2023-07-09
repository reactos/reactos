#include "commobj.h"

typedef struct
{
    CKnownShellExtInit  kshx;
    HKEY                hkeyProgID;
    IDataObject         *pdtobj;
    STGMEDIUM           medium;
} CCommonShellExtInit;

void CCommonShellExtInit_Init(CCommonShellExtInit *pcshx, CCommonUnknown *pcunk);
void CCommonShellExtInit_Delete(CCommonShellExtInit *pcshx);

typedef HRESULT (*LPFNADDPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);

typedef struct
{
    CKnownShellPropSheetExt kspx;
    LPFNADDPAGES lpfnAddPages;
} CCommonShellPropSheetExt;

void CCommonShellPropSheetExt_Init(CCommonShellPropSheetExt *pcspx, CCommonUnknown *pcunk, LPFNADDPAGES pfnAddPages);
