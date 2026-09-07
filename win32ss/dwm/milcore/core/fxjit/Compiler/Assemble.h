// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class CAssembleContext and its derivatives.
//
//-----------------------------------------------------------------------------
#pragma once

class COperator;
class CVarState;
class CMapper;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAssembleContext
//
//  Synopsis:
//      Hooks program context to low level code generator.
//      Serves as an argument of COperator::Assemble() so that
//      the latter can access current operator and CCoder86.
//
//      CAssembleContext is abstract: virtual Emit() members
//      declared in CCoder86 are not defined. They are defined
//      in two CAssembleContext derivatives, CAssemblePass1 and
//      CAssemblePass2. Pass 1 is idle: Emit() calls do not
//      store the code but during this pass we can accumulate
//      labes offsets and get to know final size of binary code.
//      Pass 2 executes real job.
//
//      Usage pattern:
//          See CProgram::Assemble()
//
//-----------------------------------------------------------------------------
class CAssembleContext : public CCoder86
{
public:
    CAssembleContext(CMapper const & mapper, bool fUseNegativeStackOffsets);
    void AssemblePrologue(
        __in UINT32 uFrameSize,
        __in UINT32 uFrameAlignment
        );

#if DBG_DUMP
    void AssembleProgram(CProgram * pProgram, bool fDumpEnabled);
#else
    void AssembleProgram(CProgram * pProgram);
#endif //DBG_DUMP

    UINT32 GetOperatorFlags() const
    {
        return m_uOperatorFlags;
    }

    memptr FramePtr(UINT32 nDisplacement) const
    {
        return memptr(gsp, (INT32)(nDisplacement - m_uEspOffset));
    }

    virtual void* Place(void* pData, UINT32 dataType) = 0;

    UINT32 GetOffset(UINT32 uVarID) const;

    UINT32 GetEspOffset() const { return m_uEspOffset; }

public:
    // Offset from ebp to 1st argument, see AssemblePrologue().
#if WPFGFX_FXJIT_X86
    // 8 = 4 bytes for saved ebp + 4 bytes for ret adds.
    static const int sc_uArgOffset = 8;
#else //_AMD64_
    // 16 = 8 bytes for saved rbp + 8 bytes for ret adds.
    static const int sc_uArgOffset = 16;
#endif

    CMapper const & m_mapper;

protected:
    // Offset from stack frame bottom to the position pointed by esp/rsp register.
    // This offset is used to reduce code size by involving negative displacement values.
    UINT32 m_uEspOffset;

private:
    COperator *m_pCurrentOperator;
    UINT32 m_uOperatorFlags;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAssemblePass1
//
//  Synopsis:
//      Idle implementation of CAssembleContext.
//      Contains Emit() functions that do not store data
//      but calculate amount of storage needed for binary code.
//
//-----------------------------------------------------------------------------
class CAssemblePass1 : public CAssembleContext
{
public:
    CAssemblePass1(CMapper const & mapper, bool fUseNegativeStackOffsets) : CAssembleContext(mapper, fUseNegativeStackOffsets) {}

    void Emit(UINT32 /*data*/) {m_uCount++;}
    void Emit4(UINT32 /*data*/) {m_uCount += 4;}
    void EmitOpcode(UINT32 opcode)
    {
        UINT32 uDelta = (opcode & OpcSize) >> OpcShiftSize;
#if WPFGFX_FXJIT_X86
#else // _AMD64_
        if (opcode & OpcREX)
        {
            uDelta++;
        }
#endif
        m_uCount += uDelta;
    }
    UINT_PTR GetBase() const {return 0;}
    void* Place(void* pData, UINT32 /*dataType*/) {return pData;}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAssemblePass2
//
//  Synopsis:
//      Final pass implementation of CAssembleContext.
//      Contains Emit() functions that store binary code into the
//      given memory.
//
//-----------------------------------------------------------------------------
class CAssemblePass2 : public CAssembleContext
{
public:
    CAssemblePass2(
        CMapper const & mapper,
        bool fUseNegativeStackOffsets,
        UINT8 * pData,
        INT_PTR uStatic4Offset,
        INT_PTR uStatic8Offset,
        INT_PTR uStatic16Offset
        );
    void Emit(UINT32 data) {m_pData[m_uCount++] = static_cast<UINT8>(data);}
    void Emit4(UINT32 data)
    {
        m_pData[m_uCount    ] = static_cast<UINT8>(data      );
        m_pData[m_uCount + 1] = static_cast<UINT8>(data >>  8);
        m_pData[m_uCount + 2] = static_cast<UINT8>(data >> 16);
        m_pData[m_uCount + 3] = static_cast<UINT8>(data >> 24);
        m_uCount += 4;
    }

    void EmitOpcode(UINT32 opcode)
    {
#if DBG
        UINT32 uNextCount = m_uCount + ((opcode & OpcSize) >> OpcShiftSize);
#endif
        C_ASSERT(Prefix_None == 0 && Prefix_F20F == 1 && Prefix_F30F == 2 && Prefix_660F == 3);
        static const UINT32 prefixes[4] = { 0, 0xF20F, 0xF30F, 0x660F };
        UINT32 prefix = prefixes[(opcode & OpcPrefix) >> OpcShiftPrefix];
        if (prefix)
        {
            m_pData[m_uCount++] = static_cast<UINT8>(prefix >> 8);
        }

#if WPFGFX_FXJIT_X86
#else // _AMD64_
        if (opcode & OpcREX)
        {
#if DBG
            uNextCount++;
#endif
            m_pData[m_uCount++] = static_cast<UINT8>( ( (opcode & OpcREX) >> OpcShiftREX) | 0x40);
        }
#endif
        if (prefix)
        {
            m_pData[m_uCount++] = static_cast<UINT8>(prefix);
        }

        if (opcode & OpcIsLong)
        {
            m_pData[m_uCount++] = static_cast<UINT8>( (opcode & OpcByte1) >> OpcShiftByte1);
        }

        m_pData[m_uCount++] = static_cast<UINT8>( (opcode & OpcByte2) >> OpcShiftByte2);

        WarpAssert(m_uCount == uNextCount);
    }

    UINT_PTR GetBase() const
    {
        return reinterpret_cast<UINT_PTR>(m_pData);
    }

    void* Place(void* pData, UINT32 dataType);

private:
    UINT8* const m_pData;
    INT_PTR const m_uStatic4Offset;
    INT_PTR const m_uStatic8Offset;
    INT_PTR const m_uStatic16Offset;
};

