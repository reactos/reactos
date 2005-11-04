/* author: stephen crowley, crow@debian.org */

/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * STEPHEN CROWLEY, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgaregs.h,v 1.6 2003/01/12 03:55:46 tsi Exp $ */

#ifndef _MGAREGS_H_
#define _MGAREGS_H_

/*************** (START) AUTOMATICLY GENERATED REGISTER FILE *****************/
/*
 * Generated on Wed Jan 26 13:44:46 MST 2000
 */



/*
 * Power Graphic Mode Memory Space Registers
 */

#define MGAREG_MGA_EXEC 			0x0100
#define MGAREG_AGP_PLL 			0x1e4c

#    define AGP_PLL_agp2xpllen_MASK 	0xfffffffe 	/* bit 0 */
#    define AGP_PLL_agp2xpllen_disable 	0x0 		
#    define AGP_PLL_agp2xpllen_enable 	0x1 		

#define MGAREG_CFG_OR 				0x1e4c

#    define CFG_OR_comp_or_MASK 	0xfffffff7 	/* bit 3 */
#    define CFG_OR_comp_or_disable 	0x0 		
#    define CFG_OR_comp_or_enable 	0x8 		
#    define CFG_OR_compfreq_MASK 	0xffffff0f 	/* bits 4-7 */
#    define CFG_OR_compfreq_SHIFT 	4 		
#    define CFG_OR_comporup_MASK 	0xfffff0ff 	/* bits 8-11 */
#    define CFG_OR_comporup_SHIFT 	8 		
#    define CFG_OR_compordn_MASK 	0xffff0fff 	/* bits 12-15 */
#    define CFG_OR_compordn_SHIFT 	12 		
#    define CFG_OR_e2pq_MASK 		0xfffeffff 	/* bit 16 */
#    define CFG_OR_e2pq_disable 	0x0 		
#    define CFG_OR_e2pq_enable 		0x10000 	
#    define CFG_OR_e2pqbypcsn_MASK 	0xfffdffff 	/* bit 17 */
#    define CFG_OR_e2pqbypcsn_disable 	0x0 		
#    define CFG_OR_e2pqbypcsn_enable 	0x20000 	
#    define CFG_OR_e2pqbypd_MASK 	0xfffbffff 	/* bit 18 */
#    define CFG_OR_e2pqbypd_disable 	0x0 		
#    define CFG_OR_e2pqbypd_enable 	0x40000 	
#    define CFG_OR_e2pbypclk_MASK 	0xfff7ffff 	/* bit 19 */
#    define CFG_OR_e2pbypclk_disable 	0x0 		
#    define CFG_OR_e2pbypclk_enable 	0x80000 	
#    define CFG_OR_e2pbyp_MASK 		0xffefffff 	/* bit 20 */
#    define CFG_OR_e2pbyp_disable 	0x0 		
#    define CFG_OR_e2pbyp_enable 	0x100000 	
#    define CFG_OR_rate_cap_or_MASK 	0xff1fffff 	/* bits 21-23 */
#    define CFG_OR_rate_cap_or_SHIFT 	21 		
#    define CFG_OR_rq_or_MASK 		0xe0ffffff 	/* bits 24-28 */
#    define CFG_OR_rq_or_SHIFT 		24 		

#define MGAREG_ALPHACTRL 			0x2c7c

#    define AC_src_MASK 		0xfffffff0 	/* bits 0-3 */
#    define AC_src_zero 		0x0 		/* val 0, shift 0 */
#    define AC_src_one 			0x1 		/* val 1, shift 0 */
#    define AC_src_dst_color 		0x2 		/* val 2, shift 0 */
#    define AC_src_om_dst_color 	0x3 		/* val 3, shift 0 */
#    define AC_src_src_alpha 		0x4 		/* val 4, shift 0 */
#    define AC_src_om_src_alpha 	0x5 		/* val 5, shift 0 */
#    define AC_src_dst_alpha 		0x6 		/* val 6, shift 0 */
#    define AC_src_om_dst_alpha 	0x7 		/* val 7, shift 0 */
#    define AC_src_src_alpha_sat 	0x8 		/* val 8, shift 0 */
#    define AC_dst_MASK 		0xffffff0f 	/* bits 4-7 */
#    define AC_dst_zero 		0x0 		/* val 0, shift 4 */
#    define AC_dst_one 			0x10 		/* val 1, shift 4 */
#    define AC_dst_src_color 		0x20 		/* val 2, shift 4 */
#    define AC_dst_om_src_color 	0x30 		/* val 3, shift 4 */
#    define AC_dst_src_alpha 		0x40 		/* val 4, shift 4 */
#    define AC_dst_om_src_alpha 	0x50 		/* val 5, shift 4 */
#    define AC_dst_dst_alpha 		0x60 		/* val 6, shift 4 */
#    define AC_dst_om_dst_alpha 	0x70 		/* val 7, shift 4 */
#    define AC_amode_MASK 		0xfffffcff 	/* bits 8-9 */
#    define AC_amode_FCOL 		0x0 		/* val 0, shift 8 */
#    define AC_amode_alpha_channel 	0x100 		/* val 1, shift 8 */
#    define AC_amode_video_alpha 	0x200 		/* val 2, shift 8 */
#    define AC_amode_RSVD 		0x300 		/* val 3, shift 8 */
#    define AC_astipple_MASK 		0xfffff7ff 	/* bit 11 */
#    define AC_astipple_disable 	0x0 		
#    define AC_astipple_enable 		0x800 		
#    define AC_aten_MASK 		0xffffefff 	/* bit 12 */
#    define AC_aten_disable 		0x0 		
#    define AC_aten_enable 		0x1000 		
#    define AC_atmode_MASK 		0xffff1fff 	/* bits 13-15 */
#    define AC_atmode_noacmp 		0x0 		/* val 0, shift 13 */
#    define AC_atmode_ae 		0x4000 		/* val 2, shift 13 */
#    define AC_atmode_ane 		0x6000 		/* val 3, shift 13 */
#    define AC_atmode_alt 		0x8000 		/* val 4, shift 13 */
#    define AC_atmode_alte 		0xa000 		/* val 5, shift 13 */
#    define AC_atmode_agt 		0xc000 		/* val 6, shift 13 */
#    define AC_atmode_agte 		0xe000 		/* val 7, shift 13 */
#    define AC_atref_MASK 		0xff00ffff 	/* bits 16-23 */
#    define AC_atref_SHIFT 		16 		
#    define AC_alphasel_MASK 		0xfcffffff 	/* bits 24-25 */
#    define AC_alphasel_fromtex 	0x0 		/* val 0, shift 24 */
#    define AC_alphasel_diffused 	0x1000000 	/* val 1, shift 24 */
#    define AC_alphasel_modulated 	0x2000000 	/* val 2, shift 24 */
#    define AC_alphasel_trans 		0x3000000 	/* val 3, shift 24 */

#define MGAREG_ALPHASTART 			0x2c70
#define MGAREG_ALPHAXINC 			0x2c74
#define MGAREG_ALPHAYINC 			0x2c78
#define MGAREG_AR0 				0x1c60

#    define AR0_ar0_MASK 		0xfffc0000 	/* bits 0-17 */
#    define AR0_ar0_SHIFT 		0 		

#define MGAREG_AR1 				0x1c64

#    define AR1_ar1_MASK 		0xff000000 	/* bits 0-23 */
#    define AR1_ar1_SHIFT 		0 		

#define MGAREG_AR2 				0x1c68

#    define AR2_ar2_MASK 		0xfffc0000 	/* bits 0-17 */
#    define AR2_ar2_SHIFT 		0 		

#define MGAREG_AR3 				0x1c6c

#    define AR3_ar3_MASK 		0xff000000 	/* bits 0-23 */
#    define AR3_ar3_SHIFT 		0 		
#    define AR3_spage_MASK 		0xf8ffffff 	/* bits 24-26 */
#    define AR3_spage_SHIFT 		24 		

#define MGAREG_AR4 				0x1c70

#    define AR4_ar4_MASK 		0xfffc0000 	/* bits 0-17 */
#    define AR4_ar4_SHIFT 		0 		

#define MGAREG_AR5 				0x1c74

#    define AR5_ar5_MASK 		0xfffc0000 	/* bits 0-17 */
#    define AR5_ar5_SHIFT 		0 		

#define MGAREG_AR6 				0x1c78

#    define AR6_ar6_MASK 		0xfffc0000 	/* bits 0-17 */
#    define AR6_ar6_SHIFT 		0 		

#define MGAREG_BCOL 				0x1c20
#define MGAREG_BESA1CORG 			0x3d10
#define MGAREG_BESA1ORG 			0x3d00
#define MGAREG_BESA2CORG 			0x3d14
#define MGAREG_BESA2ORG 			0x3d04
#define MGAREG_BESB1CORG 			0x3d18
#define MGAREG_BESB1ORG 			0x3d08
#define MGAREG_BESB2CORG 			0x3d1c
#define MGAREG_BESB2ORG 			0x3d0c
#define MGAREG_BESCTL 				0x3d20

#    define BC_besen_MASK 		0xfffffffe 	/* bit 0 */
#    define BC_besen_disable 		0x0 		
#    define BC_besen_enable 		0x1 		
#    define BC_besv1srcstp_MASK 	0xffffffbf 	/* bit 6 */
#    define BC_besv1srcstp_even 	0x0 		
#    define BC_besv1srcstp_odd 		0x40 		
#    define BC_besv2srcstp_MASK 	0xfffffeff 	/* bit 8 */
#    define BC_besv2srcstp_disable 	0x0 		
#    define BC_besv2srcstp_enable 	0x100 		
#    define BC_beshfen_MASK 		0xfffffbff 	/* bit 10 */
#    define BC_beshfen_disable 		0x0 		
#    define BC_beshfen_enable 		0x400 		
#    define BC_besvfen_MASK 		0xfffff7ff 	/* bit 11 */
#    define BC_besvfen_disable 		0x0 		
#    define BC_besvfen_enable 		0x800 		
#    define BC_beshfixc_MASK 		0xffffefff 	/* bit 12 */
#    define BC_beshfixc_weight 		0x0 		
#    define BC_beshfixc_coeff 		0x1000 		
#    define BC_bescups_MASK 		0xfffeffff 	/* bit 16 */
#    define BC_bescups_disable 		0x0 		
#    define BC_bescups_enable 		0x10000 	
#    define BC_bes420pl_MASK 		0xfffdffff 	/* bit 17 */
#    define BC_bes420pl_422 		0x0 		
#    define BC_bes420pl_420 		0x20000 	
#    define BC_besdith_MASK 		0xfffbffff 	/* bit 18 */
#    define BC_besdith_disable 		0x0 		
#    define BC_besdith_enable 		0x40000 	
#    define BC_beshmir_MASK 		0xfff7ffff 	/* bit 19 */
#    define BC_beshmir_disable 		0x0 		
#    define BC_beshmir_enable 		0x80000 	
#    define BC_besbwen_MASK 		0xffefffff 	/* bit 20 */
#    define BC_besbwen_color 		0x0 		
#    define BC_besbwen_bw 		0x100000 	
#    define BC_besblank_MASK 		0xffdfffff 	/* bit 21 */
#    define BC_besblank_disable 	0x0 		
#    define BC_besblank_enable 		0x200000 	
#    define BC_besfselm_MASK 		0xfeffffff 	/* bit 24 */
#    define BC_besfselm_soft 		0x0 		
#    define BC_besfselm_hard 		0x1000000 	
#    define BC_besfsel_MASK 		0xf9ffffff 	/* bits 25-26 */
#    define BC_besfsel_a1 		0x0 		/* val 0, shift 25 */
#    define BC_besfsel_a2 		0x2000000 	/* val 1, shift 25 */
#    define BC_besfsel_b1 		0x4000000 	/* val 2, shift 25 */
#    define BC_besfsel_b2 		0x6000000 	/* val 3, shift 25 */

