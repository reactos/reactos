/*
 * Simple program to display the Windows System Metrics.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Robert Dickenson <robd@reactos.org>, July 11, 2002.
 */
#include <stdio.h>
#include <windows.h>


int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
	fprintf(stderr,"Windows System Metric:\t  Value\n");

	fprintf(stderr,"    SM_ARRANGE:\t\t  %d\n", GetSystemMetrics(SM_ARRANGE));
	fprintf(stderr,"    SM_CLEANBOOT:\t  %d\n", GetSystemMetrics(SM_CLEANBOOT));
//	fprintf(stderr,"    SM_CMONITORS:\t  %d\n", GetSystemMetrics(SM_CMONITORS));
	fprintf(stderr,"    SM_CMOUSEBUTTONS:\t  %d\n", GetSystemMetrics(SM_CMOUSEBUTTONS));
	fprintf(stderr,"    SM_CXBORDER:\t  %d\n", GetSystemMetrics(SM_CXBORDER));
	fprintf(stderr,"    SM_CYBORDER:\t  %d\n", GetSystemMetrics(SM_CYBORDER));
	fprintf(stderr,"    SM_CXCURSOR:\t  %d\n", GetSystemMetrics(SM_CXCURSOR));
	fprintf(stderr,"    SM_CYCURSOR:\t  %d\n", GetSystemMetrics(SM_CYCURSOR));
	fprintf(stderr,"    SM_CXDLGFRAME:\t  %d\n", GetSystemMetrics(SM_CXDLGFRAME));
	fprintf(stderr,"    SM_CYDLGFRAME:\t  %d\n", GetSystemMetrics(SM_CYDLGFRAME));
	fprintf(stderr,"    SM_CXDOUBLECLK:\t  %d\n", GetSystemMetrics(SM_CXDOUBLECLK));
	fprintf(stderr,"    SM_CYDOUBLECLK:\t  %d\n", GetSystemMetrics(SM_CYDOUBLECLK));
 	fprintf(stderr,"    SM_CXDRAG:\t\t  %d\n", GetSystemMetrics(SM_CXDRAG));
	fprintf(stderr,"    SM_CYDRAG:\t\t  %d\n", GetSystemMetrics(SM_CYDRAG));
	fprintf(stderr,"    SM_CXEDGE:\t\t  %d\n", GetSystemMetrics(SM_CXEDGE));
	fprintf(stderr,"    SM_CYEDGE:\t\t  %d\n", GetSystemMetrics(SM_CYEDGE));
	fprintf(stderr,"    SM_CXFIXEDFRAME:\t  %d\n", GetSystemMetrics(SM_CXFIXEDFRAME));
	fprintf(stderr,"    SM_CYFIXEDFRAME:\t  %d\n", GetSystemMetrics(SM_CYFIXEDFRAME));
 	fprintf(stderr,"    SM_CXFRAME:\t\t  %d\n", GetSystemMetrics(SM_CXFRAME));
	fprintf(stderr,"    SM_CYFRAME:\t\t  %d\n", GetSystemMetrics(SM_CYFRAME));
	fprintf(stderr,"    SM_CXFULLSCREEN:\t  %d\n", GetSystemMetrics(SM_CXFULLSCREEN));
	fprintf(stderr,"    SM_CYFULLSCREEN:\t  %d\n", GetSystemMetrics(SM_CYFULLSCREEN));
	fprintf(stderr,"    SM_CXHSCROLL:\t  %d\n", GetSystemMetrics(SM_CXHSCROLL));
	fprintf(stderr,"    SM_CYHSCROLL:\t  %d\n", GetSystemMetrics(SM_CYHSCROLL));
	fprintf(stderr,"    SM_CXHTHUMB:\t  %d\n", GetSystemMetrics(SM_CXHTHUMB));
	fprintf(stderr,"    SM_CXICON:\t\t  %d\n", GetSystemMetrics(SM_CXICON));
	fprintf(stderr,"    SM_CYICON:\t\t  %d\n", GetSystemMetrics(SM_CYICON));
	fprintf(stderr,"    SM_CXICONSPACING:\t  %d\n", GetSystemMetrics(SM_CXICONSPACING));
	fprintf(stderr,"    SM_CYICONSPACING:\t  %d\n", GetSystemMetrics(SM_CYICONSPACING));
	fprintf(stderr,"    SM_CXMAXIMIZED:\t  %d\n", GetSystemMetrics(SM_CXMAXIMIZED));
	fprintf(stderr,"    SM_CYMAXIMIZED:\t  %d\n", GetSystemMetrics(SM_CYMAXIMIZED));
	fprintf(stderr,"    SM_CXMAXTRACK:\t  %d\n", GetSystemMetrics(SM_CXMAXTRACK));
	fprintf(stderr,"    SM_CYMAXTRACK:\t  %d\n", GetSystemMetrics(SM_CYMAXTRACK));
	fprintf(stderr,"    SM_CXMENUCHECK:\t  %d\n", GetSystemMetrics(SM_CXMENUCHECK));
	fprintf(stderr,"    SM_CYMENUCHECK:\t  %d\n", GetSystemMetrics(SM_CYMENUCHECK));
	fprintf(stderr,"    SM_CXMENUSIZE:\t  %d\n", GetSystemMetrics(SM_CXMENUSIZE));
	fprintf(stderr,"    SM_CYMENUSIZE:\t  %d\n", GetSystemMetrics(SM_CYMENUSIZE));
	fprintf(stderr,"    SM_CXMIN:\t\t  %d\n", GetSystemMetrics(SM_CXMIN));
	fprintf(stderr,"    SM_CYMIN:\t\t  %d\n", GetSystemMetrics(SM_CYMIN));
	fprintf(stderr,"    SM_CXMINIMIZED:\t  %d\n", GetSystemMetrics(SM_CXMINIMIZED));
	fprintf(stderr,"    SM_CYMINIMIZED:\t  %d\n", GetSystemMetrics(SM_CYMINIMIZED));
	fprintf(stderr,"    SM_CXMINSPACING:\t  %d\n", GetSystemMetrics(SM_CXMINSPACING));
	fprintf(stderr,"    SM_CYMINSPACING:\t  %d\n", GetSystemMetrics(SM_CYMINSPACING));
	fprintf(stderr,"    SM_CXMINTRACK:\t  %d\n", GetSystemMetrics(SM_CXMINTRACK));
	fprintf(stderr,"    SM_CYMINTRACK:\t  %d\n", GetSystemMetrics(SM_CYMINTRACK));
	fprintf(stderr,"    SM_CXSCREEN:\t  %d\n", GetSystemMetrics(SM_CXSCREEN));
	fprintf(stderr,"    SM_CYSCREEN:\t  %d\n", GetSystemMetrics(SM_CYSCREEN));
	fprintf(stderr,"    SM_CXSIZE:\t\t  %d\n", GetSystemMetrics(SM_CXSIZE));
	fprintf(stderr,"    SM_CYSIZE:\t\t  %d\n", GetSystemMetrics(SM_CYSIZE));
	fprintf(stderr,"    SM_CXSIZEFRAME:\t  %d\n", GetSystemMetrics(SM_CXSIZEFRAME));
	fprintf(stderr,"    SM_CYSIZEFRAME:\t  %d\n", GetSystemMetrics(SM_CYSIZEFRAME));

	fprintf(stderr,"    SM_CXSMICON:\t  %d\n", GetSystemMetrics(SM_CXSMICON));
	fprintf(stderr,"    SM_CYSMICON:\t  %d\n", GetSystemMetrics(SM_CYSMICON));
	fprintf(stderr,"    SM_CXSMSIZE:\t  %d\n", GetSystemMetrics(SM_CXSMSIZE));
	fprintf(stderr,"    SM_CYSMSIZE:\t  %d\n", GetSystemMetrics(SM_CYSMSIZE));
