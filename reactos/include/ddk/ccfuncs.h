#ifndef _NTOS_CCFUNCS_H
#define _NTOS_CCFUNCS_H
/* $Id: ccfuncs.h,v 1.4 2000/03/05 19:17:37 ea Exp $ */
VOID
STDCALL
CcMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN	PMDL		MdlChain
	);
#endif
