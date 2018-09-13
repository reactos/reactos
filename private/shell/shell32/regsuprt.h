#ifndef __REGSUPRT_H
#define __REGSUPRT_H

#define REG_OPTION_INVALID  0xFFFFFFFF

// MAX_ROOT is the maximum we support for the root
#define MAX_ROOT            MAX_PATH

class CRegSupport
{
public:
    CRegSupport();
    virtual ~CRegSupport();

public:
    void RSInitRoot(HKEY hkey, LPCTSTR pszSubKey1, LPCTSTR pszSubKey2, 
        DWORD dwRootOptions = REG_OPTION_VOLATILE, 
        DWORD dwDefaultOptions = REG_OPTION_VOLATILE);

    virtual BOOL RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    virtual BOOL RSSubKeyExist(LPCTSTR pszSubKey);

    virtual BOOL RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    virtual BOOL RSDeleteSubKey(LPCTSTR pszSubKey);
    virtual BOOL RSDeleteKey();

    virtual HKEY RSDuplicateRootKey();

    BOOL RSGetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               LPTSTR pszValue, DWORD* pcchValue);
    BOOL RSGetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               PBYTE pb, DWORD* pcb);
    BOOL RSGetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, DWORD* pdwValue);

    BOOL RSSetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, LPCTSTR pszValue,
        DWORD dwOptions = REG_OPTION_INVALID);
    BOOL RSSetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, PBYTE pb, DWORD cb,
        DWORD dwOptions = REG_OPTION_INVALID);
    BOOL RSSetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, DWORD dwValue,
        DWORD dwOptions = REG_OPTION_INVALID);

protected:
    virtual void _CloseRegSubKey(HKEY hkeyVolumeSubKey);

    virtual HKEY _GetRootKey(BOOL fCreate, DWORD dwOptions = REG_OPTION_INVALID);

    virtual BOOL _SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD cb, DWORD dwType,
                                DWORD dwOptions);
    virtual BOOL _GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD* pcb);

    HKEY _GetSubKey(LPCTSTR pszSubKey, BOOL fCreate,
        DWORD dwOptions = REG_OPTION_INVALID);

    static HKEY _RegCreateKeyExHelper(HKEY hkey, LPCTSTR pszSubKey,
        DWORD dwOptions);
    static HKEY _RegOpenKeyExHelper(HKEY hkey, LPCTSTR pszSubKey);

protected:
    virtual BOOL _InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2);
    void _InitCSKeyRoot();
    void _EnterCSKeyRoot();
    void _LeaveCSKeyRoot();

    virtual LPCTSTR _GetRoot(LPTSTR pszRoot, DWORD cchRoot);

protected:
    LPCTSTR                 _pszSubKey1;
    LPCTSTR                 _pszSubKey2;

private:
    DWORD                   _dwRootOptions;
    DWORD                   _dwDefaultOptions;

    HKEY                    _hkeyInit; // HKEY_CURRENT_USER, ...
    
    CRITICAL_SECTION        _csKeyRoot;
    BOOL                    _fcsKeyRoot;

#ifdef DEBUG
    static UINT             _cRefHKEY;
    static UINT             _cRefExternalHKEY;

    BOOL                    _fInited;
#endif
};

#endif //__REGSUPRT_H