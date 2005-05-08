/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/tcpcore.h
 * PURPOSE:     Transmission Control Protocol definitions
 * REVISIONS:
 *   CSH 01/01-2003 Ported from linux kernel 2.4.20
 */

/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the TCP module.
 *
 * Version:	@(#)tcp.h	1.0.5	05/23/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef __TCPCORE_H
#define __TCPCORE_H

#include "tcpdef.h"


struct socket;



#if 1 /* skbuff */

#define HAVE_ALLOC_SKB		/* For the drivers to know */
#define HAVE_ALIGNABLE_SKB	/* Ditto 8)		   */
#define SLAB_SKB 		/* Slabified skbuffs 	   */

#define CHECKSUM_NONE 0
#define CHECKSUM_HW 1
#define CHECKSUM_UNNECESSARY 2

#define SKB_DATA_ALIGN(X)	(((X) + (SMP_CACHE_BYTES-1)) & ~(SMP_CACHE_BYTES-1))
#define SKB_MAX_ORDER(X,ORDER)	(((PAGE_SIZE<<(ORDER)) - (X) - sizeof(struct skb_shared_info))&~(SMP_CACHE_BYTES-1))
#define SKB_MAX_HEAD(X)		(SKB_MAX_ORDER((X),0))
#define SKB_MAX_ALLOC		(SKB_MAX_ORDER(0,2))

/* A. Checksumming of received packets by device.
 *
 *	NONE: device failed to checksum this packet.
 *		skb->csum is undefined.
 *
 *	UNNECESSARY: device parsed packet and wouldbe verified checksum.
 *		skb->csum is undefined.
 *	      It is bad option, but, unfortunately, many of vendors do this.
 *	      Apparently with secret goal to sell you new device, when you
 *	      will add new protocol to your host. F.e. IPv6. 8)
 *
 *	HW: the most generic way. Device supplied checksum of _all_
 *	    the packet as seen by netif_rx in skb->csum.
 *	    NOTE: Even if device supports only some protocols, but
 *	    is able to produce some skb->csum, it MUST use HW,
 *	    not UNNECESSARY.
 *
 * B. Checksumming on output.
 *
 *	NONE: skb is checksummed by protocol or csum is not required.
 *
 *	HW: device is required to csum packet as seen by hard_start_xmit
 *	from skb->h.raw to the end and to record the checksum
 *	at skb->h.raw+skb->csum.
 *
 *	Device must show its capabilities in dev->features, set
 *	at device setup time.
 *	NETIF_F_HW_CSUM	- it is clever device, it is able to checksum
 *			  everything.
 *	NETIF_F_NO_CSUM - loopback or reliable single hop media.
 *	NETIF_F_IP_CSUM - device is dumb. It is able to csum only
 *			  TCP/UDP over IPv4. Sigh. Vendors like this
 *			  way by an unknown reason. Though, see comment above
 *			  about CHECKSUM_UNNECESSARY. 8)
 *
 *	Any questions? No questions, good. 		--ANK
 */

#ifdef __i386__
#define NET_CALLER(arg) (*(((void**)&arg)-1))
#else
#define NET_CALLER(arg) __builtin_return_address(0)
#endif

#ifdef CONFIG_NETFILTER
struct nf_conntrack {
	atomic_t use;
	void (*destroy)(struct nf_conntrack *);
};

struct nf_ct_info {
	struct nf_conntrack *master;
};
#endif

struct sk_buff_head {
	/* These two members must be first. */
	struct sk_buff	* next;
	struct sk_buff	* prev;

	__u32		qlen;
	spinlock_t	lock;
};

struct sk_buff;

#define MAX_SKB_FRAGS 6

typedef struct skb_frag_struct skb_frag_t;

struct skb_frag_struct
{
	struct page *page;
	__u16 page_offset;
	__u16 size;
};

/* This data is invariant across clones and lives at
 * the end of the header data, ie. at skb->end.
 */
struct skb_shared_info {
	atomic_t	dataref;
	unsigned int	nr_frags;
	struct sk_buff	*frag_list;
	skb_frag_t	frags[MAX_SKB_FRAGS];
};

struct sk_buff {
	/* These two members must be first. */
	struct sk_buff	* next;			/* Next buffer in list 				*/
	struct sk_buff	* prev;			/* Previous buffer in list 			*/

	struct sk_buff_head * list;		/* List we are on				*/
	struct sock	*sk;			/* Socket we are owned by 			*/
	struct timeval	stamp;			/* Time we arrived				*/
	struct net_device	*dev;		/* Device we arrived on/are leaving by		*/

	/* Transport layer header */
	union
	{
		struct tcphdr	*th;
		struct udphdr	*uh;
		struct icmphdr	*icmph;
		struct igmphdr	*igmph;
		struct iphdr	*ipiph;
		struct spxhdr	*spxh;
		unsigned char	*raw;
	} h;

	/* Network layer header */
	union
	{
		struct iphdr	*iph;
		struct ipv6hdr	*ipv6h;
		struct arphdr	*arph;
		struct ipxhdr	*ipxh;
		unsigned char	*raw;
	} nh;
  
	/* Link layer header */
	union 
	{	
	  	struct ethhdr	*ethernet;
	  	unsigned char 	*raw;
	} mac;

	struct  dst_entry *dst;

	/* 
	 * This is the control buffer. It is free to use for every
	 * layer. Please put your private variables there. If you
	 * want to keep them across layers you have to do a skb_clone()
	 * first. This is owned by whoever has the skb queued ATM.
	 */ 
	char		cb[48];	 

	unsigned int 	len;			/* Length of actual data			*/
 	unsigned int 	data_len;
	unsigned int	csum;			/* Checksum 					*/
	unsigned char 	__unused,		/* Dead field, may be reused			*/
			cloned, 		/* head may be cloned (check refcnt to be sure). */
  			pkt_type,		/* Packet class					*/
  			ip_summed;		/* Driver fed us an IP checksum			*/
	__u32		priority;		/* Packet queueing priority			*/
	atomic_t	users;			/* User count - see datagram.c,tcp.c 		*/
	unsigned short	protocol;		/* Packet protocol from driver. 		*/
	unsigned short	security;		/* Security level of packet			*/
	unsigned int	truesize;		/* Buffer size 					*/

	unsigned char	*head;			/* Head of buffer 				*/
	unsigned char	*data;			/* Data head pointer				*/
	unsigned char	*tail;			/* Tail pointer					*/
	unsigned char 	*end;			/* End pointer					*/

	void 		(*destructor)(struct sk_buff *);	/* Destruct function		*/
#ifdef CONFIG_NETFILTER
	/* Can be used for communication between hooks. */
        unsigned long	nfmark;
	/* Cache info */
	__u32		nfcache;
	/* Associated connection, if any */
	struct nf_ct_info *nfct;
#ifdef CONFIG_NETFILTER_DEBUG
        unsigned int nf_debug;
#endif
#endif /*CONFIG_NETFILTER*/

#if defined(CONFIG_HIPPI)
	union{
		__u32	ifield;
	} private;
#endif

#ifdef CONFIG_NET_SCHED
       __u32           tc_index;               /* traffic control index */
#endif
};

#define SK_WMEM_MAX	65535
#define SK_RMEM_MAX	65535

#if 1
//#ifdef __KERNEL__
/*
 *	Handling routines are only of interest to the kernel
 */

extern void			__kfree_skb(struct sk_buff *skb);
extern struct sk_buff *		alloc_skb(unsigned int size, int priority);
extern void			kfree_skbmem(struct sk_buff *skb);
extern struct sk_buff *		skb_clone(struct sk_buff *skb, int priority);
extern struct sk_buff *		skb_copy(const struct sk_buff *skb, int priority);
extern struct sk_buff *		pskb_copy(struct sk_buff *skb, int gfp_mask);
extern int			pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, int gfp_mask);
extern struct sk_buff *		skb_realloc_headroom(struct sk_buff *skb, unsigned int headroom);
extern struct sk_buff *		skb_copy_expand(const struct sk_buff *skb, 
						int newheadroom,
						int newtailroom,
						int priority);
#define dev_kfree_skb(a)	kfree_skb(a)
extern void	skb_over_panic(struct sk_buff *skb, int len, void *here);
extern void	skb_under_panic(struct sk_buff *skb, int len, void *here);

/* Internal */
#define skb_shinfo(SKB)		((struct skb_shared_info *)((SKB)->end))

/**
 *	skb_queue_empty - check if a queue is empty
 *	@list: queue head
 *
 *	Returns true if the queue is empty, false otherwise.
 */
 
static inline int skb_queue_empty(struct sk_buff_head *list)
{
	return (list->next == (struct sk_buff *) list);
}

/**
 *	skb_get - reference buffer
 *	@skb: buffer to reference
 *
 *	Makes another reference to a socket buffer and returns a pointer
 *	to the buffer.
 */
 
static inline struct sk_buff *skb_get(struct sk_buff *skb)
{
	atomic_inc(&skb->users);
	return skb;
}

/*
 * If users==1, we are the only owner and are can avoid redundant
 * atomic change.
 */
 
/**
 *	kfree_skb - free an sk_buff
 *	@skb: buffer to free
 *
 *	Drop a reference to the buffer and free it if the usage count has
 *	hit zero.
 */
 
static inline void kfree_skb(struct sk_buff *skb)
{
	if (atomic_read(&skb->users) == 1 || atomic_dec_and_test(&skb->users))
		__kfree_skb(skb);
}

/* Use this if you didn't touch the skb state [for fast switching] */
static inline void kfree_skb_fast(struct sk_buff *skb)
{
	if (atomic_read(&skb->users) == 1 || atomic_dec_and_test(&skb->users))
		kfree_skbmem(skb);	
}

/**
 *	skb_cloned - is the buffer a clone
 *	@skb: buffer to check
 *
 *	Returns true if the buffer was generated with skb_clone() and is
 *	one of multiple shared copies of the buffer. Cloned buffers are
 *	shared data so must not be written to under normal circumstances.
 */

static inline int skb_cloned(struct sk_buff *skb)
{
	return skb->cloned && atomic_read(&skb_shinfo(skb)->dataref) != 1;
}

/**
 *	skb_shared - is the buffer shared
 *	@skb: buffer to check
 *
 *	Returns true if more than one person has a reference to this
 *	buffer.
 */
 
static inline int skb_shared(struct sk_buff *skb)
{
	return (atomic_read(&skb->users) != 1);
}

/** 
 *	skb_share_check - check if buffer is shared and if so clone it
 *	@skb: buffer to check
 *	@pri: priority for memory allocation
 *	
 *	If the buffer is shared the buffer is cloned and the old copy
 *	drops a reference. A new clone with a single reference is returned.
 *	If the buffer is not shared the original buffer is returned. When
 *	being called from interrupt status or with spinlocks held pri must
 *	be GFP_ATOMIC.
 *
 *	NULL is returned on a memory allocation failure.
 */
 
static inline struct sk_buff *skb_share_check(struct sk_buff *skb, int pri)
{
	if (skb_shared(skb)) {
		struct sk_buff *nskb;
		nskb = skb_clone(skb, pri);
		kfree_skb(skb);
		return nskb;
	}
	return skb;
}


/*
 *	Copy shared buffers into a new sk_buff. We effectively do COW on
 *	packets to handle cases where we have a local reader and forward
 *	and a couple of other messy ones. The normal one is tcpdumping
 *	a packet thats being forwarded.
 */
 
/**
 *	skb_unshare - make a copy of a shared buffer
 *	@skb: buffer to check
 *	@pri: priority for memory allocation
 *
 *	If the socket buffer is a clone then this function creates a new
 *	copy of the data, drops a reference count on the old copy and returns
 *	the new copy with the reference count at 1. If the buffer is not a clone
 *	the original buffer is returned. When called with a spinlock held or
 *	from interrupt state @pri must be %GFP_ATOMIC
 *
 *	%NULL is returned on a memory allocation failure.
 */
 
static inline struct sk_buff *skb_unshare(struct sk_buff *skb, int pri)
{
	struct sk_buff *nskb;
	if(!skb_cloned(skb))
		return skb;
	nskb=skb_copy(skb, pri);
	kfree_skb(skb);		/* Free our shared copy */
	return nskb;
}

/**
 *	skb_peek
 *	@list_: list to peek at
 *
 *	Peek an &sk_buff. Unlike most other operations you _MUST_
 *	be careful with this one. A peek leaves the buffer on the
 *	list and someone else may run off with it. You must hold
 *	the appropriate locks or have a private queue to do this.
 *
 *	Returns %NULL for an empty list or a pointer to the head element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */
 
static inline struct sk_buff *skb_peek(struct sk_buff_head *list_)
{
	struct sk_buff *list = ((struct sk_buff *)list_)->next;
	if (list == (struct sk_buff *)list_)
		list = NULL;
	return list;
}

/**
 *	skb_peek_tail
 *	@list_: list to peek at
 *
 *	Peek an &sk_buff. Unlike most other operations you _MUST_
 *	be careful with this one. A peek leaves the buffer on the
 *	list and someone else may run off with it. You must hold
 *	the appropriate locks or have a private queue to do this.
 *
 *	Returns %NULL for an empty list or a pointer to the tail element.
 *	The reference count is not incremented and the reference is therefore
 *	volatile. Use with caution.
 */

static inline struct sk_buff *skb_peek_tail(struct sk_buff_head *list_)
{
	struct sk_buff *list = ((struct sk_buff *)list_)->prev;
	if (list == (struct sk_buff *)list_)
		list = NULL;
	return list;
}

/**
 *	skb_queue_len	- get queue length
 *	@list_: list to measure
 *
 *	Return the length of an &sk_buff queue. 
 */
 
static inline __u32 skb_queue_len(struct sk_buff_head *list_)
{
	return(list_->qlen);
}

static inline void skb_queue_head_init(struct sk_buff_head *list)
{
	spin_lock_init(&list->lock);
	list->prev = (struct sk_buff *)list;
	list->next = (struct sk_buff *)list;
	list->qlen = 0;
}

/*
 *	Insert an sk_buff at the start of a list.
 *
 *	The "__skb_xxxx()" functions are the non-atomic ones that
 *	can only be called with interrupts disabled.
 */

/**
 *	__skb_queue_head - queue a buffer at the list head
 *	@list: list to use
 *	@newsk: buffer to queue
 *
 *	Queue a buffer at the start of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */	
 
static inline void __skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
	struct sk_buff *prev, *next;

	newsk->list = list;
	list->qlen++;
	prev = (struct sk_buff *)list;
	next = prev->next;
	newsk->next = next;
	newsk->prev = prev;
	next->prev = newsk;
	prev->next = newsk;
}


/**
 *	skb_queue_head - queue a buffer at the list head
 *	@list: list to use
 *	@newsk: buffer to queue
 *
 *	Queue a buffer at the start of the list. This function takes the
 *	list lock and can be used safely with other locking &sk_buff functions
 *	safely.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */	

static inline void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);
	__skb_queue_head(list, newsk);
	spin_unlock_irqrestore(&list->lock, flags);
}

/**
 *	__skb_queue_tail - queue a buffer at the list tail
 *	@list: list to use
 *	@newsk: buffer to queue
 *
 *	Queue a buffer at the end of a list. This function takes no locks
 *	and you must therefore hold required locks before calling it.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */	
 

static inline void __skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	struct sk_buff *prev, *next;

	newsk->list = list;
	list->qlen++;
	next = (struct sk_buff *)list;
	prev = next->prev;
	newsk->next = next;
	newsk->prev = prev;
	next->prev = newsk;
	prev->next = newsk;
}

