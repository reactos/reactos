/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Tests for IInitializeSpy
 * PROGRAMMERS:     Mark Jansen
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdio.h>
#include <wine/test.h>

#include <winuser.h>
#include <winreg.h>

#include <shlwapi.h>
#include <unknownbase.h>

#define test_S_OK(hres, message) ok((hres) == S_OK, "%s (0x%lx instead of S_OK)\n", (message), (hres))
#define test_HRES(hres, hresExpected, message) ok((hres) == (hresExpected), "%s (0x%lx instead of 0x%lx)\n", (message), (hres), (hresExpected))
#define test_ref(spy, expectedRef) ok((spy)->GetRef() == (expectedRef), "unexpected refcount, %ld instead of %d\n", (spy)->GetRef(), (expectedRef))


typedef HRESULT (WINAPI *pCoRegisterInitializeSpy_t)(_In_ LPINITIALIZESPY pSpy, _Out_ ULARGE_INTEGER *puliCookie);
typedef HRESULT (WINAPI *pCoRevokeInitializeSpy_t)(_In_ ULARGE_INTEGER uliCookie);
pCoRegisterInitializeSpy_t pCoRegisterInitializeSpy;
pCoRevokeInitializeSpy_t pCoRevokeInitializeSpy;


const DWORD INVALID_VALUE = 0xdeadbeef;


class CTestSpy : public CUnknownBase<IInitializeSpy>
{
public:
    HRESULT hr;
    ULARGE_INTEGER Cookie;

    // expected values to check against
    HRESULT m_hrCoInit;
    DWORD m_CoInit;
    DWORD m_CurAptRefs;

    // keeping count of the times called
    LONG m_PreInitCalled;
    LONG m_PostInitCalled;
    LONG m_PreUninitCalled;
    LONG m_PostUninitCalled;

    // fake out some
    bool m_FailQueryInterface;
    bool m_AlwaysReturnOK;

    CTestSpy()
        : CUnknownBase( false, 0 ),
        hr(0),
        m_hrCoInit(0),
        m_CoInit(0),
        m_CurAptRefs(0),
        m_FailQueryInterface(false),
        m_AlwaysReturnOK(false)
    {
        Cookie.HighPart = Cookie.LowPart = INVALID_VALUE;
        Clear();
    }

    ~CTestSpy()
    {
        // always try to revoke if we succeeded to register.
        if (SUCCEEDED(hr))
        {
            hr = pCoRevokeInitializeSpy(Cookie);
            test_S_OK(hr, "CoRevokeInitializeSpy");
        }
        // we should be done.
        ok(GetRef() == 0, "Expected m_lRef to be 0, was: %ld\n", GetRef());
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if (m_FailQueryInterface)
        {
            return E_NOINTERFACE;
        }
        return CUnknownBase::QueryInterface(riid, ppv);
    }

    const QITAB* GetQITab()
    {
        static const QITAB tab[] = { { &IID_IInitializeSpy, OFFSETOFCLASS(IInitializeSpy, CTestSpy) }, { 0 } };
        return tab;
    }


