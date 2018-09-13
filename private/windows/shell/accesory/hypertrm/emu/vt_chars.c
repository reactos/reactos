    /*	File: D:\WACKER\emu\vt_chars.c (Created: 27-Dec-1993)
     *
     *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
     *	All rights reserved
     *
     *	$Revision: 1 $
     *	$Date: 10/05/98 12:28p $
     */
    
    #include <windows.h>
    #pragma hdrstop
    
	#include <tdll\assert.h>
    #include <tdll\stdtyp.h>
    #include <tdll\session.h>
    #include <tdll\print.h>
    #include <tdll\capture.h>
    #include <tdll\mc.h>
    
    #include "emu.h"
    #include "emu.hh"
    #include "emudec.hh"
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
     * vt_charset
     *
     * DESCRIPTION: initialize character set mappings for VT terms
     *
     * ARGUMENTS:
     *
     * RETURNS:
     *
     */
    void vt_charset_init(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	if (hhEmu->mode_vt220 || hhEmu->mode_vt320)
    		/* started as a 320 or 220 even if now 100 */
    		{
    		pstPRI->vt_charset[0] = pstPRI->vt_charset[1] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[2] = pstPRI->vt_charset[3] = ETEXT('<');	  /* DEC supplemental char set */
    		pstPRI->gl = 0;
    		pstPRI->gr = 2;
    		}
    	else if (hhEmu->mode_vt280)
    		/* Kanji or Katakana terminal support */
    		{
    		pstPRI->vt_charset[0] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[1] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[2] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[3] = ETEXT('0');	  /* DEC special graphics set */
    		pstPRI->gl = 0;
    		pstPRI->gr = 2;
    		}
    	else
    		{
    		pstPRI->vt_charset[0] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[1] = ETEXT('0');	  /* DEC special graphics set */
    		pstPRI->gl = 0;
    		pstPRI->gr = 1;
    		}
    	vt_dsptbl(hhEmu, pstPRI->vt_charset[pstPRI->gl], pstPRI->vt_charset[pstPRI->gr]);
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
     * vt_charset
     *
     * DESCRIPTION:
     *
     * ARGUMENTS:
     *
     * RETURNS:
     *
     */
    void vt_charset_save(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
   		MemCopy(pstPRI->vt_sv_charset, pstPRI->vt_charset, sizeof(pstPRI->vt_sv_charset));
    
    	pstPRI->sv_gl = pstPRI->gl;
    	pstPRI->sv_gr = pstPRI->gr;
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
     * vt_charset_restore
     *
     * DESCRIPTION:
     *
     * ARGUMENTS:
     *
     * RETURNS:
     *
     */
    void vt_charset_restore(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
   		MemCopy(pstPRI->vt_charset, pstPRI->vt_sv_charset, sizeof(pstPRI->vt_charset));
    
    	pstPRI->gl = pstPRI->sv_gl;
    	pstPRI->gr = pstPRI->sv_gr;
    
    	vt_dsptbl(hhEmu, pstPRI->vt_charset[pstPRI->gl], pstPRI->vt_charset[pstPRI->gr]);
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
     * vt_scs1
     *
     * DESCRIPTION:
     *	 Sets up the type of character set to be loaded next.
     *
     * ARGUMENTS:
     *	 none
     *
     * RETURNS:
     *	 nothing
     */
    void vt_scs1(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	switch (hhEmu->emu_code)
    		{
    	case ETEXT('(') :
    		pstPRI->gn = 0;
    		break;
    	case ETEXT(')') :
    		pstPRI->gn = 1;
    		break;
    	case ETEXT('*') :
    		pstPRI->gn = 2;
    		break;
    	case ETEXT('+') :
    		pstPRI->gn = 3;
    		break;
    	default:
    		break;
    		}
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
     * vt_scs2
     *
     * DESCRIPTION:
     *	 Loads another character set.
     *
     * ARGUMENTS:
     *	 none
     *
     * RETURNS:
     *	 nothing
     */
    void vt_scs2(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	pstPRI->vt_charset[pstPRI->gn] = hhEmu->emu_code;
    
    	vt_dsptbl(hhEmu,
    				pstPRI->vt_charset[pstPRI->gl],
    				pstPRI->vt_charset[pstPRI->gr]);
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
     * vt_setkanji
     *
     * DESCRIPTION:
     *	 Sets the display mode to Kanji/Katakana
     *
     * ARGUMENTS:
     *	 
     *
     * RETURNS:
     *	 nothing
     */
    void vt_setkanji(const HHEMU hhEmu, const int nSetKanji)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	if (nSetKanji)
    		{
    		pstPRI->vt_charset[0] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[1] = ETEXT('0');	  /* DEC special graphics set */
    		pstPRI->vt_charset[2] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[3] = ETEXT('Y');	  /* Kanji char set */
    		pstPRI->gl = 0;
    		pstPRI->gr = 2;
    		}
    	else
    		{
    		pstPRI->vt_charset[0] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[1] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[2] = ETEXT('B');	  /* ASCII char set */
    		pstPRI->vt_charset[3] = ETEXT('0');	  /* DEC special graphics set */
    		pstPRI->gl = 0;
    		pstPRI->gr = 2;
    		}
    
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
     * vt_charshift
     *
     * DESCRIPTION:
     *	 Loads another character set.
     *
     * ARGUMENTS:
     *	 none
     *
     * RETURNS:
     *	 nothing
     */
    void vt_charshift(const HHEMU hhEmu)
    	{
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	switch(hhEmu->emu_code)
    		{
    	case 0x0F : 			/* LS0 */
    	case ETEXT('G') :		/* vt52 use ASCII */
    		pstPRI->gl = 0;
    		break;
    	case 0x0E : 			/* LS1 */
    	case ETEXT('F') :		/* vt52 use graphics */
    		pstPRI->gl = 1;
    		break;
    	case ETEXT('~') :		/* LS1R */
    		pstPRI->gr = 1;
    		break;
    	case ETEXT('n') :		/* LS2 */
    		pstPRI->gl = 2;
    		break;
    	case ETEXT('}') :		/* LS2R */
    		pstPRI->gr = 2;
    		break;
    	case ETEXT('o') :		/* LS3 */
    		pstPRI->gl = 3;
    		break;
    	case ETEXT('|') :		/* LS3R */
    		pstPRI->gr = 3;
    		break;
    	case 0x8E : 			/* SS2 */
    	case ETEXT('N')	:		/* ESC N */
    		pstPRI->old_gl = pstPRI->gl;
    		pstPRI->gl = 2;
    		hhEmu->emu_datain = vt_char_emulatecmd;
    		break;
    	case 0x8F : 			/* SS3 */
    	case ETEXT('O')	:		/* ESC O */
    		pstPRI->old_gl = pstPRI->gl;
    		pstPRI->gl = 3;
    		hhEmu->emu_datain = vt_char_emulatecmd;
    		break;
    	default:
    		break;
    		}
    
    	vt_dsptbl(hhEmu,
    				pstPRI->vt_charset[pstPRI->gl],
    				pstPRI->vt_charset[pstPRI->gr]);
    	}
    
    /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
     * vt_char_emulatecmd
     *
     * DESCRIPTION:
     *
     * ARGUMENTS:
     *
     * RETURNS:
     *
     */
  #if defined(EXTENDED_FEATURES)
  int vt_char_emulatecmd(const HHEMU hhEmu, const ECHAR ccode)
  	{
  	struct trans_entry *tptr;
  	int ntrans;
  
  #else
    int vt_char_emulatecmd(const HEMU hEmu, const ECHAR ccode)
    	{
    	struct trans_entry *tptr;
    	int ntrans;
    
    	const HHEMU hhEmu = (HHEMU)hEmu;
  #endif
    	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
    
    	emuLock((HEMU)hhEmu);
    
    	hhEmu->emu_code = ccode;
    
    	if (hhEmu->state == 0)
    		{
    		if (IN_RANGE(ccode, ETEXT(' '), 0x7F) || IN_RANGE(ccode, 0xA0, 0xFF))
    			{
    			(*hhEmu->emu_graphic)(hhEmu);
    			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
    						CF_CAP_CHARS, ccode);
    			printEchoChar(hhEmu->hPrintEcho, ccode);
    			goto reset;
    			}
    		if (ccode == ETEXT('\r'))
    			{
    			carriagereturn(hhEmu);
    			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
    						CF_CAP_CHARS, ccode);
    			printEchoChar(hhEmu->hPrintEcho, ccode);
    			return(0);
    			}
    		if (ccode == ETEXT('\n'))
    			{
    			emuLineFeed(hhEmu);
    			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
    						CF_CAP_CHARS, ccode);
    			printEchoChar(hhEmu->hPrintEcho, ccode);
    			return(0);
    			}
    		}
    
    	/* Seek next state by finding character range */
    	tptr = hhEmu->state_tbl[hhEmu->state].first_trans;
    
    	ntrans = hhEmu->state_tbl[hhEmu->state].number_trans;
    
    	for (; ntrans > 0; ntrans--, ++tptr)
    		if (ccode >= tptr->lochar && ccode <= tptr->hichar)
    			break;
    
    	if (ntrans <= 0)
    		{
    		commanderror(hhEmu);
    		CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
    					CF_CAP_CHARS, ccode);
    		printEchoChar(hhEmu->hPrintEcho, ccode);
    		return (0);
    		}
    
    	hhEmu->state = tptr->next_state;
    	(*tptr->funct_ptr)(hhEmu);
    
    	if (hhEmu->state == 0)
    		{
    		hhEmu->num_param_cnt = hhEmu->selector_cnt = hhEmu->selector[0] =
    		hhEmu->num_param[0] = 0;
    
    		hhEmu->DEC_private = FALSE;
    		}
    
    	if (tptr->funct_ptr != hhEmu->emu_graphic)
    		return(0);
    
    	reset:
    #if defined(EXTENDED_FEATURES)	
    	hhEmu->emu_datain = emuStdDataIn;
    #else
    	hhEmu->emu_datain = emuDataIn;
    #endif
    	pstPRI->gl = pstPRI->old_gl;
    
    	// Restore the character set.
    	//
    	vt_dsptbl(hhEmu,
    				pstPRI->vt_charset[pstPRI->gl],
    				pstPRI->vt_charset[pstPRI->gr]);
    
    	emuUnlock((HEMU)hhEmu);
    
    	return (0);
    	}
    
    void vt_dsptbl(const HHEMU hhEmu, ECHAR left, ECHAR right)
    	{
    	// Copy the appropriate characters into the display character table.
    	//
    	std_dsptbl(hhEmu, FALSE);						/* start off from known condition */
    	vt_setdtbl(hhEmu, &hhEmu->dspchar[0], left);	/* install gl chars. */
    	vt_setdtbl(hhEmu, &hhEmu->dspchar[128], right); /* install gr chars. */
    	}
    
    /* ARGSUSED*/
    void vt_setdtbl(const HHEMU hhEmu, ECHAR tbl[], ECHAR cset)
    	{
    	switch (cset)
    		{
    	case ETEXT('A'):	/* British */
    		tbl[0x23]=0x9C; /* replace # with British pound sign */
    		break;
    	case ETEXT('R'):	/* French */
    		tbl[0x23]=0x9C; tbl[0x40]=0x85; tbl[0x5B]=0xF8; tbl[0x5C]=0x87;
    		tbl[0x5D]=0x15; tbl[0x7B]=0x82; tbl[0x7C]=0x97; tbl[0x7D]=0x8A;
    		tbl[0x7E]=0xF9;
    		break;
    	case ETEXT('Q'):	/* French Canadian */
    		tbl[0x40]=0x85; tbl[0x5B]=0x83; tbl[0x5C]=0x87; tbl[0x5D]=0x88;
    		tbl[0x5E]=0x8C; tbl[0x60]=0x93; tbl[0x7B]=0x82; tbl[0x7C]=0x97;
    		tbl[0x7D]=0x8A; tbl[0x7E]=0x96;
    		break;
    	case ETEXT('K'):	/* German */
    		tbl[0x40]=0x15; tbl[0x5B]=0x8E; tbl[0x5C]=0x99; tbl[0x5D]=0x9A;
    		tbl[0x7B]=0x84; tbl[0x7C]=0x94; tbl[0x7D]=0x81; tbl[0x7E]=0xE1;
    		break;
    	case ETEXT('0'):	/* DEC special graphics */
    		tbl[0x5F]=' ';
    		tbl[0x60]=0x04; tbl[0x61]=0xB1; tbl[0x62]='H';	tbl[0x63]='F';
    		tbl[0x64]='R';	tbl[0x65]='L';	tbl[0x66]=0xF8; tbl[0x67]=0xF1;
    		tbl[0x68]='N';	tbl[0x69]='V';	tbl[0x6A]=0xD9; tbl[0x6B]=0xBF;
    		tbl[0x6C]=0xDA; tbl[0x6D]=0xC0; tbl[0x6E]=0xC5; tbl[0x6F]=0xC4;
    		tbl[0x70]=0xC4; tbl[0x71]=0xC4; tbl[0x72]=0xC4; tbl[0x73]=0xC4;
    		tbl[0x74]=0xC3; tbl[0x75]=0xB4; tbl[0x76]=0xC1; tbl[0x77]=0xC2;
    		tbl[0x78]=0xB3; tbl[0x79]=0xF3; tbl[0x7A]=0xF2; tbl[0x7B]=0xE3;
    		tbl[0x7C]=0xF0; tbl[0x7D]=0x9C; tbl[0x7E]=0xFA;
    		break;
    	case ETEXT('<'):	/* DEC supplemental graphics */
    		tbl[0x20]=' ';	tbl[0x21]=0xAD; tbl[0x22]=0x9B; tbl[0x23]=0x9C;
    		tbl[0x24]=' ';	tbl[0x25]=0x9D; tbl[0x26]=' ';	tbl[0x27]=0x15;
    		tbl[0x28]=0x0F; tbl[0x29]='C';	tbl[0x2A]=0xA6; tbl[0x2B]=0xAE;
    		tbl[0x2C]=' ';	tbl[0x2D]=' ';	tbl[0x2E]=' ';	tbl[0x2F]=' ';
    		tbl[0x30]=0xF8; tbl[0x31]=0xF1; tbl[0x32]=0xFD; tbl[0x33]='3';
    		tbl[0x34]=' ';	tbl[0x35]=0xE6; tbl[0x36]=0x14; tbl[0x37]=0xFA;
    		tbl[0x38]=' ';	tbl[0x39]='1';	tbl[0x3A]=0xA7; tbl[0x3B]=0xAF;
    		tbl[0x3C]=0xAC; tbl[0x3D]=0xAB; tbl[0x3E]=' ';	tbl[0x3F]=0xA8;
    		tbl[0x40]='A';	tbl[0x41]='A';	tbl[0x42]='A';	tbl[0x43]='A';
    		tbl[0x44]=0x8E; tbl[0x45]=0x8F; tbl[0x46]=0x92; tbl[0x47]=0x80;
    		tbl[0x48]='E';	tbl[0x49]=0x90; tbl[0x4A]='E';	tbl[0x4B]='E';
    		tbl[0x4C]='I';	tbl[0x4D]='I';	tbl[0x4E]='I';	tbl[0x4F]='I';
    		tbl[0x50]=' ';	tbl[0x51]=0xA5; tbl[0x52]='O';	tbl[0x53]='O';
    		tbl[0x54]='O';	tbl[0x55]='O';	tbl[0x56]=0x99; tbl[0x57]='O';
    		tbl[0x58]=0xED; tbl[0x59]='U';	tbl[0x5A]='U';	tbl[0x5B]='U';
    		tbl[0x5C]=0x9A; tbl[0x5D]='Y';	tbl[0x5E]=' ';	tbl[0x5F]=0xE1;
    		tbl[0x60]=0x85; tbl[0x61]=0xA0; tbl[0x62]=0x83; tbl[0x63]='a';
    		tbl[0x64]=0x84; tbl[0x65]=0x86; tbl[0x66]=0x91; tbl[0x67]=0x87;
    		tbl[0x68]=0x8A; tbl[0x69]=0x82; tbl[0x6A]=0x88; tbl[0x6B]=0x89;
    		tbl[0x6C]=0x8D; tbl[0x6D]=0xA1; tbl[0x6E]=0x8C; tbl[0x6F]=0x8B;
    		tbl[0x70]=' ';	tbl[0x71]=0xA4; tbl[0x72]=0x95; tbl[0x73]=0xA2;
    		tbl[0x74]=0x93; tbl[0x75]='o' ; tbl[0x76]=0x94; tbl[0x77]='o';
    		tbl[0x78]='o';	tbl[0x79]=0x97; tbl[0x7A]=0xA3; tbl[0x7B]=0x96;
    		tbl[0x7C]=0x81; tbl[0x7D]=0x98; tbl[0x7E]=' ';	tbl[0x7F]=' ';
    		break;
    	case ETEXT('B'):	/* normal ASCII */
    	case ETEXT('Y') :	/* KANJI */
    	default :	/* unimplemented */
    		break;	/* leave as is */
    		}
    	}
    
    /* end of vt_chars.c */
