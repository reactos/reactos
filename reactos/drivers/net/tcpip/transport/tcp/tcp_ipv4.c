/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp_ipv4.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15-01-2003 Imported from linux kernel 2.4.20
 */

/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Implementation of the Transmission Control Protocol(TCP).
 *
 * Version:	$Id: tcp_ipv4.c,v 1.1 2003/01/15 21:57:31 chorns Exp $
 *
 *		IPv4 specific functions
 *
 *
 *		code split from:
 *		linux/ipv4/tcp.c
 *		linux/ipv4/tcp_input.c
 *		linux/ipv4/tcp_output.c
 *
 *		See tcp.c for author information
 *
 *	This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

/*
 * Changes:
 *		David S. Miller	:	New socket lookup architecture.
 *					This code is dedicated to John Dyson.
 *		David S. Miller :	Change semantics of established hash,
 *					half is devoted to TIME_WAIT sockets
 *					and the rest go in the other half.
 *		Andi Kleen :		Add support for syncookies and fixed
 *					some bugs: ip options weren't passed to
 *					the TCP layer, missed a check for an ACK bit.
 *		Andi Kleen :		Implemented fast path mtu discovery.
 *	     				Fixed many serious bugs in the
 *					open_request handling and moved
 *					most of it into the af independent code.
 *					Added tail drop and some other bugfixes.
 *					Added new listen sematics.
 *		Mike McLagan	:	Routing by source
 *	Juan Jose Ciarlante:		ip_dynaddr bits
 *		Andi Kleen:		various fixes.
 *	Vitaly E. Lavrov	:	Transparent proxy revived after year coma.
 *	Andi Kleen		:	Fix new listen.
 *	Andi Kleen		:	Fix accept error reporting.
 */

#if 0
#include <linux/config.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/random.h>
#include <linux/cache.h>
#include <linux/init.h>

#include <net/icmp.h>
#include <net/tcp.h>
#include <net/ipv6.h>
#include <net/inet_common.h>

#include <linux/inet.h>
#include <linux/stddef.h>
#include <linux/ipsec.h>
#else
#include "linux.h"
#include "tcpcore.h"
#endif

extern int sysctl_ip_dynaddr;
extern int sysctl_ip_default_ttl;
int sysctl_tcp_tw_reuse = 0;

/* Check TCP sequence numbers in ICMP packets. */
#define ICMP_MIN_LENGTH 8

/* Socket used for sending RSTs */ 	
#if 0
static struct inode tcp_inode;
static struct socket *tcp_socket=&tcp_inode.u.socket_i;
#endif

void tcp_v4_send_check(struct sock *sk, struct tcphdr *th, int len, 
		       struct sk_buff *skb);

/*
 * ALL members must be initialised to prevent gcc-2.7.2.3 miscompilation
 */
#if 0
struct tcp_hashinfo __cacheline_aligned tcp_hashinfo = {
	__tcp_ehash:          NULL,
	__tcp_bhash:          NULL,
	__tcp_bhash_size:     0,
	__tcp_ehash_size:     0,
	__tcp_listening_hash: { NULL, },
	__tcp_lhash_lock:     RW_LOCK_UNLOCKED,
	__tcp_lhash_users:    ATOMIC_INIT(0),
	__tcp_lhash_wait:
	  __WAIT_QUEUE_HEAD_INITIALIZER(tcp_hashinfo.__tcp_lhash_wait),
	__tcp_portalloc_lock: SPIN_LOCK_UNLOCKED
};
#endif

/*
 * This array holds the first and last local port number.
 * For high-usage systems, use sysctl to change this to
 * 32768-61000
 */
int sysctl_local_port_range[2] = { 1024, 4999 };
int tcp_port_rover = (1024 - 1);

static __inline__ int tcp_hashfn(__u32 laddr, __u16 lport,
				 __u32 faddr, __u16 fport)
{
	int h = ((laddr ^ lport) ^ (faddr ^ fport));
	h ^= h>>16;
	h ^= h>>8;
	return h & (tcp_ehash_size - 1);
}

static __inline__ int tcp_sk_hashfn(struct sock *sk)
{
	__u32 laddr = sk->rcv_saddr;
	__u16 lport = sk->num;
	__u32 faddr = sk->daddr;
	__u16 fport = sk->dport;

	return tcp_hashfn(laddr, lport, faddr, fport);
}

/* Allocate and initialize a new TCP local port bind bucket.
 * The bindhash mutex for snum's hash chain must be held here.
 */
struct tcp_bind_bucket *tcp_bucket_create(struct tcp_bind_hashbucket *head,
					  unsigned short snum)
{
#if 0
	struct tcp_bind_bucket *tb;

	tb = kmem_cache_alloc(tcp_bucket_cachep, SLAB_ATOMIC);
	if(tb != NULL) {
		tb->port = snum;
		tb->fastreuse = 0;
		tb->owners = NULL;
		if((tb->next = head->chain) != NULL)
			tb->next->pprev = &tb->next;
		head->chain = tb;
		tb->pprev = &head->chain;
	}
	return tb;
#else
  return NULL;
#endif
}

/* Caller must disable local BH processing. */
static __inline__ void __tcp_inherit_port(struct sock *sk, struct sock *child)
{
#if 0
	struct tcp_bind_hashbucket *head = &tcp_bhash[tcp_bhashfn(child->num)];
	struct tcp_bind_bucket *tb;

	spin_lock(&head->lock);
	tb = (struct tcp_bind_bucket *)sk->prev;
	if ((child->bind_next = tb->owners) != NULL)
		tb->owners->bind_pprev = &child->bind_next;
	tb->owners = child;
	child->bind_pprev = &tb->owners;
	child->prev = (struct sock *) tb;
	spin_unlock(&head->lock);
#endif
}

__inline__ void tcp_inherit_port(struct sock *sk, struct sock *child)
{
#if 0
	local_bh_disable();
	__tcp_inherit_port(sk, child);
	local_bh_enable();
#endif
}

static inline void tcp_bind_hash(struct sock *sk, struct tcp_bind_bucket *tb, unsigned short snum)
{
#if 0
	sk->num = snum;
	if ((sk->bind_next = tb->owners) != NULL)
		tb->owners->bind_pprev = &sk->bind_next;
	tb->owners = sk;
	sk->bind_pprev = &tb->owners;
	sk->prev = (struct sock *) tb;
#endif
}

static inline int tcp_bind_conflict(struct sock *sk, struct tcp_bind_bucket *tb)
{
#if 0
	struct sock *sk2 = tb->owners;
	int sk_reuse = sk->reuse;
	
	for( ; sk2 != NULL; sk2 = sk2->bind_next) {
		if (sk != sk2 &&
		    sk2->reuse <= 1 &&
		    sk->bound_dev_if == sk2->bound_dev_if) {
			if (!sk_reuse	||
			    !sk2->reuse	||
			    sk2->state == TCP_LISTEN) {
				if (!sk2->rcv_saddr	||
				    !sk->rcv_saddr	||
				    (sk2->rcv_saddr == sk->rcv_saddr))
					break;
			}
		}
	}
	return sk2 != NULL;
#else
  return 0;
#endif
}

/* Obtain a reference to a local port for the given sock,
 * if snum is zero it means select any available local port.
 */
static int tcp_v4_get_port(struct sock *sk, unsigned short snum)
{
#if 0
	struct tcp_bind_hashbucket *head;
	struct tcp_bind_bucket *tb;
	int ret;

	local_bh_disable();
	if (snum == 0) {
		int low = sysctl_local_port_range[0];
		int high = sysctl_local_port_range[1];
		int remaining = (high - low) + 1;
		int rover;

		spin_lock(&tcp_portalloc_lock);
		rover = tcp_port_rover;
		do {	rover++;
			if ((rover < low) || (rover > high))
				rover = low;
			head = &tcp_bhash[tcp_bhashfn(rover)];
			spin_lock(&head->lock);
			for (tb = head->chain; tb; tb = tb->next)
				if (tb->port == rover)
					goto next;
			break;
		next:
			spin_unlock(&head->lock);
		} while (--remaining > 0);
		tcp_port_rover = rover;
		spin_unlock(&tcp_portalloc_lock);

		/* Exhausted local port range during search? */
		ret = 1;
		if (remaining <= 0)
			goto fail;

		/* OK, here is the one we will use.  HEAD is
		 * non-NULL and we hold it's mutex.
		 */
		snum = rover;
		tb = NULL;
	} else {
		head = &tcp_bhash[tcp_bhashfn(snum)];
		spin_lock(&head->lock);
		for (tb = head->chain; tb != NULL; tb = tb->next)
			if (tb->port == snum)
				break;
	}
	if (tb != NULL && tb->owners != NULL) {
		if (sk->reuse > 1)
			goto success;
		if (tb->fastreuse > 0 && sk->reuse != 0 && sk->state != TCP_LISTEN) {
			goto success;
		} else {
			ret = 1;
			if (tcp_bind_conflict(sk, tb))
				goto fail_unlock;
		}
	}
	ret = 1;
	if (tb == NULL &&
	    (tb = tcp_bucket_create(head, snum)) == NULL)
			goto fail_unlock;
	if (tb->owners == NULL) {
		if (sk->reuse && sk->state != TCP_LISTEN)
			tb->fastreuse = 1;
		else
			tb->fastreuse = 0;
	} else if (tb->fastreuse &&
		   ((sk->reuse == 0) || (sk->state == TCP_LISTEN)))
		tb->fastreuse = 0;
success:
	if (sk->prev == NULL)
		tcp_bind_hash(sk, tb, snum);
	BUG_TRAP(sk->prev == (struct sock *) tb);
 	ret = 0;

fail_unlock:
	spin_unlock(&head->lock);
fail:
	local_bh_enable();
	return ret;
#else
  return 0;
#endif
}

/* Get rid of any references to a local port held by the
 * given sock.
 */
