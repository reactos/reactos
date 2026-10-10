/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IQueryAssociations
 * COPYRIGHT:   Copyright 2002 Jon Griffiths (Wine)
 *              Copyright 2024-2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"
#include <shlwapi_undoc.h> // AssocCreateElement
#include "evalcmd/elements.h" // ASSOCQUERY_*
#include <evalcmd.h> // SHELL32_CAssocElement_Version
#if NTDDI_VERSION < NTDDI_WIN10_RS1
#define ASSOCF_PER_MACHINE_ONLY 0x00008000
#endif

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static inline HRESULT SHRegDuplicateHKey(HKEY hKey, HKEY *phKey)
{
    hKey = *phKey = ::SHRegDuplicateHKey(hKey);
    return hKey ? S_OK : HResultFromWin32(GetLastError());
}

static inline HRESULT SH32_AssocCreateElement(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ PVOID *ppv)
{
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA
    return AssocCreateElement(rclsid, riid, ppv);
#else
    return AssocCreate(rclsid, riid, ppv);
#endif
}

static HRESULT AssocCreateElementInternal(REFCLSID rclsid, IQuerySource *pQS, REFIID riid, PVOID *ppv)
{
    HRESULT hr;
    C_ASSERT(&IID_IObjectWithQuerySource == &IID_IObjectWithQuerySourceOld);
    CComPtr<IObjectWithQuerySource> pOWQS;
    if (FAILED(hr = SH32_AssocCreateElement(rclsid, IID_PPV_ARG(IObjectWithQuerySource, &pOWQS))))
        return hr;
    return (SUCCEEDED(hr = pOWQS->SetSource(pQS))) ? pOWQS->QueryInterface(riid, ppv) : hr;
}

static HRESULT AssocCreateElementOnKeyInternal(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                               _In_ REFIID riid, _Outptr_ PVOID *ppv, _In_ BYTE Major)
{
    CComPtr<IUnknown> pQSO;
    REFIID riidQS = Major > 5 ? IID_IQuerySource : IID_IQuerySourceOld;
    HRESULT hr = QuerySourceCreateFromKey(hKey, pszSubKey, false, riidQS, (void**)&pQSO);
    if (FAILED(hr))
        return hr;
    return AssocCreateElementInternal(rclsid, (IQuerySource*)static_cast<IUnknown*>(pQSO), riid, ppv);
}

static HRESULT SH32_AssocCreateElementOnKey(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                            _In_ REFIID riid, _Outptr_ PVOID *ppv)
{
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA
    HRESULT hr = AssocCreateElementOnKeyInternal(rclsid, hKey, pszSubKey, riid, ppv, 6);
    if (SUCCEEDED(hr))
        return hr;
#endif
    return AssocCreateElementOnKeyInternal(rclsid, hKey, pszSubKey, riid, ppv, 5);
}

static HRESULT SH32_QueryAssocElementString(_In_ REFCLSID rclsid, _In_ UINT Query, _In_ HKEY hKey,
                                            _In_opt_ PCWSTR pszSubKey, _In_ PCWSTR pszExtra, _Out_ PWSTR *ppszValue)
{
#if SHELL32_CAssocElement_Version >= _WIN32_WINNT_VISTA
    CComPtr<IAssociationElement> pAE;
    REFIID riid = IID_IAssociationElement;
#else
    CComPtr<IAssociationElementOld> pAE;
    REFIID riid = IID_IAssociationElementOld;
#endif
    HRESULT hr = SH32_AssocCreateElementOnKey(rclsid, hKey, pszSubKey, riid, (void**)&pAE);
    if (FAILED(hr))
        return hr;
    return pAE->QueryString(Query, pszExtra, ppszValue);
}

template<class T> static HRESULT GetKeyFromObjectWithQuerySource(T &Obj, HKEY *phKey)
{
    HRESULT hr;
    C_ASSERT(&IID_IObjectWithQuerySource == &IID_IObjectWithQuerySourceOld);
    CComPtr<IObjectWithQuerySource> pOWQS;
    if (FAILED(hr = Obj.QueryInterface(IID_PPV_ARG(IObjectWithQuerySource, &pOWQS))))
        return hr;
    CComPtr<IObjectWithRegistryKeyOld> pOWRKO;
    CComPtr<IObjectWithRegistryKey> pOWRK;
    if (SUCCEEDED(hr = pOWQS->GetSource(IID_PPV_ARG(IObjectWithRegistryKeyOld, &pOWRKO))))
        hr = pOWRKO->GetKey(phKey);
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA
    else if (SUCCEEDED(hr = pOWQS->GetSource(IID_PPV_ARG(IObjectWithRegistryKey, &pOWRK))))
        hr = pOWRK->GetKey(MAXIMUM_ALLOWED, phKey);
#endif
    return hr;
}

