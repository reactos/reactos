#include <roscfg.h>
#include <oskittcp.h>
#include <ntddk.h>
#include <sys/callout.h>
#include <oskitfreebsd.h>
#include <oskitdebug.h>

typedef struct _SLEEPING_THREAD {
    LIST_ENTRY Entry;
    PVOID SleepToken;
    KEVENT Event;
} SLEEPING_THREAD, *PSLEEPING_THREAD;

LIST_ENTRY SleepingThreadsList;
FAST_MUTEX SleepingThreadsLock;

/* clock_init */
int ncallout = 256;
struct callout *callout;

void init_freebsd_sched() {
    ExInitializeFastMutex( &SleepingThreadsLock );
    InitializeListHead( &SleepingThreadsList );    
}

int tsleep( void *token, int priority, char *wmesg, int tmio ) {
    KIRQL OldIrql;
    KEVENT Event;
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;
    
    OS_DbgPrint(OSK_MID_TRACE,
		("Called TSLEEP: tok = %x, pri = %d, wmesg = %s, tmio = %x\n",
		 token, priority, wmesg, tmio));

    SleepingThread = ExAllocatePool( NonPagedPool, sizeof( *SleepingThread ) );
    if( SleepingThread ) {
	KeInitializeEvent( &SleepingThread->Event, NotificationEvent, FALSE );
	SleepingThread->SleepToken = token;

	ExAcquireFastMutex( &SleepingThreadsLock );
	InsertTailList( &SleepingThreadsList, &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	OS_DbgPrint(OSK_MID_TRACE,("Waiting on %x\n", token));
	KeWaitForSingleObject( &SleepingThread->Event,
			       WrSuspended,
			       KernelMode,
			       TRUE,
			       NULL );

	ExAcquireFastMutex( &SleepingThreadsLock );
	RemoveEntryList( &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	ExFreePool( SleepingThread );
    }
    OS_DbgPrint(OSK_MID_TRACE,("Waiting finished: %x\n", token));
    return 0;
}

void wakeup( struct socket *so, struct selinfo *si, void *token ) {
    KIRQL OldIrql;
    KEVENT Event;
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;

    OS_DbgPrint
	(OSK_MID_TRACE,("XXX Bytes to receive: %d\n", so->so_rcv.sb_cc));

    if( so->so_rcv.sb_cc && si )
	si->si_flags |= SEL_READ;

    OS_DbgPrint(OSK_MID_TRACE,("Wakeup %x (socket %x, si_flags %x, state %x)!\n",
			       token, so, si ? si->si_flags : 0,
			       so->so_state));

    if( OtcpEvent.SocketState ) {
	OS_DbgPrint(OSK_MID_TRACE,("Calling client's socket state fn\n"));
	OtcpEvent.SocketState( OtcpEvent.ClientData,
			       so,
			       so->so_connection,
			       si ? si->si_flags : 0,
			       so->so_state );
    }

    ExAcquireFastMutex( &SleepingThreadsLock );
    Entry = SleepingThreadsList.Flink;
    while( Entry != &SleepingThreadsList ) {
	SleepingThread = CONTAINING_RECORD(Entry, SLEEPING_THREAD, Entry);
	OS_DbgPrint(OSK_MID_TRACE,("Sleeper @ %x\n", SleepingThread));
	if( SleepingThread->SleepToken == token ) {
	    OS_DbgPrint(OSK_MID_TRACE,("Setting event to wake %x\n", token));
	    KeSetEvent( &SleepingThread->Event, IO_NETWORK_INCREMENT, FALSE );
	}
	Entry = Entry->Flink;
    }
    ExReleaseFastMutex( &SleepingThreadsLock );
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
	boottime = time;
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