__inline__ void __tcp_put_port(struct sock *sk)
{
#if 0
	struct tcp_bind_hashbucket *head = &tcp_bhash[tcp_bhashfn(sk->num)];
	struct tcp_bind_bucket *tb;

	spin_lock(&head->lock);
	tb = (struct tcp_bind_bucket *) sk->prev;
	if (sk->bind_next)
		sk->bind_next->bind_pprev = sk->bind_pprev;
	*(sk->bind_pprev) = sk->bind_next;
	sk->prev = NULL;
	sk->num = 0;
	if (tb->owners == NULL) {
		if (tb->next)
			tb->next->pprev = tb->pprev;
		*(tb->pprev) = tb->next;
		kmem_cache_free(tcp_bucket_cachep, tb);
	}
	spin_unlock(&head->lock);
#endif
}

void tcp_put_port(struct sock *sk)
{
#if 0
	local_bh_disable();
	__tcp_put_port(sk);
	local_bh_enable();
#endif
}

/* This lock without WQ_FLAG_EXCLUSIVE is good on UP and it can be very bad on SMP.
 * Look, when several writers sleep and reader wakes them up, all but one
 * immediately hit write lock and grab all the cpus. Exclusive sleep solves
 * this, _but_ remember, it adds useless work on UP machines (wake up each
 * exclusive lock release). It should be ifdefed really.
 */

void tcp_listen_wlock(void)
{
#if 0
	write_lock(&tcp_lhash_lock);

	if (atomic_read(&tcp_lhash_users)) {
		DECLARE_WAITQUEUE(wait, current);

		add_wait_queue_exclusive(&tcp_lhash_wait, &wait);
		for (;;) {
			set_current_state(TASK_UNINTERRUPTIBLE);
			if (atomic_read(&tcp_lhash_users) == 0)
				break;
			write_unlock_bh(&tcp_lhash_lock);
			schedule();
			write_lock_bh(&tcp_lhash_lock);
		}

		__set_current_state(TASK_RUNNING);
		remove_wait_queue(&tcp_lhash_wait, &wait);
	}
#endif
}

static __inline__ void __tcp_v4_hash(struct sock *sk, const int listen_possible)
{
#if 0
	struct sock **skp;
	rwlock_t *lock;

	BUG_TRAP(sk->pprev==NULL);
	if(listen_possible && sk->state == TCP_LISTEN) {
		skp = &tcp_listening_hash[tcp_sk_listen_hashfn(sk)];
		lock = &tcp_lhash_lock;
		tcp_listen_wlock();
	} else {
		skp = &tcp_ehash[(sk->hashent = tcp_sk_hashfn(sk))].chain;
		lock = &tcp_ehash[sk->hashent].lock;
		write_lock(lock);
	}
	if((sk->next = *skp) != NULL)
		(*skp)->pprev = &sk->next;
	*skp = sk;
	sk->pprev = skp;
	sock_prot_inc_use(sk->prot);
	write_unlock(lock);
	if (listen_possible && sk->state == TCP_LISTEN)
		wake_up(&tcp_lhash_wait);
#endif
}

static void tcp_v4_hash(struct sock *sk)
{
#if 0
	if (sk->state != TCP_CLOSE) {
		local_bh_disable();
		__tcp_v4_hash(sk, 1);
		local_bh_enable();
	}
#endif
}

void tcp_unhash(struct sock *sk)
{
#if 0
	rwlock_t *lock;

	if (!sk->pprev)
		goto ende;

	if (sk->state == TCP_LISTEN) {
		local_bh_disable();
		tcp_listen_wlock();
		lock = &tcp_lhash_lock;
	} else {
		struct tcp_ehash_bucket *head = &tcp_ehash[sk->hashent];
		lock = &head->lock;
		write_lock_bh(&head->lock);
	}

	if(sk->pprev) {
		if(sk->next)
			sk->next->pprev = sk->pprev;
		*sk->pprev = sk->next;
		sk->pprev = NULL;
		sock_prot_dec_use(sk->prot);
	}
	write_unlock_bh(lock);

 ende:
	if (sk->state == TCP_LISTEN)
		wake_up(&tcp_lhash_wait);
#endif
}

/* Don't inline this cruft.  Here are some nice properties to
 * exploit here.  The BSD API does not allow a listening TCP
 * to specify the remote port nor the remote address for the
 * connection.  So always assume those are both wildcarded
 * during the search since they can never be otherwise.
 */
static struct sock *__tcp_v4_lookup_listener(struct sock *sk, u32 daddr, unsigned short hnum, int dif)
{
#if 0
	struct sock *result = NULL;
	int score, hiscore;

	hiscore=0;
	for(; sk; sk = sk->next) {
		if(sk->num == hnum) {
			__u32 rcv_saddr = sk->rcv_saddr;

			score = 1;
			if(rcv_saddr) {
				if (rcv_saddr != daddr)
					continue;
				score++;
			}
			if (sk->bound_dev_if) {
				if (sk->bound_dev_if != dif)
					continue;
				score++;
			}
			if (score == 3)
				return sk;
			if (score > hiscore) {
				hiscore = score;
				result = sk;
			}
		}
	}
	return result;
#else
  return NULL;
#endif
}

/* Optimize the common listener case. */
__inline__ struct sock *tcp_v4_lookup_listener(u32 daddr, unsigned short hnum, int dif)
{
#if 0
	struct sock *sk;

	read_lock(&tcp_lhash_lock);
	sk = tcp_listening_hash[tcp_lhashfn(hnum)];
	if (sk) {
		if (sk->num == hnum &&
		    sk->next == NULL &&
		    (!sk->rcv_saddr || sk->rcv_saddr == daddr) &&
		    !sk->bound_dev_if)
			goto sherry_cache;
		sk = __tcp_v4_lookup_listener(sk, daddr, hnum, dif);
	}
	if (sk) {
sherry_cache:
		sock_hold(sk);
	}
	read_unlock(&tcp_lhash_lock);
	return sk;
#else
  return NULL;
#endif
}

/* Sockets in TCP_CLOSE state are _always_ taken out of the hash, so
 * we need not check it for TCP lookups anymore, thanks Alexey. -DaveM
 *
 * Local BH must be disabled here.
 */

static inline struct sock *__tcp_v4_lookup_established(u32 saddr, u16 sport,
						       u32 daddr, u16 hnum, int dif)
{
#if 0
	struct tcp_ehash_bucket *head;
	TCP_V4_ADDR_COOKIE(acookie, saddr, daddr)
	__u32 ports = TCP_COMBINED_PORTS(sport, hnum);
	struct sock *sk;
	int hash;

	/* Optimize here for direct hit, only listening connections can
	 * have wildcards anyways.
	 */
	hash = tcp_hashfn(daddr, hnum, saddr, sport);
	head = &tcp_ehash[hash];
	read_lock(&head->lock);
	for(sk = head->chain; sk; sk = sk->next) {
		if(TCP_IPV4_MATCH(sk, acookie, saddr, daddr, ports, dif))
			goto hit; /* You sunk my battleship! */
	}

	/* Must check for a TIME_WAIT'er before going to listener hash. */
	for(sk = (head + tcp_ehash_size)->chain; sk; sk = sk->next)
		if(TCP_IPV4_MATCH(sk, acookie, saddr, daddr, ports, dif))
			goto hit;
	read_unlock(&head->lock);

	return NULL;

hit:
	sock_hold(sk);
	read_unlock(&head->lock);
	return sk;
#else
  return NULL;
#endif
}

static inline struct sock *__tcp_v4_lookup(u32 saddr, u16 sport,
					   u32 daddr, u16 hnum, int dif)
{
#if 0
	struct sock *sk;

	sk = __tcp_v4_lookup_established(saddr, sport, daddr, hnum, dif);

	if (sk)
		return sk;
		
	return tcp_v4_lookup_listener(daddr, hnum, dif);
#else
  return NULL;
#endif
}

__inline__ struct sock *tcp_v4_lookup(u32 saddr, u16 sport, u32 daddr, u16 dport, int dif)
{
#if 0
	struct sock *sk;

	local_bh_disable();
	sk = __tcp_v4_lookup(saddr, sport, daddr, ntohs(dport), dif);
	local_bh_enable();

	return sk;
#else
  return NULL;
#endif
}

static inline __u32 tcp_v4_init_sequence(struct sock *sk, struct sk_buff *skb)
{
#if 0
	return secure_tcp_sequence_number(skb->nh.iph->daddr,
					  skb->nh.iph->saddr,
					  skb->h.th->dest,
					  skb->h.th->source);
#else
  return 0;
#endif
}

/* called with local bh disabled */
static int __tcp_v4_check_established(struct sock *sk, __u16 lport,
				      struct tcp_tw_bucket **twp)
{
#if 0
	u32 daddr = sk->rcv_saddr;
	u32 saddr = sk->daddr;
	int dif = sk->bound_dev_if;
	TCP_V4_ADDR_COOKIE(acookie, saddr, daddr)
	__u32 ports = TCP_COMBINED_PORTS(sk->dport, lport);
	int hash = tcp_hashfn(daddr, lport, saddr, sk->dport);
	struct tcp_ehash_bucket *head = &tcp_ehash[hash];
	struct sock *sk2, **skp;
	struct tcp_tw_bucket *tw;

	write_lock(&head->lock);

