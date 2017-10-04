/*****************************************************************************

  Fraginator !!!

*****************************************************************************/


#ifndef FRAGINATOR_H
#define FRAGINATOR_H


#include "Unfrag.h"
#include <commctrl.h>


//int WINAPI  WinMain (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR CmdLine, int ShowCmd);
Defragment *StartDefragBox (wstring Drive, DefragType Method);


extern HINSTANCE   GlobalHInstance;
extern Defragment *Defrag;

//extern INT PASCAL wWinMain (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPCWSTR CmdLine, INT ShowCmd);

#endif // FRAGINATOR_H
