/**************************************************************************
Etherboot -  Network Bootstrap Program

Literature dealing with the network protocols:
	ARP - RFC826
	RARP - RFC903
	UDP - RFC768
	BOOTP - RFC951, RFC2132 (vendor extensions)
	DHCP - RFC2131, RFC2132 (options)
	TFTP - RFC1350, RFC2347 (options), RFC2348 (blocksize), RFC2349 (tsize)
	RPC - RFC1831, RFC1832 (XDR), RFC1833 (rpcbind/portmapper)
	NFS - RFC1094, RFC1813 (v3, useful for clarifications, not implemented)
	IGMP - RFC1112

**************************************************************************/

/* #define MDEBUG */

#include "etherboot.h"
#include "dev.h"
#include "nic.h"
#include "timer.h"
#include "cpu.h"

int	url_port;		

#ifdef	IMAGE_FREEBSD
int freebsd_howto = 0;
char freebsd_kernel_env[FREEBSD_KERNEL_ENV_SIZE];
#endif

#ifdef FREEBSD_PXEEMU
extern char		pxeemu_nbp_active;
#endif	/* FREEBSD_PXEBOOT */


/**************************************************************************
LOADKERNEL - Try to load kernel image
**************************************************************************/
struct proto {
	char *name;
	int (*load)(const char *name,
		int (*fnc)(unsigned char *, unsigned int, unsigned int, int));
};
static const struct proto protos[] = {
#ifdef DOWNLOAD_PROTO_TFTM
	{ "x-tftm", url_tftm },
#endif
#ifdef DOWNLOAD_PROTO_SLAM
	{ "x-slam", url_slam },
#endif
#ifdef DOWNLOAD_PROTO_NFS
	{ "nfs", nfs },
#endif
#ifdef DOWNLOAD_PROTO_DISK
	{ "file", url_file },
#endif
#ifdef DOWNLOAD_PROTO_TFTP
	{ "tftp", tftp },
#endif
};

int loadkernel(const char *fname)
{
	static const struct proto * const last_proto = 
		&protos[sizeof(protos)/sizeof(protos[0])];
	const struct proto *proto;
	in_addr ip;
	int len;
	const char *name;

#if 0 && defined(CAN_BOOT_DISK)
	if (!memcmp(fname,"/dev/",5) && fname[6] == 'd') {
		int dev, part = 0;
		if (fname[5] == 'f') {
			if ((dev = fname[7] - '0') < 0 || dev > 3)
				goto nodisk; }
		else if (fname[5] == 'h' || fname[5] == 's') {
			if ((dev = 0x80 + fname[7] - 'a') < 0x80 || dev > 0x83)
				goto nodisk;
			if (fname[8]) {
				part = fname[8] - '0';
				if (fname[9])
					part = 10*part + fname[9] - '0'; }
			/* bootdisk cannot cope with more than eight partitions */
			if (part < 0 || part > 8)
				goto nodisk; }
		else
			goto nodisk;
		return(bootdisk(dev, part, load_block));
	}
#endif
	ip.s_addr = arptable[ARP_SERVER].ipaddr.s_addr;
	name = fname;
	url_port = -1;
	len = 0;
	while(fname[len] && fname[len] != ':') {
		len++;
	}
	for(proto = &protos[0]; proto < last_proto; proto++) {
		if (memcmp(name, proto->name, len) == 0) {
			break;
		}
	}
	if ((proto < last_proto) && (memcmp(fname + len, "://", 3) == 0)) {
		name += len + 3;
		if (name[0] != '/') {
			name += inet_aton(name, &ip);
			if (name[0] == ':') {
				name++;
				url_port = strtoul(name, &name, 10);
			}
		}
		if (name[0] == '/') {
			arptable[ARP_SERVER].ipaddr.s_addr = ip.s_addr;
			printf( "Loading %s ", fname );
			return proto->load(name + 1, load_block);
		}
	}
	printf("Loading %@:%s ", arptable[ARP_SERVER].ipaddr, fname);
#ifdef	DEFAULT_PROTO_NFS
	return nfs(fname, load_block);
#else
	return tftp(fname, load_block);
#endif
}


/**************************************************************************
CLEANUP - shut down networking and console so that the OS may be called 
**************************************************************************/
void cleanup(void)
{
#ifdef	DOWNLOAD_PROTO_NFS
	nfs_umountall(ARP_SERVER);
#endif
	/* Stop receiving packets */
	eth_disable();
}

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
