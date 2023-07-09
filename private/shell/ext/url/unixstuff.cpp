#include "project.hpp"

extern "C" void WINAPI NewsProtocolHandler(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);
extern "C" void WINAPI MailToProtocolHandler(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);
extern "C" void WINAPI TelnetProtocolHandler(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);
extern "C" void WINAPI FileProtocolHandler(HWND hwndPanent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);
extern "C" void WINAPI OpenURL(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);

STDAPI_(void) DummyEntryPoint(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow);

extern "C" void WINAPI NewsProtocolHandlerA(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd)
{
    NewsProtocolHandler(hwndParent, hinst, pszCmdLine, nShowCmd);
}

extern "C" void WINAPI MailToProtocolHandlerA(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd)
{
   MailToProtocolHandler(hwndParent, hinst, pszCmdLine, nShowCmd);
}

extern "C" void WINAPI TelnetProtocolHandlerA(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd)
{
   TelnetProtocolHandler(hwndParent, hinst, pszCmdLine, nShowCmd);
}

extern "C" void WINAPI FileProtocolHandlerA(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd)
{
   FileProtocolHandler(hwndParent, hinst, pszCmdLine, nShowCmd);
}

extern "C" void WINAPI OpenURLA(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd)
{
   OpenURL(hwndParent, hinst, pszCmdLine, nShowCmd);
}

STDAPI_(void) DummyEntryPointA(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   DummyEntryPoint(hwnd, hAppInstance, lpszCmdLine, nCmdShow);
}
