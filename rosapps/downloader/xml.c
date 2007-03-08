/* PROJECT:         ReactOS Downloader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base\applications\downloader\xml.c
 * PURPOSE:         Parsing of application information xml files
 * PROGRAMMERS:     Maarten Bosma, Lester Kortenhoeven
 */

#include <libs/expat/expat.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#include "structures.h"
#include "resources.h"

BOOL TagOpen;
BOOL InstallScriptOpen;
BOOL UninstallScriptOpen;
struct Category* Current;
struct Application* CurrentApplication;
struct ScriptElement* CurrentScript;
char DML_Name[0x100];
char DML_Target[0x100];
char Path [0x100];
char CurrentTag [0x100];

extern WCHAR Strings [STRING_COUNT][MAX_STRING_LENGHT];
BOOL ImportXML (const char*);

void ImportFolder (const char* folder)
{
	WCHAR buffer[0x100];
	char buffer2[0x100];
	struct _wfinddata_t Finddata;
	DWORD Findhandle;
	buffer[0]='\0';
	strcpy(buffer2, Path);
	strncat(buffer2, folder, 0x100-strlen(buffer2));
	strncat(buffer2, "\\*.dml", 0x100-strlen(buffer2));
	MultiByteToWideChar(CP_UTF8, 0, buffer2, -1, buffer, 0x100);
	if((Findhandle=_wfindfirst(buffer, &Finddata)) == -1)
		return;
	do {
		buffer[0]='\0';
		MultiByteToWideChar(CP_UTF8, 0, folder, -1, buffer, 0x100);
		wcsncat(buffer, L"\\", 0x100-wcslen(buffer));
		wcsncat(buffer, Finddata.name, 0x100-wcslen(buffer));
		WideCharToMultiByte(CP_UTF8, 0, buffer, -1, buffer2, 0x100, NULL, FALSE);
		ImportXML(buffer2);
	} while(_wfindnext(Findhandle, &Finddata)==0);
	_findclose(Findhandle);
}


void Script_tag_opened (void* usrdata, const char* tag, const char** arg)
{
	int i;
	if (!strcmp(tag, "script")) {
		return;
	} else if (InstallScriptOpen && (CurrentScript == NULL)) {
		CurrentApplication->InstallScript = malloc(sizeof(struct ScriptElement));
		CurrentScript = CurrentApplication->InstallScript;
	} else if (UninstallScriptOpen && (CurrentScript == NULL)) {
		CurrentApplication->UninstallScript = malloc(sizeof(struct ScriptElement));
		CurrentScript = CurrentApplication->UninstallScript;
	} else if (CurrentScript != NULL) {
		CurrentScript->Next = malloc(sizeof(struct ScriptElement));
		CurrentScript = CurrentScript->Next;
	} else {
		return;
	}
	memset(CurrentScript, 0, sizeof(struct ScriptElement));
	if (!strcmp(tag, "download")) {
		wcscpy(CurrentScript->Func, L"download");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[1], 0x100);
			} else if(!strcmp(arg[i], "url")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else if (!strcmp(tag, "exec")) {
		wcscpy(CurrentScript->Func, L"exec");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else if (!strcmp(tag, "del")) {
		wcscpy(CurrentScript->Func, L"del");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else if (!strcmp(tag, "unzip")) {
		wcscpy(CurrentScript->Func, L"unzip");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			} else if(!strcmp(arg[i], "outdir")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[1], 0x100);
			}
		}
	} else if (!strcmp(tag, "adduninstaller")) {
		wcscpy(CurrentScript->Func, L"adduninstaller");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "regname")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			} else if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[1], 0x100);
			}
		}
	} else if (!strcmp(tag, "removeuninstaller")) {
		wcscpy(CurrentScript->Func, L"removeuninstaller");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "regname")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else if (!strcmp(tag, "message")) {
		wcscpy(CurrentScript->Func, L"message");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "text")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else if (!strcmp(tag, "load")) {
		wcscpy(CurrentScript->Func, L"load");
		for (i=0; arg[i]; i+=2) {
			if(!strcmp(arg[i], "file")) {
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentScript->Arg[0], 0x100);
			}
		}
	} else 
		MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
}


