///////////////////////////////////////////////////////////////////
//                                                               //
//                                                               //
//      Key translations - I.Ioannou (roryt@hol.gr)              //
//          Athens - Greece    December 18, 1996 02:56am         //
//          Reads a .cfg file and keeps the key definitions      //
//                for the WIN32 console telnet                   //
//      modified for alternate keymap swiching                   //
//          by Andrey V. Smilianets (smile@head.aval.kiev.ua)    //
//          Kiev - Ukraine, December 1997.                       //
///////////////////////////////////////////////////////////////////
//                                                               //
//                class KeyTranslator                            //
//                                                               //
//  Load          : loads or replaces the keymap                 //
//  TranslateKey  : returns a char * to the key def              //
//  AddKeyDef     : Changes or adds the key translation          //
//  DeleteKeyDef  : Deletes a key def from the list              //
///////////////////////////////////////////////////////////////////

#ifndef __KEYTRANS_H
#define __KEYTRANS_H

#include "tkeydef.h"
#include "tkeymap.h"

#define TOKEN_DELIMITERS " +\t" // The word's delimiters

// Ioannou 2 June 98:  Borland needs them - quick hack
#ifdef __BORLANDC__
#define bool BOOL
#define true TRUE
#define false FALSE
#endif //  __BORLANDC__

// Maybe not portable, but this is for application cursor mode
// (Paul Brannan 5/27/98)
// Updated for correct precedence in tncon.cpp (Paul Brannan 12/9/98)
#define APP4_KEY			0x8000
#define APP3_KEY			0x4000
#define APP2_KEY			0x2000
#define APP_KEY				0x1000

/////////////////////////////////////////////////////////////
//                class KeyTranslator                      //
//  Load          : loads or replaces the keymap           //
//  TranslateKey  : returns a sz to the key def            //
//  AddKeyDef     : Changes or adds the key translation    //
//  DeleteKeyDef  : Deletes a key def from the list        //
/////////////////////////////////////////////////////////////

class KeyTranslator {
friend class TMapLoader;			// FIX ME!!  This isn't the best solution
public:
    KeyTranslator();
    ~KeyTranslator() { DeleteAllDefs(); }

    int  SwitchTo(int);				// switch to selected keymap
	int switchMap(TKeyDef& tk);

    // Returns a pointer to the string that should be printed.
    // Should return NULL if there is no translation for the key.
    const char *TranslateKey(WORD wVirtualKeyCode, DWORD dwControlKeyState);

    // Changes or adds the key translation associated with
    // wVirtualScanCode and dwControlKeyState.
    // Return 1 on success.
    int AddKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState, char *lpzKeyDef);
	int AddKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState, tn_ops op);

    // Delete a key translation
	int DeleteKeyDef(WORD wVirtualKeyCode, DWORD dwControlKeyState);

	// Paul Brannan 8/28/98
	void set_ext_mode(DWORD mode) {ext_mode |= mode;}
	void unset_ext_mode(DWORD mode) {ext_mode &= ~mode;}
	void clear_ext_mode() {ext_mode = 0;}
	DWORD get_ext_mode() {return ext_mode;}

private:
	DWORD     Fix_ControlKeyState(char *);
	char*     Fix_Tok(char *);
	DWORD ext_mode;								// Paul Brannan 8/28/98

	TArrayAsVector<KeyMap> mapArray;
	TArrayAsVector<TKeyDef> globals;

	void      DeleteAllDefs(void);
	int       AddGlobalDef(WORD wVirtualKeyCode, char*lpzKeyDef);
	int		  LookOnGlobal(char* vkey);
	DWORD	  GetGlobalCode(int i) {return globals[i].GetCodeKey();}

	int currentKeyMap, mainKeyMap;				// AVS

};
	
#endif // __KEYTRANS_H
