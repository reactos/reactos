#ifndef _INTEL_H_
#define _INTEL_H_

#include "xf86drm.h"		/* drm_handle_t, etc */

/* Intel */
#ifndef PCI_CHIP_I810
#define PCI_CHIP_I810              0x7121
#define PCI_CHIP_I810_DC100        0x7123
#define PCI_CHIP_I810_E            0x7125
#define PCI_CHIP_I815              0x1132
#define PCI_CHIP_I810_BRIDGE       0x7120
#define PCI_CHIP_I810_DC100_BRIDGE 0x7122
#define PCI_CHIP_I810_E_BRIDGE     0x7124
#define PCI_CHIP_I815_BRIDGE       0x1130
#endif

#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M			0x3577

#ifndef PCI_CHIP_I855_GM
#define PCI_CHIP_I855_GM	   0x3582
#define PCI_CHIP_I855_GM_BRIDGE	   0x3580
#endif

#ifndef PCI_CHIP_I865_G
#define PCI_CHIP_I865_G		   0x2572
#define PCI_CHIP_I865_G_BRIDGE	   0x2570
#endif

#ifndef PCI_CHIP_I915_G
#define PCI_CHIP_I915_G		   0x2582
#define PCI_CHIP_I915_G_BRIDGE	   0x2580
#endif

#ifndef PCI_CHIP_I915_GM
#define PCI_CHIP_I915_GM	   0x2592
#define PCI_CHIP_I915_GM_BRIDGE	   0x2590
#endif

#ifndef PCI_CHIP_E7221_G
#define PCI_CHIP_E7221_G	   0x258A
/* Same as I915_G_BRIDGE */
#define PCI_CHIP_E7221_G_BRIDGE	   0x2580
#endif

#ifndef PCI_CHIP_I945_G
#define PCI_CHIP_I945_G        0x2772
#define PCI_CHIP_I945_G_BRIDGE 0x2770
#endif

#ifndef PCI_CHIP_I945_GM
#define PCI_CHIP_I945_GM        0x27A2
#define PCI_CHIP_I945_GM_BRIDGE 0x27A0
#endif

#define IS_I810(pI810) (pI810->Chipset == PCI_CHIP_I810 ||	\
			pI810->Chipset == PCI_CHIP_I810_DC100 || \
			pI810->Chipset == PCI_CHIP_I810_E)
#define IS_I815(pI810) (pI810->Chipset == PCI_CHIP_I815)
#define IS_I830(pI810) (pI810->Chipset == PCI_CHIP_I830_M)
#define IS_845G(pI810) (pI810->Chipset == PCI_CHIP_845_G)
#define IS_I85X(pI810)  (pI810->Chipset == PCI_CHIP_I855_GM)
#define IS_I852(pI810)  (pI810->Chipset == PCI_CHIP_I855_GM && (pI810->variant == I852_GM || pI810->variant == I852_GME))
#define IS_I855(pI810)  (pI810->Chipset == PCI_CHIP_I855_GM && (pI810->variant == I855_GM || pI810->variant == I855_GME))
#define IS_I865G(pI810) (pI810->Chipset == PCI_CHIP_I865_G)

#define IS_I915G(pI810) (pI810->Chipset == PCI_CHIP_I915_G || pI810->Chipset == PCI_CHIP_E7221_G)
#define IS_I915GM(pI810) (pI810->Chipset == PCI_CHIP_I915_GM)
#define IS_I945G(pI810) (pI810->Chipset == PCI_CHIP_I945_G)
#define IS_I945GM(pI810) (pI810->Chipset == PCI_CHIP_I945_GM)
#define IS_I9XX(pI810) (IS_I915G(pI810) || IS_I915GM(pI810) || IS_I945G(pI810) || IS_I945GM(pI810))

#define IS_MOBILE(pI810) (IS_I830(pI810) || IS_I85X(pI810) || IS_I915GM(pI810) || IS_I945GM(pI810))

#define I830_GMCH_CTRL		0x52

#define I830_GMCH_MEM_MASK      0x1
#define I830_GMCH_MEM_64M       0x1
#define I830_GMCH_MEM_128M      0

#define I830_GMCH_GMS_MASK			0x70
#define I830_GMCH_GMS_DISABLED		0x00
#define I830_GMCH_GMS_LOCAL			0x10
#define I830_GMCH_GMS_STOLEN_512	0x20
#define I830_GMCH_GMS_STOLEN_1024	0x30
#define I830_GMCH_GMS_STOLEN_8192	0x40

