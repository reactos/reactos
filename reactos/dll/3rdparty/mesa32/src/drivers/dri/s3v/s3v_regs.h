/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef _S3V_REG_H
#define _S3V_REG_H

#define S3V_REGS_NUM 256

/************
 * DMA REGS *
 ************/

#define S3V_DMA_ID				0
#define S3V_DMA_REG				0x8590
#define S3V_DMA_WRITEP_ID			1
#define S3V_DMA_WRITEP_REG			0x8594
#define S3V_DMA_READP_ID			2
#define S3V_DMA_READP_REG			0x8598
#define S3V_DMA_ENABLE_ID			3
#define S3V_DMA_ENABLE_REG			0x859C
#define S3V_DMA_UPDATE_ID			4
#define S3V_DMA_UPDATE_REG			0x10000

/***************
 * STATUS REGS *
 ***************/

#define S3V_STAT_ID				10
#define S3V_STAT_REG				0x8504
#define S3V_STAT_VSYNC_ID			11
#define S3V_STAT_VSYNC_REG			0x8505
#define S3V_STAT_3D_DONE_ID			12
#define S3V_STAT_3D_DONE_REG			0x8506
#define S3V_STAT_FIFO_OVER_ID			13
#define S3V_STAT_FIFO_OVER_REG			0x8508
#define S3V_STAT_FIFO_EMPTY_ID			14
#define S3V_STAT_FIFO_EMPTY_REG			0x850C
#define S3V_STAT_HDMA_DONE_ID			15
#define S3V_STAT_HDMA_DONE_REG			0x8514
#define S3V_STAT_CDMA_DONE_ID			16
#define S3V_STAT_CDMA_DONE_REG			0x8524
#define S3V_STAT_3D_FIFO_EMPTY_ID		17
#define S3V_STAT_3D_FIFO_EMPTY_REG		0x8544
#define S3V_STAT_LPB_ID				18
#define S3V_STAT_LPB_REG			0x8584
#define S3V_STAT_3D_BUSY_ID			19
#define S3V_STAT_3D_BUSY_REG			0x8704

/***********
 * 2D REGS *
 ***********/

#define S3V_BITBLT_ID				30
#define S3V_BITBLT_REG				0xA400
#define S3V_BITBLT_SRC_BASE_ID			31
#define S3V_BITBLT_SRC_BASE_REG			0xA4D4
#define S3V_BITBLT_DEST_BASE_ID			32
#define S3V_BITBLT_DEST_BASE_REG		0xA4D8
#define S3V_BITBLT_CLIP_L_R_ID			33
#define S3V_BITBLT_CLIP_L_R_REG			0xA4DC
#define S3V_BITBLT_CLIP_T_B_ID			34
#define S3V_BITBLT_CLIP_T_B_REG			0xA4E0
#define S3V_BITBLT_DEST_SRC_STRIDE_ID		35
#define S3V_BITBLT_DEST_SRC_STRIDE_REG		0xA4E4
#define S3V_BITBLT_MONO_PAT0_ID			36
#define S3V_BITBLT_MONO_PAT0_REG		0xA4E8
#define S3V_BITBLT_MONO_PAT1_ID			37
#define S3V_BITBLT_MONO_PAT1_REG		0xA4EC
#define S3V_BITBLT_PAT_BG_COLOR_ID		38
#define S3V_BITBLT_PAT_BG_COLOR_REG		0xA4F0
#define S3V_BITBLT_PAT_FG_COLOR_ID		39
#define S3V_BITBLT_PAT_FG_COLOR_REG		0xA4F4
#define S3V_BITBLT_CMDSET_ID			40
#define S3V_BITBLT_CMDSET_REG			0xA500
#define S3V_BITBLT_WIDTH_HEIGHT_ID		41
#define S3V_BITBLT_WIDTH_HEIGHT_REG		0xA504
#define S3V_BITBLT_SRC_X_Y_ID			42
#define S3V_BITBLT_SRC_X_Y_REG			0xA508
#define S3V_BITBLT_DEST_X_Y_ID			43
#define S3V_BITBLT_DEST_X_Y_REG			0xA50C
#define S3V_2DLINE_ID				44
#define S3V_2DLINE_REG				0xA800
#define S3V_2DPOLY_ID				45
#define S3V_2DPOLY_REG				0xAC00