/**
 *	skb_queue_tail - queue a buffer at the list tail
 *	@list: list to use
 *	@newsk: buffer to queue
 *
 *	Queue a buffer at the tail of the list. This function takes the
 *	list lock and can be used safely with other locking &sk_buff functions
 *	safely.
 *
 *	A buffer cannot be placed on two lists at the same time.
 */	

static inline void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);
	__skb_queue_tail(list, newsk);
	spin_unlock_irqrestore(&list->lock, flags);
}

/**
 *	__skb_dequeue - remove from the head of the queue
 *	@list: list to dequeue from
 *
 *	Remove the head of the list. This function does not take any locks
 *	so must be used with appropriate locks held only. The head item is
 *	returned or %NULL if the list is empty.
 */

static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *next, *prev, *result;

	prev = (struct sk_buff *) list;
	next = prev->next;
	result = NULL;
	if (next != prev) {
		result = next;
		next = next->next;
		list->qlen--;
		next->prev = prev;
		prev->next = next;
		result->next = NULL;
		result->prev = NULL;
		result->list = NULL;
	}
	return result;
}

/**
 *	skb_dequeue - remove from the head of the queue
 *	@list: list to dequeue from
 *
 *	Remove the head of the list. The list lock is taken so the function
 *	may be used safely with other locking list functions. The head item is
 *	returned or %NULL if the list is empty.
 */

static inline struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	unsigned long flags;
	struct sk_buff *result;

	spin_lock_irqsave(&list->lock, flags);
	result = __skb_dequeue(list);
	spin_unlock_irqrestore(&list->lock, flags);
	return result;
}

/*
 *	Insert a packet on a list.
 */

static inline void __skb_insert(struct sk_buff *newsk,
	struct sk_buff * prev, struct sk_buff *next,
	struct sk_buff_head * list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev = newsk;
	prev->next = newsk;
	newsk->list = list;
	list->qlen++;
}

/**
 *	skb_insert	-	insert a buffer
 *	@old: buffer to insert before
 *	@newsk: buffer to insert
 *
 *	Place a packet before a given packet in a list. The list locks are taken
 *	and this function is atomic with respect to other list locked calls
 *	A buffer cannot be placed on two lists at the same time.
 */

static inline void skb_insert(struct sk_buff *old, struct sk_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&old->list->lock, flags);
	__skb_insert(newsk, old->prev, old, old->list);
	spin_unlock_irqrestore(&old->list->lock, flags);
}

/*
 *	Place a packet after a given packet in a list.
 */

static inline void __skb_append(struct sk_buff *old, struct sk_buff *newsk)
{
	__skb_insert(newsk, old, old->next, old->list);
}

/**
 *	skb_append	-	append a buffer
 *	@old: buffer to insert after
 *	@newsk: buffer to insert
 *
 *	Place a packet after a given packet in a list. The list locks are taken
 *	and this function is atomic with respect to other list locked calls.
 *	A buffer cannot be placed on two lists at the same time.
 */


static inline void skb_append(struct sk_buff *old, struct sk_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&old->list->lock, flags);
	__skb_append(old, newsk);
	spin_unlock_irqrestore(&old->list->lock, flags);
}

/*
 * remove sk_buff from list. _Must_ be called atomically, and with
 * the list known..
 */
 
static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
	struct sk_buff * next, * prev;

	list->qlen--;
	next = skb->next;
	prev = skb->prev;
	skb->next = NULL;
	skb->prev = NULL;
	skb->list = NULL;
	next->prev = prev;
	prev->next = next;
}

/**
 *	skb_unlink	-	remove a buffer from a list
 *	@skb: buffer to remove
 *
 *	Place a packet after a given packet in a list. The list locks are taken
 *	and this function is atomic with respect to other list locked calls
 *	
 *	Works even without knowing the list it is sitting on, which can be 
 *	handy at times. It also means that THE LIST MUST EXIST when you 
 *	unlink. Thus a list must have its contents unlinked before it is
 *	destroyed.
 */

static inline void skb_unlink(struct sk_buff *skb)
{
	struct sk_buff_head *list = skb->list;

	if(list) {
		unsigned long flags;

		spin_lock_irqsave(&list->lock, flags);
		if(skb->list == list)
			__skb_unlink(skb, skb->list);
		spin_unlock_irqrestore(&list->lock, flags);
	}
}

/* XXX: more streamlined implementation */

/**
 *	__skb_dequeue_tail - remove from the tail of the queue
 *	@list: list to dequeue from
 *
 *	Remove the tail of the list. This function does not take any locks
 *	so must be used with appropriate locks held only. The tail item is
 *	returned or %NULL if the list is empty.
 */

static inline struct sk_buff *__skb_dequeue_tail(struct sk_buff_head *list)
{
	struct sk_buff *skb = skb_peek_tail(list); 
	if (skb)
		__skb_unlink(skb, list);
	return skb;
}

/**
 *	skb_dequeue - remove from the head of the queue
 *	@list: list to dequeue from
 *
 *	Remove the head of the list. The list lock is taken so the function
 *	may be used safely with other locking list functions. The tail item is
 *	returned or %NULL if the list is empty.
 */

static inline struct sk_buff *skb_dequeue_tail(struct sk_buff_head *list)
{
	unsigned long flags;
	struct sk_buff *result;

	spin_lock_irqsave(&list->lock, flags);
	result = __skb_dequeue_tail(list);
	spin_unlock_irqrestore(&list->lock, flags);
	return result;
}

static inline int skb_is_nonlinear(const struct sk_buff *skb)
{
	return skb->data_len;
}

static inline int skb_headlen(const struct sk_buff *skb)
{
	return skb->len - skb->data_len;
}

#define SKB_PAGE_ASSERT(skb) do { if (skb_shinfo(skb)->nr_frags) out_of_line_bug(); } while (0)
#define SKB_FRAG_ASSERT(skb) do { if (skb_shinfo(skb)->frag_list) out_of_line_bug(); } while (0)
#define SKB_LINEAR_ASSERT(skb) do { if (skb_is_nonlinear(skb)) out_of_line_bug(); } while (0)

/*
 *	Add data to an sk_buff
 */
 
static inline unsigned char *__skb_put(struct sk_buff *skb, unsigned int len)
{
	unsigned char *tmp=skb->tail;
	SKB_LINEAR_ASSERT(skb);
	skb->tail+=len;
	skb->len+=len;
	return tmp;
}

/**
 *	skb_put - add data to a buffer
 *	@skb: buffer to use 
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer. If this would
 *	exceed the total buffer size the kernel will panic. A pointer to the
 *	first byte of the extra data is returned.
 */
 
static inline unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
#if 0
	unsigned char *tmp=skb->tail;
	SKB_LINEAR_ASSERT(skb);
	skb->tail+=len;
	skb->len+=len;
	if(skb->tail>skb->end) {
		skb_over_panic(skb, len, current_text_addr());
	}
	return tmp;
#else
return NULL;
#endif
}

static inline unsigned char *__skb_push(struct sk_buff *skb, unsigned int len)
{
	skb->data-=len;
	skb->len+=len;
	return skb->data;
}

/**
 *	skb_push - add data to the start of a buffer
 *	@skb: buffer to use 
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer at the buffer
 *	start. If this would exceed the total buffer headroom the kernel will
 *	panic. A pointer to the first byte of the extra data is returned.
 */

static inline unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
#if 0
	skb->data-=len;
	skb->len+=len;
	if(skb->data<skb->head) {
		skb_under_panic(skb, len, current_text_addr());
	}
	return skb->data;
#else
  return NULL;
#endif
}

static inline char *__skb_pull(struct sk_buff *skb, unsigned int len)
{
	skb->len-=len;
	if (skb->len < skb->data_len)
		out_of_line_bug();
	return 	skb->data+=len;
}

/**
 *	skb_pull - remove data from the start of a buffer
 *	@skb: buffer to use 
 *	@len: amount of data to remove
 *
 *	This function removes data from the start of a buffer, returning
 *	the memory to the headroom. A pointer to the next data in the buffer
 *	is returned. Once the data has been pulled future pushes will overwrite
 *	the old data.
 */

static inline unsigned char * skb_pull(struct sk_buff *skb, unsigned int len)
{	
	if (len > skb->len)
		return NULL;
	return __skb_pull(skb,len);
}

extern unsigned char * __pskb_pull_tail(struct sk_buff *skb, int delta);

static inline char *__pskb_pull(struct sk_buff *skb, unsigned int len)
{
	if (len > skb_headlen(skb) &&
	    __pskb_pull_tail(skb, len-skb_headlen(skb)) == NULL)
		return NULL;
	skb->len -= len;
	return 	skb->data += len;
}

static inline unsigned char * pskb_pull(struct sk_buff *skb, unsigned int len)
{	
	if (len > skb->len)
		return NULL;
	return __pskb_pull(skb,len);
}

static inline int pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
	if (len <= skb_headlen(skb))
		return 1;
	if (len > skb->len)
		return 0;
	return (__pskb_pull_tail(skb, len-skb_headlen(skb)) != NULL);
}

/**
 *	skb_headroom - bytes at buffer head
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the head of an &sk_buff.
 */
 
static inline int skb_headroom(const struct sk_buff *skb)
{
	return skb->data-skb->head;
}

/**
 *	skb_tailroom - bytes at buffer end
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the tail of an sk_buff
 */

static inline int skb_tailroom(const struct sk_buff *skb)
{
	return skb_is_nonlinear(skb) ? 0 : skb->end-skb->tail;
}

/**
 *	skb_reserve - adjust headroom
 *	@skb: buffer to alter
 *	@len: bytes to move
 *
 *	Increase the headroom of an empty &sk_buff by reducing the tail
 *	room. This is only allowed for an empty buffer.
 */

static inline void skb_reserve(struct sk_buff *skb, unsigned int len)
{
	skb->data+=len;
	skb->tail+=len;
}

extern int ___pskb_trim(struct sk_buff *skb, unsigned int len, int realloc);

static inline void __skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (!skb->data_len) {
		skb->len = len;
		skb->tail = skb->data+len;
	} else {
		___pskb_trim(skb, len, 0);
	}
}

/**
 *	skb_trim - remove end from a buffer
 *	@skb: buffer to alter
 *	@len: new length
 *
 *	Cut the length of a buffer down by removing data from the tail. If
 *	the buffer is already under the length specified it is not modified.
 */

static inline void skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (skb->len > len) {
		__skb_trim(skb, len);
	}
}


static inline int __pskb_trim(struct sk_buff *skb, unsigned int len)
{
	if (!skb->data_len) {
		skb->len = len;
		skb->tail = skb->data+len;
		return 0;
	} else {
		return ___pskb_trim(skb, len, 1);
	}
}

static inline int pskb_trim(struct sk_buff *skb, unsigned int len)
{
	if (len < skb->len)
		return __pskb_trim(skb, len);
	return 0;
}

/**
 *	skb_orphan - orphan a buffer
 *	@skb: buffer to orphan
 *
 *	If a buffer currently has an owner then we call the owner's
 *	destructor function and make the @skb unowned. The buffer continues
 *	to exist but is no longer charged to its former owner.
 */


static inline void skb_orphan(struct sk_buff *skb)
{
	if (skb->destructor)
		skb->destructor(skb);
	skb->destructor = NULL;
	skb->sk = NULL;
}

/**
 *	skb_purge - empty a list
 *	@list: list to empty
 *
 *	Delete all buffers on an &sk_buff list. Each buffer is removed from
 *	the list and one reference dropped. This function takes the list
 *	lock and is atomic with respect to other list locking functions.
 */


static inline void skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb=skb_dequeue(list))!=NULL)
		kfree_skb(skb);
}

/**
 *	__skb_purge - empty a list
 *	@list: list to empty
 *
 *	Delete all buffers on an &sk_buff list. Each buffer is removed from
 *	the list and one reference dropped. This function does not take the
 *	list lock and the caller must hold the relevant locks to use it.
 */


static inline void __skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb=__skb_dequeue(list))!=NULL)
		kfree_skb(skb);
}

/**
 *	__dev_alloc_skb - allocate an skbuff for sending
 *	@length: length to allocate
 *	@gfp_mask: get_free_pages mask, passed to alloc_skb
 *
 *	Allocate a new &sk_buff and assign it a usage count of one. The
 *	buffer has unspecified headroom built in. Users should allocate
 *	the headroom they think they need without accounting for the
 *	built in space. The built in space is used for optimisations.
 *
 *	%NULL is returned in there is no free memory.
 */
 
static inline struct sk_buff *__dev_alloc_skb(unsigned int length,
					      int gfp_mask)
{
	struct sk_buff *skb;

	skb = alloc_skb(length+16, gfp_mask);
	if (skb)
		skb_reserve(skb,16);
	return skb;
}

/**
 *	dev_alloc_skb - allocate an skbuff for sending
 *	@length: length to allocate
 *
 *	Allocate a new &sk_buff and assign it a usage count of one. The
 *	buffer has unspecified headroom built in. Users should allocate
 *	the headroom they think they need without accounting for the
 *	built in space. The built in space is used for optimisations.
 *
 *	%NULL is returned in there is no free memory. Although this function
 *	allocates memory it can be called from an interrupt.
 */
 
static inline struct sk_buff *dev_alloc_skb(unsigned int length)
{
#if 0
	return __dev_alloc_skb(length, GFP_ATOMIC);
#else
  return NULL;
#endif
}

/**
 *	skb_cow - copy header of skb when it is required
 *	@skb: buffer to cow
 *	@headroom: needed headroom
 *
 *	If the skb passed lacks sufficient headroom or its data part
 *	is shared, data is reallocated. If reallocation fails, an error
 *	is returned and original skb is not changed.
 *
 *	The result is skb with writable area skb->head...skb->tail
 *	and at least @headroom of space at head.
 */

static inline int
skb_cow(struct sk_buff *skb, unsigned int headroom)
{
#if 0
	int delta = (headroom > 16 ? headroom : 16) - skb_headroom(skb);

	if (delta < 0)
		delta = 0;

	if (delta || skb_cloned(skb))
		return pskb_expand_head(skb, (delta+15)&~15, 0, GFP_ATOMIC);
	return 0;
#else
  return 0;
#endif
}

/**
 *	skb_linearize - convert paged skb to linear one
 *	@skb: buffer to linarize
 *	@gfp: allocation mode
 *
 *	If there is no free memory -ENOMEM is returned, otherwise zero
 *	is returned and the old skb data released.  */
int skb_linearize(struct sk_buff *skb, int gfp);

static inline void *kmap_skb_frag(const skb_frag_t *frag)
{
#if 0
#ifdef CONFIG_HIGHMEM
	if (in_irq())
		out_of_line_bug();

	local_bh_disable();
#endif
	return kmap_atomic(frag->page, KM_SKB_DATA_SOFTIRQ);
#else
  return NULL;
#endif 
}

static inline void kunmap_skb_frag(void *vaddr)
{
#if 0
	kunmap_atomic(vaddr, KM_SKB_DATA_SOFTIRQ);
#ifdef CONFIG_HIGHMEM
	local_bh_enable();
#endif
#endif
}

#define skb_queue_walk(queue, skb) \
		for (skb = (queue)->next;			\
		     (skb != (struct sk_buff *)(queue));	\
		     skb=skb->next)


