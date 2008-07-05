////////////////////////////////////////////////////////
//
// tree.cpp
//
// Loading of the package tree
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "package.hpp"
#include "expat.h"

vector <int> parents;

int LoadOptions (TREE* tree);

void tree_end (void* tree, const char* tag);
void tree_start (void* usrdata, const char* tag, const char** arg);

int PML_XmlDownload (pTree tree, const char* file, void* usrdata, XML_StartElementHandler start,
											XML_EndElementHandler end, XML_CharacterDataHandler text=0);


// Load the tree
extern "C" int PML_LoadTree (TREE** tree, char* url, PML_AddItem AddItem)
{
	// get the memory
	(*tree) = new TREE;

	// set every to zero
	memset((*tree), 0, sizeof((*tree)));

	// set addItem callback
	(*tree)->addItem = AddItem;

	LoadOptions(*tree);

	return PML_XmlDownload (*tree, url, (void*)(*tree), tree_start, tree_end);
}

// expat callback for start of a "node" tag
void tree_start (void* usrdata, const char* tag, const char** arg)
{
	int i, icon = 0;
	static int id = 1;

	TREE* tree = (TREE*)usrdata;

	// ignore if tag is the root tag ("tree")
	if(!strcmp(tag, "tree"))
		return;

	// set the package memory
	tree->packages.resize(id+1);
	memset(&tree->packages[id], 0, sizeof(tree->packages[id]));

	tree->packages[id].loaded = FALSE;
	tree->packages[id].icon = FALSE;
	tree->packages[id].none = TRUE;
	tree->packages[id].path = NULL;
	tree->packages[id].name = "\0";

	// read the arguments
	for (i=0; arg[i]; i+=2)
	{
		if(!strcmp(arg[i], "name"))
		{
			tree->packages[id].name = new char [strlen(arg[i+1])+1];
			strcpy(tree->packages[id].name, arg[i+1]);
		}

		if(!strcmp(arg[i], "icon"))
		{
			icon = atoi(arg[i+1]);
			tree->packages[id].icon = TRUE;
		}

		if(!strcmp(arg[i], "file"))
		{
			tree->packages[id].path = new char [strlen(arg[i+1])+1];
			strcpy(tree->packages[id].path, arg[i+1]);

			if(strcmp(tag, "bin"))
				tree->packages[id].inst = TRUE;

			if(strcmp(tag, "src"))
				tree->packages[id].src_inst = TRUE;
		}
	}

	if(tree->packages[id].name[0] == '\0') return;

	// add it
	if(!parents.size())
	{
		if(tree->addItem)
			tree->addItem(id, tree->packages[id].name, 0, icon);
	}

	// do some manipulation at the parent
	else
	{
	   if(tree->addItem)
	       tree->addItem(id, tree->packages[id].name, parents.back(), icon);

	   // list as child in the parent node
	   tree->packages[parents.back()].children.push_back(id);

	   // this is for the buttons
	   tree->packages[parents.back()].inst = tree->packages[parents.back()].inst || tree->packages[id].inst;
	   tree->packages[parents.back()].src_inst = tree->packages[parents.back()].src_inst || tree->packages[id].src_inst;
	}

	parents.push_back(id++);
}

// expat callback for end of a "node" tag
void tree_end (void* tree, const char* tag)
{
	// delete last item
	parents.pop_back();
}

