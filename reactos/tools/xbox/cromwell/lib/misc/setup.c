#include "boot.h"
#include "memory_layout.h"

/* parameters to be passed to the kernel */
/* most parameters are documented in linux/Documentation/i386/zero-page.txt */
struct kernel_setup_t {
	unsigned char  orig_x;                  /* 0x00 */
	unsigned char  orig_y;                  /* 0x01 */
	unsigned short ext_mem_k;		/* 0x02 -- EXT_MEM_K sits here */
	unsigned short orig_video_page;         /* 0x04 */
	unsigned char  orig_video_mode;         /* 0x06 */
	unsigned char  orig_video_cols;         /* 0x07 */
	unsigned short unused2;                 /* 0x08 */
	unsigned short orig_video_ega_bx;       /* 0x0a */
	unsigned short unused3;                 /* 0x0c */
	unsigned char  orig_video_lines;        /* 0x0e */
	unsigned char  orig_video_isVGA;        /* 0x0f */
	unsigned short orig_video_points;       /* 0x10 */

	/* VESA graphic mode -- linear frame buffer */
	unsigned short lfb_width;               /* 0x12 */
	unsigned short lfb_height;              /* 0x14 */
	unsigned short lfb_depth;               /* 0x16 */
	unsigned long  lfb_base;                /* 0x18 */
	unsigned long  lfb_size;                /* 0x1c */
	unsigned short cmd_magic;	/*  32: Command line magic 0xA33F */
	unsigned short cmd_offset;	/*  34: Command line offset from 0x90000 */
	unsigned short lfb_linelength;          /* 0x24 */
	unsigned char  red_size;                /* 0x26 */
	unsigned char  red_pos;                 /* 0x27 */
	unsigned char  green_size;              /* 0x28 */
	unsigned char  green_pos;               /* 0x29 */
	unsigned char  blue_size;               /* 0x2a */
	unsigned char  blue_pos;                /* 0x2b */
	unsigned char  rsvd_size;               /* 0x2c */
	unsigned char  rsvd_pos;                /* 0x2d */
	unsigned short vesapm_seg;              /* 0x2e */
	unsigned short vesapm_off;              /* 0x30 */
	unsigned short pages;                   /* 0x32 */
	char __pad2[436];
	unsigned char  e820_map_nr; /* 488: 0x1e8 number of entries in e820 map */
	char __pad3[8];
	unsigned char  setup_sects; /* 497: 0x1f1  setup size in sectors (512) */
	unsigned short root_flags;	/* 498: 0x1f2  1 = ro ; 0 = rw */
	unsigned short kernel_para;	/* 500: 0x1f4 kernel size in paragraphs (16) syssize bootsect.S */
	unsigned short swap_dev;	/* 502: 0x1f6  OBSOLETED */
	unsigned short ram_size;	/* 504: 0x1f8 bootsect.S*/
	unsigned short vid_mode;	/* 506: 0x1fa */
	unsigned short root_dev;	/* 508: 0x1fc 0x301 */
	unsigned short boot_flag;	/* 510: 0x1fe signature 0xaa55 */
	unsigned short jump;        /* 512: 0x200 jump to startup code */
	char signature[4];          /* 514: 0x202 "HdrS" */
	unsigned short version;     /* 518: 0x206 header version 0x203*/
	unsigned long pHookRealmodeSwitch; /* 520 0x208 hook */
	unsigned short start_sys;           /* 524: 0x20c start_sys */
	unsigned short ver_offset;  /* 526: 0x20e kernel version string */
	unsigned char loader;       /* 528: 0x210 loader type */
	unsigned char flags;        /* 529: 0x211 loader flags */
	unsigned short a;           /* 530: 0x212 setup move size more LOADLIN hacks */
	unsigned long start;        /* 532: 0x214 kernel start, filled in by loader */
	unsigned long ramdisk;      /* 536: 0x218 RAM disk start address */
	unsigned long ramdisk_size; /* 540: 0x21c RAM disk size */
	unsigned short b,c;         /* 544: 0x220 bzImage hacks */
	unsigned short heap_end_ptr;/* 548: 0x224 end of free area after setup code */
	unsigned char __pad4[2];  // was wrongfully [4]
	unsigned int cmd_line_ptr;  /* 552: pointer to command line */
	unsigned int initrd_addr_max;/*556: highest address that can be used by initrd */
	unsigned char __pad5[160];
	unsigned long long e820_addr1; /* 720: first e820 map entry */
	unsigned long long e820_size1;
	unsigned long e820_type1;
	unsigned long long e820_addr2; /* 740: second e820 map entry */
	unsigned long long e820_size2;
	unsigned long e820_type2;
	unsigned long long e820_addr3; /* 760: third e820 map entry */
	unsigned long long e820_size3;
	unsigned long e820_type3;
	unsigned long long e820_addr4; /* 780: fourth e820 map entry */
	unsigned long long e820_size4;
	unsigned long e820_type4;
};

extern void* framebuffer;

void setup(void* KernelPos, void* PhysInitrdPos, unsigned long InitrdSize, const char* kernel_cmdline) {
    
    struct kernel_setup_t *kernel_setup = (struct kernel_setup_t*)KernelPos;

    memset(kernel_setup->__pad2,0x00,sizeof(kernel_setup->__pad2));
    memset(kernel_setup->__pad3,0x00,sizeof(kernel_setup->__pad3));
    memset(kernel_setup->__pad4,0x00,sizeof(kernel_setup->__pad4));
    memset(kernel_setup->__pad5,0x00,sizeof(kernel_setup->__pad5));
    kernel_setup->unused2=0;
    kernel_setup->unused3=0;
    
    /* init kernel parameters */
    kernel_setup->loader = 0xff;		/* must be != 0 */
    kernel_setup->heap_end_ptr = 0xffff;	/* 64K heap */
    kernel_setup->flags = 0x81;			/* loaded high, heap existant */
    kernel_setup->start = KERNEL_PM_CODE;
    kernel_setup->ext_mem_k = (((xbox_ram-1) * 1024) - FB_SIZE / 1024) & 0xffff ; /* now replaced by e820 map */

    /* initrd */
    /* ED : only if initrd */

    if(InitrdSize != 0) {
	    kernel_setup->ramdisk = (long)PhysInitrdPos;
	    kernel_setup->ramdisk_size = InitrdSize;
    	kernel_setup->initrd_addr_max = RAMSIZE_USE - 1;
    }

    /* Framebuffer setup */
    kernel_setup->orig_video_isVGA = 0x23;
    kernel_setup->orig_x = 0;
    kernel_setup->orig_y = 0;
    if(vmode.width==640) {

	kernel_setup->vid_mode = 0x312;		/* 640x480x16M Colors (works if you are already in x576 as well)*/
    } else {
	kernel_setup->vid_mode = 0x315;		/* 800x600x16M Colors */
    }

    kernel_setup->orig_video_mode = kernel_setup->vid_mode-0x300;
    kernel_setup->orig_video_cols = vmode.width/8;
    kernel_setup->orig_video_lines = vmode.height/16;
    kernel_setup->orig_video_ega_bx = 0;
    kernel_setup->orig_video_points = 16;
    kernel_setup->lfb_depth = 32;
    kernel_setup->lfb_width = vmode.width;

    kernel_setup->lfb_height = vmode.height; // SCREEN_HEIGHT_480;
    
    kernel_setup->lfb_base =  (0xf0000000 | ((xbox_ram*0x100000) - FB_SIZE));

    kernel_setup->lfb_size = FB_SIZE / 0x10000;

    kernel_setup->lfb_linelength = vmode.width*4;
    kernel_setup->pages=1;
    kernel_setup->vesapm_seg = 0;
    kernel_setup->vesapm_off = 0;
    kernel_setup->blue_size = 8;
    kernel_setup->blue_pos = 0;
    kernel_setup->green_size = 8;
    kernel_setup->green_pos = 8;
    kernel_setup->red_size = 8;
    kernel_setup->red_pos = 16;
    kernel_setup->rsvd_size = 8;
    kernel_setup->rsvd_pos = 24;

    kernel_setup->root_dev=0x0301; // 0x0301..?? /dev/hda1 default if no comline override given
    kernel_setup->pHookRealmodeSwitch=0;
    kernel_setup->start_sys=0;
    kernel_setup->a=0;
    kernel_setup->b=0;
    kernel_setup->c=0;
    
    kernel_setup->root_flags=0; // allow read/write

    /* setup e820 memory map */
    kernel_setup->e820_map_nr=4;
    kernel_setup->e820_addr1=0x0000000000000000;
    kernel_setup->e820_size1=0x000000000009f000;
    kernel_setup->e820_type1=1; /* RAM */
    kernel_setup->e820_addr2=0x000000000009f000;
    kernel_setup->e820_size2=0x0000000000061000;
    kernel_setup->e820_type2=2; /* Reserved, legacy memory region */
    kernel_setup->e820_addr3=0x0000000000100000;
    kernel_setup->e820_size3=(xbox_ram - 1) * 1024 * 1024 - FB_SIZE;
    kernel_setup->e820_type3=1; /* RAM*/
    kernel_setup->e820_addr4=(xbox_ram * 1024 * 1024) - FB_SIZE;
    kernel_setup->e820_size4=FB_SIZE;
    kernel_setup->e820_type4=2; /* Reserved, framebuffer */

    /* set command line */

    kernel_setup->cmd_offset = 0;
    kernel_setup->cmd_magic = 0xA33F;
    kernel_setup->cmd_line_ptr = (int)(CMD_LINE_LOC); /* place cmd_line at 0x90800 */
    strncpy((void*)kernel_setup->cmd_line_ptr, kernel_cmdline, 512);
    *((char*)(kernel_setup->cmd_line_ptr+511)) = 0x00; /* make sure string is terminated */
}
