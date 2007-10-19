/* Unit test suite for comm functions
 *
 * Copyright 2003 Kevin Groeneveld
 * Copyright 2005 Uwe Bonnes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winnls.h"

#define TIMEOUT 1000   /* one second for Timeouts*/
#define SLOWBAUD 150
#define FASTBAUD 115200
#define TIMEDELTA 100  /* 100 ms uncertainty allowed */

/* Define the appropriate LOOPBACK(s) TRUE if you have a Loopback cable with
 * the mentioned shorts connected to your Serial port
 */
#define LOOPBACK_TXD_RXD  FALSE /* Sub-D 9: Short 2-3 */
#define LOOPBACK_CTS_RTS  FALSE /* Sub-D 9: Short 7-8 */
#define LOOPBACK_DTR_DSR  FALSE /* Sub-D 9: Short 4-6 */
#define LOOPBACK_DTR_RING FALSE /* Sub-D 9: Short 4-9 */
#define LOOPBACK_DTR_DCD  FALSE /* Sub-D 9: Short 4-1 */
/* Many Linux serial drivers have the TIOCM_LOOP flag in the TIOCM_SET ioctl
 * available. For the 8250 this is equivalent to TXD->RXD, OUT2->DCD,
 * OUT1->RI, RTS->CTS and DTR->DSR
 */

typedef struct
{
	char string[100];
	BOOL result;
	BOOL old_style;
	DCB dcb1, dcb2;
	COMMTIMEOUTS timeouts1, timeouts2;
} TEST;

