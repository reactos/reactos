/*
 * MSGBOX.C - msgbox internal command.
 *
 * clone from 4nt msgbox command
 *
 * 25 Aug 1999
 *     started - Paolo Pantaleo <paolopan@freemail.it>
 */

#include "config.h"

#ifdef INCLUDE_CMD_MSGBOX
#include <windows.h>
#include <ctype.h>
#include <string.h>
#include <tchar.h>
#include "cmd.h"
//#include <assert.h>

//#include <malloc.h>


#define U_TYPE_INIT 0

//undefine it to allow to omit arguments
//that will be replaced by default ones
#define _SYNTAX_CHECK


INT CommandMsgbox (LPTSTR cmd, LPTSTR param)
{
	//used to parse command line
	LPTSTR tmp;

	//used to find window title (used as messagebox title)
	//and to find window handle to pass to MessageBox
	HWND hWnd;
	TCHAR buff[128];

	//these are MessabeBox() parameters
	LPTSTR title, prompt="";
	UINT uType=U_TYPE_INIT;

	//set default title to window title
	GetConsoleTitle(buff,128);
	title = buff;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T(
		              "display a message box and return user responce\n"
		              "\n"
		              "MSGBOX type [\"title\"] prompt\n"
		              "\n"
		              "type          button displayed\n"
		              "              possible values are: OK, OKCANCEL,\n" 
		              "              YESNO, YESNOCANCEL\n"
		              "title         title of message box\n"
		              "prompt        text displayed by the message box\n"
		              "\n"
		              "\n"
		              "ERRORLEVEL is set according the button pressed:\n"
		              "\n"
		              "YES  :  10    |  NO      :  11\n"
		              "OK   :  10    |  CANCEL  :  12\n"));
		return 0;
	}

	//yes here things are quite massed up :)

	//skip spaces
	while(_istspace(*param))
		param++;

	//search for type of messagebox (ok, okcancel, ...)
	if (_tcsnicmp(param, _T("ok "),3 ) == 0)
	{	
		uType |= MB_ICONEXCLAMATION | MB_OK;
		param+=3;
	}
	else if (_tcsnicmp(param, _T("okcancel "),9 ) == 0)
	{
		uType |= MB_ICONQUESTION | MB_OKCANCEL;
		param+=9;
	}
	else if (_tcsnicmp(param, _T("yesno "),6 ) == 0)
	{
		uType |= MB_ICONQUESTION | MB_YESNO;
		param+=6;
	}
	else if (_tcsnicmp(param, _T("yesnocancel "), 12 ) == 0)
	{
		uType |= MB_ICONQUESTION | MB_YESNOCANCEL;
		param+=12;
	}
	else{
#ifdef _SYNTAX_CHECK
		error_req_param_missing ();
		return 1;
#else
		uType |= MB_ICONEXCLAMATION | MB_OK;
#endif
	}


	//skip spaces
	while(_istspace(*param))
		param++;

#ifdef _SYNTAX_CHECK
	//if reached end of string
	//it is an error becuase we do not yet have prompt
	if ( *param == 0)
	{
		error_req_param_missing ();
		return 1;
	}
#endif

	//search for "title"
	tmp = param;

	if(*param == '"')
	{
		tmp = _tcschr(param+1,'"');
		if (tmp)
		{
			*tmp = 0;
		title = param+1;
		tmp++;
		param = tmp;
		}
	}

	//skip spaces
	while(_istspace(*param))
		param++;
#ifdef _SYNTAX_CHECK
	//get prompt
	if ( *param == 0)
	{
		error_req_param_missing ();
		return 1;
	}
#endif

	prompt = param;

	hWnd=GetConsoleWindow ();
//        DebugPrintf("FindWindow hWnd = %d\n",hWnd); 
        ConErrPrintf("FindWindow hWnd = %d\n",hWnd); 

	switch (
		MessageBox(hWnd,prompt,title,uType)
		)
	{
	case IDYES:
	case IDOK:
		nErrorLevel = 10;
		break;

	case IDNO:
		nErrorLevel = 11;
		break;

	case IDCANCEL:
		nErrorLevel = 12;
		break;
	}

	return 0;
}

#endif /* INCLUDE_CMD_MSGBOX */
