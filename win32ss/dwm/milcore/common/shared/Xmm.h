// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Classes to execute fast calculations on integer data
//      using SSE2 instruction set extention.
//
//-----------------------------------------------------------------------------

#pragma once 

#if defined(_X86_)
#define _BUILD_SSE_
#elif defined(_AMD64_)
#define _BUILD_SSE_
#endif

#if defined(_BUILD_SSE_)

class CXmmValue;
class CXmmBytes;
class CXmmWords;
class CXmmDWords;
class CXmmQWords;
class CXmmFloat;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmValue
//
//  Synopsis:
//      Holds 128-bit value that can reside either in memory or in XMM register.
//      Provides vector operations that treat it as an array of bytes, words,
//      double- or octal-words.
//
//      Implementation is based on Microsoft (R) C/C++ Optimizing
//      Compiler's intrinsic functions defined in header file
//      <emmintrin.h> (Intel Corporation)
//
//------------------------------------------------------------------------------
class CXmmValue
{
public:

    //
    // Construction
    //

    CXmmValue(){}
    CXmmValue(__in_ecount(1) CXmmValue const & source)
    {
        m_data = source.m_data;
    }

    //
    // Casting
    //

    __out_ecount(1) operator __m128i&() {return m_data;}
    __out_ecount(1) CXmmValue& operator=(__m128i data) {m_data = data; return *this;}

    __out_ecount(1) operator __m128&() {return m_dataR;}
    __out_ecount(1) CXmmValue& operator=(__m128 data) {m_dataR = data; return *this;}

    __out_ecount(1) operator CXmmBytes&() {return *reinterpret_cast<CXmmBytes*>(this);}
    __out_ecount(1) operator CXmmWords&() {return *reinterpret_cast<CXmmWords*>(this);}
    __out_ecount(1) operator CXmmDWords&() {return *reinterpret_cast<CXmmDWords*>(this);}
    __out_ecount(1) operator CXmmQWords&() {return *reinterpret_cast<CXmmQWords*>(this);}
    __out_ecount(1) operator CXmmFloat&() {return *reinterpret_cast<CXmmFloat*>(this);}

    __out_ecount(1) CXmmBytes& AsBytes() {return *reinterpret_cast<CXmmBytes*>(this);}
    __out_ecount(1) CXmmWords& AsWords() {return *reinterpret_cast<CXmmWords*>(this);}
    __out_ecount(1) CXmmDWords& AsDWords() {return *reinterpret_cast<CXmmDWords*>(this);}
    __out_ecount(1) CXmmQWords& AsQWords() {return *reinterpret_cast<CXmmQWords*>(this);}
    __out_ecount(1) CXmmFloat& AsFloat() {return *reinterpret_cast<CXmmFloat*>(this);}

    //
    // Loads and Stores
    //

    __out_ecount(1) CXmmValue& LoadDWord(__int32 data);
    __out_ecount(1) CXmmValue& Load2DWords(__int32 data1, __int32 data0);
    __out_ecount(1) CXmmValue& Load4DWords(__int32 data3, __int32 data2, __int32 data1, __int32 data0);
    __out_ecount(1) CXmmValue& LoadQWord(__in_ecount(1) __int64 const *pData);
    __out_ecount(1) CXmmValue& StoreQWord(__out_ecount(1) __int64 *pData);

    //
    // Reordering, Shuffling, Packing and Unpacking
    //

    __out_ecount(1) CXmmValue& LoadLowQWords(__in_ecount(1) CXmmValue const &source1, __in_ecount(1) CXmmValue const &source0);
    __out_ecount(1) CXmmValue& LoadHighQWords(__in_ecount(1) CXmmValue const &source1, __in_ecount(1) CXmmValue const &source0);

    __out_ecount(1) CXmmValue& UnpackBytesToWords();
    __out_ecount(1) CXmmValue& PackWordsToBytes();
    template<int idx3, int idx2, int idx1, int idx0> __out_ecount(1) CXmmValue& ShuffleLowWords();
    template<int idx3, int idx2, int idx1, int idx0> __out_ecount(1) CXmmValue& ShuffleHighWords();
    template<int idx> __out_ecount(1) CXmmValue& ReplicateWord4Times();
    template<int idx> __out_ecount(1) CXmmValue& ReplicateWord8Times();

    CXmmValue GetHighQWord() const;

    __out_ecount(1) CXmmValue& DuplicateLowQWord();
    __out_ecount(1) CXmmValue& DuplicateHighQWord();

    __int32 GetLowDWord() const;

    //
    // Arithmetic
    //

