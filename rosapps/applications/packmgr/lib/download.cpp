////////////////////////////////////////////////////////
//
// download.cpp
//
// Stuff related to downloading
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////

#include "package.hpp"
#include "expat.h"
#include "log.h"
#include <urlmon.h>

HRESULT WINAPI URLDownloadToFileA(
    LPUNKNOWN pCaller,
    LPCSTR szURL,
    LPCSTR szFileName,
    DWORD dwReserved,
    LPBINDSTATUSCALLBACK lpfnCB
);

int FindCount (string What, string Where, int start = 0, int end = -1);


// Download a file
char* PML_Download (pTree tree, const char* url, const char* server = "tree", const char* filename = "packmgr.xml")
{
	UINT i;
	static char downl [MAX_PATH]; // the full url
	static char path [MAX_PATH]; // the full resulting Path

	// It goes to the temp folder when no other path is entered (or even compleatly no filename)
	// If server == "tree" it will be downloaded from the server speficied in option.xml
	// File:// links are possible too

	// get temp dir
	if(!filename)
		GetTempPathA (200, path);

	else if(!strstr(filename, "\\"))
		GetTempPathA (200, path);

	else
		strcpy(path, "");


	// create the local file name
	if(filename)
	{
		strcat(path, filename);
		DeleteFileA (path);
	}
	else
		GetTempFileNameA (path, "pml", 1, path);

	// get the url
	if (!server)
		strcpy(downl, "");

	else if(!strcmp(server, "tree"))
	{
		char* ret;
		for (i=0; i<tree->sources.size(); i++)
		{
			ret = PML_Download(tree, url, tree->sources[i], filename);
			if(ret)
				return ret;
		}
		return NULL;
	}

	else
		strcpy(downl, server);

	strcat(downl, url);

	// is this a file link ?
	if (strstr(downl, "file://") || strstr(downl, "File://"))
	{
		if(!filename)
		{
			return &downl[7];
		}

		else
		{
			CopyFileA(filename, &downl[7], FALSE);
			return (char*)filename;
		}
	}


	// download the file
	if(URLDownloadToFileA (NULL, downl, path, 0, NULL) != S_OK)
	{
		Log("!  ERROR: Unable to download ");
		LogAdd(downl);

		return NULL;
	}

	return path;
}

// Download and prozess a xml file
int PML_XmlDownload (pTree tree, const char* url, void* usrdata,
						 XML_StartElementHandler start, XML_EndElementHandler end, XML_CharacterDataHandler text)
{
	int done = 0;
	char buffer[255];
	char* filename = 0;

	// logging
	Log("*  prozess the xml file: ");
	LogAdd(url);

	// download the file
	if(strstr(url, "file://"))
		filename = PML_Download(tree, url, NULL, NULL);

	else
		filename = PML_Download(tree, url);


	if(!filename)
	{
		Log("!  ERROR: Could not download the xml file");
		return ERR_DOWNL;
	}

	// open the file
	FILE* file = fopen(filename, "r");
	if(!file)
	{
		Log("!  ERROR: Could not open the xml file ");
		LogAdd(filename);
		return ERR_GENERIC;
	}

	// parse the xml file
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData (parser, usrdata);
	XML_SetElementHandler(parser, start, end);
	XML_SetCharacterDataHandler(parser, text);

	while (!done)
	{
		size_t len = fread (buffer, 1, sizeof(buffer), file);
		done = len < sizeof(buffer);

		buffer[len] = 0;
		if(!XML_Parse(parser, buffer, len, done))
		{
			Log("!  ERROR: Could not parse the xml file");
			return ERR_GENERIC;
		}
	}

	XML_ParserFree(parser);
	fclose(file);

	return ERR_OK;
}

