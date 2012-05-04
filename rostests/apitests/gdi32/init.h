
extern HBITMAP ghbmpDIB32;
extern HDC ghdcDIB32;
extern PULONG pulDIB32Bits;
extern HPALETTE ghpal;
extern struct
{
    LOGPALETTE logpal;
    PALETTEENTRY logpalettedata[8];
} gpal;

BOOL InitStuff(void);