void tag_opened (void* usrdata, const char* tag, const char** arg)
{
	int i;

	if(!strcmp(tag, "import"))
	{
		for (i=0; arg[i]; i+=2) 
		{
			if(!strcmp(arg[i], "file"))
			{
				ImportXML(arg[i+1]);
			}
			else if(!strcmp(arg[i], "folder"))
			{
				ImportFolder(arg[i+1]);
			}
		}
	}
	else if(!strcmp(tag, "tree") && !CurrentApplication)
	{
		// check version
	}

	else if(!strcmp(tag, "category") && !CurrentApplication)
	{
		if (!Current)
		{
			Current = malloc(sizeof(struct Category));
			memset(Current, 0, sizeof(struct Category));
		}
		else if (TagOpen)
		{
			Current->Children = malloc(sizeof(struct Category));
			memset(Current->Children, 0, sizeof(struct Category));
			Current->Children->Parent = Current;
			Current = Current->Children;
		}
		else
		{
			Current->Next = malloc(sizeof(struct Category));
			memset(Current->Next, 0, sizeof(struct Category));
			Current->Next->Parent = Current->Parent;
			Current = Current->Next;
		}
		TagOpen = TRUE;

		for (i=0; arg[i]; i+=2) 
		{
			if(!strcmp(arg[i], "name"))
			{
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, Current->Name, 0x100);
			}
			if(!strcmp(arg[i], "icon"))
			{
				Current->Icon = atoi(arg[i+1]);
			}
		}
	}

	else if(!strcmp(tag, "application") && !CurrentApplication)
	{
		if(Current->Apps)
		{
			CurrentApplication = Current->Apps;
			while(CurrentApplication->Next)
				CurrentApplication = CurrentApplication->Next;
			CurrentApplication->Next = malloc(sizeof(struct Application));
			memset(CurrentApplication->Next, 0, sizeof(struct Application));
			CurrentApplication = CurrentApplication->Next;
		}
		else
		{
			Current->Apps = malloc(sizeof(struct Application));
			memset(Current->Apps, 0, sizeof(struct Application));
			CurrentApplication = Current->Apps;
		}

		for (i=0; arg[i]; i+=2) 
		{
			if(!strcmp(arg[i], "name"))
			{
				MultiByteToWideChar(CP_UTF8, 0, arg[i+1], -1, CurrentApplication->Name, 0x100);
			}
		}
	}
	else if (CurrentApplication)
	{
		if (!strcmp(tag, "installscript")) {
			InstallScriptOpen = TRUE;
		} else 	if (!strcmp(tag, "uninstallscript")) {
			UninstallScriptOpen = TRUE;
		} else {
			Script_tag_opened(usrdata, tag, arg);
			if (CurrentScript == NULL) { 
				strncpy(CurrentTag, tag, 0x100);
			}
		}
	}
	else
		MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
}


void text (void* usrdata, const char* data, int len)
{
	if (!CurrentApplication)
		return;

	if(!strcmp(CurrentTag, "maintainer"))
	{
		int currentlengt = lstrlenW(CurrentApplication->Maintainer);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->Maintainer[currentlengt], 0x100-currentlengt);
	}
	else if(!strcmp(CurrentTag, "regname"))
	{
		int currentlengt = lstrlenW(CurrentApplication->RegName);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->RegName[currentlengt], 0x100-currentlengt);
	}
	else if(!strcmp(CurrentTag, "description"))
	{
		int currentlengt = lstrlenW(CurrentApplication->Description);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->Description[currentlengt], 0x400-currentlengt);
	}
	else if(!strcmp(CurrentTag, "location"))
	{
		int currentlengt = lstrlenW(CurrentApplication->Location);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->Location[currentlengt], 0x100-currentlengt);
	}
	else if(!strcmp(CurrentTag, "version"))
	{
		int currentlengt = lstrlenW(CurrentApplication->Version);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->Version[currentlengt], 0x400-currentlengt);
	}
	else if(!strcmp(CurrentTag, "licence"))
	{
		int currentlengt = lstrlenW(CurrentApplication->Licence);
		MultiByteToWideChar(CP_UTF8, 0, data, len, &CurrentApplication->Licence[currentlengt], 0x100-currentlengt);
	}
}

void tag_closed (void* tree, const char* tag)
{
	CurrentTag[0] = 0;

	if(!strcmp(tag, "category"))
	{
		if (TagOpen)
		{
			TagOpen = FALSE;
		}
		else
		{
			Current = Current->Parent;
		}
	}
	else if(!strcmp(tag, "application"))
	{
		CurrentApplication = NULL;
	}
	else if(!strcmp(tag, "installscript") || !strcmp(tag, "uninstallscript"))
	{
		CurrentScript = NULL;
		InstallScriptOpen = FALSE;
		UninstallScriptOpen = FALSE;
	}
}

BOOL ImportXML (const char* filename)
{
	int done = 0;
	char buffer[0x100];
	FILE* file;
	XML_Parser parser;
	strcpy(buffer, Path);
	strncat(buffer, filename, 0x100-strlen(buffer));
	file = fopen(buffer, "r");
	if(!file) 
	{
		MessageBoxW(0,Strings[IDS_XMLERROR_1],0,0);
		return FALSE;
	}

	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, tag_opened, tag_closed);
	XML_SetCharacterDataHandler(parser, text);

	while (!done)
	{
		size_t len = fread (buffer, 1, sizeof(buffer), file);
		done = len < sizeof(buffer);

		buffer[len] = 0;
		if(!XML_Parse(parser, buffer, len, done)) 
		{
			MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
			return FALSE;
		}
	}

	XML_ParserFree(parser);
	fclose(file);

	return TRUE;
}

