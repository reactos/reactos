#ifndef LSTXTCFG_DEFINED
#define LSTXTCFG_DEFINED

#include "lsdefs.h"
#include "plstxtcf.h"

typedef struct lstxtcfg
{
	long cEstimatedCharsPerLine;

	WCHAR wchUndef;
	WCHAR wchNull;
	WCHAR wchSpace;
	WCHAR wchHyphen;
	WCHAR wchTab;
	WCHAR wchEndPara1;
	WCHAR wchEndPara2;
	WCHAR wchAltEndPara;
	WCHAR wchEndLineInPara;				/* Word "CCRJ", */
	WCHAR wchColumnBreak;
	WCHAR wchSectionBreak;
	WCHAR wchPageBreak;
	WCHAR wchNonBreakSpace;				/* char code of non-breaking space */
	WCHAR wchNonBreakHyphen;
	WCHAR wchNonReqHyphen;				/* discretionary hyphen */
	WCHAR wchEmDash;
	WCHAR wchEnDash;
	WCHAR wchEmSpace;
	WCHAR wchEnSpace;
	WCHAR wchNarrowSpace;
	WCHAR wchOptBreak;
	WCHAR wchNoBreak;
	WCHAR wchFESpace;
	WCHAR wchJoiner;
	WCHAR wchNonJoiner;
	WCHAR wchToReplace;					/* backslash in FE Word				*/
	WCHAR wchReplace;					/* Yen in FE Word				*/


	WCHAR wchVisiNull;					/* visi char for wch==wchNull		*/
	WCHAR wchVisiAltEndPara;			/* visi char for end "table cell"	*/
	WCHAR wchVisiEndLineInPara;			/* visi char for wchEndLineInPara	*/
	WCHAR wchVisiEndPara;				/* visi char for "end para"			*/
	WCHAR wchVisiSpace;					/* visi char for "space"			*/
	WCHAR wchVisiNonBreakSpace;			/* visi char for wchNonBreakSpace	*/
	WCHAR wchVisiNonBreakHyphen;		/* visi char for wchNonBreakHyphen	*/
	WCHAR wchVisiNonReqHyphen;			/* visi char for wchNonReqHyphen	*/
	WCHAR wchVisiTab;					/* visi char for "tab"				*/
	WCHAR wchVisiEmSpace;				/* visi char for wchEmSpace			*/
	WCHAR wchVisiEnSpace;				/* visi char for wchEnSpace			*/
	WCHAR wchVisiNarrowSpace;			/* visi char for wchNarrowSpace		*/
	WCHAR wchVisiOptBreak;              /* visi char for wchOptBreak		*/
	WCHAR wchVisiNoBreak;				/* visi char for wchNoBreak			*/
	WCHAR wchVisiFESpace;				/* visi char for wchOptBreak		*/

	WCHAR wchEscAnmRun;

	WCHAR wchPad;
} LSTXTCFG;

#endif /* !LSTXTCFG_DEFINED */
