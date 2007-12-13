#ifndef	_IP_H
#define	_IP_H

struct iphdr {
	uint8_t  verhdrlen;
	uint8_t  service;
	uint16_t len;
	uint16_t ident;
	uint16_t frags;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t chksum;
	in_addr src;
	in_addr dest;
};

#endif	/* _IP_H */
