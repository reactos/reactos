// main.c

#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <olectlid.h>
#include <initguid.h>
#include "vrsscan.h"
#include "resource.h"
#include <commdlg.h>
#include "test.h"
//#include "crtfree.h"

#define VIRUS_NAME "VirusTest"

BOOL g_bOleInited;

LRESULT CALLBACK VirusTestWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void VirusTest_Command(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LPWSTR GetFilenameToCheck(HWND hwnd);
void IStreamCheckForVirus(HWND hwnd, BOOL bAPI);
LPSTR MakeAnsiStrFromWide(LPWSTR pwsz);
LPWSTR MakeWideStrFromAnsi(LPSTR psz);

IUnknown *pIUnk = NULL;


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int nShow)
{
    WNDCLASS wc;
    HWND hwnd;               
    MSG msg;
    HRESULT hr;
   
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc   = VirusTestWndProc;
    // wc.cbWndExtra    = CBFRAMEWNDEXTRA;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAIN_MENU);
    wc.lpszClassName = VIRUS_NAME;

    RegisterClass(&wc);
    
    hwnd = CreateWindow(VIRUS_NAME, "Virus Test App", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                       NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);
   
   hr = OleInitialize(NULL);
   if(FAILED(hr))
   {
      MessageBox(hwnd, "COM Initialization failed", "COM Error", MB_OK); 
      return 0;
   }
   else if(hr == S_OK)
   {
      g_bOleInited = TRUE;
   }
   else
   {
      g_bOleInited = FALSE;
   }
   
   hr = CoCreateInstance(CLSID_VirusScan, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown,(void **) &pIUnk);
   if(FAILED(hr))
   {
      MessageBox(hwnd, "Could not create virus check instance", "Virus check error", MB_OK); 
      goto CLEAN_EXIT;
   }

   while (GetMessage(&msg, NULL, 0,0 ))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   pIUnk->Release();
CLEAN_EXIT:   
   
   if(g_bOleInited)
      OleUninitialize();

    return msg.wParam;
}

LRESULT CALLBACK VirusTestWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, VirusTest_Command);
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      default:
            return(DefWindowProc(hwnd, uMsg, wParam, lParam));
    }
   return 0;
}

void VirusTest_Command(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    IVirusScanner *pIVirusScanner;
    IRegisterVirusScanEngine *pIRegVirusProv;
    VIRUSINFO vrsInfo;
    HRESULT hr;
    STGMEDIUM stgMed;
    LPWSTR  pszwFile;
    static DWORD   dwCookie;

   switch (id)
   {
      case ID_CHECKFORVIRUS:
        if ((pszwFile = GetFilenameToCheck(hwnd)) != NULL)
        {
            stgMed.tymed = TYMED_FILE;
            stgMed.lpszFileName = pszwFile;
      
            hr = pIUnk->QueryInterface(IID_IVirusScanner, (void **) &pIVirusScanner);
            if(FAILED(hr))
            {
                MessageBox(hwnd, "No Virus Scanner Interface!", "Virus Error", MB_OK); 
                break;
            }
            //_asm { int 3 }
            vrsInfo.cbSize = sizeof(VIRUSINFO);
            pIVirusScanner->ScanForVirus(hwnd, &stgMed, pszwFile, SFV_DOUI, &(vrsInfo) );
          
            CoTaskMemFree(pszwFile);
            pIVirusScanner->Release();

        }
         break;

      case ID_REGISTER:
         hr = pIUnk->QueryInterface(IID_IRegisterVirusScanEngine, (void **) &pIRegVirusProv);
         if(FAILED(hr))
         {
            MessageBox(hwnd, "No Register Virus Provider Interface!", "Virus Error", MB_OK); 
            break;
         }
                 
         pIRegVirusProv->RegisterScanEngine(CLSID_VirusScanner1, L"Dummy Scanner", NULL, 0, &dwCookie  );
         pIRegVirusProv->Release();
         break;

      case ID_UNREGISTER:
         hr = pIUnk->QueryInterface(IID_IRegisterVirusScanEngine, (void **) &pIRegVirusProv);
         if(FAILED(hr))
         {
            MessageBox(hwnd, "No Register Virus Provider Interface!", "Virus Error", MB_OK); 
            break;
         }
         pIRegVirusProv->UnRegisterScanEngine(CLSID_VirusScanner1, L"Dummy Scanner", NULL, 0, dwCookie);
         pIRegVirusProv->Release();
         break;
    
      case ID_ISTR_SCAN:
          IStreamCheckForVirus(hwnd, FALSE);
          break;

       default:
         break;
   }
}

LPWSTR GetFilenameToCheck(HWND hwnd)
{
    OPENFILENAME OpenFileName;
    char szFilename[MAX_PATH];
    LPWSTR  pwsz = NULL;

    lstrcpy(szFilename, "*.*");
    
    ZeroMemory((void*)&OpenFileName, sizeof(OPENFILENAME));
    OpenFileName.lStructSize       = sizeof (OPENFILENAME);
    OpenFileName.hwndOwner         = hwnd;
    OpenFileName.hInstance         = NULL;
    OpenFileName.lpstrFilter       = "*.*";
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0L;
    OpenFileName.nFilterIndex      = 1L;
    OpenFileName.lpstrFile         = szFilename;
    OpenFileName.nMaxFile          = sizeof (szFilename);
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = NULL;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = NULL;
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 0;
    OpenFileName.Flags             |= (OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER);

    if (GetOpenFileNameA(&OpenFileName))
        pwsz = MakeWideStrFromAnsi(szFilename);

    return pwsz;
}



void IStreamCheckForVirus(HWND hwnd, BOOL bAPI)
{
    LPWSTR  pszwFile;
    HRESULT hr = 0;
    LPSTORAGE pIStorage = NULL;
    LPSTREAM pIStream = NULL;
    VIRUSINFO vrsInfo;
    IVirusScanner *pIVirusScanner = NULL;
    STGMEDIUM stgMed;

    if ((pszwFile = GetFilenameToCheck(hwnd)) != NULL)
    {
        hr = StgCreateDocfile( pszwFile,
                                STGM_TRANSACTED | STGM_CONVERT |
                                STGM_SHARE_DENY_WRITE | STGM_READWRITE,
                                0, &pIStorage );
        if (!FAILED(hr))
        {
            hr = pIStorage->OpenStream( L"CONTENTS", 
                            NULL,
                            STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                            0, &pIStream );
            if (!FAILED(hr))
            {
                stgMed.tymed = TYMED_ISTREAM;
                stgMed.pstm = pIStream;

                    hr = pIUnk->QueryInterface(IID_IVirusScanner, (void **) &pIVirusScanner);
                    if(FAILED(hr))
                    {
                        MessageBox(hwnd, "No Virus Scanner Interface!", "Virus Error", MB_OK); 
                    }
                    else
                    {

                        vrsInfo.cbSize = sizeof(VIRUSINFO);
                        _asm { int 3 }
                        pIVirusScanner->ScanForVirus(hwnd, &stgMed, L"foo.bar", SFV_DOUI, &vrsInfo );
                        pIVirusScanner->Release();
                    }

                pIStream->Release();
            }
            pIStorage->Release();
        }
        CoTaskMemFree(pszwFile);

    }
}


LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz)
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length
    //
    i =  WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);
    if (i <= 0) return NULL;

    psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));

    if (!psz) return NULL;
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}

