#ifndef ___TELNET_H
#define ___TELNET_H

/*
* Copyright (c) 1983 Regents of the University of California.
* All rights reserved.
*
* Redistribution and use in source and binary forms are permitted provided
* that: (1) source distributions retain this entire copyright notice and
* comment, and (2) distributions including binaries display the following
* acknowledgement:  ``This product includes software developed by the
* University of California, Berkeley and its contributors'' in the
* documentation or other materials provided with the distribution and in
* all advertising materials mentioning features or use of this software.
* Neither the name of the University nor the names of its contributors may
* be used to endorse or promote products derived from this software without
* specific prior written permission.
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*
*	@(#)telnet.h	5.12 (Berkeley) 3/5/91
*/

// This file modified 5/15/98 by Paul Brannan.
// Added (unsigned char) to the #defines.
// Formatted for readability.

/*
* Definitions for the TELNET protocol.
*/
#define	IAC		(unsigned char)255		/* interpret as command: */
#define	DONT	(unsigned char)254		/* you are not to use option */
#define	DO		(unsigned char)253		/* please, you use option */
#define	WONT	(unsigned char)252		/* I won't use option */
#define	WILL	(unsigned char)251		/* I will use option */
#define	SB		(unsigned char)250		/* interpret as subnegotiation */
#define	GA		(unsigned char)249		/* you may reverse the line */
#define	EL		(unsigned char)248		/* erase the current line */
#define	EC		(unsigned char)247		/* erase the current character */
#define	AYT		(unsigned char)246		/* are you there */
#define	AO		(unsigned char)245		/* abort output--but let prog finish */
#define	IP		(unsigned char)244		/* interrupt process--permanently */
#define	BREAK	(unsigned char)243		/* break */
#define	DM		(unsigned char)242		/* data mark--for connect. cleaning */
#define	NOP		(unsigned char)241		/* nop */
#define	SE		(unsigned char)240		/* end sub negotiation */
#define EOR     (unsigned char)239             /* end of record (transparent mode) */
#define	ABORT	(unsigned char)238		/* Abort process */
#define	SUSP	(unsigned char)237		/* Suspend process */
#define	xEOF	(unsigned char)236		/* End of file: EOF is already used... */

#define SYNCH	(unsigned char)242		/* for telfunc calls */

#ifdef TELCMDS
char *telcmds[] = {
	"EOF", "SUSP", "ABORT", "EOR",
		"SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
		"EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC", 0,
};
#else
extern char *telcmds[];
#endif

#define	TELCMD_FIRST		xEOF
#define	TELCMD_LAST			IAC
#define	TELCMD_OK(x)		((x) <= TELCMD_LAST && (x) >= TELCMD_FIRST)
#define	TELCMD(x)			telcmds[(x)-TELCMD_FIRST]