EXTERN_C HRESULT SHELL32_AssocGetFSDirectoryDescription(PWSTR Buf, UINT cchBuf)
{
    static WCHAR cache[33] = {};
    if (!*cache)
        LoadStringW(shell32_hInstance, IDS_DIRECTORY, cache, _countof(cache));
    return StringCchCopyW(Buf, cchBuf, cache);
}

static HRESULT GetExtensionDefaultDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf)
{
    static WCHAR fmt[33] = {};
    if (!*fmt)
        LoadStringW(shell32_hInstance, IDS_ANY_FILE, fmt, _countof(fmt));
    return StringCchPrintfW(Buf, cchBuf, fmt, DotExt);
}

static HRESULT SHELL32_AssocGetExtensionDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf)
{
    HRESULT hr;
    if (!DotExt[0] || (!DotExt[1] && DotExt[0] == '.'))
    {
        if (SUCCEEDED(hr = GetExtensionDefaultDescription(L"", Buf, cchBuf)))
            StrTrimW(Buf, L" -"); // Remove the empty %s so we are left with "File"
        return hr;
    }
    HKEY hKey;
    if (SUCCEEDED(hr = HCR_GetProgIdKeyOfExtension(DotExt, &hKey, TRUE)))
    {
        DWORD err = RegLoadMUIStringW(hKey, L"FriendlyTypeName", Buf, cchBuf, NULL, 0, NULL);
        if (err && hr == S_OK) // ProgId default value fallback (but not if we only have a .ext key)
        {
            DWORD cb = cchBuf * sizeof(*Buf);
            err = RegGetValueW(hKey, NULL, NULL, RRF_RT_REG_SZ, NULL, Buf, &cb);
        }
        RegCloseKey(hKey);
        if (!err)
            return err;
    }
    // No information in the registry, default to "UPPERCASEEXT File"
    WCHAR ext[MAX_PATH + 33];
    if (LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, ++DotExt, -1, ext, _countof(ext)))
        DotExt = ext;
    return GetExtensionDefaultDescription(DotExt, Buf, cchBuf);
}

EXTERN_C HRESULT SHELL32_AssocGetFileDescription(PCWSTR Name, PWSTR Buf, UINT cchBuf)
{
    return SHELL32_AssocGetExtensionDescription(PathFindExtensionW(Name), Buf, cchBuf);
}

struct ClassesRegKey : CQueryAssociations::RegKeyRef
{
    HRESULT Initialize(ASSOCF fA, PCWSTR pszSource);
};

HRESULT ClassesRegKey::Initialize(ASSOCF fA, PCWSTR pszSource)
{
    if (!(fA & ASSOCF_PER_MACHINE_ONLY))
    {
        hKey = HKEY_CLASSES_ROOT;
        return S_OK;
    }
    HKEY hKeyClasses;
    LONG err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes", 0, KEY_READ, &hKeyClasses);
    if (err)
        return HResultFromWin32(err);
    err = RegOpenKeyExW(hKeyClasses, pszSource, 0, KEY_READ, &hKey);
    RegCloseKey(hKeyClasses);
    if (err == ERROR_SUCCESS)
        hKeyClose = hKey;
    return HResultFromWin32(err);
}

static HRESULT OpenShellVerbKey(HKEY hClass, LPCWSTR pszVerb, HKEY *phKey)
{
    const SIZE_T cchPrefix = sizeof("shell\\") - 1;
    WCHAR szPath[cchPrefix + MAX_PATH];
    if (FAILED(StringCchPrintfW(szPath, _countof(szPath), L"shell\\%s", pszVerb ? pszVerb : L"")))
        return E_FAIL;

    if (!pszVerb && !HCR_GetDefaultVerbW(hClass, NULL, szPath + cchPrefix, _countof(szPath) - cchPrefix))
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    return HResultFromWin32(RegOpenKeyExW(hClass, szPath, 0, KEY_READ, phKey));
}

static HRESULT OpenClassWithShellExecKey(ASSOCF fA, HKEY hClass, LPCWSTR pszVerb, HKEY *phKey)
{
    if (fA & ASSOCF_VERIFY)
    {
        HRESULT hr = OpenShellVerbKey(hClass, pszVerb, phKey);
        if (FAILED(hr))
            return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        RegCloseKey(*phKey);
    }
    return SHRegDuplicateHKey(hClass, phKey);
}

static inline HRESULT GetClassKeyName(ASSOCF fA, PCWSTR pszSource, HKEY hSource, LPWSTR pszClass, UINT cchMax)
{
    PCWSTR pszSubKey = *pszSource == L'{' ? L"ProgID" : NULL;
    return SHELL_RegGetString(hSource, pszSubKey, NULL, pszClass, cchMax);
}

