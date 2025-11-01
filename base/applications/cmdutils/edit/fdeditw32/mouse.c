/* ------------- mouse.c ------------- */

#include "dflat.h"

static union REGS regs;
static struct SREGS sregs;

static void near mouse(int m1,int m2,int m3,int m4)
{
    regs.x.dx = m4;
    regs.x.cx = m3;
    regs.x.bx = m2;
    regs.x.ax = m1;
    int86x(MOUSE, &regs, &regs, &sregs);
}

/* ---------- reset the mouse ---------- */
void resetmouse(void)
{
	segread(&sregs);
    mouse(0,0,0,0);
}

/* ----- test to see if the mouse driver is installed ----- */
BOOL mouse_installed(void)
{
    unsigned char far *ms;
    ms = MK_FP(peek(0, MOUSE*4+2), peek(0, MOUSE*4));
    return (SCREENWIDTH <= 80 && ms != NULL && *ms != 0xcf);
}

/* ------ return true if mouse buttons are pressed ------- */
int mousebuttons(void)
{
    if (mouse_installed())	{
		segread(&sregs);
        mouse(3,0,0,0);
	    return regs.x.bx & 3;
	}
	return 0;
}

/* ---------- return mouse coordinates ---------- */
void get_mouseposition(int *x, int *y)
{
	*x = *y = -1;
    if (mouse_installed())    {
		segread(&sregs);
        mouse(3,0,0,0);
        *x = regs.x.cx/8;
        *y = regs.x.dx/8;
		if (SCREENWIDTH == 40)
			*x /= 2;
    }
}

/* -------- position the mouse cursor -------- */
void set_mouseposition(int x, int y)
{
    if (mouse_installed())	{
		segread(&sregs);
		if (SCREENWIDTH == 40)
			x *= 2;
        mouse(4,0,x*8,y*8);
	}
}

/* --------- display the mouse cursor -------- */
void show_mousecursor(void)
{
    if (mouse_installed())	{
		segread(&sregs);
        mouse(1,0,0,0);
	}
}

/* --------- hide the mouse cursor ------- */
void hide_mousecursor(void)
{
    if (mouse_installed())	{
		segread(&sregs);
        mouse(2,0,0,0);
	}
}

/* --- return true if a mouse button has been released --- */
int button_releases(void)
{
    if (mouse_installed())	{
		segread(&sregs);
        mouse(6,0,0,0);
	    return regs.x.bx;
	}
	return 0;
}

/* ----- set mouse travel limits ------- */
void set_mousetravel(int minx, int maxx, int miny, int maxy)
{
    if (mouse_installed())	{
		if (SCREENWIDTH == 40)	{
			minx *= 2;
			maxx *= 2;
		}
		segread(&sregs);
        mouse(7, 0, minx*8, maxx*8);
		mouse(8, 0, miny*8, maxy*8);
	}
}

