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
KSPIN_LOCK SleepingThreadsLock;

/* clock_init */
int ncallout = 256;
struct callout *callout;

void init_freebsd_sched() {
    KeInitializeSpinLock( &SleepingThreadsLock );
    InitializeListHead( &SleepingThreadsList );    
}

void tsleep( void *token, int priority, char *wmesg, int tmio ) {
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
	ExInterlockedInsertTailList( &SleepingThreadsList,
				     &SleepingThread->Entry,
				     &SleepingThreadsLock );
    }

    OS_DbgPrint(OSK_MID_TRACE,("Waiting on %x\n", token));
    KeWaitForSingleObject( &SleepingThread->Event,
			   WrSuspended,
			   KernelMode,
			   TRUE,
			   NULL );
    OS_DbgPrint(OSK_MID_TRACE,("Waiting finished: %x\n", token));
}

void wakeup( void *token ) {
    KIRQL OldIrql;
    KEVENT Event;
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;
    
    OS_DbgPrint(OSK_MID_TRACE,("Wakeup %x!\n",token));

    KeAcquireSpinLock( &SleepingThreadsLock, &OldIrql );
    Entry = SleepingThreadsList.Flink;
    while( Entry != &SleepingThreadsList ) {
	SleepingThread = CONTAINING_RECORD(Entry, SLEEPING_THREAD, Entry);
	if( SleepingThread->SleepToken == token ) {
	    RemoveEntryList(Entry);
	    KeReleaseSpinLock( &SleepingThreadsLock, OldIrql );
	    OS_DbgPrint(OSK_MID_TRACE,("Setting event to wake %x\n", token));
	    KeSetEvent( &SleepingThread->Event, IO_NO_INCREMENT, FALSE );
	    ExFreePool( SleepingThread );
	    return;
	}
	Entry = Entry->Flink;
    }
    KeReleaseSpinLock( &SleepingThreadsLock, OldIrql );
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

#if 0
GENSPL(splbio, cpl |= bio_imask)
GENSPL(splclock, cpl = HWI_MASK | SWI_MASK)
GENSPL(splhigh, cpl = HWI_MASK | SWI_MASK)
GENSPL(splimp, cpl |= net_imask)
GENSPL(splnet, cpl |= SWI_NET_MASK)
GENSPL(splsoftclock, cpl = SWI_CLOCK_MASK)
GENSPL(splsofttty, cpl |= SWI_TTY_MASK)
GENSPL(splstatclock, cpl |= stat_imask)
GENSPL(spltty, cpl |= tty_imask)
#endif

#if 0
void spl0(void) {
    cpl = SWI_AST_MASK;
    if (ipending & ~SWI_AST_MASK)
	splz();
}

void splx(int ipl) {
    cpl = ipl;
    if (ipending & ~ipl)
	splz();
}
#endif

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

void selrecord( struct proc *selector, struct selinfo *sip) {
    OS_DbgPrint(OSK_MID_TRACE,("Called selrecord\n"));
}

void wakeupsocket( struct socket *so, struct selinfo *sel ) {
    void *connection = so->so_connection;
    char *data = 0;
    int datalen = 0;
    int flags = 0;
    
    OS_DbgPrint(OSK_MID_TRACE,("Wakeup: %x\n", so));
#if 0
    if( soreceive(so, &paddr, 0, &mp0, &controlp, flags) == 0 ) {
	/* We have data available */
	OS_DbgPrint(OSK_MID_TRACE,("Data available on %x\n", so));
    }
#endif
}

void selwakeup( struct selinfo *sel ) {
    OS_DbgPrint(OSK_MID_TRACE,("Called selwakeup\n"));
}
