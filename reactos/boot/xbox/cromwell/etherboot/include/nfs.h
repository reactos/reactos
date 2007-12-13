#ifndef	_NFS_H
#define	_NFS_H

#define SUNRPC_PORT	111

#define PROG_PORTMAP	100000
#define PROG_NFS	100003
#define PROG_MOUNT	100005

#define MSG_CALL	0
#define MSG_REPLY	1

#define PORTMAP_GETPORT	3

#define MOUNT_ADDENTRY	1
#define MOUNT_UMOUNTALL	4

#define NFS_LOOKUP	4
#define	NFS_READLINK	5
#define NFS_READ	6

#define NFS_FHSIZE	32

#define NFSERR_PERM	1
#define NFSERR_NOENT	2
#define NFSERR_ACCES	13
#define	NFSERR_ISDIR	21
#define	NFSERR_INVAL	22

/* Block size used for NFS read accesses.  A RPC reply packet (including  all
 * headers) must fit within a single Ethernet frame to avoid fragmentation.
 * Chosen to be a power of two, as most NFS servers are optimized for this.  */
#define NFS_READ_SIZE	1024

#define NFS_MAXLINKDEPTH 16

struct rpc_t {
	struct iphdr ip;
	struct udphdr udp;
	union {
		uint8_t  data[300];		/* longest RPC call must fit!!!! */
		struct {
			uint32_t id;
			uint32_t type;
			uint32_t rpcvers;
			uint32_t prog;
			uint32_t vers;
			uint32_t proc;
			uint32_t data[1];
		} call;
		struct {
			uint32_t id;
			uint32_t type;
			uint32_t rstatus;
			uint32_t verifier;
			uint32_t v2;
			uint32_t astatus;
			uint32_t data[1];
		} reply;
	} u;
};

#endif	/* _NFS_H */
