////////////////////////////////////////////////////////
//
// main.cpp
// 
// Implementation of a Commandlne Interface
// for the ReactOs Package Manager
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "main.h"
#include <stdio.h>


int main (int argc, char **argv) 
{
	wprintf(L"ReactOs PackageManager %d.%d.%d Commandline Interface \n\n", PACKMGR_VERSION_MAJOR, PACKMGR_VERSION_MINOR, PACKMGR_VERSION_PATCH_LEVEL);
	Argv = argv; Argc = argc;

	if(argc<2)
		return Help();

	// install a package
	if (!strcmp(argv[1], "install")) 
		Install();

	// install a package from source
	else if (!strcmp(argv[1], "src-inst"))
	{
		wprintf(L"Sorry but I can't do that yet. \n");
	}

	// update a package
	else if (!strcmp(argv[1], "update"))
	{
		wprintf(L"Sorry but I can't do that yet. \n");
	}

	// update everything
	else if (!strcmp(argv[1], "dist-upgrade"))
	{
		wprintf(L"Sorry but I can't do that yet. \n");
	}

	// remove a package
	else if (!strcmp(argv[1], "remove"))
	{
		wprintf(L"Sorry but I can't do that yet. \n");
	}

	// search for a package
	else if (!strcmp(argv[1], "show"))
	{
		Show();
	}

	// search for a package
	else if (!strcmp(argv[1], "search"))
	{
		wprintf(L"Sorry but I can't do that yet. \n");
	}

	else
		Help();

	//
	wprintf(L"\n");
	

	return 0;
}

int Help (void)
{
	wprintf(L"Usage: ros-get [command] \n\n");

	wprintf(L"Possible commands: \n");
	wprintf(L"  install [package name] \t Installs a package \n\n");
	wprintf(L"  show [package name] \t\t Shows you detailed information about a package \n");

	wprintf(L"Currently unimplemented commands: \n");
	wprintf(L"  src-install [package name] \t Installs a package from source code \n");
	wprintf(L"  update [package name] \t Updates a package \n");
	wprintf(L"  dist-update [package name] \t Updates a package \n");
	wprintf(L"  remove [package name] \t Uninstalls a package \n\n");

	wprintf(L"  search [search agrument] \t Finds a package \n");
	wprintf(L"  list \t\t\t\t Lists all installed programs \n");

	return 0;
}

int Ask (const WCHAR* question)
{
	// ask the user
	wprintf(L"%s [y/n] ", question);
	char answer = getchar();

	// clear keybuffer
	while(getchar()!='\n');
	wprintf(L"\n");

	// prozess answer
	if (answer == 'y')
		return 1;

	else if (answer == 'n')
		return 0;

	return Ask(question);
}
	
int SetStatus (int status1, int status2, WCHAR* text)
{
	WCHAR errbuf[2000];
	if(text)
		wprintf(L"%s\n", text);

	// If the Status is 1000 things are done
	if(status1==1000)
	{
		wprintf(L"%s\n", PML_TransError(status2, errbuf, sizeof(errbuf)/sizeof(WCHAR)));
		done = TRUE;
	}

	return 0;
}

int Install (void)
{
	pTree tree;
	int i, error;
	WCHAR errbuf[2000];

	// load the tree
	error = PML_LoadTree (&tree, "tree.xml", NULL);
	if(error)
	{

		wprintf(PML_TransError(error, errbuf, sizeof(errbuf)/sizeof(WCHAR)));
		return 0;
	}
		
	// look up the item
	for (i=2; i<Argc; i++)
	{
		int id = PML_FindItem(tree, Argv[i]);

		if(id)
		{
			PML_LoadPackage(tree, id, NULL);
			PML_SetAction(tree, id, 1, NULL, Ask);
		}

		else 
			printf("Could not find the Package \"%s\"\n", Argv[i]);
	}

	// do it
	error = PML_DoIt (tree, SetStatus, Ask);
	if(error)
	{

		wprintf(PML_TransError(error, errbuf, sizeof(errbuf)/sizeof(WCHAR)));
		PML_CloseTree (tree);
		return 0;
	}

	// wait
	while (!done)
		Sleep(1000);

	// clean up
	PML_CloseTree (tree);

	return 0;
}

int Show (void)
{
	pTree tree;
	int i, error;
	WCHAR errbuf[2000];

	// load the tree
	error = PML_LoadTree (&tree, "tree_bare.xml", NULL);
	if(error)
	{
		wprintf(PML_TransError(error, errbuf, sizeof(errbuf)/sizeof(WCHAR)));
		return 0;
	}

	// look up the item
	for (i=2; i<Argc; i++)
	{
		int id = PML_FindItem(tree, Argv[i]);

		if(id)
			printf(PML_GetDescription(tree, id));

		else 
			printf("Could not find the Package \"%s\"\n", Argv[i]);
	}

	// clean up
	PML_CloseTree (tree);

	return 0;
}