extern struct sk_buff *		skb_recv_datagram(struct sock *sk,unsigned flags,int noblock, int *err);
extern unsigned int		datagram_poll(struct file *file, struct socket *sock, struct poll_table_struct *wait);
extern int			skb_copy_datagram(const struct sk_buff *from, int offset, char *to,int size);
extern int			skb_copy_datagram_iovec(const struct sk_buff *from, int offset, struct iovec *to,int size);
extern int			skb_copy_and_csum_datagram(const struct sk_buff *skb, int offset, u8 *to, int len, unsigned int *csump);
extern int			skb_copy_and_csum_datagram_iovec(const struct sk_buff *skb, int hlen, struct iovec *iov);
extern void			skb_free_datagram(struct sock * sk, struct sk_buff *skb);

extern unsigned int		skb_checksum(const struct sk_buff *skb, int offset, int len, unsigned int csum);
extern int			skb_copy_bits(const struct sk_buff *skb, int offset, void *to, int len);
extern unsigned int		skb_copy_and_csum_bits(const struct sk_buff *skb, int offset, u8 *to, int len, unsigned int csum);
extern void			skb_copy_and_csum_dev(const struct sk_buff *skb, u8 *to);

extern void skb_init(void);
extern void skb_add_mtu(int mtu);

#ifdef CONFIG_NETFILTER
static inline void
nf_conntrack_put(struct nf_ct_info *nfct)
{
	if (nfct && atomic_dec_and_test(&nfct->master->use))
		nfct->master->destroy(nfct->master);
}
static inline void
nf_conntrack_get(struct nf_ct_info *nfct)
{
	if (nfct)
		atomic_inc(&nfct->master->use);
}
#endif


#endif /* skbuff */





struct sock;

typedef struct sockaddr
{
  int x;
} _sockaddr;


struct msghdr {
	void	*	msg_name;	/* Socket name			*/
	int		msg_namelen;	/* Length of name		*/
	struct iovec *	msg_iov;	/* Data blocks			*/
	__kernel_size_t	msg_iovlen;	/* Number of blocks		*/
	void 	*	msg_control;	/* Per protocol magic (eg BSD file descriptor passing) */
	__kernel_size_t	msg_controllen;	/* Length of cmsg list */
	unsigned	msg_flags;
};


/* IP protocol blocks we attach to sockets.
 * socket layer -> transport layer interface
 * transport -> network interface is defined by struct inet_proto
 */
struct proto {
	void			(*close)(struct sock *sk, 
					long timeout);
	int			(*connect)(struct sock *sk,
				        struct sockaddr *uaddr, 
					int addr_len);
	int			(*disconnect)(struct sock *sk, int flags);

	struct sock *		(*accept) (struct sock *sk, int flags, int *err);

	int			(*ioctl)(struct sock *sk, int cmd,
					 unsigned long arg);
	int			(*init)(struct sock *sk);
	int			(*destroy)(struct sock *sk);
	void			(*shutdown)(struct sock *sk, int how);
	int			(*setsockopt)(struct sock *sk, int level, 
					int optname, char *optval, int optlen);
	int			(*getsockopt)(struct sock *sk, int level, 
					int optname, char *optval, 
					int *option);  	 
	int			(*sendmsg)(struct sock *sk, struct msghdr *msg,
					   int len);
	int			(*recvmsg)(struct sock *sk, struct msghdr *msg,
					int len, int noblock, int flags, 
					int *addr_len);
	int			(*bind)(struct sock *sk, 
					struct sockaddr *uaddr, int addr_len);

	int			(*backlog_rcv) (struct sock *sk, 
						struct sk_buff *skb);

	/* Keeping track of sk's, looking them up, and port selection methods. */
	void			(*hash)(struct sock *sk);
	void			(*unhash)(struct sock *sk);
	int			(*get_port)(struct sock *sk, unsigned short snum);

	char			name[32];

	struct {
		int inuse;
  } stats[32];
//		u8  __pad[SMP_CACHE_BYTES - sizeof(int)];
//	} stats[NR_CPUS];
};







/* This defines a selective acknowledgement block. */
struct tcp_sack_block {
	__u32	start_seq;
	__u32	end_seq;
};


struct tcp_opt {
	int	tcp_header_len;	/* Bytes of tcp header to send		*/

/*
 *	Header prediction flags
 *	0x5?10 << 16 + snd_wnd in net byte order
 */
	__u32	pred_flags;

/*
 *	RFC793 variables by their proper names. This means you can
 *	read the code and the spec side by side (and laugh ...)
 *	See RFC793 and RFC1122. The RFC writes these in capitals.
 */
 	__u32	rcv_nxt;	/* What we want to receive next 	*/
 	__u32	snd_nxt;	/* Next sequence we send		*/

 	__u32	snd_una;	/* First byte we want an ack for	*/
 	__u32	snd_sml;	/* Last byte of the most recently transmitted small packet */
	__u32	rcv_tstamp;	/* timestamp of last received ACK (for keepalives) */
	__u32	lsndtime;	/* timestamp of last sent data packet (for restart window) */

	/* Delayed ACK control data */
	struct {
		__u8	pending;	/* ACK is pending */
		__u8	quick;		/* Scheduled number of quick acks	*/
		__u8	pingpong;	/* The session is interactive		*/
		__u8	blocked;	/* Delayed ACK was blocked by socket lock*/
		__u32	ato;		/* Predicted tick of soft clock		*/
		unsigned long timeout;	/* Currently scheduled timeout		*/
		__u32	lrcvtime;	/* timestamp of last received data packet*/
		__u16	last_seg_size;	/* Size of last incoming segment	*/
		__u16	rcv_mss;	/* MSS used for delayed ACK decisions	*/ 
	} ack;

	/* Data for direct copy to user */
	struct {
		//struct sk_buff_head	prequeue;
		struct task_struct	*task;
		struct iovec		*iov;
		int			memory;
		int			len;
	} ucopy;

	__u32	snd_wl1;	/* Sequence for window update		*/
	__u32	snd_wnd;	/* The window we expect to receive	*/
	__u32	max_window;	/* Maximal window ever seen from peer	*/
	__u32	pmtu_cookie;	/* Last pmtu seen by socket		*/
	__u16	mss_cache;	/* Cached effective mss, not including SACKS */
	__u16	mss_clamp;	/* Maximal mss, negotiated at connection setup */
	__u16	ext_header_len;	/* Network protocol overhead (IP/IPv6 options) */
	__u8	ca_state;	/* State of fast-retransmit machine 	*/
	__u8	retransmits;	/* Number of unrecovered RTO timeouts.	*/

	__u8	reordering;	/* Packet reordering metric.		*/
	__u8	queue_shrunk;	/* Write queue has been shrunk recently.*/
	__u8	defer_accept;	/* User waits for some data after accept() */

/* RTT measurement */
	__u8	backoff;	/* backoff				*/
	__u32	srtt;		/* smothed round trip time << 3		*/
	__u32	mdev;		/* medium deviation			*/
	__u32	mdev_max;	/* maximal mdev for the last rtt period	*/
	__u32	rttvar;		/* smoothed mdev_max			*/
	__u32	rtt_seq;	/* sequence number to update rttvar	*/
	__u32	rto;		/* retransmit timeout			*/

	__u32	packets_out;	/* Packets which are "in flight"	*/
	__u32	left_out;	/* Packets which leaved network		*/
	__u32	retrans_out;	/* Retransmitted packets out		*/


/*
 *	Slow start and congestion control (see also Nagle, and Karn & Partridge)
 */
 	__u32	snd_ssthresh;	/* Slow start size threshold		*/
 	__u32	snd_cwnd;	/* Sending congestion window		*/
 	__u16	snd_cwnd_cnt;	/* Linear increase counter		*/
	__u16	snd_cwnd_clamp; /* Do not allow snd_cwnd to grow above this */
	__u32	snd_cwnd_used;
	__u32	snd_cwnd_stamp;

	/* Two commonly used timers in both sender and receiver paths. */
	unsigned long		timeout;
 	struct timer_list	retransmit_timer;	/* Resend (no ack)	*/
 	struct timer_list	delack_timer;		/* Ack delay 		*/

	struct sk_buff_head	out_of_order_queue; /* Out of order segments go here */

	struct tcp_func		*af_specific;	/* Operations which are AF_INET{4,6} specific	*/
	struct sk_buff		*send_head;	/* Front of stuff to transmit			*/
	struct page		*sndmsg_page;	/* Cached page for sendmsg			*/
	u32			sndmsg_off;	/* Cached offset for sendmsg			*/

 	__u32	rcv_wnd;	/* Current receiver window		*/
	__u32	rcv_wup;	/* rcv_nxt on last window update sent	*/
	__u32	write_seq;	/* Tail(+1) of data held in tcp send buffer */
	__u32	pushed_seq;	/* Last pushed seq, required to talk to windows */
	__u32	copied_seq;	/* Head of yet unread data		*/
/*
 *      Options received (usually on last packet, some only on SYN packets).
 */
	char	tstamp_ok,	/* TIMESTAMP seen on SYN packet		*/
		wscale_ok,	/* Wscale seen on SYN packet		*/
		sack_ok;	/* SACK seen on SYN packet		*/
	char	saw_tstamp;	/* Saw TIMESTAMP on last packet		*/
        __u8	snd_wscale;	/* Window scaling received from sender	*/
        __u8	rcv_wscale;	/* Window scaling to send to receiver	*/
	__u8	nonagle;	/* Disable Nagle algorithm?             */
	__u8	keepalive_probes; /* num of allowed keep alive probes	*/

/*	PAWS/RTTM data	*/
        __u32	rcv_tsval;	/* Time stamp value             	*/
        __u32	rcv_tsecr;	/* Time stamp echo reply        	*/
        __u32	ts_recent;	/* Time stamp to echo next		*/
        long	ts_recent_stamp;/* Time we stored ts_recent (for aging) */

/*	SACKs data	*/
	__u16	user_mss;  	/* mss requested by user in ioctl */
	__u8	dsack;		/* D-SACK is scheduled			*/
	__u8	eff_sacks;	/* Size of SACK array to send with next packet */
	struct tcp_sack_block duplicate_sack[1]; /* D-SACK block */
	struct tcp_sack_block selective_acks[4]; /* The SACKS themselves*/

	__u32	window_clamp;	/* Maximal window to advertise		*/
	__u32	rcv_ssthresh;	/* Current window clamp			*/
	__u8	probes_out;	/* unanswered 0 window probes		*/
	__u8	num_sacks;	/* Number of SACK blocks		*/
	__u16	advmss;		/* Advertised MSS			*/

	__u8	syn_retries;	/* num of allowed syn retries */
	__u8	ecn_flags;	/* ECN status bits.			*/
	__u16	prior_ssthresh; /* ssthresh saved at recovery start	*/
	__u32	lost_out;	/* Lost packets				*/
	__u32	sacked_out;	/* SACK'd packets			*/
	__u32	fackets_out;	/* FACK'd packets			*/
	__u32	high_seq;	/* snd_nxt at onset of congestion	*/

	__u32	retrans_stamp;	/* Timestamp of the last retransmit,
				 * also used in SYN-SENT to remember stamp of
				 * the first SYN. */
	__u32	undo_marker;	/* tracking retrans started here. */
	int	undo_retrans;	/* number of undoable retransmissions. */
	__u32	urg_seq;	/* Seq of received urgent pointer */
	__u16	urg_data;	/* Saved octet of OOB data and control flags */
	__u8	pending;	/* Scheduled timer event	*/
	__u8	urg_mode;	/* In urgent mode		*/
	__u32	snd_up;		/* Urgent pointer		*/

	/* The syn_wait_lock is necessary only to avoid tcp_get_info having
	 * to grab the main lock sock while browsing the listening hash
	 * (otherwise it's deadlock prone).
	 * This lock is acquired in read mode only from tcp_get_info() and
	 * it's acquired in write mode _only_ from code that is actively
	 * changing the syn_wait_queue. All readers that are holding
	 * the master sock lock don't need to grab this lock in read mode
	 * too as the syn_wait_queue writes are always protected from
	 * the main sock lock.
	 */
	rwlock_t		syn_wait_lock;
	struct tcp_listen_opt	*listen_opt;

	/* FIFO of established children */
	struct open_request	*accept_queue;
	struct open_request	*accept_queue_tail;

	int			write_pending;	/* A write to socket waits to start. */

	unsigned int		keepalive_time;	  /* time before keep alive takes place */
	unsigned int		keepalive_intvl;  /* time interval between keep alive probes */
	int			linger2;

	unsigned long last_synq_overflow; 
};




/* This is the per-socket lock.  The spinlock provides a synchronization
 * between user contexts and software interrupt processing, whereas the
 * mini-semaphore synchronizes multiple users amongst themselves.
 */
typedef struct {
	spinlock_t		slock;
	unsigned int		users;
	wait_queue_head_t	wq;
} socket_lock_t;

struct sock {
	/* Socket demultiplex comparisons on incoming packets. */
	__u32			daddr;		/* Foreign IPv4 addr			*/
	__u32			rcv_saddr;	/* Bound local IPv4 addr		*/
	__u16			dport;		/* Destination port			*/
	unsigned short		num;		/* Local port				*/
	int			bound_dev_if;	/* Bound device index if != 0		*/

	/* Main hash linkage for various protocol lookup tables. */
	struct sock		*next;
	struct sock		**pprev;
	struct sock		*bind_next;
	struct sock		**bind_pprev;

	volatile unsigned char	state,		/* Connection state			*/
				zapped;		/* In ax25 & ipx means not linked	*/
	__u16			sport;		/* Source port				*/

	unsigned short		family;		/* Address family			*/
	unsigned char		reuse;		/* SO_REUSEADDR setting			*/
	unsigned char		shutdown;
	atomic_t		refcnt;		/* Reference count			*/

	socket_lock_t		lock;		/* Synchronizer...			*/
	int			rcvbuf;		/* Size of receive buffer in bytes	*/

	wait_queue_head_t	*sleep;		/* Sock wait queue			*/
	struct dst_entry	*dst_cache;	/* Destination cache			*/
	rwlock_t		dst_lock;
	atomic_t		rmem_alloc;	/* Receive queue bytes committed	*/
	struct sk_buff_head	receive_queue;	/* Incoming packets			*/
	atomic_t		wmem_alloc;	/* Transmit queue bytes committed	*/
	struct sk_buff_head	write_queue;	/* Packet sending queue			*/
	atomic_t		omem_alloc;	/* "o" is "option" or "other" */
	int			wmem_queued;	/* Persistent queue size */
	int			forward_alloc;	/* Space allocated forward. */
	__u32			saddr;		/* Sending source			*/
	unsigned int		allocation;	/* Allocation mode			*/
	int			sndbuf;		/* Size of send buffer in bytes		*/
	struct sock		*prev;

	/* Not all are volatile, but some are, so we might as well say they all are.
	 * XXX Make this a flag word -DaveM
	 */
	volatile char		dead,
				done,
				urginline,
				keepopen,
				linger,
				destroy,
				no_check,
				broadcast,
				bsdism;
	unsigned char		debug;
	unsigned char		rcvtstamp;
	unsigned char		use_write_queue;
	unsigned char		userlocks;
	/* Hole of 3 bytes. Try to pack. */
	int			route_caps;
	int			proc;
	unsigned long	        lingertime;

	int			hashent;
	struct sock		*pair;

	/* The backlog queue is special, it is always used with
	 * the per-socket spinlock held and requires low latency
	 * access.  Therefore we special case it's implementation.
	 */
	struct {
		struct sk_buff *head;
		struct sk_buff *tail;
	} backlog;

	rwlock_t		callback_lock;

