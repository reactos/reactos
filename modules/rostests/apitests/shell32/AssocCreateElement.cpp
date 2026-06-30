/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for AssocCreateElement
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi_undoc.h>
#include <shellutils.h>
#include <versionhelpers.h>

#define ASSOCQUERY_DE    (ASSOCQUERY_DIRECT | ASSOCQUERY_EXISTS)
#define ASSOCQUERY_SDE   (ASSOCQUERY_STRING | ASSOCQUERY_DE)
#define ASSOCQUERY_SDED  (ASSOCQUERY_SDE | ASSOCQUERY_DWORD)
#define ASSOCQUERY_SDEI  (ASSOCQUERY_SDE | ASSOCQUERY_INDIRECT)
#define ASSOCQUERY_SDEEV (ASSOCQUERY_SDE | ASSOCQUERY_EXTRA_VERB)

typedef HRESULT (WINAPI *FN_AssocCreateElement)(REFCLSID, REFIID, PVOID*);

static FN_AssocCreateElement g_fnAssocCreateElement = NULL;
static BOOL g_bVistaPlus = FALSE;

static HRESULT MyAssocCreateElement(REFCLSID rclsid, REFIID riid, PVOID* ppvObj)
{
    if (g_fnAssocCreateElement)
        return g_fnAssocCreateElement(rclsid, riid, ppvObj);
    return AssocCreate(rclsid, riid, ppvObj);
}

struct CLASS_ENTRY
{
    const CLSID *pclsid;
    PCWSTR pszName;
};

static const CLASS_ENTRY g_Classes[] =
{
    { &CLSID_AssocShellElement,       L"AssocShellElement" },
    { &CLSID_AssocApplicationElement, L"AssocApplicationElement" },
    { &CLSID_AssocProgidElement,      L"AssocProgidElement" },
    { &CLSID_AssocClsidElement,       L"AssocClsidElement" },
    { &CLSID_AssocSystemElement,      L"AssocSystemElement" },
    { &CLSID_AssocFolderElement,      L"AssocFolderElement" },
    { &CLSID_AssocStarElement,        L"AssocStarElement" },
    { &CLSID_AssocPerceivedElement,   L"AssocPerceivedElement" },
    { &CLSID_AssocClientElement,      L"AssocClientElement" },
};

// Verify that CLASS_E_CLASSNOTAVAILABLE is returned when an unknown CLSID is passed
static void Test_InvalidClsid(void)
{
    IUnknown *pUnk = (IUnknown *)(LONG_PTR)0xdeadbeef;
    HRESULT hr = MyAssocCreateElement(CLSID_NULL, IID_PPV_ARG(IUnknown, &pUnk));
    ok_hex(hr, CLASS_E_CLASSNOTAVAILABLE);
    ok(pUnk == NULL, "pUnk should be NULL, got %p\n", pUnk);
}

