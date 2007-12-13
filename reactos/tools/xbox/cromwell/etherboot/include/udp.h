#ifndef	_UDP_H
#define	_UDP_H

struct udp_pseudo_hdr {
	in_addr  src;
	in_addr  dest;
	uint8_t  unused;
	uint8_t  protocol;
	uint16_t len;
};
struct udphdr {
	uint16_t src;
	uint16_t dest;
	uint16_t len;
	uint16_t chksum;
};

#endif	/* _UDP_H */