	/* Error queue, rarely used. */
	struct sk_buff_head	error_queue;

	struct proto		*prot;

#if defined(CONFIG_IPV6) || defined (CONFIG_IPV6_MODULE)
	union {
		struct ipv6_pinfo	af_inet6;
	} net_pinfo;
#endif

	union {
		struct tcp_opt		af_tcp;
#if defined(CONFIG_INET) || defined (CONFIG_INET_MODULE)
		struct raw_opt		tp_raw4;
#endif
#if defined(CONFIG_IPV6) || defined (CONFIG_IPV6_MODULE)
		struct raw6_opt		tp_raw;
#endif /* CONFIG_IPV6 */
#if defined(CONFIG_SPX) || defined (CONFIG_SPX_MODULE)
		struct spx_opt		af_spx;
#endif /* CONFIG_SPX */

	} tp_pinfo;

	int			err, err_soft;	/* Soft holds errors that don't
						   cause failure but are the cause
						   of a persistent failure not just
						   'timed out' */
	unsigned short		ack_backlog;
	unsigned short		max_ack_backlog;
	__u32			priority;
	unsigned short		type;
	unsigned char		localroute;	/* Route locally only */
	unsigned char		protocol;
//	struct ucred		peercred;
	int			rcvlowat;
	long			rcvtimeo;
	long			sndtimeo;

#ifdef CONFIG_FILTER
	/* Socket Filtering Instructions */
	struct sk_filter      	*filter;
#endif /* CONFIG_FILTER */

	/* This is where all the private (optional) areas that don't
	 * overlap will eventually live. 
	 */
	union {
		void *destruct_hook;
//	  	struct unix_opt	af_unix;
#if defined(CONFIG_INET) || defined (CONFIG_INET_MODULE)
		struct inet_opt af_inet;
#endif
#if defined(CONFIG_ATALK) || defined(CONFIG_ATALK_MODULE)
		struct atalk_sock	af_at;
#endif
#if defined(CONFIG_IPX) || defined(CONFIG_IPX_MODULE)
		struct ipx_opt		af_ipx;
#endif
#if defined (CONFIG_DECNET) || defined(CONFIG_DECNET_MODULE)
		struct dn_scp           dn;
#endif
#if defined (CONFIG_PACKET) || defined(CONFIG_PACKET_MODULE)
		struct packet_opt	*af_packet;
#endif
#if defined(CONFIG_X25) || defined(CONFIG_X25_MODULE)
		x25_cb			*x25;
#endif
#if defined(CONFIG_AX25) || defined(CONFIG_AX25_MODULE)
		ax25_cb			*ax25;
#endif
#if defined(CONFIG_NETROM) || defined(CONFIG_NETROM_MODULE)
		nr_cb			*nr;
#endif
#if defined(CONFIG_ROSE) || defined(CONFIG_ROSE_MODULE)
		rose_cb			*rose;
#endif
#if defined(CONFIG_PPPOE) || defined(CONFIG_PPPOE_MODULE)
		struct pppox_opt	*pppox;
#endif
		struct netlink_opt	*af_netlink;
#if defined(CONFIG_ECONET) || defined(CONFIG_ECONET_MODULE)
		struct econet_opt	*af_econet;
#endif
#if defined(CONFIG_ATM) || defined(CONFIG_ATM_MODULE)
		struct atm_vcc		*af_atm;
#endif
#if defined(CONFIG_IRDA) || defined(CONFIG_IRDA_MODULE)
		struct irda_sock        *irda;
#endif
#if defined(CONFIG_WAN_ROUTER) || defined(CONFIG_WAN_ROUTER_MODULE)
               struct wanpipe_opt      *af_wanpipe;
#endif
	} protinfo;  		


	/* This part is used for the timeout functions. */
	struct timer_list	timer;		/* This is the sock cleanup timer. */
	struct timeval		stamp;

	/* Identd and reporting IO signals */
	struct socket		*socket;

	/* RPC layer private data */
	void			*user_data;
  
	/* Callbacks */
	void			(*state_change)(struct sock *sk);
	void			(*data_ready)(struct sock *sk,int bytes);
	void			(*write_space)(struct sock *sk);
	void			(*error_report)(struct sock *sk);

  	int			(*backlog_rcv) (struct sock *sk,
						struct sk_buff *skb);  
	void                    (*destruct)(struct sock *sk);
};




#if 1 /* dst (_NET_DST_H) */

#if 0
#include <linux/config.h>
#include <net/neighbour.h>
#endif

/*
 * 0 - no debugging messages
 * 1 - rare events and bugs (default)
 * 2 - trace mode.
 */
#define RT_CACHE_DEBUG		0

#define DST_GC_MIN	(1*HZ)
#define DST_GC_INC	(5*HZ)
#define DST_GC_MAX	(120*HZ)

struct sk_buff;

struct dst_entry
{
	struct dst_entry        *next;
	atomic_t		__refcnt;	/* client references	*/
	int			__use;
	struct net_device       *dev;
	int			obsolete;
	int			flags;
#define DST_HOST		1
	unsigned long		lastuse;
	unsigned long		expires;

	unsigned		mxlock;
	unsigned		pmtu;
	unsigned		window;
	unsigned		rtt;
	unsigned		rttvar;
	unsigned		ssthresh;
	unsigned		cwnd;
	unsigned		advmss;
	unsigned		reordering;

	unsigned long		rate_last;	/* rate limiting for ICMP */
	unsigned long		rate_tokens;

	int			error;

	struct neighbour	*neighbour;
	struct hh_cache		*hh;

	int			(*input)(struct sk_buff*);
	int			(*output)(struct sk_buff*);

#ifdef CONFIG_NET_CLS_ROUTE
	__u32			tclassid;
#endif

	struct  dst_ops	        *ops;
		
	char			info[0];
};


struct dst_ops
{
	unsigned short		family;
	unsigned short		protocol;
	unsigned		gc_thresh;

	int			(*gc)(void);
	struct dst_entry *	(*check)(struct dst_entry *, __u32 cookie);
	struct dst_entry *	(*reroute)(struct dst_entry *,
					   struct sk_buff *);
	void			(*destroy)(struct dst_entry *);
	struct dst_entry *	(*negative_advice)(struct dst_entry *);
	void			(*link_failure)(struct sk_buff *);
	int			entry_size;

	atomic_t		entries;
	kmem_cache_t 		*kmem_cachep;
};

#ifdef __KERNEL__

static inline void dst_hold(struct dst_entry * dst)
{
	atomic_inc(&dst->__refcnt);
}

static inline
struct dst_entry * dst_clone(struct dst_entry * dst)
{
	if (dst)
		atomic_inc(&dst->__refcnt);
	return dst;
}

static inline
void dst_release(struct dst_entry * dst)
{
	if (dst)
		atomic_dec(&dst->__refcnt);
}

extern void * dst_alloc(struct dst_ops * ops);
extern void __dst_free(struct dst_entry * dst);
extern void dst_destroy(struct dst_entry * dst);

static inline
void dst_free(struct dst_entry * dst)
{
	if (dst->obsolete > 1)
		return;
	if (!atomic_read(&dst->__refcnt)) {
		dst_destroy(dst);
		return;
	}
	__dst_free(dst);
}

static inline void dst_confirm(struct dst_entry *dst)
{
	if (dst)
		neigh_confirm(dst->neighbour);
}

static inline void dst_negative_advice(struct dst_entry **dst_p)
{
	struct dst_entry * dst = *dst_p;
	if (dst && dst->ops->negative_advice)
		*dst_p = dst->ops->negative_advice(dst);
}

static inline void dst_link_failure(struct sk_buff *skb)
{
	struct dst_entry * dst = skb->dst;
	if (dst && dst->ops && dst->ops->link_failure)
		dst->ops->link_failure(skb);
}

static inline void dst_set_expires(struct dst_entry *dst, int timeout)
{
	unsigned long expires = jiffies + timeout;

	if (expires == 0)
		expires = 1;

	if (dst->expires == 0 || (long)(dst->expires - expires) > 0)
		dst->expires = expires;
}

extern void		dst_init(void);

#endif /* dst */



#if 1
/* dummy types */


#endif

#define TCP_DEBUG 1
#define FASTRETRANS_DEBUG 1

/* Cancel timers, when they are not required. */
#undef TCP_CLEAR_TIMERS

#if 0
#include <linux/config.h>
#include <linux/tcp.h>
#include <linux/slab.h>
#include <linux/cache.h>
#include <net/checksum.h>
#include <net/sock.h>
#else
#include "linux.h"
#endif

/* This is for all connections with a full identity, no wildcards.
 * New scheme, half the table is for TIME_WAIT, the other half is
 * for the rest.  I'll experiment with dynamic table growth later.
 */
struct tcp_ehash_bucket {
	rwlock_t	lock;
	struct sock	*chain;
} __attribute__((__aligned__(8)));

/* This is for listening sockets, thus all sockets which possess wildcards. */
#define TCP_LHTABLE_SIZE	32	/* Yes, really, this is all you need. */

/* There are a few simple rules, which allow for local port reuse by
 * an application.  In essence:
 *
 *	1) Sockets bound to different interfaces may share a local port.
 *	   Failing that, goto test 2.
 *	2) If all sockets have sk->reuse set, and none of them are in
 *	   TCP_LISTEN state, the port may be shared.
 *	   Failing that, goto test 3.
 *	3) If all sockets are bound to a specific sk->rcv_saddr local
 *	   address, and none of them are the same, the port may be
 *	   shared.
 *	   Failing this, the port cannot be shared.
 *
 * The interesting point, is test #2.  This is what an FTP server does
 * all day.  To optimize this case we use a specific flag bit defined
 * below.  As we add sockets to a bind bucket list, we perform a
 * check of: (newsk->reuse && (newsk->state != TCP_LISTEN))
 * As long as all sockets added to a bind bucket pass this test,
 * the flag bit will be set.
 * The resulting situation is that tcp_v[46]_verify_bind() can just check
 * for this flag bit, if it is set and the socket trying to bind has
 * sk->reuse set, we don't even have to walk the owners list at all,
 * we return that it is ok to bind this socket to the requested local port.
 *
 * Sounds like a lot of work, but it is worth it.  In a more naive
 * implementation (ie. current FreeBSD etc.) the entire list of ports
 * must be walked for each data port opened by an ftp server.  Needless
 * to say, this does not scale at all.  With a couple thousand FTP
 * users logged onto your box, isn't it nice to know that new data
 * ports are created in O(1) time?  I thought so. ;-)	-DaveM
 */
struct tcp_bind_bucket {
	unsigned short		port;
	signed short		fastreuse;
	struct tcp_bind_bucket	*next;
	struct sock		*owners;
	struct tcp_bind_bucket	**pprev;
};

struct tcp_bind_hashbucket {
	spinlock_t		lock;
	struct tcp_bind_bucket	*chain;
};

extern struct tcp_hashinfo {
	/* This is for sockets with full identity only.  Sockets here will
	 * always be without wildcards and will have the following invariant:
	 *
	 *          TCP_ESTABLISHED <= sk->state < TCP_CLOSE
	 *
	 * First half of the table is for sockets not in TIME_WAIT, second half
	 * is for TIME_WAIT sockets only.
	 */
	struct tcp_ehash_bucket *__tcp_ehash;

	/* Ok, let's try this, I give up, we do need a local binding
	 * TCP hash as well as the others for fast bind/connect.
	 */
	struct tcp_bind_hashbucket *__tcp_bhash;

	int __tcp_bhash_size;
	int __tcp_ehash_size;

	/* All sockets in TCP_LISTEN state will be in here.  This is the only
	 * table where wildcard'd TCP sockets can exist.  Hash function here
	 * is just local port number.
	 */
	struct sock *__tcp_listening_hash[TCP_LHTABLE_SIZE];

	/* All the above members are written once at bootup and
	 * never written again _or_ are predominantly read-access.
	 *
	 * Now align to a new cache line as all the following members
	 * are often dirty.
	 */
	rwlock_t __tcp_lhash_lock ____cacheline_aligned;
	atomic_t __tcp_lhash_users;
	wait_queue_head_t __tcp_lhash_wait;
	spinlock_t __tcp_portalloc_lock;
} tcp_hashinfo;

#define tcp_ehash	(tcp_hashinfo.__tcp_ehash)
#define tcp_bhash	(tcp_hashinfo.__tcp_bhash)
#define tcp_ehash_size	(tcp_hashinfo.__tcp_ehash_size)
#define tcp_bhash_size	(tcp_hashinfo.__tcp_bhash_size)
#define tcp_listening_hash (tcp_hashinfo.__tcp_listening_hash)
#define tcp_lhash_lock	(tcp_hashinfo.__tcp_lhash_lock)
#define tcp_lhash_users	(tcp_hashinfo.__tcp_lhash_users)
#define tcp_lhash_wait	(tcp_hashinfo.__tcp_lhash_wait)
#define tcp_portalloc_lock (tcp_hashinfo.__tcp_portalloc_lock)

extern kmem_cache_t *tcp_bucket_cachep;
extern struct tcp_bind_bucket *tcp_bucket_create(struct tcp_bind_hashbucket *head,
						 unsigned short snum);
extern void tcp_bucket_unlock(struct sock *sk);
extern int tcp_port_rover;
extern struct sock *tcp_v4_lookup_listener(u32 addr, unsigned short hnum, int dif);

/* These are AF independent. */
static __inline__ int tcp_bhashfn(__u16 lport)
{
	return (lport & (tcp_bhash_size - 1));
}

/* This is a TIME_WAIT bucket.  It works around the memory consumption
 * problems of sockets in such a state on heavily loaded servers, but
 * without violating the protocol specification.
 */
struct tcp_tw_bucket {
	/* These _must_ match the beginning of struct sock precisely.
	 * XXX Yes I know this is gross, but I'd have to edit every single
	 * XXX networking file if I created a "struct sock_header". -DaveM
	 */
	__u32			daddr;
	__u32			rcv_saddr;
	__u16			dport;
	unsigned short		num;
	int			bound_dev_if;
	struct sock		*next;
	struct sock		**pprev;
	struct sock		*bind_next;
	struct sock		**bind_pprev;
	unsigned char		state,
				substate; /* "zapped" is replaced with "substate" */
	__u16			sport;
	unsigned short		family;
	unsigned char		reuse,
				rcv_wscale; /* It is also TW bucket specific */
	atomic_t		refcnt;

	/* And these are ours. */
	int			hashent;
	int			timeout;
	__u32			rcv_nxt;
	__u32			snd_nxt;
	__u32			rcv_wnd;
        __u32			ts_recent;
        long			ts_recent_stamp;
	unsigned long		ttd;
	struct tcp_bind_bucket	*tb;
	struct tcp_tw_bucket	*next_death;
	struct tcp_tw_bucket	**pprev_death;

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	struct in6_addr		v6_daddr;
	struct in6_addr		v6_rcv_saddr;
#endif
};

extern kmem_cache_t *tcp_timewait_cachep;

static inline void tcp_tw_put(struct tcp_tw_bucket *tw)
{
	if (atomic_dec_and_test(&tw->refcnt)) {
#ifdef INET_REFCNT_DEBUG
		printk(KERN_DEBUG "tw_bucket %p released\n", tw);
#endif
		kmem_cache_free(tcp_timewait_cachep, tw);
	}
}

