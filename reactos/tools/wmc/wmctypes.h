/*
 * Main definitions and externals
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WMC_WMCTYPES_H
#define __WMC_WMCTYPES_H


/* Byteordering defines */
#define WMC_BO_NATIVE	0x00
#define WMC_BO_LITTLE	0x01
#define WMC_BO_BIG	0x02

#define WMC_LOBYTE(w)		((WORD)(w) & 0xff)
#define WMC_HIBYTE(w)		(((WORD)(w) >> 8) & 0xff)
#define WMC_LOWORD(d)		((DWORD)(d) & 0xffff)
#define WMC_HIWORD(d)		(((DWORD)(d) >> 16) & 0xffff)
#define BYTESWAP_WORD(w)	((WORD)(((WORD)WMC_LOBYTE(w) << 8) + (WORD)WMC_HIBYTE(w)))
#define BYTESWAP_DWORD(d)	((DWORD)(((DWORD)BYTESWAP_WORD(WMC_LOWORD(d)) << 16) + ((DWORD)BYTESWAP_WORD(WMC_HIWORD(d)))))

/*
 * Tokenizer types
 */
typedef enum tok_enum {
	tok_null = 0,
	tok_keyword,
	tok_severity,
	tok_facility,
	tok_language
} tok_e;

typedef struct token {
	tok_e		type;
	const WCHAR	*name;		/* Parsed name of token */
	int		token;		/* Tokenvalue or language code */
	int		codepage;
	const WCHAR	*alias; 	/* Alias or filename */
	int		fixed;		/* Cleared if token may change */
} token_t;

typedef struct lan_cp {
	int language;
	int codepage;
} lan_cp_t;

typedef struct cp_xlat {
	int	lan;
	int	cpin;
	int	cpout;
} cp_xlat_t;

typedef struct lanmsg {
	int		lan;		/* Language code of message */
	int		cp;		/* Codepage of message */
	WCHAR		*msg;		/* Message text */
	int		len;		/* Message length including trailing '\0' */
} lanmsg_t;

typedef struct msg {
	int		id;		/* Message ID */
	unsigned	realid;		/* Combined message ID */
	WCHAR		*sym;		/* Symbolic name */
	int		sev;		/* Severity code */
	int		fac;		/* Facility code */
	lanmsg_t	**msgs;		/* Array message texts */
	int		nmsgs;		/* Number of message texts in array */
	int		base;		/* Base of number to print */
	WCHAR		*cast;		/* Typecase to use */
} msg_t;

typedef enum {
	nd_msg,
	nd_comment
} node_e;

typedef struct node {
	struct node	*next;
	struct node	*prev;
	node_e		type;
	union {
		void 	*all;
		WCHAR	*comment;
		msg_t	*msg;
	} u;
} node_t;

typedef struct block {
	unsigned	idlo;		/* Lowest ID in this set */
	unsigned	idhi;		/* Highest ID in this set */
	int		size;		/* Size of this set */
	lanmsg_t	**msgs;		/* Array of messages in this set */
	int		nmsg;		/* Number of array entries */
} block_t;

typedef struct lan_blk {
	struct lan_blk	*next;		/* Linkage for languages */
	struct lan_blk	*prev;
	int		lan;		/* The language of this block */
	block_t		*blks;		/* Array of blocks for this language */
	int		nblk;		/* Nr of blocks in array */
} lan_blk_t;

#endif
