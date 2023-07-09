#ifndef _INC_IMAGE
#define _INC_IMAGE


// internal image stuff
void FAR PASCAL InitDitherBrush(void);
void FAR PASCAL TerminateDitherBrush(void);

HBITMAP FAR PASCAL CreateMonoBitmap(int cx, int cy);
HBITMAP FAR PASCAL CreateColorBitmap(int cx, int cy);

void WINAPI ImageList_CopyDitherImage(HIMAGELIST pimlDest, WORD iDst,
    int xDst, int yDst, HIMAGELIST pimlSrc, int iSrc, UINT fStyle);

#endif  // _INC_IMAGE
