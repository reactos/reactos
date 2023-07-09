/*	File: D:\WACKER\emu\vt100.hh (Created: 13-July-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

// Private emulator data for DEC Terminals.
//
typedef struct stPrivateDEC
	{
	int sv_row,
		sv_col,
		sv_state,
		sv_AWM,
		sv_DECOM,
		sv_protectmode,
		fAttrsSaved,
		len_s,
		len_t,
		nState,
		gn,
		old_gl,
		gl,
		gr,
		sv_gr,
		sv_gl,
		fDecColHold,
		*aiLineAttr;

	ECHAR	storage[40],
			vt_charset[4],
			vt_sv_charset[4],
			terminate[4],
			*pntr;

	STATTR sv_attr;

	} DECPRIVATE;

typedef DECPRIVATE *PSTDECPRIVATE;
