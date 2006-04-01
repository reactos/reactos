/*****************************************************************************

  Fraginator !!!

*****************************************************************************/


#ifndef FRAGINATOR_H
#define FRAGINATOR_H


#include "unfrag.h"
#include <CommCtrl.h>


int WINAPI  WinMain (HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR CmdLine, int ShowCmd);
Defragment *StartDefragBox (wstring Drive, DefragType Method);


extern HINSTANCE   GlobalHInstance;
extern Defragment *Defrag;


#endif // FRAGINATOR_H