// For all public CLSIDs, verify that:
// - instantiation succeeds;
// - IPersist / IPersistString2 / IObjectWithQuerySourceOld can be obtained; and
// - GetClassID returns the CLSID passed during instantiation.
static void Test_QueryInterfaceAndClassID(void)
{
    HRESULT hr;

    for (size_t i = 0; i < _countof(g_Classes); ++i)
    {
        const CLASS_ENTRY& entry = g_Classes[i];

        if (g_bVistaPlus)
        {
            IAssociationElement *pElement = NULL;
            hr = MyAssocCreateElement(*entry.pclsid,
                                      IID_PPV_ARG(IAssociationElement, &pElement));
            ok(hr == S_OK, "[%s] AssocCreateElement failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (!pElement)
                continue;

            IPersist *pPersist = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IPersist, &pPersist));
            ok(hr == S_OK, "[%s] QI(IPersist) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pPersist)
            {
                CLSID clsid;
                hr = pPersist->GetClassID(&clsid);
                ok(hr == S_OK, "[%s] GetClassID failed: 0x%08lX\n",
                   wine_dbgstr_w(entry.pszName), hr);
                ok(IsEqualCLSID(clsid, *entry.pclsid),
                   "[%s] GetClassID returned an unexpected CLSID\n", wine_dbgstr_w(entry.pszName));
                pPersist->Release();
            }

            IPersistString2 *pPS2 = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IPersistString2, &pPS2));
            ok(hr == S_OK, "[%s] QI(IPersistString2) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pPS2)
                pPS2->Release();

            IObjectWithQuerySourceOld *pOWQS = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IObjectWithQuerySourceOld, &pOWQS));
            ok(hr == S_OK, "[%s] QI(IObjectWithQuerySourceOld) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pOWQS)
                pOWQS->Release();

            pElement->Release();
        }
        else
        {
            IAssociationElementOld *pElement = NULL;
            hr = MyAssocCreateElement(*entry.pclsid,
                                      IID_PPV_ARG(IAssociationElementOld, &pElement));
            ok(hr == S_OK, "[%s] AssocCreateElement failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (!pElement)
                continue;

            IPersist *pPersist = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IPersist, &pPersist));
            ok(hr == S_OK, "[%s] QI(IPersist) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pPersist)
            {
                CLSID clsid;
                hr = pPersist->GetClassID(&clsid);
                ok(hr == S_OK, "[%s] GetClassID failed: 0x%08lX\n",
                   wine_dbgstr_w(entry.pszName), hr);
                ok(IsEqualCLSID(clsid, *entry.pclsid),
                   "[%s] GetClassID returned an unexpected CLSID\n", wine_dbgstr_w(entry.pszName));
                pPersist->Release();
            }

            IPersistString2 *pPS2 = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IPersistString2, &pPS2));
            ok(hr == S_OK, "[%s] QI(IPersistString2) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pPS2)
                pPS2->Release();

            IObjectWithQuerySourceOld *pOWQS = NULL;
            hr = pElement->QueryInterface(IID_PPV_ARG(IObjectWithQuerySourceOld, &pOWQS));
            ok(hr == S_OK, "[%s] QI(IObjectWithQuerySourceOld) failed: 0x%08lX\n",
               wine_dbgstr_w(entry.pszName), hr);
            if (pOWQS)
                pOWQS->Release();

            pElement->Release();
        }
    }
}

// Verify that calling QueryString before SetString results in E_INVALIDARG
static void Test_QueryBeforeInit(void)
{
    if (g_bVistaPlus)
    {
        IAssociationElement *pElement = NULL;
        HRESULT hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                          IID_PPV_ARG(IAssociationElement, &pElement));
        ok(hr == S_OK, "AssocCreateElement failed: 0x%08lX\n", hr);
        if (!pElement)
            return;

        PWSTR psz = NULL;
        _SEH2_TRY
        {
            hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = 0xDEAD;
        }
        _SEH2_END;

        ok(hr == 0xDEAD, "hr was 0x%08lX\n", hr);

        pElement->Release();
    }
    else
    {
        IAssociationElementOld *pElement = NULL;
        HRESULT hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                          IID_PPV_ARG(IAssociationElementOld, &pElement));
        ok(hr == S_OK, "AssocCreateElement failed: 0x%08lX\n", hr);
        if (!pElement)
            return;

        PWSTR psz = NULL;
        hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
        ok_hex(hr, E_INVALIDARG);
        ok(psz == NULL, "psz should be NULL\n");

        pElement->Release();
    }
}

// Verify that calling SetString twice results in E_UNEXPECTED
static void Test_DoubleSetString(void)
{
    IPersistString2 *pPS2 = NULL;
    HRESULT hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                      IID_PPV_ARG(IPersistString2, &pPS2));
    ok(hr == S_OK, "AssocCreateElement failed: 0x%08lX\n", hr);
    if (!pPS2)
        return;

    hr = pPS2->SetString(L"txtfile");
    ok(hr == S_OK, "First SetString failed: 0x%08lX\n", hr);

    hr = pPS2->SetString(L"txtfile");
    if (g_bVistaPlus)
        ok_hex(hr, S_OK);
    else
        ok_hex(hr, E_UNEXPECTED);

    pPS2->Release();
}

