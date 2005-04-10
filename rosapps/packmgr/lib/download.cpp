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
#include <wine/urlmon.h>

// Server there all the files lie
const char* tree_server = "http://maarten-online.de/xml/"; 

HRESULT WINAPI URLDownloadToFileA(      
    LPUNKNOWN pCaller,
    LPCSTR szURL,
    LPCSTR szFileName,
    DWORD dwReserved,
    LPBINDSTATUSCALLBACK lpfnCB
);


// Download a file
char* PML_Download (const char* name, const char* local_name = "packmgr.txt", const char* server = tree_server, BOOL totemp = TRUE) 
{
	char url [MAX_PATH];
	static char path [MAX_PATH]; 

	// get temp dir
	if(totemp)
		GetTempPathA (200, path);
	
	// create the local file name
	if(local_name)
		strcat(path, local_name);
	else
		strcat(path, "tmp.tmp"); 

	// get the url
	if(server) strcpy(url, server);
	strcat(url, name);
	
	// make sure there is no old file
	DeleteFileA (path);	

	// download the file
	if(URLDownloadToFileA (NULL, url, path, 0, NULL) != S_OK)
	{
		Log("!  ERROR: Unable to download ");
		LogAdd(url);

		return NULL;
	}

	return path;
}

// Download and prozess a xml file
int PML_XmlDownload (const char* url, void* usrdata, XML_StartElementHandler start, 
						 XML_EndElementHandler end, XML_CharacterDataHandler text) 
{
	char buffer[255];
	int done = 0;

	// logging
	Log("*  prozess the xml file: ");
	LogAdd(url);

	// download the file
	char* filename = PML_Download(url);

	if(!filename) 
	{
		Log("!  ERROR: Could not download the xml file");
		return ERR_DOWNL;
	}

	// open the file
	FILE* file = fopen(filename, "r");
	if(!file) 
	{
		Log("!  ERROR: Could not open the xml file");
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

