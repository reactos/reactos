#ifndef TCPMISC_H
#define TCPMISC_H

#define	IF_MIB_STATS_ID		1
#define	MAX_PHYSADDR_SIZE	8
#define	MAX_IFDESCR_LEN			256

/* ID to use for requesting an IFEntry for an interface */
#define IF_MIB_STATS_ID            1

/* ID to use for requesting an IPSNMPInfo for an interface */
#define IP_MIB_STATS_ID            1

/* ID to use for requesting the route table */
#define IP_MIB_ROUTETABLE_ENTRY_ID  0x101
#define IP_MIB_ADDRTABLE_ENTRY_ID   0x102

typedef struct IFEntry {
	ULONG			if_index;
	ULONG			if_type;
	ULONG			if_mtu;
	ULONG			if_speed;
	ULONG			if_physaddrlen;
	UCHAR			if_physaddr[MAX_PHYSADDR_SIZE];
	ULONG			if_adminstatus;
	ULONG			if_operstatus;
	ULONG			if_lastchange;
	ULONG			if_inoctets;
	ULONG			if_inucastpkts;
	ULONG			if_innucastpkts;
	ULONG			if_indiscards;
	ULONG			if_inerrors;
	ULONG			if_inunknownprotos;
	ULONG			if_outoctets;
	ULONG			if_outucastpkts;
	ULONG			if_outnucastpkts;
	ULONG			if_outdiscards;
	ULONG			if_outerrors;
	ULONG			if_outqlen;
	ULONG			if_descrlen;
	UCHAR			if_descr[1];
} IFEntry;

// As in the mib from RFC 1213

typedef struct _IPRouteEntry {
    ULONG ire_dest;
    ULONG ire_index;            //matches if_index in IFEntry and iae_index in IPAddrEntry
    ULONG ire_metric1;
    ULONG ire_metric2;
    ULONG ire_metric3;
    ULONG ire_metric4;
    ULONG ire_gw;
    ULONG ire_type; 
    ULONG ire_proto;
    ULONG ire_age; 
    ULONG ire_mask;
    ULONG ire_metric5;
    ULONG ire_info;
} IPRouteEntry;

typedef struct _IPAddrEntry {
    ULONG iae_addr;
    ULONG iae_index;
    ULONG iae_mask;
    ULONG iae_bcastaddr;
    ULONG iae_reasmsize;
    ULONG iae_context;
    ULONG iae_pad;
} IPAddrEntry;

typedef struct _IPSNMPInfo {
    ULONG ipsi_index;
    ULONG ipsi_forwarding;
    ULONG ipsi_defaultttl;
    ULONG ipsi_inreceives;
    ULONG ipsi_inhdrerrors;
    ULONG ipsi_inaddrerrors;
    ULONG ipsi_inunknownprotos;
    ULONG ipsi_indiscards;
    ULONG ipsi_indelivers;
    ULONG ipsi_outrequests;
    ULONG ipsi_routingdiscards;
    ULONG ipsi_outdiscards;
    ULONG ipsi_outnoroutes;
    ULONG ipsi_reasmtimeout;
    ULONG ipsi_reasmreqds;
    ULONG ipsi_reasmoks;
    ULONG ipsi_reasmfails;
    ULONG ipsi_fragoks;
    ULONG ipsi_fragfails;
    ULONG ipsi_fragcreates;
    ULONG ipsi_numif;
    ULONG ipsi_numaddr;
    ULONG ipsi_numroutes;
} IPSNMPInfo;

#endif /* TCPMISC_H */
