/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef SAVAGE_BCI_H
#define SAVAGE_BCI_H
/***********************
  3D and 2D command
************************/

typedef enum {
    AMO_BurstCmdData=   0x01010000,
    AMO_3DReg=          0x01048500,
    AMO_MotionCompReg=  0x01048900,
    AMO_VideoEngUnit=   0x01048A00,
    AMO_CmdBufAddr=     0x01048c14,
    AMO_TiledSurfReg0=  0x01048C40,
    AMO_TiledSurfReg1=  0x01048C44,
    AMO_TiledSurfReg2=  0x01048C48,
    AMO_TiledSurfReg3=  0x01048C4C,
    AMO_TiledSurfReg4=  0x01048C50,
    AMO_TiledSurfReg5=  0x01048C54,
    AMO_TiledSurfReg6=  0x01048C58,
    AMO_TiledSurfReg7=  0x01048C5C,
    AMO_LPBModeReg=     0x0100FF00,
    AMO_LPBFifoSat=     0x0100FF04,
    AMO_LPBIntFlag=     0x0100FF08,
    AMO_LPBFmBufA0=     0x0100FF0C,
    AMO_LPBFmBufA1=     0x0100FF10,
    AMO_LPBRdWtAdr=     0x0100FF14,
    AMO_LPBRdWtDat=     0x0100FF18,
    AMO_LPBIOPort =     0x0100FF1C,
    AMO_LPBSerPort=     0x0100FF20,
    AMO_LPBVidInWinSz=  0x0100FF24,
    AMO_LPBVidDatOffs=  0x0100FF28,
    AMO_LPBHorScalCtrl= 0x0100FF2C,
    AMO_LPBVerDeciCtrl= 0x0100FF30,
    AMO_LPBLnStride=    0x0100FF34,
    AMO_LPBFmBufAddr2=  0x0100FF38,
    AMO_LPBVidCapVDCtrl=0x0100FF3C,

    AMO_LPBVidCapFdStAd=0x0100FF60,
    AMO_LPBVidCapFdMdAd=0x0100FF64,
    AMO_LPBVidCapFdBtAd=0x0100FF68,
    AMO_LPBVidCapFdSize=0x0100FF6C,
    AMO_LPBBilinDecim1= 0x0100FF70,
    AMO_LPBBilinDecim2= 0x0100FF74,
    AMO_LPBBilinDecim3= 0x0100FF78,
    AMO_LPBDspVEUHorSRR=0x0100FF7C,
    AMO_LPBDspVEUVerSRR=0x0100FF80,
    AMO_LPBDspVeuDnScDR=0x0100FF84,
    AMO_LPB_VEUERPReg=  0x0100FF88,
    AMO_LPB_VBISelReg=  0x0100FF8C,
    AMO_LPB_VBIBasAdReg=0x0100FF90,
    AMO_LPB_DatOffsReg= 0x0100FF94,
    AMO_LPB_VBIVerDcReg=0x0100FF98,
    AMO_LPB_VBICtrlReg= 0x0100FF9C,
    AMO_LPB_VIPXferCtrl=0x0100FFA0,
    AMO_LPB_FIFOWtMark= 0x0100FFA4,
    AMO_LPB_FIFOCount=  0x0100FFA8,
    AMO_LPBFdSkipPat=   0x0100FFAC,
    AMO_LPBCapVEUHorSRR=0x0100FFB0,
    AMO_LPBCapVEUVerSRR=0x0100FFB4,
    AMO_LPBCapVeuDnScDR=0x0100FFB8

}AddressMapOffset;   
/*more to add*/


typedef enum {
  CMD_DrawPrim=0x10,          /*10000*/
  CMD_DrawIdxPrim=0x11,       /*10001*/
  CMD_SetRegister=0x12,       /*10010*/
  CMD_UpdateShadowStat=0x13 , /*10011*/
  CMD_PageFlip=0x14,          /* 10100*/
  CMD_BusMasterImgXfer=0x15,  /* 10101*/
  CMD_ScaledImgXfer=0x16,     /* 10110*/
  CMD_Macroblock=0x17,         /*10111*/
  CMD_Wait= 0x18,             /*11000*/
  CMD_2D_NOP=0x08,            /* 01000*/
  CMD_2D_RCT=0x09,            /*01001   rectangular fill*/
  CMD_2D_SCNL=0x0a,           /* 01010   scan line*/
  CMD_2D_LIN=0x0b,            /*01011   line*/
  CMD_2D_SMTXT=0x0c,          /*01100*/
  CMD_2D_BPTXT=0x0d,          /*01101*/
  CMD_InitFlag=0x1f           /*11111, for S/W initialization control*/
}Command;