static HRESULT OpenClassKey(ASSOCF fA, PCWSTR pszSource, HKEY hCR, HKEY hSource, HKEY *phKey)
{
    WCHAR szBuf[MAX_PATH];
    HRESULT hr = E_FAIL;
    if (*pszSource == L'.' || (*pszSource == L'{' && !(fA & ASSOCF_INIT_NOREMAPCLSID)))
    {
        hr = GetClassKeyName(fA, pszSource, hSource, szBuf, _countof(szBuf));
        if (FAILED(hr) && *pszSource == L'.')
            return hr; // .ext can only have its ProgId in another location
    }

    HKEY hClassKey;
    if (SUCCEEDED(hr))
        hr = HResultFromWin32(RegOpenKeyExW(hCR, szBuf, 0, KEY_READ, &hClassKey));
    else
        hr = SHRegDuplicateHKey(hSource, &hClassKey);

    if (FAILED(hr))
        return hr;

    // Check for CurVer redirection 
    if (SUCCEEDED(SHELL_RegGetString(hClassKey, L"CurVer", NULL, szBuf, _countof(szBuf))))
    {
        if (RegOpenKeyExW(hCR, szBuf, 0, KEY_READ, phKey) == ERROR_SUCCESS)
        {
            if (RegKeyExists(*phKey, L"shell")) // Does it look like a valid ProgId?
            {
                RegCloseKey(hClassKey);
                return S_OK;
            }
            RegCloseKey(*phKey);
        }
    }
    *phKey = hClassKey;
    return hr;
}

static HRESULT OpenClassKey(ASSOCF fA, const WCHAR *const pszSource, HKEY *phKey)
{
    // TODO: Handle the FileExts key and ASSOCF_NOUSERSETTINGS?
    PCWSTR pszSourcePath = pszSource;
    WCHAR buf[100];
    if (*pszSource == L'{')
    {
        HRESULT hr = StringCchPrintfW(buf, _countof(buf), L"CLSID\\%s", pszSource);
        if (FAILED(hr))
            return hr;
        pszSourcePath = buf;
    }
    ClassesRegKey cr;
    if (HRESULT hr = cr.Initialize(fA, pszSource))
        return hr;
    HKEY hSourceKey;
    LONG err = RegOpenKeyExW(cr.hKey, pszSourcePath, 0, KEY_READ, &hSourceKey);
    if (err)
        return HResultFromWin32(err);
    HRESULT hr = OpenClassKey(fA, pszSource, cr.hKey, hSourceKey, phKey);
    RegCloseKey(hSourceKey);
    return hr;
}

/**************************************************************************
 *  IQueryAssociations
 *
 * DESCRIPTION
 *  This object provides a layer of abstraction over the system registry in
 *  order to simplify the process of parsing associations between files.
 *  Associations in this context means the registry entries that link (for
 *  example) the extension of a file with its description, list of
 *  applications to open the file with, and actions that can be performed on it
 *  (the shell displays such information in the context menu of explorer
 *  when you right-click on a file).
 *
 * HELPERS
 * You can use this object transparently by calling the helper functions
 * AssocQueryKeyA(), AssocQueryStringA() and AssocQueryStringByKeyA(). These
 * create an IQueryAssociations object, perform the requested actions
 * and then dispose of the object. Alternatively, you can create an instance
 * of the object using AssocCreate() and call the following methods on it:
 *
 * METHODS
 */

CQueryAssociations::CQueryAssociations() : hkeySource(0), hkeyProgID(0), m_InitFlags(0), m_InitType(0)
{
}

CQueryAssociations::~CQueryAssociations()
{
    Reset();
}

void CQueryAssociations::Reset()
{
    if (hkeySource)
        RegCloseKey(hkeySource);
    if (hkeyProgID && hkeyProgID != hkeySource)
        RegCloseKey(hkeyProgID);
    hkeySource = hkeyProgID = NULL;
    m_InitType = 0;
}

HRESULT CQueryAssociations::DuplicateHKey(HKEY hKey, HKEY *phKey)
{
    if (!hKey)
        return HResultFromWin32(IsNT5() ? ERROR_FILE_NOT_FOUND : ERROR_NO_ASSOCIATION);
    return SHRegDuplicateHKey(hKey, phKey);
}