	/* Check TIME-WAIT sockets first. */
	for(skp = &(head + tcp_ehash_size)->chain; (sk2=*skp) != NULL;
	    skp = &sk2->next) {
		tw = (struct tcp_tw_bucket*)sk2;

		if(TCP_IPV4_MATCH(sk2, acookie, saddr, daddr, ports, dif)) {
			struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);

			/* With PAWS, it is safe from the viewpoint
			   of data integrity. Even without PAWS it
			   is safe provided sequence spaces do not
			   overlap i.e. at data rates <= 80Mbit/sec.

			   Actually, the idea is close to VJ's one,
			   only timestamp cache is held not per host,
			   but per port pair and TW bucket is used
			   as state holder.

			   If TW bucket has been already destroyed we
			   fall back to VJ's scheme and use initial
			   timestamp retrieved from peer table.
			 */
			if (tw->ts_recent_stamp &&
			    (!twp || (sysctl_tcp_tw_reuse &&
				      xtime.tv_sec - tw->ts_recent_stamp > 1))) {
				if ((tp->write_seq = tw->snd_nxt+65535+2) == 0)
					tp->write_seq = 1;
				tp->ts_recent = tw->ts_recent;
				tp->ts_recent_stamp = tw->ts_recent_stamp;
				sock_hold(sk2);
				skp = &head->chain;
				goto unique;
			} else
				goto not_unique;
		}
	}
	tw = NULL;

	/* And established part... */
	for(skp = &head->chain; (sk2=*skp)!=NULL; skp = &sk2->next) {
		if(TCP_IPV4_MATCH(sk2, acookie, saddr, daddr, ports, dif))
			goto not_unique;
	}

unique:
	/* Must record num and sport now. Otherwise we will see
	 * in hash table socket with a funny identity. */
	sk->num = lport;
	sk->sport = htons(lport);
	BUG_TRAP(sk->pprev==NULL);
	if ((sk->next = *skp) != NULL)
		(*skp)->pprev = &sk->next;

	*skp = sk;
	sk->pprev = skp;
	sk->hashent = hash;
	sock_prot_inc_use(sk->prot);
	write_unlock(&head->lock);

	if (twp) {
		*twp = tw;
		NET_INC_STATS_BH(TimeWaitRecycled);
	} else if (tw) {
		/* Silly. Should hash-dance instead... */
		tcp_tw_deschedule(tw);
		tcp_timewait_kill(tw);
		NET_INC_STATS_BH(TimeWaitRecycled);

		tcp_tw_put(tw);
	}

	return 0;

not_unique:
	write_unlock(&head->lock);
	return -EADDRNOTAVAIL;
#else
  return 0;
#endif
}

/*
 * Bind a port for a connect operation and hash it.
 */
static int tcp_v4_hash_connect(struct sock *sk)
{
#if 0
	unsigned short snum = sk->num;
	struct tcp_bind_hashbucket *head;
	struct tcp_bind_bucket *tb;

	if (snum == 0) {
		int rover;
		int low = sysctl_local_port_range[0];
		int high = sysctl_local_port_range[1];
		int remaining = (high - low) + 1;
		struct tcp_tw_bucket *tw = NULL;

		local_bh_disable();

		/* TODO. Actually it is not so bad idea to remove
		 * tcp_portalloc_lock before next submission to Linus.
		 * As soon as we touch this place at all it is time to think.
		 *
		 * Now it protects single _advisory_ variable tcp_port_rover,
		 * hence it is mostly useless.
		 * Code will work nicely if we just delete it, but
		 * I am afraid in contented case it will work not better or
		 * even worse: another cpu just will hit the same bucket
		 * and spin there.
		 * So some cpu salt could remove both contention and
		 * memory pingpong. Any ideas how to do this in a nice way?
		 */
		spin_lock(&tcp_portalloc_lock);
		rover = tcp_port_rover;

		do {
			rover++;
			if ((rover < low) || (rover > high))
				rover = low;
			head = &tcp_bhash[tcp_bhashfn(rover)];
			spin_lock(&head->lock);		

			/* Does not bother with rcv_saddr checks,
			 * because the established check is already
			 * unique enough.
			 */
			for (tb = head->chain; tb; tb = tb->next) {
				if (tb->port == rover) {
					BUG_TRAP(tb->owners != NULL);
					if (tb->fastreuse >= 0)
						goto next_port;
					if (!__tcp_v4_check_established(sk, rover, &tw))
						goto ok;
					goto next_port;
				}
			}

			tb = tcp_bucket_create(head, rover);
			if (!tb) {
				spin_unlock(&head->lock);
				break;
			}
			tb->fastreuse = -1;
			goto ok;

		next_port:
			spin_unlock(&head->lock);
		} while (--remaining > 0);
		tcp_port_rover = rover;
		spin_unlock(&tcp_portalloc_lock);

		local_bh_enable();

		return -EADDRNOTAVAIL;

	ok:
		/* All locks still held and bhs disabled */
		tcp_port_rover = rover;
		spin_unlock(&tcp_portalloc_lock);

		tcp_bind_hash(sk, tb, rover);
		if (!sk->pprev) {
			sk->sport = htons(rover);
			__tcp_v4_hash(sk, 0);
		}
		spin_unlock(&head->lock);

		if (tw) {
			tcp_tw_deschedule(tw);
			tcp_timewait_kill(tw);
			tcp_tw_put(tw);
		}

		local_bh_enable();
		return 0;
	}

	head  = &tcp_bhash[tcp_bhashfn(snum)];
	tb  = (struct tcp_bind_bucket *)sk->prev;
	spin_lock_bh(&head->lock);
	if (tb->owners == sk && sk->bind_next == NULL) {
		__tcp_v4_hash(sk, 0);
		spin_unlock_bh(&head->lock);
		return 0;
	} else {
		int ret;
		spin_unlock(&head->lock);
		/* No definite answer... Walk to established hash table */
		ret = __tcp_v4_check_established(sk, snum, NULL);
		local_bh_enable();
		return ret;
	}
#else
  return 0;
#endif
}

/* This will initiate an outgoing connection. */
int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
{
#if 0
	struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);
	struct sockaddr_in *usin = (struct sockaddr_in *) uaddr;
	struct rtable *rt;
	u32 daddr, nexthop;
	int tmp;
	int err;

	if (addr_len < sizeof(struct sockaddr_in))
		return(-EINVAL);

	if (usin->sin_family != AF_INET)
		return(-EAFNOSUPPORT);

	nexthop = daddr = usin->sin_addr.s_addr;
	if (sk->protinfo.af_inet.opt && sk->protinfo.af_inet.opt->srr) {
		if (daddr == 0)
			return -EINVAL;
		nexthop = sk->protinfo.af_inet.opt->faddr;
	}

	tmp = ip_route_connect(&rt, nexthop, sk->saddr,
			       RT_CONN_FLAGS(sk), sk->bound_dev_if);
	if (tmp < 0)
		return tmp;

	if (rt->rt_flags&(RTCF_MULTICAST|RTCF_BROADCAST)) {
		ip_rt_put(rt);
		return -ENETUNREACH;
	}

	__sk_dst_set(sk, &rt->u.dst);
	sk->route_caps = rt->u.dst.dev->features;

	if (!sk->protinfo.af_inet.opt || !sk->protinfo.af_inet.opt->srr)
		daddr = rt->rt_dst;

	if (!sk->saddr)
		sk->saddr = rt->rt_src;
	sk->rcv_saddr = sk->saddr;

	if (tp->ts_recent_stamp && sk->daddr != daddr) {
		/* Reset inherited state */
		tp->ts_recent = 0;
		tp->ts_recent_stamp = 0;
		tp->write_seq = 0;
	}

	if (sysctl_tcp_tw_recycle &&
	    !tp->ts_recent_stamp &&
	    rt->rt_dst == daddr) {
		struct inet_peer *peer = rt_get_peer(rt);

		/* VJ's idea. We save last timestamp seen from
		 * the destination in peer table, when entering state TIME-WAIT
		 * and initialize ts_recent from it, when trying new connection.
		 */

		if (peer && peer->tcp_ts_stamp + TCP_PAWS_MSL >= xtime.tv_sec) {
			tp->ts_recent_stamp = peer->tcp_ts_stamp;
			tp->ts_recent = peer->tcp_ts;
		}
	}

	sk->dport = usin->sin_port;
	sk->daddr = daddr;

	tp->ext_header_len = 0;
	if (sk->protinfo.af_inet.opt)
		tp->ext_header_len = sk->protinfo.af_inet.opt->optlen;

	tp->mss_clamp = 536;

	/* Socket identity is still unknown (sport may be zero).
	 * However we set state to SYN-SENT and not releasing socket
	 * lock select source port, enter ourselves into the hash tables and
	 * complete initalization after this.
	 */
	tcp_set_state(sk, TCP_SYN_SENT);
	err = tcp_v4_hash_connect(sk);
	if (err)
		goto failure;

	if (!tp->write_seq)
		tp->write_seq = secure_tcp_sequence_number(sk->saddr, sk->daddr,
							   sk->sport, usin->sin_port);

	sk->protinfo.af_inet.id = tp->write_seq^jiffies;

	err = tcp_connect(sk);
	if (err)
		goto failure;

	return 0;

failure:
	tcp_set_state(sk, TCP_CLOSE);
	__sk_dst_reset(sk);
	sk->route_caps = 0;
	sk->dport = 0;
	return err;
#else
  return 0;
#endif
}

static __inline__ int tcp_v4_iif(struct sk_buff *skb)
{
#if 0
	return ((struct rtable*)skb->dst)->rt_iif;
#else
  return 0;
#endif
}

static __inline__ unsigned tcp_v4_synq_hash(u32 raddr, u16 rport)
{
#if 0
	unsigned h = raddr ^ rport;
	h ^= h>>16;
	h ^= h>>8;
	return h&(TCP_SYNQ_HSIZE-1);
#else
  return 0;
#endif
}

static struct open_request *tcp_v4_search_req(struct tcp_opt *tp, 
					      struct open_request ***prevp,
					      __u16 rport,
					      __u32 raddr, __u32 laddr)
{
#if 0
	struct tcp_listen_opt *lopt = tp->listen_opt;
	struct open_request *req, **prev;  

	for (prev = &lopt->syn_table[tcp_v4_synq_hash(raddr, rport)];
	     (req = *prev) != NULL;
	     prev = &req->dl_next) {
		if (req->rmt_port == rport &&
		    req->af.v4_req.rmt_addr == raddr &&
		    req->af.v4_req.loc_addr == laddr &&
		    TCP_INET_FAMILY(req->class->family)) {
			BUG_TRAP(req->sk == NULL);
			*prevp = prev;
			return req; 
		}
	}

	return NULL;
#else
  return NULL;
#endif
}

