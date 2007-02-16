
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
typedef struct ifaddr {
    struct	sockaddr *ifa_addr;	/* address of interface */
    struct	sockaddr *ifa_dstaddr;	/* other end of p-to-p link */
#define	ifa_broadaddr	ifa_dstaddr	/* broadcast address interface */
    struct	sockaddr *ifa_netmask;	/* used to determine subnet */
    unsigned short	ifa_flags;		/* mostly rt_flags for cloning */
    short	ifa_refcnt;		/* extra to malloc for link info */
    int	        ifa_metric;		/* cost of going out this interface */
    unsigned short     ifa_mtu;                /* MTU */
} OSK_IFADDR, *POSK_IFADDR;

#define	IFA_ROUTE	RTF_UP		/* route installed */
#define OSK_IFQ_MAXLEN  50

#endif/*OSKITTYPES_H*/
