#include "priv.h"
#include <shlwapi.h>
#include "w95wraps.h"

//RC Stream Type
#define RCST_RAW            0
#define RCST_COMPRESSED     1

class CRCStream : public IStream
{
public:
    CRCStream(LPCWSTR pcszFileName, int iResID, HRESULT* phres = NULL);
    ~CRCStream();
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid,void **pv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ISequentialStream methods (copied from sdk\inc\objidl.h and edited)
    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead);
    STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten) { return E_NOTIMPL; }
    
    // IStream methods (copied from sdk\inc\objidl.h and edited)
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER* plibNewPosition) { return E_NOTIMPL; }
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) { return E_NOTIMPL; }
    STDMETHODIMP CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, 
        ULARGE_INTEGER* pcbWritten) { return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD grfCommitFlags) { return E_NOTIMPL; }
    STDMETHODIMP Revert(void) { return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
        DWORD dwLockType) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
        DWORD dwLockType) { return E_NOTIMPL; }
    STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) { return E_NOTIMPL; }
    STDMETHODIMP Clone(IStream** ppstm) { return E_NOTIMPL; }
private:
    STDMETHODIMP CRCStream::Read_RCST_COMPRESSED(void* pv, ULONG cb, ULONG* pcbRead);
    STDMETHODIMP CRCStream::Read_RCST_RAW(void* pv, ULONG cb, ULONG* pcbRead);
private:
    LONG        _cRef;      

    HINSTANCE   _hinstModule;

    PBYTE       _pbData;
    ULONG       _cbData;

    WORD        _wType;

    PBYTE       _pbPos;

    //
    // RCST_COMPRESSED
    //
    // UnCompressed data to this point
    ULONG       _cbUnComp;
    // Total data to UnCompress
    ULONG       _cbTotalUnComp;
    BYTE        _cbRemain;
};

CRCStream::CRCStream(LPCWSTR pcszFileName, int iResID, HRESULT* phres) : _cRef(1)
{
    ASSERT(pcszFileName && *pcszFileName);

    HRESULT hres = E_FAIL;

    HINSTANCE _hinstModule = LoadLibraryW(pcszFileName);

    if (_hinstModule)
    {
        HRSRC hRsrc = FindResource(_hinstModule, MAKEINTRESOURCE(iResID), RT_RCDATA);

        if (hRsrc)
        {
            HGLOBAL hLoadedRes = LoadResource(_hinstModule, hRsrc);

            if (hLoadedRes)
            {
                _pbData = (PBYTE)LockResource(hLoadedRes);

                hres = (_pbData?S_OK:E_OUTOFMEMORY);

                int cbResData = SizeofResource(_hinstModule, hRsrc);

                // Extract type
                WORD* pwType = (WORD*)_pbData;

                _wType = *pwType;

                _pbData = (PBYTE)(pwType + 1);

                switch(_wType)
                {
                    case RCST_RAW:
                    {
                        // The first DWORD is the actual size of the blob we've put in the resource
                        // this needs to be done because RCDATA takes only WORDs and we may need to 
                        // put an odd number of bytes in RCDATA

                        ASSERT(sizeof(ULONG) == sizeof(DWORD));

                        DWORD* pdwBuf = (DWORD*)_pbData;

                        _cbData = *pdwBuf;

                        _pbData = (PBYTE)(pdwBuf + 1);

                        _pbPos = _pbData;

                        break;
                    }
                    case RCST_COMPRESSED:
                    {
                        ASSERT(sizeof(ULONG) == sizeof(DWORD));

                        DWORD* pdwBuf = (DWORD*)_pbData;

                        _cbTotalUnComp = *pdwBuf;
                        ++pdwBuf;

                        // Get compress file size
                        _cbData = *pdwBuf;

                        _pbData = (PBYTE)(pdwBuf + 1);

                        _pbPos = _pbData;

                        break;
                    }
                    default:
                    {
                        hres = E_FAIL;
                        break;
                    }                    
                }
            }
            else
                hres = E_OUTOFMEMORY;
        }
        else
            hres = E_INVALIDARG;
    }
    else
        hres = E_INVALIDARG;

    if (phres)
        *phres = hres;
}


CRCStream::~CRCStream()
{
}