CQueryAssociations* CQueryAssociations::TryBaseClass(ASSOCF flags)
{
    if (m_SkipBaseClass || (flags & ASSOCF_IGNOREBASECLASS))
        return NULL;
    else if (m_pBaseClass)
        return m_pBaseClass;

    CComPtr<CQueryAssociations> pQA;
    if (FAILED(ShellObjectCreator(pQA)))
        return NULL;

    ASSOCF initflags = m_InitFlags & (ASSOCF_INIT_DEFAULTTOFOLDER | ASSOCF_INIT_DEFAULTTOSTAR);
    PWSTR classes[3] = { }; // Note: Without dynamic array support we can only emulate Windows 2000 here
    if (EmulateWin2000()) // BaseClass is present in the registry but it does not seem to act like a base class on XP+
        SHELL_RegGetStringAlloc(GetClassHandle(), NULL, L"BaseClass", classes[0]); // AudioCD/DVD etc.
    if (initflags & ASSOCF_INIT_DEFAULTTOFOLDER)
        classes[1] = const_cast<PWSTR>(L"Folder");
    if (initflags & ASSOCF_INIT_DEFAULTTOSTAR)
        classes[2] = const_cast<PWSTR>(L"*");
    for (UINT i = 0; i < _countof(classes); ++i, initflags = 0)
    {
        if (FAILED(pQA->Init(initflags, classes[i], 0, NULL)))
            continue;
        m_pBaseClass = pQA;
        break;
    }
    SHFree(classes[0]);
    m_SkipBaseClass = !m_pBaseClass;
    return m_pBaseClass;
}

HRESULT CQueryAssociations::OpenShellVerbKey(ASSOCF flags, LPCWSTR pszVerb, LPCWSTR pszSubKey, RegKeyRef &KeyRef)
{
    HKEY hClass = GetClassHandle();
    if (!hClass)
        return HResultFromWin32(ERROR_NO_ASSOCIATION);
    HKEY hKey, hSubKey;
    HRESULT hr = ::OpenShellVerbKey(hClass, pszVerb, &hKey);
    if (FAILED(hr))
        return hr;
    
    if (pszSubKey)
    {
        hr = HResultFromWin32(RegOpenKeyExW(hKey, pszSubKey, 0, KEY_READ, &hSubKey));
        RegCloseKey(hKey);
        if (FAILED(hr))
            return hr;
        hKey = hSubKey;
    }
    KeyRef.Attach(hKey);
    return hr;
}

/**************************************************************************
 *  IQueryAssociations_Init
 *
 * Initialise an IQueryAssociations object.
 *
 * PARAMS
 *  cfFlags    [I] ASSOCF_ flags from "shlwapi.h"
 *  pszAssoc   [I] String for the root key name, or NULL if hkeyProgid is given
 *  hkeyProgid [I] Handle for the root key, or NULL if pszAssoc is given
 *  hWnd       [I] Reserved, must be NULL.
 *
 * RETURNS
 *  Success: S_OK. iface is initialised with the parameters given.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::Init(
    ASSOCF cfFlags,
    LPCWSTR pszAssoc,
    HKEY hkeyProgid,
    HWND hWnd)
{
    TRACE("(%p)->(%d,%s,%p,%p)\n", this,
                                    cfFlags,
                                    debugstr_w(pszAssoc),
                                    hkeyProgid,
                                    hWnd);

    enum { HANDLEDINITFLAGS = ASSOCF_INIT_BYEXENAME | ASSOCF_INIT_NOREMAPCLSID };
    HRESULT hr;
    Reset();
    m_InitFlags = cfFlags;

    if (hWnd != NULL)
        FIXME("hwnd != NULL not supported\n");

    if (cfFlags & ~HANDLEDINITFLAGS)
        FIXME("unsupported flags: %#x\n", cfFlags & ~HANDLEDINITFLAGS);

    if (pszAssoc != NULL)
    {
        WCHAR buf[sizeof("Applications") + MAX_PATH];
        m_InitType = *pszAssoc <= 0xff ? LOBYTE(*pszAssoc) : 'P';

        if (cfFlags & ASSOCF_INIT_BYEXENAME)
        {
            m_InitType = 'A';
            PCWSTR pszAppFile = PathFindFileNameW(pszAssoc);
            PCWSTR pszDotExt = *PathFindExtensionW(pszAppFile) ? L"" : L".exe";
            hr = StringCchPrintfW(buf, _countof(buf), L"%s\\%s%s", L"Applications", pszAppFile, pszDotExt);
            if (FAILED(hr))
                return hr;
            pszAssoc = buf;
        }
        else if (*pszAssoc == L'{')
        {
            if (!EmulateWin2000())
                cfFlags |= ASSOCF_INIT_NOREMAPCLSID;
        }
        else if (StrChrW(pszAssoc, L'\\'))
        {
            pszAssoc = PathFindExtensionW(pszAssoc);
            m_InitType = (BYTE)*pszAssoc;
        }

        if (!*pszAssoc)
            return E_INVALIDARG;

        if (m_InitType == '.')
        {
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pszAssoc, 0, KEY_READ, &this->hkeySource))
                goto done;
        }
        else if (m_InitType != '{' && m_InitType != 'A')
        {
            m_InitType = 'P'; // ProgId
        }

        OpenClassKey(cfFlags, pszAssoc, &this->hkeyProgID);
        if (!this->hkeySource)
            this->hkeySource = this->hkeyProgID;
done:
        return (!GetClassHandle() && IsNT5()) ? S_FALSE : S_OK;
    }
    else if (hkeyProgid != NULL)
    {
        /* reopen the key so we don't end up closing a key owned by the caller */
        RegOpenKeyExW(hkeyProgid, NULL, 0, KEY_READ, &this->hkeyProgID);
        this->hkeySource = this->hkeyProgID;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