static void tcp_v4_synq_add(struct sock *sk, struct open_request *req)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;
	struct tcp_listen_opt *lopt = tp->listen_opt;
	unsigned h = tcp_v4_synq_hash(req->af.v4_req.rmt_addr, req->rmt_port);

	req->expires = jiffies + TCP_TIMEOUT_INIT;
	req->retrans = 0;
	req->sk = NULL;
	req->dl_next = lopt->syn_table[h];

	write_lock(&tp->syn_wait_lock);
	lopt->syn_table[h] = req;
	write_unlock(&tp->syn_wait_lock);

	tcp_synq_added(sk);
#endif
}


/* 
 * This routine does path mtu discovery as defined in RFC1191.
 */
static inline void do_pmtu_discovery(struct sock *sk, struct iphdr *ip, unsigned mtu)
{
#if 0
	struct dst_entry *dst;
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;

	/* We are not interested in TCP_LISTEN and open_requests (SYN-ACKs
	 * send out by Linux are always <576bytes so they should go through
	 * unfragmented).
	 */
	if (sk->state == TCP_LISTEN)
		return; 

	/* We don't check in the destentry if pmtu discovery is forbidden
	 * on this route. We just assume that no packet_to_big packets
	 * are send back when pmtu discovery is not active.
     	 * There is a small race when the user changes this flag in the
	 * route, but I think that's acceptable.
	 */
	if ((dst = __sk_dst_check(sk, 0)) == NULL)
		return;

	ip_rt_update_pmtu(dst, mtu);

	/* Something is about to be wrong... Remember soft error
	 * for the case, if this connection will not able to recover.
	 */
	if (mtu < dst->pmtu && ip_dont_fragment(sk, dst))
		sk->err_soft = EMSGSIZE;

	if (sk->protinfo.af_inet.pmtudisc != IP_PMTUDISC_DONT &&
	    tp->pmtu_cookie > dst->pmtu) {
		tcp_sync_mss(sk, dst->pmtu);

		/* Resend the TCP packet because it's  
		 * clear that the old packet has been
		 * dropped. This is the new "fast" path mtu
		 * discovery.
		 */
		tcp_simple_retransmit(sk);
	} /* else let the usual retransmit timer handle it */
#endif
}

/*
 * This routine is called by the ICMP module when it gets some
 * sort of error condition.  If err < 0 then the socket should
 * be closed and the error returned to the user.  If err > 0
 * it's just the icmp type << 8 | icmp code.  After adjustment
 * header points to the first 8 bytes of the tcp header.  We need
 * to find the appropriate port.
 *
 * The locking strategy used here is very "optimistic". When
 * someone else accesses the socket the ICMP is just dropped
 * and for some paths there is no check at all.
 * A more general error queue to queue errors for later handling
 * is probably better.
 *
 */

void tcp_v4_err(struct sk_buff *skb, u32 info)
{
#if 0
	struct iphdr *iph = (struct iphdr*)skb->data;
	struct tcphdr *th = (struct tcphdr*)(skb->data+(iph->ihl<<2));
	struct tcp_opt *tp;
	int type = skb->h.icmph->type;
	int code = skb->h.icmph->code;
	struct sock *sk;
	__u32 seq;
	int err;

	if (skb->len < (iph->ihl << 2) + 8) {
		ICMP_INC_STATS_BH(IcmpInErrors); 
		return;
	}

	sk = tcp_v4_lookup(iph->daddr, th->dest, iph->saddr, th->source, tcp_v4_iif(skb));
	if (sk == NULL) {
		ICMP_INC_STATS_BH(IcmpInErrors);
		return;
	}
	if (sk->state == TCP_TIME_WAIT) {
		tcp_tw_put((struct tcp_tw_bucket*)sk);
		return;
	}

	bh_lock_sock(sk);
	/* If too many ICMPs get dropped on busy
	 * servers this needs to be solved differently.
	 */
	if (sk->lock.users != 0)
		NET_INC_STATS_BH(LockDroppedIcmps);

	if (sk->state == TCP_CLOSE)
		goto out;

	tp = &sk->tp_pinfo.af_tcp;
	seq = ntohl(th->seq);
	if (sk->state != TCP_LISTEN && !between(seq, tp->snd_una, tp->snd_nxt)) {
		NET_INC_STATS(OutOfWindowIcmps);
		goto out;
	}

	switch (type) {
	case ICMP_SOURCE_QUENCH:
		/* This is deprecated, but if someone generated it,
		 * we have no reasons to ignore it.
		 */
		if (sk->lock.users == 0)
			tcp_enter_cwr(tp);
		goto out;
	case ICMP_PARAMETERPROB:
		err = EPROTO;
		break; 
	case ICMP_DEST_UNREACH:
		if (code > NR_ICMP_UNREACH)
			goto out;

		if (code == ICMP_FRAG_NEEDED) { /* PMTU discovery (RFC1191) */
			if (sk->lock.users == 0)
				do_pmtu_discovery(sk, iph, info);
			goto out;
		}

		err = icmp_err_convert[code].errno;
		break;
	case ICMP_TIME_EXCEEDED:
		err = EHOSTUNREACH;
		break;
	default:
		goto out;
	}

	switch (sk->state) {
		struct open_request *req, **prev;
	case TCP_LISTEN:
		if (sk->lock.users != 0)
			goto out;

		req = tcp_v4_search_req(tp, &prev,
					th->dest,
					iph->daddr, iph->saddr); 
		if (!req)
			goto out;

		/* ICMPs are not backlogged, hence we cannot get
		   an established socket here.
		 */
		BUG_TRAP(req->sk == NULL);

		if (seq != req->snt_isn) {
			NET_INC_STATS_BH(OutOfWindowIcmps);
			goto out;
		}

		/* 
		 * Still in SYN_RECV, just remove it silently.
		 * There is no good way to pass the error to the newly
		 * created socket, and POSIX does not want network
		 * errors returned from accept(). 
		 */ 
		tcp_synq_drop(sk, req, prev);
		goto out;

	case TCP_SYN_SENT:
	case TCP_SYN_RECV:  /* Cannot happen.
			       It can f.e. if SYNs crossed.
			     */ 
		if (sk->lock.users == 0) {
			TCP_INC_STATS_BH(TcpAttemptFails);
			sk->err = err;

			sk->error_report(sk);

			tcp_done(sk);
		} else {
			sk->err_soft = err;
		}
		goto out;
	}

	/* If we've already connected we will keep trying
	 * until we time out, or the user gives up.
	 *
	 * rfc1122 4.2.3.9 allows to consider as hard errors
	 * only PROTO_UNREACH and PORT_UNREACH (well, FRAG_FAILED too,
	 * but it is obsoleted by pmtu discovery).
	 *
	 * Note, that in modern internet, where routing is unreliable
	 * and in each dark corner broken firewalls sit, sending random
	 * errors ordered by their masters even this two messages finally lose
	 * their original sense (even Linux sends invalid PORT_UNREACHs)
	 *
	 * Now we are in compliance with RFCs.
	 *							--ANK (980905)
	 */

	if (sk->lock.users == 0 && sk->protinfo.af_inet.recverr) {
		sk->err = err;
		sk->error_report(sk);
	} else	{ /* Only an error on timeout */
		sk->err_soft = err;
	}

out:
	bh_unlock_sock(sk);
	sock_put(sk);
#endif
}

/* This routine computes an IPv4 TCP checksum. */
void tcp_v4_send_check(struct sock *sk, struct tcphdr *th, int len, 
		       struct sk_buff *skb)
{
#if 0
	if (skb->ip_summed == CHECKSUM_HW) {
		th->check = ~tcp_v4_check(th, len, sk->saddr, sk->daddr, 0);
		skb->csum = offsetof(struct tcphdr, check);
	} else {
		th->check = tcp_v4_check(th, len, sk->saddr, sk->daddr,
					 csum_partial((char *)th, th->doff<<2, skb->csum));
	}
#endif
}

/*
 *	This routine will send an RST to the other tcp.
 *
 *	Someone asks: why I NEVER use socket parameters (TOS, TTL etc.)
 *		      for reset.
 *	Answer: if a packet caused RST, it is not for a socket
 *		existing in our system, if it is matched to a socket,
 *		it is just duplicate segment or bug in other side's TCP.
 *		So that we build reply only basing on parameters
 *		arrived with segment.
 *	Exception: precedence violation. We do not implement it in any case.
 */

static void tcp_v4_send_reset(struct sk_buff *skb)
{
#if 0
	struct tcphdr *th = skb->h.th;
	struct tcphdr rth;
	struct ip_reply_arg arg;

	/* Never send a reset in response to a reset. */
	if (th->rst)
		return;

	if (((struct rtable*)skb->dst)->rt_type != RTN_LOCAL)
		return;

	/* Swap the send and the receive. */
	memset(&rth, 0, sizeof(struct tcphdr)); 
	rth.dest = th->source;
	rth.source = th->dest; 
	rth.doff = sizeof(struct tcphdr)/4;
	rth.rst = 1;

	if (th->ack) {
		rth.seq = th->ack_seq;
	} else {
		rth.ack = 1;
		rth.ack_seq = htonl(ntohl(th->seq) + th->syn + th->fin
				    + skb->len - (th->doff<<2));
	}

	memset(&arg, 0, sizeof arg); 
	arg.iov[0].iov_base = (unsigned char *)&rth; 
	arg.iov[0].iov_len  = sizeof rth;
	arg.csum = csum_tcpudp_nofold(skb->nh.iph->daddr, 
				      skb->nh.iph->saddr, /*XXX*/
				      sizeof(struct tcphdr),
				      IPPROTO_TCP,
				      0); 
	arg.n_iov = 1;
	arg.csumoffset = offsetof(struct tcphdr, check) / 2; 

	tcp_socket->sk->protinfo.af_inet.ttl = sysctl_ip_default_ttl;
	ip_send_reply(tcp_socket->sk, skb, &arg, sizeof rth);

	TCP_INC_STATS_BH(TcpOutSegs);
	TCP_INC_STATS_BH(TcpOutRsts);
#endif
}