STDMETHODIMP CRCStream::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
      QITABENT(CRCStream, IStream),
      { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CRCStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CRCStream::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CRCStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
    ASSERT(pv);
    HRESULT hres = E_FAIL;

    switch(_wType)
    {
        case RCST_RAW:
        {
            hres = Read_RCST_RAW(pv, cb, pcbRead);
            break;
        }
        case RCST_COMPRESSED:
        {
            hres = Read_RCST_COMPRESSED(pv, cb, pcbRead);
            break;
        }
        default:
        {
            break;
        }                    
    }
    return hres;
}

STDMETHODIMP CRCStream::Read_RCST_COMPRESSED(void* pv, ULONG cb, ULONG* pcbRead)
{
    HRESULT hres = E_FAIL;
    ULONG cbCopy = 0;

    if (pv)
    {
        if (cb > 0)
        {
            // Is the requested nb of bytes completely fitting in buffer from
            // actual position?
            if (cb <= (_cbTotalUnComp - _cbUnComp))
            {
                // Yes
                cbCopy = cb;
            }
            else
            {
                // No, is there at least one byte to copy?
                if (_cbUnComp < _cbTotalUnComp)
                {
                    // Yes
                    cbCopy = _cbTotalUnComp - _cbUnComp;
                }
                else
                {
                    // No
                    ASSERT(_cbUnComp == _cbTotalUnComp);

                    cbCopy = 0;
                }
            }

            // Is there anything to copy?
            if (cbCopy > 0)
            {
                ULONG cbTmpCopy = cbCopy;
                PBYTE pbOut = (PBYTE)pv;

                // Yes, copy it and move the position pointer, if appropriate

                while (cbTmpCopy)
                {
                    // Do we have a repeated byte?
                    if (0 != *_pbPos)
                    {
                        // Yes, expand it

                        // e.g.: 0x05, 0x98
                        // 0x05 : 5 repeated bytes
                        // 0x98 : the byte that needs to be repeated

                        PBYTE pbTmpPos = _pbPos;

                        // Is section already partially expanded?
                        if (!_cbRemain)
                        {
                            // Total number of repeated bytes
                            _cbRemain = *_pbPos;
                        }

                        BYTE cbExpand = 0;

                        // Is the remaining bytes to expand less than/equal to the 
                        // requested bytes?
                        if (_cbRemain <= cbTmpCopy)
                        {
                            // Yes copy all the repeated bytes
                            cbExpand = _cbRemain;

                            cbTmpCopy -= cbExpand;

                            _cbRemain = 0;

                            // Next time, go to the next repeated/non-repeated section
                            _pbPos += 2;
                        }
                        else
                        {
                            // No, there will be some remaining

                            // We know cbTmpCopy <= 255, so OK to cast
                            cbExpand = (BYTE)cbTmpCopy;

                            cbTmpCopy = 0;

                            _cbRemain -= cbExpand;
                        }
                        // Do the copy
                        while (cbExpand)
                        {
                            *pbOut = *(pbTmpPos + 1);

                            ++pbOut;
        
                            --cbExpand;
                        }
                    }
                    else
                    {
                        // No, copy the non-repeated bytes as is

                        // e.g.: 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05
                        // 0x00 : non-repeat bytes flag
                        // 0x05 : there will 5 non repeated bytes
                        // ...  : the non-repeated bytes that need to be copied as is

                        PBYTE pbTmpPos = _pbPos + 2;
                        // Is section already partially expanded?
                        if (!_cbRemain)
                        {
                            // No

                            // Total number of repeated bytes
                            _cbRemain = *(_pbPos + 1);
                        }
                        else
                            pbTmpPos += *(_pbPos + 1) - _cbRemain ;

                        BYTE cbExpand = 0;

                        // Is the remaining bytes to expand less than/equal to the 
                        // requested bytes?
                        if (_cbRemain <= cbTmpCopy)
                        {
                            // Yes copy all the repeated bytes
                            cbExpand = _cbRemain;

                            cbTmpCopy -= cbExpand;

                            _cbRemain = 0;

                            // Next time, go to the next repeated/non-repeated section
                            _pbPos += 1 + 1 + (*(_pbPos + 1));
                        }
                        else
                        {
                            // No, there will be some remaining

                            // We know cbTmpCopy <= 255, so OK to cast
                            cbExpand = (BYTE)cbTmpCopy;

                            cbTmpCopy = 0;

                            _cbRemain -= cbExpand;
                        }
                        // Do the copy
                        memcpy(pbOut, pbTmpPos, cbExpand);

                        pbOut += cbExpand;
                    }
                }
            }
            hres = S_OK;

            _cbUnComp += cbCopy;
        }
        else
        {
            if (cb == 0)
                hres = S_OK;
            else
                hres = S_FALSE;
        }
    }
    else
        hres = STG_E_INVALIDPOINTER;

    if (pcbRead)
        *pcbRead = cbCopy;

    return hres;
}

