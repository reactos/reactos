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
//     Class TkeyMap - Key Mappings                    //
//                   - kept in an array container      //
//     originally part of KeyTrans.cpp                 //
/////////////////////////////////////////////////////////

#include "tkeymap.h"

KeyMap::KeyMap(DWORD state, DWORD code): map(0,0,sizeof(TKeyDef)),
                                         key(NULL,state,code) {};

KeyMap::KeyMap(TKeyDef&tk):map(0,0,sizeof(TKeyDef)){
  key = tk;
};
KeyMap::KeyMap(TKeyDef&tk, string& t):map(0,0,sizeof(TKeyDef)), orig(t){
  key = tk;
};

int KeyMap::operator==(const KeyMap & t) const{
    return key == t.key;
};

KeyMap& KeyMap::operator = (const KeyMap& t){
     key = t.key;
     map = t.map;
     orig = t.orig;
     return (*this);
};

#ifndef __BORLANDC__
bool operator<(const KeyMap &t1, const KeyMap &t2) {
	return t1.key < t2.key;
}
#endif

KeyMap::~KeyMap() {
};
