// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Permutation of variables across registers and memory.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShuffleRecord::Assemble
//
//  Synopsis:
//      Generate mov instruction.
//
//------------------------------------------------------------------------------
void
CShuffleRecord::Assemble(CAssembleContext & actx, CMapper const & mapper)
{
    if (!m_regSrc.IsDefined())
    {
        WarpAssert(m_regDst.IsDefined());

        UINT32 uOffset = mapper.GetVarOffset(m_uVarID);

        switch(m_vt)
        {
        case vtPointer: actx.cmd(mov_ptr_rm , m_regDst, actx.FramePtr(uOffset)); break;
        case vtUINT32:  actx.cmd(mov_rm     , m_regDst, actx.FramePtr(uOffset)); break;
#if WPFGFX_FXJIT_X86
        case vtMm:      actx.cmd(movq_mmx_rm, m_regDst, actx.FramePtr(uOffset)); break;
#else
        case vtUINT64:  actx.cmd(mov_64_rm  , m_regDst, actx.FramePtr(uOffset)); break;
#endif
        case vtXmm:     actx.cmd(movdqa_rm  , m_regDst, actx.FramePtr(uOffset)); break;
        case vtXmmF1:   actx.cmd(movss_rm   , m_regDst, actx.FramePtr(uOffset)); break;
        case vtXmmF4:   actx.cmd(movaps_rm  , m_regDst, actx.FramePtr(uOffset)); break;
        default: NO_DEFAULT;
        }
    }
    else if (!m_regDst.IsDefined())
    {
        WarpAssert(m_regSrc.IsDefined());

        UINT32 uOffset = mapper.GetVarOffset(m_uVarID);

        switch(m_vt)
        {
        case vtPointer: actx.cmd(mov_ptr_mr , actx.FramePtr(uOffset), m_regSrc); break;
        case vtUINT32:  actx.cmd(mov_mr     , actx.FramePtr(uOffset), m_regSrc); break;
#if WPFGFX_FXJIT_X86
        case vtMm:      actx.cmd(movq_mmx_mr, actx.FramePtr(uOffset), m_regSrc); break;
#else
        case vtUINT64:  actx.cmd(mov_64_mr  , actx.FramePtr(uOffset), m_regSrc); break;
#endif
        case vtXmm:     actx.cmd(movdqa_mr  , actx.FramePtr(uOffset), m_regSrc); break;
        case vtXmmF1:   actx.cmd(movss_mr   , actx.FramePtr(uOffset), m_regSrc); break;
        case vtXmmF4:   actx.cmd(movaps_mr  , actx.FramePtr(uOffset), m_regSrc); break;
        default: NO_DEFAULT;
        }
    }
    else
    {
        WarpAssert(m_regSrc.GetRegType() == m_regDst.GetRegType());

        switch(m_vt)
        {
        case vtPointer: actx.cmd(mov_ptr_rr , m_regDst, m_regSrc); break;
        case vtUINT32:  actx.cmd(mov_rr     , m_regDst, m_regSrc); break;
#if WPFGFX_FXJIT_X86
        case vtMm:      actx.cmd(movq_mmx_rr, m_regDst, m_regSrc); break;
#else
        case vtUINT64:  actx.cmd(mov_64_rr  , m_regDst, m_regSrc); break;
#endif
        case vtXmm:     actx.cmd(movdqa_rr  , m_regDst, m_regSrc); break;
        case vtXmmF1:   actx.cmd(movss_rr   , m_regDst, m_regSrc); break;
        case vtXmmF4:   actx.cmd(movaps_rr  , m_regDst, m_regSrc); break;
        default: NO_DEFAULT;
        }
    }
}

