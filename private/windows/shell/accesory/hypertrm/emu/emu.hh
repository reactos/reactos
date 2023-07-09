/*	File: D:\WACKER\emu\emu.hh (Created: 08-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */
typedef struct stEmuInternal *HHEMU;

// Maximum column definitions.
// They're one more than zero base maximums.  Lines won't
// wrap until next char is displayed.
//
#define VT_MAXCOL_80MODE	79
#define VT_MAXCOL_132MODE	131

// Define terminal modes using ANSI terminology.
//
#define SET TRUE
#define RESET FALSE

// Character constants.
//
#define SOH 	001
#define STX 	002
#define ETX 	003
#define EOT 	004
#define ENQ 	005
#define ACK 	006
#define BELL	007
#define BS		010
#define LF		012
#define FF		014
#define RET 	015
#define DLE 	020
#define DC3 	023
#define NAK 	025
#define CAN 	030
#define ESC 	033
#define DEL 	177
#define IND 	204
#define SS3 	217
#define CSI 	233

// Definitions for double high, double wide character processing.
//
#define NO_LINE_ATTR			0
#define DBL_WIDE_HI 			1
#define DBL_WIDE_LO 			2
#define DBL_WIDE_SINGLE_HEIGHT	3

#define MAX_STATE		35	/* Maximum states in FSA */
#define MAX_TRANSITION 200	/* Maximum state-to-state transitions */
#define MAX_NUM_PARAM	10	/* Max # numeric parameters in one cmd */
#define MAX_SELECTOR	10	/* Max # selectors in one terminal command */
#define NEW_STATE	   255	/* Special marker to indicate new state */

#define CLEAR_CURSOR_TO_SCREEN_END      0
#define CLEAR_SCREEN_START_TO_CURSOR    1
#define CLEAR_ENTIRE_SCREEN             2

#define CLEAR_CURSOR_TO_LINE_END		0
#define CLEAR_LINE_START_TO_CURSOR		1
#define CLEAR_ENTIRE_LINE				2

#define CLEAR_TAB_AT_CURSOR             0
#define CLEAR_TABS_IN_LINE              3

#define CS_STATE		 0
#define CSCLEAR_STATE	 1

#define EMU_BLANK_LINE	(-1)
#define EMU_BLANK_CHAR	TEXT('\x20')

// Key table definitions.
//
#define VK_BACKSPACE	(VK_BACK   | VIRTUAL_KEY)
#define DELETE_KEY		(VK_DELETE | VIRTUAL_KEY)
#define DELETE_KEY_EXT	(VK_DELETE | VIRTUAL_KEY | EXTENDED_KEY)

/* -------------- Key Table Transalation ------------- */

typedef struct
	{
	int key;
	int fPointer;	// TRUE means we use the CHAR * portion of the union.
	int uLen;		// length of stored keystring.
	union
		{
		TCHAR	achKeyStr[sizeof(LPTSTR)]; // want real chars here.
		TCHAR  *pachKeyStr; 			  // want real chars here.
		} u;
	} KEY;

typedef KEY * PSTKEY;

typedef struct
	{
	int 	  iMaxKeys;
	PSTKEY	  pstKeys;
	} KEYTABLE;

typedef KEYTABLE * PSTKEYTABLE;

typedef struct KeyTblStorage
    {
	int KeyCode; 
	TCHAR achKeyStr[15];
	} KEYTBLSTORAGE;

typedef struct emuNameTable
	{
	TCHAR	acName[EMU_MAX_NAMELEN];
	int 	nEmuId;
	} STEMUNAMETABLE;

typedef STEMUNAMETABLE * PSTEMUNAMETABLE;

/* Define the Finite State Automaton (FSA) which parses commands.
 * There is an array of states which points into an array of transitions
 * from that state.
 */

struct state_entry
	{
	struct trans_entry *first_trans;	/* Pointer into transition array */
	int number_trans;				  /* Number of paths out of state */
	};

struct trans_entry
	{
	int next_state; 				  /* Next state after match */
	ECHAR lochar, hichar;				/* Match range of input chars */
	void (*funct_ptr)(const HHEMU hhEmu);			 /* Pointer to function */
	};

