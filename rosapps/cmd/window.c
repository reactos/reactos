/*
 * WINDOW.C - activate internal command.
 *
 * clone from 4nt window command
 *
 * 10 Sep 1999
 *     started - Paolo Pantaleo <dfaustus@freemail.it>
 *
 *
 */


#include "config.h"

#ifdef INCLUDE_CMD_WINDOW

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


INT CommandWindow (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *p,p_tmp;
	INT argc,i;
	INT iAction=0;
	LPTSTR title=0;
	HWND hWnd;
	WINDOWPLACEMENT wp;
	RECT pos;
	LPTSTR tmp;

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts(_T("change console window aspect\n"
		              "\n"
		              "WINDOW [/POS[=]left,top,width,heigth]\n"
		              "              [MIN|MAX|RESTORE]\n"
		              "\n"
		              "/POS          specify window placement and dimensions\n"
		              "MIN           minimize the window\n"
		              "MAX           maximize the window\n"
		              "RESTORE       restore the window"));
		return 0;
	}

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

#if 0
		if(*p_tmp != '"')
		{
			error_invalid_parameter_format(p[i]);
			
			freep(p);
			return 1;
		}
#endif

		//none of them=window title
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
		SetConsoleTitle(title);

	hWnd=GetConsoleWindow();

	wp.length=sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd,&wp);

	if(iAction & A_POS)
	{
		wp.rcNormalPosition = pos;
	}

	if(iAction & A_MIN)
		wp.showCmd=SW_MINIMIZE;

	if(iAction & A_MAX)
		wp.showCmd=SW_SHOWMAXIMIZED;

	if(iAction & A_RESTORE)
		wp.showCmd=SW_RESTORE;

	wp.length=sizeof(WINDOWPLACEMENT);
	SetWindowPlacement(hWnd,&wp);

	freep(p);
	return 0;
}

#endif /* INCLUDE_CMD_WINDOW */