#ifndef __TKEYMAP_H
#define __TKEYMAP_H

#ifdef __BORLANDC__
#include <classlib\arrays.h>
#else
#include <string>
#include "stl_bids.h"
#endif

#include "tkeydef.h"

//AVS
typedef TArrayAsVector<TKeyDef> keyArray;

//AVS
// representation of keymap
struct KeyMap {
       keyArray  map;						// keymap
       string    orig;						// original string from .cfg file
       TKeyDef   key;						// 'switch to' key

       KeyMap(DWORD state, DWORD code);
       KeyMap(): map(0,0,sizeof(TKeyDef)){};
       KeyMap(TKeyDef&tk);
       KeyMap(TKeyDef&tk, string&);
       KeyMap(const string&t): map(0,0,sizeof(TKeyDef)), orig(t) {};
       int   operator==(const KeyMap & t) const;
	   KeyMap& operator = (const KeyMap& t);

#ifndef __BORLANDC__
	   // The STL needs this (Paul Brannan 5/25/98)
	   friend bool operator<(const KeyMap &t1, const KeyMap &t2);
#endif

       ~KeyMap();
};

#endif
