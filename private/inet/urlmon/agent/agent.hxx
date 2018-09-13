//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       agent.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _AGENT_HXX_
#define _AGENT_HXX_

#include "webchk.h"


class CAtomX
{
public:
    CAtomX()
    {
    }
    CAtomX(ATOM atom)
    {
        _atom = atom;
    }
    CAtomX(LPCWSTR pwzStr)
    {
        char szStr[SZMIMESIZE_MAX];
        W2A(pwzStr, szStr, SZMIMESIZE_MAX);
        _atom = AddAtom(szStr);
    }
    CAtomX(LPCSTR pszStr)
    {
        _atom = AddAtom(pszStr);
    }

    operator void* ()
    {
        return (void *)_atom;
    }

    ~CAtomX()
    {
    }

private:
    DWORD _atom;
};

class CKey
{
public:
    CKey()
    {
    }
    CKey(ATOM atom)
    {
        _atom = atom;
    }
    CKey(LPCWSTR pwzStr)
    {
        W2A(pwzStr, _szStr, SZMIMESIZE_MAX);
        _atom = AddAtom(_szStr);
        //_pwzStr = pwzStr;
        _pszStr = _szStr;

    }
    CKey(LPCSTR pszStr)
    {
        _atom = AddAtom(pszStr);
    }

    operator void* ()
    {
        return (void *)_atom;
    }

    /*
    operator LPCWSTR ()
    {
        return _pwzStr;
    }
    */
    operator LPCSTR ()
    {
        return _pszStr;
    }


    ~CKey()
    {
    }

private:
    DWORD   _atom;
    //LPCWSTR _pwzStr;
    LPCSTR  _pszStr;
    char    _szStr[SZMIMESIZE_MAX];

};

class CXUnknown
{
    public:
        CXUnknown(IUnknown *pUnk)
        {
            _pUnk = pUnk;
            _pUnk->AddRef();
        }

        ~CXUnknown()
        {
            _pUnk->Release();
        }


    private:
        IUnknown *_pUnk;
};

#ifdef foofoo
//class CXNode : public CList<CXUnknown *, CXUnknown*>
//class CXNode : public CList<IUnknown *, IUnknown*>
clase CXNode : public CPtrArray
{
    public:

        CXNode() : _cElements(0)
        {
            _pos = 0;
        }

        CXNode(IUnknown *pUnk) : _cElements(1)
        {
            Add(pUnk);
            pUnk->AddRef();
            _pos = 0;
        }

        BOOL Add(IUnknown *pUnk)
        {
            _cElements++;
            Add(pUnk);
            pUnk->AddRef();
            return TRUE;
        }

        BOOL Remove(IUnknown *pUnk)
        {
            IUnknown *pUnkVal = 0;

            if (_Map.Lookup(pUnk, (void *&)pUnkVal) )
            {
                _Map.RemoveKey(pUnk);
                _cElements--;
                pUnk->Release();
            }
            else
            {
                TransAssert((FALSE));
            }
            return (_cElements) ? TRUE : FALSE;
        }

        IUnknown * FindFirst()
        {
            IUnknown *pUnkVal = 0;

            _pos = _Map.GetStartPosition();

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) pUnkVal);
            }

            return pUnkVal;
        }

        IUnknown * FindNext()
        {
            IUnknown *pUnkVal = 0;

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (void *&)dwKey, (void *&) pUnkVal);
            }

            return pUnkVal;
        }

        ~CProtNode()
        {
            TransAssert((_cElements == 0));
        }

    private:
        CRefCount  _cElements;
        int        _pos;
};
#endif //foofoo

//+---------------------------------------------------------------------------
//
//  Class:      CProtNode ()
//
//  Purpose:
//
//  Interface:  Add --
//              Remove
//              FindFirst --
//              FindNext --
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CProtNode : public CObject
{
    public:
        CProtNode(IUnknown *pUnk) : _cElements(1)
        {
            _Map.SetAt(pUnk, pUnk);
            pUnk->AddRef();
        }

        BOOL Add(IUnknown *pUnk)
        {
            _cElements++;
            _Map.SetAt(pUnk, pUnk);
            pUnk->AddRef();
            return TRUE;
        }

        BOOL Remove(IUnknown *pUnk)
        {
            IUnknown *pUnkVal = 0;

            if (_Map.Lookup(pUnk, (void *&)pUnkVal) )
            {
                _Map.RemoveKey(pUnk);
                _cElements--;
                pUnk->Release();
            }
            else
            {
                TransAssert((FALSE));
            }
            return (_cElements) ? TRUE : FALSE;
        }

        /*
        IUnknown * FindFirst()
        {
            IUnknown *pUnkVal = 0;

            _pos = _Map.GetStartPosition();

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) pUnkVal);
                if (pUnkVal)
                {
                    pUnkVal->AddRef();
                }
            }

            return pUnkVal;
        }


        IUnknown * FindNext()
        {
            IUnknown *pUnkVal = 0;

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (void *&)dwKey, (void *&) pUnkVal);
                if (pUnkVal)
                {
                    pUnkVal->AddRef();
                }
            }

            return pUnkVal;
        }
        */

        BOOL FindFirst(IUnknown *& prUnk)
        {
            BOOL fRet = FALSE;

            _pos = _Map.GetStartPosition();

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) prUnk);
                if (prUnk)
                {
                    prUnk->AddRef();
                    fRet = TRUE;
                }
            }

            return fRet;
        }

        BOOL FindNext(IUnknown *& prUnk)
        {
            BOOL fRet = FALSE;

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) prUnk);
                if (prUnk)
                {
                    prUnk->AddRef();
                    fRet = TRUE;
                }
            }

            return fRet;
        }



        ~CProtNode()
        {
            TransAssert((_cElements == 0));
        }

    private:
        CRefCount       _cElements;
        CAtomX          _atom;
        CMapPtrToPtr    _Map;
        POSITION        _pos;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMapStrToXVal ()