extern atomic_t tcp_orphan_count;
extern int tcp_tw_count;
extern void tcp_time_wait(struct sock *sk, int state, int timeo);
extern void tcp_timewait_kill(struct tcp_tw_bucket *tw);
extern void tcp_tw_schedule(struct tcp_tw_bucket *tw, int timeo);
extern void tcp_tw_deschedule(struct tcp_tw_bucket *tw);


/* Socket demux engine toys. */
#ifdef __BIG_ENDIAN
#define TCP_COMBINED_PORTS(__sport, __dport) \
	(((__u32)(__sport)<<16) | (__u32)(__dport))
#else /* __LITTLE_ENDIAN */
#define TCP_COMBINED_PORTS(__sport, __dport) \
	(((__u32)(__dport)<<16) | (__u32)(__sport))
#endif

#if (BITS_PER_LONG == 64)
#ifdef __BIG_ENDIAN
#define TCP_V4_ADDR_COOKIE(__name, __saddr, __daddr) \
	__u64 __name = (((__u64)(__saddr))<<32)|((__u64)(__daddr));
#else /* __LITTLE_ENDIAN */
#define TCP_V4_ADDR_COOKIE(__name, __saddr, __daddr) \
	__u64 __name = (((__u64)(__daddr))<<32)|((__u64)(__saddr));
#endif /* __BIG_ENDIAN */
#define TCP_IPV4_MATCH(__sk, __cookie, __saddr, __daddr, __ports, __dif)\
	(((*((__u64 *)&((__sk)->daddr)))== (__cookie))	&&		\
	 ((*((__u32 *)&((__sk)->dport)))== (__ports))   &&		\
	 (!((__sk)->bound_dev_if) || ((__sk)->bound_dev_if == (__dif))))
#else /* 32-bit arch */
#define TCP_V4_ADDR_COOKIE(__name, __saddr, __daddr)
#define TCP_IPV4_MATCH(__sk, __cookie, __saddr, __daddr, __ports, __dif)\
	(((__sk)->daddr			== (__saddr))	&&		\
	 ((__sk)->rcv_saddr		== (__daddr))	&&		\
	 ((*((__u32 *)&((__sk)->dport)))== (__ports))   &&		\
	 (!((__sk)->bound_dev_if) || ((__sk)->bound_dev_if == (__dif))))
#endif /* 64-bit arch */

#define TCP_IPV6_MATCH(__sk, __saddr, __daddr, __ports, __dif)			   \
	(((*((__u32 *)&((__sk)->dport)))== (__ports))   			&& \
	 ((__sk)->family		== AF_INET6)				&& \
	 !ipv6_addr_cmp(&(__sk)->net_pinfo.af_inet6.daddr, (__saddr))		&& \
	 !ipv6_addr_cmp(&(__sk)->net_pinfo.af_inet6.rcv_saddr, (__daddr))	&& \
	 (!((__sk)->bound_dev_if) || ((__sk)->bound_dev_if == (__dif))))

/* These can have wildcards, don't try too hard. */
static __inline__ int tcp_lhashfn(unsigned short num)
{
#if 0
	return num & (TCP_LHTABLE_SIZE - 1);
#else
  return 0;
#endif
}

static __inline__ int tcp_sk_listen_hashfn(struct sock *sk)
{
#if 0
	return tcp_lhashfn(sk->num);
#else
  return 0;
#endif
}

#define MAX_TCP_HEADER	(128 + MAX_HEADER)

/* 
 * Never offer a window over 32767 without using window scaling. Some
 * poor stacks do signed 16bit maths! 
 */
#define MAX_TCP_WINDOW		32767U

/* Minimal accepted MSS. It is (60+60+8) - (20+20). */
#define TCP_MIN_MSS		88U

/* Minimal RCV_MSS. */
#define TCP_MIN_RCVMSS		536U

/* After receiving this amount of duplicate ACKs fast retransmit starts. */
#define TCP_FASTRETRANS_THRESH 3

/* Maximal reordering. */
#define TCP_MAX_REORDERING	127

/* Maximal number of ACKs sent quickly to accelerate slow-start. */
#define TCP_MAX_QUICKACKS	16U

/* urg_data states */
#define TCP_URG_VALID	0x0100
#define TCP_URG_NOTYET	0x0200
#define TCP_URG_READ	0x0400

#define TCP_RETR1	3	/*
				 * This is how many retries it does before it
				 * tries to figure out if the gateway is
				 * down. Minimal RFC value is 3; it corresponds
				 * to ~3sec-8min depending on RTO.
				 */

#define TCP_RETR2	15	/*
				 * This should take at least
				 * 90 minutes to time out.
				 * RFC1122 says that the limit is 100 sec.
				 * 15 is ~13-30min depending on RTO.
				 */

#define TCP_SYN_RETRIES	 5	/* number of times to retry active opening a
				 * connection: ~180sec is RFC minumum	*/

#define TCP_SYNACK_RETRIES 5	/* number of times to retry passive opening a
				 * connection: ~180sec is RFC minumum	*/


#define TCP_ORPHAN_RETRIES 7	/* number of times to retry on an orphaned
				 * socket. 7 is ~50sec-16min.
				 */


#define TCP_TIMEWAIT_LEN (60*1000)
//#define TCP_TIMEWAIT_LEN (60*HZ)
/* how long to wait to destroy TIME-WAIT
				  * state, about 60 seconds	*/
#define TCP_FIN_TIMEOUT	TCP_TIMEWAIT_LEN
                                 /* BSD style FIN_WAIT2 deadlock breaker.
				  * It used to be 3min, new value is 60sec,
				  * to combine FIN-WAIT-2 timeout with
				  * TIME-WAIT timer.
				  */

#define TCP_DELACK_MAX	((unsigned)(HZ/5))	/* maximal time to delay before sending an ACK */
#if HZ >= 100
#define TCP_DELACK_MIN	((unsigned)(HZ/25))	/* minimal time to delay before sending an ACK */
#define TCP_ATO_MIN	((unsigned)(HZ/25))
#else
#define TCP_DELACK_MIN	4U
#define TCP_ATO_MIN	4U
#endif
#define TCP_RTO_MAX	((unsigned)(120*HZ))
#define TCP_RTO_MIN	((unsigned)(HZ/5))
#define TCP_TIMEOUT_INIT ((unsigned)(3*HZ))	/* RFC 1122 initial RTO value	*/

#define TCP_RESOURCE_PROBE_INTERVAL ((unsigned)(HZ/2U)) /* Maximal interval between probes
					                 * for local resources.
					                 */

#define TCP_KEEPALIVE_TIME	(120*60*HZ)	/* two hours */
#define TCP_KEEPALIVE_PROBES	9		/* Max of 9 keepalive probes	*/
#define TCP_KEEPALIVE_INTVL	(75*HZ)

#define MAX_TCP_KEEPIDLE	32767
#define MAX_TCP_KEEPINTVL	32767
#define MAX_TCP_KEEPCNT		127
#define MAX_TCP_SYNCNT		127

/* TIME_WAIT reaping mechanism. */
#define TCP_TWKILL_SLOTS	8	/* Please keep this a power of 2. */
#define TCP_TWKILL_PERIOD	(TCP_TIMEWAIT_LEN/TCP_TWKILL_SLOTS)

#define TCP_SYNQ_INTERVAL	(HZ/5)	/* Period of SYNACK timer */
#define TCP_SYNQ_HSIZE		512	/* Size of SYNACK hash table */

#define TCP_PAWS_24DAYS	(60 * 60 * 24 * 24)
#define TCP_PAWS_MSL	60		/* Per-host timestamps are invalidated
					 * after this time. It should be equal
					 * (or greater than) TCP_TIMEWAIT_LEN
					 * to provide reliability equal to one
					 * provided by timewait state.
					 */
#define TCP_PAWS_WINDOW	1		/* Replay window for per-host
					 * timestamps. It must be less than
					 * minimal timewait lifetime.
					 */

#define TCP_TW_RECYCLE_SLOTS_LOG	5
#define TCP_TW_RECYCLE_SLOTS		(1<<TCP_TW_RECYCLE_SLOTS_LOG)

/* If time > 4sec, it is "slow" path, no recycling is required,
   so that we select tick to get range about 4 seconds.
 */

#if 0
#if HZ <= 16 || HZ > 4096
# error Unsupported: HZ <= 16 or HZ > 4096
#elif HZ <= 32
# define TCP_TW_RECYCLE_TICK (5+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 64
# define TCP_TW_RECYCLE_TICK (6+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 128
# define TCP_TW_RECYCLE_TICK (7+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 256
# define TCP_TW_RECYCLE_TICK (8+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 512
# define TCP_TW_RECYCLE_TICK (9+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 1024
# define TCP_TW_RECYCLE_TICK (10+2-TCP_TW_RECYCLE_SLOTS_LOG)
#elif HZ <= 2048
# define TCP_TW_RECYCLE_TICK (11+2-TCP_TW_RECYCLE_SLOTS_LOG)
#else
# define TCP_TW_RECYCLE_TICK (12+2-TCP_TW_RECYCLE_SLOTS_LOG)
#endif
#else
#define TCP_TW_RECYCLE_TICK (0)
#endif

/*
 *	TCP option
 */
 
#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_SACK_PERM        4       /* SACK Permitted */
#define TCPOPT_SACK             5       /* SACK Block */
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */

/*
 *     TCP option lengths
 */

#define TCPOLEN_MSS            4
#define TCPOLEN_WINDOW         3
#define TCPOLEN_SACK_PERM      2
#define TCPOLEN_TIMESTAMP      10

/* But this is what stacks really send out. */
#define TCPOLEN_TSTAMP_ALIGNED		12
#define TCPOLEN_WSCALE_ALIGNED		4
#define TCPOLEN_SACKPERM_ALIGNED	4
#define TCPOLEN_SACK_BASE		2
#define TCPOLEN_SACK_BASE_ALIGNED	4
#define TCPOLEN_SACK_PERBLOCK		8

#define TCP_TIME_RETRANS	1	/* Retransmit timer */
#define TCP_TIME_DACK		2	/* Delayed ack timer */
#define TCP_TIME_PROBE0		3	/* Zero window probe timer */
#define TCP_TIME_KEEPOPEN	4	/* Keepalive timer */

#if 0
/* sysctl variables for tcp */
extern int sysctl_max_syn_backlog;
extern int sysctl_tcp_timestamps;
extern int sysctl_tcp_window_scaling;
extern int sysctl_tcp_sack;
extern int sysctl_tcp_fin_timeout;
extern int sysctl_tcp_tw_recycle;
extern int sysctl_tcp_keepalive_time;
extern int sysctl_tcp_keepalive_probes;
extern int sysctl_tcp_keepalive_intvl;
extern int sysctl_tcp_syn_retries;
extern int sysctl_tcp_synack_retries;
extern int sysctl_tcp_retries1;
extern int sysctl_tcp_retries2;
extern int sysctl_tcp_orphan_retries;
extern int sysctl_tcp_syncookies;
extern int sysctl_tcp_retrans_collapse;
extern int sysctl_tcp_stdurg;
extern int sysctl_tcp_rfc1337;
extern int sysctl_tcp_abort_on_overflow;
extern int sysctl_tcp_max_orphans;
extern int sysctl_tcp_max_tw_buckets;
extern int sysctl_tcp_fack;
extern int sysctl_tcp_reordering;
extern int sysctl_tcp_ecn;
extern int sysctl_tcp_dsack;
extern int sysctl_tcp_mem[3];
extern int sysctl_tcp_wmem[3];
extern int sysctl_tcp_rmem[3];
extern int sysctl_tcp_app_win;
extern int sysctl_tcp_adv_win_scale;
extern int sysctl_tcp_tw_reuse;
#endif

extern atomic_t tcp_memory_allocated;
extern atomic_t tcp_sockets_allocated;
extern int tcp_memory_pressure;

struct open_request;

struct or_calltable {
	int  family;
	int  (*rtx_syn_ack)	(struct sock *sk, struct open_request *req, struct dst_entry*);
	void (*send_ack)	(struct sk_buff *skb, struct open_request *req);
	void (*destructor)	(struct open_request *req);
	void (*send_reset)	(struct sk_buff *skb);
};

struct tcp_v4_open_req {
	__u32			loc_addr;
	__u32			rmt_addr;
	struct ip_options	*opt;
};

#if defined(CONFIG_IPV6) || defined (CONFIG_IPV6_MODULE)
struct tcp_v6_open_req {
	struct in6_addr		loc_addr;
	struct in6_addr		rmt_addr;
	struct sk_buff		*pktopts;
	int			iif;
};
#endif

/* this structure is too big */
struct open_request {
	struct open_request	*dl_next; /* Must be first member! */
	__u32			rcv_isn;
	__u32			snt_isn;
	__u16			rmt_port;
	__u16			mss;
	__u8			retrans;
	__u8			__pad;
	__u16	snd_wscale : 4, 
		rcv_wscale : 4, 
		tstamp_ok : 1,
		sack_ok : 1,
		wscale_ok : 1,
		ecn_ok : 1,
		acked : 1;
	/* The following two fields can be easily recomputed I think -AK */
	__u32			window_clamp;	/* window clamp at creation time */
	__u32			rcv_wnd;	/* rcv_wnd offered first time */
	__u32			ts_recent;
	unsigned long		expires;
	struct or_calltable	*class;
	struct sock		*sk;
	union {
		struct tcp_v4_open_req v4_req;
#if defined(CONFIG_IPV6) || defined (CONFIG_IPV6_MODULE)
		struct tcp_v6_open_req v6_req;
#endif
	} af;
};

/* SLAB cache for open requests. */
extern kmem_cache_t *tcp_openreq_cachep;

#define tcp_openreq_alloc()		kmem_cache_alloc(tcp_openreq_cachep, SLAB_ATOMIC)
#define tcp_openreq_fastfree(req)	kmem_cache_free(tcp_openreq_cachep, req)

static inline void tcp_openreq_free(struct open_request *req)
{
	req->class->destructor(req);
	tcp_openreq_fastfree(req);
}

#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
#define TCP_INET_FAMILY(fam) ((fam) == AF_INET)
#else
#define TCP_INET_FAMILY(fam) 1
#endif

/*
 *	Pointers to address related TCP functions
 *	(i.e. things that depend on the address family)
 *
 * 	BUGGG_FUTURE: all the idea behind this struct is wrong.
 *	It mixes socket frontend with transport function.
 *	With port sharing between IPv6/v4 it gives the only advantage,
 *	only poor IPv6 needs to permanently recheck, that it
 *	is still IPv6 8)8) It must be cleaned up as soon as possible.
 *						--ANK (980802)
 */

struct tcp_func {
	int			(*queue_xmit)		(struct sk_buff *skb);

	void			(*send_check)		(struct sock *sk,
							 struct tcphdr *th,
							 int len,
							 struct sk_buff *skb);

	int			(*rebuild_header)	(struct sock *sk);

	int			(*conn_request)		(struct sock *sk,
							 struct sk_buff *skb);

	struct sock *		(*syn_recv_sock)	(struct sock *sk,
							 struct sk_buff *skb,
							 struct open_request *req,
							 struct dst_entry *dst);
    
	int			(*remember_stamp)	(struct sock *sk);

	__u16			net_header_len;

	int			(*setsockopt)		(struct sock *sk, 
							 int level, 
							 int optname, 
							 char *optval, 
							 int optlen);

	int			(*getsockopt)		(struct sock *sk, 
							 int level, 
							 int optname, 
							 char *optval, 
							 int *optlen);


	void			(*addr2sockaddr)	(struct sock *sk,
							 struct sockaddr *);

	int sockaddr_len;
};

