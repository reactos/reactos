////////////////////////////////////////////////////////
//
// script.cpp
// 
// Implementaion of a basic basic :) interpreter
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "package.hpp"
#include "script.h"
#include "log.h"
#include <fstream>

using namespace std;

// just a few Helpers
void Replace (string* Where, string Old, string New, int start = 0, int end = -1, int instring = 1);
int FindCount (string What, string Where, int start = 0, int end = -1);
int Find (string Where, string What, int start = 0, int end = -1, int instring = 1);


// Loads script from file, checks if it's synaxially correct
// and converts it into a easy to interprete one.
int RPS_Load (SCRIPT** script, const char* path)
{
	string source;

	/* We have to do it that way (doublepointer) because MinGw 
	   calls "delete" at the end of function otherwise. */
	(*script) = new SCRIPT;	

	// Load file to string
	ifstream file(path, ios_base::in);
	if (!file.is_open())
		return ERR_FILE;

	getline(file, source, '\0');

	// make sure last char is a new line
	source += "\n"; 
	
	// Are all subs and strings closed ?
	// FIXME: Just a quick hack sould be both checked line by line
	if(FindCount(source, "\"")%2) // if count is uneven not all strings are closed 
		return ERR_SYNATX;

	if(FindCount(source, "Sub ") != FindCount(source, "End Sub\n"))
		return ERR_SYNATX;

	// Delete comments
	while (true)
	{
		int start = Find(source, "'");
		if(start == NOTFOUND)
			break;
		int end = Find(source, "\n", start);
		source.erase(start, end-start); // needs size not line
	}

	// Converte the file into some thing easier to interprete
	Replace(&source, "(", " ");
	Replace(&source, ")", " ");
	Replace(&source, ";", " ");
	Replace(&source, ",", " ");
	Replace(&source, "\"", " \" ");
	Replace(&source, "\t", " ");

	Replace(&source, "  ", " ");
	Replace(&source, "\n ", "\n");
	Replace(&source, " \n", "\n");
	Replace(&source, "\n\n", "\n");

	if(source[0]=='\n')
		source.erase(0,1);

	// copy string into struct (line by line)
	UINT i, line=0;
	for (i=0; i < source.size(); i++)
	{
		// Make everything non capital letters
		if (source[i] >= 65 && source[i] <= 90) // ASCII-Code (A-Z 65-90)
		{
			source[i] += 32; // ASCII-Code (a-z 97-122)
		}

		else if (source[i] == '\"')
		{
			while(source[++i]!='\"');
		}

		else if (source[i] == '\n')
		{
			(*script)->code.push_back(source.substr(line, i-line));
			line = i+1;
		}
	}

	// create a sub table (with name, beginnig and end of function)
	for (i=0; i < (*script)->code.size(); i++) // code.size() is the cout of lines
	{
		SUB sub;
		
		if((*script)->code[i].substr(0,4) != "sub ")
			return ERR_SYNATX; // script has to start with sub

		sub.name = (*script)->code[i].substr(4,((*script)->code[i].size()-4));
		sub.start = i+1;

		while ((*script)->code[i] != "end sub")
		{
			i++;
			//if script does not end with "end sub" we got a problem
			if (i>(*script)->code.size())
				return ERR_SYNATX; 
		}

		sub.end = i;
		(*script)->subs.push_back(sub);
	}

	return ERR_OK;
}


// Executes a subroutine of the script
int RPS_Execute (SCRIPT* script, const char* function)
{
	char *argv[100];
	char *buffer;
	int a, b, c, nr = NOTFOUND, argc = 0;

	// find the right fuction
	for(a=0; (UINT)a<script->subs.size(); a++)
		if(script->subs[a].name == function)
			nr = a;

	// if there isn't a fuction with this name we can't do anything
	if(nr == NOTFOUND)
		return ERR_OK;

	// call the function
	for (a=script->subs[nr].start; a<script->subs[nr].end; a++)
	{
		// create a temporarry buffer 
		buffer = new char[script->code[a].size()];
		strcpy(buffer, script->code[a].c_str());

		// make the fist argument the function's name
		argv[0] = &buffer[0];
	
		int buffer_size = (int)strlen(buffer);
		for (b=0; b<buffer_size+1; b++)
		{
			// ignore chars in strings
			if(buffer[b]=='\"')
			{
				argv[argc] = &buffer[b+1];

				while(buffer[++b]!='\"');

				buffer[b] = '\0';
			}

			// create a new argument
			else if(buffer[b]==' ')
			{
				argc++;
				argv[argc] = &buffer[b+1];
				buffer[b] = '\0';

				// we don't want buffer overflows
				if(argc == 99) 
					return ERR_GENERIC;

			}

			// call the function
			else if(buffer[b]=='\0')
			{
				int error = 0;

				// log the name
				Log("*   excute command: ");
				for(c=0; c<argc+1; c++)
				{
					LogAdd(argv[c]); 
					LogAdd(" ");
				}

				for(c=0; c<FUNC_COUNT; c++)
					if(!strcmp(argv[0], FuncTable[c].name))
						error = FuncTable[c].function(argc, &argv[0]);

				if(error)
					return error;
			}

		}

		// start again with next line
		delete[] buffer;
		argc = 0;
	}

	return ERR_OK;
}

// get a Constant or a variavle
int RPS_getVar (const char* name)
{
	return ERR_OK;
}

// Clears up Memory
void RPS_Clear (SCRIPT* script)
{
	if(script)
		delete script;
}

/* Helper Functions */

// How often do we find a string inside another one
int FindCount (string where, string what, int start, int end)
{
	int counter = 0, pos;
	
	while(true)
	{
		pos = (int)where.find (what, start);
		//could could not be found or is outside of search area 
		if (pos == (int)string::npos || (end!=-1 && pos>end)) 
			break;
		start = pos+1;
		counter++;
	}

	return counter;
}

// Find (with only or not in Strings option)
int Find (string where, string what, int start, int end, int instring)
{
	int pos = (int)where.find (what, start);

	//could could not be found or is outside of search area 
	if (pos == (int)string::npos || (end!=-1 && pos>end)) 
		return -1;

	// if the count of this quotes is eaven we are in string 
	int isInString = FindCount(where, "\"", start, pos)%2;

	// if so we go on searching 
    if(isInString == instring)
		return Find (where, what, pos+1, end, instring);

	return pos;

}

// Replace (using Find)
void Replace (string* String, string Old, string New, int start, int end, int instring)
{
	int pos = start;

	while(true)
	{
		pos = Find(String->c_str(), Old, pos, end, instring);
		if (pos == -1)
			break;

		String->replace (pos, Old.length(), New);
	}
}
