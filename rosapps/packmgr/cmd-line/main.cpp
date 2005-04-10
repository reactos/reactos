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


int main (int argc, char **argv) 
{
	cout << "ReactOs PackageManager " << PACKMGR_VERSION_MAJOR << "." << PACKMGR_VERSION_MINOR << "." << PACKMGR_VERSION_PATCH_LEVEL << " Commandline Interface \n\n";

	int i;

	if(argc<2)
		return Help();

	for (i=1; i<argc; i++)
		cmdline.push_back(argv[i]);

	// install a package
	if (cmdline[0] == "install") 
		Install();

	// install a package from source
	else if (cmdline[0] == "src-inst")
	{
		cout << "Sorry but I can't do that yet. \n";
	}

	// update a package
	else if (cmdline[0] == "update")
	{
		cout << "Sorry but I can't do that yet. \n";
	}

	// update everything
	else if (cmdline[0] == "dist-upgrade")
	{
		cout << "Sorry but I can't do that yet. \n";
	}

	// remove a package
	else if (cmdline[0] == "remove")
	{
		cout << "Sorry but I can't do that yet. \n";
	}

	// search for a package
	else if (cmdline[0] == "show")
	{
		Show();
	}

	// search for a package
	else if (cmdline[0] == "search")
	{
		cout << "Sorry but I can't do that yet. \n";
	}

	else
		Help();
	
	return 0;
}

int Help (void)
{
	cout << "Usage: ros-get [command] \n\n";

	cout << "Possible commands: \n";
	cout << "  install [package name] \t Installs a package \n\n";
	cout << "  show [package name] \t\t Shows you detailed information about a package \n";

	cout << "Currently unimplemented commands: \n";
	cout << "  src-install [package name] \t Installs a package from source code \n";
	cout << "  update [package name] \t Updates a package \n";
	cout << "  dist-update [package name] \t Updates a package \n";
	cout << "  remove [package name] \t Uninstalls a package \n\n";

	cout << "  search [search agrument] \t Finds a package \n";
	cout << "  list \t\t\t\t Lists all installed programs \n\n";

	return 0;
}

int Ask (const WCHAR* question)
{
	char answer[255];

	wprintf(question);

	cout << " [y/n] ";
	cin >> answer;
	cout << endl;

	if (answer[0]=='y')
		return 1;

	else if (answer[0]=='n')
		return 0;

	return Ask(question);
}
	
int SetStatus (int status1, int status2, WCHAR* text)
{
	if(text)
		wprintf(L"%s\n", text);

	// If the Status is 1000 things are done
	if(status1==1000)
	{
		wprintf(L"%s\n", PML_TransError(status2));
		done = true;
	}

	return 0;
}

int Install (void)
{
	pTree tree;
	int i, error;

	// load the tree
	error = PML_LoadTree (&tree, "tree.xml", NULL);
	if(error)
	{
		cout << PML_TransError(error);
		return 0;
	}
		
	// look up the item
	for (i=1; (UINT)i<cmdline.size(); i++)
	{
		int id = PML_FindItem(tree, cmdline[i].c_str());

		if(id)
			PML_SetAction(tree, id, 1, NULL, Ask);

		else 
			cout << "Could not find the Package \"" << cmdline[i] << "\"\n";
	}

	// do it
	error = PML_DoIt (tree, SetStatus, Ask);
	if(error)
	{
		wprintf(L"%s\n", PML_TransError(error));
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

	// load the tree
	error = PML_LoadTree (&tree, "tree.xml", NULL);
	if(error)
	{
		cout << PML_TransError(error);
		return 0;
	}

	// look up the item
	for (i=1; (UINT)i<cmdline.size(); i++)
	{
		int id = PML_FindItem(tree, cmdline[i].c_str());

		if(id)
			cout << PML_GetDescription (tree, id) << "\n";

		else 
			cout << "Could not find the Package \"" << cmdline[i] << "\"\n";
	}

	// clean up
	PML_CloseTree (tree);

	return 0;
}
