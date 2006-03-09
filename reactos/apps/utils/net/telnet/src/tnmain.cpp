///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998  Paul Brannan
//Copyright (C) 1998  I.Ioannou
//Copyright (C) 1997  Brad Johnson
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//I.Ioannou
//roryt@hol.gr
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Module:		tnmain.cpp
//
// Contents:	telnet main program
//
// Product:		telnet
//
// Revisions: August 11, 1998	Thomas Briggs <tbriggs@qmetric.com>
//            May 14, 1998		Paul Brannan <pbranna@clemson.edu>
//            5.April.1997		jbj@nounname.com
//            5.Dec.1996		jbj@nounname.com
//            Version 2.0
//
//            02.Apr.1995		igor.milavec@uni-lj.si
//					  Original code
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <locale.h>
#include "tnmain.h"
#include "tnmisc.h"

int telCommandLine (Telnet &MyConnection);

void waitforkey() {
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD InputRecord;
	DWORD dwInput;
	BOOL done = FALSE;
	while (!done){
		WaitForSingleObject( hConsole, INFINITE );
		if (!ReadConsoleInput(hConsole, &InputRecord, 1, &dwInput)){
			done = TRUE;
			continue;
		}
		if (InputRecord.EventType == KEY_EVENT &&
			InputRecord.Event.KeyEvent.bKeyDown )
			done = TRUE;
	}
}

//char * cfgets ( char * buf, unsigned int length, char pszHistory[][80], int iHistLength){
struct cmdHistory * cfgets (char *buf, unsigned int length, struct cmdHistory *cmdhist) {

	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	unsigned int current=0, cursor =0, iEraseLength=0, i;
	char chr;
	char temp[2];
	char temp1[80];
	
	INPUT_RECORD InputRecord;
	BOOL done = FALSE;

	temp[1] = 0;
	buf[0] = '\0';

