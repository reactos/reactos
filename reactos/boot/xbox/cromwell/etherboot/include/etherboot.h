#include "etherboot_config.h"
#include "osdep.h"

#ifndef BOOT_FIRST
#define BOOT_FIRST	BOOT_NIC
#endif
#ifndef BOOT_SECOND
#define BOOT_SECOND	BOOT_NOTHING
#endif
#ifndef BOOT_THIRD
#define BOOT_THIRD	BOOT_NOTHING
#endif

#define DEFAULT_BOOT_ORDER ( \
	(BOOT_FIRST   << (0*BOOT_BITS)) | \
	(BOOT_SECOND  << (1*BOOT_BITS)) | \
	(BOOT_THIRD   << (2*BOOT_BITS)) | \
	(BOOT_NOTHING << (3*BOOT_BITS)) | \
	0)

#ifdef BOOT_INDEX
#define DEFAULT_BOOT_INDEX BOOT_INDEX
#else
#define DEFAULT_BOOT_INDEX 0
#endif

#if	!defined(TAGGED_IMAGE) && !defined(AOUT_IMAGE) && !defined(ELF_IMAGE) && !defined(ELF64_IMAGE) && !defined(COFF_IMAGE)
#define	TAGGED_IMAGE		/* choose at least one */
#endif

#define K_ESC		'\033'
#define K_EOF		'\04'  /* Ctrl-D */
#define K_INTR		'\03'  /* Ctrl-C */

/*  Edit this to change the path to hostspecific kernel image
    kernel.<client_ip_address> in RARP boot */
#ifndef	DEFAULT_KERNELPATH
#define	DEFAULT_KERNELPATH	"/tftpboot/kernel.%@"
#endif

#ifdef FREEBSD_PXEEMU
#undef DEFAULT_BOOTFILE
#ifndef PXENFSROOTPATH
#define PXENFSROOTPATH ""
#endif
#define DEFAULT_BOOTFILE	PXENFSROOTPATH "/boot/pxeboot"
#endif

/* Clean up console settings... mainly CONSOLE_FIRMWARE and CONSOLE_SERIAL are used
 * in the sources (except start.S and serial.S which cannot include
 * etherboot.h).  At least one of the CONSOLE_xxx has to be set, and
 * CONSOLE_DUAL sets both CONSOLE_CRT and CONSOLE_SERIAL.  If none is set,
 * CONSOLE_CRT is assumed.  */
#ifdef CONSOLE_CRT
#define CONSOLE_FIRMWARE
#endif
#ifdef	CONSOLE_DUAL
#undef CONSOLE_FIRMWARE
#define CONSOLE_FIRMWARE
#undef CONSOLE_SERIAL
#define CONSOLE_SERIAL
#endif
#if	defined(CONSOLE_FIRMWARE) && defined(CONSOLE_SERIAL)
#undef CONSOLE_DUAL
#define CONSOLE_DUAL
#endif
#if	!defined(CONSOLE_FIRMWARE) && !defined(CONSOLE_SERIAL)
#define CONSOLE_FIRMWARE
#endif

#if	!defined(DOWNLOAD_PROTO_TFTP) && !defined(DOWNLOAD_PROTO_NFS) && !defined(DOWNLOAD_PROTO_SLAM) && !defined(DOWNLOAD_PROTO_TFTM) && !defined(DOWNLOAD_PROTO_DISK)
#error No download protocol defined!
#endif

#ifndef	MAX_TFTP_RETRIES
#define MAX_TFTP_RETRIES	20
#endif

#ifndef	MAX_BOOTP_RETRIES
#define MAX_BOOTP_RETRIES	20
#endif

#define MAX_BOOTP_EXTLEN	(ETH_MAX_MTU-sizeof(struct bootpip_t))

#ifndef	MAX_ARP_RETRIES
#define MAX_ARP_RETRIES		20
#endif