/* telnet options */
#define TELOPT_BINARY			(unsigned char)0	/* 8-bit data path */
#define TELOPT_ECHO				(unsigned char)1	/* echo */
#define	TELOPT_RCP				(unsigned char)2	/* prepare to reconnect */
#define	TELOPT_SGA				(unsigned char)3	/* suppress go ahead */
#define	TELOPT_NAMS				(unsigned char)4	/* approximate message size */
#define	TELOPT_STATUS			(unsigned char)5	/* give status */
#define	TELOPT_TM				(unsigned char)6	/* timing mark */
#define	TELOPT_RCTE				(unsigned char)7	/* remote controlled transmission and echo */
#define TELOPT_NAOL				(unsigned char)8	/* negotiate about output line width */
#define TELOPT_NAOP				(unsigned char)9	/* negotiate about output page size */
#define TELOPT_NAOCRD			(unsigned char)10	/* negotiate about CR disposition */
#define TELOPT_NAOHTS			(unsigned char)11	/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD			(unsigned char)12	/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD			(unsigned char)13	/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS			(unsigned char)14	/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD			(unsigned char)15	/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD			(unsigned char)16	/* negotiate about output LF disposition */
#define TELOPT_XASCII			(unsigned char)17	/* extended ascic character set */
#define	TELOPT_LOGOUT			(unsigned char)18	/* force logout */
#define	TELOPT_BM				(unsigned char)19	/* byte macro */
#define	TELOPT_DET				(unsigned char)20	/* data entry terminal */
#define	TELOPT_SUPDUP			(unsigned char)21	/* supdup protocol */
#define	TELOPT_SUPDUPOUTPUT		(unsigned char)22	/* supdup output */
#define	TELOPT_SNDLOC			(unsigned char)23	/* send location */
#define	TELOPT_TTYPE			(unsigned char)24	/* terminal type */
#define	TELOPT_EOR				(unsigned char)25	/* end or record */
#define	TELOPT_TUID				(unsigned char)26	/* TACACS user identification */
#define	TELOPT_OUTMRK			(unsigned char)27	/* output marking */
#define	TELOPT_TTYLOC			(unsigned char)28	/* terminal location number */
#define	TELOPT_3270REGIME		(unsigned char)29	/* 3270 regime */
#define	TELOPT_X3PAD			(unsigned char)30	/* X.3 PAD */
#define	TELOPT_NAWS				(unsigned char)31	/* window size */
#define	TELOPT_TSPEED			(unsigned char)32	/* terminal speed */
#define	TELOPT_LFLOW			(unsigned char)33	/* remote flow control */
#define TELOPT_LINEMODE			(unsigned char)34	/* Linemode option */
#define TELOPT_XDISPLOC			(unsigned char)35	/* X Display Location */
#define TELOPT_ENVIRON			(unsigned char)36	/* Environment variables */
#define	TELOPT_AUTHENTICATION	(unsigned char)37	/* Authenticate */
#define	TELOPT_ENCRYPT			(unsigned char)38	/* Encryption option */
#define	TELOPT_EXOPL			(unsigned char)255	/* extended-options-list */


#define	NTELOPTS	(1+TELOPT_ENCRYPT)
#ifdef TELOPTS
char *telopts[NTELOPTS+1] = {
	"BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
		"STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
		"NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
		"NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
		"DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
		"SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
		"TACACS UID", "OUTPUT MARKING", "TTYLOC",
		"3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
		"LINEMODE", "XDISPLOC", "ENVIRON", "AUTHENTICATION",
		"ENCRYPT",
		0,
};
#define	TELOPT_FIRST	TELOPT_BINARY
#define	TELOPT_LAST	TELOPT_ENCRYPT
#define	TELOPT_OK(x)	((x) <= TELOPT_LAST && (x) >= TELOPT_FIRST)
#define	TELOPT(x)	telopts[(x)-TELOPT_FIRST]
#endif

/* sub-option qualifiers */
#define	TELQUAL_IS		(unsigned char)0	/* option is... */
#define	TELQUAL_SEND	(unsigned char)1	/* send option */
#define	TELQUAL_INFO	(unsigned char)2	/* ENVIRON: informational version of IS */
#define	TELQUAL_REPLY	(unsigned char)2	/* AUTHENTICATION: client version of IS */
#define	TELQUAL_NAME	(unsigned char)3	/* AUTHENTICATION: client version of IS */

/*
* LINEMODE suboptions
*/

#define	LM_MODE		1
#define	LM_FORWARDMASK	2
#define	LM_SLC		3

#define	MODE_EDIT	0x01
#define	MODE_TRAPSIG	0x02
#define	MODE_ACK	0x04
#define MODE_SOFT_TAB	0x08
#define MODE_LIT_ECHO	0x10

#define	MODE_MASK	0x1f

/* Not part of protocol, but needed to simplify things... */
#define MODE_FLOW		0x0100
#define MODE_ECHO		0x0200
#define MODE_INBIN		0x0400
#define MODE_OUTBIN		0x0800
#define MODE_FORCE		0x1000

#define	SLC_SYNCH	1
#define	SLC_BRK		2
#define	SLC_IP		3
#define	SLC_AO		4
#define	SLC_AYT		5
#define	SLC_EOR		6
#define	SLC_ABORT	7
#define	SLC_EOF		8
#define	SLC_SUSP	9
#define	SLC_EC		10
#define	SLC_EL		11
#define	SLC_EW		12
#define	SLC_RP		13
#define	SLC_LNEXT	14
#define	SLC_XON		15
#define	SLC_XOFF	16
#define	SLC_FORW1	17
#define	SLC_FORW2	18

#define	NSLC		18

