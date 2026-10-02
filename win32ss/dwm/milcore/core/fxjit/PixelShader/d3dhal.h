// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



#define D3DSI_GETREGNUM(token)  (token & D3DSP_REGNUM_MASK)
#define D3DSI_GETREGTYPE(token) ((D3DSHADER_PARAM_REGISTER_TYPE)(((token & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) | \
                                 ((token & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2)))
#define D3DSI_GETUSAGE(token) ((token & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT)
#define D3DSI_GETUSAGEINDEX(token) ((token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT)
#define D3DSI_GETINSTLENGTH(token) ((token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT)
#define D3DSI_GETCOMPARISON(token) ((D3DSHADER_COMPARISON)((token & D3DSHADER_COMPARISON_MASK) >> D3DSHADER_COMPARISON_SHIFT))
#define D3DSI_GETREGISTERPROPERTIES(token) (token & D3DSP_REGISTERPROPERTIES_MASK)
#define D3DSI_GETTEXTURETYPE(token) (token & D3DSP_TEXTURETYPE_MASK)
#define D3DSI_GETDSTMODIFIER(token) (token & D3DSP_DSTMOD_MASK)
#define D3DSI_GETSWIZZLECOMP(source, component)  (source >> ((component << 1) + 16) & 0x3)
#define D3DSI_GETSWIZZLE(token)  (token & D3DVS_SWIZZLE_MASK)
#define D3DSI_GETSRCMODIFIER(token) (token & D3DSP_SRCMOD_MASK)
#define D3DSI_GETADDRESSMODE(token) (token & D3DVS_ADDRESSMODE_MASK)

inline D3DSHADER_PARAM_REGISTER_TYPE D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(DWORD token)
{
    D3DSHADER_PARAM_REGISTER_TYPE RegType = D3DSI_GETREGTYPE(token);
    switch(RegType)
    {
    case D3DSPR_CONST4:
    case D3DSPR_CONST3:
    case D3DSPR_CONST2:
        return D3DSPR_CONST;
    default:
        return RegType;
    }
}

// The inline function below retrieves register number for an opcode,
// taking into account that: if the type is a
// D3DSPR_CONSTn, the register number needs to be remapped.
//
//           D3DSPR_CONST is for c0-c2047
//           D3DSPR_CONST2 is for c2048-c4095
//           D3DSPR_CONST3 is for c4096-c6143
//           D3DSPR_CONST4 is for c6144-c8191
//
// For example if the instruction token specifies type D3DSPR_CONST4, reg# 3,
// the register number retrieved is 6147.
// For other register types, the register number is just returned unchanged.
inline UINT D3DSI_GETREGNUM_RESOLVING_CONSTANTS(DWORD token)
{
    D3DSHADER_PARAM_REGISTER_TYPE RegType = D3DSI_GETREGTYPE(token);
    UINT RegNum = D3DSI_GETREGNUM(token);
    switch(RegType)
    {
    case D3DSPR_CONST4:
        return RegNum + 6144;
    case D3DSPR_CONST3:
        return RegNum + 4096;
    case D3DSPR_CONST2:
        return RegNum + 2048;
    default:
        return RegNum;
    }
}