typedef enum {
    VRR_List,
    VRR_Strip,
    VRR_Fan,
    VRR_QuadList
}VertexReplaceRule;

/***********************
   Destination
************************/

typedef enum {
    DFT_RGB565 = 0,
    DFT_XRGB8888
}DestinationFmt;


/*************************
    Z Buffer / Alpha test
*************************/

typedef enum {
    CF_Never,
    CF_Less,
    CF_Equal,
    CF_LessEqual,
    CF_Greater,
    CF_NotEqual,
    CF_GreaterEqual,
    CF_Always
}ZCmpFunc;   /* same for Alpha test and Stencil test compare function */

typedef ZCmpFunc ACmpFunc;

typedef enum {
  ZDS_16i,    /* .16 fixed*/
  ZDS_32f     /* 1.8.15 float*/
}ZDepthSelect;


/**********************************
    BCI Register Addressing Index
***********************************/
typedef enum {

    CRI_VTX0_X =    0x00,
    CRI_VTX0_Y =    0x01,
    CRI_VTX0_W =    0x02,
    CRI_VTX0_DIFFU= 0x03,
    CRI_VTX0_SPECU= 0x04,
    CRI_VTX0_U =    0x05,
    CRI_VTX0_V =    0x06,
    CRI_VTX0_U2 =   0x07,
    CRI_VTX0_V2 =   0x08,
    CRI_VTX1_X =    0x09,
    CRI_VTX1_Y =    0x0a,
    CRI_VTX1_W =    0x0b,
    CRI_VTX1_DIFFU= 0x0c,
    CRI_VTX1_SPECU= 0x0d,
    CRI_VTX1_U =    0x0e,
    CRI_VTX1_V =    0x0f,
    CRI_VTX1_U2 =   0x10,
    CRI_VTX1_V2 =   0x11,
    CRI_VTX2_X =    0x12,
    CRI_VTX2_Y =    0x13,
    CRI_VTX2_W =    0x14,
    CRI_VTX2_DIFFU= 0x15,
    CRI_VTX2_SPECU= 0x16,
    CRI_VTX2_U =    0x17,
    CRI_VTX2_V =    0x18,
    CRI_VTX2_U2 =   0x19,
    CRI_VTX2_V2 =   0x1a,

    CRI_ZPixelOffset  = 0x1d,
    CRI_DrawCtrlLocal = 0x1e,
    CRI_TexPalAddr    = 0x1f,
    CRI_TexCtrl0      = 0x20,
    CRI_TexCtrl1      = 0x21,
    CRI_TexAddr0      = 0x22,
    CRI_TexAddr1      = 0x23,
    CRI_TexBlendCtrl0 = 0x24,
    CRI_TexBlendCtrl1 = 0x25,
    CRI_TexXprClr     = 0x26,
    CRI_TexDescr      = 0x27,

    CRI_FogTable00= 0x28,
    CRI_FogTable04= 0x29,
    CRI_FogTable08= 0x2a,
    CRI_FogTable12= 0x2b,
    CRI_FogTable16= 0x2c,
    CRI_FogTable20= 0x2d,
    CRI_FogTable24= 0x2e,
    CRI_FogTable28= 0x2f,
    CRI_FogCtrl=    0x30,
    CRI_StencilCtrl= 0x31,
    CRI_ZBufCtrl=   0x32,
    CRI_ZBufOffset= 0x33,
    CRI_DstCtrl=    0x34,
    CRI_DrawCtrlGlobal0=   0x35,
    CRI_DrawCtrlGlobal1=   0x36,
    CRI_ZRW_WTMK =  0x37,
    CRI_DST_WTMK =  0x38,
    CRI_TexBlendColor= 0x39,

    CRI_VertBufAddr= 0x3e,
    /* new in ms1*/
    CRI_MauFrameAddr0 = 0x40,
    CRI_MauFrameAddr1 = 0x41,
    CRI_MauFrameAddr2 = 0x42,
    CRI_MauFrameAddr3 = 0x43,
    CRI_FrameDesc     = 0x44,
    CRI_IDCT9bitEn    = 0x45,
    CRI_MV0           = 0x46,
    CRI_MV1           = 0x47,
    CRI_MV2           = 0x48,
    CRI_MV3           = 0x49,
    CRI_MacroDescr    = 0x4a,  /*kickoff?*/
    
    CRI_MeuCtrl = 0x50,
    CRI_SrcYAddr = 0x51,
    CRI_DestAddr = 0x52,
    CRI_FmtrSrcDimen = 0x53,
    CRI_FmtrDestDimen = 0x54,
    CRI_SrcCbAddr = 0x55,
    CRI_SrcCrAddr = 0x56,
    CRI_SrcCrCbStride = 0x57,
    
    CRI_BCI_Power= 0x5f,
    
    CRI_PSCtrl=0xA0,
    CRI_SSClrKeyCtrl=0xA1,
    CRI_SSCtrl=0xA4,
    CRI_SSChromUpBound=0xA5,
    CRI_SSHoriScaleCtrl=0xA6,
    CRI_SSClrAdj=0xA7,
    CRI_SSBlendCtrl=0xA8,
    CRI_PSFBAddr0=0xB0,
    CRI_PSFBAddr1=0xB1,
    CRI_PSStride=0xB2,
    CRI_DB_LPB_Support=0xB3,
    CRI_SSFBAddr0=0xB4,
    CRI_SSFBAddr1=0xB5,
    CRI_SSStride=0xB6,
    CRI_SSOpaqueCtrl=0xB7,
    CRI_SSVertScaleCtrl=0xB8,
    CRI_SSVertInitValue=0xB9,
    CRI_SSSrcLineCnt=0xBA,
    CRI_FIFO_RAS_Ctrl=0xBB,
    CRI_PSWinStartCoord=0xBC,
    CRI_PSWinSize=0xBD,
    CRI_SSWinStartCoord=0xBE,
    CRI_SSWinSize=0xBF,
    CRI_PSFIFOMon0=0xC0,
    CRI_SSFIFOMon0=0xC1,
    CRI_PSFIFOMon1=0xC2,
    CRI_SSFIFOMon1=0xC3,
    CRI_PSFBSize=0xC4,
    CRI_SSFBSize=0xC5,
    CRI_SSFBAddr2=0xC6,
    /* 2D register starts at D0*/
    CRI_CurrXY=0xD0,
    CRI_DstXYorStep=0xD1 ,
    CRI_LineErr=0xD2 ,
    CRI_DrawCmd=0xD3,   /*kick off for image xfer*/
    CRI_ShortStrkVecXfer=0xD4,
    CRI_BackClr=0xD5,
    CRI_ForeClr=0xD6,
    CRI_BitPlaneWtMask=0xD7,
    CRI_BitPlaneRdMask=0xD8,
    CRI_ClrCmp=0xD9 ,
    CRI_BackAndForeMix=0xDA ,
    CRI_TopLeftSciss=0xDB ,
    CRI_BotRightSciss=0xDC ,
    CRI_PixOrMultiCtrl=0xDD ,
    CRI_MultiCtrlOrRdSelct=0xDE ,
    CRI_MinorOrMajorAxisCnt=0xDF ,
    CRI_GlobalBmpDesc1=0xE0 ,
    CRI_GlobalBmpDesc2=0xE1 ,
    CRI_BurstPriBmpDesc1=0xE2 ,
    CRI_BurstPriBmpDesc2=0xE3 ,
    CRI_BurstSecBmpDesc1=0xE4 ,
    CRI_BurstSecBmpDesc2=0xE5,
    CRI_ImageDataPort=0xF8

}CtrlRegIdx;