// The internal Emulator Handle
//
struct stEmuInternal
	{
	CRITICAL_SECTION csEmu; 		// Used to synchronize access

	void *pvPrivate;
						 
	HSESSION hSession;				// Session hdl that created this.

	HPRINT	hPrintEcho, 			// Print hdl for Printer Echo.
			hPrintHost; 			// Print hdl for Host directed printing.

	TCHAR	acAnswerback[21];

	PSTEMUNAMETABLE pstNameTable;

	int 	nEmuLoaded, 			// Identifies the emulator that is
									// currently loaded.  Use in emuLoad
									// to determine if requested emulator
									// is already loaded.
			iCurType,				// cursor type
			fWasConnected;			// used with auto attempts


	STEMUSET	stUserSettings; 	// Contains all the settings made by the 
									// user. They are initilaized from values
									// stored in the session file.

	// Variables for state table processing.
	//
	struct state_entry state_tbl[MAX_STATE];
	struct trans_entry trans_tbl[MAX_TRANSITION];

	int state,						/* State table state			*/
		num_param[MAX_NUM_PARAM],	/* Numeric valued parameters	*/
		num_param_cnt,				/* Number of parameters 		*/
		selector[MAX_SELECTOR], 	/* (Hex) option selectors		*/
		selector_cnt;				/* Number of selectors			*/

	ECHAR	emu_code,				/* current character to process */
			emu_highchar;			/* highest CHAR to bypass state table */

	// Keyboard processing tables and variables.
	//
	KEYTABLE stEmuKeyTbl,					/* the dreaded keytable 		*/
			 stEmuKeyTbl2,					/* modal keytable				*/
			 stEmuKeyTbl3,					/* modal keytable				*/
			 stEmuKeyTbl4;					/* modal keytable 				*/

	// Character attribute state information.
	//
	int iCurAttrState;

	STATTR attrState[2];

	// Virtual image variables
	//
	int emu_maxrow, 			/* maximum virtual row of emulator */
		emu_maxcol, 			/* maximum virtual column of emualtor */
		emu_currow, 			/* emulator's cursor row */
		emu_curcol, 			/* emulator's cursor column */
		emu_imgtop, 			/* line in image array of screen row 0 */
		emu_imgrow, 			/* line in image array of cursor */
		top_margin,
		bottom_margin,
		scr_scrollcnt;			/* Keeps track of screen scrolls */

	// Is the loaded emulator DBCS Enabled
	int	fDBCSSupported;			/* Emulator is DBCS Enabled	True\False. */

	// JFH:2/22/95 TCHAR	dspchar[256];		/* Character display map */
	ECHAR	dspchar[256];		/* Character display map */

	int 	tab_stop[MAX_EMUCOLS + 1],
			print_echo;

	// Pointers to text, attribute and end of line arrays.
	//
	ECHAR	*(*emu_apText);
	PSTATTR *emu_apAttr;
	int 	*emu_aiEnd;

	// Character attribute variables.
	//
	STATTR	emu_clearattr,			 /* current CHAR attribute for clearing */
			emu_clearattr_sav,		 /* Used for HA/Win - mrw */
			emu_charattr;			 /* current physical character attribute */

	// Emulator mode variables
	//
	int mode_KAM,		/* Keyboard Action Mode. RESET=enabled					*/
		mode_IRM,		/* Insertion-Replacement. RESET=replace chars			*/
		mode_VEM,		/* Vertical Editing. RESET=ins/del lines below cursor	*/
		mode_HEM,		/* Horizontal Editing. RESET=ins/del chars after cursor */
		mode_SRM,		/* Send-Receive. RESET=local character echo 			*/
		mode_AWM,		/* AutoWrap (not ANSI). RESET=wrap to next line 		*/
		mode_LNM,		/* Line feed New line. RESET=LF moves vertically only	*/
		mode_DECCKM,	/* cursor key codes. RESET=cursor, SET=application		*/
		mode_DECKPAM,	/* keypad key codeas. RESET=numeric, SET=application	*/
		mode_DECOM, 	/* DEC origin mode. RESET=cursor posn screen relative	*/
		mode_DECCOLM,	/* DEC column mode. RESET=80 column display 			*/
		mode_DECPFF,	/* Print form feed. RESET=OFF							*/
		mode_DECPEX,	/* Print extent. RESET=scroll rgn., SET=full screen 	*/
		mode_DECSCNM,	/* Screen mode. RESET=normal video, SET=reverse video	*/
		mode_DECTCEM,	/* Cursor enable. RESET=hidden, SET=visible 			*/
		mode_25enab,	/* When true (SET), emulator can use 25th line			*/
		mode_protect,	/* When true (SET), protected mode is on				*/
		mode_block, 	/* When true (SET), block mode is on					*/
		mode_local, 	/* When true (SET), local mode is on					*/
		mode_vt220,
		mode_vt280,		/* Acts as a Kanji/Katakana terminal 					*/
		mode_vt320,

        fUse8BitCodes,	// Applies to the VT220/320 emulators.
        fAllowUserKeys,	// Ditto.

		iZmodemState;	// This variable is used by all of the emulators
						// for processing AutoStart Zmodem codes.

	int DEC_private;   	// Set when '?' is part of sequence

	// Function pointer definitions.  These pointers get assigned to
	// emulator functions during the loading and initialization of a
	// specific emulator...emuLoad().
	//
#if defined(EXTENDED_FEATURES)
	int (*emu_datain)		(const HHEMU hhEmu, const ECHAR ccode);
#else
	int (*emu_datain)		(const HEMU hEmu, const ECHAR ccode);
#endif
	int (*emu_kbdin)		(const HHEMU hhEmu, int kcode, const int fTest);
	int (*emu_getscrollcnt) (const HHEMU hhEmu);
	int (*EmuScreenMode)	(const HHEMU hhEmu, int d);
	int (*emuResetTerminal) (const HHEMU hhEmu, int n);

	void (*emu_graphic) 	(const HHEMU hhEmu);
	void (*emu_setcolors)	(const HHEMU hhEmu, int fore, int back);
	void (*emu_getcolors)	(const HHEMU hhEmu, int *fore, int *back);
	void (*emu_initcolors)	(const HHEMU hhEmu);
	void (*emu_getscrsize)	(const HHEMU hhEmu, int *rows, int *cols);
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    void (*emu_setscrsize)  (const HHEMU hhEmu);	// Added 10 Jun 98 rde
#endif
	void (*emu_getcurpos)	(const HHEMU hhEmu, int *row, int *col);
	void (*emu_setcurpos)	(const HHEMU hhEmu, int row, int col);
	void (*emu_clearscreen) (const HHEMU hhEmu, int selector);
	void (*emu_clearline)	(const HHEMU hhEmu, int selector);
	void (*emu_setattr) 	(const HHEMU hhEmu, PSTATTR pstAttr);
	void (*emu_scroll)		(const HHEMU hhEmu, int nlines, BOOL direction);
	void (*emu_deinstall)	(const HHEMU hhEmu);
	void (*EmuSetCursorType)(const HHEMU hhEmu, int iCurType);
	void (*emu_ntfy)		(const HHEMU hhEmu, const int nNtfy);
	void (*emu_clearrgn)	(const HHEMU hhEmu,
								int toprow,
								int leftcol,
								int botmrow,
								int rightcol);

	ECHAR (*EmuGetPrnChar)	(const HHEMU hhEmu, ECHAR uch);
	STATTR (*emu_getattr)	(const HHEMU hhEmu);
	int (*emuHomeHostCursor)(const HHEMU hhEmu);
	};

