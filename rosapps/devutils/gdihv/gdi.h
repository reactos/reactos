typedef struct _GDI_TABLE_ENTRY
{
	PVOID KernelData; /* Points to the kernel mode structure */
	HANDLE ProcessId; /* process id that created the object, 0 for stock objects */
	LONG Type;        /* the first 16 bit is the object type including the stock obj flag, the last 16 bits is just the object type */
	PVOID UserData;   /* Points to the user mode structure, usually NULL though */
} GDI_TABLE_ENTRY, *PGDI_TABLE_ENTRY;

typedef PGDI_TABLE_ENTRY (CALLBACK * GDIQUERYPROC) (void);

/* GDI handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x10000
#define GDI_GLOBAL_PROCESS (0x0)

/* Handle Masks and shifts */
#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_BASETYPE_MASK 0x001f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_REUSECNT_SHIFT 24
#define GDI_HANDLE_UPPER_MASK 0x0000ffff

/* Handle macros */
#define GDI_HANDLE_CREATE(i, t)    \
    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) << 16)))

#define GDI_HANDLE_GET_INDEX(h)    \
    (((ULONG_PTR)(h)) & GDI_HANDLE_INDEX_MASK)

#define GDI_HANDLE_GET_TYPE(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK)

#define GDI_HANDLE_IS_TYPE(h, t)   \
    ((t) == (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK))

#define GDI_HANDLE_IS_STOCKOBJ(h)  \
    (0 != (((ULONG_PTR)(h)) & GDI_HANDLE_STOCK_MASK))

#define GDI_HANDLE_SET_STOCKOBJ(h) \
    ((h) = (HANDLE)(((ULONG_PTR)(h)) | GDI_HANDLE_STOCK_MASK))

#define GDI_HANDLE_GET_UPPER(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_UPPER_MASK)

#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
#define GDI_OBJECT_TYPE_COLORSPACE  0x00090000
#define GDI_OBJECT_TYPE_METADC      0x00660000
#define GDI_OBJECT_TYPE_METAFILE    0x00260000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00460000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/** Not really an object type. Forces GDI_FreeObj to be silent. */
#define GDI_OBJECT_TYPE_SILENT      0x80000000
