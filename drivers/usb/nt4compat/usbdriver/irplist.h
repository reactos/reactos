#ifndef __IRPLIST_H
#define __IRPLIST_H

#define MAX_IRP_LIST_SIZE	32

typedef struct _IRP_LIST_ELEMENT
{
	LIST_ENTRY  irp_link;
	PIRP		pirp;
	struct _URB *purb;

} IRP_LIST_ELEMENT, *PIRP_LIST_ELEMENT;

typedef struct _IRP_LIST
{
	KSPIN_LOCK 			irp_list_lock;
	LIST_HEAD			irp_busy_list;
	LONG				irp_free_list_count;
	LIST_HEAD			irp_free_list;
	PIRP_LIST_ELEMENT 	irp_list_element_array;

} IRP_LIST, *PIRP_LIST;

BOOLEAN
init_irp_list( 
PIRP_LIST irp_list
);

VOID
destroy_irp_list(
PIRP_LIST irp_list
);

BOOLEAN
add_irp_to_list(
PIRP_LIST irp_list,
PIRP pirp,
PURB purb
);

PURB
remove_irp_from_list(
PIRP_LIST irp_list,
PIRP pirp,
struct _USB_DEV_MANAGER *dev_mgr
);

BOOLEAN
irp_list_empty(
PIRP_LIST irp_list
);

BOOLEAN
irp_list_full(
PIRP_LIST irp_list
);

#endif