#define I855_GMCH_GMS_MASK			(0x7 << 4)
#define I855_GMCH_GMS_DISABLED			0x00
#define I855_GMCH_GMS_STOLEN_1M			(0x1 << 4)
#define I855_GMCH_GMS_STOLEN_4M			(0x2 << 4)
#define I855_GMCH_GMS_STOLEN_8M			(0x3 << 4)
#define I855_GMCH_GMS_STOLEN_16M		(0x4 << 4)
#define I855_GMCH_GMS_STOLEN_32M		(0x5 << 4)
#define I915G_GMCH_GMS_STOLEN_48M		(0x6 << 4)
#define I915G_GMCH_GMS_STOLEN_64M		(0x7 << 4)

typedef unsigned char Bool;
#define TRUE 1
#define FALSE 0

#define PIPE_NONE	0<<0
#define PIPE_CRT	1<<0
#define PIPE_TV		1<<1
#define PIPE_DFP	1<<2
#define PIPE_LFP	1<<3
#define PIPE_CRT2	1<<4
#define PIPE_TV2	1<<5
#define PIPE_DFP2	1<<6
#define PIPE_LFP2	1<<7

typedef struct _I830MemPool *I830MemPoolPtr;
typedef struct _I830MemRange *I830MemRangePtr;
typedef struct _I830MemRange {
   long Start;
   long End;
   long Size;
   unsigned long Physical;
   unsigned long Offset;		/* Offset of AGP-allocated portion */
   unsigned long Alignment;
   drm_handle_t Key;
   unsigned long Pitch; // add pitch
   I830MemPoolPtr Pool;
} I830MemRange;

typedef struct _I830MemPool {
   I830MemRange Total;
   I830MemRange Free;
   I830MemRange Fixed;
   I830MemRange Allocated;
} I830MemPool;

typedef struct {
   int tail_mask;
   I830MemRange mem;
   unsigned char *virtual_start;
   int head;
   int tail;
   int space;
} I830RingBuffer;

typedef struct _I830Rec {
   unsigned char *MMIOBase;
   unsigned char *FbBase;
   int cpp;
   uint32_t aper_size;
   unsigned int bios_version;

   /* These are set in PreInit and never changed. */
   long FbMapSize;
   long TotalVideoRam;
   I830MemRange StolenMemory;		/* pre-allocated memory */
   long BIOSMemorySize;			/* min stolen pool size */
   int BIOSMemSizeLoc;

   /* These change according to what has been allocated. */
   long FreeMemory;
   I830MemRange MemoryAperture;
   I830MemPool StolenPool;
   long allocatedMemory;

   /* Regions allocated either from the above pools, or from agpgart. */
   /* for single and dual head configurations */
   I830MemRange FrontBuffer;
   I830MemRange FrontBuffer2;
   I830MemRange Scratch;
   I830MemRange Scratch2;

   I830RingBuffer *LpRing;

   I830MemRange BackBuffer;
   I830MemRange DepthBuffer;
   I830MemRange TexMem;
   int TexGranularity;
   I830MemRange ContextMem;
   int drmMinor;
   Bool have3DWindows;

   Bool NeedRingBufferLow;
   Bool allowPageFlip;
   Bool disableTiling;

   int Chipset;
   unsigned long LinearAddr;
   unsigned long MMIOAddr;

   drmSize           registerSize;     /**< \brief MMIO register map size */
   drm_handle_t         registerHandle;   /**< \brief MMIO register map handle */
  //   IOADDRESS ioBase;
   int               irq;              /**< \brief IRQ number */
   int GttBound;

   drm_handle_t ring_map;
   unsigned int Fence[8];

} I830Rec;

/*
 * 12288 is set as the maximum, chosen because it is enough for
 * 1920x1440@32bpp with a 2048 pixel line pitch with some to spare.
 */
#define I830_MAXIMUM_VBIOS_MEM		12288
#define I830_DEFAULT_VIDEOMEM_2D	(MB(32) / 1024)
#define I830_DEFAULT_VIDEOMEM_3D	(MB(64) / 1024)

/* Flags for memory allocation function */
#define FROM_ANYWHERE			0x00000000
#define FROM_POOL_ONLY			0x00000001
#define FROM_NEW_ONLY			0x00000002
#define FROM_MASK			0x0000000f

#define ALLOCATE_AT_TOP			0x00000010
#define ALLOCATE_AT_BOTTOM		0x00000020
#define FORCE_GAPS			0x00000040

#define NEED_PHYSICAL_ADDR		0x00000100
#define ALIGN_BOTH_ENDS			0x00000200
#define FORCE_LOW			0x00000400

