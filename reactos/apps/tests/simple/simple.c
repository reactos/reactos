/*
 * The simplest Windows program you will ever write.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Colin Peters <colinp at ma.kcom.ne.jp>, July 1, 2001.
 */
#include <windows.h>

int STDCALL
WinMain (HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
	MessageBox (NULL, "Hello, Windows!", "Hello", MB_OK);
	return 0;
}
