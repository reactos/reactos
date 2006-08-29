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
//     Class TMapLoader - Key/Character Mappings       //
//                      - Loads from telnet.cfg        //
//     originally part of KeyTrans.cpp                 //
/////////////////////////////////////////////////////////

#ifdef __BORLANDC__
#include <fstream.h>
#else
#include <string>
#include <fstream>
#endif

#include "tmapldr.h"
#include "tnerror.h"
#include "tnconfig.h"

// It's probably a good idea to turn off the "identifier was truncated" warning
// in MSVC (Paul Brannan 5/25/98)
#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

// AVS
// skip inline comments, empty lines
static char * getline(istream& i, char* buf, int size){
	
	int len = 0;
	
	while (1) {
		memset(buf,0,size);
		if (i.eof()) break;
		i.getline(buf,size,'\n');
		
		while (buf[len]) {
			if ( /*(buf[len]>=0) &&*/ buf[len]< ' ' ) buf[len] = ' ';
			len++;
		};
		len = 0;
		
		// not so fast, but work ;)
		while ( buf[len] ) {
            if ( (buf[len] == ' ') && (buf[len+1] == ' ')) {
				memmove(buf+len, buf+len+1, strlen(buf+len));
            } else len++;
		};
		
		if (buf[0] == ' ') memmove(buf, buf+1, size-1);
		
		// empty or comment
		if ((buf[0]==0)||(buf[0]==';')) continue;
		
		len = 0; // look for comment like this one
		while (buf[len])
			if ((buf[len] == '/') && (buf[len+1] == '/')) buf[len] = 0;
			else len++;
			
			if (len && (buf[len-1] == ' ')) {
                len--;
                buf[len]=0;
			};
			// in case for comment like this one (in line just a comment)
			if (buf[0]==0) continue;
			
			break;
	};
	return (buf);
};

//AVS
// use string as FIFO queue for lines
static int getline(string&str, char* buf, size_t sz) {
	
	if ( !str.length() ) return 0;
	const char * p = strchr(str.c_str(),'\n');
	unsigned int len; // Changed to unsigned (Paul Brannan 6/23/98)
	if ( p==NULL )
		len = str.length();
	else
		len = p - str.c_str();
	
	len = len<sz?len:sz-1;
	
	strncpy(buf,str.c_str(), len);
	buf[len]=0;
	// DJGPP also uses erase rather than remove (Paul Brannan 6/23/98)
#ifndef __BORLANDC__
	str.erase(0, len + 1);
#else
	str.remove(0,len+1);
#endif
	return 1;
};

//AVS
// parse \nnn and \Xhh
static int getbyte(const char*str) {
	unsigned char retval = 0;
	int base = 10;
	int readed = 0;
	
	if ( (*str == 'x') || (*str == 'X') ) {
		base = 16;
		readed++;
	};
	
	while (readed != 3 && str[readed]) {
		unsigned char ch = toupper(str[readed]);
		if ( isdigit(ch) ) {
			retval = retval*base + (ch -'0');
		} else if (base == 16 && ch >= 'A' && ch <= 'F') {
			retval = retval*base + (ch-'A'+10);
		} else {
			return -1;
		};
		readed++;
	};
	// Ioannou: If we discard the 0x00 we can't undefine a key !!!
	//  if ( retval == 0 ) {
	//     return -1;
	//  };
	return retval;
};

//AVS
// a little optimization
DWORD Fix_ControlKeyState(char * Next_Token) {
	if (stricmp(Next_Token, "RIGHT_ALT" ) == 0) return RIGHT_ALT_PRESSED;
	if (stricmp(Next_Token, "LEFT_ALT"  ) == 0) return LEFT_ALT_PRESSED;
	if (stricmp(Next_Token, "RIGHT_CTRL") == 0) return RIGHT_CTRL_PRESSED;
	if (stricmp(Next_Token, "LEFT_CTRL" ) == 0) return LEFT_CTRL_PRESSED;
	if (stricmp(Next_Token, "SHIFT"     ) == 0) return SHIFT_PRESSED;
	if (stricmp(Next_Token, "NUMLOCK"   ) == 0) return NUMLOCK_ON;
	if (stricmp(Next_Token, "SCROLLLOCK") == 0) return SCROLLLOCK_ON;
	if (stricmp(Next_Token, "CAPSLOCK"  ) == 0) return CAPSLOCK_ON;
	if (stricmp(Next_Token, "ENHANCED"  ) == 0) return ENHANCED_KEY;
	
	// Paul Brannan 5/27/98
	if (stricmp(Next_Token, "APP_KEY"   ) == 0) return APP_KEY;
	// Paul Brannan 6/28/98
	if (stricmp(Next_Token, "APP2_KEY"  ) == 0) return APP2_KEY;
	// Paul Brannan 8/28/98
	if (stricmp(Next_Token, "APP3_KEY"  ) == 0) return APP3_KEY;
	// Paul Brannan 12/9/98
	if (stricmp(Next_Token, "APP4_KEY"  ) == 0) return APP4_KEY;
	
	return 0;
}


