LONG FAR PASCAL AVICreateMappedStream(PAVISTREAM FAR *ppsMapped,
			       PAVISTREAM ps,
			       int nColors);

LONG FAR PASCAL AVIStreamMakePalette(PAVISTREAM pavi, LONG lSkip,
			      HPALETTE FAR *lphpal,
			      LPBYTE lp16to8,
			      int nColors);

