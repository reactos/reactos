/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#include "freeldr.h"
#include "stdlib.h"
#include "tui.h"

int		nScreenWidth = 80;		// Screen Width
int		nScreenHeight = 25;		// Screen Height

char	cStatusBarFgColor = COLOR_BLACK;		// Status bar foreground color
char	cStatusBarBgColor = COLOR_CYAN;			// Status bar background color
char	cBackdropFgColor = COLOR_WHITE;			// Backdrop foreground color
char	cBackdropBgColor = COLOR_BLUE;			// Backdrop background color
char	cBackdropFillStyle = MEDIUM_FILL;		// Backdrop fill style
char	cTitleBoxFgColor = COLOR_WHITE;			// Title box foreground color
char	cTitleBoxBgColor = COLOR_RED;			// Title box background color
char	cMessageBoxFgColor = COLOR_WHITE;		// Message box foreground color
char	cMessageBoxBgColor = COLOR_BLUE;		// Message box background color
char	cMenuFgColor = COLOR_WHITE;				// Menu foreground color
char	cMenuBgColor = COLOR_BLUE;				// Menu background color
char	cTextColor = COLOR_YELLOW;				// Normal text color
char	cSelectedTextColor = COLOR_BLACK;		// Selected text color
char	cSelectedTextBgColor = COLOR_GRAY;		// Selected text background color
char	szTitleBoxTitleText[260] = "Boot Menu";	// Title box's title text

char	szMessageBoxLineText[4000] = "";