// AVS
// rewrited to suppert \xhh notation, a little optimized
char* Fix_Tok(char * tok) {
	static char s[256];
	int i,j,n;
	
	// setmem is nonstandard; memset is standard (Paul Brannan 5/25/98)
	memset(s, 0, 256);
	//  setmem(s, 256, 0);
	i = j = n = 0;
	if ( tok != NULL ) {
		for ( ; tok[i] != 0; ) {
			switch ( tok[i] ) {
			case '\\' :
				switch ( tok[i+1] ) {
				case '\\':
					s[j++] = '\\';
					i += 2;
					break;
				default:
					n = getbyte(tok+i+1);
					if ( n < 0 )
						s[j++] = tok[i++];
					else {
						s[j++]=n;
						i += 4;
					} ;
					break;
				};
				break;
				case '^' :
                    if ( tok[i+1] >= '@' ) {
						s[j++] = tok[i+1] - '@';
						i += 2;
						break;
                    }
				default  :
                    s[j++] = tok[i++];
			}
		}
	}
	return s;
};

// AVS
// perform 'normalization' for lines like [some text], and some checks
// maybe it will be done faster - but no time for it
int normalizeSplitter(string& buf) {
    if ( buf.length() <= 2 ) return 0;
    if ( buf[0] == '[' && buf[buf.length()-1] == ']' ) {
		while ( buf[1] == ' ' )
			// DJGPP also uses erase rather than remove (Paul Brannan 6/23/98)
#ifndef __BORLANDC__
			buf.erase(1, 1);
#else
			buf.remove(1,1);
#endif
		while ( buf[buf.length()-2] == ' ' )
			// Paul Brannan 6/23/98
#ifndef __BORLANDC__
			buf.erase(buf.length()-2,1);
#else
			buf.remove(buf.length()-2,1);
#endif
		return 1;
    }
    return 0;
};

// AVS
// looking for part in string array, see Load(..) for more info
int TMapLoader::LookForPart(stringArray& sa, const char* partType, const char* partName) {
    if ( !sa.IsEmpty() ) {
		string cmpbuf("[");
		cmpbuf += partType;
		cmpbuf += " ";
		cmpbuf += partName;
		cmpbuf += "]";
		normalizeSplitter(cmpbuf); // if no parttype, [global] for example
		int max = sa.GetItemsInContainer();
		for ( int i = 0; i<max; i++ )
			// I found some strange behavior if strnicmp was used here
            if (strnicmp(cmpbuf.c_str(),sa[i].c_str(),cmpbuf.length()) == 0)
				return i;
    };
    return INT_MAX;
};

// AVS
// load globals to 'globals'
// in buf must be a [global] part of input file
int TMapLoader::LoadGlobal(string& buf) {
	
	char wbuf[128];
	while ( buf.length() ) {
		wbuf[0]=0;
		if (!getline(buf,wbuf,sizeof(wbuf))) break;
		if ( wbuf[0]==0 ) break;
		char* Name = strtok(wbuf, TOKEN_DELIMITERS);
		if ( stricmp(Name, "[global]")==0 ) continue;
		
		char* Value = strtok(NULL, TOKEN_DELIMITERS);
		if ( Value == NULL ) {
			//              cerr << "[global] -> no value for " << Name << endl;
			printm(0, FALSE, MSG_KEYNOVAL, Name);
			continue;
		};
		int val = atoi(Value);
		if ( val > 0 && val <= 0xff ) {
			if ( !KeyTrans.AddGlobalDef(val, Name)) return 0;
		}
		else {
			//             cerr << "[global] -> bad value for " << Name << endl;
			printm(0, FALSE, MSG_KEYBADVAL, Name);
			continue;
		};
	};
	return 1;
};