#define MGAREG_BESGLOBCTL 			0x3dc0

#    define BGC_beshzoom_MASK 		0xfffffffe 	/* bit 0 */
#    define BGC_beshzoom_disable 	0x0 		
#    define BGC_beshzoom_enable 	0x1 		
#    define BGC_beshzoomf_MASK 		0xfffffffd 	/* bit 1 */
#    define BGC_beshzoomf_disable 	0x0 		
#    define BGC_beshzoomf_enable 	0x2 		
#    define BGC_bescorder_MASK 		0xfffffff7 	/* bit 3 */
#    define BGC_bescorder_even 		0x0 		
#    define BGC_bescorder_odd 		0x8 		
#    define BGC_besreghup_MASK 		0xffffffef 	/* bit 4 */
#    define BGC_besreghup_disable 	0x0 		
#    define BGC_besreghup_enable 	0x10 		
#    define BGC_besvcnt_MASK 		0xf000ffff 	/* bits 16-27 */
#    define BGC_besvcnt_SHIFT 		16 		

#define MGAREG_BESHCOORD 			0x3d28

#    define BHC_besright_MASK 		0xfffff800 	/* bits 0-10 */
#    define BHC_besright_SHIFT 		0 		
#    define BHC_besleft_MASK 		0xf800ffff 	/* bits 16-26 */
#    define BHC_besleft_SHIFT 		16 		

#define MGAREG_BESHISCAL 			0x3d30

#    define BHISF_beshiscal_MASK 	0xffe00003 	/* bits 2-20 */
#    define BHISF_beshiscal_SHIFT 	2 		

#define MGAREG_BESHSRCEND 			0x3d3c

#    define BHSE_beshsrcend_MASK 	0xfc000003 	/* bits 2-25 */
#    define BHSE_beshsrcend_SHIFT 	2 		

#define MGAREG_BESHSRCLST 			0x3d50

#    define BHSL_beshsrclst_MASK 	0xfc00ffff 	/* bits 16-25 */
#    define BHSL_beshsrclst_SHIFT 	16 		

#define MGAREG_BESHSRCST 			0x3d38

#    define BHSS_beshsrcst_MASK 	0xfc000003 	/* bits 2-25 */
#    define BHSS_beshsrcst_SHIFT 	2 		

#define MGAREG_BESPITCH 			0x3d24

#    define BP_bespitch_MASK 		0xfffff000 	/* bits 0-11 */
#    define BP_bespitch_SHIFT 		0 		

#define MGAREG_BESSTATUS 			0x3dc4

#    define BS_besstat_MASK 		0xfffffffc 	/* bits 0-1 */
#    define BS_besstat_a1 		0x0 		/* val 0, shift 0 */
#    define BS_besstat_a2 		0x1 		/* val 1, shift 0 */
#    define BS_besstat_b1 		0x2 		/* val 2, shift 0 */
#    define BS_besstat_b2 		0x3 		/* val 3, shift 0 */

#define MGAREG_BESV1SRCLST 			0x3d54

#    define BSF_besv1srclast_MASK 	0xfffffc00 	/* bits 0-9 */
#    define BSF_besv1srclast_SHIFT 	0 		

#define MGAREG_BESV2SRCLST 			0x3d58

#    define BSF_besv2srclst_MASK 	0xfffffc00 	/* bits 0-9 */
#    define BSF_besv2srclst_SHIFT 	0 		

#define MGAREG_BESV1WGHT 			0x3d48

#    define BSF_besv1wght_MASK 		0xffff0003 	/* bits 2-15 */
#    define BSF_besv1wght_SHIFT 	2 		
#    define BSF_besv1wghts_MASK 	0xfffeffff 	/* bit 16 */
#    define BSF_besv1wghts_disable 	0x0 		
#    define BSF_besv1wghts_enable 	0x10000 	

#define MGAREG_BESV2WGHT 			0x3d4c

#    define BSF_besv2wght_MASK 		0xffff0003 	/* bits 2-15 */
#    define BSF_besv2wght_SHIFT 	2 		
#    define BSF_besv2wghts_MASK 	0xfffeffff 	/* bit 16 */
#    define BSF_besv2wghts_disable 	0x0 		
#    define BSF_besv2wghts_enable 	0x10000 	

#define MGAREG_BESVCOORD 			0x3d2c

#    define BVC_besbot_MASK 		0xfffff800 	/* bits 0-10 */
#    define BVC_besbot_SHIFT 		0 		
#    define BVC_bestop_MASK 		0xf800ffff 	/* bits 16-26 */
#    define BVC_bestop_SHIFT 		16 		

#define MGAREG_BESVISCAL 			0x3d34

#    define BVISF_besviscal_MASK 	0xffe00003 	/* bits 2-20 */
#    define BVISF_besviscal_SHIFT 	2 		

#define MGAREG_CODECADDR 			0x3e44
#define MGAREG_CODECCTL 			0x3e40
#define MGAREG_CODECHARDPTR 			0x3e4c
#define MGAREG_CODECHOSTPTR 			0x3e48
#define MGAREG_CODECLCODE 			0x3e50
#define MGAREG_CXBNDRY 			0x1c80

#    define CXB_cxleft_MASK 		0xfffff000 	/* bits 0-11 */
#    define CXB_cxleft_SHIFT 		0 		
#    define CXB_cxright_MASK 		0xf000ffff 	/* bits 16-27 */
#    define CXB_cxright_SHIFT 		16 		

#define MGAREG_CXLEFT 				0x1ca0
#define MGAREG_CXRIGHT 			0x1ca4
#define MGAREG_DMAMAP30 			0x1e30
#define MGAREG_DMAMAP74 			0x1e34
#define MGAREG_DMAMAPB8 			0x1e38
#define MGAREG_DMAMAPFC 			0x1e3c
#define MGAREG_DMAPAD 				0x1c54
#define MGAREG_DR0_Z32LSB 			0x2c50
#define MGAREG_DR0_Z32MSB 			0x2c54
#define MGAREG_DR2_Z32LSB 			0x2c60
#define MGAREG_DR2_Z32MSB 			0x2c64
#define MGAREG_DR3_Z32LSB 			0x2c68
#define MGAREG_DR3_Z32MSB 			0x2c6c
#define MGAREG_DR0 				0x1cc0
#define MGAREG_DR2 				0x1cc8
#define MGAREG_DR3 				0x1ccc
#define MGAREG_DR4 				0x1cd0
#define MGAREG_DR6 				0x1cd8
#define MGAREG_DR7 				0x1cdc
#define MGAREG_DR8 				0x1ce0
#define MGAREG_DR10 				0x1ce8
#define MGAREG_DR11 				0x1cec
#define MGAREG_DR12 				0x1cf0
#define MGAREG_DR14 				0x1cf8
#define MGAREG_DR15 				0x1cfc
#define MGAREG_DSTORG 				0x2cb8

#    define DO_dstmap_MASK 		0xfffffffe 	/* bit 0 */
#    define DO_dstmap_fb 		0x0 		
#    define DO_dstmap_sys 		0x1 		
#    define DO_dstacc_MASK 		0xfffffffd 	/* bit 1 */
#    define DO_dstacc_pci 		0x0 		
#    define DO_dstacc_agp 		0x2 		
#    define DO_dstorg_MASK 		0x7 		/* bits 3-31 */
#    define DO_dstorg_SHIFT 		3 		

#define MGAREG_DWG_INDIR_WT 			0x1e80
#define MGAREG_DWGCTL 				0x1c00