/***************
 * 3DLINE REGS *
 ***************/
/* base regs */
#define S3V_3DLINE_ID                  		50
#define S3V_3DLINE_REG				0xB000
#define S3V_3DLINE_Z_BASE_ID           		51
#define S3V_3DLINE_Z_BASE_REG			0xB0D4
#define S3V_3DLINE_SRC_BASE_ID         		52   /* it is the same reg */
#define S3V_3DLINE_SRC_BASE_REG			0xB0D4
#define S3V_3DLINE_DEST_BASE_ID        		53
#define S3V_3DLINE_DEST_BASE_REG		0xB0D8
#define S3V_3DLINE_CLIP_L_R_ID         		54
#define S3V_3DLINE_CLIP_L_R_REG			0xB0DC
#define S3V_3DLINE_CLIP_T_B_ID			55
#define S3V_3DLINE_CLIP_T_B_REG         	0xB0E0
#define S3V_3DLINE_DEST_SRC_STRIDE_ID		56
#define S3V_3DLINE_DEST_SRC_STRIDE_REG  	0xB0E4
#define S3V_3DLINE_Z_STRIDE_ID       		57
#define S3V_3DLINE_Z_STRIDE_REG  		0xB0E8
#define S3V_3DLINE_TEX_BASE_ID			58
#define S3V_3DLINE_TEX_BASE_REG         	0xB0EC
#define S3V_3DLINE_TEX_B_COLOR_ID		59
#define S3V_3DLINE_TEX_B_COLOR_REG      	0xB0F0
#define S3V_3DLINE_FOG_COLOR_ID  		60
#define S3V_3DLINE_FOG_COLOR_REG      		0xB0F4
#define S3V_3DLINE_COLOR0_ID   			61
#define S3V_3DLINE_COLOR0_REG        		0xB0F8
#define S3V_3DLINE_COLOR1_ID			62
#define S3V_3DLINE_COLOR1_REG           	0xB0FC
#define S3V_3DLINE_CMDSET_ID			63
#define S3V_3DLINE_CMDSET_REG            	0xB100 /* special */
/* tex regs */
/* FIXME: shouldn't it be a 1D tex for lines? */
#define S3V_3DLINE_BASEV_ID 			64
#define S3V_3DLINE_BASEV_REG          		0xB104 
#define S3V_3DLINE_BASEU_ID			65
#define S3V_3DLINE_BASEU_REG            	0xB108
#define S3V_3DLINE_WXD_ID  			66
#define S3V_3DLINE_WXD_REG             		0xB10C
#define S3V_3DLINE_WYD_ID 			67
#define S3V_3DLINE_WYD_REG              	0xB110
#define S3V_3DLINE_WSTART_ID			68
#define S3V_3DLINE_WSTART_REG          		0xB114
#define S3V_3DLINE_DXD_ID   			69
#define S3V_3DLINE_DXD_REG          		0xB118
#define S3V_3DLINE_VXD_ID			70
#define S3V_3DLINE_VXD_REG              	0xB11C
#define S3V_3DLINE_UXD_ID			71
#define S3V_3DLINE_UXD_REG              	0xB120
#define S3V_3DLINE_DYD_ID			72
#define S3V_3DLINE_DYD_REG              	0xB124
#define S3V_3DLINE_VYD_ID			73
#define S3V_3DLINE_VYD_REG              	0xB128
#define S3V_3DLINE_UYD_ID			74
#define S3V_3DLINE_UYD_REG              	0xB12C
#define S3V_3DLINE_DSTART_ID			75
#define S3V_3DLINE_DSTART_REG           	0xB130
#define S3V_3DLINE_VSTART_ID			76
#define S3V_3DLINE_VSTART_REG           	0xB134
#define S3V_3DLINE_USTART_ID			77
#define S3V_3DLINE_USTART_REG           	0xB138
/* gourad regs */
#define S3V_3DLINE_GBD_ID   			78
#define S3V_3DLINE_GBD_REG            		0xB144
#define S3V_3DLINE_ARD_ID 			79
#define S3V_3DLINE_ARD_REG             		0xB148
#define S3V_3DLINE_GS_BS_ID			80
#define S3V_3DLINE_GS_BS_REG            	0xB14C
#define S3V_3DLINE_AS_RS_ID			81
#define S3V_3DLINE_AS_RS_REG             	0xB150
/* vertex regs */
#define S3V_3DLINE_DZ_ID    			82
#define S3V_3DLINE_DZ_REG            		0xB158
#define S3V_3DLINE_ZSTART_ID			83
#define S3V_3DLINE_ZSTART_REG            	0xB15C
#define S3V_3DLINE_XEND0_END1_ID		84
#define S3V_3DLINE_XEND0_END1_REG       	0xB16C
#define S3V_3DLINE_DX_ID        		85
#define S3V_3DLINE_DX_REG        		0xB170
#define S3V_3DLINE_XSTART_ID			86
#define S3V_3DLINE_XSTART_REG           	0xB174
#define S3V_3DLINE_YSTART_ID 			87
#define S3V_3DLINE_YSTART_REG          		0xB178
#define S3V_3DLINE_YCNT_ID  			88
#define S3V_3DLINE_YCNT_REG           		0xB17C