// AVS
// perform parsing of strings like 'VK_CODE shifts text'
// returns text on success
char* TMapLoader::ParseKeyDef(const char* buf, WORD& vk_code, DWORD& control) {
	char wbuf[256];
	strcpy(wbuf,buf);
	char* ptr = strtok(wbuf, TOKEN_DELIMITERS);
	if ( ptr == NULL ) return NULL;
	
	int i = KeyTrans.LookOnGlobal(ptr);
	if ( i == INT_MAX ) return NULL;
	
	vk_code = KeyTrans.GetGlobalCode(i);
	
	control = 0;
	DWORD st;
	while (1) {
		ptr = strtok(NULL, TOKEN_DELIMITERS);
		if ((ptr == NULL) || ((st = Fix_ControlKeyState(ptr)) == 0)) break;
		control |= st;
	};
	
	if ( ptr == NULL ) return NULL;
	
	return Fix_Tok(ptr);
};

// AVS
// load keymap to current map
// be aware - buf must passed by value, its destroyed
int TMapLoader::LoadKeyMap(string buf) {
	
	char wbuf[128];
	WORD vk_code;
	DWORD control;
	int i;

	// Paul Brannan Feb. 22, 1999
	strcpy(wbuf, "VK_");
	wbuf[4] = 0;
	wbuf[3] = ini.get_escape_key();
	i = KeyTrans.LookOnGlobal(wbuf);
	if (i != INT_MAX) {
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), RIGHT_ALT_PRESSED, TN_ESCAPE);
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), LEFT_ALT_PRESSED, TN_ESCAPE);
	}
	wbuf[3] = ini.get_scrollback_key();
	i = KeyTrans.LookOnGlobal(wbuf);
	if (i != INT_MAX) {
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), RIGHT_ALT_PRESSED, TN_SCROLLBACK);
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), LEFT_ALT_PRESSED, TN_SCROLLBACK);
	}
	wbuf[3] = ini.get_dial_key();
	i = KeyTrans.LookOnGlobal(wbuf);
	if (i != INT_MAX) {
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), RIGHT_ALT_PRESSED, TN_DIAL);
		KeyTrans.AddKeyDef(KeyTrans.GetGlobalCode(i), LEFT_ALT_PRESSED, TN_DIAL);
	}
	KeyTrans.AddKeyDef(VK_INSERT, SHIFT_PRESSED, TN_PASTE);

	while ( buf.length() ) {
		wbuf[0] = 0;
		if (!getline(buf,wbuf,sizeof(wbuf))) break;
		if ( wbuf[0]==0 ) break;
		if ( strnicmp(wbuf,"[keymap",7)==0 ) continue;
		
		char * keydef = ParseKeyDef(wbuf,vk_code,control);

		if ( keydef != NULL ) {

			// Check to see if keydef is a "special" code (Paul Brannan 3/29/00)
			if(!strnicmp(keydef, "\\tn_escape", strlen("\\tn_escape"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_ESCAPE)) return 0;
			} else if(!strnicmp(keydef, "\\tn_scrollback", strlen("\\tn_scrollback"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_SCROLLBACK)) return 0;
			} else if(!strnicmp(keydef, "\\tn_dial", strlen("\\tn_dial"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_DIAL)) return 0;
			} else if(!strnicmp(keydef, "\\tn_paste", strlen("\\tn_paste"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_PASTE)) return 0;
			} else if(!strnicmp(keydef, "\\tn_null", strlen("\\tn_null"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_NULL)) return 0;
			} else if(!strnicmp(keydef, "\\tn_cr", strlen("\\tn_cr"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_CR)) return 0;
			} else if(!strnicmp(keydef, "\\tn_crlf", strlen("\\tn_crlf"))) {
				if(!KeyTrans.AddKeyDef(vk_code, control, TN_CRLF)) return 0;
			} else 
				if(!KeyTrans.AddKeyDef(vk_code,control,keydef)) return 0;
				// else DeleteKeyDef() ???? - I'm not sure...
		}
	};

	return 1;
};

