#ifndef VNC_H
#define VNC_H

#define BOOL rfb_BOOL
#include <rfb/rfb.h>
#undef BOOL

typedef unsigned int vncPixel;

typedef struct
{
	uint16_t w, h;
	uint16_t linew;
	rfbPixelFormat *format;
	char *data;
	BOOL owner;
}
vncBuffer;

extern int vncPreparedClientSocket;
extern int vncPreparedServerSocket;

/* - Buffer management */
extern vncBuffer *vncNewBuffer(int w, int h, int depth);
extern vncBuffer *vncDupBuffer(vncBuffer * b);
extern void vncDeleteBuffer(vncBuffer * b);

/* - Colourmaps */
typedef struct
{
	uint8_t r, g, b;
}
vncColour;

extern void vncSetColourMap(rfbScreenInfoPtr s, rfbColourMap * m);
extern rfbColourMap *vncNewColourMap(rfbScreenInfoPtr s, int n);
extern void vncSetColourMapEntry(rfbColourMap * m, int i, vncPixel r, vncPixel g, vncPixel b);
extern void vncDeleteColourMap(rfbColourMap * m);

/* - Simple pixel manipulation */
extern vncPixel vncGetPixel(vncBuffer * b, int x, int y);
extern void vncSetPixel(vncBuffer * b, int x, int y, vncPixel c);

/* - Drawing primitives */
extern void vncSetRect(rfbScreenInfoPtr s, int x, int y, int w, int h, vncPixel c);
extern void vncCopyBlit(rfbScreenInfoPtr s, int x, int y, int w, int h, int srcx, int srcy);
extern void vncCopyBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h,
			    vncBuffer * b, int srcx, int srcy);
extern void vncTransBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h,
			     vncBuffer * b, int srcx, int srcy, int bg);
extern void vncXorBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h,
			   vncBuffer * b, int srcx, int srcy);
extern void vncAndBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h,
			   vncBuffer * b, int srcx, int srcy);
extern vncBuffer *vncGetRect(rfbScreenInfoPtr s, int x, int y, int w, int h);

// - Low level VNC update primitives upon which the rest are based
extern void vncQueueCopyRect(rfbScreenInfoPtr s, int x, int y, int w, int h, int src_x, int src_y);
extern void vncQueueUpdate(rfbScreenInfoPtr s, int x, int y, int w, int h);

/* cursor */
extern rfbCursorPtr vncNewCursor(vncBuffer * mask, vncBuffer * pointer, int hotx, int hoty);
extern void vncSetCursor(rfbScreenInfoPtr s, rfbCursorPtr c);

int vncListenAtTcpAddr(unsigned short port);
void vncPrintStats();

#endif
