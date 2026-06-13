// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definition of a specific specialization of the CMultiSpaceRectF class.
//
//  Usage:
//      This file should be inlined to define a specific specialization of the
//      CMultiSpaceRectF class.  Just before the #include two macros must be
//      defined.  CoordSpace1 and CoordSpace2.  They should be names of
//      CoordinateSpace types, but not fully qualified with CoordinateSpace::.
//
//      For example:

//          #define CoordSpace1 DeviceHPC
//          #define CoordSpace2 DeviceIPC
//          #include "MultiSpaceRectF.inl"
//        will define
//          CMultiSpaceRectF<CoordinateSpace::DeviceHPC,CoordinateSpace::DeviceIPC>
//        with accessor methods
//          DeviceHPC() and DeviceIPC()

//
//  Notes:
//      CoordSpace1 and CoordSpace2 are automatically undefined at the end of
//      this file.
//
//      When errors are found in this file often the including file will need
//      to be checked.  An error such as:
//      multispacerectf.inl(33) : error C2039: 'Device2' : is not a member of 'CoordinateSpace'
//      is likely to be an error in including file.
//
//-----------------------------------------------------------------------------

#ifndef CoordSpace1
    #error CoordSpace1 not defined before including this file.
#endif
#ifndef CoordSpace2
    #error CoordSpace2 not defined before including this file.
#endif

//+----------------------------------------------------------------------------
//
//  Class:
//      CMultiSpaceRectF<CoordSpace1,CoordSpace2>
//
//  Synopsis:
//      CoordinateSpace annotated rectangle class capable of being in any one
//      to two CoordinateSpaces over its lifetime.  Verification of proper
//      coordinate space out of those two is only check at runtime.  Use is
//      restricted to either of the two coordinate spaces at compile time.
//
//-----------------------------------------------------------------------------

template <>
class CMultiSpaceRectF<CoordinateSpace::CoordSpace1, CoordinateSpace::CoordSpace2>
{
    typedef CMilRectF BaseMILRect;
    typedef CRectF<CoordinateSpace::CoordSpace1> CRectSpace1;
    typedef CRectF<CoordinateSpace::CoordSpace2> CRectSpace2;

public:

    CMultiSpaceRectF()
    WHEN_DBG_ANALYSIS(: m_currentSpace(CoordinateSpaceId::Invalid))
    {
    }

    CRectSpace1 const &CoordSpace1() const
    {
        Assert(m_currentSpace == CoordinateSpace::CoordSpace1::Id);
        return *CRectSpace1::ReinterpretBaseType(&m_rc.CoordSpace1);
    };

    CRectSpace1 &CoordSpace1()
    {
        WHEN_DBG_ANALYSIS(m_currentSpace = CoordinateSpace::CoordSpace1::Id);
        return *CRectSpace1::ReinterpretBaseType(&m_rc.CoordSpace1);
    };

    CRectSpace1 const & operator=(CRectSpace1 const &rc)
    {
        WHEN_DBG_ANALYSIS(m_currentSpace = CoordinateSpace::CoordSpace1::Id);
        return (*CRectSpace1::ReinterpretBaseType(&m_rc.CoordSpace1) = rc);
    };


    CRectSpace2 const &CoordSpace2() const
    {
        Assert(m_currentSpace == CoordinateSpace::CoordSpace2::Id);
        return *CRectSpace2::ReinterpretBaseType(&m_rc.CoordSpace2);
    };

    CRectSpace2 &CoordSpace2()
    {
        WHEN_DBG_ANALYSIS(m_currentSpace = CoordinateSpace::CoordSpace2::Id);
        return *CRectSpace2::ReinterpretBaseType(&m_rc.CoordSpace2);
    };


    //+------------------------------------------------------------------------
    //
    //  Members:
    //      AnySpace
    //
    //  Synopsis:
    //      CoordinateSpace agnostic accessors that should have limited use.
    //
    //-------------------------------------------------------------------------

    BaseMILRect const &AnySpace() const
    {
        Assert(m_currentSpace != CoordinateSpaceId::Invalid);
        return *CRectSpace1::ReinterpretBaseType(&m_rc.CoordSpace1);
    }

    BaseMILRect &AnySpace()
    {
        Assert(m_currentSpace != CoordinateSpaceId::Invalid);
        return *CRectSpace1::ReinterpretBaseType(&m_rc.CoordSpace1);
    }

#if DBG_ANALYSIS
    CoordinateSpaceId::Enum DbgCurrentCoordSpace() const
    {
        return m_currentSpace;
    }
#endif

private:

    union {
        TRect_<BaseMILRect::BaseRectType, CoordinateSpace::CoordSpace1> CoordSpace1;
        TRect_<BaseMILRect::BaseRectType, CoordinateSpace::CoordSpace2> CoordSpace2;
    } m_rc;

    WHEN_DBG_ANALYSIS(CoordinateSpaceId::Enum m_currentSpace);
};

#undef CoordSpace1
#undef CoordSpace2



