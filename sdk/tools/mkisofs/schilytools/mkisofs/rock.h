/* @(#)rock.h	1.3 14/04/29 Copyright 2003-2014 J. Schilling */
/*
 * Header file for the Rock Ridge encoder and parser
 *
 * Copyright (c) 2003-2014 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * Defines for SUSP and Rock Ridge signature flags.
 * The first block is defined by RRIP-1.09.
 */
#define	RR_FLAG_PX	1	/* POSIX attributes		*/
#define	RR_FLAG_PN	2	/* POSIX device number		*/
#define	RR_FLAG_SL	4	/* Symlink			*/
#define	RR_FLAG_NM	8	/* Alternate Name		*/
#define	RR_FLAG_CL	16	/* Child link			*/
#define	RR_FLAG_PL	32	/* Parent link			*/
#define	RR_FLAG_RE	64	/* Relocated Direcotry		*/
#define	RR_FLAG_TF	128	/* Time stamp			*/

#define	RR_FLAG_SF	256	/* Sparse File			*/

#define	RR_FLAG_SP	1024	/* SUSP record			*/
#define	RR_FLAG_AA	2048	/* Apple Signature record	*/
#define	RR_FLAG_XA	4096	/* XA signature record		*/

#define	RR_FLAG_CE	8192	/* SUSP Continuation aerea	*/
#define	RR_FLAG_ER	16384	/* Extensions Reference for RR	*/
#define	RR_FLAG_RR	32768	/* RR Signature in every file	*/
#define	RR_FLAG_ZF	65535	/* Linux compression extension	*/

/*
 * Defines that control the behavior of the Rock Ridge encoder
 */
#define	NEED_RE		1	/* Need Relocated Direcotry	*/
#define	NEED_PL		2	/* Need Parent link		*/
#define	NEED_CL		4	/* Need Child link		*/
#define	NEED_CE		8	/* Need Continuation Area	*/
#define	NEED_SP		16	/* Need SUSP record		*/
#define	DID_CHDIR	1024	/* Did chdir() to file dir	*/