#    define DC_opcod_MASK 		0xfffffff0 	/* bits 0-3 */
#    define DC_opcod_line_open 		0x0 		/* val 0, shift 0 */
#    define DC_opcod_autoline_open 	0x1 		/* val 1, shift 0 */
#    define DC_opcod_line_close 	0x2 		/* val 2, shift 0 */
#    define DC_opcod_autoline_close 	0x3 		/* val 3, shift 0 */
#    define DC_opcod_trap 		0x4 		/* val 4, shift 0 */
#    define DC_opcod_texture_trap 	0x6 		/* val 6, shift 0 */
#    define DC_opcod_bitblt 		0x8 		/* val 8, shift 0 */
#    define DC_opcod_iload 		0x9 		/* val 9, shift 0 */
#    define DC_atype_MASK 		0xffffff8f 	/* bits 4-6 */
#    define DC_atype_rpl 		0x0 		/* val 0, shift 4 */
#    define DC_atype_rstr 		0x10 		/* val 1, shift 4 */
#    define DC_atype_zi 		0x30 		/* val 3, shift 4 */
#    define DC_atype_blk 		0x40 		/* val 4, shift 4 */
#    define DC_atype_i 			0x70 		/* val 7, shift 4 */
#    define DC_linear_MASK 		0xffffff7f 	/* bit 7 */
#    define DC_linear_xy 		0x0 		
#    define DC_linear_linear 		0x80 		
#    define DC_zmode_MASK 		0xfffff8ff 	/* bits 8-10 */
#    define DC_zmode_nozcmp 		0x0 		/* val 0, shift 8 */
#    define DC_zmode_ze 		0x200 		/* val 2, shift 8 */
#    define DC_zmode_zne 		0x300 		/* val 3, shift 8 */
#    define DC_zmode_zlt 		0x400 		/* val 4, shift 8 */
#    define DC_zmode_zlte 		0x500 		/* val 5, shift 8 */
#    define DC_zmode_zgt 		0x600 		/* val 6, shift 8 */
#    define DC_zmode_zgte 		0x700 		/* val 7, shift 8 */
#    define DC_solid_MASK 		0xfffff7ff 	/* bit 11 */
#    define DC_solid_disable 		0x0 		
#    define DC_solid_enable 		0x800 		
#    define DC_arzero_MASK 		0xffffefff 	/* bit 12 */
#    define DC_arzero_disable 		0x0 		
#    define DC_arzero_enable 		0x1000 		
#    define DC_sgnzero_MASK 		0xffffdfff 	/* bit 13 */
#    define DC_sgnzero_disable 		0x0 		
#    define DC_sgnzero_enable 		0x2000 		
#    define DC_shftzero_MASK 		0xffffbfff 	/* bit 14 */
#    define DC_shftzero_disable 	0x0 		
#    define DC_shftzero_enable 		0x4000 		
#    define DC_bop_MASK 		0xfff0ffff 	/* bits 16-19 */
#    define DC_bop_SHIFT 		16 		
#    define DC_trans_MASK 		0xff0fffff 	/* bits 20-23 */
#    define DC_trans_SHIFT 		20 		
#    define DC_bltmod_MASK 		0xe1ffffff 	/* bits 25-28 */
#    define DC_bltmod_bmonolef 		0x0 		/* val 0, shift 25 */
#    define DC_bltmod_bmonowf 		0x8000000 	/* val 4, shift 25 */
#    define DC_bltmod_bplan 		0x2000000 	/* val 1, shift 25 */
#    define DC_bltmod_bfcol 		0x4000000 	/* val 2, shift 25 */
#    define DC_bltmod_bu32bgr 		0x6000000 	/* val 3, shift 25 */
#    define DC_bltmod_bu32rgb 		0xe000000 	/* val 7, shift 25 */
#    define DC_bltmod_bu24bgr 		0x16000000 	/* val 11, shift 25 */
#    define DC_bltmod_bu24rgb 		0x1e000000 	/* val 15, shift 25 */
#    define DC_pattern_MASK 		0xdfffffff 	/* bit 29 */
#    define DC_pattern_disable 		0x0 		
#    define DC_pattern_enable 		0x20000000 	
#    define DC_transc_MASK 		0xbfffffff 	/* bit 30 */
#    define DC_transc_disable 		0x0 		
#    define DC_transc_enable 		0x40000000 	
#    define DC_clipdis_MASK 		0x7fffffff 	/* bit 31 */
#    define DC_clipdis_disable 		0x0 		
#    define DC_clipdis_enable 		0x80000000 	

#define MGAREG_DWGSYNC 			0x2c4c

#    define DS_dwgsyncaddr_MASK 	0x3 		/* bits 2-31 */
#    define DS_dwgsyncaddr_SHIFT 	2 		

#define MGAREG_FCOL 				0x1c24
#define MGAREG_FIFOSTATUS 			0x1e10

#    define FS_fifocount_MASK 		0xffffff80 	/* bits 0-6 */
#    define FS_fifocount_SHIFT 		0 		
#    define FS_bfull_MASK 		0xfffffeff 	/* bit 8 */
#    define FS_bfull_disable 		0x0 		
#    define FS_bfull_enable 		0x100 		
#    define FS_bempty_MASK 		0xfffffdff 	/* bit 9 */
#    define FS_bempty_disable 		0x0 		
#    define FS_bempty_enable 		0x200 		

#define MGAREG_FOGCOL 				0x1cf4
#define MGAREG_FOGSTART 			0x1cc4
#define MGAREG_FOGXINC 			0x1cd4
#define MGAREG_FOGYINC 			0x1ce4
#define MGAREG_FXBNDRY 			0x1c84

#    define XA_fxleft_MASK 		0xffff0000 	/* bits 0-15 */
#    define XA_fxleft_SHIFT 		0 		
#    define XA_fxright_MASK 		0xffff 		/* bits 16-31 */
#    define XA_fxright_SHIFT 		16 		

#define MGAREG_FXLEFT 				0x1ca8
#define MGAREG_FXRIGHT 			0x1cac
#define MGAREG_ICLEAR 				0x1e18

#    define IC_softrapiclr_MASK 	0xfffffffe 	/* bit 0 */
#    define IC_softrapiclr_disable 	0x0 		
#    define IC_softrapiclr_enable 	0x1 		
#    define IC_pickiclr_MASK 		0xfffffffb 	/* bit 2 */
#    define IC_pickiclr_disable 	0x0 		
#    define IC_pickiclr_enable 		0x4 		
#    define IC_vlineiclr_MASK 		0xffffffdf 	/* bit 5 */
#    define IC_vlineiclr_disable 	0x0 		
#    define IC_vlineiclr_enable 	0x20 		
#    define IC_wiclr_MASK 		0xffffff7f 	/* bit 7 */
#    define IC_wiclr_disable 		0x0 		
#    define IC_wiclr_enable 		0x80 		
#    define IC_wciclr_MASK 		0xfffffeff 	/* bit 8 */
#    define IC_wciclr_disable 		0x0 		
#    define IC_wciclr_enable 		0x100 		

#define MGAREG_IEN 				0x1e1c

#    define IE_softrapien_MASK 		0xfffffffe 	/* bit 0 */
#    define IE_softrapien_disable 	0x0 		
#    define IE_softrapien_enable 	0x1 		
#    define IE_pickien_MASK 		0xfffffffb 	/* bit 2 */
#    define IE_pickien_disable 		0x0 		
#    define IE_pickien_enable 		0x4 		
#    define IE_vlineien_MASK 		0xffffffdf 	/* bit 5 */
#    define IE_vlineien_disable 	0x0 		
#    define IE_vlineien_enable 		0x20 		
#    define IE_extien_MASK 		0xffffffbf 	/* bit 6 */
#    define IE_extien_disable 		0x0 		
#    define IE_extien_enable 		0x40 		
#    define IE_wien_MASK 		0xffffff7f 	/* bit 7 */
#    define IE_wien_disable 		0x0 		
#    define IE_wien_enable 		0x80 		
#    define IE_wcien_MASK 		0xfffffeff 	/* bit 8 */
#    define IE_wcien_disable 		0x0 		
#    define IE_wcien_enable 		0x100 		

#define MGAREG_LEN 				0x1c5c
#define MGAREG_MACCESS 			0x1c04

#    define MA_pwidth_MASK 		0xfffffffc 	/* bits 0-1 */
#    define MA_pwidth_8 		0x0 		/* val 0, shift 0 */
#    define MA_pwidth_16 		0x1 		/* val 1, shift 0 */
#    define MA_pwidth_32 		0x2 		/* val 2, shift 0 */
#    define MA_pwidth_24 		0x3 		/* val 3, shift 0 */
#    define MA_zwidth_MASK 		0xffffffe7 	/* bits 3-4 */
#    define MA_zwidth_16 		0x0 		/* val 0, shift 3 */
#    define MA_zwidth_32 		0x8 		/* val 1, shift 3 */
#    define MA_zwidth_15 		0x10 		/* val 2, shift 3 */
#    define MA_zwidth_24 		0x18 		/* val 3, shift 3 */
#    define MA_memreset_MASK 		0xffff7fff 	/* bit 15 */
#    define MA_memreset_disable 	0x0 		
#    define MA_memreset_enable 		0x8000 		
#    define MA_fogen_MASK 		0xfbffffff 	/* bit 26 */
#    define MA_fogen_disable 		0x0 		
#    define MA_fogen_enable 		0x4000000 	
#    define MA_tlutload_MASK 		0xdfffffff 	/* bit 29 */
#    define MA_tlutload_disable 	0x0 		
#    define MA_tlutload_enable 		0x20000000 	
#    define MA_nodither_MASK 		0xbfffffff 	/* bit 30 */
#    define MA_nodither_disable 	0x0 		
#    define MA_nodither_enable 		0x40000000 	
#    define MA_dit555_MASK 		0x7fffffff 	/* bit 31 */
#    define MA_dit555_disable 		0x0 		
#    define MA_dit555_enable 		0x80000000 	

#define MGAREG_MCTLWTST 			0x1c08

#    define MCWS_casltncy_MASK 		0xfffffff8 	/* bits 0-2 */
#    define MCWS_casltncy_SHIFT 	0 		
#    define MCWS_rrddelay_MASK 		0xffffffcf 	/* bits 4-5 */
#    define MCWS_rcddelay_MASK 		0xfffffe7f 	/* bits 7-8 */
#    define MCWS_rasmin_MASK 		0xffffe3ff 	/* bits 10-12 */
#    define MCWS_rasmin_SHIFT 		10 		
#    define MCWS_rpdelay_MASK 		0xffff3fff 	/* bits 14-15 */
#    define MCWS_wrdelay_MASK 		0xfff3ffff 	/* bits 18-19 */
#    define MCWS_rddelay_MASK 		0xffdfffff 	/* bit 21 */
#    define MCWS_rddelay_disable 	0x0 		
#    define MCWS_rddelay_enable 	0x200000 	
#    define MCWS_smrdelay_MASK 		0xfe7fffff 	/* bits 23-24 */
#    define MCWS_bwcdelay_MASK 		0xf3ffffff 	/* bits 26-27 */
#    define MCWS_bpldelay_MASK 		0x1fffffff 	/* bits 29-31 */
#    define MCWS_bpldelay_SHIFT 	29 		

#define MGAREG_MEMRDBK 			0x1e44

#    define MRB_mclkbrd0_MASK 		0xfffffff0 	/* bits 0-3 */
#    define MRB_mclkbrd0_SHIFT 		0 		
#    define MRB_mclkbrd1_MASK 		0xfffffe1f 	/* bits 5-8 */
#    define MRB_mclkbrd1_SHIFT 		5 		
#    define MRB_strmfctl_MASK 		0xff3fffff 	/* bits 22-23 */
#    define MRB_mrsopcod_MASK 		0xe1ffffff 	/* bits 25-28 */
#    define MRB_mrsopcod_SHIFT 		25 		

#define MGAREG_OPMODE 				0x1e54

#    define OM_dmamod_MASK 		0xfffffff3 	/* bits 2-3 */
#    define OM_dmamod_general 		0x0 		/* val 0, shift 2 */
#    define OM_dmamod_blit 		0x4 		/* val 1, shift 2 */
#    define OM_dmamod_vector 		0x8 		/* val 2, shift 2 */
#    define OM_dmamod_vertex 		0xc 		/* val 3, shift 2 */
#    define OM_dmadatasiz_MASK 		0xfffffcff 	/* bits 8-9 */
#    define OM_dmadatasiz_8 		0x0 		/* val 0, shift 8 */
#    define OM_dmadatasiz_16 		0x100 		/* val 1, shift 8 */
#    define OM_dmadatasiz_32 		0x200 		/* val 2, shift 8 */
#    define OM_dirdatasiz_MASK 		0xfffcffff 	/* bits 16-17 */
#    define OM_dirdatasiz_8 		0x0 		/* val 0, shift 16 */
#    define OM_dirdatasiz_16 		0x10000 	/* val 1, shift 16 */
#    define OM_dirdatasiz_32 		0x20000 	/* val 2, shift 16 */

