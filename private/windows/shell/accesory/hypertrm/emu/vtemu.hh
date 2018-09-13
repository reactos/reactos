/*	File: D:\WACKER\emu\vtemu.hh (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:29p $
 */

/* maxcol definitions */
/* they're one more than normal, line won't wrap until next CHAR to display */
#define VT_MAXCOL_80MODE	80
#define VT_MAXCOL_132MODE	132

/* from autoinit.c */
void emuAutoInit(void);

/* from ansiinit.c */
void emuAnsiInit(void);
void csrv_init(const int new_emu);
int emuAnsiReset(const int);

/* from ansi.c */
VOID ansi_setmode(VOID);
VOID ansi_resetmode(VOID);
VOID ansi_savecursor(VOID);
int ansi_kbdin(int key, const int fTest);

/* from vt52init.c */
void vt52_init(void);

/* from vt52.c */
VOID vt52PrintCommands(VOID);
void vt52Print(void);
VOID vt52_id(VOID);
VOID vt52_CUP(VOID);
INT  vt52_kbdin(int key, const BOOL fTest);

/* from vt100.c */
VOID ANSI_DA(VOID);
VOID vt100_savecursor(VOID);
VOID vt100_answerback(VOID);
VOID vt100_hostreset(VOID);
int  vt100_reset(const int);
VOID vt100_report(VOID);
int  vt100_kbdin(int key, const int);
int  fakevt_kbdin(int key, const int);

/* from vt100ini.c */
VOID wang_init(BOOL new_emu);
VOID ibm3278_init(BOOL new_emu);
VOID renx_init(BOOL new_emu);
void vt100_init(void);

/* from vt220ini.c */
VOID vt220_init(BOOL new_emu);
VOID vt220_savekeys(BOOL save_ptrs);

/* from vt220.c */
VOID vt220_hostreset(VOID);
UINT vt220_reset(BOOL host_request);
VOID vt220_softreset(VOID);
VOID vt220mode_reset(VOID);
VOID vt220_DA(VOID);
VOID vt220_2ndDA(VOID);
VOID vt220_clearkey(VOID);
VOID vt220_definekey(VOID);
VOID vt220_level(VOID);
int  vt220_kbdin(int key, const BOOL fTest);
VOID vt220_protectinsert(BOOL line_only);
VOID vt220_protectdelete(BOOL line_only);
VOID vt220_protmode(VOID);
VOID vt220_protect_IL(int toprow);
VOID vt220_protect_DL(int toprow);
VOID vt220_clearprotect(INT fromrow, INT fromcol, INT torow, INT tocol);
BOOL vt220_isprotected(INT row, INT col);
VOID vt220_deinstall(BOOL quitting);
VOID vt220_scroll(INT nlines, BOOL direction);
VOID vt220_scrollup(INT nlines);
VOID vt220_scrolldown(INT nlines);

/* from vt_chars.c */
VOID vt_charset_init(VOID);
VOID vt_charset_save(VOID);
VOID vt_charset_restore(VOID);
VOID vt_scs1(VOID);
VOID vt_scs2(VOID);
VOID vt_charshift(VOID);
int 	vt_char_emulatecmd(const HEMU hEmu, TCHAR);

/* from vt_xtra.c */
VOID ANSI_DSR(VOID);
VOID ANSI_RM(VOID);
VOID ANSI_SM(VOID);
VOID vt_alt_kpmode(VOID);
VOID vt_screen_adjust(VOID);
VOID vt_shiftscreen(INT key);
VOID vt_kbdlocked(INT key);
VOID vt_scrollrgn(VOID);
VOID DEC_STBM(INT top, INT bottom);
VOID vt52_toANSI(VOID);
VOID vt_DCH(VOID);
VOID vt_IL(VOID);
VOID vt_DL(VOID);
void vt_clearline(const int);
void vt_clearscreen(const int);
VOID vt_graphic(VOID);
VOID vt_backspace(VOID);
VOID vt_CUB(VOID);

// From vt100.c
//
void vt100PrintCommands(void);
VOID vt100_prnc(VOID);

extern TCHAR *vt220_protimg;	/* address of protected bytes map */

/* end of vtemu.hh */