// Retrieve FriendlyTypeName / DefaultIcon using CLSID_AssocShellElement + SetString(L"txtfile")
static void Test_ShellElement_txtfile(void)
{
    IPersistString2 *pPS2 = NULL;
    HRESULT hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                      IID_PPV_ARG(IPersistString2, &pPS2));
    ok(hr == S_OK, "AssocCreateElement(AssocShellElement) failed: 0x%08lX\n", hr);
    if (!pPS2)
        return;

    hr = pPS2->SetString(L"txtfile");
    if (hr != S_OK)
    {
        skip("\"txtfile\" ProgID was not found in HKCR, skipping\n");
        pPS2->Release();
        return;
    }

    if (g_bVistaPlus)
    {
        IAssociationElement *pElement = NULL;
        hr = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElement, &pElement));
        ok(hr == S_OK, "QI(IAssociationElement) failed: 0x%08lX\n", hr);
        if (pElement)
        {
            PWSTR psz = NULL;
            hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr == S_OK, "QueryString(FriendlyTypeName) failed: 0x%08lX\n", hr);
            ok(psz != NULL && psz[0] != UNICODE_NULL, "Friendly type name is empty\n");
            if (psz)
            {
                trace("txtfile FriendlyTypeName: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }

            hr = pElement->QueryExists((ASSOCQUERY_SDE | 0x1), NULL);
            ok(hr == S_OK, "QueryExists(DefaultIcon) failed: 0x%08lX\n", hr);

            pElement->Release();
        }
    }
    else
    {
        IAssociationElementOld *pElement = NULL;
        hr = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElementOld, &pElement));
        ok(hr == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr);
        if (pElement)
        {
            PWSTR psz = NULL;
            hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr == S_OK, "QueryString(FriendlyTypeName) failed: 0x%08lX\n", hr);
            ok(psz != NULL && psz[0] != UNICODE_NULL, "Friendly type name is empty\n");
            if (psz)
            {
                trace("txtfile FriendlyTypeName: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }

            hr = pElement->QueryExists((ASSOCQUERY_SDE | 0x1), NULL);
            ok(hr == S_OK, "QueryExists(DefaultIcon) failed: 0x%08lX\n", hr);

            pElement->Release();
        }
    }

    pPS2->Release();
}