BOOL ProcessXML (const char* filename, struct Category* Root)
{
	FILE* file;
	file = fopen(filename, "r");
	if(file) 
	{
		Path[0]='\0';
		fclose(file);
	}
	else
	{
		strncpy(Path, getenv("SystemRoot"), 0x100-13);
		strcat(Path, "\\packagetree\\");
	}

	if(Current)
		return FALSE;

	Current = Root;
	CurrentApplication = NULL;
	CurrentScript = NULL;
	TagOpen = TRUE;
	InstallScriptOpen = FALSE;
	UninstallScriptOpen = FALSE;

	return ImportXML(filename);
}

void DML_tag_opened (void* usrdata, const char* tag, const char** arg)
{
	int i;

	if(!strcmp(tag, "application"))
	{
		for (i=0; arg[i]; i+=2) 
		{
			if(!strcmp(arg[i], "name"))
			{
				strncpy(DML_Name, arg[i+1], 0x100);
			}
			else if(!strcmp(arg[i], "target"))
			{
				strncpy(DML_Target, arg[i+1], 0x100);
			}
		}
	}
}

void NOP_text (void* usrdata, const char* data, int len)
{
}

void NOP_tag_closed (void* tree, const char* tag)
{
}

char* addDML (const char* filename)
{
	int done = 0;
	char buffer[0x100];
	FILE* file;
	XML_Parser parser;
	DML_Target[0] = '\0';
	file = fopen(filename, "r");
	if(!file) 
	{
		MessageBoxW(0,Strings[IDS_XMLERROR_1],0,0);
		return NULL;
	}

	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, DML_tag_opened, NOP_tag_closed);
	XML_SetCharacterDataHandler(parser, NOP_text);

	while (!done)
	{
		size_t len = fread (buffer, 1, sizeof(buffer), file);
		done = len < sizeof(buffer);

		buffer[len] = 0;
		if(!XML_Parse(parser, buffer, len, done)) 
		{
			MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
			return NULL;
		}
	}

	XML_ParserFree(parser);
	fclose(file);

	if(DML_Target[0]=='\0')
	{
		MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
		return NULL;
	}
	
	strcpy(buffer, getenv("SystemRoot"));
	strncat(buffer, "\\packagetree\\", 0x100-strlen(buffer));
	strncat(buffer, DML_Target, 0x100-strlen(buffer));

	CopyFileA(filename, buffer, FALSE);
	return DML_Name;
}

void LoadScriptFunc(WCHAR* filenameW, struct ScriptElement* Script)
{
	int done = 0;
	char buffer[0x100];
	char filenameA[0x100];
	FILE* file;
	XML_Parser parser;
	struct ScriptElement* NextElement = Script->Next;
	wcscpy(Script->Func,L"NOP");
	CurrentScript = Script;
	WideCharToMultiByte(CP_UTF8, 0, filenameW, -1, filenameA, 0x100, NULL, FALSE);
	strcpy(buffer, Path);
	strncat(buffer, filenameA, 0x100-strlen(buffer));
	file = fopen(buffer, "r");
	if(!file) 
	{
		MessageBoxW(0,Strings[IDS_XMLERROR_1],0,0);
		return;
	}

	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, Script_tag_opened, NOP_tag_closed);
	XML_SetCharacterDataHandler(parser, NOP_text);

	while (!done)
	{
		size_t len = fread (buffer, 1, sizeof(buffer), file);
		done = len < sizeof(buffer);

		buffer[len] = 0;
		if(!XML_Parse(parser, buffer, len, done)) 
		{
			MessageBoxW(0,Strings[IDS_XMLERROR_2],0,0);
			CurrentScript->Next = NextElement;
			return;
		}
	}

	XML_ParserFree(parser);
	fclose(file);
	CurrentScript->Next = NextElement;
	return;
}

void FreeScript (struct ScriptElement* Script)
{
	if (Script->Next != NULL)
		FreeScript(Script->Next);
	free(Script);
}

void FreeApps (struct Application* Apps)
{
	if (Apps->Next)
		FreeApps(Apps->Next);
	if (Apps->InstallScript)
		FreeScript(Apps->InstallScript);
	if (Apps->UninstallScript)
		FreeScript(Apps->UninstallScript);

	free(Apps);
}

void FreeTree (struct Category* Node)
{
	if (Node->Children)
		FreeTree(Node->Children);

	if (Node->Next)
		FreeTree(Node->Next);

	if (Node->Apps)
		FreeApps(Node->Apps);

	free(Node);
}