/**************************************************************************
 *  IQueryAssociations_GetString
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  cfFlags  [I]   ASSOCF_ flags from "shlwapi.h"
 *  str      [I]   Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszExtra [I]   Extra information about the string location
 *  pszOut   [O]   Destination for the association string
 *  pcchOut  [I/O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetString(
    ASSOCF flags,
    ASSOCSTR str,
    LPCWSTR pszExtra,
    LPWSTR pszOut,
    DWORD *pcchOut)
{
    const ASSOCF unimplemented_flags = ~(ASSOCF_NOTRUNCATE | ASSOCF_IGNOREBASECLASS);
    DWORD len = 0;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    WCHAR path[MAX_PATH], *pszResult;

    TRACE("(%p)->(0x%08x, %u, %s, %p, %p)\n", this, flags, str, debugstr_w(pszExtra), pszOut, pcchOut);
    if (flags & unimplemented_flags)
    {
        FIXME("%08x: unimplemented flags\n", flags & unimplemented_flags);
    }

    if (!pcchOut)
        return E_UNEXPECTED;

    if (!GetClassHandle())
        goto trybase;

    switch (str)
    {
        case ASSOCSTR_COMMAND:
        {
            hr = this->GetCommand(pszExtra, &pszResult);
            if (SUCCEEDED(hr))
                return this->ReturnAndFreeString(flags, pszOut, pcchOut, pszResult);
            break;
        }
        case ASSOCSTR_EXECUTABLE:
        {
            hr = this->GetExecutable(pszExtra, path, MAX_PATH, &len);
            if (SUCCEEDED(hr))
                return this->ReturnString(flags, pszOut, pcchOut, path, ++len);
            break;
        }
        case ASSOCSTR_FRIENDLYDOCNAME:
        {
            WCHAR *pszFileType;
            // FIXME: Needs to check "FriendlyTypeName" first
            hr = this->GetValue(this->hkeySource, NULL, (void**)&pszFileType, NULL);
            if (FAILED(hr))
                goto trybase;
            DWORD size = 0;
            DWORD ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
            if (ret == ERROR_SUCCESS)
            {
                WCHAR *docName = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, size));
                if (docName)
                {
                    ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, docName, &size);
                    if (ret == ERROR_SUCCESS)
                    {
                        hr = this->ReturnString(flags, pszOut, pcchOut, docName, strlenW(docName) + 1);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ret);
                    }
                    HeapFree(GetProcessHeap(), 0, docName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ret);
            }
            HeapFree(GetProcessHeap(), 0, pszFileType);
            return hr;
        }
        case ASSOCSTR_FRIENDLYAPPNAME:
        {
            PVOID verinfoW = NULL;
            DWORD size, retval = 0;
            UINT flen;
            WCHAR *bufW;
            WCHAR fileDescW[41];

            hr = this->GetExecutable(pszExtra, path, MAX_PATH, &len);
            if (FAILED(hr))
            {
                return hr;
            }
            retval = GetFileVersionInfoSizeW(path, &size);
            if (!retval)
            {
                goto get_friendly_name_fail;
            }
            verinfoW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
            if (!verinfoW)
            {
                return E_OUTOFMEMORY;
            }
            if (!GetFileVersionInfoW(path, 0, retval, verinfoW))
            {
                goto get_friendly_name_fail;
            }
            if (VerQueryValueW(verinfoW, L"\\VarFileInfo\\Translation", (LPVOID *)&bufW, &flen))
            {
                UINT i;
                DWORD *langCodeDesc = (DWORD *)bufW;
                for (i = 0; i < flen / sizeof(DWORD); i++)
                {
                    sprintfW(fileDescW, L"\\StringFileInfo\\%04x%04x\\FileDescription",
                             LOWORD(langCodeDesc[i]), HIWORD(langCodeDesc[i]));
                    if (VerQueryValueW(verinfoW, fileDescW, (LPVOID *)&bufW, &flen))
                    {
                        /* Does strlenW(bufW) == 0 mean we use the filename? */
                        len = strlenW(bufW) + 1;
                        TRACE("found FileDescription: %s\n", debugstr_w(bufW));
                        hr = this->ReturnString(flags, pszOut, pcchOut, bufW, len);
                        HeapFree(GetProcessHeap(), 0, verinfoW);
                        return hr;
                    }
                }
            }
        get_friendly_name_fail:
            PathRemoveExtensionW(path);
            PathStripPathW(path);
            TRACE("using filename: %s\n", debugstr_w(path));
            hr = this->ReturnString(flags, pszOut, pcchOut, path, strlenW(path) + 1);
            HeapFree(GetProcessHeap(), 0, verinfoW);
            return hr;
        }
        case ASSOCSTR_CONTENTTYPE:
        {
            hr = SHELL_RegGetStringAlloc(this->hkeySource, NULL, L"Content Type", pszResult);
            if (SUCCEEDED(hr))
                return ReturnAndFreeString(flags, pszOut, pcchOut, pszResult, hr / sizeof(WCHAR));
            break;
        }
        case ASSOCSTR_DEFAULTICON:
        {
            for (HKEY hKey = GetClassHandle();; hKey = this->hkeySource)  // ProgId or .ext
            {
                hr = SHELL_RegGetStringAlloc(hKey, L"DefaultIcon", NULL, pszResult);
                if (SUCCEEDED(hr))
                    return ReturnAndFreeString(flags, pszOut, pcchOut, pszResult, hr / sizeof(WCHAR));
                else if (hKey == this->hkeySource)
                    break;
            }
            break;
        }
        case ASSOCSTR_INFOTIP:
        {
            hr = SHELL_RegGetStringAlloc(GetClassHandle(), NULL, L"InfoTip", pszResult);
            if (SUCCEEDED(hr))
                return ReturnAndFreeString(flags, pszOut, pcchOut, pszResult, hr / sizeof(WCHAR));
            // TODO: On Vista+, if not even the base class nor "*" provides a tip, it returns a default?
            break;
        }
        case ASSOCSTR_SHELLEXTENSION:
        {
            WCHAR keypath[ARRAY_SIZE(L"ShellEx\\") + 39], guid[39];
            CLSID clsid;
            HKEY hkey;

            hr = CLSIDFromString(pszExtra, &clsid);
            if (FAILED(hr))
            {
                return hr;
            }
            strcpyW(keypath, L"ShellEx\\");
            strcatW(keypath, pszExtra);
            LONG ret = RegOpenKeyExW(this->hkeySource, keypath, 0, KEY_READ, &hkey);
            if (ret)
            {
                return HRESULT_FROM_WIN32(ret);
            }
            DWORD size = sizeof(guid);
            ret = RegGetValueW(hkey, NULL, NULL, RRF_RT_REG_SZ, NULL, guid, &size);
            RegCloseKey(hkey);
            if (ret)
            {
                return HRESULT_FROM_WIN32(ret);
            }
            return this->ReturnString(flags, pszOut, pcchOut, guid, size / sizeof(WCHAR));
        }

        default:
        {
            FIXME("assocstr %d unimplemented!\n", str);
            return E_NOTIMPL;
        }
    }