// A pattern where you create a custom registry source using QuerySourceCreateFromKey
// instead of SetString, and attach it directly via IObjectWithQuerySource[Old]::SetSource.
static void Test_ShellElement_ExplicitSource(void)
{
    HRESULT hr;
    if (g_bVistaPlus)
    {
        IQuerySource *pSource = NULL;
        hr = QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"txtfile", FALSE,
                                      IID_PPV_ARG(IQuerySource, &pSource));
        if (FAILED(hr))
        {
            skip("QuerySourceCreateFromKey(txtfile) failed: 0x%08lX\n", hr);
            return;
        }

        IObjectWithQuerySource *pOWQS = NULL;
        hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                  IID_PPV_ARG(IObjectWithQuerySource, &pOWQS));
        ok(hr == S_OK, "AssocCreateElement(AssocShellElement) failed: 0x%08lX\n", hr);
        if (pOWQS)
        {
            hr = pOWQS->SetSource(pSource);
            ok(hr == S_OK, "SetSource failed: 0x%08lX\n", hr);

            IAssociationElement *pElement = NULL;
            hr = pOWQS->QueryInterface(IID_PPV_ARG(IAssociationElement, &pElement));
            ok(hr == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr);
            if (pElement)
            {
                PWSTR psz = NULL;
                hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
                ok(hr == S_OK, "QueryString(FriendlyTypeName) failed: 0x%08lX\n", hr);
                if (psz)
                {
                    trace("txtfile (explicit source) FriendlyTypeName: %s\n",
                          wine_dbgstr_w(psz));
                    CoTaskMemFree(psz);
                }
                pElement->Release();
            }

            // The second call to SetSource should result in E_UNEXPECTED
            hr = pOWQS->SetSource(pSource);
            ok_hex(hr, E_UNEXPECTED);

            pOWQS->Release();
        }

        if (pSource)
            pSource->Release();
    }
    else
    {
        IQuerySourceOld *pSource = NULL;
        hr = QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"txtfile", FALSE,
                                              IID_PPV_ARG(IQuerySourceOld, &pSource));
        if (FAILED(hr))
        {
            skip("QuerySourceCreateFromKey(txtfile) failed: 0x%08lX\n", hr);
            return;
        }

        IObjectWithQuerySourceOld *pOWQS = NULL;
        hr = MyAssocCreateElement(CLSID_AssocShellElement,
                                  IID_PPV_ARG(IObjectWithQuerySourceOld, &pOWQS));
        ok(hr == S_OK, "AssocCreateElement(AssocShellElement) failed: 0x%08lX\n", hr);
        if (pOWQS)
        {
            hr = pOWQS->SetSource(pSource);
            ok(hr == S_OK, "SetSource failed: 0x%08lX\n", hr);

            IAssociationElementOld *pElement = NULL;
            hr = pOWQS->QueryInterface(IID_PPV_ARG(IAssociationElementOld, &pElement));
            ok(hr == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr);
            if (pElement)
            {
                PWSTR psz = NULL;
                hr = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
                ok(hr == S_OK, "QueryString(FriendlyTypeName) failed: 0x%08lX\n", hr);
                if (psz)
                {
                    trace("txtfile (explicit source) FriendlyTypeName: %s\n",
                          wine_dbgstr_w(psz));
                    CoTaskMemFree(psz);
                }
                pElement->Release();
            }

            // The second call to SetSource should result in E_UNEXPECTED
            hr = pOWQS->SetSource(pSource);
            ok_hex(hr, E_UNEXPECTED);

            pOWQS->Release();
        }

        if (pSource)
            pSource->Release();
    }
}

// Verify resolution from the file extension using CLSID_AssocProgidElement + SetString(L".txt")
static void Test_ProgidElement_DotTxt(void)
{
    IPersistString2 *pPS2 = NULL;
    HRESULT hr = MyAssocCreateElement(CLSID_AssocProgidElement,
                                      IID_PPV_ARG(IPersistString2, &pPS2));
    ok(hr == S_OK, "AssocCreateElement(AssocProgidElement) failed: 0x%08lX\n", hr);
    if (!pPS2)
        return;

    hr = pPS2->SetString(L".txt");
    ok(hr == S_OK || hr == S_FALSE, "SetString(.txt) failed: 0x%08lX\n", hr);

    if (g_bVistaPlus)
    {
        IAssociationElement *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElement, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElement) failed: 0x%08lX\n", hr2);
        if (pElement)
            pElement->Release();
    }
    else
    {
        IAssociationElementOld *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElementOld, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr2);
        if (pElement)
        {
            PWSTR psz = NULL;
            HRESULT hr3 = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr3 == S_OK, "QueryString(FriendlyTypeName) for .txt failed: 0x%08lX\n", hr3);
            if (psz)
            {
                trace(".txt FriendlyTypeName: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }
            pElement->Release();
        }
    }

    pPS2->Release();
}