#ifndef	MAX_RPC_RETRIES
#define MAX_RPC_RETRIES		20
#endif

/* Link configuration time in tenths of a second */
#ifndef VALID_LINK_TIMEOUT
#define VALID_LINK_TIMEOUT	100 /* 10.0 seconds */
#endif

/* Inter-packet retry in ticks */
#ifndef TIMEOUT
#define TIMEOUT			(10*TICKS_PER_SEC)
#endif

/* Max interval between IGMP packets */
#define IGMP_INTERVAL			(10*TICKS_PER_SEC)
#define IGMPv1_ROUTER_PRESENT_TIMEOUT	(400*TICKS_PER_SEC)

/* These settings have sense only if compiled with -DCONGESTED */
/* total retransmission timeout in ticks */
#define TFTP_TIMEOUT		(30*TICKS_PER_SEC)
/* packet retransmission timeout in ticks */
#define TFTP_REXMT		(3*TICKS_PER_SEC)

#ifndef	NULL
#define NULL	((void *)0)
#endif

#include	"if_ether.h"

#define ARP_CLIENT	0
#define ARP_SERVER	1
#define ARP_GATEWAY	2
#define MAX_ARP		ARP_GATEWAY+1

#define IGMP_SERVER	0
#define MAX_IGMP	IGMP_SERVER+1

#define	RARP_REQUEST	3
#define	RARP_REPLY	4

#include	"in.h"

#define MULTICAST_MASK    0xF0000000
#define MULTICAST_NETWORK 0xE0000000

/* Helper macros used to identify when DHCP options are valid/invalid in/outside of encapsulation */
#define NON_ENCAP_OPT in_encapsulated_options == 0 &&
#ifdef ALLOW_ONLY_ENCAPSULATED
#define ENCAP_OPT in_encapsulated_options == 1 &&
#else
#define ENCAP_OPT
#endif

#include	"if_arp.h"
#include	"ip.h"
#include	"udp.h"
#include	"bootp.h"
#include	"tftp.h"
#include	"igmp.h"
#include	"nfs.h"

struct arptable_t {
	in_addr ipaddr;
	uint8_t node[6];
};

struct igmptable_t {
	in_addr group;
	unsigned long time;
};

#define	KERNEL_BUF	(BOOTP_DATA_ADDR->bootp_reply.bp_file)

#define	FLOPPY_BOOT_LOCATION	0x7c00
/* Must match offsets in loader.S */
#define ROM_SEGMENT		0x1fa
#define ROM_LENGTH		0x1fc

#define	ROM_INFO_LOCATION	(FLOPPY_BOOT_LOCATION+ROM_SEGMENT)
/* at end of floppy boot block */

struct rom_info {
	unsigned short	rom_segment;
	unsigned short	rom_length;
};

extern inline int rom_address_ok(struct rom_info *rom, int assigned_rom_segment)
{
	return (assigned_rom_segment < 0xC000
		|| assigned_rom_segment == rom->rom_segment);
}


/* Define a type for passing info to a loaded program */
struct ebinfo {
	uint8_t  major, minor;	/* Version */
	uint16_t flags;		/* Bit flags */
};

/***************************************************************************
External prototypes
***************************************************************************/
/* main.c */
struct Elf_Bhdr;
extern int main(struct Elf_Bhdr *ptr);
extern int loadkernel P((const char *fname));
/* nic.c */
extern void rx_qdrain P((void));
extern int tftp P((const char *name, int (*)(unsigned char *, unsigned int, unsigned int, int)));
extern int ip_transmit P((int len, const void *buf));
extern void build_ip_hdr P((unsigned long destip, int ttl, int protocol, 
	int option_len, int len, const void *buf));
extern void build_udp_hdr P((unsigned long destip, 
	unsigned int srcsock, unsigned int destsock, int ttl,
	int len, const void *buf));
extern int udp_transmit P((unsigned long destip, unsigned int srcsock,
	unsigned int destsock, int len, const void *buf));