/***********************
        Fog Mode
************************/
typedef enum
{
  FGM_Z_FOG,  /*Table*/
  FGM_V_FOG   /*Vertex*/
} FogMode;

/***********************
  Texture
************************/
typedef enum
{
    TAM_Wrap,
    TAM_Clamp,
    TAM_Mirror
} TexAddressModel;

typedef enum
{
    TFT_S3TC4Bit,
    TFT_Pal8Bit565,
    TFT_Pal8Bit1555,
    TFT_ARGB8888,
    TFT_ARGB1555,
    TFT_ARGB4444,
    TFT_RGB565,
    TFT_Pal8Bit4444,
    TFT_S3TC4A4Bit,  /*like S3TC4Bit but with 4 bit alpha*/
    TFT_S3TC4CA4Bit, /*like S3TC4Bit, but with 4 bit compressed alpha*/
    TFT_S3TCL4,
    TFT_S3TCA4L4,
    TFT_L8,
    TFT_A4L4,
    TFT_I8,
    TFT_A8
} TexFmt;

typedef enum
{
    TPS_64,
    TPS_128,
    TPS_192,
    TPS_256
} TexPaletteSize;

#define MAX_MIPMAP_LOD_BIAS 255
#define MIN_MIPMAP_LOD_BIAS -255

