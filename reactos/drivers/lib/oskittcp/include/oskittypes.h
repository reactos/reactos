
#ifndef OSKITTYPES_H
#define OSKITTYPES_H

typedef struct sockaddr OSK_SOCKADDR;
typedef unsigned char *OSK_PCHAR;
typedef unsigned int OSK_UINT;
typedef unsigned short OSK_UI16;
typedef char * osk_caddr_t;

#define SWAP_FAMILY_LEN(sabuf,s) \
  (sabuf)->sin_family = htons((sabuf)->sin_family)
/*  do { s = (sabuf)->sin_family; (sabuf)->sin_family = (sabuf)->sin_len; (sabuf)->sin_len = s; } while(0); */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
    struct	sockaddr *ifa_addr;	/* address of interface */
    struct	sockaddr *ifa_dstaddr;	/* other end of p-to-p link */
#define	ifa_broadaddr	ifa_dstaddr	/* broadcast address interface */
    struct	sockaddr *ifa_netmask;	/* used to determine subnet */
    u_short	ifa_flags;		/* mostly rt_flags for cloning */
    short	ifa_refcnt;		/* extra to malloc for link info */
    int	        ifa_metric;		/* cost of going out this interface */
    u_short     ifa_mtu;                /* MTU */
};
#define	IFA_ROUTE	RTF_UP		/* route installed */
#define OSK_IFQ_MAXLEN  50

/*
 * These numbers are used by reliable protocols for determining
 * retransmission behavior and are included in the routing structure.
 */
struct rt_metrics {
	u_long	rmx_locks;	/* Kernel must leave these values alone */
	u_long	rmx_mtu;	/* MTU for this path */
	u_long	rmx_hopcount;	/* max hops expected */
	u_long	rmx_expire;	/* lifetime for route, e.g. redirect */
	u_long	rmx_recvpipe;	/* inbound delay-bandwith product */
	u_long	rmx_sendpipe;	/* outbound delay-bandwith product */
	u_long	rmx_ssthresh;	/* outbound gateway buffer limit */
	u_long	rmx_rtt;	/* estimated round trip time */
	u_long	rmx_rttvar;	/* estimated rtt variance */
	u_long	rmx_pksent;	/* packets sent using this route */
	u_long	rmx_filler[4];	/* will be used for T/TCP later */
};

struct rtentry {
    struct	sockaddr *rt_gateway;	/* value */
    u_long	rt_flags;		/* up/down?, host/net */
    struct	ifaddr rt_ifa;		/* the answer: interface to use */
    struct	rt_metrics rt_rmx;	/* metrics used by rx'ing protocols */
    u_long      rt_mtu;                 /* Path MTU */
};

#endif/*OSKITTYPES_H*/
