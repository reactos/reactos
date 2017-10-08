#include <sys/queue.h>


#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#define	STAILQ_FIRST(head)	((head)->stqh_first)
