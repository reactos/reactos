////////////////////////////////////////////////////////
//
// script.cpp
// 
// Script Functions
//
//
// Klemens Friedl, 19.03.2005
// frik85@hotmail.com
//
////////////////////////////////////////////////////////////////////

#include "package.hpp"
#include "script.h"
#include "log.h"

extern const char* tree_server;
char* PML_Download (const char* name, const char* local_name, const char* server, BOOL totemp = TRUE);


int debuglog (int argc, char* argv[])
{
	Log("!  SCRIPT DEBUG: ");
	LogAdd(argv[1]);

	return ERR_OK;
}

int download (int argc, char* argv[])
{
	char* result;

	if (argc==3)
		result = PML_Download(argv[1], argv[2], argv[3]);

	else if (argc==2)
		result = PML_Download(argv[1], argv[2], NULL);

	else 
		return ERR_GENERIC;

	if(!result)
		return ERR_GENERIC;

	return ERR_OK;

}

int extract (int argc, char* argv[])
{
	return ERR_OK;
}

int msgbox (int argc, char* argv[])
{
	if (argc==1)
		MessageBoxA(0,argv[1],0,0);

	else if (argc==2)
		MessageBoxA(0,argv[1],argv[2],0);

	else 
		return ERR_GENERIC;

	return ERR_OK;
}

int shell (int argc, char* argv[])
{
	// Get the temp dir
	char tmp [MAX_PATH];
	GetTempPathA (MAX_PATH, tmp);

	SHELLEXECUTEINFOA info = {0};
	info.cbSize = sizeof(SHELLEXECUTEINFO);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = "open";
	info.lpFile = argv[1];		
	info.lpDirectory = tmp;
	info.nShow = SW_SHOW;

	if(argc >= 2)
		info.lpParameters = "";

	if(!ShellExecuteExA (&info))
		return ERR_GENERIC;

	WaitForSingleObject (info.hProcess, INFINITE);

	return ERR_OK;
}

const FUNC_TABLE FuncTable[] =
{
   /* Name */   /* Function */ 
  {"download",	download},
  {"extract",	extract},
  {"shell",		shell},
  {"msgbox",	msgbox},
  {"debug",		debuglog}
};
