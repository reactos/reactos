/* $Id: window.c,v 1.1 2003/03/20 19:19:23 rcampbell Exp $
 *
 * WINDOW.C - activate & window internal commands.
 *
 * clone from 4nt activate command
 *
 * 10 Sep 1999 (Paolo Pantaleo)
 *     started (window command in WINDOW.c)
 *
 * 29 Sep 1999 (Paolo Pantaleo)
 *     activate and window in a single file using mainly the same code
 *     (nice size optimization :)
 */


#include "config.h"

#if (  defined(INCLUDE_CMD_WINDOW) ||  defined(INCLUDE_CMD_ACTIVATE)  )

#include "cmd.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>


#define A_MIN		0x01
#define A_MAX		0x02
#define A_RESTORE	0x04
#define A_POS		0x08
#define A_SIZE		0x10
#define A_CLOSE		0x20


/*service funciton to perform actions on windows

 param is a string to parse for options/actions
 hWnd is the handle of window on wich perform actions

*/

static
INT ServiceActivate (LPTSTR param, HWND hWnd)
{
	LPTSTR *p=0,p_tmp;
	INT argc=0,i;
	INT iAction=0;
	LPTSTR title=0;
	WINDOWPLACEMENT wp;
	RECT pos;
	LPTSTR tmp;


	if(*param)
		p=split(param,&argc);


	for(i = 0; i < argc; i++)
	{
		p_tmp=p[i];
		if (*p_tmp == _T('/'))
			p_tmp++;

		if (_tcsicmp(p_tmp,_T("min"))==0)
		{
			iAction |= A_MIN;
			continue;
		}

		if (_tcsicmp(p_tmp,_T("max"))==0)
		{
			iAction |= A_MAX;
			continue;
		}

		if (_tcsicmp(p_tmp,_T("restore"))==0)
		{
			iAction |= A_RESTORE;
			continue;
		}

		if (_tcsicmp(p_tmp,_T("close"))==0)
		{
			iAction |= A_CLOSE;
			continue;
		}

		if (_tcsnicmp(p_tmp,_T("pos"),3)==0)
		{
			iAction |= A_POS;
			tmp = p_tmp+3;
			if (*tmp == _T('='))
				tmp++;

			pos.left= _ttoi(tmp);
			if(!(tmp=_tcschr(tmp,_T(','))))
			{
				error_invalid_parameter_format(p[i]);
				freep(p);
				return 1;
			}

			pos.top = _ttoi (++tmp);
			if(!(tmp=_tcschr(tmp,_T(','))))
			{
				error_invalid_parameter_format(p[i]);
				freep(p);
				return 1;
			}

			pos.right = _ttoi(++tmp)+pos.left;
			if(!(tmp=_tcschr(tmp,_T(','))))
			{
				error_invalid_parameter_format(p[i]);
				freep(p);
				return 1;
			}
			pos.bottom = _ttoi(++tmp) + pos.top;
			continue;
		}

		if (_tcsnicmp(p_tmp,_T("size"),4)==0)
		{
			iAction |=A_SIZE;
			continue;
		}

		/*none of them=window title*/
		if (title)
		{
			error_invalid_parameter_format(p[i]);
			freep(p);
			return 1;
		}

		if (p_tmp[0] == _T('"'))
		{
			title = (p_tmp+1);
			*_tcschr(p_tmp+1,_T('"'))=0;
			continue;
		}
		title = p_tmp;
	}

	if(title)
		SetWindowText(hWnd,title);

	wp.length=sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd,&wp);

	if(iAction & A_POS)	
		wp.rcNormalPosition = pos;	

	if(iAction & A_MIN)
		wp.showCmd=SW_MINIMIZE;

	if(iAction & A_MAX)
		wp.showCmd=SW_SHOWMAXIMIZED;

	/*if no actions are specified default is SW_RESTORE*/
	if( (iAction & A_RESTORE) || (!iAction) )
		wp.showCmd=SW_RESTORE;

	if(iAction & A_CLOSE)
		ConErrPrintf(_T("!!!FIXME:  CLOSE Not implemented!!!\n"));

	wp.length=sizeof(WINDOWPLACEMENT);
	SetWindowPlacement(hWnd,&wp);

	if(p)
		freep(p);

	return 0;
}




INT CommandWindow (LPTSTR cmd, LPTSTR param)
{
	HWND h;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T("change console window aspect\n"
		              "\n"
		              "WINDOW [/POS[=]left,top,width,heigth]\n"
		              "              [MIN|MAX|RESTORE] [\"title\"]\n"
		              "\n"
		              "/POS          specify window placement and dimensions\n"
		              "MIN           minimize the window\n"
		              "MAX           maximize the window\n"
		              "RESTORE       restore the window"));
		return 0;
	}

	h = GetConsoleWindow();
	Sleep(0);
	return ServiceActivate(param,h);
}


INT CommandActivate (LPTSTR cmd, LPTSTR param)
{
	LPTSTR str;
	HWND h;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T("change console window aspect\n"
		              "\n"
		              "ACTIAVTE \"window\" [/POS[=]left,top,width,heigth]\n"
		              "              [MIN|MAX|RESTORE] [\"title\"]\n"		              
		              "\n"
		              "window        tile of window on wich perform actions\n"
		              "/POS          specify window placement and dimensions\n"
		              "MIN           minimize the window\n"
		              "MAX           maximize the window\n"
		              "RESTORE       restore the window\n"
		              "title          new title"));
		return 0;
	}

	if(!(*param))
		return 1;

	str=_tcschr(param,_T(' '));

	if (str)
	{
		*str=_T('\0');
		str++;
	}
	else
		str = "";

	h=FindWindow(NULL, param);
	if (!h)
	{
		ConErrPuts("window not found");
		return 1;
	}

	return ServiceActivate(str,h);
}

#endif /* (  defined(INCLUDE_CMD_WINDOW) ||  defined(INCLUDE_CMD_ACTIVATE)  ) */
