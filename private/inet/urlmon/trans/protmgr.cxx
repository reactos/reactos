//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       protmgr.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include <shlwapip.h>
#include "oinet.hxx"

PerfDbgTag(tagCProtMgr, "Urlmon", "Log CProtMgr", DEB_PROT);

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgr::Register
//
//  Synopsis:
//
//  Arguments:  [LPOLESTR] --
//              [ULONG] --
//              [cProtocols] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgr::Register(IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pszProtocol,
                                ULONG  cPatterns, const LPCWSTR *ppwzPatterns, DWORD dwReserved)
{
    PerfDbgLog(tagCProtMgr, this, "+CProtMgr::Register");
    HRESULT hr = NOERROR;

    TransAssert((pCF && pszProtocol));
    CLock lck(_mxs);


    if (pCF && pszProtocol)
    {
        char szStr[ULPROTOCOLLEN];
        W2A(pszProtocol, szStr, ULPROTOCOLLEN);

        ATOM atom = AddAtom(szStr);

        CNodeData *pNodeIns = new CNodeData(atom, pCF, rclsid);

        if (pNodeIns)
        {
            if (_cElements < 0)
            {
                _cElements = 0;
            }
            pNodeIns->_pNextNode = _pNextNode;
            _pNextNode = pNodeIns;
            _cElements++;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::Register (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgr::Unregister
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgr::Unregister(IClassFactory *pCF, LPCWSTR pszProtocol)
{
    PerfDbgLog(tagCProtMgr, this, "+CProtMgr::Unregister");
    HRESULT hr = NOERROR;

    TransAssert((pCF && pszProtocol));
    CLock lck(_mxs);

    if (pCF && pszProtocol)
    {
        char szStr[ULPROTOCOLLEN];
        W2A(pszProtocol, szStr, ULPROTOCOLLEN);
        ATOM atom = AddAtom(szStr);
        CNodeData *pNode = _pNextNode;
        CNodeData *pPrevNode = NULL;

        while (pNode)
        {
            if (pNode->_pCFProt == pCF && pNode->_atom == atom)
            {
                // found the node

                if (pPrevNode)
                {
                    pPrevNode->_pNextNode = pNode->_pNextNode;
                }
                else
                {
                    // remove first node
                    _pNextNode = pNode->_pNextNode;
                    pPrevNode = pNode->_pNextNode;
                }
                _cElements--;
                delete pNode;
                pNode = NULL;
            }
            else
            {
                pPrevNode = pNode;
                pNode = pNode->_pNextNode;
            }

        }

        //no previous node - the first
        if (pPrevNode == NULL)
        {
            _pNextNode = 0;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::Unregister (hr:%lx)", hr);
    return hr;
}

DWORD IsKnownHandler(LPCWSTR wzHandler);
CLSID *GetKnownHandlerClsID(DWORD dwID);

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgr::FindFirstCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgr::FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid)
{
    PerfDbgLog(tagCProtMgr, this, "+CProtMgr::FindFirstCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    CNodeData *pNode;
    DWORD dwEl = 1;
    BOOL  fIsKnownHandler = FALSE;
    HRESULT hrGetClsId = NOERROR;

    CLock lck(_mxs);

    _pPosNode = 0;

    if (_cElements < 0)
    {
        _cElements = 0;
    }
    else if (_cElements > 0)
    {
        char szStr[ULPROTOCOLLEN];

        W2A(pszProt, szStr, ULPROTOCOLLEN);

        ATOM atom = FindAtom(szStr);

        if (atom)
        {
            pNode = _pNextNode;

            while (pNode && pNode->_atom != atom)
            {
                pNode = pNode->_pNextNode;
            }
            if (pNode)
            {
                *ppUnk = pNode->_pCFProt;
                pNode->_pCFProt->AddRef();
                *pclsid = pNode->_clsidProt;
                hr = NOERROR;
            }
        }
    }


    if( hr != NOERROR)
    {
        DWORD dwID = 0;
        dwID = IsKnownHandler(pszProt);
        if( dwID)
        {
            *pclsid = *GetKnownHandlerClsID(dwID);
            fIsKnownHandler = TRUE;
        }
        else
        {
            hrGetClsId = LookupClsIDFromReg(pszProt, pclsid, &dwEl);
        }
    }

    if (   (hr != NOERROR) && (hrGetClsId == NOERROR) )
    {
        IClassFactory *pCF = 0;
        hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory, (void**)&pCF);
        if (hr == NOERROR)
        {
            *ppUnk = pCF;
            
            if(fIsKnownHandler)
            { 
                char szStr[ULPROTOCOLLEN];

                W2A(pszProt, szStr, ULPROTOCOLLEN);

                *ppUnk = pCF;

                ATOM atom = AddAtom(szStr);
                pNode = new CNodeData(atom, pCF, *pclsid);

                if (pNode)
                {
                    pNode->_pNextNode = _pNextNode;
                    _pNextNode = pNode;
                    _cElements++;
                }
                // no release of CF since it is given out
            }

        }
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::FindFirstCF (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgr::FindNextCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgr::FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid)
{
    PerfDbgLog(tagCProtMgr, this, "+CProtMgr::FindNextCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    TransAssert((FALSE));

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::FindNextCF (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CProtMgr::LookupClsIDFromReg
//
//  Synopsis:
//
//  Arguments:  [pwzProt] --
//              [pclsid] --
//              [pcClsIds] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgr::LookupClsIDFromReg(LPCWSTR pwzProt, CLSID *pclsid, DWORD *pcClsIds, DWORD *pdwFlags, DWORD dwOpt)
{
    PerfDbgLog1(tagCProtMgr, this, "+CProtMgr::LookupClsIDFromReg (pszProt:%ws)", pwzProt);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    DWORD dwType;

    TransAssert((pwzProt && pclsid));

    if (pwzProt)
    {
        char pszProt[ULPROTOCOLLEN+1];

        W2A(pwzProt, pszProt, ULPROTOCOLLEN);
        pszProt[ULPROTOCOLLEN] = '\0';

        char szDelimiter = ':';

        LPSTR pszDel = StrChr(pszProt, szDelimiter);

        if (pszDel)
        {
            *pszDel = '\0';
        }

        DWORD dwLen = 256;
        char szProtocolKey[256];
        char szCLSID[256];

        strcpy(szProtocolKey, SZ_SH_PROTOCOLROOT);
        strcat(szProtocolKey, pszProt);

        char pszOptFlag[16];


        if(dwOpt)
        {
            wsprintf(pszOptFlag, "\\0x%08x", dwOpt);
            strcat(szProtocolKey, pszOptFlag); 
        }

        if (SHRegGetUSValue(
                    szProtocolKey, 
                    SZCLASS, 
                    NULL, 
                    (LPBYTE)szCLSID, 
                    &dwLen,
                    FALSE, 
                    NULL, 
                    0) == ERROR_SUCCESS)
        {
            hr = CLSIDFromStringA(szCLSID, pclsid);
            PerfDbgLog2(
                    tagCProtMgr, 
                    this, 
                    "API FOUND LookupProtocolClsIDFromReg(hr:%lx, ClsId:%s)", 
                    hr, 
                    szCLSID);
        }

    }



    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::LookupClsIDFromReg (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgrNameSpace::ShouldLookupRegistry
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//
//  Returns:    S_OK if registry should be looked up, S_FALSE if not. E_* if it encounters an error
//              of some sort. 
//
//  History:    11-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgrNameSpace::ShouldLookupRegistry(LPCWSTR pszProt)
{
    PerfDbgLog1(tagCProtMgr, this, "+CProtMgr::ShouldLookupRegistry (pszProt:%ws)", pszProt);
    HRESULT hr = E_FAIL;

    if (pszProt == NULL)
    {
        return E_INVALIDARG;
    }

 
    BOOL bFound = FALSE;
    for (CProtocolData *pCurrent = _pProtList ; pCurrent != NULL; pCurrent = pCurrent->GetNext())
    {
        // Found the protocol.
        if (0 == StrCmpICW(pszProt, pCurrent->GetProtocol()))
        {
            bFound = TRUE;
            break;
        }    
    }

    if (bFound)
    {
        hr = S_FALSE;
    }
    else
    {
        // Append to list.                  
        CProtocolData *pProtNew = new CProtocolData;
        if (pProtNew != NULL)
        {
            if (pProtNew->Init(pszProt, _pProtList))
            {
                _pProtList = pProtNew;
                hr = S_OK;
            }
            else 
            {
                // only way init can fail today is if we are out of memory
                delete [] pProtNew;
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;   
        }
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgr::ShouldLookupRegistry (hr:%lx)", hr);
    return hr;
}

                                  
//+---------------------------------------------------------------------------
//
//  Method:     CProtMgrNameSpace::FindFirstCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgrNameSpace::FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid)
{
    PerfDbgLog1(tagCProtMgr, this, "+CProtMgrNameSpace::FindFirstCF (pszProt:%ws)", pszProt);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    CLock lck(_mxs);

    _pPosNode = 0;

    //
    // check the registered protocols first
    //
    if (_cElements > 0)
    {
        char szStr[ULPROTOCOLLEN];
        W2A(pszProt, szStr, ULPROTOCOLLEN);
        ATOM atom = FindAtom(szStr);

        if (atom)
        {
            _pPosNode = _pNextNode;

            while (_pPosNode && _pPosNode->_atom != atom)
            {
                _pPosNode = _pPosNode->_pNextNode;
            }
        }

    }
    
    //
    // look up the registry once
    //
    if (   (!_pPosNode)
        && (ShouldLookupRegistry(pszProt) == S_OK))
    {
        // BUG-WORK: 64 plugable namespace are max!
        CLSID rgclsid[64];
        DWORD dwEl = 64;

        if ((hr = LookupClsIDFromReg(pszProt, rgclsid, &dwEl)) == NOERROR)
        {
            char szStr[ULPROTOCOLLEN];

            W2A(pszProt, szStr, ULPROTOCOLLEN);

            ATOM atom = AddAtom(szStr);

            for (DWORD i = 0; i < dwEl; i++)
            {
                CLSID *pclsid = &rgclsid[i];
                CNodeData *pNodeIns = new CNodeData(atom, NULL, *pclsid);

                if (pNodeIns)
                {
                    pNodeIns->_pNextNode = _pNextNode;
                    _pNextNode = pNodeIns;
                    _cElements++;
                }

                if (!_pPosNode)
                {
                    _pPosNode = pNodeIns;
                }
            }

        }
    }
    /*
    else if (_cElements > 0)
    {
        char szStr[ULPROTOCOLLEN];
        W2A(pszProt, szStr, ULPROTOCOLLEN);
        ATOM atom = FindAtom(szStr);

        if (atom)
        {
            _pPosNode = _pNextNode;

            while (_pPosNode && _pPosNode->_atom != atom)
            {
                _pPosNode = _pPosNode->_pNextNode;
            }
        }

    }
    */

    if (_pPosNode)
    {
        *pclsid = _pPosNode->_clsidProt;

        if (_pPosNode->_pCFProt == NULL)
        {
            IClassFactory *pCF = 0;
            hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory, (void**)&pCF);
            if (hr == NOERROR)
            {
                _pPosNode->_pCFProt = pCF;
            }
        }
        if (_pPosNode->_pCFProt != NULL)
        {
            *ppUnk = _pPosNode->_pCFProt;
            _pPosNode->_pCFProt->AddRef();
            hr = NOERROR;
        }
        //advance to the next node for further findnextcf calls
        _pPosNode = _pPosNode->_pNextNode;
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgrNameSpace::FindFirstCF (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CProtMgrNameSpace::FindNextCF
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppUnk] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgrNameSpace::FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid)
{
    PerfDbgLog(tagCProtMgr, this, "+CProtMgrNameSpace::FindNextCF");
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    CLock lck(_mxs);

    if (_cElements > 0 && _pPosNode)
    {
        char szStr[ULPROTOCOLLEN];
        W2A(pszProt, szStr, ULPROTOCOLLEN);

        ATOM atom = FindAtom(szStr);

        if (atom)
        {
            do
            {
                // find next matching node
                while (_pPosNode && _pPosNode->_atom != atom)
                {
                    _pPosNode = _pPosNode->_pNextNode;
                }

                if (_pPosNode)
                {
                    IClassFactory *pCF = 0;
                    *pclsid = _pPosNode->_clsidProt;

                    if (_pPosNode->_pCFProt == NULL)
                    {
                        hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory, (void**)&pCF);
                        if (hr == NOERROR)
                        {
                            _pPosNode->_pCFProt = pCF;
                        }
                    }

                    if (_pPosNode->_pCFProt != NULL)
                    {
                        *ppUnk = _pPosNode->_pCFProt;
                        _pPosNode->_pCFProt->AddRef();
                        hr = NOERROR;
                    }
                    //advance to the next node for further findnextcf calls
                    _pPosNode = _pPosNode->_pNextNode;
                }

            } while (hr != NOERROR && _pPosNode);
        }
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgrNameSpace::FindNextCF (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CProtMgrNameSpace::LookupClsIDFromReg
//
//  Synopsis:
//
//  Arguments:  [pszProt] --
//              [ppclsid] --
//              [pcClsIds] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgrNameSpace::LookupClsIDFromReg(LPCWSTR pwzProt, CLSID *ppclsid, DWORD *pcClsIds, DWORD *pdwFlags, DWORD dwOpt)
{
    PerfDbgLog1(tagCProtMgr, this, "+CProtMgrNameSpace::LookupClsIDFromReg (pwzProt:%ws)", pwzProt);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    DWORD dwType;
    DWORD cFound = 0;
    HKEY hNameSpaceKey = NULL;

    TransAssert((pwzProt && ppclsid));

    char pszProt[ULPROTOCOLLEN];

    W2A(pwzProt, pszProt, ULPROTOCOLLEN);

    char szDelimiter = ':';

    LPSTR pszDel = StrChr(pszProt, szDelimiter);

    if (pszDel)
    {
        *pszDel = '\0';
    }


    #define LENNAMEMAX 256
    DWORD dwLen = LENNAMEMAX;
    DWORD dwLenName = LENNAMEMAX;
    char szNameSpaceKey[LENNAMEMAX];
    char szName[LENNAMEMAX];

    strcpy(szNameSpaceKey, SZNAMESPACEROOT);

    LPSTR pszKey = szNameSpaceKey + strlen(szNameSpaceKey);

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szNameSpaceKey, 0, KEY_READ | KEY_QUERY_VALUE, &hNameSpaceKey) != ERROR_SUCCESS)
    {
        TransAssert((hNameSpaceKey == NULL));
    }

    if (hNameSpaceKey)
    {
        HKEY hProtKey = NULL;
        DWORD dwIndex = 0;
        DWORD dwCheck = 2;

        do
        {
            *pszKey = '\0';
            LPSTR pszKeyProt = pszKey;

            if (dwCheck == 2)
            {
                strcat(pszKeyProt, pszProt);
            }
            else if (dwCheck == 1)
            {
                strcat(pszKeyProt, SZALL);
            }
            else
            {
                TransAssert((FALSE));
            }
            strcat(pszKeyProt, "\\");


            if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szNameSpaceKey, 0, KEY_READ | KEY_QUERY_VALUE, &hProtKey) != ERROR_SUCCESS)
            {
                hProtKey = 0;
            }
            dwCheck--;


            if (hProtKey)
            {
                DWORD dwResult;
                dwLenName = LENNAMEMAX;
                LPSTR pszName = szNameSpaceKey + strlen(szNameSpaceKey);
                dwIndex = 0;

                // enum all sub keys
                while (   (dwResult = (RegEnumKeyEx(hProtKey, dwIndex, szName, &dwLenName, 0, 0, 0, 0)) == ERROR_SUCCESS)
                       && (cFound < *pcClsIds)
                    )
                {
                    BOOL fFound = FALSE;
                    HKEY hNameKey = NULL;
                    *pszName = '\0';
                    strcat(pszName, szName);

                    // open the Name-Space Handler root + protocols
                    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szNameSpaceKey, 0, KEY_QUERY_VALUE, &hNameKey) == ERROR_SUCCESS)
                    {
                        DWORD dwLenClass = LENNAMEMAX;
                        char szClass[LENNAMEMAX];
                        // get the class id
                        if (RegQueryValueEx(hNameKey, SZCLASS, NULL, &dwType, (LPBYTE)szClass, &dwLenClass) == ERROR_SUCCESS)
                        {
                            hr = CLSIDFromStringA(szClass, (ppclsid + cFound));
                            if (hr == NOERROR)
                            {
                                cFound++;
                            }

                            DbgLog2(tagCProtMgr, this, "LookupNameSpaceClsIDFromReg(hr:%lx, ClsId:%s) >FOUND<", hr, szClass);
                        }
                        RegCloseKey(hNameKey);

                    }

                    dwIndex++;
                    dwLenName = LENNAMEMAX;
                }
                RegCloseKey(hProtKey);
            }

        } while (dwCheck);

        RegCloseKey(hNameSpaceKey);
    }

    if (cFound)
    {
        *pcClsIds = cFound;
        hr = NOERROR;
    }
    
    PerfDbgLog1(tagCProtMgr, this, "-CProtMgrNameSpace::LookupClsIDFromReg (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CProtMgrMimeHandler::LookupClsIDFromReg
//
//  Synopsis:
//
//  Arguments:  [pwzMime] --
//              [pclsid] --
//              [pcClsIds] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CProtMgrMimeHandler::LookupClsIDFromReg(LPCWSTR pwzMime, CLSID *pclsid, DWORD *pcClsIds, DWORD *pdwFlags, DWORD dwOpt)
{
    PerfDbgLog1(tagCProtMgr, this, "+CProtMgrMimeHandler::LookupClsIDFromReg (pwzMime:%ws)", pwzMime);
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;

    HKEY hMimeKey = NULL;
    DWORD dwError;
    DWORD dwType;
    char szValue[256];
    DWORD dwValueLen = 256;
    char szKey[SZMIMESIZE_MAX + 256];

    TransAssert((pwzMime));

    char szStr[ULPROTOCOLLEN+1];
    W2A(pwzMime, szStr, ULPROTOCOLLEN);
    szStr[ULPROTOCOLLEN] = 0;

    strcpy(szKey, SZ_SH_FILTERROOT);
    strcat(szKey, szStr);

    if (SHRegGetUSValue(
                    szKey, 
                    SZCLASS, 
                    NULL, 
                    (LPBYTE)szValue, 
                    &dwValueLen,
                    FALSE, 
                    NULL, 
                    0) == ERROR_SUCCESS)
    {
            hr = CLSIDFromStringA(szValue, pclsid);
            PerfDbgLog2(
                    tagCProtMgr, 
                    this, 
                    "API FOUND LookupFilterClsIDFromReg(hr:%lx, ClsId:%s)", 
                    hr, 
                    szValue);
    }


    // there are some machine incorrectly installed text/html and point to 
    // PlugProt.dll and urlmon.dll. so if we find out the ksy is point to
    // these dll, we still return error, otherwise (the key might be 
    // installed by 3rd party) we uses the key.
    // const GUID CLSID_MimeHandlerTest1   = {0x79eaca02, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
    if(    !wcsicmp(pwzMime, L"text/html") 
        && IsEqualGUID(*pclsid ,CLSID_ClassInstallFilter )  
      )
    {
        hr = INET_E_UNKNOWN_PROTOCOL;
        *pclsid = CLSID_NULL;
    }

    PerfDbgLog1(tagCProtMgr, this, "-CProtMgrMimeHandler::LookupClsIDFromReg (hr:%lx)", hr);
    return hr;
}
