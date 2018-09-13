/***************************************************************************
 *
 * DoesDisplaySuck(void)
 *
 * times the display driver and determines if it "sucks" drawing 8bpp DIBs to
 * the screen.
 *
 * it "sucks" if it can BitBlt() bitmaps *much* faster than it can
 * StretchDIBits() them.
 *
 ***************************************************************************/

extern BOOL FAR PASCAL DoesDisplaySuck(void);
