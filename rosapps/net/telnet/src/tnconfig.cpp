///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998-2000 Paul Brannan
//Copyright (C) 1998 I.Ioannou
//Copyright (C) 1997 Brad Johnson
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

// tnconfig.cpp
// Written by Paul Brannan <pbranna@clemson.edu>
// Last modified August 30, 1998
//
// This is a class designed for use with Brad Johnson's Console Telnet
// see the file tnconfig.h for more information

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <memory.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tnconfig.h"

// Turn off the "forcing value to bool 'true' or 'false'" warning
#ifdef _MSC_VER
#pragma warning(disable: 4800)
#endif

// This is the ini variable that is used for everybody
TConfig ini;

TConfig::TConfig() {
	// set all default values
	startdir[0] = '\0';
	keyfile[0] = '\0';
	inifile[0] = '\0';
	dumpfile[0] = '\0';
	term[0] = '\0';
	default_config[0] = '\0';
	strcpy(printer_name, "LPT1");

	input_redir = 0;
	output_redir = 0;
	strip_redir = FALSE;

	dstrbksp = FALSE;
	eightbit_ansi = FALSE;
	vt100_mode = FALSE;
	disable_break = FALSE;
	speaker_beep = TRUE;
	do_beep = TRUE;
	preserve_colors = FALSE;
	wrapline = TRUE;
	lock_linewrap = FALSE;
	fast_write = TRUE;
	enable_mouse = TRUE;
	alt_erase = FALSE;
	wide_enable = FALSE;
	keyboard_paste = FALSE;
	set_title = TRUE;

	blink_bg = -1;
	blink_fg = 2;
	underline_bg = -1;
	underline_fg = 3;
	ulblink_bg = -1;
	ulblink_fg = 1;
	normal_bg = -1;
	normal_fg = -1;
	scroll_bg = 0;
	scroll_fg = 7;
	status_bg = 1;
	status_fg = 15;

	buffer_size = 2048;

	term_width = -1;
	term_height = -1;
	window_width = -1;
	window_height = -1;

	strcpy(escape_key, "]");
	strcpy(scrollback_key, "[");
	strcpy(dial_key, "\\");
	strcpy(default_config, "ANSI");
	strcpy(term, "ansi");

	strcpy(scroll_mode, "DUMP");
	scroll_size=32000;
	scroll_enable=TRUE;

	host[0] = '\0';
	port = "23";

	init_varlist();

	aliases = NULL;
}

TConfig::~TConfig() {
	if(aliases) {
		for(int j = 0; j < alias_total; j++) delete[] aliases[j];
		delete[] aliases;
	}
}

enum ini_data_type {
	INI_STRING,
	INI_INT,
	INI_BOOL
};

enum {
	INIFILE,
	KEYFILE,
	DUMPFILE,
	DEFAULT_CONFIG,
	TERM,
	INPUT_REDIR,
	OUTPUT_REDIR,
	STRIP_REDIR,
	DSTRBKSP,
	EIGHTBIT_ANSI,
	VT100_MODE,
	DISABLE_BREAK,
	SPEAKER_BEEP,
	DO_BEEP,
	PRESERVE_COLORS,
	WRAP_LINE,
	LOCK_LINEWRAP,
	FAST_WRITE,
	TERM_WIDTH,
	TERM_HEIGHT,
	WINDOW_WIDTH,
	WINDOW_HEIGHT,
	WIDE_ENABLE,
	CTRLBREAK_AS_CTRLC,
	BUFFER_SIZE,
	SET_TITLE,
	BLINK_BG,
	BLINK_FG,
	UNDERLINE_BG,
	UNDERLINE_FG,
	ULBLINK_BG,
	ULBLINK_FG,
	NORMAL_BG,
	NORMAL_FG,
	SCROLL_BG,
	SCROLL_FG,
	STATUS_BG,
	STATUS_FG,
	PRINTER_NAME,
	ENABLE_MOUSE,
	ESCAPE_KEY,
	SCROLLBACK_KEY,
	DIAL_KEY,
	ALT_ERASE,
	KEYBOARD_PASTE,
	SCROLL_MODE,
	SCROLL_SIZE,
	SCROLL_ENABLE,
	SCRIPTNAME,
	SCRIPT_ENABLE,
	NETPIPE,
	IOPIPE,