typedef enum
{
  TFM_Point,              /*1 TPP*/
  TFM_Bilin,              /*2 TPP*/
  TFM_Reserved,
  TFM_Trilin             /*16 TPP*/
} TexFilterMode;


#define TBC_Decal       0x00850410
#define TBC_Modul       0x00850011
#define TBC_DecalAlpha  0x00852A04
#define TBC_ModulAlpha  0x00110011
#define TBC_Copy        0x00840410
#define TBC_CopyAlpha   0x00900405
#define TBC_NoTexMap    0x00850405
#define TBC_Blend0      0x00810004
#define TBC_Blend1      0x00870e02
#define TBC_BlendAlpha0 0x00040004
#define TBC_BlendAlpha1 TBC_Blend1
#define TBC_BlendInt0   0x00040004
#define TBC_BlendInt1   0x01c20e02
#define TBC_AddAlpha    0x19910c11
#define TBC_Add         0x18110c11

#define TBC_Decal1      0x00870410
#define TBC_Modul1      0x00870013
#define TBC_DecalAlpha1 0x00832A00
#define TBC_ModulAlpha1 0x00130013
#define TBC_NoTexMap1   0x00870407
#define TBC_Copy1       0x00870400
#define TBC_CopyAlpha1  0x00900400
#define TBC_AddAlpha1   0x19930c13
#define TBC_Add1        0x18130c13

/*
 * derived from TexBlendCtrl
 */

typedef enum
{
    TBC_UseSrc,
    TBC_UseTex,
    TBC_TexTimesSrc,
    TBC_BlendTexWithSrc
} TexBlendCtrlMode;

/***********************
        Draw Control
************************/
typedef enum
{
    BCM_Reserved,
    BCM_None,
    BCM_CW,
    BCM_CCW
} BackfaceCullingMode;

typedef enum
{
    SAM_Zero,
    SAM_One,
    SAM_DstClr,
    SAM_1DstClr,
    SAM_SrcAlpha,
    SAM_1SrcAlpha,
    SAM_DstAlpha,
    SAM_1DstAlpha
} SrcAlphaBlendMode;

/* -1 from state*/
typedef enum
{
    DAM_Zero,
    DAM_One,
    DAM_SrcClr,
    DAM_1SrcClr,
    DAM_SrcAlpha,
    DAM_1SrcAlpha,
    DAM_DstAlpha,
    DAM_1DstAlpha
} DstAlphaBlendMode;

/*
 * stencil control
 */

typedef enum
{
    STENCIL_Keep,
    STENCIL_Zero,
    STENCIL_Equal,
    STENCIL_IncClamp,
    STENCIL_DecClamp,
    STENCIL_Invert,
    STENCIL_Inc,
    STENCIL_Dec
} StencilOp;

/***************************************************************
*** Bitfield Structures for Programming Interface **************
***************************************************************/

/**************************
 Command Header Entry
**************************/

typedef struct {  /*for DrawIndexPrimitive command, vert0Idx is meaningful.*/
    unsigned int vert0Idx:16;
    unsigned int vertCnt:8;
    unsigned int cont:1;
    unsigned int type:2;   /*00=list, 01=strip, 10=fan, 11=reserved*/
    unsigned int cmd:5;
}Reg_DrawIndexPrimitive;

typedef struct {  /*for DrawIndexPrimitive command, vert0Idx is meaningful.*/
    unsigned int noW:1;
    unsigned int noCd:1;
    unsigned int noCs:1;
    unsigned int noU:1;
    unsigned int noV:1;
    unsigned int noU2:1;
    unsigned int noV2:1;

    unsigned int reserved:9;
    unsigned int vertCnt:8;
    unsigned int cont:1;
    unsigned int type:2;   /* 00=list, 01=strip, 10=fan, 11=reserved*/
    unsigned int cmd:5;
}Reg_DrawPrimitive;


