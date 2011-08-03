/* HACK: broken ntddk.h */
#ifdef KMT_KERNEL_MODE
typedef struct _RTL_SPLAY_LINKS {
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;
#endif

#define RTL_USE_AVL_TABLES
#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

#if defined KMT_USER_MODE
/* HACK: missing in rtltypes.h */
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define RTL_GENERIC_TABLE               RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE              PRTL_AVL_TABLE

/* HACK: missing in rtlfuncs.h */
#define RtlInitializeGenericTable       RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable    RtlInsertElementGenericTableAvl
#define RtlDeleteElementGenericTable    RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable    RtlLookupElementGenericTableAvl
#define RtlEnumerateGenericTable        RtlEnumerateGenericTableAvl
#define RtlGetElementGenericTable       RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements   RtlNumberGenericTableElementsAvl
#endif

/* this is a little hacky, but better than duplicating the code (for now) */
#define Test_RtlSplayTree Test_RtlAvlTree
#include "RtlSplayTree.c"