	if(!ini.get_input_redir()) {
		while (!done) {
			DWORD dwInput;
			int MustRefresh = 0;
			WaitForSingleObject( hConsole, INFINITE );
			if (!ReadConsoleInput(hConsole, &InputRecord, 1, &dwInput)){
				done = TRUE;
				continue;
			}
			MustRefresh = 0;
			if (InputRecord.EventType == KEY_EVENT &&
				InputRecord.Event.KeyEvent.bKeyDown ) {
				
				if(InputRecord.Event.KeyEvent.dwControlKeyState &
					(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
					
					switch(InputRecord.Event.KeyEvent.wVirtualKeyCode) {
					case 'D': // Thomas Briggs 8/11/98
						buf[0] = '\04';
						buf[1] = '\0';
						current = 1;
						done = true;
						continue;
					case 'U': // Paul Brannan 8/11/98
						buf[0] = '\0';
						current = 0;
						cursor = 0;
						MustRefresh = 1;
						break;
					}
				}
				
				switch (InputRecord.Event.KeyEvent.wVirtualKeyCode) {
				case VK_UP:
					// crn@ozemail.com.au
					if (cmdhist != NULL) {
						if (!strcmp(buf, ""))
							strncpy(buf, cmdhist->cmd, 79);
						else if (cmdhist->prev != NULL) {
							cmdhist = cmdhist->prev;
							strncpy(buf, cmdhist->cmd, 79);
						}
						current = strlen(buf);
					}
					///
					MustRefresh = 1;
					break;
				case VK_DOWN:
					// crn@ozemail.com.au
					if (cmdhist != NULL) {
						if (cmdhist->next != NULL) {
							cmdhist = cmdhist->next;
							strncpy(buf, cmdhist->cmd, 79);
						} else {
							strncpy(buf, "", 79);
						}
						current = strlen(buf);
					}
					///
					MustRefresh = 1;
					break;
				case VK_RIGHT:		//crn@ozemail.com.au (added ctrl+arrow)
					if (cursor < current)
						if (InputRecord.Event.KeyEvent.dwControlKeyState &
							(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
							unsigned int i,j;
							for (j = cursor; j <= current; j++)
								if (buf[j+1] == ' ' || (j+1)==current)
									break;
								for (i = ++j; i <= current; i++)
									if (buf[i] != ' ' || i == current) {
										cursor = i == current ? --i : i;
										break;
									}
						} else
							cursor++;
						MustRefresh = 1;
						break;
				case VK_LEFT:		//crn@ozemail.com.au (added ctrl+arrow)
					if (cursor > 0)
						if(InputRecord.Event.KeyEvent.dwControlKeyState &
							(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
							int i,j;
							for (j = cursor; j >= 0; j--)
								if (buf[j-1] != ' ')
									break;
								for (i = --j; i >= 0; i--)
									if (buf[i] == ' ' || i == 0) {
										cursor = !i ? i : ++i;
										break;
									}
						} else
							cursor--;
						MustRefresh = 1;
						break;
				case VK_HOME:
					if (cursor>0) cursor = 0;
					MustRefresh = 1;
					break;
				case VK_END:
					if (cursor<current) cursor = current;
					MustRefresh = 1;
					break;
				case VK_DELETE:
					if (current > 0 && current > cursor) {
						strcpy(&buf[cursor],&buf[cursor+1]);
						current--;
						buf[current] = 0;
						printit("\r");
						for (i = 0; i < current+strlen("telnet>")+1 ;i++)
							printit(" ");
					}
					MustRefresh = 1;
					break;
				case VK_BACK:
					if (cursor > 0 ) {
						strcpy(&buf[cursor-1],&buf[cursor]);
						current--;
						cursor--;
						buf[current] = 0;
						printit("\r");
						for (i = 0; i < current+strlen("telnet>")+1 ;i++)
							printit(" ");
					}
					MustRefresh = 1;
					break;
					
				default:
					chr = InputRecord.Event.KeyEvent.uChar.AsciiChar;
					if (chr == '\r') {
						done = TRUE;
						continue;
					}
					if (current >= length-1){
						done = TRUE;
						continue;
					}
					if ( isprint (chr) ){
						strncpy(temp1,&buf[cursor],79);
						strncpy(&buf[cursor+1],temp1,79-(cursor+1));
						buf[cursor++]=chr;
						current++;
						buf[current] = 0;
						MustRefresh = 1;
					}
					break;
				}
				if (MustRefresh == 1)
				{
					printit("\rtelnet");
					for (i = 0; i <= iEraseLength ;i++)
						printit(" ");
					printit("\rtelnet>");
					printit(buf);
					iEraseLength = strlen(buf);
					for (i = 0; i < current-cursor; i++)
						printit("\b");
				}
			}
		}
		buf[current] = 0;
		if (strcmp(buf, "")) {
			if (cmdhist == NULL) {
				cmdhist = new struct cmdHistory;
				if (cmdhist == NULL) {
					printit ("\nUnable to allocate memory for history buffer -- use the \"flush\" command to clear the buffer.\n");
					return cmdhist;
				}
				strncpy(cmdhist->cmd, buf, 79);
				cmdhist->next = NULL;
				cmdhist->prev = NULL;
			} else {
				while (cmdhist->next != NULL)	//  move to the end of the list
					cmdhist = cmdhist->next;
				cmdhist->next = new struct cmdHistory;
				if (cmdhist->next == NULL) {
					printit ("\nUnable to allocate memory for history buffer -- use the \"flush\" command to clear the buffer.\n");
					return cmdhist;
				}
				cmdhist->next->prev = cmdhist;	//  previous is where we are now
				cmdhist = cmdhist->next;
				strncpy(cmdhist->cmd, buf, 79);
				cmdhist->next = NULL;
			}
			while (cmdhist->next)
				cmdhist = cmdhist->next;
		}
		return cmdhist;
		///
	} else {
		WaitForSingleObject( hConsole, INFINITE );
		DWORD dwInput;
		DWORD OldMode;
		GetConsoleMode(hConsole, &OldMode);
		SetConsoleMode(hConsole,
			OldMode &~ (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT) );
		while (ReadFile(hConsole, &chr, 1, &dwInput, NULL)) {
			if (chr == '\r') {
				temp[0] = chr;
				printit(&temp[0]);
				break;
			}
			if (chr == '\b' && current > 0) {
				current--;
				printit("\b \b");
			}
			if (current >= length-1){
				break;
			}
			if ( isprint (chr) ){
				temp[0] = chr;
				printit(&temp[0]);
				buf[current++]=chr;
			}
		}
		buf[current] = 0;
		SetConsoleMode(hConsole, OldMode);
		return NULL;
	}
}

// AVS ** for fix bug in command 'keys load keymapname' without file
// static char keyfile[MAX_PATH*2];

int main(int ArgC, char* ArgV[]) {

	CONSOLE_SCREEN_BUFFER_INFO  ConsoleScreenBufferInfo;
	GetConsoleScreenBufferInfo(
		GetStdHandle(STD_OUTPUT_HANDLE),
		&ConsoleScreenBufferInfo
		);
	
	char *k;
	char startdir[MAX_PATH*2];
	char exename[MAX_PATH];
	
	// strncpy(startdir, ArgV[0],MAX_PATH);
	// This should be more accurate than using argv[0] (Paul Brannan 9/16/98)
	GetModuleFileName(NULL, startdir, sizeof(startdir));

	// Get the current console title so it can be set later
	// ("Pedro A. Aranda Gutiérrez" <paag@coppi.tid.es>)
	TCHAR ConsoleTitle[255];
	GetConsoleTitle(ConsoleTitle, sizeof(ConsoleTitle));
	
	k = strrchr(startdir, '\\');
	if (k == NULL){						// if the \ character is not found...
		strcpy(exename, startdir);
		strcpy(startdir,"");			// set the path to nothing
	} else {
		// end the string after the last '\' to get rid of the file name
		strcpy(exename, k+1);
		k[1] = 0;
	}

	printm(0, FALSE, MSG_COPYRIGHT);
	printm(0, FALSE, MSG_COPYRIGHT_1);
	
	// set up the ini class
	ini.init(startdir, exename);					

	// Process the command line arguments and connect to a host if necessary
	if(ini.Process_Params(ArgC, ArgV)) {
		const char *szHost = ini.get_host();
		const char *strPort = ini.get_port();
		if(!*szHost) {
			Telnet MyConnection;
			while(telCommandLine(MyConnection));
		} else {
			Telnet MyConnection;
			if(MyConnection.Open(szHost, strPort) == TNPROMPT) {
				// still connected
				printit("\n");
				telCommandLine(MyConnection);
			}
		}
	}
	//// (Paul Brannan 5/14/98)
	
	if(ini.get_term_width() != -1 || ini.get_term_height() != -1) {
		SetConsoleScreenBufferSize(
			GetStdHandle(STD_OUTPUT_HANDLE),	// handle of console screen buffer
			ConsoleScreenBufferInfo.dwSize		// new size in character rows and cols.
			);
		SetConsoleWindowInfo(
			GetStdHandle(STD_OUTPUT_HANDLE),	// handle of console screen buffer
			TRUE,								// coordinate type flag
			&ConsoleScreenBufferInfo.srWindow 	// address of new window rectangle
			);
	}
	SetConsoleTextAttribute(
		GetStdHandle(STD_OUTPUT_HANDLE),		// handle of console screen buffer
		ConsoleScreenBufferInfo.wAttributes 	// text and background colors
		);

	// Restore the original console title
	// ("Pedro A. Aranda Gutiérrez" <paag@coppi.tid.es>)
	SetConsoleTitle(ConsoleTitle);

	return 0;
}

// AVS
enum {
	BAD_USAGE = -3,
		EMPTY_LINE = -2,
		INVALID_CMD = -1,
		__FIRST_COMMAND = 0,
		
		OPEN = __FIRST_COMMAND,
		CLOSE,
		KEYS,
		QUIT,
		HELP,
		HELP2,				// there is way for synonims
		K_LOAD,				// subcommand of 'keys'
		K_SWITCH,			// subcommand of 'keys'
		K_DISPLAY,			// subcommand of 'keys'
		
		SET,				// Paul Brannan 5/30/98
		
		SUSPEND,
		FASTQUIT,			// Thomas Briggs 8/11/98
		CMD_HISTORY,		// crn@ozemail.com.au
		CLEAR_HISTORY,		// crn@ozemail.com.au
		
		ALIASES,			// Paul Brannan 1/1/99
		
		__COMMAND_LIST_SIZE	// must be last
};


struct command {
	char* cmd;				// command
	int   minLen,			// minimal length for match
		  minParms,			// minimal count of parms
		  maxParms;			// maximal -/- (negative disables)
	int   isSubCmd,			// is a subcommand - number of wich command
		  haveSubCmd;		// have subcommands? 0 or 1
	char* usage;			// text of usage
};

command cmdList[__COMMAND_LIST_SIZE] = {
	{"open",     1,	1,  2,	-1,		0,	"o[pen] host [port]\n"},
	{"close",    2,	0,  0,	-1,		0,	NULL},
	{"keys",     2,	1,  3,	-1,		1,	"ke[ys] l[oad] keymapname [file]\n"
										"ke[ys] d[isplay]\n"
										"ke[ys] s[witch] number\n"},
	// Ioannou : i change it to q, to be more compatible with unix telnet
	{"quit",     1,	0,  0,	-1,		0,	NULL}, // must type it exactly
	{"?",        1,	0,  0,	-1,		0,	NULL},
	{"help",     1,	0,  0,	-1,		0,	NULL},
	{"load",     1,	1,  2,	KEYS,	0,	NULL},
	{"switch",   1,	1,  1,	KEYS,	0,	NULL},
	{"display",  1,	0,  0,	KEYS,	0,	NULL},
	// Paul Brannan 5/30/98
	{"set",      3,	0,	2,	-1,		0,	"set will display available groups.\n"
										"set groupname will display all variables/values in a group.\n"
										"set [variable [value]] will set variable to value.\n"},
	// Thomas Briggs 8/11/98
	{"z",		1,	0,	0,	-1,		0,	"suspend telnet\n"},
	{"\04",		1,	0,	0,	-1,		0,	NULL},
	// crn@ozemail.com.au
	{"history",	2,	0,	0,	-1,	0,	"show command history"},
	{"flush",	2,	0,	0,	-1,	0,	"flush history buffer"},
	// Paul Brannan 1/1/99
	{"aliases",	5,	0,	0,	-1,		0,	NULL}
};

// a maximal count of parms
#define MAX_PARM_COUNT 3
#define MAX_TOKEN_COUNT (MAX_PARM_COUNT+2)

static int cmdMatch(const char* cmd, const char* token, int tokenLen, int minM) {
    if ( tokenLen < minM ) return 0;
	// The (unsigned) gets rid of a compiler warning (Paul Brannan 5/25/98)
    if ( (unsigned)tokenLen > strlen(cmd) ) return 0;
    if ( strcmp(cmd,token) == 0 ) return 1;
	
    int i;
    for ( i = 0; i < minM; i++ ) if ( cmd[i] != token[i] ) return 0;
	
    for ( i = minM; i < tokenLen; i++ ) if ( cmd[i] != token[i] ) return 0;
	
    return 1;
};

static void printUsage(int cmd) {
	if ( cmdList[cmd].usage != NULL ) {
		printit(cmdList[cmd].usage);
		return;
	};
	if ( cmdList[cmd].isSubCmd >= 0 ) {
		printUsage(cmdList[cmd].isSubCmd);
		return;
	   }
	   printm(0, FALSE, MSG_BADUSAGE);
};

int tokenizeCommand(char* szCommand, int& argc, char** argv) {
    char* tokens[MAX_TOKEN_COUNT];
    char* p;
    int   args = 0;

	if(!szCommand || !*szCommand) return EMPTY_LINE;

	// Removed strtok to handle tokens with spaces; this is handled with
	// quotes.  (Paul Brannan 3/18/99)
	char *token_start = szCommand;
	for(p = szCommand;; p++) {
		if(*p == '\"') {
			char *tmp = p;
			for(p++; *p != '\"' && *p != 0; p++);	// Find the next quote
			if(*p != 0) strcpy(p, p + 1);			// Remove quote#2
			strcpy(tmp, tmp + 1);					// Remove quote#1
		}
		if(*p == 0 || *p == ' ' || *p == '\t') {
			tokens[args] = token_start;
			args++;
			if(args >= MAX_TOKEN_COUNT) break;		// Break if too many args
			token_start = p + 1;
			if(*p == 0) break;
			*p = 0;
		}
	}
	// while ( (p = strtok((args?NULL:szCommand), " \t")) != NULL && args < MAX_TOKEN_COUNT ) {
	// 	tokens[args] = p;
	// 	args++;
	// };
	
    if ( !args ) return EMPTY_LINE;
    argc = args - 1;
    args = 0;
    int curCmd = -1;
    int ok = -1;
    while ( ok < 0 ) {
		int tokenLen = strlen(tokens[args]);
		int match = 0;
		for ( int i = 0; i<__COMMAND_LIST_SIZE; i++ ) {
			if ( cmdMatch(cmdList[i].cmd, tokens[args], tokenLen, cmdList[i].minLen) ) {
				if (argc < cmdList[i].minParms || argc > cmdList[i].maxParms) {
					printUsage(i);
					return BAD_USAGE;
				};
				if ( cmdList[i].haveSubCmd && curCmd == cmdList[i].isSubCmd) {
					curCmd = i;
					args++;
					argc--;
					match = 1;
					break;
				};
				if ( curCmd == cmdList[i].isSubCmd ) {
					ok = i;
					match = 1;
					break;
				};
				printUsage(i);
				return BAD_USAGE;
			};
		};
		if ( !match ) {
			if ( curCmd < 0 ) return INVALID_CMD;
			printUsage(curCmd);
			return -3;
		};
    };
	
    for ( int i = 0; i<argc; i++ ) {
        argv[i] = tokens[i+args+1];
    };
    return ok;
	
};

int telCommandLine (Telnet &MyConnection){
#define HISTLENGTH 25
	int i, retval;
	char* Parms[MAX_PARM_COUNT];
	char szCommand[80];
	int bDone = 0;
	char *extitle, *newtitle;
	struct cmdHistory *cmdhist;
	cmdhist = NULL;
	
	// printit("\n");  // crn@ozemail.com.au 14/12/98
	while (!bDone){
		// printit("\n"); // Paul Brannan 5/25/98
		printit( "telnet>");
		cmdhist = cfgets (szCommand, 79, cmdhist);
		printit( "\n");

		strlwr(szCommand);  // convert command line to lower
		// i = sscanf(szCommand,"%80s %80s %80s %80s", szCmd, szArg1, szArg2, szArg3);
		switch ( tokenizeCommand(szCommand, i, Parms) ) {
		case BAD_USAGE:   break;
		case EMPTY_LINE:  
			if(MyConnection.Resume() == TNPROMPT) {
				printit("\n");
				break;
			}
			else
			 	return 1;
		case INVALID_CMD:
			printm(0, FALSE, MSG_INVCMD);
			break;			
		case OPEN:
			if (i == 1)
				retval = MyConnection.Open(Parms[0], "23");
			else
				retval = MyConnection.Open(Parms[0], Parms[1]);
			if(retval != TNNOCON && retval != TNPROMPT) return 1;
			if(retval == TNPROMPT) printit("\n");
			break;
		case CLOSE:
			MyConnection.Close();
			break;
		case FASTQUIT: // Thomas Briggs 8/11/98
		case QUIT:
			MyConnection.Close();
			bDone = 1;
			break;
		case HELP:
		case HELP2:
			printm(0, FALSE, MSG_HELP);
			printm(0, FALSE, MSG_HELP_1);
			break;
			// case KEYS: we should never get it
		case K_LOAD:
			if ( i == 1 ) {
				// Ioannou : changed to ini.get_keyfile()
				if(MyConnection.LoadKeyMap( ini.get_keyfile(), Parms[0]) != 1)
					printit("Error loading keymap.\n");
				break;
			};
			if(MyConnection.LoadKeyMap( Parms[1], Parms[0]) != 1)
				printit("Error loading keymap.\n");
			break;
		case K_DISPLAY:
			MyConnection.DisplayKeyMap();
			break;
		case K_SWITCH:
			MyConnection.SwitchKeyMap(atoi(Parms[0]));
			break;
			
			// Paul Brannan 5/30/98
		case SET:
			if(i == 0) {
				printit("Available groups:\n");		// Print out groups
				ini.print_groups();					// (Paul Brannan 9/3/98)
			} else if(i == 1) {
				ini.print_vars(Parms[0]);
			} else if(i >= 2) {
				ini.set_value(Parms[0], Parms[1]);
				// FIX ME !!! Ioannou: here we must call the parser routine for
				// wrap line, not the ini.set_value
				//  something like Parser.ConLineWrap(Wrap_Line);
			}
			break;
			
		case SUSPEND: // Thomas Briggs 8/11/98
			
			// remind the user we're suspended -crn@ozemail.com.au 15/12/98
			extitle = new char[128];
			GetConsoleTitle (extitle, 128);
			
			newtitle = new char[128+sizeof("[suspended]")];
			strcpy(newtitle, extitle);
			strncat(newtitle, "[suspended]", 128+sizeof("[suspended]"));
			if(ini.get_set_title()) SetConsoleTitle (newtitle);
			delete[] newtitle;
			
			if (getenv("comspec") == NULL) {
				switch (GetWin32Version()) {
				case 2:		// 'cmd' is faster than 'command' in NT -crn@ozemail.com.au
					system ("cmd");
					break;
				default:
					system ("command");
					break;
				}
			} else {
				system(getenv("comspec"));
			}
			
			if(ini.get_set_title()) SetConsoleTitle (extitle);
			delete[] extitle;
			///
			
			break;
			
		case CMD_HISTORY:	//crn@ozemail.com.au
			if (cmdhist != NULL) {
				while (cmdhist->prev != NULL)
					cmdhist = cmdhist->prev;	//rewind
				printf ("Command history:\n");
				while (1) {
					printf ("\t%s\n", cmdhist->cmd);
					
					if (cmdhist->next != NULL)
						cmdhist = cmdhist->next;
					else
						break;
				}
			} else
				printf ("No command history available.\n");
			
			break;
			
		case CLEAR_HISTORY:	//crn@ozemail.com.au
			if (cmdhist != NULL) {
				while (cmdhist->next != NULL)
					cmdhist = cmdhist->next;	//fast forward
				while (cmdhist->prev != NULL) {
					cmdhist = cmdhist->prev;
					delete cmdhist->next;
				}
				delete cmdhist;
				cmdhist = NULL;
				printf ("Command history cleared.\n");
			} else
				printf ("No command history available.\n");
			
		case ALIASES: // Paul Brannan 1/1/99
			ini.print_aliases();
			break;
			
		default: // paranoik
			printm(0, FALSE, MSG_INVCMD);
			break;
		}

	}

	return 0;
}
