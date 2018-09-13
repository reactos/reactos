/*----------------------------------------------------------------------------
|       DDEDATA structure
|
|       WM_DDE_DATA parameter structure for hData (LOWORD(lParam)).
|       The actual size of this structure depends on the size of
|       the Value array.
|
----------------------------------------------------------------------------*/

typedef struct {
	unsigned short wStatus;
	short	 cfFormat;
	HAND16	 Value;
} DDEDATA16;

typedef struct {
	unsigned short wStatus;
	short	 cfFormat;
	HANDLE	 Value;
} DDEDATA32;
