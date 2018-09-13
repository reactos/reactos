#ifndef ENDRES_DEFINED
#define ENDRES_DEFINED

enum endres
{
	endrNormal,
	endrHyphenated,
	endrEndPara,
	endrAltEndPara,
	endrSoftCR,
	endrEndColumn,
	endrEndSection,
	endrEndPage,
	endrEndParaSection,
	endrStopped
};

typedef enum endres ENDRES;

#endif /* ENDRES_DEFINED */