/* The code following below sending ACKs in SYN-RECV and TIME-WAIT states
   outside socket context is ugly, certainly. What can I do?
 */

static void tcp_v4_send_ack(struct sk_buff *skb, u32 seq, u32 ack, u32 win, u32 ts)
{
#if 0
	struct tcphdr *th = skb->h.th;
	struct {
		struct tcphdr th;
		u32 tsopt[3];
	} rep;
	struct ip_reply_arg arg;

	memset(&rep.th, 0, sizeof(struct tcphdr));
	memset(&arg, 0, sizeof arg);

	arg.iov[0].iov_base = (unsigned char *)&rep; 
	arg.iov[0].iov_len  = sizeof(rep.th);
	arg.n_iov = 1;
	if (ts) {
		rep.tsopt[0] = htonl((TCPOPT_NOP << 24) |
				     (TCPOPT_NOP << 16) |
				     (TCPOPT_TIMESTAMP << 8) |
				     TCPOLEN_TIMESTAMP);
		rep.tsopt[1] = htonl(tcp_time_stamp);
		rep.tsopt[2] = htonl(ts);
		arg.iov[0].iov_len = sizeof(rep);
	}

	/* Swap the send and the receive. */
	rep.th.dest = th->source;
	rep.th.source = th->dest; 
	rep.th.doff = arg.iov[0].iov_len/4;
	rep.th.seq = htonl(seq);
	rep.th.ack_seq = htonl(ack);
	rep.th.ack = 1;
	rep.th.window = htons(win);

	arg.csum = csum_tcpudp_nofold(skb->nh.iph->daddr, 
				      skb->nh.iph->saddr, /*XXX*/
				      arg.iov[0].iov_len,
				      IPPROTO_TCP,
				      0);
	arg.csumoffset = offsetof(struct tcphdr, check) / 2; 

	ip_send_reply(tcp_socket->sk, skb, &arg, arg.iov[0].iov_len);

	TCP_INC_STATS_BH(TcpOutSegs);
#endif
}

static void tcp_v4_timewait_ack(struct sock *sk, struct sk_buff *skb)
{
#if 0
	struct tcp_tw_bucket *tw = (struct tcp_tw_bucket *)sk;

	tcp_v4_send_ack(skb, tw->snd_nxt, tw->rcv_nxt,
			tw->rcv_wnd>>tw->rcv_wscale, tw->ts_recent);

	tcp_tw_put(tw);
#endif
}

static void tcp_v4_or_send_ack(struct sk_buff *skb, struct open_request *req)
{
#if 0
	tcp_v4_send_ack(skb, req->snt_isn+1, req->rcv_isn+1, req->rcv_wnd,
			req->ts_recent);
#endif
}

static struct dst_entry* tcp_v4_route_req(struct sock *sk, struct open_request *req)
{
#if 0
	struct rtable *rt;
	struct ip_options *opt;

	opt = req->af.v4_req.opt;
	if(ip_route_output(&rt, ((opt && opt->srr) ?
				 opt->faddr :
				 req->af.v4_req.rmt_addr),
			   req->af.v4_req.loc_addr,
			   RT_CONN_FLAGS(sk), sk->bound_dev_if)) {
		IP_INC_STATS_BH(IpOutNoRoutes);
		return NULL;
	}
	if (opt && opt->is_strictroute && rt->rt_dst != rt->rt_gateway) {
		ip_rt_put(rt);
		IP_INC_STATS_BH(IpOutNoRoutes);
		return NULL;
	}
	return &rt->u.dst;
#else
  return NULL;
#endif
}

/*
 *	Send a SYN-ACK after having received an ACK. 
 *	This still operates on a open_request only, not on a big
 *	socket.
 */ 
static int tcp_v4_send_synack(struct sock *sk, struct open_request *req,
			      struct dst_entry *dst)
{
#if 0
	int err = -1;
	struct sk_buff * skb;

	/* First, grab a route. */
	if (dst == NULL &&
	    (dst = tcp_v4_route_req(sk, req)) == NULL)
		goto out;

	skb = tcp_make_synack(sk, dst, req);

	if (skb) {
		struct tcphdr *th = skb->h.th;

		th->check = tcp_v4_check(th, skb->len,
					 req->af.v4_req.loc_addr, req->af.v4_req.rmt_addr,
					 csum_partial((char *)th, skb->len, skb->csum));

		err = ip_build_and_send_pkt(skb, sk, req->af.v4_req.loc_addr,
					    req->af.v4_req.rmt_addr, req->af.v4_req.opt);
		if (err == NET_XMIT_CN)
			err = 0;
	}

out:
	dst_release(dst);
	return err;
#else
  return 0;
#endif
}

/*
 *	IPv4 open_request destructor.
 */ 
static void tcp_v4_or_free(struct open_request *req)
{
#if 0
	if (req->af.v4_req.opt)
		kfree(req->af.v4_req.opt);
#endif
}

static inline void syn_flood_warning(struct sk_buff *skb)
{
#if 0
	static unsigned long warntime;
	
	if (jiffies - warntime > HZ*60) {
		warntime = jiffies;
		printk(KERN_INFO 
		       "possible SYN flooding on port %d. Sending cookies.\n",  
		       ntohs(skb->h.th->dest));
	}
#endif
}

/* 
 * Save and compile IPv4 options into the open_request if needed. 
 */
static inline struct ip_options * 
tcp_v4_save_options(struct sock *sk, struct sk_buff *skb)
{
#if 0
	struct ip_options *opt = &(IPCB(skb)->opt);
	struct ip_options *dopt = NULL; 

	if (opt && opt->optlen) {
		int opt_size = optlength(opt); 
		dopt = kmalloc(opt_size, GFP_ATOMIC);
		if (dopt) {
			if (ip_options_echo(dopt, skb)) {
				kfree(dopt);
				dopt = NULL;
			}
		}
	}
	return dopt;
#else
  return NULL;
#endif
}

/* 
 * Maximum number of SYN_RECV sockets in queue per LISTEN socket.
 * One SYN_RECV socket costs about 80bytes on a 32bit machine.
 * It would be better to replace it with a global counter for all sockets
 * but then some measure against one socket starving all other sockets
 * would be needed.
 *
 * It was 128 by default. Experiments with real servers show, that
 * it is absolutely not enough even at 100conn/sec. 256 cures most
 * of problems. This value is adjusted to 128 for very small machines
 * (<=32Mb of memory) and to 1024 on normal or better ones (>=256Mb).
 * Further increasing requires to change hash table size.
 */
int sysctl_max_syn_backlog = 256; 

#if 0
struct or_calltable or_ipv4 = {
	PF_INET,
	tcp_v4_send_synack,
	tcp_v4_or_send_ack,
	tcp_v4_or_free,
	tcp_v4_send_reset
};
#endif

int tcp_v4_conn_request(struct sock *sk, struct sk_buff *skb)
{
#if 0
	struct tcp_opt tp;
	struct open_request *req;
	__u32 saddr = skb->nh.iph->saddr;
	__u32 daddr = skb->nh.iph->daddr;
	__u32 isn = TCP_SKB_CB(skb)->when;
	struct dst_entry *dst = NULL;
#ifdef CONFIG_SYN_COOKIES
	int want_cookie = 0;
#else
#define want_cookie 0 /* Argh, why doesn't gcc optimize this :( */
#endif

	/* Never answer to SYNs send to broadcast or multicast */
	if (((struct rtable *)skb->dst)->rt_flags & 
	    (RTCF_BROADCAST|RTCF_MULTICAST))
		goto drop; 

	/* TW buckets are converted to open requests without
	 * limitations, they conserve resources and peer is
	 * evidently real one.
	 */
	if (tcp_synq_is_full(sk) && !isn) {
#ifdef CONFIG_SYN_COOKIES
		if (sysctl_tcp_syncookies) {
			want_cookie = 1; 
		} else
#endif
		goto drop;
	}

	/* Accept backlog is full. If we have already queued enough
	 * of warm entries in syn queue, drop request. It is better than
	 * clogging syn queue with openreqs with exponentially increasing
	 * timeout.
	 */
	if (tcp_acceptq_is_full(sk) && tcp_synq_young(sk) > 1)
		goto drop;

	req = tcp_openreq_alloc();
	if (req == NULL)
		goto drop;

	tcp_clear_options(&tp);
	tp.mss_clamp = 536;
	tp.user_mss = sk->tp_pinfo.af_tcp.user_mss;

	tcp_parse_options(skb, &tp, 0);

	if (want_cookie) {
		tcp_clear_options(&tp);
		tp.saw_tstamp = 0;
	}

	if (tp.saw_tstamp && tp.rcv_tsval == 0) {
		/* Some OSes (unknown ones, but I see them on web server, which
		 * contains information interesting only for windows'
		 * users) do not send their stamp in SYN. It is easy case.
		 * We simply do not advertise TS support.
		 */
		tp.saw_tstamp = 0;
		tp.tstamp_ok = 0;
	}
	tp.tstamp_ok = tp.saw_tstamp;

	tcp_openreq_init(req, &tp, skb);

	req->af.v4_req.loc_addr = daddr;
	req->af.v4_req.rmt_addr = saddr;
	req->af.v4_req.opt = tcp_v4_save_options(sk, skb);
	req->class = &or_ipv4;
	if (!want_cookie)
		TCP_ECN_create_request(req, skb->h.th);

	if (want_cookie) {
#ifdef CONFIG_SYN_COOKIES
		syn_flood_warning(skb);
#endif
		isn = cookie_v4_init_sequence(sk, skb, &req->mss);
	} else if (isn == 0) {
		struct inet_peer *peer = NULL;

		/* VJ's idea. We save last timestamp seen
		 * from the destination in peer table, when entering
		 * state TIME-WAIT, and check against it before
		 * accepting new connection request.
		 *
		 * If "isn" is not zero, this request hit alive
		 * timewait bucket, so that all the necessary checks
		 * are made in the function processing timewait state.
		 */
		if (tp.saw_tstamp &&
		    sysctl_tcp_tw_recycle &&
		    (dst = tcp_v4_route_req(sk, req)) != NULL &&
		    (peer = rt_get_peer((struct rtable*)dst)) != NULL &&
		    peer->v4daddr == saddr) {
			if (xtime.tv_sec < peer->tcp_ts_stamp + TCP_PAWS_MSL &&
			    (s32)(peer->tcp_ts - req->ts_recent) > TCP_PAWS_WINDOW) {
				NET_INC_STATS_BH(PAWSPassiveRejected);
				dst_release(dst);
				goto drop_and_free;
			}
		}
		/* Kill the following clause, if you dislike this way. */
		else if (!sysctl_tcp_syncookies &&
			 (sysctl_max_syn_backlog - tcp_synq_len(sk)
			  < (sysctl_max_syn_backlog>>2)) &&
			 (!peer || !peer->tcp_ts_stamp) &&
			 (!dst || !dst->rtt)) {
			/* Without syncookies last quarter of
			 * backlog is filled with destinations, proven to be alive.
			 * It means that we continue to communicate
			 * to destinations, already remembered
			 * to the moment of synflood.
			 */
			NETDEBUG(if (net_ratelimit()) \
				printk(KERN_DEBUG "TCP: drop open request from %u.%u.%u.%u/%u\n", \
					NIPQUAD(saddr), ntohs(skb->h.th->source)));
			dst_release(dst);
			goto drop_and_free;
		}

		isn = tcp_v4_init_sequence(sk, skb);
	}
	req->snt_isn = isn;

	if (tcp_v4_send_synack(sk, req, dst))
		goto drop_and_free;

	if (want_cookie) {
	   	tcp_openreq_free(req); 
	} else {
		tcp_v4_synq_add(sk, req);
	}
	return 0;

drop_and_free:
	tcp_openreq_free(req); 
drop:
	TCP_INC_STATS_BH(TcpAttemptFails);
	return 0;
#else
  return 0;
#endif
}


