#ifndef	_IGMP_H
#define	_IGMP_H

#define IGMP_QUERY	0x11
#define IGMPv1_REPORT	0x12
#define IGMPv2_REPORT	0x16
#define IGMP_LEAVE	0x17
#define GROUP_ALL_HOSTS 0xe0000001 /* 224.0.0.1 Host byte order */

struct igmp {
	uint8_t  type;
	uint8_t  response_time;
	uint16_t chksum;
	in_addr group;
};

struct igmp_ip_t { /* Format of an igmp ip packet */
	struct iphdr ip;
	uint8_t router_alert[4]; /* Router alert option */
	struct igmp igmp;
};

#endif	/* _IGMP_H */
