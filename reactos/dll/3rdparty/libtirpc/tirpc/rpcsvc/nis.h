#ifndef _RPCSVC_NIS_H
#define _RPCSVC_NIS_H

#define NIS_PK_NONE		0	/* no public key (unix/sys auth) */
#define NIS_PK_DH		1	/* Public key is Diffie-Hellman type */
#define NIS_PK_RSA		2	/* Public key is RSA type */
#define NIS_PK_KERB		3	/* Use kerberos style authentication */

typedef char * nis_name;
struct endpoint {
	char *uaddr;
	char *family;
	char *proto;
};
typedef struct endpoint endpoint;

struct nis_server{
	nis_name name;
	struct {
		u_int ep_len;
		endpoint *ep_val;
	} ep;
	uint32_t key_type;
	netobj pkey;
};
typedef struct nis_server nis_server;

#endif	/* !_RPCSVC_NIS_H */