/* 
 * The three way handshake has completed - we got a valid synack - 
 * now create the new socket. 
 */
struct sock * tcp_v4_syn_recv_sock(struct sock *sk, struct sk_buff *skb,
				   struct open_request *req,
				   struct dst_entry *dst)
{
#if 0
	struct tcp_opt *newtp;
	struct sock *newsk;

	if (tcp_acceptq_is_full(sk))
		goto exit_overflow;

	if (dst == NULL &&
	    (dst = tcp_v4_route_req(sk, req)) == NULL)
		goto exit;

	newsk = tcp_create_openreq_child(sk, req, skb);
	if (!newsk)
		goto exit;

	newsk->dst_cache = dst;
	newsk->route_caps = dst->dev->features;

	newtp = &(newsk->tp_pinfo.af_tcp);
	newsk->daddr = req->af.v4_req.rmt_addr;
	newsk->saddr = req->af.v4_req.loc_addr;
	newsk->rcv_saddr = req->af.v4_req.loc_addr;
	newsk->protinfo.af_inet.opt = req->af.v4_req.opt;
	req->af.v4_req.opt = NULL;
	newsk->protinfo.af_inet.mc_index = tcp_v4_iif(skb);
	newsk->protinfo.af_inet.mc_ttl = skb->nh.iph->ttl;
	newtp->ext_header_len = 0;
	if (newsk->protinfo.af_inet.opt)
		newtp->ext_header_len = newsk->protinfo.af_inet.opt->optlen;
	newsk->protinfo.af_inet.id = newtp->write_seq^jiffies;

	tcp_sync_mss(newsk, dst->pmtu);
	newtp->advmss = dst->advmss;
	tcp_initialize_rcv_mss(newsk);

	__tcp_v4_hash(newsk, 0);
	__tcp_inherit_port(sk, newsk);

	return newsk;

exit_overflow:
	NET_INC_STATS_BH(ListenOverflows);
exit:
	NET_INC_STATS_BH(ListenDrops);
	dst_release(dst);
	return NULL;
#else
  return NULL;
#endif
}

static struct sock *tcp_v4_hnd_req(struct sock *sk,struct sk_buff *skb)
{
#if 0
	struct open_request *req, **prev;
	struct tcphdr *th = skb->h.th;
	struct iphdr *iph = skb->nh.iph;
	struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);
	struct sock *nsk;

	/* Find possible connection requests. */
	req = tcp_v4_search_req(tp, &prev,
				th->source,
				iph->saddr, iph->daddr);
	if (req)
		return tcp_check_req(sk, skb, req, prev);

	nsk = __tcp_v4_lookup_established(skb->nh.iph->saddr,
					  th->source,
					  skb->nh.iph->daddr,
					  ntohs(th->dest),
					  tcp_v4_iif(skb));

	if (nsk) {
		if (nsk->state != TCP_TIME_WAIT) {
			bh_lock_sock(nsk);
			return nsk;
		}
		tcp_tw_put((struct tcp_tw_bucket*)nsk);
		return NULL;
	}

#ifdef CONFIG_SYN_COOKIES
	if (!th->rst && !th->syn && th->ack)
		sk = cookie_v4_check(sk, skb, &(IPCB(skb)->opt));
#endif
	return sk;
#else
  return NULL;
#endif
}

static int tcp_v4_checksum_init(struct sk_buff *skb)
{
#if 0
	if (skb->ip_summed == CHECKSUM_HW) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		if (!tcp_v4_check(skb->h.th,skb->len,skb->nh.iph->saddr,
				  skb->nh.iph->daddr,skb->csum))
			return 0;

		NETDEBUG(if (net_ratelimit()) printk(KERN_DEBUG "hw tcp v4 csum failed\n"));
		skb->ip_summed = CHECKSUM_NONE;
	}
	if (skb->len <= 76) {
		if (tcp_v4_check(skb->h.th,skb->len,skb->nh.iph->saddr,
				 skb->nh.iph->daddr,
				 skb_checksum(skb, 0, skb->len, 0)))
			return -1;
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	} else {
		skb->csum = ~tcp_v4_check(skb->h.th,skb->len,skb->nh.iph->saddr,
					  skb->nh.iph->daddr,0);
	}
	return 0;
#else
  return 0;
#endif
}


/* The socket must have it's spinlock held when we get
 * here.
 *
 * We have a potential double-lock case here, so even when
 * doing backlog processing we use the BH locking scheme.
 * This is because we cannot sleep with the original spinlock
 * held.
 */
int tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb)
{
#if 0
#ifdef CONFIG_FILTER
	struct sk_filter *filter = sk->filter;
	if (filter && sk_filter(skb, filter))
		goto discard;
#endif /* CONFIG_FILTER */

  	IP_INC_STATS_BH(IpInDelivers);

	if (sk->state == TCP_ESTABLISHED) { /* Fast path */
		TCP_CHECK_TIMER(sk);
		if (tcp_rcv_established(sk, skb, skb->h.th, skb->len))
			goto reset;
		TCP_CHECK_TIMER(sk);
		return 0; 
	}

	if (skb->len < (skb->h.th->doff<<2) || tcp_checksum_complete(skb))
		goto csum_err;

	if (sk->state == TCP_LISTEN) { 
		struct sock *nsk = tcp_v4_hnd_req(sk, skb);
		if (!nsk)
			goto discard;

		if (nsk != sk) {
			if (tcp_child_process(sk, nsk, skb))
				goto reset;
			return 0;
		}
	}

	TCP_CHECK_TIMER(sk);
	if (tcp_rcv_state_process(sk, skb, skb->h.th, skb->len))
		goto reset;
	TCP_CHECK_TIMER(sk);
	return 0;

reset:
	tcp_v4_send_reset(skb);
discard:
	kfree_skb(skb);
	/* Be careful here. If this function gets more complicated and
	 * gcc suffers from register pressure on the x86, sk (in %ebx) 
	 * might be destroyed here. This current version compiles correctly,
	 * but you have been warned.
	 */
	return 0;

csum_err:
	TCP_INC_STATS_BH(TcpInErrs);
	goto discard;
#else
  return 0;
#endif
}

/*
 *	From tcp_input.c
 */

