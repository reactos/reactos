/* ReactOS-Specific lwIP binding header - by Cameron Gutman */

/* Implmentation specific structs */
typedef PRKEVENT sys_sem_t;

typedef struct _sys_mbox_t
{
    KSPIN_LOCK Lock;
    LIST_ENTRY ListHead;
    KEVENT Event;
} *sys_mbox_t;

typedef struct _sys_prot_t
{
    KSPIN_LOCK Lock;
    KIRQL OldIrql;
} sys_prot_t;

typedef u32_t sys_thread_t;

typedef struct _LWIP_MESSAGE_CONTAINER
{
    PVOID Message;
    LIST_ENTRY ListEntry;
} LWIP_MESSAGE_CONTAINER, *PLWIP_MESSAGE_CONTAINER;

#define sys_jiffies() sys_now()

/* NULL definitions */
#define SYS_MBOX_NULL NULL
#define SYS_SEM_NULL NULL
#define SYS_ARCH_NULL NULL

void
sys_arch_protect(sys_prot_t *lev);

void
sys_arch_unprotect(sys_prot_t *lev);

void
sys_arch_decl_protect(sys_prot_t *lev);

void
sys_shutdown(void);

