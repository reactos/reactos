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

#include "tscript.h"

// FIX ME!!  This code not yet functional.

#define TERMINATOR '~'
#define SPACE_HOLDER '_'

// processScript by Bryan Montgomery
// modified to handle script file by Paul Brannan
BOOL TScript::processScript (char* data) {
/*    char* end = strchr(script,TERMINATOR);
	if (0 == end) {
		return true;
	} else {
		char* current = new char(sizeof(char)*strlen(script));
		strncpy(current,script,(int)(end-script));
		current[(int)(end-script)]=0;
		char *ptr=end;
		if (strstr(data,current) != 0) {
			script = ++end;
			end = strchr(script,TERMINATOR);
			while ((ptr = strchr(ptr,SPACE_HOLDER)) != 0 && ptr < end) {
				*ptr=' ';
			}
			Network.WriteString(script,(int)(end-script));
			Network.WriteString("\r\n",2);
			script = ++end;
		}
	delete current;
	}*/
	return TRUE;
}

void TScript::initScript (char *filename) {
	if(fp) fclose(fp);
	fp = fopen(filename, "rt");
}