/*
 * The next routines deal with comparing 32 bit unsigned ints
 * and worry about wraparound (automatic with unsigned arithmetic).
 */

extern __inline int before(__u32 seq1, __u32 seq2)
{
        return (__s32)(seq1-seq2) < 0;
}

extern __inline int after(__u32 seq1, __u32 seq2)
{
	return (__s32)(seq2-seq1) < 0;
}


/* is s2<=s1<=s3 ? */
extern __inline int between(__u32 seq1, __u32 seq2, __u32 seq3)
{
	return seq3 - seq2 >= seq1 - seq2;
}


extern struct proto tcp_prot;

#ifdef ROS_STATISTICS
extern struct tcp_mib tcp_statistics[NR_CPUS*2];

#define TCP_INC_STATS(field)		SNMP_INC_STATS(tcp_statistics, field)
#define TCP_INC_STATS_BH(field)		SNMP_INC_STATS_BH(tcp_statistics, field)
#define TCP_INC_STATS_USER(field) 	SNMP_INC_STATS_USER(tcp_statistics, field)
#endif

extern void			tcp_put_port(struct sock *sk);
extern void			__tcp_put_port(struct sock *sk);
extern void			tcp_inherit_port(struct sock *sk, struct sock *child);

extern void			tcp_v4_err(struct sk_buff *skb, u32);

extern void			tcp_shutdown (struct sock *sk, int how);

extern int			tcp_v4_rcv(struct sk_buff *skb);

extern int			tcp_v4_remember_stamp(struct sock *sk);

extern int		    	tcp_v4_tw_remember_stamp(struct tcp_tw_bucket *tw);

extern int			tcp_sendmsg(struct sock *sk, struct msghdr *msg, int size);
extern ssize_t			tcp_sendpage(struct socket *sock, struct page *page, int offset, size_t size, int flags);

extern int			tcp_ioctl(struct sock *sk, 
					  int cmd, 
					  unsigned long arg);

extern int			tcp_rcv_state_process(struct sock *sk, 
						      struct sk_buff *skb,
						      struct tcphdr *th,
						      unsigned len);

extern int			tcp_rcv_established(struct sock *sk, 
						    struct sk_buff *skb,
						    struct tcphdr *th, 
						    unsigned len);

enum tcp_ack_state_t
{
	TCP_ACK_SCHED = 1,
	TCP_ACK_TIMER = 2,
	TCP_ACK_PUSHED= 4
};

static inline void tcp_schedule_ack(struct tcp_opt *tp)
{
	tp->ack.pending |= TCP_ACK_SCHED;
}

static inline int tcp_ack_scheduled(struct tcp_opt *tp)
{
	return tp->ack.pending&TCP_ACK_SCHED;
}

static __inline__ void tcp_dec_quickack_mode(struct tcp_opt *tp)
{
	if (tp->ack.quick && --tp->ack.quick == 0) {
		/* Leaving quickack mode we deflate ATO. */
		tp->ack.ato = TCP_ATO_MIN;
	}
}

extern void tcp_enter_quickack_mode(struct tcp_opt *tp);

static __inline__ void tcp_delack_init(struct tcp_opt *tp)
{
	memset(&tp->ack, 0, sizeof(tp->ack));
}

static inline void tcp_clear_options(struct tcp_opt *tp)
{
 	tp->tstamp_ok = tp->sack_ok = tp->wscale_ok = tp->snd_wscale = 0;
}

enum tcp_tw_status
{
	TCP_TW_SUCCESS = 0,
	TCP_TW_RST = 1,
	TCP_TW_ACK = 2,
	TCP_TW_SYN = 3
};


extern enum tcp_tw_status	tcp_timewait_state_process(struct tcp_tw_bucket *tw,
							   struct sk_buff *skb,
							   struct tcphdr *th,
							   unsigned len);

extern struct sock *		tcp_check_req(struct sock *sk,struct sk_buff *skb,
					      struct open_request *req,
					      struct open_request **prev);
extern int			tcp_child_process(struct sock *parent,
						  struct sock *child,
						  struct sk_buff *skb);
extern void			tcp_enter_loss(struct sock *sk, int how);
extern void			tcp_clear_retrans(struct tcp_opt *tp);
extern void			tcp_update_metrics(struct sock *sk);

extern void			tcp_close(struct sock *sk, 
					  long timeout);
extern struct sock *		tcp_accept(struct sock *sk, int flags, int *err);
extern unsigned int		tcp_poll(struct file * file, struct socket *sock, struct poll_table_struct *wait);
extern void			tcp_write_space(struct sock *sk); 

extern int			tcp_getsockopt(struct sock *sk, int level, 
					       int optname, char *optval, 
					       int *optlen);
extern int			tcp_setsockopt(struct sock *sk, int level, 
					       int optname, char *optval, 
					       int optlen);
extern void			tcp_set_keepalive(struct sock *sk, int val);
extern int			tcp_recvmsg(struct sock *sk, 
					    struct msghdr *msg,
					    int len, int nonblock, 
					    int flags, int *addr_len);

extern int			tcp_listen_start(struct sock *sk);

extern void			tcp_parse_options(struct sk_buff *skb,
						  struct tcp_opt *tp,
						  int estab);

/*
 *	TCP v4 functions exported for the inet6 API
 */

extern int		       	tcp_v4_rebuild_header(struct sock *sk);

extern int		       	tcp_v4_build_header(struct sock *sk, 
						    struct sk_buff *skb);

extern void		       	tcp_v4_send_check(struct sock *sk, 
						  struct tcphdr *th, int len, 
						  struct sk_buff *skb);

extern int			tcp_v4_conn_request(struct sock *sk,
						    struct sk_buff *skb);

extern struct sock *		tcp_create_openreq_child(struct sock *sk,
							 struct open_request *req,
							 struct sk_buff *skb);

extern struct sock *		tcp_v4_syn_recv_sock(struct sock *sk,
						     struct sk_buff *skb,
						     struct open_request *req,
							struct dst_entry *dst);

extern int			tcp_v4_do_rcv(struct sock *sk,
					      struct sk_buff *skb);

extern int			tcp_v4_connect(struct sock *sk,
					       struct sockaddr *uaddr,
					       int addr_len);

extern int			tcp_connect(struct sock *sk);

extern struct sk_buff *		tcp_make_synack(struct sock *sk,
						struct dst_entry *dst,
						struct open_request *req);

extern int			tcp_disconnect(struct sock *sk, int flags);

extern void			tcp_unhash(struct sock *sk);

extern int			tcp_v4_hash_connecting(struct sock *sk);


/* From syncookies.c */
extern struct sock *cookie_v4_check(struct sock *sk, struct sk_buff *skb, 
				    struct ip_options *opt);
extern __u32 cookie_v4_init_sequence(struct sock *sk, struct sk_buff *skb, 
				     __u16 *mss);

/* tcp_output.c */

extern int tcp_write_xmit(struct sock *, int nonagle);
extern int tcp_retransmit_skb(struct sock *, struct sk_buff *);
extern void tcp_xmit_retransmit_queue(struct sock *);
extern void tcp_simple_retransmit(struct sock *);

extern void tcp_send_probe0(struct sock *);
extern void tcp_send_partial(struct sock *);
extern int  tcp_write_wakeup(struct sock *);
extern void tcp_send_fin(struct sock *sk);
extern void tcp_send_active_reset(struct sock *sk, int priority);
extern int  tcp_send_synack(struct sock *);
extern int  tcp_transmit_skb(struct sock *, struct sk_buff *);
extern void tcp_send_skb(struct sock *, struct sk_buff *, int force_queue, unsigned mss_now);
extern void tcp_push_one(struct sock *, unsigned mss_now);
extern void tcp_send_ack(struct sock *sk);
extern void tcp_send_delayed_ack(struct sock *sk);

/* tcp_timer.c */
extern void tcp_init_xmit_timers(struct sock *);
extern void tcp_clear_xmit_timers(struct sock *);

extern void tcp_delete_keepalive_timer (struct sock *);
extern void tcp_reset_keepalive_timer (struct sock *, unsigned long);
extern int tcp_sync_mss(struct sock *sk, u32 pmtu);

extern const char timer_bug_msg[];

/* Read 'sendfile()'-style from a TCP socket */
typedef int (*sk_read_actor_t)(read_descriptor_t *, struct sk_buff *,
				unsigned int, size_t);
extern int tcp_read_sock(struct sock *sk, read_descriptor_t *desc,
			 sk_read_actor_t recv_actor);

static inline void tcp_clear_xmit_timer(struct sock *sk, int what)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;
	
	switch (what) {
	case TCP_TIME_RETRANS:
	case TCP_TIME_PROBE0:
		tp->pending = 0;

#ifdef TCP_CLEAR_TIMERS
		if (timer_pending(&tp->retransmit_timer) &&
		    del_timer(&tp->retransmit_timer))
			__sock_put(sk);
#endif
		break;
	case TCP_TIME_DACK:
		tp->ack.blocked = 0;
		tp->ack.pending = 0;

#ifdef TCP_CLEAR_TIMERS
		if (timer_pending(&tp->delack_timer) &&
		    del_timer(&tp->delack_timer))
			__sock_put(sk);
#endif
		break;
	default:
		printk(timer_bug_msg);
		return;
	};
#endif
}

/*
 *	Reset the retransmission timer
 */
static inline void tcp_reset_xmit_timer(struct sock *sk, int what, unsigned long when)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;

	if (when > TCP_RTO_MAX) {
#ifdef TCP_DEBUG
		printk(KERN_DEBUG "reset_xmit_timer sk=%p %d when=0x%lx, caller=%p\n", sk, what, when, current_text_addr());
#endif
		when = TCP_RTO_MAX;
	}

	switch (what) {
	case TCP_TIME_RETRANS:
	case TCP_TIME_PROBE0:
		tp->pending = what;
		tp->timeout = jiffies+when;
		if (!mod_timer(&tp->retransmit_timer, tp->timeout))
			sock_hold(sk);
		break;

	case TCP_TIME_DACK:
		tp->ack.pending |= TCP_ACK_TIMER;
		tp->ack.timeout = jiffies+when;
		if (!mod_timer(&tp->delack_timer, tp->ack.timeout))
			sock_hold(sk);
		break;

	default:
		printk(KERN_DEBUG "bug: unknown timer value\n");
	};
#endif
}

/* Compute the current effective MSS, taking SACKs and IP options,
 * and even PMTU discovery events into account.
 */

static __inline__ unsigned int tcp_current_mss(struct sock *sk)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;
	struct dst_entry *dst = __sk_dst_get(sk);
	int mss_now = tp->mss_cache; 

	if (dst && dst->pmtu != tp->pmtu_cookie)
		mss_now = tcp_sync_mss(sk, dst->pmtu);

	if (tp->eff_sacks)
		mss_now -= (TCPOLEN_SACK_BASE_ALIGNED +
			    (tp->eff_sacks * TCPOLEN_SACK_PERBLOCK));
	return mss_now;
#else
  return 0;
#endif
}

/* Initialize RCV_MSS value.
 * RCV_MSS is an our guess about MSS used by the peer.
 * We haven't any direct information about the MSS.
 * It's better to underestimate the RCV_MSS rather than overestimate.
 * Overestimations make us ACKing less frequently than needed.
 * Underestimations are more easy to detect and fix by tcp_measure_rcv_mss().
 */

static inline void tcp_initialize_rcv_mss(struct sock *sk)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;
	unsigned int hint = min(tp->advmss, tp->mss_cache);

	hint = min(hint, tp->rcv_wnd/2);
	hint = min(hint, TCP_MIN_RCVMSS);
	hint = max(hint, TCP_MIN_MSS);

	tp->ack.rcv_mss = hint;
#endif
}

static __inline__ void __tcp_fast_path_on(struct tcp_opt *tp, u32 snd_wnd)
{
#if 0
	tp->pred_flags = htonl((tp->tcp_header_len << 26) |
			       ntohl(TCP_FLAG_ACK) |
			       snd_wnd);
#endif
}

static __inline__ void tcp_fast_path_on(struct tcp_opt *tp)
{
#if 0
	__tcp_fast_path_on(tp, tp->snd_wnd>>tp->snd_wscale);
#endif
}

static inline void tcp_fast_path_check(struct sock *sk, struct tcp_opt *tp)
{
#if 0
	if (skb_queue_len(&tp->out_of_order_queue) == 0 &&
	    tp->rcv_wnd &&
	    atomic_read(&sk->rmem_alloc) < sk->rcvbuf &&
	    !tp->urg_data)
		tcp_fast_path_on(tp);
#endif
}

/* Compute the actual receive window we are currently advertising.
 * Rcv_nxt can be after the window if our peer push more data
 * than the offered window.
 */
static __inline__ u32 tcp_receive_window(struct tcp_opt *tp)
{
#if 0
	s32 win = tp->rcv_wup + tp->rcv_wnd - tp->rcv_nxt;

	if (win < 0)
		win = 0;
	return (u32) win;
#else
  return 0;
#endif
}

/* Choose a new window, without checks for shrinking, and without
 * scaling applied to the result.  The caller does these things
 * if necessary.  This is a "raw" window selection.
 */
extern u32	__tcp_select_window(struct sock *sk);

/* TCP timestamps are only 32-bits, this causes a slight
 * complication on 64-bit systems since we store a snapshot
 * of jiffies in the buffer control blocks below.  We decidely
 * only use of the low 32-bits of jiffies and hide the ugly
 * casts with the following macro.
 */
#define tcp_time_stamp		((__u32)(jiffies))

/* This is what the send packet queueing engine uses to pass
 * TCP per-packet control information to the transmission
 * code.  We also store the host-order sequence numbers in
 * here too.  This is 36 bytes on 32-bit architectures,
 * 40 bytes on 64-bit machines, if this grows please adjust
 * skbuff.h:skbuff->cb[xxx] size appropriately.
 */
struct tcp_skb_cb {
	union {
#if 0
		struct inet_skb_parm	h4;
#endif
#if defined(CONFIG_IPV6) || defined (CONFIG_IPV6_MODULE)
		struct inet6_skb_parm	h6;
#endif
	} header;	/* For incoming frames		*/
	__u32		seq;		/* Starting sequence number	*/
	__u32		end_seq;	/* SEQ + FIN + SYN + datalen	*/
	__u32		when;		/* used to compute rtt's	*/
	__u8		flags;		/* TCP header flags.		*/

	/* NOTE: These must match up to the flags byte in a
	 *       real TCP header.
	 */
#define TCPCB_FLAG_FIN		0x01
#define TCPCB_FLAG_SYN		0x02
#define TCPCB_FLAG_RST		0x04
#define TCPCB_FLAG_PSH		0x08
#define TCPCB_FLAG_ACK		0x10
#define TCPCB_FLAG_URG		0x20
#define TCPCB_FLAG_ECE		0x40
#define TCPCB_FLAG_CWR		0x80

	__u8		sacked;		/* State flags for SACK/FACK.	*/
#define TCPCB_SACKED_ACKED	0x01	/* SKB ACK'd by a SACK block	*/
#define TCPCB_SACKED_RETRANS	0x02	/* SKB retransmitted		*/
#define TCPCB_LOST		0x04	/* SKB is lost			*/
#define TCPCB_TAGBITS		0x07	/* All tag bits			*/

#define TCPCB_EVER_RETRANS	0x80	/* Ever retransmitted frame	*/
#define TCPCB_RETRANS		(TCPCB_SACKED_RETRANS|TCPCB_EVER_RETRANS)

#define TCPCB_URG		0x20	/* Urgent pointer advenced here	*/

#define TCPCB_AT_TAIL		(TCPCB_URG)

