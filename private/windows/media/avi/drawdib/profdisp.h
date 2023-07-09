#ifndef PD_CAN_DRAW_DIB
    #define PD_CAN_DRAW_DIB         0x0001      /* if you can draw at all */
    #define PD_STRETCHDIB_1_1_OK    0x0004      /* is it fast? */
    #define PD_STRETCHDIB_1_2_OK    0x0008      /* ... */
    #define PD_STRETCHDIB_1_N_OK    0x0010      /* ... */
#endif

    #define PD_BITBLT_FAST          0x0020      /* try decomp to bitmap? */

LRESULT VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi);

LPVOID FAR TestDibFormats(int dx, int dy, BOOL fForceMe);
