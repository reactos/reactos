#ifndef __TMOUSE_H
#define __TMOUSE_H

#include "tnclip.h"
#include <windows.h>

class TMouse {
private:
	int normal, inverse;
	HANDLE hConsole, hStdout;
	CHAR_INFO *chiBuffer;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	Tnclip &Clipboard;

	void get_coords(COORD *start_coords, COORD *end_coords,
		COORD *first_coords, COORD *last_coords);
	void doMouse_init();
	void doMouse_cleanup();
	void move_mouse(COORD start_coords, COORD end_coords);
	void doClip(COORD start_coords, COORD end_coords);

public:
	void doMouse();
	void scrollMouse();
	TMouse(Tnclip &RefClipboard);
	~TMouse();
};

#endif
