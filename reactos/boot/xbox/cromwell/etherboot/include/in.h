#ifndef	_IN_H
#define	_IN_H

#define IP		0x0800
#define ARP		0x0806
#define	RARP		0x8035

#define IP_ICMP		1
#define IP_IGMP		2
#define IP_UDP		17

/* Same after going through htonl */
#define IP_BROADCAST	0xFFFFFFFF

typedef struct {
	uint32_t	s_addr;
} in_addr;

#endif	/* _IN_H */
