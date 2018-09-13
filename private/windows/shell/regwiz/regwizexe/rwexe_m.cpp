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
** Modification History	
**     07/20/98 : The RegWizControl is changed  from Button control to an IE object,
**                some of the properties like Text, HWND  of the button no longer exists.
*************************************************************************/

#include <stdio.h>
#include <tchar.h>
#include "rwexe_m.h"
#include "regwizC_i.c"

#define STRCONVERT_MAXLEN 256
OLECHAR* ConvertToUnicode(char FAR* szA)
{
  static OLECHAR achW[STRCONVERT_MAXLEN]; 

  MultiByteToWideChar(CP_ACP, 0, szA, -1, achW, STRCONVERT_MAXLEN);  
  return achW; 
}


int LoadAndUseRegWizCtrl(TCHAR *lpCmdLine)
{
	HRESULT hr;
	IRegWizCtrl *pRegWiz;


	hr = CoCreateInstance( CLSID_RegWizCtrl,
	                       NULL,
	                       CLSCTX_ALL,
	                       IID_IRegWizCtrl,
	                       (void**)&pRegWiz);

	if(FAILED(hr) ){
		//printf("\n Error Creating Interface...");
		return 0;
	}else {
		//printf("\n Created Interface (IExeTest)...");
	}
	// Invoke register
	#ifndef _UNICODE 
		hr = pRegWiz->InvokeRegWizard(ConvertToUnicode(lpCmdLine));
	#else
		hr = pRegWiz->InvokeRegWizard(lpCmdLine);
	#endif
		return 0;
}


int APIENTRY WinMain (HINSTANCE hinst, 
					  HINSTANCE hinstPrev, 
					  LPSTR lpCmdLine, 
					  int nCmdShow)
{
	CoInitialize(NULL);
	LoadAndUseRegWizCtrl(lpCmdLine);
	CoUninitialize();
	return 1;
}

