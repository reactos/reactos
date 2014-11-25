
#ifndef HEX
#define HEX(y) 0x##y
#endif

/* Memory layout */
//#ifdef _M_AMD64
#define PML4_ADDRESS        HEX(1000) /* One page PML4 page table */
#define PDP_ADDRESS         HEX(2000) /* One page PDP page table */
#define PD_ADDRESS          HEX(3000) /* One page PD page table */
//#endif
#define BIOSCALLBUFFER      HEX(4000) /* Buffer to store temporary data for any Int386() call */
#define STACK16ADDR         HEX(6F00) /* The 16-bit stack top will be at 0000:6F00 */
#define BSS_START           HEX(6F00)
#define STACKLOW            HEX(7000)
#define STACKADDR           HEX(F000) /* The 32/64-bit stack top will be at 0000:F000, or 0xF000 */
#define FREELDR_BASE        HEX(F800)
#define FREELDR_PE_BASE    HEX(10000)
#define DISKREADBUFFER     HEX(8E000) /* Buffer to store data read in from the disk via the BIOS */
#define MEMORY_MARGIN      HEX(9A000) /* Highest usable address */
/* 9F000- 9FFFF is reserved for the EBDA */

#define BIOSCALLBUFSEGMENT (BIOSCALLBUFFER/16) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFOFFSET   HEX(0000) /* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFSIZE     PAGE_SIZE /* max is sizeof(VESA_SVGA_INFO) = 512 */
#define MAX_FREELDR_PE_SIZE (DISKREADBUFFER - FREELDR_PE_BASE)
#define DISKREADBUFFER_SIZE (MEMORY_MARGIN - DISKREADBUFFER)

/* These addresses specify the realmode "BSS section" layout */
#define BSS_RealModeEntry        (BSS_START +  0)
#define BSS_CallbackReturn       (BSS_START +  4)
#define BSS_RegisterSet          (BSS_START +  8) /* size = 40 */
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
#define BSS_BootDrive            (BSS_START + 108) // 1 byte
#define BSS_BootPartition        (BSS_START + 109) // 1 byte


/* Realmode function IDs */
#define FNID_Int386 0
#define FNID_Reboot 1
#define FNID_ChainLoadBiosBootSectorCode 2
#define FNID_PxeCallApi 3
#define FNID_PnpBiosGetDeviceNodeCount 4
#define FNID_PnpBiosGetDeviceNode 5
#define FNID_BootLinuxKernel 6

/* Flag Masks */
#define CR0_PE_SET    HEX(00000001)    /* OR this value with CR0 to enable pmode */
#define CR0_PE_CLR    HEX(FFFFFFFE)    /* AND this value with CR0 to disable pmode */

/* Defines needed for switching between real and protected mode */
//#ifdef _M_IX86
#define NULL_DESC    HEX(00)    /* NULL descriptor */
#define PMODE_CS    HEX(08)    /* PMode code selector, base 0 limit 4g */
#define PMODE_DS    HEX(10)    /* PMode data selector, base 0 limit 4g */
#define RMODE_CS    HEX(18)    /* RMode code selector, base 0 limit 64k */
#define RMODE_DS    HEX(20)    /* RMode data selector, base 0 limit 64k */
//#else
/* Long mode selectors */
#define LMODE_CS HEX(10)
#define LMODE_DS HEX(18)
#define CMODE_CS HEX(30)
//#endif

/* Makes "x" a global variable or label */
#define EXTERN(x)    .global x; x:
