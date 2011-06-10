
#ifndef HEX
#define HEX(y) 0x##y
#endif

/* Memory layout */
#define STACK16ADDR         HEX(7000) /* The 16-bit stack top will be at 0000:7000 */
#define BSS_START           HEX(7000)
#define FREELDR_BASE        HEX(8000)
#define FREELDR_PE_BASE     HEX(9000)
#define STACK32ADDR        HEX(78000) /* The 32-bit stack top will be at 7000:8000, or 0x78000 */
#define BIOSCALLBUFFER     HEX(78000) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFSEGMENT  HEX(7800) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFOFFSET   HEX(0000) /* Buffer to store temporary data for any Int386() call */
#define FILESYSBUFFER      HEX(80000) /* Buffer to store file system data (e.g. cluster buffer for FAT) */
#define DISKREADBUFFER     HEX(90000) /* Buffer to store data read in from the disk via the BIOS */
#define DISKREADBUFFER_SIZE 512

/* These addresses specify the realmode "BSS section" layout */
#define BSS_CallbackAddress BSS_START + 0
#define BSS_CallbackReturn BSS_START + 8
#define BSS_BootDrive BSS_START + 16
#define BSS_BootPartition BSS_START + 20

#ifdef _M_AMD64
#define FrldrBootDrive *((PULONG)BSS_BootDrive)
#define FrldrBootPartition *((PULONG)BSS_BootPartition)
#endif

// Flag Masks
#define I386FLAG_CF		HEX(0001)  // Carry Flag
#define I386FLAG_RESV1	HEX(0002)  // Reserved - Must be 1
#define I386FLAG_PF		HEX(0004)  // Parity Flag
#define I386FLAG_RESV2	HEX(0008)  // Reserved - Must be 0
#define I386FLAG_AF		HEX(0010)  // Auxiliary Flag
#define I386FLAG_RESV3	HEX(0020)  // Reserved - Must be 0
#define I386FLAG_ZF		HEX(0040)  // Zero Flag
#define I386FLAG_SF		HEX(0080)  // Sign Flag
#define I386FLAG_TF		HEX(0100)  // Trap Flag (Single Step)
#define I386FLAG_IF		HEX(0200)  // Interrupt Flag
#define I386FLAG_DF		HEX(0400)  // Direction Flag
#define I386FLAG_OF		HEX(0800)  // Overflow Flag

#define CR0_PE_SET	HEX(00000001)	/* OR this value with CR0 to enable pmode */
#define CR0_PE_CLR	HEX(FFFFFFFE)	/* AND this value with CR0 to disable pmode */

/* Defines needed for switching between real and protected mode */
#ifdef _M_IX86
#define NULL_DESC	HEX(00)	/* NULL descriptor */
#define PMODE_CS	HEX(08)	/* PMode code selector, base 0 limit 4g */
#define PMODE_DS	HEX(10)	/* PMode data selector, base 0 limit 4g */
#define RMODE_CS	HEX(18)	/* RMode code selector, base 0 limit 64k */
#define RMODE_DS	HEX(20)	/* RMode data selector, base 0 limit 64k */
#endif

/* Makes "x" a global variable or label */
#define EXTERN(x)	.global x; x:
