#include <windows.h>
//#include <user32/uitools.h>
#include <user32/debug.h>

WINBOOL STDCALL DrawEdge( HDC hdc, LPRECT rc, UINT edge, UINT flags )
{
    DPRINT("graphics %04x %d,%d-%d,%d %04x %04x\n",
	  hdc, rc->left, rc->top, rc->right, rc->bottom, edge, flags );

    if(flags & BF_DIAGONAL)
      return UITOOLS95_DrawDiagEdge(hdc, rc, edge, flags);
    else
      return UITOOLS95_DrawRectEdge(hdc, rc, edge, flags);
}