#define MGAREG_PAT0 				0x1c10
#define MGAREG_PAT1 				0x1c14
#define MGAREG_PITCH 				0x1c8c

#    define P_iy_MASK 			0xffffe000 	/* bits 0-12 */
#    define P_iy_SHIFT 			0 		
#    define P_ylin_MASK 		0xffff7fff 	/* bit 15 */
#    define P_ylin_disable 		0x0 		
#    define P_ylin_enable 		0x8000 		

#define MGAREG_PLNWT 				0x1c1c
#define MGAREG_PRIMADDRESS 			0x1e58

#    define PDCA_primod_MASK 		0xfffffffc 	/* bits 0-1 */
#    define PDCA_primod_general 	0x0 		/* val 0, shift 0 */
#    define PDCA_primod_blit 		0x1 		/* val 1, shift 0 */
#    define PDCA_primod_vector 		0x2 		/* val 2, shift 0 */
#    define PDCA_primod_vertex 		0x3 		/* val 3, shift 0 */
#    define PDCA_primaddress_MASK 	0x3 		/* bits 2-31 */
#    define PDCA_primaddress_SHIFT 	2 		

#define MGAREG_PRIMEND 			0x1e5c

#    define PDEA_primnostart_MASK 	0xfffffffe 	/* bit 0 */
#    define PDEA_primnostart_disable 	0x0 		
#    define PDEA_primnostart_enable 	0x1 		
#    define PDEA_pagpxfer_MASK 		0xfffffffd 	/* bit 1 */
#    define PDEA_pagpxfer_disable 	0x0 		
#    define PDEA_pagpxfer_enable 	0x2 		
#    define PDEA_primend_MASK 		0x3 		/* bits 2-31 */
#    define PDEA_primend_SHIFT 		2 		

#define MGAREG_PRIMPTR 			0x1e50

#    define PLS_primptren0_MASK 	0xfffffffe 	/* bit 0 */
#    define PLS_primptren0_disable 	0x0 		
#    define PLS_primptren0_enable 	0x1 		
#    define PLS_primptren1_MASK 	0xfffffffd 	/* bit 1 */
#    define PLS_primptren1_disable 	0x0 		
#    define PLS_primptren1_enable 	0x2 		
#    define PLS_primptr_MASK 		0x7 		/* bits 3-31 */
#    define PLS_primptr_SHIFT 		3 		

#define MGAREG_RST 				0x1e40

#    define R_softreset_MASK 		0xfffffffe 	/* bit 0 */
#    define R_softreset_disable 	0x0 		
#    define R_softreset_enable 		0x1 		
#    define R_softextrst_MASK 		0xfffffffd 	/* bit 1 */
#    define R_softextrst_disable 	0x0 		
#    define R_softextrst_enable 	0x2 		

#define MGAREG_SECADDRESS 			0x2c40

#    define SDCA_secmod_MASK 		0xfffffffc 	/* bits 0-1 */
#    define SDCA_secmod_general 	0x0 		/* val 0, shift 0 */
#    define SDCA_secmod_blit 		0x1 		/* val 1, shift 0 */
#    define SDCA_secmod_vector 		0x2 		/* val 2, shift 0 */
#    define SDCA_secmod_vertex 		0x3 		/* val 3, shift 0 */
#    define SDCA_secaddress_MASK 	0x3 		/* bits 2-31 */
#    define SDCA_secaddress_SHIFT 	2 		

#define MGAREG_SECEND 				0x2c44

#    define SDEA_sagpxfer_MASK 		0xfffffffd 	/* bit 1 */
#    define SDEA_sagpxfer_disable 	0x0 		
#    define SDEA_sagpxfer_enable 	0x2 		
#    define SDEA_secend_MASK 		0x3 		/* bits 2-31 */
#    define SDEA_secend_SHIFT 		2 		

#define MGAREG_SETUPADDRESS 			0x2cd0

#    define SETADD_mode_MASK 		0xfffffffc 	/* bits 0-1 */
#    define SETADD_mode_vertlist 	0x0 		/* val 0, shift 0 */
#    define SETADD_address_MASK 	0x3 		/* bits 2-31 */
#    define SETADD_address_SHIFT 	2 		

#define MGAREG_SETUPEND 			0x2cd4

#    define SETEND_agpxfer_MASK 	0xfffffffd 	/* bit 1 */
#    define SETEND_agpxfer_disable 	0x0 		
#    define SETEND_agpxfer_enable 	0x2 		
#    define SETEND_address_MASK 	0x3 		/* bits 2-31 */
#    define SETEND_address_SHIFT 	2 		

#define MGAREG_SGN 				0x1c58

#    define S_sdydxl_MASK 		0xfffffffe 	/* bit 0 */
#    define S_sdydxl_y 			0x0 		
#    define S_sdydxl_x 			0x1 		
#    define S_scanleft_MASK 		0xfffffffe 	/* bit 0 */
#    define S_scanleft_disable 		0x0 		
#    define S_scanleft_enable 		0x1 		
#    define S_sdxl_MASK 		0xfffffffd 	/* bit 1 */
#    define S_sdxl_pos 			0x0 		
#    define S_sdxl_neg 			0x2 		
#    define S_sdy_MASK 			0xfffffffb 	/* bit 2 */
#    define S_sdy_pos 			0x0 		
#    define S_sdy_neg 			0x4 		
#    define S_sdxr_MASK 		0xffffffdf 	/* bit 5 */
#    define S_sdxr_pos 			0x0 		
#    define S_sdxr_neg 			0x20 		
#    define S_brkleft_MASK 		0xfffffeff 	/* bit 8 */
#    define S_brkleft_disable 		0x0 		
#    define S_brkleft_enable 		0x100 		
#    define S_errorinit_MASK 		0x7fffffff 	/* bit 31 */
#    define S_errorinit_disable 	0x0 		
#    define S_errorinit_enable 		0x80000000 	

#define MGAREG_SHIFT 				0x1c50

#    define FSC_x_off_MASK 		0xfffffff0 	/* bits 0-3 */
#    define FSC_x_off_SHIFT 		0 		
#    define FSC_funcnt_MASK 		0xffffff80 	/* bits 0-6 */
#    define FSC_funcnt_SHIFT 		0 		
#    define FSC_y_off_MASK 		0xffffff8f 	/* bits 4-6 */
#    define FSC_y_off_SHIFT 		4 		
#    define FSC_funoff_MASK 		0xffc0ffff 	/* bits 16-21 */
#    define FSC_funoff_SHIFT 		16 		
#    define FSC_stylelen_MASK 		0xffc0ffff 	/* bits 16-21 */
#    define FSC_stylelen_SHIFT 		16 		

#define MGAREG_SOFTRAP 			0x2c48

#    define STH_softraphand_MASK 	0x3 		/* bits 2-31 */
#    define STH_softraphand_SHIFT 	2 		

#define MGAREG_SPECBSTART 			0x2c98
#define MGAREG_SPECBXINC 			0x2c9c
#define MGAREG_SPECBYINC 			0x2ca0
#define MGAREG_SPECGSTART 			0x2c8c
#define MGAREG_SPECGXINC 			0x2c90
#define MGAREG_SPECGYINC 			0x2c94
#define MGAREG_SPECRSTART 			0x2c80
#define MGAREG_SPECRXINC 			0x2c84
#define MGAREG_SPECRYINC 			0x2c88
#define MGAREG_SRC0 				0x1c30
#define MGAREG_SRC1 				0x1c34
#define MGAREG_SRC2 				0x1c38
#define MGAREG_SRC3 				0x1c3c
#define MGAREG_SRCORG 				0x2cb4

#    define SO_srcmap_MASK 		0xfffffffe 	/* bit 0 */
#    define SO_srcmap_fb 		0x0 		
#    define SO_srcmap_sys 		0x1 		
#    define SO_srcacc_MASK 		0xfffffffd 	/* bit 1 */
#    define SO_srcacc_pci 		0x0 		
#    define SO_srcacc_agp 		0x2 		
#    define SO_srcorg_MASK 		0x7 		/* bits 3-31 */
#    define SO_srcorg_SHIFT 		3 		

#define MGAREG_STATUS 				0x1e14

#    define STAT_softrapen_MASK 	0xfffffffe 	/* bit 0 */
#    define STAT_softrapen_disable 	0x0 		
#    define STAT_softrapen_enable 	0x1 		
#    define STAT_pickpen_MASK 		0xfffffffb 	/* bit 2 */
#    define STAT_pickpen_disable 	0x0 		
#    define STAT_pickpen_enable 	0x4 		
#    define STAT_vsyncsts_MASK 		0xfffffff7 	/* bit 3 */
#    define STAT_vsyncsts_disable 	0x0 		
#    define STAT_vsyncsts_enable 	0x8 		
#    define STAT_vsyncpen_MASK 		0xffffffef 	/* bit 4 */
#    define STAT_vsyncpen_disable 	0x0 		
#    define STAT_vsyncpen_enable 	0x10 		
#    define STAT_vlinepen_MASK 		0xffffffdf 	/* bit 5 */
#    define STAT_vlinepen_disable 	0x0 		
#    define STAT_vlinepen_enable 	0x20 		
#    define STAT_extpen_MASK 		0xffffffbf 	/* bit 6 */
#    define STAT_extpen_disable 	0x0 		
#    define STAT_extpen_enable 		0x40 		
#    define STAT_wpen_MASK 		0xffffff7f 	/* bit 7 */
#    define STAT_wpen_disable 		0x0 		
#    define STAT_wpen_enable 		0x80 		
#    define STAT_wcpen_MASK 		0xfffffeff 	/* bit 8 */
#    define STAT_wcpen_disable 		0x0 		
#    define STAT_wcpen_enable 		0x100 		
#    define STAT_dwgengsts_MASK 	0xfffeffff 	/* bit 16 */
#    define STAT_dwgengsts_disable 	0x0 		
#    define STAT_dwgengsts_enable 	0x10000 	
#    define STAT_endprdmasts_MASK 	0xfffdffff 	/* bit 17 */
#    define STAT_endprdmasts_disable 	0x0 		
#    define STAT_endprdmasts_enable 	0x20000 	
#    define STAT_wbusy_MASK 		0xfffbffff 	/* bit 18 */
#    define STAT_wbusy_disable 		0x0 		
#    define STAT_wbusy_enable 		0x40000 	
#    define STAT_swflag_MASK 		0xfffffff 	/* bits 28-31 */
#    define STAT_swflag_SHIFT 		28 		