// CLSID_AssocStarElement: Verify that the generic "File" name can be retrieved even for
//                         unknown file extensions
static void Test_StarElement_UnknownExt(void)
{
    IPersistString2 *pPS2 = NULL;
    HRESULT hr = MyAssocCreateElement(CLSID_AssocStarElement,
                                      IID_PPV_ARG(IPersistString2, &pPS2));
    ok(hr == S_OK, "AssocCreateElement(AssocStarElement) failed: 0x%08lX\n", hr);
    if (!pPS2)
        return;

    hr = pPS2->SetString(L".rostestunknownext");
    ok(hr == S_OK, "SetString failed: 0x%08lX\n", hr);

    if (g_bVistaPlus)
    {
        IAssociationElement *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElement, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElement) failed: 0x%08lX\n", hr2);
        if (pElement)
        {
            PWSTR psz = NULL;
            HRESULT hr3 = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr3 == S_OK, "QueryString for unknown extension failed: 0x%08lX\n", hr3);
            ok(psz != NULL && psz[0] != UNICODE_NULL, "Generic type name is empty\n");
            if (psz)
            {
                trace("Generic file type name: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }
            pElement->Release();
        }
    }
    else
    {
        IAssociationElementOld *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElementOld, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr2);
        if (pElement)
        {
            PWSTR psz = NULL;
            HRESULT hr3 = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr3 == S_OK, "QueryString for unknown extension failed: 0x%08lX\n", hr3);
            ok(psz != NULL && psz[0] != UNICODE_NULL, "Generic type name is empty\n");
            if (psz)
            {
                trace("Generic file type name: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }
            pElement->Release();
        }
    }

    pPS2->Release();
}

// CLSID_AssocFolderElement: Always opens HKCR\Folder
static void Test_FolderElement(void)
{
    IPersistString2 *pPS2 = NULL;
    HRESULT hr = MyAssocCreateElement(CLSID_AssocFolderElement,
                                      IID_PPV_ARG(IPersistString2, &pPS2));
    ok(hr == S_OK, "AssocCreateElement(AssocFolderElement) failed: 0x%08lX\n", hr);
    if (!pPS2)
        return;

    hr = pPS2->SetString(L"");
    if (g_bVistaPlus)
        ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), "SetString hr: 0x%08lX\n", hr);
    else
        ok(hr == S_OK, "SetString failed: 0x%08lX\n", hr);

    if (g_bVistaPlus)
    {
        IAssociationElement *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElement, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElement) failed: 0x%08lX\n", hr2);
        if (pElement)
        {
            PWSTR psz = NULL;
            HRESULT hr3 = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr3 == S_OK, "QueryString(Folder) failed: 0x%08lX\n", hr3);
            if (psz)
            {
                trace("Folder type name: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }
            pElement->Release();
        }
    }
    else
    {
        IAssociationElementOld *pElement = NULL;
        HRESULT hr2 = pPS2->QueryInterface(IID_PPV_ARG(IAssociationElementOld, &pElement));
        ok(hr2 == S_OK, "QI(IAssociationElementOld) failed: 0x%08lX\n", hr2);
        if (pElement)
        {
            PWSTR psz = NULL;
            HRESULT hr3 = pElement->QueryString(ASSOCQUERY_SDEI, NULL, &psz);
            ok(hr3 == S_OK, "QueryString(Folder) failed: 0x%08lX\n", hr3);
            if (psz)
            {
                trace("Folder type name: %s\n", wine_dbgstr_w(psz));
                CoTaskMemFree(psz);
            }
            pElement->Release();
        }
    }

    pPS2->Release();
}

START_TEST(AssocCreateElement)
{
    g_bVistaPlus = IsWindowsVistaOrGreater();

    HINSTANCE hShell32 = GetModuleHandleW(L"shell32.dll");
    g_fnAssocCreateElement =
        (FN_AssocCreateElement)GetProcAddress(hShell32, MAKEINTRESOURCEA(764));
    if (!g_fnAssocCreateElement)
        trace("shell32!AssocCreateElement not found. Using shlwapi!AssocCreate instead...\n");

    HRESULT hrCo = CoInitialize(NULL);

    Test_InvalidClsid();
    Test_QueryInterfaceAndClassID();
    Test_QueryBeforeInit();
    Test_DoubleSetString();
    Test_ShellElement_txtfile();
    Test_ShellElement_ExplicitSource();
    Test_ProgidElement_DotTxt();
    Test_StarElement_UnknownExt();
    Test_FolderElement();

    if (SUCCEEDED(hrCo))
        CoUninitialize();
}