void DrawBackdrop(void)
{
	// Fill in the backdrop
	FillArea(0, 0, nScreenWidth-1, nScreenHeight-1, cBackdropFillStyle, ATTR(cBackdropFgColor, cBackdropBgColor));

	// Draw the title box
	DrawBox(1, 1, nScreenWidth, 5, D_VERT, D_HORZ, TRUE, FALSE, ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	// Draw version
	DrawText(3, 2, VERSION, ATTR(cTitleBoxFgColor, cTitleBoxBgColor));
	// Draw copyright
	DrawText(3, 3, "by Brian Palmer", ATTR(cTitleBoxFgColor, cTitleBoxBgColor));
	DrawText(3, 4, "<brianp@sginet.com>", ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	// Draw help text
	//DrawText(nScreenWidth-15, 4, /*"F1 for Help"*/"F8 for Options", ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	// Draw title
	DrawText((nScreenWidth/2)-(strlen(szTitleBoxTitleText)/2), 3, szTitleBoxTitleText, ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	// Draw date
	DrawText(nScreenWidth-9, 2, "01/02/03", ATTR(cTitleBoxFgColor, cTitleBoxBgColor));
	// Draw time
	DrawText(nScreenWidth-9, 3, "10:12:34", ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	// Draw status bar
	DrawStatusText("");

	// Update the date & time
	UpdateDateTime();
}

/*
 * FillArea()
 * This function assumes coordinates are zero-based
 */
void FillArea(int nLeft, int nTop, int nRight, int nBottom, char cFillChar, char cAttr /* Color Attributes */)
{
	char	*screen = (char *)SCREEN_MEM;
	int		i, j;

	for(i=nTop; i<=nBottom; i++)
	{
		for(j=nLeft; j<=nRight; j++)
		{
			screen[((i*2)*nScreenWidth)+(j*2)] = cFillChar;
			screen[((i*2)*nScreenWidth)+(j*2)+1] = cAttr;
		}
	}
}

/*
 * DrawShadow()
 * This function assumes coordinates are zero-based
 */
void DrawShadow(int nLeft, int nTop, int nRight, int nBottom)
{
	char	*screen = (char *)SCREEN_MEM;
	int		i;

	// Shade the bottom of the area
	if(nBottom < (nScreenHeight-1))
	{
		for(i=nLeft+2; i<=nRight; i++)
			screen[(((nBottom+1)*2)*nScreenWidth)+(i*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
	}

	// Shade the right of the area
	if(nRight < (nScreenWidth-1))
	{
		for(i=nTop+1; i<=nBottom; i++)
			screen[((i*2)*nScreenWidth)+((nRight+1)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
	}
	if(nRight+1 < (nScreenWidth-1))
	{
		for(i=nTop+1; i<=nBottom; i++)
			screen[((i*2)*nScreenWidth)+((nRight+2)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
	}

	// Shade the bottom right corner
	if((nRight < (nScreenWidth-1)) && (nBottom < (nScreenHeight-1)))
		screen[(((nBottom+1)*2)*nScreenWidth)+((nRight+1)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
	if((nRight+1 < (nScreenWidth-1)) && (nBottom < (nScreenHeight-1)))
		screen[(((nBottom+1)*2)*nScreenWidth)+((nRight+2)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
}

/*
 * DrawBox()
 * This function assumes coordinates are one-based
 */
void DrawBox(int nLeft, int nTop, int nRight, int nBottom, int nVertStyle, int nHorzStyle, int bFill, int bShadow, char cAttr)
{
	char	cULCorner, cURCorner, cLLCorner, cLRCorner;
	char	cHorz, cVert;

	nLeft--;
	nTop--;
	nRight--;
	nBottom--;

	cHorz = nHorzStyle;
	cVert = nVertStyle;
	if(nHorzStyle == HORZ)
	{
		if(nVertStyle == VERT)
		{
			cULCorner = UL;
			cURCorner = UR;
			cLLCorner = LL;
			cLRCorner = LR;
		}
		else // nVertStyle == D_VERT
		{
			cULCorner = VD_UL;
			cURCorner = VD_UR;
			cLLCorner = VD_LL;
			cLRCorner = VD_LR;
		}
	}
	else // nHorzStyle == D_HORZ
	{
		if(nVertStyle == VERT)
		{
			cULCorner = HD_UL;
			cURCorner = HD_UR;
			cLLCorner = HD_LL;
			cLRCorner = HD_LR;
		}
		else // nVertStyle == D_VERT
		{
			cULCorner = D_UL;
			cURCorner = D_UR;
			cLLCorner = D_LL;
			cLRCorner = D_LR;
		}
	}

	// Fill in box background
	if(bFill)
		FillArea(nLeft, nTop, nRight, nBottom, ' ', cAttr);

	// Fill in corners
	FillArea(nLeft, nTop, nLeft, nTop, cULCorner, cAttr);
	FillArea(nRight, nTop, nRight, nTop, cURCorner, cAttr);
	FillArea(nLeft, nBottom, nLeft, nBottom, cLLCorner, cAttr);
	FillArea(nRight, nBottom, nRight, nBottom, cLRCorner, cAttr);

	// Fill in left line
	FillArea(nLeft, nTop+1, nLeft, nBottom-1, cVert, cAttr);
	// Fill in top line
	FillArea(nLeft+1, nTop, nRight-1, nTop, cHorz, cAttr);
	// Fill in right line
	FillArea(nRight, nTop+1, nRight, nBottom-1, cVert, cAttr);
	// Fill in bottom line
	FillArea(nLeft+1, nBottom, nRight-1, nBottom, cHorz, cAttr);

	if(bShadow)
		DrawShadow(nLeft, nTop, nRight, nBottom);
}

/*
 * DrawText()
 * This function assumes coordinates are one-based
 */
void DrawText(int nX, int nY, char *text, char cAttr)
{
	char	*screen = (char *)SCREEN_MEM;
	int		i, j;

	nX--;
	nY--;

	// Draw the text
	for(i=nX, j=0; text[j]; i++,j++)
	{
		screen[((nY*2)*nScreenWidth)+(i*2)] = text[j];
		screen[((nY*2)*nScreenWidth)+(i*2)+1] = cAttr;
	}
}

void DrawStatusText(char *text)
{
	int	i;

	DrawText(1, nScreenHeight, text, ATTR(cStatusBarFgColor, cStatusBarBgColor));

	for(i=strlen(text)+1; i<=nScreenWidth; i++)
		DrawText(i, nScreenHeight, " ", ATTR(cStatusBarFgColor, cStatusBarBgColor));
}

void UpdateDateTime(void)
{
	char	date[260];
	char	time[260];
	char	temp[20];
	int		hour, minute, second, bPM=FALSE;

	switch(getmonth())
	{
	case 1:
		strcpy(date, "January ");
		break;
	case 2:
		strcpy(date, "February ");
		break;
	case 3:
		strcpy(date, "March ");
		break;
	case 4:
		strcpy(date, "April ");
		break;
	case 5:
		strcpy(date, "May ");
		break;
	case 6:
		strcpy(date, "June ");
		break;
	case 7:
		strcpy(date, "July ");
		break;
	case 8:
		strcpy(date, "August ");
		break;
	case 9:
		strcpy(date, "September ");
		break;
	case 10:
		strcpy(date, "October ");
		break;
	case 11:
		strcpy(date, "November ");
		break;
	case 12:
		strcpy(date, "December ");
		break;
	}
	itoa(getday(), temp, 10);
	if((getday() == 1) || (getday() == 21) || (getday() == 31))
		strcat(temp, "st");
	else if((getday() == 2) || (getday() == 22))
		strcat(temp, "nd");
	else if((getday() == 3) || (getday() == 23))
		strcat(temp, "rd");
	else
		strcat(temp, "th");

	strcat(date, temp);
	strcat(date, " ");
	itoa(getyear(), temp, 10);
	strcat(date, temp);

	// Draw the date
	DrawText(nScreenWidth-strlen(date)-1, 2, date, ATTR(cTitleBoxFgColor, cTitleBoxBgColor));

	hour = gethour();
	if(hour > 12)
	{
		hour -= 12;
		bPM = TRUE;
	}
	if (hour == 0)
		hour = 12;
	minute = getminute();
	second = getsecond();
	itoa(hour, temp, 10);
	strcpy(time, "    ");
	strcat(time, temp);
	strcat(time, ":");
	itoa(minute, temp, 10);
	if(minute < 10)
		strcat(time, "0");
	strcat(time, temp);
	strcat(time, ":");
	itoa(second, temp, 10);
	if(second < 10)
		strcat(time, "0");
	strcat(time, temp);
	if(bPM)
		strcat(time, " PM");
	else
		strcat(time, " AM");

	// Draw the time
	DrawText(nScreenWidth-strlen(time)-1, 3, time, ATTR(cTitleBoxFgColor, cTitleBoxBgColor));
}

void SaveScreen(char *buffer)
{
	char	*screen = (char *)SCREEN_MEM;
	int		i;

	for(i=0; i < (nScreenWidth * nScreenHeight * 2); i++)
		buffer[i] = screen[i];
}

void RestoreScreen(char *buffer)
{
	char	*screen = (char *)SCREEN_MEM;
	int		i;

	for(i=0; i < (nScreenWidth * nScreenHeight * 2); i++)
		screen[i] = buffer[i];
}

void MessageBox(char *text)
{
	int		width = 8;
	int		height = 1;
	int		curline = 0;
	int		i , j, k;
	int		x1, x2, y1, y2;
	char	savebuffer[8000];
	char	temp[260];
	char	key;

	SaveScreen(savebuffer);
	strcat(szMessageBoxLineText, text);

	// Find the height
	for(i=0; i<strlen(szMessageBoxLineText); i++)
	{
		if(szMessageBoxLineText[i] == '\n')
			height++;
	}

	// Find the width
	for(i=0,j=0,k=0; i<height; i++)
	{
		while((szMessageBoxLineText[j] != '\n') && (szMessageBoxLineText[j] != 0))
		{
			j++;
			k++;
		}

		if(k > width)
			width = k;

		k = 0;
		j++;
	}

	// Calculate box area
	x1 = (nScreenWidth - (width+2))/2;
	x2 = x1 + width + 3;
	y1 = ((nScreenHeight - height - 2)/2) + 1;
	y2 = y1 + height + 4;

	// Draw the box
	DrawBox(x1, y1, x2, y2, D_VERT, D_HORZ, TRUE, TRUE, ATTR(cMessageBoxFgColor, cMessageBoxBgColor));

	// Draw the text
	for(i=0,j=0; i<strlen(szMessageBoxLineText)+1; i++)
	{
		if((szMessageBoxLineText[i] == '\n') || (szMessageBoxLineText[i] == 0))
		{
			temp[j] = 0;
			j = 0;
			DrawText(x1+2, y1+1+curline, temp, ATTR(cMessageBoxFgColor, cMessageBoxBgColor));
			curline++;
		}
		else
			temp[j++] = szMessageBoxLineText[i];
	}

	// Draw OK button
	strcpy(temp, "   OK   ");
	DrawText(x1+((x2-x1)/2)-3, y2-2, temp, ATTR(COLOR_BLACK, COLOR_GRAY));

	for(;;)
	{
		if(kbhit())
		{
			key = getch();
			if(key == KEY_EXTENDED)
				key = getch();

			if(key == KEY_ENTER)
				break;
			else if(key == KEY_SPACE)
				break;
		}

		UpdateDateTime();
	}

	RestoreScreen(savebuffer);
	UpdateDateTime();
	strcpy(szMessageBoxLineText, "");
}

void MessageLine(char *text)
{
	strcat(szMessageBoxLineText, text);
	strcat(szMessageBoxLineText, "\n");
}

BOOL IsValidColor(char *color)
{
	if(stricmp(color, "Black") == 0)
		return TRUE;
	else if(stricmp(color, "Blue") == 0)
		return TRUE;
	else if(stricmp(color, "Green") == 0)
		return TRUE;
	else if(stricmp(color, "Cyan") == 0)
		return TRUE;
	else if(stricmp(color, "Red") == 0)
		return TRUE;
	else if(stricmp(color, "Magenta") == 0)
		return TRUE;
	else if(stricmp(color, "Brown") == 0)
		return TRUE;
	else if(stricmp(color, "Gray") == 0)
		return TRUE;
	else if(stricmp(color, "DarkGray") == 0)
		return TRUE;
	else if(stricmp(color, "LightBlue") == 0)
		return TRUE;
	else if(stricmp(color, "LightGreen") == 0)
		return TRUE;
	else if(stricmp(color, "LightCyan") == 0)
		return TRUE;
	else if(stricmp(color, "LightRed") == 0)
		return TRUE;
	else if(stricmp(color, "LightMagenta") == 0)
		return TRUE;
	else if(stricmp(color, "Yellow") == 0)
		return TRUE;
	else if(stricmp(color, "White") == 0)
		return TRUE;

	return FALSE;
}

char TextToColor(char *color)
{
	if(stricmp(color, "Black") == 0)
		return COLOR_BLACK;
	else if(stricmp(color, "Blue") == 0)
		return COLOR_BLUE;
	else if(stricmp(color, "Green") == 0)
		return COLOR_GREEN;
	else if(stricmp(color, "Cyan") == 0)
		return COLOR_CYAN;
	else if(stricmp(color, "Red") == 0)
		return COLOR_RED;
	else if(stricmp(color, "Magenta") == 0)
		return COLOR_MAGENTA;
	else if(stricmp(color, "Brown") == 0)
		return COLOR_BROWN;
	else if(stricmp(color, "Gray") == 0)
		return COLOR_GRAY;
	else if(stricmp(color, "DarkGray") == 0)
		return COLOR_DARKGRAY;
	else if(stricmp(color, "LightBlue") == 0)
		return COLOR_LIGHTBLUE;
	else if(stricmp(color, "LightGreen") == 0)
		return COLOR_LIGHTGREEN;
	else if(stricmp(color, "LightCyan") == 0)
		return COLOR_LIGHTCYAN;
	else if(stricmp(color, "LightRed") == 0)
		return COLOR_LIGHTRED;
	else if(stricmp(color, "LightMagenta") == 0)
		return COLOR_LIGHTMAGENTA;
	else if(stricmp(color, "Yellow") == 0)
		return COLOR_YELLOW;
	else if(stricmp(color, "White") == 0)
		return COLOR_WHITE;

	return COLOR_BLACK;
}

BOOL IsValidFillStyle(char *fill)
{
	if(stricmp(fill, "Light") == 0)
		return TRUE;
	else if(stricmp(fill, "Medium") == 0)
		return TRUE;
	else if(stricmp(fill, "Dark") == 0)
		return TRUE;

	return FALSE;
}

char TextToFillStyle(char *fill)
{
	if(stricmp(fill, "Light") == 0)
		return LIGHT_FILL;
	else if(stricmp(fill, "Medium") == 0)
		return MEDIUM_FILL;
	else if(stricmp(fill, "Dark") == 0)
		return DARK_FILL;

	return LIGHT_FILL;
}

void DrawProgressBar(int nPos)
{
	int		left, top, right, bottom;
	int		width = 50; // Allow for 50 "bars"
	int		height = 2;
	int		i;

	if(nPos > 100)
		nPos = 100;

	left = (nScreenWidth - width - 4) / 2;
	right = left + width + 3;
	top = (nScreenHeight - height - 2) / 2;
	top += 4;
	bottom = top + height + 1;

	// Draw the box
	DrawBox(left, top, right, bottom, VERT, HORZ, TRUE, TRUE, ATTR(cMenuFgColor, cMenuBgColor));

	// Draw the "Loading..." text
	DrawText(70/2, top+1, "Loading...", ATTR(cTextColor, cMenuBgColor));

	// Draw the percent complete
	for(i=0; i<(nPos/2); i++)
		DrawText(left+2+i, top+2, "\xDB", ATTR(cTextColor, cMenuBgColor));

	// Draw the rest
	for(; i<50; i++)
		DrawText(left+2+i, top+2, "\xB2", ATTR(cTextColor, cMenuBgColor));

	UpdateDateTime();
}