//	fprintf(stderr,"    SM_CXVIRTUALSCREEN:\t  %d\n", GetSystemMetrics(SM_CXVIRTUALSCREEN));
//	fprintf(stderr,"    SM_CYVIRTUALSCREEN:\t  %d\n", GetSystemMetrics(SM_CYVIRTUALSCREEN));
	fprintf(stderr,"    SM_CXVSCROLL:\t  %d\n", GetSystemMetrics(SM_CXVSCROLL));
	fprintf(stderr,"    SM_CYVSCROLL:\t  %d\n", GetSystemMetrics(SM_CYVSCROLL));
	fprintf(stderr,"    SM_CYCAPTION:\t  %d\n", GetSystemMetrics(SM_CYCAPTION));
	fprintf(stderr,"    SM_CYKANJIWINDOW:\t  %d\n", GetSystemMetrics(SM_CYKANJIWINDOW));
	fprintf(stderr,"    SM_CYMENU:\t\t  %d\n", GetSystemMetrics(SM_CYMENU));
	fprintf(stderr,"    SM_CYSMCAPTION:\t  %d\n", GetSystemMetrics(SM_CYSMCAPTION));
	fprintf(stderr,"    SM_CYVTHUMB:\t  %d\n", GetSystemMetrics(SM_CYVTHUMB));
	fprintf(stderr,"    SM_DBCSENABLED:\t  %d\n", GetSystemMetrics(SM_DBCSENABLED));
	fprintf(stderr,"    SM_DEBUG:\t\t  %d\n", GetSystemMetrics(SM_DEBUG));