trybase:
    if (FAILED(hr) && TryBaseClass(flags))
        hr = m_pBaseClass->GetString(flags, str, pszExtra, pszOut, pcchOut);
    return hr;
}

/**************************************************************************
 *  IQueryAssociations_GetKey
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get (ASSOCKEY enum from "shlwapi.h")
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains a handle to the key.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetKey(
    ASSOCF cfFlags,
    ASSOCKEY assockey,
    LPCWSTR pszExtra,
    HKEY *phkeyOut)
{
    HRESULT hr = E_INVALIDARG;
    HKEY hClass = GetClassHandle();
    const CLSID *pAssocElement = NULL;
    ASSOCSTR strInit = (ASSOCSTR)0;
    WCHAR buf[MAX_PATH];
    PCWSTR pszSetString = NULL;
    *phkeyOut = NULL;  // Note: Windows does not check if the pointer is valid

    switch (assockey)
    {
        case ASSOCKEY_SHELLEXECCLASS:
            cfFlags |= ASSOCF_VERIFY;
            if (hClass && SUCCEEDED(hr = OpenClassWithShellExecKey(cfFlags, hClass, pszExtra, phkeyOut)))
                return hr;
            hr = HResultFromWin32(IsNT5() ? (hClass ? ERROR_FILE_NOT_FOUND : E_FAIL) : ERROR_NO_ASSOCIATION);
            break;
        case ASSOCKEY_APP:
            if (m_InitFlags & ASSOCF_INIT_BYEXENAME)
            {
                pszExtra = NULL; // Always ignored
                return DuplicateHKey(this->hkeySource, phkeyOut);
            }
            pAssocElement = &CLSID_AssocApplicationElement;
            strInit = ASSOCSTR_EXECUTABLE;
            hr = IsNT5() ? E_FAIL : HResultFromWin32(ERROR_NO_ASSOCIATION);
            break;
        case ASSOCKEY_CLASS: // "A ProgID or class key"
            if (hClass)
                return DuplicateHKey(hClass, phkeyOut);
            hr = IsNT5() ? E_FAIL : HResultFromWin32(ERROR_NOT_FOUND);
            break;
        case ASSOCKEY_BASECLASS:
            assockey = ASSOCKEY_CLASS;
            break;
        case ASSOCKEY_MAX:
            // fallthrough
        default:
            FIXME("(%p,0x%8x,0x%8x,%s,%p)\n", this, cfFlags, assockey, debugstr_w(pszExtra), phkeyOut);
    }

    if (strInit)
    {
        DWORD cch = _countof(buf);
        if (SUCCEEDED(GetString(ASSOCF_NOFIXUPS | ASSOCF_NOTRUNCATE | ASSOCF_INIT_IGNOREUNKNOWN, strInit, pszExtra, buf, &cch)))
            pszSetString = buf;
    }
    if (pAssocElement && pszSetString)
    {
        CComPtr<IPersistString2> pPS2;
        if (FAILED(SH32_AssocCreateElement(*pAssocElement, IID_PPV_ARG(IPersistString2, &pPS2))))
            return hr;
        if (FAILED(pPS2->SetString(pszSetString)))
            return hr;
        hr = GetKeyFromObjectWithQuerySource(*pPS2, phkeyOut);
    }

    if (FAILED(hr) && TryBaseClass(cfFlags))
        hr = m_pBaseClass->GetKey(cfFlags, assockey, pszExtra, phkeyOut);
    return hr;
}

/**************************************************************************
 *  IQueryAssociations_GetData
 *
 * Get the data for a file association key from the registry.
 *
 * PARAMS
 *  cfFlags   [I]   ASSOCF_ flags from "shlwapi.h"
 *  assocdata [I]   Type of data to get (ASSOCDATA enum from "shlwapi.h")
 *  pszExtra  [I]   Extra information about the data location
 *  pvOut     [O]   Destination for the association key
 *  pcbOut    [I/O] Size of pvOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the data, pcbOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetData(ASSOCF cfFlags, ASSOCDATA assocdata, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut)
{
    TRACE("(%p,0x%8x,0x%8x,%s,%p,%p)\n", this, cfFlags, assocdata,
            debugstr_w(pszExtra), pvOut, pcbOut);

    enum { HANDLEDFLAGS = ASSOCF_IGNOREBASECLASS };
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    HKEY hClass = GetClassHandle();
    RegKeyRef rkr;

    if (cfFlags & ~HANDLEDFLAGS)
        FIXME("Unsupported flags: %x\n", cfFlags & ~HANDLEDFLAGS);

    switch(assocdata)
    {
        case ASSOCDATA_NOACTIVATEHANDLER:
            if (SUCCEEDED(OpenDdeKey(cfFlags, pszExtra, rkr)))
                hr = ReturnRegData(cfFlags, rkr, L"NoActivateHandler", pvOut, pcbOut);
            break;
        case ASSOCDATA_QUERYCLASSSTORE:
            if (hClass)
                hr = ReturnRegData(cfFlags, hClass, L"QueryClassStore", pvOut, pcbOut);
            break;
        case ASSOCDATA_EDITFLAGS:
            if (hClass)
                hr = ReturnRegData(cfFlags, hClass, L"EditFlags", pvOut, pcbOut);
            break;
        case ASSOCDATA_VALUE:
            if (hClass)
                hr = ReturnRegData(cfFlags, hClass, pszExtra, pvOut, pcbOut);
            break;
        default:
        {
            FIXME("Unsupported ASSOCDATA value: %d\n", assocdata);
            return E_NOTIMPL;
        }
    }
    if (FAILED(hr) && TryBaseClass(cfFlags))
        hr = m_pBaseClass->GetData(cfFlags, assocdata, pszExtra, pvOut, pcbOut);
    return hr;
}

HRESULT CQueryAssociations::ReturnRegData(ASSOCF flags, HKEY hKey, PCWSTR pszName, void *pvOut, DWORD *pcbOut)
{
    if (!pcbOut) // Asking if the data exists without returning it
        return RegValueExists(hKey, pszName) ? S_OK : S_FALSE;

    void *pData;
    DWORD cbData;
    HRESULT hr = this->GetValue(hKey, pszName, &pData, &cbData);
    if (FAILED(hr))
        return hr;
    hr = this->ReturnData(pvOut, pcbOut, pData, cbData);
    HeapFree(GetProcessHeap(), 0, pData);
    return hr;
}

/**************************************************************************
 *  IQueryAssociations_GetEnum
 *
 * Not yet implemented in native Win32.
 *
 * PARAMS
 *  cfFlags   [I] ASSOCF_ flags from "shlwapi.h"
 *  assocenum [I] Type of enum to get (ASSOCENUM enum from "shlwapi.h")
 *  pszExtra  [I] Extra information about the enum location
 *  riid      [I] REFIID to look for
 *  ppvOut    [O] Destination for the interface.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Presumably this function returns an enumerator object.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetEnum(
    ASSOCF cfFlags,
    ASSOCENUM assocenum,
    LPCWSTR pszExtra,
    REFIID riid,
    LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CQueryAssociations::GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size)
{
    DWORD size;
    LONG ret;

    ret = SHQueryValueExW(hkey, name, 0, NULL, NULL, &size);
    if (ret != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(ret);

    if (!size)
        return E_FAIL;

    *data = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*data)
        return E_OUTOFMEMORY;

    ret = SHQueryValueExW(hkey, name, 0, NULL, (LPBYTE)*data, &size);
    if (ret != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *data);
        return HRESULT_FROM_WIN32(ret);
    }

    if (data_size)
        *data_size = size;

    return S_OK;
}

HRESULT CQueryAssociations::GetCommand(const WCHAR *extra, WCHAR **command)
{
    HKEY hClass = GetClassHandle();
    if (!hClass)
        return E_UNEXPECTED;
    return SH32_QueryAssocElementString(CLSID_AssocProgidElement, ASSOCQUERY_COMMAND, hClass, NULL, extra, command);
}

HRESULT CQueryAssociations::GetExecutable(LPCWSTR pszExtra, LPWSTR path, DWORD pathlen, DWORD *len)
{
    PWSTR pszCommand, pszStart, pszEnd, pszBuf, pszFree;
    HRESULT hr = this->GetCommand(pszExtra, &pszCommand);
    if (FAILED(hr))
        return hr;

    DWORD expLen = ExpandEnvironmentStringsW(pszCommand, NULL, 0);
    if (expLen > 0)
    {
        pszBuf = (PWSTR)SHAlloc(++expLen * sizeof(WCHAR));
        if (!pszBuf)
        {
            SHFree(pszCommand);
            return E_OUTOFMEMORY;
        }
        ExpandEnvironmentStringsW(pszCommand, pszBuf, expLen);
        SHFree(pszCommand);
        pszCommand = pszBuf;
    }
    pszFree = pszCommand;

    // ASSOCF_REMAPRUNDLL implied for ASSOCSTR_EXECUTABLE
    pszEnd = PathGetArgsW(pszCommand);
    WCHAR save = *pszEnd;
    *pszEnd = UNICODE_NULL;
    pszStart = PathFindFileNameW(pszCommand + (*pszCommand == '"'));
    *pszEnd = save;
    if (!StrCmpNIW(pszStart, L"rundll", _countof(L"rundll") - 1))
    {
        pszCommand = pszEnd;
        if ((pszEnd = StrChrW(pszCommand, ',')) != NULL)
            *pszEnd = UNICODE_NULL; // Remove comma and function name
    }

    /* cleanup pszCommand */
    if (pszCommand[0] == '"')
    {
        pszStart = pszCommand + 1;
        pszEnd = strchrW(pszStart, '"');
        if (pszEnd)
        {
            *pszEnd = 0;
        }
        *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
    }
    else
    {
        pszStart = pszCommand;
        for (pszEnd = pszStart; (pszEnd = strchrW(pszEnd, ' ')); pszEnd++)
        {
            WCHAR c = *pszEnd;
            *pszEnd = 0;
            if ((*len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL)))
            {
                break;
            }
            *pszEnd = c;
        }
        if (!pszEnd)
        {
            *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
        }
    }

    SHFree(pszFree);
    if (!*len)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    return S_OK;
}

