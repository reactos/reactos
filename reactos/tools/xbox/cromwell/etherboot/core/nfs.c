#include "etherboot.h"
#include "nic.h"

#ifdef	DOWNLOAD_PROTO_NFS

/* NOTE: the NFS code is heavily inspired by the NetBSD netboot code (read:
 * large portions are copied verbatim) as distributed in OSKit 0.97.  A few
 * changes were necessary to adapt the code to Etherboot and to fix several
 * inconsistencies.  Also the RPC message preparation is done "by hand" to
 * avoid adding netsprintf() which I find hard to understand and use.  */

/* NOTE 2: Etherboot does not care about things beyond the kernel image, so
 * it loads the kernel image off the boot server (ARP_SERVER) and does not
 * access the client root disk (root-path in dhcpd.conf), which would use
 * ARP_ROOTSERVER.  The root disk is something the operating system we are
 * about to load needs to use.  This is different from the OSKit 0.97 logic.  */

/* NOTE 3: Symlink handling introduced by Anselm M Hoffmeister, 2003-July-14
 * If a symlink is encountered, it is followed as far as possible (recursion
 * possible, maximum 16 steps). There is no clearing of ".."'s inside the
 * path, so please DON'T DO THAT. thx. */

#define START_OPORT 700		/* mountd usually insists on secure ports */
#define OPORT_SWEEP 200		/* make sure we don't leave secure range */

static int oport = START_OPORT;
static int mount_port = -1;
static int nfs_port = -1;
static int fs_mounted = 0;
static unsigned long rpc_id;

/**************************************************************************
RPC_INIT - set up the ID counter to something fairly random
**************************************************************************/
void rpc_init(void)
{
	unsigned long t;

	t = currticks();
	rpc_id = t ^ (t << 8) ^ (t << 16);
}


/**************************************************************************
RPC_PRINTERROR - Print a low level RPC error message
**************************************************************************/
static void rpc_printerror(struct rpc_t *rpc)
{
	if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
	    rpc->u.reply.astatus) {
		/* rpc_printerror() is called for any RPC related error,
		 * suppress output if no low level RPC error happened.  */
		printf("RPC error: (%d,%d,%d)\n", ntohl(rpc->u.reply.rstatus),
			ntohl(rpc->u.reply.verifier),
			ntohl(rpc->u.reply.astatus));
	}
}

/**************************************************************************
AWAIT_RPC - Wait for an rpc packet
**************************************************************************/
static int await_rpc(int ival, void *ptr,
	unsigned short ptype, struct iphdr *ip, struct udphdr *udp)
{
	struct rpc_t *rpc;
	if (!udp) 
		return 0;
	if (arptable[ARP_CLIENT].ipaddr.s_addr != ip->dest.s_addr)
		return 0;
	if (ntohs(udp->dest) != ival)
		return 0;
	if (nic.packetlen < ETH_HLEN + sizeof(struct iphdr) + sizeof(struct udphdr) + 8)
		return 0;
	rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
	if (*(unsigned long *)ptr != ntohl(rpc->u.reply.id))
		return 0;
	if (MSG_REPLY != ntohl(rpc->u.reply.type))
		return 0;
	return 1;
}

/**************************************************************************
RPC_LOOKUP - Lookup RPC Port numbers
**************************************************************************/
static int rpc_lookup(int addr, int prog, int ver, int sport)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	int retries;
	long *p;

	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_PORTMAP);
	buf.u.call.vers = htonl(2);	/* portmapper is version 2 */
	buf.u.call.proc = htonl(PORTMAP_GETPORT);
	p = (long *)buf.u.call.data;
	*p++ = 0; *p++ = 0;				/* auth credential */
	*p++ = 0; *p++ = 0;				/* auth verifier */
	*p++ = htonl(prog);
	*p++ = htonl(ver);
	*p++ = htonl(IP_UDP);
	*p++ = 0;
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout;
		udp_transmit(arptable[addr].ipaddr.s_addr, sport, SUNRPC_PORT,
			(char *)p - (char *)&buf, &buf);
		timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		if (await_reply(await_rpc, sport, &id, timeout)) {
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus) {
				rpc_printerror(rpc);
				return -1;
			} else {
				return ntohl(rpc->u.reply.data[0]);
			}
		}
	}
	return -1;
}

