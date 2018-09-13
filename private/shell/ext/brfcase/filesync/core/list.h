/*
 * list.h - List ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HLIST);
DECLARE_STANDARD_TYPES(HLIST);

DECLARE_HANDLE(HNODE);
DECLARE_STANDARD_TYPES(HNODE);

/*
 * sorted list node comparison callback function
 *
 * The first pointer is reference data and the second pointer is a list node
 * data element.
 */

typedef COMPARISONRESULT (*COMPARESORTEDNODESPROC)(PCVOID, PCVOID);

/*
 * unsorted list node comparison callback function
 *
 * The first pointer is reference data and the second pointer is a list node
 * data element.
 */

typedef BOOL (*COMPAREUNSORTEDNODESPROC)(PCVOID, PCVOID);

/*
 * WalkList() callback function - called as:
 *
 *    bContinue = WalkList(pvNodeData, pvRefData);
 */

typedef BOOL (*WALKLIST)(PVOID, PVOID);

/* new list flags */

typedef enum _newlistflags
{
   /* Insert nodes in sorted order. */

   NL_FL_SORTED_ADD        = 0x0001,

   /* flag combinations */

   ALL_NL_FLAGS            = NL_FL_SORTED_ADD
}
NEWLISTFLAGS;

/* new list description */

typedef struct _newlist
{
   DWORD dwFlags;
}
NEWLIST;
DECLARE_STANDARD_TYPES(NEWLIST);


/* Prototypes
 *************/

/* list.c */

extern BOOL CreateList(PCNEWLIST, PHLIST);
extern void DestroyList(HLIST);
extern BOOL AddNode(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeAtFront(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeBefore(HNODE, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeAfter(HNODE, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern void DeleteNode(HNODE);
extern void DeleteAllNodes(HLIST);
extern PVOID GetNodeData(HNODE);
extern void SetNodeData(HNODE, PCVOID);
extern ULONG GetNodeCount(HLIST);
extern BOOL IsListEmpty(HLIST);
extern BOOL GetFirstNode(HLIST, PHNODE);
extern BOOL GetNextNode(HNODE, PHNODE);
extern BOOL GetPrevNode(HNODE, PHNODE);
extern void AppendList(HLIST, HLIST);
extern BOOL SearchSortedList(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL SearchUnsortedList(HLIST, COMPAREUNSORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL WalkList(HLIST, WALKLIST, PVOID);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidHLIST(HLIST);
extern BOOL IsValidHNODE(HNODE);

#endif

