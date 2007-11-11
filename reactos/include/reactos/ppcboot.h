#ifndef REACTOS_PPCBOOT_H
#define REACTOS_PPCBOOT_H

typedef enum _PPC_DT_BOOLEAN 
{
    PPC_DT_FALSE, PPC_DT_TRUE
} PPC_DT_BOOLEAN;

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef void * (*PPC_DEVICE_ALLOC)(int bytes);
typedef void (*PPC_DEVICE_FREE)(void *data);

typedef struct _PPC_DEVICE_NODE
{
    int type;
    int parent;
    short this_size, total_size;
    short value_offset, value_size;
    char name[1];
} PPC_DEVICE_NODE, *PPPC_DEVICE_NODE;

typedef struct _PPC_DEVICE_TREE
{
    int alloc_size, alloc_step, align, used_bytes;
    PPC_DEVICE_ALLOC allocFn;
    PPC_DEVICE_FREE freeFn;
    PPPC_DEVICE_NODE head, active;
} PPC_DEVICE_TREE, *PPPC_DEVICE_TREE;

typedef struct _PPC_DEVICE_RANGE
{
    int type;
    void *start;
    int len;
} PPC_DEVICE_RANGE, *PPPC_DEVICE_RANGE;

typedef char font_char[57];
typedef struct _boot_infos_t {
    void *loaderBlock;
    PPPC_DEVICE_NODE machine;
    int dispDeviceRect[4];
    int dispDeviceRowBytes;
    int dispDeviceDepth;
    void *dispDeviceBase;
    font_char *dispFont;
} boot_infos_t;

PPC_DT_BOOLEAN PpcDevTreeInitialize
(PPPC_DEVICE_TREE tree, int alloc_step, int align,
 PPC_DEVICE_ALLOC allocFn, PPC_DEVICE_FREE freeFn);
PPPC_DEVICE_NODE PpcDevTreeGetRootNode(PPPC_DEVICE_TREE tree);
PPC_DT_BOOLEAN PpcDevTreeNodeIsChild
(PPPC_DEVICE_NODE parent, PPPC_DEVICE_NODE child);
PPPC_DEVICE_NODE PpcDevTreeChildNode(PPPC_DEVICE_NODE parent);
PPPC_DEVICE_NODE PpcDevTreeParentNode(PPPC_DEVICE_NODE child);
PPPC_DEVICE_NODE PpcDevTreeSiblingNode(PPPC_DEVICE_NODE this_entry);
PPC_DT_BOOLEAN PpcDevTreeAddProperty
(PPPC_DEVICE_TREE tree, int type, char *propname, char *propval, int proplen);
PPC_DT_BOOLEAN PpcDevTreeAddDevice
(PPPC_DEVICE_TREE tree, int type, char *name);
PPC_DT_BOOLEAN PpcDevTreeCloseDevice(PPPC_DEVICE_TREE tree);
PPPC_DEVICE_NODE PpcDevTreeFindDevice
(PPPC_DEVICE_NODE root, int type, char *name);
char *PpcDevTreeFindProperty
(PPPC_DEVICE_NODE root, int type, char *name, int *len);

#define PPC_DT_FOURCC(w,x,y,z) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z))
#define PPC_DEVICE_SERIAL_8250  PPC_DT_FOURCC('8','2','5','0')
#define PPC_DEVICE_PCI_EAGLE    PPC_DT_FOURCC('E','g','l','e')
#define PPC_DEVICE_ISA_BUS      PPC_DT_FOURCC('I','S','A','b')
#define PPC_DEVICE_IDE_DISK     PPC_DT_FOURCC('I','D','E','c')
#define PPC_DEVICE_VGA          PPC_DT_FOURCC('V','G','A','c')

#define PPC_DEVICE_IO_RANGE     PPC_DT_FOURCC('I','O','r','g')
#define PPC_DEVICE_MEM_RANGE    PPC_DT_FOURCC('M','E','M','r')
#define PPC_DEVICE_VADDR        PPC_DT_FOURCC('v','a','d','r')
#define PPC_DEVICE_SPACE_RANGE  PPC_DT_FOURCC('r','a','n','g')
#define PPC_DEVICE_INTERRUPT    PPC_DT_FOURCC('I','n','t','r')

#endif/*REACTOS_PPCBOOT_H*/
