//
// test code for chanmgr
//
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <ole2.h>
#include <chanmgr.h>

//
// Macros
//
#define ASSERT(x)   if(!(x)) printf("ASSERT:line %d: %s", __line__, ##x);
#define CHECK(x)    printf("%s %s\n", #x, (SUCCEEDED((x)) || (x) == S_FALSE) ? "SUCCEEDED" : "FAILED" );

int _cdecl main()
{
    HRESULT hr;

    hr = CoInitialize(NULL);

    IChannelMgr *pChannelMgr = NULL;
    hr = CoCreateInstance(CLSID_ChannelMgr, NULL,  CLSCTX_INPROC_SERVER, 
            IID_IChannelMgr, (void**)&pChannelMgr);

    if (SUCCEEDED(hr))
    {
        {
            //
            // Add the sports category
            //
            CHANNELCATEGORYINFO cci = {0};
            cci.cbSize = sizeof(cci);
            cci.pszURL   = L"http://www.jiggins.com";
            cci.pszTitle = L"Sports";
            cci.pszLogo  = L"\\\\ohserv\\users\\julianj\\sports.gif";
            cci.pszIcon  = L"\\\\ohserv\\users\\julianj\\sports.ico";

            CHANNELSHORTCUTINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = L"http://www.espn.com";
            csi.pszTitle = L"Sports\\Espn";
            csi.pszLogo  = L"\\\\ohserv\\users\\julianj\\news.gif";
            csi.pszIcon  = L"\\\\ohserv\\users\\julianj\\news.ico";

            CHECK(pChannelMgr->DeleteChannelShortcut(csi.pszTitle));
            CHECK(pChannelMgr->DeleteCategory(cci.pszTitle));

            CHECK(pChannelMgr->AddCategory(&cci));
            CHECK(pChannelMgr->AddChannelShortcut(&csi));
        }

        {
            //
            // Add the News category
            //
            CHANNELCATEGORYINFO cci = {0};
            cci.cbSize = sizeof(cci);
            cci.pszURL   = L"http://www.jiggins.com";
            cci.pszTitle = L"Recent News";
            cci.pszLogo  = L"\\\\ohserv\\users\\julianj\\sports.gif";
            cci.pszIcon  = L"\\\\ohserv\\users\\julianj\\sports.ico";

            CHANNELSHORTCUTINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = L"http://www.espn.com";
            csi.pszTitle = L"Recent News\\New York Times";
            csi.pszLogo  = L"\\\\ohserv\\users\\julianj\\news.gif";
            csi.pszIcon  = L"\\\\ohserv\\users\\julianj\\news.ico";


            CHANNELSHORTCUTINFO csi2 = {0};
            csi2.cbSize   = sizeof(csi2);
            csi2.pszURL   = L"http://www.espn.com";
            csi2.pszTitle = L"Recent News\\Seattle Times";
            csi2.pszLogo  = L"\\\\ohserv\\users\\julianj\\news.gif";
            csi2.pszIcon  = L"\\\\ohserv\\users\\julianj\\news.ico";

            CHECK(pChannelMgr->DeleteChannelShortcut(csi.pszTitle));
            CHECK(pChannelMgr->DeleteChannelShortcut(csi2.pszTitle));
            CHECK(pChannelMgr->DeleteCategory(cci.pszTitle));

            CHECK(pChannelMgr->AddCategory(&cci));
            CHECK(pChannelMgr->AddChannelShortcut(&csi));
            CHECK(pChannelMgr->AddChannelShortcut(&csi2));
        }

        {
            // Add the test category
            CHANNELCATEGORYINFO cci = {0};
            cci.cbSize = sizeof(cci);
            cci.pszURL   = L"http://www.jiggins.com";
            cci.pszTitle = L"Test Category -/:*?\"<>|";
            cci.pszLogo  = L"\\\\ohserv\\users\\julianj\\sports.gif";
            cci.pszIcon  = L"\\\\ohserv\\users\\julianj\\sports.ico";

            CHANNELSHORTCUTINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = L"http://www.espn.com";
            csi.pszTitle = L"Test Category -/:*?\"<>|\\Test Shortcut 1 -/:*?\"<>|";
            csi.pszLogo  = L"\\\\ohserv\\users\\julianj\\news.gif";
            csi.pszIcon  = L"\\\\ohserv\\users\\julianj\\news.ico";

            CHECK(pChannelMgr->DeleteChannelShortcut(csi.pszTitle));
            CHECK(pChannelMgr->DeleteCategory(cci.pszTitle));

            CHECK(pChannelMgr->AddCategory(&cci));
            CHECK(pChannelMgr->AddChannelShortcut(&csi));
        }


        {
            // Add another test category
            CHANNELCATEGORYINFO cci = {0};
            cci.cbSize = sizeof(cci);
            cci.pszURL   = L"http://www.jiggins.com";
            cci.pszTitle = L"Test Category -/:*?\"<>|\\Tests";
            cci.pszLogo  = L"\\\\ohserv\\users\\julianj\\sports.gif";
            cci.pszIcon  = L"\\\\ohserv\\users\\julianj\\sports.ico";

            CHANNELSHORTCUTINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = L"http://www.espn.com";
            csi.pszTitle = L"Test Category -/:*?\"<>|\\Tests\\Test Shortcut 2 -/:*?\"<>|";
            csi.pszLogo  = L"\\\\ohserv\\users\\julianj\\news.gif";
            csi.pszIcon  = L"\\\\ohserv\\users\\julianj\\news.ico";

            CHECK(pChannelMgr->DeleteChannelShortcut(csi.pszTitle));
            CHECK(pChannelMgr->DeleteCategory(cci.pszTitle));

            CHECK(pChannelMgr->AddCategory(&cci));
            CHECK(pChannelMgr->AddChannelShortcut(&csi));
        }

        pChannelMgr->Release();
    }

    CoUninitialize();
    return 0;
}
