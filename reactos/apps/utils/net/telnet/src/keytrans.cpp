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

///////////////////////////////////////////////////////////////////
//      Key translations - I.Ioannou (roryt@hol.gr)              //
//          Athens - Greece    December 18, 1996 02:56am         //
//          Reads a .cfg file and keeps the definitions          //
//      modified for alternate keymap swiching                   //
//          by Andrey V. Smilianets (smile@head.aval.kiev.ua)    //
//          Kiev - Ukraine, December 1997.                       //
//      modified to work with MSVC and the Standard Template     //
//          library by Paul Brannan <pbranna@clemson.edu>,       //
//          May 25, 1998                                         //
//      updated June 7, 1998 by Paul Brannan to remove cout and  //
//          cerr statements                                      //
//      APP_KEY and APP2_Key added July 12, 1998 by Paul Brannan //
///////////////////////////////////////////////////////////////////
//                class KeyTranslator                            //
//  Load          : loads or replaces the keymap                 //
//  TranslateKey  : returns a char * to the key def              //
//  AddKeyDef     : Changes or adds the key translation          //
//  DeleteKeyDef  : Deletes a key def from the list              //
///////////////////////////////////////////////////////////////////

#include <windows.h>

// changed to make work with VC++ (Paul Brannan 5/25/98)
// FIX ME !!! Ioannou:  This must be __BORLANDC__ && VERSION < 5
// but what is the directive for Borland version ????
// FIXED Sept. 31, 2000 (Bernard Badger)
//
#if defined(__BORLANDC__) && (__BORLANDC < 0x0500)
#include <mem.h>
#else
#include <memory.h>
#endif

#include "keytrans.h"
#include "tnerror.h"

/////////////////////////////////////////////////////////////
//                class KeyTranslator                      //
//  Load          : loads or replaces the keymap           //
//  TranslateKey  : returns a sz to the key def            //
//  AddKeyDef     : Changes or adds the key translation    //
//  DeleteKeyDef  : Deletes a key def from the list        //
/////////////////////////////////////////////////////////////


KeyTranslator::KeyTranslator():
mapArray(0,0,sizeof(KeyMap)),
globals(0,0,sizeof(TKeyDef)) {
	ext_mode = 0;			// Paul Brannan 8/28/98
	currentKeyMap = mainKeyMap = -1;
};

//AVS
// perform keymap switching
int KeyTranslator::switchMap(TKeyDef& tk) {
    if ( mapArray.IsEmpty() ) {
		return currentKeyMap = -1;
    };
    int i = mapArray.Find(KeyMap(tk));
    if ( i != INT_MAX ) {
		if (currentKeyMap == i)
            currentKeyMap = mainKeyMap; // restore to default
		else currentKeyMap = i;
		return 1;
    };
    return 0;
};

// Let the calling function interpret the error code (Paul Brannan 12/17/98)
int KeyTranslator::SwitchTo(int to) {
	
    int max = mapArray.GetItemsInContainer();
    if (max == 0) return -1;
    if (to < 0 || to > (max-1)) return 0;
	
    currentKeyMap = to;
    return 1;
};

//AVS
// rewrited to support multiple keymaps
const char *KeyTranslator::TranslateKey(WORD wVirtualKeyCode,
											  DWORD dwControlKeyState)
{
	if ( mapArray.IsEmpty() ) return NULL;
	
	TKeyDef ask(NULL, dwControlKeyState, wVirtualKeyCode);
	
	// if a keymap switch pressed
	if ( switchMap(ask) > 0 ) return "";
	
	int i = mapArray[currentKeyMap].map.Find(ask);
	
	if ( i != INT_MAX) return mapArray[currentKeyMap].map[i].GetszKey();
	
	// if not found in current keymap
	if ( currentKeyMap != mainKeyMap ) {
		i = mapArray[mainKeyMap].map.Find(ask);
		if ( i != INT_MAX)  return mapArray[mainKeyMap].map[i].GetszKey();
	};
	return NULL;
};


//AVS
// rewrited to support multiple keymaps
int KeyTranslator::AddKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState,
                             char*lpzKeyDef)
{
	if ( ! mapArray[currentKeyMap].map.IsEmpty() ) {
		int i = mapArray[currentKeyMap].map.Find(TKeyDef(NULL, dwControlKeyState, wVirtualKeyCode));
		if ( i != INT_MAX) {
			mapArray[currentKeyMap].map[i] = lpzKeyDef;
			return 1;
		}
	};
	return mapArray[currentKeyMap].map.Add( TKeyDef(lpzKeyDef, dwControlKeyState, wVirtualKeyCode));
}

// Paul Brannan Feb. 22, 1999
int KeyTranslator::AddKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState,
                             tn_ops the_op)
{
	optype op;
	op.sendstr = 0;
	op.the_op = the_op;
	if ( ! mapArray[currentKeyMap].map.IsEmpty() ) {
		int i = mapArray[currentKeyMap].map.Find(TKeyDef(NULL, dwControlKeyState, wVirtualKeyCode));
		if ( i != INT_MAX) {
			mapArray[currentKeyMap].map[i] = op;
			return 1;
		}
	};
	return mapArray[currentKeyMap].map.Add( TKeyDef(op, dwControlKeyState, wVirtualKeyCode));
}

// AVS
int KeyTranslator::LookOnGlobal(char* vkey) {
    if ( ! globals.IsEmpty() ) {
		int max = globals.GetItemsInContainer();
		for ( int i = 0; i < max ; i++ )
			if ( stricmp(globals[i].GetszKey(), vkey) == 0 )
				return i;
    };
    return INT_MAX;
};

int KeyTranslator::AddGlobalDef(WORD wVirtualKeyCode, char*lpzKeyDef) {
	if ( ! globals.IsEmpty() ) {
		int max = globals.GetItemsInContainer();
		for ( int i = 0; i < max ; i++ ) {
			const char *s = globals[i].GetszKey();
			if ( stricmp(s, lpzKeyDef) == 0 ) {
				globals[i] = DWORD(wVirtualKeyCode);
				return 1;
			}
		}
	}
	return globals.Add( TKeyDef(lpzKeyDef, 0, wVirtualKeyCode));
}


//AVS
// rewrited to support multiple keymaps
int KeyTranslator::DeleteKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState)
{
	if ( mapArray.IsEmpty() || mapArray[currentKeyMap].map.IsEmpty() )
		return 0;
	
	int i = mapArray[currentKeyMap].map.Find(TKeyDef(NULL, dwControlKeyState, wVirtualKeyCode));
	
	if ( i != INT_MAX) {
		mapArray[currentKeyMap].map.Destroy(i);
		return 1;
	};
	return 0;
};

//AVS
// rewritten to support multiple keymaps
void KeyTranslator::DeleteAllDefs(void)
{
	// This code wants to crash under the STL; Apparently the Destroy()
	// function actually deletes the entry, rather than simply releasing
	// memory.  I think flush() should do the same thing, at least the
	// way it is written with STL_BIDS (Paul Brannan 5/25/98).
	int max;
	
	max = mapArray.GetItemsInContainer();
	if ( ! mapArray.IsEmpty() ) {
		for ( int i = 0; i < max; i++ ) {
			if ( !mapArray[i].map.IsEmpty() ) {
				mapArray[i].map.Flush();
			};
		};
	};
	globals.Flush();
	mapArray.Flush();
	currentKeyMap = -1;
	mainKeyMap    = -1;
};
