//
//  Define the two pointer triangle splay links and the associated
//  manipuliation macros and routines.  Note that the tri_splay_links should
//  be an opaque type.  Routine are provided to traverse and manipulate the
//  structure.
//
//  The structure of a tri_splay_links record is really
//
//      typedef struct _TRI_SPLAY_LINKS {
//          ULONG ParSib; // struct _TRI_SPLAY_LINKS *ParSib;
//          ULONG Child;  // struct _TRI_SPLAY_LINKS *Child;
//      } TRI_SPLAY_LINKS;
//
//  However to aid in debugging (and without extra cost) we declare the
//  structure to be a union so we can also reference the links as pointers
//  in the debugger.
//

typedef union _TRI_SPLAY_LINKS {
    struct {
        ULONG ParSib;
        ULONG Child;
    } Refs;
    struct {
        union _TRI_SPLAY_LINKS *ParSibPtr;
        union _TRI_SPLAY_LINKS *ChildPtr;
    } Ptrs;
} TRI_SPLAY_LINKS;
typedef TRI_SPLAY_LINKS *PTRI_SPLAY_LINKS;

//
//  The macro procedure InitializeSplayLinks takes as input a pointer to
//  splay link and initializes its substructure.  All splay link nodes must
//  be initialized before they are used in the different splay routines and
//  macros.
//
// VOID
// TriInitializeSplayLinks (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriInitializeSplayLinks(Links) { \
    (Links)->Refs.ParSib = MakeIntoParentRef(Links); \
    (Links)->Refs.Child = 0; \
    }

//
//  The macro function Parent takes as input a pointer to a splay link in a
//  tree and returns a pointer to the splay link of the parent of the input
//  node.  If the input node is the root of the tree the return value is
//  equal to the input value.
//
// PTRI_SPLAY_LINKS
// TriParent (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriParent(Links) ( \
    (IsParentRef((Links)->Refs.ParSib)) ? \
        MakeIntoPointer((Links)->Refs.ParSib) \
    : \
        MakeIntoPointer(MakeIntoPointer((Links)->Refs.ParSib)->Refs.ParSib) \
    )

//
//  The macro function LeftChild takes as input a pointer to a splay link in
//  a tree and returns a pointer to the splay link of the left child of the
//  input node.  If the left child does not exist, the return value is NULL.
//
// PTRI_SPLAY_LINKS
// TriLeftChild (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriLeftChild(Links) ( \
    (IsLeftChildRef((Links)->Refs.Child)) ? \
        MakeIntoPointer((Links)->Refs.Child) \
    : \
        0 \
    )

//
//  The macro function RightChild takes as input a pointer to a splay link
//  in a tree and returns a pointer to the splay link of the right child of
//  the input node.  If the right child does not exist, the return value is
//  NULL.
//
// PTRI_SPLAY_LINKS
// TriRightChild (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriRightChild(Links) ( \
    (IsRightChildRef((Links)->Refs.Child)) ? \
        MakeIntoPointer((Links)->Refs.Child) \
    : ( \
        (IsLeftChildRef((Links)->Refs.Child) && \
         IsSiblingRef(MakeIntoPointer((Links)->Refs.Child)->Refs.ParSib)) ? \
            MakeIntoPointer(MakeIntoPointer((Links)->Refs.Child)->Refs.ParSib) \
        : \
            0 \
        ) \
    )

//
//  The macro function IsRoot takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the root of the tree,
//  otherwise it returns FALSE.
//
// BOOLEAN
// TriIsRoot (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriIsRoot(Links) ( \
    (IsParentRef((Links)->Refs.ParSib) && MakeIntoPointer((Links)->Refs.ParSib) == (Links)) ? \
        TRUE \
    : \
        FALSE \
    )

//
//  The macro function IsLeftChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the left child of its
//  parent, otherwise it returns FALSE.  Note that if the input link is the
//  root node this function returns FALSE.
//
// BOOLEAN
// TriIsLeftChild (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriIsLeftChild(Links) ( \
    (TriLeftChild(TriParent(Links)) == (Links)) ? \
        TRUE \
    : \
        FALSE \
    )

//
//  The macro function IsRightChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the right child of its
//  parent, otherwise it returns FALSE.  Note that if the input link is the
//  root node this function returns FALSE.
//
// BOOLEAN
// TriIsRightChild (
//     IN PTRI_SPLAY_LINKS Links
//     );
//

#define TriIsRightChild(Links) ( \
    (TriRightChild(TriParent(Links)) == (Links)) ? \
        TRUE \
    : \
        FALSE \
    )

//
//  The macro procedure InsertAsLeftChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the left child of the first node.  The first node must not
//  already have a left child, and the second node must not already have a
//  parent.
//
// VOID
// TriInsertAsLeftChild (
//     IN PTRI_SPLAY_LINKS ParentLinks,
//     IN PTRI_SPLAY_LINKS ChildLinks
//     );
//

