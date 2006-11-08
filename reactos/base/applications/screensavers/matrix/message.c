//
//	message.c
//
//	Dissolve in/out messages into the "matrix"
//
//	
//
#include <windows.h>
#include "globals.h"
#include "message.h"
#include "matrix.h"

//
// this isn't really a random-number generator. It's based on
// a 16bit CRC algorithm. With the right mask (0xb400) it is possible
// to call this function 65536 times and get a unique result every time
// with *NO* repeats. The results look random but they're not - if we
// call this function another 65536 times we get exactly the same results
// in the same order. This is necessary for fading in messages because
// we need to be guaranteed that all cells...it's completely uniform in
// operation but looks random enough to be very effective
//
WORD crc_msgrand(WORD reg)
{
	const WORD mask = 0xb400;

	if(reg & 1)
		reg = (reg >> 1) ^ mask;
	else
		reg = (reg >> 1);

	return reg;
}

//
//	Set a new message based on font and text
//
void SetMatrixMessage(MATRIX_MESSAGE *msg, HFONT hFont, TCHAR *text)
{
	HDC		hdc;
	RECT	rect;
	int		x, y;
	
	HDC		hdcMessage;
	HBITMAP hbmMessage;

	HANDLE	hOldFont, hOldBmp;
	
	//
	// Create a monochrome off-screen buffer
	//
	hdc = GetDC(NULL);

	hdcMessage = CreateCompatibleDC(hdc);
	hbmMessage = CreateBitmap(MAXMSG_WIDTH, MAXMSG_HEIGHT, 1, 1, 0);
	hOldBmp    = SelectObject(hdcMessage, hbmMessage);

	ReleaseDC(NULL, hdc);

	//
	// Draw text into bitmap
	//
	SetRect(&rect, 0, 0, msg->width, MAXMSG_HEIGHT);
	FillRect(hdcMessage, &rect, GetStockObject(WHITE_BRUSH));

	hOldFont = SelectObject(hdcMessage, g_hFont);
	DrawText(hdcMessage, text, -1, &rect, DT_CENTER|DT_VCENTER|DT_WORDBREAK|DT_CALCRECT);

	OffsetRect(&rect, (msg->width-(rect.right-rect.left))/2, (msg->height-(rect.bottom-rect.top))/2);
	DrawText(hdcMessage, text, -1, &rect, DT_CENTER|DT_VCENTER|DT_WORDBREAK);

	//
	// Convert bitmap into an array of cells for easy drawing
	//
	for(y = 0; y < msg->height; y++)
	{
		for(x = 0; x < msg->width; x++)
		{
			msg->message[x][y] = GetPixel(hdcMessage, x, y) ? 0 : 1;
		}
	}

	//
	//	Cleanup
	//
	SelectObject(hdcMessage, hOldFont);
	SelectObject(hdcMessage, hOldBmp);

	DeleteDC(hdcMessage);
	DeleteObject(hbmMessage);
}

//
//	Draw any part of the message that is visible. Make the
//  message "shimmer" by using a random glyph each time
//
void DrawMatrixMessage(MATRIX *matrix, MATRIX_MESSAGE *msg, HDC hdc)
{
	int x, y;

	for(x = 0; x < msg->width; x++)
		for(y = 0; y < msg->height; y++)
			if((msg->message[x][y] & 0x8000) && 
			   (msg->message[x][y] & 0x00FF))
			{
				DrawGlyph(matrix, hdc, x * GLYPH_WIDTH, y * GLYPH_HEIGHT, RandomGlyph(MAX_INTENSITY));
			}
}

//
//	Reveal specified amount of message
//
void RevealMatrixMessage(MATRIX_MESSAGE *msg, int amount)
{
	while(amount--)
	{
		int pos;
		
		msg->random_reg1 = crc_msgrand(msg->random_reg1);
		pos = msg->random_reg1 & 0xffff;

		msg->message[pos / 256][pos % 256] |= GLYPH_REDRAW; 
	}
}

//
//	Reset (hide) the message
//
void ClearMatrixMessage(MATRIX_MESSAGE *msg)
{
	int x, y;

	for(x = 0; x < msg->width; x++)
		for(y = 0; y < msg->height; y++)
			msg->message[x][y] = 0;
}

//
// convert from 50-500 (fast-slow) to slow(50) - fast(500)
//
int MessageSpeed()
{
	return (MSGSPEED_MAX-MSGSPEED_MIN) - (g_nMessageSpeed-MSGSPEED_MIN) + MSGSPEED_MIN;
}

//
//	Called once for each iteration of the matrix
//
void DoMatrixMessage(HDC hdc, MATRIX *matrix)
{
	MATRIX_MESSAGE *msg = matrix->message;

	int RealSpeed = MessageSpeed();

	if(g_nNumMessages > 0)
	{
		// nothing to do yet..
		if(msg->counter++ < 0)
			return;

		// has counter reached limit..clear the message
		if(msg->counter++ == RealSpeed / 2 + (RealSpeed/4))
			ClearMatrixMessage(msg);

		// reset counter + display a new message
		if(msg->counter >= RealSpeed)
		{
			// mark all message-cells as being "invisible" so the
			// message gets cleared by the matrix decoding naturally
		
			if(g_fRandomizeMessages)
				msg->msgindex = crc_rand() % g_nNumMessages;
			else
				msg->msgindex = (msg->msgindex + 1) % g_nNumMessages;

			// make a new message..initially invisible
			SetMatrixMessage(msg, 0, g_szMessages[msg->msgindex]);
			
			msg->counter = -(int)(crc_rand() % MSGSPEED_MAX);
		}
		// reveal the next part of the message
		else if(msg->counter < RealSpeed / 2)
		{
			int w = (g_nMessageSpeed - MSGSPEED_MIN);
			w = (1 << 16) + ((w<<16) / MSGSPEED_MAX);
			w = (w * 3 * g_nMessageSpeed) >> 16;
			
			RevealMatrixMessage(msg, w + 100);
		}
			
		//
		// draw whatever part of the message is visible at this time
		//
		DrawMatrixMessage(matrix, msg, hdc);
	}
}

//
//	Set current font used for messages
//
void SetMessageFont(HWND hwnd, TCHAR *szFontName, int nPointSize, BOOL fBold)
{
	int		lfHeight;
	HDC		hdc;
	HFONT	hFont;

	hdc = GetDC(hwnd);

	lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	ReleaseDC(hwnd, hdc);

	hFont = CreateFont(lfHeight, 0, 0, 0, fBold ? FW_BOLD: FW_NORMAL, 0, 0, 0, 
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY, DEFAULT_PITCH, szFontName);

	if(hFont != 0)
	{
		if(g_hFont != 0)
			DeleteObject(g_hFont);

		g_hFont = hFont;
	}
}

//
//	Create a message!
//
MATRIX_MESSAGE *InitMatrixMessage(HWND hwnd, int width, int height)
{
	MATRIX_MESSAGE *msg;

	if((msg = malloc(sizeof(MATRIX_MESSAGE))) == 0)
		return 0;

	ClearMatrixMessage(msg);

	msg->msgindex = 0;
	msg->width    = min(width, MAXMSG_WIDTH);
	msg->height   = min(height, MAXMSG_HEIGHT);
	msg->counter  = -(int)(crc_rand() % MSGSPEED_MIN + MSGSPEED_MIN);
	
	msg->random_reg1 = (WORD)GetTickCount();

	SetMessageFont(hwnd, g_szFontName, g_nFontSize, g_fFontBold);

	SetMatrixMessage(msg, 0, g_szMessages[0]);

	return msg;
}
