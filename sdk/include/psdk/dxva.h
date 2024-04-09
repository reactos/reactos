/*
 * Copyright 2015 Michael Müller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DXVA_H
#define __WINE_DXVA_H

#ifdef __cplusplus
extern "C" {
#endif

#define DXVA_USUAL_BLOCK_WIDTH   8
#define DXVA_USUAL_BLOCK_HEIGHT  8
#define DXVA_USUAL_BLOCK_SIZE   (DXVA_USUAL_BLOCK_WIDTH * DXVA_USUAL_BLOCK_HEIGHT)

#include <pshpack1.h>

typedef struct _DXVA_PictureParameters
{
    WORD wDecodedPictureIndex;
    WORD wDeblockedPictureIndex;
    WORD wForwardRefPictureIndex;
    WORD wBackwardRefPictureIndex;
    WORD wPicWidthInMBminus1;
    WORD wPicHeightInMBminus1;
    BYTE bMacroblockWidthMinus1;
    BYTE bMacroblockHeightMinus1;
    BYTE bBlockWidthMinus1;
    BYTE bBlockHeightMinus1;
    BYTE bBPPminus1;
    BYTE bPicStructure;
    BYTE bSecondField;
    BYTE bPicIntra;
    BYTE bPicBackwardPrediction;
    BYTE bBidirectionalAveragingMode;
    BYTE bMVprecisionAndChromaRelation;
    BYTE bChromaFormat;
    BYTE bPicScanFixed;
    BYTE bPicScanMethod;
    BYTE bPicReadbackRequests;
    BYTE bRcontrol;
    BYTE bPicSpatialResid8;
    BYTE bPicOverflowBlocks;
    BYTE bPicExtrapolation;
    BYTE bPicDeblocked;
    BYTE bPicDeblockConfined;
    BYTE bPic4MVallowed;
    BYTE bPicOBMC;
    BYTE bPicBinPB;
    BYTE bMV_RPS;
    BYTE bReservedBits;
    WORD wBitstreamFcodes;
    WORD wBitstreamPCEelements;
    BYTE bBitstreamConcealmentNeed;
    BYTE bBitstreamConcealmentMethod;
} DXVA_PictureParameters, *LPDXVA_PictureParameters;

typedef struct _DXVA_SliceInfo
{
    WORD wHorizontalPosition;
    WORD wVerticalPosition;
    DWORD dwSliceBitsInBuffer;
    DWORD dwSliceDataLocation;
    BYTE bStartCodeBitOffset;
    BYTE bReservedBits;
    WORD wMBbitOffset;
    WORD wNumberMBsInSlice;
    WORD wQuantizerScaleCode;
    WORD wBadSliceChopping;
} DXVA_SliceInfo, *LPDXVA_SliceInfo;

typedef struct _DXVA_QmatrixData
{
    BYTE bNewQmatrix[4];
    WORD Qmatrix[4][DXVA_USUAL_BLOCK_WIDTH * DXVA_USUAL_BLOCK_HEIGHT];
} DXVA_QmatrixData, *LPDXVA_QmatrixData;

typedef struct
{
    union
    {
        struct
        {
            UCHAR Index7Bits     : 7;
            UCHAR AssociatedFlag : 1;
        } DUMMYSTRUCTNAME;
        UCHAR bPicEntry;
    } DUMMYUNIONNAME;
} DXVA_PicEntry_H264;

typedef struct
{
    USHORT wFrameWidthInMbsMinus1;
    USHORT wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264 CurrPic;
    UCHAR num_ref_frames;
    union
    {
        struct
        {
            USHORT field_pic_flag                   : 1;
            USHORT MbaffFrameFlag                   : 1;
            USHORT residual_colour_transform_flag   : 1;
            USHORT sp_for_switch_flag               : 1;
            USHORT chroma_format_idc                : 2;
            USHORT RefPicFlag                       : 1;
            USHORT constrained_intra_pred_flag      : 1;
            USHORT weighted_pred_flag               : 1;
            USHORT weighted_bipred_idc              : 2;
            USHORT MbsConsecutiveFlag               : 1;
            USHORT frame_mbs_only_flag              : 1;
            USHORT transform_8x8_mode_flag          : 1;
            USHORT MinLumaBipredSize8x8Flag         : 1;
            USHORT IntraPicFlag                     : 1;
        } DUMMYSTRUCTNAME;
        USHORT wBitFields;
    } DUMMYUNIONNAME;
    UCHAR bit_depth_luma_minus8;
    UCHAR bit_depth_chroma_minus8;
    USHORT Reserved16Bits;
    UINT StatusReportFeedbackNumber;
    DXVA_PicEntry_H264 RefFrameList[16];
    INT CurrFieldOrderCnt[2];
    INT FieldOrderCntList[16][2];
    CHAR pic_init_qs_minus26;
    CHAR chroma_qp_index_offset;
    CHAR second_chroma_qp_index_offset;
    UCHAR ContinuationFlag;
    CHAR pic_init_qp_minus26;
    UCHAR num_ref_idx_l0_active_minus1;
    UCHAR num_ref_idx_l1_active_minus1;
    UCHAR Reserved8BitsA;
    USHORT FrameNumList[16];

    UINT UsedForReferenceFlags;
    USHORT NonExistingFrameFlags;
    USHORT frame_num;
    UCHAR log2_max_frame_num_minus4;
    UCHAR pic_order_cnt_type;
    UCHAR log2_max_pic_order_cnt_lsb_minus4;
    UCHAR delta_pic_order_always_zero_flag;
    UCHAR direct_8x8_inference_flag;
    UCHAR entropy_coding_mode_flag;
    UCHAR pic_order_present_flag;
    UCHAR num_slice_groups_minus1;
    UCHAR slice_group_map_type;
    UCHAR deblocking_filter_control_present_flag;
    UCHAR redundant_pic_cnt_present_flag;
    UCHAR Reserved8BitsB;
    USHORT slice_group_change_rate_minus1;
    UCHAR SliceGroupMap[810];
} DXVA_PicParams_H264;

typedef struct
{
    UCHAR bScalingLists4x4[6][16];
    UCHAR bScalingLists8x8[2][64];
} DXVA_Qmatrix_H264;

typedef struct
{
    UINT BSNALunitDataLocation;
    UINT SliceBytesInBuffer;
    USHORT wBadSliceChopping;
    USHORT first_mb_in_slice;
    USHORT NumMbsForSlice;
    USHORT BitOffsetToSliceData;
    UCHAR slice_type;
    UCHAR luma_log2_weight_denom;
    UCHAR chroma_log2_weight_denom;

    UCHAR num_ref_idx_l0_active_minus1;
    UCHAR num_ref_idx_l1_active_minus1;
    CHAR slice_alpha_c0_offset_div2;
    CHAR slice_beta_offset_div2;
    UCHAR Reserved8Bits;
    DXVA_PicEntry_H264 RefPicList[2][32];
    SHORT Weights[2][32][3][2];
    CHAR slice_qs_delta;
    CHAR slice_qp_delta;
    UCHAR redundant_pic_cnt;
    UCHAR direct_spatial_mv_pred_flag;
    UCHAR cabac_init_idc;
    UCHAR disable_deblocking_filter_idc;
    USHORT slice_id;
} DXVA_Slice_H264_Long;

typedef struct
{
    UINT BSNALunitDataLocation;
    UINT SliceBytesInBuffer;
    USHORT wBadSliceChopping;
} DXVA_Slice_H264_Short;

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif /* __WINE_DXVA_H */
