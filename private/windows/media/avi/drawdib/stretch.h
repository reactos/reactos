// in stretch.asm

void FAR PASCAL StretchDIB(
	LPBITMAPINFOHEADER biDst,   //	--> BITMAPINFO of destination
	LPVOID	lpDst,		    //	--> to destination bits
	int	DstX,		    //	Destination origin - x coordinate
	int	DstY,		    //	Destination origin - y coordinate
	int	DstXE,		    //	x extent of the BLT
	int	DstYE,		    //	y extent of the BLT
	LPBITMAPINFOHEADER biSrc,   //	--> BITMAPINFO of source
	LPVOID	lpSrc,		    //	--> to source bits
	int	SrcX,		    //	Source origin - x coordinate
	int	SrcY,		    //	Source origin - y coordinate
	int	SrcXE,		    //	x extent of the BLT
	int	SrcYE); 	    //	y extent of the BLT