#define ALLOC_NO_TILING			0x00001000
#define ALLOC_INITIAL			0x00002000

#define ALLOCATE_DRY_RUN		0x80000000

/* Chipset registers for VIDEO BIOS memory RW access */
#define _855_DRAM_RW_CONTROL 0x58
#define _845_DRAM_RW_CONTROL 0x90
#define DRAM_WRITE    0x33330000

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * KB(1024))

#define GTT_PAGE_SIZE			KB(4)
#define ROUND_TO(x, y)			(((x) + (y) - 1) / (y) * (y))
#define ROUND_DOWN_TO(x, y)		((x) / (y) * (y))
#define ROUND_TO_PAGE(x)		ROUND_TO((x), GTT_PAGE_SIZE)
#define ROUND_TO_MB(x)			ROUND_TO((x), MB(1))
#define PRIMARY_RINGBUFFER_SIZE		KB(128)


/* Ring buffer registers, p277, overview p19
 */
#define LP_RING     0x2030
#define HP_RING     0x2040

#define RING_TAIL      0x00
#define TAIL_ADDR           0x000FFFF8
#define I830_TAIL_MASK	    0x001FFFF8

#define RING_HEAD      0x04
#define HEAD_WRAP_COUNT     0xFFE00000
#define HEAD_WRAP_ONE       0x00200000
#define HEAD_ADDR           0x001FFFFC
#define I830_HEAD_MASK      0x001FFFFC

#define RING_START     0x08
#define START_ADDR          0x03FFFFF8
#define I830_RING_START_MASK	0xFFFFF000

#define RING_LEN       0x0C
#define RING_NR_PAGES       0x001FF000 
#define I830_RING_NR_PAGES	0x001FF000
#define RING_REPORT_MASK    0x00000006
#define RING_REPORT_64K     0x00000002
#define RING_REPORT_128K    0x00000004
#define RING_NO_REPORT      0x00000000
#define RING_VALID_MASK     0x00000001
#define RING_VALID          0x00000001
#define RING_INVALID        0x00000000


/* Fence/Tiling ranges [0..7]
 */
#define FENCE            0x2000
#define FENCE_NR         8

#define I915G_FENCE_START_MASK	0x0ff00000

#define I830_FENCE_START_MASK	0x07f80000

#define FENCE_START_MASK    0x03F80000
#define FENCE_X_MAJOR       0x00000000
#define FENCE_Y_MAJOR       0x00001000
#define FENCE_SIZE_MASK     0x00000700
#define FENCE_SIZE_512K     0x00000000
#define FENCE_SIZE_1M       0x00000100
#define FENCE_SIZE_2M       0x00000200
#define FENCE_SIZE_4M       0x00000300
#define FENCE_SIZE_8M       0x00000400
#define FENCE_SIZE_16M      0x00000500
#define FENCE_SIZE_32M      0x00000600
#define FENCE_SIZE_64M	    0x00000700
#define I915G_FENCE_SIZE_1M       0x00000000
#define I915G_FENCE_SIZE_2M       0x00000100
#define I915G_FENCE_SIZE_4M       0x00000200
#define I915G_FENCE_SIZE_8M       0x00000300
#define I915G_FENCE_SIZE_16M      0x00000400
#define I915G_FENCE_SIZE_32M      0x00000500
#define I915G_FENCE_SIZE_64M	0x00000600
#define I915G_FENCE_SIZE_128M	0x00000700
#define FENCE_PITCH_1       0x00000000
#define FENCE_PITCH_2       0x00000010
#define FENCE_PITCH_4       0x00000020
#define FENCE_PITCH_8       0x00000030
#define FENCE_PITCH_16      0x00000040
#define FENCE_PITCH_32      0x00000050
#define FENCE_PITCH_64	    0x00000060
#define FENCE_VALID         0x00000001

#include <mmio.h>

#  define MMIO_IN8(base, offset) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset))
#  define MMIO_IN32(base, offset) \
	read_MMIO_LE32(base, offset)
#  define MMIO_OUT8(base, offset, val) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset)) = (val)
#  define MMIO_OUT32(base, offset, val) \
	*(volatile unsigned int *)(void *)(((unsigned char*)(base)) + (offset)) = CPU_TO_LE32(val)


				/* Memory mapped register access macros */
#define INREG8(addr)        MMIO_IN8(MMIO, addr)
#define INREG(addr)         MMIO_IN32(MMIO, addr)
#define OUTREG8(addr, val)  MMIO_OUT8(MMIO, addr, val)
#define OUTREG(addr, val)   MMIO_OUT32(MMIO, addr, val)

#define DSPABASE		0x70184

#endif