STDMETHODIMP CRCStream::Read_RCST_RAW(void* pv, ULONG cb, ULONG* pcbRead)
{
    HRESULT hres = E_FAIL;
    ULONG cbCopy = 0;

    if (pv)
    {
        if (cb > 0)
        {
            // Is the requested nb of bytes completely fitting in buffer from
            // actual position?
            if ((_pbPos + cb) <= (_pbData + _cbData))
            {
                // Yes
                cbCopy = cb;
            }
            else
            {
                // No, is there at least one byte to copy?
                if (_pbPos < (_pbData + _cbData))
                {
                    // Yes
                    cbCopy = (ULONG)((_pbData + _cbData) - _pbPos);
                }
                else
                {
                    // No
                    ASSERT(_pbPos == (_pbData + _cbData));

                    cbCopy = 0;
                }
            }

            // Is there anything to copy?
            if (cbCopy > 0)
            {
                // Yes, copy it and move the position pointer
                memcpy(pv, _pbPos, cbCopy);
        
                _pbPos += cbCopy;
            }

            hres = S_OK;
        }
        else
        {
            if (cb == 0)
                hres = S_OK;
            else
                hres = S_FALSE;
        }
    }
    else
        hres = STG_E_INVALIDPOINTER;

    if (pcbRead)
        *pcbRead = cbCopy;

    return hres;
}

STDAPI SHOpenStreamOnRCFromRegValueW(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValue, 
                                      IStream** pStream)
{
    ASSERT(pStream);

    HRESULT hres = E_FAIL;
    *pStream = NULL;

    HKEY hOpenedKey = NULL;

    if (ERROR_SUCCESS == RegOpenKeyExW(hkey, pszSubKey, 0, KEY_QUERY_VALUE, &hOpenedKey))
    {
        // +20 for the comma and the resource id
        WCHAR szModule[MAX_PATH + 20];
        WCHAR* pszExpanded = szModule;
        DWORD dwModule = ARRAYSIZE(szModule);

        DWORD dwType = 0;

        LONG lRes = RegQueryValueExW(hOpenedKey, pszValue, NULL, &dwType, (LPBYTE)szModule, &dwModule);

        if (ERROR_SUCCESS == lRes)
        {
            WCHAR szExpanded[MAX_PATH + 20];
            // Do we need to expand Env variable?
            if (REG_EXPAND_SZ == dwType)
            {
                // Yes
                SHExpandEnvironmentStringsW(szModule, szExpanded, ARRAYSIZE(szExpanded));
                pszExpanded = szExpanded;
            }

            // pszExpanded should now contain something like: TEXT("C:\\WINNT\\SYSTEM32\\shell32.dll,1234")
            if (dwRes && pszExpanded && *pszExpanded)
            {
                int iResID = PathParseIconLocationW(pszExpanded);

                // Create the object
                *pStream = new CRCStream(pszExpanded, iResID, &hres);
            }
        }
        RegCloseKey(hOpenedKey);
    }
    return hres;
}

STDAPI SHOpenStreamOnRCFromRegValueA(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue, 
                                      IStream** pStream)
{
    ASSERT(pStream);

    *pStream = NULL;

    HRESULT hres = E_INVALIDARG;

    if (pszSubKey)
    {
        WCHAR wszSubKey[MAX_PATH];
        WCHAR wszValue[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, pszSubKey, -1, wszSubKey, SIZECHARS(wszSubKey));
        pszSubKey = (LPCSTR)wszSubKey;

        if (pszValue)
        {
            MultiByteToWideChar(CP_ACP, 0, pszValue, -1, wszValue, SIZECHARS(wszValue));
            pszValue = (LPCSTR)wszValue;
        }

        SHOpenStreamOnRCFromRegValueW(hkey, (LPCWSTR)pszSubKey, (LPCWSTR)pszValue, pStream);
    }

    return hres;
}