// These are a group of settings saved in the session file as a block. Later
// settings were added to the session file individually rather than in a 
// structure. Do not add or remove any items from this structure!! rde 8 Jun 1998
struct stEmuBaseSFSettings
	{
	int 	nEmuId, 			// 100 = EMU_AUTO
								// 101 = EMU_ANSI
								// 102 = EMU_MINI
								// 109 = EMU_VIEW
								// 110 = EMU_TTY
								// 111 = EMU_VT100
								// 112 = EMU_VT220
								// 113 = EMU_VT320
								// 115 = EMU_VT52
								// 116 = EMU_VT100J
								//
			nTermKeys,			// 0 = EMU_KEYS_ACCEL
								// 1 = EMU_KEYS_TERM
								// 2 = EMU_KEYS_SCAN
								//
			nCursorType,		// 1 = EMU_CURSOR_BLOCK
								// 2 = EMU_CURSOR_LINE
	        					// 3 = EMU_CURSOR_NONE
								//
			nCharacterSet,		// 0 = EMU_CHARSET_ASCII
								// 1 = EMU_CHARSET_UK
								// 2 = EMU_CHARSET_SPECIAL
								//
			nAutoAttempts,		// Count of connections using the Auto
								// Detect Emulator.  At
								// EMU_MAX_AUTODETECT_ATTEMPTS, we switch
								// to Ansi emulation.  Note, this may
								// get moved into a Statictics Handle
								// if we ever develop one.
								//
			fCursorBlink,		// Blinking cursor. 			True\False.
			fMapPFkeys, 		// PF1-PF4 to top row of keypad.True\False.
			fAltKeypadMode, 	// Alternate keypad mode.		True\False.
			fKeypadAppMode, 	// Keypad application mode. 	True\False.
			fCursorKeypadMode,	// Cursor keypad mode.			True\Fales.
			fReverseDelBk,		// Reverse Del and Backsp.		True\False.
			f132Columns,		// 132 column display.			True\False.
			fDestructiveBk, 	// Destructive backspace.		True\False.
			fWrapLines, 		// Wrap lines.					True\False.
			fLbSymbolOnEnter,	// Send # symbol on Enter.		True\False.

	// Note: The following two variables were added for the VT220/320. rde:24 Jan 98
            fUse8BitCodes,      // 8-bit control codes          True\False.
            fAllowUserKeys;     // User defined keys allowed    True\False.
                                
    // Note: The following two variables are only used if the "Include
    // User Defined Backspace and Telnet Terminal Id" feature is enabled.
    // There is no compile switch here because this entire structure gets
    // written to the session file in one large chunk. Using a compile
    // switch could potentially cause version problems later on down
    // the road. - cab:11/15/96
    //
    int     nBackspaceKeys;     // 1 = EMU_BKSPKEYS_CTRLH
                                // 2 = EMU_BKSPKEYS_DEL
                                // 3 = EMU_BKSPKEYS_CTRLHSPACE

    TCHAR   acTelnetId[EMU_MAX_TELNETID];   // Telnet terminal ID
	};

