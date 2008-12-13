#include <windows.h>

int main (int flags, char **cmdline, char **inst)
{
  return (int) WinMain ((HINSTANCE) inst, NULL, (LPSTR) cmdline,(DWORD) flags);
}