/*
* For backwards compatability, we define SLC_NAMES to be the
* list of names if SLC_NAMES is not defined.
*/
#define	SLC_NAMELIST	"0", "SYNCH", "BRK", "IP", "AO", "AYT", "EOR", \
	"ABORT", "EOF", "SUSP", "EC", "EL", "EW", "RP", \
"LNEXT", "XON", "XOFF", "FORW1", "FORW2", 0,
#ifdef	SLC_NAMES
//char *slc_names[] = {
//	SLC_NAMELIST
//};
#else
extern char *slc_names[];
#define	SLC_NAMES SLC_NAMELIST
#endif

#define	SLC_NAME_OK(x)	((x) >= 0 && (x) < NSLC)
#define SLC_NAME(x)	slc_names[x]

#define	SLC_NOSUPPORT	0
#define	SLC_CANTCHANGE	1
#define	SLC_VARIABLE	2
#define	SLC_DEFAULT	3
#define	SLC_LEVELBITS	0x03

#define	SLC_FUNC	0
#define	SLC_FLAGS	1
#define	SLC_VALUE	2

#define	SLC_ACK		0x80
#define	SLC_FLUSHIN	0x40
#define	SLC_FLUSHOUT	0x20

#define	ENV_VALUE	0
#define	ENV_VAR		1
#define	ENV_ESC		2

/*
* AUTHENTICATION suboptions
*/

/*
* Who is authenticating who ...
*/
#define	AUTH_WHO_CLIENT		0	/* Client authenticating server */
#define	AUTH_WHO_SERVER		1	/* Server authenticating client */
#define	AUTH_WHO_MASK		1

/*
* amount of authentication done
*/
#define	AUTH_HOW_ONE_WAY	0
#define	AUTH_HOW_MUTUAL		2
#define	AUTH_HOW_MASK		2

#define	AUTHTYPE_NULL		0
#define	AUTHTYPE_KERBEROS_V4	1
#define	AUTHTYPE_KERBEROS_V5	2
#define	AUTHTYPE_SPX		3
#define	AUTHTYPE_MINK		4
#define	AUTHTYPE_CNT		5

#define	AUTHTYPE_TEST		99

#ifdef	AUTH_NAMES
char *authtype_names[] = {
	"NULL", "KERBEROS_V4", "KERBEROS_V5", "SPX", "MINK", 0,
};
#else
extern char *authtype_names[];
#endif

#define	AUTHTYPE_NAME_OK(x)	((x) >= 0 && (x) < AUTHTYPE_CNT)
#define	AUTHTYPE_NAME(x)	authtype_names[x]

/*
* ENCRYPTion suboptions
*/
#define	ENCRYPT_IS		0	/* I pick encryption type ... */
#define	ENCRYPT_SUPPORT		1	/* I support encryption types ... */
#define	ENCRYPT_REPLY		2	/* Initial setup response */
#define	ENCRYPT_START		3	/* Am starting to send encrypted */
#define	ENCRYPT_END		4	/* Am ending encrypted */
#define	ENCRYPT_REQSTART	5	/* Request you start encrypting */
#define	ENCRYPT_REQEND		6	/* Request you send encrypting */
#define	ENCRYPT_ENC_KEYID	7
#define	ENCRYPT_DEC_KEYID	8
#define	ENCRYPT_CNT		9

#define	ENCTYPE_ANY		0
#define	ENCTYPE_DES_CFB64	1
#define	ENCTYPE_DES_OFB64	2
#define	ENCTYPE_CNT		3

#ifdef	ENCRYPT_NAMES
char *encrypt_names[] = {
	"IS", "SUPPORT", "REPLY", "START", "END",
		"REQUEST-START", "REQUEST-END", "ENC-KEYID", "DEC-KEYID",
		0,
};
char *enctype_names[] = {
	"ANY", "DES_CFB64",  "DES_OFB64",  0,
};
#else
extern char *encrypt_names[];
extern char *enctype_names[];
#endif


#define	ENCRYPT_NAME_OK(x)	((x) >= 0 && (x) < ENCRYPT_CNT)
#define	ENCRYPT_NAME(x)		encrypt_names[x]

#define	ENCTYPE_NAME_OK(x)	((x) >= 0 && (x) < ENCTYPE_CNT)
#define	ENCTYPE_NAME(x)		enctype_names[x]
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////



#endif