static TEST test[] =
{
	{
		"baud=9600 parity=e data=5 stop=1 xon=on odsr=off octs=off dtr=on rts=on idsr=on",
		TRUE, FALSE,
		{ 0x00000000, 0x00002580, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x05, 0x02, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00002580, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x05, 0x02, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"baud=0 parity=M data=6 stop=1.5 xon=off odsr=on octs=ON dtr=off rts=off idsr=OFF",
		TRUE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x06, 0x03, 0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00000000, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x06, 0x03, 0x01, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"BAUD=4000000000 parity=n data=7 stop=2 to=off",
		TRUE, FALSE,
		{ 0x00000000, 0xee6b2800, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x00, 0x02, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xee6b2800, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x00, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 }
	},
	{
		"Baud=115200 Parity=O Data=8 To=On",
		TRUE, FALSE,
		{ 0x00000000, 0x0001c200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x01, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x0001c200, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x01, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000EA60 },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000EA60 }
	},
	{
		"PaRiTy=s           Data=7          DTR=on",
		TRUE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x04, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x04, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"data=4",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"data=9",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"parity=no",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"stop=0",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"stop=1.501",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"stop=3",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"to=foobar",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		" baud=9600",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"baud= 9600",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"baud=9600,data=8",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"11,n,8,1",
		TRUE, TRUE,
		{ 0x00000000, 0x0000006e, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x0000006e, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x00, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"30 ,E, 5,1.5",
		TRUE, TRUE,
		{ 0x00000000, 0x0000012c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x05, 0x02, 0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x0000012c, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x05, 0x02, 0x01, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"60, m, 6, 2 ",
		TRUE, TRUE,
		{ 0x00000000, 0x00000258, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x06, 0x03, 0x02, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00000258, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x06, 0x03, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"12 , o , 7 , 1",
		TRUE, TRUE,
		{ 0x00000000, 0x000004b0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x01, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x000004b0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x01, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"24,s,8,1.5",
		TRUE, TRUE,
		{ 0x00000000, 0x00000960, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x04, 0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00000960, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x04, 0x01, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"48,n,8,1,p",
		TRUE, TRUE,
		{ 0x00000000, 0x000012c0, 0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x000012c0, 1, 1, 1, 1, 2, 1, 1, 0, 0, 1, 1, 2, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x00, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,8,1 , x ",
		TRUE, TRUE,
		{ 0x00000000, 0x00002580, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00002580, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x00, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"19, e, 7, 1, x",
		TRUE, TRUE,
		{ 0x00000000, 0x00004b00, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x02, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00004b00, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x02, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"0,M,7,1,P",
		TRUE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x03, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00000000, 1, 1, 1, 1, 2, 1, 1, 0, 0, 1, 1, 2, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x03, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"4000000000,O,7,1.5,X",
		TRUE, TRUE,
		{ 0x00000000, 0xee6b2800, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x01, 0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xee6b2800, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x01, 0x01, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,8,1 to=on",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,NO,8,1",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,4,1",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,9,1",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,8,0",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,8,3",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"96,N,8,1,K",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COM0:baud=115200",
		FALSE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx:baud=38400 data=8",
		TRUE, FALSE,
		{ 0x00000000, 0x00009600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00009600, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0xff, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx  :to=on stop=1.5",
		TRUE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x01, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0x01, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000EA60 },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000EA60 }
	},
	{
		"COMx:               baud=12345     data=7",
		TRUE, FALSE,
		{ 0x00000000, 0x00003039, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00003039, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0xff, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx : xon=on odsr=off",
		TRUE, FALSE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 0, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COM0:9600,N,8,1",
		FALSE, TRUE,
		{ 0x00000000, 0x00000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x00, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0xffffffff, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx:9600,N,8,1",
		TRUE, TRUE,
		{ 0x00000000, 0x00002580, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x08, 0x00, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00002580, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x08, 0x00, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx:  11,E,7,2",
		TRUE, TRUE,
		{ 0x00000000, 0x0000006e, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x07, 0x02, 0x02, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x0000006e, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x07, 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx  :19,M,5,1",
		TRUE, TRUE,
		{ 0x00000000, 0x00004b00, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x05, 0x03, 0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x00004b00, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x05, 0x03, 0x00, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
	{
		"COMx  :    57600,S,6,2,x",
		TRUE, TRUE,
		{ 0x00000000, 0x0000e100, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0x00000, 0x0000, 0x0000, 0x0000, 0x06, 0x04, 0x02, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, 0x0000 },
		{ 0xffffffff, 0x0000e100, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x1ffff, 0xffff, 0xffff, 0xffff, 0x06, 0x04, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0xff, 0xffff },
		{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
		{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
	},
};

#define TEST_COUNT (sizeof(test) / sizeof(TEST))

/* This function can be useful if you are modifiying the test cases and want to
   output the contents of a DCB structure. */
/*static print_dcb(DCB *pdcb)
{
	printf("0x%08x, 0x%08x, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, 0x%05x, 0x%04x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, (char)0x%02x, (char)0x%02x, (char)0x%02x, (char)0x%02x, (char)0x%02x, 0x%04x\n",
		pdcb->DCBlength,
		pdcb->BaudRate,
		pdcb->fBinary,
		pdcb->fParity,
		pdcb->fOutxCtsFlow,
		pdcb->fOutxDsrFlow,
		pdcb->fDtrControl,
		pdcb->fDsrSensitivity,
		pdcb->fTXContinueOnXoff,
		pdcb->fOutX,
		pdcb->fInX,
		pdcb->fErrorChar,
		pdcb->fNull,
		pdcb->fRtsControl,
		pdcb->fAbortOnError,
		pdcb->fDummy2,
		pdcb->wReserved,
		pdcb->XonLim,
		pdcb->XoffLim,
		pdcb->ByteSize,
		pdcb->Parity,
		pdcb->StopBits,
		pdcb->XonChar & 0xff,
		pdcb->XoffChar & 0xff,
		pdcb->ErrorChar & 0xff,
		pdcb->EofChar & 0xff,
		pdcb->EvtChar & 0xff,
		pdcb->wReserved1 & 0xffff );
} */

static void check_result(const char *function, TEST *ptest, int initial_value, BOOL result)
{
	DWORD LastError = GetLastError();
	DWORD CorrectError = (ptest->result ? 0xdeadbeef : ERROR_INVALID_PARAMETER);

	ok(LastError == CorrectError, "%s(\"%s\"), 0x%02x: GetLastError() returned 0x%08lx, should be 0x%08lx\n", function, ptest->string, initial_value, LastError, CorrectError);
	ok(result == ptest->result, "%s(\"%s\"), 0x%02x: return value should be %s\n", function, ptest->string, initial_value, ptest->result ? "TRUE" : "FALSE");
}

#define check_dcb_member(a,b) ok(pdcb1->a == pdcb2->a, "%s(\"%s\"), 0x%02x: "#a" is "b", should be "b"\n", function, ptest->string, initial_value, pdcb1->a, pdcb2->a)
#define check_dcb_member2(a,c,b) if(pdcb2->a == c) { check_dcb_member(a,b); } else { ok(pdcb1->a == pdcb2->a || pdcb1->a == c, "%s(\"%s\"), 0x%02x: "#a" is "b", should be "b" or "b"\n", function, ptest->string, initial_value, pdcb1->a, pdcb2->a, c); }

static void check_dcb(const char *function, TEST *ptest, int initial_value, DCB *pdcb1, DCB *pdcb2)
{
	/* DCBlength is a special case since Win 9x sets it but NT does not.
	   We will accept either as correct. */
	check_dcb_member2(DCBlength, (DWORD)sizeof(DCB), "%lu");

	/* For old style control strings Win 9x does not set the next five members, NT does. */
	if(ptest->old_style && ptest->result)
	{
		check_dcb_member2(fOutxCtsFlow, ((unsigned int)initial_value & 1), "%u");
		check_dcb_member2(fDtrControl, ((unsigned int)initial_value & 3), "%u");
		check_dcb_member2(fOutX, ((unsigned int)initial_value & 1), "%u");
		check_dcb_member2(fInX, ((unsigned)initial_value & 1), "%u");
		check_dcb_member2(fRtsControl, ((unsigned)initial_value & 3), "%u");
	}
	else
	{
		check_dcb_member(fOutxCtsFlow, "%u");
		check_dcb_member(fDtrControl, "%u");
		check_dcb_member(fOutX, "%u");
		check_dcb_member(fInX, "%u");
		check_dcb_member(fRtsControl, "%u");
	}

	if(ptest->result)
	{
		/* For the idsr=xxx parameter, NT sets fDsrSensitivity, 9x sets
		   fOutxDsrFlow. */
		if(!ptest->old_style)
		{
			check_dcb_member2(fOutxDsrFlow, pdcb2->fDsrSensitivity, "%u");
			check_dcb_member2(fDsrSensitivity, pdcb2->fOutxDsrFlow, "%u");
		}
		else
		{
			/* For old style control strings Win 9x does not set the
			   fOutxDsrFlow member, NT does. */
			check_dcb_member2(fOutxDsrFlow, ((unsigned int)initial_value & 1), "%u");
			check_dcb_member(fDsrSensitivity, "%u");
		}
	}
	else
	{
		check_dcb_member(fOutxDsrFlow, "%u");
		check_dcb_member(fDsrSensitivity, "%u");
	}

	/* Check the result of the DCB members. */
	check_dcb_member(BaudRate, "%lu");
	check_dcb_member(fBinary, "%u");
	check_dcb_member(fParity, "%u");
	check_dcb_member(fTXContinueOnXoff, "%u");
	check_dcb_member(fErrorChar, "%u");
	check_dcb_member(fNull, "%u");
	check_dcb_member(fAbortOnError, "%u");
	check_dcb_member(fDummy2, "%u");
	check_dcb_member(wReserved, "%u");
	check_dcb_member(XonLim, "%u");
	check_dcb_member(XoffLim, "%u");
	check_dcb_member(ByteSize, "%u");
	check_dcb_member(Parity, "%u");
	check_dcb_member(StopBits, "%u");
	check_dcb_member(XonChar, "%d");
	check_dcb_member(XoffChar, "%d");
	check_dcb_member(ErrorChar, "%d");
	check_dcb_member(EofChar, "%d");
	check_dcb_member(EvtChar, "%d");
	check_dcb_member(wReserved1, "%u");
}

#define check_timeouts_member(a) ok(ptimeouts1->a == ptimeouts2->a, "%s(\"%s\"), 0x%02x: "#a" is %lu, should be %lu\n", function, ptest->string, initial_value, ptimeouts1->a, ptimeouts2->a);

static void check_timeouts(const char *function, TEST *ptest, int initial_value, COMMTIMEOUTS *ptimeouts1, COMMTIMEOUTS *ptimeouts2)
{
	check_timeouts_member(ReadIntervalTimeout);
	check_timeouts_member(ReadTotalTimeoutMultiplier);
	check_timeouts_member(ReadTotalTimeoutConstant);
	check_timeouts_member(WriteTotalTimeoutMultiplier);
	check_timeouts_member(WriteTotalTimeoutConstant);
}

static void test_BuildCommDCBA(TEST *ptest, int initial_value, DCB *pexpected_dcb)
{
	BOOL result;
	DCB dcb;

	/* set initial conditions */
	memset(&dcb, initial_value, sizeof(DCB));
	SetLastError(0xdeadbeef);

	result = BuildCommDCBA(ptest->string, &dcb);

	/* check results */
	check_result("BuildCommDCBA", ptest, initial_value, result);
	check_dcb("BuildCommDCBA", ptest, initial_value, &dcb, pexpected_dcb);
}

static void test_BuildCommDCBAndTimeoutsA(TEST *ptest, int initial_value, DCB *pexpected_dcb, COMMTIMEOUTS *pexpected_timeouts)
{
	BOOL result;
	DCB dcb;
	COMMTIMEOUTS timeouts;

	/* set initial conditions */
	memset(&dcb, initial_value, sizeof(DCB));
	memset(&timeouts, initial_value, sizeof(COMMTIMEOUTS));
	SetLastError(0xdeadbeef);

	result = BuildCommDCBAndTimeoutsA(ptest->string, &dcb, &timeouts);

	/* check results */
	check_result("BuildCommDCBAndTimeoutsA", ptest, initial_value, result);
	check_dcb("BuildCommDCBAndTimeoutsA", ptest, initial_value, &dcb, pexpected_dcb);
	check_timeouts("BuildCommDCBAndTimeoutsA", ptest, initial_value, &timeouts, pexpected_timeouts);
}

static void test_BuildCommDCBW(TEST *ptest, int initial_value, DCB *pexpected_dcb)
{
	BOOL result;
	DCB dcb;
	WCHAR wide_string[sizeof(ptest->string)];

	MultiByteToWideChar(CP_ACP, 0, ptest->string, -1, wide_string, sizeof(wide_string) / sizeof(WCHAR));

	/* set initial conditions */
	memset(&dcb, initial_value, sizeof(DCB));
	SetLastError(0xdeadbeef);

	result = BuildCommDCBW(wide_string, &dcb);

	if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
		return;

	/* check results */
	check_result("BuildCommDCBW", ptest, initial_value, result);
	check_dcb("BuildCommDCBW", ptest, initial_value, &dcb, pexpected_dcb);
}

static void test_BuildCommDCBAndTimeoutsW(TEST *ptest, int initial_value, DCB *pexpected_dcb, COMMTIMEOUTS *pexpected_timeouts)
{
	BOOL result;
	DCB dcb;
	COMMTIMEOUTS timeouts;
	WCHAR wide_string[sizeof(ptest->string)];

	MultiByteToWideChar(CP_ACP, 0, ptest->string, -1, wide_string, sizeof(wide_string) / sizeof(WCHAR));

	/* set initial conditions */
	memset(&dcb, initial_value, sizeof(DCB));
	memset(&timeouts, initial_value, sizeof(COMMTIMEOUTS));
	SetLastError(0xdeadbeef);

	result = BuildCommDCBAndTimeoutsW(wide_string, &dcb, &timeouts);

	if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
		return;

	/* check results */
	check_result("BuildCommDCBAndTimeoutsA", ptest, initial_value, result);
	check_dcb("BuildCommDCBAndTimeoutsA", ptest, initial_value, &dcb, pexpected_dcb);
	check_timeouts("BuildCommDCBAndTimeoutsA", ptest, initial_value, &timeouts, pexpected_timeouts);
}

static void test_BuildCommDCB(void)
{
	char port_name[] = "COMx";
	char port = 0;
	unsigned int i;
	char *ptr;

	/* Some of these tests require a valid COM port.  This loop will try to find
	   a valid port. */
	for(port_name[3] = '1'; port_name[3] <= '9'; port_name[3]++)
	{
		COMMCONFIG commconfig;
		DWORD size = sizeof(COMMCONFIG);

		if(GetDefaultCommConfig(port_name, &commconfig, &size))
		{
			port = port_name[3];
			break;
		}
	}

	if(!port)
		trace("Could not find a valid COM port.  Some tests will be skipped.\n");

	for(i = 0; i < TEST_COUNT; i++)
	{
		/* Check if this test case needs a valid COM port. */
		ptr = strstr(test[i].string, "COMx");

		/* If required, substitute valid port number into device control string. */
		if(ptr)
		{
			if(port)
				ptr[3] = port;
			else
				continue;
		}

		test_BuildCommDCBA(&test[i], 0x00, &test[i].dcb1);
		test_BuildCommDCBA(&test[i], 0xff, &test[i].dcb2);
		test_BuildCommDCBAndTimeoutsA(&test[i], 0x00, &test[i].dcb1, &test[i].timeouts1);
		test_BuildCommDCBAndTimeoutsA(&test[i], 0xff, &test[i].dcb2, &test[i].timeouts2);

		test_BuildCommDCBW(&test[i], 0x00, &test[i].dcb1);
		test_BuildCommDCBW(&test[i], 0xff, &test[i].dcb2);
		test_BuildCommDCBAndTimeoutsW(&test[i], 0x00, &test[i].dcb1, &test[i].timeouts1);
		test_BuildCommDCBAndTimeoutsW(&test[i], 0xff, &test[i].dcb2, &test[i].timeouts2);
	}
}

static HANDLE test_OpenComm(BOOL doOverlap)
{
    HANDLE hcom = INVALID_HANDLE_VALUE;
    char port_name[] = "COMx";
    static BOOL shown = FALSE;
    DWORD errors;
    COMSTAT comstat;

    /* Try to find a port */
    for(port_name[3] = '1'; port_name[3] <= '9'; port_name[3]++)
    {
	hcom = CreateFile( port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			   (doOverlap)?FILE_FLAG_OVERLAPPED:0, NULL );
	if (hcom != INVALID_HANDLE_VALUE)
	    break;
    }
    if(!shown)
    {
	if (hcom == INVALID_HANDLE_VALUE)
	    trace("Could not find a valid COM port.  Skipping test_ReadTimeOut\n");
	else
	    trace("Found Com port %s. Connected devices may disturbe results\n", port_name);
	/*shown = TRUE; */
    }
    if (hcom != INVALID_HANDLE_VALUE)
    {
        ok(ClearCommError(hcom,&errors,&comstat), "Unexpected errors on open\n");
        ok(comstat.cbInQue == 0, "Unexpected %ld chars in InQueue\n",comstat.cbInQue);
        ok(comstat.cbOutQue == 0, "Still pending %ld charcters in OutQueue\n", comstat.cbOutQue);
        ok(errors == 0, "Unexpected errors 0x%08lx\n", errors);
    }
    return hcom;
}

static void test_GetModemStatus(HANDLE hcom)
{
    DWORD ModemStat;

    ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
    trace("GetCommModemStatus returned 0x%08lx->%s%s%s%s\n", ModemStat,
	  (ModemStat &MS_RLSD_ON)?"MS_RLSD_ON ":"",
	  (ModemStat &MS_RING_ON)?"MS_RING_ON ":"",
	  (ModemStat &MS_DSR_ON)?"MS_DSR_ON ":"",
	  (ModemStat &MS_CTS_ON)?"MS_CTS_ON ":"");
}

/* When we don't write anything, Read should time out even on a loopbacked port */
static void test_ReadTimeOut(HANDLE hcom)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    char rbuf[32];
    DWORD before, after, read, timediff, LastError;
    BOOL res;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.BaudRate = FASTBAUD;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.fRtsControl=RTS_CONTROL_ENABLE;
    dcb.fDtrControl=DTR_CONTROL_ENABLE;
    dcb.StopBits = ONESTOPBIT;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");

    ZeroMemory( &timeouts, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant = TIMEOUT;
    ok(SetCommTimeouts(hcom, &timeouts),"SetCommTimeouts failed\n");

    before = GetTickCount();
    SetLastError(0xdeadbeef);
    res = ReadFile(hcom, rbuf, sizeof(rbuf), &read, NULL);
    LastError = GetLastError();
    after = GetTickCount();
    ok( res == TRUE, "A timed-out read should return TRUE\n");
    ok( LastError == 0xdeadbeef, "err=%ld\n", LastError);
    timediff = after - before;
    ok( timediff > TIMEOUT>>2 && timediff < TIMEOUT *2,
	"Unexpected TimeOut %ld, expected %d\n", timediff, TIMEOUT);
}

static void test_waittxempty(HANDLE hcom)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    char tbuf[]="test_waittxempty";
    DWORD before, after, written, timediff, evtmask = 0;
    BOOL res_write, res;
    DWORD baud = SLOWBAUD;

    trace("test_waittxempty\n");
    /* set a low baud rate to have ample time*/
    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.fRtsControl=RTS_CONTROL_ENABLE;
    dcb.fDtrControl=DTR_CONTROL_ENABLE;
    dcb.StopBits = ONESTOPBIT;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");

    ZeroMemory( &timeouts, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant = TIMEOUT;
    ok(SetCommTimeouts(hcom, &timeouts),"SetCommTimeouts failed\n");

    ok(SetupComm(hcom,1024,1024),"SetUpComm failed\n");
    ok(SetCommMask(hcom, EV_TXEMPTY), "SetCommMask failed\n");

    before = GetTickCount();
    res_write=WriteFile(hcom, tbuf, sizeof(tbuf), &written, NULL);
    after = GetTickCount();
    ok(res_write == TRUE, "WriteFile failed\n");
    ok(written == sizeof(tbuf),
       "WriteFile: Unexpected write_size %ld , expected %d\n", written, sizeof(tbuf));

    trace("WriteFile succeeded, took %ld ms to write %d Bytes at %ld Baud\n",
	  after - before, sizeof(tbuf), baud);

    before = GetTickCount();
    res = WaitCommEvent(hcom, &evtmask, NULL);
    after = GetTickCount();

    ok(res == TRUE, "WaitCommEvent failed\n");
    ok((evtmask & EV_TXEMPTY),
		 "WaitCommEvent: Unexpected EvtMask 0x%08lx, expected 0x%08x\n",
		 evtmask, EV_TXEMPTY);

    timediff = after - before;

    trace("WaitCommEvent for EV_TXEMPTY took %ld ms\n", timediff);
    /* 050604: This shows a difference between XP (tested with mingw compiled crosstest):
       XP returns Writefile only after everything went out of the Serial port,
       while wine returns immedate.
       Thus on XP, WaintCommEvent after setting the CommMask for EV_TXEMPTY
       nearly return immediate,
       while on wine the most time is spent here
    */
}

/* A new open handle should not return error or have bytes in the Queues */
static void test_ClearCommErrors(HANDLE hcom)
{
    DWORD   errors;
    COMSTAT lpStat;

    ok(ClearCommError(hcom, &errors, &lpStat), "ClearCommError failed\n");
    ok(lpStat.cbInQue == 0, "Unexpected %ld chars in InQueue\n", lpStat.cbInQue);
    ok(lpStat.cbOutQue == 0, "Unexpected %ld chars in OutQueue\n", lpStat.cbOutQue);
    ok(errors == 0, "ClearCommErrors: Unexpected error 0x%08lx\n", errors);
    trace("test_ClearCommErrors done\n");
}

static void test_non_pending_errors(HANDLE hcom)
{
    DCB dcb;
    DWORD err;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.ByteSize = 255; /* likely bogus */
    ok(!SetCommState(hcom, &dcb), "SetCommState should have failed\n");
    ok(ClearCommError(hcom, &err, NULL), "ClearCommError should succeed\n");
    ok(!(err & CE_MODE), "ClearCommError shouldn't set CE_MODE byte in this case (%lx)\n", err);
}

/**/
static void test_LoopbackRead(HANDLE hcom)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    char rbuf[32];
    DWORD before, after, diff, read, read1, written, evtmask=0, i;
    BOOL res;
    char tbuf[]="test_LoopbackRead";

    trace("Starting test_LoopbackRead\n");
    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.BaudRate = FASTBAUD;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.fRtsControl=RTS_CONTROL_ENABLE;
    dcb.fDtrControl=DTR_CONTROL_ENABLE;
    dcb.StopBits = ONESTOPBIT;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");

    ZeroMemory( &timeouts, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant = TIMEOUT;
    ok(SetCommTimeouts(hcom, &timeouts),"SetCommTimeouts failed\n");

    ok(SetCommMask(hcom, EV_TXEMPTY), "SetCommMask failed\n");

    before = GetTickCount();
    ok(WriteFile(hcom,tbuf,sizeof(tbuf),&written, NULL), "WriteFile failed\n");
    after = GetTickCount();
    ok(written == sizeof(tbuf),"WriteFile %ld bytes written, expected %d\n",
       written, sizeof(tbuf));
    diff = after -before;

    /* make sure all bytes are written, so Readfile will succeed in one call*/
    ok(WaitCommEvent(hcom, &evtmask, NULL), "WaitCommEvent failed\n");
    before = GetTickCount();
    ok(evtmask == EV_TXEMPTY,
		 "WaitCommEvent: Unexpected EvtMask 0x%08lx, expected 0x%08x\n",
		 evtmask, EV_TXEMPTY);
    trace("Write %ld ms WaitCommEvent EV_TXEMPTY %ld ms\n", diff, before- after);

    read=0;
    ok(ReadFile(hcom, rbuf, sizeof(rbuf), &read, NULL), "Readfile failed\n");
    ok(read == sizeof(tbuf),"ReadFile read %ld bytes, expected %d \"%s\"\n", read, sizeof(tbuf),rbuf);

    /* Now do the same withe a slower Baud rate.
       As we request more characters then written, we will hit the timeout
    */

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.fRtsControl=RTS_CONTROL_ENABLE;
    dcb.fDtrControl=DTR_CONTROL_ENABLE;
    dcb.StopBits = ONESTOPBIT;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");

    ok(SetCommMask(hcom, EV_RXCHAR), "SetCommMask failed\n");
    ok(WriteFile(hcom,tbuf,sizeof(tbuf),&written, NULL), "WriteFile failed\n");
    ok(written == sizeof(tbuf),"WriteFile %ld bytes written, expected %d\n",
       written, sizeof(tbuf));

    trace("WaitCommEventEV_RXCHAR\n");
    ok(WaitCommEvent(hcom, &evtmask, NULL), "WaitCommEvent failed\n");
    ok(evtmask == EV_RXCHAR, "WaitCommEvent: Unexpected EvtMask 0x%08lx, expected 0x%08x\n",
       evtmask, EV_RXCHAR);

    before = GetTickCount();
    res = ReadFile(hcom, rbuf, sizeof(rbuf), &read, NULL);
    after = GetTickCount();
    ok(res, "Readfile failed\n");
    ok(read == sizeof(tbuf),"ReadFile read %ld bytes, expected %d\n", read, sizeof(tbuf));
    diff = after - before;
    trace("Readfile for %d chars with %d avail took %ld ms\n",
	  sizeof(rbuf), sizeof(tbuf), diff);
    ok( (diff > TIMEOUT - TIMEDELTA) && (diff < TIMEOUT + TIMEDELTA),
	"Timedout Wait took %ld ms, expected around %d\n", diff, TIMEOUT);

    /* now do a plain read with slow speed
     * This will result in several low level reads and a timeout to happen
     */
    dcb.BaudRate = SLOWBAUD;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");
    ok(WriteFile(hcom,tbuf,sizeof(tbuf),&written, NULL), "WriteFile failed\n");
    before = GetTickCount();
    read = 0;
    read1 =0;
    i=0;
    do
    {
	res = ReadFile(hcom, rbuf+read, sizeof(rbuf-read), &read1, NULL);
	ok(res, "Readfile failed\n");
	read += read1;
	i++;
    }
    while ((read < sizeof(tbuf)) && (i <10));
    after =  GetTickCount();
    ok( read == sizeof(tbuf),"ReadFile read %ld bytes, expected %d\n", read, sizeof(tbuf));
    trace("Plain Read for %d char at %d baud took %ld ms\n", sizeof(tbuf), SLOWBAUD, after-before);

}

static void test_LoopbackCtsRts(HANDLE hcom)
{
    DWORD ModemStat, defaultStat;
    DCB dcb;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
    {
	trace("RTS_CONTROL_HANDSHAKE is set, so don't manipulate RTS\n");
	return;
    }
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    /* XP returns some values in the low nibble, so mask them out*/
    defaultStat &= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if(defaultStat & MS_CTS_ON)
    {
	ok(EscapeCommFunction(hcom, CLRRTS), "EscapeCommFunction failed to clear RTS\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_CTS_ON) == 0, "CTS didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat & ~MS_CTS_ON));
	ok(EscapeCommFunction(hcom, SETRTS), "EscapeCommFunction failed to clear RTS\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore CTS: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
    else
    {
	ok(EscapeCommFunction(hcom, SETRTS), "EscapeCommFunction failed to set RTS\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_CTS_ON) == MS_CTS_ON,
	    "CTS didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat | MS_CTS_ON));
	ok(EscapeCommFunction(hcom, CLRRTS), "EscapeCommFunction failed to clear RTS\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore CTS: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
}

static void test_LoopbackDtrDcd(HANDLE hcom)
{
    DWORD ModemStat, defaultStat;
    DCB dcb;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    /* XP returns some values in the low nibble, so mask them out*/
    defaultStat &= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if(defaultStat & MS_RLSD_ON)
    {
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_RLSD_ON) == 0, "RLSD didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat & ~MS_RLSD_ON));
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to set DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore RLSD: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
    else
    {
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to set DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_RLSD_ON) == MS_RLSD_ON,
	    "RLSD didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat | MS_RLSD_ON));
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore RLSD: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
}

static void test_LoopbackDtrDsr(HANDLE hcom)
{
    DWORD ModemStat, defaultStat;
    DCB dcb;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_DISABLE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    /* XP returns some values in the low nibble, so mask them out*/
    defaultStat &= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if(defaultStat & MS_DSR_ON)
    {
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_DSR_ON) == 0, "CTS didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat & ~MS_DSR_ON));
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore DSR: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
    else
    {
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to set DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_DSR_ON) == MS_DSR_ON,
	    "CTS didn't react: 0x%04lx,expected 0x%04lx\n",
	    ModemStat, (defaultStat | MS_DSR_ON));
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore DSR: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
}

static void test_LoopbackDtrRing(HANDLE hcom)
{
    DWORD ModemStat, defaultStat;
    DCB dcb;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    /* XP returns some values in the low nibble, so mask them out*/
    defaultStat &= MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON;
    if(defaultStat & MS_RING_ON)
    {
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_RING_ON) == 0, "RING didn't react: 0x%04lx,  expected 0x%04lx\n",
	    ModemStat, (defaultStat & ~MS_RING_ON));
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to set DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore RING: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
    else
    {
	ok(EscapeCommFunction(hcom, SETDTR), "EscapeCommFunction failed to set DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok ((ModemStat & MS_RING_ON) == MS_RING_ON,
	    "RING didn't react: 0x%04lx,expected 0x%04lx\n",
	    ModemStat, (defaultStat | MS_RING_ON));
	ok(EscapeCommFunction(hcom, CLRDTR), "EscapeCommFunction failed to clear DTR\n");
	ok(GetCommModemStatus(hcom, &ModemStat), "GetCommModemStatus failed\n");
	ok (ModemStat ==  defaultStat, "Failed to restore RING: 0x%04lx, expected 0x%04lx\n",
	    ModemStat, defaultStat);
    }
}

/*
 * Set up a WaitCommEvent for anything in the receive buffer,
 * then write to TX to put a character
 * into the RX buffer
 * Need Loopback TX->RX
*/

static void  test_WaitRx(HANDLE hcom)
{
    OVERLAPPED overlapped, overlapped_w;
    HANDLE hComPortEvent, hComWriteEvent;
    DWORD before, after, after1, diff, success_wait = FALSE, success_write;
    DWORD err_wait, err_write, written, evtmask=0;

    ok(SetCommMask(hcom, EV_RXCHAR), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;

    ok((hComWriteEvent =  CreateEvent( NULL, TRUE, FALSE, NULL )) !=0,
       "CreateEvent res 0x%08lx\n",
       GetLastError());
    ZeroMemory( &overlapped_w, sizeof(overlapped_w));
    overlapped_w.hEvent = hComWriteEvent;

    before = GetTickCount();
    {success_wait = WaitCommEvent(hcom, &evtmask, &overlapped);}
    err_wait = GetLastError();
    after = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success_wait, err_wait, evtmask);
    ok(success_wait || err_wait == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");

    success_write= WriteFile(hcom, "X", 1, &written, &overlapped_w);
    err_write = GetLastError();
    ok(success_write || err_write == ERROR_IO_PENDING,
       "overlapped WriteFile failed, err 0x%08lx\n",
       err_write);

    if (!success_write && (err_write == ERROR_IO_PENDING)) {
      success_write = WaitForSingleObjectEx(hComWriteEvent, TIMEOUT, TRUE);
      err_write = GetLastError();
      ok(success_write == WAIT_OBJECT_0, "WaitForSingleObjectEx, res 0x%08lx, err 0x%08lx\n",
	 success_write, err_write);
    }
    Sleep(TIMEOUT >>1);
    success_write = GetOverlappedResult(hcom, &overlapped_w, &written, FALSE);
    err_write = GetLastError();

    trace("Write after Wait res 0x%08lx err 0x%08lx\n",success_write, err_write);
    ok(success_write && written ==1, "Write after Wait res 0x%08lx err 0x%08lx\n",
       success_write, err_write);

    if (!success_wait && (err_wait == ERROR_IO_PENDING)) {
      success_wait = WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE);
      err_wait = GetLastError();
      ok(success_wait == WAIT_OBJECT_0, "wait hComPortEvent, res 0x%08lx, err 0x%08lx\n",
	 success_wait, err_wait);
    }
    success_wait = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err_wait = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success_wait, err_wait, evtmask, after-before, after1-before);

    ok(evtmask & EV_RXCHAR, "Detect  EV_RXCHAR: 0x%08lx, expected 0x%08x\n",
       evtmask, EV_RXCHAR);
    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
	"Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

}

/* Change the controling line after the given timeout to the given state
   By the loopback, this should trigger the WaitCommEvent
*/
static DWORD CALLBACK toggle_ctlLine(LPVOID arg)
{
    DWORD *args = (DWORD *) arg;
    DWORD timeout = args[0];
    DWORD ctl     = args[1];
    HANDLE hcom   = (HANDLE) args[2];
    HANDLE hComPortEvent = (HANDLE) args[3];
    DWORD success, err;

    trace("toggle_ctlLine timeout %ld clt 0x%08lx handle 0x%08lx\n",
	  args[0], args[1], args[2]);
    Sleep(timeout);
    ok(EscapeCommFunction(hcom, ctl),"EscapeCommFunction 0x%08lx failed\n", ctl);
    trace("toggle_ctline done\n");
    success = WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE);
    err = GetLastError();
    trace("toggle_ctline WaitForSingleObjectEx res 0x%08lx err 0x%08lx\n",
	  success, err);
    return 0;
}

/*
 * Wait for a change in CTS
 * Needs Loopback from DTR to CTS
 */
static void  test_WaitCts(HANDLE hcom)
{
    DCB dcb;
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[4], defaultStat;
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    dcb.fRtsControl=RTS_CONTROL_ENABLE;
    dcb.fDtrControl=DTR_CONTROL_ENABLE;
    ok(SetCommState(hcom, &dcb), "SetCommState failed\n");
    if (dcb.fDtrControl == RTS_CONTROL_DISABLE)
    {
	trace("RTS_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    args[0]= TIMEOUT >>1;
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    if(defaultStat & MS_CTS_ON)
	args[1] = CLRRTS;
    else
	args[1] = SETRTS;
    args[2]=(DWORD) hcom;

    trace("test_WaitCts timeout %ld clt 0x%08lx handle 0x%08lx\n",args[0], args[1], args[2]);

    ok(SetCommMask(hcom, EV_CTS), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    args[3] = (DWORD) hComPortEvent;
    alarmThread = CreateThread(NULL, 0, toggle_ctlLine, (void *) &args, 0, &alarmThreadId);
    /* Wait a minimum to let the thread start up */
    Sleep(10);
    trace("Thread created\n");
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");
    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
		     "WaitCts hComPortEvent failed\n");
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask & EV_CTS, "Failed to detect  EV_CTS: 0x%08lx, expected 0x%08x\n",
		 evtmask, EV_CTS);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");
    if(defaultStat & MS_CTS_ON)
	ok((evtmask & MS_CTS_ON) == 0,"CTS didn't change state!\n");
    else
	ok((evtmask & MS_CTS_ON), "CTS didn't change state!\n");

    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
		  "Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

    /*restore RTS Settings*/
    if(defaultStat & MS_CTS_ON)
	args[1] = SETRTS;
    else
	args[1] = CLRRTS;
}

/* Change the  Comm Mask while a Wait is going on
   WaitCommevent should return with a EVTMASK set to zero
*/
static DWORD CALLBACK reset_CommMask(LPVOID arg)
{
    DWORD *args = (DWORD *) arg;
    DWORD timeout = args[0];
    HANDLE hcom   = (HANDLE) args[1];

    trace(" Changing CommMask on the fly for handle %p after timeout %ld\n",
	  hcom, timeout);
    Sleep(timeout);
    ok(SetCommMask(hcom, 0),"SetCommMask %p failed\n", hcom);
    trace("SetCommMask changed\n");
    return 0;
}

/* Set up a Wait for a change on CTS. We don't toggle any line, but we
   reset the CommMask and expect the wait to return with a mask of 0
   No special port connections needed
*/
static void  test_AbortWaitCts(HANDLE hcom)
{
    DCB dcb;
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[2];
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == RTS_CONTROL_DISABLE)
    {
	trace("RTS_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    args[0]= TIMEOUT >>1;
    args[1]=(DWORD) hcom;

    trace("test_AbortWaitCts timeout %ld handle 0x%08lx\n",args[0], args[1]);

    ok(SetCommMask(hcom, EV_CTS), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    alarmThread = CreateThread(NULL, 0, reset_CommMask, (void *) &args, 0, &alarmThreadId);
    /* Wait a minimum to let the thread start up */
    Sleep(10);
    trace("Thread created\n");
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");
    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
		     "AbortWaitCts hComPortEvent failed\n");
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask == 0, "Incorect EventMask 0x%08lx returned on Wait aborted bu SetCommMask, expected 0x%08x\n",
		 evtmask, 0);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");
    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
		  "Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

}

/*
 * Wait for a change in DSR
 * Needs Loopback from DTR to DSR
 */
static void  test_WaitDsr(HANDLE hcom)
{
    DCB dcb;
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[3], defaultStat;
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_DISABLE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    args[0]= TIMEOUT >>1;
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    if(defaultStat & MS_DSR_ON)
	args[1] = CLRDTR;
    else
	args[1] = SETDTR;
    args[2]=(DWORD) hcom;

    trace("test_WaitDsr timeout %ld clt 0x%08lx handle 0x%08lx\n",args[0], args[1], args[2]);

    ok(SetCommMask(hcom, EV_DSR), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    alarmThread = CreateThread(NULL, 0, toggle_ctlLine, (void *) &args, 0, &alarmThreadId);
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");
    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
		     "wait hComPortEvent failed\n");
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask & EV_DSR, "Failed to detect  EV_DSR: 0x%08lx, expected 0x%08x\n",
		 evtmask, EV_DSR);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");
    if(defaultStat & MS_DSR_ON)
	ok((evtmask & MS_DSR_ON) == 0,"DTR didn't change state!\n");
    else
	ok((evtmask & MS_DSR_ON), "DTR didn't change state!\n");

    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
		  "Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

    /*restore RTS Settings*/
    if(defaultStat & MS_DSR_ON)
	args[1] = SETDTR;
    else
	args[1] = CLRDTR;
}

/*
 * Wait for a Ring
 * Needs Loopback from DTR to RING
 */
static void  test_WaitRing(HANDLE hcom)
{
    DCB dcb;
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[3], defaultStat;
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_DISABLE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    args[0]= TIMEOUT >>1;
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    if(defaultStat & MS_RING_ON)
	args[1] = CLRDTR;
    else
	args[1] = SETDTR;
    args[2]=(DWORD) hcom;

    trace("test_WaitRing timeout %ld clt 0x%08lx handle 0x%08lx\n",args[0], args[1], args[2]);

    ok(SetCommMask(hcom, EV_RING), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    alarmThread = CreateThread(NULL, 0, toggle_ctlLine, (void *) &args, 0, &alarmThreadId);
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");
    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
		     "wait hComPortEvent failed\n");
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask & EV_RING, "Failed to detect  EV_RING: 0x%08lx, expected 0x%08x\n",
       evtmask, EV_CTS);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");
    if(defaultStat & MS_RING_ON)
	ok((evtmask & MS_RING_ON) == 0,"DTR didn't change state!\n");
    else
	ok((evtmask & MS_RING_ON), "DTR didn't change state!\n");

    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
		  "Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

    /*restore RTS Settings*/
    if(defaultStat & MS_RING_ON)
	args[1] = SETDTR;
    else
	args[1] = CLRDTR;
}
/*
 * Wait for a change in DCD
 * Needs Loopback from DTR to DCD
 */
static void  test_WaitDcd(HANDLE hcom)
{
    DCB dcb;
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[3], defaultStat;
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(GetCommState(hcom, &dcb), "GetCommState failed\n");
    if (dcb.fDtrControl == DTR_CONTROL_DISABLE)
    {
	trace("DTR_CONTROL_HANDSHAKE is set, so don't manipulate DTR\n");
	return;
    }
    args[0]= TIMEOUT >>1;
    ok(GetCommModemStatus(hcom, &defaultStat), "GetCommModemStatus failed\n");
    if(defaultStat & MS_RLSD_ON)
	args[1] = CLRDTR;
    else
	args[1] = SETDTR;
    args[2]=(DWORD) hcom;

    trace("test_WaitDcd timeout %ld clt 0x%08lx handle 0x%08lx\n",args[0], args[1], args[2]);

    ok(SetCommMask(hcom, EV_RLSD), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");
    alarmThread = CreateThread(NULL, 0, toggle_ctlLine, (void *) &args, 0, &alarmThreadId);
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");
    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
		     "wait hComPortEvent failed\n");
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask & EV_RLSD, "Failed to detect  EV_RLSD: 0x%08lx, expected 0x%08x\n",
		 evtmask, EV_CTS);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");
    if(defaultStat & MS_RLSD_ON)
	ok((evtmask & MS_RLSD_ON) == 0,"DTR didn't change state!\n");
    else
	ok((evtmask & MS_RLSD_ON), "DTR didn't change state!\n");

    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
		  "Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

    /*restore RTS Settings*/
    if(defaultStat & MS_RLSD_ON)
	args[1] = SETDTR;
    else
	args[1] = CLRDTR;
}

/*
   Set Break after timeout
*/
static DWORD CALLBACK set_CommBreak(LPVOID arg)
{
    DWORD *args = (DWORD *) arg;
    DWORD timeout = args[0];
    HANDLE hcom   = (HANDLE) args[1];

    trace("SetCommBreak for handle %p after timeout %ld\n",
	  hcom, timeout);
    Sleep(timeout);
    ok(SetCommBreak(hcom),"SetCommBreak %p failed\n", hcom);
    trace("SetCommBreak done\n");
    return 0;
}

/*
   Wait for the Break condition (TX resp. RX active)
   Needs Loopback TX-RX
*/
static void  test_WaitBreak(HANDLE hcom)
{
    OVERLAPPED overlapped;
    HANDLE hComPortEvent;
    HANDLE alarmThread;
    DWORD args[2];
    DWORD alarmThreadId, before, after, after1, diff, success, err, written, evtmask=0;

    ok(SetCommMask(hcom, EV_BREAK), "SetCommMask failed\n");
    hComPortEvent =  CreateEvent( NULL, TRUE, FALSE, NULL );
    ok(hComPortEvent != 0, "CreateEvent failed\n");

    trace("test_WaitBreak\n");
    args[0]= TIMEOUT >>1;
    args[1]=(DWORD) hcom;
    alarmThread = CreateThread(NULL, 0, set_CommBreak, (void *) &args, 0, &alarmThreadId);
    /* Wait a minimum to let the thread start up */
    Sleep(10);
    trace("Thread created\n");
    ok(alarmThread !=0 , "CreateThread Failed\n");

    ZeroMemory( &overlapped, sizeof(overlapped));
    overlapped.hEvent = hComPortEvent;
    before = GetTickCount();
    success = WaitCommEvent(hcom, &evtmask, &overlapped);
    err = GetLastError();
    after = GetTickCount();

    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx\n", success, err, evtmask);
    ok(success || err == ERROR_IO_PENDING, "overlapped WaitCommEvent failed\n");
    trace("overlapped WriteCommEvent returned.\n");

    if (!success && (err == ERROR_IO_PENDING))
	ok(WaitForSingleObjectEx(hComPortEvent, TIMEOUT, TRUE) == 0,
	   "wait hComPortEvent res 0x%08lx\n", GetLastError());
    success = GetOverlappedResult(hcom, &overlapped, &written, FALSE);
    err = GetLastError();
    after1 = GetTickCount();
    trace("Success 0x%08lx err 0x%08lx evtmask 0x%08lx diff1 %ld, diff2 %ld\n",
	  success, err, evtmask, after-before, after1-before);

    ok(evtmask & EV_BREAK, "Failed to detect  EV_BREAK: 0x%08lx, expected 0x%08x\n",
       evtmask, EV_BREAK);
    ok(GetCommModemStatus(hcom, &evtmask), "GetCommModemStatus failed\n");

    diff = after1 - before;
    ok ((diff > (TIMEOUT>>1) -TIMEDELTA) && (diff < (TIMEOUT>>1) + TIMEDELTA),
	"Unexpected time %ld, expected around %d\n", diff, TIMEOUT>>1);

    ok(ClearCommBreak(hcom), "ClearCommBreak failed\n");
}

START_TEST(comm)
{
    HANDLE hcom;
    /* use variables and not #define to compile the code */
    BOOL loopback_txd_rxd  = LOOPBACK_TXD_RXD;
    BOOL loopback_rts_cts  = LOOPBACK_CTS_RTS;
    BOOL loopback_dtr_dsr  = LOOPBACK_DTR_DSR;
    BOOL loopback_dtr_ring = LOOPBACK_DTR_RING;
    BOOL loopback_dtr_dcd  = LOOPBACK_DTR_DCD;

    test_BuildCommDCB();
    hcom = test_OpenComm(FALSE);
    if (hcom != INVALID_HANDLE_VALUE)
    {
	test_GetModemStatus(hcom);
	test_ReadTimeOut(hcom);
	test_waittxempty(hcom);
	CloseHandle(hcom);
    }
    hcom = test_OpenComm(FALSE);
    if (hcom != INVALID_HANDLE_VALUE)
    {
	Sleep(200); /* Give the laster character of test_waittxempty to drop into the receiver */
	test_ClearCommErrors(hcom);
	CloseHandle(hcom);
    }
    hcom = test_OpenComm(FALSE);
    if (hcom != INVALID_HANDLE_VALUE)
    {
        test_non_pending_errors(hcom);
	CloseHandle(hcom);
    }
    if((loopback_txd_rxd) && ((hcom = test_OpenComm(FALSE))!=INVALID_HANDLE_VALUE))
    {
	test_LoopbackRead(hcom);
	CloseHandle(hcom);
    }
    if((loopback_rts_cts) && ((hcom = test_OpenComm(FALSE))!=INVALID_HANDLE_VALUE))
    {
	test_LoopbackCtsRts(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_dsr) && ((hcom = test_OpenComm(FALSE))!=INVALID_HANDLE_VALUE))
    {
	test_LoopbackDtrDsr(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_ring) && ((hcom = test_OpenComm(FALSE))!=INVALID_HANDLE_VALUE))
    {
	test_LoopbackDtrRing(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_dcd) && ((hcom = test_OpenComm(FALSE))!=INVALID_HANDLE_VALUE))
    {
	test_LoopbackDtrDcd(hcom);
	CloseHandle(hcom);
    }
    if((loopback_txd_rxd) && ((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE))
    {
        test_WaitRx(hcom);
        CloseHandle(hcom);
    }
    if((loopback_rts_cts) && ((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE))
    {
	test_WaitCts(hcom);
	CloseHandle(hcom);
    }
    if((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE)
    {
	test_AbortWaitCts(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_dsr) && ((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE))
    {
	test_WaitDsr(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_ring) && ((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE))
    {
	test_WaitRing(hcom);
	CloseHandle(hcom);
    }
    if((loopback_dtr_dcd) && ((hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE))
    {
	test_WaitDcd(hcom);
	CloseHandle(hcom);
    }
    if(loopback_txd_rxd && (hcom = test_OpenComm(TRUE))!=INVALID_HANDLE_VALUE)
    {
	test_WaitBreak(hcom);
	CloseHandle(hcom);
    }
}
