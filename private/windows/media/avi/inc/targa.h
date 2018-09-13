/* 
 * 
 * Targa Plus definitions 
 * 
 * (C) Copyright Microsoft Corporation 1991. All rights reserved.
 */

/* Port Base for regesters */
#define REGBASE	0x0220


/* Advanced Direct regesters */
/* WRITE space */
#define W_COLOR0	0x0000
#define W_COLOR1	0x0001
#define W_COLOR2	0x0002
#define W_COLOR3	0x0003

#define W_VIDCON	0x0400
#define W_INDIRECT	0x0401
#define W_HUESAT	0x0402

#define W_MASKL		0x0800
#define W_MASKH		0x0801

#define W_MODE1		0x0C00
#define W_MODE2		0x0C01
#define W_WBL		0x0C02
#define W_WBH		0x0C03

/* READ space */
#define R_VIDSTAT	0x0000
#define R_CTL		0x0002
#define R_MASKL		0x0003

#define R_READAD	0x0401
#define R_MODE1		0x0402

#define R_USCAN		0x0800
#define R_MASKH		0x0801
#define R_OSCAN		0x0802

#define R_ROWCNT	0x0C00
#define R_MODE2		0x0C01
#define R_RBL		0x0C02
#define R_RBH		0x0C03







/* Advanced Indirect Regesters */

#define ADVANCED	0x90
#define MEMORY		0xA1




/* ADVANCED bit masks */
#define ADVANCED_SET_INAE	0x08
#define ADVANCED_CLEAR_INAE	0xF7
#define ADVANCED_SET_INT	0xC0
#define ADVANCED_CLEAR_INT	0x0F


/* MODE1 bit masks */
#define MODE1_CLEAR_MOD		0x00
#define MODE1_SET_MOD		0x40
#define MODE1_MEM_CLEAR_MOD	0x01
#define MODE1_MEM_SET_MOD	0x41

/* MEMORY bit masks */
#define MEMORY_SET_SIZE8	0x80
#define MEMORY_CLEAR_SIZE8	0x7F

#define MEMORY_SET_LINEAR	0x60

#define MEMORY_BASE_CLEAR	0xE1
#define MEMORY_BASE_1000	0x02
#define MEMORY_BASE_2000	0x04
#define MEMORY_BASE_3000	0x06
#define MEMORY_BASE_4000	0x08
#define MEMORY_BASE_5000	0x0A
#define MEMORY_BASE_6000	0x0C
#define MEMORY_BASE_7000	0x0E
#define MEMORY_BASE_8000	0x10
#define MEMORY_BASE_9000	0x12
#define MEMORY_BASE_A000	0x14
#define MEMORY_BASE_B000	0x16
#define MEMORY_BASE_C000	0x18
#define MEMORY_BASE_D000	0x1A
#define MEMORY_BASE_E000	0x1C
#define MEMORY_BASE_F000	0x1E






