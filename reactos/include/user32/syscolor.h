
#ifndef __SYSCOLOR_H
#define __SYSCOLOR_H

#define COLOR_3DHIGHLIGHT          COLOR_BTNHIGHLIGHT

void SYSCOLOR_SetColor( int index, COLORREF color );
void SYSCOLOR_Init(void);
HPEN  GetSysColorPen( INT index );

#endif