#include <eapp.h>

typedef struct tagHandlerInfo
{
    LPWSTR          pwzHandler;
    DWORD           dwId;
    CLSID           *pClsID;
} HandlerInfo;


HandlerInfo rgKnownHandlers[] =
{
     { L"gzip" ,                    1,  (CLSID *) &CLSID_DeCompMimeFilter }
    ,{ L"deflate",                  2,  (CLSID *) &CLSID_DeCompMimeFilter }
    ,{ L"Class Install Handler",    3,  (CLSID *) &CLSID_ClassInstallFilter }
    ,{ L"cdl",                      4,  (CLSID *) &CLSID_CdlProtocol }
};


//+---------------------------------------------------------------------------
//
//  Function:   IsKnownHandler
//
//  Synopsis:   looks up if the Known Handler 
//
//  Arguments:  [wzHandler] --
//
//  Returns:    
//
//  History:    07-17-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD IsKnownHandler(LPCWSTR wzHandler)
{
    DWORD dwRet = 0;
    int i = 0;
    int cSize = sizeof(rgKnownHandlers)/sizeof(HandlerInfo);

    for (i = 0; i < cSize; ++i)
    {
        if (!_wcsicmp(wzHandler, rgKnownHandlers[i].pwzHandler) )
        {
            dwRet = rgKnownHandlers[i].dwId;
            i = cSize;
        }
    }
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetKnownHandlerClsID
//
//  Synopsis:
//
//  Arguments:  [dwId] --
//
//  Returns:
//
//  History:    07-17-1997   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CLSID *GetKnownHandlerClsID(DWORD dwId)
{
    CLSID *pclsid = 0;
    int cSize = sizeof(rgKnownHandlers)/sizeof(HandlerInfo);

    for (int i = 0; i < cSize; ++i)
    {
        if (dwId == rgKnownHandlers[i].dwId )
        {
            pclsid = rgKnownHandlers[i].pClsID;
            i = cSize;
        }
    }

    return pclsid;
}