	MAX_INI_VARS			// must be last
};

struct ini_variable {
	char *name;				// variable name
	char *section;			// name of ini file section the variable is in
	enum ini_data_type data_type;		// type of data
	void *ini_data;			// pointer to data
	int  max_size;			// max size if string
};

// Note: default values are set in the constructor, TConfig()
ini_variable ini_varlist[MAX_INI_VARS];

enum {
	KEYBOARD,
	TERMINAL,
	COLORS,
	MOUSE,
	PRINTER,
	SCROLLBACK,
	SCRIPTING,
	PIPES,

	MAX_INI_GROUPS						// Must be last
};

char *ini_groups[MAX_INI_GROUPS];

void TConfig::init_varlist() {
	static const ini_variable static_ini_varlist[MAX_INI_VARS] = {
		{"Inifile",		NULL,			INI_STRING,		&inifile,	sizeof(inifile)},
		{"Keyfile",		"Keyboard",		INI_STRING,		&keyfile,	sizeof(keyfile)},
		{"Dumpfile",	"Terminal",		INI_STRING,		&dumpfile,	sizeof(dumpfile)},
		{"Default_Config","Keyboard",   INI_STRING,		&default_config, sizeof(default_config)},
		{"Term",		"Terminal",		INI_STRING,		&term,		sizeof(term)},
		{"Input_Redir",	"Terminal",		INI_INT,		&input_redir, 0},
		{"Output_Redir","Terminal",		INI_INT,		&output_redir, 0},
		{"Strip_Redir",	"Terminal",		INI_BOOL,		&strip_redir, 0},
		{"Destructive_Backspace","Terminal",INI_BOOL,	&dstrbksp, 0},
		{"EightBit_Ansi","Terminal",	INI_BOOL,		&eightbit_ansi, 0},
		{"VT100_Mode",	"Terminal",		INI_BOOL,		&vt100_mode, 0},
		{"Disable_Break","Terminal",	INI_BOOL,		&disable_break, 0},
		{"Speaker_Beep","Terminal",		INI_BOOL,		&speaker_beep, 0},
		{"Beep",		"Terminal",		INI_BOOL,		&do_beep, 0},
		{"Preserve_Colors","Terminal",	INI_BOOL,		&preserve_colors, 0},
		{"Wrap_Line",	"Terminal",		INI_BOOL,		&wrapline, 0},
		{"Lock_linewrap","Terminal",	INI_BOOL,		&lock_linewrap, 0},
		{"Fast_Write",	"Terminal",		INI_BOOL,		&fast_write, 0},
		{"Term_Width",	"Terminal",		INI_INT,		&term_width, 0},
		{"Term_Height",	"Terminal",		INI_INT,		&term_height, 0},
		{"Window_Width","Terminal",		INI_INT,		&window_width, 0},
		{"Window_Height","Terminal",	INI_INT,		&window_height, 0},
		{"Wide_Enable",	"Terminal",		INI_BOOL,		&wide_enable, 0},
		{"Ctrlbreak_as_Ctrlc","Keyboard", INI_BOOL,		&ctrlbreak_as_ctrlc, 0},
		{"Buffer_Size",	"Terminal",		INI_INT,		&buffer_size, 0},
		{"Set_Title",	"Terminal",		INI_BOOL,		&set_title, 0},
		{"Blink_bg",	"Colors",		INI_INT,		&blink_bg, 0},
		{"Blink_fg",	"Colors",		INI_INT,		&blink_fg, 0},
		{"Underline_bg","Colors",		INI_INT,		&underline_bg, 0},
		{"Underline_fg","Colors",		INI_INT,		&underline_fg, 0},
		{"UlBlink_bg",	"Colors",		INI_INT,		&ulblink_bg, 0},
		{"UlBlink_fg",	"Colors",		INI_INT,		&ulblink_fg, 0},
		{"Normal_bg",	"Colors",		INI_INT,		&normal_bg, 0},
		{"Normal_fg",	"Colors",		INI_INT,		&normal_fg, 0},
		{"Scroll_bg",	"Colors",		INI_INT,		&scroll_bg, 0},
		{"Scroll_fg",	"Colors",		INI_INT,		&scroll_fg, 0},
		{"Status_bg",	"Colors",		INI_INT,		&status_bg, 0},
		{"Status_fg",	"Colors",		INI_INT,		&status_fg,	0},
		{"Enable_Mouse","Mouse",		INI_BOOL,		&enable_mouse, 0},
		{"Printer_Name","Printer",		INI_STRING,		&printer_name, sizeof(printer_name)},
		{"Escape_Key",	"Keyboard",		INI_STRING,		&escape_key, 1},
		{"Scrollback_Key","Keyboard",	INI_STRING,		&scrollback_key, 1},
		{"Dial_Key",	"Keyboard",		INI_STRING,		&dial_key, 1},
		{"Alt_Erase",	"Keyboard",		INI_BOOL,		&alt_erase, 0},
		{"Keyboard_Paste","Keyboard",	INI_BOOL,		&keyboard_paste, 0},
		{"Scroll_Mode",	"Scrollback",	INI_STRING,		&scroll_mode, sizeof(scroll_mode)},
		{"Scroll_Size",	"Scrollback",	INI_INT,		&scroll_size, 0},
		{"Scroll_Enable","Scrollback",	INI_BOOL,		&scroll_enable, 0},
		{"Scriptname",	"Scripting",	INI_STRING,		&scriptname, sizeof(scriptname)},
		{"Script_enable","Scripting",	INI_BOOL,		&script_enable, 0},
		{"Netpipe",		"Pipes",		INI_STRING,		&netpipe, sizeof(netpipe)},
		{"Iopipe",		"Pipes",		INI_STRING,		&iopipe, sizeof(iopipe)}
	};

	static const char *static_ini_groups[MAX_INI_GROUPS] = {
		"Keyboard",
		"Terminal",
		"Colors",
		"Mouse",
		"Printer",
		"Scrollback",
		"Scripting",
		"Pipes"
	};

	memcpy(ini_varlist, static_ini_varlist, sizeof(ini_varlist));
	memcpy(ini_groups, static_ini_groups, sizeof(ini_groups));
}