/**************************************************************************
RPC_ADD_CREDENTIALS - Add RPC authentication/verifier entries
**************************************************************************/
static long *rpc_add_credentials(long *p)
{
	int hl;

	/* Here's the executive summary on authentication requirements of the
	 * various NFS server implementations:  Linux accepts both AUTH_NONE
	 * and AUTH_UNIX authentication (also accepts an empty hostname field
	 * in the AUTH_UNIX scheme).  *BSD refuses AUTH_NONE, but accepts
	 * AUTH_UNIX (also accepts an empty hostname field in the AUTH_UNIX
	 * scheme).  To be safe, use AUTH_UNIX and pass the hostname if we have
	 * it (if the BOOTP/DHCP reply didn't give one, just use an empty
	 * hostname).  */

	hl = (hostnamelen + 3) & ~3;

	/* Provide an AUTH_UNIX credential.  */
	*p++ = htonl(1);		/* AUTH_UNIX */
	*p++ = htonl(hl+20);		/* auth length */
	*p++ = htonl(0);		/* stamp */
	*p++ = htonl(hostnamelen);	/* hostname string */
	if (hostnamelen & 3) {
		*(p + hostnamelen / 4) = 0; /* add zero padding */
	}
	memcpy(p, hostname, hostnamelen);
	p += hl / 4;
	*p++ = 0;			/* uid */
	*p++ = 0;			/* gid */
	*p++ = 0;			/* auxiliary gid list */

	/* Provide an AUTH_NONE verifier.  */
	*p++ = 0;			/* AUTH_NONE */
	*p++ = 0;			/* auth length */

	return p;
}

/**************************************************************************
NFS_PRINTERROR - Print a NFS error message
**************************************************************************/
static void nfs_printerror(int err)
{
	switch (-err) {
	case NFSERR_PERM:
		printf("Not owner\n");
		break;
	case NFSERR_NOENT:
		printf("No such file or directory\n");
		break;
	case NFSERR_ACCES:
		printf("Permission denied\n");
		break;
	case NFSERR_ISDIR:
		printf("Directory given where filename expected\n");
		break;
	case NFSERR_INVAL:
		printf("Invalid filehandle\n");
		break; // INVAL is not defined in NFSv2, some NFS-servers
		// seem to use it in answers to v2 nevertheless.
	case 9998:
		printf("low-level RPC failure (parameter decoding problem?)\n");
		break;
	case 9999:
		printf("low-level RPC failure (authentication problem?)\n");
		break;
	default:
		printf("Unknown NFS error %d\n", -err);
	}
}

/**************************************************************************
NFS_MOUNT - Mount an NFS Filesystem
**************************************************************************/
static int nfs_mount(int server, int port, char *path, char *fh, int sport)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	int retries;
	long *p;
	int pathlen = strlen(path);

	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_MOUNT);
	buf.u.call.vers = htonl(1);	/* mountd is version 1 */
	buf.u.call.proc = htonl(MOUNT_ADDENTRY);
	p = rpc_add_credentials((long *)buf.u.call.data);
	*p++ = htonl(pathlen);
	if (pathlen & 3) {
		*(p + pathlen / 4) = 0;	/* add zero padding */
	}
	memcpy(p, path, pathlen);
	p += (pathlen + 3) / 4;
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout;
		udp_transmit(arptable[server].ipaddr.s_addr, sport, port,
			(char *)p - (char *)&buf, &buf);
		timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		if (await_reply(await_rpc, sport, &id, timeout)) {
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
				rpc_printerror(rpc);
				if (rpc->u.reply.rstatus) {
					/* RPC failed, no verifier, data[0] */
					return -9999;
				}
				if (rpc->u.reply.astatus) {
					/* RPC couldn't decode parameters */
					return -9998;
				}
				return -ntohl(rpc->u.reply.data[0]);
			} else {
				fs_mounted = 1;
				memcpy(fh, rpc->u.reply.data + 1, NFS_FHSIZE);
				return 0;
			}
		}
	}
	return -1;
}

/**************************************************************************
NFS_UMOUNTALL - Unmount all our NFS Filesystems on the Server
**************************************************************************/
void nfs_umountall(int server)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	int retries;
	long *p;

	if (!arptable[server].ipaddr.s_addr) {
		/* Haven't sent a single UDP packet to this server */
		return;
	}
	if ((mount_port == -1) || (!fs_mounted)) {
		/* Nothing mounted, nothing to umount */
		return;
	}
	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_MOUNT);
	buf.u.call.vers = htonl(1);	/* mountd is version 1 */
	buf.u.call.proc = htonl(MOUNT_UMOUNTALL);
	p = rpc_add_credentials((long *)buf.u.call.data);
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		udp_transmit(arptable[server].ipaddr.s_addr, oport, mount_port,
			(char *)p - (char *)&buf, &buf);
		if (await_reply(await_rpc, oport, &id, timeout)) {
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus) {
				rpc_printerror(rpc);
			}
			fs_mounted = 0;
			return;
		}
	}
}
/***************************************************************************
 * NFS_READLINK (AH 2003-07-14)
 * This procedure is called when read of the first block fails -
 * this probably happens when it's a directory or a symlink
 * In case of successful readlink(), the dirname is manipulated,
 * so that inside the nfs() function a recursion can be done.
 **************************************************************************/
static int nfs_readlink(int server, int port, char *fh, char *path, char *nfh,
	int sport)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	long *p;
	int retries;
	int pathlen = strlen(path);

	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_NFS);
	buf.u.call.vers = htonl(2);	/* nfsd is version 2 */
	buf.u.call.proc = htonl(NFS_READLINK);
	p = rpc_add_credentials((long *)buf.u.call.data);
	memcpy(p, nfh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		udp_transmit(arptable[server].ipaddr.s_addr, sport, port,
			(char *)p - (char *)&buf, &buf);
		if (await_reply(await_rpc, sport, &id, timeout)) {
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
				rpc_printerror(rpc);
				if (rpc->u.reply.rstatus) {
					/* RPC failed, no verifier, data[0] */
					return -9999;
				}
				if (rpc->u.reply.astatus) {
					/* RPC couldn't decode parameters */
					return -9998;
				}
				return -ntohl(rpc->u.reply.data[0]);
			} else {
				// It *is* a link.
				// If it's a relative link, append everything to dirname, filename TOO!
				retries = strlen ( (char *)(&(rpc->u.reply.data[2]) ));
				if ( *((char *)(&(rpc->u.reply.data[2]))) != '/' ) {
					path[pathlen++] = '/';
					while ( ( retries + pathlen ) > 298 ) {
						retries--;
					}
					if ( retries > 0 ) {
						memcpy(path + pathlen, &(rpc->u.reply.data[2]), retries + 1);
					} else { retries = 0; }
					path[pathlen + retries] = 0;
				} else {
					// Else make it the only path.
					if ( retries > 298 ) { retries = 298; }
					memcpy ( path, &(rpc->u.reply.data[2]), retries + 1 );
					path[retries] = 0;
				}
				return 0;
			}
		}
	}
	return -1;
}
/**************************************************************************
NFS_LOOKUP - Lookup Pathname
**************************************************************************/
static int nfs_lookup(int server, int port, char *fh, char *path, char *nfh,
	int sport)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	long *p;
	int retries;
	int pathlen = strlen(path);

	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_NFS);
	buf.u.call.vers = htonl(2);	/* nfsd is version 2 */
	buf.u.call.proc = htonl(NFS_LOOKUP);
	p = rpc_add_credentials((long *)buf.u.call.data);
	memcpy(p, fh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(pathlen);
	if (pathlen & 3) {
		*(p + pathlen / 4) = 0;	/* add zero padding */
	}
	memcpy(p, path, pathlen);
	p += (pathlen + 3) / 4;
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		udp_transmit(arptable[server].ipaddr.s_addr, sport, port,
			(char *)p - (char *)&buf, &buf);
		if (await_reply(await_rpc, sport, &id, timeout)) {
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
				rpc_printerror(rpc);
				if (rpc->u.reply.rstatus) {
					/* RPC failed, no verifier, data[0] */
					return -9999;
				}
				if (rpc->u.reply.astatus) {
					/* RPC couldn't decode parameters */
					return -9998;
				}
				return -ntohl(rpc->u.reply.data[0]);
			} else {
				memcpy(nfh, rpc->u.reply.data + 1, NFS_FHSIZE);
				return 0;
			}
		}
	}
	return -1;
}