#define MGAREG_STENCIL 			0x2cc8

#    define S_sref_MASK 		0xffffff00 	/* bits 0-7 */
#    define S_sref_SHIFT 		0 		
#    define S_smsk_MASK 		0xffff00ff 	/* bits 8-15 */
#    define S_smsk_SHIFT 		8 		
#    define S_swtmsk_MASK 		0xff00ffff 	/* bits 16-23 */
#    define S_swtmsk_SHIFT 		16 		

#define MGAREG_STENCILCTL 			0x2ccc

#    define SC_smode_MASK 		0xfffffff8 	/* bits 0-2 */
#    define SC_smode_salways 		0x0 		/* val 0, shift 0 */
#    define SC_smode_snever 		0x1 		/* val 1, shift 0 */
#    define SC_smode_se 		0x2 		/* val 2, shift 0 */
#    define SC_smode_sne 		0x3 		/* val 3, shift 0 */
#    define SC_smode_slt 		0x4 		/* val 4, shift 0 */
#    define SC_smode_slte 		0x5 		/* val 5, shift 0 */
#    define SC_smode_sgt 		0x6 		/* val 6, shift 0 */
#    define SC_smode_sgte 		0x7 		/* val 7, shift 0 */
#    define SC_sfailop_MASK 		0xffffffc7 	/* bits 3-5 */
#    define SC_sfailop_keep 		0x0 		/* val 0, shift 3 */
#    define SC_sfailop_zero 		0x8 		/* val 1, shift 3 */
#    define SC_sfailop_replace 		0x10 		/* val 2, shift 3 */
#    define SC_sfailop_incrsat 		0x18 		/* val 3, shift 3 */
#    define SC_sfailop_decrsat 		0x20 		/* val 4, shift 3 */
#    define SC_sfailop_invert 		0x28 		/* val 5, shift 3 */
#    define SC_sfailop_incr 		0x30 		/* val 6, shift 3 */
#    define SC_sfailop_decr 		0x38 		/* val 7, shift 3 */
#    define SC_szfailop_MASK 		0xfffffe3f 	/* bits 6-8 */
#    define SC_szfailop_keep 		0x0 		/* val 0, shift 6 */
#    define SC_szfailop_zero 		0x40 		/* val 1, shift 6 */
#    define SC_szfailop_replace 	0x80 		/* val 2, shift 6 */
#    define SC_szfailop_incrsat 	0xc0 		/* val 3, shift 6 */
#    define SC_szfailop_decrsat 	0x100 		/* val 4, shift 6 */
#    define SC_szfailop_invert 		0x140 		/* val 5, shift 6 */
#    define SC_szfailop_incr 		0x180 		/* val 6, shift 6 */
#    define SC_szfailop_decr 		0x1c0 		/* val 7, shift 6 */
#    define SC_szpassop_MASK 		0xfffff1ff 	/* bits 9-11 */
#    define SC_szpassop_keep 		0x0 		/* val 0, shift 9 */
#    define SC_szpassop_zero 		0x200 		/* val 1, shift 9 */
#    define SC_szpassop_replace 	0x400 		/* val 2, shift 9 */
#    define SC_szpassop_incrsat 	0x600 		/* val 3, shift 9 */
#    define SC_szpassop_decrsat 	0x800 		/* val 4, shift 9 */
#    define SC_szpassop_invert 		0xa00 		/* val 5, shift 9 */
#    define SC_szpassop_incr 		0xc00 		/* val 6, shift 9 */
#    define SC_szpassop_decr 		0xe00 		/* val 7, shift 9 */

#define MGAREG_TDUALSTAGE0 			0x2cf8

#    define TD0_color_arg2_MASK 	0xfffffffc 	/* bits 0-1 */
#    define TD0_color_arg2_diffuse 	0x0 		/* val 0, shift 0 */
#    define TD0_color_arg2_specular 	0x1 		/* val 1, shift 0 */
#    define TD0_color_arg2_fcol 	0x2 		/* val 2, shift 0 */
#    define TD0_color_arg2_prevstage 	0x3 		/* val 3, shift 0 */
#    define TD0_color_alpha_MASK 	0xffffffe3 	/* bits 2-4 */
#    define TD0_color_alpha_diffuse 	0x0 		/* val 0, shift 2 */
#    define TD0_color_alpha_fcol 	0x4 		/* val 1, shift 2 */
#    define TD0_color_alpha_currtex 	0x8 		/* val 2, shift 2 */
#    define TD0_color_alpha_prevtex 	0xc 		/* val 3, shift 2 */
#    define TD0_color_alpha_prevstage 	0x10 		/* val 4, shift 2 */
#    define TD0_color_arg1_replicatealpha_MASK 0xffffffdf 	/* bit 5 */
#    define TD0_color_arg1_replicatealpha_disable 0x0 		
#    define TD0_color_arg1_replicatealpha_enable 0x20 		
#    define TD0_color_arg1_inv_MASK 	0xffffffbf 	/* bit 6 */
#    define TD0_color_arg1_inv_disable 	0x0 		
#    define TD0_color_arg1_inv_enable 	0x40 		
#    define TD0_color_arg2_replicatealpha_MASK 0xffffff7f 	/* bit 7 */
#    define TD0_color_arg2_replicatealpha_disable 0x0 		
#    define TD0_color_arg2_replicatealpha_enable 0x80 		
#    define TD0_color_arg2_inv_MASK 	0xfffffeff 	/* bit 8 */
#    define TD0_color_arg2_inv_disable 	0x0 		
#    define TD0_color_arg2_inv_enable 	0x100 		
#    define TD0_color_alpha1inv_MASK 	0xfffffdff 	/* bit 9 */
#    define TD0_color_alpha1inv_disable 0x0 		
#    define TD0_color_alpha1inv_enable 	0x200 		
#    define TD0_color_alpha2inv_MASK 	0xfffffbff 	/* bit 10 */
#    define TD0_color_alpha2inv_disable 0x0 		
#    define TD0_color_alpha2inv_enable 	0x400 		
#    define TD0_color_arg1mul_MASK 	0xfffff7ff 	/* bit 11 */
#    define TD0_color_arg1mul_disable 	0x0 		/* val 0, shift 11 */
#    define TD0_color_arg1mul_alpha1 	0x800 		/* val 1, shift 11 */
#    define TD0_color_arg2mul_MASK 	0xffffefff 	/* bit 12 */
#    define TD0_color_arg2mul_disable 	0x0 		/* val 0, shift 12 */
#    define TD0_color_arg2mul_alpha2 	0x1000 		/* val 1, shift 12 */
#    define TD0_color_arg1add_MASK 	0xffffdfff 	/* bit 13 */
#    define TD0_color_arg1add_disable 	0x0 		/* val 0, shift 13 */
#    define TD0_color_arg1add_mulout 	0x2000 		/* val 1, shift 13 */
#    define TD0_color_arg2add_MASK 	0xffffbfff 	/* bit 14 */
#    define TD0_color_arg2add_disable 	0x0 		/* val 0, shift 14 */
#    define TD0_color_arg2add_mulout 	0x4000 		/* val 1, shift 14 */
#    define TD0_color_modbright_MASK 	0xfffe7fff 	/* bits 15-16 */
#    define TD0_color_modbright_disable 0x0 		/* val 0, shift 15 */
#    define TD0_color_modbright_2x 	0x8000 		/* val 1, shift 15 */
#    define TD0_color_modbright_4x 	0x10000 	/* val 2, shift 15 */
#    define TD0_color_add_MASK 		0xfffdffff 	/* bit 17 */
#    define TD0_color_add_sub 		0x0 		/* val 0, shift 17 */
#    define TD0_color_add_add 		0x20000 	/* val 1, shift 17 */
#    define TD0_color_add2x_MASK 	0xfffbffff 	/* bit 18 */
#    define TD0_color_add2x_disable 	0x0 		
#    define TD0_color_add2x_enable 	0x40000 	
#    define TD0_color_addbias_MASK 	0xfff7ffff 	/* bit 19 */
#    define TD0_color_addbias_disable 	0x0 		
#    define TD0_color_addbias_enable 	0x80000 	
#    define TD0_color_blend_MASK 	0xffefffff 	/* bit 20 */
#    define TD0_color_blend_disable 	0x0 		
#    define TD0_color_blend_enable 	0x100000 	
#    define TD0_color_sel_MASK 		0xff9fffff 	/* bits 21-22 */
#    define TD0_color_sel_arg1 		0x0 		/* val 0, shift 21 */
#    define TD0_color_sel_arg2 		0x200000 	/* val 1, shift 21 */
#    define TD0_color_sel_add 		0x400000 	/* val 2, shift 21 */
#    define TD0_color_sel_mul 		0x600000 	/* val 3, shift 21 */
#    define TD0_alpha_arg1_inv_MASK 	0xff7fffff 	/* bit 23 */
#    define TD0_alpha_arg1_inv_disable 	0x0 		
#    define TD0_alpha_arg1_inv_enable 	0x800000 	
#    define TD0_alpha_arg2_MASK 	0xfcffffff 	/* bits 24-25 */
#    define TD0_alpha_arg2_diffuse 	0x0 		/* val 0, shift 24 */
#    define TD0_alpha_arg2_fcol 	0x1000000 	/* val 1, shift 24 */
#    define TD0_alpha_arg2_prevtex 	0x2000000 	/* val 2, shift 24 */
#    define TD0_alpha_arg2_prevstage 	0x3000000 	/* val 3, shift 24 */
#    define TD0_alpha_arg2_inv_MASK 	0xfbffffff 	/* bit 26 */
#    define TD0_alpha_arg2_inv_disable 	0x0 		
#    define TD0_alpha_arg2_inv_enable 	0x4000000 	
#    define TD0_alpha_add_MASK 		0xf7ffffff 	/* bit 27 */
#    define TD0_alpha_add_disable 	0x0 		
#    define TD0_alpha_add_enable 	0x8000000 	
#    define TD0_alpha_addbias_MASK 	0xefffffff 	/* bit 28 */
#    define TD0_alpha_addbias_disable 	0x0 		
#    define TD0_alpha_addbias_enable 	0x10000000 	
#    define TD0_alpha_add2x_MASK 	0xdfffffff 	/* bit 29 */
#    define TD0_alpha_add2x_disable 	0x0 		
#    define TD0_alpha_add2x_enable 	0x20000000 	
#    define TD0_alpha_modbright_MASK 	0xcfffffff 	/* bits 28-29 */
#    define TD0_alpha_modbright_disable 0x0 		/* val 0, shift 28 */
#    define TD0_alpha_modbright_2x 	0x10000000 	/* val 1, shift 28 */
#    define TD0_alpha_modbright_4x 	0x20000000 	/* val 2, shift 28 */
#    define TD0_alpha_sel_MASK 		0x3fffffff 	/* bits 30-31 */
#    define TD0_alpha_sel_arg1 		0x0 		/* val 0, shift 30 */
#    define TD0_alpha_sel_arg2 		0x40000000 	/* val 1, shift 30 */
#    define TD0_alpha_sel_add 		0x80000000 	/* val 2, shift 30 */
#    define TD0_alpha_sel_mul 		0xc0000000 	/* val 3, shift 30 */