void TConfig::init(char *dirname, char *execname) {
	// Copy temporary dirname to permanent startdir
	strncpy(startdir, dirname, sizeof(startdir));
	startdir[sizeof(startdir) - 1] = 0;

	// Copy temp execname to permanent exename (Thomas Briggs 12/7/98)
	strncpy(exename, execname, sizeof(exename));
	exename[sizeof(exename) - 1] = 0;

	// Initialize INI file
	inifile_init();

	// Initialize redir
	// Note that this must be done early, so error messages will be printed
	// properly
	redir_init();

	// Initialize aliases (Paul Brannan 1/1/99)
	init_aliases();

	// Make sure the file that we're trying to work with exists
	int iResult = access(inifile, 04);

	// Thomas Briggs 9/14/98
	if( iResult == 0 )
		// Tell the user what file we are reading
		// We cannot print any messages before initializing telnet_redir
		printm(0, FALSE, MSG_CONFIG, inifile);
	else
		// Tell the user that the file doesn't exist, but later read the
		// file anyway simply to populate the defaults
		printm(0, FALSE, MSG_NOINI, inifile);

	init_vars();								// Initialize misc. vars
	keyfile_init();								// Initialize keyfile
}

// Alias support (Paul Brannan 1/1/99)
void TConfig::init_aliases() {
	char *buffer;
	alias_total = 0;

	// Find the correct buffer size
	// FIX ME!! some implementations of Mingw32 don't have a
	// GetPrivateProfileSecionNames function.  What do we do about this?
#ifndef __MINGW32__
	{
		int size=1024, Result = 0;
		for(;;) {
			buffer = new char[size];
			Result = GetPrivateProfileSectionNames(buffer, size, inifile);
			if(Result < size - 2) break;
			size *= 2;
			delete[] buffer;
		}
	}
#else
	return;
#endif

	// Find the maximum number of aliases
	int max = 0;
	char *tmp;
	for(tmp = buffer; *tmp != 0; tmp += strlen(tmp) + 1)
		max++;

	aliases = new char*[max];

	// Load the aliases into an array
	for(tmp = buffer; *tmp != 0; tmp += strlen(tmp) + 1) {
		int flag = 0;
		for(int j = 0; j < MAX_INI_GROUPS; j++) {
			if(!stricmp(ini_groups[j], tmp)) flag = 1;
		}
		if(!flag) {
			aliases[alias_total] = new char[strlen(tmp)+1];
			strcpy(aliases[alias_total], tmp);
			alias_total++;
		}
	}

	delete[] buffer;
}

