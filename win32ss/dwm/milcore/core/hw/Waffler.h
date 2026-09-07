// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains class declarations for triangle waffler.
//

//+----------------------------------------------------------------------------
//
//  Class:     PointXYA
//
//  Synopsis:  Floating point X, Y, A(lpha)
//
//-----------------------------------------------------------------------------
struct PointXYA
{
    float x,y,a;
};

//+----------------------------------------------------------------------------
//
//  Class:     ITriangleSink<T>
//
//  Synopsis:  Type specific interface for something with method that consumes
//             three T's and makes a triangle out of them.
//
//-----------------------------------------------------------------------------
template<typename T>
class ITriangleSink
{
public:
    virtual HRESULT AddTriangle(
        __in_ecount(1) const T &v0_,
        __in_ecount(1) const T &v1_,
        __in_ecount(1) const T &v2_
        ) = 0;
};

//+----------------------------------------------------------------------------
//
//  Class:      TriangleWaffler<T>
//
//  Implements: ITriangleSink<T>
//
//  Synopsis:   Given a 1D partition into cells defined by the integral values
//              of the equation ax + by + c this triangle sink takes
//              triangles and retriangulates them into new triangles
//              which each lie in only one cell of the partition.  It
//              them sends them to its member sink.
//
//  Inputs required:
//    - A,B,C defining partitions and an output ITriangleSink
//

template<typename T>
class TriangleWaffler : public ITriangleSink<T>
{
public:
    TriangleWaffler()
    {
    }

    typedef ITriangleSink<T> ISink;
    
    void Set(float aa, float bb, float cc, __in_ecount(1) ITriangleSink<T> *tc)
    {
        m_a = aa;
        m_b = bb;
        
        //
        // cc may be very large (if, say, the brush transform has a large
        // translation) and may cause numerical overflow later on down the line.
        // Since two waffles with identical a and b components are equivalent
        // iff the fractional parts of their c's are equal, we can safely store
        // only the fractional part of c (note that if cc is negative, m_c will
        // be, too).
        //
        // Note: We are forced to do this casting since XP IA64 didn't export
        // modff in msvcrt.dll
        //
        double fDummy;
        m_c = static_cast<float>(modf(static_cast<double>(cc), &fDummy));
        m_consumer = tc;
    }
    
    void SetSink(__in_ecount(1) ITriangleSink<T> *tc)
    {
        m_consumer = tc;
    }
    
    HRESULT AddTriangle(
        __in_ecount(1) const T &v0,
        __in_ecount(1) const T &v1,
        __in_ecount(1) const T &v2
        );

private:
    HRESULT SendTriangle(
        __in_ecount(1) const T &v0, 
        __in_ecount(1) const T &v1, 
        __in_ecount(1) const T &v2
        );
    
    HRESULT SendQuad(
        __in_ecount(1) const T &v0, 
        __in_ecount(1) const T &v1, 
        __in_ecount(1) const T &v2, 
        __in_ecount(1) const T &v3
        );
    
    HRESULT SendPent(
        __in_ecount(1) const T &v0, 
        __in_ecount(1) const T &v1, 
        __in_ecount(1) const T &v2, 
        __in_ecount(1) const T &v3, 
        __in_ecount(1) const T &v4
        );

    // This defines a 1D subdivision using the lines given by
    // m_a * x + m_b * y + m_c = I for integer I.
    float m_a, m_b, m_c;

    // Output
    ITriangleSink<T> *m_consumer;
};

//+----------------------------------------------------------------------------
//
//  Class:     ILineSink<T>
//
//  Synopsis:  Type specific interface for something with method that consumes
//             two T's and makes a line out of them.
//
//-----------------------------------------------------------------------------
template<typename T>
class ILineSink
{
public:
    virtual HRESULT AddLine(
        __in_ecount(1) const T &v0,
        __in_ecount(1) const T &v1
        ) = 0;
};

//+----------------------------------------------------------------------------
//
//  Class:      LineWaffler<T>
//
//  Implements: ILineSink<T>
//
//  Synopsis:   Given a 1D partition into cells defined by the integral values
//              of the equation ax + by + c this line sink takes
//              line segments and divides them so that each lies in only one
//              cell of the partition.  It them sends them to its member
//              sink.
//
//  Inputs required:
//    - A,B,C defining partitions and an output ILineSink
//

template<typename T>
class LineWaffler : public ILineSink<T>
{
public:
    LineWaffler()
    {
    }

    typedef ILineSink<T> ISink;
    
    void Set(float aa, float bb, float cc, __in_ecount(1) ILineSink<T> *tc)
    {
        m_a = aa;
        m_b = bb;

        //
        // cc may be very large (if, say, the brush transform has a large
        // translation) and may cause numerical overflow later on down the line.
        // Since two waffles with identical a and b components are equivalent
        // iff the fractional parts of their c's are equal, we can safely store
        // only the fractional part of c (note that if cc is negative, m_c will
        // be, too).
        //
        // Note: We are forced to do this casting since XP IA64 didn't export
        // modff in msvcrt.dll
        //
        double fDummy;
        m_c = static_cast<float>(modf(static_cast<double>(cc), &fDummy));
        m_consumer = tc;
    }
    
    void SetSink(__in_ecount(1) ILineSink<T> *tc)
    {
        m_consumer = tc;
    }
    
    HRESULT AddLine(__in_ecount(1) const T &v0, __in_ecount(1) const T &v1);

private:
    // This defines a 1D subdivision using the lines given by
    // m_a * x + m_b * y + m_c = I for integer I.
    float m_a, m_b, m_c;

    // Output
    ILineSink<T> *m_consumer;
};




