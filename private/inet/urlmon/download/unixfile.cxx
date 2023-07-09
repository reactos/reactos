#include "unixfile.h"

void UnixEnsureDir(char *pszFile)
{
    char szDirPath[MAX_PATH];
    int iLen;

    lstrcpy(szDirPath, pszFile);
    iLen = lstrlen(szDirPath);

    while (szDirPath[iLen] != '/')
    {
    iLen--;
    }
    szDirPath[iLen] = '\0';

    CreateDirectory(szDirPath, NULL);
}

void UnixifyFileName(char* lpszName)
{
    while(*lpszName)
    {
        if(*lpszName == '\\')
            *lpszName = '/';
        lpszName++;
    }
}

const GUID CLSID_JAVA_VM =
{
   0x08b0e5c0, 0x4fcb, 0x11cf, {0xaa, 0xa5, 0x00, 0x40, 0x1c, 0x60, 0x85, 0x00}
};
#define JAVA_DLL      TEXT("msjava.dll")
#define szVMInstalled TEXT("IsVMInstalled")

HRESULT CheckIEFeatureOnUnix(LPCWSTR pwszIEFeature,
                             DWORD* dwInstalledVerHi,
                             DWORD* dwInstalledVerLo,
                             DWORD  dwFlags)
{
   HRESULT hr = E_FAIL;
   CLSID   curCLSID;

   if (dwInstalledVerHi)
      memset(dwInstalledVerHi, 0, sizeof(DWORD));

   if (dwInstalledVerLo)
      memset(dwInstalledVerLo, 0, sizeof(DWORD));

   if (CLSIDFromString((LPOLESTR)pwszIEFeature, &curCLSID) != ERROR_SUCCESS)
   {
      hr = S_FALSE;
      goto Cleanup;
   }

   if (IsEqualCLSID(curCLSID, CLSID_JAVA_VM))
   {
      HMODULE hLibJava = NULL;

      typedef BOOL (WINAPI *LPISVMINSTALLED)();
      LPISVMINSTALLED lpfnIsVMInstalled;

      hr = ERROR_PRODUCT_UNINSTALLED; /* We are handling it in any case */
      if ((hLibJava = LoadLibrary(JAVA_DLL)) != NULL)
      {
         lpfnIsVMInstalled = (LPISVMINSTALLED)GetProcAddress(hLibJava, szVMInstalled);
         if (lpfnIsVMInstalled)
         {
            if (lpfnIsVMInstalled())
               hr = S_OK;
         }

         FreeLibrary(hLibJava);
      }
   }

Cleanup:
   return hr;
}