void TConfig::print_aliases() {
	for(int j = 0; j < alias_total; j++) {
		char alias_name[20];
		set_string(alias_name, aliases[j], sizeof(alias_name));
		for(unsigned int i = strlen(alias_name); i < sizeof(alias_name) - 1; i++)
			alias_name[i] = ' ';
		alias_name[sizeof(alias_name) - 1] = 0;
		printit(alias_name);
		if((j % 4) == 3) printit("\n");
	}
	printit("\n");
}

bool find_alias(const char *alias_name) {
	return false;
}

void TConfig::print_vars() {
	int j;
	for(j = 0; j < MAX_INI_VARS; j++) {
		if(print_value(ini_varlist[j].name) > 40) printit("\n");
		else if(j % 2) printit("\n");
		else printit("\t");
	}
	if(j % 2) printit("\n");
}

// Paul Brannan 9/3/98
void TConfig::print_vars(char *s) {
	if(!strnicmp(s, "all", 3)) {					// Print out all vars
		print_vars();
		return;
	}

	// See if the group exists
	int j, flag;
	for(j = 0, flag = 0; j < MAX_INI_GROUPS; j++)
		if(!stricmp(ini_groups[j], s)) break;
	// If not, print out the value of the variable by that name
	if(j == MAX_INI_GROUPS) {
		print_value(s);
		printit("\n");
		return;
	}
	
	// Print out the vars in the given group
	int count = 0;
	for(j = 0; j < MAX_INI_VARS; j++) {
		if(ini_varlist[j].section == NULL) continue;
		if(!stricmp(ini_varlist[j].section, s)) {
			if(print_value(ini_varlist[j].name) > 40) printit("\n");
			else if(count % 2) printit("\n");
			else printit("\t");
			count++;
		}
	}
	if(count % 2) printit("\n");
}

// Paul Brannan 9/3/98
void TConfig::print_groups() {
	for(int j = 0; j < MAX_INI_GROUPS; j++) {
		char group_name[20];
		set_string(group_name, ini_groups[j], sizeof(group_name));
		for(unsigned int i = strlen(group_name); i < sizeof(group_name) - 1; i++)
			group_name[i] = ' ';
		group_name[sizeof(group_name) - 1] = 0;
		printit(group_name);
		if((j % 4) == 3) printit("\n");
	}
	printit("\n");
}

// Ioannou : The index in the while causes segfaults if there is no match
// changes to for(), and strcmp to stricmp (prompt gives rong names)

bool TConfig::set_value(const char *var, const char *value) {
   //int j = 0;
   //while(strcmp(var, ini_varlist[j].name) && j < MAX_INI_VARS) j++;
   for (int j = 0; j < MAX_INI_VARS; j++)
   {
      if (stricmp(var, ini_varlist[j].name) == 0)
      {
         switch(ini_varlist[j].data_type) {
            case INI_STRING:
               set_string((char *)ini_varlist[j].ini_data, value,
                  ini_varlist[j].max_size);
               break;
            case INI_INT:
               *(int *)ini_varlist[j].ini_data = atoi(value);
               break;
            case INI_BOOL:
               set_bool((bool *)ini_varlist[j].ini_data, value);
               break;
         }
         // j = MAX_INI_VARS;
		 return TRUE;
      }
   }
   return FALSE;
}

