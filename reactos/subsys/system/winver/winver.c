#include <windows.h>
#include <tchar.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  static const WCHAR szROS[] = { 'R','e','a','c','t','O','S','\n',0 };
  ShellAbout(0, szROS, 0, 0);
  return 1;
}
