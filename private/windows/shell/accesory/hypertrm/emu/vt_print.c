/*	File: D:\WACKER\emu\vt_print.c (Created: 23-Dec-1993)
 *
 *	Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:29p $
 */

#include <windows.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_printcmnds
 *
 * DESCRIPTION:
 *	 Processes vt100 printing commands.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt100_printcmnds(void)
	{
//*jcm
#if 0
	int line, from, to;
	TCHAR sel;

	sel = (TCHAR)selector[0];

	DbgOutStr("sel=%u\r\n", (INT)sel, 0, 0, 0, 0);

	switch (sel)
		{
	case 0xF5:				/* auto print on */
		DbgOutStr("print-on\r\n", 0, 0, 0, 0, 0);
		print_echo = TRUE;
		break;

	case 0xF4:				/* auto print off */
		DbgOutStr("print-off\r\n", 0, 0, 0, 0, 0);
		print_echo = FALSE;
		break;

	case 0x00:				/* print screen */
		if (mode_DECPEX == RESET)
			from = top_margin, to = bottom_margin;
		else
			from = 0, to = 23;

		for (line = from; line <= to; ++line)
			print_line(emu_afpText[row_index(line)]);

		if (mode_DECPFF == SET)
			/* print form feed */
			print_line("0x0C");
		break;

	case 0xF1:				/* print cursor line */
		print_line(emu_afpText[row_index(emu_currow)]);   /* maybe */
		break;

	case 0x05:				/* enter printer controller mode */
		DbgOutStr("print-control on\r\n", 0, 0, 0, 0, 0);
		state = 6;			/* this is hard coded for now, bad news */
		break;

	case 0x04:				/* exit printer controller mode */
		break;				/* seen when not in controller mode, ignore */
		}
#endif
	}

void vt100_prnc(void)
	{
#if 0
	static TCHAR storage[40];
	static TCHAR *pntr = storage;
	static int len_s = 0;

	*pntr++ = emu_code;
	*pntr = '\0';
	++len_s;

	if ((cnfg.emu_term == EMU_VT220 && len_s>=3 && strcmp(pntr - 3, TEXT("\2334i")) == 0)
			|| (len_s >= 4 && lstrcmp(pntr - 4, TEXT("\033[4i")) == 0))
		{
		/* received termination string, wrap it up */
		emu_print(storage, len_s - ((*(pntr - 3) == (TCHAR)TEXT('\233')) ?
					 3 : 4));
		pntr = storage;
		len_s = 0;
		state = 0;	/* drop out of this routine */

		// Finish-up print job
		DbgOutStr("print-control off\r\n", 0, 0, 0, 0, 0);
		PrintEchoClose(WUDGE.hHostPrn);
		return;
		}

	/* haven't received termination sequence yet, is storage filled? */
	if (len_s >= sizeof(storage) - 1)
		{
		/* copy most of string to print buffer */
		emu_print(storage, len_s - 4);

		/* move end of string to beginning of storage */
		memmove(storage, &storage[len_s - 4], (unsigned)4);
		pntr = storage + 4;
		len_s = 4;
		}
#endif
	}
