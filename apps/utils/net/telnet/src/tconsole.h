#ifndef __TNPARSER_H
#define __TNPARSER_H

#include "tnconfig.h"

/* A diagram of the following values:
 *
 *           (0,0)
 *              +----------------------------------------+
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |                                        |
 *              |          CON_TOP                       |
 *              +---------------------------+.....?......|     ---
 *              |              .            |            |      |
 *              |              .            | <-- OR --> |      |
 *              |              .            |            |      |
 *  CON_LEFT    |              .            | CON_RIGHT  |      
 *    (=0)      |              .            | (=CON_     |  CON_LINES
 *              |..............*            |   WIDTH)   |      
 *              |            (CON_CUR_X,    |            |      |
 *              |             CON_CUR_Y)    |            |      |
 *              |                           |            |      |
 *              |                           |            |      |
 *              |                           |            |      |
 *              +---------------------------+------------+     ---
 *                   CON_BOTTOM (=CON_TOP + CON_HEIGHT)
 *
 *              |--------- CON_COLS --------|
 *
 * Keep in mind that CON_TOP, CON_BOTTOM, CON_LEFT, and CON_RIGHT are relative
 * to zero, but CON_CUR_X, CON_CUR_Y, CON_WIDTH, and CON_HEIGHT are relative to
 * CON_TOP and CON_LEFT
 */

#define CON_TOP		ConsoleInfo.srWindow.Top
#define CON_BOTTOM	ConsoleInfo.srWindow.Bottom

#define CON_LEFT	0
#define CON_RIGHT	(ConsoleInfo.dwSize.X - 1)

#define CON_HEIGHT	(CON_BOTTOM - CON_TOP)
#define CON_WIDTH	(CON_RIGHT - CON_LEFT)
#define CON_LINES	(CON_HEIGHT + 1)
#define CON_COLS	(CON_WIDTH + 1)

#define CON_CUR_X	(ConsoleInfo.dwCursorPosition.X - CON_LEFT)
#define CON_CUR_Y	(ConsoleInfo.dwCursorPosition.Y - CON_TOP)


class TConsole {
public:
	TConsole(HANDLE hConsole);
	~TConsole();
	void sync();

	// Cursor movement routines
    int GetRawCursorX() {return CON_CUR_X;}
    int GetRawCursorY() {return CON_CUR_Y;}
    int GetCursorX() {return CON_CUR_X;}
    int GetCursorY() {
		if(iScrollStart != -1)
			return CON_CUR_Y - iScrollStart;
		return GetRawCursorY();
	}
	void SetRawCursorPosition(int x, int y);
	void SetCursorPosition(int x, int y);
	void SetCursorSize(int pct);
	void MoveCursorPosition(int x, int y);
	
	// Screen mode/size routines
    int GetWidth() {return CON_COLS;}
    int GetHeight() {return CON_LINES;}
	void SetExtendedMode(int iFunction, BOOL bEnable);
	void SetWindowSize(int width, int height);	// Set the size of the window,
												// but not the buffer
   
	// Color/attribute routines	
	void SetAttrib(unsigned char wAttr) {wAttributes = wAttr;}
    unsigned char GetAttrib() {return wAttributes;}
    void Normal();								// Reset all attributes
    void HighVideo();							// Aka "bold"
    void LowVideo();
    void SetForeground(unsigned char wAttrib);	// Set the foreground directly
    void SetBackground(unsigned char wAttrib);
    void BlinkOn();								// Blink on/off
    void BlinkOff();
	void UnderlineOn();							// Underline on/off
	void UnderlineOff();
	void UlBlinkOn();							// Blink+Underline on/off
	void UlBlinkOff();
    void ReverseOn();							// Reverse on/off
    void ReverseOff();
	void Lightbg();								// High-intensity background
	void Darkbg();								// Low-intensity background
	void setDefaultFg(unsigned char u) {defaultfg = u;}
	void setDefaultBg(unsigned char u) {defaultbg = u;}

	// Text output routines
	unsigned long WriteText(const char *pszString, unsigned long cbString);
	unsigned long WriteString(const char* pszString, unsigned long cbString);
	unsigned long WriteStringFast(const char *pszString, unsigned long cbString);
	unsigned long WriteCtrlString(const char* pszString, unsigned long cbString);
	unsigned long WriteCtrlChar(char c);
	unsigned long NetWriteString(const char* pszString, unsigned long cbString);

	// Clear screen/screen area functions
	void ClearScreen(char c = ' ');
	void ClearWindow(int start, int end, char c = ' ');
	void ClearEOScreen(char c = ' ');
	void ClearBOScreen(char c = ' ');
	void ClearLine(char c = ' ');
	void ClearEOLine(char c = ' ');
	void ClearBOLine(char c = ' ');

	// Scrolling and text output control functions
	void SetScroll(int start, int end);
    void ScrollDown(int iStartRow , int iEndRow, int bUp);
	void ScrollAll(int bUp) {ScrollDown(iScrollStart, iScrollEnd, bUp);}
	void index();
	void reverse_index();
	void setLineWrap(bool bEnabled){
		if(!ini.get_lock_linewrap()) 
			ini.set_value("Wrap_Line", bEnabled ? "true" : "false");
	}
	bool getLineWrap() {return ini.get_wrapline();}

    // Insert/delete characters/lines
	void InsertLine(int numlines);			// Added by Titus von Boxberg 30/3/97		
    void InsertCharacter(int numchar);		// "
    void DeleteCharacter(int numchar);		// "
	void InsertMode(int i) {insert_mode = i;}

	// Miscellaneous functions
	void Beep();

protected:
	HANDLE hConsole;

	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	
	unsigned char wAttributes;
	unsigned char fg, bg;
	unsigned char defaultfg, defaultbg;
	unsigned char origfg, origbg;

	bool blink;
	bool underline;
	bool reverse;
	
	int iScrollStart;
	int iScrollEnd;
	int insert_mode;
};

// Non-member functions for saving state -- used by the scrollback buffer viewer
void saveScreen(CHAR_INFO* chiBuffer);
void restoreScreen(CHAR_INFO* chiBuffer);
CHAR_INFO* newBuffer();
void deleteBuffer(CHAR_INFO* chiBuffer);

#endif
