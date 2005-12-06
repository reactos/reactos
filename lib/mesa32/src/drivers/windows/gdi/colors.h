/* Values for wmesa->pixelformat: */

#define PF_8A8B8G8R	3	/* 32-bit TrueColor:  8-A, 8-B, 8-G, 8-R */
#define PF_8R8G8B	4	/* 32-bit TrueColor:  8-R, 8-G, 8-B */
#define PF_5R6G5B	5	/* 16-bit TrueColor:  5-R, 6-G, 5-B bits */
#define PF_DITHER8	6	/* Dithered RGB using a lookup table */
#define PF_LOOKUP	7	/* Undithered RGB using a lookup table */
#define PF_GRAYSCALE	10	/* Grayscale or StaticGray */
#define PF_BADFORMAT	11
#define PF_INDEX8	12


#define BGR8(r,g,b) (unsigned)(((BYTE)((b & 0xc0) | ((g & 0xe0)>>2) | \
                                      ((r & 0xe0)>>5))))

/* Windows uses 5,5,5 for 16-bit */
#define BGR16(r,g,b) (  (((unsigned short)b       ) >> 3) | \
                        (((unsigned short)g & 0xf8) << 2) | \
                        (((unsigned short)r & 0xf8) << 7) )

#define BGR24(r,g,b) (unsigned long)((DWORD)(((BYTE)(b)| \
                                    ((WORD)((BYTE)(g))<<8))| \
                                    (((DWORD)(BYTE)(r))<<16)))

#define BGR32(r,g,b) (unsigned long)((DWORD)(((BYTE)(b)| \
                                    ((WORD)((BYTE)(g))<<8))| \
                                    (((DWORD)(BYTE)(r))<<16)))