#define TriInsertAsLeftChild(ParentLinks,ChildLinks) { \
    PTRI_SPLAY_LINKS RightChild; \
    if ((ParentLinks)->Refs.Child == 0) { \
        (ParentLinks)->Refs.Child = MakeIntoLeftChildRef(ChildLinks); \
        (ChildLinks)->Refs.ParSib = MakeIntoParentRef(ParentLinks); \
    } else { \
        RightChild = TriRightChild(ParentLinks); \
        (ParentLinks)->Refs.Child = MakeIntoLeftChildRef(ChildLinks); \
        (ChildLinks)->Refs.ParSib = MakeIntoSiblingRef(RightChild); \
    } \
}

//
//  The macro procedure InsertAsRightChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the right child of the first node.  The first node must not
//  already have a right child, and the second node must not already have a
//  parent.
//
// VOID
// TriInsertAsRightChild (
//     IN PTRI_SPLAY_LINKS ParentLinks,
//     IN PTRI_SPLAY_LINKS ChildLinks
//     );
//

#define TriInsertAsRightChild(ParentLinks,ChildLinks) { \
    PTRI_SPLAY_LINKS LeftChild; \
    if ((ParentLinks)->Refs.Child == 0) { \
        (ParentLinks)->Refs.Child = MakeIntoRightChildRef(ChildLinks); \
        (ChildLinks)->Refs.ParSib = MakeIntoParentRef(ParentLinks); \
    } else { \
        LeftChild = TriLeftChild(ParentLinks); \
        LeftChild->Refs.ParSib = MakeIntoSiblingRef(ChildLinks); \
        (ChildLinks)->Refs.ParSib = MakeIntoParentRef(ParentLinks); \
    } \
}

//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

PTRI_SPLAY_LINKS
TriSplay (
    IN PTRI_SPLAY_LINKS Links
    );

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//

PTRI_SPLAY_LINKS
TriDelete (
    IN PTRI_SPLAY_LINKS Links
    );

//
//  The SubtreeSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node of
//  the substree rooted at the input node.  If there is not a successor, the
//  return value is NULL.
//

PTRI_SPLAY_LINKS
TriSubtreeSuccessor (
    IN PTRI_SPLAY_LINKS Links
    );

//
//  The SubtreePredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node of
//  the substree rooted at the input node.  If there is not a predecessor,
//  the return value is NULL.
//

PTRI_SPLAY_LINKS
TriSubtreePredecessor (
    IN PTRI_SPLAY_LINKS Links
    );

//
//  The RealSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node within
//  the entire tree.  If there is not a successor, the return value is NULL.
//

PTRI_SPLAY_LINKS
TriRealSuccessor (
    IN PTRI_SPLAY_LINKS Links
    );

//
//  The RealPredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node
//  within the entire tree.  If there is not a predecessor, the return value
//  is NULL.
//

PTRI_SPLAY_LINKS
TriRealPredecessor (
    IN PTRI_SPLAY_LINKS Links
    );


//
//  The remainder of this module really belong in triangle.c  None of
//  the macros or routines are (logically) exported for use by the programmer
//  however they need to appear in this module to allow the earlier macros
//  to function properly.
//
//  In the splay record (declared earlier) the low order bit of the
//  ParSib field indicates whether the link is to a Parent or a Sibling, and
//  the low order bit of the Child field is used to indicate if the link
//  is to a left child or a right child.  The values are:
//
//      A parent field has the lower bit set to 0
//      A sibling field has the lower bit set to 1
//      A left child field has the lower bit set to 0
//      A right child field has the lower bit set to 1
//
//  The comments and code in triangle.c use the term "Ref" to indicate a
//  ParSib field or a Child field with the low order bit to indicate its type.
//  A ref cannot be directly used as a pointer.  The following macros help
//  in deciding the type of a ref and making refs from pointers.  There is
//  also a macro (MakeIntoPointer) that takes a ref and returns a pointer.
//

#define IsParentRef(Ulong)           (((((ULONG)Ulong) & 1) == 0) && ((Ulong) != 0) ? TRUE : FALSE)
#define MakeIntoParentRef(Ulong)     (((ULONG)Ulong) & 0xfffffffc)

#define IsSiblingRef(Ulong)          ((((ULONG)Ulong) & 1) == 1 ? TRUE : FALSE)
#define MakeIntoSiblingRef(Ulong)    (((ULONG)Ulong) | 1)

#define IsLeftChildRef(Ulong)        (((((ULONG)Ulong) & 1) == 0) && ((Ulong) != 0) ? TRUE : FALSE)
#define MakeIntoLeftChildRef(Ulong)  (((ULONG)Ulong) & 0xfffffffc)

#define IsRightChildRef(Ulong)       ((((ULONG)Ulong) & 1) == 1 ? TRUE : FALSE)
#define MakeIntoRightChildRef(Ulong) (((ULONG)Ulong) | 1)

#define MakeIntoPointer(Ulong)       ((PTRI_SPLAY_LINKS)((Ulong) & 0xfffffffc))


