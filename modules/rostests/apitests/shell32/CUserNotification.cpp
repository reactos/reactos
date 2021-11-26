/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CUserNotification
 * COPYRIGHT:   Copyright 2018 Hermes Belusca-Maito
 */

#include "shelltest.h"

#define NDEBUG
#include <debug.h>

#define ok_hr(status, expected)     ok_hex(status, expected)

#define HRESULT_CANCELLED   HRESULT_FROM_WIN32(ERROR_CANCELLED)


/* An implementation of the IQueryContinue interface */
class CQueryContinue : public IQueryContinue
{
private:
    HRESULT m_hContinue;

public:
    CQueryContinue(HRESULT hContinue = S_OK) : m_hContinue(hContinue) {}
    ~CQueryContinue() {}

    CQueryContinue& operator=(const CQueryContinue& qc)
    {
        if (this != &qc)
            m_hContinue = qc.m_hContinue;
        return *this;
    }

    CQueryContinue& operator=(HRESULT hContinue)
    {
        m_hContinue = hContinue;
        return *this;
    }

    operator HRESULT()
    {
        return m_hContinue;
    }

public:
    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return 1;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        return 0;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
    {
        return S_OK;
    }

    // IQueryContinue
    virtual HRESULT STDMETHODCALLTYPE QueryContinue(void)
    {
        // TRACE("IQueryContinue::QueryContinue() returning 0x%lx\n", m_hContinue);
        return m_hContinue;
    }
};


static void
TestNotification(void)
{
    HRESULT hr;
    CComPtr<IUserNotification> pUserNotif;
    CQueryContinue queryContinue(S_OK);

    // hr = pUserNotif.CoCreateInstance(CLSID_UserNotification);
    hr = ::CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL,
                            /*IID_PPV_ARG(IUserNotification, &pUserNotif)*/
                            IID_IUserNotification, (void**)&pUserNotif);
    ok(hr == S_OK, "CoCreateInstance, hr = 0x%lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate IUserNotification\n");
        return;
    }

    /* Set an invalid icon for the notification icon */
    hr = pUserNotif->SetIconInfo((HICON)UlongToHandle(0xdeadbeef), L"Tooltip text");
    ok_hr(hr, S_OK);

#if 0
    /* Seting an invalid string would crash the application */
    hr = pUserNotif->SetIconInfo(NULL, (LPCWSTR)0xdeadbeef);
    ok_hr(hr, S_OK);
#endif

    /* Set a default icon for the notification icon */
    hr = pUserNotif->SetIconInfo(NULL, L"Tooltip text");
    ok_hr(hr, S_OK);

    /*
     * Since just displaying a notification icon without balloon hangs (expected),
     * for making this test automatable we instead just test balloon functionality
     * where timeouts can be programmed.
     */

    /* Set up a balloon associated to the notification icon */
    hr = pUserNotif->SetBalloonInfo(L"Balloon title", L"Balloon text", NIIF_ERROR);
    ok_hr(hr, S_OK);

    /*
     * Try to display twice the balloon if the user cancels it.
     * Without setting balloon retry, we would wait for a very long time...
     */
    hr = pUserNotif->SetBalloonRetry(2000, 1000, 2);
    ok_hr(hr, S_OK);

    /* Display the balloon and also the tooltip if one points on the icon */
    hr = pUserNotif->Show(NULL, 0);
    ok_hr(hr, HRESULT_CANCELLED);

    /*
     * Setting icon information *after* having enabled balloon info,
     * allows to automatically set the notification icon according
     * to the dwInfoFlags passed to SetBalloonInfo() and by giving
     * NULL to the hIcon parameter of SetIconInfo().
     */
    hr = pUserNotif->SetIconInfo(NULL, NULL);
    ok_hr(hr, S_OK);

    /* Display the balloon and also the tooltip if one points on the icon */
    hr = pUserNotif->Show(NULL, 0);
    ok_hr(hr, HRESULT_CANCELLED);

    /*
     * This line shows the balloon, but without title nor icon in it.
     * Note that the balloon icon is not displayed when not setting any title.
     */
    hr = pUserNotif->SetBalloonInfo(NULL, L"Balloon text", NIIF_WARNING);
    ok_hr(hr, S_OK);

    hr = pUserNotif->Show(NULL, 0);
    ok_hr(hr, HRESULT_CANCELLED);


    /* Test support of the IQueryContinue interface */

    hr = pUserNotif->SetBalloonInfo(L"Balloon title", L"Balloon text", NIIF_WARNING);
    ok_hr(hr, S_OK);

    hr = pUserNotif->Show(&queryContinue, 2000); /* Timeout of 2 seconds */
    ok_hr(hr, HRESULT_CANCELLED);

#if 0 // Commented because this test (the Show() call) is unreliable.
    /* Try to hide the balloon by setting an empty string (can use either NULL or L"") */
    hr = pUserNotif->SetBalloonInfo(L"Balloon title", NULL, NIIF_WARNING);
    ok_hr(hr, S_OK);

    hr = pUserNotif->Show(&queryContinue, 2000); /* Timeout of 2 seconds */
    ok_hr(hr, HRESULT_CANCELLED);
#endif

    hr = pUserNotif->SetBalloonInfo(L"Balloon title", L"Balloon text", NIIF_WARNING);
    ok_hr(hr, S_OK);

    queryContinue = S_FALSE;
    hr = pUserNotif->Show(&queryContinue, 2000); /* Timeout of 2 seconds */
    ok_hr(hr, S_FALSE);
}

DWORD
CALLBACK
TestThread(LPVOID lpParam)
{
    /* Initialize COM */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* Start the test */
    TestNotification();

    /* Cleanup and return */
    CoUninitialize();
    return 0;
}

START_TEST(CUserNotification)
{
    HANDLE hThread;
    DWORD dwWait;

    /* We create a test thread, because the notification tests can hang */
    hThread = CreateThread(NULL, 0, TestThread, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed with error 0x%lu\n", GetLastError());
    if (!hThread)
    {
        skip("Could not create the CUserNotification test thread!\n");
        return;
    }

    /* Wait a maximum of 60 seconds for the thread to finish (the timeout tests take some time) */
    dwWait = WaitForSingleObject(hThread, 60 * 1000);
    ok(dwWait == WAIT_OBJECT_0, "WaitForSingleObject returned %lu, expected WAIT_OBJECT_0\n", dwWait);

    /* Cleanup and return */
    CloseHandle(hThread);
}