int TConfig::print_value(const char *var) {
	//int j = 0;
	//while(strcmp(var, ini_varlist[j].name) && j < MAX_INI_VARS) j++;
	int Result = 0;
	for (int j = 0; j < MAX_INI_VARS; j++)
	{
		if (stricmp(var, ini_varlist[j].name) == 0)
		{
			char var_name[25];
			set_string(var_name, var, sizeof(var_name));
			for(unsigned int i = strlen(var_name); i < sizeof(var_name) - 1; i++)
				var_name[i] = ' ';
			var_name[sizeof(var_name) - 1] = 0;
			Result = sizeof(var_name);

			printit(var_name);
			printit("\t");
			Result = Result / 8 + 8;
			
			switch(ini_varlist[j].data_type) {
            case INI_STRING:
				printit((char *)ini_varlist[j].ini_data);
				Result += strlen((char *)ini_varlist[j].ini_data);
				break;
            case INI_INT:
				char buffer[20]; // this may not be safe
				// Ioannou : Paul this was _itoa, but Borland needs itoa !!
				itoa(*(int *)ini_varlist[j].ini_data, buffer, 10);
				printit(buffer);
				Result += strlen(buffer);
				break;
            case INI_BOOL:
				if(*(bool *)ini_varlist[j].ini_data == true) {
					printit("on");
					Result += 2;
				} else {
					printit("off");
					Result += 3;
				}
			}
			// printit("\n");
			j = MAX_INI_VARS;
		}
	}
	return Result;
}

void TConfig::init_vars() {
	char buffer[4096];
	for(int j = 0; j < MAX_INI_VARS; j++) {
		if(ini_varlist[j].section != NULL) {
			GetPrivateProfileString(ini_varlist[j].section, ini_varlist[j].name, "",
				buffer, sizeof(buffer), inifile);
			if(*buffer != 0) set_value(ini_varlist[j].name, buffer);
		}
	}
}

void TConfig::inifile_init() {
	// B. K. Oxley 9/16/98	
	char* env_telnet_ini = getenv (ENV_TELNET_INI);
	if (env_telnet_ini && *env_telnet_ini) {
		strncpy (inifile, env_telnet_ini, sizeof(inifile));
		return;
	}

	strcpy(inifile, startdir);
	if (sizeof(inifile) >= strlen(inifile)+strlen("telnet.ini")) {
		strcat(inifile,"telnet.ini"); // add the default filename to the path
	} else {
		// if there is not enough room set the path to nothing
		strcpy(inifile,"");
	}
}

void TConfig::keyfile_init() {
	// check to see if there is a key config file environment variable.
	char *k;
	if ((k = getenv(ENV_TELNET_CFG)) == NULL){
		// if there is no environment variable
		GetPrivateProfileString("Keyboard", "Keyfile", "", keyfile,
			sizeof(keyfile), inifile);
		if(keyfile == 0 || *keyfile == 0) {
			// and there is no profile string
			strcpy(keyfile, startdir);
			if (sizeof(keyfile) >= strlen(keyfile)+strlen("telnet.cfg")) {
				struct stat buf;

				strcat(keyfile,"telnet.cfg"); // add the default filename to the path
				if(stat(keyfile, &buf) != 0) {
					char *s = keyfile + strlen(keyfile) - strlen("telnet.cfg");
					strcpy(s, "keys.cfg");
				}
			} else {
				// if there is not enough room set the path to nothing
				strcpy(keyfile,"");
			}

		// Vassili Bourdo (vassili_bourdo@softhome.net)
		} else {
			// check that keyfile really exists
			if( access(keyfile,04) == -1 ) {
				//it does not...
				char pathbuf[MAX_PATH], *fn;
				//substitute keyfile path with startdir path
				if((fn = strrchr(keyfile,'\\'))) strcpy(keyfile,fn);
					strcat(strcpy(pathbuf,startdir),keyfile);
				//check that startdir\keyfile does exist
				if( access(pathbuf,04) == -1 ) {
					//it does not...
					//so, look for it in all paths
					_searchenv(keyfile, "PATH", pathbuf);
					if( *pathbuf == 0 ) //no luck - revert it to INI file value
						GetPrivateProfileString("Keyboard", "Keyfile", "",
							keyfile, sizeof(keyfile), inifile);
				} else {
					strcpy(keyfile, pathbuf);
				}
			}
		}
		////

	} else {
		// set the keyfile to the value of the environment variable
		strncpy(keyfile, k, sizeof(keyfile));
	}
}

