#include <wintirpc.h>

//#include <sys/cdefs.h>
//#include <pthread.h>
#include <reentrant.h>
#include <rpc/rpc.h>
//#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

/*
 * XXX
 * For Windows, these must initialized in the DLLMain() function.
 * Cannot do static initialization!!
 * XXX
 */

/* protects the services list (svc.c) */
rwlock_t	svc_lock;

/* protects svc_fdset and the xports[] array */
rwlock_t	svc_fd_lock;

/* protects the RPCBIND address cache */
rwlock_t	rpcbaddr_cache_lock;

/* protects authdes cache (svcauth_des.c) */
mutex_t	authdes_lock;

/* serializes authdes ops initializations */
mutex_t authdes_ops_lock;

/* protects des stats list */
mutex_t svcauthdesstats_lock;

#ifdef KERBEROS
/* auth_kerb.c serialization */
mutex_t authkerb_lock = PTHREAD_MUTEX_INITIALIZER;
/* protects kerb stats list */
mutex_t svcauthkerbstats_lock = PTHREAD_MUTEX_INITIALIZER;
#endif /* KERBEROS */

/* auth_none.c serialization */
mutex_t	authnone_lock;

/* protects the Auths list (svc_auth.c) */
mutex_t	authsvc_lock;

/* protects client-side fd lock array */
mutex_t	clnt_fd_lock;

/* clnt_raw.c serialization */
mutex_t	clntraw_lock;

/* domainname and domain_fd (getdname.c) and default_domain (rpcdname.c) */
mutex_t	dname_lock;

/* dupreq variables (svc_dg.c) */
mutex_t	dupreq_lock;

/* protects first_time and hostname (key_call.c) */
mutex_t	keyserv_lock;

/* serializes rpc_trace() (rpc_trace.c) */
mutex_t	libnsl_trace_lock;

/* loopnconf (rpcb_clnt.c) */
mutex_t	loopnconf_lock;

/* serializes ops initializations */
mutex_t	ops_lock;

/* protects ``port'' static in bindresvport() */
mutex_t	portnum_lock;

/* protects proglst list (svc_simple.c) */
mutex_t	proglst_lock;

/* serializes clnt_com_create() (rpc_soc.c) */
mutex_t	rpcsoc_lock;

/* svc_raw.c serialization */
mutex_t	svcraw_lock;

/* protects TSD key creation */
mutex_t	tsd_lock;

/* Library global tsd keys */
thread_key_t clnt_broadcast_key;
thread_key_t rpc_call_key = -1;
thread_key_t tcp_key = -1;
thread_key_t udp_key = -1;
thread_key_t nc_key = -1;
thread_key_t rce_key = -1;

/* xprtlist (svc_generic.c) */
mutex_t	xprtlist_lock;

/* serializes calls to public key routines */
mutex_t serialize_pkey;

/* netconfig serialization */
mutex_t nc_lock;

#ifdef _WIN32
/*
 * Initialize all the mutexes (CriticalSections)
 */
void multithread_init(void)
{
	InitializeCriticalSection(&authdes_lock);
	InitializeCriticalSection(&authdes_ops_lock);
	InitializeCriticalSection(&svcauthdesstats_lock);
	InitializeCriticalSection(&authnone_lock);
	InitializeCriticalSection(&authsvc_lock);
	InitializeCriticalSection(&clnt_fd_lock);
	InitializeCriticalSection(&clntraw_lock);
	InitializeCriticalSection(&dname_lock);
	InitializeCriticalSection(&dupreq_lock);
	InitializeCriticalSection(&keyserv_lock);
	InitializeCriticalSection(&libnsl_trace_lock);
	InitializeCriticalSection(&loopnconf_lock);
	InitializeCriticalSection(&ops_lock);
	InitializeCriticalSection(&portnum_lock);
	InitializeCriticalSection(&proglst_lock);
	InitializeCriticalSection(&rpcsoc_lock);
	InitializeCriticalSection(&svcraw_lock);
	InitializeCriticalSection(&tsd_lock);
	InitializeCriticalSection(&xprtlist_lock);
	InitializeCriticalSection(&serialize_pkey);
	InitializeCriticalSection(&nc_lock);
}
#endif

#undef	rpc_createerr

struct rpc_createerr rpc_createerr;

struct rpc_createerr *
__rpc_createerr()
{
	struct rpc_createerr *rce_addr;

	mutex_lock(&tsd_lock);
	if (rce_key == -1)
		rce_key = TlsAlloc();	//thr_keycreate(&rce_key, free);
	mutex_unlock(&tsd_lock);

	rce_addr = (struct rpc_createerr *)thr_getspecific(rce_key);
	if (!rce_addr) {
		rce_addr = (struct rpc_createerr *)
			malloc(sizeof (struct rpc_createerr));
		if (!rce_addr ||
		    thr_setspecific(rce_key, (void *) rce_addr) == 0) {
			if (rce_addr)
				free(rce_addr);
			return (&rpc_createerr);
		}
		memset(rce_addr, 0, sizeof (struct rpc_createerr));
	}
	return (rce_addr);
}

void tsd_key_delete(void)
{
	if (clnt_broadcast_key != -1)
		thr_keydelete(clnt_broadcast_key);
	if (rpc_call_key != -1)
		thr_keydelete(rpc_call_key);
	if (tcp_key != -1)
		thr_keydelete(tcp_key);
	if (udp_key != -1)
		thr_keydelete(udp_key);
	if (nc_key != -1)
		thr_keydelete(nc_key);
	if (rce_key != -1)
		thr_keydelete(rce_key);
	return;
}

