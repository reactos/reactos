#ifndef	_IF_ARP_H
#define	_IF_ARP_H

#define ARP_REQUEST	1
#define ARP_REPLY	2

/*
 * A pity sipaddr and tipaddr are not longword aligned or we could use
 * in_addr. No, I don't want to use #pragma packed.
 */
struct arprequest {
	uint16_t hwtype;
	uint16_t protocol;
	uint8_t  hwlen;
	uint8_t  protolen;
	uint16_t opcode;
	uint8_t  shwaddr[6];
	uint8_t  sipaddr[4];
	uint8_t  thwaddr[6];
	uint8_t  tipaddr[4];
};

#endif	/* _IF_ARP_H */
