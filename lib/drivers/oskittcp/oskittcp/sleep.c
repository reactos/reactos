#include <oskittcp.h>
#include <sys/callout.h>
#include <oskitfreebsd.h>
#include <oskitdebug.h>

/* clock_init */
int ncallout = 256;
struct callout *callout;

void init_freebsd_sched() {
}

int tsleep( void *token, int priority, char *wmesg, int tmio ) {
    if( !OtcpEvent.Sleep ) panic("no sleep");
    return
	OtcpEvent.Sleep( OtcpEvent.ClientData, token, priority, wmesg, tmio );
}

void wakeup( struct socket *so, void *token ) {
    OSK_UINT flags = 0;

    OS_DbgPrint
	(OSK_MID_TRACE,("XXX Bytes to receive: %d state %x\n",
			so->so_rcv.sb_cc, so->so_state));

    if( so->so_state & SS_ISCONNECTED ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket connected!\n"));
	flags |= SEL_CONNECT;
    }
    if( so->so_q ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket accepting q\n"));
	flags |= SEL_ACCEPT;
    }
    if( so->so_rcv.sb_cc > 0 ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket readable\n"));
	flags |= SEL_READ;
    }
    if( 0 < sbspace(&so->so_snd) ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket writeable\n"));
	flags |= SEL_WRITE;
    }
    if( so->so_state & SS_CANTRCVMORE ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket can't be read any longer\n"));
	flags |= SEL_FIN;
    }

    OS_DbgPrint(OSK_MID_TRACE,("Wakeup %x (socket %x, state %x)!\n",
			       token, so,
			       so->so_state));

    if( OtcpEvent.SocketState )
	OtcpEvent.SocketState( OtcpEvent.ClientData,
			       so,
			       so ? so->so_connection : 0,
			       flags );

    if( OtcpEvent.Wakeup )
	OtcpEvent.Wakeup( OtcpEvent.ClientData, token );

    OS_DbgPrint(OSK_MID_TRACE,("Wakeup done %x\n", token));
}

/* ---------------------------------------------------------------------- */


static void
timeout_init(void)
{
	int i;

	callout = (struct callout *)
	    malloc(sizeof(struct callout) * ncallout, M_FREE, M_WAITOK);
	if (!callout)
	        panic("can't allocate callout queue!\n");

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

}

/* get clock up and running */
void clock_init()
{
	timeout_init();
	/* inittodr(0); // what does this do? */
	/* boottime = kern_time; */
	/* Start a clock we can use for timeouts */
}


extern	unsigned bio_imask;	/* group of interrupts masked with splbio() */
extern	unsigned cpl;		/* current priority level mask */
extern	volatile unsigned idelayed;	/* interrupts to become pending */
extern	volatile unsigned ipending;	/* active interrupts masked by cpl */
extern	unsigned net_imask;	/* group of interrupts masked with splimp() */
extern	unsigned stat_imask;	/* interrupts masked with splstatclock() */
extern	unsigned tty_imask;	/* group of interrupts masked with spltty() */

/*
 * ipending has to be volatile so that it is read every time it is accessed
 * in splx() and spl0(), but we don't want it to be read nonatomically when
 * it is changed.  Pretending that ipending is a plain int happens to give
 * suitable atomic code for "ipending |= constant;".
 */
#define	setdelayed()	(*(unsigned *)&ipending |= loadandclear(&idelayed))
#define	setsoftast()	(*(unsigned *)&ipending |= SWI_AST_PENDING)
#define	setsoftclock()	(*(unsigned *)&ipending |= SWI_CLOCK_PENDING)
#define	setsoftnet()	(*(unsigned *)&ipending |= SWI_NET_PENDING)
#define	setsofttty()	(*(unsigned *)&ipending |= SWI_TTY_PENDING)

#define	schedsofttty()	(*(unsigned *)&idelayed |= SWI_TTY_PENDING)

#define	GENSPL(name, set_cpl) \
static __inline int name(void)			\
{						\
	unsigned x;				\
						\
	__asm __volatile("" : : : "memory");	\
	x = cpl;				\
	set_cpl;				\
	return (x);				\
}

void splz(void) {
    OS_DbgPrint(OSK_MID_TRACE,("Called SPLZ\n"));
}

/*
 * functions to save and restore the current cpl
 */
void save_cpl(unsigned *x)
{
    *x = cpl;
}

void restore_cpl(unsigned x)
{
    cpl = x;
}