#define MGAREG_TDUALSTAGE1 			0x2cfc

#    define TD1_color_arg2_MASK 	0xfffffffc 	/* bits 0-1 */
#    define TD1_color_arg2_diffuse 	0x0 		/* val 0, shift 0 */
#    define TD1_color_arg2_specular 	0x1 		/* val 1, shift 0 */
#    define TD1_color_arg2_fcol 	0x2 		/* val 2, shift 0 */
#    define TD1_color_arg2_prevstage 	0x3 		/* val 3, shift 0 */
#    define TD1_color_alpha_MASK 	0xffffffe3 	/* bits 2-4 */
#    define TD1_color_alpha_diffuse 	0x0 		/* val 0, shift 2 */
#    define TD1_color_alpha_fcol 	0x4 		/* val 1, shift 2 */
#    define TD1_color_alpha_tex0 	0x8 		/* val 2, shift 2 */
#    define TD1_color_alpha_prevtex 	0xc 		/* val 3, shift 2 */
#    define TD1_color_alpha_prevstage 	0x10 		/* val 4, shift 2 */
#    define TD1_color_arg1_replicatealpha_MASK 0xffffffdf 	/* bit 5 */
#    define TD1_color_arg1_replicatealpha_disable 0x0 		
#    define TD1_color_arg1_replicatealpha_enable 0x20 		
#    define TD1_color_arg1_inv_MASK 	0xffffffbf 	/* bit 6 */
#    define TD1_color_arg1_inv_disable 	0x0 		
#    define TD1_color_arg1_inv_enable 	0x40 		
#    define TD1_color_arg2_replicatealpha_MASK 0xffffff7f 	/* bit 7 */
#    define TD1_color_arg2_replicatealpha_disable 0x0 		
#    define TD1_color_arg2_replicatealpha_enable 0x80 		
#    define TD1_color_arg2_inv_MASK 	0xfffffeff 	/* bit 8 */
#    define TD1_color_arg2_inv_disable 	0x0 		
#    define TD1_color_arg2_inv_enable 	0x100 		
#    define TD1_color_alpha1inv_MASK 	0xfffffdff 	/* bit 9 */
#    define TD1_color_alpha1inv_disable 0x0 		
#    define TD1_color_alpha1inv_enable 	0x200 		
#    define TD1_color_alpha2inv_MASK 	0xfffffbff 	/* bit 10 */
#    define TD1_color_alpha2inv_disable 0x0 		
#    define TD1_color_alpha2inv_enable 	0x400 		
#    define TD1_color_arg1mul_MASK 	0xfffff7ff 	/* bit 11 */
#    define TD1_color_arg1mul_disable 	0x0 		/* val 0, shift 11 */
#    define TD1_color_arg1mul_alpha1 	0x800 		/* val 1, shift 11 */
#    define TD1_color_arg2mul_MASK 	0xffffefff 	/* bit 12 */
#    define TD1_color_arg2mul_disable 	0x0 		/* val 0, shift 12 */
#    define TD1_color_arg2mul_alpha2 	0x1000 		/* val 1, shift 12 */
#    define TD1_color_arg1add_MASK 	0xffffdfff 	/* bit 13 */
#    define TD1_color_arg1add_disable 	0x0 		/* val 0, shift 13 */
#    define TD1_color_arg1add_mulout 	0x2000 		/* val 1, shift 13 */
#    define TD1_color_arg2add_MASK 	0xffffbfff 	/* bit 14 */
#    define TD1_color_arg2add_disable 	0x0 		/* val 0, shift 14 */
#    define TD1_color_arg2add_mulout 	0x4000 		/* val 1, shift 14 */
#    define TD1_color_modbright_MASK 	0xfffe7fff 	/* bits 15-16 */
#    define TD1_color_modbright_disable 0x0 		/* val 0, shift 15 */
#    define TD1_color_modbright_2x 	0x8000 		/* val 1, shift 15 */
#    define TD1_color_modbright_4x 	0x10000 	/* val 2, shift 15 */
#    define TD1_color_add_MASK 		0xfffdffff 	/* bit 17 */
#    define TD1_color_add_sub 		0x0 		/* val 0, shift 17 */
#    define TD1_color_add_add 		0x20000 	/* val 1, shift 17 */
#    define TD1_color_add2x_MASK 	0xfffbffff 	/* bit 18 */
#    define TD1_color_add2x_disable 	0x0 		
#    define TD1_color_add2x_enable 	0x40000 	
#    define TD1_color_addbias_MASK 	0xfff7ffff 	/* bit 19 */
#    define TD1_color_addbias_disable 	0x0 		
#    define TD1_color_addbias_enable 	0x80000 	
#    define TD1_color_blend_MASK 	0xffefffff 	/* bit 20 */
#    define TD1_color_blend_disable 	0x0 		
#    define TD1_color_blend_enable 	0x100000 	
#    define TD1_color_sel_MASK 		0xff9fffff 	/* bits 21-22 */
#    define TD1_color_sel_arg1 		0x0 		/* val 0, shift 21 */
#    define TD1_color_sel_arg2 		0x200000 	/* val 1, shift 21 */
#    define TD1_color_sel_add 		0x400000 	/* val 2, shift 21 */
#    define TD1_color_sel_mul 		0x600000 	/* val 3, shift 21 */
#    define TD1_alpha_arg1_inv_MASK 	0xff7fffff 	/* bit 23 */
#    define TD1_alpha_arg1_inv_disable 	0x0 		
#    define TD1_alpha_arg1_inv_enable 	0x800000 	
#    define TD1_alpha_arg2_MASK 	0xfcffffff 	/* bits 24-25 */
#    define TD1_alpha_arg2_diffuse 	0x0 		/* val 0, shift 24 */
#    define TD1_alpha_arg2_fcol 	0x1000000 	/* val 1, shift 24 */
#    define TD1_alpha_arg2_prevtex 	0x2000000 	/* val 2, shift 24 */
#    define TD1_alpha_arg2_prevstage 	0x3000000 	/* val 3, shift 24 */
#    define TD1_alpha_arg2_inv_MASK 	0xfbffffff 	/* bit 26 */
#    define TD1_alpha_arg2_inv_disable 	0x0 		
#    define TD1_alpha_arg2_inv_enable 	0x4000000 	
#    define TD1_alpha_add_MASK 		0xf7ffffff 	/* bit 27 */
#    define TD1_alpha_add_disable 	0x0 		
#    define TD1_alpha_add_enable 	0x8000000 	
#    define TD1_alpha_addbias_MASK 	0xefffffff 	/* bit 28 */
#    define TD1_alpha_addbias_disable 	0x0 		
#    define TD1_alpha_addbias_enable 	0x10000000 	
#    define TD1_alpha_add2x_MASK 	0xdfffffff 	/* bit 29 */
#    define TD1_alpha_add2x_disable 	0x0 		
#    define TD1_alpha_add2x_enable 	0x20000000 	
#    define TD1_alpha_modbright_MASK 	0xcfffffff 	/* bits 28-29 */
#    define TD1_alpha_modbright_disable 0x0 		/* val 0, shift 28 */
#    define TD1_alpha_modbright_2x 	0x10000000 	/* val 1, shift 28 */
#    define TD1_alpha_modbright_4x 	0x20000000 	/* val 2, shift 28 */
#    define TD1_alpha_sel_MASK 		0x3fffffff 	/* bits 30-31 */
#    define TD1_alpha_sel_arg1 		0x0 		/* val 0, shift 30 */
#    define TD1_alpha_sel_arg2 		0x40000000 	/* val 1, shift 30 */
#    define TD1_alpha_sel_add 		0x80000000 	/* val 2, shift 30 */
#    define TD1_alpha_sel_mul 		0xc0000000 	/* val 3, shift 30 */

#define MGAREG_TEST0 				0x1e48

#    define TST_ramtsten_MASK 		0xfffffffe 	/* bit 0 */
#    define TST_ramtsten_disable 	0x0 		
#    define TST_ramtsten_enable 	0x1 		
#    define TST_ramtstdone_MASK 	0xfffffffd 	/* bit 1 */
#    define TST_ramtstdone_disable 	0x0 		
#    define TST_ramtstdone_enable 	0x2 		
#    define TST_wramtstpass_MASK 	0xfffffffb 	/* bit 2 */
#    define TST_wramtstpass_disable 	0x0 		
#    define TST_wramtstpass_enable 	0x4 		
#    define TST_tcachetstpass_MASK 	0xfffffff7 	/* bit 3 */
#    define TST_tcachetstpass_disable 	0x0 		
#    define TST_tcachetstpass_enable 	0x8 		
#    define TST_tluttstpass_MASK 	0xffffffef 	/* bit 4 */
#    define TST_tluttstpass_disable 	0x0 		
#    define TST_tluttstpass_enable 	0x10 		
#    define TST_luttstpass_MASK 	0xffffffdf 	/* bit 5 */
#    define TST_luttstpass_disable 	0x0 		
#    define TST_luttstpass_enable 	0x20 		
#    define TST_besramtstpass_MASK 	0xffffffbf 	/* bit 6 */
#    define TST_besramtstpass_disable 	0x0 		
#    define TST_besramtstpass_enable 	0x40 		
#    define TST_ringen_MASK 		0xfffffeff 	/* bit 8 */
#    define TST_ringen_disable 		0x0 		
#    define TST_ringen_enable 		0x100 		
#    define TST_apllbyp_MASK 		0xfffffdff 	/* bit 9 */
#    define TST_apllbyp_disable 	0x0 		
#    define TST_apllbyp_enable 		0x200 		
#    define TST_hiten_MASK 		0xfffffbff 	/* bit 10 */
#    define TST_hiten_disable 		0x0 		
#    define TST_hiten_enable 		0x400 		
#    define TST_tmode_MASK 		0xffffc7ff 	/* bits 11-13 */
#    define TST_tmode_SHIFT 		11 		
#    define TST_tclksel_MASK 		0xfffe3fff 	/* bits 14-16 */
#    define TST_tclksel_SHIFT 		14 		
#    define TST_ringcnten_MASK 		0xfffdffff 	/* bit 17 */
#    define TST_ringcnten_disable 	0x0 		
#    define TST_ringcnten_enable 	0x20000 	
#    define TST_ringcnt_MASK 		0xc003ffff 	/* bits 18-29 */
#    define TST_ringcnt_SHIFT 		18 		
#    define TST_ringcntclksl_MASK 	0xbfffffff 	/* bit 30 */
#    define TST_ringcntclksl_disable 	0x0 		
#    define TST_ringcntclksl_enable 	0x40000000 	
#    define TST_biosboot_MASK 		0x7fffffff 	/* bit 31 */
#    define TST_biosboot_disable 	0x0 		
#    define TST_biosboot_enable 	0x80000000 	