/**************
 * 3DTRI REGS *
 **************/
/* base regs */
#define S3V_3DTRI_ID                   		100
#define S3V_3DTRI_REG				0xB400
#define S3V_3DTRI_Z_BASE_ID			101
#define S3V_3DTRI_Z_BASE_REG            	0xB4D4
#define S3V_3DTRI_SRC_BASE_ID          		102 /* it is the same reg */
#define S3V_3DTRI_SRC_BASE_REG			0xB4D4
#define S3V_3DTRI_DEST_BASE_ID         		103
#define S3V_3DTRI_DEST_BASE_REG			0xB4D8
#define S3V_3DTRI_CLIP_L_R_ID          		104
#define S3V_3DTRI_CLIP_L_R_REG 			0xB4DC
#define S3V_3DTRI_CLIP_T_B_ID          		105
#define S3V_3DTRI_CLIP_T_B_REG			0xB4E0
#define S3V_3DTRI_DEST_SRC_STRIDE_ID   		106
#define S3V_3DTRI_DEST_SRC_STRIDE_REG		0xB4E4
#define S3V_3DTRI_Z_STRIDE_ID          		107
#define S3V_3DTRI_Z_STRIDE_REG			0xB4E8
#define S3V_3DTRI_TEX_BASE_ID          		108
#define S3V_3DTRI_TEX_BASE_REG			0xB4EC
#define S3V_3DTRI_TEX_B_COLOR_ID       		109
#define S3V_3DTRI_TEX_B_COLOR_REG		0xB4F0
#define S3V_3DTRI_FOG_COLOR_ID         		110
#define S3V_3DTRI_FOG_COLOR_REG			0xB4F4
#define S3V_3DTRI_COLOR0_ID            		111
#define S3V_3DTRI_COLOR0_REG			0xB4F8
#define S3V_3DTRI_COLOR1_ID            		112
#define S3V_3DTRI_COLOR1_REG			0xB4FC
#define S3V_3DTRI_CMDSET_ID            		113  /* special */
#define S3V_3DTRI_CMDSET_REG			0xB500
/* tex regs */
#define S3V_3DTRI_BASEV_ID             		114
#define S3V_3DTRI_BASEV_REG			0xB504
#define S3V_3DTRI_BASEU_ID             		115
#define S3V_3DTRI_BASEU_REG			0xB508
#define S3V_3DTRI_WXD_ID               		116
#define S3V_3DTRI_WXD_REG			0xB50C
#define S3V_3DTRI_WYD_ID               		117
#define S3V_3DTRI_WYD_REG			0xB510
#define S3V_3DTRI_WSTART_ID            		118
#define S3V_3DTRI_WSTART_REG 			0xB514
#define S3V_3DTRI_DXD_ID               		119
#define S3V_3DTRI_DXD_REG			0xB518
#define S3V_3DTRI_VXD_ID               		120
#define S3V_3DTRI_VXD_REG			0xB51C
#define S3V_3DTRI_UXD_ID            		121
#define S3V_3DTRI_UXD_REG			0xB520
#define S3V_3DTRI_DYD_ID            		122
#define S3V_3DTRI_DYD_REG			0xB524
#define S3V_3DTRI_VYD_ID            		123
#define S3V_3DTRI_VYD_REG			0xB528
#define S3V_3DTRI_UYD_ID            		124
#define S3V_3DTRI_UYD_REG			0xB52C
#define S3V_3DTRI_DSTART_ID         		125
#define S3V_3DTRI_DSTART_REG			0xB530
#define S3V_3DTRI_VSTART_ID         		126
#define S3V_3DTRI_VSTART_REG			0xB534
#define S3V_3DTRI_USTART_ID         		127
#define S3V_3DTRI_USTART_REG			0xB538
/* gourad regs */
#define S3V_3DTRI_GBX_ID               		128
#define S3V_3DTRI_GBX_REG			0xB53C
#define S3V_3DTRI_ARX_ID               		129
#define S3V_3DTRI_ARX_REG			0xB540
#define S3V_3DTRI_GBY_ID			130
#define S3V_3DTRI_GBY_REG               	0xB544
#define S3V_3DTRI_ARY_ID			131
#define S3V_3DTRI_ARY_REG               	0xB548
#define S3V_3DTRI_GS_BS_ID			132
#define S3V_3DTRI_GS_BS_REG             	0xB54C
#define S3V_3DTRI_AS_RS_ID			133
#define S3V_3DTRI_AS_RS_REG             	0xB550
/* vertex regs */
#define S3V_3DTRI_ZXD_ID               		134
#define S3V_3DTRI_ZXD_REG			0xB554
#define S3V_3DTRI_ZYD_ID               		135
#define S3V_3DTRI_ZYD_REG			0xB558
#define S3V_3DTRI_ZSTART_ID            		136
#define S3V_3DTRI_ZSTART_REG			0xB55C
#define S3V_3DTRI_TXDELTA12_ID         		137
#define S3V_3DTRI_TXDELTA12_REG			0xB560
#define S3V_3DTRI_TXEND12_ID           		138
#define S3V_3DTRI_TXEND12_REG			0xB564
#define S3V_3DTRI_TXDELTA01_ID         		139
#define S3V_3DTRI_TXDELTA01_REG			0xB568
#define S3V_3DTRI_TXEND01_ID           		140
#define S3V_3DTRI_TXEND01_REG			0xB56C
#define S3V_3DTRI_TXDELTA02_ID         		141
#define S3V_3DTRI_TXDELTA02_REG			0xB570
#define S3V_3DTRI_TXSTART02_ID         		142
#define S3V_3DTRI_TXSTART02_REG			0xB574
#define S3V_3DTRI_TYS_ID               		143
#define S3V_3DTRI_TYS_REG			0xB578
#define S3V_3DTRI_TY01_Y12_ID          		144
#define S3V_3DTRI_TY01_Y12_REG			0xB57C