int tcp_v4_rcv(struct sk_buff *skb)
{
#if 0
	struct tcphdr *th;
	struct sock *sk;
	int ret;

	if (skb->pkt_type!=PACKET_HOST)
		goto discard_it;

	/* Count it even if it's bad */
	TCP_INC_STATS_BH(TcpInSegs);

	if (!pskb_may_pull(skb, sizeof(struct tcphdr)))
		goto discard_it;

	th = skb->h.th;

	if (th->doff < sizeof(struct tcphdr)/4)
		goto bad_packet;
	if (!pskb_may_pull(skb, th->doff*4))
		goto discard_it;

	/* An explanation is required here, I think.
	 * Packet length and doff are validated by header prediction,
	 * provided case of th->doff==0 is elimineted.
	 * So, we defer the checks. */
	if ((skb->ip_summed != CHECKSUM_UNNECESSARY &&
	     tcp_v4_checksum_init(skb) < 0))
		goto bad_packet;

	th = skb->h.th;
	TCP_SKB_CB(skb)->seq = ntohl(th->seq);
	TCP_SKB_CB(skb)->end_seq = (TCP_SKB_CB(skb)->seq + th->syn + th->fin +
				    skb->len - th->doff*4);
	TCP_SKB_CB(skb)->ack_seq = ntohl(th->ack_seq);
	TCP_SKB_CB(skb)->when = 0;
	TCP_SKB_CB(skb)->flags = skb->nh.iph->tos;
	TCP_SKB_CB(skb)->sacked = 0;

	sk = __tcp_v4_lookup(skb->nh.iph->saddr, th->source,
			     skb->nh.iph->daddr, ntohs(th->dest), tcp_v4_iif(skb));

	if (!sk)
		goto no_tcp_socket;

process:
	if(!ipsec_sk_policy(sk,skb))
		goto discard_and_relse;

	if (sk->state == TCP_TIME_WAIT)
		goto do_time_wait;

	skb->dev = NULL;

	bh_lock_sock(sk);
	ret = 0;
	if (!sk->lock.users) {
		if (!tcp_prequeue(sk, skb))
			ret = tcp_v4_do_rcv(sk, skb);
	} else
		sk_add_backlog(sk, skb);
	bh_unlock_sock(sk);

	sock_put(sk);

	return ret;

no_tcp_socket:
	if (skb->len < (th->doff<<2) || tcp_checksum_complete(skb)) {
bad_packet:
		TCP_INC_STATS_BH(TcpInErrs);
	} else {
		tcp_v4_send_reset(skb);
	}

discard_it:
	/* Discard frame. */
	kfree_skb(skb);
  	return 0;

discard_and_relse:
	sock_put(sk);
	goto discard_it;

do_time_wait:
	if (skb->len < (th->doff<<2) || tcp_checksum_complete(skb)) {
		TCP_INC_STATS_BH(TcpInErrs);
		goto discard_and_relse;
	}
	switch(tcp_timewait_state_process((struct tcp_tw_bucket *)sk,
					  skb, th, skb->len)) {
	case TCP_TW_SYN:
	{
		struct sock *sk2;

		sk2 = tcp_v4_lookup_listener(skb->nh.iph->daddr, ntohs(th->dest), tcp_v4_iif(skb));
		if (sk2 != NULL) {
			tcp_tw_deschedule((struct tcp_tw_bucket *)sk);
			tcp_timewait_kill((struct tcp_tw_bucket *)sk);
			tcp_tw_put((struct tcp_tw_bucket *)sk);
			sk = sk2;
			goto process;
		}
		/* Fall through to ACK */
	}
	case TCP_TW_ACK:
		tcp_v4_timewait_ack(sk, skb);
		break;
	case TCP_TW_RST:
		goto no_tcp_socket;
	case TCP_TW_SUCCESS:;
	}
	goto discard_it;
#endif
}

/* With per-bucket locks this operation is not-atomic, so that
 * this version is not worse.
 */
static void __tcp_v4_rehash(struct sock *sk)
{
#if 0
	sk->prot->unhash(sk);
	sk->prot->hash(sk);
#endif
}

static int tcp_v4_reselect_saddr(struct sock *sk)
{
#if 0
	int err;
	struct rtable *rt;
	__u32 old_saddr = sk->saddr;
	__u32 new_saddr;
	__u32 daddr = sk->daddr;

	if(sk->protinfo.af_inet.opt && sk->protinfo.af_inet.opt->srr)
		daddr = sk->protinfo.af_inet.opt->faddr;

	/* Query new route. */
	err = ip_route_connect(&rt, daddr, 0,
			       RT_TOS(sk->protinfo.af_inet.tos)|sk->localroute,
			       sk->bound_dev_if);
	if (err)
		return err;

	__sk_dst_set(sk, &rt->u.dst);
	sk->route_caps = rt->u.dst.dev->features;

	new_saddr = rt->rt_src;

	if (new_saddr == old_saddr)
		return 0;

	if (sysctl_ip_dynaddr > 1) {
		printk(KERN_INFO "tcp_v4_rebuild_header(): shifting sk->saddr "
		       "from %d.%d.%d.%d to %d.%d.%d.%d\n",
		       NIPQUAD(old_saddr), 
		       NIPQUAD(new_saddr));
	}

	sk->saddr = new_saddr;
	sk->rcv_saddr = new_saddr;

	/* XXX The only one ugly spot where we need to
	 * XXX really change the sockets identity after
	 * XXX it has entered the hashes. -DaveM
	 *
	 * Besides that, it does not check for connection
	 * uniqueness. Wait for troubles.
	 */
	__tcp_v4_rehash(sk);
	return 0;
#else
  return 0;
#endif
}

int tcp_v4_rebuild_header(struct sock *sk)
{
#if 0
	struct rtable *rt = (struct rtable *)__sk_dst_check(sk, 0);
	u32 daddr;
	int err;

	/* Route is OK, nothing to do. */
	if (rt != NULL)
		return 0;

	/* Reroute. */
	daddr = sk->daddr;
	if(sk->protinfo.af_inet.opt && sk->protinfo.af_inet.opt->srr)
		daddr = sk->protinfo.af_inet.opt->faddr;

	err = ip_route_output(&rt, daddr, sk->saddr,
			      RT_CONN_FLAGS(sk), sk->bound_dev_if);
	if (!err) {
		__sk_dst_set(sk, &rt->u.dst);
		sk->route_caps = rt->u.dst.dev->features;
		return 0;
	}

	/* Routing failed... */
	sk->route_caps = 0;

	if (!sysctl_ip_dynaddr ||
	    sk->state != TCP_SYN_SENT ||
	    (sk->userlocks & SOCK_BINDADDR_LOCK) ||
	    (err = tcp_v4_reselect_saddr(sk)) != 0)
		sk->err_soft=-err;

	return err;
#else
  return 0;
#endif
}

static void v4_addr2sockaddr(struct sock *sk, struct sockaddr * uaddr)
{
#if 0
	struct sockaddr_in *sin = (struct sockaddr_in *) uaddr;

	sin->sin_family		= AF_INET;
	sin->sin_addr.s_addr	= sk->daddr;
	sin->sin_port		= sk->dport;
#endif
}

/* VJ's idea. Save last timestamp seen from this destination
 * and hold it at least for normal timewait interval to use for duplicate
 * segment detection in subsequent connections, before they enter synchronized
 * state.
 */

int tcp_v4_remember_stamp(struct sock *sk)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;
	struct rtable *rt = (struct rtable*)__sk_dst_get(sk);
	struct inet_peer *peer = NULL;
	int release_it = 0;

	if (rt == NULL || rt->rt_dst != sk->daddr) {
		peer = inet_getpeer(sk->daddr, 1);
		release_it = 1;
	} else {
		if (rt->peer == NULL)
			rt_bind_peer(rt, 1);
		peer = rt->peer;
	}

	if (peer) {
		if ((s32)(peer->tcp_ts - tp->ts_recent) <= 0 ||
		    (peer->tcp_ts_stamp + TCP_PAWS_MSL < xtime.tv_sec &&
		     peer->tcp_ts_stamp <= tp->ts_recent_stamp)) {
			peer->tcp_ts_stamp = tp->ts_recent_stamp;
			peer->tcp_ts = tp->ts_recent;
		}
		if (release_it)
			inet_putpeer(peer);
		return 1;
	}

	return 0;
#else
  return 0;
#endif
}

int tcp_v4_tw_remember_stamp(struct tcp_tw_bucket *tw)
{
#if 0
	struct inet_peer *peer = NULL;

	peer = inet_getpeer(tw->daddr, 1);

	if (peer) {
		if ((s32)(peer->tcp_ts - tw->ts_recent) <= 0 ||
		    (peer->tcp_ts_stamp + TCP_PAWS_MSL < xtime.tv_sec &&
		     peer->tcp_ts_stamp <= tw->ts_recent_stamp)) {
			peer->tcp_ts_stamp = tw->ts_recent_stamp;
			peer->tcp_ts = tw->ts_recent;
		}
		inet_putpeer(peer);
		return 1;
	}

	return 0;
#else
  return 0;
#endif
}

#if 0
struct tcp_func ipv4_specific = {
	ip_queue_xmit,
	tcp_v4_send_check,
	tcp_v4_rebuild_header,
	tcp_v4_conn_request,
	tcp_v4_syn_recv_sock,
	tcp_v4_remember_stamp,
	sizeof(struct iphdr),

	ip_setsockopt,
	ip_getsockopt,
	v4_addr2sockaddr,
	sizeof(struct sockaddr_in)
};
#endif

/* NOTE: A lot of things set to zero explicitly by call to
 *       sk_alloc() so need not be done here.
 */
static int tcp_v4_init_sock(struct sock *sk)
{
#if 0
	struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);

	skb_queue_head_init(&tp->out_of_order_queue);
	tcp_init_xmit_timers(sk);
	tcp_prequeue_init(tp);

	tp->rto  = TCP_TIMEOUT_INIT;
	tp->mdev = TCP_TIMEOUT_INIT;
      
	/* So many TCP implementations out there (incorrectly) count the
	 * initial SYN frame in their delayed-ACK and congestion control
	 * algorithms that we must have the following bandaid to talk
	 * efficiently to them.  -DaveM
	 */
	tp->snd_cwnd = 2;

	/* See draft-stevens-tcpca-spec-01 for discussion of the
	 * initialization of these values.
	 */
	tp->snd_ssthresh = 0x7fffffff;	/* Infinity */
	tp->snd_cwnd_clamp = ~0;
	tp->mss_cache = 536;

	tp->reordering = sysctl_tcp_reordering;

	sk->state = TCP_CLOSE;

	sk->write_space = tcp_write_space;
	sk->use_write_queue = 1;

	sk->tp_pinfo.af_tcp.af_specific = &ipv4_specific;

	sk->sndbuf = sysctl_tcp_wmem[1];
	sk->rcvbuf = sysctl_tcp_rmem[1];

	atomic_inc(&tcp_sockets_allocated);

	return 0;
#else
  return 0;
#endif
}