    __out_ecount(1) CXmmValue& AddBytes(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& AddBytesSignedSaturate(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& AddBytesUnsignedSaturate(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& AddWords(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& AddWordsSignedSaturate(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& AddWordsUnsignedSaturate(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& AddDWords(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& AddQWords(__in_ecount(1) CXmmValue const &that);


    __out_ecount(1) CXmmValue& SubBytes(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& SubBytesSignedSaturate(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& SubBytesUnsignedSaturate(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& SubWords(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& SubWordsSignedSaturate(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& SubWordsUnsignedSaturate(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& SubDWords(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& SubQWords(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& MulWords(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& MulWordsSignedHigh(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& MulWordsUnsignedHigh(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& MulDWords(__in_ecount(1) CXmmValue const &that);

    __out_ecount(1) CXmmValue& ShiftWordsLeft(int shift);
    __out_ecount(1) CXmmValue& ShiftWordsRightLogical(int shift);
    __out_ecount(1) CXmmValue& ShiftWordsRightArithmetic(int shift);

    __out_ecount(1) CXmmValue& ShiftDWordsLeft(int shift);
    __out_ecount(1) CXmmValue& ShiftDWordsRightLogical(int shift);
    __out_ecount(1) CXmmValue& ShiftDWordsRightArithmetic(int shift);

    __out_ecount(1) CXmmValue& ShiftQWordsLeft(int shift);
    __out_ecount(1) CXmmValue& ShiftQWordsRightLogical(int shift);

    __out_ecount(1) CXmmValue& MaxSignedWords(__in_ecount(1) CXmmValue const &that);
    __out_ecount(1) CXmmValue& MinSignedWords(__in_ecount(1) CXmmValue const &that);


    //
    // Others
    //

    static CXmmValue Zero();

protected:
    union
    {
        __m128i m_data;
        unsigned __int64 m_qwords[2];
        unsigned __int32 m_dwords[4];
        unsigned __int16 m_words[8];
        unsigned __int8 m_bytes[16];

        __m128 m_dataR;
        float m_floats[4];
    };

private:
    struct __declspec(align(16)) QWords
    {
        __int64 qwords[2];
    };

    static const QWords sc_Zero;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmBytes
//
//  Synopsis:
//      Treats 128-bit CXmmValue as an array of 8 16-bit unsigned integer values.
//      Provides vector operations.
//
//------------------------------------------------------------------------------
class CXmmBytes : public CXmmValue
{
public:
    CXmmBytes operator+(__in_ecount(1) CXmmBytes const & other) const;
    __out_ecount(1) CXmmBytes& operator+=(__in_ecount(1) CXmmBytes const & other);

    CXmmBytes operator-(__in_ecount(1) CXmmBytes const & other) const;
    __out_ecount(1) CXmmBytes& operator-=(__in_ecount(1) CXmmBytes const & other);
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmWords
//
//  Synopsis:
//      Treats 128-bit CXmmValue as an array of 8 16-bit unsigned integer values.
//      Provides vector operations.
//
//------------------------------------------------------------------------------
class CXmmWords : public CXmmValue
{
public:
    CXmmWords operator+(__in_ecount(1) CXmmWords const & other) const;
    __out_ecount(1) CXmmWords& operator+=(__in_ecount(1) CXmmWords const & other);

    CXmmWords operator-(__in_ecount(1) CXmmWords const & other) const;
    __out_ecount(1) CXmmWords& operator-=(__in_ecount(1) CXmmWords const & other);

    CXmmWords operator*(__in_ecount(1) CXmmWords const & other) const;
    __out_ecount(1) CXmmWords& operator*=(__in_ecount(1) CXmmWords const & other);

    CXmmWords operator<<(int shift) const;
    __out_ecount(1) CXmmWords& operator<<=(int shift);

    CXmmWords operator>>(int shift) const;
    __out_ecount(1) CXmmWords& operator>>=(int shift);

    __out_ecount(1) CXmmWords& Max(__in_ecount(1) CXmmWords const & that);
    __out_ecount(1) CXmmWords& Min(__in_ecount(1) CXmmWords const & that);

    static CXmmWords Half8dot8();

private:
    struct __declspec(align(16)) Words
    {
        __int16 words[8];
    };

    static const Words sc_Half8dot8;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmDWords
//
//  Synopsis:
//      Treats 128-bit CXmmValue as an array of 4 32-bit unsigned integer values.
//      Provides vector operations.
//
//------------------------------------------------------------------------------
class CXmmDWords : public CXmmValue
{
public:
    CXmmDWords operator+(__in_ecount(1) CXmmDWords const & other) const;
    __out_ecount(1) CXmmDWords& operator+=(__in_ecount(1) CXmmDWords const & other);

    CXmmDWords operator-(__in_ecount(1) CXmmDWords const & other) const;
    __out_ecount(1) CXmmDWords& operator-=(__in_ecount(1) CXmmDWords const & other);

    CXmmDWords operator*(__in_ecount(1) CXmmDWords const & other) const;
    __out_ecount(1) CXmmDWords& operator*=(__in_ecount(1) CXmmDWords const & other);

    CXmmDWords operator<<(int shift) const;
    __out_ecount(1) CXmmDWords& operator<<=(int shift);

    CXmmDWords operator>>(int shift) const;
    __out_ecount(1) CXmmDWords& operator>>=(int shift);

};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmQWords
//
//  Synopsis:
//      Treats 128-bit CXmmValue as an array of 2 64-bit unsigned integer values.
//      Provides vector operations.
//
//------------------------------------------------------------------------------
class CXmmQWords : public CXmmValue
{
public:
    CXmmQWords operator<<(int shift) const;
    __out_ecount(1) CXmmQWords& operator<<=(int shift);

    CXmmQWords operator>>(int shift) const;
    __out_ecount(1) CXmmQWords& operator>>=(int shift);

};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CXmmFloat
//
//  Synopsis:
//      Treats 128-bit CXmmValue as single 32-bits floating point value.
//
//------------------------------------------------------------------------------
class CXmmFloat : public CXmmValue
{
public:

    //
    // Construction
    //
    MIL_FORCEINLINE CXmmFloat() {}
    MIL_FORCEINLINE CXmmFloat(__in_ecount(1) CXmmFloat const & other);
    MIL_FORCEINLINE CXmmFloat(__in_ecount(1) __m128 const& data);
    MIL_FORCEINLINE CXmmFloat(float data);
    MIL_FORCEINLINE CXmmFloat(int data);

    //
    // Conversions
    //
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator=(float data);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator=(int data);
    MIL_FORCEINLINE int Round() const;
    MIL_FORCEINLINE int Trunc() const;

    //
    // Calculations
    //
    MIL_FORCEINLINE CXmmFloat operator+(__in_ecount(1) CXmmFloat const & other) const;
    MIL_FORCEINLINE CXmmFloat operator+(float data) const;
    MIL_FORCEINLINE CXmmFloat operator+(int data) const;
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator+=(__in_ecount(1) CXmmFloat const & other);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator+=(float data);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator+=(int data);

    MIL_FORCEINLINE CXmmFloat operator-(__in_ecount(1) CXmmFloat const & other) const;
    MIL_FORCEINLINE CXmmFloat operator-(float data) const;
    MIL_FORCEINLINE CXmmFloat operator-(int data) const;
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator-=(__in_ecount(1) CXmmFloat const & other);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator-=(float data);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator-=(int data);

    MIL_FORCEINLINE CXmmFloat operator*(__in_ecount(1) CXmmFloat const & other) const;
    MIL_FORCEINLINE CXmmFloat operator*(float data) const;
    MIL_FORCEINLINE CXmmFloat operator*(int data) const;
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator*=(__in_ecount(1) CXmmFloat const & other);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator*=(float data);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator*=(int data);

    MIL_FORCEINLINE CXmmFloat operator/(__in_ecount(1) CXmmFloat const & other) const;
    MIL_FORCEINLINE CXmmFloat operator/(float data) const;
    MIL_FORCEINLINE CXmmFloat operator/(int data) const;
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator/=(__in_ecount(1) CXmmFloat const & other);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator/=(float data);
    MIL_FORCEINLINE __out_ecount(1) CXmmFloat& operator/=(int data);

    MIL_FORCEINLINE static CXmmFloat Reciprocal(__in_ecount(1) CXmmFloat const & data);
    MIL_FORCEINLINE static CXmmFloat Sqrt(__in_ecount(1) CXmmFloat const & data);
    MIL_FORCEINLINE static CXmmFloat Min(__in_ecount(1) CXmmFloat const & data1, __in_ecount(1) CXmmFloat const & data2);
    MIL_FORCEINLINE static CXmmFloat Max(__in_ecount(1) CXmmFloat const & data1, __in_ecount(1) CXmmFloat const & data2);
};

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::LoadDWord
//
//  Synopsis:
//      Load given 32-bit value to the low bits of m_data.
//      Fill remaining bits with zeros.
//
//  Operation:
//      m_dwords[0] = data;
//      m_dwords[1] = 
//      m_dwords[2] = 
//      m_dwords[3] = 0;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::LoadDWord(__int32 data)
{
    m_data = _mm_cvtsi32_si128(data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::Load2DWords
//
//  Synopsis:
//      Load given 32-bit values into low data.
//      Zero high data.
//
//  Operation:
//      m_dwords[0] = data0;
//      m_dwords[1] = data1;
//      m_dwords[2] = 0;
//      m_dwords[3] = 0;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::Load2DWords(__int32 data1, __int32 data0)
{
    CXmmValue high;

    LoadDWord(data0);
    high.LoadDWord(data1);

    m_data = _mm_unpacklo_epi32(m_data, high.m_data);

    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::Load4DWords
//
//  Synopsis:
//      Load given 32-bit values.
//
//  Operation:
//      m_dwords[0] = data0;
//      m_dwords[1] = data1;
//      m_dwords[2] = data2;
//      m_dwords[3] = data3;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::Load4DWords(__int32 data3, __int32 data2, __int32 data1, __int32 data0)
{
    m_data = _mm_set_epi32(data3, data2, data1, data0);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::LoadQWord
//
//  Synopsis:
//      Load given 64-bit value to the low bits of m_data.
//      Fill remaining bits with zeros.
//
//  Operation:
//      m_qwords[0] = data;
//      m_dwords[1] = 0;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::LoadQWord(__in_ecount(1) __int64 const *pData)
{
    m_data = _mm_loadl_epi64(reinterpret_cast<__m128i*>(const_cast<__int64*>(pData)));
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::StoreQWord
//
//  Synopsis:
//      Store low 64-bit value to memory.
//
//  Operation:
//      *pData = m_qwords[0];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::StoreQWord(__out_ecount(1) __int64 *pData)
{
    _mm_storel_epi64(reinterpret_cast<__m128i*>(pData), m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::LoadLowQWords
//
//  Synopsis:
//      Compose data from low 64-bit values of two given operands.
//
//  Operation:
//      m_qwords[0] = source0.m_qwords[0];
//      m_dwords[1] = source1.m_qwords[0];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::LoadLowQWords(__in_ecount(1) CXmmValue const &source1, __in_ecount(1) CXmmValue const &source0)
{
    m_data = _mm_unpacklo_epi64(source0.m_data, source1.m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::LoadHighQWords
//
//  Synopsis:
//      Compose data from high 64-bit values of two given operands.
//
//  Operation:
//      m_qwords[0] = source0.m_qwords[1];
//      m_dwords[1] = source1.m_qwords[1];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::LoadHighQWords(__in_ecount(1) CXmmValue const &source1, __in_ecount(1) CXmmValue const &source0)
{
    m_data = _mm_unpackhi_epi64(source0.m_data, source1.m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::UnpackBytesToWords
//
//  Synopsis:
//      Expand low 8 bytes to words, filling high bits with zeros.
//
//  Operation:
//      m_words[0] = m_bytes[0];
//      m_words[1] = m_bytes[1];
//      m_words[2] = m_bytes[2];
//      m_words[3] = m_bytes[3];
//      m_words[4] = m_bytes[4];
//      m_words[5] = m_bytes[5];
//      m_words[6] = m_bytes[6];
//      m_words[7] = m_bytes[7];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::UnpackBytesToWords()
{
    m_data = _mm_unpacklo_epi8(m_data, Zero().m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::PackWordsToBytes
//
//  Synopsis:
//      Pack all the 8 words to low 8 bytes.
//
//  Operation:
//      m_bytes[0] = m_bytes[0] = SaturateSignedWordToUnsignedByte(m_words[0]);
//      m_bytes[1] = m_bytes[1] = SaturateSignedWordToUnsignedByte(m_words[1]);
//      m_bytes[2] = m_bytes[2] = SaturateSignedWordToUnsignedByte(m_words[2]);
//      m_bytes[3] = m_bytes[3] = SaturateSignedWordToUnsignedByte(m_words[3]);
//      m_bytes[4] = m_bytes[4] = SaturateSignedWordToUnsignedByte(m_words[4]);
//      m_bytes[5] = m_bytes[5] = SaturateSignedWordToUnsignedByte(m_words[5]);
//      m_bytes[6] = m_bytes[6] = SaturateSignedWordToUnsignedByte(m_words[6]);
//      m_bytes[7] = m_bytes[7] = SaturateSignedWordToUnsignedByte(m_words[7]);
//
//      where:
//      unsigned __int8 SaturateSignedWordToUnsignedByte(__int16 x)
//      {
//          if (x < 0) return 0;
//          if (x > 255) return 255;
//          return x;
//      }
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::PackWordsToBytes()
{
    m_data = _mm_packus_epi16(m_data, m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShuffleLowWords
//
//  Synopsis:
//      Reorder 4 low words.
//
//  Operation:
//      __int16 word0 = m_words[idx0];
//      __int16 word1 = m_words[idx1];
//      __int16 word2 = m_words[idx2];
//      __int16 word3 = m_words[idx3];
//      m_words[0] = word0;
//      m_words[1] = word1;
//      m_words[2] = word2;
//      m_words[3] = word3;
//      m_words[4] - unchanged
//      m_words[5] - unchanged
//      m_words[6] - unchanged
//      m_words[7] - unchanged
//
//------------------------------------------------------------------------------
template<int idx3, int idx2, int idx1, int idx0>
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShuffleLowWords()
{
    C_ASSERT(idx0 >= 0 && idx0 <= 3);
    C_ASSERT(idx1 >= 0 && idx1 <= 3);
    C_ASSERT(idx2 >= 0 && idx2 <= 3);
    C_ASSERT(idx3 >= 0 && idx3 <= 3);

    m_data = _mm_shufflelo_epi16(m_data, idx3 * 64 + idx2 * 16 + idx1 * 4 + idx0);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShuffleHighWords
//
//  Synopsis:
//      Reorder 4 high words.
//
//  Operation:
//      __int16 word4 = m_words[4 + idx0];
//      __int16 word5 = m_words[4 + idx1];
//      __int16 word6 = m_words[4 + idx2];
//      __int16 word7 = m_words[4 + idx3];
//      m_words[0] - unchanged
//      m_words[1] - unchanged
//      m_words[2] - unchanged
//      m_words[3] - unchanged
//      m_words[4] = word4;
//      m_words[5] = word5;
//      m_words[6] = word6;
//      m_words[7] = word7;
//
//------------------------------------------------------------------------------
template<int idx3, int idx2, int idx1, int idx0>
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShuffleHighWords()
{
    C_ASSERT(idx0 >= 0 && idx0 <= 3);
    C_ASSERT(idx1 >= 0 && idx1 <= 3);
    C_ASSERT(idx2 >= 0 && idx2 <= 3);
    C_ASSERT(idx3 >= 0 && idx3 <= 3);

    m_data = _mm_shufflehi_epi16(m_data, idx3 * 64 + idx2 * 16 + idx1 * 4 + idx0);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ReplicateWord4Times
//
//  Synopsis:
//      Copy one of the words to either low 4 words or high 4 words.
//
//  Operation:
//      __int16 word = m_words[idx];
//      if (idx < 4)
//      {
//          m_words[0] = m_words[1] = m_words[2] = m_words[3] = word;
//      }
//      else
//      {
//          m_words[4] = m_words[5] = m_words[6] = m_words[7] = word;
//      }
//      Remaining words are unchanged.
//
//------------------------------------------------------------------------------
template<int idx>
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ReplicateWord4Times()
{
    C_ASSERT(idx >= 0 && idx <= 7);
    if (idx < 4)
    {
        return ShuffleLowWords<idx&3, idx&3, idx&3, idx&3>();
    }
    else
    {
        return ShuffleHighWords<idx&3, idx&3, idx&3, idx&3>();
    }
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ReplicateWord8Times
//
//  Synopsis:
//      Copy one of the words to all 8 of them.
//
//  Operation:
//      __int16 word = m_words[idx];
//      m_words[0] =
//      m_words[1] =
//      m_words[2] =
//      m_words[3] =
//      m_words[4] =
//      m_words[5] =
//      m_words[6] =
//      m_words[7] = word;
//
//------------------------------------------------------------------------------
template<int idx>
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ReplicateWord8Times()
{
    ReplicateWord4Times<idx>();
    if (idx < 4)
    {
        m_data = _mm_unpacklo_epi64(m_data, m_data);
    }
    else
    {
        m_data = _mm_unpackhi_epi64(m_data, m_data);
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::GetHighQWord
//
//  Synopsis:
//      Move 64 high bits to low 64 bits;
//      zero 64 high words
//
//  Operation:
//      m_qwords[0] = m_qwords[1];
//      m_qwords[1] = 0;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmValue
CXmmValue::GetHighQWord() const
{
    CXmmValue result;
    result.m_data = _mm_srli_si128(m_data, 8);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::DuplicateLowQWord
//
//  Synopsis:
//      Copy low 64 bits to high 64 bits.
//
//  Operation:
//      m_qwords[1] = m_qwords[0];
//      m_qwords[0] - unchanged
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::DuplicateLowQWord()
{
    m_data = _mm_unpacklo_epi64(m_data, m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::DuplicateHighQWord
//
//  Synopsis:
//      Copy high 64 bits to low 64 bits.
//
//  Operation:
//      m_qwords[0] = m_qwords[1];
//      m_qwords[1] - unchanged
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::DuplicateHighQWord()
{
    m_data = _mm_unpackhi_epi64(m_data, m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddBytes
//
//  Synopsis:
//      Add bytes in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          m_bytes[i] += that.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddBytes(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_add_epi8(m_data, that.m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddBytesSignedSaturate
//
//  Synopsis:
//      Add bytes with signed saturation.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          int a = static_cast<__int8>(     m_bytes[i]);
//          int b = static_cast<__int8>(that.m_bytes[i]);
//          int s = a + b;
//
//          __int8 min = 0x80;
//          __int8 max = 0x7F;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_bytes[i] = static_cast<__int8>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddBytesSignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_adds_epi8(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddBytesUnsignedSaturate
//
//  Synopsis:
//      Add bytes with unsigned saturation.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          int a =      m_bytes[i];
//          int b = that.m_bytes[i];
//          int s = a + b;
//
//          unsinged __int8 min = 0x00;
//          unsinged __int8 max = 0xFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_bytes[i] = static_cast<unsigned __int8>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddBytesUnsignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_adds_epu8(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddWords
//
//  Synopsis:
//      Add words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] += that.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_add_epi16(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddWordsSignedSaturate
//
//  Synopsis:
//      Add words with signed saturation.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a = static_cast<__int16>(     m_words[i]);
//          int b = static_cast<__int16>(that.m_words[i]);
//          int s = a + b;
//
//          __int16 min = 0x8000;
//          __int16 max = 0x7FFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_words[i] = static_cast<__int16>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddWordsSignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_adds_epi16(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddWordsUnsignedSaturate
//
//  Synopsis:
//      Add words with unsigned saturation.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a =      m_words[i];
//          int b = that.m_words[i];
//          int s = a + b;
//
//          unsigned __int16 min = 0x0000;
//          unsigned __int16 max = 0xFFFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_words[i] = static_cast<unsigned __int16>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddWordsUnsignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_adds_epu16(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddDWords
//
//  Synopsis:
//      Add double words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] += that.m_dwords[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddDWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_add_epi32(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::AddQWords
//
//  Synopsis:
//      Add quadra words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 2; i++)
//      {
//          m_qwords[i] += that.m_qwords[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::AddQWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_add_epi64(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubBytes
//
//  Synopsis:
//      Subtract bytes in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          m_bytes[i] -= that.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubBytes(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_sub_epi8(m_data, that.m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubBytesSignedSaturate
//
//  Synopsis:
//      Subtract bytes with signed saturation.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          int a = static_cast<__int8>(     m_bytes[i]);
//          int b = static_cast<__int8>(that.m_bytes[i]);
//          int s = a - b;
//
//          __int8 min = 0x80;
//          __int8 max = 0x7F;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_bytes[i] = static_cast<__int8>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubBytesSignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_subs_epi8(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubBytesUnsignedSaturate
//
//  Synopsis:
//      Subtract bytes with unsigned saturation.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          int a =      m_bytes[i];
//          int b = that.m_bytes[i];
//          int s = a - b;
//
//          unsigned __int8 min = 0x00;
//          unsinged __int8 max = 0xFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_bytes[i] = s;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubBytesUnsignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_subs_epu8(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubWords
//
//  Synopsis:
//      Subtract words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] -= that.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_sub_epi16(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubWordsSignedSaturate
//
//  Synopsis:
//      Subtract words with signed saturation.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a = static_cast<__int16>(     m_words[i]);
//          int b = static_cast<__int16>(that.m_words[i]);
//          int s = a - b;
//
//          __int16 min = 0x8000;
//          __int16 max = 0x7FFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_words[i] = static_cast<__int16>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubWordsSignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_subs_epi16(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubWordsUnsignedSaturate
//
//  Synopsis:
//      Subtract words with unsigned saturation.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a =      m_words[i];
//          int b = that.m_words[i];
//          int s = a - b;
//
//          unsigned __int16 min = 0x0000;
//          unsigned __int16 max = 0xFFFF;
//
//          if (s < min) s = min;
//          if (s > max) s = max;
//
//          m_words[i] = static_cast<unsigned __int16>(s);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubWordsUnsignedSaturate(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_subs_epu16(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubDWords
//
//  Synopsis:
//      Subtract double words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] -= that.m_dwords[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubDWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_sub_epi32(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::SubQWords
//
//  Synopsis:
//      Subtract quadra words in wrapping mode.
//
//  Operation:
//      for (int i = 0; i < 2; i++)
//      {
//          m_qwords[i] -= that.m_qwords[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::SubQWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_sub_epi64(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::MulWords
//
//  Synopsis:
//      Multiply words; store low 16 bits of products.
//      Note that the resuld does not depend on whether
//      given data are treated as signed or unsigned values.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] *= that.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MulWords(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_mullo_epi16(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::MulWordsSignedHigh
//
//  Synopsis:
//      Multiply words as signed values; store high 16 bits of products.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a = static_cast<__int16>(     m_words[i]);
//          int b = static_cast<__int16>(that.m_words[i]);
//          int p = a*b;
//          m_qwords[i] = static_cast<__int16>(s >> 16);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MulWordsSignedHigh(__in_ecount(1) CXmmValue const &that)
{
    m_data = _mm_mulhi_epi16(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::MulWordsUnsignedHigh
//
//  Synopsis:
//      Multiply words as unsigned values; store high 16 bits of products.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          int a =      m_words[i];
//          int b = that.m_words[i];
//          int p = a*b;
//          m_qwords[i] = static_cast<unsigned __int16>(s >> 16);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MulWordsUnsignedHigh(__in_ecount(1) CXmmValue const & that)
{
    m_data = _mm_mulhi_epu16(m_data, that.m_data);
    return *this;
}




//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::CXmmValue::MulDWords
//
//  Synopsis:
//      Multiply double words; store low 32 bits of products.
//      Note that the resuld does not depend on whether
//      given data are treated as signed or unsigned values.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] *= that.m_dwords[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MulDWords(__in_ecount(1) CXmmValue const & that)
{
    m_data = _mm_mul_epu32(m_data, that.m_data);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftWordsLeft
//
//  Synopsis:
//      Left shift every word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] <<= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftWordsLeft(int shift)
{
    m_data = _mm_slli_epi16(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftWordsRightLogical
//
//  Synopsis:
//      Right shift every word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] >>= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftWordsRightLogical(int shift)
{
    m_data = _mm_srli_epi16(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftWordsRightArithmetic
//
//  Synopsis:
//      Right shift every word, extending with signs.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] = static_cast<__int16>(m_words[i]) >> shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftWordsRightArithmetic(int shift)
{
    m_data = _mm_srai_epi16(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftDWordsLeft
//
//  Synopsis:
//      Left shift every double word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] <<= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftDWordsLeft(int shift)
{
    m_data = _mm_slli_epi32(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftDWordsRightLogical
//
//  Synopsis:
//      Right shift every double word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] >>= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftDWordsRightLogical(int shift)
{
    m_data = _mm_srli_epi32(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftDWordsRightArithmetic
//
//  Synopsis:
//      Right shift every double word, extending with signs.
//
//  Operation:
//      for (int i = 0; i < 4; i++)
//      {
//          m_dwords[i] = static_cast<__int32>(m_dwords[i]) >> shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftDWordsRightArithmetic(int shift)
{
    m_data = _mm_srai_epi32(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftQWordsLeft
//
//  Synopsis:
//      Left shift every quadra word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 2; i++)
//      {
//          m_qwords[i] <<= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftQWordsLeft(int shift)
{
    m_data = _mm_slli_epi64(m_data, shift);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::ShiftQWordsRightLogical
//
//  Synopsis:
//      Right shift every quadra word, extending with zeros.
//
//  Operation:
//      for (int i = 0; i < 2; i++)
//      {
//          m_qwords[i] >>= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::ShiftQWordsRightLogical(int shift)
{
    m_data = _mm_srli_epi64(m_data, shift);
    return *this;
}



//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::MaxSignedWords
//
//  Synopsis:
//      Perform a SIMD compare of signed word integers.
//      Store maximums in this instance.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          __int16 a = static_cast<__int16>(     m_words[i]);
//          __int16 b = static_cast<__int16>(that.m_words[i]);
//          m_words[i] = max(a,b);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MaxSignedWords(__in_ecount(1) CXmmValue const & that)
{
    m_data = _mm_max_epi16(m_data, that.m_data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::MinSignedWords
//
//  Synopsis:
//      Perform a SIMD compare of signed word integers.
//      Store minimums in this instance.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          __int16 a = static_cast<__int16>(     m_words[i]);
//          __int16 b = static_cast<__int16>(that.m_words[i]);
//          m_words[i] = min(a,b);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmValue&
CXmmValue::MinSignedWords(__in_ecount(1) CXmmValue const & that)
{
    m_data = _mm_min_epi16(m_data, that.m_data);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::GetLowDWord
//
//  Synopsis:
//      Fetch low double word.
//
//  Operation:
//      return m_dword[0];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __int32
CXmmValue::GetLowDWord() const
{
    return _mm_cvtsi128_si32(m_data);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmValue::Zero
//
//  Synopsis:
//      Return the instance filled with zeros.
//
//  Operation:
//      CXmmValue result;
//      result.m_qwords[0] = m_qwords[1] = 0;
//      return result;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmValue
CXmmValue::Zero()
{
    return *reinterpret_cast<const CXmmValue*>(&sc_Zero);
}

// This might be faster way to get zero; for now we keep memory
// based Zero() as it came from Intel.
//MIL_FORCEINLINE CXmmValue
//CXmmValue::Zero()
//{
//    CXmmValue result;
//    result.m_data = _mm_xor_si128(result.m_data, result.m_data);
//    return result;
//}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmBytes::operator+
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          result.m_bytes[i] = m_bytes[i] + other.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmBytes
CXmmBytes::operator+(__in_ecount(1) CXmmBytes const & other) const
{
    CXmmBytes result(*this);
    result.AddBytes(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmBytes::operator+=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          m_bytes[i] += other.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmBytes&
CXmmBytes::operator+=(__in_ecount(1) CXmmBytes const & other)
{
    AddBytes(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmBytes::operator-
//
//  Synopsis:
//      Execute per-component subtraction.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          result.m_bytes[i] = m_bytes[i] - other.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmBytes
CXmmBytes::operator-(__in_ecount(1) CXmmBytes const & other) const
{
    CXmmBytes result(*this);
    result.SubBytes(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmBytes::operator-=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 16; i++)
//      {
//          m_bytes[i] -= other.m_bytes[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmBytes&
CXmmBytes::operator-=(__in_ecount(1) CXmmBytes const & other)
{
    SubBytes(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator+
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = m_words[i] + other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::operator+(__in_ecount(1) CXmmWords const & other) const
{
    CXmmWords result(*this);
    result.AddWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator+=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] += other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::operator+=(__in_ecount(1) CXmmWords const & other)
{
    AddWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator-
//
//  Synopsis:
//      Execute per-component subtraction.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = m_words[i] - other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::operator-(__in_ecount(1) CXmmWords const & other) const
{
    CXmmWords result(*this);
    result.SubWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator-=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] -= other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::operator-=(__in_ecount(1) CXmmWords const & other)
{
    SubWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator*
//
//  Synopsis:
//      Execute per-component multiplication.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = m_words[i] * other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::operator*(__in_ecount(1) CXmmWords const & other) const
{
    CXmmWords result(*this);
    result.MulWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator*=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] *= other.m_words[i];
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::operator*=(__in_ecount(1) CXmmWords const & other)
{
    MulWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator<<
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each word with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = m_words[i] << shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::operator<<(int shift) const
{
    CXmmWords result(*this);
    result.ShiftWordsLeft(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator<<=
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each word with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] <<= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::operator<<=(int shift)
{
    ShiftWordsLeft(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator>>
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each word with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = m_words[i] >> shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::operator>>(int shift) const
{
    CXmmWords result(*this);
    result.ShiftWordsRightLogical(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::operator>>=
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each word with zeros.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          m_words[i] >>= shift;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::operator>>=(int shift)
{
    ShiftWordsRightLogical(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::Max
//
//  Synopsis:
//      Perform a SIMD compare of signed word integers.
//      Store maximums in this instance.
//
//  Note:
//      All the words are treated as signed.
//      SSE2 has no support for unsigned word maxs and mins.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          __int16 a = static_cast<__int16>(     m_words[i]);
//          __int16 b = static_cast<__int16>(that.m_words[i]);
//          m_words[i] = max(a,b);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::Max(__in_ecount(1) CXmmWords const & that)
{
    MaxSignedWords(that);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::Min
//
//  Synopsis:
//      Perform a SIMD compare of signed word integers.
//      Store maximums in this instance.
//
//  Note:
//      All the words are treated as signed.
//      SSE2 has no support for unsigned word maxs and mins.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          __int16 a = static_cast<__int16>(     m_words[i]);
//          __int16 b = static_cast<__int16>(that.m_words[i]);
//          m_words[i] = min(a,b);
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmWords&
CXmmWords::Min(__in_ecount(1) CXmmWords const & that)
{
    MinSignedWords(that);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmWords::Half8dot8
//
//  Synopsis:
//      Return the instance filled with 0x0080 in each word.
//      This value means 1/2 assuming fixed 8.8 word format.
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.m_words[i] = 0x0080;
//      }
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
CXmmWords::Half8dot8()
{
    return *reinterpret_cast<const CXmmWords*>(&sc_Half8dot8);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator+
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      result.m_dwords[0] = m_dwords[0] + other.m_dwords[0];
//      result.m_dwords[1] = m_dwords[1] + other.m_dwords[1];
//      result.m_dwords[2] = m_dwords[2] + other.m_dwords[2];
//      result.m_dwords[3] = m_dwords[3] + other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmDWords
CXmmDWords::operator+(__in_ecount(1) CXmmDWords const & other) const
{
    CXmmDWords result(*this);
    result.AddDWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator+=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      m_dwords[0] += other.m_dwords[0];
//      m_dwords[1] += other.m_dwords[1];
//      m_dwords[2] += other.m_dwords[2];
//      m_dwords[3] += other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmDWords&
CXmmDWords::operator+=(__in_ecount(1) CXmmDWords const & other)
{
    AddDWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator-
//
//  Synopsis:
//      Execute per-component subtraction.
//
//  Operation:
//      result.m_dwords[0] = m_dwords[0] - other.m_dwords[0];
//      result.m_dwords[1] = m_dwords[1] - other.m_dwords[1];
//      result.m_dwords[2] = m_dwords[2] - other.m_dwords[2];
//      result.m_dwords[3] = m_dwords[3] - other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmDWords
CXmmDWords::operator-(__in_ecount(1) CXmmDWords const & other) const
{
    CXmmDWords result(*this);
    result.SubDWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator-=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      m_dwords[0] -= other.m_dwords[0];
//      m_dwords[1] -= other.m_dwords[1];
//      m_dwords[2] -= other.m_dwords[2];
//      m_dwords[3] -= other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmDWords&
CXmmDWords::operator-=(__in_ecount(1) CXmmDWords const & other)
{
    SubDWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator*
//
//  Synopsis:
//      Execute per-component multiplication.
//
//  Operation:
//      result.m_dwords[0] = m_dwords[0] * other.m_dwords[0];
//      result.m_dwords[1] = m_dwords[1] * other.m_dwords[1];
//      result.m_dwords[2] = m_dwords[2] * other.m_dwords[2];
//      result.m_dwords[3] = m_dwords[3] * other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmDWords
CXmmDWords::operator*(__in_ecount(1) CXmmDWords const & other) const
{
    CXmmDWords result(*this);
    result.MulDWords(other);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator*=
//
//  Synopsis:
//      Execute per-component addition.
//
//  Operation:
//      m_dwords[0] *= other.m_dwords[0];
//      m_dwords[1] *= other.m_dwords[1];
//      m_dwords[2] *= other.m_dwords[2];
//      m_dwords[3] *= other.m_dwords[3];
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmDWords&
CXmmDWords::operator*=(__in_ecount(1) CXmmDWords const & other)
{
    MulDWords(other);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator<<
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each dword with zeros.
//
//  Operation:
//      result.m_dwords[0] = m_dwords[0] << shift;
//      result.m_dwords[1] = m_dwords[1] << shift;
//      result.m_dwords[2] = m_dwords[2] << shift;
//      result.m_dwords[3] = m_dwords[3] << shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmDWords
CXmmDWords::operator<<(int shift) const
{
    CXmmDWords result(*this);
    result.ShiftDWordsLeft(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator<<=
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each dword with zeros.
//
//  Operation:
//      m_dwords[0] <<= shift;
//      m_dwords[1] <<= shift;
//      m_dwords[2] <<= shift;
//      m_dwords[3] <<= shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmDWords&
CXmmDWords::operator<<=(int shift)
{
    ShiftDWordsLeft(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator>>
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each dword with zeros.
//
//  Operation:
//      result.m_dwords[0] = m_dwords[0] >> shift;
//      result.m_dwords[1] = m_dwords[1] >> shift;
//      result.m_dwords[2] = m_dwords[2] >> shift;
//      result.m_dwords[3] = m_dwords[3] >> shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmDWords
CXmmDWords::operator>>(int shift) const
{
    CXmmDWords result(*this);
    result.ShiftDWordsRightLogical(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmDWords::operator>>=
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each dword with zeros.
//
//  Operation:
//      m_dwords[0] >>= shift;
//      m_dwords[1] >>= shift;
//      m_dwords[2] >>= shift;
//      m_dwords[3] >>= shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmDWords&
CXmmDWords::operator>>=(int shift)
{
    ShiftDWordsRightLogical(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmQWords::operator<<
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each dword with zeros.
//
//  Operation:
//      result.m_qwords[0] = m_qwords[0] << shift;
//      result.m_qwords[1] = m_qwords[1] << shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmQWords
CXmmQWords::operator<<(int shift) const
{
    CXmmQWords result(*this);
    result.ShiftQWordsLeft(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmQWords::operator<<=
//
//  Synopsis:
//      Execute per-component left shift.
//      Fill low bits of each dword with zeros.
//
//  Operation:
//      m_qwords[0] <<= shift;
//      m_qwords[1] <<= shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmQWords&
CXmmQWords::operator<<=(int shift)
{
    ShiftQWordsLeft(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmQWords::operator>>
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each dword with zeros.
//
//  Operation:
//      result.m_qwords[0] = m_qwords[0] >> shift;
//      result.m_qwords[1] = m_qwords[1] >> shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmQWords
CXmmQWords::operator>>(int shift) const
{
    CXmmQWords result(*this);
    result.ShiftQWordsRightLogical(shift);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmQWords::operator>>=
//
//  Synopsis:
//      Execute per-component right shift.
//      Fill high bits of each dword with zeros.
//
//  Operation:
//      m_qwords[0] >>= shift;
//      m_qwords[1] >>= shift;
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmQWords&
CXmmQWords::operator>>=(int shift)
{
    ShiftQWordsRightLogical(shift);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::CXmmFloat
//
//  Synopsis:
//      Construct CXmmFloat of another CXmmFloat
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
CXmmFloat::CXmmFloat(__in_ecount(1) CXmmFloat const & other)
{
    m_dataR = other.m_dataR;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::CXmmFloat
//
//  Synopsis:
//      Construct CXmmFloat from given __m128 value
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
CXmmFloat::CXmmFloat(__in_ecount(1) __m128 const& data)
{
    m_dataR = data;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::CXmmFloat
//
//  Synopsis:
//      Construct CXmmFloat from given 32-bits floating point value.
//
//  Operation:
//      m_floats[0] = data;
//      m_floats[1, 2, 3]: undefined
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
CXmmFloat::CXmmFloat(float data)
{
    // _mm_load_ss keeps three higher floats in destination unchanged;
    // _mm_set_ss fills them with zeros.
    m_dataR = _mm_load_ss(&data);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::CXmmFloat
//
//  Synopsis:
//      Construct CXmmFloat from given 32-bits integer value.
//      
//  Operation:
//      m_floats[0] = static_cast<float>(data);
//      m_floats[1, 2, 3]: undefined
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
CXmmFloat::CXmmFloat(int data)
{
    m_dataR = _mm_cvt_si2ss(m_dataR, data);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator=
//
//  Synopsis:
//      Assign 32-bits floating point value.
//
//  Operation:
//      m_floats[0] = data;
//      m_floats[1, 2, 3]: unchanged
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator=(float data)
{
    m_dataR = _mm_load_ss(&data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator=
//
//  Synopsis:
//      Assign 32-bits integer value.
//
//  Operation:
//      m_floats[0] = static_cast<float>(data);
//      m_floats[1, 2, 3]: unchanged
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator=(int data)
{
    m_dataR = _mm_cvt_si2ss(m_dataR, data);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Round
//
//  Synopsis:
//      Convert m_floats[0] to closest integer value, depending on current
//      SSE rounding mode. Default rounding mode is round-to-nearest, assuming
//      converting half-integers to closest even number.
//------------------------------------------------------------------------------
MIL_FORCEINLINE int
CXmmFloat::Round() const
{
    return _mm_cvt_ss2si(m_dataR);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Trunc
//
//  Synopsis:
//      Convert m_floats[0] to closest integer value that does not not exceed
//      it by absolute value. I.e. round-to-zero.
//      This method does not depend on rounding mode and does not infer expences
//      for switching rounding mode.
//
//  Operation:
//      return static_cast<int>(m_floats[0]);
//------------------------------------------------------------------------------
MIL_FORCEINLINE int
CXmmFloat::Trunc() const
{
    return _mm_cvtt_ss2si(m_dataR);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] + other.m_floats[0];
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator+(__in_ecount(1) CXmmFloat const & other) const
{
    CXmmFloat result;
    result.m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] + data;
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator+(float data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] + static_cast<float>(data);
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator+(int data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return result;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+=
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      m_floats[0] += other.m_floats[0];
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator+=(__in_ecount(1) CXmmFloat const & other)
{
    m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+=
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      m_floats[0] += data;
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator+=(float data)
{
    CXmmFloat other(data);
    m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator+=
//
//  Synopsis:
//      Scalar float addition.
//
//  Operation:
//      m_floats[0] += static_cast<float>(data);
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator+=(int data)
{
    CXmmFloat other(data);
    m_dataR = _mm_add_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] - other.m_floats[0];
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator-(__in_ecount(1) CXmmFloat const & other) const
{
    CXmmFloat result;
    result.m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] - data;
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator-(float data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] - static_cast<float>(data);
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator-(int data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return result;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-=
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      m_floats[0] -= other.m_floats[0];
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator-=(__in_ecount(1) CXmmFloat const & other)
{
    m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-=
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      m_floats[0] -= data;
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator-=(float data)
{
    CXmmFloat other(data);
    m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator-=
//
//  Synopsis:
//      Scalar float subtraction.
//
//  Operation:
//      m_floats[0] -= static_cast<float>(data);
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator-=(int data)
{
    CXmmFloat other(data);
    m_dataR = _mm_sub_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] * other.m_floats[0];
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator*(__in_ecount(1) CXmmFloat const & other) const
{
    CXmmFloat result;
    result.m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] * data;
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator*(float data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] * static_cast<float>(data);
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator*(int data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*=
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      m_floats[0] *= other.m_floats[0];
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator*=(__in_ecount(1) CXmmFloat const & other)
{
    m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*=
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      m_floats[0] *= data;
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator*=(float data)
{
    CXmmFloat other(data);
    m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator*=
//
//  Synopsis:
//      Scalar float multiplication.
//
//  Operation:
//      m_floats[0] *= static_cast<float>(data);
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator*=(int data)
{
    CXmmFloat other(data);
    m_dataR = _mm_mul_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] / other.m_floats[0];
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator/(__in_ecount(1) CXmmFloat const & other) const
{
    CXmmFloat result;
    result.m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] / data;
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator/(float data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      result.m_floats[0] = this.m_floats[0] / static_cast<float>(data);
//      result.m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::operator/(int data) const
{
    CXmmFloat result;
    CXmmFloat other(data);
    result.m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/=
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      m_floats[0] /= other.m_floats[0];
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator/=(__in_ecount(1) CXmmFloat const & other)
{
    m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/=
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      m_floats[0] /= data;
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator/=(float data)
{
    CXmmFloat other(data);
    m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::operator/=
//
//  Synopsis:
//      Scalar float division.
//
//  Operation:
//      m_floats[0] /= static_cast<float>(data);
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE __out_ecount(1) CXmmFloat&
CXmmFloat::operator/=(int data)
{
    CXmmFloat other(data);
    m_dataR = _mm_div_ss(m_dataR, other.m_dataR);
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Reciprocal
//
//  Synopsis:
//      Scalar calculate reciprocal value.
//
//  Operation:
//      m_floats[0] = 1.0f/m_floats[0];
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::Reciprocal(__in_ecount(1) CXmmFloat const & data)
{
    CXmmFloat result;
    result.m_dataR = _mm_rcp_ss(data.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Sqrt
//
//  Synopsis:
//      Scalar square root.
//
//  Operation:
//      m_floats[0] = sqrtf(m_floats[0]);
//      m_floats[1, 2, 3]: unchanged.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::Sqrt(__in_ecount(1) CXmmFloat const & data)
{
    CXmmFloat result;
    result.m_dataR = _mm_sqrt_ss(data.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Min
//
//  Synopsis:
//      Scalar calculate minimum of two floats.
//
//  Operation:
//      result.m_floats[0] = min(data1.m_floats[0], data2.m_floats[0]);
//      m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::Min(__in_ecount(1) CXmmFloat const & data1, __in_ecount(1) CXmmFloat const & data2)
{
    CXmmFloat result;
    result.m_dataR = _mm_min_ss(data1.m_dataR, data2.m_dataR);
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CXmmFloat::Max
//
//  Synopsis:
//      Scalar calculate maximum of two floats.
//
//  Operation:
//      result.m_floats[0] = max(data1.m_floats[0], data2.m_floats[0]);
//      m_floats[1, 2, 3]: undefined.
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmFloat
CXmmFloat::Max(__in_ecount(1) CXmmFloat const & data1, __in_ecount(1) CXmmFloat const & data2)
{
    CXmmFloat result;
    result.m_dataR = _mm_max_ss(data1.m_dataR, data2.m_dataR);
    return result;
}

#endif //_BUILD_SSE_

