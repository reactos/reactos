////////////////////////////////////////////////////////
//
// package.cpp
// 
// package related functions
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "package.hpp"
#include "expat.h"
#include "log.h"

int PML_XmlDownload (pTree, const char* url, void* usrdata, XML_StartElementHandler start, 
									XML_EndElementHandler end, XML_CharacterDataHandler text=0);


// expat callback for start of a package tag
void pack_start (void* usrdata, const char* tag, const char** arg)
{
	int i, id;
	PACKAGE* pack = (PACKAGE*)usrdata;

	// if the tag is a script tag ...
	if(!strcmp(tag, "scripts"))
	{
		// ... read the arguments
		for (i=0; arg[i]; i+=2) 
		{
			if(!strcmp(arg[i], "inst"))
				id = 0;

			else if(!strcmp(arg[i], "update"))
				id = 1;

			else if(!strcmp(arg[i], "uinst"))
				id = 2;

			else if(!strcmp(arg[i], "srcinst"))
				id = 3;

			else
				continue;

			pack->files[id] = new char [strlen(arg[i+1])+1];
			strcpy(pack->files[id], arg[i+1]);
		}
	}

	// ... save the field
	else
	{
		if(!strcmp(tag, "name"))
			pack->field = &pack->name;

		else if(!strcmp(tag, "description"))
			pack->field = &pack->description;

		else if (!strcmp(tag, "depent"))
		{
			pack->depencies.push_back((char*)NULL);
			pack->field = &pack->depencies.back();
		}
	}
}

// expat callback for end of a package tag
void pack_end (void* usrdata, const char* tag)
{
	PACKAGE* pack = (PACKAGE*)usrdata;

	pack->field = NULL;
}

// expat callback for text
void pack_text (void* usrdata, const char* data, int len)
{
	PACKAGE* pack = (PACKAGE*)usrdata;

	if(!pack->field)
		return;

	*pack->field = new char[len+1];
	strncpy(*pack->field, data, len);
	(*pack->field)[len] = '\0';
}

// The user clicks on a package
extern "C" int PML_LoadPackage (TREE* tree, int id, PML_SetButton SetButton)
{
	PACKAGE* pack = &tree->packages[id];
	tree->setButton = SetButton;

	if(SetButton)
	{
		SetButton(1, pack->action);
		SetButton(2, pack->inst); // && pack->action != 0
		SetButton(3, pack->src_inst); 
		SetButton(4, pack->update);
		SetButton(5, pack->uninstall); 
	}

	// root notes (like network) return here
	if(!pack->path)
		return 1;

	if(!pack->loaded)
	{
		PML_XmlDownload (tree, pack->path, (void*)pack, pack_start, pack_end, pack_text);
		pack->loaded = TRUE;
	}

	return ERR_OK;
}

extern "C" int PML_FindItem (TREE* tree, const char* what)
{
	int i, j;
	bool found;

	// if we have children, same action for them
	for (i=1; (UINT)i<tree->packages.size(); i++)
	{
		found = true;

		for(j=0; (UINT)j<strlen(what); j++)
		{
			if(tolower(what[j]) != tolower(tree->packages[i].name[j]))
			{
				found = false;
				break;
			}
		}

		if(found)
			return i;
	}

	return 0;
}

// The user chooses a actions like Install
extern "C" int PML_SetAction (TREE* tree, int id, int action, PML_SetIcon SetIcon, PML_Ask Ask)
{
	UINT i;
	int ret = ERR_OK;

	tree->setIcon = SetIcon;
	PACKAGE* pack = &tree->packages[id];

	// if we have children, same action for them
	for (i=0; i<pack->children.size(); i++)
		ret = ret || PML_SetAction(tree, pack->children[i], action, SetIcon, Ask);

	// is the action possible ? 
	if(!pack->actions[action])
		return ERR_GENERIC;

	// is it already set 
	if(pack->action == action)
		return ERR_OK;

	//
	if(pack->depencies.size() && action)
	{
		UINT count = pack->depencies.size();
		WCHAR buffer[2000], buffer2[200], errbuf[2000];
		PML_TransError(ERR_DEP1, (WCHAR*)buffer, sizeof(buffer)/sizeof(WCHAR));

		for (i=0; i<pack->depencies.size(); i++)
		{
			int item = PML_FindItem(tree, pack->depencies[i]);

			if(!item)
				return ERR_GENERIC;

			if(action == tree->packages[item].action)// || tree->packages[item].installed
			{
				count--;
				continue;
			}

			MultiByteToWideChar (CP_ACP, 0, pack->depencies[i], strlen(pack->depencies[i])+1, buffer2, 200);
			wsprintf(buffer, L"%s - %s\n", buffer, buffer2);//
		}

		wcscat(buffer, PML_TransError(ERR_DEP2, (WCHAR*)errbuf, sizeof(errbuf)/sizeof(WCHAR)));

		if(count)
		{
			if(!Ask(buffer))
				return ERR_GENERIC;

			for (i=0; i<pack->depencies.size(); i++)
			{
				int item = PML_FindItem(tree, pack->depencies[i]);

				tree->packages[item].neededBy.push_back(id);

				PML_SetAction(tree, item, action, SetIcon, Ask);
			}
		}
	}

	// load it if it's not loaded yet
	else if (!pack->loaded && pack->path)
	{
		PML_XmlDownload (tree, pack->path, (void*)pack, pack_start, pack_end, pack_text);
		pack->loaded = TRUE;

		return PML_SetAction(tree, id, action, SetIcon, Ask);
	}

	// set the icon
	if(SetIcon && !pack->icon)
			SetIcon(id, action);

	// set the button(s)
	if(tree->setButton && action != 2)
	{
		tree->setButton(1, action);
		//tree->setButton(pack->action+1, action);
	}

	// can't do src install yet
	if(action == 2)
	{
		MessageBox(0, L"Sorry, but source install is not implemented yet.", 0,0);
		return ERR_OK;
	}

	// everything but undoing is done here
	else if (action != 0)
	{
		// since we are setting a action we undo it again
		if(tree->setButton)
			tree->setButton(1, 1);
		//tree->setButton(action+1, 0);

		pack->action = action;

		// root notes (like network) return here
		if(!pack->path)
			return ret; 

		// save the name of the corresponding script in a vector
		tree->todo.push_back(pack->files[action-1]);
	}

	// undoing
	else 
	{
		for(i=0; i<pack->neededBy.size(); i++)
		{
			if(tree->packages[pack->neededBy[i]].action)
			{
				SetIcon(id, pack->action);
				return ERR_GENERIC;
			}
		}

		// root notes (like network) return here
		if(!pack->path || pack->action==0)
			return ret; 

		// erase from todo list
		for(i=0; i<tree->todo.size(); i++)
			if(!strcmp(tree->todo[i], pack->files[pack->action-1])) // look for right entry
				tree->todo.erase(tree->todo.begin()+i); // delete it

		// set action back
		pack->action = 0;
	}

	return ret;
}

//
extern "C" char* PML_GetDescription (TREE* tree, int id)
{
	PML_LoadPackage(tree, id, NULL);

	return tree->packages[id].description;
}