typedef int (*reply_t)(int ival, void *ptr, unsigned short ptype, struct iphdr *ip, struct udphdr *udp);
extern int await_reply P((reply_t reply,	int ival, void *ptr, long timeout));
extern int decode_rfc1533 P((unsigned char *, unsigned int, unsigned int, int));
extern void join_group(int slot, unsigned long group);
extern void leave_group(int slot);
#define RAND_MAX 2147483647L
extern uint16_t ipchksum P((const void *ip, unsigned long len));
extern uint16_t add_ipchksums P((unsigned long offset, uint16_t sum, uint16_t new));
extern int32_t random P((void));
extern long rfc2131_sleep_interval P((long base, int exp));
extern long rfc1112_sleep_interval P((long base, int exp));
#ifndef DOWNLOAD_PROTO_TFTP
#define	tftp(fname, load_block) 0
#endif
extern void cleanup P((void));

/* nfs.c */
extern void rpc_init(void);
extern int nfs P((const char *name, int (*)(unsigned char *, unsigned int, unsigned int, int)));
extern void nfs_umountall P((int));

/* proto_slam.c */
extern int url_slam P((const char *name, int (*fnc)(unsigned char *, unsigned int, unsigned int, int)));

/* proto_tftm.c */
extern int url_tftm P((const char *name, int (*fnc)(unsigned char *, unsigned int, unsigned int, int)));

/* config.c */
extern void print_config(void);

/* heap.c */
extern void init_heap(void);
extern void *allot(size_t size);
void forget(void *ptr);
/* Physical address of the heap */
extern size_t heap_ptr, heap_top, heap_bot;

/* osloader.c */
extern int bios_disk_dev;
/* Be careful with sector_t it is an unsigned long long on x86 */
typedef uint64_t sector_t;
typedef sector_t (*os_download_t)(unsigned char *data, unsigned int len, int eof);
extern os_download_t probe_image(unsigned char *data, unsigned int len);
extern int load_block P((unsigned char *, unsigned int, unsigned int, int ));
extern os_download_t pxe_probe P((unsigned char *data, unsigned int len));

/* misc.c */
extern void twiddle P((void));
extern void sleep P((int secs));
extern void interruptible_sleep P((int secs));
extern void poll_interruptions P((void));
extern int strcasecmp P((const char *a, const char *b));
extern char *substr P((const char *a, const char *b));
extern unsigned long strtoul P((const char *p, const char **, int base));
// extern void printf P((const char *, ...));
extern int sprintf P((char *, const char *, ...));
extern int inet_aton P((const char *p, in_addr *i));
#ifdef PCBIOS
extern void gateA20_set P((void));
#ifdef	RELOCATE
#define gateA20_unset()
#else
extern void gateA20_unset P((void));
#endif
#else
#define gateA20_set()
#define gateA20_unset()
#endif
extern int putchar P((int));
extern int getchar P((void));
extern int iskey P((void));

/* pcbios.S */
extern int console_getc P((void));
extern void console_putc P((int));
extern int console_ischar P((void));
extern int getshift P((void));
extern int int15 P((int));
#ifdef	POWERSAVE
extern void cpu_nap P((void));
#endif	/* POWERSAVE */
extern void fake_irq ( uint8_t irq );

/* basemem.c */
extern uint32_t get_free_base_memory ( void );
extern void adjust_real_mode_stack ( void );
extern void * allot_base_memory ( size_t );
extern void forget_base_memory ( void*, size_t );
extern void free_unused_base_memory ( void );

#define PACKED __attribute__((packed))