/**************************************************************************
NFS_READ - Read File on NFS Server
**************************************************************************/
static int nfs_read(int server, int port, char *fh, int offset, int len,
		    int sport)
{
	struct rpc_t buf, *rpc;
	unsigned long id;
	int retries;
	long *p;

	static int tokens=0;
	/*
	 * Try to implement something similar to a window protocol in
	 * terms of response to losses. On successful receive, increment
	 * the number of tokens by 1 (cap at 256). On failure, halve it.
	 * When the number of tokens is >= 2, use a very short timeout.
	 */

	id = rpc_id++;
	buf.u.call.id = htonl(id);
	buf.u.call.type = htonl(MSG_CALL);
	buf.u.call.rpcvers = htonl(2);	/* use RPC version 2 */
	buf.u.call.prog = htonl(PROG_NFS);
	buf.u.call.vers = htonl(2);	/* nfsd is version 2 */
	buf.u.call.proc = htonl(NFS_READ);
	p = rpc_add_credentials((long *)buf.u.call.data);
	memcpy(p, fh, NFS_FHSIZE);
	p += NFS_FHSIZE / 4;
	*p++ = htonl(offset);
	*p++ = htonl(len);
	*p++ = 0;		/* unused parameter */
	for (retries = 0; retries < MAX_RPC_RETRIES; retries++) {
		long timeout = rfc2131_sleep_interval(TIMEOUT, retries);
		if (tokens >= 2)
			timeout = TICKS_PER_SEC/2;

		udp_transmit(arptable[server].ipaddr.s_addr, sport, port,
			(char *)p - (char *)&buf, &buf);
		if (await_reply(await_rpc, sport, &id, timeout)) {
			if (tokens < 256)
				tokens++;
			rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];
			if (rpc->u.reply.rstatus || rpc->u.reply.verifier ||
			    rpc->u.reply.astatus || rpc->u.reply.data[0]) {
				rpc_printerror(rpc);
				if (rpc->u.reply.rstatus) {
					/* RPC failed, no verifier, data[0] */
					return -9999;
				}
				if (rpc->u.reply.astatus) {
					/* RPC couldn't decode parameters */
					return -9998;
				}
				return -ntohl(rpc->u.reply.data[0]);
			} else {
				return 0;
			}
		} else
			tokens >>= 1;
	}
	return -1;
}

