#ifndef MESSAGE_INCLUDED
#define MESSAGE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//
// counter starts off negative (rand() < 0)..when it gets
// to zero the message starts to display
//
typedef struct
{
	WORD	message[MAXMSG_WIDTH][MAXMSG_HEIGHT];

	int		msgindex;

	int		counter;
	WORD	random_reg1;

	int		width, height;

} MATRIX_MESSAGE;

void			SetMessageFont(HWND hwnd, TCHAR *szFontName, int nPointSize, BOOL fBold);
MATRIX_MESSAGE *InitMatrixMessage(HWND hwnd, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