/* COMMANDS (to 0xB100 [lines] or 0xB500 [tris]) */

/* Auto execute */
#define AUTO_EXEC_MASK		0x00000001
#define AUTO_EXEC_OFF		(0x0)
#define AUTO_EXEC_ON		(0x1)
/* HW clipping */
#define HW_CLIP_MASK		0x00000002
#define HW_CLIP_OFF		(0x0 << 1)
#define HW_CLIP_ON		(0x1 << 1)
/* Destination color */
#define DEST_COL_MASK		0x0000001c
#define DEST_COL_PAL		(0x0 << 2)	/* 8 bpp - palettized */
#define DEST_COL_1555		(0x1 << 2)	/* 16 bpp - ZRGB */
#define DEST_COL_888  		(0x2 << 2)	/* 24 bpp - RGB */
/* Texture color */
#define TEX_COL_MASK		0x000000e0
#define TEX_COL_ARGB8888	(0x0 << 5)	/* 32 bpp - ARGB */
#define TEX_COL_ARGB4444	(0x1 << 5)  	/* 16 bpp - ARGB */
#define TEX_COL_ARGB1555	(0x2 << 5)  	/* 16 bpp - ARGB */
#define TEX_COL_ALPHA4		(0x3 << 5)  	/* 8 bpp - ALPHA4 */
#define TEX_COL_BLEND4_LOW 	(0x4 << 5)  	/* 4 bpp - BLEND4 low nibble */
#define TEX_COL_BLEND4_HIGH	(0x5 << 5)  	/* 4 bpp - BLEND4 high nibble */
#define TEX_COL_PAL		(0x6 << 5)	/* 8 bpp - palettized */
#define TEX_COL_YUV		(0x7 << 5)	/* 16 bpp - YUV */
/* Mipmap level */
#define MIP_MASK		0x00000f00
#define MIPMAP_LEVEL(s)		(s << 8)	/* 8 -> 11 bits */
/* Texture filtering */
#define TEX_FILTER_MASK		0x00007000
#define MIP_NEAREST		(0x0 << 12)
#define LINEAR_MIP_NEAREST  	(0x1 << 12)
#define MIP_LINEAR	        (0x2 << 12)
#define LINEAR_MIP_LINEAR   	(0x3 << 12)
#define NEAREST		        (0x4 << 12)
#define FAST_BILINEAR       	(0x5 << 12)
#define LINEAR		        (0x6 << 12)
/* Texture blending */
#define TEX_BLEND_MAKS		0x00018000
#define TEX_REFLECT		(0x0 << 15)
#define TEX_MODULATE    	(0x1 << 15)
#define TEX_DECAL	        (0x2 << 15)
/* Fog */
#define FOG_MASK		0x00020000
#define FOG_OFF			(0x0 << 17)
#define FOG_ON              	(0x1 << 17)
/* Alpha blending */
#define ALPHA_BLEND_MASK	0x000c0000
#define ALPHA_OFF		(0x0 << 18) | (0x0 << 19)
#define ALPHA_TEX		(0x2 << 18)
#define ALPHA_SRC           	(0x3 << 18)
/* Depth compare mode */
#define Z_MODE_MASK		0x00700000
#define Z_NEVER     		(0x0 << 20)
#define Z_GREATER   		(0x1 << 20)
#define Z_EQUAL     		(0x2 << 20)
#define Z_GEQUAL    		(0x3 << 20)
#define Z_LESS      		(0x4 << 20)
#define Z_NOTEQUAL  		(0x5 << 20)
#define Z_LEQUAL    		(0x6 << 20) 
#define Z_ALWAYS   		(0x7 << 20)
/* Depth update */
#define Z_UPDATE_MASK		0x00800000
#define Z_UPDATE_OFF		(0x0 << 23)	/* disable z update */
#define Z_UPDATE_ON         	(0x1 << 23)
/* Depth buffering mode */
#define Z_BUFFER_MASK		0x03000000
#define Z_BUFFER		(0x0 << 24) | (0x0 << 25)
#define Z_MUX_BUF		(0x1 << 24) | (0x0 << 25)
#define Z_MUX_DRAW		(0x2 << 24)
#define Z_OFF			(0x3 << 24) /* no z buffering */
/* Texture wrapping */
#define TEX_WRAP_MASK		0x04000000
#define TEX_WRAP_OFF		(0x0 << 26)
#define TEX_WRAP_ON		(0x1 << 26)
/* 3d command */
#define DO_MASK			0x78000000
#define DO_GOURAUD_TRI		(0x0 << 27)
#define DO_TEX_LIT_TRI_OLD	(0x1 << 27) 
#define DO_TEX_UNLIT_TRI_OLD 	(0x2 << 27)
#define DO_TEX_LIT_TRI		(0x5 << 27)
#define DO_TEX_UNLIT_TRI	(0x6 << 27)
#define DO_3D_LINE		(0x8 << 27)
#define	DO_NOP			(0xf << 27) /* turn on autoexec */
/* status */
#define CMD_MASK		0x80000000
#define CMD_2D			(0x0 << 31) /* execute a 2d cmd */
#define CMD_3D			(0x1 << 31) /* execute a 3d cmd */

/* global masks */
#define TEX_MASK		( TEX_COL_MASK | TEX_WRAP_MASK | MIP_MASK \
				| TEX_FILTER_MASK | TEX_BLEND_MAKS \
				| TEX_WRAP_MASK )
#define Z_MASK			( Z_MODE_MASK | Z_UPDATE_MASK | Z_BUFFER_MASK )

#endif /* _S3V_REG_H */