	__u16		urg_ptr;	/* Valid w/URG flags is set.	*/
	__u32		ack_seq;	/* Sequence number ACK'd	*/
};

#define TCP_SKB_CB(__skb)	((struct tcp_skb_cb *)&((__skb)->cb[0]))

#define for_retrans_queue(skb, sk, tp) \
		for (skb = (sk)->write_queue.next;			\
		     (skb != (tp)->send_head) &&			\
		     (skb != (struct sk_buff *)&(sk)->write_queue);	\
		     skb=skb->next)


//#include <net/tcp_ecn.h>


/*
 *	Compute minimal free write space needed to queue new packets. 
 */
static inline int tcp_min_write_space(struct sock *sk)
{
#if 0
	return sk->wmem_queued/2;
#else
return 0;
#endif
}
 
static inline int tcp_wspace(struct sock *sk)
{
#if 0
	return sk->sndbuf - sk->wmem_queued;
#else
return 0;
#endif
}


/* This determines how many packets are "in the network" to the best
 * of our knowledge.  In many cases it is conservative, but where
 * detailed information is available from the receiver (via SACK
 * blocks etc.) we can make more aggressive calculations.
 *
 * Use this for decisions involving congestion control, use just
 * tp->packets_out to determine if the send queue is empty or not.
 *
 * Read this equation as:
 *
 *	"Packets sent once on transmission queue" MINUS
 *	"Packets left network, but not honestly ACKed yet" PLUS
 *	"Packets fast retransmitted"
 */
static __inline__ unsigned int tcp_packets_in_flight(struct tcp_opt *tp)
{
#if 0
	return tp->packets_out - tp->left_out + tp->retrans_out;
#else
  return 0;
#endif
}

/* Recalculate snd_ssthresh, we want to set it to:
 *
 * 	one half the current congestion window, but no
 *	less than two segments
 */
static inline __u32 tcp_recalc_ssthresh(struct tcp_opt *tp)
{
#if 0
	return max(tp->snd_cwnd >> 1U, 2U);
#else
  return 0;
#endif
}

/* If cwnd > ssthresh, we may raise ssthresh to be half-way to cwnd.
 * The exception is rate halving phase, when cwnd is decreasing towards
 * ssthresh.
 */
static inline __u32 tcp_current_ssthresh(struct tcp_opt *tp)
{
#if 0
	if ((1<<tp->ca_state)&(TCPF_CA_CWR|TCPF_CA_Recovery))
		return tp->snd_ssthresh;
	else
		return max(tp->snd_ssthresh,
			   ((tp->snd_cwnd >> 1) +
			    (tp->snd_cwnd >> 2)));
#else
  return 0;
#endif
}

static inline void tcp_sync_left_out(struct tcp_opt *tp)
{
#if 0
	if (tp->sack_ok && tp->sacked_out >= tp->packets_out - tp->lost_out)
		tp->sacked_out = tp->packets_out - tp->lost_out;
	tp->left_out = tp->sacked_out + tp->lost_out;
#endif
}

extern void tcp_cwnd_application_limited(struct sock *sk);

/* Congestion window validation. (RFC2861) */

static inline void tcp_cwnd_validate(struct sock *sk, struct tcp_opt *tp)
{
#if 0
	if (tp->packets_out >= tp->snd_cwnd) {
		/* Network is feed fully. */
		tp->snd_cwnd_used = 0;
		tp->snd_cwnd_stamp = tcp_time_stamp;
	} else {
		/* Network starves. */
		if (tp->packets_out > tp->snd_cwnd_used)
			tp->snd_cwnd_used = tp->packets_out;

		if ((s32)(tcp_time_stamp - tp->snd_cwnd_stamp) >= tp->rto)
			tcp_cwnd_application_limited(sk);
	}
#endif
}

/* Set slow start threshould and cwnd not falling to slow start */
static inline void __tcp_enter_cwr(struct tcp_opt *tp)
{
#if 0
	tp->undo_marker = 0;
	tp->snd_ssthresh = tcp_recalc_ssthresh(tp);
	tp->snd_cwnd = min(tp->snd_cwnd,
			   tcp_packets_in_flight(tp) + 1U);
	tp->snd_cwnd_cnt = 0;
	tp->high_seq = tp->snd_nxt;
	tp->snd_cwnd_stamp = tcp_time_stamp;
	TCP_ECN_queue_cwr(tp);
#endif
}

static inline void tcp_enter_cwr(struct tcp_opt *tp)
{
#if 0
	tp->prior_ssthresh = 0;
	if (tp->ca_state < TCP_CA_CWR) {
		__tcp_enter_cwr(tp);
		tp->ca_state = TCP_CA_CWR;
	}
#endif
}

extern __u32 tcp_init_cwnd(struct tcp_opt *tp);

/* Slow start with delack produces 3 packets of burst, so that
 * it is safe "de facto".
 */
static __inline__ __u32 tcp_max_burst(struct tcp_opt *tp)
{
	return 3;
}

static __inline__ int tcp_minshall_check(struct tcp_opt *tp)
{
#if 0
	return after(tp->snd_sml,tp->snd_una) &&
		!after(tp->snd_sml, tp->snd_nxt);
#else
  return 0;
#endif
}

static __inline__ void tcp_minshall_update(struct tcp_opt *tp, int mss, struct sk_buff *skb)
{
#if 0
	if (skb->len < mss)
		tp->snd_sml = TCP_SKB_CB(skb)->end_seq;
#endif
}

/* Return 0, if packet can be sent now without violation Nagle's rules:
   1. It is full sized.
   2. Or it contains FIN.
   3. Or TCP_NODELAY was set.
   4. Or TCP_CORK is not set, and all sent packets are ACKed.
      With Minshall's modification: all sent small packets are ACKed.
 */

static __inline__ int
tcp_nagle_check(struct tcp_opt *tp, struct sk_buff *skb, unsigned mss_now, int nonagle)
{
#if 0
	return (skb->len < mss_now &&
		!(TCP_SKB_CB(skb)->flags & TCPCB_FLAG_FIN) &&
		(nonagle == 2 ||
		 (!nonagle &&
		  tp->packets_out &&
		  tcp_minshall_check(tp))));
#else
  return 0;
#endif
}

/* This checks if the data bearing packet SKB (usually tp->send_head)
 * should be put on the wire right now.
 */
static __inline__ int tcp_snd_test(struct tcp_opt *tp, struct sk_buff *skb,
				   unsigned cur_mss, int nonagle)
{
#if 0
	/*	RFC 1122 - section 4.2.3.4
	 *
	 *	We must queue if
	 *
	 *	a) The right edge of this frame exceeds the window
	 *	b) There are packets in flight and we have a small segment
	 *	   [SWS avoidance and Nagle algorithm]
	 *	   (part of SWS is done on packetization)
	 *	   Minshall version sounds: there are no _small_
	 *	   segments in flight. (tcp_nagle_check)
	 *	c) We have too many packets 'in flight'
	 *
	 * 	Don't use the nagle rule for urgent data (or
	 *	for the final FIN -DaveM).
	 *
	 *	Also, Nagle rule does not apply to frames, which
	 *	sit in the middle of queue (they have no chances
	 *	to get new data) and if room at tail of skb is
	 *	not enough to save something seriously (<32 for now).
	 */

	/* Don't be strict about the congestion window for the
	 * final FIN frame.  -DaveM
	 */
	return ((nonagle==1 || tp->urg_mode
		 || !tcp_nagle_check(tp, skb, cur_mss, nonagle)) &&
		((tcp_packets_in_flight(tp) < tp->snd_cwnd) ||
		 (TCP_SKB_CB(skb)->flags & TCPCB_FLAG_FIN)) &&
		!after(TCP_SKB_CB(skb)->end_seq, tp->snd_una + tp->snd_wnd));
#else
  return 0;
#endif
}

static __inline__ void tcp_check_probe_timer(struct sock *sk, struct tcp_opt *tp)
{
#if 0
	if (!tp->packets_out && !tp->pending)
		tcp_reset_xmit_timer(sk, TCP_TIME_PROBE0, tp->rto);
#endif
}

static __inline__ int tcp_skb_is_last(struct sock *sk, struct sk_buff *skb)
{
#if 0
	return (skb->next == (struct sk_buff*)&sk->write_queue);
#else
  return 0;
#endif
}

/* Push out any pending frames which were held back due to
 * TCP_CORK or attempt at coalescing tiny packets.
 * The socket must be locked by the caller.
 */
static __inline__ void __tcp_push_pending_frames(struct sock *sk,
						 struct tcp_opt *tp,
						 unsigned cur_mss,
						 int nonagle)
{
#if 0
	struct sk_buff *skb = tp->send_head;

	if (skb) {
		if (!tcp_skb_is_last(sk, skb))
			nonagle = 1;
		if (!tcp_snd_test(tp, skb, cur_mss, nonagle) ||
		    tcp_write_xmit(sk, nonagle))
			tcp_check_probe_timer(sk, tp);
	}
	tcp_cwnd_validate(sk, tp);
#endif
}

static __inline__ void tcp_push_pending_frames(struct sock *sk,
					       struct tcp_opt *tp)
{
#if 0
	__tcp_push_pending_frames(sk, tp, tcp_current_mss(sk), tp->nonagle);
#endif
}

static __inline__ int tcp_may_send_now(struct sock *sk, struct tcp_opt *tp)
{
#if 0
	struct sk_buff *skb = tp->send_head;

	return (skb &&
		tcp_snd_test(tp, skb, tcp_current_mss(sk),
			     tcp_skb_is_last(sk, skb) ? 1 : tp->nonagle));
#else
  return 0;
#endif
}

static __inline__ void tcp_init_wl(struct tcp_opt *tp, u32 ack, u32 seq)
{
#if 0
	tp->snd_wl1 = seq;
#endif
}

static __inline__ void tcp_update_wl(struct tcp_opt *tp, u32 ack, u32 seq)
{
#if 0
	tp->snd_wl1 = seq;
#endif
}

extern void			tcp_destroy_sock(struct sock *sk);


/*
 * Calculate(/check) TCP checksum
 */
static __inline__ u16 tcp_v4_check(struct tcphdr *th, int len,
				   unsigned long saddr, unsigned long daddr, 
				   unsigned long base)
{
#if 0
	return csum_tcpudp_magic(saddr,daddr,len,IPPROTO_TCP,base);
#else
  return 0;
#endif
}

static __inline__ int __tcp_checksum_complete(struct sk_buff *skb)
{
#if 0
	return (unsigned short)csum_fold(skb_checksum(skb, 0, skb->len, skb->csum));
#else
  return 0;
#endif
}

static __inline__ int tcp_checksum_complete(struct sk_buff *skb)
{
#if 0
	return skb->ip_summed != CHECKSUM_UNNECESSARY &&
		__tcp_checksum_complete(skb);
#else
  return 0;
#endif
}

/* Prequeue for VJ style copy to user, combined with checksumming. */

static __inline__ void tcp_prequeue_init(struct tcp_opt *tp)
{
#if 0
	tp->ucopy.task = NULL;
	tp->ucopy.len = 0;
	tp->ucopy.memory = 0;
	skb_queue_head_init(&tp->ucopy.prequeue);
#endif
}

/* Packet is added to VJ-style prequeue for processing in process
 * context, if a reader task is waiting. Apparently, this exciting
 * idea (VJ's mail "Re: query about TCP header on tcp-ip" of 07 Sep 93)
 * failed somewhere. Latency? Burstiness? Well, at least now we will
 * see, why it failed. 8)8)				  --ANK
 *
 * NOTE: is this not too big to inline?
 */
static __inline__ int tcp_prequeue(struct sock *sk, struct sk_buff *skb)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;

	if (tp->ucopy.task) {
		__skb_queue_tail(&tp->ucopy.prequeue, skb);
		tp->ucopy.memory += skb->truesize;
		if (tp->ucopy.memory > sk->rcvbuf) {
			struct sk_buff *skb1;

			if (sk->lock.users)
				out_of_line_bug();

			while ((skb1 = __skb_dequeue(&tp->ucopy.prequeue)) != NULL) {
				sk->backlog_rcv(sk, skb1);
				NET_INC_STATS_BH(TCPPrequeueDropped);
			}

			tp->ucopy.memory = 0;
		} else if (skb_queue_len(&tp->ucopy.prequeue) == 1) {
			wake_up_interruptible(sk->sleep);
			if (!tcp_ack_scheduled(tp))
				tcp_reset_xmit_timer(sk, TCP_TIME_DACK, (3*TCP_RTO_MIN)/4);
		}
		return 1;
	}
	return 0;
#else
  return 0;
#endif
}


#undef STATE_TRACE

#ifdef STATE_TRACE
static char *statename[]={
	"Unused","Established","Syn Sent","Syn Recv",
	"Fin Wait 1","Fin Wait 2","Time Wait", "Close",
	"Close Wait","Last ACK","Listen","Closing"
};
#endif

static __inline__ void tcp_set_state(struct sock *sk, int state)
{
#if 0
	int oldstate = sk->state;

	switch (state) {
	case TCP_ESTABLISHED:
		if (oldstate != TCP_ESTABLISHED)
			TCP_INC_STATS(TcpCurrEstab);
		break;

	case TCP_CLOSE:
		sk->prot->unhash(sk);
		if (sk->prev && !(sk->userlocks&SOCK_BINDPORT_LOCK))
			tcp_put_port(sk);
		/* fall through */
	default:
		if (oldstate==TCP_ESTABLISHED)
			tcp_statistics[smp_processor_id()*2+!in_softirq()].TcpCurrEstab--;
	}

	/* Change state AFTER socket is unhashed to avoid closed
	 * socket sitting in hash tables.
	 */
	sk->state = state;

#ifdef STATE_TRACE
	SOCK_DEBUG(sk, "TCP sk=%p, State %s -> %s\n",sk, statename[oldstate],statename[state]);
#endif	
#endif
}

static __inline__ void tcp_done(struct sock *sk)
{
#if 0
	tcp_set_state(sk, TCP_CLOSE);
	tcp_clear_xmit_timers(sk);

	sk->shutdown = SHUTDOWN_MASK;

	if (!sk->dead)
		sk->state_change(sk);
	else
		tcp_destroy_sock(sk);
#endif
}

static __inline__ void tcp_sack_reset(struct tcp_opt *tp)
{
#if 0
	tp->dsack = 0;
	tp->eff_sacks = 0;
	tp->num_sacks = 0;
#endif
}

static __inline__ void tcp_build_and_update_options(__u32 *ptr, struct tcp_opt *tp, __u32 tstamp)
{
#if 0
	if (tp->tstamp_ok) {
		*ptr++ = __constant_htonl((TCPOPT_NOP << 24) |
					  (TCPOPT_NOP << 16) |
					  (TCPOPT_TIMESTAMP << 8) |
					  TCPOLEN_TIMESTAMP);
		*ptr++ = htonl(tstamp);
		*ptr++ = htonl(tp->ts_recent);
	}
	if (tp->eff_sacks) {
		struct tcp_sack_block *sp = tp->dsack ? tp->duplicate_sack : tp->selective_acks;
		int this_sack;

		*ptr++ = __constant_htonl((TCPOPT_NOP << 24) |
					  (TCPOPT_NOP << 16) |
					  (TCPOPT_SACK << 8) |
					  (TCPOLEN_SACK_BASE +
					   (tp->eff_sacks * TCPOLEN_SACK_PERBLOCK)));
		for(this_sack = 0; this_sack < tp->eff_sacks; this_sack++) {
			*ptr++ = htonl(sp[this_sack].start_seq);
			*ptr++ = htonl(sp[this_sack].end_seq);
		}
		if (tp->dsack) {
			tp->dsack = 0;
			tp->eff_sacks--;
		}
	}
#endif
}