// This macro returns the virtual image row of the supplied row number.  That is,
// what appears as row 10 on the terminal image may actually be row 5 in the
// virtual image.
//
#define row_index(h, r) (((r) + h->emu_imgtop + MAX_EMUROWS) % (MAX_EMUROWS))

// The emualtor image is an array of characters.  emu_aiEnd is an array that
// contains the column number of the rightmost character in a given row.
// This macro returns the a number representing the length of the image for
// the supplied row, from location 0 to the rightmost column.  It
// simplifies accessing a row of the emulator as though it were a string.
// Remember, the emualtor matrix is zero based, and EMU_BALNK_LINE indicates
// that there are no characters in the row.
//
#define emuRowLen(h, r) ((h->emu_aiEnd[r] == EMU_BLANK_LINE) ? 0 : h->emu_aiEnd[r] + 1)

// from emu_std.c
//
int 	std_kbdin(const HHEMU hhEmu, int kcode, const int fTest);
int 	std_getscrollcnt(const HHEMU hhEmu);
void	std_getscrsize(const HHEMU hhEmu, int *rows, int *cols);
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
void	std_setscrsize(const HHEMU hhEmu);
#endif
void	std_getcurpos(const HHEMU hhEmu, int *row, int *col);
void	std_setcurpos(const HHEMU hhEmu, const int iRow, const int iCol);
STATTR	std_getattr(const HHEMU hhEmu);
void	std_setattr(const HHEMU hhEmu, PSTATTR pstAttr);
void	std_setcolors(const HHEMU hhEmu, const int fore, const int back);
void	std_getcolors(const HHEMU hhEmu, int *fore, int *back);
void	std_initcolors(const HHEMU hhEmu);
void	std_restorescreen(const HHEMU hhEmu);
void	std_clearscreen(const HHEMU hhEmu, const int nClearSelect);
void	std_clearline(const HHEMU hhEmu, const int nClearSelect);
void	std_clearrgn(const HHEMU hhEmu, int toprow, int leftcol, int botmrow, int rightcol);
void	EmuStdSetCursorType(const HHEMU hhEmu, int iCurType);
void	EmuChkChar(const HHEMU hhEmu);
void	EmuStdChkZmdm(const HHEMU hhEmu);
void	std_dsptbl(const HHEMU hhEmu, int bit8);
void	std_scroll(const HHEMU hhEmu, const INT nlines, const BOOL direction);
void	std_deinstall(const HHEMU hhEmu);
void	vt_dsptbl(const HHEMU hhEmu, ECHAR left, ECHAR right);
void	vt_setdtbl(const HHEMU hhEmu, ECHAR tbl[], ECHAR cset);
int 	stdResetTerminal(const HHEMU hhEmu, const int);
void	std_emu_ntfy(const HHEMU hhEmu, const int nNtfy);
int 	std_HomeHostCursor(const HHEMU hhEmu);