// AVS
// load [charmap ...] part to xlat
int TMapLoader::LoadCharMap(string buf) {
	char wbuf[128];
	char charmapname[128];
	charmapname[0] = 0;
	
	//        xlat.init(); now it done by KeyTranslator::Load()
	
	while ( buf.length() ) {
		wbuf[0]=0;
		if (!getline(buf,wbuf,sizeof(wbuf))) break;
		if ( wbuf[0]==0 ) break;
		if ( strnicmp(wbuf,"[charmap",8)==0 ) {
			strcpy(charmapname,wbuf);
			continue;
		};
		char * host = strtok(wbuf, " ");
		char * console = strtok(NULL, " ");
		
		int bHost;
		int bConsole;
		
		if ( host == NULL || console == NULL ) {
			//              cerr << charmapname << " -> Bad structure" << endl;
			printm(0, FALSE, MSG_KEYBADSTRUCT, charmapname);
			return 0;
		};
		if ( strlen(host) > 1 && host[0] == '\\' )
			bHost = getbyte(host+1);
		else
			bHost = (unsigned char)host[0];
		
		if ( strlen(console) > 1 && console[0] == '\\' )
			bConsole = getbyte(console+1);
		else
			bConsole = (unsigned char)console[0];
		
		if ( bHost <= 0 || bConsole <= 0 ) {
			//              cerr << charmapname << " -> Bad chars? "
			//                   << host << " -> " << console << endl;
			printm(0, FALSE, MSG_KEYBADCHARS, charmapname, host, console);
			return 0;
		};
		// xlat.table[bHost] = bConsole;
		Charmap.modmap(bHost, 'B', bConsole);
	};
	return (Charmap.enabled = 1);
	return 1;
};

// AVS
// ignore long comment [comment] ... [end comment]
// recursive!
int getLongComment(istream& is, char* wbuf, size_t sz) {
	
	int bufLen;
    while ( is ) {
		wbuf[0] = 0;
		getline(is, wbuf, sz);
		if ( wbuf[0]==0 ) return 1;
		bufLen = strlen(wbuf);
		if ( wbuf[0] == '[' && wbuf[bufLen-1] == ']' ) {
			string temps(wbuf);
			
			if (!normalizeSplitter(temps)) {
				//           cerr << "Unexpected line '" << temps << "'\n";
				printm(0, FALSE, MSG_KEYUNEXPLINE, temps.c_str());
				return 0;
			};
			if ( stricmp(temps.c_str(),"[comment]") == 0 ) {
				// do recursive call
				if ( !getLongComment(is, wbuf, sz) ) return 0;
				continue;
			};
			if ( stricmp(temps.c_str(),"[end comment]") == 0 ) return 1;
		};
    };
	// we get a warning if we don't put a return here (Paul Brannan 5/25/98)
	return 0;
};