    HRESULT STDMETHODCALLTYPE PreInitialize(DWORD dwCoInit, DWORD dwCurThreadAptRefs)
    {
        InterlockedIncrement(&m_PreInitCalled);
        ok(m_CoInit == dwCoInit, "Unexpected dwCoInit: got %lx, expected %lx\n", dwCoInit, m_CoInit);
        DWORD expectApt = m_hrCoInit == RPC_E_CHANGED_MODE ? m_CurAptRefs : m_CurAptRefs -1;
        ok(expectApt == dwCurThreadAptRefs, "Unexpected dwCurThreadAptRefs: got %lx, expected %lx\n", dwCurThreadAptRefs, expectApt);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PostInitialize(HRESULT hrCoInit, DWORD dwCoInit, DWORD dwNewThreadAptRefs)
    {
        InterlockedIncrement(&m_PostInitCalled);
        ok(m_PreInitCalled == m_PostInitCalled, "Expected balanced pre/post: %ld / %ld\n", m_PreInitCalled, m_PostInitCalled);
        test_HRES(hrCoInit, m_hrCoInit, "Unexpected hrCoInit in PostInitialize");
        ok(m_CoInit == dwCoInit, "Unexpected dwCoInit: got %lx, expected %lx\n", dwCoInit, m_CoInit);
        ok(m_CurAptRefs == dwNewThreadAptRefs, "Unexpected dwNewThreadAptRefs: got %lx, expected %lx\n", dwNewThreadAptRefs, m_CurAptRefs);
        if (m_AlwaysReturnOK)
            return S_OK;
        return hrCoInit;
    }

    HRESULT STDMETHODCALLTYPE PreUninitialize(DWORD dwCurThreadAptRefs)
    {
        InterlockedIncrement(&m_PreUninitCalled);
        ok(m_CurAptRefs == dwCurThreadAptRefs, "Unexpected dwCurThreadAptRefs: got %lx, expected %lx\n", dwCurThreadAptRefs, m_CurAptRefs);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PostUninitialize(DWORD dwNewThreadAptRefs)
    {
        InterlockedIncrement(&m_PostUninitCalled);
        ok(m_PreUninitCalled == m_PostUninitCalled, "Expected balanced pre/post: %ld / %ld\n", m_PreUninitCalled, m_PostUninitCalled);
        DWORD apt = m_CurAptRefs ? (m_CurAptRefs-1) : 0;
        ok(apt == dwNewThreadAptRefs, "Unexpected dwNewThreadAptRefs: got %lx, expected %lx\n", dwNewThreadAptRefs, apt);
        return S_OK;
    }

    void Clear()
    {
        m_PreInitCalled = 0;
        m_PostInitCalled = 0;
        m_PreUninitCalled = 0;
        m_PostUninitCalled = 0;
    }

    void Expect(HRESULT hrCoInit, DWORD CoInit, DWORD CurAptRefs)
    {
        m_hrCoInit = hrCoInit;
        m_CoInit = CoInit;
        m_CurAptRefs = CurAptRefs;
    }

    void Check(LONG PreInit, LONG PostInit, LONG PreUninit, LONG PostUninit)
    {
        ok(m_PreInitCalled == PreInit, "Expected PreInit to be %ld, was: %ld\n", PreInit, m_PreInitCalled);
        ok(m_PostInitCalled == PostInit, "Expected PostInit to be %ld, was: %ld\n", PostInit, m_PostInitCalled);
        ok(m_PreUninitCalled == PreUninit, "Expected PreUninit to be %ld, was: %ld\n", PreUninit, m_PreUninitCalled);
        ok(m_PostUninitCalled == PostUninit, "Expected PostUninit to be %ld, was: %ld\n", PostUninit, m_PostUninitCalled);
    }
};


void test_IInitializeSpy_register2()
{
    CTestSpy spy, spy2;

    // first we register 2 spies
    spy.hr = pCoRegisterInitializeSpy(&spy, &spy.Cookie);
    test_S_OK(spy.hr, "CoRegisterInitializeSpy");
    test_ref(&spy, 1);

    spy2.hr = pCoRegisterInitializeSpy(&spy2, &spy2.Cookie);
    test_S_OK(spy2.hr, "CoRegisterInitializeSpy");
    test_ref(&spy, 1);

    // tell them what we expect
    spy.Expect(S_OK, COINIT_APARTMENTTHREADED, 1);
    spy2.Expect(S_OK, COINIT_APARTMENTTHREADED, 1);

    // Call CoInitializeEx and validate the results
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    test_S_OK(hr, "CoInitializeEx");
    spy.Check(1, 1, 0, 0);
    spy2.Check(1, 1, 0, 0);

    // Calling CoInit twice with the same apartment makes it return S_FALSE but still increment count
    spy.Expect(S_FALSE, COINIT_APARTMENTTHREADED, 2);
    spy2.Expect(S_FALSE, COINIT_APARTMENTTHREADED, 2);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    test_HRES(hr, S_FALSE, "CoInitializeEx");
    spy.Check(2, 2, 0, 0);
    spy2.Check(2, 2, 0, 0);

    /* the order we registered the spies in is important here.
        we have the second one to forcibly return S_OK, which makes the first spy see
        S_OK instead of S_FALSE.. */
    spy.Expect(S_OK, COINIT_APARTMENTTHREADED, 3);
    spy2.m_AlwaysReturnOK = true;
    spy2.Expect(S_FALSE, COINIT_APARTMENTTHREADED, 3);

    // and the S_OK also influences the returned value from CoInit.
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    test_S_OK(hr, "CoInitializeEx");
    spy.Check(3, 3, 0, 0);
    spy2.Check(3, 3, 0, 0);

    CoUninitialize();
    spy.Check(3, 3, 1, 1);
    spy2.Check(3, 3, 1, 1);

    spy.m_CurAptRefs = spy2.m_CurAptRefs = 2;

    CoUninitialize();
    spy.Check(3, 3, 2, 2);
    spy2.Check(3, 3, 2, 2);

    spy.m_CurAptRefs = spy2.m_CurAptRefs = 1;

    CoUninitialize();
    spy.Check(3, 3, 3, 3);
    spy2.Check(3, 3, 3, 3);

    spy.m_CurAptRefs = spy2.m_CurAptRefs = 0;

    CoUninitialize();
    spy.Check(3, 3, 4, 4);
    spy2.Check(3, 3, 4, 4);
}

void test_IInitializeSpy_switch_apt()
{
    CTestSpy spy;

    spy.hr = pCoRegisterInitializeSpy(&spy, &spy.Cookie);
    test_S_OK(spy.hr, "CoRegisterInitializeSpy");
    test_ref(&spy, 1);

    spy.Expect(S_OK, COINIT_APARTMENTTHREADED, 1);

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    test_S_OK(hr, "CoInitializeEx");
    spy.Check(1, 1, 0, 0);

    spy.Expect(RPC_E_CHANGED_MODE, COINIT_MULTITHREADED, 1);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    test_HRES(hr, RPC_E_CHANGED_MODE, "CoInitializeEx");
    spy.Check(2, 2, 0, 0);


    CoUninitialize();
    spy.Check(2, 2, 1, 1);

    spy.m_CurAptRefs = 0;

    CoUninitialize();
    spy.Check(2, 2, 2, 2);

    CoUninitialize();
    spy.Check(2, 2, 3, 3);
}

void test_IInitializeSpy_fail()
{
    CTestSpy spy;

    spy.m_FailQueryInterface = true;

    spy.hr = pCoRegisterInitializeSpy(&spy, &spy.Cookie);
    test_HRES(spy.hr, E_NOINTERFACE, "Unexpected hr while registering invalid interface");
    test_ref(&spy, 0);
    ok(spy.Cookie.HighPart == 0xffffffff, "Unexpected Cookie.HighPart, expected 0xffffffff got: 0x%08lx\n", spy.Cookie.HighPart);
    ok(spy.Cookie.LowPart == 0xffffffff, "Unexpected Cookie.HighPart, expected 0xffffffff got: 0x%08lx\n", spy.Cookie.LowPart);

    spy.Cookie.HighPart = spy.Cookie.LowPart = 0xffffffff;
    HRESULT hr = pCoRevokeInitializeSpy(spy.Cookie);
    test_HRES(hr, E_INVALIDARG, "Unexpected hr while unregistering invalid interface");
    test_ref(&spy, 0);

    spy.Cookie.HighPart = spy.Cookie.LowPart = 0;
    hr = pCoRevokeInitializeSpy(spy.Cookie);
    test_HRES(hr, E_INVALIDARG, "Unexpected hr while unregistering invalid interface");
    test_ref(&spy, 0);

    /* we should not crash here, just return E_NOINTERFACE
        do note the Cookie is not even being touched at all, compared to calling this with an interface
        that does not respond to IID_IInitializeSpy */
    spy.Cookie.HighPart = spy.Cookie.LowPart = INVALID_VALUE;
    hr = pCoRegisterInitializeSpy(NULL, &spy.Cookie);
    test_HRES(spy.hr, E_NOINTERFACE, "Unexpected hr while registering NULL interface");
    ok(spy.Cookie.HighPart == INVALID_VALUE, "Unexpected Cookie.HighPart, expected 0xdeadbeef got: %lx\n", spy.Cookie.HighPart);
    ok(spy.Cookie.LowPart == INVALID_VALUE, "Unexpected Cookie.HighPart, expected 0xdeadbeef got: %lx\n", spy.Cookie.LowPart);
}

void test_IInitializeSpy_twice()
{
    CTestSpy spy;

    spy.hr = pCoRegisterInitializeSpy(&spy, &spy.Cookie);
    test_S_OK(spy.hr, "CoRegisterInitializeSpy");
    test_ref(&spy, 1);

    ULARGE_INTEGER Cookie = { { INVALID_VALUE, INVALID_VALUE } };
    HRESULT hr = pCoRegisterInitializeSpy(&spy, &Cookie);
    test_S_OK(hr, "CoRegisterInitializeSpy");
    test_ref(&spy, 2);

    hr = pCoRevokeInitializeSpy(Cookie);
    test_S_OK(hr, "CoRevokeInitializeSpy");
    test_ref(&spy, 1);
}


START_TEST(initializespy)
{
    HMODULE ole32 = LoadLibraryA("ole32.dll");
    pCoRegisterInitializeSpy = (pCoRegisterInitializeSpy_t)GetProcAddress(ole32, "CoRegisterInitializeSpy");
    pCoRevokeInitializeSpy = (pCoRevokeInitializeSpy_t)GetProcAddress(ole32, "CoRevokeInitializeSpy");

    test_IInitializeSpy_register2();
    test_IInitializeSpy_switch_apt();
    test_IInitializeSpy_fail();
    test_IInitializeSpy_twice();
}