struct e820entry {
	uint64_t addr;
	uint64_t size;
	uint32_t type;
#define E820_RAM	1
#define E820_RESERVED	2
#define E820_ACPI	3 /* usable as RAM once ACPI tables have been read */
#define E820_NVS	4
} PACKED;
#define E820MAX 32
struct meminfo {
	uint16_t basememsize;
	uint16_t pad;
	uint32_t memsize;
	uint32_t map_count;
	struct e820entry map[E820MAX];
} PACKED;
extern struct meminfo meminfo;
extern void get_memsizes(void);
extern unsigned long get_boot_order(unsigned long order, unsigned *index);
#ifdef RELOCATE
extern void relocate(void);
extern void relocate_to(unsigned long phys_dest);
#else
#define relocate() do {} while(0)
#endif
extern void disk_init P((void));
extern unsigned int pcbios_disk_read P((int drv,int c,int h,int s,char *buf));

/* start32.S */
struct os_entry_regs {
	/* Be careful changing this structure
	 * as it is used by assembly language code.
	 */
	uint32_t  edi; /*  0 */
	uint32_t  esi; /*  4 */
	uint32_t  ebp; /*  8 */
	uint32_t  esp; /* 12 */
	uint32_t  ebx; /* 16 */
	uint32_t  edx; /* 20 */
	uint32_t  ecx; /* 24 */
	uint32_t  eax; /* 28 */
	
	uint32_t saved_ebp; /* 32 */
	uint32_t saved_esi; /* 36 */
	uint32_t saved_edi; /* 40 */
	uint32_t saved_ebx; /* 44 */
	uint32_t saved_eip; /* 48 */
	uint32_t saved_esp; /* 52 */
};
struct regs {
	/* Be careful changing this structure
	 * as it is used by assembly language code.
	 */
	uint32_t  edi; /*  0 */
	uint32_t  esi; /*  4 */
	uint32_t  ebp; /*  8 */
	uint32_t  esp; /* 12 */
	uint32_t  ebx; /* 16 */
	uint32_t  edx; /* 20 */
	uint32_t  ecx; /* 24 */
	uint32_t  eax; /* 28 */
};
extern struct os_entry_regs os_regs;
extern struct regs initial_regs;
extern unsigned long real_mode_stack;
extern void xstart16 P((unsigned long, unsigned long, char *));
extern int xstart32(unsigned long entry_point, ...);
extern int xstart_lm(unsigned long entry_point, unsigned long params);
extern void xend32 P((void));
struct Elf_Bhdr *prepare_boot_params(void *header);
extern int elf_start(unsigned long machine, unsigned long entry, unsigned long params);
extern unsigned long currticks P((void));
extern void exit P((int status));

/* serial.c */
extern int serial_getc P((void));
extern void serial_putc P((int));
extern int serial_ischar P((void));
extern int serial_init P((void));
extern void serial_fini P((void));

/* floppy.c */
extern int bootdisk P((int dev,int part));

/***************************************************************************
External variables
***************************************************************************/
/* main.c */
extern struct rom_info rom;
extern char *hostname;
extern int hostnamelen;
extern int url_port;
extern struct arptable_t arptable[MAX_ARP];
extern struct igmptable_t igmptable[MAX_IGMP];
#ifdef	IMAGE_MENU
extern int menutmo,menudefault;
extern unsigned char *defparams;
extern int defparams_max;
#endif
#ifdef	MOTD
extern unsigned char *motd[RFC1533_VENDOR_NUMOFMOTD];
#endif
extern struct bootpd_t bootp_data;
#define	BOOTP_DATA_ADDR	(&bootp_data)
extern unsigned char *end_of_rfc1533;
#ifdef	IMAGE_FREEBSD
extern int freebsd_howto;
#define FREEBSD_KERNEL_ENV_SIZE 256
extern char freebsd_kernel_env[FREEBSD_KERNEL_ENV_SIZE];
#endif

/* bootmenu.c */

/* osloader.c */

/* xbox.c */
#define printf printk
#define longjmp(a,b) a(b)
extern void restart_etherboot(int);

/* created by linker */
extern char _virt_start[], _text[], _etext[], _text16[], _etext16[];
extern char _data[], _edata[], _bss[], _ebss[], _end[];


/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
