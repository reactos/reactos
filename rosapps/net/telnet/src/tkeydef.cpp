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

/////////////////////////////////////////////////////////
//     Class TkeyDef - Key Definitions                 //
//                   - kept in an array container      //
//     originally part of KeyTrans.cpp                 //
/////////////////////////////////////////////////////////

#include "tkeydef.h"

// This class did not properly release memory before, and a buffer overrun
// was apparent in operator=(char*).  Fixed.  (Paul Brannan Feb. 4, 1999)

TKeyDef::TKeyDef() {
	uKeyDef.szKeyDef = 0;
	vk_code = dwState = 0;
}

TKeyDef::TKeyDef(char *def, DWORD state, DWORD code) {
	uKeyDef.szKeyDef = 0;
	if (def != NULL && *def != 0) {
		// szKeyDef = (char *) GlobalAlloc(GPTR, strlen(def) +1);
		uKeyDef.szKeyDef = new char[strlen(def)+1];
		strcpy(uKeyDef.szKeyDef, def);
	}
	dwState = state;
	vk_code = code;
}

TKeyDef::TKeyDef(optype op, DWORD state, DWORD code) {
	uKeyDef.op = new optype;
	uKeyDef.op->sendstr = 0;
	uKeyDef.op->the_op = op.the_op;
	dwState = state;
	vk_code = code;
}

TKeyDef::TKeyDef(const TKeyDef &t) {
	if(t.uKeyDef.szKeyDef == NULL) {
		uKeyDef.szKeyDef = (char *)NULL;
	} else if(t.uKeyDef.op->sendstr == 0) {
		uKeyDef.op = new optype;
		uKeyDef.op->sendstr = 0;
		uKeyDef.op->the_op = t.uKeyDef.op->the_op;
	} else {
		uKeyDef.szKeyDef = new char[strlen(t.uKeyDef.szKeyDef)+1];
		strcpy(uKeyDef.szKeyDef, t.uKeyDef.szKeyDef);
	}
	dwState = t.dwState;
	vk_code = t.vk_code;
}

TKeyDef::~TKeyDef() {
	if(uKeyDef.szKeyDef) delete[] uKeyDef.szKeyDef;
}

char * TKeyDef::operator=(char *def) {
	if(def != NULL && *def != 0) {
		if(uKeyDef.szKeyDef) delete[] uKeyDef.szKeyDef;
		uKeyDef.szKeyDef = new char[strlen(def)+1];
		strcpy(uKeyDef.szKeyDef, def);
	}
	return uKeyDef.szKeyDef;
}

DWORD TKeyDef::operator=(DWORD code) {
	return vk_code = code;
}

TKeyDef& TKeyDef::operator=(const TKeyDef &t) {
	if(t.uKeyDef.szKeyDef) {
		if(uKeyDef.szKeyDef) delete[] uKeyDef.szKeyDef;
		if(t.uKeyDef.op->sendstr) {
			uKeyDef.szKeyDef = new char[strlen(t.uKeyDef.szKeyDef)+1];
			strcpy(uKeyDef.szKeyDef, t.uKeyDef.szKeyDef);
		} else {
			uKeyDef.op = new optype;
			uKeyDef.op->sendstr = 0;
			uKeyDef.op->the_op = t.uKeyDef.op->the_op;
		}
	} else {
		uKeyDef.szKeyDef = (char *)NULL;
	}
	dwState = t.dwState;
	vk_code = t.vk_code;
	return *this;
}

const optype& TKeyDef::operator=(optype op) {
	uKeyDef.op = new optype;
	uKeyDef.op->sendstr = 0;
	uKeyDef.op->the_op = op.the_op;
	return *uKeyDef.op;
}

// STL requires that operators be friends rather than member functions
// (Paul Brannan 5/25/98)
#ifndef __BORLANDC__
bool operator==(const TKeyDef & t1, const TKeyDef & t2) {
	return ((t1.vk_code == t2.vk_code) && (t1.dwState == t2.dwState));
}
// We need this function for compatibility with STL (Paul Brannan 5/25/98)
bool operator< (const TKeyDef& t1, const TKeyDef& t2) {
	if (t1.vk_code == t2.vk_code) return t1.dwState < t2.dwState;
	return t1.vk_code < t2.vk_code;
}
#else
int   TKeyDef::operator==(TKeyDef & t) {
	return ((vk_code == t.vk_code) && (dwState == t.dwState));
}
#endif

