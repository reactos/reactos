
#ifndef HEX
#define HEX(y) 0x##y
#endif

/* Memory layout */
#define STACK16ADDR         HEX(6F00) /* The 16-bit stack top will be at 0000:6F00 */
#define BSS_START           HEX(6F00)
#if defined(_USE_ML) || defined(_MSC_VER)
#define FREELDR_BASE        HEX(f000)
#define FREELDR_PE_BASE    HEX(10000)
#else
#define FREELDR_BASE        HEX(8000)
#define FREELDR_PE_BASE     HEX(9000)
#endif
#define STACK32ADDR        HEX(78000) /* The 32-bit stack top will be at 7000:8000, or 0x78000 */
#define BIOSCALLBUFFER     HEX(78000) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFSEGMENT  HEX(7800) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFOFFSET   HEX(0000) /* Buffer to store temporary data for any Int386() call */
#define FILESYSBUFFER      HEX(80000) /* Buffer to store file system data (e.g. cluster buffer for FAT) */
#define DISKREADBUFFER     HEX(90000) /* Buffer to store data read in from the disk via the BIOS */
#define DISKREADBUFFER_SIZE 512

/* These addresses specify the realmode "BSS section" layout */
#define BSS_RealModeEntry        (BSS_START +  0)
#define BSS_CallbackAddress      (BSS_START +  4)
#define BSS_CallbackReturn       (BSS_START +  8)
#define BSS_RegisterSet          (BSS_START + 16) /* size = 36 */
#define BSS_IntVector            (BSS_START + 52)
#define BSS_PxeEntryPoint        (BSS_START + 56)
#define BSS_PxeBufferSegment     (BSS_START + 60)
#define BSS_PxeBufferOffset      (BSS_START + 64)
#define BSS_PxeFunction          (BSS_START + 68)
#define BSS_PxeResult            (BSS_START + 72)
#define BSS_PnpBiosEntryPoint    (BSS_START + 76)
#define BSS_PnpBiosDataSegment   (BSS_START + 80)
#define BSS_PnpBiosBufferSegment (BSS_START + 84)
#define BSS_PnpBiosBufferOffset  (BSS_START + 88)
#define BSS_PnpNodeSize          (BSS_START + 92)
#define BSS_PnpNodeCount         (BSS_START + 96)
#define BSS_PnpNodeNumber        (BSS_START + 100)
#define BSS_PnpResult            (BSS_START + 104)


/* Realmode function IDs */
#define FNID_Int386 0
#define FNID_SoftReboot 1
#define FNID_ChainLoadBiosBootSectorCode 2
#define FNID_PxeCallApi 3
#define FNID_PnpBiosGetDeviceNodeCount 4
#define FNID_PnpBiosGetDeviceNode 5
#define FNID_BootLinuxKernel 6

/* Layout of the REGS structure */
#define REGS_EAX 0
#define REGS_EBX 4
#define REGS_ECX 8
#define REGS_EDX 12
#define REGS_ESI 16
#define REGS_EDI 20
#define REGS_DS 24
#define REGS_ES 26
#define REGS_FS 28
#define REGS_GS 30
#define REGS_EFLAGS 32


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
//#ifdef _M_IX86
#define NULL_DESC	HEX(00)	/* NULL descriptor */
#define PMODE_CS	HEX(08)	/* PMode code selector, base 0 limit 4g */
#define PMODE_DS	HEX(10)	/* PMode data selector, base 0 limit 4g */
#define RMODE_CS	HEX(18)	/* RMode code selector, base 0 limit 64k */
#define RMODE_DS	HEX(20)	/* RMode data selector, base 0 limit 64k */
//#endif

/* Makes "x" a global variable or label */
#define EXTERN(x)	.global x; x:
