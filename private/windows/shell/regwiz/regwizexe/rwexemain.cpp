/*************************************************************************
**
**    
**	File 	:    RWExeMain.cpp
**      Product  :	
**	Date 	:    05/07/97
**	Author 	:    Suresh Krishnan	
**
**   Registration Wizard Exe programs main file
**   The Exe version of Registration Wizard is implemented using the Active X 
**   component
**
*************************************************************************/
#define STRICT

#include <windows.h>
#include <windowsx.h>
#ifdef WIN16   
  #include <ole2.h>
  #include <compobj.h>    
  #include <dispatch.h> 
  #include <variant.h>
  #include <olenls.h>
  #include <commdlg.h>  
#endif  
#include <initguid.h>   
#include "RWInterface.h" 
#include "RWExeMain.h"      
#include "resource.h"


// Globals
HINSTANCE g_hinst;                          // Instance of application
HWND      g_hwnd;                           // Toplevel window handle

TCHAR g_szNotFound[STR_LEN];
TCHAR g_szError[STR_LEN]; 

#define INVOKDE_REGISTRATION   1
#define INVOKE_TRANSFER        2 
#define INVOKE_ERROR           3
#define chSpace 32
int ProcessCmdLine(LPTSTR lpCmd) 
{
	int iC=0;
	int iExit=1;
	LPTSTR szCurr = lpCmd;
	if (lpCmd  == NULL || lpCmd[0] == 0)
	{
		return INVOKE_ERROR;
	}
	while (*szCurr == chSpace)
		{
			szCurr = CharNext(szCurr);
		}
	if (*szCurr != '/' && *szCurr != '-') {
		return INVOKE_ERROR ;
	}
		szCurr = CharNext(szCurr);
		if (*szCurr == 'I' || *szCurr == 'i') {
			return INVOKDE_REGISTRATION ;
		}
		if (*szCurr == 'T' || *szCurr == 't') {
			return INVOKE_TRANSFER ;
		}
		
		return INVOKE_ERROR;


}
/*
 * WinMain
 *
 * Purpose:
 *  Main entry point of application. Should register the app class
 *  if a previous instance has not done so and do any other one-time
 *  initializations.
 *
 */
/*
int _tWinMain   ( HINSTANCE hinst, 
				  HINSTANCE hinstPrev, 
				  LPTSTR lpCmdLine, 
				  int nCmdShow)
*/
int APIENTRY WinMain (HINSTANCE hinst, 
					  HINSTANCE hinstPrev, 
					  LPSTR lpCmdLine, 
					  int nCmdShow)

{
   MSG msg;
   static IRegWizCtrl FAR* pRegWiz = NULL;    
   HRESULT hr;
   LPUNKNOWN punk;
   
   
   //  It is recommended that all OLE applications set
   //  their message queue size to 96. This improves the capacity
   //  and performance of OLE's LRPC mechanism.
   int cMsg = 96;                  // Recommend msg queue size for OLE
   while (cMsg && !SetMessageQueue(cMsg))  // take largest size we can get.
       cMsg -= 8;
   if (!cMsg)
       return -1;                  // ERROR: we got no message queue
	LoadString(hinst, IDS_RWNOTFOUND, g_szNotFound, STR_LEN);
    LoadString(hinst, IDS_ERROR, g_szError, STR_LEN);
   
   
   if (!hinstPrev)
      if (!InitApplication(hinst))
         return (FALSE);

   if(OleInitialize(NULL) != NOERROR)
      return FALSE;
      
   if (!InitInstance(hinst, nCmdShow))
      return (FALSE);

   hr = CoCreateInstance(CLSID_RegWizCtrl, NULL, CLSCTX_INPROC_SERVER, 
        IID_IUnknown, (void FAR* FAR*)&punk);
	if (FAILED(hr))                 {
            MessageBox(NULL,g_szNotFound , g_szError, MB_OK); 
            return 0L;
        }                     
        hr = punk->QueryInterface(IID_IRegWizCtrl ,  (void FAR* FAR*)&pRegWiz);   
        if (FAILED(hr))  
        {
            MessageBox(NULL, TEXT("QueryInterface(IID_IHello)"), g_szError, MB_OK);
            punk->Release(); 
            return 0L;
        }
#ifndef _UNICODE 
			hr = pRegWiz->InvokeRegWizard(ConvertToUnicode(lpCmdLine));
#else
			hr = pRegWiz->InvokeRegWizard(lpCmdLine);
#endif

/*
	int iStatus = ProcessCmdLine(lpCmdLine);
	switch(iStatus ) 
	{
	case INVOKDE_REGISTRATION :
#ifndef _UNICODE 
			hr = pRegWiz->InvokeRegWizard(ConvertToUnicode(lpCmdLine));
#else
			hr = pRegWiz->InvokeRegWizard(lpCmdLine);
#endif
			break;
	case INVOKE_TRANSFER :
#ifndef _UNICODE 
			hr = pRegWiz->TransferRegWizInformation(ConvertToUnicode(lpCmdLine));
#else
			hr = pRegWiz->TransferRegWizInformation(lpCmdLine);
#endif
	default:
		break;

	}
*/
	//	ConvertToUnicode("/i \"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\"")	);
	
   OleUninitialize();
   if(SUCCEEDED(hr))
	   return NO_ERROR;
   else
	   return 1;

   //return (msg.wParam); // Returns the value from PostQuitMessage
}

/*
 * InitApplication
 *
 * Purpose:
 *  Registers window class
 *
 * Parameters:
 *  hinst       hInstance of application
 *
 * Return Value:
 *  TRUE if initialization succeeded, FALSE otherwise.
 */
BOOL InitApplication (HINSTANCE hinst)
{
   WNDCLASS wc;

   wc.style = CS_DBLCLKS;
   wc.lpfnWndProc = MainWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hinst;
   wc.hIcon =0; // LoadIcon(hinst, TEXT("ControlIcon"));
   wc.hCursor =0; // LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wc.lpszMenuName = NULL ;//TEXT("");
   wc.lpszClassName = TEXT("RegistrationWizard");
   return RegisterClass(&wc);
 }

/*
 * InitInstance
 *
 * Purpose:
 *  Creates and shows main window
 *
 * Parameters:
 *  hinst           hInstance of application
 *  nCmdShow        specifies how window is to be shown
 *
 * Return Value:
 *  TRUE if initialization succeeded, FALSE otherwise.
 */
BOOL InitInstance (HINSTANCE hinst, int nCmdShow)
{
  
   g_hinst   = hinst;
   return TRUE;
}

/*
 * MainWndProc
 *
 * Purpose:
 *  Window procedure for main window
 *
 */
LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
         
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      default:                         
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
   
   return NULL;
}

/*
 * DisplayError
 *
 * Purpose:
 *  Obtains Rich Error Information about the automation error from
 *  the IErrorInfo interface.
 *
 */
void DisplayError(IRegWizCtrl FAR* phello)
{  
   IErrorInfo FAR* perrinfo;    
   BSTR bstrDesc;
   HRESULT hr;
   ISupportErrorInfo FAR* psupporterrinfo;  

   hr = phello->QueryInterface(IID_ISupportErrorInfo, (LPVOID FAR*)&psupporterrinfo);
   if (FAILED(hr)) 
   {
      MessageBox(NULL, TEXT("QueryInterface(IID_ISupportErrorInfo)"), g_szError, MB_OK);
      return;
   }
   
   hr = psupporterrinfo->InterfaceSupportsErrorInfo(IID_IRegWizCtrl);   
   if (hr != NOERROR)
   {   
       psupporterrinfo->Release();
       return;
   }
   psupporterrinfo->Release();
  
   // In this example only the error description is obtained and displayed. 
   // See the IErrorInfo interface for other information that is available. 
   hr = GetErrorInfo(0, &perrinfo); 
   if (FAILED(hr))
       return;   
   hr = perrinfo->GetDescription(&bstrDesc);
   if (FAILED(hr)) 
   {
       perrinfo->Release(); 
       return;
   }  
   
   MessageBox(NULL, FROM_OLE_STRING(bstrDesc), g_szError, MB_OK);   
   SysFreeString(bstrDesc);
}

#ifdef WIN32

#ifndef UNICODE
char* ConvertToAnsi(OLECHAR FAR* szW)
{
  static char achA[STRCONVERT_MAXLEN]; 
  
  WideCharToMultiByte(CP_ACP, 0, szW, -1, achA, STRCONVERT_MAXLEN, NULL, NULL);  
  return achA; 
} 

OLECHAR* ConvertToUnicode(char FAR* szA)
{
  static OLECHAR achW[STRCONVERT_MAXLEN]; 

  MultiByteToWideChar(CP_ACP, 0, szA, -1, achW, STRCONVERT_MAXLEN);  
  return achW; 
}
#endif

#endif   
   
