/*	File: D:\WACKER\help\hlptable.h (Created: 7-Jan-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 3/26/99 8:07a $
 */

// NOTE:
// This file contains HELP CONTEXT Identifiers.
// This file is included internally and should be included by *.HPJ file
// when the .HLP file will be created.

// New Connection Description Dialog Box
//
#define IDH_TERM_NEWCONN_NAME			40960	// 0xA000
#define IDH_TERM_NEWCONN_ICON   		40961
#define IDH_BROWSE              		40962

//
// New Connection Phone Number Dialog Box.
//
#define IDH_TERM_NEWPHONE_COUNTRY		40970
#define IDH_TERM_NEWPHONE_AREA      	40971
#define IDH_TERM_NEWPHONE_NUMBER    	40972
#define IDH_TERM_NEWPHONE_DEVICE    	40973
#define IDH_TERM_NEWPHONE_CONFIGURE		40974	// New help identifier
#define IDH_TERM_NEWPHONE_HOSTADDRESS   40975
#define IDH_TERM_NEWPHONE_PORTNUMBER    40976
#define IDH_TERM_NEWPHONE_REDIAL        40977   // New help identifier
#define IDH_TERM_NEWPHONE_USECCAC       40978
//
// Dial Or Open Confirmation Dialog Box.
//
#define IDH_TERM_DIAL_MODIFY			40980
#define IDH_TERM_DIAL_EDITNEW       	40981
#define IDH_TERM_DIAL_LOCATION      	40982
#define IDH_TERM_DIAL_DIAL          	40983
#define IDH_TERM_DIAL_OPEN          	40984
#define IDH_TERM_DIAL_CALLING_CARD		40985	// New help identifier
#define IDH_TERM_DIAL_PHONENUMBER		40986	// New help identifier

//
// Phone Number Properties Sheet
//
// Deleted IDH_TERM_PHONEPROP_... ids, not used
#define IDH_TERM_PHONEPROP_CHANGEICON	41001

//
// Terminal Properties Sheet
//                                      41010
#define IDH_TERM_SETTING_EMULATION      41011	// New id
#define IDH_TERM_SETTING_BACKSCROLL		41012	
#define IDH_TERM_SETTING_ASCIISET		41013	
#define IDH_TERM_SETTING_SOUND			41014	
#define IDH_TERM_SETTING_USEKEYS		41015	
#define IDH_TERM_SETTING_TERMSET		41016	
#define IDH_TERM_SETTING_BACKSPACE		41017
#define IDH_TERM_SETTING_CTRLH			41018
#define IDH_TERM_SETTING_DEL			41019
#define IDH_TERM_SETTING_CTRLH2			41020
#define IDH_TERM_SETTING_TELNETID		41021
#define IDH_TERM_SETTING_HIDE_CURSOR    41022
#define IDH_TERM_SETTING_ENTER          41023

//
// Receive File Dialog Box
//
#define IDH_TERM_RECEIVE_DIRECTORY      41030
#define IDH_TERM_RECEIVE_PROTOCOL		41031
#define IDH_TERM_RECEIVE_RECEIVE		41032

//
// Send Dialog Box
//
#define IDH_TERM_SEND_FILENAME			41040
#define IDH_TERM_SEND_PROTOCOL			41041
#define IDH_TERM_SEND_SEND				41042
#define IDH_TERM_SEND_FOLDER            41043

//
// General help for terminal window
//
//#define IDH_TERM_WINDOW				  41050

//
// Emulator Settings dialogs
//
#define IDH_TERM_EMUSET_MODES			41071	// New help identifier
#define IDH_TERM_EMUSET_CURSOR			41072
#define IDH_TERM_EMUSET_DESTRUCTIVE		41073
#define IDH_TERM_EMUSET_ALTMODE			41074
#define IDH_TERM_EMUSET_KEYPADMODE		41075
#define IDH_TERM_EMUSET_CURSORMODE		41076
#define IDH_TERM_EMUSET_132COLUMNS		41077
#define IDH_TERM_EMUSET_CHARSETS		41078
#define IDH_TERM_EMUSET_8BITCODES		41079	
#define IDH_TERM_EMUSET_USERDEFKEYS		41080
#define IDH_TERM_SETTING_EXIT			41082
#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
#define IDH_TERM_EMUSET_ROWSANDCOLS		41083
#define IDH_TERM_EMUSET_COLORS			41084
#define IDH_TERM_SETTING_COLOR			41085
#define IDH_TERM_COLOR_PREVIEW			41086
#endif
//
// ASCII dialog box
//
#define IDH_TERM_ASCII_SEND_LINE		41090
#define IDH_TERM_ASCII_SEND_ECHO		41091
#define IDH_TERM_ASCII_REC_APPEND		41092
#define IDH_TERM_ASCII_REC_FORCE		41093
#define IDH_TERM_ASCII_REC_WRAP			41094
#define IDH_TERM_ASCII_SENDING			41095 	// New help identifiers
#define IDH_TERM_ASCII_RECEIVING		41096	// New help identifiers
#define IDH_TERM_ASCII_LINE_DELAY		41097
#define IDH_TERM_ASCII_CHAR_DELAY		41098

//
// Capture dialog box
//
#define IDH_TERM_CAPT_FILENAME			41100
#define IDH_TERM_CAPT_DIRECTORY			41101
#define IDH_TERM_CAPT_START             41102

// What's This from the context menu
//
//#define IDH_TERM_CONTEXT_WHATS_THIS	  41110

//
// Used in receive and send dialogs.
//
#define IDH_CLOSE_DIALOG				41120

//
// JIS to Shift JIS translation Dialog
//
#define IDH_HTRN_DIALOG					41130
#define IDH_HTRN_SHIFTJIS				41131
#define IDH_HTRN_JIS					41132

//
// Default Telnet app dialog
//
#define IDH_TELNETCK_STOP_ASKING		41140
#define IDH_TELNETCK_YES				41141
#define IDH_TELNETCK_NO					41142

//
// key macros
//
#define IDH_LB_KEYS_KEYLIST        41150
#define IDH_PB_KEYS_MODIFY         41151
#define IDH_PB_KEYS_NEW            41152
#define IDH_PB_KEYS_DELETE         41153
#define IDH_EF_KEYS_KEYNAME        41154
#define IDH_ML_KEYS_MACRO          41155

BOOL isControlinHelpTable(const DWORD aHlpTable[], const INT cntrlID);
void doContextHelp(const DWORD aHlpTable[], WPARAM wPar, LPARAM lPar, BOOL bContext, BOOL bForce);