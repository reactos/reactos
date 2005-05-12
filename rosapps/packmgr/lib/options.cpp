////////////////////////////////////////////////////////
//
// options.cpp
// 
// Settting and Loading Options 
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "package.hpp"
#include "log.h"
#include "expat.h"


int PML_XmlDownload (pTree tree, const char* url, void* usrdata,
						 XML_StartElementHandler start, XML_EndElementHandler end, XML_CharacterDataHandler text) ;


// expat callback for start of a "node" tag
void opt_start (void* usrdata, const char* tag, const char** arg)
{
	TREE* tree = (TREE*)usrdata;

	if (!strcmp(tag, "source"))
	{
		tree->sources.push_back((char*)NULL);
		tree->field = &tree->sources.back();
	}
}

// expat callback for end of a "node" tag
void opt_end (void* usrdata, const char* tag)
{
	TREE* tree = (TREE*)usrdata;

	tree->field = NULL;
}

// expat callback for end of a "node" tag
void opt_text (void* usrdata, const char* data, int len)
{
	TREE* tree = (TREE*)usrdata;

	if(!tree->field)
		return;

	*tree->field = new char[len+1];
	strncpy(*tree->field, data, len);
	(*tree->field)[len] = '\0';
}


int CreateOptions (TREE* tree)
{
//	string source;

//	ifstream file ("help.txt", ios_base::in);
	Log("* Creating options,xml");

	return ERR_OK;
}

int LoadOptions (TREE* tree)
{
	int error;

	error = PML_XmlDownload(tree, "file://options.xml", (void*)tree, opt_start, opt_end, opt_text);
	if(!error)
		return ERR_OK;

	CreateOptions(tree);
	return PML_XmlDownload(tree, "file://options.xml", (void*)tree, opt_start, opt_end, opt_text);
}