// from emu_scr.c
//
void	backspace(const HHEMU hhEmu);
void	carriagereturn(const HHEMU hhEmu);
void	emuLineFeed(const HHEMU hhEmu);
void	emuPrintChars(const HHEMU hhEmu, ECHAR *bufr, int nLen);
void	scrolldown(const HHEMU hhEmu, int nlines);
void	scrollup(const HHEMU hhEmu, int nlines);
void	tab(const HHEMU hhEmu);
void	backtab(const HHEMU hhEmu);
void	tabn(const HHEMU hhEmu);
void	emu_bell(const HHEMU hhEmu);
void	emu_clearword(const HHEMU hhEmu, int fromcol, int tocol);
void	clear_imgrow(const HHEMU hhEmu, int iRow);

// From autoinit.c
//
void emuAutoInit(const HHEMU hhEmu);

// From emu_ansi.c
//
void	ANSI_CNL(const HHEMU hhEmu, int nlines);
void	ANSI_CUB(const HHEMU hhEmu);
void	ANSI_CUD(const HHEMU hhEmu);
void	ANSI_CUF(const HHEMU hhEmu);
void	ANSI_CUP(const HHEMU hhEmu);
void	ANSI_CUU(const HHEMU hhEmu);
void	ANSI_DL(const HHEMU hhEmu);
void	ANSI_ED(const HHEMU hhEmu);
void	ANSI_EL(const HHEMU hhEmu);
void	ANSI_DCH(const HHEMU hhEmu);
void	AnsiFormFeed(const HHEMU hhEmu);
void	ANSI_HTS(const HHEMU hhEmu);
void	ANSI_ICH(const HHEMU hhEmu);
void	ANSI_IL(const HHEMU hhEmu);
void	ANSI_IND(const HHEMU hhEmu);
void	ANSI_NEL(const HHEMU hhEmu);
void	ANSI_Pn(const HHEMU hhEmu);
void	ANSI_Pn_Clr(const HHEMU hhEmu);
void	ANSI_Pn_End(const HHEMU hhEmu);
void	ANSI_RI(const HHEMU hhEmu);
void	ANSI_RIS(const HHEMU hhEmu);
void	ANSI_SGR(const HHEMU hhEmu);
void	ANSI_TBC(const HHEMU hhEmu);

// From emu.c
//
void	nothing(const HHEMU hhEmu);
void	char_pn(const HHEMU hhEmu);
void	commanderror(const HHEMU hhEmu);
void	emuStdGraphic(const HHEMU hhEmu);
int 	emuCreateTextAttrBufs(const HEMU hEmu, const size_t nRows, size_t nCols);
void	emuDestroyTextAttrBufs(const HEMU hEmu);
void	emu_cleartabs(const HHEMU hhEmu, int selector);
void	emu_reverse_image(const HHEMU hhEmu);
int 	emu_is25lines(const HHEMU hhEmu);
int 	emuKbdKeyLookup(const HHEMU hhEmu, const int key, const PSTKEYTABLE pstKeyTbl);
void	emuInstallStateTable(const HHEMU hhEmu, struct trans_entry const *, int iSize);
//int 	emuKeyTableLoad(const HHEMU hhEmu, const int nTableId, PSTKEYTABLE const pstKeyTbl);
int 	emuKeyTableLoad(const HHEMU hhEmu, const KEYTBLSTORAGE pstKeySource[], const int nNumKeys, PSTKEYTABLE const pstKeyTbl);
int 	emu_kbdlocked(const HHEMU hhEmu, int key, const int fTest);
void 	emuSendKeyString(const HHEMU hhEmu, const int index, const PSTKEYTABLE pstKeyTbl);
void	emuSendString(const HHEMU hhEmu, ECHAR *str, int strln);
void	emuKeyTableFree(PSTKEYTABLE const pstKeyTbl);
int		emuSetEmuName(const HEMU hEmu, const int nEmuId);
void	emuAutoDetectLoad(const HHEMU hhEmu, const int nEmuID);

