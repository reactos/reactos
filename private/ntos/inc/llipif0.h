/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** LLIPIF0.H - Lower layer IP interface definitions. (Version 0)
//
// This file contains the definitions defining the interface between IP
// and a lower layer, such as ARP or SLIP.
//
//
// Modifications:
//
// Feb 12, 97	ArvindM		Created this by making a copy of the existing
//							llipif.h, as a snapshot of the pre NT 5.0
//							version.
//

#ifndef LLIPIF0_INCLUDED
#define LLIPIF0_INCLUDED

// Typedefs for function pointers passed to ARPRegister.
typedef void  (__stdcall *IPRcvRtn)(void *, void *, uint , uint , NDIS_HANDLE , uint,
	uint);
typedef void  (__stdcall *IPTDCmpltRtn)(void *, PNDIS_PACKET , NDIS_STATUS , uint );
typedef	void  (__stdcall *IPTxCmpltRtn)(void *, PNDIS_PACKET , NDIS_STATUS );
typedef	void  (__stdcall *IPStatusRtn)(void *, uint, void *, uint);
typedef	void  (__stdcall *IPRcvCmpltRtn)(void);

typedef int (__stdcall *LLIPRegRtn)(PNDIS_STRING, void *, IPRcvRtn, IPTxCmpltRtn,
	IPStatusRtn, IPTDCmpltRtn, IPRcvCmpltRtn, struct LLIPBindInfo *, uint);

#define	LLIP_ADDR_LOCAL		0
#define	LLIP_ADDR_MCAST		1
#define	LLIP_ADDR_BCAST		2
#define	LLIP_ADDR_PARP		4

// Structure of information returned from ARP register call.
struct LLIPBindInfo {
	void		*lip_context;		// LL context handle.
	uint		lip_mss;			// Maximum segment size.
	uint		lip_speed;			// Speed of this i/f.
	uint		lip_index;			// Interface index ID.
	NDIS_STATUS (__stdcall *lip_transmit)(void *, PNDIS_PACKET, IPAddr, RouteCacheEntry *);
	NDIS_STATUS (__stdcall *lip_transfer)(void *, NDIS_HANDLE, uint, uint, uint, PNDIS_PACKET,
				uint *);			// Pointer to transfer data routine.
	void		(__stdcall *lip_close)(void *);
	uint		(__stdcall *lip_addaddr)(void *, uint, IPAddr, IPMask, void *);
	uint		(__stdcall *lip_deladdr)(void *, uint, IPAddr, IPMask);
	void		(__stdcall *lip_invalidate)(void *, RouteCacheEntry *);
	void		(__stdcall *lip_open)(void *);
	int			(__stdcall *lip_qinfo)(void *, struct TDIObjectID *, PNDIS_BUFFER,
					uint *, void *);
	int			(__stdcall *lip_setinfo)(void *, struct TDIObjectID *, void *, uint);
	int			(__stdcall *lip_getelist)(void *, void *, uint *);
	uint		lip_flags;			// Flags for this interface.
    uint        lip_addrlen;        // Length in bytes of address.
    uchar       *lip_addr;          // Pointer to interface address.
}; /* LLIPBindInfo */

#define	LIP_COPY_FLAG		1		// Copy lookahead flag.
#define LIP_P2P_FLAG        2       // Interface is point to point

typedef	struct LLIPBindInfo LLIPBindInfo;

//*	Status codes from the lower layer.
#define	LLIP_STATUS_MTU_CHANGE		1
#define	LLIP_STATUS_SPEED_CHANGE	2
#define LLIP_STATUS_ADDR_MTU_CHANGE	3

//*	The LLIP_STATUS_MTU_CHANGE passed a pointer to this structure.
struct LLIPMTUChange {
	uint		lmc_mtu;			// New MTU.
}; /* LLIPMTUChange */

typedef struct LLIPMTUChange LLIPMTUChange;

//*	The LLIP_STATUS_SPEED_CHANGE passed a pointer to this structure.
struct LLIPSpeedChange {
	uint		lsc_speed;			// New speed.
}; /* LLIPSpeedChange */

typedef struct LLIPSpeedChange LLIPSpeedChange;

//*	The LLIP_STATUS_ADDR_MTU_CHANGE passed a pointer to this structure.
struct LLIPAddrMTUChange {
	uint		lam_mtu;			// New MTU.
	uint		lam_addr;			// Address that changed.
}; /* LLIPAddrMTUChange */

typedef struct LLIPAddrMTUChange LLIPAddrMTUChange;

#ifdef NT

//
// Registration IOCTL code definition -
//
// This IOCTL is issued to a lower layer driver to retrieve the address
// of its registration function. There is no input buffer. The output
// buffer will contain a LLIPIF_REGISTRATION_DATA structure. This
// buffer is pointed to by Irp->AssociatedIrp.SystemBuffer and should be
// filled in before completion.
//

//
// structure passed in the registration IOCTL.
//
typedef struct llipif_registration_data {
	LLIPRegRtn    RegistrationFunction;
} LLIPIF_REGISTRATION_DATA;


extern IP_STATUS IPAddInterface(PNDIS_STRING ConfigName, void *PNP,
        void *Context,  LLIPRegRtn      RegRtn, LLIPBindInfo *BindInfo);
extern void IPDelInterface(void *Context);

// Definiton for IPAddInterfacePtr and IPDelInterfacePtr routines
//
typedef IP_STATUS (*IPAddInterfacePtr)(PNDIS_STRING ConfigName,
                                       void *PNPContext,
                                       void *Context,
                                       LLIPRegRtn  RegRtn,
                                       LLIPBindInfo *BindInfo);

typedef IP_STATUS (*IPDelInterfacePtr)(void *Context) ;

//* Structure used in IOCTL_IP_GET_PNP_ARP_POINTERS ioctl sent to \device\ip by ARP modules
//
typedef struct ip_get_arp_pointers {
    IPAddInterfacePtr   IPAddInterface ;    // Pointer to IP's add interface routine
    IPDelInterfacePtr   IPDelInterface ; // Pointer to IP's del interface routine
} IP_GET_PNP_ARP_POINTERS, *PIP_GET_PNP_ARP_POINTERS ;


#define FSCTL_LLIPIF_BASE     FILE_DEVICE_NETWORK

#define _LLIPIF_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_LLIPIF_BASE, function, method, access)


#define IOCTL_LLIPIF_REGISTER    \
            _LLIPIF_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // NT


#endif // LLIPIF0_INCLUDED