/**************************************************************************
NFS - Download extended BOOTP data, or kernel image from NFS server
**************************************************************************/
int nfs(const char *name, int (*fnc)(unsigned char *, unsigned int, unsigned int, int))
{
	static int recursion = 0;
	int sport;
	int err, namelen = strlen(name);
	char dirname[300], *fname;
	char dirfh[NFS_FHSIZE];		/* file handle of directory */
	char filefh[NFS_FHSIZE];	/* file handle of kernel image */
	unsigned int block;
	int rlen, size, offs, len;
	struct rpc_t *rpc;

	rx_qdrain();

	sport = oport++;
	if (oport > START_OPORT+OPORT_SWEEP) {
		oport = START_OPORT;
	}
	if ( name != dirname ) {
		memcpy(dirname, name, namelen + 1);
	}
	recursion = 0;
nfssymlink:
	if ( recursion > NFS_MAXLINKDEPTH ) {
		printf ( "\nRecursion: More than %d symlinks followed. Abort.\n", NFS_MAXLINKDEPTH );
		return	0;
	}
	recursion++;
	fname = dirname + (namelen - 1);
	while (fname >= dirname) {
		if (*fname == '/') {
			*fname = '\0';
			fname++;
			break;
		}
		fname--;
	}
	if (fname < dirname) {
		printf("can't parse file name %s\n", name);
		return 0;
	}

	if (mount_port == -1) {
		mount_port = rpc_lookup(ARP_SERVER, PROG_MOUNT, 1, sport);
	}
	if (nfs_port == -1) {
		nfs_port = rpc_lookup(ARP_SERVER, PROG_NFS, 2, sport);
	}
	if (nfs_port == -1 || mount_port == -1) {
		printf("can't get nfs/mount ports from portmapper\n");
		return 0;
	}


	err = nfs_mount(ARP_SERVER, mount_port, dirname, dirfh, sport);
	if (err) {
		printf("mounting %s: ", dirname);
		nfs_printerror(err);
		/* just to be sure... */
		nfs_umountall(ARP_SERVER);
		return 0;
	}

	err = nfs_lookup(ARP_SERVER, nfs_port, dirfh, fname, filefh, sport);
	if (err) {
		printf("looking up %s: ", fname);
		nfs_printerror(err);
		nfs_umountall(ARP_SERVER);
		return 0;
	}

	offs = 0;
	block = 1;	/* blocks are numbered starting from 1 */
	size = -1;	/* will be set properly with the first reply */
	len = NFS_READ_SIZE;	/* first request is always full size */
	do {
		err = nfs_read(ARP_SERVER, nfs_port, filefh, offs, len, sport);
                if ((err <= -NFSERR_ISDIR)&&(err >= -NFSERR_INVAL) && (offs == 0)) {
			// An error occured. NFS servers tend to sending
			// errors 21 / 22 when symlink instead of real file
			// is requested. So check if it's a symlink!
			block = nfs_readlink(ARP_SERVER, nfs_port, dirfh, dirname,
			                filefh, sport);
			if ( 0 == block ) {
				printf("\nLoading symlink:%s ..",dirname);
				goto nfssymlink;
			}
			nfs_printerror(err);
			nfs_umountall(ARP_SERVER);
			return 0;
		}
		if (err) {
			printf("reading at offset %d: ", offs);
			nfs_printerror(err);
			nfs_umountall(ARP_SERVER);
			return 0;
		}

		rpc = (struct rpc_t *)&nic.packet[ETH_HLEN];

		/* size must be found out early to allow EOF detection */
		if (size == -1) {
			size = ntohl(rpc->u.reply.data[6]);
		}
		rlen = ntohl(rpc->u.reply.data[18]);
		if (rlen > len) {
			rlen = len;	/* shouldn't happen...  */
		}

		err = fnc((char *)&rpc->u.reply.data[19], block, rlen,
			(offs+rlen == size));
		if (err <= 0) {
			nfs_umountall(ARP_SERVER);
			return err;
		}

		block++;
		offs += rlen;
		/* last request is done with matching requested read size */
		if (size-offs < NFS_READ_SIZE) {
			len = size-offs;
		}
	} while (len != 0);
	/* len == 0 means that all the file has been read */
	return 1;
}

#endif	/* DOWNLOAD_PROTO_NFS */
