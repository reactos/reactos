////////////////////////////////////////////////////////
//
// main.cpp
//
// Doit stuff and
// everything that fits nowhere else.
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "package.hpp"
#include "log.h"
#include "script.h"

HANDLE hThread = NULL;
BOOL thread_abort = FALSE;

char* PML_Download (pTree, const char* url, const char* server = "tree",  const char* filename = NULL);


// Abort other thread
extern "C" void PML_Abort (void)
{
	thread_abort = TRUE;

	if(hThread)
		WaitForSingleObject(hThread, INFINITE);
}

// Callback function of the "doit"-thread
DWORD WINAPI DoitThread (void* lpParam)
{
	UINT i;
	int ret = ERR_OK;
	TREE* tree = (TREE*)lpParam;
	vector<SCRIPT*> scripts;

	/* Load the scripts */

	tree->setStatus(0, 0, L"Downloading Install instructions ...");

	for(i=0; i<tree->todo.size(); i++)
	{
		SCRIPT* script;

		char* path = PML_Download(tree, tree->todo[i]);

		if(RPS_Load(&script, path) == ERR_OK)
			scripts.push_back(script);
		else
			ret = ERR_PACK;
	}

	/* Preinstall */

	Log("*  enter preinstall");

	tree->setStatus(250, 0, L"Preinstall");

	for(i=0; i<scripts.size(); i++)
	{
		if(RPS_Execute(scripts[i], "preinstall") != ERR_OK)
			ret = ERR_PACK;
	}

	/* Install */

	Log("*  enter install");

	tree->setStatus(500, 0, L"Install");

	for(i=0; i<scripts.size(); i++)
	{
		if(RPS_Execute(scripts[i], "main") != ERR_OK)
			ret = ERR_PACK;
	}

	/* Postinstall */

	Log("*  enter postinstall");

	tree->setStatus(750, 0, L"Postinstall");

	for(i=0; i<scripts.size(); i++)
	{
		if(RPS_Execute(scripts[i], "after") != ERR_OK)
			ret = ERR_PACK;
	}

	/* Finish */

	for(i=0; i<tree->todo.size(); i++)
		RPS_Clear(scripts[i]);

	// clear the todo list
	tree->todo.clear();

	// set all actions to none
	for(i=0; i<tree->packages.size(); i++)
		PML_SetAction (tree, i, 0, tree->setIcon, NULL);

	tree->setStatus(1000, ret, NULL);

    return 1;
}

// Do the actions the user wants us to do
extern "C" int PML_DoIt (TREE* tree, PML_SetStatus SetStatus, PML_Ask Ask)
{
    DWORD dummy;
	tree->setStatus = SetStatus;

	if(!tree->todo.size())
		return ERR_NOTODO;

	//ask
	WCHAR buffer [2000];
	WCHAR errbuf [2000];

	wsprintf(buffer, PML_TransError(ERR_READY, (WCHAR*)errbuf, sizeof(errbuf)/sizeof(WCHAR)), tree->todo.size());

	if(!Ask(buffer))
		return ERR_GENERIC;


	hThread = CreateThread(NULL, 0, DoitThread, tree, 0, &dummy);

	if(!hThread)
		return ERR_GENERIC;

	LogAdd("\n");

	return ERR_OK;
}

// Translates Errorcode into human language
extern "C" WCHAR* PML_TransError (int code, WCHAR *string, INT maxchar)
{

	if(!LoadString(GetModuleHandle(L"package"), code, string, maxchar))
		return PML_TransError(ERR_GENERIC, string, maxchar);

	return string;
}

// Free alloced memory
extern "C" void PML_CloseTree (TREE* tree)
{
	UINT i;

	LogAdd ("\n");
	Log("*  free alloced memory");
	Log("*  package manager will exit now. Bye!");

	for(i=0; i<tree->packages.size(); i++)
	{
		if(tree->packages[i].path)
			delete tree->packages[i].path;

		if(tree->packages[i].name)
			delete tree->packages[i].name;

		if(tree->packages[i].description)
			delete tree->packages[i].description;

		tree->packages.clear();

		if(tree->packages[i].files[0])
			delete tree->packages[i].files[0];

		if(tree->packages[i].files[1])
			delete tree->packages[i].files[1];

		if(tree->packages[i].files[2])
			delete tree->packages[i].files[2];

		if(tree->packages[i].files[3])
			delete tree->packages[i].files[3];
	}

	tree->descriptionPath.clear();
	tree->todo.clear();
	tree->packages.clear();

	if(tree)
		delete tree;
}

