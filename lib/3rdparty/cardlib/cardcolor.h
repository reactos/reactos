
COLORREF ColorScaleRGB(	const COLORREF Col1,
						const COLORREF Col2,
						const double Ratio);

COLORREF ColorScaleHSL(	const COLORREF Col1,
						const COLORREF Col2,
						const double Ratio);


COLORREF ColorDarker(COLORREF col, double ratio);
COLORREF ColorLighter(COLORREF col, double ratio);

COLORREF ScaleLumRGB(COLORREF col1, double ratio);

#define MAKE_PALETTERGB(colref) (0x02000000 | colref)