void TConfig::redir_init() {
	// check to see if the environment variable 'TELNET_REDIR' is not 0;
	char* p = getenv(ENV_TELNET_REDIR);
	if (p) {
		input_redir = output_redir = atoi(p);
		if((p = getenv(ENV_INPUT_REDIR))) input_redir = atoi(p);
		if((p = getenv(ENV_OUTPUT_REDIR))) output_redir = atoi(p);
	} else {
		input_redir = output_redir = GetPrivateProfileInt("Terminal",
			"Telnet_Redir", 0, inifile);
		input_redir = GetPrivateProfileInt("Terminal",
			"Input_Redir", input_redir, inifile);
		output_redir = GetPrivateProfileInt("Terminal",
			"Output_Redir", output_redir, inifile);
	}
	if ((input_redir > 1) || (output_redir > 1))
		setlocale(LC_CTYPE,"");
	// tell isprint() to not ignore local characters, if the environment
	// variable "LANG" has a valid value (e.g. LANG=de for german characters)
	// and the file LOCALE.BLL is installed somewhere along the PATH.
}

// Modified not to use getopt() by Paul Brannan 12/17/98
bool TConfig::Process_Params(int argc, char *argv[]) {
	int optind = 1;
	char *optarg = argv[optind];
	char c;

	while(optind < argc) {
		if(argv[optind][0] != '-') break;

		// getopt
		c = argv[optind][1];
		if(argv[optind][2] == 0)
			optarg = argv[++optind];
		else
			optarg = &argv[optind][2];
		optind++;

		switch(c) {
			case 'd':
				set_string(dumpfile, optarg, sizeof(dumpfile));
				printm(0, FALSE, MSG_DUMPFILE, dumpfile);
				break;
			// added support for setting options on the command-line
			// (Paul Brannan 7/31/98)
			case '-':
				{
					int j;
					for(j = 0; optarg[j] != ' ' && optarg[j] != '=' && optarg[j] != 0; j++);
					if(optarg == 0) {
						printm(0, FALSE, MSG_USAGE);		// print a usage message
						printm(0, FALSE, MSG_USAGE_1);
						return FALSE;
					}
					optarg[j] = 0;
					if(!set_value(optarg, &optarg[j+1]))
						printm(0, FALSE, MSG_BADVAL, optarg);
				}
				break;
			default:
				printm(0, FALSE, MSG_USAGE);		// print a usage message
				printm(0, FALSE, MSG_USAGE_1);
				return FALSE;
		}
	}
	if(optind < argc)
		set_string(host, argv[optind++], sizeof(host)-1);
	if(!strnicmp(host, "telnet://", 9)) {
		// we have a URL to parse
		char *s, *t;

		for(s = host+9, t = host; *s != 0; *(t++) = *(s++));
		*t = 0;
		for(s = host; *s != ':' && *s != 0; s++);
		if(*s != 0) {
			*(s++) = 0;
			port = s;
		}
	}		
	if(optind < argc)
		port = argv[optind++];

	return TRUE;
}

void TConfig::set_string(char *dest, const char *src, const int length) {
   int l = length;
   strncpy(dest, src, l);
 //  dest[length-1] = '\0';
 // Ioannou : this messes strings - is this really needed ?
 // The target string, dest, might not be null-terminated
 // if the length of src is length or more.
 // it should be dest[length] = '\0' for strings with length 1
 // (Escape_string etc), but doesn't work with others (like host).
 // dest is long enough to avoid this in all the tested cases
}

// Ioannou : ignore case for true or on

void TConfig::set_bool(bool *boolval, const char *str) {
   if(!stricmp(str, "true")) *boolval = true;
   else if(!stricmp(str, "on")) *boolval = true;
	else *boolval = (bool)atoi(str);
}