#define MGAREG_TEXBORDERCOL 			0x2c5c
#define MGAREG_TEXCTL 				0x2c30

#    define TMC_tformat_MASK 		0xfffffff0 	/* bits 0-3 */
#    define TMC_tformat_tw4 		0x0 		/* val 0, shift 0 */
#    define TMC_tformat_tw8 		0x1 		/* val 1, shift 0 */
#    define TMC_tformat_tw15 		0x2 		/* val 2, shift 0 */
#    define TMC_tformat_tw16 		0x3 		/* val 3, shift 0 */
#    define TMC_tformat_tw12 		0x4 		/* val 4, shift 0 */
#    define TMC_tformat_tw32 		0x6 		/* val 6, shift 0 */
#    define TMC_tformat_tw8a 		0x7 		/* val 7, shift 0 */
#    define TMC_tformat_tw8al 		0x8 		/* val 8, shift 0 */
#    define TMC_tformat_tw422 		0xa 		/* val 10, shift 0 */
#    define TMC_tformat_tw422uyvy	0xb 		/* val 11, shift 0 */
#    define TMC_tpitchlin_MASK 		0xfffffeff 	/* bit 8 */
#    define TMC_tpitchlin_disable 	0x0 		
#    define TMC_tpitchlin_enable 	0x100 		
#    define TMC_tpitchext_MASK 		0xfff001ff 	/* bits 9-19 */
#    define TMC_tpitchext_SHIFT 	9 		
#    define TMC_tpitch_MASK 		0xfff8ffff 	/* bits 16-18 */
#    define TMC_tpitch_SHIFT 		16 		
#    define TMC_owalpha_MASK 		0xffbfffff 	/* bit 22 */
#    define TMC_owalpha_disable 	0x0 		
#    define TMC_owalpha_enable 		0x400000 	
#    define TMC_azeroextend_MASK 	0xff7fffff 	/* bit 23 */
#    define TMC_azeroextend_disable 	0x0 		
#    define TMC_azeroextend_enable 	0x800000 	
#    define TMC_decalckey_MASK 		0xfeffffff 	/* bit 24 */
#    define TMC_decalckey_disable 	0x0 		
#    define TMC_decalckey_enable 	0x1000000 	
#    define TMC_takey_MASK 		0xfdffffff 	/* bit 25 */
#    define TMC_takey_0 		0x0 		
#    define TMC_takey_1 		0x2000000 	
#    define TMC_tamask_MASK 		0xfbffffff 	/* bit 26 */
#    define TMC_tamask_0 		0x0 		
#    define TMC_tamask_1 		0x4000000 	
#    define TMC_clampv_MASK 		0xf7ffffff 	/* bit 27 */
#    define TMC_clampv_disable 		0x0 		
#    define TMC_clampv_enable 		0x8000000 	
#    define TMC_clampu_MASK 		0xefffffff 	/* bit 28 */
#    define TMC_clampu_disable 		0x0 		
#    define TMC_clampu_enable 		0x10000000 	
#    define TMC_tmodulate_MASK 		0xdfffffff 	/* bit 29 */
#    define TMC_tmodulate_disable 	0x0 		
#    define TMC_tmodulate_enable 	0x20000000 	
#    define TMC_strans_MASK 		0xbfffffff 	/* bit 30 */
#    define TMC_strans_disable 		0x0 		
#    define TMC_strans_enable 		0x40000000 	
#    define TMC_itrans_MASK 		0x7fffffff 	/* bit 31 */
#    define TMC_itrans_disable 		0x0 		
#    define TMC_itrans_enable 		0x80000000 	

#define MGAREG_TEXCTL2 			0x2c3c

#    define TMC_decalblend_MASK 	0xfffffffe 	/* bit 0 */
#    define TMC_decalblend_disable 	0x0 		
#    define TMC_decalblend_enable 	0x1 		
#    define TMC_idecal_MASK 		0xfffffffd 	/* bit 1 */
#    define TMC_idecal_disable 		0x0 		
#    define TMC_idecal_enable 		0x2 		
#    define TMC_decaldis_MASK 		0xfffffffb 	/* bit 2 */
#    define TMC_decaldis_disable 	0x0 		
#    define TMC_decaldis_enable 	0x4 		
#    define TMC_ckstransdis_MASK 	0xffffffef 	/* bit 4 */
#    define TMC_ckstransdis_disable 	0x0 		
#    define TMC_ckstransdis_enable 	0x10 		
#    define TMC_borderen_MASK 		0xffffffdf 	/* bit 5 */
#    define TMC_borderen_disable 	0x0 		
#    define TMC_borderen_enable 	0x20 		
#    define TMC_specen_MASK 		0xffffffbf 	/* bit 6 */
#    define TMC_specen_disable 		0x0 		
#    define TMC_specen_enable 		0x40 		
#    define TMC_dualtex_MASK 		0xffffff7f 	/* bit 7 */
#    define TMC_dualtex_disable 	0x0 		
#    define TMC_dualtex_enable 		0x80 		
#    define TMC_tablefog_MASK 		0xfffffeff 	/* bit 8 */
#    define TMC_tablefog_disable 	0x0 		
#    define TMC_tablefog_enable 	0x100 		
#    define TMC_bumpmap_MASK 		0xfffffdff 	/* bit 9 */
#    define TMC_bumpmap_disable 	0x0 		
#    define TMC_bumpmap_enable 		0x200 		
#    define TMC_map1_MASK 		0x7fffffff 	/* bit 31 */
#    define TMC_map1_disable 		0x0 		
#    define TMC_map1_enable 		0x80000000 	

#define MGAREG_TEXFILTER 			0x2c58

#    define TF_minfilter_MASK 		0xfffffff0 	/* bits 0-3 */
#    define TF_minfilter_nrst 		0x0 		/* val 0, shift 0 */
#    define TF_minfilter_bilin 		0x2 		/* val 2, shift 0 */
#    define TF_minfilter_cnst 		0x3 		/* val 3, shift 0 */
#    define TF_minfilter_mm1s 		0x8 		/* val 8, shift 0 */
#    define TF_minfilter_mm2s 		0x9 		/* val 9, shift 0 */
#    define TF_minfilter_mm4s 		0xa 		/* val 10, shift 0 */
#    define TF_minfilter_mm8s 		0xc 		/* val 12, shift 0 */
#    define TF_magfilter_MASK 		0xffffff0f 	/* bits 4-7 */
#    define TF_magfilter_nrst 		0x0 		/* val 0, shift 4 */
#    define TF_magfilter_bilin 		0x20 		/* val 2, shift 4 */
#    define TF_magfilter_cnst 		0x30 		/* val 3, shift 4 */
#    define TF_uvoffset_SHIFT		17
#    define TF_uvoffset_OGL		(0U << TF_uvoffset_SHIFT)
#    define TF_uvoffset_D3D		(1U << TF_uvoffset_SHIFT)
#    define TF_uvoffset_MASK		(~(1U << TF_uvoffset_SHIFT))
#    define TF_reserved_MASK		(~0x1ff00)	/* bits 8-16 */
#    define TF_mapnbhigh_SHIFT 		18
#    define TF_mapnbhigh_MASK 		(~(1U << TF_mapnbhigh_SHIFT))
#    define TF_avgstride_MASK 		0xfff7ffff 	/* bit 19 */
#    define TF_avgstride_disable 	0x0 		
#    define TF_avgstride_enable 	0x80000 	
#    define TF_filteralpha_MASK 	0xffefffff 	/* bit 20 */
#    define TF_filteralpha_disable 	0x0 		
#    define TF_filteralpha_enable 	0x100000 	
#    define TF_fthres_MASK 		0xe01fffff 	/* bits 21-28 */
#    define TF_fthres_SHIFT 		21 		
#    define TF_mapnb_MASK 		0x1fffffff 	/* bits 29-31 */
#    define TF_mapnb_SHIFT 		29 		

#define MGAREG_TEXHEIGHT 			0x2c2c

#    define TH_th_MASK 			0xffffffc0 	/* bits 0-5 */
#    define TH_th_SHIFT 		0 		
#    define TH_rfh_MASK 		0xffff81ff 	/* bits 9-14 */
#    define TH_rfh_SHIFT 		9 		
#    define TH_thmask_MASK 		0xe003ffff 	/* bits 18-28 */
#    define TH_thmask_SHIFT 		18 		

#define MGAREG_TEXORG 				0x2c24

#    define TO_texorgmap_MASK 		0xfffffffe 	/* bit 0 */
#    define TO_texorgmap_fb 		0x0 		
#    define TO_texorgmap_sys 		0x1 		
#    define TO_texorgacc_MASK 		0xfffffffd 	/* bit 1 */
#    define TO_texorgacc_pci 		0x0 		
#    define TO_texorgacc_agp 		0x2 		
#    define TO_texorgoffsetsel 		0x4 		
#    define TO_texorg_MASK 		0x1f 		/* bits 5-31 */
#    define TO_texorg_SHIFT 		5 		

#define MGAREG_TEXORG1 			0x2ca4
#define MGAREG_TEXORG2 			0x2ca8
#define MGAREG_TEXORG3 			0x2cac
#define MGAREG_TEXORG4 			0x2cb0
#define MGAREG_TEXTRANS 			0x2c34

#    define TT_tckey_MASK 		0xffff0000 	/* bits 0-15 */
#    define TT_tckey_SHIFT 		0 		
#    define TT_tkmask_MASK 		0xffff 		/* bits 16-31 */
#    define TT_tkmask_SHIFT 		16 		

#define MGAREG_TEXTRANSHIGH 			0x2c38

#    define TT_tckeyh_MASK 		0xffff0000 	/* bits 0-15 */
#    define TT_tckeyh_SHIFT 		0 		
#    define TT_tkmaskh_MASK 		0xffff 		/* bits 16-31 */
#    define TT_tkmaskh_SHIFT 		16 		

#define MGAREG_TEXWIDTH 			0x2c28