//	fprintf(stderr,"    SM_IMMENABLED:\t  %d\n", GetSystemMetrics(SM_IMMENABLED));

    fprintf(stderr,"    SM_MENUDROPALIGNMENT: %d\n", GetSystemMetrics(SM_MENUDROPALIGNMENT));
	fprintf(stderr,"    SM_MIDEASTENABLED:\t  %d\n", GetSystemMetrics(SM_MIDEASTENABLED));
	fprintf(stderr,"    SM_MOUSEPRESENT:\t  %d\n", GetSystemMetrics(SM_MOUSEPRESENT));
#ifndef _MSC_VER
    fprintf(stderr,"    SM_MOUSEWHEELPRESENT: %d\n", GetSystemMetrics(SM_MOUSEWHEELPRESENT));
#endif
    fprintf(stderr,"    SM_NETWORK:\t\t  %d\n", GetSystemMetrics(SM_NETWORK));
	fprintf(stderr,"    SM_PENWINDOWS:\t  %d\n", GetSystemMetrics(SM_PENWINDOWS));
//	fprintf(stderr,"    SM_REMOTESESSION:\t  %d\n", GetSystemMetrics(SM_REMOTESESSION));
    fprintf(stderr,"    SM_SECURE:\t\t  %d\n", GetSystemMetrics(SM_SECURE));
//	fprintf(stderr,"    SM_SAMEDISPLAYFORMAT:  %d\n", GetSystemMetrics(SM_SAMEDISPLAYFORMAT));
	fprintf(stderr,"    SM_SHOWSOUNDS:\t  %d\n", GetSystemMetrics(SM_SHOWSOUNDS));
	fprintf(stderr,"    SM_SLOWMACHINE:\t  %d\n", GetSystemMetrics(SM_SLOWMACHINE));
	fprintf(stderr,"    SM_SWAPBUTTON:\t  %d\n", GetSystemMetrics(SM_SWAPBUTTON));
//	fprintf(stderr,"    SM_XVIRTUALSCREEN:  %d\n", GetSystemMetrics(SM_XVIRTUALSCREEN));
//	fprintf(stderr,"    SM_YVIRTUALSCREEN:  %d\n", GetSystemMetrics(SM_YVIRTUALSCREEN));
	return 0;
}
