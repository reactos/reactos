#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  static const WCHAR szROS[] = { 'R','e','a','c','t','O','S',0 };
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(hInstance);
  ShellAboutW(0, szROS, 0, 0);
  return 1;
}