#    define TW_tw_MASK 			0xffffffc0 	/* bits 0-5 */
#    define TW_tw_SHIFT 		0 		
#    define TW_rfw_MASK 		0xffff81ff 	/* bits 9-14 */
#    define TW_rfw_SHIFT 		9 		
#    define TW_twmask_MASK 		0xe003ffff 	/* bits 18-28 */
#    define TW_twmask_SHIFT 		18 		

#define MGAREG_TMR0 				0x2c00
#define MGAREG_TMR1 				0x2c04
#define MGAREG_TMR2 				0x2c08
#define MGAREG_TMR3 				0x2c0c
#define MGAREG_TMR4 				0x2c10
#define MGAREG_TMR5 				0x2c14
#define MGAREG_TMR6 				0x2c18
#define MGAREG_TMR7 				0x2c1c
#define MGAREG_TMR8 				0x2c20
#define MGAREG_VBIADDR0 			0x3e08
#define MGAREG_VBIADDR1 			0x3e0c
#define MGAREG_VCOUNT 				0x1e20
#define MGAREG_WACCEPTSEQ 			0x1dd4

#    define WAS_seqdst0_MASK 		0xffffffc0 	/* bits 0-5 */
#    define WAS_seqdst0_SHIFT 		0 		
#    define WAS_seqdst1_MASK 		0xfffff03f 	/* bits 6-11 */
#    define WAS_seqdst1_SHIFT 		6 		
#    define WAS_seqdst2_MASK 		0xfffc0fff 	/* bits 12-17 */
#    define WAS_seqdst2_SHIFT 		12 		
#    define WAS_seqdst3_MASK 		0xff03ffff 	/* bits 18-23 */
#    define WAS_seqdst3_SHIFT 		18 		
#    define WAS_seqlen_MASK 		0xfcffffff 	/* bits 24-25 */
#    define WAS_wfirsttag_MASK 		0xfbffffff 	/* bit 26 */
#    define WAS_wfirsttag_disable 	0x0 		
#    define WAS_wfirsttag_enable 	0x4000000 	
#    define WAS_wsametag_MASK 		0xf7ffffff 	/* bit 27 */
#    define WAS_wsametag_disable 	0x0 		
#    define WAS_wsametag_enable 	0x8000000 	
#    define WAS_seqoff_MASK 		0xefffffff 	/* bit 28 */
#    define WAS_seqoff_disable 		0x0 		
#    define WAS_seqoff_enable 		0x10000000 	

#define MGAREG_WCODEADDR 			0x1e6c

#    define WMA_wcodeaddr_MASK 		0xff 		/* bits 8-31 */
#    define WMA_wcodeaddr_SHIFT 	8 		

#define MGAREG_WFLAG 				0x1dc4

#    define WF_walustsflag_MASK 	0xffffff00 	/* bits 0-7 */
#    define WF_walustsflag_SHIFT 	0 		
#    define WF_walucfgflag_MASK 	0xffff00ff 	/* bits 8-15 */
#    define WF_walucfgflag_SHIFT 	8 		
#    define WF_wprgflag_MASK 		0xffff 		/* bits 16-31 */
#    define WF_wprgflag_SHIFT 		16 		

#define MGAREG_WFLAG1 				0x1de0

#    define WF1_walustsflag1_MASK 	0xffffff00 	/* bits 0-7 */
#    define WF1_walustsflag1_SHIFT 	0 		
#    define WF1_walucfgflag1_MASK 	0xffff00ff 	/* bits 8-15 */
#    define WF1_walucfgflag1_SHIFT 	8 		
#    define WF1_wprgflag1_MASK 		0xffff 		/* bits 16-31 */
#    define WF1_wprgflag1_SHIFT 	16 		

#define MGAREG_WFLAGNB 			0x1e64
#define MGAREG_WFLAGNB1 			0x1e08
#define MGAREG_WGETMSB 			0x1dc8

#    define WGV_wgetmsbmin_MASK 	0xffffffe0 	/* bits 0-4 */
#    define WGV_wgetmsbmin_SHIFT 	0 		
#    define WGV_wgetmsbmax_MASK 	0xffffe0ff 	/* bits 8-12 */
#    define WGV_wgetmsbmax_SHIFT 	8 		
#    define WGV_wbrklefttop_MASK 	0xfffeffff 	/* bit 16 */
#    define WGV_wbrklefttop_disable 	0x0 		
#    define WGV_wbrklefttop_enable 	0x10000 	
#    define WGV_wfastcrop_MASK 		0xfffdffff 	/* bit 17 */
#    define WGV_wfastcrop_disable 	0x0 		
#    define WGV_wfastcrop_enable 	0x20000 	
#    define WGV_wcentersnap_MASK 	0xfffbffff 	/* bit 18 */
#    define WGV_wcentersnap_disable 	0x0 		
#    define WGV_wcentersnap_enable 	0x40000 	
#    define WGV_wbrkrighttop_MASK 	0xfff7ffff 	/* bit 19 */
#    define WGV_wbrkrighttop_disable 	0x0 		
#    define WGV_wbrkrighttop_enable 	0x80000 	

#define MGAREG_WIADDR 				0x1dc0

#    define WIA_wmode_MASK 		0xfffffffc 	/* bits 0-1 */
#    define WIA_wmode_suspend 		0x0 		/* val 0, shift 0 */
#    define WIA_wmode_resume 		0x1 		/* val 1, shift 0 */
#    define WIA_wmode_jump 		0x2 		/* val 2, shift 0 */
#    define WIA_wmode_start 		0x3 		/* val 3, shift 0 */
#    define WIA_wagp_MASK 		0xfffffffb 	/* bit 2 */
#    define WIA_wagp_pci 		0x0 		
#    define WIA_wagp_agp 		0x4 		
#    define WIA_wiaddr_MASK 		0x7 		/* bits 3-31 */
#    define WIA_wiaddr_SHIFT 		3 		

#define MGAREG_WIADDR2 			0x1dd8

#    define WIA2_wmode_MASK 		0xfffffffc 	/* bits 0-1 */
#    define WIA2_wmode_suspend 		0x0 		/* val 0, shift 0 */
#    define WIA2_wmode_resume 		0x1 		/* val 1, shift 0 */
#    define WIA2_wmode_jump 		0x2 		/* val 2, shift 0 */
#    define WIA2_wmode_start 		0x3 		/* val 3, shift 0 */
#    define WIA2_wagp_MASK 		0xfffffffb 	/* bit 2 */
#    define WIA2_wagp_pci 		0x0 		
#    define WIA2_wagp_agp 		0x4 		
#    define WIA2_wiaddr_MASK 		0x7 		/* bits 3-31 */
#    define WIA2_wiaddr_SHIFT 		3 		

#define MGAREG_WIADDRNB 			0x1e60
#define MGAREG_WIADDRNB1 			0x1e04
#define MGAREG_WIADDRNB2 			0x1e00
#define MGAREG_WIMEMADDR 			0x1e68

#    define WIMA_wimemaddr_MASK 	0xffffff00 	/* bits 0-7 */
#    define WIMA_wimemaddr_SHIFT 	0 		

#define MGAREG_WIMEMDATA 			0x2000
#define MGAREG_WIMEMDATA1 			0x2100
#define MGAREG_WMISC 				0x1e70

#    define WM_wucodecache_MASK 	0xfffffffe 	/* bit 0 */
#    define WM_wucodecache_disable 	0x0 		
#    define WM_wucodecache_enable 	0x1 		
#    define WM_wmaster_MASK 		0xfffffffd 	/* bit 1 */
#    define WM_wmaster_disable 		0x0 		
#    define WM_wmaster_enable 		0x2 		
#    define WM_wcacheflush_MASK 	0xfffffff7 	/* bit 3 */
#    define WM_wcacheflush_disable 	0x0 		
#    define WM_wcacheflush_enable 	0x8 		

#define MGAREG_WR 				0x2d00
#define MGAREG_WVRTXSZ 			0x1dcc

#    define WVS_wvrtxsz_MASK 		0xffffffc0 	/* bits 0-5 */
#    define WVS_wvrtxsz_SHIFT 		0 		
#    define WVS_primsz_MASK 		0xffffc0ff 	/* bits 8-13 */
#    define WVS_primsz_SHIFT 		8 		

#define MGAREG_XDST 				0x1cb0
#define MGAREG_XYEND 				0x1c44

#    define XYEA_x_end_MASK 		0xffff0000 	/* bits 0-15 */
#    define XYEA_x_end_SHIFT 		0 		
#    define XYEA_y_end_MASK 		0xffff 		/* bits 16-31 */
#    define XYEA_y_end_SHIFT 		16 		

#define MGAREG_XYSTRT 				0x1c40

#    define XYSA_x_start_MASK 		0xffff0000 	/* bits 0-15 */
#    define XYSA_x_start_SHIFT 		0 		
#    define XYSA_y_start_MASK 		0xffff 		/* bits 16-31 */
#    define XYSA_y_start_SHIFT 		16 		

#define MGAREG_YBOT 				0x1c9c
#define MGAREG_YDST 				0x1c90

#    define YA_ydst_MASK 		0xff800000 	/* bits 0-22 */
#    define YA_ydst_SHIFT 		0 		
#    define YA_sellin_MASK 		0x1fffffff 	/* bits 29-31 */
#    define YA_sellin_SHIFT 		29 		

#define MGAREG_YDSTLEN 			0x1c88

#    define YDL_length_MASK 		0xffff0000 	/* bits 0-15 */
#    define YDL_length_SHIFT 		0 		
#    define YDL_yval_MASK 		0xffff 		/* bits 16-31 */
#    define YDL_yval_SHIFT 		16 		

#define MGAREG_YDSTORG 			0x1c94
#define MGAREG_YTOP 				0x1c98
#define MGAREG_ZORG 				0x1c0c

#    define ZO_zorgmap_MASK 		0xfffffffe 	/* bit 0 */
#    define ZO_zorgmap_fb 		0x0 		
#    define ZO_zorgmap_sys 		0x1 		
#    define ZO_zorgacc_MASK 		0xfffffffd 	/* bit 1 */
#    define ZO_zorgacc_pci 		0x0 		
#    define ZO_zorgacc_agp 		0x2 		
#    define ZO_zorg_MASK 		0x3 		/* bits 2-31 */
#    define ZO_zorg_SHIFT 		2 		




/**************** (END) AUTOMATICLY GENERATED REGISTER FILE ******************/

/* Copied from mga_drv.h kernel file.
 */

#define MGA_ILOAD_ALIGN		64
#define MGA_ILOAD_MASK		(MGA_ILOAD_ALIGN - 1)

#endif 	/* _MGAREGS_H_ */