/* Construct a tcp options header for a SYN or SYN_ACK packet.
 * If this is every changed make sure to change the definition of
 * MAX_SYN_SIZE to match the new maximum number of options that you
 * can generate.
 */
static inline void tcp_syn_build_options(__u32 *ptr, int mss, int ts, int sack,
					     int offer_wscale, int wscale, __u32 tstamp, __u32 ts_recent)
{
#if 0
	/* We always get an MSS option.
	 * The option bytes which will be seen in normal data
	 * packets should timestamps be used, must be in the MSS
	 * advertised.  But we subtract them from tp->mss_cache so
	 * that calculations in tcp_sendmsg are simpler etc.
	 * So account for this fact here if necessary.  If we
	 * don't do this correctly, as a receiver we won't
	 * recognize data packets as being full sized when we
	 * should, and thus we won't abide by the delayed ACK
	 * rules correctly.
	 * SACKs don't matter, we never delay an ACK when we
	 * have any of those going out.
	 */
	*ptr++ = htonl((TCPOPT_MSS << 24) | (TCPOLEN_MSS << 16) | mss);
	if (ts) {
		if(sack)
			*ptr++ = __constant_htonl((TCPOPT_SACK_PERM << 24) | (TCPOLEN_SACK_PERM << 16) |
						  (TCPOPT_TIMESTAMP << 8) | TCPOLEN_TIMESTAMP);
		else
			*ptr++ = __constant_htonl((TCPOPT_NOP << 24) | (TCPOPT_NOP << 16) |
						  (TCPOPT_TIMESTAMP << 8) | TCPOLEN_TIMESTAMP);
		*ptr++ = htonl(tstamp);		/* TSVAL */
		*ptr++ = htonl(ts_recent);	/* TSECR */
	} else if(sack)
		*ptr++ = __constant_htonl((TCPOPT_NOP << 24) | (TCPOPT_NOP << 16) |
					  (TCPOPT_SACK_PERM << 8) | TCPOLEN_SACK_PERM);
	if (offer_wscale)
		*ptr++ = htonl((TCPOPT_NOP << 24) | (TCPOPT_WINDOW << 16) | (TCPOLEN_WINDOW << 8) | (wscale));
#endif
}

/* Determine a window scaling and initial window to offer.
 * Based on the assumption that the given amount of space
 * will be offered. Store the results in the tp structure.
 * NOTE: for smooth operation initial space offering should
 * be a multiple of mss if possible. We assume here that mss >= 1.
 * This MUST be enforced by all callers.
 */
static inline void tcp_select_initial_window(int __space, __u32 mss,
	__u32 *rcv_wnd,
	__u32 *window_clamp,
	int wscale_ok,
	__u8 *rcv_wscale)
{
#if 0
	unsigned int space = (__space < 0 ? 0 : __space);

	/* If no clamp set the clamp to the max possible scaled window */
	if (*window_clamp == 0)
		(*window_clamp) = (65535 << 14);
	space = min(*window_clamp, space);

	/* Quantize space offering to a multiple of mss if possible. */
	if (space > mss)
		space = (space / mss) * mss;

	/* NOTE: offering an initial window larger than 32767
	 * will break some buggy TCP stacks. We try to be nice.
	 * If we are not window scaling, then this truncates
	 * our initial window offering to 32k. There should also
	 * be a sysctl option to stop being nice.
	 */
	(*rcv_wnd) = min(space, MAX_TCP_WINDOW);
	(*rcv_wscale) = 0;
	if (wscale_ok) {
		/* See RFC1323 for an explanation of the limit to 14 */
		while (space > 65535 && (*rcv_wscale) < 14) {
			space >>= 1;
			(*rcv_wscale)++;
		}
		if (*rcv_wscale && sysctl_tcp_app_win && space>=mss &&
		    space - max((space>>sysctl_tcp_app_win), mss>>*rcv_wscale) < 65536/2)
			(*rcv_wscale)--;
	}

	/* Set initial window to value enough for senders,
	 * following RFC1414. Senders, not following this RFC,
	 * will be satisfied with 2.
	 */
	if (mss > (1<<*rcv_wscale)) {
		int init_cwnd = 4;
		if (mss > 1460*3)
			init_cwnd = 2;
		else if (mss > 1460)
			init_cwnd = 3;
		if (*rcv_wnd > init_cwnd*mss)
			*rcv_wnd = init_cwnd*mss;
	}
	/* Set the clamp no higher than max representable value */
	(*window_clamp) = min(65535U << (*rcv_wscale), *window_clamp);
#endif
}

static inline int tcp_win_from_space(int space)
{
#if 0
	return sysctl_tcp_adv_win_scale<=0 ?
		(space>>(-sysctl_tcp_adv_win_scale)) :
		space - (space>>sysctl_tcp_adv_win_scale);
#else
  return 0;
#endif
}

/* Note: caller must be prepared to deal with negative returns */ 
static inline int tcp_space(struct sock *sk)
{
#if 0
	return tcp_win_from_space(sk->rcvbuf - atomic_read(&sk->rmem_alloc));
#else
  return 0;
#endif
} 

static inline int tcp_full_space( struct sock *sk)
{
#if 0
	return tcp_win_from_space(sk->rcvbuf); 
#else
  return 0;
#endif
}

static inline void tcp_acceptq_removed(struct sock *sk)
{
#if 0
	sk->ack_backlog--;
#endif
}

static inline void tcp_acceptq_added(struct sock *sk)
{
#if 0
	sk->ack_backlog++;
#endif
}

static inline int tcp_acceptq_is_full(struct sock *sk)
{
#if 0
	return sk->ack_backlog > sk->max_ack_backlog;
#else
  return 0;
#endif
}

static inline void tcp_acceptq_queue(struct sock *sk, struct open_request *req,
					 struct sock *child)
{
#if 0
	struct tcp_opt *tp = &sk->tp_pinfo.af_tcp;

	req->sk = child;
	tcp_acceptq_added(sk);

	if (!tp->accept_queue_tail) {
		tp->accept_queue = req;
	} else {
		tp->accept_queue_tail->dl_next = req;
	}
	tp->accept_queue_tail = req;
	req->dl_next = NULL;
#endif
}

struct tcp_listen_opt
{
	u8			max_qlen_log;	/* log_2 of maximal queued SYNs */
	int			qlen;
	int			qlen_young;
	int			clock_hand;
	struct open_request	*syn_table[TCP_SYNQ_HSIZE];
};

static inline void
tcp_synq_removed(struct sock *sk, struct open_request *req)
{
#if 0
	struct tcp_listen_opt *lopt = sk->tp_pinfo.af_tcp.listen_opt;

	if (--lopt->qlen == 0)
		tcp_delete_keepalive_timer(sk);
	if (req->retrans == 0)
		lopt->qlen_young--;
#endif
}

static inline void tcp_synq_added(struct sock *sk)
{
#if 0
	struct tcp_listen_opt *lopt = sk->tp_pinfo.af_tcp.listen_opt;

	if (lopt->qlen++ == 0)
		tcp_reset_keepalive_timer(sk, TCP_TIMEOUT_INIT);
	lopt->qlen_young++;
#endif
}

static inline int tcp_synq_len(struct sock *sk)
{
#if 0
	return sk->tp_pinfo.af_tcp.listen_opt->qlen;
#else
  return 0;
#endif
}

static inline int tcp_synq_young(struct sock *sk)
{
#if 0
	return sk->tp_pinfo.af_tcp.listen_opt->qlen_young;
#else
  return 0;
#endif
}

static inline int tcp_synq_is_full(struct sock *sk)
{
#if 0
	return tcp_synq_len(sk)>>sk->tp_pinfo.af_tcp.listen_opt->max_qlen_log;
#else
  return 0;
#endif
}

static inline void tcp_synq_unlink(struct tcp_opt *tp, struct open_request *req,
				       struct open_request **prev)
{
#if 0
	write_lock(&tp->syn_wait_lock);
	*prev = req->dl_next;
	write_unlock(&tp->syn_wait_lock);
#endif
}

static inline void tcp_synq_drop(struct sock *sk, struct open_request *req,
				     struct open_request **prev)
{
#if 0
	tcp_synq_unlink(&sk->tp_pinfo.af_tcp, req, prev);
	tcp_synq_removed(sk, req);
	tcp_openreq_free(req);
#endif
}

static __inline__ void tcp_openreq_init(struct open_request *req,
					struct tcp_opt *tp,
					struct sk_buff *skb)
{
#if 0
	req->rcv_wnd = 0;		/* So that tcp_send_synack() knows! */
	req->rcv_isn = TCP_SKB_CB(skb)->seq;
	req->mss = tp->mss_clamp;
	req->ts_recent = tp->saw_tstamp ? tp->rcv_tsval : 0;
	req->tstamp_ok = tp->tstamp_ok;
	req->sack_ok = tp->sack_ok;
	req->snd_wscale = tp->snd_wscale;
	req->wscale_ok = tp->wscale_ok;
	req->acked = 0;
	req->ecn_ok = 0;
	req->rmt_port = skb->h.th->source;
#endif
}

#define TCP_MEM_QUANTUM	((int)PAGE_SIZE)

static inline void tcp_free_skb(struct sock *sk, struct sk_buff *skb)
{
#if 0
	sk->tp_pinfo.af_tcp.queue_shrunk = 1;
	sk->wmem_queued -= skb->truesize;
	sk->forward_alloc += skb->truesize;
	__kfree_skb(skb);
#endif
}

static inline void tcp_charge_skb(struct sock *sk, struct sk_buff *skb)
{
#if 0
	sk->wmem_queued += skb->truesize;
	sk->forward_alloc -= skb->truesize;
#endif
}

extern void __tcp_mem_reclaim(struct sock *sk);
extern int tcp_mem_schedule(struct sock *sk, int size, int kind);

static inline void tcp_mem_reclaim(struct sock *sk)
{
#if 0
	if (sk->forward_alloc >= TCP_MEM_QUANTUM)
		__tcp_mem_reclaim(sk);
#endif
}

static inline void tcp_enter_memory_pressure(void)
{
#if 0
	if (!tcp_memory_pressure) {
		NET_INC_STATS(TCPMemoryPressures);
		tcp_memory_pressure = 1;
	}
#endif
}

static inline void tcp_moderate_sndbuf(struct sock *sk)
{
#if 0
	if (!(sk->userlocks&SOCK_SNDBUF_LOCK)) {
		sk->sndbuf = min(sk->sndbuf, sk->wmem_queued/2);
		sk->sndbuf = max(sk->sndbuf, SOCK_MIN_SNDBUF);
	}
#endif
}

static inline struct sk_buff *tcp_alloc_pskb(struct sock *sk, int size, int mem, int gfp)
{
#if 0
	struct sk_buff *skb = alloc_skb(size+MAX_TCP_HEADER, gfp);

	if (skb) {
		skb->truesize += mem;
		if (sk->forward_alloc >= (int)skb->truesize ||
		    tcp_mem_schedule(sk, skb->truesize, 0)) {
			skb_reserve(skb, MAX_TCP_HEADER);
			return skb;
		}
		__kfree_skb(skb);
	} else {
		tcp_enter_memory_pressure();
		tcp_moderate_sndbuf(sk);
	}
	return NULL;
#else
  return NULL;
#endif
}

static inline struct sk_buff *tcp_alloc_skb(struct sock *sk, int size, int gfp)
{
#if 0
	return tcp_alloc_pskb(sk, size, 0, gfp);
#else
  return NULL;
#endif
}

static inline struct page * tcp_alloc_page(struct sock *sk)
{
#if 0
	if (sk->forward_alloc >= (int)PAGE_SIZE ||
	    tcp_mem_schedule(sk, PAGE_SIZE, 0)) {
		struct page *page = alloc_pages(sk->allocation, 0);
		if (page)
			return page;
	}
	tcp_enter_memory_pressure();
	tcp_moderate_sndbuf(sk);
	return NULL;
#else
  return NULL;
#endif
}

static inline void tcp_writequeue_purge(struct sock *sk)
{
#if 0
	struct sk_buff *skb;

	while ((skb = __skb_dequeue(&sk->write_queue)) != NULL)
		tcp_free_skb(sk, skb);
	tcp_mem_reclaim(sk);
#endif
}

extern void tcp_rfree(struct sk_buff *skb);

static inline void tcp_set_owner_r(struct sk_buff *skb, struct sock *sk)
{
#if 0
	skb->sk = sk;
	skb->destructor = tcp_rfree;
	atomic_add(skb->truesize, &sk->rmem_alloc);
	sk->forward_alloc -= skb->truesize;
#endif
}

extern void tcp_listen_wlock(void);

/* - We may sleep inside this lock.
 * - If sleeping is not required (or called from BH),
 *   use plain read_(un)lock(&tcp_lhash_lock).
 */

static inline void tcp_listen_lock(void)
{
#if 0
	/* read_lock synchronizes to candidates to writers */
	read_lock(&tcp_lhash_lock);
	atomic_inc(&tcp_lhash_users);
	read_unlock(&tcp_lhash_lock);
#endif
}

static inline void tcp_listen_unlock(void)
{
#if 0
	if (atomic_dec_and_test(&tcp_lhash_users))
		wake_up(&tcp_lhash_wait);
#endif
}

static inline int keepalive_intvl_when(struct tcp_opt *tp)
{
#if 0
	return tp->keepalive_intvl ? : sysctl_tcp_keepalive_intvl;
#else
  return 0;
#endif
}

static inline int keepalive_time_when(struct tcp_opt *tp)
{
#if 0
	return tp->keepalive_time ? : sysctl_tcp_keepalive_time;
#else
  return 0;
#endif
}

static inline int tcp_fin_time(struct tcp_opt *tp)
{
#if 0
	int fin_timeout = tp->linger2 ? : sysctl_tcp_fin_timeout;

	if (fin_timeout < (tp->rto<<2) - (tp->rto>>1))
		fin_timeout = (tp->rto<<2) - (tp->rto>>1);

	return fin_timeout;
#else
  return 0;
#endif
}

static inline int tcp_paws_check(struct tcp_opt *tp, int rst)
{
#if 0
	if ((s32)(tp->rcv_tsval - tp->ts_recent) >= 0)
		return 0;
	if (xtime.tv_sec >= tp->ts_recent_stamp + TCP_PAWS_24DAYS)
		return 0;

	/* RST segments are not recommended to carry timestamp,
	   and, if they do, it is recommended to ignore PAWS because
	   "their cleanup function should take precedence over timestamps."
	   Certainly, it is mistake. It is necessary to understand the reasons
	   of this constraint to relax it: if peer reboots, clock may go
	   out-of-sync and half-open connections will not be reset.
	   Actually, the problem would be not existing if all
	   the implementations followed draft about maintaining clock
	   via reboots. Linux-2.2 DOES NOT!

	   However, we can relax time bounds for RST segments to MSL.
	 */
	if (rst && xtime.tv_sec >= tp->ts_recent_stamp + TCP_PAWS_MSL)
		return 0;
	return 1;
#else
  return 0;
#endif
}

#define TCP_CHECK_TIMER(sk) do { } while (0)

#endif /* __TCPCORE_H */


//
#endif
#endif
