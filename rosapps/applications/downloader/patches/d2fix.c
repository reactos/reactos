#include <windows.h>
#include <stdio.h>

int main()
{
 
  HKEY hKey; 
  DWORD dwVal = 1;
  DWORD dwSize = MAX_PATH;
  DWORD lpdwDisposition = 0;
  CHAR szBuf[MAX_PATH];

  printf("%s", "Setting Diablo 2 commandline parameters to -w -glide...");

  if (RegCreateKeyExA (HKEY_CURRENT_USER,
                    "SOFTWARE\\Blizzard Entertainment\\Diablo II Shareware",
                    0,
					NULL,
                    0,
					KEY_ALL_ACCESS,
					NULL,
                    &hKey,
					&lpdwDisposition)); 
  {
    strcpy(szBuf, "-w -glide");

    RegSetValueExA(hKey, "UseCmdLine", 0, REG_DWORD, (LPCSTR)&dwVal, sizeof(dwVal));
    RegSetValueExA(hKey, "CmdLine", 0, REG_SZ, (LPCSTR)szBuf, strlen(szBuf)+1);

    RegCloseKey(hKey);
	printf("%s", "done.");

  }

  return 0;
}