// AVS
// completelly rewrited to support new conceptions
int TMapLoader::Load(const char * filename, const char * szActiveEmul) {
	char buf[256];
	int bufLen;
	
	ifstream inpfile(filename);
	KeyTrans.DeleteAllDefs();
	Charmap.init();
	
	// it is an array for store [...] ... [end ...] parts from file
	stringArray SA(0,0,sizeof(string));
	int AllOk = 0;
	
	while ( inpfile ) {
		
		getline(inpfile, buf, 255);
		bufLen = strlen(buf);
		if ( !bufLen ) continue;
		
		if ( buf[0] == '[' && buf[bufLen-1] == ']' ) {
			// is a part splitter [...]
			string temps(buf);
			
			if (!normalizeSplitter(temps)) {
				printm(0, FALSE, MSG_KEYUNEXPLINE, temps.c_str());
				AllOk = 0;
				break;
			};
			// if a comment
			if ( stricmp(temps.c_str(),"[comment]") == 0 ) {
#ifdef KEYDEBUG
				printit(temps.c_str());
#endif
				if ( !getLongComment(inpfile, buf, sizeof(buf)) ) {
					printm(0, FALSE, MSG_KEYUNEXPEOF);
					break;
				};
#ifdef KEYDEBUG
				printit("\r          \r");
#endif
				continue;
			};
			
			
			string back = temps;
			// prepare line for make it as [end ...]
			// and check it
			if ( strnicmp(back.c_str(), "[global]", 8) == 0 ) {} // do nothing
			else if ( strnicmp(back.c_str(), "[keymap", 7) == 0 ) {
				// DJGPP also uses erase rather than remove (Paul Brannan 6/23/98)
#ifndef __BORLANDC__
				back.erase(7);
#else
				back.remove(7);
#endif
				back += "]";
			}
			else if ( strnicmp(back.c_str(), "[charmap", 8) == 0 ) {
				// Paul Brannan 6/23/98
#ifndef __BORLANDC__
				back.erase(8);
#else
				back.remove(8);
#endif
				back += "]";
			}
			else if ( strnicmp(back.c_str(), "[config", 7) == 0 ) {
				// Paul Brannan 6/23/98
#ifndef __BORLANDC__
				back.erase(7);
#else
				back.remove(7);
#endif
				back += "]";
			}
			else {
				//        cerr << "Unexpected token " << back << endl;
				printm(0, FALSE, MSG_KEYUNEXPTOK, back.c_str());
				break;
			};
			
			back.insert(1,"END "); // now it looks like [END ...]
#ifdef KEYDEBUG
			printit(temps.c_str());
#endif
			
			int ok = 0;
			// fetch it to temps
			while ( 1 ) {
				getline(inpfile, buf, sizeof(buf));
				bufLen = strlen(buf);
				if ( !bufLen ) break;
				if ( buf[0] == '[' && buf[bufLen-1] == ']' ) {
					string t(buf);
					if ( !normalizeSplitter(t) ) break;
					
					if ( stricmp(t.c_str(),back.c_str()) == 0 ) {
						ok = 1;
						break;
					};
					
					// AVS 31.12.97 fix [comment] block inside another block
					if ( stricmp(t.c_str(),"[comment]") == 0 &&
						getLongComment(inpfile, buf, sizeof(buf)) ) continue;
					
					break;
				};
				temps += "\n";
				temps += buf;
			};
			if ( !ok ) {
				//         cerr << "Unexpected end of file or token" << endl;
				printm(0, FALSE, MSG_KEYUNEXP);
				AllOk = 0;
				break;
			};
#ifdef KEYDEBUG
			printit("\r                                                                      \r");
#endif
			AllOk = SA.Add(temps);;
			if ( !AllOk ) break;
		} else {
			//       cerr << "Unexpected line '" << buf << "'\n";
			printm(0, FALSE, MSG_KEYUNEXPLINE, buf);
			AllOk = 0;
			break;
		};
	};
	
	inpfile.close();
	
	if ( !AllOk ) return 0;
	
	// now all file are in SA, comments are stripped
	
	int i = LookForPart(SA, "global", "");
	if ( i == INT_MAX ) {
		//     cerr << "No [GLOBAL] definition!" << endl;
		printm(0, FALSE, MSG_KEYNOGLOBAL);
		return 0;
	};
	if ( !LoadGlobal(SA[i]) ) {
		return 0;
	};
	
	// look for need configuration
	i = LookForPart(SA, "config", szActiveEmul);
	if ( i == INT_MAX ) {
		//     cerr << "No [CONFIG " << szActiveEmul << "]\n";
		printm(0, FALSE, MSG_KEYNOCONFIG, szActiveEmul);
		return 0;
	};
	//  cerr << "use configuration: " << szActiveEmul << endl;
	printm(0, FALSE, MSG_KEYUSECONFIG, szActiveEmul);
	BOOL hadKeys = FALSE;
	
	string config = SA[i];
	// parse it
	while ( config.length() ) {
		buf[0] = 0;
		getline(config,buf,sizeof(buf));
		bufLen = strlen(buf);
		if ( !bufLen || (buf[0] == '[' && buf[bufLen-1] == ']') ) continue;
		if ( strnicmp(buf,"keymap",6) == 0 ) {
			string orig(buf);
			printit("\t"); printit(buf); printit("\n");
			char * mapdef = strtok(buf,":");
			char * switchKey = strtok(NULL,"\n");
			
			if ( !KeyTrans.mapArray.IsEmpty() && switchKey == NULL ) {
				//            cerr << "no switch Key for '" << mapdef
				//                 << "'" << endl;
				printm(0, FALSE, MSG_KEYNOSWKEY, mapdef);
				break;
			};
			if ( KeyTrans.mapArray.IsEmpty() ) {
				if ( switchKey != NULL ) { // create default keymap
					//               cerr << "You cannot define switch key for default keymap -> ignored"
					//                    << endl;
					printm(0, FALSE, MSG_KEYCANNOTDEF);
				};
				TKeyDef empty;
				KeyTrans.mapArray.Add(KeyMap(string(mapdef)));
				KeyTrans.switchMap(empty); // set it as current keymap
				KeyTrans.mainKeyMap = KeyTrans.currentKeyMap;
			}
			else {
				string keydef(switchKey);
				keydef += " !*!*!*"; // just for check
				WORD vk_code;
				DWORD control;
				switchKey = ParseKeyDef(keydef.c_str(),vk_code,control);
				if ( switchKey != NULL ) {
					TKeyDef swi(NULL,control,vk_code);
					if ( KeyTrans.switchMap(swi) > 0 ) {
						//                  cerr << "Duplicate switching key\n";
						printm(0, FALSE, MSG_KEYDUPSWKEY);
						break;
					};
					KeyTrans.mapArray.Add(KeyMap(swi, orig));
					KeyTrans.switchMap(swi); // set it as current keymap
				}
			};
			mapdef+=7; // 'keymap '
			// now load defined keymaps to current
			while ((mapdef != NULL)&&
				(mapdef = strtok(mapdef,TOKEN_DELIMITERS)) != NULL ) {
				i = LookForPart(SA,"keymap",mapdef);
				if ( i == INT_MAX ) {
					//               cerr << "Unknown KEYMAP " << mapdef << endl;
					printm(0, FALSE, MSG_KEYUNKNOWNMAP, mapdef);
				} else {
					mapdef = strtok(NULL,"\n"); // strtok is used in LoadKeyMap
					// so - save pointer!
					hadKeys = LoadKeyMap(SA[i]); // load it
				};
			};
			
		}
		else if ( strnicmp(buf,"charmap",7) == 0 ) {
			printit("\t"); printit(buf); printit("\n");
			char * mapdef = buf + 8;// 'charmap '
			int SuccesLoaded = 0;
			// now load defined charmaps to current
			while ((mapdef != NULL)&&
				(mapdef = strtok(mapdef,TOKEN_DELIMITERS)) != NULL ) {
				i = LookForPart(SA,"charmap",mapdef);
				if ( i == INT_MAX ) {
					//               cerr << "Unknown KEYMAP " << mapdef << endl;
					printm(0, FALSE, MSG_KEYUNKNOWNMAP, mapdef);
				} else {
					mapdef = strtok(NULL,"\n"); // strtok is used in LoadKeyMap
					// so - save pointer!
					if (LoadCharMap(SA[i])) // load it
						SuccesLoaded++;
				};
			};
			if (!SuccesLoaded) {
				//            cerr << "No charmaps loaded\n";
				printm(0, FALSE, MSG_KEYNOCHARMAPS);
				Charmap.init();
			};
			/*         strtok(buf," ");
			
			  char* name = strtok(NULL," ");
			  if ( name == NULL ) {
			  cerr << "No name for CHARMAP" << endl;
			  } else {
			  i = LookForPart(SA,"charmap", name);
			  if ( i == INT_MAX ) {
			  cerr << "Unknown CHARMAP " << name << endl;
			  } else {
			  LoadCharMap(SA[i]);
			  };
			  };
			*/
		}
		else {
			//        cerr << "unexpected token in " << szActiveEmul << endl;
			printm(0, FALSE, MSG_KEYUNEXPTOKIN, szActiveEmul);
		}
	}

	if ( hadKeys) {
		TKeyDef empty;
		KeyTrans.switchMap(empty); // switch to default
		KeyTrans.mainKeyMap = KeyTrans.currentKeyMap; // save it's number
		//     cerr << "There are " << (KeyTrans.mapArray.GetItemsInContainer()) << " maps\n";
		char s[12]; // good enough for a long int (32-bit)
		itoa(KeyTrans.mapArray.GetItemsInContainer(), s, 10);
		printm(0, FALSE, MSG_KEYNUMMAPS, s);
		return 1;
	};
	return 0;
}

void TMapLoader::Display() {

    int max = KeyTrans.mapArray.GetItemsInContainer();
    if (max == 0) {
       printm(0, FALSE, MSG_KEYNOKEYMAPS);
       return;
    };
    for ( int i = 0; i < max; i++ ) {
       char buf[20];
       itoa(i,buf,10);
       printit("\t");
       // Ioannou : we can show the current
       if (KeyTrans.currentKeyMap == i)
         printit("*");
       else
         printit(" ");
       strcat(buf," ");
       printit(buf);
       char * msg = new char [KeyTrans.mapArray[i].orig.length()+1];
       strcpy(msg,KeyTrans.mapArray[i].orig.c_str());
       printit(msg);
       delete[] msg;
       printit("\n");
    };
};