// From vt_xtra.c
//
void	emuSetDecColumns(const HHEMU hhEmu, const int nColumns, const int fClear);
void	ANSI_DSR(const HHEMU hhEmu);
void	vt_scrollrgn(const HHEMU hhEmu);
void	ANSI_RM(const HHEMU hhEmu);
void	ANSI_SM(const HHEMU hhEmu);
void	vt_alt_kpmode(const HHEMU hhEmu);
void	vt_screen_adjust(const HHEMU hhEmu);
void	DEC_STBM(const HHEMU hhEmu, int top, int bottom);
void	vt52_toANSI(const HHEMU hhEmu);
void	vt_DCH(const HHEMU hhEmu);
void	vt_IL(const HHEMU hhEmu);
void	vt_DL(const HHEMU hhEmu);
void	vt_clearline(const HHEMU hhEmu, const int nSelect);
void	vt_clearscreen(const HHEMU hhEmu, const int nSelect);
void	vt_backspace(const HHEMU hhEmu);
void	vt_CUB(const HHEMU hhEmu);

// From ansi.c
//
void	ansi_setmode(const HHEMU hhEmu);
void	ansi_resetmode(const HHEMU hhEmu);
int 	ansi_kbdin(const HHEMU hhEmu, int key, const int fTest);
void	ansi_savecursor(const HHEMU hhEmu);
void	DoorwayMode(const HHEMU hhEmu);
void	emuAnsiUnload(const HHEMU hhEmu);

// From ansiinit.c
//
int 	emuAnsiReset(const HHEMU hhEmu, const int fHostRequest);
void	emuAnsiInit(const HHEMU hhEmu);
void	csrv_init(const HHEMU hhEmu, const int new_emu);

// From vt100.c
//
int 	vt100_kbdin(const HHEMU hhEmu, int key, const int fTest);
void	ANSI_DA(const HHEMU hhEmu);
void	vt100_savecursor(const HHEMU hhEmu);
void	vt100_hostreset(const HHEMU hhEmu);
int 	vt100_reset(const HHEMU hhEmu, const int host_request);
int 	fakevt_kbdin(const HHEMU hhEmu, int key, const int fTest);
void	vt100_prnc(const HHEMU hhEmu);
void	vt100PrintCommands(const HHEMU hhEmu);
void	vt100_answerback(const HHEMU hhEmu);
void	vt100_report(const HHEMU hhEmu);
void	emuDecGraphic(const HHEMU hhEmu);
void	emuSetDoubleAttr(const HHEMU hhEmu);
void	emuSetSingleAttrRow(const HHEMU hhEmu);
void	emuSetDoubleAttrRow(const HHEMU hhEmu, const int iLineAttr);
void	emuFromDblToSingle(const HHEMU hhEmu);
void	emuDecTab(const HHEMU hhEmu);
void	emuDecCUF(const HHEMU hhEmu);
void	emuDecCUP(const HHEMU hhEmu);
void	emuDecCUB(const HHEMU hhEmu);
void	emuDecED(const HHEMU hhEmu);
void	emuVT100Unload(const HHEMU hhEmu);
void	emuDecIND(const HHEMU hhEmu);
void	emuDecRI(const HHEMU hhEmu);
void	emuDecCUU(const HHEMU hhEmu);
void	emuDecCUD(const HHEMU hhEmu);
void	emuDecSetCurPos(const HHEMU hhEmu, const int iRow, const int iCol);
void	emuDecClearScreen(const HHEMU hhEmu, const int iSelector);

// From vt100ini.c
//
void	vt100_init(const HHEMU hhEmu);

// From vt_chars.c
//
void	vt_charset_init(const HHEMU hhEmu);
void	vt_charset_save(const HHEMU hhEmu);
void	vt_charset_restore(const HHEMU hhEmu);
void	vt_charshift(const HHEMU hhEmu);
void	vt_scs1(const HHEMU hhEmu);
void	vt_scs2(const HHEMU hhEmu);
#if defined(EXTENDED_FEATURES)
int 	vt_char_emulatecmd(const HHEMU hhEmu, const ECHAR ccode);
#else
int 	vt_char_emulatecmd(const HEMU hEmu, const ECHAR ccode);
#endif

// From vt52.c
//
void	vt52PrintCommands(const HHEMU hhEmu);
void	vt52Print(const HHEMU hhEmu);
void	vt52_id(const HHEMU hhEmu);
void	vt52_CUP(const HHEMU hhEmu);
int 	vt52_kbdin(const HHEMU hhEmu, int key, const BOOL fTest);
void	emuVT52Unload(const HHEMU hhEmu);

// From vt52init.c
//
void	vt52_init(const HHEMU hhEmu);

// From emuhdl.c
int emuCreateNameTable(const HHEMU hhEmu);
int emuStdDataIn(const HHEMU hhEmu, const ECHAR ccode);