typedef struct {
    unsigned int startRegIdx:8;
    unsigned int reserved:8;
    unsigned int regCnt:8;
    unsigned int resvered1:1;
    unsigned int lowEn:1;
    unsigned int highEn:1;
    unsigned int cmd:5;
}Reg_SetRegister;

typedef struct {
    unsigned int reserved1:22;
    unsigned int isPrimary:1;
    unsigned int MIU_SYNC:1;
    unsigned int reserved2:3;
    unsigned int cmd:5;
}Reg_QueuedPageFlip;

typedef struct {
    unsigned int reserved1:22;
    unsigned int DIR:1;
    unsigned int CTG:1; /*set to 0*/
    unsigned int BPP:1;
    unsigned int reserved2:1;
    unsigned int cmd:5;
}Reg_MasterImgXfer;

typedef struct {
    unsigned int PD:4;   /*PM=mono, PS=descriptor specified*/
    unsigned int PT:1;
    unsigned int SD:4;
    unsigned int ST:1;
    unsigned int DD:3;
    unsigned int DC:2; /*DC=destination clip*/
  unsigned int CS:1;  /*cs=color specified*/
    unsigned int MIX3:8;
    unsigned int XP:1;
    unsigned int YP:1;
    unsigned int LP:1;
    unsigned int cmd:5;
}Reg_2D;

typedef struct {
    unsigned int CodedBlkPattern:6;
    unsigned int DCT_Type:1;
    unsigned int MB_Type:2;
    unsigned int MotionType:2;
    unsigned int MB_Row:6;
    unsigned int MB_Column:6;
    unsigned int mv3:1;
    unsigned int mv2:1;
    unsigned int mv1:1;
    unsigned int mv0:1;
    unsigned int cmd:5;
}Reg_MacroBlock;

typedef struct {
    unsigned int scanLnCnt:11;
    unsigned int clkCnt:5;
    unsigned int e3d:1;
    unsigned int e2d:1;
    unsigned int mau:1;
    unsigned int veu:1;
    unsigned int meuMit:1;
    unsigned int meuSit:1;
    unsigned int meuVx:1;
    unsigned int meuMau:1;
    unsigned int pageFlip:1;
    unsigned int scanLn:1;
    unsigned int clk:1;
    unsigned int cmd:5;
}Reg_Wait;

typedef struct{
    unsigned int reserved:27;
    unsigned int cmd:5;
}Reg_ScaledImgXfer  ;

typedef struct{
    unsigned int eventTag:16;
    unsigned int reserved2:6;
    unsigned int ET:1;
    unsigned int INT:1;
    unsigned int reserved1:3;
    unsigned int cmd:5;
}Reg_UpdtShadowStat;

typedef union {
    Reg_DrawPrimitive  vert;
    Reg_DrawIndexPrimitive  vertIdx;
    Reg_SetRegister    set;
    Reg_QueuedPageFlip pageFlip;
    Reg_MasterImgXfer  masterImgXfer;
    Reg_ScaledImgXfer  scaledImgXfer;
    Reg_UpdtShadowStat updtShadow;
    Reg_MacroBlock     macroBlk;
    Reg_2D             cmd2D;
    Reg_Wait           wait;
}CmdHeaderUnion;


/*frank 2001/11/14 add BCI write macros*/
/* Registers not used in the X server
 */

#define SAVAGE_NOP_ID           0x2094
#define SAVAGE_NOP_ID_MASK        ((1<<22)-1)


/* 3D instructions
 */

/*          Draw Primitive Control */


#define SAVAGE_HW_NO_Z          (1<<0)
#define SAVAGE_HW_NO_W          (1<<1)
#define SAVAGE_HW_NO_CD         (1<<2)
#define SAVAGE_HW_NO_CS         (1<<3)
#define SAVAGE_HW_NO_U0         (1<<4)
#define SAVAGE_HW_NO_V0         (1<<5)
#define SAVAGE_HW_NO_UV0        ((1<<4) | (1<<5))
#define SAVAGE_HW_NO_U1         (1<<6)
#define SAVAGE_HW_NO_V1         (1<<7)
#define SAVAGE_HW_NO_UV1        ((1<<6) | (1<<7))
#define SAVAGE_HW_SKIPFLAGS     0x000000ff

#endif






