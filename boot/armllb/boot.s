/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM LLB Entry Point"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    NESTED_ENTRY _start
    PROLOG_END _start

#ifdef _BEAGLE_ // This is only used for TI BootROM on Beagle/Emulator for now
    /*
     * On Beagle, the boot is directly from TI BootROM that reads NAND flash.
     * First word is size of program to load.
     * Second word is load address of program. Since DDR is not initialized,
     * we load to SDRAM at 40200000h. Max 64K.
     */
    .word 0x8000
    .word 0x40200000
#elif _ZOOM2_
	/*
	 * On ZOOM2, we currently load from u-boot to make bring-up easier.
	 *
	 * In order to get ATAG and all that goodness, we have to fool u-boot into
	 * thinking we are a Linux ARM kernel.
	 *
	 * So this is a 'fake' uImage-format header, which will make u-boot grok our
	 * image and correctly execute it.
	 *
	 * Note that a data checksum is in the header, but thankfully we can disable
	 * the check.
	 *
	 * There's also a header checksum, but as long as there's no need to modify
	 * this header, we can leave it static.
	 *
	 * Finally, note that the "Image String" is sized as a 32-byte array in the
	 * uImage header format. The string chosen below is not only accurate, but
	 * also happens to fit exactly in 32 bytes, meaning we don't need to pad.
	 */
	.word 0x56190527 // Header Magic
	.word 0x5E4B8444 // Checksum
	.word 0x483BE54C // Timestamp
	.word 0x0CA10000 // Image size (64K)
	.word 0x00000081 // Load address
	.word 0x40000081 // Entrypoint
	.word 0x90873DD8 // Data Checksum ('setenv verify n' must be set!)
	.byte 5 		 // Linux OS
	.byte 2 		 // ARM
	.byte 2 		 // Kernel
	.byte 0 		 // No compression
	.ascii "ReactOS ARM Low-Level Bootloader"
#endif

    /* Load C entrypoint and setup LLB stack */
    ldr lr, L_LlbStartup
    ldr sp, L_BootStackEnd
    bx lr
    ENTRY_END _start

L_BootStackEnd:
#ifdef _BEAGLE_ // This is only used for TI BootROM on Beagle/Emulator for now
    .long 0x00010000
#elif _ZOOM2_ // On ZOOM2 RAM starts at 0x80000000, not 0
	.long 0x81014000
#else
#error Stack Address Not Defined
#endif

L_LlbStartup:
    .long LlbStartup

/* EOF */
