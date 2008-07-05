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

#include <fstream>


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


	// !	!	!	F	I	X	M	E	!	!	! //
/*
int CreateOptions (TREE* tree)
{
	ofstream file ("options.xml");

	Log("* Creating options.xml from Resources");

	HRSRC hres = FindResource(GetModuleHandle(L"package"), MAKEINTRESOURCE(123), RT_RCDATA);
	if (!hres)
	{
		Log("! ERROR: Could not load it !");
		return ERR_GENERIC;
	}

	MessageBox(0,(WCHAR*)LockResource(LoadResource(NULL, hres)), 0, 0);	// is empty
	//file << (WCHAR*)LockResource(LoadResource(NULL, hres));

	return ERR_OK;
}
*/

char* PML_Download (pTree, const char* url, const char* server, const char* filename);

int CreateOptions (TREE* tree)
{
	Log("* Load options.xml from the Internet (Temporary Hack)");

	CopyFileA( PML_Download(tree, "http://svn.reactos.org/svn/*checkout*/reactos/trunk/rosapps/packmgr/lib/options.xml", NULL, "options.xml"), "options.xml", TRUE);

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