//
//  Purpose:
//
//  Interface:  QueryInterface --
//              AddRef --
//              Release --
//              AddVal --
//              UnAddVal --
//              FindFirst --
//              FindNext --
//              _cElements --
//              _CRefs --
//              _cElements --
//              _Map --
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CMapStrToXVal
{
public:
    STDMETHODIMP AddVal(IUnknown *pUnk,LPCWSTR pwzName);
    STDMETHODIMP RemoveVal(IUnknown *pUnk,LPCWSTR pwzName);
    STDMETHODIMP FindFirst(LPCWSTR pwzName, IUnknown **ppUnk);
    STDMETHODIMP FindNext(LPCWSTR pwzName, IUnknown **ppUnk);

public:
    CMapStrToXVal() : _cElements(0), _CRefs(1)
    {
    }

    ~CMapStrToXVal()
    {
        TransAssert((_cElements == 0));
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // the total refcount of this object
    CMapStringToOb  _Map;
    CMutexSem       _mxs;           // single access to protocol manager
};



#define WEBCHECK_ITEMSINKS_MAX 256


class CUnknown : public IUnknown
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

private:
    CRefCount           _CRefs;          // the total refcount of this object
};


class COInetItem : public IOInetItem
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP GetURL(
            LPOLESTR *ppwzUrl
            );

    STDMETHODIMP GetInfo(
            DWORD     dwOptions,
            LPOLESTR *ppwzItemMime,
            LPCLSID  *pclsidItem,
            LPOLESTR *ppwzProtocol,
            LPCLSID  *pclsidProtocol,
            DWORD    *pdwOut
            );

    STDMETHODIMP GetItemData(
            DWORD *grfITEMF,
            ITEMDATA * pitemdata
            );

private:
    CRefCount           _CRefs;          // the total refcount of this object

};

class COInetItemSink : public IOInetItemSink
{
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP OnItem(
        ITEMTYPE   itemtype,
        IOInetItem *pWChkItem,
        DWORD      dwReserved
        );

private:
    CRefCount           _CRefs;          // the total refcount of this object
};


class COInetAgent : public IOInetAgent
{
public:

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // will start a listening protocol (APP) and listen for items
    // eg. mc or alert etc.
    STDMETHODIMP StartListen(
        const LPCWSTR       szProtocol,
        IOInetItemFilter   *pWChkItemFilter,
        DWORD               grfFilterMode,
        CLSID              *pclsidProtocol,
        DWORD               dwReserved
        );

    // will release the sink passed in at StartListen
    STDMETHODIMP StopListen(
        CLSID   *pclsidProtocol
        );

    // add item to the spooler
    STDMETHODIMP ScheduleItem(
        IOInetItem          *pWChkItem,
        IOInetDestination   *pWChkDest,
        SCHEDULEDATA       *pschdata,
        DWORD               dwMode,
        DWORD     *pdwCookie
        );

    STDMETHODIMP RevokeItem(
        DWORD      dwCookie
        );


private:
    CRefCount           _CRefs;          // the total refcount of this object
    COInetItemSink       _aCWIS[WEBCHECK_ITEMSINKS_MAX];    // the connection points
    COInetItemSink       _aCWISListen[WEBCHECK_ITEMSINKS_MAX];    // the connection points

};

class COInetAdvisor : public IOInetAdvisor
{
public:

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // will start a listening protocol (APP) and listen for items
    // eg. mc or alert etc.
    STDMETHODIMP StartListen(
        const LPCWSTR  wzProtocol,
        IOInetItemFilter   *pWChkItemFilter,
        DWORD          grfFilterMode,
        CLSID         *pclsidProtocol,
        DWORD          dwReserved
        );

    // will release the sink passed in at StartListen
    STDMETHODIMP StopListen(
        CLSID   *pclsidProtocol
        );

    STDMETHODIMP Advise(
        IOInetItemSink *pWChkItemSink,
        DWORD          grfMode,
        ULONG          cMimes,
        const LPCWSTR *ppwzItemMimes,
        DWORD          dwReserved
        );

    STDMETHODIMP Unadvise(
        IOInetItemSink *pWChkItemSink,
        ULONG          cMimes,
        const LPCWSTR *ppwzItemMimes
        );

    STDMETHODIMP SendAdvise(
        ITEMTYPE       itemtype,
        IOInetItem     *pWChkItem,
        DWORD          grfMode,
        LPCWSTR        pwzItemMimes,
        DWORD          dwReserved
        );

public:
    COInetAdvisor() : _CRefs(1)
    {
    }

    ~COInetAdvisor()
    {
    }


private:
    CRefCount           _CRefs;          // the total refcount of this object
    CMapStrToXVal       _AdvSinks;

};
HRESULT GetOInetAdvisor(DWORD dwMode, IOInetAdvisor **ppOInetAdvisor, DWORD dwReserved);


#endif // _AGENT_HXX_