HRESULT CQueryAssociations::ReturnData(void *out, DWORD *outlen, const void *data, DWORD datalen)
{
    if (out)
    {
        DWORD lenNoReturn = PtrToUlong(outlen);
        if (IS_INTRESOURCE(outlen) && IsNT5())
            outlen = &lenNoReturn;

        if (*outlen < datalen)
        {
            *outlen = datalen;
            return E_POINTER;
        }
        *outlen = datalen;
        memcpy(out, data, datalen);
        return S_OK;
    }
    else
    {
        *outlen = datalen;
        return S_FALSE;
    }
}

HRESULT CQueryAssociations::ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen)
{
    HRESULT hr = S_OK;
    DWORD len;

    TRACE("flags=0x%08x, data=%s\n", flags, debugstr_w(data));

    if (!out)
    {
        *outlen = datalen;
        return S_FALSE;
    }

    DWORD lenNoReturn = PtrToUlong(outlen);
    if (IS_INTRESOURCE(outlen) && IsNT5())
        outlen = &lenNoReturn;

    if (*outlen < datalen)
    {
        if (flags & ASSOCF_NOTRUNCATE)
        {
            len = 0;
            if (*outlen > 0) out[0] = 0;
            hr = E_POINTER;
        }
        else
        {
            len = min(*outlen, datalen);
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        *outlen = datalen;
    }
    else
    {
        *outlen = len = datalen;
    }

    if (len)
    {
        memcpy(out, data, len*sizeof(WCHAR));
    }

    return hr;
}
