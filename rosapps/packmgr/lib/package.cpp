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

int PML_XmlDownload (const char* url, void* usrdata, XML_StartElementHandler start, 
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
extern "C" int PML_LoadPackage (TREE* tree, int id, PML_SetButton SetButton, PML_SetText SetText)
{
	PACKAGE* pack = &tree->packages[id];
	tree->setButton = SetButton;

	SetButton(1, pack->action);
	SetButton(2, pack->inst); // && pack->action != 0
	SetButton(3, pack->src_inst); 
	SetButton(4, pack->update);
	SetButton(5, pack->uninstall); 

	// root notes (like network) return here
	if(!pack->path)
		return 1;

	if(!pack->loaded)
	{
		PML_XmlDownload (pack->path, (void*)pack, pack_start, pack_end, pack_text);
		pack->loaded = TRUE;
	}

	if(pack->description)
		SetText(pack->description);

	return ERR_OK;
}

// The user chooses a actions like Install
extern "C" int PML_SetAction (TREE* tree, int id, int action, PML_SetIcon SetIcon)
{
	UINT i;
	int ret = ERR_OK;

	tree->setIcon = SetIcon;
	PACKAGE* pack = &tree->packages[id];

	// if we have children, same action for them
	for (i=0; i<pack->children.size(); i++)
		ret = ret || PML_SetAction(tree, pack->children[i], action, SetIcon);

	// is the action possible ? 
	if(!pack->actions[action])
		return ERR_GENERIC;

	// is it already set 
	if(pack->action == action)
		return ERR_OK;

	// set the icon
	if(!pack->icon)
		SetIcon(id, action);

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
		tree->setButton(1, 1);
		//tree->setButton(action+1, 0);
		pack->action = action;

		// root notes (like network) return here
		if(!pack->path)
			return ret; 

		// load it if it's not loaded yet
		if(!pack->loaded)
		{
			PML_XmlDownload (pack->path, (void*)pack, pack_start, pack_end, pack_text);
			pack->loaded = TRUE;
		}

		// save the name of the corresponding script in a vector
		tree->todo.push_back(pack->files[action-1]);
	}

	// undoing
	else 
	{+
		// set other things back
		tree->setButton(1, 0);
		//tree->setButton(pack->action+1, 1);
		pack->action = 0;

		// root notes (like network) return here
		if(!pack->path || pack->action==0)
			return ret; 
	
		// erase from todo list
		for(i=0; i<tree->todo.size(); i++)
			if(!strcmp(tree->todo[i], pack->files[pack->action-1])) // look for right entry
				tree->todo.erase(tree->todo.begin()+i); // delete it

		return ERR_OK;
	}

	return ret;
}

