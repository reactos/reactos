///////////////////////////////////////////////////////////////////
//                                                               //
// File format :                                                 //
//                                                               //
//  Comments with a ; in column 1                                //
//  Empty Lines ignored                                          //
//  The words are separated by a space, a tab, or a plus ("+")   //
//                                                               //
//  First a [GLOBAL] section :                                   //
//   [GLOBAL]                                                    //
//   VK_F1         112                                           //
//   .                                                           //
//   .                                                           //
//   [END_GLOBAL]                                                //
//                                                               //
//   The GLOBAL section defines the names of the keys            //
//   and the virtual key code they have.                         //
//   If you repeat a name you'll overwrite the code.             //
//   You can name the keys anything you like                     //
//   The Virtual key nymber must be in Decimal                   //
//   After the number you can put anything : it is ignored       //
//   Here you must put ALL the keys you'll use in the            //
//   other sections.                                             //
//                                                               //
//  Then the emulations sections :                               //
//                                                               //
//   [SCO_ANSI]                                                  //
//                                                               //
//   VK_F1                    \027[M or                          //
//   VK_F1                    ^[[M   or                          //
//   VK_F1 SHIFT              ^[[W  etc                          //
//   .                                                           //
//   .                                                           //
//   [SCO_ANSI_END]                                              //
//                                                               //
//   There are three parts :                                     //
//      a) the key name                                          //
//      b) the shift state                                       //
//         here you put compination of the words :               //
//                                                               //
//                RIGHT_ALT                                      //
//                LEFT_ALT                                       //
//                RIGHT_CTRL                                     //
//                LEFT_CTRL                                      //
//                SHIFT                                          //
//                NUMLOCK                                        //
//                SCROLLLOCK                                     //
//                CAPSLOCK                                       //
//                ENHANCED                                       //
//                APP_KEY                                        //
//      c) the assigned string :                                 //
//         you can use the ^ for esc (^[ = 0x1b)                 //
//                         \ and a three digit decimal number    //
//                         (\027)                                //
//         You can't use the NULL !!!                            //
//         Also (for the moment) you can't use spaces            //
//         in the string : everything after the 3rd word is      //
//           ignored - use unsderscore instead.                  //
//                                                               //
//   for  example :                                              //
//                                                               //
//         VK_F4  SHIFT+LEFT_ALT  \0274m^[[M = 0x1b 4 m 0x1b [ M //
//         VK_F1  RIGHT_CTRL      This_is_ctrl_f1                //
//                                                               //
// You may have as many sections as you like                     //
// If you repeat any section (even the GLOBAL) you'll overwrite  //
// the common parts.                                             //
//                                                               //
///////////////////////////////////////////////////////////////////

#ifndef __TLOADMAP_H
#define __TLOADMAP_H

#include "keytrans.h"
#include "tcharmap.h"

// AVS
typedef TArrayAsVector<string> stringArray;

class TMapLoader {
public:
	TMapLoader(KeyTranslator &RefKeyTrans, TCharmap &RefCharmap):
	  KeyTrans(RefKeyTrans), Charmap(RefCharmap) {}
	~TMapLoader() {}

	// If called more than once the new map replaces the old one.
	// load with a different KeysetName to change keysets
	// Return 0 on error
    int Load(const char * filename, const char * szKeysetName);

	void Display();
private:
	KeyTranslator &KeyTrans;
	TCharmap &Charmap;

	int LookForPart(stringArray& sa, const char* partType, const char* partName);
	char* ParseKeyDef(const char* buf, WORD& vk_code, DWORD& control);

	int LoadGlobal(string& buf);
	int LoadKeyMap(string buf);
	int LoadCharMap(string buf);

};

#endif