static int tcp_v4_destroy_sock(struct sock *sk)
{
#if 0
	struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);

	tcp_clear_xmit_timers(sk);

	/* Cleanup up the write buffer. */
  	tcp_writequeue_purge(sk);

	/* Cleans up our, hopefully empty, out_of_order_queue. */
  	__skb_queue_purge(&tp->out_of_order_queue);

	/* Clean prequeue, it must be empty really */
	__skb_queue_purge(&tp->ucopy.prequeue);

	/* Clean up a referenced TCP bind bucket. */
	if(sk->prev != NULL)
		tcp_put_port(sk);

	/* If sendmsg cached page exists, toss it. */
	if (tp->sndmsg_page != NULL)
		__free_page(tp->sndmsg_page);

	atomic_dec(&tcp_sockets_allocated);

	return 0;
#else
  return 0;
#endif
}

/* Proc filesystem TCP sock list dumping. */
static void get_openreq(struct sock *sk, struct open_request *req, char *tmpbuf, int i, int uid)
{
#if 0
	int ttd = req->expires - jiffies;

	sprintf(tmpbuf, "%4d: %08X:%04X %08X:%04X"
		" %02X %08X:%08X %02X:%08X %08X %5d %8d %u %d %p",
		i,
		req->af.v4_req.loc_addr,
		ntohs(sk->sport),
		req->af.v4_req.rmt_addr,
		ntohs(req->rmt_port),
		TCP_SYN_RECV,
		0,0, /* could print option size, but that is af dependent. */
		1,   /* timers active (only the expire timer) */  
		ttd, 
		req->retrans,
		uid,
		0,  /* non standard timer */  
		0, /* open_requests have no inode */
		atomic_read(&sk->refcnt),
		req
		); 
#endif
}

static void get_tcp_sock(struct sock *sp, char *tmpbuf, int i)
{
#if 0
	unsigned int dest, src;
	__u16 destp, srcp;
	int timer_active;
	unsigned long timer_expires;
	struct tcp_opt *tp = &sp->tp_pinfo.af_tcp;

	dest  = sp->daddr;
	src   = sp->rcv_saddr;
	destp = ntohs(sp->dport);
	srcp  = ntohs(sp->sport);
	if (tp->pending == TCP_TIME_RETRANS) {
		timer_active	= 1;
		timer_expires	= tp->timeout;
	} else if (tp->pending == TCP_TIME_PROBE0) {
		timer_active	= 4;
		timer_expires	= tp->timeout;
	} else if (timer_pending(&sp->timer)) {
		timer_active	= 2;
		timer_expires	= sp->timer.expires;
	} else {
		timer_active	= 0;
		timer_expires = jiffies;
	}

	sprintf(tmpbuf, "%4d: %08X:%04X %08X:%04X"
		" %02X %08X:%08X %02X:%08lX %08X %5d %8d %lu %d %p %u %u %u %u %d",
		i, src, srcp, dest, destp, sp->state, 
		tp->write_seq-tp->snd_una, tp->rcv_nxt-tp->copied_seq,
		timer_active, timer_expires-jiffies,
		tp->retransmits,
		sock_i_uid(sp),
		tp->probes_out,
		sock_i_ino(sp),
		atomic_read(&sp->refcnt), sp,
		tp->rto, tp->ack.ato, (tp->ack.quick<<1)|tp->ack.pingpong,
		tp->snd_cwnd, tp->snd_ssthresh>=0xFFFF?-1:tp->snd_ssthresh
		);
#endif
}

static void get_timewait_sock(struct tcp_tw_bucket *tw, char *tmpbuf, int i)
{
#if 0
	unsigned int dest, src;
	__u16 destp, srcp;
	int ttd = tw->ttd - jiffies;

	if (ttd < 0)
		ttd = 0;

	dest  = tw->daddr;
	src   = tw->rcv_saddr;
	destp = ntohs(tw->dport);
	srcp  = ntohs(tw->sport);

	sprintf(tmpbuf, "%4d: %08X:%04X %08X:%04X"
		" %02X %08X:%08X %02X:%08X %08X %5d %8d %d %d %p",
		i, src, srcp, dest, destp, tw->substate, 0, 0,
		3, ttd, 0, 0, 0, 0,
		atomic_read(&tw->refcnt), tw);
#endif
}

#define TMPSZ 150

int tcp_get_info(char *buffer, char **start, off_t offset, int length)
{
#if 0
	int len = 0, num = 0, i;
	off_t begin, pos = 0;
	char tmpbuf[TMPSZ+1];

	if (offset < TMPSZ)
		len += sprintf(buffer, "%-*s\n", TMPSZ-1,
			       "  sl  local_address rem_address   st tx_queue "
			       "rx_queue tr tm->when retrnsmt   uid  timeout inode");

	pos = TMPSZ;

	/* First, walk listening socket table. */
	tcp_listen_lock();
	for(i = 0; i < TCP_LHTABLE_SIZE; i++) {
		struct sock *sk;
		struct tcp_listen_opt *lopt;
		int k;

		for (sk = tcp_listening_hash[i]; sk; sk = sk->next, num++) {
			struct open_request *req;
			int uid;
			struct tcp_opt *tp = &(sk->tp_pinfo.af_tcp);

			if (!TCP_INET_FAMILY(sk->family))
				goto skip_listen;

			pos += TMPSZ;
			if (pos >= offset) {
				get_tcp_sock(sk, tmpbuf, num);
				len += sprintf(buffer+len, "%-*s\n", TMPSZ-1, tmpbuf);
				if (pos >= offset + length) {
					tcp_listen_unlock();
					goto out_no_bh;
				}
			}

skip_listen:
			uid = sock_i_uid(sk);
			read_lock_bh(&tp->syn_wait_lock);
			lopt = tp->listen_opt;
			if (lopt && lopt->qlen != 0) {
				for (k=0; k<TCP_SYNQ_HSIZE; k++) {
					for (req = lopt->syn_table[k]; req; req = req->dl_next, num++) {
						if (!TCP_INET_FAMILY(req->class->family))
							continue;

						pos += TMPSZ;
						if (pos <= offset)
							continue;
						get_openreq(sk, req, tmpbuf, num, uid);
						len += sprintf(buffer+len, "%-*s\n", TMPSZ-1, tmpbuf);
						if (pos >= offset + length) {
							read_unlock_bh(&tp->syn_wait_lock);
							tcp_listen_unlock();
							goto out_no_bh;
						}
					}
				}
			}
			read_unlock_bh(&tp->syn_wait_lock);

			/* Completed requests are in normal socket hash table */
		}
	}
	tcp_listen_unlock();

	local_bh_disable();

	/* Next, walk established hash chain. */
	for (i = 0; i < tcp_ehash_size; i++) {
		struct tcp_ehash_bucket *head = &tcp_ehash[i];
		struct sock *sk;
		struct tcp_tw_bucket *tw;

		read_lock(&head->lock);
		for(sk = head->chain; sk; sk = sk->next, num++) {
			if (!TCP_INET_FAMILY(sk->family))
				continue;
			pos += TMPSZ;
			if (pos <= offset)
				continue;
			get_tcp_sock(sk, tmpbuf, num);
			len += sprintf(buffer+len, "%-*s\n", TMPSZ-1, tmpbuf);
			if (pos >= offset + length) {
				read_unlock(&head->lock);
				goto out;
			}
		}
		for (tw = (struct tcp_tw_bucket *)tcp_ehash[i+tcp_ehash_size].chain;
		     tw != NULL;
		     tw = (struct tcp_tw_bucket *)tw->next, num++) {
			if (!TCP_INET_FAMILY(tw->family))
				continue;
			pos += TMPSZ;
			if (pos <= offset)
				continue;
			get_timewait_sock(tw, tmpbuf, num);
			len += sprintf(buffer+len, "%-*s\n", TMPSZ-1, tmpbuf);
			if (pos >= offset + length) {
				read_unlock(&head->lock);
				goto out;
			}
		}
		read_unlock(&head->lock);
	}

out:
	local_bh_enable();
out_no_bh:

	begin = len - (pos - offset);
	*start = buffer + begin;
	len -= begin;
	if (len > length)
		len = length;
	if (len < 0)
		len = 0; 
	return len;
#endif
}

struct proto tcp_prot = {
	name:		"TCP",
	close:		tcp_close,
	connect:	tcp_v4_connect,
	disconnect:	tcp_disconnect,
	accept:		tcp_accept,
	ioctl:		tcp_ioctl,
	init:		tcp_v4_init_sock,
	destroy:	tcp_v4_destroy_sock,
	shutdown:	tcp_shutdown,
	setsockopt:	tcp_setsockopt,
	getsockopt:	tcp_getsockopt,
	sendmsg:	tcp_sendmsg,
	recvmsg:	tcp_recvmsg,
	backlog_rcv:	tcp_v4_do_rcv,
	hash:		tcp_v4_hash,
	unhash:		tcp_unhash,
	get_port:	tcp_v4_get_port,
};



void tcp_v4_init(struct net_proto_family *ops)
{
#if 0
	int err;

	tcp_inode.i_mode = S_IFSOCK;
	tcp_inode.i_sock = 1;
	tcp_inode.i_uid = 0;
	tcp_inode.i_gid = 0;
	init_waitqueue_head(&tcp_inode.i_wait);
	init_waitqueue_head(&tcp_inode.u.socket_i.wait);

	tcp_socket->inode = &tcp_inode;
	tcp_socket->state = SS_UNCONNECTED;
	tcp_socket->type=SOCK_RAW;

	if ((err=ops->create(tcp_socket, IPPROTO_TCP))<0)
		panic("Failed to create the TCP control socket.\n");
	tcp_socket->sk->allocation=GFP_ATOMIC;
	tcp_socket->sk->protinfo.af_inet.ttl = MAXTTL;

	/* Unhash it so that IP input processing does not even
	 * see it, we do not wish this socket to see incoming
	 * packets.
	 */
	tcp_socket->sk->prot->unhash(tcp_socket->sk);
#endif
}
